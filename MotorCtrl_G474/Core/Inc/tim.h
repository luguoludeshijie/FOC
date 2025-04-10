/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    tim.h
  * @brief   This file contains all the function prototypes for
  *          the tim.c file
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
#ifndef __TIM_H__
#define __TIM_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

extern TIM_HandleTypeDef htim1;

extern TIM_HandleTypeDef htim5;

extern TIM_HandleTypeDef htim6;

/* USER CODE BEGIN Private defines */

#define PWM_FREQ      ((uint16_t)20000)
#define PWM_fullPer   ((uint16_t)1700)
#define PWM_HalfPer   ((uint16_t)850)

/* USER CODE END Private defines */

void MX_TIM1_Init(void);
void MX_TIM5_Init(void);
void MX_TIM6_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* USER CODE BEGIN Prototypes */
void Set_PwmTrgAdcStart(void);
void Set_PwmTrgAdcStop(void);
void Set_PwmStart(void);
void Set_PwmStop(void);
void Set_PWMValue(uint16_t *PwmValue);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __TIM_H__ */

