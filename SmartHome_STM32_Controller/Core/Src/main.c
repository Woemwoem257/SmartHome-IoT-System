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
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "cJSON.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c3;

UART_HandleTypeDef huart6;
DMA_HandleTypeDef hdma_usart6_rx;

/* Definitions for Sensor_Task */
osThreadId_t Sensor_TaskHandle;
const osThreadAttr_t Sensor_Task_attributes = {
  .name = "Sensor_Task",
  .stack_size = 128 * 4,
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

// Khai báo Struct chứa Payload
typedef struct {
    uint16_t cmd_id;
    char action[32];
    uint32_t timeout_ms;
} AppCommand_t;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C3_Init(void);
static void MX_USART6_UART_Init(void);
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
  MX_I2C3_Init();
  MX_USART6_UART_Init();
  /* USER CODE BEGIN 2 */

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
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

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
  HAL_GPIO_WritePin(GPIOA, BUZZER_Pin|MOTOR1_Pin|MOTOR2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, RELAY2_Pin|RELAY1_Pin|LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(DATA_OUT_GPIO_Port, DATA_OUT_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : BUZZER_Pin MOTOR1_Pin MOTOR2_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin|MOTOR1_Pin|MOTOR2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
	       * @brief Parse JSON string from UART/MQTT into C Struct
	       * @param json_str Pointer to raw string buffer
	       * @param out_cmd Pointer to the struct to hold parsed data
	       * @return bool True if successful, False if invalid JSON or missing fields
	       */
	      bool Parse_Command_JSON(const char* json_str, AppCommand_t* out_cmd) {
	          // 1. Sanity Check
	          if (json_str == NULL || out_cmd == NULL) {
	              return false;
	          }

	          // 2. Phân tích chuỗi thành cJSON Object (Quá trình này có malloc)
	          cJSON *root = cJSON_Parse(json_str);
	          if (root == NULL) {
	              // Parse thất bại (chuỗi không đúng chuẩn JSON)
	              return false;
	          }

	          bool parse_status = true; // Cờ trạng thái

	          // 3. Trích xuất từng Field (cJSON_GetObjectItem) và ép kiểu (Validation)

	          // Xử lý field "cmd_id"
	          cJSON *cmd_id_item = cJSON_GetObjectItem(root, "cmd_id");
	          if (cJSON_IsNumber(cmd_id_item)) {
	              out_cmd->cmd_id = (uint16_t)cmd_id_item->valueint;
	          } else {
	              parse_status = false;
	          }

	          // Xử lý field "action"
	          cJSON *action_item = cJSON_GetObjectItem(root, "action");
	          if (cJSON_IsString(action_item) && (action_item->valuestring != NULL)) {
	              // Copy chuỗi một cách an toàn (tránh Buffer Overflow)
	              strncpy(out_cmd->action, action_item->valuestring, sizeof(out_cmd->action) - 1);
	              out_cmd->action[sizeof(out_cmd->action) - 1] = '\0'; // Đảm bảo null-terminated
	          } else {
	              parse_status = false;
	          }

	          // Xử lý field "timeout_ms"
	          cJSON *timeout_item = cJSON_GetObjectItem(root, "timeout_ms");
	          if (cJSON_IsNumber(timeout_item)) {
	              out_cmd->timeout_ms = (uint32_t)timeout_item->valuedouble; // Dùng valuedouble cho số lớn
	          } else {
	              parse_status = false;
	          }

	          // 4. BẮT BUỘC: Giải phóng bộ nhớ đã cấp phát cho cJSON object để tránh Memory Leak
	          cJSON_Delete(root);

	          return parse_status;
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
  /* Infinite loop */
  for(;;)
  {



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
	  /* Khởi tạo trạng thái an toàn: Ghim mức HIGH để TẮT MOSFET trước khi vào vòng lặp */
	  HAL_GPIO_WritePin(GPIOA, MOTOR1_Pin, GPIO_PIN_SET);
  /* Infinite loop */
  for(;;)
  {
	  /*
	       * TRẠNG THÁI 1: BẬT ĐỘNG CƠ (ACTIVE-LOW)
	       * Data Flow: MCU xuất LOW (0V) -> Opto TẮT -> R21 kéo cực Gate lên ~16.1V -> MOSFET DẪN
	       */
	      HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_RESET);
	      HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_SET);

	      // Giữ trạng thái 4 giây để ổn định que đo VOM hoặc quan sát động cơ khởi động
	      osDelay(3000);

	      /*
	       * TRẠNG THÁI 2: TẮT ĐỘNG CƠ
	       * Data Flow: MCU xuất HIGH (3.3V) -> Opto BẬT -> Cực Gate bị kéo xuống Mass -> MOSFET NGẮT
	       */
	      HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_SET);
	      HAL_GPIO_WritePin(GPIOB, RELAY1_Pin, GPIO_PIN_RESET);


	      // Giữ trạng thái 4 giây để đo đạc và chờ động cơ xả hết trớn (Inertia)
	      osDelay(3000);
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
	    AppCommand_t my_cmd; // Biến cấu trúc để hứng dữ liệu sau khi bóc tách

	    // Dọn sạch rác trong RAM trước khi nhận
	    memset(rx_buffer, 0, sizeof(rx_buffer));
	    // Khởi động DMA lắng nghe
	    HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rx_buffer, sizeof(rx_buffer));

	    /* Infinite loop */
	    for(;;)
	    {
	        // 1. NGỦ ĐÔNG chờ cờ hiệu (Thread Flag) từ ngắt IDLE DMA
	        osThreadFlagsWait(0x01, osFlagsWaitAny, osWaitForever);

	        // 2. ÉP KIỂU VÀ PARSE JSON (Gọi hàm)
	        // Lưu ý: Ép kiểu rx_buffer (uint8_t*) thành (char*) để cJSON hiểu
	        if (Parse_Command_JSON((char*)rx_buffer, &my_cmd))
	        {
	            // Nếu code chạy vào đây -> Bóc tách JSON thành công!
	            // Ví dụ: Kiểm tra nếu ESP32 gửi action là "bật_đèn"
	            if (strcmp(my_cmd.action, "turn_on_relay") == 0) {
	                // Tương lai: osMessageQueuePut(...) để đẩy lệnh sang Task02
	            }
	        }

	        // 3. ECHO TEST: Gửi trả lại đúng đoạn JSON đó lên ESP32 để Debug
	        // Dùng hàm strlen để tính đúng số byte có nghĩa, không gửi mảng thừa
	        HAL_UART_Transmit_DMA(&huart6, rx_buffer, strlen((char*)rx_buffer));

	        // 4. RESET BUFFER & LẮNG NGHE TIẾP
	        memset(rx_buffer, 0, sizeof(rx_buffer));
	        HAL_UARTEx_ReceiveToIdle_DMA(&huart6, rx_buffer, sizeof(rx_buffer));
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

	      // Khóa khẩn cấp tải DC (MOSFET thường Active High nên set LOW để ngắt)
	      HAL_GPIO_WritePin(GPIOA, MOTOR1_Pin | MOTOR2_Pin, GPIO_PIN_RESET);

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
