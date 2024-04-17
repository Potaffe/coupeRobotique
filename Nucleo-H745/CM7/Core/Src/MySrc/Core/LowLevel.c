/*
 * LowLevel.c
 *
 *  Created on: May 27, 2016
 *      Author: Md
 */

#ifdef STM32H745xx
#include "stm32h7xx_hal.h"
#endif

#ifdef STM32F722xx
#include "stm32f7xx_hal.h"
#endif

#include "tim.h"
#include "gpio.h"
#include "TempsMs.h"
#include "../../../../Common/Src/LCD_VT100.h"
#include "../App/Config.h"
#include "LowLevel.h"
#include "spidma.h"
#include "../../../../Common/Src/SharedMemory.h"

volatile float V_BAT;

unsigned VitMotG(int16_t v)
    // Renvoie la valeur � mettre dans les regs P1DCx pour
    //  obtenir la vitesse v (-1000 <= v <= +1000)
{
   if((V_BAT>5.f)&&(V_BAT<20.f)) v=v*(VREF/V_BAT); //Compensation de la variation de la tension batterie
   if(v>1000) v=1000;
   else if(v<-1000) v=-1000;
   if(INV_MOTG) v=-v;
   v+=1000;
   v=PWMmin+v*(PWMmax-PWMmin)/2000;
   return v;
}

unsigned VitMotX(int16_t v)
    // Renvoie la valeur � mettre dans les regs P1DCx pour
    //  obtenir la vitesse v (-1000 <= v <= +1000)
{
   if((V_BAT>5.f)&&(V_BAT<20.f)) v=v*(VREF/V_BAT); //Compensation de la variation de la tension batterie
   if(v>1000) v=1000;
   else if(v<-1000) v=-1000;
   if(INV_MOTX) v=-v;
   v+=1000;
   v=PWMmin+v*(PWMmax-PWMmin)/2000;
   return v;
}


unsigned VitMotD(int16_t v)
    // Renvoie la valeur � mettre dans les regs P1DCx pour
    //  obtenir la vitesse v (-1000 <= v <= +1000)
{
   if((V_BAT>5.f)&&(V_BAT<20.f)) v=v*(VREF/V_BAT); //Compensation de la variation de la tension batterie
   if(v>1000) v=1000;
   else if(v<-1000) v=-1000;
   if(INV_MOTD) v=-v;
   v+=1000;
   v=PWMmin+v*(PWMmax-PWMmin)/2000;
   return v;
}


void SetVitMotG(int16_t v)
{
	PWMG=VitMotG(v);
}

void SetVitMotD(int16_t v)
{
	PWMD=VitMotD(v);
}

void SetVitMotX(int16_t v)
{
	PWMX=VitMotX(v);
}


void SetVitMotGD(int16_t vg, int16_t vd)
{
	unsigned g,d;
	g=VitMotG(vg);
	d=VitMotD(vd);
	PWMG=g;
	PWMD=d;
}

//1300 : Levé
//2100 : Rangé
void LeveDrapeau(uint16_t p)
{
	p=(p?1300:2100);
	//((&htimFlag)->Instance->CCR1)=p;
}

void Powen(int16_t st)
{
	_Powen(st);
}

void VitTurbine(float pourcent)
{
	if(pourcent<0.f)pourcent=0.f;
	if(pourcent>100.f)pourcent=100.f;
	(&htimPWM)->Instance->CCR3=(PWMmax*0.01f)*pourcent;
}

void PowTurbine(int16_t st)
{
	_PowTurbine(st);
}

int GetDetect(void)
{
   return BAL_EMERGENCY;
	//return _CAPTUS;
}

