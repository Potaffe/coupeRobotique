/*
 * Dijkstra.c
 *
 *  Created on: 19 juin 2019
 *      Author: deloizy
 */

#include <stdint.h>
#include <math.h>
#ifndef __SIM__
#include <FreeRTOS.h>
#include <task.h>
#include "Odometrie.h"
#endif
#include "Dijkstra.h"

#define INF 9999
#define false 0
#define true 1




#define DIV 1  //Division pour éviter débordements
#define MARGE 100 //100 - Distance de sécurité % bord de table
#define SZX 2000 //Taille de la table
#define SZY 3000

#if 1 //950ms @216MHz STM32F7
#define NBX 12 //14   //Nombre de cases en X
#define NBY 19 //9	 //Nombre de cases en Y
#else //250ms @216MHz STM32F7
#define NBX 14   //Nombre de cases en X
#define NBY 9    //Nombre de cases en Y
#endif

#define DX ((SZX-2*MARGE)/NBX)
#define DY ((SZY-2*MARGE)/NBY)

#define NBC (NBX*NBY)


typedef struct
{
   int xmin,xmax,ymin,ymax;
   }T_Rect;

typedef struct
{
   int16_t x,y;
   }T_DIJPOINT;

   typedef struct
   {
      T_DIJPOINT p;
      uint32_t tfin; //Instant de désactivation de l'obstacle (0 si jamais)
      uint16_t larg;
      uint16_t actif; //Indique si obstacle actif ou non
   }T_OBST;


static uint16_t adjMatrix[NBC][NBC];
static struct
{
   uint16_t distance;
   int predecessor:15;
   unsigned mark:1;
}P[NBC];

static uint16_t source,dest;


#define DIST_SECUR 140
static T_Rect FIXES_RECT[]=
{
   //{.xmin=1000-DIST_SECUR,.xmax=1500,.ymin=300-DIST_SECUR,.ymax=1200},
   //{.xmin=-1500,.xmax=-1000+DIST_SECUR,.ymin=300-DIST_SECUR,.ymax=1200},
   //{.xmin=-1000-DIST_SECUR,.xmax=1000+DIST_SECUR,.ymin=1540-DIST_SECUR,.ymax=2000},
   //{.xmin=-DIST_SECUR,.xmax=+DIST_SECUR,.ymin=1540-200-DIST_SECUR,.ymax=1500},
		//zones adverses
	{.xmin=-1500,.xmax=-1050+DIST_SECUR,.ymin=0,.ymax=450+DIST_SECUR},
	{.xmin=1050-DIST_SECUR,.xmax=1500,.ymin=225-DIST_SECUR,.ymax=225+DIST_SECUR},
	{.xmin=-1500,.xmax=-1050+DIST_SECUR,.ymin=1550-DIST_SECUR,.ymax=2000},
		//zones alliés
	{.xmin=1050-DIST_SECUR,.xmax=1500,.ymin=0,.ymax=450+DIST_SECUR},
	{.xmin=-1500,.xmax=-1050+DIST_SECUR,.ymin=225-DIST_SECUR,.ymax=225+DIST_SECUR},
	{.xmin=1050-DIST_SECUR,.xmax=1500,.ymin=1550-DIST_SECUR,.ymax=2000},
		//zone pami
	{.xmin=-450-DIST_SECUR,.xmax=450+DIST_SECUR,.ymin=0,.ymax=150+DIST_SECUR},
};

static T_OBST FIXES_ROND[]=
{
   //{.p={.x=500,.y=1050},.larg=250,.tfin=0,.actif=1},
   //{.p={.x=-500,.y=1050},.larg=250,.tfin=0,.actif=1},
   //{.p={.x=800,.y=300},.larg=300,.tfin=0,.actif=1},
   //{.p={.x=-800,.y=300},.larg=200,.tfin=0,.actif=1},
		//ronds pour les plantes
	{.p={.x=0,.y=500},.larg=120+DIST_SECUR,.tfin=0,.actif=1},
	{.p={.x=500,.y=700},.larg=120+DIST_SECUR,.tfin=0,.actif=1},
	{.p={.x=500,.y=1300},.larg=120+DIST_SECUR,.tfin=0,.actif=1},
	{.p={.x=0,.y=1500},.larg=120+DIST_SECUR,.tfin=0,.actif=1},
	{.p={.x=-500,.y=1300},.larg=120+DIST_SECUR,.tfin=0,.actif=1},
	{.p={.x=-500,.y=700},.larg=120+DIST_SECUR,.tfin=0,.actif=1},
};

static T_OBST MOBILES[20];


//Supprime les obstacles mobiles
void DijkClearObstacles(void)
{
	uint16_t i;
	for(i=0; i<sizeof(MOBILES)/sizeof(*MOBILES); i++)
	{
		MOBILES[i].actif=0;
	}
}

static void TestActivity(void)
{
   TickType_t t;
	uint16_t i;
	t=xTaskGetTickCount();
	for(i=0; i<sizeof(MOBILES)/sizeof(*MOBILES); i++)
	{
		if(MOBILES[i].actif)
		   {
		   if(MOBILES[i].tfin && (t>=MOBILES[i].tfin)) MOBILES[i].actif=0;
		   }
	}
}

static int IsNear(const T_DIJPOINT *p1, const T_DIJPOINT *p, int eps)
{
   float r,dxr,dyr,x2;
   dxr=p1->x-p->x; dyr=p1->y-p->y;
   r=eps;
   r*=r;
   x2=dxr*dxr+dyr*dyr;
   return x2<=r;
}


#define ECMOB 200 //Ecart min entre les obstacles mobiles pour pouvoir les distinguer (en mm)

//Ajoute un obstacle (mobile) sur la table
//renvoie 0 si trop d'obstacles définis
//ms : durée de vie de l'obstacle (infinie si 0)
//Si ajout proche (voir ECMOB) d'un autre déjà défini : mise à jour de celui déjà existant
int16_t DijkAddObstacle(int16_t x, int16_t y, uint16_t larg, uint16_t ms)
{
	TickType_t t;
	uint16_t i;
	TestActivity();
	t=xTaskGetTickCount();
	for(i=0; i<sizeof(MOBILES)/sizeof(*MOBILES); i++)
	   {
	   T_DIJPOINT n;
	   n.x=x; n.y=y;
	   if(MOBILES[i].actif && IsNear(&n,&MOBILES[i].p,ECMOB))
	      {
	      MOBILES[i].p.x=x;
	      MOBILES[i].p.y=y;
	      MOBILES[i].larg=larg;
	      if(ms) MOBILES[i].tfin=t+ms; else MOBILES[i].tfin=0;
	      return 1;
	      }
	   }
	for(i=0; i<sizeof(MOBILES)/sizeof(*MOBILES); i++)
	{
		if(!MOBILES[i].actif)
		{
		MOBILES[i].p.x=x;
		MOBILES[i].p.y=y;
		MOBILES[i].larg=larg;
		if(ms) MOBILES[i].tfin=t+ms; else MOBILES[i].tfin=0;
		MOBILES[i].actif=1;
		return 1;
		}
	}
	return 0;
}


static int16_t DijInInRect(int x, int y,const T_Rect *r)
{
    if (x<r->xmin) return 0;
    if (x>r->xmax) return 0;
    if (y<r->ymin) return 0;
    if (y>r->ymax) return 0;
    return 1;
}


static int IsBetween(int16_t x, int16_t a, int16_t b)
{
   int16_t t;
   if(b<a) {t=a; a=b; b=t;}
   if(x<a) return 0;
   if(x>b) return 0;
   return 1;
}

//Indique si p se trouve à une distance inférieure à eps du segment (p1,p2)
//Version du 24/06/2019
static int IsOnSeg(const T_DIJPOINT *p1, const T_DIJPOINT *p2, const T_DIJPOINT *p, int eps)
    {
    float dxr,dyr;
    float a,b,c,d,e,g;
    int16_t dx,dy;
    if(IsNear(p1,p,eps)) return 1;
    dx=p2->x-p1->x; dy=p2->y-p1->y;
    if(!(dx||dy))  //Segment de taille nulle !
       {
       return 0;
       }
    if(IsNear(p2,p,eps)) return 1;
    if(dx) //Si pas droite verticale
    {
       dxr=dx; dyr=dy;
       a=-dyr/dxr;
       b=1;
       c=-p1->y-a*p1->x; //Coef droite ax+by+c=0
       if(dy)
       {
          g=p->y-p->x/a;//Droite orthogonale y=(-1/a)x+g
          if(!IsBetween((g+c)/(-a-1.f/a),p1->x,p2->x)) return 0;
       }
       else //x=p->x droite orthogonale verticale
       {
          if(!IsBetween(p->x,p1->x,p2->x)) return 0;
       }
    }
    else //droite verticale
    {
       a=1;
       b=0;
       c=-p1->x;
       //g=p->y;  //y=p->y; droite orthogonale horizontale
       if(!IsBetween(p->y,p1->y,p2->y)) return 0;
    }
    d=a*p->x+b*p->y+c;
    d=d*d/(a*a+b*b); //Distance² % droite
    e=eps;
    e*=e;
    return d<e;
    }

static int SegIsInRect(const T_DIJPOINT *p1, const T_DIJPOINT *p2, const T_Rect *r)
   {
   float dxr, dyr;
   float a;
   int16_t dx, dy, x, y, b;
   union
      {
         uint16_t w;
         struct
            {
               unsigned xr1 :1;
               unsigned xr2 :1;
               unsigned yr1 :1;
               unsigned yr2 :1;
               unsigned yn :1;
               unsigned ym :1;
               unsigned xn :1;
               unsigned xm :1;
            } b;
         struct
            {
               unsigned r :4;
               unsigned o :4;
            } h;
      } m;
   if(DijInInRect(p1->x,p1->y,r)) return 1;
   if(DijInInRect(p2->x,p2->y,r)) return 1;
   m.w=0;
   if(IsBetween(p1->x,r->xmin,r->xmax)) m.b.xr1=1;
   if(IsBetween(p2->x,r->xmin,r->xmax)) m.b.xr2=1;
   if(IsBetween(p1->y,r->ymin,r->ymax)) m.b.yr1=1;
   if(IsBetween(p2->y,r->ymin,r->ymax)) m.b.yr2=1;
   if(IsBetween(r->ymin,p1->y,p2->y)) m.b.yn=1;
   if(IsBetween(r->ymax,p1->y,p2->y)) m.b.ym=1;
   if(IsBetween(r->xmin,p1->x,p2->x)) m.b.xn=1;
   if(IsBetween(r->xmax,p1->x,p2->x)) m.b.xm=1;
   if(!m.h.r) return 0;
   dx=p2->x-p1->x;
   dy=p2->y-p1->y;
   if(!(dx||dy))  //Segment de taille nulle !
      {
      return 0; //DijInInRect(p1->x,p1->y,r);
      }
   if(!dx) //Si droite verticale (x=p1->x)
      {
      return m.b.xr1&&(m.b.yn||m.b.ym);
      }
   dxr=dx;
   dyr=dy;
   a=dyr/dxr;
   b=p1->y-a*p1->x;
   if(dy)
      {
      x=(r->ymin-b)/a;
      if(IsBetween(x,r->xmin,r->xmax)&&(m.b.yn||m.b.ym)) return 1;
      x=(r->ymax-b)/a;
      if(IsBetween(x,r->xmin,r->xmax)&&(m.b.yn||m.b.ym)) return 1;
      }
   if(!(m.b.xn||m.b.xm)) return 0;
   y=(int16_t)(a*r->xmin)+b;
   if(IsBetween(y,r->ymin,r->ymax)) return 1;
   if(!dy) return 0;
   y=(int16_t)(a*r->xmax)+b;
   if(IsBetween(y,r->ymin,r->ymax)) return 1;
   return 0;
   }



static uint16_t Dist(int xs,int ys,int xd,int yd)
{
   unsigned i,d;
   int dx,dy;
   T_DIJPOINT p1,p2;
   p1.x=xs; p1.y=SZY-ys;
   p2.x=xd; p2.y=SZY-yd;
   for(i=0; i<sizeof(FIXES_RECT)/sizeof(*FIXES_RECT); i++)
      {
         if(SegIsInRect(&p1,&p2,FIXES_RECT+i)) return INF;
      }

   for(i=0; i<sizeof(FIXES_ROND)/sizeof(*FIXES_ROND); i++)
   {
      if(IsOnSeg(&p1,&p2,&FIXES_ROND[i].p,FIXES_ROND[i].larg)) return INF;
   }

   for(i=0; i<sizeof(MOBILES)/sizeof(*MOBILES); i++)
   {
	   if(!MOBILES[i].actif) continue;
	   if(IsOnSeg(&p1,&p2,&MOBILES[i].p,MOBILES[i].larg)) return INF;
   }
   dx=(xd-xs)/DIV;
   dy=(yd-ys)/DIV;
   d=sqrtf(dx*dx+dy*dy);

   return d;
}

//Calcul distances entre 2 points sans prise en compte obstacles
static uint16_t Dist2(int16_t xs,int16_t ys,int16_t xd,int16_t yd)
{
   uint16_t d;
   int16_t dx,dy;
   dx=(xd-xs)/DIV;
   dy=(yd-ys)/DIV;
   d=sqrtf(dx*dx+dy*dy);
   return d;
}

static void InitMatrix(void)
{
   uint16_t i,j,m,n;
   int16_t xs,ys,xd,yd;
   TestActivity();
   for(i=0; i<NBX; i++)
   {
      xs=i*DX+(DX/2)-(SZX/2)+MARGE;
      for(j=0; j<NBY; j++)
      {
         ys=j*DY+(DY/2)+MARGE;
         ys=SZY-ys;
         for(m=0; m<NBX; m++)
         {
            xd=m*DX+(DX/2)-(SZX/2)+MARGE;
            for(n=0; n<NBY; n++)
            {
               yd=n*DY+(DY/2)+MARGE;
               yd=SZY-yd;
               adjMatrix[i*NBY+j][m*NBY+n]=Dist(xs,ys,xd,yd);
            }
         }
      }
   }
}


static void PNode(uint16_t n, uint16_t *ix, uint16_t *iy)
{
   *ix=n/NBY;
   *iy=n%NBY;
   //*iy=NBY-1-*iy;
}

static void Coord(uint16_t node, int16_t *x, int16_t *y)
{
   uint16_t ix,iy;
   PNode(node,&ix,&iy);
   *x=ix*DX+(DX/2)-(SZX/2)+MARGE;
   *y=iy*(DY)+(DY/2)+MARGE;
   //*y=SZY-*y;
}

static uint16_t Case(int16_t x, int16_t y)
{
   int16_t ix,iy;
   ix=(x-MARGE+(SZX/2)-(DX/2))/DX;
   iy=(y-MARGE-(DY/2))/DY;
   //iy=NBY-1-iy;
   return ix*NBY+iy;
}



static void initialize(void)
   {
   uint16_t i;
   for(i=0; i<NBC; i++)
      {
      P[i].mark=0;
      P[i].predecessor = -1;
      P[i].distance = INF;
      }
   P[source].distance=0;
   }


static int16_t getClosestUnmarkedNode(void)
   {
   uint16_t i;
   uint16_t minDistance = INF;
   uint16_t closestUnmarkedNode;
   closestUnmarkedNode=-1;
   for(i=0; i<NBC; i++)
      {
      if(P[i].mark) continue;
      if( minDistance >= P[i].distance)
         {
         minDistance = P[i].distance;
         closestUnmarkedNode = i;
         }
      }
   return closestUnmarkedNode;
   }


static void calculateDistance(void)
   {
   uint16_t i;
   unsigned d;
   int16_t n;
   uint16_t closestUnmarkedNode;
   uint16_t count;
   initialize();
   count = 0;
   while(count < NBC)
      {
      n=getClosestUnmarkedNode();
      if(n<0) continue;
      closestUnmarkedNode = n;
      P[closestUnmarkedNode].mark=1;
      for(i=0; i<NBC; i++)
         {
         if(P[i].mark) continue;
         if(adjMatrix[closestUnmarkedNode][i]!=INF) //>0)
            {
            d=P[closestUnmarkedNode].distance+adjMatrix[closestUnmarkedNode][i];
            if(P[i].distance > d)
               {
               P[i].distance = d;
               P[i].predecessor = closestUnmarkedNode;
               }
            }
         }
      count++;
      }
   }

static T_DIJPOINT CHEMIN[NBX+NBY];
static uint16_t NB_CHEMIN;
static void AddChemin(int16_t x, int16_t y)
{
   if(NB_CHEMIN>=sizeof(CHEMIN)/sizeof(*CHEMIN)) return;
   CHEMIN[NB_CHEMIN].x=x;
   CHEMIN[NB_CHEMIN].y=y;
   NB_CHEMIN++;
}

//Renvoie dans x & y le prochain point du tracé établi par Dijkstra
//La fonction renvoie 0 si la destination est atteinte ou 1 si un point est disponible
//Le chemin entier n'est plus disponible à la suite de l'appel de cette fonction
int16_t DijkGetNextPoint(int16_t *x, int16_t *y)
{
   if(NB_CHEMIN<1) return 0;
   NB_CHEMIN--;
   *x=CHEMIN[NB_CHEMIN].x;
   *y=CHEMIN[NB_CHEMIN].y;
   return 1;
}

//Renvoie le nombre de points constituant un chemin
uint16_t DijkGetNb(void)
{
	return NB_CHEMIN;
}

//Renvoie dans x & y le point n du tracé établi par Dijkstra
//n peut varier entre 0 et DijkGetNb()-1
//La fonction renvoie 0 si le point n'est pas dans le chemin ou 1 sinon
//Le chemin est préservé suite à l'appel de cette fonction
int16_t DijkReadPoint(uint16_t n, int16_t *x, int16_t *y)
{
   if(n>=NB_CHEMIN) return 0;
   n=NB_CHEMIN-1-n;
   *x=CHEMIN[n].x;
   *y=CHEMIN[n].y;
   return 1;
}


static int AddPath(uint16_t node)
   {
   int16_t x,y;
   Coord(node,&x,&y);
   AddChemin(x,y);
   if(node == source) return 1;
   if(P[node].predecessor == -1) return 0;
   if(!AddPath(P[node].predecessor)) return 0;
   return 1;
   }


//Construit un chemin depuis case s jusqu'à case d
static int BuildRoad(uint16_t s, uint16_t d)
{
   source=s;
   dest=d;
   calculateDistance();
   if(!AddPath(dest)) {NB_CHEMIN=0; return -1;}
   return P[d].distance;
}

static void SupprSeg(int16_t a, int16_t b)
{
   int16_t i,j;
   if(b<a) return;
   j=0;
   for(i=b; i<NB_CHEMIN; i++)
   {
      CHEMIN[a+j]=CHEMIN[b+j+1];
      j++;
   }
   NB_CHEMIN-=b-a+1;
}

static uint16_t CalcLongChem(void)
{
   int16_t i,d;
   d=0;
   for(i=1; i<NB_CHEMIN; i++)
      {
      d+=Dist2(CHEMIN[i-1].x,CHEMIN[i-1].y,CHEMIN[i].x,CHEMIN[i].y);
      }
   return d;
}

static uint16_t OptRoad(void)
   {
   int16_t i,j,modif;
   int16_t d;

   do
      {
      modif=0;
      for(i=0; i<NB_CHEMIN-2; i++)
         {
         for(j=NB_CHEMIN-1; j>i+1; j--)
            {
            d=Dist(CHEMIN[i].x,SZY-CHEMIN[i].y,CHEMIN[j].x,SZY-CHEMIN[j].y);
            if(d!=INF)
               {
               SupprSeg(i+1,j-1);
               modif=1;
               break;
               }
            }
         if(modif) break;
         }
      }
   while(modif);
   return CalcLongChem();
   }



//Calcule un chemin depuis la position courante (P_X, P_Y) jusqu'à (x,y)
//Renvoie la longueur du chemin trouvé ou 0 si pas de chemin
int DijkRoadTo(int16_t x, int16_t y)
{
   uint16_t s,d;
   int16_t r;
   s=Case(P_X,P_Y);
   d=Case(x,y);
   InitMatrix();
   source=s;
   NB_CHEMIN=0;
   AddChemin(x,y);
   r=BuildRoad(s,d);
   if(r>=0)
   {
      AddChemin(P_X,P_Y);
      r=OptRoad();
      return r;
      }
   NB_CHEMIN=0;
   return 0;
}




