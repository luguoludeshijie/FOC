/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
// #include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
#include "App.h"
/* USER CODE BEGIN Includes */


/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define Motor_Start_Pin GPIO_PIN_13
#define Motor_Start_GPIO_Port GPIOC
#define Hall_U_Pin GPIO_PIN_0
#define Hall_U_GPIO_Port GPIOA
#define Hall_V_Pin GPIO_PIN_1
#define Hall_V_GPIO_Port GPIOA
#define Hall_W_Pin GPIO_PIN_2
#define Hall_W_GPIO_Port GPIOA
#define SCL_Pin GPIO_PIN_0
#define SCL_GPIO_Port GPIOD
#define SDA_Pin GPIO_PIN_1
#define SDA_GPIO_Port GPIOD
#define LED_Pin GPIO_PIN_0
#define LED_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */
#define AS5600_ADDR       0x6C
#define AS5600_WRITE      0x00
#define AS5600_READ       0x01
#define AS5600_ANGLE_H    0x0E
#define AS5600_ANGLE_L    0x0F

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
