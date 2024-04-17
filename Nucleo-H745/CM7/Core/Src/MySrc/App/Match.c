/*
 * Match.c
 *
 *  Created on: 28 nov. 2016
 *      Author: Robot
 */
#ifndef __SIM__
#if defined STM32F7
#include "stm32f7xx_hal.h"
#elsif defined STM32H7
#include "stm32h7xx_hal.h"
#elsif defined STM32F4
#include "stm32f4xx_hal.h"
#elsif defined STM32L4
#include "stm32l4xx_hal.h"
#elsif defined STM32G4
#include "stm32g4xx_hal.h"
#endif
#endif

#include <math.h>
#include <stdlib.h>

#include "Match.h"

#ifndef __SIM__
#include "ServoDefs.h"
#include "../Core/CommandesServos.h"
#include "../Core/Sequenceur.h"
#include "../Core/Odometrie.h"
#include "../Core/spidma.h"
#include "../Core/TempsMs.h"
#include "../../../../../Common/Src/LCD_VT100.h"
#include "../Core/CommandesServos.h"
#include "../Core/I2CMaitre.h"
#include "../Core/I2CUSCommun.h"
#include "Config.h"
#include "../Core/LowLevel.h"
#include "Menu.h"
#include "RT-Main.h"
#include "Communic.h"
#include "../Core/Asserv.h"
#include "../Core/Dijkstra.h"

#include "OtherTasks.h"
#endif


#define DBG 0

#define P_X0 (1000-225)
#define P_Y0 110


static uint16_t FINALE=0;

//Fonction de recopie d’un tableau de commandes groupées
void ServoGCopy(int16_t *dest, const int16_t *src)
   {
   while(*src!=GC_FIN) *(dest++ )=*(src++ );
   *dest=GC_FIN;
   }

/*
 void PgOpen(int16_t ngr)
   {
   //Le tableau gr est dynamique : cela permet d’avoir des éléments variables
   int16_t gr[]=
      {
      1, SV_PINCEG, PG_OPEN, GC_FIN
      };
   static int16_t t[sizeof(gr)/sizeof(*gr)];

   ServoGCopy(t,gr);
   NewGCMD(ngr,t);
   GMouvementW(ngr);
   }
*/


void FinMatch(void)
   {

   }

#define ParamLinStd() ParamLin(30000,30)
#define ParamRotStd() ParamRot(20000,25)
#define USDistNorm() USDist(200)

#define WAIT 0x80



void AffOdo(void)
   {
   lcdgotoxy(1,2);
   lcdprintf("X=%d Y=%d A=%.1f\t\n",P_X,P_Y,0.1f*THETA);
//   lcdgotoxy(1,4);
//   lcdprintf("Dist=%.1f\t",VDist);

   //lcdgotoxy(1,6); lcdprintf("Vbat=%.1f  -  Reste %.1f\t",V_BAT,0.1f*RBT_TIME);
   //lcdgotoxy(1,7);
   //lcdprintf("Mes Points=%u\t",GetGros()->score);

   }


void MatchQualif(void)
   {
   MatchStd();

   }

void MatchFinale(void)
   {
   FINALE=1;
   MatchStd();
   }

void TestDijkstra(int16_t x0, int16_t y0, int16_t xd, int16_t yd)
{
	   TickType_t t;
	   int r;
	   uint16_t i,nb;
	   InitOdometrie(x0,y0,0);
	   lcdprintf("(%d,%d)->(%d,%d) :\n",x0,y0,xd,yd);
	   t=xTaskGetTickCount();
	   _LEDR(1);
	   r=DijkRoadTo(xd,yd);
	   _LEDR(0);
	   t=xTaskGetTickCount()-t;
	   lcdprintf("Longueur : %d - Calcul : %dms",r,t);
	   nb=DijkGetNb();
	   for(i=0; i<nb; i++)
	      {
		   int16_t x,y;
		  DijkReadPoint(i,&x,&y);
	      if(!(i%3)) lcdprintf("\n");
	      lcdprintf("(%d,%d) ",x,y);
	      }
	   lcdprintf("\n------------\n");


}


#define AUTOEVIT 1
#define AFFDEP 1



//Cherche chemin vers (xd,yd). Exécute lastact() avant de réaliser le dernier tronçon
int DijkstraGoLastAct(int16_t xd, int16_t yd, int (*move)(int,int), int (*lastact)(void))
   {
   int r;
   uint16_t i,nb;
   int16_t x,y;
#if AFFDEP
   lcdprintf("Recherche chemin vers %d,%d\n",xd,yd);
#endif
   do
      {
      r=DijkRoadTo(xd,yd);
      if(r==0)  //Pas de chemin trouvé
         {
         StopUrgence();
#if AFFDEP
         lcdprintf("Pas de chemin !\n");
#endif
         return 0;
         }
      nb=DijkGetNb();
#if AFFDEP
      lcdprintf("Chemin (nb=%d) : //",nb);
      for(i=1; i<nb; i++)
         {
         DijkReadPoint(i,&x,&y);
         lcdprintf("(%d,%d) ",x,y);
         }
      lcdprintf("//\n");
#endif
      for(i=1; i<nb; i++)
         {
         DijkReadPoint(i,&x,&y);
         SiUrgence(50,1000);
         if(lastact && (i==nb-1)) r=lastact();
         else
            {
            //r=AvanceVers(x,y);
            if(move) r=move(x,y);
            else r=RBT_EOTHER;
            }
         if(r!=RBT_EOK) break;
         }
      }
   while(i!=nb);
   StopUrgence();
   return 1;
   }

int DijkstraAvanceVers(int16_t xd, int16_t yd)
{
   return DijkstraGoLastAct(xd,yd,_AvanceVers,0);
}




void AfficheScore(int points){
   lcdclrscr();
   lcdSetFont("Arial28");
   lcdBackColor565(C565_White);
   lcdTextColor565(C565_Black);
   lcdgotoxy(1,2);
   lcdprintf("score=%d \t",points);
}


void BrasPreparePrisePlanteSol(void)
{
	static const int16_t gcmd[]=
	{
			3,SV_EPAULE,SV_BRAS,SV_PINCE,
			GC_VIT,1000,1000,1000,
			EPAULE_PRISE_POT,BRAS_PRISE_PLANTE,PINCE_OUVERTE,
			EPAULE_PRISE_PLANTE,BRAS_PRISE_PLANTE,PINCE_OUVERTE,
			GC_FIN
	};
	GMouvementGr(gcmd);
}

void BrasPrendPlanteSolEtLeve(void)
{
	static const int16_t gcmd[]=
	{
			3,SV_EPAULE,SV_BRAS,SV_PINCE,
			GC_VIT,300,300,300,
			EPAULE_PRISE_PLANTE,BRAS_PRISE_PLANTE,PINCE_FERMEE,
			EPAULE_TRANSPORT,BRAS_TRANSPORT,PINCE_FERMEE,
			GC_FIN
	};
	GMouvementGr(gcmd);
}
/*
 * @param plante 0 plante milieu ; 1 plante de droite ; 2 plante de gauche
 */
void PrendPlanteEtLeve(uint8_t plante){
	PushParam(0);
	ParamLin(15000,20);
	ReculeDe(300);
	BrasPreparePrisePlanteSol();
	ParamLin(1500,20);
	switch(plante){	// dEntrePlantes: 90 ; dFourcheRef: 80
	case 1:
		TourneDroiteDe(20);
		AvanceDe(180);
		break;
	case 2:
		TourneGaucheDe(20);
		AvanceDe(180);
		break;
	default:
		AvanceDe(170); // plante du milieu ; dPincesFourches: 130
		break;
	}

	ServoMoveWW(SV_PINCE,PINCE_FERMEE); //plante centrale prise
	BrasPrendPlanteSolEtLeve(); //plante dans la pince
	PopParam(0);
}

void BrasDepotJardiniereEtRange(void)
{
	static const int16_t gcmd[]=
	{
			3,SV_EPAULE,SV_BRAS,SV_PINCE,
			GC_VIT,300,300,300,
			EPAULE_DEPOT_PLANTE,BRAS_DEPOT_PLANTE,PINCE_FERMEE,
			EPAULE_DEPOT_PLANTE,BRAS_DEPOT_PLANTE,PINCE_OUVERTE,
			//GC_VIT,1000,1000,1000,
			EPAULE_RANGE,BRAS_RANGE,PINCE_OUVERTE,
			GC_FIN
	};
	GMouvementGr(gcmd);
}


void RangerBras(void)
{
	static const int16_t gcmd[]=
	{
			3,SV_EPAULE,SV_BRAS,SV_PINCE,
			GC_VIT,500,500,500,
			EPAULE_PRISE_POT,BRAS_PRISE_POT,PINCE_OUVERTE,
			EPAULE_RANGE,BRAS_RANGE,PINCE_OUVERTE,
			GC_FIN
	};
	GMouvementGr(gcmd);
}


void ReculeEtEsquive(int dist)
{
   int r;
   BrasDepotJardiniereEtRange(); //plante relâchée dans la jardinière
   SiUrgence(30,1000);
   r=ReculeDe(dist);
   SiUrgence(0,0);
   if(r!=RBT_EOK)
   {
      TourneDroiteDe(900);
      ReculeDe(300);
   }
}

void TatannePanneauxAllies(){

	ServoInitStd(SV_SOLAIRED, 254, 1023, 750, 10);
	ServoMove(SV_SOLAIRED,SOLD_REPLI_DROITE);
	if(USE_LIDAR){
		int ret=0;
		ret=DijkstraAvanceVers(1000,1700);
		if(ret==0)return;
	}else{
		AvanceVers(1000,1700);
	}


	ReculeVers(1284,1812);
	TourneVers(1800);
	ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_GAUCHE);
	AvanceDe(190);
	ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_DROITE);

	AvanceVers(1060,1812);
	TourneVers(1800);
	ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_GAUCHE);
	AvanceDe(190);
	ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_DROITE);

	AvanceVers(835,1812);
	TourneVers(1800);
	ServoMoveWW(SV_SOLAIRED,SOLD_REPLI_GAUCHE);

	//ReculeDe(20);
	//TourneGaucheDe(900);


	AvanceVers(500,1700);
	ServoInitStd(SV_SOLAIRED, 254, 700, 500, 10);
	ServoMove(SV_SOLAIRED,SOLD_REPLI_DROITE);

}

void MatchStd(void)
   {

   lcdclrscr();
   lcdprintf("Go...");
   DefineFctDuringMovement(AffOdo);
   VTCtrl(0);
   VTExtended(0);

   //ParamLin(3000,20); ParamRot(3000,20); AvanceVers(500,500); DelaiMs(1000); ReculeVers(1200,400); while(1);

   VTCtrl(1);

   DelaiMs(10);
   DetectDist(100);
   ParamLin(20000,25);
   ParamRot(8000,20);

   int x,y,a;
   x=P_X; y=P_Y; a=THETA;

   /*Rbt_AssStop(0);
   while(1) AffOdo();*/

   /*PushParam(0);
   ParamLin(3000,20);
   AvanceVers(700,250);
   DijkstraAvanceVers(-500,1700); //test dijkstra
   PopParam(0);*/

   AvanceVers(1000,700);
   TatannePanneauxAllies();
   /*ReculeVers(1280,300);
   TourneVers(900);
   return;*/
   AvanceVers(1000,1700);

   AvanceVers(1000,700);
   AvanceVers(1500-680,700);
   PushParam(0);
   ParamLin(1500,20);  // pour s'approcher des plantes
   AvanceDe(250);
   PrendPlanteEtLeve(1);
   PopParam(0);
   ReculeDe(100);
   AvanceVers(760,280);
   PushParam(0);
   ParamLin(1500,20);
   AvanceVers(760,220);
   BrasDepotJardiniereEtRange(); //plante relâchée dans la jardinière
   PopParam(0);
   ReculeDe(100);

   ReculeVers(1280,300);
   TourneVers(900);
   return;



   ReculeVers(1000, 500);
   AvanceVers(800,500);
   ReculeEtEsquive(300); //plante relâchée dans la jardinière


   AvanceVers(x,y); // retourne chercher les autres plantes
   TourneVers(a);
   BrasPreparePrisePlanteSol(); // recupère une autre plante

   ParamLin(1500,20);
   Rbt_Stretch(20);
   AvanceVers(-1199,847);

   ParamLin(15000,20); // acc
   BrasPrendPlanteSolEtLeve(); // plante direction dépot jardinière
   ReculeVers(-1000, 500);
   AvanceVers(-800,550);
   ReculeEtEsquive(300); //plante relâchée dans la jardnière
   AvanceVers(x,y); // retourne chercher les autres plantes
   TourneVers(a);
   BrasPreparePrisePlanteSol(); // recupère une autre plante

   ParamLin(1500,20);
   Rbt_Stretch(20);
   AvanceVers(-1252,850);

   ParamLin(15000,20); // acc
   BrasPrendPlanteSolEtLeve(); // plante direction dépot jardinière
   ReculeVers(-1000, 500);
  AvanceVers(-800,600);
  ReculeEtEsquive(300); //plante relâchée dans la jardnière
  ReculeVers(-1250, 500); // retourne maison
  ReculeVers(-1280, 220);
  TourneVers(900);

  return;

   Powen(0);
   while(1) AffOdo();

   while(1);

   Rbt_Stretch(200);
   AvanceVers(-580,2260);

   ParamLin(25000,20);

   ReculeVers(x,y);
   TourneVers(a);
   TourneVers(1800);


   while (1);
      }


void AffHomolog(void)
   {

//   lcdgotoxy(1,4);
//   lcdprintf("Mes Points=%u\t",GetGros()->score);

   lcdgotoxy(1,5);
   lcdprintf("Vbat=%.1f V\t",V_BAT);
   lcdgotoxy(1,6);
   lcdprintf("Reste %.1f s\t",0.1f*RBT_TIME);
   }


void Homologation(void)
   {
   lcdclrscr();
   lcdprintf("Homologation...");

//   if(COULEUR==COUL_POS) InitOdometrie(P_X0,741,2700+7); //2707
//   else InitOdometrie(-(P_X0),753,900-7);

   _Powen(1);
   EnableAsserv(1);
   VTCtrl(1);

   DelaiMs(10);
   ParamLin(10000,25);
   DefineFctDuringMovement(AffHomolog);

   AvanceVers(1000,700);
      AvanceVers(1500-680,700);

      PushParam(0);
      ParamLin(1500,20);  // pour s'approcher des plantes
      AvanceDe(250);
      PrendPlanteEtLeve(1);
      PopParam(0);
      ReculeDe(100);
      AvanceVers(760,280);
      PushParam(0);
      ParamLin(1500,20);
      AvanceVers(760,220);
      BrasDepotJardiniereEtRange(); //plante relâchée dans la jardinière
      PopParam(0);
      ReculeDe(100);

      ReculeVers(1280,300);
      TourneVers(900);
   }

void TestVitesse(void)
{
	lcdclrscr();
	lcdprintf("TestVitesse...");

	//   if(COULEUR==COUL_POS) InitOdometrie(P_X0,741,2700+7); //2707
	//   else InitOdometrie(-(P_X0),753,900-7);

	_Powen(1);
	EnableAsserv(1);
	VTCtrl(1);

	DelaiMs(10);
	ParamLin(10000,25);
	DefineFctDuringMovement(AffHomolog);

	AvanceVers(1200,500);
	ReculeVers(1300,500);
	ParamLin(20000,25);
	BasicMenu();
	AvanceDe(2000);
	BasicMenu();
	AvanceVers(1200,500);
	ReculeVers(1280,300);
	TourneVers(900);
}



void Demo(void)
   {
   DefineEndOfGame(FinMatch);
   lcdclrscr();
   lcdprintf("Démo...\n");
#if 0
   lcdprintf("Attente Tirette...\n");
   while(!_Tirette) DelaiMs(10);
   while(_Tirette);
#endif

   if(COULEUR==COUL_POS) InitOdometrie(P_X0,741,2700+7); //2707
   else InitOdometrie(-(P_X0),753,900-7);

   _Powen(1);
   EnableAsserv(1);
   VTCtrl(1);


   }


