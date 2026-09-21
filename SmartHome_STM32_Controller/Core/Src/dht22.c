#include "dht22.h"
#include "cmsis_os.h"

// Lưu trữ thông số phần cứng cục bộ, hoàn toàn cách ly với main.c
static TIM_HandleTypeDef *dht_timer;
static GPIO_TypeDef      *dht_port;
static uint16_t          dht_pin;

// Hàm tạo trễ micro-giây sử dụng con trỏ dht_timer cục bộ
static void delay_us(uint16_t us) {
    uint16_t start_tick = __HAL_TIM_GET_COUNTER(dht_timer);
    while ((uint16_t)(__HAL_TIM_GET_COUNTER(dht_timer) - start_tick) < us) {
        // Chờ phần cứng
    }
}

void DHT22_Init(TIM_HandleTypeDef *timer, GPIO_TypeDef *port, uint16_t pin) {
    // Nhận thông số từ main.c bơm vào và lưu lại
    dht_timer = timer;
    dht_port = port;
    dht_pin = pin;

    HAL_GPIO_WritePin(dht_port, dht_pin, GPIO_PIN_SET);
}

bool DHT22_Read_Data(DHT22_Data_t *dht_data) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint8_t bit_index = 7;
    uint8_t byte_index = 0;

    uint16_t start_tick;
    uint16_t pulse_ticks;

    uint16_t threshold_40us = 40;
    uint16_t timeout_100us = 100;

    // 1. TÍN HIỆU START
    HAL_GPIO_WritePin(dht_port, dht_pin, GPIO_PIN_RESET);
    osDelay(2); // Giữ LOW 2ms bằng nhịp hệ điều hành

    HAL_GPIO_WritePin(dht_port, dht_pin, GPIO_PIN_SET);
    delay_us(30); // Nhả bus 30us bằng Timer

    // ==========================================================
    taskENTER_CRITICAL();
    // ==========================================================

    // 2. CHỜ PHẢN HỒI TỪ CẢM BIẾN (Đếm bằng dht_timer)
    start_tick = __HAL_TIM_GET_COUNTER(dht_timer);
    while (HAL_GPIO_ReadPin(dht_port, dht_pin) == GPIO_PIN_SET) {
        if ((uint16_t)(__HAL_TIM_GET_COUNTER(dht_timer) - start_tick) > timeout_100us) goto READ_ERROR;
    }

    start_tick = __HAL_TIM_GET_COUNTER(dht_timer);
    while (HAL_GPIO_ReadPin(dht_port, dht_pin) == GPIO_PIN_RESET) {
        if ((uint16_t)(__HAL_TIM_GET_COUNTER(dht_timer) - start_tick) > timeout_100us) goto READ_ERROR;
    }

    start_tick = __HAL_TIM_GET_COUNTER(dht_timer);
    while (HAL_GPIO_ReadPin(dht_port, dht_pin) == GPIO_PIN_SET) {
        if ((uint16_t)(__HAL_TIM_GET_COUNTER(dht_timer) - start_tick) > timeout_100us) goto READ_ERROR;
    }

    // 3. ĐỌC 40 BITS DỮ LIỆU
    for (int i = 0; i < 40; i++) {
        start_tick = __HAL_TIM_GET_COUNTER(dht_timer);
        while (HAL_GPIO_ReadPin(dht_port, dht_pin) == GPIO_PIN_RESET) {
            if ((uint16_t)(__HAL_TIM_GET_COUNTER(dht_timer) - start_tick) > timeout_100us) goto READ_ERROR;
        }

        // Đo độ rộng xung HIGH
        start_tick = __HAL_TIM_GET_COUNTER(dht_timer);
        while (HAL_GPIO_ReadPin(dht_port, dht_pin) == GPIO_PIN_SET) {
            if ((uint16_t)(__HAL_TIM_GET_COUNTER(dht_timer) - start_tick) > timeout_100us) goto READ_ERROR;
        }

        pulse_ticks = (uint16_t)(__HAL_TIM_GET_COUNTER(dht_timer) - start_tick);

        // Phân biệt bit 1 và bit 0
        if (pulse_ticks > threshold_40us) {
            data[byte_index] |= (1 << bit_index);
        }

        if (bit_index == 0) {
            bit_index = 7;
            byte_index++;
        } else {
            bit_index--;
        }
    }

    // ==========================================================
    taskEXIT_CRITICAL();
    // ==========================================================

    // 4. KIỂM TRA CHECKSUM VÀ CHUYỂN ĐỔI DỮ LIỆU
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) return false;

    uint16_t raw_temp = (data[2] << 8) | data[3];
    if (raw_temp & 0x8000) {
        raw_temp &= 0x7FFF;
        dht_data->Temperature = (float)raw_temp / -10.0f;
    } else {
        dht_data->Temperature = (float)raw_temp / 10.0f;
    }

    dht_data->Humidity = (float)((data[0] << 8) | data[1]) / 10.0f;
    return true;

READ_ERROR:
    taskEXIT_CRITICAL();
    return false;
}
