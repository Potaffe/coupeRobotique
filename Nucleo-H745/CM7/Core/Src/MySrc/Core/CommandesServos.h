/*
 * CommandesServos.h
 *
 *  Created on: Jun 10, 2016
 *      Author: Md
 */

#ifndef COMMANDESSERVOS_H_
#define COMMANDESSERVOS_H_

#include <stdint.h>
//Version du 27/10/2014 : R�ception par buffer (pas de fifo)
//Version du 24/04/2103 : Gestion récupération défauts : réarme automatiquement couple
//Version du 29/11/2010 - Gestion des Servos AX & RX
//Nécessite TempsMs.c, TempsMs.h


#define ALL_SERVOS 0xFE // Identifiant d'utilisation simultanée de tous les servos (Broadcasting id)
#define BROADCASTING_ID 0xFE   // m�me que ci-dessus

//Macros d'accès aux servos AX ou RX
// exemples :
//   ServoLed(AXID(1),1) allume la LED du servo AX12-ID1
//   ServoLed(RXID(2),0) �teint la LED du servo RX24-ID2
//   ServoId(RXID(1),2)  renomme le servo RX24-ID1 en RX24-ID2

#define AXID(id) (id)          // Calcule l'identifiant des AX (0 � 127)
#define RXID(id) ((id)|0x80)  // Calcule l'identifiant des RX (0 � 125 -> 128 � 253)


// Initialisation des périphériques liés aux servos
void ServoInit(void);

// Renvoie un code d'erreur suite � l'ex�cution d'une commande de servo
unsigned GetServoErr(void);

// Efface toutes les erreurs
// Note : les erreurs sont syst�matiquement effac�es au d�but de l'ex�cution d'une commande
void ClearServoErr(void);

//Renvoie UN message d'erreur et acquitte l'erreur indiqu�e
//Renvoie une cha�ne vide si pas d'erreur ("")
const char *GetSvErrorMsg(void);


/* SetAlarmLED
If the corresponding alrm Bit is set to 1, the LED blinks when an Error occurs.
BitFunction
Bit 7 : 0
Bit 6 : If set to 1, the LED blinks when an Instruction Error occurs
Bit 5 : If set to 1, the LED blinks when an Overload Error occurs
Bit 4 : If set to 1, the LED blinks when a Checksum Error occurs
Bit 3 : If set to 1, the LED blinks when a Range Error occurs
Bit 2 : If set to 1, the LED blinks when an Overheating Error occurs
Bit 1 : If set to 1, the LED blinks when an Angle Limit Error occurs
Bit 0 : If set to 1, the LED blinks when an Input Voltage Error occurs
This function operates following the �OR� logical operation of all bits.
For example, if the value is set to 0X05, the LED will blink when an Input Voltage Error
occurs or when an Overheating Error occurs.
Upon returning to a normal condition from an error state, the LED stops blinking after 2 seconds.
*/
unsigned char *SetAlarmLED(unsigned id, unsigned char alrm);


/*SetAlarmShutdown
If the corresponding alrm Bit is set to a 1, the Dynamixel actuator�s torque will be turned off
when an error occurs.
Bit 7 : 0
Bit 6 : If set to 1, torque off when an Instruction Error occurs
Bit 5 : If set to 1, torque off when an Overload Error occurs
Bit 4 : If set to 1, torque off when a Checksum Error occurs
Bit 3 : If set to 1, torque off when a Range Error occurs
Bit 2 : If set to 1, torque off when an Overheating Error occurs
Bit 1 : If set to 1, torque off when an Angle Limit Error occurs
Bit 0 : If set to 1, torque off when an Input Voltage Error occurs
This function operates following the �OR� logical operation of all bits.
However, unlike the Alarm LED, after returning to a normal condition, it maintains
the torque off status. To recover, the Torque Enable (Address0X18) needs to be reset to 1.
*/
unsigned char *SetAlarmShutdown(unsigned id, unsigned char alrm);


// Renvoie l'identifiant (1 � 253) du servo ou 0xFF si pas de r�ponse
unsigned char ServoPing(unsigned id);

// Remet le servo dans la config par d�faut (� �viter, car remet ID � 1)
unsigned char ServoReset(unsigned id);

// Ex�cute une commande mise en attente juste avant par ServoRegMode(1)
// Permet des executions de commandes multiples et synchrones
// Les commandes multiples doivent porter sur les m�mes servos (soit AX, soit RX, mais pas les 2)
void ServoAction(void);

//Positionne le servo � la position angulaire indiqu�e (en �/10, de 0 � 3000)
unsigned char *SetServoAngleDeg(unsigned id, unsigned angle);

//Positionne le servo � la position angulaire indiqu�e (0 � 1023 pour 0� � 300�)
unsigned char *SetServoAngle(unsigned id, unsigned angle);
uint16_t *GetServoAngle(unsigned id);

//Renvoie la derni�re consigne de position demand�e � un servo (ou -1 si pas de consigne)
int GetServoCons(unsigned id);
//Affecte une valeur de consigne de position sans la transmettre au servo
void SetServoCons(unsigned id, unsigned angle);


uint16_t  *GetServoModel(unsigned id);

// sp=0...1023
unsigned char *SetServoSpeed(unsigned id, unsigned sp);
uint16_t  *GetServoSpeed(unsigned id);

// Couple appliqu�. (sign� 10 bits, n�gatif si CCW)
int16_t  *GetServoLoad(unsigned id);

unsigned char *SetServoMaxTorque(unsigned id, unsigned t);
uint16_t  *GetServoMaxTorque(unsigned id);
int GetServoConsTorque(unsigned id);


//Permet de fournir du couple (si en=1)
unsigned char *EnableServoTorque(unsigned id, unsigned en);

//Donne l'�tat du couple (ON ou OFF)
//Rappel : couple d�sactiv� en cas d'erreur
int GetServoTorqueStatus(unsigned id);

/*
 * Status Return Level
It decides how to return Status Packet. There are three ways like the below table.
Value Return of Status Packet
 0      No return against all commands (Except PING Command)
 1      Return only for the READ command
 2      Return for all commands
 */
unsigned char *ServoStatusReturnLevel(unsigned id, unsigned st);
int GetServoStatusReturnLevel(unsigned id);


// Allume (led=1) ou �teint (led=0) la LED du servo
unsigned char *ServoLed(unsigned id, unsigned led);

// Renvoie l'�tat de la LED du servo
int GetServoLed(unsigned id);

//Modifie l'identifiant du servo
unsigned char *ServoId(unsigned id, unsigned new_id);

//Donne les angles mini et maxi
unsigned char *SetServoAngleLimits(unsigned id, unsigned cw, unsigned ccw);

unsigned char *SetServoSlope(unsigned id, unsigned char cw, unsigned char ccw);

unsigned char *SetServoMargin(unsigned id, unsigned char cw, unsigned char ccw);

unsigned char *SetServoPunch(unsigned id, unsigned x);

//Renvoie la position courante du servo
uint16_t GetServoPosition(unsigned id);

//Passe en mode rotation infinie � la vitesse indiqu�e
unsigned char *SetEndlessTurn(unsigned id, unsigned speed);

// Envoie la commande qui suit en mode REG_WRITE (action ex�cut�e par ServoAction(id))
void ServoRegMode(char mode);

// Recherche des servos connect�s, en d�butant du N� first jusqu'au num�ro last.
// Renvoie le N� du premier servo trouv� (� partir de first)
// Renvoie last+1 si pas trouv�
int GetNextServo(unsigned first, unsigned last);


// Interroge le servo
// Renvoie 1 si mouvement, 0 si arr�t et -1 si erreur
int GetServoMoving(unsigned id);

//fonction pour servos doubl�s
unsigned char *SetServoSpeedDouble(unsigned id1, unsigned id2, unsigned sp);
unsigned char *SetServoPunchDouble(unsigned id1,unsigned id2, unsigned x);
unsigned char *SetServoMarginDouble(unsigned id1,unsigned id2, unsigned char cw, unsigned char ccw);
unsigned char *SetServoSlopeDouble(unsigned id1,unsigned id2, unsigned char cw, unsigned char ccw);
unsigned char *SetServoAngleLimitsDouble(unsigned id1, unsigned id2, unsigned cw, unsigned ccw);
unsigned char *SetServoMaxTorqueDouble(unsigned id1,unsigned id2, unsigned t);
unsigned char *EnableServoTorqueDouble(unsigned id1, unsigned id2, unsigned en);
unsigned char *SetServoAngleDegDouble(unsigned id1,unsigned id2, unsigned angle);
unsigned char *SetServoAngleDouble(unsigned id1,unsigned id2, unsigned angle);
unsigned char *SetServoAngleDegDouble(unsigned id1,unsigned id2, unsigned angle);
unsigned char *SetServoAngleDegDegDouble(unsigned char id1,unsigned char id2, unsigned angle);



typedef struct
   {
   uint8_t id;  //Id du servo
   uint16_t pos; //Position souhait�e
   uint16_t err; //Erreur de position acceptable - Si=0 : pas contr�le err position
   }T_GROUPE;

void DetectServos(void);  // détecte les servos AX et RX et les affiche
void TestAutoServos(void);
void AffServos(unsigned nmax);
void AffAllGCMD(void);
void ArretDesServos(int);
void InitialisationDesServos(void);


//Lance le mouvement sans attendre
int ServoMove(unsigned id, unsigned pos);

//Lance le mouvement et attend la fin de celui-ci pour effectuer la prochaine commande
int ServoMoveW(unsigned id, unsigned pos);

//Lance le mouvement, attend la fin de celui-ci pour effectuer la prochaine commande et reste en attente de la fin d'ex�cution du mouvement
int ServoMoveWW(unsigned id, unsigned pos);



int DServoMoveWait(unsigned id1, unsigned pos1, unsigned id2, unsigned pos2);
//int GServoMoveWait(const T_GROUPE *s, unsigned nb);


void DServoMove(unsigned id1, unsigned pos1, unsigned id2, unsigned pos2);

void RXInit(int idorg, int iddest);
void AXInit(int idorg, int iddest);

int ServoInitStd(int id, unsigned slope, unsigned vit, unsigned torque, unsigned margin);

//Programme une synchro en attendant que la fonction fct renvoie 1.
int ServoSync(int(*fct)(void));

//Programme une synchro en attendant que la fonction fct renvoie 1. Se met en attente que synchro soit effectu�e
int ServoSyncW(int(*fct)(void));


//Appel de fonction avec paramètres
//prm contient nbprm entiers
//nbprm max = 4
int ServoFct(void (*fprm)(uint16_t nbprm, const int16_t *prm), uint16_t nbprm, const int16_t *prm);
int ServoFctW(void (*fprm)(uint16_t nbprm, const int16_t *prm), uint16_t nbprm, const int16_t *prm);

//Définition de l'erreur maximale admissible pour les mouvements avec attente
void DefineErrMax(uint16_t err);


void ServosEnable(int en);
int GMouvement(uint16_t n);
int GMouvementW(uint16_t n);
void InitSvGr(void);
int NewGFCT(uint16_t n, void (*fct)(void));

int GMouvementGr(const int16_t *gcmd);

void InitSvNames(void);
void SetSvName(uint16_t id, const char *name);
const char *GetSvName(uint16_t id);


//void InitGCMD(void);
void InitGCMD();
int NewGCMD(uint16_t n, const int16_t *gcmd); // Le tableau gcmd doit �tre statique !!!
void AffGCMD(uint16_t n);

//Gestion des commandes de groupes de servos
#define NBGCMDMAX 60  //Nombre maxi de groupes de commandes de servos
#define NBGCMDSVMAX 10  //Nombre maxi de servos dans les groupes de commandes
#define G_ERR_DEF 10  //Erreur de position par d�faut
#define NBGFCTMAX 40  //Nombre maxi de fonctions utilisables dans les commandes groupées de servo


enum GrCmdes{
     GC_FIN=-1,
     GC_SUB=-2,
     GC_DELAI=-3,
     GC_VIT=-4,
     GC_TORQUE=-5,
     GC_MARGIN=-6,
     GC_SLOPE=-7,
     GC_ERR=-8,
     GC_TEST=-9,
     GC_ADJUST=-10,
     GC_AFFCP=-11,
     GC_MODEWAIT=-12,
	 GC_FCT=-13,
};

//Affichage des positions des servos indiqués
void TestPosServos(uint16_t nb,...);

#ifndef __SIM__
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"


void RT_Servos( void *pvParameters );
extern QueueHandle_t QServos;
extern TaskHandle_t hServos;
void ServoStopRequest(void);
#endif

void ServosSetModeRT(uint16_t m); //Active ou d�sactive le mode RT (non activ� par d�faut - Activ� automatiquement si t�che RT_Servos activ�e)
uint16_t ServosGetModeRT(void);

//Indique que les initialisations de la tâche des servos ont été effectuées
int GetServoReady(void);

#endif /* COMMANDESSERVOS_H_ */
