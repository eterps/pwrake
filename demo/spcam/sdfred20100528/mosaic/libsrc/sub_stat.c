#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "stat.h"

int floatweightedmeanrms(int ndat, float *dat, float *weight,
			 float *mean, float *sigma)
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

int floatmeanrms(int ndat, float *dat, float *mean, float *sigma)
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
    *sigma=sqrt(sx2/(n-1));
  return 0;
}

float floatweightedmedian(int ndat, float *dat, float *weight)
{
  int i;
  float sw,mw;
  float *a,*w,med;

  if(ndat==1) return dat[0];

  if((a=(float*)malloc(sizeof(float)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in floatweightedmedian, ndat=%d\n",
	      ndat);
      exit(-1);
    }
  if((w=(float*)malloc(sizeof(float)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate w in floatweightedmedian, ndat=%d\n",
	      ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(float)*ndat);
  memcpy(w,weight,sizeof(float)*ndat);

  /* sort */
  heapsort2(ndat,a,w); /* sorted by dat */
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

float floatmedian(int ndat,float *dat)
{
/*Caution !!
  This routine is not quick if ndat is small.*/
  float *a,m;

  if((a=(float*)malloc(sizeof(float)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in floatmedian, ndat=%d\n",ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(float)*ndat);
  m=nth(ndat,a,(float)(ndat-1)*0.5);
  free(a);
  return m;
}

float floatquartile0(int ndat,float *dat)
{
/*Caution !!
  This routine is not quick if ndat is small.*/
  float *a,m;
/*
  printf("%d\n",ndat);
*/

  if((a=(float*)malloc(sizeof(float)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in floatquartile, ndat=%d\n",ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(float)*ndat);
  m=nth(ndat,a,(float)(ndat-1)*0.25);
  free(a);
  return m;
}

float floatquartile2(int ndat,float *dat)
{
/*Caution !!
  This routine is not quick if ndat is small.*/
  float *a,m;
/*
  printf("%d\n",ndat);
*/

  if((a=(float*)malloc(sizeof(float)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in floatquartile, ndat=%d\n",ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(float)*ndat);
  m=nth(ndat,a,(float)(ndat-1)*0.75);
  free(a);
  return m;
}

float floatMAD(int ndat,float *dat)
{
  float med,mad;
  int i;
  float *a;
 
  if((a=(float*)malloc(sizeof(float)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in floatMAD, ndat=%d\n",ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(float)*ndat);
  med=nth(ndat,a,(float)(ndat-1)*0.5);

  for(i=0;i<ndat;i++)
  {
    a[i]=(float)fabs(a[i]-med);
  }
  mad=nth(ndat,a,(float)(ndat-1)*0.5);
    free(a);
  return mad;
}

float getTukey(int ndat,float *dat,float *med,float *mad)
{
  int niter=10;
  int n,i;
  float t,c,s;
  float u;
  double sdenom,snumer;
  float *a;

  if((a=(float*)malloc(sizeof(float)*ndat))==NULL)
    {
      fprintf(stderr,"Cannot allocate a in getTukey, ndat=%d\n",ndat);
      exit(-1);
    }

  memcpy(a,dat,sizeof(float)*ndat);
  *med=nth(ndat,a,(float)(ndat-1)*0.5);

  for(i=0;i<ndat;i++)
  {
    a[i]=(float)fabs((double)(a[i]-(*med)));
  }

  *mad=nth(ndat,a,((float)(ndat-1)*0.5));
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
	  t=(float)(snumer/sdenom);
	}
      else
	{
	  c*=1.1;
	}
    }
  return t;
}

float Tukey(int ndat,float *dat)
{
  int niter=10;
  int n,i;
  float s;
  float t,u,c;
  double sdenom,snumer;

  s=floatMAD(ndat,dat)/0.6745;
  t=floatmedian(ndat,dat);
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
