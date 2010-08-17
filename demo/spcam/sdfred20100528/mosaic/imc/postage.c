#include <stdio.h>
#include "imc.h"

void makePostage(char *filename,float *pix,int npx,int npy)
{
  FILE *fp;
  struct imh imhout={""};
  struct icom icomout={0};
  int iy;
  icomout.ncom=0;
  imhout.npx   = npx;
  imhout.npy   = npy;
  imhout.ndatx = imhout.npx;
  imhout.ndaty = imhout.npy;
  imhout.dtype= -1;
  imc_fits_set_dtype(&imhout,FITSFLOAT,0.0,1.0);
  fp=imwopen(filename,&imhout,&icomout);
  for( iy=0; iy<npy; iy++ ) 
    {
      (void) imwl_rto( &imhout, fp, pix+npx*iy,iy);
    }
  imclose(fp,&imhout,&icomout);
}
