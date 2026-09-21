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

        float calc_temp, calc_hum;
        uint16_t raw_temp = (data[2] << 8) | data[3];
        if (raw_temp & 0x8000) {
            raw_temp &= 0x7FFF;
            calc_temp = (float)raw_temp / -10.0f;
        } else {
            calc_temp = (float)raw_temp / 10.0f;
        }

        calc_hum = (float)((data[0] << 8) | data[1]) / 10.0f;

        // 5. BỘ LỌC BIÊN ĐỘ (BOUNDARY CHECK)
        // Chặn đứng các giá trị rác do nhiễu điện từ (EMI) vượt quá ngưỡng vật lý của DHT22
        if (calc_temp < -40.0f || calc_temp > 80.0f) return false;
        if (calc_hum < 0.0f || calc_hum > 100.0f) return false;

        // 6. BỘ LỌC TRUNG BÌNH TRƯỢT HÀM MŨ (EWMA - ĐỂ LÀM MỊN ĐƯỜNG ĐẶC TÍNH)
        static float filtered_temp = 0.0f;
        static float filtered_hum = 0.0f;
        static bool first_reading = true;

        // Hệ số Alpha = 0.2: Tin tưởng 20% vào giá trị mới, giữ lại 80% lịch sử cũ để triệt tiêu gai nhiễu
        #define EWMA_ALPHA 0.2f

        if (first_reading) {
            // Lần đọc hợp lệ đầu tiên sẽ khởi tạo bộ lọc để tránh giá trị bị kéo lê từ 0 lên
            filtered_temp = calc_temp;
            filtered_hum = calc_hum;
            first_reading = false;
        } else {
            filtered_temp = (EWMA_ALPHA * calc_temp) + ((1.0f - EWMA_ALPHA) * filtered_temp);
            filtered_hum = (EWMA_ALPHA * calc_hum) + ((1.0f - EWMA_ALPHA) * filtered_hum);
        }

        // Trả về dữ liệu đã được lọc sạch
        dht_data->Temperature = filtered_temp;
        dht_data->Humidity = filtered_hum;

        return true;

    READ_ERROR:
        taskEXIT_CRITICAL();
        return false;
    }
