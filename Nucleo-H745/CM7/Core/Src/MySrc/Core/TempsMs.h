#ifndef __TempsMs_H__
#define __TempsMs_H__

#include <stdint.h>

#define USE_RT 1

#if USE_RT
#include "FreeRTOS.h"
#include "task.h"
#endif

extern volatile int16_t RBT_TIME; //Décomptage automatique toutes les secondes
extern volatile uint16_t T_DETECT;  //TimeOut sur _DETECT
/********************************/

/********************************/

void InitTempsMs(void);
void GestTMs(void);


extern void (*MsFct1)(void);
extern void (*MsFct2)(void);


#if USE_RT
#define DelaiMs(ms) vTaskDelay((ms)/portTICK_PERIOD_MS)
TickType_t CreateDelaiMs(uint32_t ms);
int FinDelaiMs(TickType_t n);
void CloseDelaiMs(int n);
int ResteDelaiMs(TickType_t n);
#else
void DelaiMs(uint16_t ms);

//CreateDelaiMs :
//D�bute une temporisation en ms sans attendre la fin.
//Renvoie un entier � utiliser lors de l'appel de FinDelaiMs()
//Terminer la session obligatoirement avec CloseDelaiMs
int CreateDelaiMs(uint16_t ms);

//FinDelaiMs :
//Renvoie 1 si la temporisation 'n' est termin�e ou est incorrecte
//Renvoie 0 si la temporisation est en cours
//Renvoie -1 en cas d'erreur
int FinDelaiMs(int n);


//CloseDelaiMs :
// Arr�te la temporisation 'n' et termine la session ouverte avec CloseDelaiMs
void CloseDelaiMs(int n);

//Renvoie le temps restant en ms de la temporisation 'n'
//Renvoie 0 si la temporisation est finie
//Renvoie -1 en cas d'erreur
int ResteDelaiMs(int n);
#endif


#endif

