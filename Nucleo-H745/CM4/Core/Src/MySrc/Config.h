
#ifndef __CONFIG__
#define __CONFIG__

#include <stdint.h>
#include "tim.h"
#include "usart.h"
#include "main.h"
#include "../../../Common/Src/clk.h"



#define huartRP huart7
//#define huartAff huart1  //à définir en option -DhuartAff=xxxx au compilateur

#define htimMs htim14
#define htimPWM htim12
#define htimMes htim5

#define PWM_TIM_CHANNEL_x TIM_CHANNEL_2
#define PWM_CCRx CCR2

#define FPWM 20000

#define PWMmin 0
#define PWMmax ((uint16_t)(FCY/FPWM))

#define __Pin(x,st) HAL_GPIO_WritePin(x##_GPIO_Port, x##_Pin, (st)?GPIO_PIN_SET:GPIO_PIN_RESET)
#define __InvPin(x,st) HAL_GPIO_WritePin(x##_GPIO_Port, x##_Pin, (st)?GPIO_PIN_RESET:GPIO_PIN_SET)

#define __StPin(x) HAL_GPIO_ReadPin(x##_GPIO_Port,x##_Pin)


#define _BP HAL_GPIO_ReadPin(BUTTON_GPIO_Port,BUTTON_Pin)

#define _LEDV(st) __Pin(LEDV,st)



#endif

