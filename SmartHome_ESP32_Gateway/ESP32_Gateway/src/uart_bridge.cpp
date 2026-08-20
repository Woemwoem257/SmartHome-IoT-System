#include "uart_bridge.h"
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>
#include "aws_mqtt.h"
#include <cJSON.h>

// Cấu hình Hardware Map
#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define TXD_PIN            17  // Nối với chân RX của STM32
#define RXD_PIN            16  // Nối với chân TX của STM32

const char* UartBridge::TAG = "UART_BRIDGE";
const int UartBridge::RX_BUF_SIZE = 1024;

void UartBridge::init() {
    uart_config_t uart_config = {};
    uart_config.baud_rate = UART_BAUD_RATE;
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity    = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_APB;

    // 1. Nạp cấu hình
    ESP_ERROR_CHECK(uart_param_config(UART_PORT_NUM, &uart_config));
    
    // 2. Thiết lập chân I/O
    ESP_ERROR_CHECK(uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    // 3. Cài đặt Driver (Khai báo Buffer RX/TX)
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));

    // 4. Khởi tạo RTOS Task để nhận dữ liệu (Gắn độ ưu tiên cao: 5)
    xTaskCreate(rx_task, "uart_rx_task", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "Hardware UART Bridge Initialized on TX:%d RX:%d", TXD_PIN, RXD_PIN);
}

void UartBridge::rx_task(void* arg) {
    // Cấp phát tĩnh 1 lần tại Startup, miễn nhiễm với lỗi phân mảnh Heap
    static uint8_t data[RX_BUF_SIZE];
    
    while (1) {
        int rxBytes = uart_read_bytes(UART_PORT_NUM, data, RX_BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        
        if (rxBytes > 0) {
            // 1. KIỂM TRA PHÂN MẢNH NGAY LẬP TỨC TRƯỚC KHI LÀM BẤT CỨ VIỆC GÌ KHÁC
            if(rxBytes == RX_BUF_SIZE - 1){
                ESP_LOGW(TAG, "Canh bao: UART payload cham nguong (%d bytes). Tien hanh don rac DMA.", rxBytes);
                // Xả toàn bộ dữ liệu thừa đang kẹt trong bộ đệm cứng của UART 
                uart_flush_input(UART_PORT_NUM);
                // Bỏ qua mảnh vỡ này vì nó không còn nguyên vẹn
                continue; 
            }

            // 2. TRIM DỮ LIỆU THỪA TỪ STM32
            while (rxBytes > 0 && (data[rxBytes - 1] == '\r' || data[rxBytes - 1] == '\n')){
                rxBytes--;
            }
            
            // 3. CHỐT CHUỖI NULL-TERMINATOR
            data[rxBytes] = '\0';

            // 4. KIỂM TRA HỢP LỆ VÀ ĐỊNH TUYẾN
            cJSON *json = cJSON_Parse((char*)data);
            if (json != NULL){
                ESP_LOGI(TAG, "Nhan tu STM32: %s", (char*)data);
                AwsMqtt::publish("gateway/sensor/data", (char*)data);
                cJSON_Delete(json); // Dọn dẹp Heap an toàn
            } else {
                ESP_LOGW(TAG, "Invalid JSON from STM32: %s", (char*)data);
            }
        }
    }
}

void UartBridge::send_command(const std::string& cmd) {
    uart_write_bytes(UART_PORT_NUM, cmd.c_str(), cmd.length());
    ESP_LOGI(TAG, "Gui xuong STM32: %s", cmd.c_str());
}