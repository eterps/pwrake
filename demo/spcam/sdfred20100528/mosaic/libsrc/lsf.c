#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>
#include <string.h>
/*-------------------------------------------------------------*/

/* linear fitting,
   input arrays are kept */

int lsf(double      xx[],                /* data x */
	double      yy[],                /* data y */
	int         ndata,              /* number of data */
	int         rej_sigma_start,    /* n-sigma rejection start */
	int         rej_sigma,          /* n-sigma rejection end */
	double      *a,                 /* gradient */
	double      *b)                 /* */

/*-------------------------------------------------------------*/

/* Description : data array xx[], yy[] are fitted by linear function form,
                 i.e. y = a * x + b.
		 via n-sigma rejection.

   Return Value : fit succeed : 0
                  fit failed  : -1
*/

{
  int i, j, n, rej,N;
  double x, y, sx, sy, sx2, sxy, sy2, SD=FLT_MAX;

  double *ax,*ay;
  int flag=0;

  *a=1.0;
  *b=0.0;

  if( ndata < 2 )
    {
      printf("lsf : Too few data to fit\n");
      return -1;
    }

  if ((ax=(double*)malloc(ndata*sizeof(double)))==NULL)
    {
      fprintf(stderr,"Cannot allocate memory for ax in lsf, ndat=%d\n",
	      ndata);
      return -1;
    }
  if ((ay=(double*)malloc(ndata*sizeof(double)))==NULL)
    {
      fprintf(stderr,"Cannot allocate memory for ax in lsf.\n");
      free(ax);
      return -1;
    }

  /* Yagi rewrite */
  /* if you know the Number of array, 
     you'd better use as array, not list, I believe */
  
  n = 0;
  memcpy(ax,xx,ndata*sizeof(double));
  memcpy(ay,yy,ndata*sizeof(double));
 
  N=ndata;


  for(i = rej_sigma_start; i >= rej_sigma; i--)
    {
      
/*
      printf("%d sigma rejection start\n", i);
*/
      do
	{
	  sx = 0.0;
	  sy = 0.0; 
	  sx2 = 0.0; 
	  sxy = 0.0; 
	  sy2 = 0.0; 
	  n = 0;
	  rej=0;

	  for(j=0;j<N;j++)
	    {
	      /* first path always through */
	      if( flag==0 ||
		 fabs( (*a)*ax[j] + (*b) - ay[j] ) < (double)i * SD )
		{
		  n++;
		  x=ax[j]-sx;
		  y=ay[j]-sy;
		  sx+=x/(double)n;
		  sy+=y/(double)n;
		  sx2+=(double)(n-1)*x*x/(double)n;
		  sy2+=(double)(n-1)*y*y/(double)n;
		  sxy+=(double)(n-1)*x*y/(double)n;
		}
	      else
		{
		  /* reject, and not use any more */
		  ax[j]=ax[N-1];
		  ay[j]=ay[N-1];
		  rej++;
		  N--;
		  j--;
		}
	    }
	  
	  if(flag==0)
	    {
	      /* first path */
	      if (rej_sigma_start>0)
		{
		  rej=1; /* to continue */
		  flag=1;
		}
	    }

	  /* This subroutine assumes sx2!=0 ie. not 'x=const' function */
	  if (n<2 || sx2==0) 
	    {
	      free(ax);
	      free(ay);	      
/*
	      printf("n<2\n");
*/
	      return -1;
	    }	      
	  else
	    {
	      *a = sxy/sx2;
	      *b = sy-(*a)*sx; 
/*
	      printf("lsf:: a=%f b=%f\n",*a,*b);
*/	      
	      SD=sqrt(((*a)*(*a)*(sx2)
		 -2*(*a)*(sxy)
		 +(sy2))/(double)(n-1)); 
	    }

	  if( SD == 0.0 )
	    {
	      /* OK good fit */
/*
	      printf("SD==0\n");
*/
	      free(ax);
	      free(ay);
	      return 0;
	    }
/*
	    {
	      printf("n = %d : a = %f : b = %f", n, *a, *b);
	      printf("\n%f %f %f %f %f\n",sx,sy,sx2,sxy,sy2);
	      printf("%f %f %f %f\n",(*a)*(*a)*sx2,2*(*a)*sxy,sy2,(*b)*(*b));
	      printf(" : SD = %f\n", SD);
	    }	  
*/
	}
      while( rej != 0 );      
    }
  free(ax);
  free(ay);
  return 0;
}
