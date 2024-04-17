/*
 * RT-Main.c
 *
 *  Created on: 19 août 2017
 *      Author: Md
 */

#include "main.h"

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


//#include "stm32f7xx_hal.h"
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

#include "Communic.h"




#include "RT-Main.h"

TaskHandle_t hMain;

//define dans le .h


void Coucou(int n, uint16_t d)
   {
   int i;
   for(i=0; i<n; i++)
      {
	  _LEDJ(0);
      _LEDR(1);
      DelaiMs(d);
      _LEDR(0);
      //_LEDV(1); DelaiMs(d); _LEDV(0);
      _LEDJ(1);
      DelaiMs(d);
      }
   _LEDJ(0);
   }



void DbgServoMoveW(uint16_t id, uint16_t pos)
{
   ServoMoveW(id,pos);
   lcdprintf("MoveW %s %u : %s\n",GetSvName(id),pos,GetSvErrorMsg());
}


#if 0
static volatile uint16_t TLR, TLV, TLO, TLB;
void GestLeds(void)
   {
   if(TLR)
      {
      TLR--;
      if(!TLR) _LED_ROUGE(0);
      }
   if(TLV)
      {
      TLV--;
      if(!TLV) _LED_VERTE(0);
      }
   if(TLO)
      {
      TLO--;
      if(!TLO) _LED_ORANGE(0);
      }
   if(TLB)
      {
      TLB--;
      if(!TLB) _LED_BLEUE(0);
      }

   }


void _LED_ROUGE_ON(uint16_t ms)
   {
   if(!ms) return;
   _LED_ROUGE(1);
   TLR=ms;
   }

void _LED_VERTE_ON(uint16_t ms)
   {
   if(!ms) return;
   _LED_VERTE(1);
   TLV=ms;
   }

void _LED_ORANGE_ON(uint16_t ms)
   {
   if(!ms) return;
   _LED_ORANGE(1);
   TLO=ms;
   }

void _LED_BLEUE_ON(uint16_t ms)
   {
   if(!ms) return;
   _LED_BLEUE(1);
   TLB=ms;
   }
#endif






//void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *tim)
//   {
//   if(tim==&htimADC) CT_TADC++;
//   }


//Attente tirette en affichant niveau batterie
void BasicWaitTirette(void)
   {
   //VTCtrl(1);
   //lcdclrscr();

   lcdSetFont("ComicSansMs14");
   const uint16_t dclgn=300;
   int16_t n;
   TickType_t t;
   t=CreateDelaiMs(dclgn);
   n=0;
   while(!_Tirette)
      {
      if(FinDelaiMs(t))
         {
         CloseDelaiMs(t);
         t=CreateDelaiMs(dclgn);
         if(n) lcdTextColor565(C565_Lime);
         else lcdTextColor565(C565_FerrariRed);
         n=!n;
         }
      lcdgotoxy(1,10);
      lcdprintf("Attente tirette...\t");
      DelaiMs(10);
      }
   CloseDelaiMs(t);
   lcdTextColor565(C565_FerrariRed);
   lcdgotoxy(1,10);
   lcdprintf("Tirette en place !\t");

   lcdSetFont("CourierNew12");
   do
      {
      lcdgotoxy(1,12);
      lcdTextColor565(C565_RedWine);
      lcdprintf("Vbat=%.1f\t",V_BAT);
      if(V_BAT<V_BAT_MIN)
         {
         lcdTextColor565(C565_BloodRed);
         lcdprintf(" !!! Faible !!!\t");
         }
      }
   while(_Tirette);
   lcdBackColor565(C565_White);
   lcdTextColor565(C565_Black);
   lcdSetFont("CourierNew12");
   lcdclrscr();

   if(0) //Pour filmer (et se préparer)...
   {
	   lcdprintf("Départ dans 5 secondes...\n");
       DelaiMs(5000); //
   }

   }


//Attente tirette en affichant niveau batterie
void BasicWaitTiretteText(void)
   {
   VTCtrl(1);
   lcdclrscr();

   const uint16_t dclgn=300;
   int16_t n;
   TickType_t t;
   t=CreateDelaiMs(dclgn);
   n=0;
   while(!_Tirette)
      {
      if(FinDelaiMs(t))
         {
         CloseDelaiMs(t);
         t=CreateDelaiMs(dclgn);
         n=!n;
         }
      lcdgotoxy(1,10);
      lcdprintf("Attente tirette...\t");
      DelaiMs(10);
      }
   CloseDelaiMs(t);
   lcdgotoxy(1,10);
   lcdprintf("Tirette en place !\t");

   do
      {
      lcdgotoxy(1,12);
      lcdprintf("Vbat=%.1f\t",V_BAT);
      if(V_BAT<V_BAT_MIN)
         {
         lcdprintf(" !!! Faible !!!\t");
         }
      }
   while(_Tirette);
   lcdclrscr();
   }



//Attente appui OK sur écran tactile avec affichage niveau batterie
void BasicMenu(void)
{
   const char *txt=" GO ";
   int w,wt;

    w=lcdGetPixelWidth();

    lcdSetFont("Consolas28");
    wt=lcdGetTextWidth(txt);
    int h=lcdGetPoliceHeight();
    lcdTextColor565(C565_DarkOrchid);
    lcdBackColor565(C565_LightCyan);
    lcdDefTouch(w/2-wt, lcdGetPixelHeight()-1-h, 100,txt, C565_LightCyan, 0);

    char *s;
    lcdSetFont("Consolas14");
    lcdTextColor565(C565_RoyalBlue);

    while(1)
    {
    	extern volatile float TEMPCPU;
       s=lcdGetK();
       if(!s) continue;
       if(*s=='A') break;
       lcdgotoxy(1,3); lcdprintf("Vbat=%.2f  T=%.1f\t", V_BAT,TEMPCPU);
       //lcdgotoxy(1,4); lcdprintf("Rasp %s\t",CodeErreur?"Ok":"Non Ok");
       DelaiMs(50);
    }
 lcdSetFont("Consolas12");
 lcdBackColor565(C565_White);
 lcdTextColor565(C565_Black);
 lcdclrscr();
 lcdUndefTouch(0);

}



void AffBal(void)
{
	unsigned i,j;
	T_Pos b;
	lcdprintf("--Balises : --\t\n");
	for(i=j=0; i<NBBALMAX; i++)
	{
		b=BAL[i];
		if(getBAL_LIFE(b))
		{
			j++;
			lcdprintf("%u : %d %d\t\n",j,getBAL_X(b),getBAL_Y(b));
		}
	}
	lcdprintf("------------\t");
}


int ChercheBordure(void)
{
	static uint16_t ct;
	if(!_ContactAR) {ct=100; return 0;}
	if(ct) ct--;
	if(!ct) return 0;
	return 1;
}

//Le robot doit être dans le rectangle de départ, orienté à 90°
/*
 * @param zone 0: positif, 1: négatif, 0x80: Basic Menu, 0x40: BasicWaitTirette
 */
void CalageOdometrieAuto(int zone)
{
	VTCtrl(1);
    VTExtended(1);
    int LCD_XM=lcdGetPixelWidth();
    int LCD_YM=lcdGetPixelHeight();
    lcdDefTextArea(0,LCD_XM,0,LCD_YM);
    lcdBackColor565(C565_White);
      lcdTextColor565(C565_Black);
      lcdclrscr();    lcdclrscr();
    lcdSetFont("courierNew12");
    lcdgotoxy(1,5); lcdprintf("Prêt pour un calage automatique ?");
    if(zone&0x80) BasicMenu();
    if(zone&0x40) BasicWaitTirette();
    zone&=0x3F;

	   _Powen(1);
	   EnableAsserv(1);
	   VTCtrl(1);

	   DelaiMs(10);

	   PushParam(2);
	   ParamLin(3000,20);
	   ParamRot(3000,20);

	   switch(zone)
	   {
	   case 0: //Côté positif
	      InitOdometrie(1500-300,300,900);
    	  Rbt_HaltReq(40,ChercheBordure);
	      ReculeDe(500);
	      InitOdometrie(P_X,110,900);
 	      AvanceDe(150);
	      TourneGaucheDe(900);
	      Rbt_HaltReq(40,ChercheBordure);
	      ReculeDe(300);
  	      InitOdometrie(1500-110,P_Y,1800);
  	      break;
	   case 1: //Côté négatif
	      InitOdometrie(-1500+300,300,900);
    	  Rbt_HaltReq(40,ChercheBordure);
	      ReculeDe(500);
	      InitOdometrie(P_X,110,900);
 	      AvanceDe(150);
	      TourneDroiteDe(900);
	      Rbt_HaltReq(40,ChercheBordure);
	      ReculeDe(300);
  	      InitOdometrie(-1500+110,P_Y,0);
  	      break;
	   }


	   Rbt_HaltReq(0,0);
	   AvanceDe(80);

	   PopParam(2);
}


void RetourMaison(void)
   {
   PushParam(0);
   PushParam(1);
   ParamLin(30000,20);
   ParamRot(30000,20);
   if(P_X>0)
      {
      if(P_Y<600) //Zone 0
         {
         AvanceDe(50);
         TourneVers(1800);
         PopParam(1);
         PopParam(0);
         return;
         }
      if(P_Y<2100) //Zone 1
         {
         AvanceVers(300,1500);
         AvanceVers(0,1200);
         AvanceVers(0,600); //********
         ReculeVers(1000-200,250);
         TourneVers(1800);
         PopParam(1);
         PopParam(0);
         return;
         }
      //Zone 2
      AvanceVers(0,1900);
      AvanceVers(0,600); //********
      ReculeVers(1000-200,250);
      TourneVers(1800);
      PopParam(1);
      PopParam(0);
      return;
      }
   if(P_Y<1500) //Zone 4
      {
      AvanceVers(0,600); //********
      ReculeVers(1000-200,250);
      TourneVers(1800);
      PopParam(1);
      PopParam(0);
      return;
      }
   //Zone 3
   AvanceVers(0,1900);
   AvanceVers(0,600); //********
   ReculeVers(1000-200,250);
   TourneVers(1800);
   PopParam(1);
   PopParam(0);

   }





struct
{
   const int x,y;
   unsigned nb;
}Z[]=
      {
            {.x=750,.y=1125,.nb=0},
            {.x=770,.y=2325,.nb=0},
            {.x=1000-350,.y=3000-350,.nb=0}
      };

int TaskVisionLidar(void)
   {
   unsigned i,j,n;
   T_Pos b[NBBALMAX],bj;
   n=0;
   for(j=0; j<NBBALMAX; j++)
      {
      bj=BAL[j];
      if(getBAL_LIFE(bj)) b[n++]=bj;
      }
   for(i=0;i<DIM(Z); i++)
      {
      for(j=0; j<n; j++)
         {
         if(hypotf(Z[i].x-getBAL_X(b[j]),Z[i].y-getBAL_Y(b[j]))<200) Z[i].nb++;
         }
      }
   return 0;
   }


void AffBalP(void)
   {
   lcdgotoxy(1,1);
   AffBal();
   }




void RT_Main(void *pvParameters)
   {
   //Initialisations (ne pas modifier ici !)
   SYM_AUTO=0;
   COULEUR=COULEUR_DEF;
   SYM_AUTO=DEF_SYM_AUTO;
   //-----------------------------
   InitOdometrie(0,200,900);
   InitSharedData();
   LARGEUR_ROBOT=400;
   BAL_DIST_AV=400;
   BAL_DIST_AR=400;
    _PowenX(0);
   lcdTaskAllow(xTaskGetCurrentTaskHandle());  //Autorise affichage seulement pour tâche en cours
   Coucou(40,50);
   lcdinit(VT100);
   VTCtrl(1);
      lcdputc(3); //Code init
      lcdputc(3); //Code init
      lcdputc(3); //Code init

      lcdclrscr();lcdclrscr();lcdclrscr();

      VTExtended(1);
      lcdSetFont("courierNew10");
   lcdBackColor565(C565_White);
   lcdTextColor565(C565_Black);
   lcdclrscr();

   lcdprintf("Bonjour\nRobot sur H7 (CM7)\n");
   //lcdprintf("PCLK1=%uHz\n",HAL_RCC_GetPCLK1Freq());
   //lcdprintf("PCLK2=%uHz\n",HAL_RCC_GetPCLK2Freq());
   lcdprintf("Sys=%fMHz\n",HAL_RCC_GetSysClockFreq()/1000000.f);
   lcdprintf("Temp=%.1f Vbat=%.3f\n",TEMPCPU,V_BAT);



   if(0)
   {
   T_Pos b,c;
   setBAL_LIFE(&b,123); setBAL_X(&b,1000);  setBAL_Y(&b,3000);
   lcdprintf("b=%lX\n",b);
   c=b;
   lcdprintf("x=%d y=%d life=%d\n",getBAL_X(c),getBAL_Y(c),getBAL_LIFE(c));
   setBAL_LIFE(&b,500); setBAL_X(&b,-1000); setBAL_Y(&b,2345);
   c=b;
   lcdprintf("x=%d y=%d life=%d\n",getBAL_X(c),getBAL_Y(c),getBAL_LIFE(c));
   while(1);
   }




   if(0)
      {
      extern volatile uint16_t ADCV[4];
      extern volatile unsigned CT_ADC;//,CT_TADC;
      while(1)
         {
         lcdgotoxy(1,5); lcdprintf("ADC=%u  CM7 : %u\t",CT_ADC,CT_CM7);
         lcdgotoxy(1,6); lcdprintf("Temp=%.1f Vbat=%.3f Vdda=%.2f\t",TEMPCPU,V_BAT,VDDA);
         for(unsigned i=0; i<4; i++)
            {
            lcdgotoxy(1,7+i);
            lcdprintf("ADC %u : %u\t",i,ADCV[i]);
            }
         }
      }






   if(0)
      {
      uint32_t t=HAL_GetTick()+5000;
      uint16_t reset=0;

      EnableAsserv(1);
      while(1)
         {
         lcdgotoxy(1,8);
         lcdprintf("P_X=%d P_Y=%d TH=%.1*10D\t",P_X,P_Y,THETA);
         if(!reset && (((int32_t)(t-HAL_GetTick()))<0))
            {
            reset=1;
            lcdgotoxy(1,9); lcdprintf("Reset");
            SCB->AIRCR=(SCB->AIRCR & 0xFFFF) | 0x05FA0000 | (1<<2); //Active SYSRESETREQ pour CM4
            }
         }
      }




   if(0)
      {
      EnableAsserv(1);
      uint32_t t=HAL_GetTick()+5000;
      while(1)
         {
         //RefreshLidarData();  //déjà fait dans iT Asserv
         lcdgotoxy(1,6);

         lcdprintf("CM4 : %u CM7 : %u\t",CT_CM4,CT_CM7);
         lcdgotoxy(1,7);
         lcdprintf("CM4_2 : %u CM7_2 : %u\t",CT2_CM4,CT2_CM7);
         if(!START_LIDAR && (((int32_t)(t-HAL_GetTick()))<0))
            {
            START_LIDAR=1;
            lcdgotoxy(1,9);
            lcdprintf("Démarrage Lidar...\t");
            }
         if(LIDAR_READY)
            {
            lcdgotoxy(1,10);
            lcdprintf("Lidar prêt\t");
            }
         }
      }


   if(0) BasicMenu();
   if(0) {
	   VTCtrl(1);
	   lcdclrscr();
	   BasicWaitTirette();
       }

//================= Tests initiaux ==================
   if(0) TestFrottementsSecs(300,1,50);

   if(0) TestMoteurs(500,50,500); //TestMoteurs(uint16_t vmax, uint16_t dv, uint16_t dt)
   if(0) TestCodeurs();
   if(0) TestMotX(200,50,500);
   if(0) TestCodeurX();

   if(0) TEST1M();
   if(0) TEST360();
//===================================================

   if(0) { AffPeriphI2C(); while(1);}
   //if(0) { CCLeds(1); TestCaptCoul();}
   //if(0) { CCLeds(0); Test5Servos();}






  /*if(0)
   {
	   lcdprintf("Test Turbine...\n");
	   AsservMotX(0);
	   PWMX = VitMotX(0);
      _PowenX(1);
      DelaiMs(1000);
      lcdprintf("Marche\n");
      PWMX = VitMotX(1000);
      DelaiMs(5000);
      PWMX = VitMotX(0);
      lcdprintf("Arrêt\n");
      while(1);
   }*/


   if(0) //Test Calage Odométrie Automatique
   {
	   //
	   CalageOdometrieAuto(0);
	   lcdprintf("P_X=%d  P_Y=%d  THETA=%d\t",P_X,P_Y,THETA);
	   while(1);
   }





   DelaiMs(1000);
     //Démarrage Lidar et Reset si pas de réponse
#if USE_LIDAR
        {
        int k;
        EnableAsserv(1);
        uint32_t t=HAL_GetTick()+10000;
        START_LIDAR=1;
        lcdgotoxy(1,6); lcdprintf("Démarrage Lidar...\t");
        k=0;
        while(!LIDAR_READY)
           {
           lcdgotoxy(1,7); lcdprintf("Vbat=%.2f  Temp=%.1f\t",V_BAT,TEMPCPU);
           if((!k) && ((int32_t)(t-HAL_GetTick()))<1000) {k=1; lcdgotoxy(18,6); lcdprintf("Echec -> Reset dans 1s");}
           if(((int32_t)(t-HAL_GetTick()))<0)
              {
              RESET_CPU();
              }
           }
        lcdprintf("Ok\n");
        }
#endif






   //InitSvGr();

//   lcdprintf("ModeRT : %u\n",ServosGetModeRT());
   while(!GetServoReady());

   void ServoTestSend(void);
   //ServoTestSend();



   if(0) {DetectServos(); while(1);} //Affichage des servos sur les bus

   if(0) ServoId(RXID(1), 45); //Reprogrammation de l'ID d'un servo


   if(0)
      {
      SetSvName(SV_BRAS,"BRAS");
      SetSvName(SV_PINCE,"PINCE");
      SetSvName(SV_EPAULE,"EPAULE");
      SetSvName(SV_SOLAIREG,"SOL_GAUCHE");
      SetSvName(SV_SOLAIRED,"SOL_DROITE");
      SetSvName(SV_FOURCHE, "FOURCHE");
      TestPosServos(6,SV_EPAULE,SV_BRAS,SV_PINCE,SV_SOLAIREG,SV_SOLAIRED,SV_FOURCHE); //Affichage des positions des servos indiqués
      }


   if(0) {EnableAsserv(1); while(1);}

   if(0)
   {
   ServoMoveW(SV_EPAULE,EPAULE_HAUT);
   ServoMoveW(SV_BRAS,BRAS_HAUT);
   ServoMoveW(SV_PINCE,PINCE_OUVERTE);
   while(1);
   }

   RangerBras();



if(0)
{
	 InitOdometrie(0,200,900);

   BAL_DETECT_AV=1;
      BAL_DETECT_AR=1;
      BAL_DIST_AV=500;
      BAL_DIST_AR=500;
   FromLidarToRobot->NbBal=0;
   FromLidarToRobot->DistMin=0;
   while(1)
   {
	   RefreshLidarData();
	   lcdgotoxy(1,4); lcdprintf("X=%d Y=%d\t",P_X,P_Y);
	   lcdgotoxy(1,5); lcdprintf("%u balises\t",NB_BAL);
	   lcdgotoxy(1,6); lcdprintf("%f mm\t",(BAL_DISTMIN<0)?-1.f:DistBal(BAL[BAL_DISTMIN]));
	   lcdgotoxy(1,7); lcdprintf("DETECT=%u\t",BAL_EMERGENCY);
	   lcdgotoxy(1,8); AffBal();
   }
}

   /*
   InitCerises();
   PurgeCerises();
   EclAllColRGB(50,0,0);
   ChargeCerises();
   EclAllColRGB(0,0,0);
   */
   //ServoMove(SV_GAUCHE,GAUCHE_FERME);
   //ServoMove(SV_DROIT,DROIT_FERME);

   //BasicMenu();


   if(0)
      {
   lcdprintf("Attente vision...");
   while(!IsComReceivingData());
      }


   if (0)
      {
      for (int i = 0; i <= 4; i++)
         {
         CalageOdometrieAuto(i);
         //AffOdo();
         DelaiMs(5000);
         RetourMaison();
         }
      }




   //TestVisionLidar(); while(1);



   //VTCtrl(1); lcdclrscr(); BasicWaitTirette();


   /*
    * ************************************************************************
    * ************************** LANCEMENT JEU *******************************
    * ************************************************************************
    */

   //MatchStd(); while(1);


#if(0) 		// prend la pose (pour les photos)
	   static const int16_t gcmd[]=
	   	{
	   			3,SV_EPAULE,SV_BRAS,SV_PINCE,
	   			GC_VIT,300,300,300,
	   			EPAULE_PRISE_PLANTE,BRAS_PRISE_PLANTE,PINCE_OUVERTE,
	   			GC_FIN
	   	};
	   	GMouvementGr(gcmd);

#endif

   if(0) CalageBarillet();

   //GMouvementW(g_reposPlat);

   if(1) {
	   //BasicMenu();
	   CalageOdometrieAuto(0|0x80);
	   MatchStd();
	   while(1);
       }

   VTCtrl(1);

   void (*f)(void);

   f=Menu();

   DelaiMs(1000);
   //VTCtrl(1);
   lcdBackColor565(COULEUR==COUL_POS?VAL_COUL_POS:VAL_COUL_NEG);
   lcdclrscr();
   //while(1);
   //CalageOdometrieAuto(0);

   if(f) f();

   f=GetFctDuringMovement();
   while(1)
      {
      if(f) f();
      //if(TEMPS_RESTANT<-10) LanceFusee();
      }

   }

