#ifndef __SEQUENCEUR_H__
#define __SEQUENCEUR_H__
//#include "Estimateur.h"

#include "stdint.h"

typedef struct
   {
   int x,y;
   }T_POINT;

#define RBT_EOK 0
#define RBT_EINTFCTSTOP 1
#define RBT_EROTEXIT 2
#define RBT_EMOVEXIT 4
#define RBT_EEMERGENCY 8
#define RBT_EBLOCKING 16
#define RBT_EHALTREQ 32
#define RBT_EHALTCONTF 64
#define RBT_EOTHER 128


#define RBT_AV 1
#define RBT_AR 0

#define __VMIN_DEF__ 800

typedef struct
   {
   unsigned *Vmax;
   unsigned *Dec, *Acc;
   int *Circ;  //Pour trajet circulaire entre -1000 et 1000
   const long *Nbp;
   }T_ControlPrm;

void DefineEndOfGame(void (*fct)(void));
void DefineFctDuringMovement(void (*fct)(void));
void (*GetFctDuringMovement(void))(void);

void DefineFctAtTime(void (*fct)(void), int16_t t);

void SpeedLimitNear(unsigned vmax, unsigned dnear, unsigned ddetect);

//Active la protection anti-collision avec bord de table. (P_X,P_Y) ne peut se rapprocher à moins de dist du bord
//Désactivation de la protection si dist=0 (par défaut)
//Attention : Stretch permet d'aller au delà de la limite !!!
void SetBoardGuard(unsigned dist);


int Rbt_Move(int xc, int yc, int av);
int Rbt_MoveX(int xc, int yc);
int Rbt_RMove(int dl);
int Rbt_RForward(unsigned dl);
int Rbt_RBackward(unsigned dl);
int Rbt_RRotate(int da);
int Rbt_RRotateLeft(unsigned da);
int Rbt_RRotateRight(unsigned da);
int Rbt_AbsRotate(int ang);
void Rbt_MovPrm(unsigned vmin, unsigned vmax, unsigned acc);
void Rbt_RotPrm(unsigned vmin, unsigned vmax, unsigned acc);
void Rbt_ControlFct(int (*fct)(T_ControlPrm *));
void Rbt_HaltReq(unsigned dec, int (*fct)(void));
void Rbt_DefInterFct(int (*fct)(void));
void Rbt_DefRotInterFct(void (*fct)(int));
void Rbt_DefMovInterFct(void (*fct)(int));
void Rbt_MemoPos(void);
void Rbt_AssStop(int en);
void Rbt_CurveM(int c);
void Rbt_Stretch(int dl);
void Rbt_Emergency(unsigned dec, unsigned ms);

int _AvanceVers(int x, int y);
int _ReculeVers(int x, int y);
int _VaVers(int x, int y);


#define AvanceVers(x,y) Rbt_Move((x),(y),1)
#define ReculeVers(x,y) Rbt_Move((x),(y),0)
#define VaVers(x,y) Rbt_MoveX((x),(y))
#define AvanceDe(d) Rbt_RForward(d)
#define ReculeDe(d) Rbt_RBackward(d)
#define TourneGaucheDe(da) Rbt_RRotateLeft(da)
#define TourneDroiteDe(da) Rbt_RRotateRight(da)
#define TourneVers(a) Rbt_AbsRotate(a)
#define ParamLin(vmax,acc) Rbt_MovPrm(__VMIN_DEF__,(vmax),(acc));
#define ParamRot(vmax,acc) Rbt_RotPrm(__VMIN_DEF__,(vmax),(acc));
#define SiUrgence(dec,ms) Rbt_Emergency((dec),(ms))
#define DecUrgence(dec) Rbt_Emergency((dec),0)
#define StopUrgence() Rbt_Emergency(0,0)


//Met en mémoire (pile LIFO) les paramètres de déplacement linéaires (si type=0) ou de rotation (si type=1)
//Renvoie 0 en cas d'erreur (plus de place ou type invalide)
int PushParam(int type);

//Récupère de la pile les paramètres de déplacement linéaires (si type=0) ou de rotation (si type=1)
//Renvoie 0 en cas d'erreur (pile vide ou type invalide)
int PopParam(int type);




#define TEMPS_RESTANT RBT_TIME

enum ValEtatProfil {P_ARRET,P_MOVE,P_INIT};

enum {CMDE_STOP,CMDE_AVi,CMDE_ARi,CMDE_PDi,CMDE_PGi};


typedef struct
   {
   int Cmde;
   long lpp;
   }T_CMD;

struct T_SEQ
   {
   long Nbp;
   long LRest; //restant � effectuer quand mouvement interrompu
   int Cmde;
   unsigned MoveVmin,MoveVmax,MoveAcc;
   unsigned PivotVmin,PivotVmax,PivotAcc;
   unsigned HaltReqDec;
   int (*ControlMovFct)(T_ControlPrm *);
   int (*HaltReq)(void);
   volatile uint16_t EtatProfil;//uint16_t EtatProfil;
   unsigned UrgDec;  //décélération d'urgence
   unsigned DelaiMsUrg; //délai d'attente (ms) avant de lancer le profil d'urgence (URG)
   int (*INTERFCT)(void);
   void (*RINTERFCT)(int);  //Fonction appelée avant rotation (arg=1 si PD)
   void (*MINTERFCT)(int);  //Fonction appelée avant translation (arg=1 si AV)
   void (*FctAtTime)(void);  //Fonction à appeler à l'instant TIME_FCT
   int16_t TIME_FCT;

   unsigned CorCapVmax,AccCorMax,DistCorCap; //Vit max pour corr cap, Acc max pour corr  cap (à vit nulle), Distance min correct. cap (mm)
   int TBlocMax,NbTopsBloc,VBLOC;  //Durée blocage en ms, 18.6 tops/mm pour roue 70mm,Vitesse min de détection du blocage (0 � 1000)
   int MEMO_PX,MEMO_PY,MEMO_THETA;
   int TrajC;   //Possibilité de faire une trajectoire circulaire (-1000 à 1000, en relatif vitesses)
   int Stretch;  //Allongement ou raccoursissement d'un trajet linéaire
   int Dnear,Ddectect,Vnear; //Modulation de la vitesse max à proximité d'un autre robot
   unsigned BoardGuard; //Distance minimale de la bordure de table % robot. Désactivé si 0
   volatile unsigned MouvFini:1;
   volatile unsigned MouvInterrompu:1;
   unsigned _URG:1;
   unsigned _HaltReq:1;
   unsigned _HaltControlMovFct:1;
   unsigned _UrgReq:1;
   unsigned _Pivot:1; //Indique un mouvement en Pivot
   unsigned _Sens:1; //Sens AV/AR ou Pivot D/G
   unsigned _CorCap:1; //Activation/Désactivation correction de cap
   unsigned _AssStop:1; //Asservissement position en arrêt
   unsigned _ModulVmax:1;
   };

int SeqEtat(T_POINT *org);

extern volatile struct T_SEQ SEQ;

void DetectDist(int16_t mm);



#endif
