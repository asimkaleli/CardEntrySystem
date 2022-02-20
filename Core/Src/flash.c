/*
 * flash.c
 *
 *  Created on: Dec 29, 2021
 *      Author: ASIM
 */

#include "flash.h"
#include "string.h"

#define FLASH_USER_START_ADDR   0x0800FC00   /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR     0x08010000   /* End @ of user Flash area */
#define TEMP_BUFFER_SIZE		100

extern Card_t CardFlash[MAX_ID_NUMBER] __attribute__((section (".dataSection")));
extern Card_t Card[MAX_ID_NUMBER];

uint32_t Address = 0, PAGEError = 0;
static FLASH_EraseInitTypeDef EraseInitStruct;


void SaveDataToFlash(void)
{
	uint32_t tempBuffer[sizeof(Card)/4 +1] = {0};

	memcpy(tempBuffer, Card, sizeof(tempBuffer));

	HAL_FLASH_Unlock();

	  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
	  EraseInitStruct.PageAddress = FLASH_USER_START_ADDR;
	  EraseInitStruct.NbPages     = 1;

	if (HAL_FLASHEx_Erase(&EraseInitStruct, &PAGEError) != HAL_OK)
	{
		__NOP();
	}

	  Address = FLASH_USER_START_ADDR;

	for (int i = 0; i < sizeof(tempBuffer) / 4; i++)
	{
		if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, Address, tempBuffer[i]) == HAL_OK)
		{
			Address = Address + 4;
		}
	}

	  HAL_FLASH_Lock();
}

void ReadDataFromFlash(void)
{
	memcpy(Card,CardFlash,sizeof(Card));
}
