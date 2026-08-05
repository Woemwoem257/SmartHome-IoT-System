/*
 * dht22.h
 *
 *  Created on: Aug 5, 2026
 *      Author: ducnam
 */

#ifndef DHT22_H_
#define DHT22_H_

#include "main.h"
#include <stdbool.h>

// Định nghĩa cấu trúc lưu trữ dữ liệu
typedef struct {
    float Temperature;
    float Humidity;
} DHT22_Data_t;

// Cấu hình chân GPIO giao tiếp (Đồng bộ với CubeMX)
#define DHT22_PORT DATA_OUT_GPIO_Port
#define DHT22_PIN  DATA_OUT_Pin

// Các hàm API giao tiếp
void DHT22_Init(void);
bool DHT22_Read_Data(DHT22_Data_t *dht_data);

#endif /* DHT22_H_ */
