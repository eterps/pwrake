#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "sortd.h"

void shellsort_d(int n, double a[])
{
  int h, i, j ;
  double x;

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

void mos_heapsort_d(int n,double a[])
{
  int i, j, k;
  double x;
  
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

void heapsort_reverse_d(int n,double a[])
{
  int i, j, k;
  double x;
  
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

void heapsort2_d(int n,double key[],double val[])
{
  int i, j, k;
  double key0;
  double val0;
  
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


int *makeidlist(int n)
{
  int *id;
  int i;
  id=(int*)malloc(n*sizeof(int));
  for(i=0;i<n;i++) id[i]=i;
  return id;
}

void heapsort2id(int n, double key[], int id[])
{
  int i, j, k;
  double key0;
  int id0;
  
  for (k=n/2-1;k>=0;k--) 
    {
      i=k;  

      key0=key[i];
      id0=id[i];

      j=2*i+1;

      while (j<=n-1) 
	{
	  if (j<n-1 && key[j]<key[j+1]) j++;
	  if (key0 >= key[j]) break;

	  /* swap */
	  key[i]=key[j];
	  id[i]=id[j];

	  i=j;
	  j=2*i+1;
	}
      key[i]=key0;
      id[i]=id0;
    }

  while (n>0) 
    {
      key0=key[n-1];
      key[n-1]=key[0];

      id0=id[n-1];
      id[n-1]=id[0];
      
      n--;
      i=0;
      j=2*i+1;
      while (j<=n-1) 
	{
	  if (j<n-1 && key[j]<key[j+1]) j++;
	  if (key0 >= key[j]) break;
	  
	  key[i]=key[j];  
	  id[i]=id[j];  
	  
	  i=j;
	  j=2*i+1;
	}
      key[i]=key0;
      id[i]=id0;
    }
}


void id_reorder(int ndat,int id[],size_t siz,void *data)
{
  int *rid;
  int i,wid;
  char *work;
  
  rid=(int*)malloc(ndat*sizeof(int));
  for(i=0;i<ndat;i++)
    {
      rid[i]=i;
    }

  work=(char*)malloc(siz);
  wid=-1;
  i=0;
  
  for(i=0;i<ndat;i++)
    {
      while(id[i]!=rid[i])
	{
	  if(wid==-1)
	    {
	      /*
	      printf("debug: mv %d -> work\n",i);
	      */
	      wid=i;
	      memcpy(work,(char*)data+i*siz,siz);
	      /*
		printf("debug: mv %d -> %d\n",id[i],i);
	      */
	      memcpy((char*)data+i*siz,(char*)data+id[i]*siz,siz);
	      rid[i]=id[i];
	      i=id[i];
	    }
	  else
	    {
	      if(id[i]==wid)
		{
		  /*
		    printf("debug: mv work -> %d\n",i);
		  */
		  memcpy((char*)data+i*siz,work,siz);
		  rid[i]=wid;
		  wid=-1;
		  break;
		}
	      else
		{
		  /*
		    printf("debug: mv %d -> %d\n",id[i],i);
		  */
		  memcpy((char*)data+i*siz,(char*)data+id[i]*siz,siz);
		  rid[i]=id[i];
		  i=id[i];
		}
	    }
	}
    }
  free(work);
  free(rid);
}

double doublemin(int ndat,double *dat)
 {
  int i;
  double fmin;
  fmin=dat[0];
  for(i=1;i<ndat;i++)
    if(dat[i]<fmin) fmin=dat[i];
  return fmin;
}

double doublemax(int ndat,double *dat)
 {
  int i;
  double fmax;
  fmax=dat[0];
  for(i=1;i<ndat;i++)
      if(dat[i]>fmax) fmax=dat[i];
  return fmax;
}

double nthd(int ndat,double *dat,double n)
 {
  double a,temp;
  int i,nmax;
  double s;
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
	  return nthd(nmax-nsame,dat+nsame,n);
	}
      else
	{
	  /* ans is {nmax-nsame-1,(nmax-nsame=dat[0])}*/
	  s=n-(nmax-nsame-1);
	  return (doublemax(nmax-nsame,dat+nsame)*(1.-s)+s*dat[0]);	  
	}
    } 

  if (n<nmax-nsame-1) 
    {
      return nthd(nmax-nsame,dat+nsame,n);    
    }
  else if (n<nmax-nsame)
    {
      s=n-(nmax-nsame-1);
      return (doublemax(nmax-nsame,dat+nsame)*(1.-s)+s*dat[0]);	  
    }
  else if (n<=nmax-1)
    {
      return a;
    }

  else if(n<nmax)
    {
      /* nmax-1< | n<nmax  */
      s=n-(nmax-1);
      return (dat[0]*(1.-s)+s*doublemin(ndat-nmax,dat+nmax));
    }
  else 
    {
      return nthd(ndat-nmax,dat+nmax,n-nmax);
    }
}
