/**
  ******************************************************************************
  * @file    Wifi/WiFi_HTTP_Server/Inc/main.h 
  * @author  MCD Application Team
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "wifi.h"
#include "stm32f413h_discovery.h"
#include "stm32f413h_discovery_lcd.h"  

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
#define ADCx                             ADC3
#define ADCx_CLK_ENABLE()               __HAL_RCC_ADC3_CLK_ENABLE()
#define ADCx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOC_CLK_ENABLE()

#define ADCx_FORCE_RESET()              __HAL_RCC_ADC_FORCE_RESET()
#define ADCx_RELEASE_RESET()            __HAL_RCC_ADC_RELEASE_RESET()

/* Definition for ADCx Channel Pin */
#define ADCx_CHANNEL_PIN                GPIO_PIN_10
#define ADCx_CHANNEL_GPIO_PORT          GPIOF

/* Definition for ADCx's Channel */
#define ADCx_CHANNEL                    ADC_CHANNEL_8
#define SAMPLINGTIME                    ADC_SAMPLETIME_3CYCLES

#define TEMP_REFRESH_PERIOD       1000    /* Internal temperature refresh period */
#define MAX_CONVERTED_VALUE       4095    /* Max converted value */
#define AMBIENT_TEMP                25    /* Ambient Temperature */
#define VSENS_AT_AMBIENT_TEMP      760    /* VSENSE value (mv) at ambient temperature */
#define AVG_SLOPE                   25    /* Avg_Solpe multiply by 10 */
#define VREF                      3300

/* ADC handler declaration */

/* Exported macro ------------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
