// Pilotage Servos AX-RX série [1Mbps-56kbps]/8bits/1stop/No par.

#ifdef STM32H745xx
#include "stm32h7xx_hal.h"
#endif

#ifdef STM32F722xx
#include "stm32f7xx_hal.h"
#endif

#include "gpio.h"
#include "usart.h"

#include "TempsMs.h"
#include "../../../../../Common/Src/LCD_VT100.h"
#include "../App/Config.h"
#include "CommandesServos.h"
#include "ServosBuf.h"


#define AX_USED 0
#define AFF_MUX 0

#define SENS_RD 0
#define SENS_WR 1

static unsigned char ID;

#define MODE_RX_RD 0
#define MODE_RX_WR 1

#define MODE_AX_RD 0
#define MODE_AX_WR 1

static void RXSetModeOut(void)
{
   _ServoRX_RW(MODE_RX_WR);          //en écriture
}

static void RXSetModeIn(void)
{
      _ServoRX_RW(MODE_RX_RD);
}


static void AXSetModeOut(void)
{
	_ServoAX_RW(MODE_AX_WR);
}

static void AXSetModeIn(void)
{
	_ServoAX_RW(MODE_AX_RD);
}


#define BUF_SZ 60  // Taille du buffer

typedef struct
{
   UART_HandleTypeDef *huart;
   void (*SetModeOut)(void);
   void (*SetModeIn)(void);
   uint16_t nb; //Nombre d'octets dans le buffer
   uint8_t chk; //Chksum
   uint8_t b[BUF_SZ];
   uint8_t data;
   uint8_t st_recept;
   unsigned REG_MODE:1;
   unsigned sent:1;
   unsigned received:1;
} T_Comm;

static volatile T_Comm CommRX={
      .huart=&huartRX,
      .SetModeOut=RXSetModeOut,
      .SetModeIn=RXSetModeIn,
      .st_recept=0,
      .sent=0,
      .received=0,
      .nb=0,
      .chk=0,
      };

static volatile T_Comm CommAX={
      .huart=&huartAX,
      .SetModeOut=AXSetModeOut,
      .SetModeIn=AXSetModeIn,
      .st_recept=0,
      .sent=0,
      .received=0,
      .nb=0,
      .chk=0,
      };

static volatile T_Comm *Comm;

void SetID(uint8_t id)
   {
   ID=(id)&(RXID(0)-1);
   if((id)&(RXID(0))) Comm=&CommRX;
   else Comm=&CommAX;
   }



/* Control Table
========= EEPROM Area :
 Address  	 Item  	                        Access  	 Initial Value  
 0(0X00)  	 Model Number(L)  	            RD  	      12(0x0C)  
 1(0X01)  	 Model Number(H)  	            RD  	      0(0x00)  
 2(0X02)  	 Version of Firmware  	         RD  	      ?  
 3(0X03)  	 ID  	                           RD,W R  	   1(0x01)  
 4(0X04)  	 Baud Rate  	                  RD,W R  	   1(0x01)  
 5(0X05)  	 Return Delay Time  	            RD,W R  	   250(0xFA)  
 6(0X06)  	 CW Angle Limit(L)  	            RD,W R  	   0(0x00)  
 7(0X07)  	 CW Angle Limit(H)  	            RD,W R  	   0(0x00)  
 8(0X08)  	 CCW Angle Limit(L)  	         RD,W R  	   255(0xFF)  
 9(0X09)  	 CCW Angle Limit(H)  	         RD,W R  	   3(0x03)  
 10(0x0A)  	 (Reserved)  	                  - 	         0(0x00)  
 11(0X0B)  	 the Highest Limit Temperature  	RD,W R  	   85(0x55)  
 12(0X0C)  	 the Lowest Limit Voltage  	   RD,W R  	   60(0X3C)  
 13(0X0D)  	 the Highest Limit Voltage  	   RD,W R  	   190(0xBE)  
 14(0X0E)  	 Max Torque(L)  	               RD,W R  	   255(0XFF)  
 15(0X0F)  	 Max Torque(H)  	               RD,W R  	   3(0x03)  
 16(0X10)  	 Status Return Level  	         RD,W R  	   2(0x02)  
 17(0X11)  	 Alarm LED  	                  RD,W R  	   4(0x04)  
 18(0X12)  	 Alarm Shutdown  	               RD,W R  	   4(0x04)  
 19(0X13)  	 (Reserved)  	                  RD,W R  	   0(0x00)  
 20(0X14)  	 Down Calibration(L)  	         RD  	      ?  
 21(0X15)  	 Down Calibration(H)  	         RD  	      ?  
 22(0X16)  	 Up Calibration(L)  	            RD  	      ?  
 23(0X17)  	 Up Calibration(H)  	            RD  	      ?  
 ========= RAM Area :
 24(0X18)  	 Torque Enable  	               RD,W R  	   0(0x00)  
 25(0X19)  	 LED  	                        RD,W R  	   0(0x00)  
 26(0X1A)  	 CW Compliance Margin  	         RD,W R  	   0(0x00)  
 27(0X1B)  	 CCW Compliance Margin  	      RD,W R  	   0(0x00)  
 28(0X1C)  	 CW Compliance Slope  	         RD,W R  	   32(0x20)  
 29(0X1D)  	 CCW Compliance Slope  	         RD,W R  	   32(0x20)  
 30(0X1E)  	 Goal Position(L)  	            RD,W R  	   [Addr36]value  
 31(0X1F)  	 Goal Position(H)  	            RD,W R  	   [Addr37]value  
 32(0X20)  	 Moving Speed(L)  	            RD,W R  	   0  
 33(0X21)  	 Moving Speed(H)  	            RD,W R  	   0  
 34(0X22)  	 Torque Limit(L)  	            RD,W R  	   [Addr14] value  
 35(0X23)  	 Torque Limit(H)  	            RD,W R  	   [Addr15] value  
 36(0X24)  	 Present Position(L)  	         RD  	      ?  
 37(0X25)  	 Present Position(H)  	         RD  	      ?  
 38(0X26)  	 Present Speed(L)  	            RD  	      ?  
 39(0X27)  	 Present Speed(H)  	            RD  	      ?  
 40(0X28)  	 Present Load(L)  	            RD  	      ?  
 41(0X29)  	 Present Load(H)  	            RD  	      ?  
 42(0X2A)  	 Present Voltage  	            RD  	      ?  
 43(0X2B)  	 Present Temperature  	         RD  	      ?  
 44(0X2C)  	 Registered Instruction  	      RD,W R  	   0(0x00)  
 45(0X2D)  	 (Reserved)  	                  - 	         0(0x00)  
 46[0x2E)  	 Moving  	                     RD  	      0(0x00)  
 47[0x2F)  	 Lock  	                        RD,W R  	   0(0x00)  
 48[0x30)  	 Punch(L)  	                     RD,W R  	   32(0x20)  
 49[0x31)  	 Punch(H)  	                     RD,W R  	   0(0x00)
 
 ************************************
 *          Instruction Set :       *
 ************************************
  Instruction  Value  	 Nb_Param  	 Function  
 PING  	      0x01  	 0  	      No action. Used for obtaining a Status Packet  
 READ DATA  	0x02  	 2  	      Reading values in the Control Table  
 WRITE DATA  	0x03  	 2 ~  	   Writing values to the Control Table  
 REG WRITE  	0x04  	 2 ~  	   Similar to WRITE_DATA, but stays in standby mode until the ACION instruction is given  
 ACTION  	   0x05  	 0  	      Triggers the action registered by the REG_WRITE instruction  
 RESET  	      0x06  	 0  	      Changes the control table values of the Dynamixel actuator to the Factory Default Value settings  
 SYNC WRITE  	0x83  	 4~  	      Used for controlling many Dynamixel actuators at the same time   
*/

#define C_PING  	    0x01
#define C_READ_DATA  	0x02
#define C_WRITE_DATA  	0x03
#define C_REG_WRITE  	0x04
#define C_ACTION  	   	0x05
#define C_RESET  	    0x06
#define C_SYNC_WRITE  	0x83

/*
#define ERREUR.W 		0x11
#define CLEAR_ERREUR	0x12
*/

typedef union
   {
	uint16_t w;
      struct {unsigned low:8; unsigned high:8;}b;
   }TWB;   


typedef union
   {
   unsigned W;
   struct
      {
      unsigned char low,high;
      }B;
   struct 
      {
      unsigned InputVoltage:1; //Set to 1 if the voltage is out of the operating voltage range as defined in the control table.
      unsigned AngleLimit:1; //Set as 1 if the Goal Position is set outside of the range between CW Angle Limit and CCW Angle Limit
      unsigned Overheating:1; //Set to 1 if the internal temperature of the Dynamixel unit is above the operating temperature range as defined in the control table.
      unsigned Range:1; //Set to 1 if the instruction sent is out of the defined range.
      unsigned Checksum:1; //Set to 1 if the checksum of the instruction packet is incorrect.
      unsigned Overload:1; //Set to 1 if the specified maximum torque can't control the applied load.
      unsigned Instruction:1; //Set to 1 if an undefined instruction is sent or an action instruction is sent without a Reg_Write instruction.
      unsigned :1;
      unsigned NoFF1:1;        // Pas de r�ception 0xFF en d�but de trame
      unsigned NoFF2:1;        // Pas de r�ception 0xFF en d�but de trame
      unsigned NoResponse:1;  // Timeout en r�ception
      unsigned DataLoss:1;    // Perte de donn�es en r�ception
      }b;
   }T_ERREUR;   

static T_ERREUR ERREUR;


unsigned GetSvServoErr(void)
{
   return ERREUR.W;
}

const char *GetSvErrorMsg(void)
   {
   if(ERREUR.W==0) return "";
   if(ERREUR.b.DataLoss)
      {
      ERREUR.b.DataLoss=0;
      return "Perte de donnee";
      }
   if(ERREUR.b.NoResponse)
      {
      ERREUR.b.NoResponse=0;
      return "Pas de reponse";
      }
   if(ERREUR.b.NoFF1)
      {
      ERREUR.b.NoFF1=0;
      return "0xFF (1) manquant";
      }   
   if(ERREUR.b.NoFF2)
      {
      ERREUR.b.NoFF2=0;
      return "0xFF (2) manquant";
      }
   if(ERREUR.b.Instruction)
      {
      ERREUR.b.Instruction=0;
      return "Instruction invalide";
      }
   if(ERREUR.b.Overload)
      {
      ERREUR.b.Overload=0;
      return "Couple excessif";
      }
   if(ERREUR.b.Checksum)
      {
      ERREUR.b.Checksum=0;
      return "Checksum incorrect";
      }
   if(ERREUR.b.Range)
      {
      ERREUR.b.Range=0;
      return "Valeur de parametre invalide";
      }
   if(ERREUR.b.Overheating)
      {
      ERREUR.b.Overheating=0;
      return "Surchauffe";
      }
   if(ERREUR.b.AngleLimit)
      {
      ERREUR.b.AngleLimit=0;
      return "Angle destination hors zone";
      }
   if(ERREUR.b.InputVoltage)
      {
      ERREUR.b.InputVoltage=0;
      return "Tension d'alimentation incorrecte";
      }
   return "";               
   }   

//Fonction qui annule l'erreur des servos--> mais ca marche pas!!!!!
void ClearServoErr(void)
{
   ERREUR.W=0;
  // WriteByte(0xFE,0x04,0x11);
}   




static void emiss(unsigned char c)
{
   Comm->b[(Comm->nb)++]=c;
   Comm->chk+=c;
}

static void GenericHuart_Init(volatile T_Comm *c)
   {
   HAL_HalfDuplex_Init(c->huart);
   __HAL_UART_ENABLE_IT(c->huart,UART_IT_TC);
   __HAL_UART_ENABLE_IT(c->huart,UART_IT_RXNE);
   HAL_UART_Receive_IT(c->huart,(uint8_t*)&(c->data),1);
   }

void ServoInit(void)
{
    InitGCMD();
    InitSvNames();
    //  InitBUF();
    GenericHuart_Init(&CommRX);
    /*
    HAL_HalfDuplex_Init(&huartRX);
    __HAL_UART_ENABLE_IT(&huartRX,UART_IT_TC);
    __HAL_UART_ENABLE_IT(&huartRX,UART_IT_RXNE);
    HAL_UART_Receive_IT(&huartRX,(uint8_t*)&CommRX.data,1);
    */
    GenericHuart_Init(&CommAX);
    ERREUR.W=0;
}	


#include "../App/Tests.h"

static void GenericHuart_TxCpltCallback(volatile T_Comm *c)
   {
   uint16_t j;
   //RXSetModeIn();
   for(j=0; j<sizeof(c->b); j++) c->b[j]=0; //Initialise buffer de réception
   c->nb=0;
   c->sent=1;
   c->received=0;
   c->st_recept=0;
   }

void HuartRX_TxCpltCallback(void)
   {
   RXSetModeIn();
   GenericHuart_TxCpltCallback(&CommRX);
   /*
   uint16_t j;
   RXSetModeIn();
   for(j=0; j<sizeof(CommRX.b); j++) CommRX.b[j]=0; //Initialise buffer de réception
   CommRX.nb=0;
   CommRX.sent=1;
   CommRX.received=0;
   CommRX.st_recept=0;
   */
   }

void HuartAX_TxCpltCallback(void)
   {
   AXSetModeIn();
   GenericHuart_TxCpltCallback(&CommAX);
   }


static void store_recept(volatile T_Comm *c)
   {
   if(c->nb<sizeof(c->b)) c->b[c->nb++]=c->data;
   }

static void GenericHuart_RxCpltCallback(volatile T_Comm *c, uint16_t *nb)
   {
   switch(c->st_recept)
      {
      case 0:
         if(c->data==0xFF) {store_recept(c); c->st_recept=1; break;}
         //c'était quoi ??? On attend 0xFF
         break;
      case 1: //On a reçu précédemment 0xFF
         store_recept(c);
         if(c->data!=0xFF) //Réception id;
            {
            c->st_recept=2;
            break;
            }
         break;
      case 2: //On reçoit nb
         *nb=c->data;
         store_recept(c);
         c->st_recept=3;
         break;
      case 3: //Réception suite trame
         store_recept(c);
         (*nb)--;
         if(!(*nb))
            {
            c->st_recept=0;
            c->received=1;
            }
         break;
      }
   HAL_UART_Receive_IT(c->huart,(uint8_t*)&c->data,1);
   }

void HuartRX_RxCpltCallback(void)
   {
   static uint16_t nb;
//   volatile T_Comm *c=&CommRX;
   GenericHuart_RxCpltCallback(&CommRX,&nb);
/*
   switch(c->st_recept)
      {
      case 0:
         if(c->data==0xFF) {store_recept(c); c->st_recept=1; break;}
         //c'était quoi ??? On attend 0xFF
         break;
      case 1: //On a reçu précédemment 0xFF
         store_recept(c);
         if(c->data!=0xFF) //Réception id;
            {
            c->st_recept=2;
            break;
            }
         break;
      case 2: //On reçoit nb
         nb=c->data;
         store_recept(c);
         c->st_recept=3;
         break;
      case 3: //Réception suite trame
         store_recept(c);
         nb--;
         if(!nb)
            {
            c->st_recept=0;
            c->received=1;
            }
         break;
      }
   HAL_UART_Receive_IT(c->huart,(uint8_t*)&c->data,1);
   */
   }


void HuartAX_RxCpltCallback(void)
   {
   static uint16_t nb;
   GenericHuart_RxCpltCallback(&CommAX,&nb);
   }


//Emission continue sur Uart AX ou RX pour test
void ServoTestSend(void)
   {
   volatile T_Comm *c=&CommAX;
   uint8_t buf=0x3A;
   c->SetModeOut();
   while(1)
      {
      c->SetModeOut();
      c->sent=0;
      HAL_UART_Transmit_IT(c->huart,&buf,1);
      while(!c->sent);
      }
   }

uint16_t SV_NBERR;
static unsigned char * ServoSend(unsigned char cmd, unsigned char *prm, unsigned char nbprm)
{
   uint16_t i;

   HAL_StatusTypeDef err;
//   lcdprintf("CMD %02X ->%d [%s]\n",cmd,ID,_isRX?"RX":"AX");
//   DelaiMs(120);
   //SetModeAffOff();
   //if(_isRX) RXSetModeOut(); else AXSetModeOut();
   Comm->SetModeOut();
   
   Comm->nb=0;
   Comm->chk=0;

   emiss(0xFF);
   emiss(0xFF);
   Comm->chk=0;
   emiss(ID);
   emiss(nbprm+2);
   emiss(cmd);
   while(nbprm--) emiss(*(prm++));
   emiss(~Comm->chk);
   Comm->sent=0;

   __HAL_UART_CLEAR_OREFLAG(Comm->huart);
   __HAL_UART_CLEAR_NEFLAG(Comm->huart);

   HAL_UART_Transmit_IT(Comm->huart,(uint8_t*)Comm->b,Comm->nb);
   while(!Comm->sent);
   
   if(ID==BROADCASTING_ID)
      {
      return 0;
      } // Broadcasting id => pas de réponse




   __HAL_UART_CLEAR_OREFLAG(Comm->huart);
   __HAL_UART_CLEAR_NEFLAG(Comm->huart);

   //lcdprintf("Recept...");
   TickType_t tf;
   tf=xTaskGetTickCount()+20/portTICK_PERIOD_MS;
   err=0;
   while(!Comm->received)
      {
      vTaskDelay((1)/portTICK_PERIOD_MS);
      if(xTaskGetTickCount()>tf) {err=HAL_TIMEOUT; break;}
      }

   unsigned char *p;
   if(err==HAL_TIMEOUT) ERREUR.b.NoResponse=1;
   else ERREUR.b.NoResponse=0;
   //if(ERREUR.b.NoResponse) lcdputc('*');

   if(err==HAL_TIMEOUT)
      {
	  ERREUR.b.NoResponse=1;
	  SV_NBERR++;
	  return 0;
      }
   else ERREUR.b.NoResponse=0;
   

   for(p=(uint8_t*)Comm->b; *p==0xFF; p++);
   if(p-Comm->b <2)
      {
	   return 0;
      }
   p-=2;
   i=2;
   if(p[i]!=ID)
   {
	   return 0;
   }

   Comm->chk=p[i++]; //id
   Comm->chk+=Comm->nb=p[i++]; //longueur
   if(Comm->nb>40)
      {
	   return 0;
      }
   Comm->nb+=4;
   do
      {
      Comm->chk+=p[i++];
      }
      while(i<Comm->nb);
   ERREUR.B.low=p[4];
//   lcdprintf("Err=%2X  Chk=%2X \n",ServoBuf[4],CHK);
   if(Comm->chk!=0xFF)
      {
      return 0;
      }
   return p+2;
}

unsigned char ServoPing(unsigned id)
   {
   unsigned char *r;
   ERREUR.W=0;
   SetID(id);
   r=ServoSend(C_PING,0,0);
   if(r) return r[2];
   return 0xFF;
   }

unsigned char ServoReset(unsigned id)
   {
   unsigned char *r;
   ERREUR.W=0;
   SetID(id);
   r=ServoSend(C_RESET,0,0);
   if(r) return r[2];
   return 0xFF;
   }
   
void ServoAction(void)
   {
   ERREUR.W=0;
   ID=BROADCASTING_ID;
   ServoSend(C_ACTION,0,0);
   }

static unsigned char *ReadData(unsigned char adb, unsigned char nb)
{
   unsigned char *r;
   unsigned char ServoBuf[5];
   ServoBuf[0]=adb;
   ServoBuf[1]=nb;
   r=ServoSend(C_READ_DATA,ServoBuf,2);
   if(r) return r+3;
   return 0;
}



static unsigned char *ReadByte(unsigned char ad)
{
   unsigned char *r;
   unsigned char ServoBuf[5];
   ServoBuf[0]=ad;
   ServoBuf[1]=1;
   r=ServoSend(C_READ_DATA,ServoBuf,2);
   if(r) return r+3;
   return 0;
}

static uint16_t *ReadWord(unsigned char ad)
{
   unsigned char *r;
   static TWB w;
   unsigned char ServoBuf[5];
   ServoBuf[0]=ad;
   ServoBuf[1]=2;
   r=ServoSend(C_READ_DATA,ServoBuf,2);
   if(!r) return 0;
   w.b.low=r[3];
   w.b.high=r[4];
   return &w.w;
}

//Fonction de test... Lit toute la mémoire du servo
unsigned char *ServoReadMem(void)
   {
   return ReadData(0,50);
   }   

/*
static unsigned char *WriteData(unsigned char id, unsigned char *data, unsigned char adb, unsigned char nb)
{
   unsigned char *r,i;
   i=0;
   ServoBuf[i++]=adb;
   while(nb--) ServoBuf[i++]=*(data++);
   if(REG_MODE) r=ServoSend(id,C_REG_WRITE,ServoBuf,i);
   else r=ServoSend(id,C_WRITE_DATA,ServoBuf,i);
   REG_MODE=0;
   if(r) return r+3;
   return 0;
}
*/
static unsigned char *WriteByte(unsigned char data, unsigned char ad)
{
   unsigned char *r;
   unsigned char ServoBuf[5];
   ServoBuf[0]=ad;
   ServoBuf[1]=data;
   if(Comm->REG_MODE) r=ServoSend(C_REG_WRITE,ServoBuf,2);
   else r=ServoSend(C_WRITE_DATA,ServoBuf,2);
   Comm->REG_MODE=0;
   if(r) return r+3;
   return 0;   
}

static unsigned char *WriteWord(unsigned data, unsigned char ad)
{
   unsigned char *r;
   TWB x;
   unsigned char ServoBuf[5];
   x.w=data;
   ServoBuf[0]=ad;
   ServoBuf[1]=x.b.low;//  ((TWB*)(&data))->b.low;
   ServoBuf[2]=x.b.high;// ((TWB*)(&data))->b.high;
   if(Comm->REG_MODE) r=ServoSend(C_REG_WRITE,ServoBuf,3);
   else r=ServoSend(C_WRITE_DATA,ServoBuf,3);
   Comm->REG_MODE=0;
   if(r) return r+3;
   return 0;   
}

unsigned char *SetAlarmShutdown(unsigned id, unsigned char alrm)
{
   ERREUR.W=0;
   SetID(id);
   return WriteByte(alrm,0x12);
}   

unsigned char *SetAlarmLED(unsigned id, unsigned char alrm)
{
   ERREUR.W=0;
   SetID(id);
   return WriteByte(alrm,0x11);
}   




//Positionne le servo � la position angulaire indiqu�e (en �/10, de 0 � 3000)
unsigned char *SetServoAngleDeg(unsigned id, unsigned angle)
   {
   ERREUR.W=0;
   SetID(id);
   if(angle>3000) angle=3000;
   angle = (1023.f/3000.f)*angle;
   return WriteWord(angle,0x1E);
   }

static struct
   {
   unsigned pos:15;
   unsigned def:1;
   } MEMSERVOANGLE[256];

static struct
   {
   unsigned trq:14;
   unsigned en:1;
   unsigned def:1;
   } MEMSERVOTORQUE[256];

//Positionne le servo � la position angulaire indiqu�e (0 � 1023 pour 0� � 300�)
unsigned char *SetServoAngle(unsigned id, unsigned angle)
   {
   ERREUR.W=0;
   if(angle>1023) angle=1023;
   MEMSERVOANGLE[id&0xFF].pos=angle;
   MEMSERVOANGLE[id&0xFF].def=1;
   SetID(id);
   return WriteWord(angle,0x1E);
   }

int GetServoCons(unsigned id)
   {
   if(!MEMSERVOANGLE[id&0xFF].def) return -1;
   return MEMSERVOANGLE[id&0xFF].pos;
   }   

void SetServoCons(unsigned id, unsigned angle)
   {
   if(angle>1023) angle=1023;
   MEMSERVOANGLE[id&0xFF].pos=angle;
   }   



uint16_t  *GetServoAngle(unsigned id)
   {
   ERREUR.W=0;
   SetID(id);
   return ReadWord(0x24);
   }   

uint16_t  *GetServoModel(unsigned id)
   {
   ERREUR.W=0;
   SetID(id);
   return ReadWord(0x00);
   }   


// sp=0...1023
unsigned char *SetServoSpeed(unsigned id, unsigned sp)
   {
   ERREUR.W=0;
   SetID(id);
   if(sp>1023) sp=1023;
   return WriteWord(sp,0x20);
   }

uint16_t  *GetServoSpeed(unsigned id)
   {
   ERREUR.W=0;
   SetID(id);
   return ReadWord(0x26);
   }   

// Couple appliqu�. (sign� 10 bits, n�gatif si CCW)
int16_t  *GetServoLoad(unsigned id)
   {
   int16_t *w;
   ERREUR.W=0;
   SetID(id);
   w=(int16_t*)ReadWord(0x28);
   if(!w) return w;
   *w&=0x3FF;
   //if(!((*w)&(1<<10))) *w=-(*w&0x3FF);
   return w;
   }   

unsigned char *SetServoMaxTorque(unsigned id, unsigned t)
   {
   ERREUR.W=0;
   if(t>1023) t=1023;
   MEMSERVOTORQUE[id&0xFF].trq=t;
   MEMSERVOTORQUE[id&0xFF].def=1;
   SetID(id);
   return WriteWord(t,0x22);
   }

uint16_t  *GetServoMaxTorque(unsigned id)
   {
   ERREUR.W=0;
   SetID(id);
   return ReadWord(0x22);
   }   


unsigned char *EnableServoTorque(unsigned id, unsigned en)
   {
   ERREUR.W=0;
   MEMSERVOTORQUE[id&0xFF].en=en;
   SetID(id);
   en=(en!=0);
   return WriteByte(en,0x18);
   }
   
int GetServoConsTorque(unsigned id)
   {
   if(!MEMSERVOTORQUE[id&0xFF].def) return -1;
   return MEMSERVOTORQUE[id&0xFF].trq;
   }   

int GetServoConsTorqueEn(unsigned id)
   {
   if(!MEMSERVOTORQUE[id&0xFF].def) return -1;
   return MEMSERVOTORQUE[id&0xFF].en;
   }   


void SetServoConsTorque(unsigned id, unsigned t)
   {
   if(t>1023) t=1023;
   MEMSERVOTORQUE[id&0xFF].trq=t;
   }   

   

int GetServoTorqueStatus(unsigned id)
{
   unsigned char *b;
   ERREUR.W=0;
   SetID(id);
   b=ReadByte(0x18);
   if(b)return *b;
   return -1;
}   

unsigned char *ServoStatusReturnLevel(unsigned id, unsigned st)
   {
   ERREUR.W=0;
   SetID(id);
   return WriteByte(st,0x10);
   }

int GetServoStatusReturnLevel(unsigned id)
   {
   unsigned char *b;
   ERREUR.W=0;
   SetID(id);
   b=ReadByte(0x10);
   if(b) return *b;
   return -1;
   }


unsigned char *ServoLed(unsigned id, unsigned led)
   {
   ERREUR.W=0;
   SetID(id);
   led=(led!=0);
   return WriteByte(led,0x19);
   }

int GetServoLed(unsigned id)
   {
   unsigned char *b;
   ERREUR.W=0;
   SetID(id);
   b=ReadByte(0x19);
   if(b) return *b;
   return -1;
   }


//Modifie l'identifiant du servo
unsigned char *ServoId(unsigned id, unsigned new_id)
   {
   ERREUR.W=0;
   SetID(id);
   new_id &= 0xFF;
   if(new_id>253) new_id=253;
   return WriteByte(new_id,0x03);
   }

//Donne les angles mini et maxi
unsigned char *SetServoAngleLimits(unsigned id, unsigned cw, unsigned ccw)
   {
   char r;
   ERREUR.W=0;
   SetID(id);
   r=Comm->REG_MODE;
   if(cw>1023) cw=1023;
   if(!WriteWord(cw,0x06)) return 0;
   if(ccw>1023) ccw=1023;
   Comm->REG_MODE=r;
   return WriteWord(ccw,0x08);
   }   

unsigned char *SetServoSlope(unsigned id, unsigned char cw, unsigned char ccw)
   {
   char r;
   ERREUR.W=0;
   SetID(id);
   r=Comm->REG_MODE;
   if(cw<1) cw=1; else if(cw>254) cw=254;
   if(!WriteByte(cw,0x1C)) return 0;
   if(ccw<1) ccw=1; else if(ccw>254) ccw=254;
   Comm->REG_MODE=r;
   return WriteByte(ccw,0x1D);
   }

unsigned char *SetServoMargin(unsigned id, unsigned char cw, unsigned char ccw)
   {
   char r;
   ERREUR.W=0;
   SetID(id);
   r=Comm->REG_MODE;
   if(cw>254) cw=254;
   if(!WriteByte(cw,0x1A)) return 0;
   if(ccw>254) ccw=254;
   Comm->REG_MODE=r;
   return WriteByte(ccw,0x1B);
   }

unsigned char *SetServoPunch(unsigned id, unsigned x)
   {
   ERREUR.W=0;
   SetID(id);
   if(x>1023) x=1023;
   return WriteWord(x,0x30);
   }

//Renvoie la position courante du servo
uint16_t GetServoPosition(unsigned id)
   {
	uint16_t Pos;
   ERREUR.W=0;
	   	SetID(id);
	   	Pos = (*ReadWord(0x24));
   		return Pos;
   }   

//Passe en mode rotation infinie � la vitesse indiqu�e
unsigned char *SetEndlessTurn(unsigned id, unsigned speed)
   {
   char r;
   ERREUR.W=0;
   SetID(id);
   r=Comm->REG_MODE;
   if(!SetServoAngleLimits(id,0,0)) return 0;
   Comm->REG_MODE=r;
   return SetServoSpeed(id,speed);   
   }
   
// Envoie la commande qui suit en mode REG_WRITE (action exécutée par ServoAction(id))
void ServoRegMode(char mode)
   {
   Comm->REG_MODE=(mode!=0);
   }   

// Recherche des servos connect�s, en débutant du N° first jusqu'au numéro last.
// Renvoie le N° du premier servo trouvé (à partir de first)
// Renvoie last+1 si pas trouvé
int GetNextServo(unsigned first, unsigned last)
   {
   Comm->REG_MODE=0;
   do
      {
      ERREUR.W=0;
      if(ServoPing(first)==0) break;
      first++;
      }
      while(first<=last);
   return first&0xFF;  
   }

int GetServoMoving(unsigned id)
{
   unsigned char *b;
   ERREUR.W=0;
   SetID(id);
   b=ReadByte(0x2E);
   if(b)return *b;
   return -1;
}   

unsigned char *Temperature( unsigned id )
{
   ERREUR.W=0;
SetID(id);
return ReadData(43,1);
}



//fonctions pour commandes servos doubles

unsigned char *SetServoSpeedDouble(unsigned id1, unsigned id2, unsigned sp)
   {//Commande d'un servo doubl� 
   ERREUR.W=0;
   if(sp>1023) sp=1023;
   SetID(id1);
   WriteWord(sp,0x20);
   SetID(id2);
   return WriteWord(sp,0x20);
   } 
unsigned char *SetServoPunchDouble(unsigned id1,unsigned id2, unsigned x)
   {
   ERREUR.W=0;
   if(x>1023) x=1023;
   SetID(id1);
   WriteWord(x,0x30);
   SetID(id2);
   return  WriteWord(x,0x30);
   }


unsigned char *SetServoMarginDouble(unsigned id1,unsigned id2, unsigned char cw, unsigned char ccw)
   {
   char r;
   ERREUR.W=0;
   r=Comm->REG_MODE;
   if(cw>254) cw=254;
   if(ccw>254) ccw=254;
   SetID(id1);
   if(!(WriteByte(ccw,0x1B))) return 0;
   SetID(id2);
   if(!(WriteByte(cw,0x1A))) return 0;
   Comm->REG_MODE=r;
   WriteByte(ccw,0x1B);
   SetID(id1);
   return WriteByte (cw,0x1A) ;
   }
 
unsigned char *SetServoSlopeDouble(unsigned id1,unsigned id2, unsigned char cw, unsigned char ccw)
   {
   char r;
   ERREUR.W=0;
   if(cw<1) cw=1; else if(cw>254) cw=254;
   if(ccw<1) ccw=1; else if(ccw>254) ccw=254;
   SetID(id1);
   r=Comm->REG_MODE;
   if(!WriteByte(ccw,0x1D)) return 0;
   Comm->REG_MODE=r;
   SetID(id2);
   r=Comm->REG_MODE;
   if(!WriteByte(cw,0x1C)) return 0;
   Comm->REG_MODE=r;
   SetID(id1);
   WriteByte(cw,0x1C);
   SetID(id2);
   return WriteByte(ccw,0x1D);
   }  

//Donne les angles mini et maxi
unsigned char *SetServoAngleLimitsDouble(unsigned id1, unsigned id2, unsigned cw, unsigned ccw)
   {
   char r;
   ERREUR.W=0;
   if(cw>1023) cw=1023;
   SetID(id1);
   r=Comm->REG_MODE;
   if(!WriteWord(ccw,0X0B)) return 0;
   Comm->REG_MODE=r;
   SetID(id2);
   r=Comm->REG_MODE;
   if(!WriteWord(cw,0x06)) return 0;
   Comm->REG_MODE=r;
   if(ccw>1023) ccw=1023;
   WriteByte(cw,0x06);
   SetID(id1);
   return WriteByte(ccw,0x08);
   }
   
unsigned char *SetServoMaxTorqueDouble(unsigned id1,unsigned id2, unsigned t)
   {
   ERREUR.W=0;
   if(t>1023) t=1023;
   SetID(id1);
   WriteWord(t,0x22);
   SetID(id2);
   return WriteWord(t,0x22);
   }

unsigned char *EnableServoTorqueDouble(unsigned id1, unsigned id2, unsigned en)
   {
   ERREUR.W=0;
   en=(en!=0);
   SetID(id1);
   WriteByte(en,0x18);
   SetID(id2);
   return WriteByte(en,0x18);
   }

//Positionne le servo � la position angulaire indiqu�e (en �/10, de 0 � 3000)
unsigned char *SetServoAngleDegDouble(unsigned id1,unsigned id2, unsigned angle)
   {
   char r1,r2;
   ERREUR.W=0;

   if(angle>3000) angle=3000;
   angle = (1023.f/3000.f)*angle;
   SetID(id2);
   r2=Comm->REG_MODE;
   ServoRegMode(1);WriteWord(1023-angle,0x1E);
   SetID(id1);
   r1=Comm->REG_MODE;
   ServoRegMode(1);WriteWord(angle,0x1E);
   if(!r1)
   	   {
	   ServoAction();
	   }
   if(!r2)
         {
         SetID(id2);
         ServoAction();
         }
   if((!r1)||(!r2)) return 0;
   SetID(id1);
   Comm->REG_MODE=r1;
   SetID(id2);
   Comm->REG_MODE=r2;
   return 0;
   }

//Positionne le servo � la position angulaire indiqu�e (0 � 1023 pour 0� � 300�)
unsigned char *SetServoAngleDouble(unsigned id1,unsigned id2, unsigned angle)
   {
   char r1,r2;
   ERREUR.W=0;
   if(angle>1023) angle=1023;
   SetID(id2);
   r2=Comm->REG_MODE;
   ServoRegMode(1);WriteWord(1023-angle,0x1E);
   SetID(id1);
   r1=Comm->REG_MODE;
   ServoRegMode(1);WriteWord(angle,0x1E);
   if(!r1)
         {
         ServoAction();
         }
   if(!r2)
         {
         SetID(id2);
         ServoAction();
         }
   if((!r1)||(!r2)) return 0;
   SetID(id1);
   Comm->REG_MODE=r1;
   SetID(id2);
   Comm->REG_MODE=r2;
   return 0;
   }
