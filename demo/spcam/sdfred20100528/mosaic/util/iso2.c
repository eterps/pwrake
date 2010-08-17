#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "paint_sub.h"
#include "imc.h"
#include "getargs.h"


int main(int argc,char **argv)
{
  float *g,*gtmp;
  int *map;
  int i,j;
  struct imh imhin={""},imhout={""};
  struct icom icomin={0},icomout={0};
  float x=-1.,y=-1.;

  int iy,ix;
  FILE *fp;
  int npx,npy;
  int npx2,npy2;
  int yshift,xshift;
  float IGNRVAL=-FLT_MAX;
  int pixignr=INT_MIN,pixignr0=INT_MIN;

  int ipeak,jpeak;
  float thres=-FLT_MAX;
  double *total;




  int cutsize=-1;

 char fnamin[BUFSIZ]="";
 char fnamout[BUFSIZ]="";

 char dtype[BUFSIZ]="";
 float bzero=0.0,bscale=1.0;
 int bzeroset=0;
 int fitsdtype;

 /* for getarg */
  getargopt opts[20];
  char *files[3]={NULL};
  int n=0;
  int helpflag;

  files[0]=fnamin;
  files[1]=fnamout;

  setopts(&opts[n++],"-x=", OPTTYP_FLOAT , &x,
	  "X of the object (always needed)");
  setopts(&opts[n++],"-y=", OPTTYP_FLOAT , &y,
	  "Y of the object (always needed)");
  setopts(&opts[n++],"-thres=", OPTTYP_FLOAT , &thres,
	  "threshold (always needed)");
  setopts(&opts[n++],"-size=", OPTTYP_INT , &cutsize,
	  "cutoff size (default:no cutoff)");

  setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero,
	  "bzero");
  setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale,
	  "bscale");
  setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
	  "datatyp(FITSFLOAT,FITSSHORT...)");
  setopts(&opts[n++],"-pixignr=", OPTTYP_INT , &pixignr,
	  "pixignr value");


  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamout[0]=='\0')
   {
      fprintf(stderr,"Error: No output file specified\n");
      helpflag=1;
    }
  if (x<0||y<0) 
    {
      fprintf(stderr,"Error: reference position is not set or negative\n");
      helpflag=1;
    }
  if (thres==-FLT_MAX)
    {
      fprintf(stderr,"Error: threshold is not specified\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: iso2 [option] (infilename) (outfilename)",
		 opts,"");
      exit(-1);
    }

  /*
   * ... Open Input
   */

 if ((fp=imropen(fnamin,&(imhin),&(icomin)))==NULL)
   {
     printf("Error: File \"%s\" not found.\n",fnamin);
     exit(-1);
   }
 
 npx=imhin.npx;
 npy=imhin.npy;
 ipeak=(int)floor(x+0.5);
 jpeak=(int)floor(y+0.5);

 if(cutsize<=0)
   {
     npx2=imhin.npx;
     npy2=imhin.npy;
     xshift=0;
     yshift=0;
   }
 else
   {
     npx2=cutsize;
     npy2=cutsize;
     xshift=ipeak-(cutsize/2);
     yshift=jpeak-(cutsize/2);
     ipeak-=xshift;
     jpeak-=yshift;
   }

 gtmp=(float*)malloc(sizeof(float)*npx);
 g=(float*)malloc(sizeof(float)*npy2*npx2);
 total=(double*)malloc(sizeof(double)*npy2*npx2);
 map=(int *)calloc(npy2*npx2,sizeof(int));

 /* Read */
 imc_mos_set_default_add( CONST_ADD );
 imc_mos_set_default_maxadd(2);
 pixignr0=imget_pixignr(&imhin);

 for(iy=0;iy<npy2;iy++)
   {
     if (iy+yshift<0 || iy+yshift>=npy)
       for(ix=0;ix<npx;ix++)
	 g[npx2*iy+ix]=IGNRVAL;
     else
       {
	 imrl_tor(&imhin,fp,gtmp,iy+yshift);
	 for(ix=0;ix<npx2;ix++)
	   {
	     if(xshift+ix<0 || xshift+ix>npx-1 ||
		gtmp[xshift+ix]==(float)pixignr0) 
	       g[npx2*iy+ix]=IGNRVAL;
	     else
	       g[npx2*iy+ix]=gtmp[xshift+ix];
	   }
       }
   }

 imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);
 imhout.npx=imhout.ndatx=npx2;
 imhout.npy=imhout.ndaty=npy2;
 imclose(fp,&imhin,&icomin);

 doflood(g,npx2,npy2,ipeak,jpeak,thres,map);

 for (j = 0; j < npy2; j++)
   for (i = 0; i < npx2; i++) 
     if (map[j*npx2+i]==0) 
       g[j*npx2+i]=IGNRVAL;

  /* !! need 2000/07/02 or later version of IMC */

  if(dtype[0]=='\0'||(fitsdtype=imc_fits_dtype_string(dtype))==-1)
    (void)imc_fits_get_dtype( &imhout, &fitsdtype, NULL, NULL, NULL);

  if(bzeroset!=1)
    (void)imc_fits_get_dtype( &imhout, NULL, &bzero, &bscale, NULL);

  if(imc_fits_set_dtype(&imhout,fitsdtype,bzero,bscale)==0)
    {
      printf("Error: Cannot set FITS \"%s\"\n",fnamout);
      printf("       Type %d BZERO %f BSCALE %f\n",fitsdtype,bzero,bscale);      
      exit(-1);
    }	  

  if(pixignr==INT_MIN)
    pixignr=pixignr0;

  /* replace IGNRVAL */
  for(i=0;i<npx2*npy2;i++)
    if(g[i]==IGNRVAL) g[i]=(float)pixignr;
  imset_pixignr(&imhout, &icomout, pixignr);

  /* 2002/05/13 WCS change is added */
  imc_shift_WCS(&icomout, (float)xshift, (float)yshift);
  
 if( (fp=imwopen (fnamout, &imhout, &icomout))== NULL)
   {
     /* error */
     fprintf(stderr,"Error: Cannot open the output file \"%s\"\n",fnamout);
     exit(-1);
   }

 if( !imwall_rto(&imhout,fp,g))
   {
     fprintf(stderr,"Error: Cannot write the output file \"%s\"\n",fnamout);
     /* error */
     exit(-1);
   }
 
 imclose(fp,&imhout,&icomout);
 
 return 0;
}
