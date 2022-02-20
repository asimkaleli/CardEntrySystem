/*
 * buzzerControl.c
 *
 *  Created on: Dec 26, 2021
 *      Author: ASIM
 */

#include "buzzerControl.h"

Buzzer_t Buzzer;

void BuzzerUpdate(void)
{

	static uint8_t buzzerChangeCnt = 0;

	switch (Buzzer.Mode) {
	case BM_ONE_TIME:

		HAL_GPIO_WritePin(Buzzer.GPIO_Port, Buzzer.GPIO_Pin, Buzzer.PinState); //on

		if (HAL_GetTick() - Buzzer.StartTime > Buzzer.ActiveTime)
		{
			Buzzer.Mode = BM_OFF;
		}
		break;

	case BM_TWICE_TIME:

		HAL_GPIO_WritePin(Buzzer.GPIO_Port, Buzzer.GPIO_Pin, Buzzer.PinState); //on

		if (HAL_GetTick() - Buzzer.StartTime > Buzzer.ActiveTime)
		{
			buzzerChangeCnt++;

			if (Buzzer.PinState == GPIO_PIN_SET)
			{
				Buzzer.PinState = GPIO_PIN_RESET;
				;
			} else
			{
				Buzzer.PinState = GPIO_PIN_SET;
			}

			Buzzer.StartTime = HAL_GetTick();
		}

		if (buzzerChangeCnt == 3)
		{
			Buzzer.Mode = BM_OFF;
			buzzerChangeCnt = 0;
		}
		break;

	case BM_THIRD_TIME:

		HAL_GPIO_WritePin(Buzzer.GPIO_Port, Buzzer.GPIO_Pin, Buzzer.PinState); //on

		if (HAL_GetTick() - Buzzer.StartTime > Buzzer.ActiveTime)
		{
			buzzerChangeCnt++;

			if (Buzzer.PinState == GPIO_PIN_SET)
			{
				Buzzer.PinState = GPIO_PIN_RESET;
				;
			} else
			{
				Buzzer.PinState = GPIO_PIN_SET;
			}

			Buzzer.StartTime = HAL_GetTick();
		}

		if (buzzerChangeCnt == 5)
		{
			Buzzer.Mode = BM_OFF;
			buzzerChangeCnt = 0;
		}
		break;

	case BM_INFINITY:

		HAL_GPIO_WritePin(Buzzer.GPIO_Port, Buzzer.GPIO_Pin, GPIO_PIN_RESET); //on

		break;

	case BM_CYCLE:

		HAL_GPIO_WritePin(Buzzer.GPIO_Port, Buzzer.GPIO_Pin, Buzzer.PinState); //on

		if (HAL_GetTick() - Buzzer.StartTime > Buzzer.ActiveTime)
		{
			Buzzer.StartTime = HAL_GetTick();

			if (Buzzer.PinState == GPIO_PIN_SET)
			{
				Buzzer.PinState = GPIO_PIN_RESET;
			} else
			{
				Buzzer.PinState = GPIO_PIN_SET;
			}
		}

		break;

	case BM_OFF:
		HAL_GPIO_WritePin(Buzzer.GPIO_Port, Buzzer.GPIO_Pin, GPIO_PIN_SET); //off
		Buzzer.Active = 0;
		break;
	default:
		break;
	}
}

void BuzzerControl(uint32_t activeTime, BuzzerMode_t buzzerMode)
{
	if (Buzzer.Active == 0)
	{
		Buzzer.ActiveTime = activeTime;
		Buzzer.PinState = GPIO_PIN_RESET;
		Buzzer.StartTime = HAL_GetTick();
		Buzzer.Mode = buzzerMode;
		Buzzer.Active = 1;
	}
}

void BuzzerInit(Buzzer_t *buzzer)
{
	Buzzer.GPIO_Port = buzzer->GPIO_Port;
	Buzzer.GPIO_Pin = buzzer->GPIO_Pin;
}
