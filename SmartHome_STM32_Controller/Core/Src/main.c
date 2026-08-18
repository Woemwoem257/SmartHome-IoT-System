/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include "cJSON.h"
#include "dht22.h"
#include <stdlib.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
// Định nghĩa cấu trúc bản tin điều khiển
typedef struct {
    uint8_t device_id;  // 1: Relay 1 | 2: Relay 2 |
    uint8_t state;      // 0: TẮT | 1: BẬT
} ControlCmd_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart6_rx;

/* Definitions for Sensor_Task */
osThreadId_t Sensor_TaskHandle;
const osThreadAttr_t Sensor_Task_attributes = {
  .name = "Sensor_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Actuator_Task */
osThreadId_t Actuator_TaskHandle;
const osThreadAttr_t Actuator_Task_attributes = {
  .name = "Actuator_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UART_Parse_Task */
osThreadId_t UART_Parse_TaskHandle;
const osThreadAttr_t UART_Parse_Task_attributes = {
  .name = "UART_Parse_Task",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for Alarm_Task */
osThreadId_t Alarm_TaskHandle;
const osThreadAttr_t Alarm_Task_attributes = {
  .name = "Alarm_Task",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for ActuatorQueue */
osMessageQueueId_t ActuatorQueueHandle;
const osMessageQueueAttr_t ActuatorQueue_attributes = {
  .name = "ActuatorQueue"
};
/* Definitions for Timer_Heartbeat */
osTimerId_t Timer_HeartbeatHandle;
const osTimerAttr_t Timer_Heartbeat_attributes = {
  .name = "Timer_Heartbeat"
};
/* Definitions for AlarmSemaphore */
osSemaphoreId_t AlarmSemaphoreHandle;
const osSemaphoreAttr_t AlarmSemaphore_attributes = {
  .name = "AlarmSemaphore"
};
/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART6_UART_Init(void);
static void MX_TIM3_Init(void);
void StartDefaultTask(void *argument);
void StartTask02(void *argument);
void StartTask03(void *argument);
void StartTask04(void *argument);
void Callback01(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART6_UART_Init();
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
    HAL_TIM_Base_Start(&htim3);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of AlarmSemaphore */
  AlarmSemaphoreHandle = osSemaphoreNew(1, 1, &AlarmSemaphore_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* Create the timer(s) */
  /* creation of Timer_Heartbeat */
  Timer_HeartbeatHandle = osTimerNew(Callback01, osTimerPeriodic, NULL, &Timer_Heartbeat_attributes);

  /* USER CODE BEGIN RTOS_TIMERS */
  osTimerStart(Timer_HeartbeatHandle, 1000U);
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of ActuatorQueue */
  ActuatorQueueHandle = osMessageQueueNew (10, sizeof(ControlCmd_t), &ActuatorQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of Sensor_Task */
  Sensor_TaskHandle = osThreadNew(StartDefaultTask, NULL, &Sensor_Task_attributes);

  /* creation of Actuator_Task */
  Actuator_TaskHandle = osThreadNew(StartTask02, NULL, &Actuator_Task_attributes);

  /* creation of UART_Parse_Task */
  UART_Parse_TaskHandle = osThreadNew(StartTask03, NULL, &UART_Parse_Task_attributes);

  /* creation of Alarm_Task */
  Alarm_TaskHandle = osThreadNew(StartTask04, NULL, &Alarm_Task_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 100-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART6 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART6_UART_Init(void)
{

  /* USER CODE BEGIN USART6_Init 0 */

  /* USER CODE END USART6_Init 0 */

  /* USER CODE BEGIN USART6_Init 1 */

  /* USER CODE END USART6_Init 1 */
  huart6.Instance = USART6;
  huart6.Init.BaudRate = 115200;
  huart6.Init.WordLength = UART_WORDLENGTH_8B;
  huart6.Init.StopBits = UART_STOPBITS_1;
  huart6.Init.Parity = UART_PARITY_NONE;
  huart6.Init.Mode = UART_MODE_TX_RX;
  huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart6.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart6) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART6_Init 2 */

  /* USER CODE END USART6_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, RELAY2_Pin|RELAY1_Pin|LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DATA_OUT_GPIO_Port, DATA_OUT_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RELAY2_Pin RELAY1_Pin LED_Pin */
  GPIO_InitStruct.Pin = RELAY2_Pin|RELAY1_Pin|LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : D_OUT_Pin */
  GPIO_InitStruct.Pin = D_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(D_OUT_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DATA_OUT_Pin */
  GPIO_InitStruct.Pin = DATA_OUT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DATA_OUT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
/**
  * @brief  EXTI line detection callbacks.
  * @param  GPIO_Pin: Specifies the pins connected EXTI line
  * @retval None
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  // Kiểm tra xem có đúng là ngắt từ chân MQ-2 (PA10) không
  if(GPIO_Pin == D_OUT_Pin)
  {
    // Bắn tín hiệu Semaphore đánh thức Task_Alarm
    // Ưu điểm của CMSIS_V2: osSemaphoreRelease tự động xử lý an toàn trong môi trường ngắt
    osSemaphoreRelease(AlarmSemaphoreHandle);
  }
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if (huart->Instance == USART6)
    {
        // Gói tin từ ESP32 đã đến trọn vẹn.
        // Biến 'Size' cho bạn biết chính xác ESP32 đã gửi bao nhiêu byte.

        // Bắn Cờ hiệu (Thread Flag 0x01) để đánh thức Task03 dậy xử lý Buffer
        osThreadFlagsSet(UART_Parse_TaskHandle, 0x01);
    }
}

/**
 * @brief Parse JSON tĩnh không dùng cấp phát động (Zero-Allocation)
 * @param json_str Con trỏ trỏ tới buffer chứa chuỗi JSON
 * @return bool True nếu bóc tách thành công ít nhất 1 lệnh
 */
bool Parse_And_Queue_JSON(const char* json_str) {
    if (json_str == NULL) return false;

    bool parsed_any = false;

    // --- XỬ LÝ RELAY 1 ---
    // 1. Tìm vị trí xuất hiện của Key "relay1"
    const char *p1 = strstr(json_str, "\"relay1\"");
    if (p1 != NULL) {
        // 2. Tìm vị trí dấu hai chấm ':' ngay sau Key
        p1 = strchr(p1, ':');
        if (p1 != NULL) {
            // 3. Ép kiểu phần tử ngay sau dấu ':' thành số nguyên
            int state1 = atoi(p1 + 1);

            // 4. Kiểm tra biên bảo mật (Sanity Check) và đẩy vào Queue
            if (state1 == 0 || state1 == 1) {
                ControlCmd_t cmd1;
                cmd1.device_id = 1;
                cmd1.state = (uint8_t)state1;
                osMessageQueuePut(ActuatorQueueHandle, &cmd1, 0, 0); //
                parsed_any = true;
            }
        }
    }

    // --- XỬ LÝ RELAY 2 ---
    const char *p2 = strstr(json_str, "\"relay2\"");
    if (p2 != NULL) {
        p2 = strchr(p2, ':');
        if (p2 != NULL) {
            int state2 = atoi(p2 + 1);
            if (state2 == 0 || state2 == 1) {
                ControlCmd_t cmd2;
                cmd2.device_id = 2;
                cmd2.state = (uint8_t)state2;
                osMessageQueuePut(ActuatorQueueHandle, &cmd2, 0, 0); //[cite: 3]
                parsed_any = true;
            }
        }
    }

    return parsed_any;
}

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the App_Main_Task thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN 5 */
    DHT22_Data_t my_dht_data;
    char tx_buffer[128]; // Bộ đệm tĩnh chứa chuỗi JSON đẩy lên ESP32

    // Khởi tạo cảm biến
    DHT22_Init();

    /* Infinite loop */
    for(;;)
    {
    	// 1. ĐỌC DỮ LIỆU TỪ DHT22
        if (DHT22_Read_Data(&my_dht_data)) {
            // 2. ĐÓNG GÓI JSON (Zero-Allocation)
            // Cấu trúc dự kiến: {"temp": 28.5, "hum": 60.2}
            // Sử dụng snprintf để ngăn chặn triệt để lỗi Buffer Overflow
            snprintf(tx_buffer, sizeof(tx_buffer), "{\"temp\": %.1f, \"hum\": %.1f}\r\n",
                     my_dht_data.Temperature, my_dht_data.Humidity);

            // 3. GỬI DỮ LIỆU LÊN ESP32
            // Tạm thời sử dụng cơ chế Polling (Blocking) với Timeout = 100ms
            HAL_UART_Transmit(&huart6, (uint8_t*)tx_buffer, strlen(tx_buffer), 100);

        } else {
            // Xử lý khi lỗi đọc cảm biến (Bắn chuỗi cảnh báo lên Dashboard)
            const char* error_msg = "{\"error\": \"DHT22_Disconnected\"}\r\n";
            HAL_UART_Transmit(&huart6, (uint8_t*)error_msg, strlen(error_msg), 100);
        }

        // Chu kỳ lấy mẫu: 2 giây
        osDelay(3000);
    }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the Task_Sensor thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  /* USER CODE BEGIN StartTask02 */
	  // 1. Trạng thái an toàn ban đầu: Tắt toàn bộ tải
	  HAL_GPIO_WritePin(GPIOB, RELAY1_Pin | RELAY2_Pin, GPIO_PIN_RESET); // Active Low

	  ControlCmd_t rx_cmd; // Biến cục bộ để hứng dữ liệu từ Queue

	  /* Infinite loop */
	  for(;;)
	  {
	      // 2. NGỦ ĐÔNG chờ lệnh. Task bị block ở đây không tốn CPU
	      if (osMessageQueueGet(ActuatorQueueHandle, &rx_cmd, NULL, osWaitForever) == osOK)
	      {
	          // 3. THỰC THI LỆNH (Đã lấy được bản tin ra khỏi Queue)
	          switch (rx_cmd.device_id)
	          {
	              case 1: // Điều khiển Relay 1 (AC)
	                  // Mạch NPN Active High: state=1 => Kéo HIGH để bật
	                  if (rx_cmd.state == 1) HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_SET);
	                  else HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_RESET);
	                  break;

	              case 2: // Điều khiển Relay 2 (AC)
	                  if (rx_cmd.state == 1) HAL_GPIO_WritePin(GPIOB, RELAY2_Pin, GPIO_PIN_SET);
	                  else HAL_GPIO_WritePin(GPIOB, RELAY2_Pin, GPIO_PIN_RESET);
	                  break;
	          }
	      }
	  }
  /* USER CODE END StartTask02 */
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the Task_UART_Parse thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void *argument)
{
  /* USER CODE BEGIN StartTask03 */
    uint8_t rx_buffer[256];
    uint8_t process_buffer[256]; // Thêm Local Buffer để xử lý an toàn

    memset(rx_buffer, 0, sizeof(rx_buffer));

    // Khởi động DMA lắng nghe ở chế độ IDLE
    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rx_buffer, sizeof(rx_buffer));

    for(;;)
    {
        // 1. Chờ cờ hiệu từ hàm ngắt (HAL_UARTEx_RxEventCallback)
        osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

        // Tắt ngắt DMA tạm thời để copy data an toàn (Tránh đụng độ nếu data đến liên tục)
        HAL_UART_DMAStop(&huart6);

        // 2. Clone data sang Process Buffer & Dọn dẹp RX Buffer
        memcpy(process_buffer, rx_buffer, sizeof(rx_buffer));
        memset(rx_buffer, 0, sizeof(rx_buffer));

        // 3. Khởi động lại luồng thu thập DMA ngay lập tức để không rớt gói tin tiếp theo
        HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rx_buffer, sizeof(rx_buffer));

        // 4. Phân tích trên process_buffer (an toàn tuyệt đối)
        Parse_And_Queue_JSON((char*)process_buffer);

        // 5. Gửi Debug phản hồi (Sử dụng hàm Blocking để đảm bảo an toàn vùng nhớ)
        HAL_UART_Transmit(&huart6, process_buffer, strlen((char*)process_buffer), 100);
    }
  /* USER CODE END StartTask03 */
}

/* USER CODE BEGIN Header_StartTask04 */
/**
* @brief Function implementing the Task_Alarm thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask04 */
void StartTask04(void *argument)
{
  /* USER CODE BEGIN StartTask04 */

	// RÚT CẠN SEMAPHORE: Lấy Token dư thừa lúc khởi tạo (Không chờ)
	  osSemaphoreAcquire(AlarmSemaphoreHandle, 0);
	  /* Infinite loop */
	for(;;)
	  {
	      // 1. NGỦ ĐÔNG VÔ TẬN: Task sẽ dừng ở dòng này, nhường 100% CPU cho tác vụ khác.
	      // Nó chỉ chạy tiếp khi hàm ngắt ở trên gọi hàm Release()
	      osSemaphoreAcquire(AlarmSemaphoreHandle, osWaitForever);

	      // 2. KHI CÓ KHÓI (Đã vượt qua được Semaphore)
	      // Do task có quyền Realtime, CPU sẽ lập tức chạy đoạn này, bỏ qua mọi task khác

	      // Kích hoạt còi báo động (BUZZER_Pin)
	      HAL_GPIO_WritePin(GPIOA, BUZZER_Pin, GPIO_PIN_SET);

	      // Khóa khẩn cấp các tải AC (Giả sử Relay đang Active Low thì set lên HIGH để ngắt)
	      HAL_GPIO_WritePin(GPIOB, RELAY1_Pin | RELAY2_Pin, GPIO_PIN_SET);

	      // Vòng lặp khóa chết hệ thống: Bíp còi liên tục, không cho mạch làm việc gì khác
	      // Cho đến khi người dùng ngắt nguồn để reset
	      while(1) {
	          HAL_GPIO_TogglePin(GPIOA, BUZZER_Pin);
	          osDelay(3000); // Nhịp hú của còi
	      }
	  }
  /* USER CODE END StartTask04 */
}

/* Callback01 function */
void Callback01(void *argument)
{
  /* USER CODE BEGIN Callback01 */
	HAL_GPIO_TogglePin(GPIOB, LED_Pin);
  /* USER CODE END Callback01 */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM10 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM10)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
