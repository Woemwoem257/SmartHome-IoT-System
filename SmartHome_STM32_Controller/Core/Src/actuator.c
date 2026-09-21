/*
 * actuator.c
 *
 *  Created on: Sep 21, 2026
 *      Author: ducnam
 */
#include "actuator.h"
#include "main.h" // Chứa định nghĩa Macro chân GPIO từ CubeMX

void Actuator_Init(void) {
    // Trạng thái an toàn ban đầu: Tắt toàn bộ tải
    Actuator_SetRelay1(false);
    Actuator_SetRelay2(false);
    Actuator_SetMosfet1(false);
    Actuator_SetMosfet2(false);
    Actuator_SetBuzzer(false);
}

void Actuator_SetRelay1(bool state) {
    // Relay NPN Active High: state=true => SET
    HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Actuator_SetRelay2(bool state) {
    HAL_GPIO_WritePin(GPIOB, RELAY2_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Actuator_SetMosfet1(bool state) {
    // MOSFET Active Low: state=true => RESET
    HAL_GPIO_WritePin(GPIOA, MOTOR1_Pin, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void Actuator_SetMosfet2(bool state) {
    HAL_GPIO_WritePin(GPIOA, MOTOR2_Pin, state ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void Actuator_SetBuzzer(bool state) {
    HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, state ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

