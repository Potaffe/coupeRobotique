/*
 * Menu.c
 *
 *  Created on: 2 mars 2017
 *      Author: Robot
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
#include "../../../../../Common/Src/LCD_VT100.h"
#include "Tests.h"
#include "Match.h"
#include "../Core/LowLevel.h"
#include "../Core/TempsMs.h"
#include "../Core/Sequenceur.h"
#include "Config.h"
#include "../../../../Common/Src/SharedMemory.h"


#include "Menu.h"


//uint16_t COULEUR=COULEUR_DEF;


enum {K_NONE, K_OK, K_MOINS, K_PLUS, K_COUL, K_MODE, K_TYPE};

#ifndef DIM
#define DIM(x) (sizeof(x)/sizeof(*x))
#endif

typedef struct
   {
   void (*prog)(void);
   const char *t;
   }T_DEFPROG;

T_DEFPROG PROG_COMPET[]=
   {
      {MatchStd,"Match standard"},
     // {MatchFinale,"Match FINALE"},
      {Demo,"DEMONSTRATION"},
//      {MatchStd,"Match Rapide"},
   };

T_DEFPROG PROG_HOMOLOG[]=
   {
   //   {Homolog,"Homolog DUO"},
      {Homologation,"Homologation"},
	  {TestVitesse,"Test vitesse"},
   };

T_DEFPROG PROG_TEST[]=
   {
      /********************************************************************/
      /********************************************************************/
      //{TestSoufflerie,"Soufflerie"},
      //{TestLacheTapis,"Depose tapis"},
//      {TESTDIV,"Divers"},
      {DeploiMax,          "Déploi. max"},
      {TEST1M,             "1M AV/AR"},
      {TEST360,            "360 PD/PG"},
      {USDistAV100,        "US AV 10cm"},
      {USDistAV200,        "US AV 20cm"},
      {USDistAR100,        "US AR 10cm"},
      {USDistAR200,        "US AR 20cm"},
      {TESTAFFUS,          "Aff. US"},
	  {SPINBOT,			   "SpinFire!!!"},
	  {TESTPANNEAUX, 	   "Test panneaux solaires"},
//      {TestAffOpt,"Trajets"},
//      {TESTUS50,"US 50cm"},
//      {TESTUS100,"US 100cm"},
//      {TESTAFFUS,"Aff US"},
//      {TESTAFFCIBLEUS,"Cibles US"},
//      {TESTAVV,"1,5M AVV"},

      {TestFrotSecs,"Frot. Secs"},

   };

static uint16_t S_MODE=0, S_COMPET=0, S_HOMOLOG=0, S_TEST=0; //S�lections courantes pour sous-menus

static const struct
      {
      const char *t;
      T_DEFPROG *p;
      uint16_t nbp; //Nombre de lignes de sous menu
      uint16_t *sel; //Pointe sur variable indiquant s�lection dans sous-menu
      uint16_t nbsel; //Nombre de sous menus
      uint16_t dur; //Dur�e match en 0,1 s
      int col:1;  //D�finition de couleur
      int tir:1; //Gestion tirette
      int gif:1; //Affichage gifs anim�s
      int calage:1; //Permet mouvement du robot en attendant tirette (pour calage)
      }MENU[]={
         {.t="COMPET",.p=PROG_COMPET,.dur=1000,.nbp=DIM(PROG_COMPET),.sel=&S_COMPET,.nbsel=DIM(PROG_COMPET),.col=1,.tir=1,.gif=0,.calage=1},
         {.t="HOMOLOG",.p=PROG_HOMOLOG,.dur=1000,.nbp=DIM(PROG_HOMOLOG),.sel=&S_HOMOLOG,.nbsel=DIM(PROG_HOMOLOG),.col=1,.tir=1,.gif=0,.calage=1},
         {.t="TEST",.p=PROG_TEST,.dur=0,.nbp=DIM(PROG_TEST),.sel=&S_TEST,.nbsel=DIM(PROG_TEST),.col=1,.tir=0,.gif=0,.calage=0},
      };


static void AffBatTime(void)
{
	lcdPushTxtPos();
	lcdSetOutWind(1);
	lcdgotoxy(1,1);
	lcdprintf("Vbat=%.1*1000D  -  Reste %.1*10D\t",V_BAT,RBT_TIME);
	lcdSetOutWind(0);
	lcdPopTxtPos();
}

static void AffBat(void)
{
	lcdPushTxtPos();
	lcdSetOutWind(1);
	lcdgotoxy(1,1);
    	lcdprintf("Vbat=%.1f\t",V_BAT);
	lcdSetOutWind(0);
	lcdPopTxtPos();
}



static int16_t Wait_poussoirs(void)  //attente poussoirs. Retourne K_NONE, K_OK, K_MOINS, K_PLUS, ...
{
   int16_t k;
   //static uint16_t c=1;
   char *s;
   s=lcdGetK();
   if(!s) return K_NONE;
   /*
   if(s) { while(*s) PushFifoK(*(s++)); }
   k=PopFifoK();
   */
   if(*s) k=*s; else k=-1;
   //if(k!=-1) {lcdPushTxtPos(); lcdgotoxy(c++,1); lcdputc(k); lcdPopTxtPos();}
   switch(k)
	      {
           case 'A':  //Moins
               return K_MOINS;
           case 'B':  //Plus
               return K_PLUS;
           case 'C':  //Ok
               return K_OK;
           case 'D':  //Couleur
               return K_COUL;
           case 'E':  //Mode
               return K_MODE;
           case 'F':  //Type
               return K_TYPE;
	      }
   return K_NONE;
}

static uint16_t LCD_XM, LCD_YM;


static void AffSelectColor(void)
{
	uint16_t col;
	uint16_t x,y,h;
	if(COULEUR==COUL_POS) col=VAL_COUL_POS; else col=VAL_COUL_NEG;
	x=lcdGetAbsX();
	y=lcdGetAbsY();
	h=lcdGetPoliceHeight();
	lcdRectFull(x,x+50,y,y+h,col);
}

static void AffSelectMode(void)
{
   lcdputs(MENU[S_MODE].t);
   lcdputc('\t');
}


static void AffSelectType(void)
{
	lcdputs(MENU[S_MODE].p[*(MENU[S_MODE].sel)].t );
	lcdputc('\t');
}

void (*Menu(void))(void)
    {
    const struct
	   {
    	char *txt;
    	void (*aff)(void);
    	uint16_t *sel;
    	uint16_t nbsel;
	   }desc[]=
			   {
					   {.txt="Couleur", .aff=AffSelectColor, .sel=(uint16_t*)(&COULEUR), .nbsel=2},
					   {.txt="Mode", .aff=AffSelectMode, .sel=&S_MODE, .nbsel=DIM(MENU)},
					   {.txt="Type", .aff=AffSelectType, .sel=0, .nbsel=0},
			   };
    uint16_t h,i,sel;
    int16_t k;
    const uint16_t ofstx=10;


    //InitFifoK();
    VTCtrl(1);
    lcdputc(3); //Code init
    lcdputc(3); //Code init
    lcdputc(3); //Code init

    lcdclrscr();lcdclrscr();lcdclrscr();

    VTExtended(1);
    LCD_XM=lcdGetPixelWidth();
    LCD_YM=lcdGetPixelHeight();
    lcdBackColor565(C565_DarkBlue);
    lcdDefTextArea(0,LCD_XM,0,LCD_YM);
    lcdclrscr();

    lcdSetFont("Consolas28");
    h=lcdGetPoliceHeight();
    lcdTextColor565(C565_DarkOrchid);
    lcdBackColor565(C565_LightCyan);
    lcdDefTouch(ofstx, LCD_YM-1-h, 100," - ", C565_LightCyan, 0);
    lcdDefTouch(LCD_XM-ofstx-lcdGetTextWidth(" + "), LCD_YM-1-h, 100, " + ", C565_LightCyan, 1);
    lcdDefTouch(LCD_XM/2-lcdGetTextWidth("OK")/2, LCD_YM-1-h, 100, "OK", C565_LightCyan, 2);

    lcdDefTextArea(0,LCD_XM,4,LCD_YM-10-h);
    lcdBackColor565(C565_White);
    lcdTextColor565(C565_Black);
    lcdSetFont("CourierNew14");
    lcdclrscr();

    while(0)
	{
	static uint16_t n;
	lcdgotoxy(1,1); lcdprintf("Bonjour\t");
	lcdgotoxy(20,8); lcdprintf("Coucou\t");
	lcdgotoxy(5,5); lcdprintf("%u\t",n++);
	lcdgotoxy(20,2); lcdprintf("Au revoir\t");
	lcdgotoxy(1,7); lcdprintf("Adieu\t");
	lcdgotoxy(1,6); lcdprintf("X=%u   Y=%u\t",lcdGetAbsX(),lcdGetAbsY());
	TickType_t tf=xTaskGetTickCount(); lcdgotoxy(1,9); lcdprintf("T=%u\t",tf);
	}




    sel=0;
/*
    lcdprintf("%u %u\n",LCD_XM,LCD_YM);
    while(1)
	{
	static uint16_t n;
	k=GetChar();
	if(k!=-1) lcdputc(k);
	lcdPushTxtPos();
	lcdgotoxy(1,10); lcdprintf("%u\t",n++);
	lcdPopTxtPos();
	}
*/
    struct
	{
    	uint16_t vbp;
    	unsigned vbat:1;
    	unsigned tir:1;
    	unsigned tirp1:1;
    	unsigned tirp2:1;
    	unsigned desc:1;
	}chg;
	chg.vbat=1;
	chg.tir=1;
	chg.vbp=(uint16_t)(V_BAT*200.f);
	chg.desc=1;
	uint16_t xvbat=LCD_XM-lcdGetTextWidth("Vbat=88.8 V  !!! Faible !!!");
	uint16_t yvbat=4;
    while(1)
    {
    	int16_t x;
		if (chg.vbat)
		    {
		    //lcdgotoxy(10, 1);
		    lcdGotoXYa(xvbat,yvbat);
		    lcdTextColor565(C565_RedWine);
		    lcdprintf("Vbat=%.1fV \t", V_BAT);
		    if (V_BAT < V_BAT_MIN)
			{
			lcdBackColor565(C565_BloodRed);
			lcdTextColor565(C565_Yellow);
			lcdprintf(" !!! Faible !!!");
			lcdBackColor565(C565_White);
			lcdprintf("\t");
			}
		    chg.vbat = 0;
		    }
		else
		    {
		    x = (uint16_t)(V_BAT*200.f);
		    if (x != chg.vbp) chg.vbat = 1;
		    chg.vbp = x;
		    }

	if(chg.tir)
	    {
		if (MENU[S_MODE].tir)
		    {
		    lcdTextColor565(C565_PurpleMonster);
		    lcdgotoxy(4, 2);
		    if (!_Tirette) lcdputs("!! METTRE TIRETTE !!\t");
		    else lcdputs("TIRETTE EN PLACE\t");
		    }
		else
		    {
		    lcdgotoxy(4, 2);
		    lcdputs("\t");
		    }
	   chg.tir=0;
	    }
	else
	    {
	    if(chg.tirp1!=_Tirette) chg.tir=1;
	    if(chg.tirp2!=MENU[S_MODE].tir) chg.tir=1;
	    chg.tirp1=_Tirette;
	    chg.tirp2=MENU[S_MODE].tir;
	    }


	if(chg.desc)
	    {
		for (i = 0; i < DIM(desc); i++)
		    {
		    lcdgotoxy(1, 4 + i * 2);
		    if (i == sel)
			{
			lcdBackColor565(C565_FerrariRed);
			lcdTextColor565(C565_White);
			}
		    lcdDefTouch(lcdGetAbsX(), lcdGetAbsY(), 150, desc[i].txt,C565_Black, i + 3);
		    //lcdDefTouch(-1, -1, 150, desc[i].txt,C565_Black, i + 3);
		    lcdBackColor565(C565_White);
		    lcdTextColor565(C565_Black);
		    if (desc[i].aff)
			{
			lcdgotoxy(10, 4 + i * 2);
			desc[i].aff();
			}
		    }
	    }


		chg.desc=1;
		k=Wait_poussoirs();
		switch(k)
		{
		default:
		    chg.desc=0;
		    break;
		case K_MOINS: // - (MOINS)
			if(desc[sel].sel)
			   {
				x=*(desc[sel].sel);
				if(x==0) x=desc[sel].nbsel-1; else x--;
				*(desc[sel].sel)=x;
			   }
			else
			{
				x=*(MENU[S_MODE].sel);
				if(x==0) x=(MENU[S_MODE].nbsel)-1; else x--;
				*(MENU[S_MODE].sel)=x;
			}
			//_LED_ORANGE_ON(200);
			break;
		case K_PLUS: // + (PLUS)
			if(desc[sel].sel)
			   {
				x=*(desc[sel].sel);
				x++;
				if(x==desc[sel].nbsel) x=0;
				*(desc[sel].sel)=x;
			   }
			else
			{
				x=*(MENU[S_MODE].sel);
				x++;
				if(x==(MENU[S_MODE].nbsel)) x=0;
				*(MENU[S_MODE].sel)=x;
			}
			//_LED_ORANGE_ON(200);
			break;
		case K_OK: //OK
		   {
			void (*ret)(void);
			int16_t n;
			TickType_t t;
			//_LED_ORANGE_ON(200);
			ret=MENU[S_MODE].p[*(MENU[S_MODE].sel)].prog;
			for(i=0; i<DIM(desc)+3; i++) lcdUndefTouch(i);
			lcdSetFont("ComicSansMs14");
			if(MENU[S_MODE].tir)
			   {
			   CalageOdometrieAuto(0);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				const uint16_t dclgn=300;
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
 		          lcdgotoxy(1,10); lcdprintf("Attente tirette...\t");
 		          DelaiMs(10);
				  }
				CloseDelaiMs(t);
			if(MENU[S_MODE].dur) DefineFctDuringMovement(AffBatTime);
			else DefineFctDuringMovement(AffBat);
			lcdBackColor565(C565_White);
			lcdTextColor565(C565_Black);
			lcdclrscr();
			lcdSetFont("ComicSansMs18");
			lcdgotoxy(1,1); lcdprintf("*** EN ATTENTE ***\t");
			lcdgotoxy(1,2); lcdprintf("Couleur : "); AffSelectColor();
			lcdgotoxy(1,3);  lcdprintf("Mode : %s\t",MENU[S_MODE].t);
			lcdgotoxy(1,4); lcdprintf("Type : %s\t",MENU[S_MODE].p[*(MENU[S_MODE].sel)].t);
			lcdTextColor565(C565_FerrariRed);

			   //lcdSetFont("CourierNew12");

			   do
			      {
				  lcdgotoxy(1,6);
				  lcdTextColor565(C565_RedWine);
				  lcdprintf("Vbat=%.1f\t",V_BAT);
				  if(V_BAT<V_BAT_MIN) {lcdTextColor565(C565_BloodRed); lcdprintf(" !!! Faible !!!\t");}
				  else lcdprintf(" Ok !\t");
			      }
			      while(_Tirette);
			   if(MENU[S_MODE].dur)
				   {
				   RBT_TIME=MENU[S_MODE].dur;
				   }
			   if(MENU[S_MODE].gif) lcdAffLGif(890,"0;1;2;3;4;5;6;7;8;12;13;14;15;16;17;18;1*10");
			   }
         lcdBackColor565(C565_White);
         lcdTextColor565(C565_Black);
         lcdDefTextArea(0,LCD_XM,0,LCD_YM);
         lcdSetFont("CourierNew12");

         lcdclrscr();

         lcdSetScroll(1);
         //VTCtrl(0);
         lcdprintf("Couleur : "); AffSelectColor(); lcdputs("\n");
         lcdprintf("Mode : %s\n",MENU[S_MODE].t);
         lcdprintf("Type : %s\n",MENU[S_MODE].p[*(MENU[S_MODE].sel)].t);
         lcdputs("C'est parti !\n");

			return ret;
		   }
		case K_COUL: //Couleur
			sel=0;
			//_LED_ORANGE_ON(200);
			break;
		case K_MODE: //Mode
			sel=1;
			//_LED_ORANGE_ON(200);
			break;
		case K_TYPE: //Type
			sel=2;
			//_LED_ORANGE_ON(200);
			break;
		}
    }




    return 0;
    }


#if 0
void (*SelectProg(void))(void)
   {
   const struct
         {
         const char *t;
         int v:8;
         int cd:1;  //D�finition de couleur
         int tir:1; //Gestion tirette
         }m[]={
            {"COMPET",M_COMPET,.cd=1,.tir=1},
            {"HOMOLOG",M_HOMOLOG,.cd=1,.tir=1},
            {"TEST",M_TEST,.cd=0,.tir=0},
         };
   int MODE;
   T_DEFPROG *p;
   int r,c,d,x,y;
   unsigned dur=900;
   p=PROG_COMPET;
   x=lcdgetx();
   y=lcdgety();
   c=0;
   do
      {
      lcdgotoxy(x,y);
      MODE=m[c].v;
      lcdprintf("MODE : %s\t",m[c].t);
      lcdgotoxy(x,y+1);
      r=Wait_poussoirs();
      if(r==K_PLUS)
         {
         c++;
         if(c>=sizeof(m)/sizeof(*m)) c=0;
         }
      }
      while(r!=K_OK);
   if(m[c].cd) { SelectColor();  }
   x=lcdgetx();
   y=lcdgety();
   if(m[c].tir)
      {
      if(!TIRETTE)
         {
         lcdgotoxy(x,y);
         lcdputs("Mettre tirette\t");
         while(!TIRETTE) DelaiMs(100);
         }
      }
   switch(m[c].v)
      {
      case M_COMPET:
         if(COULEUR==COUL_POS) p=PROG_COMPET;
         else p=PROG_COMPET;
         dur=900;
         break;
      case M_HOMOLOG:
         if(COULEUR==COUL_POS) p=PROG_HOMOLOG;
         else p=PROG_HOMOLOG;
         dur=900;
         break;
      case M_TEST:
         p=PROG_TEST;
         dur=0x7FFF;
         break;
      }
   d=0;
   if(p[d].prog==0) {lcdputs("Pas de prog !"); while(1);}
   do
      {
      lcdgotoxy(x,y);
      lcdprintf("PROG : %s\t",p[d].t);
      lcdgotoxy(x,y+1);
      r=Wait_poussoirs();
      if(r==K_PLUS)
         {
         d++;
         if(p[d].prog==0) d=0;
         }
      }
      while(r!=K_OK);
   if(m[c].tir)
      {
      lcdputs("Att. tir...\t");
      while(TIRETTE);
//      CaptStart(); //Initialise observateur...
//      ActStart();
      lcdclrscr();
      }
   RBT_TIME=dur;
   return p[d].prog;
   }
#endif


