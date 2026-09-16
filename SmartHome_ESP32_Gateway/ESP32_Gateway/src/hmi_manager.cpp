#include "hmi_manager.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

// 1. Kéo Mutex từ main.cpp sang
extern SemaphoreHandle_t xGuiSemaphore;

// 2. Lưu trữ con trỏ Label ra phạm vi toàn cục của file này
static lv_obj_t * label_sensor_global = NULL;
static lv_obj_t * btn_relay1_global = NULL; 
static lv_obj_t * btn_relay2_global = NULL; 
static lv_obj_t * btn_mosfet1_global = NULL; 
static lv_obj_t * btn_mosfet2_global = NULL; 
static lv_obj_t * btn_alarm_global = NULL; 
lv_obj_t * label_wifi_global = NULL;

void HmiManager::build_ui() {
    // Kích hoạt Dark Theme tối giản
    lv_theme_default_init(NULL, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);

    lv_obj_t * scr_main = lv_obj_create(NULL);
    lv_obj_t * scr_dashboard = lv_obj_create(NULL);

    /* ==========================================
     * 1. MÀN HÌNH CHÍNH (TỐI GIẢN CỰC HẠN)
     * ========================================== */
    // Nền đen tuyền tiết kiệm năng lượng và CPU
    lv_obj_set_style_bg_color(scr_main, lv_color_hex(0x000000), LV_STATE_DEFAULT);

    lv_obj_t * btn_enter = lv_btn_create(scr_main);
    lv_obj_set_size(btn_enter, 160, 50);
    lv_obj_center(btn_enter);
    lv_obj_set_style_radius(btn_enter, 5, 0); // Bo góc cực nhẹ, không đổ bóng

    lv_obj_t * label_enter = lv_label_create(btn_enter);
    lv_label_set_text(label_enter, LV_SYMBOL_HOME " START SYSTEM");

    // Sự kiện chuyển cảnh
    lv_obj_add_event_cb(btn_enter, [](lv_event_t * e) {
        lv_obj_t * target_scr = (lv_obj_t *)lv_event_get_user_data(e);
        // SỬ DỤNG CHUYỂN CẢNH TỨC THỜI (NONE) ĐỂ KHÔNG ÉP CPU TÍNH TOÁN ANIMATION
        lv_scr_load_anim(target_scr, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }, LV_EVENT_CLICKED, scr_dashboard);


    /* ==========================================
     * 2. MÀN HÌNH DASHBOARD (LƯỚI ĐIỀU KHIỂN)
     * ========================================== */
    // --- PHÂN VÙNG HEADER ---
    label_wifi_global = lv_label_create(scr_dashboard);
    lv_label_set_text(label_wifi_global, LV_SYMBOL_WIFI " Wait..");
    lv_obj_set_style_text_color(label_wifi_global, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(label_wifi_global, LV_ALIGN_TOP_RIGHT, -10, 10);

    // --- PHÂN VÙNG DISPLAY (SENSOR PANEL) ---
    lv_obj_t * panel_sensor = lv_obj_create(scr_dashboard);
    lv_obj_set_size(panel_sensor, 220, 80);
    lv_obj_align(panel_sensor, LV_ALIGN_TOP_MID, 0, 35);
    lv_obj_set_style_border_color(panel_sensor, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_set_style_border_width(panel_sensor, 2, 0);

    label_sensor_global = lv_label_create(panel_sensor);
    lv_label_set_text(label_sensor_global, "Temp: --.-C\nHum: --.-%\n" LV_SYMBOL_WARNING " Smoke: Syncing...");
    lv_obj_center(label_sensor_global);

    // --- PHÂN VÙNG CONTROL (ACTUATORS GRID) ---
    int btn_w = 100, btn_h = 50;

    // Relay 1 (Đèn)
    btn_relay1_global = lv_btn_create(scr_dashboard); 
    lv_obj_set_size(btn_relay1_global, btn_w, btn_h);
    lv_obj_align(btn_relay1_global, LV_ALIGN_CENTER, -55, 10);
    lv_obj_add_flag(btn_relay1_global, LV_OBJ_FLAG_CHECKABLE); 
    lv_label_set_text(lv_label_create(btn_relay1_global), LV_SYMBOL_POWER " L1");
    lv_obj_add_event_cb(btn_relay1_global, [](lv_event_t * e) {
        ControlMsg_t msg = {DEV_RELAY_1, (uint8_t)(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0)};
        if (actuator_queue != NULL) xQueueSend(actuator_queue, &msg, 0);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Relay 2 (Đèn)
    btn_relay2_global = lv_btn_create(scr_dashboard); 
    lv_obj_set_size(btn_relay2_global, btn_w, btn_h);
    lv_obj_align(btn_relay2_global, LV_ALIGN_CENTER, 55, 10);
    lv_obj_add_flag(btn_relay2_global, LV_OBJ_FLAG_CHECKABLE); 
    lv_label_set_text(lv_label_create(btn_relay2_global), LV_SYMBOL_POWER " L2");
    lv_obj_add_event_cb(btn_relay2_global, [](lv_event_t * e) {
        ControlMsg_t msg = {DEV_RELAY_2, (uint8_t)(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0)};
        if (actuator_queue != NULL) xQueueSend(actuator_queue, &msg, 0);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // MOSFET 1 (Quạt)
    btn_mosfet1_global = lv_btn_create(scr_dashboard);
    lv_obj_set_size(btn_mosfet1_global, btn_w, btn_h);
    lv_obj_align(btn_mosfet1_global, LV_ALIGN_CENTER, -55, 70);
    lv_obj_add_flag(btn_mosfet1_global, LV_OBJ_FLAG_CHECKABLE);
    lv_label_set_text(lv_label_create(btn_mosfet1_global), LV_SYMBOL_POWER " Fan 1");
    lv_obj_add_event_cb(btn_mosfet1_global, [](lv_event_t * e) {
        ControlMsg_t msg = {DEV_MOSFET_1, (uint8_t)(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0)};
        if (actuator_queue != NULL) xQueueSend(actuator_queue, &msg, 0);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // MOSFET 2 (Quạt)
    btn_mosfet2_global = lv_btn_create(scr_dashboard);
    lv_obj_set_size(btn_mosfet2_global, btn_w, btn_h);
    lv_obj_align(btn_mosfet2_global, LV_ALIGN_CENTER, 55, 70);
    lv_obj_add_flag(btn_mosfet2_global, LV_OBJ_FLAG_CHECKABLE);
    lv_label_set_text(lv_label_create(btn_mosfet2_global), LV_SYMBOL_POWER " Fan 2");
    lv_obj_add_event_cb(btn_mosfet2_global, [](lv_event_t * e) {
        ControlMsg_t msg = {DEV_MOSFET_2, (uint8_t)(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0)};
        if (actuator_queue != NULL) xQueueSend(actuator_queue, &msg, 0);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // --- NÚT BÁO ĐỘNG ---
    btn_alarm_global = lv_btn_create(scr_dashboard);
    lv_obj_set_size(btn_alarm_global, 210, 45);
    lv_obj_align(btn_alarm_global, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_flag(btn_alarm_global, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_bg_color(btn_alarm_global, lv_palette_main(LV_PALETTE_GREEN), LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(btn_alarm_global, lv_palette_main(LV_PALETTE_RED), LV_STATE_CHECKED);
    lv_label_set_text(lv_label_create(btn_alarm_global), LV_SYMBOL_BELL " ALARM STATUS");
    lv_obj_add_event_cb(btn_alarm_global, [](lv_event_t * e) {
        ControlMsg_t msg = {DEV_ALARM_CLEAR, (uint8_t)(lv_obj_has_state(lv_event_get_target(e), LV_STATE_CHECKED) ? 1 : 0)};
        if (actuator_queue != NULL) xQueueSend(actuator_queue, &msg, 0);
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // 3. Khởi chạy màn hình Welcome đầu tiên
    lv_scr_load(scr_main);
}

    // 3. HÀM CẬP NHẬT GIAO DIỆN AN TOÀN ĐA LUỒNG (THREAD-SAFE)
void HmiManager::update_sensor_data(float temp, float hum) {
    // Chờ tối đa 100 Ticks (100ms) để lấy chìa khóa Mutex từ luồng HMI_Task
    if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        
        // 1. Đọc trạng thái hiện tại của nút Báo động để suy ra trạng thái khói
        bool is_smoke_detected = false;
        if (btn_alarm_global != NULL) {
            is_smoke_detected = lv_obj_has_state(btn_alarm_global, LV_STATE_CHECKED);
        }

        // 2. Chèn trạng thái thực tế vào chuỗi
        char buf[64];
        if (is_smoke_detected) {
            snprintf(buf, sizeof(buf), "Temp: %.1fC | Hum: %.1f%%\nSmoke: DANGER!", temp, hum);
        } else {
            snprintf(buf, sizeof(buf), "Temp: %.1fC | Hum: %.1f%%\nSmoke: Clear", temp, hum);
        }
        // Cập nhật lên Object LVGL
        if(label_sensor_global != NULL) {
            lv_label_set_text(label_sensor_global, buf);
        }
        
        // Bắt buộc phải nhả khóa ra để LVGL tiếp tục render khung hình
        xSemaphoreGive(xGuiSemaphore);
    }  
}

void HmiManager::update_actuator_state(DeviceID_t id, bool is_on) {
    if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Bộ định tuyến con trỏ LVGL
        lv_obj_t * target_btn = NULL;
        if (id == DEV_RELAY_1) target_btn = btn_relay1_global;
        else if (id == DEV_RELAY_2) target_btn = btn_relay2_global;
        else if (id == DEV_MOSFET_1) target_btn = btn_mosfet1_global;
        else if (id == DEV_MOSFET_2) target_btn = btn_mosfet2_global;
        else if (id == DEV_ALARM_CLEAR) target_btn = btn_alarm_global;

        // Cập nhật trạng thái đồ họa an toàn
        if (target_btn != NULL) {
            if (is_on) lv_obj_add_state(target_btn, LV_STATE_CHECKED);
            else lv_obj_clear_state(target_btn, LV_STATE_CHECKED);
        }
        xSemaphoreGive(xGuiSemaphore);
    }
}

void HmiManager::update_network_status(bool is_connected) {
    if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (label_wifi_global != NULL) {
            if (is_connected) {
                lv_label_set_text(label_wifi_global, LV_SYMBOL_WIFI " AWS: OK");
                lv_obj_set_style_text_color(label_wifi_global, lv_palette_main(LV_PALETTE_GREEN), 0);
            } else {
                lv_label_set_text(label_wifi_global, LV_SYMBOL_WIFI " Offline");
                lv_obj_set_style_text_color(label_wifi_global, lv_palette_main(LV_PALETTE_RED), 0);
            }
        }
        xSemaphoreGive(xGuiSemaphore);
    }
}
