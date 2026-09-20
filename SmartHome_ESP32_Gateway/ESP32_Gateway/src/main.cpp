#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <lvgl.h> 
#include "hmi_manager.h"

QueueHandle_t actuator_queue = NULL;
SemaphoreHandle_t xGuiSemaphore = NULL;

#include "wifi_manager.h"
#include "uart_bridge.h"
#include "aws_mqtt.h"

#define TOUCH_CS   15
#define TOUCH_IRQ  27
#define TOUCH_SCLK 14
#define TOUCH_MISO 12
#define TOUCH_MOSI 13

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

const char *TAG = "app_main";

TFT_eSPI tft = TFT_eSPI(); 
SPIClass touchSPI(HSPI); 
XPT2046_Touchscreen ts(TOUCH_CS, TOUCH_IRQ);

TaskHandle_t HMITaskHandle = NULL;

// [Giữ nguyên my_disp_flush và my_touchpad_read như cũ]
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp_drv);
}

void my_touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    if (ts.touched()) {
        TS_Point p = ts.getPoint();
        data->state = LV_INDEV_STATE_PR; 
        data->point.x = map(p.x, 200, 3800, 0, SCREEN_WIDTH);
        data->point.y = map(p.y, 200, 3800, 0, SCREEN_HEIGHT);
    } else {
        data->state = LV_INDEV_STATE_REL; 
    }
}

// [Giữ nguyên HMI_Task và Actuator_Task như cũ]
void HMI_Task(void *pvParameters) {
    lv_init();
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[SCREEN_WIDTH * 10];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    HmiManager::build_ui();

    while (1) {
        if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler(); 
            xSemaphoreGive(xGuiSemaphore); 
        }
        vTaskDelay(pdMS_TO_TICKS(33)); 
    }
}

void Actuator_Task(void *pvParameters) {
    ControlMsg_t msg;
    while (1) {
        if (xQueueReceive(actuator_queue, &msg, portMAX_DELAY) == pdPASS) {
            ESP_LOGI("ACTUATOR", "Nhan lenh HMI -> Thiet bi: %d, Trang thai: %d", msg.device_id, msg.state);
            char cmd[64];
            if (msg.device_id == DEV_RELAY_1) snprintf(cmd, sizeof(cmd), "{\"relay1\":%d}\r\n", msg.state);
            else if (msg.device_id == DEV_RELAY_2) snprintf(cmd, sizeof(cmd), "{\"relay2\":%d}\r\n", msg.state); 
            else if (msg.device_id == DEV_MOSFET_1) snprintf(cmd, sizeof(cmd), "{\"mosfet1\":%d}\r\n", msg.state);
            else if (msg.device_id == DEV_MOSFET_2) snprintf(cmd, sizeof(cmd), "{\"mosfet2\":%d}\r\n", msg.state);
            else if (msg.device_id == DEV_ALARM_CLEAR) snprintf(cmd, sizeof(cmd), "{\"alarm_clear\":%d}\r\n", msg.state);
            
            UartBridge::send_command(cmd);
        }
    }
}

/* =========================================================================
 * NEW: TASK QUẢN LÝ MẠNG VÀ BẢO MẬT (CHẠY TRÊN CORE 0)
 * ========================================================================= */
void Network_Task(void *pvParameters) {
    ESP_LOGI("SYSTEM", "--> B1. Khoi tao Wi-Fi (Dinh dong 1)...");
    WiFiManager::init();

    // Đồng bộ thời gian thực SNTP
    esp_sntp_setoperatingmode((esp_sntp_operatingmode_t)SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "time.google.com");      
    esp_sntp_init();

    time_t now; struct tm timeinfo;
    time(&now); localtime_r(&now, &timeinfo);
    
    while (timeinfo.tm_year < (2020 - 1900)) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay không chặn FreeRTOS
        time(&now); localtime_r(&now, &timeinfo);
    }
    
    ESP_LOGI("SYSTEM", "--> B2. Dong bo thoi gian thanh cong! Cho 3s de IC nguon nghi ngoi...");
    // NGHỈ NGƠI CHIẾN LƯỢC: CHỜ 3 GIÂY ĐỂ TỤ NẠP LẠI ĐIỆN VÀ IC ỔN ÁP TẢN NHIỆT
    vTaskDelay(pdMS_TO_TICKS(3000)); 

    ESP_LOGI("SYSTEM", "--> B3. Bat dau ma hoa mTLS (Dinh dong 2)...");
    AwsMqtt::init();

    // Luồng mạng chỉ chạy 1 lần để khởi tạo, sau khi xong tự sát để giải phóng RAM
    vTaskDelete(NULL); 
}

/* =========================================================================
 * KHỞI TẠO TUẦN TỰ (POWER SEQUENCING) TRONG APP_MAIN
 * ========================================================================= */
extern "C" void app_main() 
{ 
    initArduino();
    ESP_LOGI(TAG, "Middleware Gateway Started"); 
    
    // 1. TẦNG CƠ BẢN (Ăn 0mA)
    actuator_queue = xQueueCreate(10, sizeof(ControlMsg_t));
    xTaskCreatePinnedToCore(Actuator_Task, "Actuator", 4096, NULL, 6, NULL, 1);
    UartBridge::init();

    ESP_LOGI(TAG, "Cho 1 giay de on dinh dien ap boot...");
    vTaskDelay(pdMS_TO_TICKS(1000)); 

    // 2. TẦNG ĐỒ HỌA (Ăn ~100mA cho đèn nền màn hình)
    tft.init();
    tft.setRotation(2);
    tft.invertDisplay(true); 
    tft.fillScreen(TFT_BLACK);

    touchSPI.begin(TOUCH_SCLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
    ts.begin(touchSPI);
    ts.setRotation(2);

    xGuiSemaphore = xSemaphoreCreateMutex();
    xTaskCreatePinnedToCore(HMI_Task, "HMI_Task", 8192, NULL, 5, &HMITaskHandle, 1);

    ESP_LOGI(TAG, "Man hinh da bat. Cho them 2 giay de Tu dien nap day...");
    vTaskDelay(pdMS_TO_TICKS(2000)); 

    // 3. TẦNG VIỄN THÔNG VÀ AWS (Ăn ~500mA)
    // Tách toàn bộ quá trình kết nối mạng (Wi-Fi, SNTP, AWS) sang một Task độc lập
    // ÉP LUỒNG MẠNG CHẠY Ở CORE 0 (PRO_CPU) ĐỂ CHIA SẺ NHIỆT LƯỢNG VỚI CORE 1
    xTaskCreatePinnedToCore(Network_Task, "Network_Task", 8192, NULL, 4, NULL, 0); 
}