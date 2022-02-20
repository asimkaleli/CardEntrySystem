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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "RC-522.h"
#include "buzzerControl.h"
#include "ledControl.h"
#include "flash.h"
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
CAN_HandleTypeDef hcan;

SPI_HandleTypeDef hspi2;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t rxArrivedFlag;
uint32_t rxArrivedTime;
uint8_t wrongAccess;
uint8_t receivedWrongAccess;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);
static void MX_SPI2_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

typedef struct
{
	uint32_t TxSentBytes;
	uint32_t TxPackageSize;
	uint32_t RxReceivedBytes;
	uint32_t RxPackageSize;
	uint8_t RxData;
	uint8_t RxReceivedId;
	uint8_t TxBuffer[UART_BUF_SIZE];
	uint8_t RxBuffer[UART_BUF_SIZE];
	uint8_t HandShake;
} UartCom_t;

typedef struct
{
	uint32_t StartTime;
	uint32_t ActiveTime;
	uint8_t Open;
}DoorSwitch_t;

DoorSwitch_t Door;

UartCom_t Uart1;
UartCom_t Uart2;

Card_t Card[MAX_ID_NUMBER];
Card_t CardFlash[MAX_ID_NUMBER] __attribute__((section (".dataSection")));

Card_t ReceivedCard;
DeviceStatus_t TypeOfProcess = DS_NONE;

DeviceStatus_t DeviceStatus;
DeviceStatus_t OldDeviceStatus;
DeviceZone_t DeviceZone;

uint8_t status;
uint8_t str[MAX_LEN]; // Max_LEN = 16
uint8_t serNum[5];

CAN_TxHeaderTypeDef TxHeader;
CAN_RxHeaderTypeDef RxHeader;

uint32_t TxMailbox;

uint8_t TxData[8];
uint8_t RxData[8];

Card_t tempCard =
{ 0 };

uint8_t ProcessCardID(DeviceStatus_t status, uint8_t *receivedID, uint8_t cardIndex);
uint8_t CheckUart(uint32_t PackageSize, const uint8_t *RxBuffer);
uint8_t ReaderControl(Card_t *card, uint8_t *newSerialNumber);
void DoorControl(uint32_t activeTime);
/**
 * @brief  Period elapsed callback in non-blocking mode
 * @param  htim TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if (htim == &htim3)
	{
		BuzzerUpdate();
		LedUpdate();

		if (HAL_GetTick() - rxArrivedTime > RX_ARRIVED_TIMEOUT)
		{
			rxArrivedFlag = 0;
		}

		if(Door.ActiveTime)
		{

		}

		if (Door.ActiveTime)
		{
			Door.ActiveTime--;
			if (Door.Open == 0)
			{
				Door.Open = 1;
				HAL_GPIO_WritePin(DOOR_GPIO_Port, DOOR_Pin, GPIO_PIN_SET);
			}
		}
		else
		{
			Door.Open = 0;
			HAL_GPIO_WritePin(DOOR_GPIO_Port, DOOR_Pin, GPIO_PIN_RESET);
		}

		if (TypeOfProcess != DS_NONE)
		{
			uint8_t cardIndex = ReaderControl(Card, ReceivedCard.ID);

			switch (TypeOfProcess) {

			case DS_RUNNING:

				if (cardIndex != CARD_NOT_SAVED)
				{
					ProcessCardID(DS_RUNNING, ReceivedCard.ID, cardIndex);
				}

				TypeOfProcess = DS_NONE;
				break;

			case DS_SAVE_VIP_CARD:

				if (cardIndex == CARD_NOT_SAVED)
				{
				ProcessCardID(DS_SAVE_VIP_CARD, ReceivedCard.ID, cardIndex);
				}

				TypeOfProcess = DS_NONE;
				break;

			case DS_SAVE_NORMAL_CARD:

				if (cardIndex == CARD_NOT_SAVED)
				{
					ProcessCardID(DS_SAVE_NORMAL_CARD, ReceivedCard.ID, cardIndex);
				}

				TypeOfProcess = DS_NONE;
				break;

			case DS_REMOVE_CARD:

				if (cardIndex != CARD_NOT_SAVED)
				{
					ProcessCardID(DS_REMOVE_CARD, ReceivedCard.ID, cardIndex);
				}

				TypeOfProcess = DS_NONE;
				break;
			default:
				break;
			}
		}
	}
}

//uint32_t flag;
///**
// * @brief  Rx FIFO 0 message pending callback.
// * @param  hcan pointer to a CAN_HandleTypeDef structure that contains
// *         the configuration information for the specified CAN.
// * @retval None
// */
//void HAL_CAN_RxFifo1MsgPendingCallback(CAN_HandleTypeDef *hcan)
//{
//	HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO1, &RxHeader, RxData);
//	if (RxHeader.DLC == 2)
//	{
//		flag = 1;
//	}
//}

uint8_t WhichCardEmpty(Card_t *card)
{
	int i;

	for (i = 0; i < MAX_ID_NUMBER; i++)
	{
		if (Card[i].Position == P_NOT_SAVED_CARD)
		{
			return i;
		}
	}

	if (i < MAX_ID_NUMBER)
	{
		return CARD_NOT_SAVED;
	} else
	{
		return MAX_ID_NUMBER_OVERFLOWED;
	}
}

void DeviceStatusUpdate(void)
{

	uint8_t switch1, switch2, switch3, switch4, temp, deviceStatus, deviceZone;

	switch1 = HAL_GPIO_ReadPin(SWITCH_1_GPIO_Port, SWITCH_1_Pin);
	switch2 = HAL_GPIO_ReadPin(SWITCH_2_GPIO_Port, SWITCH_2_Pin);
	switch3 = HAL_GPIO_ReadPin(SWITCH_3_GPIO_Port, SWITCH_3_Pin);
	switch4 = HAL_GPIO_ReadPin(SWITCH_4_GPIO_Port, SWITCH_4_Pin);

	temp = (uint8_t) (switch4 << 3 | switch3 << 2 | switch2 << 1 | switch1);
	deviceStatus = (~temp) & 0x03;
	deviceZone = ((~temp) & 0x0C) >> 2;

	switch (deviceStatus) {
	case DS_RUNNING:
		DeviceStatus = DS_RUNNING;
		if (OldDeviceStatus != DeviceStatus)
		{
			LedResetActiveState();
			LedControl(OFF, C_ALL, 0, LM_INFINITY);
		}

		break;
	case DS_SAVE_VIP_CARD:
		DeviceStatus = DS_SAVE_VIP_CARD;
		if (OldDeviceStatus != DeviceStatus)
		{
			LedResetActiveState();
			LedControl(ON, C_BLUE, 0, LM_INFINITY);
		}

		break;
	case DS_SAVE_NORMAL_CARD:
		DeviceStatus = DS_SAVE_NORMAL_CARD;
		if (OldDeviceStatus != DeviceStatus)
		{
			LedResetActiveState();
			LedControl(ON, C_YELLOW, 0, LM_INFINITY);
		}
		break;
	case DS_REMOVE_CARD:
		DeviceStatus = DS_REMOVE_CARD;
		if (OldDeviceStatus != DeviceStatus)
		{
			LedResetActiveState();
			LedControl(ON, C_WHITE, 0, LM_INFINITY);
		}

		break;
	default:
		break;
	}

	OldDeviceStatus = DeviceStatus;

	switch (deviceZone) {
	case DZ_OUTSIDE:
		DeviceZone = DZ_OUTSIDE;
		break;
	case DZ_OUTSIDE_MID:
		DeviceZone = DZ_OUTSIDE_MID;
		break;
	case DZ_VIP_ROOM:
		DeviceZone = DZ_VIP_ROOM;
		break;
	case DZ_VIP_ROOM_MID:
		DeviceZone = DZ_VIP_ROOM_MID;
		break;
	default:
		break;
	}
}

//return 1 saved Card
uint8_t ReaderControl(Card_t *card, uint8_t *newSerialNumber)
{
	for (int i = 0; i < MAX_ID_NUMBER; i++)
	{
		if (!memcmp(card[i].ID, newSerialNumber, SER_NUM_BYTE_CNT))
		{
			return i;
		}
	}
	return CARD_NOT_SAVED;
}

/**
 * @brief  Rx Transfer completed callbacks.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if (huart == &huart1)
	{
		HAL_UART_Receive_IT(&huart1, &Uart1.RxData, 1);

		switch (Uart1.RxReceivedBytes) {
		case 0:
			if (Uart1.RxData == UART_LABEL1)
			{
				Uart1.RxBuffer[Uart1.RxReceivedBytes++] = Uart1.RxData;
			} else
			{
				Uart1.RxReceivedBytes = 0;
				return;
			}
			break;
		case 1:
			if (Uart1.RxData == UART_LABEL2)
			{
				Uart1.RxBuffer[Uart1.RxReceivedBytes++] = Uart1.RxData;
			} else
			{
				Uart1.RxReceivedBytes = 0;
				return;
			}
			break;
		case 2:
			if (Uart1.RxData == UART_LABEL3)
			{
				Uart1.RxBuffer[Uart1.RxReceivedBytes++] = Uart1.RxData;
			} else
			{
				Uart1.RxReceivedBytes = 0;
				return;
			}
			break;
		default:
			if (Uart1.RxReceivedBytes <= Uart1.RxPackageSize)
			{
				Uart1.RxBuffer[Uart1.RxReceivedBytes++] = Uart1.RxData;
				if (Uart1.RxReceivedBytes > Uart1.RxPackageSize)
				{
					Uart1.RxReceivedBytes = 0;
					if (CheckUart(Uart1.RxPackageSize, Uart1.RxBuffer))
					{

						if (rxArrivedFlag == 0)
						{
							rxArrivedFlag = 1;
							rxArrivedTime = HAL_GetTick();

							memcpy(ReceivedCard.ID, &Uart1.RxBuffer[3], SER_NUM_BYTE_CNT);
							TypeOfProcess = (DeviceStatus_t) Uart1.RxBuffer[8];
							ReceivedCard.Zone = Uart1.RxBuffer[9];
							ReceivedCard.Position = Uart1.RxBuffer[10];

							receivedWrongAccess = Uart1.RxBuffer[14];

							if (receivedWrongAccess && (DeviceZone == DZ_OUTSIDE || DeviceZone == DZ_VIP_ROOM))
							{
								DoorControl(0);
							}

							HAL_UART_Transmit_IT(&huart2, Uart1.RxBuffer, Uart1.TxPackageSize + 1);
						}
					}
				}
			}
			break;
		}
	}

	if (huart == &huart2)
	{
		HAL_UART_Receive_IT(&huart2, &Uart2.RxData, 1);

		switch (Uart2.RxReceivedBytes) {
		case 0:
			if (Uart2.RxData == UART_LABEL1)
			{
				Uart2.RxBuffer[Uart2.RxReceivedBytes++] = Uart2.RxData;
			} else
			{
				Uart2.RxReceivedBytes = 0;
				return;
			}
			break;
		case 1:
			if (Uart2.RxData == UART_LABEL2)
			{
				Uart2.RxBuffer[Uart2.RxReceivedBytes++] = Uart2.RxData;
			} else
			{
				Uart2.RxReceivedBytes = 0;
				return;
			}
			break;
		case 2:
			if (Uart2.RxData == UART_LABEL3)
			{
				Uart2.RxBuffer[Uart2.RxReceivedBytes++] = Uart2.RxData;
			} else
			{
				Uart2.RxReceivedBytes = 0;
				return;
			}
			break;
		default:
			if (Uart2.RxReceivedBytes <= Uart2.RxPackageSize)
			{
				Uart2.RxBuffer[Uart2.RxReceivedBytes++] = Uart2.RxData;
				if (Uart2.RxReceivedBytes > Uart2.RxPackageSize)
				{
					Uart2.RxReceivedBytes = 0;
					if (CheckUart(Uart2.RxPackageSize, Uart2.RxBuffer))
					{

						if (rxArrivedFlag == 0)
						{
							rxArrivedFlag = 1;
							rxArrivedTime = HAL_GetTick();

							memcpy(ReceivedCard.ID, &Uart2.RxBuffer[3], SER_NUM_BYTE_CNT);
							TypeOfProcess = (DeviceStatus_t) Uart2.RxBuffer[8];
							ReceivedCard.Zone = Uart2.RxBuffer[9];
							ReceivedCard.Position = Uart2.RxBuffer[10];

							receivedWrongAccess = Uart2.RxBuffer[14];

							if (receivedWrongAccess && (DeviceZone == DZ_OUTSIDE || DeviceZone == DZ_VIP_ROOM))
							{
								DoorControl(0);
							}

							HAL_UART_Transmit_IT(&huart1, Uart2.RxBuffer, Uart2.TxPackageSize + 1);
						}
					}
				}
			}
			break;
		}
	}
}

uint8_t CheckUart(uint32_t PackageSize, const uint8_t *RxBuffer)
{
	uint32_t i;
	uint8_t CheckByte;
	uint8_t ReturnValue = 0;

	CheckByte = RxBuffer[0];
	for (i = 1; i < PackageSize; i++)
		CheckByte ^= RxBuffer[i];

	if (CheckByte == RxBuffer[PackageSize])
	{
		ReturnValue = 1;
	}

	return ReturnValue;
}

void PrepareUartPackage(UartCom_t *uart, Card_t *card, DeviceStatus_t typeOfProcess, uint8_t TransmitID)
{

	uart->HandShake = 0;

	uart->TxBuffer[0] = UART_LABEL1;
	uart->TxBuffer[1] = UART_LABEL2;
	uart->TxBuffer[2] = UART_LABEL3;

	memcpy(&uart->TxBuffer[3], card->ID, SER_NUM_BYTE_CNT);
	uart->TxBuffer[8] = (uint8_t) typeOfProcess;
	uart->TxBuffer[9] = card->Zone;
	uart->TxBuffer[10] = card->Position;
	if(wrongAccess)
	{
		uart->TxBuffer[14] = 1;
	}
	else
	{
		uart->TxBuffer[14] = 0;
	}
	uart->TxBuffer[15] = TransmitID;
	uart->TxBuffer[uart->TxPackageSize] = uart->TxBuffer[0];

	for (int i = 1; i < uart->TxPackageSize; i++)
	{
		uart->TxBuffer[uart->TxPackageSize] = uart->TxBuffer[uart->TxPackageSize] ^ uart->TxBuffer[i];
	}
}

uint8_t ProcessCardID(DeviceStatus_t status, uint8_t *receivedID, uint8_t cardIndex)
{

	uint8_t emptyCardIndex = 0;
	uint8_t returnValue = 0;

	switch (status) {

	case DS_RUNNING:

		Card[cardIndex].Position = ReceivedCard.Position;
		Card[cardIndex].Zone = ReceivedCard.Zone;

		if((ReceivedCard.Zone == CZ_OUTSIDE && DeviceZone == DZ_OUTSIDE)
				|| (ReceivedCard.Zone == CZ_VIP_ROOM && DeviceZone == DZ_VIP_ROOM))
		{
			if (receivedWrongAccess)
			{
				receivedWrongAccess = 0;
				DoorControl(0);
			} else
			{
				DoorControl(Door.ActiveTime + 5000);
			}

		}

		returnValue = 1;
		break;

	case DS_SAVE_VIP_CARD:

		emptyCardIndex = WhichCardEmpty(Card);

		if (emptyCardIndex != MAX_ID_NUMBER_OVERFLOWED)
		{
			memcpy(&Card[emptyCardIndex].ID, receivedID, 5);	//function for c language:(para1:that place save data,para2:the the source of data,para3:size)
			Card[emptyCardIndex].Position = P_VIP;

			if (rxArrivedFlag)
			{
				Card[emptyCardIndex].Zone = ReceivedCard.Zone;
			} else
			{
				if (DeviceZone == DZ_OUTSIDE)
				{
					Card[emptyCardIndex].Zone = CZ_OUTSIDE;
				} else if (DeviceZone == DZ_OUTSIDE_MID || DeviceZone == DZ_VIP_ROOM_MID)
				{
					Card[emptyCardIndex].Zone = CZ_MID;
				} else
				{
					Card[emptyCardIndex].Zone = CZ_VIP_ROOM;
				}
			}

			SaveDataToFlash();

			returnValue = 1;
		}

		break;

	case DS_SAVE_NORMAL_CARD:

		emptyCardIndex = WhichCardEmpty(Card);

		if (emptyCardIndex != MAX_ID_NUMBER_OVERFLOWED)
		{
			memcpy(&Card[emptyCardIndex].ID, receivedID, 5);	//function for c language:(para1:that place save data,para2:the the source of data,para3:size)
			Card[emptyCardIndex].Position = P_NORMAL;

			if (rxArrivedFlag)
			{
				Card[emptyCardIndex].Zone = ReceivedCard.Zone;
			} else
			{
				if (DeviceZone == DZ_OUTSIDE)
				{
					Card[emptyCardIndex].Zone = CZ_OUTSIDE;
				} else if (DeviceZone == DZ_OUTSIDE_MID || DeviceZone == DZ_VIP_ROOM_MID)
				{
					Card[emptyCardIndex].Zone = CZ_MID;
				} else
				{
					Card[emptyCardIndex].Zone = CZ_VIP_ROOM;
				}
			}

			SaveDataToFlash();

			returnValue = 1;
		}

		break;

	case DS_REMOVE_CARD:

		memset(&Card[cardIndex], 0, sizeof(Card[cardIndex]));

		SaveDataToFlash();

		returnValue = 1;

		break;
	default:
		returnValue = 0;
		break;
	}

	return returnValue;
}

void SendData(DeviceStatus_t deviceStatus, uint8_t cardIndex)
{
	PrepareUartPackage(&Uart1, &Card[cardIndex], deviceStatus, DATA);
	for (int i = 0; i < 5; i++)
	{
		HAL_UART_Transmit_IT(&huart1, Uart1.TxBuffer, Uart1.TxPackageSize + 1);
		HAL_Delay(25);
	}

	PrepareUartPackage(&Uart2, &Card[cardIndex], deviceStatus, DATA);
	for (int i = 0; i < 5; i++)
	{
		HAL_UART_Transmit_IT(&huart2, Uart2.TxBuffer, Uart2.TxPackageSize + 1);
		HAL_Delay(25);
	}
}

uint8_t CardControl(Card_t * card)
{

	switch (card->Position) {
	case P_VIP:

		if ((DeviceZone == DZ_OUTSIDE && card->Zone == CZ_OUTSIDE)
				|| (DeviceZone == DZ_OUTSIDE_MID && card->Zone == CZ_MID)
				|| (DeviceZone == DZ_VIP_ROOM_MID && card->Zone == CZ_MID)
				|| (DeviceZone == DZ_VIP_ROOM && card->Zone == CZ_VIP_ROOM))
		{

			if (DeviceZone == DZ_OUTSIDE)
			{
				card->Zone = CZ_MID;
			} else if (DeviceZone == DZ_OUTSIDE_MID)
			{
				card->Zone = CZ_OUTSIDE;
			} else if (DeviceZone == DZ_VIP_ROOM)
			{
				card->Zone = CZ_MID;
			} else if (DeviceZone == DZ_VIP_ROOM_MID)
			{
				card->Zone = CZ_VIP_ROOM;
			}

			return 1;
		}

		return 0;
		break;

	case P_NORMAL:

		if ((DeviceZone == DZ_OUTSIDE && card->Zone == CZ_OUTSIDE)
				|| (DeviceZone == DZ_OUTSIDE_MID && card->Zone == CZ_MID))
		{
			if (DeviceZone == DZ_OUTSIDE)
			{
				card->Zone = CZ_MID;
			} else if (DeviceZone == DZ_OUTSIDE_MID)
			{
				card->Zone = CZ_OUTSIDE;
			}

			return 1;
		}
		return 0;

		break;
	default:
		return 0;
		break;
	}
}

void DoorControl(uint32_t activeTime)
{

	if (activeTime < 10000)
	{
		Door.ActiveTime = activeTime;
	} else
	{
		Door.ActiveTime = 10000;
	}

	Door.StartTime = HAL_GetTick();
}


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

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
  MX_CAN_Init();
  MX_SPI2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

#ifdef DEBUG
	HAL_Delay(1000);
#endif

	ReadDataFromFlash();

	Buzzer_t Buzzer;
	Buzzer.GPIO_Port = BUZZER_GPIO_Port;
	Buzzer.GPIO_Pin = BUZZER_Pin;
	BuzzerInit(&Buzzer);

	Led_t Led;
	Led.Red_GPIO_Port = RED_LED_GPIO_Port;
	Led.Red_GPIO_Pin = RED_LED_Pin;
	Led.Green_GPIO_Port = GREEN_LED_GPIO_Port;
	Led.Green_GPIO_Pin = GREEN_LED_Pin;
	Led.Blue_GPIO_Port = BLUE_LED_GPIO_Port;
	Led.Blue_GPIO_Pin = BLUE_LED_Pin;
	LedInit(&Led);

	for (int i = 0; i < 5; i++)
	{
		MFRC522_Init();
	}

	HAL_TIM_Base_Start_IT(&htim3);

	Uart1.RxPackageSize = 16;
	Uart1.TxPackageSize = 16;

	Uart2.RxPackageSize = 16;
	Uart2.TxPackageSize = 16;

	HAL_UART_Receive_IT(&huart1, &Uart1.RxData, 1);
	HAL_UART_Receive_IT(&huart2, &Uart2.RxData, 1);

//  HAL_CAN_Start(&hcan);
//  HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO1_MSG_PENDING);
//
//  TxHeader.DLC = 2;
//  TxHeader.ExtId = 0;
//  TxHeader.IDE = CAN_ID_STD;
//  TxHeader.RTR = CAN_RTR_DATA;
//  TxHeader.StdId = 0x103;
//  TxHeader.TransmitGlobalTime = DISABLE;
//
//  TxData[0] = 100;
//  TxData[1] = 10;
//
//  if (HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox) != HAL_OK)
//  {
//	  Error_Handler();
//  }

	uint8_t Index = 0;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		DeviceStatusUpdate();

		status = MFRC522_Request(PICC_REQIDL, str);	//MFRC522_Request(0x26, str)
		status = MFRC522_Anticoll(str);	//Take a collision, look up 5 bytes

		if (status == MI_OK)
		{

			switch (DeviceStatus) {
			case DS_RUNNING:

				Index = ReaderControl(Card, &str[0]);

				if (Index == CARD_NOT_SAVED)
				{
					BuzzerControl(1000, BM_ONE_TIME);
					LedControl(ON, C_RED, 1000, LM_ONE_TIME);

					if (DeviceZone == DZ_OUTSIDE || DeviceZone == DZ_VIP_ROOM)
					{
						DoorControl(0);
					}

					wrongAccess = 1;

					SendData(DS_RUNNING, Index); //Datasend the bus

					wrongAccess = 0;

					break;
				}

				switch (Card[Index].Position) {
				case P_VIP:

					if (CardControl(&Card[Index]))
					{
						BuzzerControl(500, BM_ONE_TIME);
						LedControl(ON, C_GREEN, 500, LM_ONE_TIME);
						if(DeviceZone == DZ_OUTSIDE || DZ_VIP_ROOM)
						{
							DoorControl(Door.ActiveTime + 5000);
						}

						SendData(DS_RUNNING, Index); //Datasend the bus

					} else
					{
						if (DeviceZone == DZ_OUTSIDE || DeviceZone == DZ_VIP_ROOM)
						{
							DoorControl(0);
						}

						wrongAccess = 1;

						BuzzerControl(1000, BM_ONE_TIME);
						LedControl(ON, C_RED, 1000, LM_ONE_TIME);

						SendData(DS_RUNNING, Index); //Datasend the bus
						wrongAccess = 0;
					}

					break;

				case P_NORMAL:

					if (CardControl(&Card[Index]))
					{
						BuzzerControl(500, BM_ONE_TIME);
						LedControl(ON, C_GREEN, 500, LM_ONE_TIME);

						if (DeviceZone == DZ_OUTSIDE || DeviceZone == DZ_VIP_ROOM)
						{
							DoorControl(Door.ActiveTime + 5000);
						}

						SendData(DS_RUNNING, Index);

					} else
					{

						if (DeviceZone == DZ_OUTSIDE || DeviceZone == DZ_VIP_ROOM)
						{
							DoorControl(0);
						}

						wrongAccess = 1;

						BuzzerControl(1000, BM_ONE_TIME);
						LedControl(ON, C_RED, 1000, LM_ONE_TIME);

						SendData(DS_RUNNING, Index); //Datasend the bus
						wrongAccess = 0;
					}

					break;
				default:
					break;

				}

				break;
			case DS_SAVE_VIP_CARD:

				Index = ReaderControl(Card, &str[0]);

				if (Index == CARD_NOT_SAVED)
				{
					if (ProcessCardID(DS_SAVE_VIP_CARD, str, Index))
					{
						BuzzerControl(500, BM_ONE_TIME);
					} else
					{
						BuzzerControl(250, BM_TWICE_TIME);
					}
				} else
				{
					BuzzerControl(250, BM_TWICE_TIME);
				}

				Index = ReaderControl(Card, &str[0]);
				SendData(DS_SAVE_VIP_CARD, Index);

				break;

			case DS_SAVE_NORMAL_CARD:

				Index = ReaderControl(Card, &str[0]);
				if (Index == CARD_NOT_SAVED)
				{
					if (ProcessCardID(DS_SAVE_NORMAL_CARD, str, Index))
					{
						BuzzerControl(500, BM_ONE_TIME);
					} else
					{
						BuzzerControl(250, BM_TWICE_TIME);
					}
				} else
				{
					BuzzerControl(250, BM_TWICE_TIME);
				}

				Index = ReaderControl(Card, &str[0]);
				SendData(DS_SAVE_NORMAL_CARD, Index);

				break;

			case DS_REMOVE_CARD:

				Index = ReaderControl(Card, &str[0]);

				if (Index != CARD_NOT_SAVED)
				{
					BuzzerControl(500, BM_ONE_TIME);

					SendData(DS_REMOVE_CARD, Index);

					ProcessCardID(DS_REMOVE_CARD, str,Index);
				} else
				{
					BuzzerControl(250, BM_THIRD_TIME);

					memcpy(tempCard.ID, str, SER_NUM_BYTE_CNT);

					PrepareUartPackage(&Uart1, &tempCard, DS_REMOVE_CARD, DATA);
					for (int i = 0; i < 5; i++)
					{
						HAL_UART_Transmit_IT(&huart1, Uart1.TxBuffer, Uart1.TxPackageSize + 1);
						HAL_Delay(25);
					}

					PrepareUartPackage(&Uart2, &tempCard, DS_REMOVE_CARD, DATA);
					for (int i = 0; i < 5; i++)
					{
						HAL_UART_Transmit_IT(&huart2, Uart2.TxBuffer, Uart2.TxPackageSize + 1);
						HAL_Delay(25);
					}
				}

				break;
			default:
				break;
			}
			HAL_Delay(1000);
		}
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 18;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_2TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */
	CAN_FilterTypeDef canConfigFilter;
	canConfigFilter.FilterActivation = CAN_FILTER_ENABLE;
	canConfigFilter.FilterBank = 10;
	canConfigFilter.FilterFIFOAssignment = CAN_RX_FIFO1;
	canConfigFilter.FilterIdHigh = 0;
	canConfigFilter.FilterIdLow = 0x0000;
	canConfigFilter.FilterMaskIdHigh = 0;
	canConfigFilter.FilterMaskIdLow = 0x0000;
	canConfigFilter.FilterMode = CAN_FILTERMODE_IDMASK;
	canConfigFilter.FilterScale = CAN_FILTERSCALE_32BIT;
	canConfigFilter.SlaveStartFilterBank = 0;

	HAL_CAN_ConfigFilter(&hcan, &canConfigFilter);
  /* USER CODE END CAN_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 10-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 7200-1;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

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
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, RED_LED_Pin|GREEN_LED_Pin|BLUE_LED_Pin|SPI2_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SPI2_RESET_Pin|DOOR_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SWITCH_3_Pin SWITCH_2_Pin SWITCH_1_Pin SWITCH_4_Pin */
  GPIO_InitStruct.Pin = SWITCH_3_Pin|SWITCH_2_Pin|SWITCH_1_Pin|SWITCH_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : RED_LED_Pin GREEN_LED_Pin BLUE_LED_Pin SPI2_CS_Pin */
  GPIO_InitStruct.Pin = RED_LED_Pin|GREEN_LED_Pin|BLUE_LED_Pin|SPI2_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : SPI2_RESET_Pin */
  GPIO_InitStruct.Pin = SPI2_RESET_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(SPI2_RESET_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : DOOR_Pin */
  GPIO_InitStruct.Pin = DOOR_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(DOOR_GPIO_Port, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

