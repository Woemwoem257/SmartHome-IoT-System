/*
 * dht22.h
 *
 *  Created on: Aug 5, 2026
 *      Author: ducnam
 */

#ifndef DHT22_H_
#define DHT22_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>

// Định nghĩa cấu trúc lưu trữ dữ liệu
typedef struct {
    float Temperature;
    float Humidity;
} DHT22_Data_t;

// Các hàm API giao tiếp (Đã tiêm phụ thuộc phần cứng vào tham số)
void DHT22_Init(TIM_HandleTypeDef *timer, GPIO_TypeDef *port, uint16_t pin);
bool DHT22_Read_Data(DHT22_Data_t *dht_data);

#endif /* DHT22_H_ */
