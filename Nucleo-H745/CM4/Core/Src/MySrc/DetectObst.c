/*
 * DetectObst.c
 *
 *  Created on: Oct 29, 2019
 *      Author: deloi
 */
#include <stdint.h>
#include "Lidar.h"

typedef struct
{
   int16_t x,y;
   }T_POINT;


static int IsNear(const T_POINT *p1, const T_POINT *p, int eps)
{
   float r,dxr,dyr,x2;
   dxr=p1->x-p->x; dyr=p1->y-p->y;
   r=eps;
   r*=r;
   x2=dxr*dxr+dyr*dyr;
   return x2<=r;
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
static int IsOnSeg(const T_POINT *p1, const T_POINT *p2, const T_POINT *p, int eps)
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

#include "../../../Common/Src/SharedMemory.h"


//Renvoie 1 si le chemin devant ou derrière le robot contient une cible, et 0 sinon
int DetectObstacle(void)
   {
   T_POINT p0,p2,pc;
   float dx,dy;
   unsigned i;
   T_Pos b;
   p0.x=P_X; p0.y=P_Y;
   if(BAL_DETECT_AV)
      {
      dx=COSTH*BAL_DIST_AV;
      dy=SINTH*BAL_DIST_AV;
      p2.x=p0.x+dx;
      p2.y=p0.y+dy;
      for(i=0; i<DIM(BAL); i++)
         {
         b=BAL[i];
         if(getBAL_LIFE(b))
            {
            pc.x=getBAL_X(b);
            pc.y=getBAL_Y(b);
            if(IsOnSeg(&p0,&p2,&pc,LARGEUR_ROBOT/2)) return 1;
            }
         }
      }
   if(BAL_DETECT_AR)
      {
      dx=COSTH*BAL_DIST_AR;
      dy=SINTH*BAL_DIST_AR;
      p2.x=p0.x-dx;
      p2.y=p0.y-dy;
      for(i=0; i<DIM(BAL); i++)
         {
         b=BAL[i];;
         if(getBAL_LIFE(b))
            {
            pc.x=getBAL_X(b);
            pc.y=getBAL_Y(b);
            if(IsOnSeg(&p0,&p2,&pc,LARGEUR_ROBOT/2)) return 1;
            }
         }
      }
   return 0;
   }
