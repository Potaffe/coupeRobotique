/*
 * Lidar.c
 *
 *  Created on: 21 nov. 2017
 *      Author: Md
 *      Lidar géré par huartRP (huart7)
 *      Tx : PA15 -> Rx Lidar (vert)
 *      Rx : PE7 -> Tx Lidar (jaune)
 *      PWM : PB15 (TIM12_CH2) -> PWM Lidar (bleu)
 *
 */


/*
#define HISTO 0
#define GRAPHIC 1
#define ROBOT 1  //Mode fonctionnement normal pour robot
#define ROBAFF 1 //Affichage graphique mode robot
#define ROBDBG 0 //Affichage infos debug
#define STOP_EN 1 //Autorise arrêt an appuyant sur BP (bleu)
*/

#define HISTO 0
#define GRAPHIC 0
#define ROBOT 1  //Mode fonctionnement normal pour robot
#define ROBAFF 0 //Affichage graphique mode robot
#define ROBDBG 1 //Affichage infos debug
#define STOP_EN 0 //Autorise arrêt an appuyant sur BP (bleu)


#include <stdint.h>
#include <stdlib.h>
#include "usart.h"
#include "tim.h"


#include "Lidar.h"
#include "../../../Common/Src/LCD_VT100.h"
#include "TempsMs.h"
#include "Config.h"
#include "../../../Common/Src/SharedMemory.h"


#define EXPRESS 1

#define DMAX 3000 //Distance de détection maximale


TaskHandle_t hRT_LidarAcq, hRT_LidarTrt;

#define CMDE_STOP 0x25 //No response Exit the current state and enter the idle state
#define CMDE_RESET 0x40 //Reset(reboot) the RPLIDAR core
#define CMDE_SCAN 0x20 //Multiple response Enter the scanning state
#define CMDE_EXPRESS_SCAN 0x82 //Multiple response Enter the scanning state and working at the highest speed
#define CMDE_FORCE_SCAN 0x21 //Multiple response Enter the scanning state and force data output without checking rotation speed
#define CMDE_GET_INFO 0x50 //Single response Send out the device info (e.g. serial number)
#define CMDE_GET_HEALTH 0x52 //Single response Send out the device health info
#define CMDE_GET_SAMPLERATE 0x59 //Send out single sampling time

//add for A2 to set RPLIDAR motor pwm when using accessory board
#define CMDE_SET_MOTOR_PWM 0xF0
#define CMD_GET_ACC_BOARD_FLAG 0xFF

//Send Mode :
#define Single_Response_Mode   0x0 //Single Request � RPLIDAR will send only one data response packet in the current session.
#define Multiple_Response_Mode 0x1 //Single Request � RPLIDAR will continuously send out data response packets with the same format in the current session.

typedef union
{
   uint8_t tr[7];
   struct
   {
      uint8_t StF1; //0xA5
      uint8_t StF2; //0x5A
      unsigned sz:30; //Data Response Length
      unsigned m:2; //Send Mode
      uint8_t dt; //Data Type
   }__attribute__((packed));
}__attribute__((packed)) T_Rep;

#define START_FLAG 0xA5

static volatile unsigned LidarReceiveComplete,ModeRx1,SetModeRx1;

static volatile unsigned LidarTransmitComplete;
void HuartRP_TxCpltCallback(void)
   {
      LidarTransmitComplete=1;
   }

static volatile uint16_t NBERR_TX, ERR_TX;
void HuartRP_ErrorCallback(void)
   {
   NBERR_TX++;
   ERR_TX=huartRP.ErrorCode;
   }

static void LidarSendx(const uint8_t *tr, uint16_t nb)
   {
   LidarTransmitComplete=0;
   HAL_UART_Transmit_IT(&huartRP,(uint8_t *)tr,nb);
   while(!LidarTransmitComplete);
   }

static void LidarGetx(void *tr, uint16_t nb)
   {
   LidarReceiveComplete=0;
   HAL_UART_Receive_IT(&huartRP,tr,nb);
   while(!LidarReceiveComplete);
   }

void LidarStop(void)
   {
   static const uint8_t tr[]={START_FLAG,CMDE_STOP};
   LidarSendx(tr,sizeof(tr));
   DelaiMs(2);
   }

void LidarReset(void)
   {
   static const uint8_t tr[]={START_FLAG,CMDE_RESET};
   LidarSendx(tr,sizeof(tr));
   DelaiMs(100);
   }

static T_Rep REP;

uint8_t *LidarGetHealth(void)
   {
   static const uint8_t tr[]={START_FLAG,CMDE_GET_HEALTH};
   static uint8_t h[3];
   LidarSendx(tr,sizeof(tr));
   LidarGetx(REP.tr,sizeof(REP.tr));
   LidarGetx(h,sizeof(h));
   return h;
   }

unsigned LidarGetSampleRate(void)
   {
   static const uint8_t tr[]={START_FLAG,CMDE_GET_SAMPLERATE};
   uint16_t sr[2];
   LidarSendx(tr,sizeof(tr));
   LidarGetx(REP.tr,sizeof(REP.tr));
   LidarGetx(sr,sizeof(sr));
#if EXPRESS
   return sr[1];
#else
   return sr[0];
#endif
   }


uint8_t *LidarGetInfo(void)
   {
   static uint8_t info[20];
   static const uint8_t tr[]={START_FLAG,CMDE_GET_INFO};
   LidarSendx(tr,sizeof(tr));
   LidarGetx(REP.tr,sizeof(REP.tr));
   LidarGetx(info,sizeof(info));
   return info;
   }

void SetMotorSpeed(uint16_t v)
   {
   uint8_t tr[6]={START_FLAG,CMDE_SET_MOTOR_PWM,2,0,0,0};
   uint16_t i;
   tr[3]=v;
   tr[4]=v>>8;
   tr[5]=0;
   for(i=0; i<5; i++) tr[5]+=tr[i];
   LidarSendx(tr,sizeof(tr));
   }

void StartScan(void)
   {
#if EXPRESS
   static const uint8_t tr[]={START_FLAG,CMDE_EXPRESS_SCAN,5,0,0,0,0,0,0x22};
#else
   static const uint8_t tr[]={START_FLAG,CMDE_SCAN};
#endif
   LidarSendx(tr,sizeof(tr));
   LidarGetx(REP.tr,sizeof(REP.tr));
   }

void AffRep(void)
   {
   uint16_t i;
   lcdprintf("Rep : ");
   for(i=0; i<sizeof(REP.tr); i++) lcdprintf("%02X ",REP.tr[i]);
   lcdprintf("\n");
   }

volatile unsigned NBRCV=0,STLDR=0,NBTR=0,AKmin=65000,AKmax=0,NB_ESCN=0,NBERR=0,TOP=0,CT=0,NBERR1=0,NBERR2=0;


#define _u8 uint8_t
#define _u16 uint16_t


typedef struct {
    _u8    sync_quality;      // syncbit:1;syncbit_inverse:1;quality:6;
    _u16   angle_q6_checkbit; // check_bit:1;angle_q6:15;
    _u16   distance_q2;
} __attribute__((packed)) TNode;



//[distance_sync flags]
#define RPLIDAR_RESP_MEASUREMENT_EXP_ANGLE_MASK           (0x3)
#define RPLIDAR_RESP_MEASUREMENT_EXP_DISTANCE_MASK        (0xFC)

typedef struct _rplidar_response_cabin_nodes_t {
    _u16   distance_angle_1; // see [distance_sync flags]
    _u16   distance_angle_2; // see [distance_sync flags]
    _u8    offset_angles_q3;
} __attribute__((packed)) rplidar_response_cabin_nodes_t;


#define RPLIDAR_RESP_MEASUREMENT_EXP_SYNC_1               0xA
#define RPLIDAR_RESP_MEASUREMENT_EXP_SYNC_2               0x5

#define RPLIDAR_RESP_MEASUREMENT_EXP_SYNCBIT              (0x1<<15)

typedef struct {
    _u8                             s_checksum_1; // see [s_checksum_1]
    _u8                             s_checksum_2; // see [s_checksum_1]
    _u16                            start_angle_sync_q6;
    rplidar_response_cabin_nodes_t  cabins[16];
} __attribute__((packed)) CapsNode_T;





#define DIM(x) (sizeof(x)/sizeof(*(x)))

#if EXPRESS
static CapsNode_T CNODE1,CNODE2;
static CapsNode_T *PCNode=&CNODE1;
#else
static TNode CNODE1,CNODE2;
static TNode *PCNode=&CNODE1;
#endif
//static TNode DIST[DIM(nodes)];


#include <string.h>

static struct T_FIFO
   {
   TNode n[400];
   TNode * volatile prd,* volatile pwr;
   }FIFONode = {.prd=FIFONode.n,.pwr=FIFONode.n};

static int PushNode(const TNode *n)
   {
   TNode *p;
   p=FIFONode.pwr+1; if(p==FIFONode.n+DIM(FIFONode.n)) p=FIFONode.n;
   if(p==FIFONode.prd) return 0;
   *(FIFONode.pwr)=*n;
   FIFONode.pwr=p;
   return 1;
   }

static int PopNode(TNode *n)
   {
   TNode *p;
   if(FIFONode.pwr==FIFONode.prd) return 0;
   *n=*(FIFONode.prd);
   p=FIFONode.prd+1; if(p==FIFONode.n+DIM(FIFONode.n)) p=FIFONode.n;
   FIFONode.prd=p;
   return 1;
   }

static volatile unsigned ErrFull=0, ErrFull2=0;


void CapsuleToNormalPush(const CapsNode_T *capsule);
static volatile uint32_t DT_TOUR,DT_TOURM,NB_TOUR;
static volatile uint32_t DTTR[50],NBDTTR;
void HuartRP_RxCpltCallback(void)
   {
      if(ModeRx1)
         {
#if EXPRESS
         CapsuleToNormalPush(PCNode);
#else
         if(!PushNode(PCNode)) ErrFull++;
         static uint32_t tp;
         if(PCNode->sync_quality&1)
            {
            uint32_t t;
            t=htimMes.Instance->CNT;
            DT_TOUR=t-tp;
            if(tp)
               {
               NB_TOUR++;
               DT_TOURM+=DT_TOUR;
               if(NBDTTR<DIM(DTTR)) DTTR[NBDTTR++]=DT_TOUR;
               }
            tp=t;
            }
#endif
         if(PCNode==&CNODE1) PCNode=&CNODE2; else PCNode=&CNODE1;
         HAL_UART_Receive_IT(&huartRP,(uint8_t*)PCNode,sizeof(*PCNode));
         }
      else
         {
         if(SetModeRx1)
            {
            ModeRx1=1;
            HAL_UART_Receive_IT(&huartRP,(uint8_t*)PCNode,sizeof(*PCNode));
            }
         }
      LidarReceiveComplete=1;
   }



//Vitesse de rotation du Lidar (vit=0...1000)
static void Vitesse(uint16_t vit)
   {
   if(vit>1000) vit=1000;
   //htimPWM.Instance->CCR1=(5000*0.001f)*vit;
   htimPWM.Instance->PWM_CCRx=(PWMmax*0.001f)*vit;
   }

//Accélère Lidar jusqu'à la vitesse indiquée. Durée d'accélération : dur en ms
static void AccLidar2(uint16_t vit, float dur)
   {
   TickType_t xLastWakeTime;
   const TickType_t xFrequency = 10/portTICK_PERIOD_MS;
   unsigned nb;
   float v,dv;
   nb=dur/100.f;
   dv=(float)vit/nb;
   xLastWakeTime = xTaskGetTickCount();
   for(v=0.f; v<vit; v+=dv)
      {
      Vitesse(v);
      vTaskDelayUntil( &xLastWakeTime, xFrequency );
      }
   Vitesse(vit);
   }


#define RESULT_OPERATION_FAIL 0
#define RESULT_OK 1

#define RPLIDAR_STATUS_OK                 0x0
#define RPLIDAR_STATUS_WARNING            0x1
#define RPLIDAR_STATUS_ERROR              0x2

#define RPLIDAR_RESP_MEASUREMENT_SYNCBIT        1
#define RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT  2
#define RPLIDAR_RESP_MEASUREMENT_CHECKBIT       1
#define RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT    1

static CapsNode_T _cached_previous_capsuledata;
static uint16_t _is_previous_capsuledataRdy=0;
void CapsuleToNormalPush(const CapsNode_T *capsule)
{
     if (_is_previous_capsuledataRdy) {
         int diffAngle_q8;
         int currentStartAngle_q8 = ((capsule->start_angle_sync_q6 & 0x7FFF)<< 2);
         int prevStartAngle_q8 = ((_cached_previous_capsuledata.start_angle_sync_q6 & 0x7FFF) << 2);

         diffAngle_q8 = (currentStartAngle_q8) - (prevStartAngle_q8);
         if (prevStartAngle_q8 >  currentStartAngle_q8) {
             diffAngle_q8 += (360<<8);
         }

         int angleInc_q16 = (diffAngle_q8 << 3);
         int currentAngle_raw_q16 = (prevStartAngle_q8 << 8);
         for (uint16_t pos = 0; pos < DIM(_cached_previous_capsuledata.cabins); ++pos)
         {
             int dist_q2[2];
             int angle_q6[2];
             int syncBit[2];

             dist_q2[0] = (_cached_previous_capsuledata.cabins[pos].distance_angle_1 & 0xFFFC);
             dist_q2[1] = (_cached_previous_capsuledata.cabins[pos].distance_angle_2 & 0xFFFC);

             int angle_offset1_q3 = ( (_cached_previous_capsuledata.cabins[pos].offset_angles_q3 & 0xF) | ((_cached_previous_capsuledata.cabins[pos].distance_angle_1 & 0x3)<<4));
             int angle_offset2_q3 = ( (_cached_previous_capsuledata.cabins[pos].offset_angles_q3 >> 4) | ((_cached_previous_capsuledata.cabins[pos].distance_angle_2 & 0x3)<<4));

             angle_q6[0] = ((currentAngle_raw_q16 - (angle_offset1_q3<<13))>>10);
             syncBit[0] =  (( (currentAngle_raw_q16 + angleInc_q16) % (360<<16)) < angleInc_q16 )?1:0;
             currentAngle_raw_q16 += angleInc_q16;


             angle_q6[1] = ((currentAngle_raw_q16 - (angle_offset2_q3<<13))>>10);
             syncBit[1] =  (( (currentAngle_raw_q16 + angleInc_q16) % (360<<16)) < angleInc_q16 )?1:0;
             currentAngle_raw_q16 += angleInc_q16;

             for (int cpos = 0; cpos < 2; ++cpos) {

                 if (angle_q6[cpos] < 0) angle_q6[cpos] += (360<<6);
                 if (angle_q6[cpos] >= (360<<6)) angle_q6[cpos] -= (360<<6);

                 TNode node;

                 node.sync_quality = (syncBit[cpos] | ((!syncBit[cpos]) << 1));
                 if (dist_q2[cpos]) node.sync_quality |= (0x2F << RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT);

                 node.angle_q6_checkbit = (1 | (angle_q6[cpos]<<1));
                 node.distance_q2 = dist_q2[cpos];

                 if(!PushNode(&node)) ErrFull++;
                 static uint32_t tp;
                 if(node.sync_quality&1)
                    {
                    uint32_t t;
                    t=htimMes.Instance->CNT;
                    DT_TOUR=t-tp;
                    if(tp)
                       {
                       NB_TOUR++;
                       DT_TOURM+=DT_TOUR;
                       if(NBDTTR<DIM(DTTR)) DTTR[NBDTTR++]=DT_TOUR;
                       }
                    tp=t;
                    }

              }

         }
     }


    _cached_previous_capsuledata = *capsule;
    _is_previous_capsuledataRdy = 1;
}


#define getAngle(node) (((node).angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT) / 64.f)

#define setAngle(node,v) {\
          (node).angle_q6_checkbit = (((_u16)((v) * 64.0f)) << RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT);\
          if((node).angle_q6_checkbit & RPLIDAR_RESP_MEASUREMENT_CHECKBIT) (node).angle_q6_checkbit|=1;\
       }

#define getDistanceQ2(node) (node).distance_q2

/*
static int angleLessThan(const void *a, const void *b)
{
    return getAngle(*(TNode*)a) < getAngle(*(TNode*)b);
}
*/


static uint16_t ascendScanData_(TNode * nodebuffer, uint16_t count)
{
    float inc_origin_angle = 360.f/count;
    uint16_t i = 0;

    //Tune head
    for (i = 0; i < count; i++) {
        if(getDistanceQ2(nodebuffer[i]) == 0) {
            continue;
        } else {
            while(i != 0) {
                i--;
                float expect_angle = getAngle(nodebuffer[i+1]) - inc_origin_angle;
                if (expect_angle < 0.0f) expect_angle = 0.0f;
                setAngle(nodebuffer[i], expect_angle);
            }
            break;
        }
    }

    // all the data is invalid
    if (i == count) return RESULT_OPERATION_FAIL;

    //Tune tail
    for (i = count - 1; i >= 0; i--) {
        if(getDistanceQ2(nodebuffer[i]) == 0) {
            continue;
        } else {
            while(i != (count - 1)) {
                i++;
                float expect_angle = getAngle(nodebuffer[i-1]) + inc_origin_angle;
                if (expect_angle > 360.0f) expect_angle -= 360.0f;
                setAngle(nodebuffer[i], expect_angle);
            }
            break;
        }
    }

    //Fill invalid angle in the scan
    float frontAngle = getAngle(nodebuffer[0]);
    for (i = 1; i < count; i++) {
        if(getDistanceQ2(nodebuffer[i]) == 0) {
            float expect_angle =  frontAngle + i * inc_origin_angle;
            if (expect_angle > 360.0f) expect_angle -= 360.0f;
            setAngle(nodebuffer[i], expect_angle);
        }
    }

    // Reorder the scan according to the angle value
    //qsort(nodebuffer,count,sizeof(*nodebuffer),angleLessThan);

    return RESULT_OK;
}


#define BRUT 0

typedef struct
   {
   int a;
   unsigned mm;
   uint16_t q;
   } T_DIST;

   static struct
      {
      T_DIST x[2880];//[1440];
      T_DIST * volatile prd,* volatile pwr;
      }FIFODist = {.prd=FIFODist.x,.pwr=FIFODist.x};

   static int PushDist(const T_DIST *x)
      {
      T_DIST *p;
      p=FIFODist.pwr+1; if(p==FIFODist.x+DIM(FIFODist.x)) p=FIFODist.x;
      if(p==FIFODist.prd) return 0;
      *(FIFODist.pwr)=*x;
      FIFODist.pwr=p;
      return 1;
      }

   static int PopDist(T_DIST *x)
      {
      T_DIST *p;
      if(FIFODist.pwr==FIFODist.prd) return 0;
      *x=*(FIFODist.prd);
      p=FIFODist.prd+1; if(p==FIFODist.x+DIM(FIFODist.x)) p=FIFODist.x;
      FIFODist.prd=p;
      return 1;
      }

   void GetDist(float *a, unsigned *mm)
   {
      T_DIST x;
      while(1)
      {
         if(!PopDist(&x)) continue;
         if(!x.mm) continue;
         if(x.mm > DMAX) continue;
         *a=x.a*2.727076956E-4f; // *PI/180/64
         *mm=x.mm;
         return;
      }
   }

//volatile int DistReady=0;
void RT_LidarAcq(void *pvParameters)
   {
   uint16_t i;
   static TNode n[720];

   while(!ModeRx1) DelaiMs(1); //Attente début acquisition

   //DistReady=0;
   while(1)
      {
      for(i=0; i<DIM(n); i++)
         {
         while(!PopNode(n+i)) DelaiMs(1);
         }
#if !BRUT
      uint16_t r;
      r=ascendScanData_(n,DIM(n));
      if(r!=RESULT_OK) NBERR++;
#endif
      for(i=0; i<DIM(n); i++)
         {
         T_DIST d;
#if BRUT
         d.a=n[i].angle_q6_checkbit;
         d.mm=n[i].distance_q2;
         d.q=n[i].sync_quality;
#else
         //DIST[i].a=n[i].angle_q6_checkbit;
         d.a=n[i].angle_q6_checkbit >> RPLIDAR_RESP_MEASUREMENT_ANGLE_SHIFT; //en degrésx64
         d.mm=n[i].distance_q2/4;
         d.q=n[i].sync_quality >> RPLIDAR_RESP_MEASUREMENT_QUALITY_SHIFT;
#endif
         if(!PushDist(&d)) ErrFull2++;
         //while(!PushDist(&d));
         }
      //DistReady=1;
      }

   while(1);
   }

#include <math.h>


#if HISTO
void Histo(int brut, uint16_t sp)
   {
   uint16_t NbMem;
   const unsigned dt=10000;
   unsigned i,j;
   static struct
   {
      uint16_t a;
      uint16_t d;
      uint16_t init;
   }MEM[2048];
   TickType_t tf;
   tf=xTaskGetTickCount()+dt/portTICK_PERIOD_MS;
   //DistReady=0;
   NbMem=0;
   do
      {
      float a;
      unsigned d;
      GetDist(&a,&d);
      if(NbMem>=DIM(MEM)) break;
      MEM[NbMem].a=a;
      MEM[NbMem].d=d;
      NbMem++;
      }
      while(xTaskGetTickCount()<=tf);
   Vitesse(0);
   VTCtrl(0);
   if(brut)
      {
      lcdprintf("\n//Qual,Angle,Distance\n");
      lcdprintf("static TNode n[]=\n{\n");
      for(i=0; i<NbMem; i++)
         {
         lcdprintf("  {%u,%u,%u},\n",MEM[i].init,MEM[i].a,MEM[i].d);
         }
      lcdprintf("};\n");
      }
   else
      {
      lcdprintf("\nAngle;Distance\n");
      j=0;
      for(i=0; i<NbMem; i++)
         {
         if(MEM[i].init) {lcdprintf("####\n");}
         lcdprintf("%u %.2f;%u\n",j,MEM[i].a/64.f,MEM[i].d);
         j++;
         }
      }
   lcdprintf("ERR=%u\n",NBERR);
   lcdprintf("ERR1=%u\n",NBERR1);
   lcdprintf("ERR2=%u\n",NBERR2);
   lcdprintf("RCV=%u\n",NBRCV);
   lcdprintf("STLDR=%u\n",STLDR);
   lcdprintf("NBERR_TX=%u (%4X)\n",NBERR_TX,ERR_TX);
   lcdprintf("ErrFull=%u\n",ErrFull);
   lcdprintf("ErrFull2=%u\n",ErrFull2);
   DT_TOURM/=NB_TOUR;
   lcdprintf("Echantillonnage=%uus\n",sp);
   lcdprintf("DT tour=%f (Moy: %f - %u tr)\n",DT_TOUR*0.0001f,DT_TOURM*0.0001f,NB_TOUR);
   if(DT_TOURM) lcdprintf("%f tr/s (%f deg/tr)\n",10000.f/DT_TOURM,(0.01f*360.f)*sp/DT_TOURM);
   for(i=0; i<NBDTTR; i++)
      {
      lcdprintf("   dt[%u]=%f\n",i,DTTR[i]*0.0001f);
      }
   }
#endif

#if GRAPHIC
void Graphic(void)
   {
   const unsigned dt=3000;
   int16_t xm, ym, ct;
   xm=lcdGetPixelWidth()-1;
   ym=lcdGetPixelHeight()-1;
   lcdclrscr();
   lcdMoveTo(0,ym/2); lcdLineTo(xm,ym/2);
   lcdMoveTo(xm/2,0); lcdLineTo(xm/2,ym);
   TickType_t tf;
   tf=xTaskGetTickCount()+dt/portTICK_PERIOD_MS;
   //DistReady=0;
   ct=0;
   while(1)
      {
      float a;
      unsigned d;
      int x,y;
      GetDist(&a,&d);
      ct++;
      if(ct<3) continue;
      ct=0;
      d*=0.04f;
      x=d*sinf(a);
      y=d*cosf(a);
      if(x<-xm) x=-xm;
      else if(x>xm) x=xm;
      if(y<-ym) y=-ym;
      else if(y>ym) y=ym;

         //if(!i) lcdMoveTo(xm/2+x,ym/2+y); else lcdLineTo(xm/2+x,ym/2+y);
      lcdPutPixel(xm/2+x,ym/2-y);
      if(xTaskGetTickCount()>=tf)
         {
         lcdclrscr();
         tf=xTaskGetTickCount()+dt/portTICK_PERIOD_MS;
         lcdMoveTo(0,ym/2); lcdLineTo(xm,ym/2);
         lcdMoveTo(xm/2,0); lcdLineTo(xm/2,ym);
         }
#if STOP_EN
      if(_BP) break;
#endif
      }
   }
#endif

#if ROBOT


typedef struct
{
   int xmin,xmax,ymin,ymax;
   }T_Rect;

typedef struct
   {
      int16_t x,y;
      }T_POINT;

#define DIST_SECUR 100
static T_Rect FIXES_RECT[]=
{
//   {.xmin=1000-DIST_SECUR,.xmax=1500,.ymin=300-DIST_SECUR,.ymax=1200},
//   {.xmin=-1500,.xmax=-1000+DIST_SECUR,.ymin=300-DIST_SECUR,.ymax=1200},
//   {.xmin=-1000-DIST_SECUR,.xmax=1000+DIST_SECUR,.ymin=1540-DIST_SECUR,.ymax=2000},
//   {.xmin=-DIST_SECUR,.xmax=+DIST_SECUR,.ymin=1540-200-DIST_SECUR,.ymax=1500},
};

static struct T_ROND
   {
   int16_t x,y,rayon;
   } FIXES_ROND[]=
{
//   {.x=500,.y=1050,.rayon=150},
//   {.x=-500,.y=1050,.rayon=150},
};

typedef struct
   {
      float a;
      float mm;
      float larg;
   } T_POL;




static int16_t IsInRect(int x, int y,const T_Rect *r)
{
    if (x<r->xmin) return 0;
    if (x>r->xmax) return 0;
    if (y<r->ymin) return 0;
    if (y>r->ymax) return 0;
    return 1;
}

static int IsNear(int x, int y, const struct T_ROND *r)
{
   float d,dxr,dyr,x2;
   dxr=x-r->x; dyr=y-r->y;
   d=r->rayon;
   d*=d;
   x2=dxr*dxr+dyr*dyr;
   return x2<=d;
}


#define MG_TABLE 100     //Distance non-détection % bord de table

#define _2PI 6.283185307f

float THETArad,COSTH,SINTH;

#if ROBAFF
#define AFF_WIDTH 480
#define AFF_HEIGHT 272



#define SZ_PLOT 6
#define SZ_HPLOT ((int)(SZ_PLOT*1.732))
static void Plot(int x, int y)
   {
   x=0.1596666666667f*(x+1500);
   y=271-0.1355f*y;
   lcdCircleXY(x,y,SZ_PLOT);
//   lcdMoveTo(x-SZ_PLOT,y+SZ_PLOT);
//   lcdLineTo(x+SZ_PLOT,y+SZ_PLOT); lcdLineTo(x,y-SZ_HPLOT); lcdLineTo(x-SZ_PLOT,y+SZ_PLOT);
   }

static void PlotMe(void)
   {
   int x,y,dx,dy;
   x=0.1596666666667f*(P_X+1500);
   y=271-0.1355f*P_Y;
   lcdCircleXY(x,y,SZ_PLOT);
//   lcdMoveTo(x-SZ_PLOT,y+SZ_PLOT);
//   lcdLineTo(x+SZ_PLOT,y+SZ_PLOT); lcdLineTo(x+SZ_PLOT,y-SZ_PLOT); lcdLineTo(x-SZ_PLOT,y-SZ_PLOT); lcdLineTo(x-SZ_PLOT,y+SZ_PLOT);
   dx=(SZ_PLOT/2)*COSTH;
   dy=-(SZ_PLOT/2)*SINTH;
   lcdMoveTo(x-dx,y-dy);
   x+=dx;
   y+=dy;
   lcdLineTo(x,y);
   lcdPutPixel(x+1,y+1);lcdPutPixel(x-1,y-1);lcdPutPixel(x+1,y-1);lcdPutPixel(x-1,y+1);
   }

static void TraceTable(void)
   {
//   int16_t xm, ym;
//   xm=AFF_WIDTH-1;
//   ym=AFF_HEIGHT-1;
   lcdclrscr();
   //lcdMoveTo(0,0); lcdLineTo(0,ym); lcdLineTo(xm,ym); lcdLineTo(xm,0); lcdLineTo(0,0);
   PlotMe();
   }


#define TRafr 2000
#else
#define TRafr 1000
#endif

static void AddBalise(const T_POL *c)
   {
   int x, y;
   unsigned i;
   float d,a,ec;
   d=c->mm;
   a=THETArad - c->a;
   x=d*cosf(a)+P_X;
   y=d*sinf(a)+P_Y;
//Eclusion cible hors table
   if((x < (-(SZX_TABLE/2)+MG_TABLE)) || (x > ((SZX_TABLE/2)-MG_TABLE)) || (y < MG_TABLE) || (y  >(SZY_TABLE-MG_TABLE)) ) return;
//Exclusion zones fixes rectangulaires
   for(i=0; i<DIM(FIXES_RECT); i++)
      {
      if(IsInRect(x,y,FIXES_RECT+i)) return;
      }
//Exclusion zones fixes circulaires
   for(i=0; i<DIM(FIXES_ROND); i++)
      {
      if(IsNear(x,y,FIXES_ROND+i)) return;
      }
//Détection balises déjà détectées
   for(i=0; i<DIM(BAL); i++)
      {
      T_Pos b=BAL[i];
      if(!getBAL_LIFE(b)) continue;
      ec=hypotf(x-getBAL_X(b),y-getBAL_Y(b));
      if(ec<EC_MAX_BAL)
         {
         setBAL_X(&b,x);
         setBAL_Y(&b,y);
         setBAL_LIFE(&b,DT_BAL_RAF);
         BAL[i]=b;
         return;
         }
      }
//Enregistrement nouvelle balise (si place disponible)
   for(i=0; i<DIM(BAL); i++)
      {
      T_Pos b=BAL[i];
      if(!getBAL_LIFE(b))
         {
         setBAL_X(&b,x);
         setBAL_Y(&b,y);
         setBAL_LIFE(&b,DT_BAL_RAF);
         BAL[i]=b;
         return;
         }
      }
   }



//Gestion durée de vie des balises
//static volatile unsigned NB_BAL;
static void MajBal(void)
   {
	//BAL_DISTMIN à calculer
   unsigned i,j,n;
   int imax;
   static uint8_t ct=0;
   float d,dmax;
   ct++;
   if(ct==10) //Pour maj life toutes les 10 ms
      {
      ct=0;
      j=0;
      imax=-1;
      dmax=10000.f;
      for(i=0; i<DIM(BAL); i++)
         {
         T_Pos b=BAL[i];
         if((n=getBAL_LIFE(b))!=0)
            {
            setBAL_LIFE(&b,n-1);
            BAL[i]=b;
            j++;
            d=DistBal(b);
            if(imax<0) {dmax=d; imax=i;}
            else {if(d<dmax) {dmax=d; imax=i;}}
            }
         }
      NB_BAL=j;
      BAL_DISTMIN=imax;
      }
//Vérification et activation déclenchement arrêt nécessaire...
   BAL_EMERGENCY=DetectObstacle();
   }

void Robot(void)
   {
   T_POL c;
   unsigned n, di, dt;
   float a, ap, a0, d, av;
   P_X=0;P_Y=1000;THETA=400;
   av=0;
#if ROBAFF || ROBDBG
#if ROBAFF
   TraceTable();
#endif
   TickType_t tf;
   tf=xTaskGetTickCount()+TRafr/portTICK_PERIOD_MS;
#endif
   MsFct1=MajBal;
   GetDist(&a,&di);
   n=0;
   dt=0;
   a0=a;
   while(1)
      {
      THETArad=0.001745329252f*THETA;
      COSTH=cosf(THETArad);
      SINTH=sinf(THETArad);
      ap=a;
      if(ap>=_2PI) ap-=_2PI;
      GetDist(&a,&di);
      if(a<=ap) a+=_2PI;
      d=(a-ap)*di;
      if(d<EC_MAX_ECH)
         {
         if(!n) a0=ap;
         av=a;
         dt+=di;
         n++;
         }
      else
         {
         if(n)
            {
            c.mm=dt/n;
            if(a0>av) av+=_2PI;
            d=(av-a0)*c.mm;
            if(d>=SZ_BAL_MIN)
               {
               c.a=0.5f*(av+a0);
               c.larg=(av-a0)*c.mm;
               if(c.larg<=SZ_BAL_MAX)
                  {
                  AddBalise(&c);
                  }
               }
            }
         n=0;
         dt=0;
         a0=a;
         }
#if ROBAFF || ROBDBG
      if(xTaskGetTickCount()>tf)
         {
#if ROBAFF
         TraceTable();
#endif
#if ROBDBG
         //if(ErrFull2||ErrFull)
            {
            lcdgotoxy(2,2);
            lcdprintf("Err : %u %u\t",ErrFull,ErrFull2);
            lcdgotoxy(2,3);
            lcdprintf("NB=%u\t",NB_BAL);
            }
#endif
#if ROBAFF
         unsigned i;
         for(i=0; i<DIM(BAL); i++)
            {
            T_Pos b=BAL[i];
            if(getBAL_LIFE(b)) Plot(getBAL_X(b),getBAL_Y(b));
            }
#endif

         tf=xTaskGetTickCount()+TRafr/portTICK_PERIOD_MS;
         }
#endif
#if STOP_EN
      if(_BP) break;
#endif
      }
   }
#endif


void Coucou(int n, uint16_t d)
   {
   int i;
   for(i=0; i<n; i++)
      {
      _LEDV(1); DelaiMs(d);
      _LEDV(0); DelaiMs(d);
      }
   }


void RT_LidarTrt(void *pvParameters)
   {
   uint16_t i;
   uint16_t sp;

   Coucou(20,50);

   lcdTaskAllow(xTaskGetCurrentTaskHandle());  //Autorise affichage seulement pour tâche en cours
   lcdinit(VT100);
   VTCtrl(1);
   VTExtended(1);
   lcdputc(3); //Code init
   lcdputc(3); //Code init
   lcdputc(3); //Code init

   lcdclrscr();lcdclrscr();lcdclrscr();

#if GRAPHIC || ROBAFF

      lcdSetFont("Consolas12");
      lcdBackColor565(C565_White);
      lcdTextColor565(C565_Black);
#endif
   lcdclrscr();

   VTExtended(1); lcdSetFont("courierNew10"); lcdBackColor565(C565_White); lcdTextColor565(C565_Black);
   lcdprintf("Bonjour\nLidar sur H7 (CM4)\n");

   //lcdprintf("PCLK1=%uHz\n",HAL_RCC_GetPCLK1Freq());
   //lcdprintf("PCLK2=%uHz\n",HAL_RCC_GetPCLK2Freq());
   //lcdprintf("Sys=%uHz\n",HAL_RCC_GetSysClockFreq ());


   //Vitesse(500);
   while(0)
      {
      lcdgotoxy(1,6);
      lcdprintf("CM4 : %u CM7 : %u\t",CT_CM4,CT_CM7);
      lcdgotoxy(1,7);
      lcdprintf("CM4_2 : %u CM7_2 : %u\t",CT2_CM4,CT2_CM7);
      lcdgotoxy(1,8);
      lcdprintf("P_X=%d P_Y=%d TH=%.1*10D\t",P_X,P_Y,THETA);
      }



#if 0
   lcdSetFont("Delius48");
   lcdDefTouch(50,80,150,"START",C565_GlacialBlueIce,0);
   {
      char *p;
      do
         {
         p=lcdGetK();
         }
      while(*p!='A');
   }
#endif

   lcdprintf("Attente démarrage par CM7...");
   while(!START_LIDAR);


   lcdSetFont("Consolas12");
   lcdclrscr();
   //LidarReset();
    LidarStop();
    STLDR++;

    {
    uint8_t *INFO;
    INFO=LidarGetInfo();
    STLDR++;
    lcdprintf("\nInfo=");
    lcdprintf("Modèle %u\n",INFO[0]);
    lcdprintf("Firmware V%u.%u\n",INFO[2],INFO[1]);
    lcdprintf("Hardware V%u\n",INFO[3]);
    for(i=0; i<16; i++)
       {
       lcdprintf("%2X",INFO[i+4]);
       }
    lcdprintf("\n");
    }

    {
    uint8_t *h;
    h=LidarGetHealth();
    lcdprintf("Health=%u",*h);
    if(*h) lcdprintf("(Code err.=%u)",h[1]+(h[2]<<8));
    lcdprintf("\n");
    }

    STLDR++;

    lcdprintf("Démarrage moteur...");
    //AccLidar2(645,5000);
    AccLidar2(800,5000);
    DelaiMs(4000);
    lcdprintf(" Ok\n");
    STLDR++;

    sp=LidarGetSampleRate();
    lcdprintf("Echantillonnage=%uus\n",sp);


    STLDR++;
    DelaiMs(2);
    LidarStop();
    DelaiMs(20);

    SetModeRx1=1;  //Active le mode réception trames dist après la réponse de la commande
    StartScan();
    STLDR++;

    LIDAR_READY=1;


    LidarReceiveComplete=0;

//   DistReady=0;
#if HISTO
   Histo(1,sp);
#endif
#if GRAPHIC
   Graphic();
#endif
#if ROBOT
   Robot();
#endif
   Vitesse(0);
   while(1);
   }


