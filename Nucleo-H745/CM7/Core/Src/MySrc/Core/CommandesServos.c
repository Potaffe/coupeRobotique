/*******************************************************************************
 Pilotage des servos
 Utilisation FreeRTOS (06/2019)
 *******************************************************************************/

#include "CommandesServos.h"
#include "TempsMs.h"
#include "../../../../../Common/Src/LCD_VT100.h"
#include "../App/Config.h"


//#define AFF 1
#define SVDBG 0


static uint16_t MODE_RT=0; //Indique mode RT (FreeRTOS) utilisé ou non
void ServosSetModeRT(uint16_t m)
    {
    MODE_RT=m;
    }

uint16_t ServosGetModeRT(void)
    {
    return MODE_RT;
    }

// détecte les servos AX et RX
// et les affiche
void DetectServos(void)
   {
   const unsigned smax=253;
   unsigned i;
   //lcdclrscr();
   lcdprintf("Detection servos...\r\n");
   for(i=1; i<=smax; i++)
      {
      i=GetNextServo(i,smax);
      if(i<=smax)
         {
         if(i&(RXID(0))) lcdprintf("RX:%u,",i&(RXID(0)-1));
         else lcdprintf("AX:%u,",i);
         }
      }
   lcdprintf("\r\nFIN\r\n");
   ClearServoErr();
   }

typedef struct
{
      uint16_t id;
      const char *name;
}T_ServoDefs;

static T_ServoDefs ServoDefs[40];
static uint16_t NB_ServoDefs;

void InitSvNames(void)
   {
   uint16_t i;
   NB_ServoDefs=0;
   for(i=0; i<sizeof(ServoDefs)/sizeof(*ServoDefs); i++)
      {
      ServoDefs[i].id=0;
      ServoDefs[i].name=0;
      }
   }

void SetSvName(uint16_t id, const char *name)
   {
   uint16_t i;
   for(i=0; i<NB_ServoDefs; i++)
      {
      if(id==ServoDefs[i].id) {ServoDefs[i].name=name; return;}
      }
   if(NB_ServoDefs>=sizeof(ServoDefs)/sizeof(*ServoDefs)) return;
   ServoDefs[NB_ServoDefs].id=id;
   ServoDefs[NB_ServoDefs].name=name;
   NB_ServoDefs++;
   }

const char *GetSvName(uint16_t id)
{
      uint16_t i;
      for(i=0; i<NB_ServoDefs; i++)
      {
            if(id==ServoDefs[i].id) return ServoDefs[i].name;
      }
      return 0;
}


static int LocServoMove(unsigned id, unsigned pos)
   {
   int r,i;
   for(i=0; i<5; i++)
      {
      EnableServoTorque(id,1);
      r=GetServoConsTorque(id);
      if(r>=0) SetServoMaxTorque(id,r);
      r=SetServoAngle(id,pos)!=0;
      if(!r) break;
      }
   return r;
   }


static volatile int16_t _StopRequest, _StopPending;
static int StopRequest(unsigned dt)
    {
    int r;
    if(MODE_RT)
       {
       r=ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(dt));
       if(r)
          {
          _StopPending=1;
          //_LED(1);
          }
       return r;
       }
    return _StopRequest;
    }


static int Err(int x1, int x2)
{
   int r;
   r=x1-x2;
   if(r<0) r=-r;
   return r;
}

//Renvoie 1 si Ok
//Renvoie 0 si position non Ok
//Renvoie -1 si _StopRequest
static uint16_t ERR_MAX=10;

//Définition de l'erreur maximale admissible pour les mouvements avec attente
void DefineErrMax(uint16_t err)
{
ERR_MAX=err;
}

static int _ServoMoveWait(unsigned id, unsigned pos)
   {
   uint16_t *px;
   int i,r, t;
   for(i=0; i<5; i++)
      {
      if(!SetServoAngle(id,pos)) break;
      }
   t=50;
   do
      {
      if(!t) break;
      px=GetServoAngle(id);
      if(!px) return 0;
      if(Err(*px,pos)<ERR_MAX) break;
      r=GetServoMoving(id);
      if(r<0) return 0;
      if(StopRequest(10)) return -1;
      DelaiMs(1);
      t--;
      }
   while(r==0); //Attente mise en rotation
   t=200;
   do
      {
      if(!t) break;
      r=GetServoMoving(id);
      if(r<0) return 0;
      if(StopRequest(10)) return -1;
      DelaiMs(1);
      t--;
      }
   while(r); //Continuer tant que rotation
   px=GetServoAngle(id);
   if(!px) return 0;
   if(Err(*px,pos)<ERR_MAX) return 1;
   return 0;
   }

static int LocServoMoveWait(unsigned id, unsigned pos)
   {
   int r;
   EnableServoTorque(id,1);
   r=_ServoMoveWait(id,pos);
   if(r) return r;
   EnableServoTorque(id,1);
   r=GetServoConsTorque(id);
   if(r>=0) SetServoMaxTorque(id,r);
   return _ServoMoveWait(id,pos);
   }

void DServoMove(unsigned id1, unsigned pos1, unsigned id2, unsigned pos2)
   {
   int r;
   r=GetServoConsTorque(id1);
   if(r>=0) SetServoMaxTorque(id1,r);
   r=GetServoConsTorque(id2);
   if(r>=0) SetServoMaxTorque(id2,r);
   ServoRegMode(1);
   SetServoAngle(id1,pos1);
   ServoRegMode(1);
   SetServoAngle(id2,pos2);
   ServoAction();
   }

#define NB_RETRY 5
#define TMS_RETRY 5

static unsigned char *SetServoAngleRetryM1(unsigned id, unsigned angle)
   {
   unsigned char *px;
   ServoRegMode(1);
   px=SetServoAngle(id,angle);
   if(!px)
      {
      unsigned j;
      for(j=0; j<NB_RETRY; j++)
         {
         DelaiMs(TMS_RETRY);
         ServoRegMode(1);
         px=SetServoAngle(id,angle);
         if(px) break;
         }
      }
   return px;
   }

static uint16_t *GetServoAngleRetry(unsigned id)
   {
   uint16_t *px;
   px=GetServoAngle(id);
   if(!px)
      {
      unsigned j;
      for(j=0; j<NB_RETRY; j++)
         {
         DelaiMs(TMS_RETRY);
         px=GetServoAngle(id);
         if(px) break;
         }
      }
   return px;
   }

static int GetServoMovingRetry(unsigned id)
   {
   int r;
   r=GetServoMoving(id);
   if(r<0)
      {
      unsigned j;
      for(j=0; j<NB_RETRY; j++)
         {
         DelaiMs(TMS_RETRY);
         r=GetServoMoving(id);
         if(r>=0) break;
         }
      if(j==NB_RETRY) return r;
      }
   return r;
   }

static int GServoMoveNoWait(const T_GROUPE *s, unsigned nb)
   {
   unsigned char *px;
   unsigned i;
   for(i=0; i<nb; i++)
      {
      //ServoRegMode(1);
      if(StopRequest(0)) return -100;  //Demande abandon mouvement
      px=SetServoAngleRetryM1(s[i].id,s[i].pos);
      if(!px)
         {/*lcdprintf("#1# ");*/
         return -1;
         }
      }
   ServoAction();
   return 0;
   }

static int _GServoMoveWait(const T_GROUPE *s, unsigned nb)
   {
   unsigned char *px;
   uint16_t *pxw;
   int r, e;
   unsigned i, c;
   TickType_t t;
   if(SVDBG) lcdputc('.');
   for(i=0; i<nb; i++)
      {
      //ServoRegMode(1);
      if(StopRequest(0)) return -100;  //Demande abandon mouvement
      px=SetServoAngleRetryM1(s[i].id,s[i].pos);
      if(!px)
         {/*lcdprintf("#1# ");*/
         return -1;
         }
      }
   if(SVDBG) lcdputc('#');
   ServoAction();
   t=xTaskGetTickCount()+(500/portTICK_PERIOD_MS);  //CreateDelaiMs(500);
   do
      {
      c=r=0;
      for(i=0; i<nb; i++)
         {
         if(StopRequest(0)) return -100;  //Demande abandon mouvement
         pxw=GetServoAngleRetry(s[i].id);
         if(!pxw)
            {/*lcdprintf("#2# ");*/
            return -1;
            }
         e=*pxw-s[i].pos;
         if(e<0) e=-e;
         if(((unsigned)e<=s[i].err)||(!s[i].err)) c++;
         if(c==nb) break;  //Tous servos � bonne position
         if(!i) vTaskDelay(10/portTICK_PERIOD_MS);  //DelaiMs(10);
         e=GetServoMovingRetry(s[i].id);
         if(e<0)
            {
            return -2;
            }
         r+=(e!=0);  //Comptage servos en mouvement
         }
      if(xTaskGetTickCount()>=t) break;  //if(FinDelaiMs(t)) break;
      }
   while(r==0);  // Continuer tant que servos ne bougent pas
   //CloseDelaiMs(t);
   if(SVDBG) lcdputc('&');
   t=xTaskGetTickCount()+(3000/portTICK_PERIOD_MS);  //t=CreateDelaiMs(3000);
   do
      {
      r=0;
      for(i=0; i<nb; i++)
         {
         if(StopRequest(0)) return -100;  //Demande abandon mouvement
         e=GetServoMovingRetry(s[i].id);
         if(e<0)
            {
            return -2;
            }
         r+=(e==0);
         }
      if(xTaskGetTickCount()>=t) break;  //if(FinDelaiMs(t)) break;
      }
   while((unsigned)r<nb);  //Continuer tant que mouvement en cours
   //CloseDelaiMs(t);
   if(SVDBG) lcdputc('@');
   c=0;
   for(i=0; i<nb; i++)
      {
      if(StopRequest(0)) return -100;  //Demande abandon mouvement
      pxw=GetServoAngleRetry(s[i].id);
      if(!pxw)
         {/*lcdprintf("#3# ")*/
         return -1;
         }
      e=*pxw-s[i].pos;
      if(e<0) e=-e;
      if(((unsigned)e<=s[i].err)||(!s[i].err))
         {
         c++;
         //lcdprintf("%u Ok\n",s[i].id);
         }
      }
   if(SVDBG) lcdputc('\\');
   if(c==nb) return 0;  //Tous servos à bonne position
   return c;  //c servo(s) mal positionné(s)
   }

static int GServoMoveWait(const T_GROUPE *s, unsigned nb)
   {
   int r;
   unsigned i;
   r=_GServoMoveWait(s,nb);
   if(r<=0) return r;
   for(i=0; i<nb; i++)
      {
      EnableServoTorque(s[i].id,1);
      r=GetServoConsTorque(s[i].id);
      if(r>=0) SetServoMaxTorque(s[i].id,r);
      }
   return _GServoMoveWait(s,nb);
   }

int DServoMoveWait(unsigned id1, unsigned pos1, unsigned id2, unsigned pos2)
   {
   T_GROUPE s[2];
   s[0].id=id1;
   s[1].id=id2;
   s[0].pos=pos1;
   s[1].pos=pos2;
   s[0].err=2;
   s[1].err=2;
   return GServoMoveWait(s,2)==0;
   }

static const int16_t *GCMD[NBGCMDMAX];
static void (*GFCT[NBGFCTMAX])(void);
void InitGCMD(void)
   {
   unsigned i;
   for(i=0; i<NBGCMDMAX; i++) GCMD[i]=0;
   for(i=0; i<NBGFCTMAX; i++) GFCT[i]=0;
   }


// Le tableau gcmd doit être statique !!!
static int LocNewGCMD(uint16_t n, const int16_t *gcmd)
   {
   if(n>=NBGCMDMAX) return 1;
   GCMD[n]=gcmd;
   return 0;  //Création Ok
   }

static int LocNewGFCT(uint16_t n, void (*fct)(void))
   {
   if(n>=NBGFCTMAX) return 1;
   GFCT[n]=fct;
   return 0;  //Création Ok
   }


void AffGCMD(uint16_t n)
   {
   unsigned nb, i, j;
   const int16_t *gcmd;
   int16_t id;
   int ModeWait;
   ModeWait=1;
   lcdprintf("Groupe de commande %d\n",n);
   if(n>=DIM(GCMD))
      {
      lcdprintf("Groupe %d invalide\n",n);
      return;
      }
   if(!GCMD[n])
      {
      lcdprintf("Groupe %d non défini\n",n);
      return;
      }
   gcmd=GCMD[n];
   nb=*(gcmd++);
   if(nb>NBGCMDSVMAX)
      {
      lcdprintf("Nombre de servos (%d) invalide\n",nb);
      return;
      }
   for(i=0; i<nb; i++)
      {
      id=*(gcmd++);
//      err=*(gcmd++);
      lcdprintf("Servo %d\n",id);
      }
   j=1;
   while(1)
      {
      if(*gcmd==GC_FIN) break;
      if(*gcmd==GC_MODEWAIT)
         {
         gcmd++;
         ModeWait=*(gcmd++);
         lcdprintf("Mode %s\n",ModeWait?"WAIT":"NOWAIT");
         continue;
         }
      if(*gcmd==GC_SUB)  //Sous-Commande
         {
         gcmd++;
         i=*(gcmd++);
         if(i!=n) lcdprintf("Execution ss-groupe %d\n",i);
         continue;
         }
      if(*gcmd==GC_DELAI)  //Tempo
         {
         gcmd++;
         i=*(gcmd++);
         lcdprintf("Temporisation %u ms\n",i);
         continue;
         }
      if(*gcmd==GC_ERR)  //Erreurs
         {
         gcmd++;
         lcdprintf("Erreurs : ");
         for(i=0; i<nb; i++)
            lcdprintf("%u ",*(gcmd++));
         lcdprintf("\n");
         continue;
         }
      if(*gcmd==GC_VIT)  //Vitesses
         {
         gcmd++;
         lcdprintf("Vitesses : ");
         for(i=0; i<nb; i++)
            lcdprintf("%u ",*(gcmd++));
         lcdprintf("\n");
         continue;
         }
      if(*gcmd==GC_TORQUE)
         {
         gcmd++;
         lcdprintf("Couples : ");
         for(i=0; i<nb; i++)
            lcdprintf("%u ",*(gcmd++));
         lcdprintf("\n");
         continue;
         }
      if(*gcmd==GC_MARGIN)
         {
         gcmd++;
         lcdprintf("Marges : ");
         for(i=0; i<nb; i++)
            lcdprintf("%u ",*(gcmd++));
         lcdprintf("\n");
         continue;
         }
      if(*gcmd==GC_SLOPE)
         {
         gcmd++;
         lcdprintf("Pentes : ");
         for(i=0; i<nb; i++)
            lcdprintf("%u ",*(gcmd++));
         lcdprintf("\n");
         continue;
         }
      if(*gcmd==GC_TEST)  //Passage en mode TEST des servos (aff positions)
         {
         gcmd++;
         lcdprintf("Affichage positions\n");
         continue;
         }
      if(*gcmd==GC_ADJUST)  //Ajustement des positions des servos sur les consignes
         {
         gcmd++;
         lcdprintf("Ajustement positions\n");
         continue;
         }
      if(*gcmd==GC_FCT)
      {
          gcmd++;
          lcdprintf("Appel fonction %u\n",*(gcmd++));
          continue;
      }
      if(*gcmd==GC_AFFCP)  //Affichage consignes/positions
         {
         gcmd++;
         lcdprintf("Affichage consignes/positions\n");
         continue;
         }
      lcdprintf("Mouvement %u : [%s] ",j++,ModeWait?"WAIT":"NOWAIT");
      for(i=0; i<nb; i++)
         lcdprintf("%d ",*(gcmd++));
      lcdprintf("\n");
      }
   lcdprintf("----------------\n");
   }

void AffAllGCMD(void)
   {
   unsigned i;
   lcdprintf("\n++++++++++++\nGroupes :\n");
   for(i=0; i<DIM(GCMD); i++)
      {
      if(GCMD[i]) AffGCMD(i);
      }
   lcdprintf("++++++++++++\n");
   }

static int _GetServoAngle(unsigned id)
   {
   uint16_t *px;
   px=GetServoAngle(id);
   if(!px) return -1;
   return *px;
   }

static void AdjustServoPos(unsigned id)
   {
   int p, pos;
   pos=GetServoCons(id);
   if(pos==-1) return;
   p=_GetServoAngle(id);
   if(p==-1) return;
   while(_GetServoAngle(id)<pos)
      LocServoMoveWait(id,++p);
   while(_GetServoAngle(id)>pos)
      LocServoMoveWait(id,--p);
//   SetServoCons(id,pos);
   }

//Exécute une suite de commandes simultanées décrite dans le tableau transmis
//gcmd[0] : Nombre de servos (NBGCMDSVMAX maxi)
//gcmd[1] : id du servo 1
//gcmd[2] : id du servo 2
//...
//gcmd[n] : id du servo n
//*** les gcmd[] suivants sont les commandes de positionnement et de contrôle :
//(pos1,pos2,...,posn) : ensemble de positions 1
//(pos1,pos2,...,posn) : ensemble de positions 2
//...
//(pos1,pos2,...,posn) : ensemble de positions i
//*** contrôles possibles :
//GC_FIN : fin de la liste de commandes
//GC_SUB : exécution d'un groupe de commande spécifié (GC_SUB,Num groupe de 0 à NBGCMDMAX-1) Récursivité interdite !!!
//GC_DELAI : temporisation en ms (GC_DELAI, durée tempo de 1 à 65534)

int GMouvementGr(const int16_t *gcmd)
{
	   static T_GROUPE p[NBGCMDSVMAX];  //servo max en commande simultanée
	   uint16_t i, v;
	   uint16_t nb;
	   const int16_t *gcmd0;
	   int ModeWait;
	   ModeWait=1;

	   gcmd0=gcmd;

	   nb=*(gcmd++);
	   if(nb>DIM(p)) return 0;
	   for(i=0; i<nb; i++)
	      {
	      p[i].id=*(gcmd++);
	      p[i].err= G_ERR_DEF;
	      }
	/*
	   lcdprintf("%d Servos : ",nb);
	   for(i=0; i<nb; i++)
	       {
	       lcdprintf("%u ",p[i].id);
	       }

	   while(1)
		{
		static unsigned ctn;
		lcdgotoxy(1,3); lcdprintf("%u\t",ctn++);
		}

	*/


	while(1)
	      {
	      if(SVDBG) lcdprintf("Gcmde : %d\n",*gcmd);
	      if(*gcmd==GC_FIN) break;
	      if(*gcmd==GC_MODEWAIT)
	         {
	         gcmd++;
	         ModeWait=*(gcmd++);
	         continue;
	         }
	      if(*gcmd==GC_SUB)  //Sous-Commande
	         {
	    	 const int16_t *gcmdc;
	    	 int r;
	         gcmd++;
	         i=*(gcmd++);
	         if(i<DIM(GCMD)) gcmdc=GCMD[i];
	         else return 0;
	         if(!gcmdc) return 0;

	         if(gcmdc!=gcmd0) if((r=GMouvementGr(gcmdc))<0) return r;
	         continue;
	         }
	      if(*gcmd==GC_DELAI)  //Tempo
	         {
	         gcmd++;
	         i=*(gcmd++);
	         vTaskDelay(i/portTICK_PERIOD_MS);
	         //DelaiMs(i);
	         continue;
	         }
	      if(*gcmd==GC_ERR)  //Règlage erreurs
	         {
	         gcmd++;
	         for(i=0; i<nb; i++)
	            {
	            p[i].err=*(gcmd++);
	            }
	         continue;
	         }
	      if(*gcmd==GC_VIT)  //Règlage vitesses
	         {
	         gcmd++;
	         for(i=0; i<nb; i++)
	            {
	            SetServoSpeed(p[i].id,*(gcmd++));
	            }
	         continue;
	         }
	      if(*gcmd==GC_TORQUE)  //Règlage couples
	         {
	         gcmd++;
	         for(i=0; i<nb; i++)
	            {
	            SetServoMaxTorque(p[i].id,*(gcmd++));
	            }
	         continue;
	         }
	      if(*gcmd==GC_MARGIN)  //Règlage marges
	         {
	         gcmd++;
	         for(i=0; i<nb; i++)
	            {
	            v=*(gcmd++);
	            SetServoMargin(p[i].id,v,v);
	            }
	         continue;
	         }
	      if(*gcmd==GC_SLOPE)  //Règlage pentes
	         {
	         gcmd++;
	         for(i=0; i<nb; i++)
	            {
	            v=*(gcmd++);
	            SetServoSlope(p[i].id,v,v);
	            }
	         continue;
	         }
	      if(*gcmd==GC_ADJUST)  //ajustements positions
	         {
	         gcmd++;
	         for(i=0; i<nb; i++)
	            {
	            AdjustServoPos(p[i].id);
	            }
	         continue;
	         }
	      if(*gcmd==GC_FCT)
	         {
	    	 gcmd++;
	    	 v=*(gcmd++);
	    	 if(v>=NBGFCTMAX) return 0;
	    	 if(GFCT[v]) GFCT[v]();
	    	 continue;
	         }
	      if(*gcmd==GC_AFFCP)  //affichage consignes/positions
	         {
	         unsigned n=0;
	         gcmd++;
	         VTCtrl(1);
	         lcdclrscr();
	         /*
	          lcdgotoxy(1,1);
	          for(i=0; i<nb; i++)
	          {
	          if(p[i].id &(RXID(0))) lcdprintf("RX %u : ",p[i].id&(RXID(0)-1));
	          else lcdprintf("AX %u : ",p[i].id);
	          lcdprintf("Cons=%d\t\n",GetServoCons(p[i].id));
	          }
	          */
	         while(1)  //(!ETAT.Break)
	            {
	            uint16_t *pr;
	            int px;
	            const char *e;
	            lcdgotoxy(1,10);
	            for(i=0; i<nb; i++)
	               {
	               if(p[i].id&(RXID(0))) lcdprintf("RX %u : ",p[i].id&(RXID(0)-1));
	               else
	                  lcdprintf("AX %u : ",p[i].id);
	               pr=GetServoAngle(p[i].id);
	               if(pr) px=*pr;
	               else
	                  px=-1;
	               e=GetSvErrorMsg();
	               if(*e) lcdprintf("%s\t\n",e);
	               else
	                  lcdprintf("Pos=%d Cons=%d\t\n",px,GetServoCons(p[i].id));
	               }
	            lcdprintf("%u\t",n++);
	            }
	         continue;
	         }

	      if(*gcmd==GC_TEST)  //Passage en mode TEST des servos (aff positions)
	         {
	         unsigned n=0;
	         gcmd++;
	         VTCtrl(1);
	         lcdclrscr();

	         lcdprintf("%d Servos : ",nb);
	         for(i=0; i<nb; i++)
	             {
	            EnableServoTorque(p[i].id,0);  //SetServoMaxTorque(p[i].id,0);
	            lcdprintf("%u ",p[i].id);
	             }
	         while(1)  //(!ETAT.Break)
	            {
	            uint16_t *pr;
	            int px;
	            const char *e;
	            //static unsigned ctn=0;
	            lcdgotoxy(1,4);
	            for(i=0; i<nb; i++)
	               {
	               if(p[i].id&(RXID(0)))
	            	   {
            		   const char *nsv;
            		   nsv=GetSvName(p[i].id);
            		   if(nsv) lcdprintf("[%s] ",nsv);
	            	   lcdprintf("RX %u : ",p[i].id&(RXID(0)-1));
	            	   }
	               else
	               {
           		   const char *nsv;
           		   nsv=GetSvName(p[i].id);
           		   if(nsv) lcdprintf("[%s] ",nsv);
                     lcdprintf("AX %u : ",p[i].id);
	               }
	               pr=GetServoAngle(p[i].id);
	               if(pr) px=*pr;
	               else px=-1;
	               e=GetSvErrorMsg();
	               if(*e) lcdprintf("%s\t\n",e);
	               else lcdprintf("[%d] Ok\t\n",px);
	               }
	            lcdprintf("%u\t",n++);
	            }
	//         for(i=0; i<nb; i++) EnableServoTorque(p[i].id,1);
	         continue;
	         }

	      for(i=0; i<nb; i++)
	         p[i].pos=*(gcmd++);
	      if(SVDBG)
	         {
	         lcdprintf("Mouvements : ");
	         for(i=0; i<nb; i++)
	            lcdprintf("%d[%d] ",p[i].id,p[i].pos);
	         lcdprintf("\n");
	         }
	      if(ModeWait) v=GServoMoveWait(p,nb);
	      else v=GServoMoveNoWait(p,nb);
	      if(SVDBG) lcdprintf("Ok\n");
	      if(v)
	         {
	         if(SVDBG) lcdprintf("Err Pos %d svG [%s]\n",v,GetSvErrorMsg());
	         if(v==-100) return -1;  //Arrêt demandé
	         }
	      }
	   return 1;
}


static int LocGMouvement(uint16_t n)
   {
   const int16_t *gcmd;
//   lcdprintf("Mvmnt %u\n",n);
   if(n<DIM(GCMD)) gcmd=GCMD[n];
   else return 0;
   if(!gcmd) return 0;
   return GMouvementGr(gcmd);
   }

// renomme le servo RX24-IDorg en RX24-IDdest
// positionne le servo en position médiane
void RXInit(int idorg, int iddest)
   {
   ServoId(RXID(idorg),iddest);
   iddest=RXID(iddest);
   EnableServoTorque(iddest,1);
   SetServoMaxTorque(iddest,1000);
   SetServoSlope(iddest,32,32);
   SetServoAngleLimits(iddest,0,1023);
   SetServoSpeed(iddest,1023);
   SetServoMargin(iddest,0,0);
   LocServoMoveWait(iddest,512);
   }

// renomme le servAX12-IDorg en AX12-IDdest
// positionne le servo en position médiane
void AXInit(int idorg, int iddest)
   {
   ServoId(AXID(idorg),iddest);
   iddest=AXID(iddest);
   EnableServoTorque(iddest,1);
   SetServoMaxTorque(iddest,1000);
   SetServoSlope(iddest,32,32);
   SetServoAngleLimits(iddest,0,1023);
   SetServoSpeed(iddest,1023);
   SetServoMargin(iddest,0,0);
   LocServoMoveWait(iddest,512);
   }

static void LocServoInitStd(int id, unsigned slope, unsigned vit, unsigned torque, unsigned marg)
   {
   EnableServoTorque(id,1);
   SetServoMaxTorque(id,torque); //0 à 1023
   SetServoSlope(id,slope,slope); // 1 à 254
   SetServoAngleLimits(id,0,1023);
   SetServoSpeed(id,vit); //1 à 1023
   SetServoMargin(id,marg,marg); //1 à 254
   }

/*
 void DServoInitStd(int id1, int id2, unsigned slope, unsigned vit, unsigned torque,unsigned marg)
 {
 SetServoMaxTorqueDouble(id1,id2,torque);
 SetServoSlopeDouble(id1,id2,slope,slope);
 SetServoAngleLimits(id1,0,1023);
 SetServoAngleLimits(id2,0,1023);
 SetServoSpeedDouble(id1,id2,vit);
 SetServoMarginDouble(id1,id2,marg,marg);
 EnableServoTorque(id1,1);
 EnableServoTorque(id2,1);
 }
 */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

enum
   {
   QSInit,  //ServoInitStd(int id, unsigned slope, unsigned vit, unsigned torque,unsigned marg)
   QSMove,  //ServoMove(unsigned id, unsigned pos)
   QSMoveW,  //ServoMoveWait(unsigned id, unsigned pos)
//   QSNewGCMD,  //Définition d'un groupe de commandes de servos int NewGCMD(unsigned n, const int *gcmd)
//   QSNewGFCT,
   QSGMouvement,  //Exécute un groupe de commande int GMouvement(uint16_t n)
   QSSync,  //se met en attente tant que fonction synchro ne renvoie pas 1.
   QSFPrm, //Appel d'une fonction avec paramètres de type int16_t
   };

TaskHandle_t hServos;

static volatile int16_t CTMove, CTMoveW, CTGMouvement, CTSync, CTFPrm;



typedef struct
   {
      uint16_t cmde;  //Commande transmise
      int16_t prm[5];  //paramètres de la commande
      union
         {
            const int16_t *vprm;  //Pour commande à grand nombre de paramètres ou nombre variable. prm[0] contient le nombre de paramètres
            int (*fsync)(void);  //Fonction de synchronisation
            void (*fct)(void); //Fonction de groupe de servos
            void (*fprm)(uint16_t nbprm, const int16_t *prm); //Fonction à paramètres contenus dans prm[]
         } u;
   } T_QServos;
QueueHandle_t QServos;

static void Reset_RTServos(void)
   {
   xQueueReset(QServos);
   CTMove=0;
   CTMoveW=0;
   CTGMouvement=0;
   CTSync=0;
   }


#include <stdint.h>

volatile unsigned ServoReady=0;

int GetServoReady(void)
   {
   return ServoReady;
   }

#define NB_QUEUE 50

void RT_Servos(void *pvParameters)
   {
   T_QServos x;
   static uint8_t QSpace[NB_QUEUE*sizeof(T_QServos)];
   static StaticQueue_t xStaticQueue;
   const uint16_t ms2wait=50;
   int r, stp;
   MODE_RT=1;
   ServoInit();

   QServos=xQueueCreateStatic(NB_QUEUE,sizeof(T_QServos),QSpace,&xStaticQueue);
   ServoReady=1;
   while(1)
      {
      _StopPending=0;
      stp=0;
      if(xQueueReceive(QServos, &x, ms2wait/portTICK_PERIOD_MS)==pdTRUE)
         {
         switch(x.cmde)
            {
            case QSInit:
               LocServoInitStd(x.prm[0],x.prm[1],x.prm[2],x.prm[3],x.prm[4]);
               break;
            case QSMove:
               LocServoMove(x.prm[0],x.prm[1]);
               CTMove--;
               break;
            case QSMoveW:
               r=LocServoMoveWait(x.prm[0],x.prm[1]);
               CTMoveW--;
               if(r==-1) stp=1;
               break;
/*
            case QSNewGCMD:
               LocNewGCMD(x.prm[0],x.u.vprm);
               break;
*/
/*
            case QSNewGFCT:
               LocNewGFCT(x.prm[0],x.u.fct);
               break;
*/
            case QSGMouvement:
               r=LocGMouvement(x.prm[0]);
               CTGMouvement--;
               if(r==-1) stp=1;
               break;
            case QSSync:
               if(!x.u.fsync) break;
               while(!x.u.fsync())
                  {
                  if(ulTaskNotifyTake(pdTRUE,0))
                     {
                     stp=1;
                     break;
                     }
                  }
               CTSync--;
               break;
            case QSFPrm:
               if(!x.u.fprm) break;
               x.u.fprm(x.prm[0],x.prm+1);
               CTFPrm--;
               break;

            }
         }
      if(stp||_StopPending)
         {
         Reset_RTServos();
         _StopRequest=0;
         _StopPending=0;
         }
      if(ulTaskNotifyTake(pdTRUE,0))
         {
         Reset_RTServos();
         _StopRequest=0;
         }
      if(_StopRequest)
         {
          Reset_RTServos();
          _StopRequest=0;
         }
      }

   }

//Arrêt des actions en cours sur servos et annulation des commandes en attente
void ServoStopRequest(void)
   {
	if((!CTMove) && (!CTMoveW) && (!CTGMouvement) && (!CTSync) && (!CTFPrm)) return;
   _StopRequest=1;
   xTaskNotifyGive(hServos);
   while(_StopRequest);
   }

static const uint16_t QSendServosWait=10/portTICK_PERIOD_MS;

/*
 * @parm id : id du servo
 * @parm slope : pente de 1 à 254
 * @parm vit : vitesse de 1 à 1023
 * @parm torque : couple de 1 à 1023
 * @parm marge : marge sur la position de 1 à 254
 */
int ServoInitStd(int id, unsigned slope, unsigned vit, unsigned torque, unsigned marg)
   {
    if(!MODE_RT) { LocServoInitStd(id,slope,vit,torque,marg); return 1; }
   T_QServos x;
   x.cmde=QSInit;
   x.prm[0]=id;
   x.prm[1]=slope;
   x.prm[2]=vit;
   x.prm[3]=torque;
   x.prm[4]=marg;
   return xQueueSend(QServos,&x,QSendServosWait)==pdTRUE;
   }

//Lance le mouvement sans attendre
int ServoMove(unsigned id, unsigned pos)
   {
    if(!MODE_RT) return LocServoMove(id,pos);
   T_QServos x;
   int r;
   x.cmde=QSMove;
   x.prm[0]=id;
   x.prm[1]=pos;
   r= xQueueSend(QServos,&x,QSendServosWait)==pdTRUE;
   if(r) CTMove++;
   return r;
   }

//Lance le mouvement et attend la fin de celui-ci pour effectuer la prochaine commande
int ServoMoveW(unsigned id, unsigned pos)
   {
    if(!MODE_RT) return LocServoMoveWait(id,pos);
   T_QServos x;
   int r;
   x.cmde=QSMoveW;
   x.prm[0]=id;
   x.prm[1]=pos;
   r= xQueueSend(QServos,&x,QSendServosWait)==pdTRUE;
   if(r) CTMoveW++;
   return r;
   }

static void WaitForEndOfExec(volatile int16_t *v)
   {
    if(!MODE_RT) return;
   TickType_t xLastWakeTime;
   xLastWakeTime=xTaskGetTickCount();
   while(*v)
      {
      //Prévoir sortie sur demande !??!??!
      vTaskDelayUntil(&xLastWakeTime,pdMS_TO_TICKS(10));
      }
   }

//Lance le mouvement, attend la fin de celui-ci pour effectuer la prochaine commande et reste en attente de la fin d'ex�cution du mouvement
int ServoMoveWW(unsigned id, unsigned pos)
   {
   int r;
   r=ServoMoveW(id,pos);
   if(!MODE_RT) return r;
   if(r) WaitForEndOfExec(&CTMoveW);
   return r;
   }

#include <stdarg.h>



int ServoFct(void (*fprm)(uint16_t nbprm, const int16_t *prm), uint16_t nbprm, const int16_t *prm)
   {
   T_QServos x;
   int r;
   uint16_t i;
   if(!MODE_RT) {fprm(nbprm,prm); return 0;}
   x.cmde=QSFPrm;
   x.u.fprm=fprm;
   if(nbprm>sizeof(x.prm)/sizeof(x.prm[0])-2) nbprm=sizeof(x.prm)/sizeof(x.prm[0])-2;
   x.prm[0]=nbprm;
   for(i=0; i<nbprm; i++) x.prm[i+1]=prm[i];
   x.prm[0]=nbprm;

   r= xQueueSend(QServos,&x,QSendServosWait)==pdTRUE;
   if(r) CTFPrm++;
   return r;
   }


int ServoFctW(void (*fprm)(uint16_t nbprm, const int16_t *prm), uint16_t nbprm, const int16_t *prm)
   {
   int r;
   r=ServoFct(fprm,nbprm,prm);
   if(!MODE_RT) return 0;
   if(r) WaitForEndOfExec(&CTFPrm);
   return r;
   }


int NewGCMD(uint16_t n, const int16_t *gcmd)
   {
   return LocNewGCMD(n,gcmd);
   }

int NewGFCT(uint16_t n, void (*fct)(void))
   {
   return LocNewGFCT(n,fct);
   }


//Programme le mouvement. Rend la main immédiatement
int GMouvement(uint16_t n)
   {
    if(!MODE_RT) return LocGMouvement(n);
   T_QServos x;
   int r;
   x.cmde=QSGMouvement;
   x.prm[0]=n;
   r= xQueueSend(QServos,&x,QSendServosWait)==pdTRUE;
   if(r) CTGMouvement++;
   return r;
   }

//Lancement du mouvement. Rend la main quand le mouvement est exécuté complètement
int GMouvementW(uint16_t n)
   {
   int r;
   r=GMouvement(n);
   if(!MODE_RT) return r;
   if(r) WaitForEndOfExec(&CTGMouvement);
   return r;
   }

//Programme une synchro en attendant que la fonction fct renvoie 1.
int ServoSync(int (*fct)(void))
   {
    if(!MODE_RT) return 1;
   T_QServos x;
   int r;
   x.cmde=QSSync;
   x.u.fsync=fct;
   r= xQueueSend(QServos,&x,QSendServosWait)==pdTRUE;
   if(r) CTSync++;
   return r;
   }

//Programme une synchro en attendant que la fonction fct renvoie 1. Se met en attente que synchro soit effectu�e
int ServoSyncW(int (*fct)(void))
   {
   int r;
   r=ServoSync(fct);
   if(r) WaitForEndOfExec(&CTSync);
   return r;
   }

void TestPosServos(uint16_t nb,...)
   {
   va_list ap;
   va_start(ap,nb);
   uint16_t id[NBGCMDSVMAX];
   uint16_t i;
   VTCtrl(1);
   lcdclrscr();
   for(i=0; i<nb; i++)
      {
      id[i]=va_arg(ap,int);
      }
   va_end(ap);
   lcdprintf("%d Servos : ",nb);
      for(i=0; i<nb; i++)
          {
         EnableServoTorque(id[i],0);  //SetServoMaxTorque(p[i].id,0);
         lcdprintf("%u ",id[i]);
          }
      while(1)  //(!ETAT.Break)
         {
         uint16_t *pr;
         int px;
         const char *e;
         //static unsigned ctn=0;
         lcdgotoxy(1,4);
         for(i=0; i<nb; i++)
            {
            if(id[i]&(RXID(0)))
               {
               const char *nsv;
               nsv=GetSvName(id[i]);
               if(nsv) lcdprintf("[%s] ",nsv);
               lcdprintf("RX %u : ",id[i]&(RXID(0)-1));
               }
            else
               {
               const char *nsv;
               nsv=GetSvName(id[i]);
               if(nsv) lcdprintf("[%s] ",nsv);
               lcdprintf("AX %u : ",id[i]);
               }
            pr=GetServoAngle(id[i]);
            if(pr) px=*pr;
            else px=-1;
            e=GetSvErrorMsg();
            if(*e) lcdprintf("%s\t\n",e);
            else lcdprintf("[%d] Ok\t\n",px);
            }
         }
   }
