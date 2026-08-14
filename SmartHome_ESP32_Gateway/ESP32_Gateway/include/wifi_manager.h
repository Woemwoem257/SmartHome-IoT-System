#pragma once
#include <esp_wifi.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>

class WiFiManager {
private:
    static int s_retry_num;
    static const char*  TAG;

    //Khai báo nguyên mẫu hàm (Prototype)
    static void event_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data);

    public:
        static void init();
};