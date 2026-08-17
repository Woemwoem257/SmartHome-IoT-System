/*
 * dht22.c
 *
 *  Created on: Aug 5, 2026
 *      Author: ducnam
 */

#include "dht22.h"
#include "cmsis_os.h"

#define DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004)

// Hàm tạo trễ micro-giây (YÊU CẦU PHẢI IMPLEMENT BẰNG TIMER HOẶC DWT)
extern void delay_us(uint32_t us);

void DHT22_Init(void) {
	// PA15 đã được cấu hình tĩnh là Open-Drain trong MX_GPIO_Init.
	// Việc kéo chân lên SET ở chế độ OD tương đương với việc thả nổi,
	// cho phép điện trở 10k ngoài mạch kéo điện áp lên 5V chuẩn xác.
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
    uint32_t start_tick;
    uint32_t pulse_tick;

    // 1. TÍNH TOÁN NGƯỠNG THỜI GIAN THỰC
    uint32_t ticks_per_us = HAL_RCC_GetHCLKFreq() / 1000000;
    uint32_t threshold_40us = 40 * ticks_per_us; // Ngưỡng 40us để phân biệt bit 0 và 1
    uint32_t timeout_100us = 100 * ticks_per_us; // Ngưỡng 100us chống treo hệ thống

    // 2. TÍN HIỆU START (Không đặt trong vùng Critical để tránh treo RTOS quá lâu)
    // Kéo LOW để đánh thức cảm biến
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_RESET);
    osDelay(2); // Giữ LOW > 1ms (osDelay dùng SysTick nên an toàn)

    // Kéo HIGH để "nhả" bus, chờ DHT22 phản hồi
    HAL_GPIO_WritePin(DHT22_PORT, DHT22_PIN, GPIO_PIN_SET);
    delay_us(30);

    // ==========================================================
    // VÀO VÙNG TỚI HẠN (CRITICAL SECTION)
    // Cấm ngắt bắt đầu từ đây để đếm xung micro-giây chính xác
    taskENTER_CRITICAL();
    // ==========================================================

    // 3. CHỜ PHẢN HỒI TỪ CẢM BIẾN
    start_tick = DWT_CYCCNT;
    while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        if ((DWT_CYCCNT - start_tick) > timeout_100us) goto READ_ERROR;
    }

    start_tick = DWT_CYCCNT;
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
            if ((DWT_CYCCNT - start_tick) > timeout_100us) goto READ_ERROR;
	}


	start_tick = DWT_CYCCNT;
		while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
			if ((DWT_CYCCNT - start_tick) > timeout_100us) goto READ_ERROR;
	}

    // 4. ĐỌC 40 BITS DỮ LIỆU
    for (int i = 0; i < 40; i++) {
        // Đợi hết xung LOW
    	start_tick = DWT_CYCCNT;
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_RESET) {
        	if ((DWT_CYCCNT - start_tick) > timeout_100us) goto READ_ERROR;
        }

        start_tick = DWT_CYCCNT;
        // Đo độ rộng xung HIGH
        while (HAL_GPIO_ReadPin(DHT22_PORT, DHT22_PIN) == GPIO_PIN_SET) {
        	if ((DWT_CYCCNT - start_tick) > timeout_100us) goto READ_ERROR;
        }
        pulse_tick = DWT_CYCCNT - start_tick; // Chốt thời gian xung HIGH

        // So sánh trực tiếp số Tick phần cứng với ngưỡng 40us
        if (pulse_tick > threshold_40us) {
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
    if (checksum != data[4]) return false;

    // Tính nhiệt độ
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
