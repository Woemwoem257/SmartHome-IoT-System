/*
 * dht22.c
 *
 *  Created on: Aug 5, 2026
 *      Author: ducnam
 */

#include "dht22.h"
#include "cmsis_os.h"

/*
 * Macro điều khiển GPIO cực nhanh sử dụng thanh ghi BSRR
 * giúp giảm độ trễ so với việc gọi hàm HAL_GPIO_WritePin
 */
#define DHT22_DIR_OUT() do { \
                            GPIO_InitTypeDef GPIO_InitStruct = {0}; \
                            GPIO_InitStruct.Pin = DHT22_PIN; \
                            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP; \
                            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; \
                            HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct); \
                        } while(0)

#define DHT22_DIR_IN()  do { \
                            GPIO_InitTypeDef GPIO_InitStruct = {0}; \
                            GPIO_InitStruct.Pin = DHT22_PIN; \
                            GPIO_InitStruct.Mode = GPIO_MODE_INPUT; \
                            GPIO_InitStruct.Pull = GPIO_NOPULL; \
                            HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStruct); \
                        } while(0)

// Hàm tạo trễ micro-giây (YÊU CẦU PHẢI IMPLEMENT BẰNG TIMER HOẶC DWT)
extern void delay_us(uint32_t us);

void DHT22_Init(void) {
    DHT22_DIR_OUT();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET); // Kéo lên HIGH ở trạng thái nghỉ
}

/**
 * @brief Đọc dữ liệu từ DHT22
 * @param dht_data Con trỏ tới struct chứa kết quả
 * @return true nếu đọc thành công và Checksum đúng, false nếu lỗi
 */
bool DHT22_Read_Data(DHT22_Data_t *dht_data) {
    uint8_t data[5] = {0, 0, 0, 0, 0};
    uint8_t bit_index = 7;
    uint8_t byte_index = 0;
    uint16_t timeout;

    // 1. TÍN HIỆU START (Không đặt trong vùng Critical để tránh treo RTOS quá lâu)
    DHT22_DIR_OUT();
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    osDelay(2); // Giữ LOW > 1ms (osDelay dùng SysTick nên an toàn)

    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    delay_us(30); // Giữ HIGH 20-40us

    DHT22_DIR_IN(); // Chuyển sang chế độ đọc

    // ==========================================================
    // VÀO VÙNG TỚI HẠN (CRITICAL SECTION)
    // Cấm mọi ngắt chen ngang trong 4-5ms tới
    taskENTER_CRITICAL();
    // ==========================================================

    // 2. CHỜ PHẢN HỒI TỪ CẢM BIẾN
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        delay_us(1);
        if (++timeout > 100) goto READ_ERROR;
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
        delay_us(1);
        if (++timeout > 100) goto READ_ERROR;
    }

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        delay_us(1);
        if (++timeout > 100) goto READ_ERROR;
    }

    // 3. ĐỌC 40 BITS DỮ LIỆU
    for (int i = 0; i < 40; i++) {
        timeout = 0;
        // Đợi hết xung LOW (chuẩn bị vào xung HIGH mang dữ liệu)
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
            delay_us(1);
            if (++timeout > 100) goto READ_ERROR;
        }

        timeout = 0;
        // Đo độ rộng xung HIGH
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
            delay_us(1);
            if (++timeout > 100) goto READ_ERROR;
        }

        // Quyết định bit 0 hay 1 (Ngưỡng 40us)
        if (timeout > 40) {
            data[byte_index] |= (1 << bit_index); // Ghi bit 1
        }

        if (bit_index == 0) {
            bit_index = 7;
            byte_index++;
        } else {
            bit_index--;
        }
    }

    // ==========================================================
    // THOÁT VÙNG TỚI HẠN
    // Mở lại các ngắt hệ thống để RTOS hoạt động bình thường
    taskEXIT_CRITICAL();
    // ==========================================================

    // 4. KIỂM TRA CHECKSUM VÀ CHUYỂN ĐỔI DỮ LIỆU
    uint8_t checksum = data[0] + data[1] + data[2] + data[3];
    if (checksum != data[4]) {
        return false;
    }

    // Xử lý bit dấu của nhiệt độ
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
    // Nếu xảy ra lỗi Timeout (Cảm biến bị rút dây, hỏng hóc)
    taskEXIT_CRITICAL(); // BẮT BUỘC phải nhả Critical Section trước khi thoát hàm
    return false;
}
