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

void HmiManager::build_ui() {
    // --- KHAI BÁO CÁC MÀN HÌNH ---
    lv_obj_t * scr_main = lv_obj_create(NULL);
    lv_obj_t * scr_dashboard = lv_obj_create(NULL);

    /* ==========================================
     * 1. THIẾT KẾ MÀN HÌNH CHÍNH (scr_main)
     * ========================================== */
    lv_obj_t * btn_enter = lv_btn_create(scr_main);
    lv_obj_set_size(btn_enter, 160, 60);
    lv_obj_center(btn_enter);
    lv_obj_t * label_enter = lv_label_create(btn_enter);
    lv_label_set_text(label_enter, "Open Dashboard");

    lv_obj_add_event_cb(btn_enter, [](lv_event_t * e) {
        lv_obj_t * target_scr = (lv_obj_t *)lv_event_get_user_data(e);
        lv_scr_load_anim(target_scr, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
    }, LV_EVENT_CLICKED, scr_dashboard);

    /* ==========================================
     * 2. THIẾT KẾ MÀN HÌNH DASHBOARD (scr_dashboard)
     * ========================================== */
    lv_obj_t * btn_back = lv_btn_create(scr_dashboard);
    lv_obj_set_pos(btn_back, 10, 10);
    lv_obj_t * label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "<- Back");
    
    lv_obj_add_event_cb(btn_back, [](lv_event_t * e) {
        lv_obj_t * target_scr = (lv_obj_t *)lv_event_get_user_data(e);
        lv_scr_load_anim(target_scr, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
    }, LV_EVENT_CLICKED, scr_main);

    // Khu vực A: Sensor
    label_sensor_global = lv_label_create(scr_dashboard); // Gán vào con trỏ toàn cục
    lv_label_set_text(label_sensor_global, "Temp: --.-C | Hum: --.-%\nWaiting for node...");
    lv_obj_align(label_sensor_global, LV_ALIGN_TOP_MID, 0, 70);

    // Khu vực B: Actuator 
    // --- KHU VỰC B1: RELAY 1 ---
    btn_relay1_global = lv_btn_create(scr_dashboard); 
    lv_obj_align(btn_relay1_global, LV_ALIGN_CENTER, -60, -10);
    lv_obj_add_flag(btn_relay1_global, LV_OBJ_FLAG_CHECKABLE); 
    lv_obj_t * label_relay1 = lv_label_create(btn_relay1_global);
    lv_label_set_text(label_relay1, "Relay 1");

    // Bắt sự kiện khi trạng thái Toggle thay đổi
    lv_obj_add_event_cb(btn_relay1_global, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        bool is_on = lv_obj_has_state(btn, LV_STATE_CHECKED);
        ControlMsg_t msg;
        msg.device_id = DEV_RELAY_1;
        msg.state = is_on ? 1 : 0;
        if (actuator_queue != NULL) {
            xQueueSend(actuator_queue, &msg, 0);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

     // --- KHU VỰC B2: RELAY 2 ---
    btn_relay2_global = lv_btn_create(scr_dashboard); 
    lv_obj_align(btn_relay2_global, LV_ALIGN_CENTER, 60, -10);
    lv_obj_add_flag(btn_relay2_global, LV_OBJ_FLAG_CHECKABLE); 
    lv_obj_t * label_relay2 = lv_label_create(btn_relay2_global);
    lv_label_set_text(label_relay2, "Relay 2");

    // Bắt sự kiện khi trạng thái Toggle thay đổi
    lv_obj_add_event_cb(btn_relay2_global, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        bool is_on = lv_obj_has_state(btn, LV_STATE_CHECKED);
        ControlMsg_t msg;
        msg.device_id = DEV_RELAY_2;
        msg.state = is_on ? 1 : 0;
        if (actuator_queue != NULL) {
            xQueueSend(actuator_queue, &msg, 0);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // --- KHU VỰC B3: MOSFET 1 ---
    btn_mosfet1_global = lv_btn_create(scr_dashboard);
    lv_obj_align(btn_mosfet1_global, LV_ALIGN_CENTER, -60, 50);
    lv_obj_add_flag(btn_mosfet1_global, LV_OBJ_FLAG_CHECKABLE); // Kích hoạt Toggle
    lv_obj_t * label_mosfet1 = lv_label_create(btn_mosfet1_global);
    lv_label_set_text(label_mosfet1, "MOSFET 1");

    // Bắt sự kiện MOSFET 1 
    lv_obj_add_event_cb(btn_mosfet1_global, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        bool is_on = lv_obj_has_state(btn, LV_STATE_CHECKED);
        
        ControlMsg_t msg;
        msg.device_id = DEV_MOSFET_1;
        msg.state = is_on ? 1 : 0;
        
        if (actuator_queue != NULL) {
            xQueueSend(actuator_queue, &msg, 0);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // --- KHU VỰC B4: MOSFET 2 ---
    btn_mosfet2_global = lv_btn_create(scr_dashboard);
    lv_obj_align(btn_mosfet2_global, LV_ALIGN_CENTER, 60, 50);
    lv_obj_add_flag(btn_mosfet2_global, LV_OBJ_FLAG_CHECKABLE); // Kích hoạt Toggle
    lv_obj_t * label_mosfet2 = lv_label_create(btn_mosfet2_global);
    lv_label_set_text(label_mosfet2, "MOSFET 2");

    // Bắt sự kiện MOSFET 2 
    lv_obj_add_event_cb(btn_mosfet2_global, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        bool is_on = lv_obj_has_state(btn, LV_STATE_CHECKED);
        
        ControlMsg_t msg;
        msg.device_id = DEV_MOSFET_2;
        msg.state = is_on ? 1 : 0;
        
        if (actuator_queue != NULL) {
            xQueueSend(actuator_queue, &msg, 0);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    // Khu vực C: Alarm
    btn_alarm_global = lv_btn_create(scr_dashboard);
    lv_obj_set_style_bg_color(btn_alarm_global, lv_palette_main(LV_PALETTE_RED), 0);
    lv_obj_align(btn_alarm_global, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_flag(btn_alarm_global, LV_OBJ_FLAG_CHECKABLE); 
    lv_obj_t * label_alarm = lv_label_create(btn_alarm_global);
    lv_label_set_text(label_alarm, "ALARM CONTROL");

    // Bắt sự kiện ALARM 
    lv_obj_add_event_cb(btn_alarm_global, [](lv_event_t * e) {
        lv_obj_t * btn = lv_event_get_target(e);
        bool is_on = lv_obj_has_state(btn, LV_STATE_CHECKED);
        
        ControlMsg_t msg;
        msg.device_id = DEV_ALARM_CLEAR;
        msg.state = is_on ? 1 : 0;
        
        if (actuator_queue != NULL) {
            xQueueSend(actuator_queue, &msg, 0);
        }
    }, LV_EVENT_VALUE_CHANGED, NULL);

    lv_scr_load(scr_main);
}
    // 3. HÀM CẬP NHẬT GIAO DIỆN AN TOÀN ĐA LUỒNG (THREAD-SAFE)
void HmiManager::update_sensor_data(float temp, float hum) {
    // Chờ tối đa 100 Ticks (100ms) để lấy chìa khóa Mutex từ luồng HMI_Task
    if (xSemaphoreTake(xGuiSemaphore, pdMS_TO_TICKS(100)) == pdTRUE) {
        
        // Định dạng dữ liệu Float thành chuỗi ký tự C-string
        char buf[64];
        snprintf(buf, sizeof(buf), "Temp: %.1fC | Hum: %.1f%%\nSmoke: Safe", temp, hum);
        
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
