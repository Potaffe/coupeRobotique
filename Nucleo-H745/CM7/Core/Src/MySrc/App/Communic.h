/*
 * Communic.h
 *
 *  Created on: 25 mars 2019
 *      Author: Robot
 */

#ifndef COMMUNIC_H_
#define COMMUNIC_H_


#define NB_VIS_ARUCO_MAX 5
typedef struct
{
	uint16_t nb;
	int16_t id[NB_VIS_ARUCO_MAX];
	int16_t x[NB_VIS_ARUCO_MAX],y[NB_VIS_ARUCO_MAX];
}T_VIS_ARUCO;

extern volatile T_VIS_ARUCO VIS_ARUCO;


typedef struct
   {
   int16_t score,x,y;
   }T_Data;

//extern volatile T_Data Gros, Petit, Exp;

volatile const T_Data *GetGros(void);
volatile const T_Data *GetPetit(void);
volatile const T_Data *GetExp(void);


extern TaskHandle_t hCommunic;
void RT_Communic(void *pvParameters);
void HuartCom_RxCpltCallback(void);


//Indique réception trames sur communication série
//Renvoie 1 si trame reçue. Renvoie ensuite 0 si pas d'autre trame reçue
unsigned IsComReceivingData(void);



void AddScore(uint16_t sc);

extern volatile int CodeErreur;

#endif /* COMMUNIC_H_ */
