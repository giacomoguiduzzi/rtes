/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32l475e_iot01.h"
#include "stm32l475e_iot01_tsensor.h"
#include "stm32l475e_iot01_hsensor.h"
#include "stm32l475e_iot01_magneto.h"
#include "stm32l475e_iot01_psensor.h"
#include <math.h>
#include <stdio.h>
#include <inttypes.h>
#include "wifi_lib.h"

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
SPI_HandleTypeDef hspi3;

UART_HandleTypeDef huart1;

osThreadId defaultTaskHandle;
osThreadId readTempHandle;
osThreadId readHumidityHandle;
osThreadId readPressureHandle;
osThreadId readMagnetoHandle;
osThreadId WebServerHandle;
/* USER CODE BEGIN PV */

sensors_delay_t delay = FAST;
sensor_data_t sensor_data;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART1_UART_Init(void);
void StartDefaultTask(void const * argument);
void StartReadTemp(void const * argument);
void StartReadHum(void const * argument);
void StartReadPressure(void const * argument);
void StartReadMagnetometer(void const * argument);
void StartWebServer(void const * argument);

/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len){
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 10);
	return len;
}
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
  MX_SPI3_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of readTemp */
  osThreadDef(readTemp, StartReadTemp, osPriorityNormal, 0, 128);
  readTempHandle = osThreadCreate(osThread(readTemp), NULL);

  /* definition and creation of readHumidity */
  osThreadDef(readHumidity, StartReadHum, osPriorityNormal, 0, 128);
  readHumidityHandle = osThreadCreate(osThread(readHumidity), NULL);

  /* definition and creation of readPressure */
  osThreadDef(readPressure, StartReadPressure, osPriorityNormal, 0, 128);
  readPressureHandle = osThreadCreate(osThread(readPressure), NULL);

  /* definition and creation of readMagneto */
  osThreadDef(readMagneto, StartReadMagnetometer, osPriorityNormal, 0, 128);
  readMagnetoHandle = osThreadCreate(osThread(readMagneto), NULL);

  /* definition and creation of WebServer */
  osThreadDef(WebServer, StartWebServer, osPriorityNormal, 0, 128);
  WebServerHandle = osThreadCreate(osThread(WebServer), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_4BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : LED2_Pin */
  GPIO_InitStruct.Pin = LED2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(LED2_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  for(;;)
  {
	 // HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
	 osDelay(1000);
  }

  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartReadTemp */
/**
* @brief Function implementing the readTemp thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReadTemp */
void StartReadTemp(void const * argument)
{
  /* USER CODE BEGIN StartReadTemp */
	float temp_value = 0;
	char *str_tmp = "Temperatura = %d.%02d°C\n\r";
	char output_str[sizeof(str_tmp)];
	int tmpInt1, tmpInt2;
	float tmpFrac;

	BSP_TSENSOR_Init();
	/* Infinite loop */
	for(;;)
	{
		temp_value = BSP_TSENSOR_ReadTemp();
		tmpInt1 = temp_value;
		tmpFrac = temp_value - tmpInt1;
		tmpInt2 = trunc(tmpFrac * 100);
		snprintf(output_str, sizeof(output_str), str_tmp, tmpInt1, tmpInt2);
		HAL_UART_Transmit(&huart1, (uint8_t *)output_str, sizeof(str_tmp), 1000);
		osDelay(delay);
	}
  /* USER CODE END StartReadTemp */
}

/* USER CODE BEGIN Header_StartReadHum */
/**
* @brief Function implementing the readHumidity thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReadHum */
void StartReadHum(void const * argument)
{
  /* USER CODE BEGIN StartReadHum */
  /* Infinite loop */
	float hum_value = 0;
	char *str_hum = "Umidita' = %d.%02d\n\r";
	char output_str[sizeof(str_hum)];
	int humInt1, humInt2;
	float humFrac;

	BSP_HSENSOR_Init();
	/* Infinite loop */
	for(;;)
	{
		hum_value = BSP_HSENSOR_ReadHumidity();
		humInt1 = hum_value;
		humFrac = hum_value - humInt1;
		humInt2 = trunc(humFrac * 100);
		snprintf(output_str, sizeof(output_str), str_hum, humInt1, humInt2);
		HAL_UART_Transmit(&huart1, (uint8_t *)output_str, sizeof(str_hum), 1000);
		osDelay(delay);
	}
  /* USER CODE END StartReadHum */
}

/* USER CODE BEGIN Header_StartReadPressure */
/**
* @brief Function implementing the readPressure thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReadPressure */
void StartReadPressure(void const * argument)
{
  /* USER CODE BEGIN StartReadPressure */
	float pres_value = 0;
	char *str_pres = "Pressione = %d.%02d hPa\n\r";
	char output_str[sizeof(str_pres)];
	int presInt1, presInt2;
	float presFrac;

	BSP_PSENSOR_Init();
	/* Infinite loop */
	for(;;)
	{
		pres_value = BSP_PSENSOR_ReadPressure();
		presInt1 = pres_value;
		presFrac = pres_value - presInt1;
		presInt2 = trunc(presFrac * 100);
		snprintf(output_str, sizeof(output_str), str_pres, presInt1, presInt2);
		HAL_UART_Transmit(&huart1, (uint8_t *)output_str, sizeof(str_pres), 1000);
		osDelay(delay);
	}
  /* USER CODE END StartReadPressure */
}

/* USER CODE BEGIN Header_StartReadMagnetometer */
/**
* @brief Function implementing the readMagneto thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReadMagnetometer */
void StartReadMagnetometer(void const * argument)
{
  /* USER CODE BEGIN StartReadMagnetometer */
	int16_t xyz[3];
	int16_t x, y;
	char *str_tmp = "Direzione del nord = %d.%02d°\n\r";
	char output_str[sizeof(str_tmp)];
	double direction, magnFrac;
	int magnInt1, magnInt2;
	double declination_angle = 3.45;


	BSP_MAGNETO_Init();
	/* Infinite loop */
	for(;;)
	{
		BSP_MAGNETO_GetXYZ(xyz);
		x = xyz[0];
		y = xyz[1];

		if(y > 0)
			direction = 90 - (atan(x/y) * 180/M_PI);
		else if(y < 0)
			direction = 270 - (atan(x/y) * 180/M_PI);
		else if(y == 0 && x < 0)
			direction = 180.0;
		else if(y == 0 && x > 0)
			direction = 0.0;
		else
			direction = -1.0;

		if(direction != -1.0){
			direction += declination_angle;
			magnInt1 = direction;
			magnFrac = direction - magnInt1;
			magnInt2 = trunc(magnFrac * 100);
			snprintf(output_str, sizeof(output_str), str_tmp, magnInt1, magnInt2);
			HAL_UART_Transmit(&huart1, (uint8_t *)output_str, sizeof(str_tmp), 1000);
		}

		osDelay(delay);
	}
  /* USER CODE END StartReadMagnetometer */
}

/* USER CODE BEGIN Header_StartWebServer */
/**
* @brief Function implementing the WebServer thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartWebServer */
void StartWebServer(void const * argument)
{
  /* USER CODE BEGIN StartWebServer */

  /* Infinite loop */
  for(;;)
  {
	  do{
		  Initialize_WiFi(LED2_GPIO_Port, LED2_Pin);
	  }while(State == WS_ERROR);

	  WebServerProcess();
  }
  /* USER CODE END StartWebServer */
}

 /**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
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

#ifdef  USE_FULL_ASSERT
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

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
