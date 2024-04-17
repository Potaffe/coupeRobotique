/*
 * spidma.c
 *
 *  Created on: Jun 9, 2016
 *      Author: Md
 */

#ifdef STM32H745xx
#include "stm32h7xx_hal.h"
#endif

#ifdef STM32F722xx
#include "stm32f7xx_hal.h"
#endif

//#include "spi.h"
#include "Odometrie.h"
#include "../App/Config.h"
//#include "spidma.h"
#include "../../../../Common/Src/SharedMemory.h"

volatile unsigned NBSendSPI,NBCallBackSPI,NBErrSPI;

void SendSpi(void)
{
   //HAL_SPI_TransmitReceive_DMA(&hspi4, (uint8_t *) (&SharedUpData), (uint8_t *)(&SharedDnData), sizeof(SharedDnData)/2);
   NBSendSPI++;
}


void InitSpiDma(void)
{
}

#if 0
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef * hspi)
   {
 	   if(hspi==&hspi4)
	      {
	      NBCallBackSPI++;
	      }
   }


void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *spi)
{
if(spi==&hspi4)
	{
		   NBErrSPI++;
		//_LED_ORANGE(0);
		//_LED_ROUGE(1);
	}
}
#endif

