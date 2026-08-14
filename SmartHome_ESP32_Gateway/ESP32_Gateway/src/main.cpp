#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "wifi_manager.h"
#include "uart_bridge.h"

const char *TAG = "app_main";

extern "C" void app_main() 
{ 
    ESP_LOGI(TAG, "Middleware Gateway Started"); 

    // 1. Kích hoạt kết nối Wi-Fi 
    WiFiManager::init();

    // 2. Kích hoạt cầu nối UART (Tự động sinh ra rx_task chạy ngầm)
    UartBridge::init();

    // (Test) Giả lập gửi một lệnh JSON xuống STM32 sau 5 giây
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    UartBridge::send_command("{\"relay1\": 1, \"relay2\": 0}\r\n");
}