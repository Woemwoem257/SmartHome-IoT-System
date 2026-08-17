#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "wifi_manager.h"
#include "uart_bridge.h"
#include "aws_mqtt.h"
#include <esp_sntp.h>

const char *TAG = "app_main";

extern "C" void app_main() 
{ 
    ESP_LOGI(TAG, "Middleware Gateway Started"); 

    // 1. Kích hoạt kết nối Wi-Fi 
    WiFiManager::init();

    // 2. Kích hoạt cầu nối UART (Tự động sinh ra rx_task chạy ngầm)
    UartBridge::init();

    // 3. Khối Code đồng bộ thời gian
    ESP_LOGI(TAG, "Dang dong bo thoi gian tu NTP Server...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
   
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
    
    // 4. Kích hoạt mqtt 
    AwsMqtt::init();

    // (Test) Giả lập gửi một lệnh JSON xuống STM32 sau 5 giây
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    UartBridge::send_command("{\"relay1\": 1, \"relay2\": 0}\r\n");
}