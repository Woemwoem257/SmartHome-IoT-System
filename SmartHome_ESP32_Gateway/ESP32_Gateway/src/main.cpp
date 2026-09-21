#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <esp_log.h>

#include "board_config.h"
#include "uart_bridge.h"
#include "app_tasks.h"

const char *TAG = "app_main";

// Khởi tạo các object phần cứng vật lý
TFT_eSPI tft = TFT_eSPI(); 
SPIClass touchSPI(HSPI); 
XPT2046_Touchscreen ts(Board::TouchCsPin, Board::TouchIrqPin);

extern "C" void app_main() 
{ 
    initArduino();
    ESP_LOGI(TAG, "Middleware Gateway Started"); 
    
    // 1. KHỞI TẠO TẦNG CƠ BẢN
    UartBridge::init();

    ESP_LOGI(TAG, "Cho 1 giay de on dinh dien ap boot...");
    vTaskDelay(pdMS_TO_TICKS(1000)); 

    // 2. KHỞI TẠO TẦNG ĐỒ HỌA (Hardware Level)
    tft.init();
    tft.setRotation(2);
    tft.invertDisplay(true); 
    tft.fillScreen(TFT_BLACK);

    touchSPI.begin(Board::SpiSclkPin, Board::SpiMisoPin, Board::SpiMosiPin, Board::TouchCsPin);
    ts.begin(touchSPI);
    ts.setRotation(2);

    ESP_LOGI(TAG, "Man hinh da bat. Cho them 2 giay de Tu dien nap day...");
    vTaskDelay(pdMS_TO_TICKS(2000)); 

    // 3. BÀN GIAO CHO HỆ ĐIỀU HÀNH (RTOS Level)
    App_InitTasks();
}