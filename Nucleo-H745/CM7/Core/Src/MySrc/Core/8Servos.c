/*
 * CaptCol.c
 *
 *  Created on: 21 oct. 2019
 *      Author: deloi
 */
#ifdef STM32H745xx
#include "stm32h7xx_hal.h"
#endif

#ifdef STM32F722xx
#include "stm32f7xx_hal.h"
#endif

#include "I2CMaitre.h"
#include "../App/Config.h"
#include "8Servos.h"

enum {CCLNone,CCLReq,CCLPosSv};


void CCPosServo(uint8_t n, uint16_t pos)
   {
   uint8_t t[3];
   uint16_t x;
   if(n>5) return;
   if(pos>1000) return;
   t[0]=CCLPosSv;
   x=pos|(n<<12);
   t[1]=x;
   t[2]=x>>8;
   I2CWriteBytes(I2CAD_SERVO,t,sizeof(t));
   }



