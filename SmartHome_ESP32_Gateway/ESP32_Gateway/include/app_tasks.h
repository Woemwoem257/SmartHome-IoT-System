#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "hmi_manager.h" // Để lấy định nghĩa ControlMsg_t

// Khởi tạo hệ điều hành
void App_InitTasks();

// API Giao tiếp an toàn (Không dùng extern)
bool App_SendActuatorCmd(ControlMsg_t msg);
bool App_TakeGuiMutex(uint32_t timeout_ms);
void App_GiveGuiMutex();
void App_SuspendHMITask();
void App_ResumeHMITask();