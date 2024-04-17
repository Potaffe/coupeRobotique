/*
 * spidma.h
 *
 *  Created on: Jun 9, 2016
 *      Author: Md
 */

#ifndef SPIDMA_H_
#define SPIDMA_H_

//#include "../../../../LidarNucleo64/Core/Src/Common/SharedData.h"

extern volatile unsigned NBSendSPI,NBCallBackSPI,NBErrSPI;

void SendSpi(void);
void InitSpiDma(void);

#endif /* SPIDMA_H_ */
