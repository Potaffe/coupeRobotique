/*
 * Communic.c
 *
 *  Created on: 25 mars 2019
 *      Author: Robot
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
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

#include "../Core/Odometrie.h"

#include "Config.h"
#include "Communic.h"

volatile int CodeErreur;

TaskHandle_t hCommunic;

static volatile T_Data Gros, Petit, Exp;

volatile const T_Data *GetGros(void)
    {
    return (volatile const T_Data *)(&Gros);
    }

volatile const T_Data *GetPetit(void)
    {
    return (volatile const T_Data *)(&Petit);
    }

volatile const T_Data *GetExp(void)
    {
    return (volatile const T_Data *)(&Exp);
    }

//Trame : #Gpx,py,scoreG:GpxL,pyL,ScoreGL!
//G ou P ou E
//x,y en cm. x : abscisse+150 (toujours >= 0)
//Formes littérales : 0='A', 1='B'

static void AddVal(uint16_t v, char **p, char base)
   {
   uint16_t d,x;
   if(!v) {**p=base;(*p)++;return;}
   d=10000;
   while(d>v) d/=10;
   do
      {
      x=v/d;
      v-=d*x;
      **p=x+base;
      (*p)++;
      d/=10;
      }while(d);
   }

volatile char *TRAME;
char *BuildFrame(T_Data *d, char t)
   {
   static char tr[100];
   char *ptr;
   TRAME=tr;
   ptr=tr;
   *(ptr++)='#';
   *(ptr++)=t;
   AddVal(d->x,&ptr,'0');
   *(ptr++)=',';
   AddVal(d->y,&ptr,'0');
   *(ptr++)=',';
   AddVal(d->score,&ptr,'0');
   *(ptr++)=':';
   *(ptr++)=t;
   AddVal(d->x,&ptr,'A');
   *(ptr++)=',';
   AddVal(d->y,&ptr,'A');
   *(ptr++)=',';
   AddVal(d->score,&ptr,'A');
   *(ptr++)='!';
   *ptr=0;
   return tr;
   }



static int16_t ReadVal(char **buf, uint16_t *nb)
   {
   uint16_t sc;
   sc=0;
   while(*nb)
      {
      if((**buf<'0') || (**buf>'9')) return -1;
      sc *= 10;
      sc += **buf - '0';
      (*buf)++;
      if(*nb) (*nb)--; else return -1;
      if ((**buf == ':')||(**buf == ',')) return sc;
      }
   return -1;
   }

static int16_t ReadValL(char **buf, uint16_t *nb)
   {
   uint16_t sc;
   sc=0;
   while(*nb)
      {
      if((**buf<'A') || (**buf>('A'+'9'))) return -1;
      sc *= 10;
      sc += **buf - 'A';
      (*buf)++;
      if(*nb) (*nb)--; else return -1;
      if ((**buf == '!')||(**buf == ',')) return sc;
      }
   return -1;
   }



static int InterpretTrame(char *buf, uint16_t nb)
   {
   T_Data d;
   volatile T_Data *dest;
   int16_t v;
   if(!nb) return 0;
   if(*buf!='#') return 0;
   buf++;
   if(nb) nb--;
   else return 0;
   switch(*buf)
      {
      case 'G': dest=&Gros;
         break;
      case 'P': dest=&Petit;
         break;
      case 'E': dest=&Exp;
         break;
      default: return 0;
      }
   buf++;
   if(nb) nb--;
   else return 0;
   v=ReadVal(&buf, &nb); //Lecture px
   if(v<0) return 0;
   if(*buf!=',') return 0;
   d.x=v;
   buf++;
   if(nb) nb--;
   else return 0;
   v=ReadVal(&buf, &nb); //Lecture py
   if(v<0) return 0;
   if(*buf!=',') return 0;
   d.y=v;
   buf++;
   if(nb) nb--;
   else return 0;
   v=ReadVal(&buf, &nb); //Lecture score
   if(v<0) return 0;
   if(*buf!=':') return 0;
   d.score=v;
   buf++;
   if(nb) nb--;
   else return 0;
   //Contrôle valeurs littérales...
   switch(*buf)
      {
      case 'G': if(dest!=&Gros) return 0;
         break;
      case 'P': if(dest!=&Petit) return 0;
         break;
      case 'E': if(dest!=&Exp) return 0;
         break;
      default: return 0;
      }
   buf++;
   if(nb) nb--; else return 0;

   v=ReadValL(&buf, &nb); //Lecture px
   if(v<0) return 0;
   if(*buf!=',') return 0;
   if(v!=d.x) return 0;
   buf++;
   if(nb) nb--;
   else return 0;
   v=ReadValL(&buf, &nb); //Lecture py
   if(v<0) return 0;
   if(*buf!=',') return 0;
   if(v!=d.y) return 0;
   buf++;
   if(nb) nb--;
   else return 0;
   v=ReadValL(&buf, &nb); //Lecture score
   if(v<0) return 0;
   if(*buf!='!') return 0;
   if(v!=d.score) return 0;
   d.x=(d.x-150)*10;
   d.y*=10;
   *dest=d; //Mise à jour résultat
   return 1;
   }


static uint8_t RECEPT;

void RT_Communic(void *pvParameters)
    {
    static uint16_t ct;
    char *p;
    T_Data r;
    __HAL_UART_ENABLE_IT(&huartCom,UART_IT_TC);
    __HAL_UART_ENABLE_IT(&huartCom,UART_IT_RXNE);
    HAL_UART_Receive_IT(&huartCom,&RECEPT,1);
    while(1)
      {
       vTaskDelay(211/portTICK_PERIOD_MS); //Attendre 211ms entre émission trames
       Gros.x=P_X;
       ct++; if(ct==1000) ct=0;
       Gros.y=P_Y;
       r.x=(Gros.x+1500)/10;
       r.y=Gros.y/10;
       r.score=Gros.score;
       p=BuildFrame(&r,'G');
       //while(HAL_UART_Transmit_IT(&huartCom,(uint8_t *)p,strlen(p))==HAL_BUSY);
       HAL_UART_Transmit_IT(&huartCom,(uint8_t *)p,strlen(p));
      }
    }

void AddScore(uint16_t sc)
    {
    Gros.score+=sc;
    }

static uint16_t RUNNING_COM=0;


//Indique réception trames sur communication série
//Renvoie 1 si trame reçue. Renvoie ensuite 0 si pas d'autre trame reçue
unsigned IsComReceivingData(void)
{
	static TickType_t tf;

	if(RUNNING_COM)
	{
		tf=xTaskGetTickCount()+100/portTICK_PERIOD_MS;
		return 1;
	}
	if(xTaskGetTickCount()>=tf)
	{
	   RUNNING_COM=0;
	   tf=xTaskGetTickCount();
	   return 0;
	}
	return 1;
}

volatile T_VIS_ARUCO VIS_ARUCO;
//Trame de la forme !nn#rr:xx,yy;rr:xx,yy;...

void HuartCom_RxCpltCallback(void)
    {
	static T_VIS_ARUCO ar;
    static char buf[256];
    static uint16_t nb=0;
    static uint16_t nt=0;
 #if DBG
    if(nb2<sizeof(buf2)) buf2[nb2++]=RECEPT;
 #endif
    switch(RECEPT)
       {
       case '!':
    	  VIS_ARUCO=ar;
          nb=0;
          nt=0;
          RUNNING_COM=1;
          break;
       case '#':
          buf[nb]=0;
          ar.nb=atoi(buf);
          nb=0;
          nt=0;
          break;
       case ':':
    	   buf[nb]=0;
    	   ar.id[nt]=atoi(buf);
    	   nb=0;
    	   break;
       case ',':
    	   buf[nb]=0;
    	   ar.x[nt]=atoi(buf);
    	   nb=0;
    	   break;
       case ';':
    	   buf[nb]=0;
    	   ar.y[nt]=atoi(buf);
    	   nb=0;
    	   if(nt<NB_VIS_ARUCO_MAX) nt++;
    	   break;
       default:
          if(nb<sizeof(buf)-1) //-1 pour laisser de la place pour '!'
             {
             if(nb<sizeof(buf)) buf[nb++]=RECEPT;
             break;
             }
       }

    HAL_UART_Receive_IT(&huartCom,&RECEPT,1);
    }


