/*
 * app_types.h
 *
 *  Created on: Sep 21, 2026
 *      Author: ducnam
 */

#ifndef INC_APP_TYPES_H_
#define INC_APP_TYPES_H_

#include <stdint.h>
#include <stdbool.h>

// Định nghĩa cấu trúc bản tin điều khiển
typedef struct {
    uint8_t device_id;  // 1: Relay1 | 2: Relay2 | 3: MOSFET1 | 4: MOSFET2 | 5: ALARM
    uint8_t state;      // 0: TẮT | 1: BẬT
} ControlCmd_t;

#endif /* INC_APP_TYPES_H_ */
