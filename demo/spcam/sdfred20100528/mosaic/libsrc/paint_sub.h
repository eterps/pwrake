#ifndef PAINT_SUB_H
#define PAINT_SUB_H

extern int doflood(float *tmp,int npx,int npy,int x,int y,
		   float thres,int *map);

extern int doflood3r(float *tmp,int npx,int npy,int x,int y,
		     int (*checkfunc)(float, int, void*), 
		     int (*setfunc)(int,int,float,void*),
		     void *result, int *map, int mapval, int npar, 
		     void *par);


extern int doflood3a(float *tmp,int npx,int npy,int x,int y,
		     float thres,
		     int (*setfunc)(int,int,float,void*),
		     void *result, int *map, int mapval);

extern int clean(float *g, int npx, int npy, float xcen, 
		 float ycen, float thres); /* returns npix */

extern int doflood2(float *tmp,int npx,int npy,int x,int y,
		    int (*func)(float,int,void*), int *map, int npar,
		    void *par);


extern int flood_lessthan(float a, int npar, void *par); /* double */
extern int flood_greaterthan(float a, int npar, void *par); /* double */
extern int flood_greaterequal(float a, int npar, void *par); /* double */
extern int flood_between(float a, int npar, void *par); /* double,double */

#endif
