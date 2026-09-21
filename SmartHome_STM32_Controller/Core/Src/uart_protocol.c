/*
 * uart_protocol.c
 *
 *  Created on: Sep 21, 2026
 *      Author: ducnam
 */

#include "uart_protocol.h"
#include "app_types.h"
#include "cmsis_os.h"
#include <string.h>
#include <stdlib.h>

// Mượn các biến toàn cục từ hệ điều hành (hiện đang nằm trong main.c)
extern osMessageQueueId_t ActuatorQueueHandle;
extern osThreadId_t Alarm_TaskHandle;

bool Parse_And_Queue_JSON(const char* json_str) {
    if (json_str == NULL) return false;

    bool parsed_any = false;

    // --- XỬ LÝ RELAY 1 ---
    const char *p1 = strstr(json_str, "\"relay1\"");
    if (p1 != NULL) {
        p1 = strchr(p1, ':');
        if (p1 != NULL) {
            int state1 = atoi(p1 + 1);
            if (state1 == 0 || state1 == 1) {
                ControlCmd_t cmd1;
                cmd1.device_id = 1; // 1: Relay 1
                cmd1.state = (uint8_t)state1;
                osMessageQueuePut(ActuatorQueueHandle, &cmd1, 0, 0);
                parsed_any = true;
            }
        }
    }

    // --- XỬ LÝ RELAY 2 ---
    const char *p2 = strstr(json_str, "\"relay2\"");
    if (p2 != NULL) {
        p2 = strchr(p2, ':');
        if (p2 != NULL) {
            int state2 = atoi(p2 + 1);
            if (state2 == 0 || state2 == 1) {
                ControlCmd_t cmd2;
                cmd2.device_id = 2; // 2: Relay 2
                cmd2.state = (uint8_t)state2;
                osMessageQueuePut(ActuatorQueueHandle, &cmd2, 0, 0);
                parsed_any = true;
            }
        }
    }

    // --- XỬ LÝ MOSFET 1 ---
    const char *p3 = strstr(json_str, "\"mosfet1\"");
    if (p3 != NULL) {
        p3 = strchr(p3, ':');
        if (p3 != NULL) {
            int state3 = atoi(p3 + 1);
            if (state3 == 0 || state3 == 1) {
                ControlCmd_t cmd3;
                cmd3.device_id = 3; // 3: MOSFET 1
                cmd3.state = (uint8_t)state3;
                osMessageQueuePut(ActuatorQueueHandle, &cmd3, 0, 0);
                parsed_any = true;
            }
        }
    }

    // --- XỬ LÝ MOSFET 2 ---
    const char *p4 = strstr(json_str, "\"mosfet2\"");
    if (p4 != NULL) {
        p4 = strchr(p4, ':');
        if (p4 != NULL) {
            int state4 = atoi(p4 + 1);
            if (state4 == 0 || state4 == 1) {
                ControlCmd_t cmd4;
                cmd4.device_id = 4; // 4: MOSFET 2
                cmd4.state = (uint8_t)state4;
                osMessageQueuePut(ActuatorQueueHandle, &cmd4, 0, 0);
                parsed_any = true;
            }
        }
    }

    // --- XỬ LÝ LỆNH MỞ KHÓA BÁO ĐỘNG  ---
    const char *p5 = strstr(json_str, "\"alarm_clear\"");
    if (p5 != NULL) {
        p5 = strchr(p5, ':');
        if (p5 != NULL) {
            int clear_cmd = atoi(p5 + 1);
            if (clear_cmd == 0 || clear_cmd == 1) {
                // Bắn cờ hiệu (Flag 0x02) sang luồng Alarm_Task
                osThreadFlagsSet(Alarm_TaskHandle, 0x02);
                parsed_any = true;
            }
        }
    }

    return parsed_any;
}
