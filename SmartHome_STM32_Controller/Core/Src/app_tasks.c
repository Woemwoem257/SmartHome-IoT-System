/*
 * app_tasks.c
 *
 *  Created on: Sep 21, 2026
 *      Author: ducnam
 */

#include "app_tasks.h"
#include "cmsis_os.h"
#include "main.h"
#include "dht22.h"
#include "app_types.h"
#include "actuator.h"
#include "uart_protocol.h"
#include "stm32f4xx_hal.h"
#include <stdio.h>
#include <string.h>

// Mượn ngoại vi từ main.c
extern UART_HandleTypeDef huart6;
extern TIM_HandleTypeDef htim3;
extern volatile uint16_t uart_rx_size;

// ==============================================================================
// KHAI BÁO CÁC HANDLE CỦA FREERTOS (Chuyển từ main.c sang)
// ==============================================================================
osThreadId_t Sensor_TaskHandle;
osThreadId_t Actuator_TaskHandle;
osThreadId_t UART_Parse_TaskHandle;
osThreadId_t Alarm_TaskHandle;

osMessageQueueId_t ActuatorQueueHandle;
osSemaphoreId_t AlarmSemaphoreHandle;

// Cấu hình thuộc tính cho các Task
const osThreadAttr_t Sensor_Task_attributes = { .name = "Sensor_Task", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityNormal, };
const osThreadAttr_t Actuator_Task_attributes = { .name = "Actuator_Task", .stack_size = 128 * 4, .priority = (osPriority_t) osPriorityNormal, };
const osThreadAttr_t UART_Parse_Task_attributes = { .name = "UART_Parse_Task", .stack_size = 256 * 4, .priority = (osPriority_t) osPriorityAboveNormal, };
const osThreadAttr_t Alarm_Task_attributes = { .name = "Alarm_Task", .stack_size = 128 * 4, .priority = (osPriority_t) osPriorityRealtime, };
const osMessageQueueAttr_t ActuatorQueue_attributes = { .name = "ActuatorQueue" };
const osSemaphoreAttr_t AlarmSemaphore_attributes = { .name = "AlarmSemaphore" };

// ==============================================================================
// NỘI DUNG CÁC TASK THỰC THI
// ==============================================================================

// 1. Task Đọc Cảm Biến (Đổi tên từ StartDefaultTask)
void App_SensorTask(void *argument) {
    DHT22_Data_t my_dht_data;
    char tx_buffer[128];
    DHT22_Init(&htim3, DATA_OUT_GPIO_Port, DATA_OUT_Pin);

    for(;;) {
        if (DHT22_Read_Data(&my_dht_data)) {
            int temp_int = (int)my_dht_data.Temperature;
            int temp_frac = (int)(my_dht_data.Temperature * 10) % 10;
            int hum_int = (int)my_dht_data.Humidity;
            int hum_frac = (int)(my_dht_data.Humidity * 10) % 10;

            snprintf(tx_buffer, sizeof(tx_buffer), "{\"temperature\": %d.%d, \"humidity\": %d.%d}\r\n", temp_int, temp_frac, hum_int, hum_frac);
            HAL_UART_Transmit(&huart6, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);
        } else {
            const char* error_msg = "{\"error\": \"DHT22_Disconnected\"}\r\n";
            HAL_UART_Transmit(&huart6, (uint8_t*)error_msg, strlen(error_msg), 100);
        }
        osDelay(3000);
    }
}

// 2. Task Chấp Hành (Đổi tên từ StartTask02)
void App_ActuatorTask(void *argument) {
    Actuator_Init();
    ControlCmd_t rx_cmd;
    for(;;) {
        if (osMessageQueueGet(ActuatorQueueHandle, &rx_cmd, NULL, osWaitForever) == osOK) {
            bool is_on = (rx_cmd.state == 1);
            switch (rx_cmd.device_id) {
                case 1: Actuator_SetRelay1(is_on); break;
                case 2: Actuator_SetRelay2(is_on); break;
                case 3: Actuator_SetMosfet1(is_on); break;
                case 4: Actuator_SetMosfet2(is_on); break;
            }
        }
    }
}

// 3. Task Xử lý UART (Đổi tên từ StartTask03)
void App_UARTParseTask(void *argument) {
    uint8_t rx_buffer[256];
    uint8_t process_buffer[256];
    memset(rx_buffer, 0, sizeof(rx_buffer));
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rx_buffer, sizeof(rx_buffer));

    for(;;) {
        osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);
        HAL_UART_DMAStop(&huart6);

        uint16_t safe_size = uart_rx_size;
        if (safe_size >= sizeof(process_buffer)) safe_size = sizeof(process_buffer) - 1;
        memcpy(process_buffer, rx_buffer, safe_size);
        process_buffer[safe_size] = '\0';

        memset(rx_buffer, 0, sizeof(rx_buffer));
        HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rx_buffer, sizeof(rx_buffer));

        Parse_And_Queue_JSON((char*)process_buffer);
        HAL_UART_Transmit(&huart6, process_buffer, safe_size, 100);
    }
}

// 4. Task Báo Động (Đổi tên từ StartTask04)
void App_AlarmTask(void *argument) {
    osSemaphoreAcquire(AlarmSemaphoreHandle, 0); // Vét cạn token rác
    char uplink_msg[128];

    for(;;) {
        osSemaphoreAcquire(AlarmSemaphoreHandle, osWaitForever);
        Actuator_SetBuzzer(true);
        Actuator_Init(); // Khóa chéo: Đưa các relay/mosfet về trạng thái an toàn tắt

        snprintf(uplink_msg, sizeof(uplink_msg), "{\"relay1\":0, \"relay2\":0, \"mosfet1\":0, \"mosfet2\":0, \"alarm_clear\":1}\r\n");
        HAL_UART_Transmit(&huart6, (uint8_t*)uplink_msg, strlen(uplink_msg), 100);

        while(1) {
            HAL_GPIO_TogglePin(BUZZER_GPIO_Port, BUZZER_Pin);
            uint32_t flags = osThreadFlagsWait(0x02, osFlagsWaitAny, 500);

            if (flags == 0x02) {
                Actuator_SetBuzzer(false);
                snprintf(uplink_msg, sizeof(uplink_msg), "{\"alarm_clear\":0}\r\n");
                HAL_UART_Transmit(&huart6, (uint8_t*)uplink_msg, strlen(uplink_msg), 100);

                while (osSemaphoreAcquire(AlarmSemaphoreHandle, 0) == osOK) {}
                while (HAL_GPIO_ReadPin(D_OUT_GPIO_Port, D_OUT_Pin) == GPIO_PIN_RESET) { osDelay(500); }
                break;
            }
        }
    }
}

// ==============================================================================
// HÀM KHỞI TẠO TỔNG
// ==============================================================================
void App_Init(void) {
    // 1. Tạo Semaphore & Timer
    AlarmSemaphoreHandle = osSemaphoreNew(1, 1, &AlarmSemaphore_attributes);

    // 2. Tạo Queue
    ActuatorQueueHandle = osMessageQueueNew(10, sizeof(ControlCmd_t), &ActuatorQueue_attributes);

    // 3. Tạo Threads
    Sensor_TaskHandle = osThreadNew(App_SensorTask, NULL, &Sensor_Task_attributes);
    Actuator_TaskHandle = osThreadNew(App_ActuatorTask, NULL, &Actuator_Task_attributes);
    UART_Parse_TaskHandle = osThreadNew(App_UARTParseTask, NULL, &UART_Parse_Task_attributes);
    Alarm_TaskHandle = osThreadNew(App_AlarmTask, NULL, &Alarm_Task_attributes);
}
