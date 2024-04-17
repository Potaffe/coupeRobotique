/*
 * I2CMaitre.c
 *
 *  Created on: 23 nov. 2016
 *      Author: Md
 */
#ifdef STM32H745xx
#include "stm32h7xx_hal.h"
#endif

#ifdef STM32F722xx
#include "stm32f7xx_hal.h"
#endif

#include "i2c.h"
#include "I2CMaitre.h"
#include "../App/Config.h"

#define CxReq 1

uint16_t I2CPing(uint8_t ad)
{
	return HAL_I2C_IsDeviceReady(&hi2c,ad<<1,2,2)==HAL_OK;
}

uint16_t I2CWriteByte(uint8_t ad, uint8_t b)
{
	return HAL_I2C_Master_Transmit(&hi2c,ad<<1,&b,1,2);
}



uint16_t I2CWriteBytes(uint8_t ad, uint8_t *b, uint16_t nb)
{
	return HAL_I2C_Master_Transmit(&hi2c,ad<<1,b,nb,2);
}

static union
{
	uint16_t w;
	struct
	{
		uint8_t cmd,req;
	}b;
}adx={.b={.req=CxReq}};


int16_t I2CReadByte(uint8_t ad, uint8_t cmde)
{
    HAL_StatusTypeDef err;
    uint8_t rep;
    adx.b.cmd=cmde;
    err=HAL_I2C_Mem_Read(&hi2c,ad<<1,adx.w,2,&rep,1,2);
    if(err!=HAL_OK) return -err;
    return rep;
}

int16_t I2CReadBytes(uint8_t ad, uint8_t cmde, uint8_t *data, uint16_t nb)
{
    adx.b.cmd=cmde;
    return HAL_I2C_Mem_Read(&hi2c,ad<<1,adx.w,2,data,nb,5);
}




