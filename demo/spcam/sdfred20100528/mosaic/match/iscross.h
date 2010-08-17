#ifndef ISCROSS_H
#define ISCROSS_H

typedef struct point
{
  float x,y;
} POINT;

extern int isCross(POINT *a,POINT *b,POINT *p,POINT *q);
extern int isCross2(float ax, float ay,
		    float bx, float by,
		    float px, float py,
		    float qx, float qy);

#endif
