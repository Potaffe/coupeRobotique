
#ifndef __Odometrie_H_
#define __Odometrie_H_
//#include "spidma.h"
#include "../../../../Common/Src/SharedMemory.h"

void InitOdometrie(int16_t x, int16_t y, int16_t th);
void Odometrie(void);
#define GetP_X() P_X
#define GetP_Y() P_Y
#define GetTHETA() THETA

void StartTps(void); //D�marre une mesure de temps
uint32_t GetTps(void); //Renvoie le nombre de cycles correspondant � la mesure (@80MHz)
uint32_t TpsToMs(uint32_t n);  //Donne le nombre de ms correspondant au nombre de cycles transmis
uint32_t TpsToUs(uint32_t n);   //Donne le nombre de �s correspondant au nombre de cycles transmis
uint32_t TpsToNs(uint32_t n);   //Donne le nombre de ns correspondant au nombre de cycles transmis

extern uint32_t MaxTps;

#endif

