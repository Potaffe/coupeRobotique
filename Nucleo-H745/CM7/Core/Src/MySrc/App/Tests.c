/*
 * Tests.c
 *
 *  Created on: May 25, 2016
 *      Author: Md
 */
//#include "stm32f7xx_hal.h"

#if defined STM32F7
#include "stm32f7xx_hal.h"
#elsif defined STM32H7
#include "stm32h7xx_hal.h"
#elsif defined STM32F4
#include "stm32f4xx_hal.h"
#elsif defined STM32L4
#include "stm32l4xx_hal.h"
#elsif defined STM32G4
#include "stm32g4xx_hal.h"
#endif



#include "tim.h"
#include "gpio.h"
#include "../Core/TempsMs.h"
#include "../../../../../Common/Src/LCD_VT100.h"
#include "Config.h"
#include "../Core/LowLevel.h"
#include "Tests.h"
#include "../Core/Odometrie.h"
#include "../Core/Sequenceur.h"
#include "../Core/CommandesServos.h"
#include "../Core/I2CMaitre.h"

#include "Match.h"
#include "Menu.h"
#include "RT-Main.h"
#include "../Core/8Servos.h"
#include "ServoDefs.h"

void TestCodeurs(void)
   {
   int16_t d, g;
   uint16_t ct=0;
   EnableAsserv(0);
   VTCtrl(1);
   lcdclrscr();
   while (1)
      {
      g = POS1CNT;
      d = POS2CNT;
      lcdgotoxy(1, 1);
      lcdprintf("G=%d\t", g);
      lcdgotoxy(1, 2);
      lcdprintf("D=%d\t", d);
      lcdgotoxy(1, 3); lcdprintf("%u\t",ct++);
      //DelaiMs(100);
      }
   }

void TestCodeurX(void)
   {
   int16_t x;
   uint16_t ct=0;
   VTCtrl(1);
   lcdclrscr();
   while (1)
      {
      x = POSXCNT;
      lcdgotoxy(1, 1);
      lcdprintf("X=%d\t", x);
      lcdgotoxy(1, 3); lcdprintf("%u\t",ct++);
      //DelaiMs(100);
      }
   }


void TestMotX(uint16_t vmax, uint16_t dv, uint16_t dt)
    {
    int16_t i;
    int16_t imax = vmax;
    EnableAsserv(0);
    _PowenX(1);
    while (1)
	{
	      for (i = 0; i <= imax; i += dv)
	         {
	         lcdgotoxy(1, 2);
	         lcdprintf("X : VitMot(%d)\t", i);
	         PWMX = VitMotX(i);
	         DelaiMs(dt);
	         }
	      for (i = imax; i >= -imax; i -= dv)
	         {
	         lcdgotoxy(1, 2);
	         lcdprintf("X : VitMot(%d)\t", i);
	         PWMX = VitMotX(i);
	         DelaiMs(dt);
	         }
	      for (i = -imax; i <= 0; i += dv)
	         {
	         lcdgotoxy(1, 2);
	         lcdprintf("X : VitMot(%d)\t", i);
	         PWMX = VitMotX(i);
	         DelaiMs(dt);
	         }
	}
    }


void TestMoteurs(uint16_t vmax, uint16_t dv, uint16_t dt)
   {
   int16_t i;
   int16_t imax = vmax;
   EnableAsserv(0);
   _Powen(1);
   while (1)
      {
      for (i = 0; i <= imax; i += dv)
         {
         lcdgotoxy(1, 2);
         lcdprintf("D : VitMot(%d)\t", i);
         PWMD = VitMotD(i);
         DelaiMs(dt);
         }
      for (i = imax; i >= -imax; i -= dv)
         {
         lcdgotoxy(1, 2);
         lcdprintf("D : VitMot(%d)\t", i);
         PWMD = VitMotD(i);
         DelaiMs(dt);
         }
      for (i = -imax; i <= 0; i += dv)
         {
         lcdgotoxy(1, 2);
         lcdprintf("D : VitMot(%d)\t", i);
         PWMD = VitMotD(i);
         DelaiMs(dt);
         }
      for (i = 0; i <= imax; i += dv)
         {
         lcdgotoxy(1, 2);
         lcdprintf("G : VitMot(%d)\t", i);
         PWMG = VitMotG(i);
         DelaiMs(dt);
         }
      for (i = imax; i >= -imax; i -= dv)
         {
         lcdgotoxy(1, 2);
         lcdprintf("G : VitMot(%d)\t", i);
         PWMG = VitMotG(i);
         DelaiMs(dt);
         }
      for (i = -imax; i <= 0; i += dv)
         {
         lcdgotoxy(1, 2);
         lcdprintf("G : VitMot(%d)\t", i);
         PWMG = VitMotG(i);
         DelaiMs(dt);
         }
      }
   }

void TestFrottementsSecs(uint16_t vmax, uint16_t dv, uint16_t dt)
   {
	EnableAsserv(0);
   int16_t i;
   _Powen(1);
   for (i = 0; i <= vmax; i += dv)
      {
      lcdgotoxy(1, 2);
      lcdprintf("Vit=%d\t", i);
      PWMD = VitMotD(i);
      PWMG = VitMotG(i);
      DelaiMs(dt);
      }
   _Powen(1);
   while (1)
      ;
   }

void TestFrotSecs(void)
   {
   TestFrottementsSecs(500, 10, 500);
   }

void TestDist(unsigned dist)
   {
   InitOdometrie(1000, 500, 900);
   _Powen(1);
   EnableAsserv(1);
   ParamLin(10000, 10);
   lcdgotoxy(1, 2);
   lcdprintf("X=%d Y=%d\t", P_X,P_Y);
   lcdgotoxy(1, 3);
   lcdprintf("A=%d\t", THETA);
   AvanceDe(dist);
   lcdgotoxy(1, 2);
   lcdprintf("X=%d Y=%d\t", P_X,P_Y);
   lcdgotoxy(1, 3);
   lcdprintf("A=%d\t", THETA);
   DelaiMs(2000);
   ReculeDe(dist);
   lcdgotoxy(1, 2);
   lcdprintf("X=%d Y=%d\t", P_X,P_Y);
   lcdgotoxy(1, 3);
   lcdprintf("A=%d\t", THETA);
   DelaiMs(1000);
   _Powen(0);
   while (1)
      ;
   }

void TestRot(unsigned ang)
   {
   InitOdometrie(1000, 500, 900);
   _Powen(1);
   EnableAsserv(1);
   ParamRot(5000, 10);
   lcdgotoxy(1, 2);
   lcdprintf("X=%d Y=%d\t", P_X,P_Y);
   lcdgotoxy(1, 3);
   lcdprintf("A=%d\t", THETA);
   TourneDroiteDe(ang);
   lcdgotoxy(1, 2);
   lcdprintf("X=%d Y=%d\t", P_X,P_Y);
   lcdgotoxy(1, 3);
   lcdprintf("A=%d\t", THETA);
   DelaiMs(5000);
   TourneGaucheDe(ang);
   lcdgotoxy(1, 2);
   lcdprintf("X=%d Y=%d\t", P_X,P_Y);
   lcdgotoxy(1, 3);
   lcdprintf("A=%d\t", THETA);
   DelaiMs(1000);
   _Powen(0);
   while (1)
      ;
   }

void TEST1M(void)
   {
   TestDist(1000);
   while (1)
      ;
   }

void TEST360(void)
   {
   TestRot(3600);
   while (1)
      ;
   }

void SPINBOT(void)
{
	InitOdometrie(1000, 500, 900);
	_Powen(1);
	EnableAsserv(1);
	ParamRot(20000, 20);
	while(1) TourneDroiteDe(3600);
}

void TESTPANNEAUX(void){
	while(1){
		ServoInitStd(SV_SOLAIREG, 200, 1023, 900, 10);
		ServoInitStd(SV_SOLAIRED, 200, 1023, 900, 10);
		ServoMoveWW(SV_SOLAIRED,SOLD_DEPLI);
		ServoMoveWW(SV_SOLAIREG,SOLG_DEPLI);
		ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_GAUCHE);
		ServoMoveWW(SV_SOLAIREG,SOLG_REPLI_DROITE);
		ServoMoveWW(SV_SOLAIRED,SOLD_DEPLI);
		ServoMoveWW(SV_SOLAIREG,SOLG_DEPLI);
		ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_DROITE);
		ServoMoveWW(SV_SOLAIREG,SOLG_REPLI_GAUCHE);

		ServoInitStd(SV_SOLAIREG, 254, 1023, 1023, 10);
		ServoInitStd(SV_SOLAIRED, 254, 1023, 1023, 10);
		ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_GAUCHE);
		ServoMoveWW(SV_SOLAIREG,SOLG_REPLI_DROITE);
		ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_DROITE);
		ServoMoveWW(SV_SOLAIREG,SOLG_REPLI_GAUCHE);
	}
}

void TestFusee(void)
   {
   void LanceFusee(void);
   RBT_TIME = -20;
   //LanceFusee();
   }

void DeploiMax(void)
{
	RangerBras();
	while (1){

	}
}

#include "../Core/I2CUSCommun.h"


void AffPeriphI2C(void)
   {
   uint16_t ad;
   VTCtrl(0);
   lcdprintf("Scan I2C...\n");
   //lcdprintf("TEST\n"); while(1) I2CPing(0x20);
   for(ad=5; ad<=125; ad++)
      {
      if(I2CPing(ad)) lcdprintf("I2C %2XH ok\n",ad);
      }
   lcdprintf("Terminé\n");
   }


/*
  void TestCaptCoul(void)
   {
   int r;
   CCLeds(1);
   while(1)
      {
      r=CCGetCol(1);
      lcdgotoxy(1,5);
      lcdprintf("C1=%d ",r);
      if(r>=0) lcdputs(StrCol(r));
      lcdputs("\t");

      r=CCGetCol(2);
      lcdgotoxy(1,6);
      lcdprintf("C2=%d ",r);
      if(r>=0) lcdputs(StrCol(r));
      lcdputs("\t");
      }
   }
*/

void Test5Servos(void)
   {
   unsigned i,x;
   while(1)
      {
      for(i=0; i<5; i++)
         {
         x=400; lcdgotoxy(1,2+i); lcdprintf("SV%u : %u\t",i+1,x); CCPosServo(i,x);
         DelaiMs(2000);
         x=600; lcdgotoxy(1,2+i); lcdprintf("SV%u : %u\t",i+1,x); CCPosServo(i,x);
         DelaiMs(2000);
         x=500; lcdgotoxy(1,2+i); lcdprintf("SV%u : %u\t",i+1,x); CCPosServo(i,x);
         }
      }
   }

static void AffDetect(void)
   {
   VTCtrl(1);
   lcdgotoxy(1, 10);
   while (1)
      {
      lcdgotoxy(1, 9);
      lcdprintf("X=%d   Y=%d  A=%d\t", P_X,P_Y,THETA);
      lcdgotoxy(1, 10);
      lcdprintf("DETECT=%d\t", GetDetect());
      }
   }

void USDistAV100(void)
   {
   InitOdometrie(0, 200, 900);
   Powen(0);
   /*
   USAV();
   USDist(100);
   */
   AffDetect();
   }

void USDistAV200(void)
   {
   InitOdometrie(0, 200, 900);
   Powen(0);
   /*
   USAV();
   USDist(200);
   */
   AffDetect();
   }

void USDistAR100(void)
   {
   InitOdometrie(0, 200, 900);
   Powen(0);
/*
   USAR();
   USDist(100);
   */
   AffDetect();
   }

void USDistAR200(void)
   {
   InitOdometrie(0, 200, 900);
   Powen(0);
/*
   USAR();
   USDist(200);
   */
   AffDetect();
   }

#if 0
int16_t I2CReadByte(uint8_t ad, uint8_t cmde);

unsigned GetUSDistAVD(void)
   {
   int16_t b;
   b = I2CReadByte(I2C_AD_US, CUSGetDistAVD);
   if (b < 0) return 0;
   return b * 10;
   }

unsigned GetUSDistAV(void)
   {
   int16_t b;
   b = I2CReadByte(I2C_AD_US, CUSGetDistAV);
   if (b < 0) return 0;
   return b * 10;
   }

unsigned GetUSDistAR(void)
   {
   int16_t b;
   b = I2CReadByte(I2C_AD_US, CUSGetDistAR);
   if (b < 0) return 0;
   return b * 10;
   }

unsigned GetUSDistAVG(void)
   {
   int16_t b;
   b = I2CReadByte(I2C_AD_US, CUSGetDistAVG);
   if (b < 0) return 0;
   return b * 10;
   }

unsigned GetUSDistARG(void)
   {
   int16_t b;
   b = I2CReadByte(I2C_AD_US, CUSGetDistARG);
   if (b < 0) return 0;
   return b * 10;
   }

unsigned GetUSDistARD(void)
   {
   int16_t b;
   b = I2CReadByte(I2C_AD_US, CUSGetDistARD);
   if (b < 0) return 0;
   return b * 10;
   }
#endif

void TESTAFFUS(void)
   {

   VTCtrl(1);
   lcdclrscr();
   InitOdometrie(0, 200, 900);
   Powen(0);
/*
   USDist(200);
   USAlt(2);
   USAVAR();
   while (1)
      {
      int davd, davg, dard, darg;
      davd = GetUSDistAVD();
      davg = GetUSDistAVG();
      dard = GetUSDistARD();
      darg = GetUSDistARG();
      lcdgotoxy(1, 1);
      lcdprintf("X=%d   Y=%d  A=%d\t", P_X,P_Y,THETA);
      lcdgotoxy(1, 3);
      if (davg)
         lcdprintf("US AVG=%ucm\t", davg / 10);
      else lcdprintf("US AVG=####\t");
      lcdgotoxy(1, 4);
      if (davd)
         lcdprintf("US AVD=%ucm\t", davd / 10);
      else lcdprintf("US AVD=####\t");
      lcdgotoxy(1, 5);
      if (darg)
         lcdprintf("US ARG=%ucm\t", darg / 10);
      else lcdprintf("US ARG=####\t");
      lcdgotoxy(1, 6);
      if (dard)
         lcdprintf("US ARD=%ucm\t", dard / 10);
      else lcdprintf("US ARD=####\t");
//       Odometrie();
      }
*/
   }


void TestIOnum(void)
   {
   VTCtrl(1);
   lcdclrscr();
   lcdprintf("Capteurs num�riques...");
   //PoignetLeve(0);
   //Pince(100);
   while(1)
     {
//     lcdgotoxy(1,2);lcdprintf("G=%u M=%u D=%u\t",_Proxi_Gauche,_Proxi_Milieu,_Proxi_Droit);
//     lcdgotoxy(1,3); lcdprintf("Pince=%u\t",_Capt_Pince);
//     lcdgotoxy(1,4); lcdprintf("Viseur=%u\t",_Detect_Viseur);
     }
   }

void TestADC(void)
   {

//HAL_ADC_GetValue(&hadc1);
   }

//Mesures de temps

static uint32_t TPS;

void InitMesTemps(void)
   {
   HAL_TIM_Base_Start(&htim2);
   }

//Temps en us
uint32_t GetTps(void)
   {
   return htim2.Instance->CNT - TPS;
   }

void StartTps(void)
   {
   TPS = htim2.Instance->CNT;
   }

uint32_t TpsToMs(uint32_t n)
   {
   return (uint32_t) (n / 1E3f);
   }

uint32_t TpsToUs(uint32_t n)
   {
   return (uint32_t) (n);
   }

uint32_t TpsToNs(uint32_t n)
   {
   return (uint32_t) (n / 1E-3f);
   }

