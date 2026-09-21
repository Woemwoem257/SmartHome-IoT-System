#include "uart_bridge.h"
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>
#include "aws_mqtt.h"

// 1. Kéo các thư viện mới vào
#include <ArduinoJson.h>
#include "hmi_manager.h" 

#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define TXD_PIN            17
#define RXD_PIN            16

const char* UartBridge::TAG = "UART_BRIDGE";
const int UartBridge::RX_BUF_SIZE = 1024;
static QueueHandle_t uart_queue;

// ==============================================================================
// BỘ ĐỊNH TUYẾN DỮ LIỆU (ROUTER)
// ==============================================================================
static void process_json_packet(const char* json_str) {
    StaticJsonDocument<256> doc; 
    DeserializationError error = deserializeJson(doc, json_str);
    if (error) {
        ESP_LOGE("ROUTER", "Loi parse JSON: %s - Payload: %s", error.c_str(), json_str);
        return;
    }

    // --- NHÁNH 1: DỮ LIỆU VIỄN TRẮC (SENSOR) ---
    if (doc.containsKey("temperature") && doc.containsKey("humidity")) {
        float temp = doc["temperature"];
        float hum = doc["humidity"];
        HmiManager::update_sensor_data(temp, hum);
        AwsMqtt::publish("gateway/sensor/data", json_str);
        ESP_LOGI("ROUTER", "Da cap nhat UI & Cloud -> Temp: %.1fC, Hum: %.1f%%", temp, hum);
    } 
    
    // --- NHÁNH 2: PHẢN HỒI THIẾT BỊ (ACTUATOR ACK) ---
const char* keys[] = {"relay1", "relay2", "mosfet1", "mosfet2", "alarm_clear"};
DeviceID_t ids[] = {DEV_RELAY_1, DEV_RELAY_2, DEV_MOSFET_1, DEV_MOSFET_2, DEV_ALARM_CLEAR};
int num_devices = sizeof(keys) / sizeof(keys[0]);

bool has_ack = false; // Bổ sung cờ kiểm soát

for (int i = 0; i < num_devices; i++) {
    if (doc.containsKey(keys[i])) {
        int state = doc[keys[i]];
        
        // Ép giao diện UI cập nhật
        HmiManager::update_actuator_state(ids[i], state == 1);
        has_ack = true; // Đánh dấu có lệnh hợp lệ
        
        ESP_LOGI("ROUTER", "Nhan ACK tu STM32 [%s]: %d", keys[i], state);
    }
}

// CHỈ PUBLISH 1 LẦN DUY NHẤT NGOÀI VÒNG LẶP
    if (has_ack) {
        AwsMqtt::publish("gateway/control/ack", json_str);
    }
}


// ==============================================================================
// KHỞI TẠO UART
// ==============================================================================
void UartBridge::init() {
    uart_config_t uart_config = {};
    uart_config.baud_rate = UART_BAUD_RATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;

    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 20, &uart_queue, 0));

    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "Hardware UART Bridge Initialized on TX:%d RX:%d", TXD_PIN, RXD_PIN);
}

// ==============================================================================
// LUỒNG NHẬN DỮ LIỆU & BỘ ĐỆM TRƯỢT (STREAM TOKENIZER)
// ==============================================================================
void UartBridge::rx_task(void* arg) {
    uart_event_t event;
    static uint8_t data[RX_BUF_SIZE];
    
    // Tạo bộ đệm dòng tĩnh (Line Buffer) để hứng từng ký tự
    static char line_buffer[256]; 
    static uint16_t line_pos = 0;
    
    while (1) {
        // ĐÓNG BĂNG TASK (0% CPU): Chỉ thức dậy khi Trình phục vụ ngắt (ISR) báo có event
        if (xQueueReceive(uart_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            switch (event.type) {
                // Sự kiện: Có dữ liệu mới tràn vào thanh ghi
                case UART_DATA:
                    // Đọc chính xác số byte phần cứng vừa báo (event.size)
                    uart_read_bytes(UART_PORT_NUM, data, event.size, portMAX_DELAY);
                    
                    // Phân tích Frame JSON dựa trên ký tự kết thúc dòng (CR/LF)
                    for (int i = 0; i < event.size; i++) {
                        char c = (char)data[i];
                        if (c == '\n' || c == '\r') {
                            if (line_pos > 0) {
                                line_buffer[line_pos] = '\0'; 
                                process_json_packet(line_buffer); // Đẩy vào luồng xử lý
                                line_pos = 0; // Reset để hứng bản tin sau
                            }
                        } 
                        else if (line_pos < sizeof(line_buffer) - 1) {
                            line_buffer[line_pos++] = c;
                        }
                    }
                    break;
                    
                // Sự kiện: Tràn bộ đệm FIFO phần cứng (Bảo vệ bộ nhớ)
                case UART_FIFO_OVF:
                    ESP_LOGW(TAG, "Loi: HW FIFO Overflow - Giam toc do truyen STM32 xuong");
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(uart_queue);
                    break;
                    
                // Sự kiện: Tràn bộ đệm Ring Buffer phần mềm (Bảo vệ bộ nhớ)
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "Loi: Ring Buffer Full - He thong dang qua tai");
                    uart_flush_input(UART_PORT_NUM);
                    xQueueReset(uart_queue);
                    break;
                    
                // Bỏ qua các sự kiện không quan trọng khác (Break, Parity err...)
                default:
                    break;
            }
        }
    }
}

void UartBridge::send_command(const std::string& cmd) {
    uart_write_bytes(UART_PORT_NUM, cmd.c_str(), cmd.length());
    ESP_LOGI(TAG, "Gui xuong STM32: %s", cmd.c_str());
}