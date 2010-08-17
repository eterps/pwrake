#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include "imc.h"

#define sqr(x)  ((x) * (x))

#define LMAX          40     /* max char string length */
#define PTPARAM      25.     /* threshould for peak/tail of gf */

#define FMAX          21     /* max size of smoothing filter */


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

double gaus(double A,double S,double r) /* S is sigma^2 */
{
  return A/sqrt(S)*exp(-0.5*r*r/S);
}

double gauss4(double A1,double S1, /* S1-S4 are sigma^2 */
	     double A2,double S2,
	     double A3,double S3,
	     double A4,double S4,
	     double r)
{
  return gaus(A1,S1,r)+gaus(A2,S2,r)+gaus(A3,S3,r)+gaus(A4,S4,r);
}


float* gauss4flt (
		  double A1,double S1, /* S1-S4 are sigma^2 */
		  double A2,double S2,
		  double A3,double S3,
		  double A4,double S4,
		  int  size   /* filter size (odd)*/
               ) 
{
   int   i,ix,iy; /* do loop */
   double total; /* sum of all components */
   float *gsf; /* gaussian filter array */
   int xc,yc;
   float rad2,rad2max;

   xc=yc=(size-1)/2;
   rad2max=xc*xc+1;

   total=0.;
   gsf = (float*) calloc(size*size, sizeof(float));

   for (iy=0; iy<size; iy++)
     {
       for (ix=0; ix<size; ix++)
	 {
	   rad2=(float)((ix-xc)*(ix-xc)+(iy-yc)*(iy-yc));
	   if(rad2<rad2max)
	     {
	       gsf[iy*size+ix]=
		 (float)gauss4(A1,S1,A2,S2,A3,S3,A4,S4,sqrt(rad2));
	       total=total+gsf[iy*size+ix];
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


/* 2002/05 - 06 */
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

   int         i;                /* do loop */

   int         ifx,ify;          /* do loop for filter */


   int        xdim, ydim;       /* x,y-pixel data size */
   int        pdim;             /* total-pixel data size */
   int        fsize;            /* filter size (pixel)*/

   float       *ipix;            /* input pixel array */
   float       *opix;            /* output pixel array */
   float       *gf;              /* gaussian filter array */


   struct imh  imhin={""};             /* imh header structure */
   struct icom icomin={0};             /* imh comment structure */

   struct imh  imhout={""};             /* imh header structure */
   struct icom icomout={0};             /* imh comment structure */

   /* for empty pixels (WHT) */
   int pixignr;

   /* define struct for file pointers */
   FILE *fpin,*fpout;
   char *fnamin,*fnamout;


   float s1in,s2in,a1in,a2in;
   float s1out,s2out,a1out,a2out;
   
   double A1,A2,A3,A4,S1,S2,S3,S4,d,v;

   if(argc!=9)
     {
       printf("Usage: dsmth file_in s1in s2in a1in file_out s1out s2out a2out  \n");
       exit(-1);
     }
   else
     {
       fnamin=argv[1];
       s1in=atof(argv[2]);
       s2in=atof(argv[3]);
       a1in=atof(argv[4]);
       a2in=1.0-a1in;
       fnamout=argv[5];
       s1out=atof(argv[6]);
       s2out=atof(argv[7]);
       a1out=atof(argv[8]);
       a2out=1.0-a1out;
     }

   if ((fpin = imropen(fnamin, &imhin, &icomin)) == NULL)
	{
	  fprintf(stderr,"\a\n Cannot open input file %s\n",fnamin);
	  exit(1);
	}
      xdim  = imhin.npx;
      ydim  = imhin.npy;
      pdim = xdim * ydim;
      ipix = (float*)calloc(pdim, sizeof(float));
      
      printf("\n> %s reading\n",fnamin);    
      if (imrall_tor(&imhin, fpin, ipix, pdim) == 0)
	{
	  fprintf(stderr,"\a\n Cannot read file \n");     
	  exit(1);
	}
      pixignr = imget_pixignr( &imhin );
      printf("smth:  %s PixIgnor= %d\n",fnamin,pixignr);

      /*                          ----- make a smoothing filter */
 

   /************ FILTER making up! **************/

   /* Function is  \Sum An/sqrt(Sn)*exp(-0.5*r^2/Sn) */
   
   A1=(a1out*s1out)/(a1in*s1in);
   A2=(a2out*s2out)/(a1in*s1in);
   A3=-(a1out*s1out)/(a1in*s1in)*(a2in*s2in)/(a1in*s1in);
   A4=-(a2out*s2out)/(a1in*s1in)*(a2in*s2in)/(a1in*s1in);
   S1=(s1out*s1out-s1in*s1in);
   S2=(s2out*s2out-s1in*s1in);
   S3=(s1out*s1out+s2in*s2in)-2.0*(s1in*s1in);
   S4=(s1out*s1out+s2in*s2in)-2.0*(s1in*s1in);
   d=(a2in*s2in)/(a1in*s1in)*exp(-M_PI*S1);
   
   if(S1<0 || S2<0 || S3<0 || S4<0 || 
          d*d>0.05 /* <---- */
      )
     {
       printf("Cannot smooth, we need deconvolution! \n");
       exit(-1);
     }

   printf("%f %f %f %f %f\n",A1,A2,A3,A4,A1+A2+A3+A4);
   printf("%f %f %f %f \n",S1,S2,S3,S4);

   /* renormalize */
   v=A1+A2+A3+A4;
   A1/=v;
   A2/=v;
   A3/=v;
   A4/=v;

   printf("%f %f %f %f %f\n",A1,A2,A3,A4,A1+A2+A3+A4);
   printf("%f %f %f %f \n",S1,S2,S3,S4);

   /* fsize(=2n+1), gf[] */
   for(i=1;i<101;i++)
     {
       v=gauss4(A1,S1,A2,S2,A3,S3,A4,S4,(double)i);
       if (v*PTPARAM<1.) break;
     }
   fsize=2*i+1;

   imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);
   imclose(fpin,&imhin,&icomin);


   if (fsize == 1)
     {
       printf("\a\n *input seeing is near to output one \n");
       printf(" there is no need to smooth the image \n");
       opix=ipix;
     }
   else
     {
       printf("                  size = %d pix\n",fsize);
       
       /* Make filter */
       
       gf = (float*) gauss4flt(A1,S1,A2,S2,A3,S3,A4,S4,fsize);
       
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
       opix = (float*) calloc(pdim, sizeof(float));
       
       /* Convolve */
       /* 2002/06/06 replaced with convolve2 */
       convolve2(xdim,ydim,ipix, opix, fsize, gf,pixignr);
       
       free(ipix);
       free(gf);

       /*                                ----- write output file */
       
       /* 2002/06/06 updated */
       /*
	 (void) sprintf( line, "DSMTH:  filter size=%d",fsize);
	 (void) imaddcom( &icomout, line );
       */
       imaddhistf(&icomout,"DSMTH: (%f %f %f) => (%f %f %f)",
		  s1in,s2in,a1in,s1out,s2out,a1out);
       imaddhistf(&icomout,"DSMTH: filter size=%d",fsize);
     }

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


