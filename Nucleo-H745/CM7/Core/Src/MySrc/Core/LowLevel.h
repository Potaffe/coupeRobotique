/*
 * LowLevel.h
 *
 *  Created on: May 27, 2016
 *      Author: Md
 */

#ifndef LOWLEVEL_H_
#define LOWLEVEL_H_



unsigned VitMotG(int16_t v);
unsigned VitMotD(int16_t v);
unsigned VitMotX(int16_t v);


void SetVitMotG(int16_t v);
void SetVitMotD(int16_t v);
void SetVitMotX(int16_t v);
void SetVitMotGD(int16_t vg, int16_t vd);

extern volatile float V_BAT;



void VitTurbine(float pourcent);

void Powen(int16_t st);
void PowTurbine(int16_t st);

int GetDetect(void);


void _LED_ROUGE_ON(uint16_t ms);
void _LED_VERTE_ON(uint16_t ms);
void _LED_ORANGE_ON(uint16_t ms);
void _LED_BLEUE_ON(uint16_t ms);

#define RESET_CPU() (SCB->AIRCR=(SCB->AIRCR & 0xFFFF) | 0x05FA0000 | (1<<2)) //Active SYSRESETREQ pour CM4


void LeveDrapeau(uint16_t p);

#endif /* LOWLEVEL_H_ */
