
#include "Sequenceur.h"
#include <math.h>

#ifndef __SIM__
#include "../../../../../Common/Src/LCD_VT100.h"

#include "TempsMs.h"

#include "../App/Config.h"
#include "Asserv.h"
#include "LowLevel.h"
#include "Odometrie.h"

#include "../App/Menu.h"

#define AFF 0
#define DBG_SEQ 0  //Affichage infos debug

#endif

void DetectStop(void)
   {
   BAL_DETECT_AV=0;
   BAL_DETECT_AR=0;
   }

void DetectAV(void)
   {
   BAL_DETECT_AV=1;
   BAL_DETECT_AR=0;
   }

void DetectAR(void)
   {
   BAL_DETECT_AV=0;
   BAL_DETECT_AR=1;
   }

void USAVAR(void)
   {
   BAL_DETECT_AV=1;
   BAL_DETECT_AR=1;
   }

//Définition de la distance de sécurité entre les robots (bras relevés), en mm. Mettre 0 pour contact.
void DetectDist(int16_t mm)
   {
   if(mm<=0)  //detect off
      {
      BAL_DIST_AV=50;
      BAL_DIST_AR=50;
      return;
      }
   mm+=DETECT_DIST_ADV;
   BAL_DIST_AV=mm+DETECT_DIST_AV;
   BAL_DIST_AR=mm+DETECT_DIST_AR;
   }


volatile struct T_SEQ SEQ =
          {
           .Nbp=0,
           .LRest=0,
           .Cmde=CMDE_STOP,
           .MoveVmin=__VMIN_DEF__,.MoveVmax=10000,.MoveAcc=30,
           .PivotVmin=__VMIN_DEF__,.PivotVmax=8000,.PivotAcc=15,
           .HaltReqDec=40,
           .ControlMovFct=0,
           .HaltReq=0,
           .EtatProfil=P_ARRET,
           .UrgDec=30,
           .DelaiMsUrg=1000,
           ._URG=0,
           .INTERFCT=0,
           .RINTERFCT=0,
           .MINTERFCT=0,
           .CorCapVmax=4000,.AccCorMax=2,.DistCorCap=100,
           .TBlocMax=30,.NbTopsBloc=20,.VBLOC=500,
           .TrajC=0,
           .Stretch=0,
           .BoardGuard=0,
           .MouvFini=0,
           .MouvInterrompu=0,
           ._HaltReq=0,
           ._HaltControlMovFct=0,
           ._UrgReq=0,
           ._Pivot=0,
           ._Sens=0,
           ._CorCap=0,
           ._AssStop=1,
           ._ModulVmax=0,
           };


//==============================================================================
//Gestion des SUB-Profils (eq. sous pgms)

#ifndef DIM
#define DIM(x) (sizeof(x)/sizeof(*x))
#endif




static int LAST_PX,LAST_PY;  //Dernières positions avant mouvement

//Renvoie type du mouvement pr�c�dent
// 0 : AV
// 1 : AR
// 2 : PD
// 3 : PG
int SeqEtat(T_POINT *org)
   {
   int x;
   if(org)
      {
      org->x=LAST_PX;
      org->y=LAST_PY;
      }
   x=SEQ._Sens; //0 : av/pd - 1 : ar/pg
   if(SEQ._Pivot)
      {
      x|=2;
      if(SYM_AUTO && MIROIR) x^=1; //
      }
   return x;
   }

//R�cup�ration des flags mis à 0 dans Asserv. pour recopie dans séquenceur
static void RestoreAutoAcquit(struct T_SEQ *s)
   {
   s->ControlMovFct=SEQ.ControlMovFct;
   s->HaltReq=SEQ.HaltReq;
   }


static void (*ActionFinMatch)(void)=0;  //Fonction à appeler en fin de Match
static void (*FctWaitDuringMovement)(void)=0; //

//Lancement d'une fonction fct à un instant t déterminé (exécuté pendant l'interruption)
//La fonction est exécutée quand RBT_TIME atteint t
//La fonction ne s'exécute qu'une fois (désactivation après l'exécution)
void DefineFctAtTime(void (*fct)(void), int16_t t)
{
	SEQ.TIME_FCT=t;
	SEQ.FctAtTime=fct;
}

void DefineEndOfGame(void (*fct)(void))
   {
   ActionFinMatch=fct;
   }

static void EndOfGame(void)
   {
   _Powen(0);
   EnableAsserv(0);
   if(ActionFinMatch) ActionFinMatch();
   while(1);
   }

void DefineFctDuringMovement(void (*fct)(void))
   {
   FctWaitDuringMovement=fct;
   }

void (*GetFctDuringMovement(void))(void)
   {
   return FctWaitDuringMovement;
   }

#define TTIMEOUT_DETECT 200
//Attente _DETECT inactif pendant plus de TTIMEOUT_DETECT ms...
//Ttimeout : numéro de tempo utilisée pour Timeout ou -1 si pas de timeout
//Renvoie 0 si arrêter ou 1 si continuer, et valeur "filtrée" de _DETECT dans *cus
static int WaitCaptusFree(int Ttimeout, int *cus)
   {
   int c;
   T_DETECT=TTIMEOUT_DETECT;
   while(1)
      {
	   if(RBT_TIME<=0) EndOfGame();
      c=GetDetect();
#ifdef __SIM__
      TestForExit();   //Pour simulateur
#endif
      if(cus) *cus=c;
      if(!c)
         {
         if(!T_DETECT) {break;}
         }
      else T_DETECT=TTIMEOUT_DETECT;
      if(Ttimeout>=0) {if(FinDelaiMs(Ttimeout)) return 0;}
//      DelaiMs(10);
      }
   return c;
   }



//Ex�cute un mouvement
//Renvoie 1 si normal ou 0 si demande de changement de séquence de profils
static int ExecMovement(void)
   {
   struct T_SEQ s;
   int cmde;
   int r;
   if(RBT_TIME<=0) EndOfGame();
   LAST_PX=GetP_X();
   LAST_PY=GetP_Y();
   r=RBT_EOK;
   s=SEQ;
   cmde=SEQ.Cmde;
#if DBG_SEQ
   //lcdprintf("Cmde %d (%lu)\n",cmde,SEQ.Nbp);
#endif
   switch(cmde)
      {
      case CMDE_AVi:
         DetectAV();
#if DBG_SEQ
         lcdprintf("DetectAV\n");
#endif
         break;
      case CMDE_ARi:
         DetectAR();
#if DBG_SEQ
         lcdprintf("DetectAR\n");
#endif
         break;
      case CMDE_PGi:
      case CMDE_PDi:
         DetectStop();
#if DBG_SEQ
         lcdprintf("DetectStop\n");
#endif
         break;
      default:
#if DBG_SEQ
         lcdprintf("US ???\n");
#endif
         break;
      }

   DelaiMs(2); //Attente prise en compte US

   SEQ.EtatProfil=P_INIT;          //exécute le mouvement
#ifndef __SIM__
   while(SEQ.EtatProfil==P_INIT);
#endif
#ifdef __SIM__
   SEQ.MouvFini=0;
#endif
   while((!SEQ.MouvFini)||(SEQ.MouvInterrompu))
      {
      if(RBT_TIME<=0) EndOfGame();
      DelaiMs(1);
      if(FctWaitDuringMovement) FctWaitDuringMovement();
#ifdef __SIM__
      SimMvmt();   //Pour simulateur de mouvements...
      DelaiMs(500);
#endif
      if(SEQ.MouvFini && SEQ.MouvInterrompu)     //Gestion mouvement interrompu
         {
#if DBG_SEQ
   lcdprintf("Mouvement interrompu [%.1fs]\n",0.1*TEMPS_RESTANT);
#endif
         // tester redémarrage possible ???
         if(SEQ._URG)
            {
             TickType_t t=CreateDelaiMs(SEQ.DelaiMsUrg);
            int cus;
            do
               {
               if(RBT_TIME<=0) { CloseDelaiMs(t); EndOfGame();}


#ifdef __SIM__
               TestForExit();   //Pour simulateur
               //printf("#"); DelaiMs(500);
#endif
                if(!WaitCaptusFree(t,&cus)) break;
                if(FctWaitDuringMovement) FctWaitDuringMovement();
                }
                while(cus);
            CloseDelaiMs(t);
            if(cus)
               {
                RestoreAutoAcquit(&s);
                SEQ=s;
                SEQ.EtatProfil=P_ARRET;
                SEQ.MouvInterrompu=0;
                SEQ.MouvFini=1;
                return r|RBT_EEMERGENCY;
                }
            }
         else
            {
            while(WaitCaptusFree(-1,0))
               {
               if(RBT_TIME<=0) EndOfGame();
               if(FctWaitDuringMovement) FctWaitDuringMovement();

#ifdef __SIM__
               TestForExit();   //Pour simulateur
               //printf("@");  DelaiMs(500);
#endif
               DelaiMs(100);
               }
            }
         //Redémarrage sur même profil :
//         static T_CMD cmd2;
//         cmd2=*(SEQ.CURRENT);
//         cmd2.p=SEQ.LRest;
//         SEQ.CURRENT=&cmd2;
         SEQ.Nbp=SEQ.LRest;
//            SEQ.MoveVmax=20000;   //Modification de la vitesse max si nécessaire
         SEQ.EtatProfil=P_INIT; //Redémarrage
         DelaiMs(2);  //Attente Lecture P_INIT par Asserv
#ifdef __SIM__
      SEQ.MouvFini=0;
#endif

        }
//      Aff_Tout();
      }

#if DBG_SEQ
   if(SEQ._HaltReq) lcdprintf("HaltReq\n");
#endif
    if(SEQ._HaltControlMovFct) {s._HaltControlMovFct=0; r|=RBT_EHALTCONTF;}
    if(SEQ._HaltReq) {s.HaltReq=0; r|=RBT_EHALTREQ;}
   RestoreAutoAcquit(&s);
   SEQ=s; //Restitution du profil initial

   return r;
   }

void SetBoardGuard(unsigned dist)
   {
   if(dist<500) SEQ.BoardGuard=dist;
   }

static void BoardGuard(int *xc, int *yc)
   {
   const float pi2=M_PI*2;
   int xn,xm,yn,ym;
   int typ;
   float tfa;
   xn=-SZX_TABLE/2+SEQ.BoardGuard; xm=SZX_TABLE/2-SEQ.BoardGuard;
   yn=0+SEQ.BoardGuard; ym=SZY_TABLE-SEQ.BoardGuard;
   if((*xc>=xn)&&(*xc<=xm)&&(*yc>=yn)&&(*yc<=ym)) return;
   float a=atan2f(*yc-P_Y,*xc-P_X);
   float a1=atan2f(ym-P_Y,xm-P_X);
   float a2=atan2f(ym-P_Y,xn-P_X);
   float a3=atan2f(yn-P_Y,xn-P_X);
   float a4=atan2f(yn-P_Y,xm-P_X);

   if(*xc>xm)
      {
      if(a<a4) typ=3;
      else if(a>a1) typ=1;
      else typ=0;
      }
   else if(*xc<xn)
      {
      if(a<0.f) a+=pi2;
      if(a2<0.f) a2+=pi2;
      if(a3<0.f) a3+=pi2;
      if(a<a2) typ=1;
      else if(a>a3) typ=3;
      else typ=2;
      }
   else if(*yc>ym)
      {
      if(a>a2) typ=2;
      else if(a<a1) typ=0;
      else typ=1;
      }
   else   //*yc<yn
      {
      if(a<a3) typ=2;
      else if(a>a4) typ=0;
      else typ=3;
      }
   tfa=tanf(a);
   //printf(" [%d] ",typ);
   switch(typ)
      {
   case 0:
      *xc=xm;
      *yc=P_Y+(xm-P_X)*tfa;
      break;
   case 1:
      *yc=ym;
      if(fabs(tfa)>1E-6f) *xc=P_X+(ym-P_Y)/tanf(a); else *xc=P_X;
      break;
   case 2:
      *xc=xn;
      *yc=P_Y+(xn-P_X)*tfa;
      break;
   case 3:
      *yc=yn;
      if(fabs(tfa)>1E-6f) *xc=P_X+(yn-P_Y)/tanf(a); else *xc=P_X;
      }
   }


extern int X_Cible, Y_Cible; // Coordonnées du point à atteindre

int Rbt_Move(int xc, int yc, int av)
   {
            int ex,ey,Ecart,Type;
            float ang;
            int p;
            int r;
            int (*MemHaltReq)(void);
            if(SEQ.INTERFCT)
               {
               if(SEQ.INTERFCT())
                  {
                  SEQ.INTERFCT=0;
                  return RBT_EINTFCTSTOP;
                  }
               }

            if(SEQ.BoardGuard) BoardGuard(&xc,&yc);
            X_Cible=xc;
            Y_Cible=yc;

            ex=xc-GetP_X();
            ey=yc-GetP_Y();
            if(ex||ey)
               {
               ang=atan2f(ey,ex); //Angle cible en rad entre -PI et PI
               ang=ang*(1800.f/PI);
               Ecart=GetTHETA()-ang;
               while(Ecart>=1800)  Ecart-=3600;
               while(Ecart<=-1800) Ecart+=3600;
               if(Ecart<-900) Type=3;
                  else if(Ecart<0) Type=4;
                     else if(Ecart<900) Type=1;
                        else Type=2;

               if(!av) Type=-Type;

               p=0;
               switch(Type)
                  {
                  case 1:
                  case 2:
                     p=Ecart; //Angle en deg/10
                     SEQ.Cmde=CMDE_PDi;
#if DBG_SEQ
   lcdprintf("PDi(%d)...",p);
#endif
                     break;
                  case 3:
                  case 4:
                     p=-Ecart;
                     SEQ.Cmde=CMDE_PGi;
#if DBG_SEQ
   lcdprintf("PGi(%d)...",p);
#endif
                     break;
                  case -1:
                  case -2:
                     p=1800-Ecart;
                     SEQ.Cmde=CMDE_PGi;
#if DBG_SEQ
   lcdprintf("PGi(%d)...",p);
#endif
                     break;
                  case -3:
                  case -4:
                     p=1800+Ecart;
                     SEQ.Cmde=CMDE_PDi;
#if DBG_SEQ
   lcdprintf("PDi(%d)...",p);
#endif
                     break;
                  }

               MemHaltReq=SEQ.HaltReq;
               if(SEQ.RINTERFCT) SEQ.RINTERFCT(SEQ.Cmde==CMDE_PDi);
               if(p)
                  {
                  SEQ.Nbp=_ConvAnglelpp(p);
                  if((r=ExecMovement())!=RBT_EOK) return r|RBT_EROTEXIT;
                  }
               ex=xc-GetP_X();
               ey=yc-GetP_Y();
               if(Type<0) SEQ.Cmde=CMDE_ARi;
               else SEQ.Cmde=CMDE_AVi;
               p=hypotf(ex,ey);
               SEQ.HaltReq=MemHaltReq;
#if DBG_SEQ
   if(Type<0) lcdprintf("ARi(%d)\n",p);
   else lcdprintf("AVi(%d)\n",p);
#endif
 //              lcdgotoxy(1,2); lcdprintf("Dist=%u\t",temp.P.iParam[0]); DelaiMs(3000);
               if(SEQ.MINTERFCT) SEQ.MINTERFCT(SEQ.Cmde==CMDE_AVi);
               if((SEQ.Stretch<0) && (p<-SEQ.Stretch)) p=0;
               else p+=SEQ.Stretch;
               SEQ.Stretch=0;
               if(p)
                  {
                  SEQ.Nbp=_ConvDistlpp(p);
                  if((r=ExecMovement())!=RBT_EOK) return r|RBT_EMOVEXIT;
                  }
               }
       //      lcdprintf(" %.3*1000Us\n",T_Trajet);
            return RBT_EOK;
            }

int Rbt_MoveX(int xc, int yc)
   {
   int ex,ey,ang,Ecart,Type;
   int p;
   int r;
   int (*MemHaltReq)(void);
   if(SEQ.INTERFCT)
      {
      if(SEQ.INTERFCT())
         {
         SEQ.INTERFCT=0;
         return RBT_EINTFCTSTOP;
         }
      }
   X_Cible=xc;
   Y_Cible=yc;
   ex=xc-GetP_X();
   ey=yc-GetP_Y();
   if(ex||ey)
      {
      ang=(1800.f/PI)*atan2f(ey,ex)+0.5f; //Angle cible en rad entre -PI et PI
      Ecart=ang-GetTHETA();
	   if(Ecart>=1800)  Ecart-=3600;
      if(Ecart<=-1800) Ecart+=3600;
	   if(Ecart>900)
	      {
         Ecart-=1800;
   	   if(Ecart<0) Type=1; else Type=-1;
   	   }
	      else if(Ecart<-900)
	         {
   	      Ecart+=1800;
   	      if(Ecart<0) Type=2; else Type=-2;
   	      }
            else  // -900 <= Ecart <= 900
               {
               if(Ecart<0) Type=3; else Type=-3;
               }
      if(Ecart<0) Ecart=-Ecart;
      if(Type<0) SEQ.Cmde=CMDE_PGi;
         else SEQ.Cmde=CMDE_PDi;
      MemHaltReq=SEQ.HaltReq;
      if(SEQ.RINTERFCT) SEQ.RINTERFCT(SEQ.Cmde==CMDE_PDi);
      if(Ecart)
         {
         SEQ.Nbp=_ConvAnglelpp(Ecart);
         if((r=ExecMovement())!=RBT_EOK) return r|RBT_EROTEXIT;
         }
      ex=xc-GetP_X();
      ey=yc-GetP_Y();
      if(Type<0) Type=-Type;
      if(Type==3) SEQ.Cmde=CMDE_AVi;
         else SEQ.Cmde=CMDE_ARi;
      p=hypotf(ex,ey);
      SEQ.HaltReq=MemHaltReq;
      if(SEQ.MINTERFCT) SEQ.MINTERFCT(SEQ.Cmde==CMDE_AVi);
      if((SEQ.Stretch<0) && (p<-SEQ.Stretch)) p=0;
      else p+=SEQ.Stretch;
      SEQ.Stretch=0;
      if(p)
         {
         SEQ.Nbp=_ConvDistlpp(p);
         if((r=ExecMovement())!=RBT_EOK) return r|RBT_EMOVEXIT;
         }
      }
   return RBT_EOK;
   }


int Rbt_RMove(int dl)
   {
   int r;
#if DBG_SEQ
   lcdprintf("Rbt_RMove(%d)\n",dl);
#endif
   if(SEQ.INTERFCT)
      {
      if(SEQ.INTERFCT())
         {
         SEQ.INTERFCT=0;
         return RBT_EINTFCTSTOP;
         }
      }
   if(dl>=0) SEQ.Cmde=CMDE_AVi; else {SEQ.Cmde=CMDE_ARi; dl=-dl;}
   if(SEQ.MINTERFCT) SEQ.MINTERFCT(SEQ.Cmde==CMDE_AVi);
   if((SEQ.Stretch<0) && (dl<-SEQ.Stretch)) dl=0;
   else dl+=SEQ.Stretch;
   SEQ.Stretch=0;

   if(dl)
      {
      SEQ.Nbp=_ConvDistlpp(dl);
      if((r=ExecMovement())!=RBT_EOK) return r|RBT_EMOVEXIT;
      }
   return RBT_EOK;
   }

int Rbt_RForward(unsigned dl)
   {
   return Rbt_RMove(dl);
   }

int Rbt_RBackward(unsigned dl)
   {
   return Rbt_RMove(-dl);
   }

int Rbt_RRotate(int da)
   {
   int r;
   if(SEQ.INTERFCT)
      {
      if(SEQ.INTERFCT())
         {
         SEQ.INTERFCT=0;
         return RBT_EINTFCTSTOP;
         }
      }
   if(da>=0) SEQ.Cmde=CMDE_PGi; else {SEQ.Cmde=CMDE_PDi; da=-da;}
   if(SEQ.MINTERFCT) SEQ.MINTERFCT(SEQ.Cmde==CMDE_PDi);

   if(da)
      {
      SEQ.Nbp=_ConvAnglelpp(da);
      if((r=ExecMovement())!=RBT_EOK) return r|RBT_EROTEXIT;
      }
   return RBT_EOK;
   }

int Rbt_RRotateLeft(unsigned da)
   {
   return Rbt_RRotate(da);
   }

int Rbt_RRotateRight(unsigned da)
   {
   return Rbt_RRotate(-da);
   }


int Rbt_AbsRotate(int ang)
   {
   int Ecart,Type,p;
   int r;
   p=0;
   X_Cible=GetP_X();
   Y_Cible=GetP_Y();
   if(SEQ.INTERFCT)
      {
      if(SEQ.INTERFCT())
         {
         SEQ.INTERFCT=0;
         return RBT_EINTFCTSTOP;
         }
      }
   Ecart=GetTHETA()-ang;
   if(Ecart)
      {
      while(Ecart>=1800)  Ecart-=3600;
      if(Ecart<=-1800) Ecart+=3600;
      if(Ecart<-900) Type=3;
         else if(Ecart<0) Type=4;
            else if(Ecart<900) Type=1;
               else Type=2;
      switch(Type)
         {
         case 1:
         case 2:
            p=Ecart; //Angle en deg/10
            SEQ.Cmde=CMDE_PDi;
            break;
         case 3:
         case 4:
            p=-Ecart;
            SEQ.Cmde=CMDE_PGi;
            break;
         case -1:
         case -2:
            p=1800-Ecart;
            SEQ.Cmde=CMDE_PGi;
            break;
         case -3:
         case -4:
            p=1800+Ecart;
            SEQ.Cmde=CMDE_PDi;
            break;
         }
      if(SEQ.RINTERFCT) SEQ.RINTERFCT(SEQ.Cmde==CMDE_PDi);
      if(p)
         {
         SEQ.Nbp=_ConvAnglelpp(p);
         if((r=ExecMovement())!=RBT_EOK) return r|RBT_EROTEXIT;
         }
      }
   return RBT_EOK;
   }

void Rbt_MovPrm(unsigned vmin, unsigned vmax, unsigned acc)
   {
   SEQ.MoveVmin=vmin;
   SEQ.MoveVmax=vmax;
   SEQ.MoveAcc=acc;
#ifdef __SIM__
   SimParamLin(vmax);
#endif
   }

void Rbt_RotPrm(unsigned vmin, unsigned vmax, unsigned acc)
   {
   SEQ.PivotVmin=vmin;
   SEQ.PivotVmax=vmax;
   SEQ.PivotAcc=acc;
#ifdef __SIM__
   SimParamRot(vmin);
#endif
   }

#define NBMAXPARAMSTACK 10
static unsigned NBPARAMSTACK;
static unsigned PARAMSTACK[NBMAXPARAMSTACK*3];

//Met en mémoire (pile LIFO) les paramètres de déplacement linéaires (si type=0), de rotation (si type=1) ou les deux  (si type=2)
//Renvoie 0 en cas d'erreur (plus de place ou type invalide)
int PushParam(int type)
   {
   if((type<0)||(type>2)) return 0;
   if(NBPARAMSTACK==NBMAXPARAMSTACK) return 0;
   switch(type)
      {
   case 0:
      PARAMSTACK[NBPARAMSTACK*3]=SEQ.MoveVmin;
      PARAMSTACK[NBPARAMSTACK*3+1]=SEQ.MoveVmax;
      PARAMSTACK[NBPARAMSTACK*3+2]=SEQ.MoveAcc;
      break;
   case 1:
      PARAMSTACK[NBPARAMSTACK*3]=SEQ.PivotVmin;
      PARAMSTACK[NBPARAMSTACK*3+1]=SEQ.PivotVmax;
      PARAMSTACK[NBPARAMSTACK*3+2]=SEQ.PivotAcc;
      break;
   case 2:
      if(NBPARAMSTACK>=NBMAXPARAMSTACK-1) return 0;
      PARAMSTACK[NBPARAMSTACK*3]=SEQ.MoveVmin;
      PARAMSTACK[NBPARAMSTACK*3+1]=SEQ.MoveVmax;
      PARAMSTACK[NBPARAMSTACK*3+2]=SEQ.MoveAcc;
      NBPARAMSTACK++;
      PARAMSTACK[NBPARAMSTACK*3]=SEQ.PivotVmin;
      PARAMSTACK[NBPARAMSTACK*3+1]=SEQ.PivotVmax;
      PARAMSTACK[NBPARAMSTACK*3+2]=SEQ.PivotAcc;
      break;
      }
   NBPARAMSTACK++;
   return 1;
   }

//Récupère de la pile les paramètres de déplacement linéaires (si type=0), de rotation (si type=1) ou les deux  (si type=2)
//Renvoie 0 en cas d'erreur (pile vide ou type invalide)
int PopParam(int type)
   {
   if((type<0)||(type>2)) return 0;
   if(!NBPARAMSTACK) return 0;
   NBPARAMSTACK--;
   switch(type)
      {
   case 0:
      SEQ.MoveVmin=PARAMSTACK[NBPARAMSTACK*3];
      SEQ.MoveVmax=PARAMSTACK[NBPARAMSTACK*3+1];
      SEQ.MoveAcc=PARAMSTACK[NBPARAMSTACK*3+2];
      break;
   case 1:
      SEQ.PivotVmin=PARAMSTACK[NBPARAMSTACK*3];
      SEQ.PivotVmax=PARAMSTACK[NBPARAMSTACK*3+1];
      SEQ.PivotAcc=PARAMSTACK[NBPARAMSTACK*3+2];
      break;
   case 2:
      if(!NBPARAMSTACK) return 0;
      SEQ.PivotVmin=PARAMSTACK[NBPARAMSTACK*3];
      SEQ.PivotVmax=PARAMSTACK[NBPARAMSTACK*3+1];
      SEQ.PivotAcc=PARAMSTACK[NBPARAMSTACK*3+2];
      NBPARAMSTACK--;
      SEQ.MoveVmin=PARAMSTACK[NBPARAMSTACK*3];
      SEQ.MoveVmax=PARAMSTACK[NBPARAMSTACK*3+1];
      SEQ.MoveAcc=PARAMSTACK[NBPARAMSTACK*3+2];
      break;
      }
#ifdef __SIM__
   SimParamMaj();
#endif
   return 1;
   }


void Rbt_ControlFct(int (*fct)(T_ControlPrm *))
   {
   SEQ._HaltControlMovFct=0;
   SEQ.ControlMovFct=fct;
   }

void Rbt_HaltReq(unsigned dec, int (*fct)(void))
   {
   SEQ.HaltReq=fct;
   SEQ.HaltReqDec=dec;
   }

void Rbt_DefInterFct(int (*fct)(void))
   {
   SEQ.INTERFCT=fct;
   }

void Rbt_DefRotInterFct(void (*fct)(int))
   {
   SEQ.RINTERFCT=fct;
   }

void Rbt_DefMovInterFct(void (*fct)(int))
   {
   SEQ.MINTERFCT=fct;
   }

void Rbt_MemoPos(void)
   {
   SEQ.MEMO_PX=GetP_X();
   SEQ.MEMO_PY=GetP_Y();
   SEQ.MEMO_THETA=GetTHETA();
   }

void Rbt_AssStop(int en)
   {
   DelaiMs(2);
   SEQ._AssStop=(en!=0);
   DelaiMs(2);
   }

void Rbt_CurveM(int c)
   {
   SEQ.TrajC=c;
   }

void Rbt_Stretch(int dl)
   {
   SEQ.Stretch=dl;
   }

void Rbt_Emergency(unsigned dec, unsigned ms)
   {
   if(dec) SEQ.UrgDec=dec;
   if(ms) {SEQ.DelaiMsUrg=ms; SEQ._URG=1;}
   else SEQ._URG=0;
   }

void SpeedLimitNear(unsigned vmax, unsigned dnear, unsigned ddetect)
   {
   if((!vmax) || (ddetect<=dnear)) {SEQ._ModulVmax=0; return;}
   SEQ.Vnear=vmax;
   SEQ.Ddectect=ddetect;
   SEQ.Dnear=dnear;
   SEQ._ModulVmax=1;
   }


int _AvanceVers(int x, int y) {return AvanceVers(x,y);}

int _ReculeVers(int x, int y) {return ReculeVers(x,y);}

int _VaVers(int x, int y) {return VaVers(x,y);}




