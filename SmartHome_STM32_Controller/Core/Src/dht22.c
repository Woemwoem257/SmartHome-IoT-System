#include "dht22.h"
#include "cmsis_os.h"

// Gọi biến TIM3 do CubeMX sinh ra từ main.c
extern TIM_HandleTypeDef htim3;

// Hàm tạo trễ micro-giây an toàn tuyệt đối bằng TIM3
void delay_us(uint16_t us) {
    uint16_t start_tick = __HAL_TIM_GET_COUNTER(&htim3);
    while ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim3) - start_tick) < us) {
        // Chờ phần cứng
    }
}

void DHT22_Init(void) {
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
}

bool DHT22_Read_Data(DHT22_Data_t *dht_data) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint8_t bit_index = 7;
    uint8_t byte_index = 0;

    // Sử dụng kiểu uint16_t cho thanh ghi 16-bit của TIM3
    uint16_t start_tick;
    uint16_t pulse_ticks;

    // TIM3 đã đếm chuẩn 1 tick = 1us, định mức ngưỡng tĩnh
    uint16_t threshold_40us = 40;
    uint16_t timeout_100us = 100;

    // 1. TÍN HIỆU START (Đã dọn dẹp code lặp)
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    osDelay(2); // Giữ LOW 2ms bằng nhịp hệ điều hành

    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    delay_us(30); // Nhả bus 30us bằng Timer

    // ==========================================================
    taskENTER_CRITICAL();
    // ==========================================================

    // 2. CHỜ PHẢN HỒI TỪ CẢM BIẾN (Đếm bằng TIM3)
    start_tick = __HAL_TIM_GET_COUNTER(&htim3);
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        if ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim3) - start_tick) > timeout_100us) goto READ_ERROR;
    }

    start_tick = __HAL_TIM_GET_COUNTER(&htim3);
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
        if ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim3) - start_tick) > timeout_100us) goto READ_ERROR;
    }

    start_tick = __HAL_TIM_GET_COUNTER(&htim3);
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        if ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim3) - start_tick) > timeout_100us) goto READ_ERROR;
    }

    // 3. ĐỌC 40 BITS DỮ LIỆU
    for (int i = 0; i < 40; i++) {
        start_tick = __HAL_TIM_GET_COUNTER(&htim3);
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
            if ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim3) - start_tick) > timeout_100us) goto READ_ERROR;
        }

        // Đo độ rộng xung HIGH
        start_tick = __HAL_TIM_GET_COUNTER(&htim3);
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
            if ((uint16_t)(__HAL_TIM_GET_COUNTER(&htim3) - start_tick) > timeout_100us) goto READ_ERROR;
        }

        // Chốt độ rộng xung
        pulse_ticks = (uint16_t)(__HAL_TIM_GET_COUNTER(&htim3) - start_tick);

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
    taskEXIT_CRITICAL(); // Nhả hệ điều hành khi bị Timeout
    return false;
}
