#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "imc.h"

int   imstat(const float	*pix,	/* pixel data array */
	     const int	pdim,	/* pixel data size */
	     const int	nrej,	/* how many times perform rejection */
	     int	*num,		/* number of non-rejected pixels */
	     float	*mean,		/* mean value of sky */
	     float	*sigma,		/* sigma value of sky */
	     float      pixignr )
{
  char	*flag;		/* rejection flag */
  int	i;		/* loop counter */
  int	times = 0;		/* rejection times counter */
  double Sx, Sxx;	/* sum of data */
  float x;

  /* Allocation of rejection flag array */
  /* Initialize rejection flag array */
  flag = (char*) calloc(pdim, sizeof(char));

  if(flag==NULL)
    {
      fprintf(stderr,"Cannot allocate memory for flag in imstat\n");
      exit(-1);
    }
  
  /* 1999/09/14 yagi revised */

  /* Calculate mean and sigma value of sky */
  
  while(1) 
    {
      *num = 0;			/* Initialization */
      Sx = 0.0;	Sxx = 0.0;	/* Initialization */
      /* Calculation */
      for (i = 0; i < pdim; i++) 
	{
	  if((pix[i]==pixignr)||
	     (times!=0 && fabs(pix[i]-(*mean)) > 2.0*(*sigma)))
	    {
	      flag[i]=1;
	    }
	  else if(flag[i]==0)
	    {
	      (*num)++;
	      x=pix[i]-Sx;
	      Sx+=x/(float)(*num);
	      Sxx+=(float)((*num)+1)*x*x/(float)(*num);
	    }	
	}
      
      if((*num)>1)
	{      
	  *mean = Sx;
	  *sigma = sqrt(Sxx/ (float)((*num) - 1));
	}
      else
	{
	  *mean=0;
	  *sigma=-1.0;
	}

      /* ????? why fabs(*mean) < 0.1 ?????? */

      if (fabs(*mean) < 0.1 || times == nrej) break;
      
      /* Rejection */
      times++;
    }	/* while */
  
  /* Release rejection flag array */
  free(flag);
  if (*sigma<0) return 1; /* NG */
  else return 0; /* OK */
}


float *imextract2(float *pix, /* image pixel data array */
	       int npx,   /* image data size */
	       int npy,
	       int x0,	  /* left-bottom corner to extract */
	       int y0,
	       int x1,	  /* right-top corner to extract */
	       int y1)
{
  static float *tmp;
  int j;
  int npx2,npy2;

  if(x0<0||y0<0||x1>=npx||y1>=npy) return NULL;

  npx2=x1-x0+1;
  npy2=y1-y0+1;

  tmp=(float*)malloc(npx2*npy2*sizeof(float));
  if (tmp==NULL) return NULL;
  
  for(j=y0;j<=y1;j++)
    memcpy(tmp+(j-y0)*npx2,pix+(x0+j*npx),npx2*sizeof(float));

  /* tmp should be freed by caller */
  return tmp;
}

int	imextra(const float *pix, /* image pixel data array */
		const int npx,   /* image data size */
		const int npy,
		const int x0,	  /* left-bottom corner to extract */
		const int y0,
		const int x1,	  /* right-top corner to extract */
		const int y1,
		float *tmp)	  /* temporary array to store extracted image */
{
  int	j;		/* loop counter */

  int	dx, dy;		/* dimension of extracted data array */

  int ix0,ix1,iy0,iy1;

  dx = x1-x0+1;
  dy = y1-y0+1;

  if(npx<=0||npy<=0||dx<=0||dy<=0) return -1; /* error */

  /* set 0 to all blank field */
  memset(tmp,0,dx*dy*sizeof(float));

  /* 1999/09/14 Yagi */

  if (x0<0) ix0=0; else ix0=x0;
  if (x1>=npx) ix1=npx-1; else ix1=x1;

  /* PixIgnore is ignored here ... */
  if(ix1-ix0+1>0)
    {
      if (y0<0) iy0=0; else iy0=y0;
      if (y1>=npy) iy1=npy-1; else iy1=y1;

      for (j = 0; j <= iy1-iy0; j++) 
	memcpy(tmp+(j+iy0-y0)*dx+(ix0-x0),pix+ix0+npx*(iy0+j),(ix1-ix0+1)*sizeof(float));
      return (ix1-ix0+1)*(iy1-iy0+1);
    }
  else
    return -1; /* error */
}


/* pixel which does not construct a object will be assigned 0 */
int clean_pix(float *pix,	  /* pixel array */
	  const int npx, /* pixel array extent */
	  const int npy,
	  const int *map) /* object 1/0 map */
{
  int	i, j;
  int	num;

  /* needed, or rewrite detect_sub.c ... */

  for (j = 0; j < npy; j++)
    for (i = 0; i < npx; i++) 
      {
	num = j * npx + i;
	if (map[num] == 0)
	  pix[num] = 0.0;
      }
  return 0;
}


int 	skydet(const float *pix,	 /* (I) pixel array  */
	       const int npx,           /* (I) pixel array extent */
	       const int npy,           /* (I) pixel array extent */
	       int *nsky,	         /* (O) number of pixels used */
	       float *sky_mean,	         /* (O) mean value of sky  */
	       float *sky_sigma,
	       float pixignr)         /* (O) rms value of sky  */
{
  int	i,index=0;
  float	tmp[10000];

  int	dx, dy;
  int	npix[4];
  float	mean[4], sigma[4];
  int tmpdim=0;
  
  if (npx > 200)
    dx = (npx - 200) / 3;
  else
	dx = 0;
  if (npy > 200)
    dy = (npy - 200) / 3;
  else
    dy = 0;
  printf("%5d, %5d\n", dx, dy);
  for (i = 0; i < 4; i++) {
    switch (i) {
    case 0 :
      tmpdim = imextra(pix, npx, npy,
		       dx, dy, dx+99, dy+99, tmp);
      break;
    case 1 :
      tmpdim = imextra(pix, npx, npy,
		       npx-dx-100, dy, npx-1-dx, dy+99, tmp);
      break;
    case 2 :
      tmpdim = imextra(pix, npx, npy,
		       dx, npy-dy-100, dx+99, npy-1-dy, tmp);
      break;
    case 3 :
      tmpdim = imextra(pix, npx, npy,
		       npx-dx-100, npy-dy-100, npx-1-dx, npy-1-dy, tmp);
      break;
    }
    imstat(tmp, tmpdim, 3, &npix[i], &mean[i], &sigma[i], pixignr);
    printf("%3d : Nsky = %d, Mean = %f, Sigma = %f\n",
	   i, npix[i], mean[i], sigma[i]);
  }

  /* 1999/09/14 */
  *sky_sigma=FLT_MAX;
  for (i = 0; i < 4; i++)
    {
      if (sigma[i] > 0.0 && sigma[i] < *sky_sigma)
	{
	  *sky_sigma = sigma[i];
	  index = i;
	}
    }
  *nsky = npix[index];
  *sky_mean = mean[index];

  if ((*sky_sigma)==FLT_MAX) return 1 ; /* NG */
  else return 0; /* OK */
}
