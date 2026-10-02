#include <esp_log.h>
#include "uart_bridge.h"
#include <Arduino.h>

const char *TAG = "app_main";

extern "C" void app_main() 
{ 
    initArduino();
    ESP_LOGI(TAG, "Middleware Gateway Started"); 
    
    // 1. KHỞI TẠO TẦNG CƠ BẢN
    UartBridge::init();
}