#ifndef __ASSERV_H__
#define __ASSERV_H__


#ifndef __SIM__
#include "../App/Config.h"
#endif

#ifndef PI
#define  PI 3.1415926535897932384626433832795f
#endif


//Constantes à ne pas modifier !
#define INTER_ROUE_LIBRE (INTER_ROUE_LIBREC*KDISSYM)
#define KDIM ((float)(PI*DIAM_ROUE_LIBRE/TOPSCODEUR_ROUE_LIBRE))   //mm par top
#define D_COD(mm) ((float)((mm)/KDIM)) // dist.en mm convertie en tops codeur

#define MULPREC 1024

//#define _ConvAnglelpp (PI*INTER_ROUE_LIBREC/KDIM*MULPREC/3600.f)
#ifndef __SIM__
#define _ConvAnglelpp(x) ((float)(x)*(INTER_ROUE_LIBREC/3600.f/DIAM_ROUE_LIBRE*MULPREC*TOPSCODEUR_ROUE_LIBRE))
#define _ConvDistlpp(x) ((float)(x)*(MULPREC/KDIM))
#else
#define _ConvAnglelpp(x) x
#define _ConvDistlpp(x) x
#endif


/////////////////////////////////////////////////////////////////////////



long GetPosG(void);
long GetPosD(void);
void TryToSyncSPI(void);
void GestAsserv(void);

void SetPrmX(float vmax, float acc);
void StartX(int pos);  //Lance le mouvement sur le moteur X vers la position pos
int XIsRunning(void);  //Indique si le moteur X est en train de tourner
void MoveX(int pos);  //Bouge le moteur X jusqu'à la position indiquée. Attend la fin du mouvement.
int GetPosMotX(void); //Renvoie la position codeur du moteur X
void StopMotX(void);
void AsservMotX(unsigned en);


#endif
