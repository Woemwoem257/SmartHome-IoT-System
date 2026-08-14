#pragma once
#include <string>

class UartBridge {
private:
    static const char* TAG;
    static const int RX_BUF_SIZE;
    
    // Tác vụ RTOS ngầm để liên tục đọc dữ liệu
    static void rx_task(void* arg);

public:
    // Khởi tạo ngoại vi UART
    static void init();

    // Hàm đẩy lệnh điều khiển (JSON) xuống STM32
    static void send_command(const std::string& cmd);
};