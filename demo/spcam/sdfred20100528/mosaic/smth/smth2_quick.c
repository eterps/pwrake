/* ======================================================

                        SMTH.C


         ------------------------------------

                  under development

                      1993.01.13
		      1995.10.30 minor change by Yagi
		        (error message to stderr, etc. )
                      1996.07.16 deal with pixignr by kashik
		      1997.10.01 1d method applied
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

#define LMAX          40     /* max char string length */
/*#define TPARAM  (0.5/0.75)   *//* translate from seeing to gsgm */
/*#define PTPARAM      10.     *//* threshould for peak/tail of gf */
#define PTPARAM      10000.     /* threshould for peak/tail of gf */

#define FMAX          21     /* max size of smoothing filter */

float*  gausflt (float, int);
float*  gausflt_1d (float, int);
float   edge_x_1d(int,int,int,int,int,float*,float*,int);
float   edge_y_1d(int,int,int,int,int,float*,float*,int);

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
   int   ix,iy; /* do loop */

   double total; /* sum of all components */
   float *gsf; /* gaussian filter array */
   int xc,yc;
   float rad2,sgm2;
   
   xc=yc=(size-1)/2;
   sgm2=sgm*sgm;

   total=0.;
   gsf = (float*) calloc(size*size, sizeof(float));
   if(gsf==NULL) 
     {
       fprintf(stderr,"Cannot allocate gsf\n");
       exit(-1);
     }


   for (iy=0; iy<size; iy++)
     {
       for (ix=0; ix<size; ix++)
	 {
	   rad2=(float)((ix-xc)*(ix-xc)+(iy-yc)*(iy-yc));
	   if(rad2<(xc*xc+1))
	     {
	       gsf[iy*size+ix]=(float)exp(-0.5*rad2/sgm2); 
	       total=total+gsf[iy*size+ix];
	     }
	 }
     }
   
   for (iy=0; iy<size; iy++)
     {
       for (ix=0; ix<size; ix++)
	 {
	   gsf[iy*size+ix]=gsf[iy*size+ix]/total;
	 }
     }
   return gsf;
}


float* gausflt_1d (float sgm,   /* rms of gausian */
		   int  size   /* filter size (odd)*/
		   )  /* gaussian filter */

{
   int   ix; /* do loop */

   double total; /* sum of all components */
   float *gsf; /* gaussian filter array */
   int xc;
   float rad2,sgm2;
   
   xc=(size-1)/2;
   sgm2=sgm*sgm;

   total=0.;
   gsf = (float*) calloc(size, sizeof(float));
   if(gsf==NULL) 
     {
       fprintf(stderr,"Cannot allocate gsf\n");
       exit(-1);
     }


   for (ix=0; ix<size; ix++)
     {
       rad2=(float)((ix-xc)*(ix-xc));
       gsf[ix]=(float)exp(-0.5*rad2/sgm2); 
       total=total+gsf[ix];	 	 
     }
   
   for (ix=0; ix<size; ix++)
     {
       gsf[ix]=gsf[ix]/total;
     }     
   return gsf;
}

/* ------------------------------------------------------ */

float edge_x_1d(int px,         /* pixel ID */
           int py,         /* pixel ID */
           int fsize,     /* filter size */
           int xdim,      /* x-dimension pix of image */
           int ydim,      /* y-dimension pix of image */ 
           float *ipix,    /* input pixel array */
           float *gft,     /* gaussian filter */
           int pix_ignr   /* pix to be ignore */
          ) /* renormalize the filter on the frame edge*/ 

{
   int   ix; /* do loop */
   float total; /* total filter value of outside */
   float *wgft; /* temporary gaussian filter */
   float y; /* return value */

   total=0.;
   y=0.;

   wgft = (float*) calloc(fsize, sizeof(float));
   if(wgft==NULL) 
     {
       fprintf(stderr,"Cannot allocate wgft\n");
       exit(-1);
     }

   memcpy( wgft, gft,
	  (size_t)(fsize*sizeof(float)/sizeof(char))); 

   for (ix=0; ix<fsize; ix++)
     {
       if ( (px-(fsize-1)/2+ix) < 0    || 
	   (px-(fsize-1)/2+ix) >= xdim || 
	   ipix[(px-(fsize-1)/2+ix)+py*xdim]==pix_ignr)
	 { 
	   total=total+wgft[ix];
	   wgft[ix]=0.;
	 } 
     }
   
   
   for (ix=0; ix<fsize; ix++)
     {
       if ((px-(fsize-1)/2+ix)+py*xdim < 0)
	 continue;
       else if ((px-(fsize-1)/2+ix)+py*xdim >= xdim * ydim)
	 continue;
       else
	 y+=ipix[(px-(fsize-1)/2+ix)+py*xdim]
	   *wgft[ix]/(1.-total);
     }    
   free(wgft);  
 
   return y;
}

float edge_y_1d(int px,         /* pixel ID */
           int py,         /* pixel ID */
           int fsize,     /* filter size */
           int xdim,      /* x-dimension pix of image */
           int ydim,      /* y-dimension pix of image */ 
           float *ipix,    /* input pixel array */
           float *gft,     /* gaussian filter */
           int pix_ignr   /* pix to be ignore */
          ) /* renormalize the filter on the frame edge*/ 
{
   int   iy; /* do loop */
   float total; /* total filter value of outside */
   float *wgft; /* temporary gaussian filter */
   float y; /* return value */

   total=0.;
   y=0.;

   wgft = (float*) calloc(fsize*fsize, sizeof(float));
   if(wgft==NULL) 
     {
       fprintf(stderr,"Cannot allocate wgft\n");
       exit(-1);
     }

   memcpy( wgft, gft,
	  (size_t)(fsize*sizeof(float)/sizeof(char))); 

   for (iy=0; iy<fsize; iy++){
         if ( (py-(fsize-1)/2+iy) < 0    ||
              (py-(fsize-1)/2+iy) >= ydim ||
              ipix[px+(py-(fsize-1)/2+iy)*xdim]==pix_ignr)
	   { 
	     total=total+wgft[iy];
	     wgft[iy]=0.;
	   } 
       }

   for (iy=0; iy<fsize; iy++){
     if (px+(py-(fsize-1)/2+iy)*xdim < 0)
       continue;
     else if (px+(py-(fsize-1)/2+iy)*xdim >= xdim * ydim)
       continue;
     else
       y=y+ipix[px+(py-(fsize-1)/2+iy)*xdim]
	 *wgft[iy]/(1.-total);   
   }    

   free(wgft);  
 
   return y;
}


int main(int argc, char *argv[])
{
/*                                     ----- declarations */
   char        line[LMAX];       /* input line*/



   int         ifx,ify;          /* do loop for filter */
   int         ix,iy;            /* do loop for image */
   int         ip;               /* iy*xdim+ix */
   int        xdim, ydim;       /* x,y-pixel data size */
   int        pdim;             /* total-pixel data size */
   int        fsize;            /* filter size (pixel)*/

   float       *ipix;            /* input pixel array */
   float       *opix=NULL;            /* output pixel array */
   float       *opix2=NULL;            /* output pixel array */
   float       gsgm;             /* gaussian sigma */
   float       *gf;              /* gaussian filter array */

   struct imh  imhin={""};             /* imh header structure */
   struct icom icomin={0};             /* imh comment structure */

   struct imh  imhout={""};             /* imh header structure */
   struct icom icomout={0};             /* imh comment structure */

   /* For empty pixels (WHT) */
   int pixignr; 

   FILE *fpin, *fpout;
   char *fnamin,*fnamout;



   if(argc!=4)
     {
       printf("Usage: smth2 file_in filtersize file_out\n");
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

   ipix = (float*)calloc(pdim, sizeof(float));
   if(ipix==NULL) 
     {
       fprintf(stderr,"Cannot allocate ipix\n");
       exit(-1);
     }
   
   printf("\n> %s reading\n",fnamin);    
   if (imrall_tor(&imhin, fpin, ipix, pdim) == 0)
     {
       fprintf(stderr,"\7\n Cannot read file \n");     
       exit(1);
     }
   pixignr = imget_pixignr( &imhin );
   printf("smth:  %s PixIgnor= %d\n",fnamin,pixignr);
   
   /*                          ----- make a smoothing filter */
   
   fsize= 2*(int)ceil(gsgm*sqrt(log(PTPARAM)))-1;
   
   imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);
   imclose(fpin,&imhin,&icomin);
   
   if (fsize == 1)
     {
       printf("\7\n *input seeing is near to output one \n");
       printf(" there is no need to smooth the image \n");
       opix2=ipix;
     }
   else
     {
       printf("\n> gaussian filter sgm  = %f pix\n",gsgm);
       printf("                  size = %d pix\n",fsize);
       
       gf = (float*) gausflt_1d(gsgm, fsize);
       
       printf("\n> filter map \n");
       for (ifx=0; ifx<fsize; ifx++) 
	 {
	   printf(" %7.5f",gf[ifx]);
	 }
	  
       /*                               ----- smoothing the file */
       
       /* Y axis */
       printf("\n> %s smoothing\n",fnamin);
       opix=(float*) calloc(pdim, sizeof(float));
       if(opix==NULL) 
	 {
	   fprintf(stderr,"Cannot allocate opix\n");
	   exit(-1);
	 }
       
       
       for (iy=0; iy<ydim; iy++) 
	 {
	   for (ix=0; ix<xdim; ix++) 
	     {
	       ip=iy*xdim+ix;
	       if (ipix[ip]==pixignr)
		 {
		   opix[ip]=ipix[ip];
		 }
	       else if ((iy-(fsize-1)/2) < 0 || (iy+(fsize-1)/2) >= ydim ||
			ipix[ip-(fsize-1)/2*xdim]==pixignr ||
			ipix[ip+(fsize-1)/2*xdim]==pixignr)
		 {
		   opix[ip]=(float) edge_y_1d(ix,iy,fsize,xdim,ydim,ipix,gf,pixignr);
		 }
	       else
		 {
		   for (ify=0; ify<fsize; ify++) 
		     {
		       opix[ip]+=ipix[(iy-(fsize-1)/2+ify)*xdim+ix]*gf[ify];
		     }	
		 }       
	     }
	 }
       
       opix2=(float*)calloc(pdim, sizeof(float));
       if(opix2==NULL) 
	 {
	   fprintf(stderr,"Cannot allocate opix2\n");
	   exit(-1);
	 }
       
       for (iy=0; iy<ydim; iy++) 
	 {
	   for (ix=0; ix<xdim; ix++) 
	     {
	       ip=iy*xdim+ix;
	       if (opix[ip]==pixignr) opix2[ip]=pixignr;
	       else if ((ix-(fsize-1)/2) < 0 || (ix+(fsize-1)/2) >= xdim ||
			opix[ip-(fsize-1)/2]==pixignr ||
			opix[ip+(fsize-1)/2]==pixignr)
		 {
		   opix2[ip]=(float) edge_x_1d(ix,iy,fsize,xdim,ydim,opix,gf,pixignr);
		 }
	       else
		 for (ifx=0; ifx<fsize; ifx++) 
		   {
		     opix2[ip]+=opix[iy*xdim+(ix-(fsize-1)/2+ifx)]*gf[ifx];	       
		   }
	     }
	 }
       
       free(ipix);
       free(gf);
       
       /*                                ----- write output file */
       
       (void) sprintf( line, "SMTH2_quick:  filter sgm=%4.2f: size=%d",gsgm,fsize);
       (void) imaddcom( &icomin, line );
       
     }
   
   /*
     (void) sprintf( line, "        original sgm=%4.2f: resulted sgm=%4.2f",
     isee[nflm],osee);
     (void) imaddcom( &icomin, line );
   */
   
   if( (fpout=imwopen(fnamout, &imhout, &icomout)) == NULL )
     {
       fprintf(stderr,"Cannot open output file \n");
       exit(1);
     }
   
   printf("\n> %s writing --------------\n",fnamout); 
   
   imwall_rto(&imhout, fpout, opix2);
   
   fclose(fpout);
   if(opix!=NULL)
     {
       free(opix);
       free(opix2);
     }
   /* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

   return 0;
}  

