/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */
#define MAX_ID_NUMBER				10
#define MAX_ID_NUMBER_OVERFLOWED	0xFE
#define SER_NUM_BYTE_CNT			5
#define UART_BUF_SIZE				128

#define UART_LABEL1     		0xFE
#define UART_LABEL2     		0xFF
#define UART_LABEL3     		0xFB

#define HANDSHAKE				0xAA
#define DATA					0x00
#define CARD_NOT_SAVED			0xFF

#define RX_ARRIVED_TIMEOUT		200 //ms

typedef enum
{
	DZ_OUTSIDE, DZ_OUTSIDE_MID, DZ_VIP_ROOM_MID, DZ_VIP_ROOM,
} DeviceZone_t;

typedef enum
{
	CZ_OUTSIDE, CZ_MID, CZ_VIP_ROOM
} CardZone_t;

typedef enum
{
	P_NOT_SAVED_CARD, P_NORMAL, P_VIP,
} Position_t;

typedef enum
{
	DS_RUNNING, DS_SAVE_VIP_CARD, DS_SAVE_NORMAL_CARD, DS_REMOVE_CARD, DS_NONE = 0xFF,
} DeviceStatus_t;

typedef struct __attribute__((__packed__))
{
	uint8_t ID[SER_NUM_BYTE_CNT]; //4byte ID + 1 byte checksum
	CardZone_t Zone;
	Position_t Position;
} Card_t;
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
#define LED_Pin GPIO_PIN_13
#define LED_GPIO_Port GPIOC
#define SWITCH_3_Pin GPIO_PIN_4
#define SWITCH_3_GPIO_Port GPIOA
#define SWITCH_2_Pin GPIO_PIN_5
#define SWITCH_2_GPIO_Port GPIOA
#define SWITCH_1_Pin GPIO_PIN_6
#define SWITCH_1_GPIO_Port GPIOA
#define SWITCH_4_Pin GPIO_PIN_7
#define SWITCH_4_GPIO_Port GPIOA
#define RED_LED_Pin GPIO_PIN_0
#define RED_LED_GPIO_Port GPIOB
#define GREEN_LED_Pin GPIO_PIN_1
#define GREEN_LED_GPIO_Port GPIOB
#define BLUE_LED_Pin GPIO_PIN_10
#define BLUE_LED_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_11
#define BUZZER_GPIO_Port GPIOB
#define SPI2_CS_Pin GPIO_PIN_12
#define SPI2_CS_GPIO_Port GPIOB
#define SPI2_RESET_Pin GPIO_PIN_8
#define SPI2_RESET_GPIO_Port GPIOA
#define DOOR_Pin GPIO_PIN_10
#define DOOR_GPIO_Port GPIOA
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
