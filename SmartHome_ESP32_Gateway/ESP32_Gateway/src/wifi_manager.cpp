#include "wifi_manager.h" // Dùng ngoặc kép cho file cục bộ
#include <string.h>       // Thư viện cần thiết cho lệnh strcpy

#define WIFI_SSID "realme 10"
#define WIFI_PASS "woemwoem"
#define MAXIMUM_RETRY 5

// 1. Khởi tạo các biến Static của class (Bắt buộc phải nằm ở .cpp)
int WiFiManager::s_retry_num = 0;
const char* WiFiManager::TAG = "WIFI_MANAGER";

// 2. Triển khai nội dung hàm event_handler (Dùng toán tử phân giải phạm vi ::)
void WiFiManager::event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Thu ket noi lai vao diem truy cap...");
        } else {
            ESP_LOGE(TAG, "Ket noi Wi-Fi that bai.");
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Nhan IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    }
}

// 3. Triển khai nội dung hàm init
void WiFiManager::init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Truyền con trỏ hàm tĩnh bằng toán tử &WiFiManager::event_handler
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &WiFiManager::event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &WiFiManager::event_handler, NULL, NULL));

    wifi_config_t wifi_config = {};
    strcpy((char*)wifi_config.sta.ssid, WIFI_SSID);
    strcpy((char*)wifi_config.sta.password, WIFI_PASS);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}