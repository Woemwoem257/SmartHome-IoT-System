#include "app_tasks.h"
#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_log.h>
#include <esp_sntp.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "hmi_manager.h"
#include "wifi_manager.h"
#include "uart_bridge.h"
#include "aws_mqtt.h"
#include "board_config.h"

static const char *TAG = "APP_TASKS";

// 1. DI DỜI CÁC BIẾN RTOS TỪ MAIN.CPP SANG ĐÂY
QueueHandle_t actuator_queue = NULL;
SemaphoreHandle_t xGuiSemaphore = NULL;
TaskHandle_t HMITaskHandle = NULL;

// 2. MƯỢN ĐỐI TƯỢNG PHẦN CỨNG TỪ MAIN.CPP
extern TFT_eSPI tft;
extern XPT2046_Touchscreen ts;

/* =========================================================================
 * CALLBACK MÀN HÌNH LVGL
 * ========================================================================= */
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
        data->point.x = map(p.x, 200, 3800, 0, Board::ScreenWidth);
        data->point.y = map(p.y, 200, 3800, 0, Board::ScreenHeight);
    } else {
        data->state = LV_INDEV_STATE_REL; 
    }
}

/* =========================================================================
 * NỘI DUNG CÁC TASKS
 * ========================================================================= */
void HMI_Task(void *pvParameters) {
    lv_init();
    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[Board::ScreenWidth * 10];
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, Board::ScreenWidth * 10);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = Board::ScreenWidth;
    disp_drv.ver_res = Board::ScreenHeight;
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
            ESP_LOGI("ACTUATOR", "Nhan lenh -> Thiet bi: %d, Trang thai: %d", msg.device_id, msg.state);
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

void Network_Task(void *pvParameters) {
    ESP_LOGI("SYSTEM", "Khoi tao Wi-Fi...");
    WiFiManager::init();

    esp_sntp_setoperatingmode((esp_sntp_operatingmode_t)SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "time.google.com");      
    esp_sntp_init();

    time_t now; struct tm timeinfo;
    time(&now); localtime_r(&now, &timeinfo);
    
    while (timeinfo.tm_year < (2020 - 1900)) {
        vTaskDelay(pdMS_TO_TICKS(1000)); 
        time(&now); localtime_r(&now, &timeinfo);
    }
    
    ESP_LOGI("SYSTEM", "Dong bo thoi gian thanh cong! Cho 3s de on dinh...");
    vTaskDelay(pdMS_TO_TICKS(3000)); 

    ESP_LOGI("SYSTEM", "Bat dau mTLS...");
    AwsMqtt::init();

    vTaskDelete(NULL); 
}

/* =========================================================================
 * KHỞI TẠO HỆ ĐIỀU HÀNH
 * ========================================================================= */
void App_InitTasks() {
    // 1. Tạo các Object với cơ chế bẫy lỗi (Error Trapping)
    actuator_queue = xQueueCreate(10, sizeof(ControlMsg_t));
    if (actuator_queue == NULL) {
        ESP_LOGE(TAG, "Loi: Khong du RAM tao Actuator Queue");
        abort();
    }

    xGuiSemaphore = xSemaphoreCreateMutex();
    if (xGuiSemaphore == NULL) {
        ESP_LOGE(TAG, "Loi: Khong du RAM tao GUI Mutex");
        abort();
    }

    // 2. Kích hoạt các luồng
    BaseType_t res;
    res = xTaskCreatePinnedToCore(Actuator_Task, "Actuator", 4096, NULL, 6, NULL, 1);
    if (res != pdPASS) { ESP_LOGE(TAG, "Loi tao Actuator_Task"); abort(); }

    res = xTaskCreatePinnedToCore(HMI_Task, "HMI_Task", 8192, NULL, 5, &HMITaskHandle, 1);
    if (res != pdPASS) { ESP_LOGE(TAG, "Loi tao HMI_Task"); abort(); }

    res = xTaskCreatePinnedToCore(Network_Task, "Network_Task", 8192, NULL, 4, NULL, 0);
    if (res != pdPASS) { ESP_LOGE(TAG, "Loi tao Network_Task"); abort(); }
}

/* =========================================================================
 * PUBLIC API (Thay thế hoàn toàn cho extern)
 * ========================================================================= */
bool App_SendActuatorCmd(ControlMsg_t msg) {
    if (actuator_queue == NULL) return false;
    return (xQueueSend(actuator_queue, &msg, 0) == pdPASS);
}

bool App_TakeGuiMutex(uint32_t timeout_ms) {
    if (xGuiSemaphore == NULL) return false;
    return (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(timeout_ms)) == pdTRUE);
}

void App_GiveGuiMutex() {
    if (xGuiSemaphore != NULL) xSemaphoreGive(xGuiSemaphore);
}

void App_SuspendHMITask() {
    if (HMITaskHandle != NULL) vTaskSuspend(HMITaskHandle);
}

void App_ResumeHMITask() {
    if (HMITaskHandle != NULL) vTaskResume(HMITaskHandle);
}