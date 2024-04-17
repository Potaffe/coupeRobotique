/*
 * Match.h
 *
 *  Created on: 28 nov. 2016
 *      Author: Robot
 */

#ifndef MATCH_H_
#define MATCH_H_


void StartXPos(int n);
void MoveXPos(int n);


extern volatile float VDist;

void Pompe(int st); //-1 : aspire   0 : Arret   1 : souffle


void MatchStd(void);

void Homologation(void);
void TestVitesse(void);

void MatchQualif(void);
void MatchFinale(void);
void Demo(void);

void RangerBras(void);

#endif /* MATCH_H_ */
