#pragma once
#include <lvgl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Định danh các thiết bị chấp hành
typedef enum {
    DEV_RELAY_1,
    DEV_MOSFET_1
} DeviceID_t;

// Cấu trúc gói tin điều khiển
typedef struct {
    DeviceID_t device_id;
    uint8_t state; // 0: Tắt, 1: Bật
} ControlMsg_t;

// Khai báo Queue toàn cục để các file khác có thể truy cập
extern QueueHandle_t actuator_queue;

class HmiManager {
public:
    static void build_ui();
    static void update_sensor_data(float temp, float hum);
};