
#ifdef STM32H745xx
#include "stm32h7xx_hal.h"
#endif

#ifdef STM32F722xx
#include "stm32f7xx_hal.h"
#endif

#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include <math.h>

#include "Sequenceur.h"
#include "Odometrie.h"
#include "TempsMs.h"


#include "Asserv.h"
#include "LowLevel.h"
#include "../App/Menu.h"



static long POSD,POSG;
static long PCONSD,PCONSG;


#define EcartCap 0   //Angle d'erreur � partir duquel on effectue une correction de cap (en 1/10 degr�s)


static void GetPos(void)  //met � jour POSD,POSG
   {
   static uint16_t ppg,ppd;  //positions pr�c�dentes
   uint16_t pg,pd;
   int16_t dg,dd;
   pg=POS1CNT;
   pd=POS2CNT;
   dg=pg-ppg;
   ppg=pg;
   POSG+=dg;
   dd=pd-ppd;
   ppd=pd;
   POSD+=dd;
   }

/*
long GetPosG(void)
   {
   return POSG;
   }   
 long GetPosD(void)
   {
   return POSD;
   }   
*/

/*
  long GetDelta(void)
   {
   return ((POSG-POSGdeb+POSD-POSDdeb)/2);
   }   
  long GetAlpha(void)
   {
   return ((POSD-POSDdeb-POSG+POSGdeb)/2);
   }     
*/   
#define GetDelta() ((POSG-POSGdeb+POSD-POSDdeb)/2)
#define GetAlpha() ((POSD-POSDdeb-POSG+POSGdeb)/2)   
   

static int DX_Cible, DY_Cible; // Coordonn�es relatives au robot du point � atteindre

int X_Cible, Y_Cible; // Coordonn�es du point � atteindre

int GetDX_Cible(void)
   {
   return DX_Cible;
   }   

int GetDY_Cible(void)
   {
   return DY_Cible;
   }   
   
/*
static unsigned DL_Cible;  //Distance du robot % cible
static void DXY_Cible(void)
   {
   int dx, dy, a;
   dx=X_Cible-P_X;
   dy=Y_Cible-P_Y;
   if((dx==0)&&(dy==0)) {DX_Cible=DY_Cible=DL_Cible=0; return;}
   a=MulDivS(THETA,SIN_NBA,3600);
   DX_Cible=DivS(MulS(dx,Nsin(a))-MulS(dy,Ncos(a)),KSIN);
   DY_Cible=DivS(MulS(dx,Ncos(a))+MulS(dy,Nsin(a)),KSIN);
   DL_Cible=Hypot(dx,dy);
   }   
*/


//Calcul de l'�cart angulaire de cap et de la distance� jusqu'� la cible (si dist != 0)
static float ErrCap(float *dist)
   {
   float dx, dy, a;
   dx=X_Cible-GetP_X();
   dy=Y_Cible-GetP_Y();
   if(dist)
	   {
	   *dist=hypotf(dx,dy); //__builtin_mulss(dx,dx)+__builtin_mulss(dy,dy);
	   }
   a=atan2f(dy,dx); // r�sultat entre -PI et PI
   a=a*(1800.f/PI)-GetTHETA();
   if(a<-1800.f) a+=3600.f;
   else if(a>=1800.f) a-=3600.f;
   return a;
   }


//Calcul du nombre de pas de décélération pour passer de la vitesse v à vmin, avec une décélération dec.
#define _NbPasDec(v,vmin,dec) (0.5f*((v)*(v)-(vmin)*(vmin))/((dec)-1))
#define _NbPasDec0(v,dec) (0.5f*v*v/(dec-1))

//#define DEBUG

#ifdef DEBUG
#define SZMEM 1000
int MEMv[SZMEM]; int MEMvodo[SZMEM];
unsigned NbMEM=0;
#endif




static long XDCONS,XGCONS;


static void CalculConsignes(void)
    {
    static unsigned Vit; //Vitesse de profil trapézoïdal
    long CVit; // Vitesse de consigne du robot
    static int CorCapVit; //Ajustement direction pour correction de cap
    unsigned AccCorVit;
    unsigned vprec,vacc;
    static long Xd,Xg;
    static long vd,vg; //vitesses des roues gauche et droite
    long vdprec,vgprec; //vitesses précédentes des roues gauche et droite

    static long xpp,lpp,lpp0;
    static int Type;
    long dx,Ndec;
    static unsigned acc,dec,vmax;
    static int circ=0;
    static int FirstStop=1;
    static unsigned vmin=200;	//Vitesse minimale
//    unsigned L;
    static T_ControlPrm ControlPrm={.Vmax=&vmax,.Acc=&acc,.Dec=&dec,.Circ=&circ,.Nbp=&lpp0};
    
    XDCONS=XGCONS=0;
    
    if(SEQ.Cmde==CMDE_STOP)
        {
        if(FirstStop)
            {
            vd=vg=0; 
            if(!SEQ._AssStop)
               {
               POSG=0;     //stockage des positions absolues
               POSD=0;     //de début du mouvement
               }
            FirstStop=0;
            CorCapVit=0;
            XDCONS=0; XGCONS=0;
            }
        Type=CMDE_STOP;
        return;
        }
    else FirstStop=1;
    if(SEQ.EtatProfil==P_ARRET)
       {
       if(!SEQ._AssStop)
          {
          POSG=0; POSD=0; XDCONS=0; XGCONS=0; vd=vg=0; return;
          }
       else
          {
          XDCONS=Xd/MULPREC; XGCONS=Xg/MULPREC; vd=vg=0; return; 
          }
       }
//    if(SEQ.EtatProfil==P_ARRET) { XDCONS=Xd/MULPREC; XGCONS=Xg/MULPREC; return; }
    if((SEQ.EtatProfil==P_INIT))  //préparation et démarrage du mouvement
        {
        POSG=0;
        POSD=0;
        Type=SEQ.Cmde;
        vprec=0;
        xpp=0;
        Xd=0;
        Xg=0;
        XDCONS=0; XGCONS=0;
        vd=vg=0;
        SEQ._Sens=0;
        CorCapVit=0;
        circ=SEQ.TrajC;
        switch(Type)
           {
           case CMDE_PGi:
              SEQ._Sens=1;
           case CMDE_PDi:
              acc=SEQ.PivotAcc;
              dec=acc;
              Vit=vmin=SEQ.PivotVmin;
              vmax=SEQ.PivotVmax;
              SEQ._Pivot=1;
              if(SYM_AUTO && MIROIR) SEQ._Sens=!SEQ._Sens;
              break;
           case CMDE_ARi:
              SEQ._Sens=1;
           case CMDE_AVi:
              acc=SEQ.MoveAcc;
              dec=acc;
              Vit=vmin=SEQ.MoveVmin;
              vmax=SEQ.MoveVmax;
              SEQ._Pivot=0;
              break;
           }
        switch(Type)
            {
            case CMDE_AVi:
            case CMDE_ARi:
                lpp=SEQ.Nbp; 
                break;
            case CMDE_PGi: //ne pas calculer la position de la cible
            case CMDE_PDi: //ne pas calculer la position de la cible
                lpp=SEQ.Nbp;
                break;
            }
        SEQ.MouvFini=0;
        SEQ.MouvInterrompu=0;
        lpp0=lpp;
        SEQ._HaltReq=0;
        SEQ._UrgReq=0;
        SEQ.EtatProfil=P_MOVE;              //démarrage d'un mouvement
        }
    if(SEQ.EtatProfil==P_MOVE)  //Mouvement en cours
        {
        //Début du générateur de profil de vitesse trapézoïdal (sur Vit)
        vprec=Vit;
        int vm=vmax;
        if(SEQ._ModulVmax && ((Type==CMDE_AVi)||(Type==CMDE_ARi)))
           {
           if(BAL_DISTMIN<=SEQ.Ddectect)
              {
              if(BAL_DISTMIN<=SEQ.Dnear) vm=SEQ.Vnear;
              else
                 {
                 if(vmax>SEQ.Vnear) vm=(float)(vmax-SEQ.Vnear)/(SEQ.Ddectect-SEQ.Dnear)*(BAL_DISTMIN-SEQ.Dnear)+SEQ.Vnear;
                 }
              }
           }
        if(Vit>vm) vacc=Vit-dec;  //Si vmax a diminué, on décélère
        else
           {
           vacc=Vit+acc;   //On essaie d'accélérer ...
           if(vacc>vm) vacc=vm; //... et on écrête éventuellement si on va trop vite !
           }
        if(vacc<vmin) vacc=vmin;
        Ndec=_NbPasDec(vacc,vmin,dec); //+((unsigned long)vacc+vprec)/2;  //Calcul du nombre de pas à faire pour s'arr�ter
        if(Ndec<lpp) Vit=vacc; //Si on n'a pas besoin de décélèrer
        else
            {
            if(Vit>vmin+dec) Vit-=dec;
            else Vit=vmin;
            }
         // Fin du générateur de profil de vitesse trapézoïdal
         // Vit contient la nouvelle consigne de vitesse du profil

        //Calcul de l'évolution de la position
        dx=((unsigned long)Vit+vprec)/2;
        xpp+=dx;
        lpp-=dx;
        lpp0-=dx;
        if(lpp<0) lpp=0;
//        X4=xpp;
        if((lpp==0) && (Vit==vmin))
            {
            SEQ.MouvFini=1;
            SEQ.EtatProfil=P_ARRET;
            Vit=vmin;
            if(lpp0<0) lpp0=0;
            if(SEQ._Pivot) SEQ.LRest=lpp0;
            else SEQ.LRest=lpp0;
            }
       if((SEQ.Cmde==CMDE_AVi)||(SEQ.Cmde==CMDE_ARi))
          {
          if(SEQ.ControlMovFct)
             {
	           if(SEQ.ControlMovFct(&ControlPrm))
	              {
   	           lpp=lpp0=0;
   	           SEQ.ControlMovFct=0;
   	           SEQ._HaltControlMovFct=1;
   	           }   
	          }
   	      if(SEQ.HaltReq)
   	        {
   		      if(SEQ.HaltReq())
   		         {
   		         lpp=lpp0=0;
   		         if(SEQ.HaltReqDec) dec=SEQ.HaltReqDec;
   		         SEQ.HaltReq=0;
   		         SEQ._HaltReq=1;
   		         }
   		      }
   		   }

        if((!SEQ.MouvInterrompu) && GetDetect())  //Arrêt sur obstacle
           {
           if(SEQ.UrgDec) dec=SEQ.UrgDec;
           lpp=0;
           SEQ.MouvInterrompu=1;
           SEQ._UrgReq=1;
           }

        }
   
   switch(Type)
  	   {
  	   default:
     	case CMDE_STOP:
     	   break;
	   case CMDE_AVi:    //translation av
      case CMDE_ARi:    //translation ar
      case CMDE_PDi:    //pivot droit
      case CMDE_PGi:    //pivot gauche
         CVit=Vit;
         if(SEQ._Sens) CVit=-CVit;
	      vdprec=vd; vgprec=vg;
         if(SEQ._CorCap)
            {
            if(!SEQ._Pivot)
               {
               float ea,d;
               ea=ErrCap(&d);  //Calcul angle d'erreur & distance�
               if(SEQ._Sens) ea=-ea;
               //Calcul accélération correction de cap (dépend de la vitesse de consigne)
               if(Vit<SEQ.CorCapVmax) AccCorVit=(SEQ.CorCapVmax-Vit)*SEQ.AccCorMax/SEQ.CorCapVmax;
               else AccCorVit=0;
               //Calcul de la vitesse de correction de cap
               if(d<SEQ.DistCorCap)  //à proximité de la cible : arr^tt de la correction de cap
                  {
                  if(CorCapVit>0) {CorCapVit-=AccCorVit; if(CorCapVit<0) CorCapVit=0;}
                  else if(CorCapVit<0) {CorCapVit+=AccCorVit; if(CorCapVit>0) CorCapVit=0;}
                  }
               else         //loin de la cible : ajustement de la correction de cap
                  {
                  if(ea>EcartCap) {if(CorCapVit>-30000) CorCapVit-=AccCorVit; }
                  else if(ea<-EcartCap) {if(CorCapVit<30000) CorCapVit+=AccCorVit; }
                  }   
               }
            else CorCapVit=0;
            }
         else CorCapVit=0;
	      //CorCapVit=CVit*20/100;
         if(SEQ._Pivot) {vd=-CVit; vg=CVit;}
         else
            {
            vd=CVit+CorCapVit/8;
            vg=CVit-CorCapVit/8;
            if(circ)
               {
               int x=0.001f*CVit*circ;
               if(SYM_AUTO && MIROIR) x=-x;
               vd+=x;
               vg-=x;
               }   
            }
         Xd+=(vd+vdprec)/2;
         Xg+=(vg+vgprec)/2;
         XDCONS=Xd/MULPREC;
         XGCONS=Xg/MULPREC;
        	break;
      }
   }


typedef struct
    {
    const float kp,kd,frs,seuilfrs;
    float e_prec;
    } T_Correc;

static T_Correc CorrecX= { .kp=CORRX_KP,.kd=CORRX_KD,.frs=CORRX_FRS,.seuilfrs=CORRX_SEUILFRS, .e_prec=0 };

static T_Correc CorrecG= { .kp=CORR_KP,.kd=CORR_KD,.frs=CORR_FRS,.seuilfrs=CORR_SEUILFRS, .e_prec=0 };
static T_Correc CorrecD= { .kp=CORR_KP,.kd=CORR_KD,.frs=CORR_FRS,.seuilfrs=CORR_SEUILFRS, .e_prec=0 };


static int16_t Correcteur(long cons, long pos, T_Correc *c)
{
   float e,u,deriv;

   e=cons-pos;                       //*******erreur

   u=e*c->kp;

   deriv=e-c->e_prec;
   c->e_prec=e;

   u+=deriv*c->kd;

   if (u > c->seuilfrs) u+=c->frs;   //compensation du frottement sec
   else if(u < -c->seuilfrs) u-=c->frs;

   if(u>1000.f) u=1000.f; else if(u<-1000.f) u=-1000.f;
   return u;
}


static volatile struct
   {
   int PXCONS; //Consigne appliquée au moteur
   int PX;   // position courante (codeur)
   float nbp; //nombre de pas à faire
   float vmax, vmin, acc;
   float vk,vkp,pc;
   volatile unsigned go:1;
   volatile unsigned done:1;
   unsigned sens:1;  //1 : avant - 0 : arrière
   unsigned fdec:1; //indique décélération enclenchée
   unsigned facc:1;
   unsigned asserv:1; //Active l'asservissement ou inactive
   }PRM_MOTX=
   {
      .vmax=10.f, .vmin=0.8f, .acc=0.1f,
      .vk=0.f, .vkp=0.f,
      .go=0,
      .done=1,
      .sens=0,
	  .asserv=0,
   };

void SetPrmX(float vmax, float acc)
    {
    if(vmax<PRM_MOTX.vmin) vmax=PRM_MOTX.vmin+acc;
    PRM_MOTX.vmax=vmax;
    PRM_MOTX.acc=acc;
    }

void StartX(int pos)
    {
    PRM_MOTX.PXCONS=pos;
    PRM_MOTX.go=1;
    while(PRM_MOTX.go);
    }

int XIsRunning(void)
    {
    return !PRM_MOTX.done;
    }

void MoveX(int pos)
    {
    StartX(pos);
    while(XIsRunning());
    }

int GetPosMotX(void)
    {
    return PRM_MOTX.PX;
    }

void StopMotX(void)
{
	PRM_MOTX.nbp=0;
	while(XIsRunning());
}

void AsservMotX(unsigned en)
{
	PRM_MOTX.asserv=((en!=0)?1:0);
}

static void GestMotX(void)
    {
    static int16_t ppx=0; //position pr�c�dente
    int16_t px,dx;
    float nr,fdx;
    //Gestion codeur
    px=POSXCNT;
    dx=px-ppx;
    ppx=px;
    PRM_MOTX.PX+=dx;
    //-------
    if(PRM_MOTX.go)
       {
       PRM_MOTX.nbp=PRM_MOTX.PXCONS-PRM_MOTX.PX;
       if(PRM_MOTX.nbp<0) {PRM_MOTX.nbp=-PRM_MOTX.nbp; PRM_MOTX.sens=0;} else PRM_MOTX.sens=1;
       PRM_MOTX.vk=PRM_MOTX.vmin;
       PRM_MOTX.vkp=0.f;
       PRM_MOTX.go=0;
       PRM_MOTX.done=0;
       PRM_MOTX.fdec=0;
       PRM_MOTX.facc=1;
       }
    if(!PRM_MOTX.done)
       {
       if(!PRM_MOTX.fdec)
          {
          float nx=(PRM_MOTX.vk-PRM_MOTX.vmin)/PRM_MOTX.acc;
          nr=nx*PRM_MOTX.vk-0.5f*nx*nx*PRM_MOTX.acc+0.5f;
          //nr=(PRM_MOTX.vk*PRM_MOTX.vk-PRM_MOTX.vmin*PRM_MOTX.vmin)/PRM_MOTX.acc/2.f;
          if(PRM_MOTX.nbp>nr)
             {
             PRM_MOTX.vk+=PRM_MOTX.acc;
             if(PRM_MOTX.vk>PRM_MOTX.vmax)
                {
                PRM_MOTX.facc=0;
                PRM_MOTX.vk=PRM_MOTX.vkp-PRM_MOTX.acc;
                if(PRM_MOTX.vk<PRM_MOTX.vmax) PRM_MOTX.vk=PRM_MOTX.vmax;
                }
             }
          else PRM_MOTX.fdec=1;
          }
        if(PRM_MOTX.fdec)
            {
            PRM_MOTX.vk-=PRM_MOTX.acc;
            if(PRM_MOTX.vk<PRM_MOTX.vmin) PRM_MOTX.vk=PRM_MOTX.vmin;
            }

       }
   //Calcul consignes position
   fdx=(PRM_MOTX.vk+PRM_MOTX.vkp)*0.5f;
   PRM_MOTX.vkp=PRM_MOTX.vk;
   PRM_MOTX.nbp-=fdx;
   if(PRM_MOTX.nbp<=0.f)
   {
      fdx+=PRM_MOTX.nbp;
      PRM_MOTX.nbp=0.0f;
      PRM_MOTX.vkp=PRM_MOTX.vk=0.0f;
      PRM_MOTX.done=1;
      PRM_MOTX.fdec=0;
   }

    if(PRM_MOTX.sens) PRM_MOTX.pc+=fdx;else PRM_MOTX.pc-=fdx;

    PWMX=VitMotX(Correcteur(PRM_MOTX.pc,PRM_MOTX.PX,&CorrecX));
    }


/*****************************/
volatile unsigned T_Trajet=0;
/*****************************/
static unsigned NSyncSPI=0; //Resynchronise SPI si mis à 0
static unsigned CtSyncSPI=0; //compteur pour resynchroniser SPI

void TryToSyncSPI(void)
   {
   EnableAsserv(0);
   NSyncSPI=0;
   CtSyncSPI=0;
   EnableAsserv(1);
   }


void GestAsserv(void)
   {
   int VG,VD;
   long pg,pd;
   RefreshLidarData(); //Pour accéder aux variables partagées avec le Lidar (dévalide Data Cache Lidar)
   	/***************************************************/
	T_Trajet++;
    /***************************************************/


if(1)
   {//clignotement LED pendant asservissement
   static unsigned ct=0;
   ct++;
   if(ct==500) {_LEDJ(0);}
   if(ct==1000) {ct=0;_LEDJ(1);}
   }

 _LEDR(GetDetect());

GetPos();
pg=POSG;
pd=POSD;
Odometrie();
    	
    	
//Synchro SPI au démarrage
//Emission d'une première trame, puis une 2ème 15ms plus tard. -> permet synchro capteur (DT>10ms)
//Après 2ème trame : 1 trame toutes les ms
#if 0
   {
   if(NSyncSPI>1) SendSpi();
   else
      {
      if(CtSyncSPI==0) {CtSyncSPI=15; SendSpi(); NSyncSPI++;}
      CtSyncSPI--;
      }
   }
#endif

   if(SEQ.FctAtTime)
      {
	  if(RBT_TIME==SEQ.TIME_FCT)
	     {
		  SEQ.FctAtTime();
		  SEQ.FctAtTime=0;
	     }
      }

   if(RBT_TIME<=0)  //Fin du match
      {
      if(RBT_TIME<-10000) RBT_TIME=-10000;
         _Powen(0);
         _PowenX(0);
	     return;
	     }

	   CalculConsignes();

    PCONSD=XDCONS;
    PCONSG=XGCONS;

     
 //*****************************************************************************
 //      asservissement en position des moteurs D et G
 //*****************************************************************************
 	//static float eg_prec=0,ed_prec=0;
  
  	VG=Correcteur(PCONSG,pg,&CorrecG); //&eg_prec);
  	VD=Correcteur(PCONSD,pd,&CorrecD); //&ed_prec);

   

	PWMG=VitMotG(VG);
	PWMD=VitMotD(VD);

	if(PRM_MOTX.asserv) GestMotX();

 /***************************************************************************   
  //        asservissement polaire Alpha Delta 
  
  	 GetPos();
     commande_dist = DCONS-GetDelta();
     commande_rot  = ACONS-GetAlpha();
     
     PCONSD=commande_dist+commande_rot;
     PCONSG=commande_dist-commande_rot;
     
     
 
  e=PCONSG-pg;
  eu=(e>=0)?e:(-e);
	if(eu>1000/kp0g) pkg=(e>=0)?1000:(-1000);
	else 
    	{   pkg=e*kp0g;
            if (pkg) pkg=pkg+frsg;
            else    pkg=pkg-frsg;
      }   
  e=PCONSD-pd;
	eu=(e>=0)?e:(-e);
	if(eu>1000/kp0d) pkd=(e>=0)?1000:(-1000);
	else 
    	{   pkd=e*kp0d;
            if (pkd) pkd=pkd+frsd;
            else    pkd=pkd-frsd;
      }   
	PWMG=VitMotG(pkg);
	PWMD=VitMotD(pkd);

**********************************************************************/
   }   




