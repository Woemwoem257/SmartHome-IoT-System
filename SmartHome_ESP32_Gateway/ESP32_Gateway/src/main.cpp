#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// Các thư viện Middleware cũ
#include "wifi_manager.h"
#include "uart_bridge.h"
#include "aws_mqtt.h"

// --- CẤU HÌNH HARDWARE PINS (TOUCH - HSPI) ---
#define TOUCH_CS   15
#define TOUCH_IRQ  27
#define TOUCH_SCLK 14
#define TOUCH_MISO 12
#define TOUCH_MOSI 13

const char *TAG = "app_main";

// --- KHỞI TẠO OBJECTS ---
TFT_eSPI tft = TFT_eSPI(); 
SPIClass touchSPI(HSPI); 
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

static TaskHandle_t HMITaskHandle = NULL;

// Task chuyên biệt để xử lý màn hình và cảm ứng (Tách biệt khỏi luồng mạng)
void HMI_Task(void *pvParameters) {
    while (1) {
        if (ts.touched()) {
            TS_Point p = ts.getPoint();
            // In log tọa độ nguyên bản để phục vụ Calibration sau này
            ESP_LOGI("HMI", "Touch -> X: %d, Y: %d", p.x, p.y);
            
            // Xử lý đồ họa tạm thời trước khi đưa LVGL vào
            long mapped_x = map(p.x, 200, 3800, 0, 240);
            long mapped_y = map(p.y, 200, 3800, 0, 320);
            tft.fillCircle(mapped_x, mapped_y, 2, TFT_RED);
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // Giải phóng CPU cho các Task mạng
    }
}

extern "C" void app_main() 
{ 
    // BẮT BUỘC: Khởi tạo lõi Arduino trong môi trường ESP-IDF
    initArduino();

    ESP_LOGI(TAG, "Middleware Gateway Started"); 

    // --- 1. KHỞI TẠO TẦNG HIỂN THỊ (VSPI & HSPI) ---
    ESP_LOGI(TAG, "Khoi tao man hinh ILI9341 va Cam ung...");

    // BỔ SUNG: Chờ IC màn hình ổn định nguồn trước khi bơm lệnh SPI
    vTaskDelay(pdMS_TO_TICKS(100));

    tft.init();
    tft.setRotation(0);
    
    // BỔ SUNG: Kích hoạt cờ đảo màu cấp độ phần cứng
    tft.invertDisplay(true);

    tft.fillScreen(TFT_BLACK);

    touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin(touchSPI);
    ts.setRotation(0);

    // Kích hoạt HMI Task chạy trên Core 1 (App Core)
    xTaskCreatePinnedToCore(HMI_Task, "HMI_Task", 4096, NULL, 5, &HMITaskHandle, 1);

    // --- 2. KHỞI TẠO TẦNG NETWORK & MIDDLEWARE ---
    // Kích hoạt kết nối Wi-Fi 
    WiFiManager::init();

    // Kích hoạt cầu nối UART (Tự động sinh ra rx_task chạy ngầm)
    UartBridge::init();

    // Khối Code đồng bộ thời gian
    ESP_LOGI(TAG, "Dang dong bo thoi gian tu NTP Server...");
    esp_sntp_setoperatingmode((esp_sntp_operatingmode_t)SNTP_OPMODE_POLL);
   
    // Phân bổ tải trọng truy vấn  cho 3 máy chủ mạnh nhất 
    esp_sntp_setservername(0, "time.google.com");      
    esp_sntp_setservername(1, "time.cloudflare.com");  
    esp_sntp_setservername(2, "vn.pool.ntp.org");

    esp_sntp_init();

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // Khóa chết luồng này cho đến khi NTP cập nhật năm > 2020 (Năm 1970 + 50)
    while (timeinfo.tm_year < (2020 - 1900)) {
        ESP_LOGW(TAG, "Thoi gian hien tai la nam 1970. Dang cho IP va NTP Sync...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    // In ra thời gian thực để xác nhận
    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "THOI GIAN DA DONG BO: %s", strftime_buf);
    
    // Kích hoạt mqtt 
    AwsMqtt::init();

    // (Test) Giả lập gửi một lệnh JSON xuống STM32 sau 5 giây
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    UartBridge::send_command("{\"relay1\": 1, \"relay2\": 0}\r\n");
}