#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Định danh các thiết bị chấp hành
typedef enum {
    DEV_RELAY_1 = 1,
    DEV_RELAY_2 = 2,
    DEV_MOSFET_1 = 3,
    DEV_MOSFET_2 = 4,
    DEV_ALARM_CLEAR = 5 
} DeviceID_t;

// Cấu trúc gói tin điều khiển
typedef struct {
    DeviceID_t device_id;
    uint8_t state; // 0: Tắt, 1: Bật
} ControlMsg_t;

// Khai báo Queue toàn cục
extern QueueHandle_t actuator_queue;