/*
 * MotX.c
 *
 *  Created on: 16 juin 2019
 *      Author: deloizy
 */

#include "Asserv.h"
#include "../App/Config.h"
#include "LowLevel.h"
#include "../../../../../Common/Src/LCD_VT100.h"
#include "TempsMs.h"
#include "MotX.h"

static int OffsetX=0;

void StartXPos(int n)
    {
    int p,p0,dp;
    AsservMotX(1);
    if(n<0) n=0;
    if(n>7) n=7;
    p0=GetPosMotX();
    p=OffsetX+870+n*(38400/8)+700;//400;
    dp=p-p0;
    if(dp>38400/2) p-=38400;
    else if(dp<-38400/2) p+=38400;
    StartX(p);
    }

void MoveXPos(int n)
   {
   StartXPos(n);
   while(XIsRunning());
   }

static int CalageMotX(float vit)
    {
    int calage=0;
    int p,r;
   _PowenX(1);
   AsservMotX(1);
   EnableAsserv(1);
   SetPrmX(vit,0.1);
   p=GetPosMotX();
   if(p>38400) p-=38400;
   StartX(p+40000);
   while(XIsRunning())
      {
      if(_TopX)
         {
    	  OffsetX=GetPosMotX();
    	  r=p-OffsetX;
    	  if(r<0) r=-r;
    	  if(r>(int)(38400/6.28/135*20)) { calage=1; break;}
         }
      }
   StopMotX();
    return calage;
    }

void CalageBarillet(void)
   {
   //Calage barillet (Moteur X)
   if(CalageMotX(5.f))  //!!! Met l'asservissement en marche !!!
              {
              lcdprintf("Calage Ok\n");
              }
          else
             {
             lcdprintf("Faire calage manuel ! (5 secondes)");
             EnableAsserv(0);
             _PowenX(0);


             _PowenX(1);
             EnableAsserv(1);
             }
   //MoveXPos(20);
   StartXPos(0);
   }
