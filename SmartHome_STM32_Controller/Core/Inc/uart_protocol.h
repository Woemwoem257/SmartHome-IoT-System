/*
 * uart_protocol.h
 *
 *  Created on: Sep 21, 2026
 *      Author: ducnam
 */

#ifndef INC_UART_PROTOCOL_H_
#define INC_UART_PROTOCOL_H_

#include <stdbool.h>

// Hàm nhận chuỗi JSON và tự động đẩy lệnh vào Queue
bool Parse_And_Queue_JSON(const char* json_str);

#endif /* INC_UART_PROTOCOL_H_ */
