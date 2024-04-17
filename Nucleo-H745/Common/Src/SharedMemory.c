
#include <math.h>

#include "SharedMemory.h"
volatile T_FromRobotToLidar * const FromRobotToLidar = (volatile T_FromRobotToLidar *)0x10040000; //SRAM3 (@0x30040000 CM7)
volatile T_FromLidarToRobot * const FromLidarToRobot = (volatile T_FromLidarToRobot *)0x10040400;



#define USE_UNION 0
typedef union
{
	T_Pos w;
	struct
	{
		int x:NB_BITS_X;
		unsigned y:NB_BITS_Y;
		unsigned life:NB_BITS_LIFE;
	}f;
}T_UNION_POS;

#if USE_UNION
uint16_t getBAL_LIFE(T_Pos b)
   {
	T_UNION_POS u;
	u.w=b;
	return u.f.life;
   }

int16_t getBAL_X(T_Pos b)
{
	T_UNION_POS u;
	u.w=b;
	return u.f.x;
}

int16_t getBAL_Y(T_Pos b)
{
	T_UNION_POS u;
	u.w=b;
	return u.f.y;
}

void setBAL_LIFE(T_Pos *b, uint16_t life)
{
	((T_UNION_POS*)(b))->f.life=life;
}

void setBAL_X(T_Pos *b, int16_t x)
{
	((T_UNION_POS*)(b))->f.x=x;
}

void setBAL_Y(T_Pos *b, int16_t y)
{
	((T_UNION_POS*)(b))->f.y=y;
}

#else

#define MASK_NB_BITS(nb) ((1uL<<(nb))-1)

#define _getBAL_LIFE(b) ((uint16_t)((b)&MASK_NB_BITS(NB_BITS_LIFE)))
#define _getBAL_X(b) ((int16_t)(((int32_t)(b))>>(32-NB_BITS_X)))
#define _getBAL_Y(b) ((int16_t)(((b)>>9)&MASK_NB_BITS(NB_BITS_Y)))

#define _setBAL_LIFE(b,life) ((*(b))=((*(b))&(~MASK_NB_BITS(NB_BITS_LIFE)))|((life)&MASK_NB_BITS(NB_BITS_LIFE)))
#define _setBAL_Y(b,y) ((*(b))=(*(b)&(~(MASK_NB_BITS(NB_BITS_Y)<<NB_BITS_LIFE)))|((((int32_t)(y))&MASK_NB_BITS(NB_BITS_Y))<<NB_BITS_LIFE))
#define _setBAL_X(b,x) ((*(b))=(*(b)&(~(MASK_NB_BITS(NB_BITS_X)<<(32-NB_BITS_X))))|((((int32_t)(x))&MASK_NB_BITS(NB_BITS_X))<<(32-NB_BITS_X)))

uint16_t getBAL_LIFE(T_Pos b)
   {
	return _getBAL_LIFE(b);
   }

int16_t getBAL_X(T_Pos b)
{
	return _getBAL_X(b);
}

int16_t getBAL_Y(T_Pos b)
{
	return _getBAL_Y(b);
}

void setBAL_LIFE(T_Pos *b, uint16_t life)
{
	_setBAL_LIFE(b,life);
}

void setBAL_X(T_Pos *b, int16_t x)
{
	_setBAL_X(b,x);
}

void setBAL_Y(T_Pos *b, int16_t y)
{
	_setBAL_Y(b,y);
}

#endif




float DistBal(T_Pos b)
{
	return hypotf(P_X-getBAL_X(b),P_Y-getBAL_Y(b));
}


void InitSharedData(void)
   {
   unsigned i;
   START_LIDAR=0;
   LIDAR_READY=0;
   BAL_EMERGENCY=0;
   EC_MAX_ECH=25.f;  //Distance max entre 2 échantillons de la même cible
   SZ_BAL_MIN=30.f; //Taille min de la cible à trouver
   SZ_BAL_MAX=200.f; //Taille max de la cible à trouver
   EC_MAX_BAL=150.f; //Ecart min entre 2 balises pour pouvoir les distinguer (sinon : confondues)
   DT_BAL_RAF=100;  //Période de rafraichissement des balises en 10 ms
   LARGEUR_ROBOT=550;

   THETA=900;
   P_X=0;
   P_Y=200;

   BAL_DETECT_AV=0;
   BAL_DETECT_AR=0;

   BAL_DIST_AV=500;
   BAL_DIST_AR=500;

   setBAL_LIFE((T_Pos*)(&AMI),0);
   for(i=0; i<sizeof(BAL)/sizeof(BAL[0]); i++) setBAL_LIFE((T_Pos*)(BAL+i),0);
   }

#ifdef CORE_CM7

#include "stm32h7xx_hal.h"
#include "cmsis_os.h"

//A cause de la cache Data du CPU !
void RefreshLidarData(void)
   {
   SCB_InvalidateDCache_by_Addr((uint32_t *)FromLidarToRobot,sizeof(T_FromLidarToRobot));
   }

uint32_t Get_CT_CM4(void)
   {
   uint32_t x;
   //SCB_DisableDCache();
   //SCB_InvalidateDCache_by_Addr((uint32_t *)SharedData,sizeof(T_shared_data));
   x=CT_CM4;
   //SCB_EnableDCache();
   return x;
   }

uint32_t Get_CT2_CM4(void)
   {
   uint32_t x;
   //SCB_DisableDCache();
   //SCB_InvalidateDCache_by_Addr((uint32_t *)SharedData,sizeof(T_shared_data));
   x=CT2_CM4;
   //SCB_EnableDCache();
   return x;
   }
#endif
