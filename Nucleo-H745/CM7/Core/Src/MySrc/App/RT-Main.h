/*
 * RT-Main.h
 *
 *  Created on: 19 ao�t 2017
 *      Author: Md
 */

#ifndef RT_MAIN_H_
#define RT_MAIN_H_

void RT_Main(void *pvParameters);
void BasicMenu(void);

extern volatile unsigned CT_ADC,ERR_ADC;
extern TaskHandle_t hMain;

#endif /* RT_MAIN_H_ */

#define USE_TIRETTE 1
#define USE_LIDAR 0
