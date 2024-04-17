// Gestion LCD & terminal vt100 sur RS232C
// Compatible TeraTerm
// mise à jour du 16/02/2010 : caractères accentués
// mise à jour du 17/03/2010 : FIFO émission caractères (& prêt pour XON-XOFF)
// mise à jour du 19/03/2010 : Support LCD & vt100 dans la même bibliothèque
// mise à jour du 02/12/2010 : suppression bug %dX
// mise à jour du 30/03/2011 :
//   - les lignes RX & TX doivent être préinitialisées (TRISx)
//   - les macros UARTNUM (1 ou 2) & UxALT (0 ou 1) doivent être définies à l'invocation du compilateur
//   - redirection de la sortie : définition de la fonction lcdFout
// mise à jour du 08/03/2013 : supression du bug -32768 en %d
// mise à jour du 02/06/2015 : Adaptation pour STM32
// mise à jour du 20/08/2017 : Adaptation pour FreeRTOS
// mise à jour du 09/06/2019 : Ajout du codage UTF8
// mise à jour du 06/07/2019 : ajout des fonctions graphiques
// mise à jour du 25/07/2019 : ajout fonctions cercles (avec afficheur date > 25/07/2019)
//                             ajout de la police Delius et tailles polices complétées (10,12,14,18,22,24,28,48)
// mise à jour du 31/07/2019 : suppression modif pos curseur quand utilisation lcdFout())
//                             modification gestion FIFOs
//                             Ajout des polices 16pts - ajout 'ù' manquant
// mise à jour du 19/08/2019 : Ajout largeur fixe pour toutes polices
//                             Ajout fonctions lcdSetSpc(), lcdGetSpc() et lcdSetIncrust()


#define UTF8 1 //Codage UTF8 ou ANSI 1252


#if defined STM32F7
#include "stm32f7xx.h"
#include "stm32f7xx_hal_conf.h"
#include "stm32f7xx_hal_uart.h"
#elsif defined STM32H7
#include "stm32h7xx.h"
#include "stm32h7xx_hal_conf.h"
#include "stm32h7xx_hal_uart.h"
#elsif defined STM32F4
#include "stm32f4xx.h"
#include "stm32f4xx_hal_conf.h"
#include "stm32f4xx_hal_uart.h"
#elsif defined STM32L4
#include "stm32l4xx.h"
#include "stm32l4xx_hal_conf.h"
#include "stm32l4xx_hal_uart.h"
#elsif defined STM32G4
#include "stm32g4xx.h"
#include "stm32g4xx_hal_conf.h"
#include "stm32g4xx_hal_uart.h"
#endif

#ifndef huartAff
#error huartAff undefined !!! use -DhuartAff=huartx when compiling !
#endif


#include "usart.h"

#include "LCD_VT100.h"
//#include "TempsMs.h"
//#include "../App/Config.h"
#define TIMEOUT_READREP 100

static TaskHandle_t hTaskAllowed=0;
static volatile int16_t IS_RUNNING=0;
void WaitIfRunning(void)
   {
   if(hTaskAllowed==xTaskGetCurrentTaskHandle()) return; //Tâche autorisée à afficher
   while(IS_RUNNING)
      ;
   IS_RUNNING=1;
   }
#define RunComplete() (IS_RUNNING=0)

//Indique la seule tâche autorisée à afficher (id=0 pour toutes)
//Faire TaskAllow(xTaskGetCurrentTaskHandle()) pour autoriser la tâche courante
void lcdTaskAllow(TaskHandle_t id)
   {
   hTaskAllowed=id;
   }

static unsigned char NBLIG=4;
static unsigned char NBCOL=20;


//-------------------
typedef unsigned char BYTE;

static struct
   {
      unsigned FOpen :1;
      unsigned _LCD :1;
      unsigned Extended :1;  //Utilisation des fonctions avancées de l'afficheur tactile
      unsigned g :1;
      unsigned Flong :1;
      volatile unsigned TxInProgress :1;   // émission par fifo amorçée
      volatile unsigned Xoff :1;
      unsigned ctrl :1;   // autorisation des caractères de contrôle ou non pour sortie VT100
   } LCD_FLAGS=
   {
   .FOpen=0, ._LCD=0, .Extended=0, .g=0, .Flong=0, .TxInProgress=0, .Xoff=0, .ctrl=1
   };

#define _FOpen (LCD_FLAGS.FOpen)
#define _LCD (LCD_FLAGS._LCD)
#define _bitg (LCD_FLAGS.g)
#define _Flong (LCD_FLAGS.Flong)
#define _TxInProgress (LCD_FLAGS.TxInProgress)
#define _FXoff (LCD_FLAGS.Xoff)
#define _CTRL (LCD_FLAGS.ctrl)
#define _Extended (LCD_FLAGS.Extended)
static unsigned Col=1;
static unsigned Lig=1;

static void _lcdgotoxy(unsigned char x, unsigned char y);

void VTCtrl(int s)
   {
   WaitIfRunning();
   if(s) _CTRL=1;
   else
      _CTRL=0;
   RunComplete();
   }

void VTExtended(uint16_t e)
   {
   WaitIfRunning();
   if(e) _Extended=1;
   else
      _Extended=0;
   RunComplete();
   }

unsigned lcdgetx(void)
   {
   //WaitIfRunning();
   return Col;
   }

unsigned lcdgety(void)
   {
   //WaitIfRunning();
   return Lig;
   }

static struct
   {
      uint16_t col;
      uint16_t lig;
   } pile[5];
static unsigned char NBPILE=0;

int lcdpushxy(unsigned char x, unsigned char y)
   {
   WaitIfRunning();
   if(NBPILE>=sizeof(pile)/sizeof(*pile))
      {
      RunComplete();
      return 0;
      }
   pile[NBPILE].col=Col;
   pile[NBPILE].lig=Lig;
   _lcdgotoxy(x,y);
   NBPILE++;
   RunComplete();
   return 1;
   }

int lcdpopxy(void)
   {
   WaitIfRunning();
   if(NBPILE==0)
      {
      RunComplete();
      return 0;
      }
   NBPILE--;
   _lcdgotoxy(pile[NBPILE].col,pile[NBPILE].lig);
   RunComplete();
   return 1;
   }

static struct
   {
   uint8_t buf[200];
   uint8_t * volatile pr, * volatile pw;
   } RFIFO;

static struct
   {
   uint8_t buf[400];
   uint8_t * volatile pr, * volatile pw;
   } SFIFO;


#define FIFOvide(f) (((f).pr)==((f).pw))

static void InitFIFOs(void)
   {
   RFIFO.pr=RFIFO.pw=RFIFO.buf;
   SFIFO.pr=SFIFO.pw=SFIFO.buf;
   }


static int WriteSFIFO(uint8_t x)
   {
   uint8_t *p;
   p=SFIFO.pw+1;
   if(p==SFIFO.buf+sizeof(SFIFO.buf)) p=SFIFO.buf;
   if(p==SFIFO.pr) return 0;
   *(SFIFO.pw)=x;
   SFIFO.pw=p;
   return 1;
   }

static int ReadSFIFO(uint8_t *x)
   {
   uint8_t *p;
   if(SFIFO.pr==SFIFO.pw) return 0;
   *x=*(SFIFO.pr);
   p=SFIFO.pr+1;
   if(p==SFIFO.buf+sizeof(SFIFO.buf)) p=SFIFO.buf;
   SFIFO.pr=p;
   return 1;
   }

static int WriteRFIFO(uint8_t x)
   {
   uint8_t *p;
   p=RFIFO.pw+1;
   if(p==RFIFO.buf+sizeof(RFIFO.buf)) p=RFIFO.buf;
   if(p==RFIFO.pr) return 0;
   *(RFIFO.pw)=x;
   RFIFO.pw=p;
   return 1;
   }

static int ReadRFIFO(uint8_t *x)
   {
   uint8_t *p;
   if(RFIFO.pr==RFIFO.pw) return 0;
   *x=*(RFIFO.pr);
   p=RFIFO.pr+1;
   if(p==RFIFO.buf+sizeof(RFIFO.buf)) p=RFIFO.buf;
   RFIFO.pr=p;
   return 1;
   }

//uint32_t NBSEND,NBWRITE;

static int16_t TxComplete(void)
   {
   return _TxInProgress==0;
   }

static uint8_t RECEPT;
void HuartAff_ErrorCallback(void)
   {
   //volatile uint32_t err=huartAff.ErrorCode;
   }

//char REPRBUF[20]; uint16_t NBREPRBUF,NBREPRBUF0,NBREPRBUF1;
void HuartAff_RxCpltCallback(void)
{
//   if(NBREPRBUF<sizeof(REPRBUF)) REPRBUF[NBREPRBUF++]=RECEPT;
	WriteRFIFO(RECEPT);
	HAL_UART_Receive_IT(&huartAff,&RECEPT,1);
}



static void TransmitIt(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
   {
   HAL_UART_Transmit_IT(huart,pData,Size);
   }

void HuartAff_TxCpltCallback(void)
{
	static uint8_t x;
    if(!ReadSFIFO(&x))
       {
       _TxInProgress=0;
       return;
       }
    TransmitIt(&huartAff,&x,1);
//    NBSEND++;
}

int16_t GetChar(void)
   {
   uint8_t x;
   WaitIfRunning();
   if(!ReadRFIFO(&x))
      {
      RunComplete();
      return -1;
      }
   RunComplete();
   return x;
   }

int16_t GetCharTimeOut(uint16_t ms)
   {
   int16_t x;
   uint32_t t;
   WaitIfRunning();
   t=HAL_GetTick()+ms;
   do
      {
      x=GetChar();
      if(x!=-1)
         {
         RunComplete();
         return x;
         }
      }
   while(((int32_t)(t-HAL_GetTick()))>=0);
   RunComplete();
   return -1;
   }

static void (*Femiss)(unsigned char)=0;

void lcdFout(void (*fct)(unsigned char))
   {
   WaitIfRunning();
   Femiss=fct;
   RunComplete();
   }

static void lcd_emiss(unsigned char c)
   {
   if(Femiss)
      {
      Femiss(c);
      return;
      }
//NBWRITE++;
   if(!_TxInProgress)
      {
      static uint8_t buf[20];
	   static uint8_t x;
	   unsigned nb;
	   if(!FIFOvide(SFIFO))
	      {
	      nb=0;
	      while(ReadSFIFO(&x))
	         {
	         if(nb>=sizeof(buf)-1) break;
	         buf[nb++]=x;
	         }
	      if(FIFOvide(SFIFO)) buf[nb++]=c;
	      else WriteSFIFO(c);
	      TransmitIt(&huartAff,buf,nb);
	      _TxInProgress=1;
	      return;
	      }
	   x=c;
	   TransmitIt(&huartAff,&x,1);
	   _TxInProgress=1;
//        NBSEND++;
      }
   else
      {
      while(!WriteSFIFO(c));
      }
   }

static void _direct_lcdputs(const char *s)
   {
   while(*s)
      lcd_emiss(*(s++));
   }

//************************************
//Fonctions spécifiques LCD

static void InitCG(void)
   {
   const struct
      {
         char car;
         char tb[8];
      } def[]=
      {
         {
         0xE9,//'é',
            {
            0x02, 0x04, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0
            }
         },
         {
         0xE8,//'è',
            {
            0x08, 0x04, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0
            }
         },
         {
         0xEA,//'ê',
            {
            0x04, 0x0A, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0
            }
         },
         {
         0xEB,//'ë',
            {
            0x00, 0x0A, 0x0E, 0x11, 0x1F, 0x10, 0x0E, 0
            }
         },
         {
         0xE0,//'à',
            {
            0x08, 0x04, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0
            }
         },
         {
         0xE2,//'â',
            {
            0x04, 0x0A, 0x0E, 0x01, 0x0F, 0x11, 0x0F, 0
            }
         },
         {
         0xF4,//'ô',
            {
            0x04, 0x0A, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0
            }
         },
         {
         0xF9,//'ù',
            {
            0x08, 0x04, 0x11, 0x11, 0x11, 0x13, 0x0D, 0
            }
         }
      };
   unsigned char i, j;
   for(i=0; i<sizeof(def)/sizeof(*def); i++)
      {
      lcd_emiss(27);
      lcd_emiss(128+i);
      for(j=0; j<8; j++)
         {
         lcd_emiss(def[i].tb[j]);
         }
      }
   }

//************************************

void lcdinit(int prf)
   {
   _LCD=(prf==LCD);
   if(_LCD) _Extended=0;
   InitFIFOs();
   _FOpen=1;
   NBPILE=0;
   Col=1;
   Lig=1;
   //MX_USART3_UART_Init();
   __HAL_UART_ENABLE_IT(&huartAff,UART_IT_TC);
   __HAL_UART_ENABLE_IT(&huartAff,UART_IT_RXNE);
   HAL_UART_Receive_IT(&huartAff,&RECEPT,1);
   //GPIOD->OTYPER|=(1<<8);  //Mets GPIOD8 à 1 pour Open Drain
   if(_LCD)
      {
      NBLIG=4;
      NBCOL=20;
      InitCG();
      lcdclrscr();
      }
   else
      {
      NBLIG=24;
      NBCOL=80;
      _direct_lcdputs("               \r\n\r\n");
      if(_CTRL)
         {
         _direct_lcdputs("\033c");
         lcdclrscr();
         _direct_lcdputs("\033[?25l");  //efface curseur
// _direct_lcdputs("\033[?25h"); //affiche curseur
         }
      }
   }

static void lcdputc_lcd(unsigned char c)
   {
   if((c<0x20)||(c>0x7F))
      {
      switch(c)
         {
         case 3: //code 3 autorisé !!! (Code init)
            break;
         case '\t':
            {
            unsigned char x;
            if(Femiss) break;
            x=Col;
            while(Col<=NBCOL)
               {
               lcd_emiss(' ');
               Col++;
               }
            _lcdgotoxy(x,Lig);
            return;
            }
         case '\r':
            if(Femiss) break;
            _lcdgotoxy(1,Lig);
            return;
         case '\n':
            if(Femiss) break;
            Lig++;
            if(Lig>NBLIG)
               {
               _lcdgotoxy(Col,1);
               return;
               }
            _lcdgotoxy(Col,Lig);
            return;
         case 8:
            if(Femiss) return;
            if(Col<=1) return;
            Col--;
            lcd_emiss(8);
            return;
         case 0xE9: //(BYTE)'é':
            c=128;
            break;
         case 0xE8: //(BYTE)'è':
            c=129;
            break;
         case 0xEA: //(BYTE)'ê':
            c=130;
            break;
         case 0xEB: //(BYTE)'ë':
            c=131;
            break;
         case 0xE0: //(BYTE)'à':
            c=132;
            break;
         case 0xE2: //(BYTE)'â':
            c=133;
            break;
         case 0xF4: //(BYTE)'ô':
            c=134;
            break;
         case 0xF9: //(BYTE)'ù':
            c=135;
            break;
         case 0xB0: //(BYTE)'°':
            c=0xDF;
            break;
         case 0xB5: //(BYTE)'µ':
            c=0xE4;
            break;
         case 0xF6: //(BYTE)'ö':
            c=0xEF;
            break;
         case 0xFC: //(BYTE)'ü':
            c=0xF5;
            break;
         case 0x90:
            c=0xE0;
            break;      //alpha
         case 0x91:
            c=0xE2;
            break;      //beta
         case 0x92:
            c=0xE3;
            break;      //epsilon
         case 0x93:
            c=0xE5;
            break;      //sigma
         case 0x94:
            c=0xE6;
            break;      //rau
         case 0x95:
            c=0xF2;
            break;      //teta
         case 0x96:
            c=0xF3;
            break;      //infini
         case 0x97:
            c=0xF4;
            break;      //omega
         case 0x98:
            c=0xF6;
            break;      //SIGMA
         case 0x99:
            c=0xF7;
            break;      //pi
         case 0x9A:
            c=0xEE;
            break;  //ñ
         default:
            c='?';
            break;
         }
      }

   lcd_emiss(c);
   if(Femiss) return;
   Col++;
   if(Col>NBCOL)
      {
      Col=1;
      Lig++;
      if(Lig>NBLIG) _lcdgotoxy(1,1);
      }
   }

static void lcdputc_vt(unsigned char c)
   {
   if((c<0x20)||(c>0x7F))
      {
      switch(c)
         {
         case 3: //code 3 autorisé !!! (Code init)
            break;
         case '\t':
            {
            if(Femiss) break;
            if(_CTRL) _direct_lcdputs("\033[K");
            return;
            }
         case '\r':
            if(_CTRL&&(!Femiss)) _lcdgotoxy(1,Lig);
            else
               {
               lcd_emiss('\r');
               lcd_emiss('\n');
               }
            return;
         case '\n':
            if(_CTRL&&(!Femiss))
               {
               Lig++;
               Col=1;
               if(Lig>NBLIG)
                  {
                  _lcdgotoxy(Col,1);
                  return;
                  }
               _lcdgotoxy(Col,Lig);
               }
            else
               {
               lcd_emiss('\r');
               lcd_emiss('\n');
               }
            return;
         case 8:
            if(!_CTRL) return;
            if(Femiss) return;
            if(Col<=1) return;
            Col--;
            _direct_lcdputs("\033[D");
            return;
            /*
             //ASCII étendu
             case (unsigned char)'é': lcd_emiss(0x82); return;
             case (unsigned char)'è': lcd_emiss(0x8A); return;
             case (unsigned char)'ë': lcd_emiss(0x89); return;
             case (unsigned char)'ê': lcd_emiss(0x88); return;
             case (unsigned char)'à': lcd_emiss(0x85); return;
             case (unsigned char)'â': lcd_emiss(0x83); return;
             case (unsigned char)'ù': lcd_emiss(0x97); return;
             case (unsigned char)'û': lcd_emiss(0x96); return;
             case (unsigned char)'ü': lcd_emiss(0x81); return;
             case (unsigned char)'ö': lcd_emiss(0x94); return;
             case (unsigned char)'ô': lcd_emiss(0x93); return;
             case (unsigned char)'ä': lcd_emiss(0x84); return;
             case (unsigned char)'ï': lcd_emiss(0x8B); return;
             case (unsigned char)'î': lcd_emiss(0x8C); return;
             case (unsigned char)'£': lcd_emiss(0x9C); return;
             case (unsigned char)'ç': lcd_emiss(0x87); return;
             */
#if !UTF8
         case (unsigned char)'é':
         case (unsigned char)'è':
         case (unsigned char)'ë':
         case (unsigned char)'ê':
         case (unsigned char)'à':
         case (unsigned char)'â':
         case (unsigned char)'ù':
         case (unsigned char)'û':
         case (unsigned char)'ü':
         case (unsigned char)'ö':
         case (unsigned char)'ô':
         case (unsigned char)'ä':
         case (unsigned char)'ï':
         case (unsigned char)'î':
         case (unsigned char)'£':
         case (unsigned char)'ç':
            break;
#else
         case 0xE9: //é
         case 0xE8: //è
         case 0xEB: //ë
         case 0xEA: //ê
         case 0xE0: //à
         case 0xE2: //â
         case 0xF9: //ù
         case 0xFB: //û
         case 0xFC: //ü
         case 0xF6: //ö
         case 0xF4: //ô
         case 0xE4: //ä
         case 0xEF: //ï
         case 0xEE: //î
         case 0xA3: //£
         case 0xE7: //ç
         case 0xB5: //µ
         case 0xB0: //°
            break;
#endif
         default:
            c='?';
            break;
         }
      }

   lcd_emiss(c);
   if(Femiss) return;
   if(!_CTRL) return;
   Col++;
   if(Col>NBCOL)
      {
      Col=1;
      Lig++;
      if(Lig>NBLIG) _lcdgotoxy(1,1);
      }
   }

#if UTF8
static void _lcdputc(unsigned char c)
   {
   static uint16_t m=0;
   if((c==0xC3)||(c==0xC2)) {m=c<<8; return;}
   if(m)
      {
      m|=c;
      switch(m)
         {
         case 0xC3A9: c=0xE9; break; //é
         case 0xC3A8: c=0xE8; break; //è
         case 0xC3AB: c=0xEB; break; //ë
         case 0xC3AA: c=0xEA; break; //ê
         case 0xC3A0: c=0xE0; break; //à
         case 0xC3A2: c=0xE2; break; //â
         case 0xC3B9: c=0xF9; break; //ù
         case 0xC3BB: c=0xFB; break; //û
         case 0xC3BC: c=0xFC; break; //ü
         case 0xC3B6: c=0xF6; break; //ö
         case 0xC3B4: c=0xF4; break; //ô
         case 0xC3A4: c=0xE4; break; //ä
         case 0xC3AF: c=0xEF; break; //ï
         case 0xC3AE: c=0xEE; break; //î
         case 0xC2A3: c=0xA3; break; //£
         case 0xC3A7: c=0xE7; break; //ç
         case 0xC2B5: c=0xB5; break; //µ
         case 0xC2B0: c=0xB0; break; //°
         default:
            m=0;
            break;
         }
      m=0;
      }
   if(_LCD) lcdputc_lcd(c);
   else
      lcdputc_vt(c);
   }
#else
static void _lcdputc(unsigned char c)
   {
   if(_LCD) lcdputc_lcd(c);
   else
      lcdputc_vt(c);
   }
#endif

void lcdputc(unsigned char c)
   {
   WaitIfRunning();
   if(!_FOpen)
      {
      RunComplete();
      return;
      }
   _lcdputc(c);
   RunComplete();
   }

static char *_utoab(register unsigned n, unsigned base);

void lcdtextcolor(uint8_t r, uint8_t g, uint8_t b)
   {
   WaitIfRunning();
   if(!_FOpen)
      {
      RunComplete();
      return;
      }
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   _direct_lcdputs("\033[XC");
   _direct_lcdputs(_utoab(r,10));
   lcd_emiss(';');
   _direct_lcdputs(_utoab(g,10));
   lcd_emiss(';');
   _direct_lcdputs(_utoab(b,10));
   lcd_emiss('#');
   RunComplete();
   }

static void _lcdgotoxy(unsigned char x, unsigned char y)
   {
   if(_LCD)
      {
      lcd_emiss(3);
      lcd_emiss(Lig=y);
      lcd_emiss(Col=x);
      }
   else
      {
      if(_CTRL)
         {
         lcd_emiss('\033');
         lcd_emiss('[');
         _direct_lcdputs(_utoab(Lig=y,10));
         lcd_emiss(';');
         _direct_lcdputs(_utoab(Col=x,10));
         lcd_emiss('f');
         }
      else
         {
         Lig=y;
         Col=x;
         }
      }
   }

void lcdgotoxy(unsigned char x, unsigned char y)
   {
   WaitIfRunning();
   if(!_FOpen)
      {
      RunComplete();
      return;
      }
   _lcdgotoxy(x,y);
   RunComplete();
   }

static void _lcdputs(const char *str)
   {
   while(*str)
      _lcdputc(*(str++));
   }

void lcdputs(const char *str)
   {
   WaitIfRunning();
   if(!_FOpen)
      {
      RunComplete();
      return;
      }
   _lcdputs(str);
   RunComplete();
   }


void lcdclrscr(void)
   {
   WaitIfRunning();
   if(!_FOpen)
      {
      RunComplete();
      return;
      }
   if(_LCD)
      {
      lcd_emiss(12);
      Col=Lig=1;
      }
   else
      {
      if(_CTRL) _direct_lcdputs("\033[2J");
      _lcdgotoxy(1,1);
      }
   RunComplete();
   }

void lcdclreol(void)
   {
   WaitIfRunning();
   if(!_FOpen)
      {
      RunComplete();
      return;
      }
   _lcdputc('\t');
   RunComplete();
   }

void lcdhome(void)
   {
   WaitIfRunning();
   if(!_FOpen)
      {
      RunComplete();
      return;
      }
   if(_LCD)
      {
      lcd_emiss(1);
      Col=Lig=1;
      }
   else
      {
      if(_CTRL) _direct_lcdputs("\033[H");
      Col=Lig=1;
      }
   RunComplete();
   }

void lcdcursor(unsigned char curs)
   {
   if(!_FOpen)
      {
      RunComplete();
      return;
      }
   WaitIfRunning();
   if(_LCD)
      {
      curs%=3;
      lcd_emiss(curs+4);
      }
   else
      {
      if(_CTRL)
         {
         switch(curs)
            {
            case 0:
               _direct_lcdputs("\033[?25l");  //efface curseur
               RunComplete();
               return;
            case 1:
               _direct_lcdputs("\033[?25h");  //affiche curseur
               RunComplete();
               return;
            }
         }
      }
   RunComplete();
   }

#include <ctype.h>
static char _s[64];
static const char HEX[]="0123456789ABCDEF";
static unsigned char PREC=2;

static char *_utoab(register unsigned n, unsigned base)
   {
   unsigned sz, n1;
   if((base<=1)||(base>16)) base=10;
   sz=sizeof(_s);
   _s[--sz]=0;
   if(!n) _s[--sz]='0';
   else
      while(sz&&n)
         {
         n1=n/base;
         _s[--sz]=HEX[n-n1*base];
         n=n1;
         }
   return _s+sz;
   }

static char *_ultoab(unsigned long n, unsigned base)
   {
   unsigned sz;
   unsigned long n1;
   if((base<=1)||(base>16)) base=10;
   sz=sizeof(_s);
   _s[--sz]=0;
   if(!n) _s[--sz]='0';
   else
      while(sz&&n)
         {
         n1=n/base;
         _s[--sz]=HEX[n-n1*base];
         n=n1;
         }
   return _s+sz;
   }

static char *_utoan(register unsigned n, register unsigned nb)
   {
   unsigned sz, n1;
   sz=sizeof(_s);
   _s[--sz]=0;
   while(sz&&(nb||n))
      {
      n1=n/10;
      _s[--sz]=HEX[n-n1*10];
      n=n1;
      nb--;
      }
   return _s+sz;
   }

static char *_ultoan(unsigned long n, unsigned nb)
   {
   unsigned sz;
   unsigned long n1;
   sz=sizeof(_s);
   _s[--sz]=0;
   while(sz&&(nb||n))
      {
      n1=n/10;
      _s[--sz]=HEX[n-n1*10];
      n=n1;
      nb--;
      }
   return _s+sz;
   }

static char *_utoanb(unsigned n, unsigned nb, unsigned base)
   {
   unsigned sz, n1;
   if((base<=1)||(base>16)) base=10;
   sz=sizeof(_s);
   _s[--sz]=0;
   while(sz&&(nb||n))
      {
      n1=n/base;
      _s[--sz]=HEX[n-n1*base];
      n=n1;
      if(nb!=0) nb--;
      }
   return _s+sz;
   }

static char *_ultoanb(unsigned long n, unsigned nb, unsigned base)
   {
   unsigned sz;
   unsigned long n1;
   if((base<=1)||(base>16)) base=10;
   sz=sizeof(_s);
   _s[--sz]=0;
   while(sz&&(nb||n))
      {
      n1=n/base;
      _s[--sz]=HEX[n-n1*base];
      n=n1;
      if(nb!=0) nb--;
      }
   return _s+sz;
   }

static char *_ufractoa(register unsigned n, register unsigned d)
   {
   char *se;
   unsigned t, i, p;
   if(!d) return 0;
   p=PREC;
   t=n/d;
   se=_utoab(t,10);
   for(i=0; *se; i++)
      _s[i]=*(se++);
   if(n) _s[i++]='.';
   for(; (i<sizeof(_s)-1)&&n; i++)
      {
      if(!p--) break;
      n-=d*t;
      n*=10;
      t=n/d;
      _s[i]=HEX[t];
      }
   _s[i]=0;
   return _s;
   }

static char *_ulfractoa(register unsigned long n, register unsigned d)
   {
   char *se;
   unsigned long t;
   unsigned i, p;
   if(!d) return 0;
   p=PREC;
   t=n/d;
   se=_ultoab(t,10);
   for(i=0; *se; i++)
      _s[i]=*(se++);
   if(n) _s[i++]='.';
   for(; (i<sizeof(_s)-1)&&n; i++)
      {
      if(!p--) break;
      n-=d*t;
      n*=10;
      t=n/d;
      _s[i]=HEX[t];
      }
   _s[i]=0;
   return _s;
   }

static char *_itoa(register int n)
   {
   int sz, n1;
//   static bit g;
   sz=sizeof(_s);
   _s[--sz]=0;
   if(n<0)
      {
      _bitg=1;
      n=-n;
      }
   else
      _bitg=0;
   if(!n) _s[--sz]='0';
   else
      {
      while(sz&&n)
         {
         n1=(unsigned)n/10;
         _s[--sz]=HEX[n-n1*10];
         n=n1;
         }
      }
   if(_bitg&&sz) _s[--sz]='-';
   return _s+sz;
   }

static char *_ltoa(long n)
   {
   int sz;
   long n1;
   //static bit g;
   sz=sizeof(_s);
   _s[--sz]=0;
   if(n<0)
      {
      _bitg=1;
      n=-n;
      }
   else
      _bitg=0;
   if(!n) _s[--sz]='0';
   else
      {
      while(sz&&n)
         {
         n1=n/10;
         _s[--sz]=HEX[n-n1*10];
         n=n1;
         }
      }
   if(_bitg&&sz) _s[--sz]='-';
   return _s+sz;
   }

static char *_itoan(int n, int nb)
   {
   int sz;
   int n1;
   //static bit g;
   sz=sizeof(_s);
   _s[--sz]=0;
   if(n<0)
      {
      _bitg=1;
      n=-n;
      nb--;
      }
   else
      _bitg=0;
   if(!n) _s[--sz]='0';
   else
      {
      while(sz&&(nb||n))
         {
         n1=n/10;
         _s[--sz]=HEX[n-n1*10];
         n=n1;
         if(nb!=0) nb--;
         }
      }
   if(_bitg&&sz) _s[--sz]='-';
   return _s+sz;
   }

static char *_ltoan(long n, int nb)
   {
   int sz;
   long n1;
   //static bit g;
   sz=sizeof(_s);
   _s[--sz]=0;
   if(n<0)
      {
      _bitg=1;
      n=-n;
      nb--;
      }
   else
      _bitg=0;
   if(!n) _s[--sz]='0';
   else
      {
      while(sz&&(nb||n))
         {
         n1=n/10;
         _s[--sz]=HEX[n-n1*10];
         n=n1;
         nb--;
         }
      }
   if(_bitg&&sz) _s[--sz]='-';
   return _s+sz;
   }

static char *_ifractoa(register int n, register int d)
   {
   char *se;
   int t, i, p;
   if(!d) return 0;
   p=PREC;
   t=n/d;
   if(!t)
      {
      if(n<0) se="-0";
      else
         se="0";
      }
   else
      se=_itoa(t);
   if(t<0) t=-t;
   if(n<0) n=-n;
   for(i=0; *se; i++)
      _s[i]=*(se++);
   _s[i++]='.';
   for(; (i<(int)sizeof(_s)-1)&&n; i++)
      {
      if(!p--) break;
      n-=d*t;
      n*=10;
      t=n/d;
      _s[i]=HEX[t];
      }
   _s[i]=0;
   return _s;
   }

static char *_lfractoa(register long n, register int d)
   {
   char *se;
   long t;
   int i, p;
   if(!d) return 0;
   p=PREC;
   t=n/d;
   if(!t)
      {
      if(n<0) se="-0";
      else
         se="0";
      }
   else
      se=_ltoa(t);
   if(t<0) t=-t;
   if(n<0) n=-n;
   for(i=0; *se; i++)
      _s[i]=*(se++);
   _s[i++]='.';
   for(; (i<(int)sizeof(_s)-1)&&n; i++)
      {
      if(!p--) break;
      n-=d*t;
      n*=10;
      t=n/d;
      _s[i]=HEX[t];
      }
   _s[i]=0;
   return _s;
   }

static int _atois(const char **s)
   {
   int d;
   d=0;
   while(isdigit((int16_t )**s))
      {
      d=d*10+(**s-'0');
      (*s)++;
      }
   return d;
   }

#include <math.h>
static void _afff(float v)
   {
   extern float pow10f(float);
   int16_t lnv;
   uint32_t pe;
   uint8_t d, n;
   float dv;
   if(v<0.f)
      {
      _lcdputc('-');
      v=-v;
      }
   if(v==0.f)
      {
      _lcdputc('0');
      return;
      }
   lnv=log10f(v);
   if((lnv<-6)||(lnv>6))
      {
      if(lnv<0) lnv--;
      v*=pow10f((-lnv));
      }
   else
      lnv=0;
   if(!PREC) dv=0.5e-6;
   else
      dv=0.5*pow10f(-PREC);
   v+=dv;
   pe=v;
   _lcdputs(_ultoab(pe,10));
   _lcdputc('.');
   v-=pe;
   n=0;
   while(v>dv)
      {
      if(PREC)
         {
         if(n==PREC) break;
         }
      else
         {
         if(n>6) break;
         }
      v*=10.f;
      dv*=10.f;
      d=v;
      _lcdputc(d+'0');
      n++;
      v-=d;
      }
   if(lnv)
      {
      _lcdputc('E');
      _lcdputs(_itoa(lnv));
      }
   }

#include <stdarg.h>

void lcdprintf(const char *fmt, ...)
   {
   int x;
   long xl;
   unsigned nbc, mul;
   WaitIfRunning();
   if(!_FOpen)
      {
      RunComplete();
      return;
      }

   va_list ap;
   va_start(ap,fmt);
   while(*fmt)
      {
      switch(*fmt)
         {
         case '%':
            fmt++;
            nbc=0;
            PREC=0;
            mul=1;
            _Flong=0;
            if(isdigit((int16_t )*fmt))
               {
               nbc=_atois(&fmt);
               }
            if(*fmt=='.')
               {
               fmt++;
               PREC=_atois(&fmt);
               }
            if(*fmt=='*')
               {
               fmt++;
               mul=_atois(&fmt);
               if(mul==0) mul=1;
               }
            if(*fmt=='l')
               {
               fmt++;
               _Flong=1;
               }
            switch(*fmt)
               {
               case '%':
                  _lcdputc('%');
                  break;
               case 'c':
                  x=va_arg(ap,int);
                  _lcdputc(x);
                  break;
               case 's':
                  {
                  char *s=va_arg(ap,char*);
                  for(x=0; *s; x++)
                     {
                     if(PREC&&(x>=PREC)) break;
                     _lcdputc(*(s++));
                     }
                  break;
                  }
               case 'u':
                  if(!_Flong)
                     {
                     x=va_arg(ap,int);
                     if(!nbc) _lcdputs(_utoab(x,10));
                     else
                        _lcdputs(_utoan(x,nbc));
                     }
                  else
                     {
                     xl=va_arg(ap,long);
                     if(!nbc) _lcdputs(_ultoab(xl,10));
                     else
                        _lcdputs(_ultoan(xl,nbc));
                     }
                  break;
               case 'X':
               case 'x':
                  if(!_Flong)
                     {
                     x=va_arg(ap,int);
                     if(!nbc) _lcdputs(_utoab(x,16));
                     else
                        _lcdputs(_utoanb(x,nbc,16));
                     }
                  else
                     {
                     xl=va_arg(ap,long);
                     if(!nbc) _lcdputs(_ultoab(xl,16));
                     else
                        _lcdputs(_ultoanb(xl,nbc,16));
                     }
                  break;
               case 'B':
               case 'b':
                  if(!_Flong)
                     {
                     x=va_arg(ap,int);
                     if(!nbc) _lcdputs(_utoab(x,2));
                     else
                        _lcdputs(_utoanb(x,nbc,2));
                     }
                  else
                     {
                     xl=va_arg(ap,long);
                     if(!nbc) _lcdputs(_ultoab(xl,2));
                     else
                        _lcdputs(_ultoanb(xl,nbc,2));
                     }
                  break;
               case 'd':
                  if(!_Flong)
                     {
                     x=va_arg(ap,int);
                     if(!nbc) _lcdputs(_itoa(x));
                     else
                        _lcdputs(_itoan(x,nbc));
                     }
                  else
                     {
                     xl=va_arg(ap,long);
                     if(!nbc) _lcdputs(_ltoa(xl));
                     else
                        _lcdputs(_ltoan(xl,nbc));
                     }
                  break;
               case 'U':
                  if(!_Flong)
                     {
                     x=va_arg(ap,int);
                     _lcdputs(_ufractoa(x,mul));
                     }
                  else
                     {
                     xl=va_arg(ap,long);
                     _lcdputs(_ulfractoa(xl,mul));
                     }
                  break;
               case 'D':
                  if(!_Flong)
                     {
                     x=va_arg(ap,int);
                     _lcdputs(_ifractoa(x,mul));
                     }
                  else
                     {
                     xl=va_arg(ap,long);
                     _lcdputs(_lfractoa(xl,mul));
                     }
                  break;
               case 'f':
                  {
                  double xd;
                  float xf;
                  xd=va_arg(ap,double);
                  xf=xd;
                  _afff(xf);
                  }
               }
            break;
         default:
            _lcdputc(*fmt);
            break;
         }
      fmt++;
      }
   va_end(ap);
   RunComplete();
   }

#if 0
void TestCharSet(unsigned char c1, unsigned char c2)
   {
   unsigned char c;
   for(c=c1; c<=c2; c++)
      {
      lcdprintf("%2X : ",c);
      lcd_emiss(c);
      lcd_emiss('\r');lcd_emiss('\n');
      Col=Lig=1;
      }
   }
#endif

//Fonctions étendues (afficheur tactile)

#define ReadCom() GetChar()
#define SendComStr(s) _direct_lcdputs(s)
#define SendCom(x) lcd_emiss(x)

#include <stdlib.h>

static void _itoaE(char **buf, unsigned int d)
   {
   int div=1;
   while(d/div>=10)
      div*=10;

   while(div!=0)
      {
      int num=d/div;
      d=d%div;
      div/=10;
      *((*buf)++)=num+'0';
      }
   }

void lcdTextColor565(uint16_t c)
   {
   char buf[80], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='T';
   _itoaE(&p,c);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   RunComplete();
   }

void lcdTextColor(uint32_t c)
   {
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   uint16_t c565=C565(c);
   lcdTextColor565(c565);
   RunComplete();
   }

void lcdBackColor565(uint16_t c)
   {
   char buf[80], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='B';
   _itoaE(&p,c);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   RunComplete();
   }

void lcdBackColor(uint32_t c)
   {
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   uint16_t c565=C565(c);
   lcdBackColor565(c565);
   RunComplete();
   }

void lcdSetFont(const char *nf)
   {
   char buf[80], *p, *pf;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   pf=buf+sizeof(buf)-3;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='F';
   while(*nf&&(p!=pf))
      *(p++)=*(nf++);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   RunComplete();
   }

void lcdSetFontN(uint16_t NumFont, uint16_t Size)
   {
   char buf[80], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='f';
   _itoaE(&p,NumFont);
   *(p++)=';';
   _itoaE(&p,Size);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   RunComplete();
   }


//Définit et affiche une zone tactile (identifiant id avec id=0..15)
//aux coordonnées x,y (si négatif(s) : position(s) courante(s))
//Le texte est affiché avec les couleurs courantes et encadré par la couleur col565
//mul correspond à un coef d'agrandissement en hauteur pour la zone tactile en % (Ex : mul=150 -> h*1,5)
//quand zone touchée : renvoie ('A'+id)
//ESC[P<id>;<x>;<y>;<mul>C<col565>;<txt>[1AH]#
void lcdDefTouch(int16_t x, int16_t y, uint16_t mul, const char *txt, uint16_t col565, uint16_t id)
   {
   char buf[100], *p, *pf;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   pf=buf+sizeof(buf)-3;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='P';
   _itoaE(&p,id);
   *(p++)=';';
   if(x<0) *(p++)='-';
   else
      _itoaE(&p,x);
   *(p++)=';';
   if(y<0) *(p++)='-';
   else
      _itoaE(&p,y);
   *(p++)=';';
   _itoaE(&p,mul);
   *(p++)='C';
   _itoaE(&p,col565);
   *(p++)=';';
   while(*txt&&(p!=pf))
      *(p++)=*(txt++);
   *(p++)=0x1A;
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   RunComplete();
   }

//Désactive la zone tactile d'identifiant id
void lcdUndefTouch(uint16_t id)
   {
   char buf[100], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='N';
   _itoaE(&p,id);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete()) ;
   RunComplete();
   }

void lcdSetSpc(uint16_t nb)
   {
   char buf[100], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='e';
   _itoaE(&p,nb);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete()) ;
   RunComplete();
   }

void lcdSetIncrust(uint16_t on)
   {
   char buf[100], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='i';
   *(p++)=(on?'1':'0');
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete()) ;
   RunComplete();
   }



void lcdDefTextArea(uint16_t xn, uint16_t xm, uint16_t yn, uint16_t ym)
   {
   char buf[80], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='A';
   _itoaE(&p,xn);
   *(p++)=';';
   _itoaE(&p,xm);
   *(p++)=';';
   _itoaE(&p,yn);
   *(p++)=';';
   _itoaE(&p,ym);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   RunComplete();
   }

static void TwoParam(uint16_t x,uint16_t y,char cmd)
   {
   char buf[80],*p;
   if(!_Extended) return;
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)=cmd;
   _itoaE(&p,x);
   *(p++)=';';
   _itoaE(&p,y);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());

   }


void lcdGotoXYa(uint16_t x, uint16_t y)
   {
   char buf[80], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='G';
   _itoaE(&p,x);
   *(p++)=';';
   _itoaE(&p,y);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   RunComplete();
   }

void lcdRectFull(uint16_t xn, uint16_t xm, uint16_t yn, uint16_t ym, uint16_t col)
   {
   char buf[100], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='r';
   _itoaE(&p,xn);
   *(p++)=';';
   _itoaE(&p,xm);
   *(p++)=';';
   _itoaE(&p,yn);
   *(p++)=';';
   _itoaE(&p,ym);
   *(p++)='C';
   _itoaE(&p,col);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   RunComplete();
   }

void lcdRect(uint16_t xn, uint16_t xm, uint16_t yn, uint16_t ym, uint16_t col)
   {
   char buf[100], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='R';
   _itoaE(&p,xn);
   *(p++)=';';
   _itoaE(&p,xm);
   *(p++)=';';
   _itoaE(&p,yn);
   *(p++)=';';
   _itoaE(&p,ym);
   *(p++)='C';
   _itoaE(&p,col);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   RunComplete();
   }

void lcdSetScroll(uint16_t c)
   {
   char buf[80], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='S';
   _itoaE(&p,c);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   RunComplete();
   }

void lcdSetOutWind(uint16_t c)
   {
   char buf[80], *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='O';
   _itoaE(&p,c);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   RunComplete();
   }

void lcdPushTxtPos(void)
   {
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   SendComStr("\033[M");
   while(!TxComplete())
      ;
   RunComplete();
   }

void lcdPopTxtPos(void)
   {
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   SendComStr("\033[m");
   while(!TxComplete())
      ;
   RunComplete();
   }

void lcdClearAll(void)
   {
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   SendComStr("\033[X");
   while(!TxComplete())
      ;
   RunComplete();
   }


//fixe la position d'écriture en absolu dans la fenêtre graphique
void lcdMoveTo(uint16_t x, uint16_t y)
   {
   TwoParam(x,y,'v');
   }

//trace une ligne jusqu'à la position indiquée dans fenêtre graphique
void lcdLineTo(uint16_t x, uint16_t y)
   {
   TwoParam(x,y,'L');
   }

//trace un pixel dans fenêtre graphique (couleur courante)
void lcdPutPixel(uint16_t x, uint16_t y)
   {
   TwoParam(x,y,'p');
   }

void lcdCircle(uint16_t r)
   {
   char buf[80],*p;
   if(!_Extended) return;
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='C';
   _itoaE(&p,r);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   }

void lcdCircleXY(uint16_t x, uint16_t y, uint16_t r)
   {
   char buf[100],*p;
   if(!_Extended) return;
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='c';
   _itoaE(&p,x);
   *(p++)=';';
   _itoaE(&p,y);
   *(p++)=';';
   _itoaE(&p,r);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   }


//trace un pixel dans fenêtre graphique dans la couleur indiquée
void lcdPutPixelC(uint16_t x, uint16_t y, uint16_t col565)
   {
   char buf[80],*p;
   if(!_Extended) return;
   p=buf;
   *(p++)='\033';
   *(p++)='[';
   *(p++)='p';
   _itoaE(&p,x);
   *(p++)=';';
   _itoaE(&p,y);
   *(p++)='C';
        _itoaE(&p,col565);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   }


#include <stdio.h>
void lcdAffLGif(uint16_t dur, const char *liste)
   {
   char buf[128];
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return;
      }
   sprintf(buf,"\033[b%u;%s#",dur,liste);
   SendComStr(buf);
//   while(!TxComplete());
   RunComplete();
   }



//Fonctions d'interrogation :
//Les fonctions renvoient du texte sous la forme @nnn#, où nnn est une valeur numérique, # est le caractère de fin

/*
int32_t MAX_READREP,DT_REP;
int16_t ERRREP,CHARREP;
char REPBUFI[20]; uint16_t NBREPBUFI;
*/
static char *ReadRep(char *buf, uint16_t sz)
   {
   int16_t x;
   uint32_t t,t0;
   int32_t dt;
   char *p, *pf;

   //DelaiMs(10); while(GestComm()!=-1); //POUR LE TEST !!! ... faire le traitement de séq ESC

   t0=HAL_GetTick();
   t=t0+TIMEOUT_READREP;
   /*
   NBREPRBUF1=NBREPRBUF;
   if(!NBREPBUFI)
      {
      for(uint16_t i=0; i<NBREPRBUF1; i++) REPBUFI[i]=REPRBUF[i];
      NBREPBUFI=NBREPRBUF1;
      }
      */
   do
      {
      x=ReadCom();
      if(x=='@') break;
      dt=t-HAL_GetTick();
      }
      while(dt>=0);
   if(x!='@')
      {
 /*

       if(!ERRREP)
         {
         ERRREP=1;
         CHARREP=x;
         DT_REP=dt;
         t=HAL_GetTick()-t0;
         if(t>MAX_READREP) MAX_READREP=t;
         NBREPRBUF0=NBREPRBUF;
         }
         */
      return 0;
      }
   p=buf;
   pf=buf+sz-2;
   do
      {
      x=ReadCom();
      if(x=='#') break;
      if(x!=-1)
         {
         if(p!=pf)
            {
            *p=x;
            p++;
            }
         }
      }
      while(((int32_t)(t-HAL_GetTick()))>=0);
   *p=0;
   /*
   t=HAL_GetTick()-t0;
   if(t>MAX_READREP) MAX_READREP=t;
   */
   return buf;
   }

int16_t lcdGetTextWidth(const char *txt)
   {
   char buf[80], *p, *pf;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   pf=buf+sizeof(buf)-3;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='W';
   while(*txt&&(p!=pf))
      *(p++)=*(txt++);
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete()) ;
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }

int16_t lcdGetNbFont(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='f';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }


int16_t lcdGetPixelWidth(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='L';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }

int16_t lcdGetPixelHeight(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='H';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }

int16_t lcdGetPoliceHeight(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='h';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }

int16_t lcdGetAbsX(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='X';
   *(p++)='0';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }

int16_t lcdGetX(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='X';
   *(p++)='1';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }

int16_t lcdGetAbsY(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='Y';
   *(p++)='0';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }

int16_t lcdGetY(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='Y';
   *(p++)='1';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }

int16_t lcdGetSpc(void)
   {
   char buf[20], *p;
   int16_t n;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return -1;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='e';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete());
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return -1;
      }
   n=atoi(buf);
   RunComplete();
   return n;
   }


char *lcdGetK(void)
   {
   static char buf[20];
   char *p;
   WaitIfRunning();
   if(!_Extended)
      {
      RunComplete();
      return 0;
      }
   p=buf;
   *(p++)='\033';
   *(p++)='@';
   *(p++)='K';
   *(p++)='#';
   *p=0;
   SendComStr(buf);
   while(!TxComplete())
      ;
   if(!ReadRep(buf,sizeof(buf)))
      {
      RunComplete();
      return 0;
      }
   RunComplete();
   return buf;
   }

