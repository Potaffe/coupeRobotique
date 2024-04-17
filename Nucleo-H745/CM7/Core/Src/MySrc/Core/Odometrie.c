#ifdef STM32H745xx
#include "stm32h7xx_hal.h"
#endif

#ifdef STM32F722xx
#include "stm32f7xx_hal.h"
#endif

#include "Asserv.h"
#include "Odometrie.h"
#include "../App/Config.h"
#include "TempsMs.h"
#include "../App/Menu.h"


#include <math.h>


static float fTHETA,fP_X,fP_Y;



#define M_PI2f 6.283185307179586476925286766559f

static struct
   {
   uint16_t nbg,nbd;  // mémorisation positions codeurs
   }ODO;


void InitOdometrie(int16_t x, int16_t y, int16_t th)
   {
   P_X=x; P_Y=y; THETA=th;
   fP_X=P_X; fP_Y=P_Y; fTHETA=th*((float)(PI/1800.));
   ODO.nbd=POS2CNT;
   ODO.nbg=POS1CNT;
   DelaiMs(5);
   }

#define TEST_ODO 0

void Odometrie(void)
   {
	uint16_t g,d;
	int16_t dg,dd;
   float da,dp;
   float T;
#if TEST_ODO
   static unsigned ct=0;
   ct++;
   if(ct==1000)
      {
	  ct=0;
	  P_X+=100; P_Y+=50; THETA+=90;
	  if(P_X>=1500) P_X=-1500;
	  if(P_Y>=2000) P_Y=0;
	  if(THETA>=3600) THETA-=3600;
      }
   return;
#endif
   g=POS1CNT;
   d=POS2CNT;
   if((d==ODO.nbd) && (g==ODO.nbg)) return;
   dg=g-ODO.nbg;
   dd=d-ODO.nbd;
   ODO.nbg=g;
   ODO.nbd=d;
   da=(dd-dg)*(KDIM/INTER_ROUE_LIBRE);
   if(SYM_AUTO && MIROIR) da=-da;
   dp=(dd+dg)*(KDIM*0.5f);
   T=fTHETA+da*0.5f;
   fP_X += dp*cosf(T);
   fP_Y += dp*sinf(T); // en mm
   fTHETA+=da;
   if(fTHETA>=M_PI2f) fTHETA-=M_PI2f; //mise en forme 0--360deg
   else if(fTHETA<0.f) fTHETA+=M_PI2f;
   P_X=fP_X+0.5f; P_Y=fP_Y+0.5f; THETA=fTHETA*((float)(1800./PI))+0.5f;
   }



