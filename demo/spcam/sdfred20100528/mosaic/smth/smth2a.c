/* ======================================================

                        SMTH.C


         ------------------------------------

                  under development

                      1993.01.13
		      1995.10.30 minor change by Yagi
		        (error message to stderr, etc. )
                      1996.07.16 deal with pixignr by kashik
         ------------------------------------
    problems :> 1) edge treatment variation
                2) filter figures
   ====================================================== */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "imc.h"

#define sqr(x)  ((x) * (x))

#define LMAX          40     /* max char string length */
#define PTPARAM      25.     /* threshold for peak/tail of gf */


/* ------------------------------------------------------ */
/*                                                        */
/*                      functions                         */
/*                                                        */
/* ------------------------------------------------------ */

/* ------------------------------------------------------ */

float* gausflt (float sgm,   /* rms of gausian */
                int  size   /* filter size (odd)*/
               )  /* gaussian filter */

{
   int   i,ix,iy; /* do loop */
   double total; /* sum of all components */
   float *gsf; /* gaussian filter array */
   int xc,yc;
   double rad2,sgm2,rad2max;
   double a; 

   xc=yc=(size-1)/2;
   rad2max=xc*xc+1;
   sgm2=sgm*sgm;
   a=-0.5/sgm2;

   total=0.;
   gsf = (float*) calloc(size*size, sizeof(float));

   for (iy=0; iy<size; iy++)
     {
       for (ix=0; ix<size; ix++)
	 {
	   rad2=(double)((ix-xc)*(ix-xc)+(iy-yc)*(iy-yc));
	   if(rad2<rad2max)
	     {
	       gsf[iy*size+ix]=(float)exp(a*rad2); 
	       total+=gsf[iy*size+ix];
	     }
	 }
     }

   /* normalize */
   if (total<=0) 
     {/* error */}
   else
     for (i=0; i<size*size; i++)
       gsf[i]/=total;
   
   return gsf;
}

/* ------------------------------------------------------ */

float edge(int px,         /* pixel ID */
           int py,         /* pixel ID */
           int fsize,     /* filter size */
           int xdim,      /* x-dimension pix of image */
           int ydim,      /* y-dimension pix of image */ 
           float *ipix,    /* input pixel array */
           float *gft,     /* gaussian filter */
           int pixignr   /* pix to be ignore */
          ) /* renormalize the filter on the frame edge*/ 

{
   int   ix,iy; /* do loop */
   float total; /* total filter value of outside */
   float v; /* return value */
   int x,y;

   /* 2001/02/23 rewritten by Yagi for speed up */
   total=0.;
   v=0.;

   for (iy=0; iy<fsize; iy++)
     {
       for (ix=0; ix<fsize; ix++)
	 {
	   x=(px-(fsize-1)/2+ix);
	   y=(py-(fsize-1)/2+iy);
	   if (x<0|| y<0 || x >= xdim || y>= ydim )
	     total+=gft[iy*fsize+ix];
	   /* add nakata 1999/10/31 */
	   /* modified by Yagi 1999/10/31 */
	   else if (ipix[x+y*xdim]==(float)pixignr)
	     return (float)pixignr;
	   else 
	     v+=ipix[x+y*xdim]*gft[iy*fsize+ix];
	 } 
     }

   if (total<1.0)
     return v/(1.0-total);
   else
     return (float)pixignr;
 }

int convolve2(int xdim,int ydim,
	      float *ipix, float *opix, 
	      int fsize, float *gf,
	      float pixignr)
{
  int iy, ix;
  int ify, ifx;
  int ip,ip0;
  int ig,ig0,ip2,ip20;
  int siz;
  int xmin,xmax,ymin,ymax;

  siz=(fsize-1)/2;
  xmin=ymin=siz;
  xmax=xdim-siz;
  ymax=ydim-siz;

  for (iy=0; iy<ymin; iy++) 
    {
      ip0=iy*xdim;
      for (ix=0; ix<xdim; ix++) 
	{
	  ip=ip0+ix;
	  opix[ip]=(float) edge(ix,iy,fsize,xdim,ydim,ipix,gf,pixignr);
	}
    }

  for (iy=ymax; iy<ydim; iy++) 
    {
      ip0=iy*xdim;
      for (ix=0; ix<xdim; ix++) 
	{
	  ip=ip0+ix;
	  opix[ip]=(float) edge(ix,iy,fsize,xdim,ydim,ipix,gf,pixignr);
	}
    }

  for (iy=ymin; iy<ymax; iy++) 
    {
      ip0=iy*xdim;
      for (ix=0; ix<xmin; ix++) 
	{
	  ip=ip0+ix;
	  opix[ip]=(float) edge(ix,iy,fsize,xdim,ydim,ipix,gf,pixignr);
	}
      for (ix=xmax; ix<xdim; ix++) 
	{
	  ip=ip0+ix;
	  opix[ip]=(float) edge(ix,iy,fsize,xdim,ydim,ipix,gf,pixignr);
	}

      for (ix=xmin; ix<xmax; ix++) 
	{
	  /* 1999/06/16 
	     pixignr is not edge any longer */
	  ip=ip0+ix;
	  for (ify=0; ify<fsize; ify++) 
	    {
	      ip20=(iy-siz+ify)*xdim;
	      ig0=ify*fsize;
	      for (ifx=0; ifx<fsize; ifx++) 
		{
		  ig=ig0+ifx;
		  if(gf[ig]<=0)continue;
		  ip2=ip20+(ix-siz+ifx);
		  if(ipix[ip2]==(float)pixignr)
		    {
		      opix[ip]=(float)pixignr;
		      ify=fsize;
		      break;
		    }
		  opix[ip]+=ipix[ip2]*gf[ig];
		}		
	    } 	 
	}
    }
  return 0;
}





int main(int argc, char *argv[])
{
/*                                     ----- declarations */




   int         ifx,ify;          /* do loop for filter */


   int        xdim, ydim;       /* x,y-pixel data size */
   int        pdim;             /* total-pixel data size */
   int        fsize;            /* filter size (pixel)*/

   float       *ipix;            /* input pixel array */
   float       *opix;            /* output pixel array */
   float       gsgm;             /* gaussian sigma */
   float       *gf;              /* gaussian filter array */

   struct imh  imhin={""};             /* imh header structure */
   struct icom icomin={0};             /* imh comment structure */

   struct imh  imhout={""};             /* imh header structure */
   struct icom icomout={0};             /* imh comment structure */

   /* for empty pixels (WHT) */
   int pixignr; 

   /* define struct for file pointers */
   FILE *fpin,*fpout;

   /* define struct for file names */
   char *fnamin,*fnamout;



   if(argc!=4)
     {
       printf("Usage: smth2 file_in filtersigma file_out\n");
       exit(-1);
     }
   else
     {
       fnamin=argv[1];
       gsgm=atof(argv[2]);
       fnamout=argv[3];
     }

   if ((fpin = imropen(fnamin, &imhin, &icomin)) == NULL)
	{
	  fprintf(stderr,"\7\n Cannot open input file %s\n",fnamin);
	  exit(1);
	}
      xdim  = imhin.npx;
      ydim  = imhin.npy;
      pdim = xdim * ydim;
      ipix = (float*)malloc(pdim*sizeof(float));
      
      printf("\n> %s reading\n",fnamin);    
      if (imrall_tor(&imhin, fpin, ipix, pdim) == 0)
	{
	  fprintf(stderr,"\7\n Cannot read file \n");     
	  exit(1);
	}
   pixignr = imget_pixignr( &imhin );
   printf("smth:  %s PixIgnor= %d\n",fnamin,pixignr);
   
   /*                          ----- make a smoothing filter */
   
   /* .... rather fatal mistake is found (2001/02/23) */
   /* fsize= 2*(int)ceil(gsgm*sqrt(log(PTPARAM)))-1; */
   fsize= 2*(int)ceil(gsgm*sqrt(2.0*log(PTPARAM)))-1;

   imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);
   imclose(fpin,&imhin,&icomin);

   if (fsize <= 1) /* add nakata 1999/10/31 */
     {
       printf("\7\n *input seeing is near to output one \n");
       printf(" there is no need to smooth the image \n");
       opix=ipix;
     }
   else
     {
       printf("\n> gaussian filter sgm  = %f pix\n",gsgm);
       printf("                  size = %d pix\n",fsize);
       
       gf = (float*) gausflt (gsgm, fsize);
       
       printf("\n> filter map \n");
       for (ify=0; ify<fsize; ify++) 
	 {
	   for (ifx=0; ifx<fsize; ifx++) 
	     printf(" %7.5f",gf[ify*fsize+ifx]);
	   printf("\n");
	 }      
   
   /*                               ----- smoothing the file */
       printf("\n> %s smoothing\n",fnamin);
       opix = (float*) malloc(pdim*sizeof(float));
       
       /* Convolve */
       
       convolve2(xdim,ydim,ipix, opix, fsize, gf,pixignr);
       
       free(ipix);
       free(gf);
       imaddhistf(&icomout,"SMTH2: filter sgm=%4.2f: size=%d",gsgm,fsize);
     }

   /*                                ----- write output file */
   /* 2002/06/06 updated */
   /*
     (void) sprintf( line, "SMTH2:  filter sgm=%4.2f: size=%d",gsgm,fsize);
     (void) imaddcom( &icomout, line );
   */


   /*
     (void) sprintf( line, "        original sgm=%4.2f: resulted sgm=%4.2f",
     isee[nflm],osee);
     (void) imaddcom( &icomout, line );
   */
   
   if( (fpout=imwopen(fnamout, &imhout, &icomout)) == NULL )
     {
       fprintf(stderr,"Cannot open output file \n");
       exit(1);
     }
   
   printf("\n> %s writing --------------\n",fnamout); 
   imwall_rto(&imhout, fpout, opix);
   imclose(fpout,&imhout,&icomout);
   free(opix);
   /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
   return 0;
}  

