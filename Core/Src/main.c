/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "lwip.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip/tcp.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define LED_PORT        GPIOD
#define LED_GREEN_PIN   GPIO_PIN_12
#define LED_ORANGE_PIN  GPIO_PIN_13
#define LED_RED_PIN     GPIO_PIN_14
#define LED_BLUE_PIN    GPIO_PIN_15

#define DEBUG_PRINT(str) HAL_UART_Transmit(&huart4, (uint8_t*)str, strlen(str), 1000)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart4;

/* USER CODE BEGIN PV */
static struct tcp_pcb *hello_tpcb;
static ip_addr_t dest_ip;

volatile int broadcastTCPFlag = 0;
uint32_t lastSend = 0;

char dbg_buf[64];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_UART4_Init(void);
/* USER CODE BEGIN PFP */
static void tcp_error_callback(void *arg, err_t err);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */


static err_t tcp_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    if (err == ERR_OK) {
        hello_tpcb = tpcb;
        snprintf(dbg_buf, sizeof(dbg_buf), "tcp_connected: OK (err=%d)\r\n", err);
        DEBUG_PRINT(dbg_buf);
    } else {
        snprintf(dbg_buf, sizeof(dbg_buf), "tcp_connected: FAILED (err=%d)\r\n", err);
        DEBUG_PRINT(dbg_buf);
    }
    return err;
}


void start_tcp_connection(void)
{
	hello_tpcb = tcp_new();
	if (hello_tpcb != NULL) {
		IP4_ADDR(&dest_ip, 192,168,1,100); // your PC IP
		tcp_connect(hello_tpcb, &dest_ip, 502, tcp_connected);
		tcp_err(hello_tpcb, tcp_error_callback);
	} else {
		Error_Handler();
	}
}

void send_hello_tcp(void)
{
    if (hello_tpcb == NULL) return;

    const char *msg = "Hello World from STM32F407\r\n";
    err_t err = tcp_write(hello_tpcb, msg, strlen(msg), TCP_WRITE_FLAG_COPY);

    if (err == ERR_OK) {
        tcp_output(hello_tpcb);
        HAL_GPIO_TogglePin(LED_PORT, LED_BLUE_PIN);
    }
    else if (err == ERR_ABRT || err == ERR_RST || err == ERR_CLSD) {
        hello_tpcb = NULL;
        HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, GPIO_PIN_SET);
    }
    else {
        HAL_GPIO_TogglePin(LED_PORT, LED_ORANGE_PIN);
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	uint8_t data = 111;

	HAL_UART_Transmit(&huart4, &data, 1, 1000);
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
  MX_LWIP_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
	HAL_GPIO_WritePin(LED_PORT, LED_ORANGE_PIN, GPIO_PIN_RESET);

	start_tcp_connection();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		MX_LWIP_Process();

		    if (broadcastTCPFlag && hello_tpcb && HAL_GetTick() - lastSend >= 2000) {
		        send_hello_tcp();
		        lastSend = HAL_GetTick();
		    }
		    else if (broadcastTCPFlag && !hello_tpcb) {
		        // try to reconnect if lost
		        start_tcp_connection();
		    }
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 64;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
static void tcp_error_callback(void *arg, err_t err)
{
    hello_tpcb = NULL; // connection lost
    HAL_GPIO_WritePin(LED_PORT, LED_RED_PIN, GPIO_PIN_SET);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	static uint32_t lastPress = 0;

	if (GPIO_Pin == GPIO_PIN_0) {
		uint32_t now = HAL_GetTick();
		if (now - lastPress > 200) {
			broadcastTCPFlag ^= 1;    // toggle flag
			if (broadcastTCPFlag) {
			//	HAL_GPIO_WritePin(LED_PORT, LED_ORANGE_PIN, GPIO_PIN_SET); // orange = ON
			} else {
			//	HAL_GPIO_WritePin(LED_PORT, LED_ORANGE_PIN, GPIO_PIN_RESET); // off
			}
			lastPress = now;
		}
	}
}
/* USER CODE END 4 */

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
