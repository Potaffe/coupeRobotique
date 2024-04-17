//Gestion du temps en ms
//Utilise Timer 1
#include "TempsMs.h"


#define FiTT1 1000  // Fréquence des interruptions Timer1
#define NB_TEMPOS 10 // Nombre de temporisations utilisables simultanément

#define DIM(x) (sizeof(x)/sizeof(*x))

volatile int16_t RBT_TIME=32767;  //Durée match en 0,1s
volatile uint16_t T_DETECT;  //TimeOut sur _DETECT

#if !USE_RT
static volatile struct T_Tms
   {
   uint16_t t;
   unsigned used:1;
   }Tms[NB_TEMPOS];
#endif

void InitTempsMs(void)
{
#if !USE_RT
   volatile struct T_Tms *pt;
  	for(pt=Tms; pt<Tms+DIM(Tms); pt++) {pt->used=0; pt->t=0;}
#endif
}   

void (*MsFct1)(void)=0;
void (*MsFct2)(void)=0;
void GestTMs(void)
{	
   static unsigned cts=100;
#if !USE_RT
   volatile struct T_Tms *pt;
	for(pt=Tms; pt<Tms+DIM(Tms); pt++)
	   {
	   if(pt->used) if(pt->t) pt->t--;
	   }
#endif
	if(T_DETECT) T_DETECT--;
	if(RBT_TIME==32767) cts=100;
	cts--;
   if(!cts)
      {
      cts=100;
      if(RBT_TIME!=32767) RBT_TIME--;
      }   
	if(MsFct1) MsFct1();
	if(MsFct2) MsFct2();
	
}

#if USE_RT
TickType_t CreateDelaiMs(uint32_t ms)
{
	TickType_t tf;
	tf=xTaskGetTickCount()+ms/portTICK_PERIOD_MS;
	return tf;
}

int FinDelaiMs(TickType_t tf)
{
	if(xTaskGetTickCount()>=tf) return 1;
	return 0;
}

int ResteDelaiMs(TickType_t n)
{
	int32_t t;
	t=n-xTaskGetTickCount();
	if(t<0) t=0;
	return t;
}

void CloseDelaiMs(int n)
{
	(void)n;
}

#else

//D�bute une temporisation en ms sans attendre la fin.
//Renvoie un entier � utiliser lors de l'appel de FinDelaiMs() ou 0 si pas de tempo dispo
int CreateDelaiMs(uint16_t ms)
{
   volatile struct T_Tms *pt;
	for(pt=Tms; pt<Tms+DIM(Tms); pt++)
	   {
   	if(!pt->used)
   	   {
      	pt->t=ms;
      	pt->used=1;
      	return pt-Tms+1;
      	}
      }
    return 0;
}

//Renvoie 1 si la temporisation 'n' est termin�e
//Renvoie 0 si la temporisation est en cours
//Renvoie -1 en cas d'erreur
int FinDelaiMs(int n)
{
   volatile struct T_Tms *pt;
   if(!n) return -1;
   n--;
   pt=Tms+n;
   if(!pt->used) return -1;
   if(pt->t) return 0;
   return 1;
}   

//Renvoie le temps restant en ms de la temporisation 'n'
//Renvoie 0 si la temporisation est finie
//Renvoie -1 en cas d'erreur
int ResteDelaiMs(int n)
{
   volatile struct T_Tms *pt;
   if(!n) return -1;
   n--;
   pt=Tms+n;
   if(!pt->used) return -1;
   return (pt->t);
}   


void CloseDelaiMs(int n)
{
   volatile struct T_Tms *pt;
   if(!n) return;
   n--;
   pt=Tms+n;
   pt->used=0;
   pt->t=0;
}
#endif
   
#if !USE_RT
void DelaiMs(uint16_t ms)
{
int i;
i=CreateDelaiMs(ms);
while(!FinDelaiMs(i));
CloseDelaiMs(i);
}
#endif
