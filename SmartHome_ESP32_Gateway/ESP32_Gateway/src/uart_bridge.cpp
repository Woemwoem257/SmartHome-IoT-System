#include "uart_bridge.h"
#include <driver/uart.h>
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <string.h>
#include "aws_mqtt.h"

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
    // Cấp phát vùng nhớ đệm trên Heap
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE);
    
    while (1) {
        // Hàm này sẽ tự động block task và nhường CPU cho đến khi có dữ liệu đến
        // Timeout 100ms (portTICK_PERIOD_MS)
        int rxBytes = uart_read_bytes(UART_PORT_NUM, data, RX_BUF_SIZE - 1, 100 / portTICK_PERIOD_MS);
        
        if (rxBytes > 0) {
            // Chốt chuỗi (Null-terminator) để in ra màn hình hoặc xử lý JSON
            data[rxBytes] = '\0';
            ESP_LOGI(TAG, "Nhan tu STM32: %s", (char*)data);
            
            // TƯƠNG LAI: Tại vị trí này, chúng ta sẽ Push chuỗi này vào một Queue.
            // MqttClient Task sẽ nằm chờ ở đầu kia Queue, bốc chuỗi này ra và Publish lên AWS IoT.
            // --- VÁ LỖ HỔNG UP-LINK ---
            // Gọi hàm publish của class AwsMqtt để đẩy dữ liệu lên Cloud
            // Đảm bảo class AwsMqtt của bạn có hàm public: static void publish(const char* topic, const char* payload);
            AwsMqtt::publish("gateway/sensor/data", (char*)data);
        }
    }
    free(data);
    vTaskDelete(NULL);
}

void UartBridge::send_command(const std::string& cmd) {
    uart_write_bytes(UART_PORT_NUM, cmd.c_str(), cmd.length());
    ESP_LOGI(TAG, "Gui xuong STM32: %s", cmd.c_str());
}