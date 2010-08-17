#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "statd.h"

int doubleweightedmeanrms(int ndat, double *dat, double *weight,
			 double *mean, double *sigma)
{
  int i;
  double x;
  double sx=0;
  double sx2=0;
  double w=0;

  for(i=0;i<ndat;i++)
    {
/*
      printf("%d %f %f \n",i,dat[i],weight[i]);
*/
      x=dat[i]-sx;
      sx+=x*(double)weight[i]/((double)weight[i]+w);
      sx2+=x*x*(double)weight[i]*w/((double)weight[i]+w);
      w+=weight[i];
    }
  *mean=sx;
  if(w>0)
    *sigma=sqrt(sx2/w);
/*
  printf("%f %f %f\n",*mean,*sigma,w);
*/
  return 0;
}

int doublemeanrms(int ndat, double *dat, double *mean, double *sigma)
{
  int n=0,i;
  double x;
  double sx=0;
  double sx2=0;
  for(i=0;i<ndat;i++)
    {
      n++;
      x=dat[i]-sx;
      sx+=x/(double)n;
      sx2+=(double)(n-1)*x*x/(double)n;
    }
  *mean=sx;
  if(n>1)
    *sigma=sqrt(sx2/((double)(n-1)));
  return 0;
}

double doubleweightedmedian(int ndat, double *dat, double *weight)
{
  int i;
  double sw,mw;
  double *a,*w,med;

  if(ndat==1) return dat[0];

  if((a=(double*)malloc(sizeof(double)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in doubleweightedmedian, ndat=%d\n",
	      ndat);
      exit(-1);
    }
  if((w=(double*)malloc(sizeof(double)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate w in doubleweightedmedian, ndat=%d\n",
	      ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(double)*ndat);
  memcpy(w,weight,sizeof(double)*ndat);

  /* sort */
  heapsort2_d(ndat,a,w); /* sorted by dat */
  sw=0;
  for(i=0;i<ndat;i++) sw+=w[i];
  mw=0.5*sw;

  sw=0;
  med=0;
  for(i=0;i<ndat;i++)
    {
      sw+=w[i];
      if(sw>mw)
	{
	  med=a[i];
	  break;
	}
      else if(sw==mw)
	{
	  med=0.5*(a[i]+a[i+1]);
	  break;
	}
    }  
  free(a);
  free(w);
  return med; /* error!! */
}

double doublemedian(int ndat,double *dat)
{
/*Caution !!
  This routine is not quick if ndat is small.*/
  double *a,m;

  if((a=(double*)malloc(sizeof(double)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in doublemedian, ndat=%d\n",
	      ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(double)*ndat);
  m=nthd(ndat,a,(double)(ndat-1)*0.5);
  free(a);
  return m;
}

double doublequartile0(int ndat,double *dat)
{
/*Caution !!
  This routine is not quick if ndat is small.*/
  double *a,m;
/*
  printf("%d\n",ndat);
*/

  if((a=(double*)malloc(sizeof(double)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in doublequartile, ndat=%d\n",
	      ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(double)*ndat);
  m=nthd(ndat,a,(double)(ndat-1)*0.25);
  free(a);
  return m;
}

double doublequartile2(int ndat,double *dat)
{
/*Caution !!
  This routine is not quick if ndat is small.*/
  double *a,m;
/*
  printf("%d\n",ndat);
*/

  if((a=(double*)malloc(sizeof(double)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in doublequartile, ndat=%d\n",
	      ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(double)*ndat);
  m=nthd(ndat,a,(double)(ndat-1)*0.75);
  free(a);
  return m;
}

double doubleMAD(int ndat,double *dat)
{
  double med,mad;
  int i;
  double *a;
 
  if((a=(double*)malloc(sizeof(double)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in doubleMAD, ndat=%d\n",
	      ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(double)*ndat);
  med=nthd(ndat,a,(double)(ndat-1)*0.5);

  for(i=0;i<ndat;i++)
  {
    a[i]=(double)fabs(a[i]-med);
  }
  mad=nthd(ndat,a,(double)(ndat-1)*0.5);
  free(a);
  return mad;
}

double getTukey_d(int ndat,double *dat,double *med,double *mad)
{
  int niter=10;
  int n,i;
  double t,c,s;
  double u;
  double sdenom,snumer;
  double *a;

  if((a=(double*)malloc(sizeof(double)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in getTukey_d, ndat=%d\n",
	      ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(double)*ndat);
  *med=nthd(ndat,a,(double)(ndat-1)*0.5);

  for(i=0;i<ndat;i++)
  {
    a[i]=(double)fabs((double)(a[i]-(*med)));
  }

  *mad=nthd(ndat,a,((double)(ndat-1)*0.5));
  free(a);

  s=(*mad)/0.6745;
  t=(*med);
  c=6.0; /* is recommended in Numerical Recipes */

  if(*mad<0.01) return t;

  for(n=0;n<niter;n++)
    {
      snumer=0.;
      sdenom=0.;
      for(i=0;i<ndat;i++)
	{
	  u=(dat[i]-t)/(c*s);
	  if(u<1.0 && u>-1.0)
	    {
	      snumer+=dat[i]*(1.0-u*u)*(1.0-u*u);
	      sdenom+=(1.0-u*u)*(1.0-u*u);
	    }
	}
      if(sdenom!=0) 
	{
	  t=(double)(snumer/sdenom);
	}
      else
	{
	  c*=1.1;
	}
    }
  return t;
}

double Tukey_d(int ndat,double *dat)
{
  int niter=10;
  int n,i;
  double s;
  double t,u,c;
  double sdenom,snumer;

  s=doubleMAD(ndat,dat)/0.6745;
  t=doublemedian(ndat,dat);
  c=6.0; /* is recommended in Numerical Recipes */

  for(n=0;n<niter;n++)
    {
      snumer=0;
      sdenom=0;
      for(i=0;i<ndat;i++)
	{
	  u=(dat[i]-t)/(c*s);
	  if(u<1 && u>-1)
	    {
	      snumer+=dat[i]*(1.-u*u)*(1.-u*u);
	      sdenom+=(1.-u*u)*(1.-u*u);
	    }
	}
      if(sdenom!=0) 
	{
	  t=snumer/sdenom;
	}
      else
	{
	  c*=1.1;
	}
    }
  return t;
}
