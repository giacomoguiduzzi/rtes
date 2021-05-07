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
#include <stdlib.h>
#include <inttypes.h>
#include "sensors.h"

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
DFSDM_Channel_HandleTypeDef hdfsdm1_channel1;

QSPI_HandleTypeDef hqspi;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart4;
UART_HandleTypeDef huart1;

PCD_HandleTypeDef hpcd_USB_OTG_FS;

osThreadId defaultTaskHandle;
osThreadId readTempHandle;
osThreadId readHumHandle;
osThreadId readPressHandle;
osThreadId readMagnetoHandle;
osThreadId sendDataHandle;
osThreadId receiveDelayHandle;
/* USER CODE BEGIN PV */

sensors_delay_t delay;
sensor_data_t sensor_data;
uint8_t *written_data;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DFSDM1_Init(void);
static void MX_QUADSPI_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USB_OTG_FS_PCD_Init(void);
static void MX_SPI1_Init(void);
static void MX_UART4_Init(void);
void StartDefaultTask(void const * argument);
void StartReadTemp(void const * argument);
void StartReadHum(void const * argument);
void StartReadPressure(void const * argument);
void StartReadMagnetometer(void const * argument);
void StartSendData(void const * argument);
void StartReceiveDelay(void const * argument);

/* USER CODE BEGIN PFP */
int _write(int file, char *ptr, int len);
void _SPI_receive_delay();
void _UART_send_sensor_data();
void _set_delay(const uint16_t new_delay);
uint8_t _sum(uint8_t *written_data);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len){
	HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, 10);
	return len;
}

void _SPI_receive_delay(){
	// TODO: find a way to set this up with an interrupt
	HAL_StatusTypeDef status = HAL_TIMEOUT;
	union{
		uint8_t uint8_delay[2];
		uint16_t uint16_delay;
	} new_delay;
	uint8_t read_counter = 0;

	printf("ReceiveDelay: Waiting for a new delay...\r");

	while(status != HAL_OK && read_counter < 2){

		status = HAL_SPI_Receive(&hspi1, &new_delay.uint8_delay[read_counter], sizeof(uint8_t), delay*2);

		if(status == HAL_OK)
			read_counter ++;
		else
			// Allow preemption
			osDelay(500);
	}

	printf("ReceiveDelay: Received new delay: %d\r", new_delay.uint16_delay);
	_set_delay(new_delay.uint16_delay);

}

uint8_t _sum(uint8_t *written_data){
	uint8_t sum = 0;

	for(uint8_t i=0; i<sizeof(written_data); i++)
		sum += written_data[i];

	return sum;
}

void _UART_send_sensor_data(){
	HAL_StatusTypeDef status;
	uint8_t *sensor_data_bytes = (uint8_t *)&sensor_data;

	printf("SendData: Sending data: \r");
	printf("SendData: Temperature = %.02f°C\r", sensor_data.temperature);
	printf("SendData: Humidity = %.02f%% \r", sensor_data.humidity);
	printf("SendData: Pressure = %.02f hPa\r", sensor_data.pressure);
	printf("SendData: North direction = %.02f°\r\r", sensor_data.north_direction);

	/* for(uint8_t i=0; i<sizeof(sensor_data_t); i++){
		status = HAL_TIMEOUT;
		while(status != HAL_OK){
			status = HAL_UART_Transmit(&huart4, sensor_data_bytes + i, sizeof(uint8_t), delay*2);
			printf(".");
		}
	}*/

	printf("\rSendData: Sensor data sent.\r");

	// reset of written_data
	for(uint8_t i=0; i<sizeof(written_data); i++)
		written_data[i] = 0;

	printf("SendData: Reset written_data counters.\r");

}

void _set_delay(const uint16_t new_delay){
	switch(new_delay){
		case 500:
			delay = FAST;
			break;
		case 1000:
			delay = MEDIUM;
			break;
		case 2500:
			delay = SLOW;
			break;
		case 5000:
			delay = VERY_SLOW;
			break;
		case 10000:
			delay = TAKE_A_BREAK;
			break;
		default:
			// Blink LED to say there was a problem, maybe transmission?
			HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
			HAL_Delay(500);
			HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
			break;
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
	written_data = malloc(sizeof(uint8_t) * 4);

	for(uint8_t i=0; i<sizeof(written_data); i++)
		written_data[i] = 0;

	delay = FAST;

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
  MX_DFSDM1_Init();
  MX_QUADSPI_Init();
  MX_USART1_UART_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_SPI1_Init();
  MX_UART4_Init();
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
  osThreadDef(defaultTask, StartDefaultTask, osPriorityIdle, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of readTemp */
  osThreadDef(readTemp, StartReadTemp, osPriorityNormal, 0, 128);
  readTempHandle = osThreadCreate(osThread(readTemp), NULL);

  /* definition and creation of readHum */
  osThreadDef(readHum, StartReadHum, osPriorityNormal, 0, 128);
  readHumHandle = osThreadCreate(osThread(readHum), NULL);

  /* definition and creation of readPress */
  osThreadDef(readPress, StartReadPressure, osPriorityNormal, 0, 128);
  readPressHandle = osThreadCreate(osThread(readPress), NULL);

  /* definition and creation of readMagneto */
  osThreadDef(readMagneto, StartReadMagnetometer, osPriorityNormal, 0, 128);
  readMagnetoHandle = osThreadCreate(osThread(readMagneto), NULL);

  /* definition and creation of sendData */
  osThreadDef(sendData, StartSendData, osPriorityNormal, 0, 128);
  sendDataHandle = osThreadCreate(osThread(sendData), NULL);

  /* definition and creation of receiveDelay */
  osThreadDef(receiveDelay, StartReceiveDelay, osPriorityNormal, 0, 128);
  receiveDelayHandle = osThreadCreate(osThread(receiveDelay), NULL);

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

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE|RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_MSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 40;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_UART4
                              |RCC_PERIPHCLK_DFSDM1|RCC_PERIPHCLK_USB;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Uart4ClockSelection = RCC_UART4CLKSOURCE_PCLK1;
  PeriphClkInit.Dfsdm1ClockSelection = RCC_DFSDM1CLKSOURCE_PCLK;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLLSAI1;
  PeriphClkInit.PLLSAI1.PLLSAI1Source = RCC_PLLSOURCE_MSI;
  PeriphClkInit.PLLSAI1.PLLSAI1M = 1;
  PeriphClkInit.PLLSAI1.PLLSAI1N = 24;
  PeriphClkInit.PLLSAI1.PLLSAI1P = RCC_PLLP_DIV7;
  PeriphClkInit.PLLSAI1.PLLSAI1Q = RCC_PLLQ_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1R = RCC_PLLR_DIV2;
  PeriphClkInit.PLLSAI1.PLLSAI1ClockOut = RCC_PLLSAI1_48M2CLK;
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
  /** Enable MSI Auto calibration
  */
  HAL_RCCEx_EnableMSIPLLMode();
}

/**
  * @brief DFSDM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_DFSDM1_Init(void)
{

  /* USER CODE BEGIN DFSDM1_Init 0 */

  /* USER CODE END DFSDM1_Init 0 */

  /* USER CODE BEGIN DFSDM1_Init 1 */

  /* USER CODE END DFSDM1_Init 1 */
  hdfsdm1_channel1.Instance = DFSDM1_Channel1;
  hdfsdm1_channel1.Init.OutputClock.Activation = ENABLE;
  hdfsdm1_channel1.Init.OutputClock.Selection = DFSDM_CHANNEL_OUTPUT_CLOCK_SYSTEM;
  hdfsdm1_channel1.Init.OutputClock.Divider = 2;
  hdfsdm1_channel1.Init.Input.Multiplexer = DFSDM_CHANNEL_EXTERNAL_INPUTS;
  hdfsdm1_channel1.Init.Input.DataPacking = DFSDM_CHANNEL_STANDARD_MODE;
  hdfsdm1_channel1.Init.Input.Pins = DFSDM_CHANNEL_FOLLOWING_CHANNEL_PINS;
  hdfsdm1_channel1.Init.SerialInterface.Type = DFSDM_CHANNEL_SPI_RISING;
  hdfsdm1_channel1.Init.SerialInterface.SpiClock = DFSDM_CHANNEL_SPI_CLOCK_INTERNAL;
  hdfsdm1_channel1.Init.Awd.FilterOrder = DFSDM_CHANNEL_FASTSINC_ORDER;
  hdfsdm1_channel1.Init.Awd.Oversampling = 1;
  hdfsdm1_channel1.Init.Offset = 0;
  hdfsdm1_channel1.Init.RightBitShift = 0x00;
  if (HAL_DFSDM_ChannelInit(&hdfsdm1_channel1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DFSDM1_Init 2 */

  /* USER CODE END DFSDM1_Init 2 */

}

/**
  * @brief QUADSPI Initialization Function
  * @param None
  * @retval None
  */
static void MX_QUADSPI_Init(void)
{

  /* USER CODE BEGIN QUADSPI_Init 0 */

  /* USER CODE END QUADSPI_Init 0 */

  /* USER CODE BEGIN QUADSPI_Init 1 */

  /* USER CODE END QUADSPI_Init 1 */
  /* QUADSPI parameter configuration*/
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 2;
  hqspi.Init.FifoThreshold = 4;
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_HALFCYCLE;
  hqspi.Init.FlashSize = 23;
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;
  if (HAL_QSPI_Init(&hqspi) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN QUADSPI_Init 2 */

  /* USER CODE END QUADSPI_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_SLAVE;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES_RXONLY;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_INPUT;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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
  huart4.Init.WordLength = UART_WORDLENGTH_9B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_EVEN;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  huart4.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart4.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

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
  huart1.Init.WordLength = UART_WORDLENGTH_9B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_EVEN;
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
  * @brief USB_OTG_FS Initialization Function
  * @param None
  * @retval None
  */
static void MX_USB_OTG_FS_PCD_Init(void)
{

  /* USER CODE BEGIN USB_OTG_FS_Init 0 */

  /* USER CODE END USB_OTG_FS_Init 0 */

  /* USER CODE BEGIN USB_OTG_FS_Init 1 */

  /* USER CODE END USB_OTG_FS_Init 1 */
  hpcd_USB_OTG_FS.Instance = USB_OTG_FS;
  hpcd_USB_OTG_FS.Init.dev_endpoints = 6;
  hpcd_USB_OTG_FS.Init.speed = PCD_SPEED_FULL;
  hpcd_USB_OTG_FS.Init.phy_itface = PCD_PHY_EMBEDDED;
  hpcd_USB_OTG_FS.Init.Sof_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.low_power_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.lpm_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.battery_charging_enable = DISABLE;
  hpcd_USB_OTG_FS.Init.use_dedicated_ep1 = DISABLE;
  hpcd_USB_OTG_FS.Init.vbus_sensing_enable = DISABLE;
  if (HAL_PCD_Init(&hpcd_USB_OTG_FS) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USB_OTG_FS_Init 2 */

  /* USER CODE END USB_OTG_FS_Init 2 */

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
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, M24SR64_Y_RF_DISABLE_Pin|M24SR64_Y_GPO_Pin|ISM43362_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, ARD_D10_Pin|SPBTLE_RF_RST_Pin|ARD_D9_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ARD_D8_Pin|ISM43362_BOOT0_Pin|ISM43362_WAKEUP_Pin|LED2_Pin
                          |SPSGRF_915_SDN_Pin|ARD_D5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, USB_OTG_FS_PWR_EN_Pin|PMOD_RESET_Pin|STSAFE_A100_RESET_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPBTLE_RF_SPI3_CSN_GPIO_Port, SPBTLE_RF_SPI3_CSN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, VL53L0X_XSHUT_Pin|LED3_WIFI__LED4_BLE_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(SPSGRF_915_SPI3_CSN_GPIO_Port, SPSGRF_915_SPI3_CSN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ISM43362_SPI3_CSN_GPIO_Port, ISM43362_SPI3_CSN_Pin, GPIO_PIN_SET);

  /*Configure GPIO pins : M24SR64_Y_RF_DISABLE_Pin M24SR64_Y_GPO_Pin ISM43362_RST_Pin ISM43362_SPI3_CSN_Pin */
  GPIO_InitStruct.Pin = M24SR64_Y_RF_DISABLE_Pin|M24SR64_Y_GPO_Pin|ISM43362_RST_Pin|ISM43362_SPI3_CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_OTG_FS_OVRCR_EXTI3_Pin SPSGRF_915_GPIO3_EXTI5_Pin SPBTLE_RF_IRQ_EXTI6_Pin ISM43362_DRDY_EXTI1_Pin */
  GPIO_InitStruct.Pin = USB_OTG_FS_OVRCR_EXTI3_Pin|SPSGRF_915_GPIO3_EXTI5_Pin|SPBTLE_RF_IRQ_EXTI6_Pin|ISM43362_DRDY_EXTI1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : BUTTON_EXTI13_Pin */
  GPIO_InitStruct.Pin = BUTTON_EXTI13_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BUTTON_EXTI13_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_A5_Pin ARD_A4_Pin ARD_A3_Pin ARD_A2_Pin
                           ARD_A1_Pin ARD_A0_Pin */
  GPIO_InitStruct.Pin = ARD_A5_Pin|ARD_A4_Pin|ARD_A3_Pin|ARD_A2_Pin
                          |ARD_A1_Pin|ARD_A0_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D10_Pin SPBTLE_RF_RST_Pin ARD_D9_Pin */
  GPIO_InitStruct.Pin = ARD_D10_Pin|SPBTLE_RF_RST_Pin|ARD_D9_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D4_Pin */
  GPIO_InitStruct.Pin = ARD_D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(ARD_D4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D3_Pin */
  GPIO_InitStruct.Pin = ARD_D3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ARD_D3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : ARD_D6_Pin */
  GPIO_InitStruct.Pin = ARD_D6_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG_ADC_CONTROL;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(ARD_D6_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D8_Pin ISM43362_BOOT0_Pin ISM43362_WAKEUP_Pin LED2_Pin
                           SPSGRF_915_SDN_Pin ARD_D5_Pin SPSGRF_915_SPI3_CSN_Pin */
  GPIO_InitStruct.Pin = ARD_D8_Pin|ISM43362_BOOT0_Pin|ISM43362_WAKEUP_Pin|LED2_Pin
                          |SPSGRF_915_SDN_Pin|ARD_D5_Pin|SPSGRF_915_SPI3_CSN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : INTERNAL_I2C2_SCL_Pin INTERNAL_I2C2_SDA_Pin */
  GPIO_InitStruct.Pin = INTERNAL_I2C2_SCL_Pin|INTERNAL_I2C2_SDA_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C2;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : INTERNAL_UART3_TX_Pin INTERNAL_UART3_RX_Pin */
  GPIO_InitStruct.Pin = INTERNAL_UART3_TX_Pin|INTERNAL_UART3_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : LPS22HB_INT_DRDY_EXTI0_Pin LSM6DSL_INT1_EXTI11_Pin ARD_D2_Pin HTS221_DRDY_EXTI15_Pin
                           PMOD_IRQ_EXTI12_Pin */
  GPIO_InitStruct.Pin = LPS22HB_INT_DRDY_EXTI0_Pin|LSM6DSL_INT1_EXTI11_Pin|ARD_D2_Pin|HTS221_DRDY_EXTI15_Pin
                          |PMOD_IRQ_EXTI12_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : USB_OTG_FS_PWR_EN_Pin SPBTLE_RF_SPI3_CSN_Pin PMOD_RESET_Pin STSAFE_A100_RESET_Pin */
  GPIO_InitStruct.Pin = USB_OTG_FS_PWR_EN_Pin|SPBTLE_RF_SPI3_CSN_Pin|PMOD_RESET_Pin|STSAFE_A100_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : VL53L0X_XSHUT_Pin LED3_WIFI__LED4_BLE_Pin */
  GPIO_InitStruct.Pin = VL53L0X_XSHUT_Pin|LED3_WIFI__LED4_BLE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : VL53L0X_GPIO1_EXTI7_Pin LSM3MDL_DRDY_EXTI8_Pin */
  GPIO_InitStruct.Pin = VL53L0X_GPIO1_EXTI7_Pin|LSM3MDL_DRDY_EXTI8_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : INTERNAL_SPI3_SCK_Pin INTERNAL_SPI3_MISO_Pin INTERNAL_SPI3_MOSI_Pin */
  GPIO_InitStruct.Pin = INTERNAL_SPI3_SCK_Pin|INTERNAL_SPI3_MISO_Pin|INTERNAL_SPI3_MOSI_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PMOD_SPI2_SCK_Pin */
  GPIO_InitStruct.Pin = PMOD_SPI2_SCK_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
  HAL_GPIO_Init(PMOD_SPI2_SCK_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : PMOD_UART2_CTS_Pin PMOD_UART2_RTS_Pin PMOD_UART2_TX_Pin PMOD_UART2_RX_Pin */
  GPIO_InitStruct.Pin = PMOD_UART2_CTS_Pin|PMOD_UART2_RTS_Pin|PMOD_UART2_TX_Pin|PMOD_UART2_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : ARD_D15_Pin ARD_D14_Pin */
  GPIO_InitStruct.Pin = ARD_D15_Pin|ARD_D14_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
	  osDelay(HAL_MAX_DELAY);
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
	// char *str_tmp = "Temperatura = %d.%02d°C\r";
	// char output_str[sizeof(str_tmp)];
	int tmpInt1, tmpInt2;
	float tmpFrac;
	uint32_t ret;

		do{
			 ret = BSP_TSENSOR_Init();

			if(ret == TSENSOR_ERROR){
				HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
				HAL_Delay(500);
				HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
			}

		}while(ret == TSENSOR_ERROR);

	printf("ReadTemp: Temperature sensor initialized.\r");
	/* Infinite loop */
	for(;;)
	{
		if(written_data[0] == 0){
			temp_value = BSP_TSENSOR_ReadTemp();
			tmpInt1 = temp_value;
			tmpFrac = temp_value - tmpInt1;
			tmpInt2 = trunc(tmpFrac * 100);
			// snprintf(output_str, sizeof(output_str), str_tmp, tmpInt1, tmpInt2);
			// HAL_UART_Transmit(&huart1, (uint8_t *)output_str, sizeof(str_tmp), 1000);
			printf("ReadTemp: Temperatura = %d.%02d°C\r", tmpInt1, tmpInt2);

			sensor_data.temperature = temp_value;
			written_data[0] = 1;

			// if(_sum(written_data) >= 4)
				// xTaskNotifyGive(sendDataHandle);
		}
		else
			printf("ReadTemp: written_data is still 1. Sleeping.\r");

		osDelay(delay);
	}
  /* USER CODE END StartReadTemp */
}

/* USER CODE BEGIN Header_StartReadHum */
/**
* @brief Function implementing the readHum thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReadHum */
void StartReadHum(void const * argument)
{
  /* USER CODE BEGIN StartReadHum */
	/* Infinite loop */
	float hum_value = 0;
	// char *str_hum = "Umidita' = %d.%02d\r";
	// char output_str[sizeof(str_hum)];
	int humInt1, humInt2;
	float humFrac;
	uint32_t ret;

		do{
			 ret = BSP_HSENSOR_Init();

			if(ret == HSENSOR_ERROR){
				HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
				HAL_Delay(500);
				HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
			}

		}while(ret == HSENSOR_ERROR);

	printf("ReadHum: Humidity sensor initialized.\r");
	/* Infinite loop */
	for(;;)
	{
		if(written_data[1] == 0){
			hum_value = BSP_HSENSOR_ReadHumidity();
			humInt1 = hum_value;
			humFrac = hum_value - humInt1;
			humInt2 = trunc(humFrac * 100);
			// snprintf(output_str, sizeof(output_str), str_hum, humInt1, humInt2);
			// HAL_UART_Transmit(&huart1, (uint8_t *)output_str, sizeof(str_hum), 1000);
			printf("ReadHum: Umidita' = %d.%02d\r", humInt1, humInt2);

			sensor_data.humidity = hum_value;
			written_data[1] = 1;

			// if(_sum(written_data) >= 4)
				// xTaskNotifyGive(sendDataHandle);
		}
		else
			printf("ReadHum: written_data is still 1. Sleeping.\r");

		osDelay(delay);
	}
  /* USER CODE END StartReadHum */
}

/* USER CODE BEGIN Header_StartReadPressure */
/**
* @brief Function implementing the readPress thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReadPressure */
void StartReadPressure(void const * argument)
{
  /* USER CODE BEGIN StartReadPressure */
	float pres_value = 0;
	// char *str_pres = "Pressione = %d.%02d hPa\r";
	// char output_str[sizeof(str_pres)];
	int presInt1, presInt2;
	float presFrac;
	uint32_t ret;

	do{
		 ret = BSP_PSENSOR_Init();

		if(ret == PSENSOR_ERROR){
			HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
			HAL_Delay(500);
			HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
		}

	}while(ret == PSENSOR_ERROR);

	printf("ReadPress: Pressure sensor initialized.\r");
	/* Infinite loop */
	for(;;)
	{
		if(written_data[2] == 0){
			pres_value = BSP_PSENSOR_ReadPressure();
			presInt1 = pres_value;
			presFrac = pres_value - presInt1;
			presInt2 = trunc(presFrac * 100);
			// snprintf(output_str, sizeof(output_str), str_pres, presInt1, presInt2);
			// HAL_UART_Transmit(&huart1, (uint8_t *)output_str, sizeof(str_pres), 1000);
			printf("ReadPress: Pressione = %d.%02d hPa\r", presInt1, presInt2);

			sensor_data.pressure = pres_value;
			written_data[2] = 1;

			// if(_sum(written_data) >= 4)
				// xTaskNotifyGive(sendDataHandle);
		}
		else
			printf("ReadPress: written_data is still 1. Sleeping.\r");

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
	// char *str_tmp = "Direzione del nord = %d.%02d°\r";
	// char output_str[sizeof(str_tmp)];
	double direction, magnFrac;
	int magnInt1, magnInt2;
	double declination_angle = 3.45;
	MAGNETO_StatusTypeDef ret;

	do{
		 ret = BSP_MAGNETO_Init();

		if(ret != MAGNETO_OK){
			HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_SET);
			HAL_Delay(500);
			HAL_GPIO_WritePin(GPIOB, LED2_Pin, GPIO_PIN_RESET);
		}

	}while(ret != MAGNETO_OK);

	printf("ReadMagn: Magnetometer initialized.\r");
	/* Infinite loop */
	for(;;)
	{
		if(written_data[3] == 0){
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
				// snprintf(output_str, sizeof(output_str), str_tmp, magnInt1, magnInt2);
				// HAL_UART_Transmit(&huart1, (uint8_t *)output_str, sizeof(str_tmp), 1000);
				printf("ReadMagn: Direzione del nord = %d.%02d°\r", magnInt1, magnInt2);

				sensor_data.north_direction = direction;
				written_data[3] = 1;

				// if(_sum(written_data) >= 4)
					// xTaskNotifyGive(sendDataHandle);
			}
		}
		else
			printf("ReadHum: written_data is still 1. Sleeping.\r");

		osDelay(delay);
	}
  /* USER CODE END StartReadMagnetometer */
}

/* USER CODE BEGIN Header_StartSendData */
/**
* @brief Function implementing the sendData thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartSendData */
void StartSendData(void const * argument)
{
  /* USER CODE BEGIN StartSendData */
	// const TickType_t xBlockTime = pdMS_TO_TICKS( HAL_MAX_DELAY );
  /* Infinite loop */
  for(;;)
  {
	  // printf("SendData: waiting for a notification...\r");
	  // ulTaskNotifyTake(pdTRUE, xBlockTime);
	  // printf("SendData: woke up.\r");
	  // if(_sum(written_data) >= 4)
		  // _UART_send_sensor_data();
	  if(_sum(written_data) >= 4)
		  _UART_send_sensor_data();
	  else
		  osDelay(delay);
  }
  /* USER CODE END StartSendData */
}

/* USER CODE BEGIN Header_StartReceiveDelay */
/**
* @brief Function implementing the receiveDelay thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartReceiveDelay */
void StartReceiveDelay(void const * argument)
{
  /* USER CODE BEGIN StartReceiveDelay */
  /* Infinite loop */
  for(;;)
  {
	  _SPI_receive_delay();
  }
  /* USER CODE END StartReceiveDelay */
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
     ex: printf("Wrong parameters value: file %s on line %d\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
