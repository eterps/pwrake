#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

void mos_radixsort(int n, unsigned int a[])
{
  int i, j;
  static int count[256+1];
  int nbin=256;
  int shift=0;
  int length;
  unsigned int *work;

  length=sizeof(int);
  /* 1999/09/14 debug */
  if((work=(unsigned int*)malloc(n*sizeof(unsigned int)))==NULL)
    {
      fprintf(stderr,"Cannot allocate work in radixsort\n");
      exit(-1);
    }
  
  for (j=0;j<length;j++) 
    {
      memset(count,0,sizeof(int)*nbin);
      shift=j*8; /* byte */
      for (i=0;i<n;i++) 
	count[(a[i]>>shift)%256]++;

      for (i=1; i<=nbin;i++) 
	count[i]+=count[i-1];

      for (i=n-1;i>=0;i--) 
	work[--count[(a[i]>>shift)%256]]=a[i];

      /* 1999/09/14 debug */
      memcpy(a,work,sizeof(unsigned int)*n);
    }
  free(work);
}

void shellsort(int n, float a[])
{
  int h, i, j;
  float x;

  h=1;
  while (h<=n) h=3*h+1;
  h/=9;
  while (h>0) 
    {
      for (i=h; i<n; i++) 
	{
	  x=a[i];
	  for (j=i-h; j>=0 && a[j]>x; j-=h)
	    a[j+h]=a[j];

	  a[j+h]=x;
	}
      h/=3;
     }
}


void mos_heapsort(int n,float a[])
{
  int i, j, k;
  float x;
  
  for (k=n/2-1;k>=0;k--) 
    {
      i=k;  
      x=a[i];
      j=2*i+1;
      while (j<=n-1) 
	{
	  if (j<n-1 && a[j]<a[j+1]) j++;
	  if (x >= a[j]) break;
	  a[i]=a[j];  
	  i=j;
	  j=2*i+1;
	}
      a[i]=x;
    }

  while (n>0) 
    {
      x=a[n-1];
      a[n-1]=a[0];
      n--;
      i=0;
      j=2*i+1;
      while (j<=n-1) 
	{
	  if (j<n-1 && a[j]<a[j+1]) j++;
	  if (x >= a[j]) break;
	  a[i]=a[j];  
	  i=j;
	  j=2*i+1;
	}
      a[i]=x;
    }
}

void heapsort_reverse(int n,float a[])
{
  int i, j, k;
  float x;
  
  for (k=n/2-1;k>=0;k--) 
    {
      i=k;  
      x=a[i];
      j=2*i+1;
      while (j<=n-1) 
	{
	  if (j<n-1 && a[j]>a[j+1]) j++;
	  if (x <= a[j]) break;
	  a[i]=a[j];  
	  i=j;
	  j=2*i+1;
	}
      a[i]=x;
    }

  while (n>0) 
    {
      x=a[n-1];
      a[n-1]=a[0];
      n--;
      i=0;
      j=2*i+1;
      while (j<=n-1) 
	{
	  if (j<n-1 && a[j]>a[j+1]) j++;
	  if (x <= a[j]) break;
	  a[i]=a[j];  
	  i=j;
	  j=2*i+1;
	}
      a[i]=x;
    }
}

void heapsort2(int n,float key[],float val[])
{
  int i, j, k;
  float key0;
  float val0;
  
  for (k=n/2-1;k>=0;k--) 
    {
      i=k;  

      key0=key[i];
      val0=val[i];

      j=2*i+1;

      while (j<=n-1) 
	{
	  if (j<n-1 && key[j]<key[j+1]) j++;
	  if (key0 >= key[j]) break;

	  /* swap */
	  key[i]=key[j];
	  val[i]=val[j];

	  i=j;
	  j=2*i+1;
	}
      key[i]=key0;
      val[i]=val0;
    }

  while (n>0) 
    {
      key0=key[n-1];
      key[n-1]=key[0];

      val0=val[n-1];
      val[n-1]=val[0];
      
      n--;
      i=0;
      j=2*i+1;
      while (j<=n-1) 
	{
	  if (j<n-1 && key[j]<key[j+1]) j++;
	  if (key0 >= key[j]) break;
	  
	  key[i]=key[j];  
	  val[i]=val[j];  
	  
	  i=j;
	  j=2*i+1;
	}
      key[i]=key0;
      val[i]=val0;
    }
}

float floatmin(int ndat,float *dat)
 {
  int i;
  float fmin;
  fmin=dat[0];
  for(i=1;i<ndat;i++)
    if(dat[i]<fmin) fmin=dat[i];
  return fmin;
}

float floatmax(int ndat,float *dat)
 {
  int i;
  float fmax;
  fmax=dat[0];
  for(i=1;i<ndat;i++)
      if(dat[i]>fmax) fmax=dat[i];
  return fmax;
}

float nth(int ndat,float *dat,float n)
 {
  float a,temp;
  int i,nmax;
  float s;
  int nsame;

  if(n>=ndat-1) return dat[ndat-1];
  if (ndat==1 || n<0) return dat[0];
  if (ndat<=2)
    {
      if(dat[0]<dat[1])
	return (dat[0]*(1.-n)+n*dat[1]);
      else
	return (dat[1]*(1.-n)+n*dat[0]);
    }    

  /* quick sort! */
  a=dat[0];
  nmax=ndat;
  nsame=1;

  for(i=1;i<nmax;i++)
    {
      if(dat[i]>a)
	{
	  /* swap */
	  temp=dat[i];
	  dat[i]=dat[nmax-1];
	  dat[nmax-1]=temp;
	  nmax--;
	  i--;
	}  
      else if(dat[i]==a) 
	{
	  temp=dat[i];
	  dat[i]=dat[nsame];
	  dat[nsame]=temp;
	  nsame++;
	}
    }

  if(nmax==ndat)
    {
      /* 1999/04/08 */
      if (nmax-nsame<=n)
	{
	  return a;
	}
      else if(nmax-nsame-1>n)
	{
	  return nth(nmax-nsame,dat+nsame,n);
	}
      else
	{
	  /* ans is {nmax-nsame-1,(nmax-nsame=dat[0])}*/
	  s=n-(nmax-nsame-1);
	  return (floatmax(nmax-nsame,dat+nsame)*(1.-s)+s*dat[0]);	  
	}
    } 

  if (n<nmax-nsame-1) 
    {
      return nth(nmax-nsame,dat+nsame,n);    
    }
  else if (n<nmax-nsame)
    {
      s=n-(nmax-nsame-1);
      return (floatmax(nmax-nsame,dat+nsame)*(1.-s)+s*dat[0]);	  
    }
  else if (n<=nmax-1)
    {
      return a;
    }

  else if(n<nmax)
    {
      /* nmax-1< | n<nmax  */
      s=n-(nmax-1);
      return (dat[0]*(1.-s)+s*floatmin(ndat-nmax,dat+nmax));
    }
  else 
    {
      return nth(ndat-nmax,dat+nmax,n-nmax);
    }
}
