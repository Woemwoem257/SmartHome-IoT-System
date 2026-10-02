#include <esp_log.h>
#include "uart_bridge.h"
#include <Arduino.h>
#include "device_control.h"
#include "wifi_manager.h"
#include "aws_mqtt.h"

const char *TAG = "app_main";

QueueHandle_t actuator_queue; // Định nghĩa biến thực thể cho Queue

extern "C" void app_main() 
{ 
    initArduino();
    ESP_LOGI(TAG, "Middleware Headless Gateway Started"); 
    
    actuator_queue = xQueueCreate(10, sizeof(ControlMsg_t));
    if (actuator_queue == NULL) {
        ESP_LOGE(TAG, "Loi cap phat Queue!");
    } else {
        // KHỞI CHẠY TÁC VỤ GỬI UART NẾU TẠO QUEUE THÀNH CÔNG
        xTaskCreate(actuator_tx_task, "actuator_tx_task", 4096, NULL, 5, NULL);
    }

    // 1. KHỞI TẠO TẦNG CƠ BẢN (Giao tiếp với STM32)
    UartBridge::init();

    // 2. KHỞI TẠO TẦNG WIFI
    WiFiManager::init();

    // TẠM DỪNG: Cho mạng Wi-Fi 5 giây để lấy IP từ Router
    ESP_LOGI(TAG, "Cho 5 giay de lay IP...");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // 3. ĐỒNG BỘ THỜI GIAN THỰC 
    WiFiManager::sync_time();

    // 4. KHỞI TẠO KẾT NỐI TỚI AWS
    AwsMqtt::init();
}