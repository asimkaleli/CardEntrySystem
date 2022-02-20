/*
 * buzzerControl.h
 *
 *  Created on: Dec 26, 2021
 *      Author: ASIM
 */

#ifndef INC_BUZZERCONTROL_H_
#define INC_BUZZERCONTROL_H_

#include "main.h"

typedef enum
{
	BM_ONE_TIME,
	BM_TWICE_TIME,
	BM_THIRD_TIME,
	BM_INFINITY,
	BM_CYCLE,
	BM_OFF,
}BuzzerMode_t;

typedef struct
{
	uint32_t StartTime;
	uint32_t ActiveTime;
	BuzzerMode_t Mode;
	uint8_t Active;
	GPIO_TypeDef* GPIO_Port;
	GPIO_PinState PinState;
	uint32_t GPIO_Pin;
}Buzzer_t;

void BuzzerInit(Buzzer_t *buzzer);
void BuzzerControl(uint32_t activeTime, BuzzerMode_t buzzerMode);
void BuzzerUpdate(void);

#endif /* INC_BUZZERCONTROL_H_ */
