#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "imc.h"
#include "getargs.h"

#define sqr(x)  ((x) * (x))

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

int main(int argc, char *argv[])
{
/*                                     ----- declarations */
   char        line[BUFSIZ];       /* input line*/



   int         ifx,ify;          /* do loop for filter */
   int         ix,iy;            /* do loop for image */
   int         ip;               /* iy*xdim+ix */
   int        xdim, ydim;       /* x,y-pixel data size */
   int        pdim;             /* total-pixel data size */
   int        xdimf, ydimf;       /* x,y-pixel data size */
   int        fdim;             /* total-pixel data size */
   int        fsize;            /* filter size (pixel)*/

   float       *ipix;            /* input pixel array */
   float       *opix;            /* output pixel array */

   float       *gf;              /* gaussian filter array */

   struct imh  imhin={""};
   struct imh  imhfil={""};
   struct imh  imhout={""};
   struct icom icomin={0};
   struct icom icomfil={0};
   struct icom icomout={0};
   
   /* for empty pixels (WHT) */
   int pixignr=-10000; 

   /* define struct for file pointers */
   FILE *fpin=NULL,*fpout=NULL;

   /* define struct for file names */
   /*
     char *fnamin=NULL,*fnamfil=NULL,*fnamout=NULL;
   */
   
   char fnamin[BUFSIZ]="";
   char fnamfil[BUFSIZ]="";
   char fnamout[BUFSIZ]="";

   getargopt opts[5];
   char *files[3]={NULL};
   int n=0;
   int helpflag;

  /***************************************/
   
   files[0]=fnamin;
   files[1]=fnamfil;
   files[2]=fnamout;
   
   setopts(&opts[n++],"",0,NULL,NULL);
   
   helpflag=parsearg(argc,argv,opts,files,NULL);
   
   if(fnamout[0]=='\0')
     {
       fprintf(stderr,"Error: No input file specified!!\n");
       helpflag=1;
     }
   if(helpflag==1)
     {
       print_help("Usage: convolve file_in file_filter file_out",
		  opts,
		  "");
       exit(-1);
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

   if ((fpin = imropen(fnamfil, &imhfil, &icomfil)) == NULL)
	{
	  fprintf(stderr,"\7\n Cannot open filter file %s\n",fnamfil);
	  exit(1);
	}
   xdimf  = imhfil.npx;
   ydimf  = imhfil.npy;
   fdim = xdimf * ydimf;
   gf = (float*)malloc(fdim*sizeof(float));
   
   printf("\n> %s reading\n",fnamfil);    
   if (imrall_tor(&imhfil, fpin, gf, fdim) == 0)
     {
       fprintf(stderr,"\7\n Cannot read file \n");     
       exit(1);
     }
   
   fsize=xdimf;
      
   printf("\n> filter map \n");
   for (ify=0; ify<fsize; ify++) 
     {
       for (ifx=0; ifx<fsize; ifx++) 
	 {
	   printf(" %7.5f",gf[ify*fsize+ifx]);
	 }
       printf("\n");
     }      
   
   /*                               ----- smoothing the file */
   
   printf("\n> %s smoothing\n",fnamin);
   opix = (float*) malloc(pdim*sizeof(float));
   
   for (iy=0; iy<ydim; iy++) 
     {
       for (ix=0; ix<xdim; ix++) 
	 {
	   /* 1999/06/16 
	      pixignr is not edge any longer */
	   ip=iy*xdim+ix;

	   if ((ix-(fsize-1)/2) < 0 || (iy-(fsize-1)/2) < 0 ||
	       (ix+(fsize-1)/2) >= xdim || (iy+(fsize-1)/2) >= ydim)
	     opix[ip]=(float) edge(ix,iy,fsize,xdim,ydim,ipix,gf,pixignr);
	   else
	     for (ify=0; ify<fsize; ify++) 
	       for (ifx=0; ifx<fsize; ifx++) 
		 {
		   if(gf[ify*fsize+ifx]<=0)continue;
		   if(ipix[(iy-(fsize-1)/2+ify)*xdim+(ix-(fsize-1)/2+ifx)]
		      ==(float)pixignr)
		     {
		       opix[ip]=(float)pixignr;
		       ify=fsize;
		       break;
		     }
		   opix[ip]=opix[ip]
		     +ipix[(iy-(fsize-1)/2+ify)*xdim+(ix-(fsize-1)/2+ifx)]
		       *gf[ify*fsize+ifx];
		 }		 	 
	 }
     }
   
   free(ipix);
   free(gf);

   imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);
   imclose(fpin,&imhin,&icomin);
   
   /*                                ----- write output file */
   
   (void) sprintf( line, "CONVOLVE:  filter %s",fnamfil);
   (void) imaddhist( &icomout, line );
      
   if( (fpout=imwopen(fnamout, &imhout, &icomout)) == NULL )
     {
       fprintf(stderr,"Cannot open output file \n");
       exit(1);
     }
   
   printf("\n> %s writing --------------\n",fnamout); 

   imwall_rto(&imhout, fpout, opix);

   
   imclose(fpout,&imhout,&icomout);
   free(opix);

   return 0;
}  
