/*
 * OtherTasks.c
 *
 *  Created on: 13 déc. 2022
 *      Author: robot
 */



#include "main.h"


#include "cmsis_os.h"
#include "adc.h"
//#include "dma.h"
#include "i2c.h"
//#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "math.h"
#include "../Core/TempsMs.h"
#include "../../../../../Common/Src/LCD_VT100.h"
#include "Config.h"
#include "../Core/LowLevel.h"
#include "Tests.h"
#include "../Core/Odometrie.h"
#include "../Core/Sequenceur.h"
#include "../Core/spidma.h"
#include "../Core/CommandesServos.h"
#include "../Core/I2CUSCommun.h"
#include "../Core/Asserv.h"
#include "../Core/I2CMaitre.h"
#include "ServoDefs.h"
#include "Match.h"
#include "Menu.h"
#include "../Core/MotX.h"
#include "../Core/8Servos.h"

#include "OtherTasks.h"

TaskHandle_t hOtherTasks;

#define NBTASKSMAX 5
static void (*TASK[NBTASKSMAX])(void);


int LoadTask(void (*fct)(void))
{
	unsigned i;
	for(i=0; i<NBTASKSMAX; i++)
	{
		if(TASK[i]==0) {TASK[i]=fct; return 1;}
	}
	return 0;
}


int NbTasks(void)
{
	unsigned i,n;
	for(i=n=0; i<NBTASKSMAX; i++)
	{
		if(TASK[i]) n++;
	}
	return n;
}

void RT_OtherTasks(void *pvParameters)
{
	unsigned i;
	for(i=0; i<NBTASKSMAX; i++) TASK[i]=0;
	while(1)
	{
		DelaiMs(10);
		for(i=0; i<NBTASKSMAX; i++)
		{
			if(TASK[i]) {TASK[i](); TASK[i]=0;}
		}
	}
}
