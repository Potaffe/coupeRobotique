/*
 * Dijkstra.h
 *
 *  Created on: 19 juin 2019
 *      Author: deloi
 */

#ifndef DIJKSTRA_H_
#define DIJKSTRA_H_

int DijkRoadTo(int16_t x, int16_t y);
int16_t DijkGetNextPoint(int16_t *x, int16_t *y);
uint16_t DijkGetNb(void);
int16_t DijkReadPoint(uint16_t n, int16_t *x, int16_t *y);

void DijkClearObstacles(void);
int16_t DijkAddObstacle(int16_t x, int16_t y, uint16_t larg, uint16_t ms);


#endif /* DIJKSTRA_H_ */
