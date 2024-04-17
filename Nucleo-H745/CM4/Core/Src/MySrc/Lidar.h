/*
 * Lidar.h
 *
 *  Created on: 22 nov. 2017
 *      Author: Md
 */

#ifndef LIDAR_H_
#define LIDAR_H_
#include "cmsis_os.h"

extern float THETArad,COSTH,SINTH;
int DetectObstacle(void);


extern TaskHandle_t hRT_LidarAcq, hRT_LidarTrt;

void RT_LidarAcq(void *pvParameters);
void RT_LidarTrt(void *pvParameters);
void LidarStop(void);
void LidarReset(void);
void StartExpressScan(void);

uint8_t *LidarGetInfo(void);

/*
union T_DIST
{
   uint16_t v;
   struct
      {
      unsigned mm:15;
      unsigned ok:1;
      };
};

typedef union
{
   uint8_t tr[84];
   struct
   {
      unsigned chk1:4;
      unsigned sync1:4; //0x0A
      unsigned chk2:4;
      unsigned sync2:4; //0x05
      unsigned st_ang_q6:15; //angle=start_angle_q6/64.0 Deg
      unsigned S:1;
      struct
      {
      unsigned sdt1:2; //dt1[5:4]
      unsigned d1:14; //Distance 1
      unsigned sdt2:2; //dt2[5:4]
      unsigned d2:14; //Distance 2
      unsigned dt1:4; //dt1[3:0]
      unsigned dt2:4; //dt2[3:0]
      }__attribute__((packed)) cabin[16];
   }__attribute__((packed));
}__attribute__((packed)) T_RepExpScan;
*/

#define DIM(x) (sizeof(x)/sizeof(*(x)))

//extern volatile union T_DIST DIST[360*64];

void HuartRP_RxCpltCallback(void);
void HuartRP_TxCpltCallback(void);
void HuartRP_ErrorCallback(void);



#endif /* LIDAR_H_ */
