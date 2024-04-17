

#ifndef __SharedMemory_h__
#define __SharedMemory_h__

#include <stdint.h>
#define NBBALMAX 10

enum {COUL_POS,COUL_NEG};


#define SZX_TABLE 3000   //Longueur en X de la table
#define SZY_TABLE 2000   //Longueur en Y de la table



typedef uint32_t T_Pos;

/*
 * T_Pos : infos balises sur 32 bits (pour manipulation "atomique")
 * b20 à b31 : abscisse (x)
 * b9 à b19 : ordonnée (y)
 * b0 à b8 : life (en 10 ms)
 */
uint16_t getBAL_LIFE(T_Pos);
int16_t getBAL_X(T_Pos);
int16_t getBAL_Y(T_Pos);
void setBAL_LIFE(T_Pos*,uint16_t);
void setBAL_X(T_Pos*,int16_t);
void setBAL_Y(T_Pos*,int16_t);

#define NB_BITS_X 11
#define NB_BITS_Y 12
#define NB_BITS_LIFE 9


typedef struct
   {
   int16_t px,py,th; //Coordonnées odométrie
   int16_t detect_av, detect_ar; //Active détection avant ou arrière
   int16_t dist_av, dist_ar; //Distances de détection pour demande arrêt
   int16_t couleur;
   int16_t sym_auto;
   int16_t start_lidar;
   //Paramètres de détection
   int16_t sz_bal_min; //Taille min de la cible à trouver
   int16_t sz_bal_max; //Taille max de la cible à trouver
   int16_t ec_max_ech; //Distance max entre 2 échantillons de la même cible
   int16_t ec_max_bal; //Ecart min entre 2 balises pour pouvoir les distinguer (sinon : confondues)
   int16_t dt_bal_raf; //Période de rafraichissement des balises en ms (durée de vie)
   int16_t larg_robot; //largeur du robot
   T_Pos ami;
   //debug-test
   uint32_t ct_CM7; //Compteur piloté par CM7
   uint32_t ct2_CM7; //Compteur piloté par CM7
   }T_FromRobotToLidar;

   typedef struct
      {
      T_Pos c[NBBALMAX];  //Position des cibles vues par Lidar
      uint16_t NbBal;  //Nombre de balises vues
      int16_t emergency; //Demande arrêt
      int16_t DistMin; //Indice de la balise la plus proche
      uint16_t Ready;   //Indique Lidar opérationnel et prêt
      //debug-test
      uint32_t ct_CM4; //Compteur piloté par CM4
      uint32_t ct2_CM4; //Compteur piloté par CM4
      }T_FromLidarToRobot;


extern volatile T_FromRobotToLidar * const FromRobotToLidar;
extern volatile T_FromLidarToRobot * const FromLidarToRobot;

void InitSharedData(void);

#define THETA (FromRobotToLidar->th)
#define P_X (FromRobotToLidar->px)
#define P_Y (FromRobotToLidar->py)

#define COULEUR (FromRobotToLidar->couleur)
#define SYM_AUTO (FromRobotToLidar->sym_auto)
#define START_LIDAR (FromRobotToLidar->start_lidar)

#define AMI (FromRobotToLidar->ami)

#define BAL (FromLidarToRobot->c)
#define NB_BAL (FromLidarToRobot->NbBal)
#define EC_MAX_ECH (FromRobotToLidar->ec_max_ech) //25.f  //Distance max entre 2 échantillons de la même cible

#define SZ_BAL_MIN (FromRobotToLidar->sz_bal_min) //35.f //Taille min de la cible à trouver
#define SZ_BAL_MAX (FromRobotToLidar->sz_bal_max) //120.f //Taille max de la cible à trouver
#define EC_MAX_BAL (FromRobotToLidar->ec_max_bal) //150.f //Ecart min entre 2 balises pour pouvoir les distinguer (sinon : confondues)
#define DT_BAL_RAF (FromRobotToLidar->dt_bal_raf) //1000  //Période de rafraichissement des balises en 10 ms
#define LARGEUR_ROBOT (FromRobotToLidar->larg_robot)

#define BAL_DETECT_AV (FromRobotToLidar->detect_av)
#define BAL_DETECT_AR (FromRobotToLidar->detect_ar)
#define BAL_DIST_AV (FromRobotToLidar->dist_av)
#define BAL_DIST_AR (FromRobotToLidar->dist_ar)

#define BAL_EMERGENCY (FromLidarToRobot->emergency)
#define LIDAR_READY (FromLidarToRobot->Ready)
#define BAL_DISTMIN (FromLidarToRobot->DistMin) //Indice de la balise la plus proche (-1 si aucune)

float DistBal(T_Pos b);
//#define DistBal(b) hypotf(P_X-getBAL_X(b),P_Y-getBAL_Y(b))

#define CT_CM4 (FromLidarToRobot->ct_CM4)
#define CT_CM7 (FromRobotToLidar->ct_CM7)
#define CT2_CM4 (FromLidarToRobot->ct2_CM4)
#define CT2_CM7 (FromRobotToLidar->ct2_CM7)



#ifdef CORE_CM7
void RefreshLidarData(void);
uint32_t Get_CT_CM4(void);
uint32_t Get_CT2_CM4(void);
#endif

#endif
