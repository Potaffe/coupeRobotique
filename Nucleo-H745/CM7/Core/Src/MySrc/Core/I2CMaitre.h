/*
 * I2CMaitre.h
 *
 *  Created on: 23 nov. 2016
 *      Author: Md
 */

#ifndef I2CMAITRE_H_
#define I2CMAITRE_H_
#include <stdint.h>

uint16_t I2CPing(uint8_t ad);
uint16_t I2CWriteByte(uint8_t ad, uint8_t b);
uint16_t I2CWriteBytes(uint8_t ad, uint8_t *b, uint16_t nb);
int16_t I2CReadByte(uint8_t ad, uint8_t cmde);
int16_t I2CReadBytes(uint8_t ad, uint8_t cmde, uint8_t *data, uint16_t nb);

#endif /* I2CMAITRE_H_ */
