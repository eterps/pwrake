#ifndef MOMENT2D_H
#define MOMENT2D_H

typedef struct moments
{
  int initialized;
  double x_mean;
  double y_mean;
  double s;
  int xdimmax;
  int ydimmax;
  int dimmax;
  double *dat; /* simple weighed sum */
  double *wdat; /* double weighed sum */
  double (*weight)(double,double,double);
  double (*weight2)(double,double,double);
} moment2d;

extern int init_moment2d(moment2d *dat, int xdimmax, 
			    int ydimmax, int dimmax);

extern int add_moment2d_raw(moment2d *dat, double x, double y, 
			   double w, double f);

extern int add_moment2d(moment2d *dat, double x, 
			   double y, double f); /* use weight func */

extern int free_moment2d(moment2d *dat);


#endif

