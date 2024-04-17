
#ifndef __CONFIG__
#define __CONFIG__

#include <stdint.h>
#include "tim.h"
#include "usart.h"
#include "main.h"
#include "../../../Common/Src/clk.h"
#include "../../../Common/Src/SharedMemory.h"

//================= définition périphériques ==================
#define huartCom huart6
#define huartRX huart2
#define huartAX huart5
//#define huartAff huart4  //à définir en option -DhuartAff=huartx au compilateur

#define htimPWM htim1  //PWM moteurs
#define htimAss htim8  //Interruptions asservissement
#define htimMs htim13  //Gestion des fonctions TempsMs
#define htimADC htim6  //Pour lecture des entrées analogiques
//Définition timers utilisés pour codeurs
#define htimCodX htim2
#define htimCodG htim3
#define htimCodD htim4

#define hadc hadc3

#define hi2c hi2c2
//===============================================================

#define V_BAT_MIN 15.f //Tension batterie min (sinon : alerte)

#define DEF_SYM_AUTO 1  ////Utilisation du symétriseur automatique ou non


//Couleur de jeu (pour menu)
#define VAL_COUL_POS C565_Blue
#define VAL_COUL_NEG C565_Yellow

//Couleur de jeu utilisée pour la définition des mouvements (pour symétrisation automatique)
#define COULEUR_DEF COUL_POS

#define MIROIR (COULEUR!=COULEUR_DEF)


//Inversion des câblages codeurs
#define INV_COD1 1 //Gauche
#define INV_COD2 0 //Droite
#define INV_CODX 0 //X


#define FPWM 20000  //Fréquence hâchage PWM moteurs

//Inversion des câblages moteurs
#define INV_MOTG 0
#define INV_MOTD 0
#define INV_MOTX 1

//Tension de référence pour la commande des moteurs (en V)
#define VREF 12.5f

//Ne pas modifier-------------
#define POS1CNT (INV_COD1? (-__HAL_TIM_GetCounter(&htimCodG)):__HAL_TIM_GetCounter(&htimCodG)) //Gauche
#define POS2CNT (INV_COD2? (-__HAL_TIM_GetCounter(&htimCodD)):__HAL_TIM_GetCounter(&htimCodD)) //Droite
#define POSXCNT (INV_CODX? (-__HAL_TIM_GetCounter(&htimCodX)):__HAL_TIM_GetCounter(&htimCodX)) //X

#define PWMG ((&htimPWM)->Instance->CCR1)
#define PWMD ((&htimPWM)->Instance->CCR2)
#define PWMX ((&htimPWM)->Instance->CCR3)
#define PWMmin 0
#define PWMmax ((uint16_t)(FCY/FPWM))


#define __Pin(x,st) HAL_GPIO_WritePin(x##_GPIO_Port, x##_Pin, (st)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define __InvPin(x,st) HAL_GPIO_WritePin(x##_GPIO_Port, x##_Pin, (st)?GPIO_PIN_RESET:GPIO_PIN_SET)

#define __StPin(x) HAL_GPIO_ReadPin(x##_GPIO_Port,x##_Pin)
//Fin ------------------------


#define _LEDV(st) __Pin(LEDV,st)
#define _LEDJ(st) __Pin(LEDJ,st)
#define _LEDR(st) __Pin(LEDR,st)




//Attention : "commandes" parfois inversées !
#define _Powen(st) __Pin(POWEN,st)

#define _PowTurbine(st) __Pin(EN_TURB,st)
#define _PowenX(st) __Pin(POWENX,st)
#define _PowPompe(st) __InvPin(PowPompe,st)
#define _PompeA(st) __InvPin(PompeA,st)
#define _PompeB(st) __InvPin(PompeB,st)

#define _ServoRX_RW(st) __Pin(ServoRX_RW,st)
#define _ServoAX_RW(st) __Pin(ServoAX_RW,st)


#define _Tirette __StPin(TIRETTE)
#define _TopX __StPin(TopX)

#define _BP __StPin(BUTTON)

#define _CAPTUS __StPin(CAPTUS)

#define _ContactAR (!__StPin(GateauD))


#define _Canon(st) __Pin(Canon,st)


#define EnableAsserv(en) ((en)? __HAL_TIM_ENABLE_IT(&htimAss, TIM_IT_UPDATE): __HAL_TIM_DISABLE_IT(&htimAss, TIM_IT_UPDATE))


#define I2CAD_COUL1 0x1A
#define I2CAD_COUL2 0x1C

#define I2CAD_SERVO 0x33  //Carte de commande 8 servos

////////////////// Paramètres odométrie//////////////////////////////

#define TOPSCODEUR_ROUE_LIBRE 4096 // Tops/tour 1024*4
#define DIAM_ROUE_LIBRE  50.45f //(50.045f)  // Diminuer pour augmenter la distance parcourue
// écart entre roues pour Commande (augmenter si angle trop faible)
#define INTER_ROUE_LIBREC 272.01f
//#define INTER_ROUE_LIBREC 335.5f // Robot démo
#define KDISSYM 1.0f //Coef. d'ajustement correction odométrie (augmenter si angle odo trop grand)


// Paramètres correcteurs (asservissement)
//Moteurs G & D
#define CORR_KP 2.f  //Gain proportionnel
#define CORR_KD 20.f //Action dérivée
#define CORR_FRS 100  //Frottement sec
#define CORR_SEUILFRS 8 //Seuil frottement sec
//Moteur X
#define CORRX_KP 5.f  //Gain proportionnel
#define CORRX_KD 0.f //Action dérivée
#define CORRX_FRS 10  //Frottement sec
#define CORRX_SEUILFRS 2 //Seuil frottement sec

//Distances séparant le Lidar % bords du robot
#define DETECT_DIST_AV 120
#define DETECT_DIST_AR 110
//Distance séparant la balise du bord du robot adverse (estimation moyenne)
#define DETECT_DIST_ADV 150



extern volatile float TEMPCPU;
extern volatile float VDDA;  //Tension réf interne


#define DIM(x) (sizeof(x)/sizeof(*(x)))

void CalageOdometrieAuto(int zone);


#endif

