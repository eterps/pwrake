#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "moment2d.h"

int init_moment2d (moment2d *dat, int xdimmax, int ydimmax, int dimmax)
{
  int nx,ny;

  dat->initialized=1;
  dat->x_mean=0;
  dat->y_mean=0;
  dat->s=0;
  dat->xdimmax=xdimmax;
  dat->ydimmax=ydimmax;
  dat->dimmax=dimmax;

  dat->weight=NULL;
  dat->weight2=NULL;

  nx=xdimmax+1;
  ny=ydimmax+1;

  /*
    dat->dat=(double*)realloc(dat->dat,nx*ny*sizeof(double));
  */
  dat->dat=(double*)malloc(nx*ny*sizeof(double));
  memset(dat->dat,0,nx*ny*sizeof(double));

  /*
    dat->wdat=(double*)realloc(dat->wdat,nx*ny*sizeof(double));
  */
  dat->wdat=(double*)malloc(nx*ny*sizeof(double));
  memset(dat->wdat,0,nx*ny*sizeof(double));

  return 0;
}

double simple_weight(double flux, double x, double y)
{
  return 1.0;
}

int add_moment2d_raw(moment2d *dat, double x, double y, double w, double f)
{
  double dx,dy;
  double ddx,ddy;
  double sdx,sdy;
  int dimmax,nx;
  /* tenuki */
  int gamma[]={1,1,2,6,24,120,720,5040,40320,362880,3628800};

  double s0,s1;
  double d1,ss1;
  int i,j,s,t,p,q;
  double *a,*b;
  double va,vb,v;
  int idx;
  
  dx=x-(dat->x_mean);
  dy=y-(dat->y_mean);

  a=dat->dat;
  b=dat->wdat;

  s0=dat->s;
  s1=dat->s+w; 
  d1=w/s1;

  /*
    printf("debug:%f %f %f\n",w,s1,d1);
  */

  ss1=1.-d1;

  ddx=d1*dx;

  sdx=ss1*dx;
  ddy=d1*dy;
  sdy=ss1*dy;

  dimmax=dat->dimmax;
  nx=dat->xdimmax+1;

  for (j=dat->ydimmax;j>=0;j--)
    for (i=dat->xdimmax;i>=0;i--)
      {
	if((i+j)>(dat->dimmax)) continue;
	idx=i+j*nx;

	va=0;
	vb=0;
	/* (f*w*pow(sdx,(double)i)*pow(sdy,(double)j)); */
	
	for(t=0;t<=j;t++)
	  {
	    q=j-t;
	    for(s=0;s<=i;s++)
	      {
		p=i-s;
		
		/* i!/s!/(i-s)!*j!/t!/(j-t)!*/

		v=(double)(gamma[i]/gamma[s]/gamma[p]*
			   gamma[j]/gamma[t]/gamma[q])*
		  pow(-ddx,(double)s)*pow(-ddy,(double)t);
		va+=v*a[p+q*nx];
		vb+=v*b[p+q*nx];
	      }
	  }
	v=w*pow(sdx,(double)i)*pow(sdy,(double)j);
	a[idx]=va+v;
	b[idx]=vb+f*v;
      }
  dat->x_mean+=ddx;
  dat->y_mean+=ddy;
  dat->s=s1;

  /**********************************/
  return 1;


  /* debug */
  /*
    printf("%f %f %f %f\n",
    s1,a[0],dat->x_mean,dat->y_mean);
  */
  for(j=0;j<=4;j++)
    {
      for(i=0;i<=4;i++)
	printf("%f ",dat->dat[i+5*j]);
      printf("\n");
    }
  printf("\n\n");
}

int free_moment2d (moment2d *dat)
{
  dat->initialized=0;
  free(dat->dat);
  free(dat->wdat);
  return 0;
}

int add_moment2d (moment2d *dat, double x, double y, double f)
{
  if (dat->weight==NULL)
    return add_moment2d_raw(dat,x,y,1.,1.);
  else if (dat->weight2==NULL)
    return add_moment2d_raw(dat,x,y,dat->weight(f,x,y),1.);
  else
    return add_moment2d_raw(dat,x,y,dat->weight(f,x,y),dat->weight2(f,x,y));
}

#if 0
/*** testcode ***/
int main(int argc, char **argv)
{
  moment2d a={0};
  int i,j;
  int npx=50,npy=50;
  float *g;
  int idx;
  double v;
  int imax=4,jmax=4,nmax=4;

  init_moment2d(&a,imax,jmax,nmax);
  g=(float*)malloc(npx*npy*sizeof(float));

  for (j=0; j<npy; j++)
    for (i=0; i<npx; i++)
      {
	idx=j*npx+i;
	g[idx]=(float)((double)rand()/(double)RAND_MAX)*100.0+100.0;
	/* g[idx]=1.0; */
	v=(double)g[idx];
	/* printf("%d %d %f\n",i,j,v); */
	add_moment2d_raw(&a, (double)i, (double)j, g[idx], log(g[idx]));
      }

  /*
    printf("%f %f %f\n",
	 a.dat[0],a.x_mean,a.y_mean);
  */

  for(j=0;j<=jmax;j++)
    {
      for(i=0;i<=imax;i++)
	printf("%f ",a.dat[i+(imax+1)*j]);
      printf("\n");
    }
  printf("\n\n");

  for(j=0;j<=jmax;j++)
    {
      for(i=0;i<=imax;i++)
	printf("%f ",a.wdat[i+(imax+1)*j]);
      printf("\n");
    }

}
#endif
