/*
 * ledControl.h
 *
 *  Created on: Dec 26, 2021
 *      Author: ASIM
 */

#ifndef INC_LEDCONTROL_H_
#define INC_LEDCONTROL_H_

#include "main.h"

#define ON 				GPIO_PIN_SET
#define OFF 			GPIO_PIN_RESET

typedef enum
{
	LM_ONE_TIME,
	LM_TWICE_TIME,
	LM_THIRD_TIME,
	LM_INFINITY,
	LM_CYCLE,
	LM_OFF,
}LightMode_t;

typedef enum
{
	C_ALL,
	C_WHITE,
	C_RED,
	C_GREEN,
	C_BLUE,
	C_YELLOW,
}LedColor_t;

typedef struct
{
	GPIO_TypeDef* Red_GPIO_Port;
	GPIO_TypeDef* Green_GPIO_Port;
	GPIO_TypeDef* Blue_GPIO_Port;
	uint32_t Red_GPIO_Pin;
	uint32_t Green_GPIO_Pin;
	uint32_t Blue_GPIO_Pin;
	uint32_t PinState;
	uint32_t StartTime;
	uint32_t ActiveTime;
	LightMode_t Mode;
	LedColor_t Color;
	uint8_t Active;
}Led_t;

void LedInit(Led_t *led);
void LedControl(GPIO_PinState onOff, LedColor_t ledColor, uint32_t activeTime, LightMode_t lightMode);
void LedUpdate(void);
void LedResetActiveState(void);

#endif /* INC_LEDCONTROL_H_ */
