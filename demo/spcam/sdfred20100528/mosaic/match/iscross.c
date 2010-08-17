#include <stdlib.h>
#include <stdio.h>
#include "iscross.h"

float dot(POINT *p1,POINT *p2)
{
  return p1->x*p2->x+p1->y*p2->y;
}


POINT ort(POINT *p1)
{
  POINT p;
  p.x=p1->y;
  p.y=-p1->x;
  return p;
}

POINT sub(POINT *p1,POINT *p2)
{
  POINT p;
  p.x=p1->x-p2->x;
  p.y=p1->y-p2->y;
  return p;
}

int izzero(POINT *a)
{
  return ((a->x==0)&&(a->y==0));
}

int isCross(POINT *a,POINT *b,POINT *p,POINT *q)
{
  POINT ba,pa,qp,qa,qpo;
  float ret;

  ba=sub(b,a);
  pa=sub(p,a);

  qp=sub(q,p);
  qa=sub(q,a);
  qpo=ort(&qp);

  if(dot(&ba,&qpo)==0)
    {
      /* pararell */
      if(dot(&pa,&qpo)!=0)
	return 0; 
      else
	{
	  if(dot(&qa,&qpo)!=0)
	    return 0; 
	  /* on the same line */

	  if(dot(&pa,&pa)<dot(&ba,&ba) && 
	     dot(&pa,&ba)>0) 
	    {
	      return 1;
	    }
	  else
	    {
	      if(dot(&qa,&qa)<dot(&ba,&ba)&&
		 dot(&qa,&ba)>0
		 ) return 1;
	      else return 0;
	    }
	}
    }

  ret=dot(&pa,&qpo)/dot(&ba,&qpo);
  if (ret>=0&&ret<=1) 
    {

      ret=(dot(&ba,&qp)*ret-dot(&pa,&qp))/dot(&qp,&qp);
      if (ret>=0&&ret<=1) 
	{      
	  return 1;
	}
    }
  return 0;
}

int isCross2(float ax, float ay,
	     float bx, float by,
	     float px, float py,
	     float qx, float qy)
{
  POINT a,b,p,q;

  a.x=ax;
  b.x=bx;
  p.x=px;
  q.x=qx;

  a.x=ay;
  b.x=by;
  p.x=py;
  q.x=qy;

  return isCross(&a,&b,&p,&q);
}
