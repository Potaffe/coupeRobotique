/*
 * OtherTasks.h
 *
 *  Created on: 13 déc. 2022
 *      Author: robot
 */

#ifndef SRC_MYSRC_APP_OTHERTASKS_H_
#define SRC_MYSRC_APP_OTHERTASKS_H_

extern TaskHandle_t hOtherTasks;

void RT_OtherTasks(void *pvParameters);

//Charge une fonction en temps que tâche
int LoadTask(void (*fct)(void));

//Renvoie le nombre de tâches encore actives
int NbTasks(void);



#endif /* SRC_MYSRC_APP_OTHERTASKS_H_ */
