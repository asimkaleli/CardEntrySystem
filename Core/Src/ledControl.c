/*
 * ledControl.c
 *
 *  Created on: Dec 26, 2021
 *      Author: ASIM
 */

#include "ledControl.h"


void LedONorOFF(GPIO_PinState onOff, LedColor_t ledColor);

Led_t Led;

void LedUpdate(void){

	static uint8_t lightChangeCnt = 0;

	switch (Led.Mode) {
		case LM_ONE_TIME:

			LedONorOFF(Led.PinState, Led.Color);

			if(HAL_GetTick() - Led.StartTime > Led.ActiveTime)
			{
				Led.Mode = LM_OFF;
			}
			break;

		case LM_TWICE_TIME:

			LedONorOFF(Led.PinState, Led.Color);

			if(HAL_GetTick() - Led.StartTime > Led.ActiveTime)
			{
				lightChangeCnt++;

				if(Led.PinState == GPIO_PIN_SET)
				{
					Led.PinState = GPIO_PIN_RESET;;
				}
				else
				{
					Led.PinState = GPIO_PIN_SET;
				}

				Led.StartTime = HAL_GetTick();
			}

			if(lightChangeCnt == 3)
			{
				Led.Mode = LM_OFF;
				lightChangeCnt = 0;
			}
			break;

		case LM_THIRD_TIME:

			LedONorOFF(Led.PinState, Led.Color);

			if(HAL_GetTick() - Led.StartTime > Led.ActiveTime)
			{
				lightChangeCnt++;

				if(Led.PinState == GPIO_PIN_SET)
				{
					Led.PinState = GPIO_PIN_RESET;;
				}
				else
				{
					Led.PinState = GPIO_PIN_SET;
				}

				Led.StartTime = HAL_GetTick();
			}

			if(lightChangeCnt == 5)
			{
				Led.Mode = LM_OFF;
				lightChangeCnt = 0;
			}
			break;

		case LM_INFINITY:

			LedONorOFF(Led.PinState, Led.Color);

			Led.Active = 0;

			break;

		case LM_CYCLE:

			LedONorOFF(Led.PinState, Led.Color);

			if(HAL_GetTick() - Led.StartTime > Led.ActiveTime)
			{
				Led.StartTime = HAL_GetTick();

				if(Led.PinState == GPIO_PIN_SET)
				{
					Led.PinState = GPIO_PIN_RESET;;
				}
				else
				{
					Led.PinState = GPIO_PIN_SET;
				}
			}

			Led.Active = 0;

			break;

		case LM_OFF:
			LedONorOFF(OFF, C_ALL);
			Led.Active = 0;
			break;
		default:
			break;
	}
}

void LedONorOFF(GPIO_PinState onOff, LedColor_t ledColor)
{
	switch (ledColor) {
		case C_RED:
			HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, onOff);
			break;

		case C_GREEN:
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, onOff);
			break;

		case C_BLUE:
			HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, onOff);
			break;
		case C_YELLOW:
			HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, onOff);
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, onOff);
			break;
		case C_ALL:
		case C_WHITE:
			HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, onOff);
			HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, onOff);
			HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, onOff);
			break;
		default:
			break;
	}
}

void LedControl(GPIO_PinState onOff, LedColor_t ledColor, uint32_t activeTime, LightMode_t lightMode)
{
	if(Led.Active == 0)
	{
		HAL_GPIO_WritePin(RED_LED_GPIO_Port, RED_LED_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(GREEN_LED_GPIO_Port, GREEN_LED_Pin, GPIO_PIN_RESET);
		HAL_GPIO_WritePin(BLUE_LED_GPIO_Port, BLUE_LED_Pin, GPIO_PIN_RESET);

		Led.PinState = onOff;
		Led.Color = ledColor;
		Led.StartTime = HAL_GetTick();
		Led.Mode = lightMode;
		Led.ActiveTime = activeTime;
		Led.Active = 1;
	}
}

void LedInit(Led_t *led){
	Led.Red_GPIO_Port = led->Red_GPIO_Port;
	Led.Red_GPIO_Pin = led->Red_GPIO_Pin;

	Led.Green_GPIO_Port = led->Green_GPIO_Port;
	Led.Green_GPIO_Pin = led->Green_GPIO_Pin;

	Led.Blue_GPIO_Port = led->Blue_GPIO_Port;
	Led.Blue_GPIO_Pin = led->Blue_GPIO_Pin;
}

void LedResetActiveState(void)
{
	Led.Active = 0;
}
