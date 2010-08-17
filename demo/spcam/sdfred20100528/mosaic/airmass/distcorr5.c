#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "imc.h"
#include "getargs.h"

/* new data fitted (2002/03/01 by W.Tanaka) */
/* 2002/04/01 consistency checked */
#define N  7
#define SCALE 1.0
#define A1 (1.0E0)
#define A2 (0)
#define A3 (4.15755e-10)
#define A4 (0)
#define A5 (1.64531e-18)
#define A6 (0)
#define A7 (-4.62459e-26)

double conv(double r, int n, double a[])
{
  /* [pix] -> [pix] */
  double r1;
  int i;
  
  r1=0;
  for(i=n;i>0;i--)
    {
      r1+=a[i];
      r1*=r;
    }
  return r1;
}

double reverseconv(double r, int n, double a[])
{
  /* [pix] -> [pix] */

  double r0,r1;
  double v;

  r1=r;
  
  v=conv(r1,n,a);
  while(v<r)
    {
      r1*=2.0;
      v=conv(r1,n,a);
    }

  /* OK, bisection */
  r0=0;
  while(r1-r0>0.01)
    {
      v=conv(0.5*(r1+r0),n,a);
      if(v>r) r1=0.5*(r0+r1);
      else r0=0.5*(r0+r1);
    }
  
  return 0.5*(r0+r1);
}


/* 2000/06/30 calc Jacobian again, but NOT needed so far!! */

double mapping(double x0, double y0, double *x1, double *y1,
	       double xcen, double ycen, 
	       int n, double a[])
{
  /*
	    dR(mm) = a2 * R^2 + a3 * R^3 + a4 * R^4
	    a2=　-4.36049E-6
	    a3=  +2.3908295E-6
	    a4=  -4.527512E-9

	    R'= R+a2 * R^2 + a3 * R^3 + a4 * R^4
	    J=R'/R*(1+2*a2*R+3*a3*R^2+4*a4*R^3)
	     =R'/R*(1+R*(2*a2+R*(3*a3+R*4*a4)

	     =R'/R * sum (N*R^(N-1)*aN)
	    */
  /* returns Jacobian */
  double dx,dy;
  double r0,r1;
  int i;
  double J;


  dx=((double)x0-xcen);
  dy=((double)y0-ycen);
	 
  r0=sqrt(dx*dx+dy*dy)*SCALE;

  r1=0.0;
  J=0.0;
  for(i=n;i>0;i--)
    {
      r1+=a[i];
      r1*=r0;
      J+=(double)i*a[i];
      J*=r0;
    }

  J*=r1/r0/r0;

  *x1=xcen+(float)(dx*r1/r0);
  *y1=ycen+(float)(dy*r1/r0);

  return J;
}


double reversemapping(double x1, double y1, double *x0, double *y0,
	       double xcen, double ycen, 
	       int n, double a[])
{
  double dx,dy;
  double r0,r1;

  
  dx=((double)x1-xcen);
  dy=((double)y1-ycen);
  
  r1=sqrt(dx*dx+dy*dy);
  r0=reverseconv(r1,n,a);
  
  *x0=xcen+(double)(dx*r0/r1);
  *y0=ycen+(double)(dy*r0/r1);

  return 0;
}

double mapping_withlintrans(double x0, double y0, double *x1, double *y1,
			    double xcen, double ycen, 
			    double b11, double b12, double b21, double b22,
			    int n, double a[])
{
  double xr,yr;
  double det;

  xr=xcen+b11*(x0-xcen)+b12*(y0-ycen);
  yr=ycen+b21*(x0-xcen)+b22*(y0-ycen);

  det=b11*b22-b21*b12;
  
  /* caution !! */
  /* b matrix is celestial => telescope */
  /* ie. det<1 for airmass */

  return det*mapping(xr,yr,x1,y1,xcen,ycen,n,a);
}

double reversemapping_withlintrans(double x0, double y0, double *x1, double *y1,
				   double xcen, double ycen, 
				   double b11, double b12, double b21, double b22,
				   int n, double a[])
{
  double xr,yr;
  double det;
  double eps=1.0e-7;

  det=b11*b22-b21*b12;
  if (det<eps&&det>-eps) return 0;
  
  xr=xcen+(b22*(x0-xcen)-b12*(y0-ycen))/det;
  yr=ycen+(-b21*(x0-xcen)+b11*(y0-ycen))/det;
  
  /* caution !! */
  /* b matrix is celestial => telescope */
  /* ie. det<1 for airmass */

  return reversemapping(xr,yr,x1,y1,xcen,ycen,n,a)/det;
}


int calc_limits_withlintrans(int npx, int npy, 
		double xcen, double ycen, 
		int n, double a[], 
		double b11, double b12, double b21, double b22,
		int *xmin_ret, int *xmax_ret,
		int *ymin_ret, int *ymax_ret)
{
  int xmin, xmax; /* in origunal(f) coordinate */
  int ymin, ymax; /* in origunal(f) coordinate */
  double bx,by;
  double x,y;  
  int i,j ;

  reversemapping_withlintrans
    (0,0,&x,&y,xcen,ycen,
     b11,b12,b21,b22,n,a);

  mapping_withlintrans
    (x,y,&bx,&by,xcen,ycen,
     b11,b12,b21,b22,n,a);


  xmin=(int)floor(x);
  xmax=(int)ceil(x);
  ymin=(int)floor(y);
  ymax=(int)ceil(y);

  for (i=0;i<=npx;i++)
    {
      j=0;
      reversemapping_withlintrans
	((double)i-0.5,(double)j-0.5,&x,&y,xcen,ycen,
	 b11,b12,b21,b22,n,a);

      if(x<xmin) xmin=(int)floor(x);
      if(x>xmax) xmax=(int)ceil(x);
      if(y<ymin) ymin=(int)floor(y);
      if(y>ymax) ymax=(int)ceil(y);

      j=npy-1;
      reversemapping_withlintrans
	((double)i-0.5,(double)j+0.5,&x,&y,xcen,ycen,
	 b11,b12,b21,b22,n,a);

      if(x<xmin) xmin=(int)floor(x);
      if(x>xmax) xmax=(int)ceil(x);
      if(y<ymin) ymin=(int)floor(y);
      if(y>ymax) ymax=(int)ceil(y);
    }
  for (j=0;j<=npy;j++)
    {
      i=0;
      reversemapping_withlintrans
	((double)i-0.5,(double)j-0.5,&x,&y,xcen,ycen,
	 b11,b12,b21,b22,n,a);
	 
      if(x<xmin) xmin=(int)floor(x);
      if(x>xmax) xmax=(int)ceil(x);
      if(y<ymin) ymin=(int)floor(y);
      if(y>ymax) ymax=(int)ceil(y);

      i=npx-1;
      reversemapping_withlintrans
	((double)i+0.5,(double)j-0.5,&x,&y,xcen,ycen,
	 b11,b12,b21,b22,n,a);

      if(x<xmin) xmin=(int)floor(x);
      if(x>xmax) xmax=(int)ceil(x);
      if(y<ymin) ymin=(int)floor(y);
      if(y>ymax) ymax=(int)ceil(y);
    }

  *xmin_ret=xmin;
  *xmax_ret=xmax;
  *ymin_ret=ymin;
  *ymax_ret=ymax;
  return 0;
}


float *distcorr_lintrans_quick3 (float *f, int npx, int npy, float pixignr,
				 double xcen, double ycen, 
				 int n, double a[], 
				 double b11, double b12, double b21, double b22,	 
				 int *npx2, int *npy2,
				 int *xshift, int *yshift,
				 int mode)
{
  /* set limit of output */
  int xmin, xmax; /* in origunal(f) coordinate */
  int ymin, ymax; /* in origunal(f) coordinate */
  int i,j;

  double J;


  double fx,fy;
  float *g;

  double bx,by;

  double v0,v1;



  
  int i0,j0,i1,j1;
  int idx0,idx1,idx2,idx3,idx;

  /* get xmin, xmax, ymin, ymax */
  calc_limits_withlintrans(npx, npy, xcen, ycen, n, a, 
	      b11,b12,b21,b22,
	      &xmin, &xmax, &ymin, &ymax);

  /* "center" of the left bottom pix is (0,0) */
  *npx2=xmax-xmin+1;
  *npy2=ymax-ymin+1;
  *xshift=xmin;
  *yshift=ymin;

  printf("%d %d %d %d\n",*npx2,*npy2,*xshift,*yshift);
  printf("%d %d %d %d\n",xmin,xmax,ymin,ymax);
  
  g=(float*)calloc((*npx2)*(*npy2),sizeof(float));

  for(j=ymin;j<=ymax;j++)
    {
      /*
	 printf("j=%d\n",j);
	 */
      for(i=xmin;i<=xmax;i++)
	{
	  
	  J=mapping_withlintrans((double)i,(double)j,&bx,&by,xcen,ycen,
				 b11,b12,b21,b22,n,a);
	  i0=(int)floor(bx);
	  i1=i0+1;
	  j0=(int)floor(by);
	  j1=j0+1;

	  idx0=i0+j0*npx;
	  idx1=idx0+1;
	  idx2=i0+j1*npx;
	  idx3=idx2+1;

	  idx=(i-xmin)+(j-ymin)*(*npx2);

	  if(
	     i0<0||i1>npx-1||j0<0||j1>npy-1||J<=0
	     ||f[idx0]==pixignr
	     ||f[idx1]==pixignr
	     ||f[idx2]==pixignr
	     ||f[idx3]==pixignr)		

	    {
	      g[idx]=pixignr;
	    }
	  else
	    {
	      /* bilinear */
	      fx=bx-(double)i0;
	      v0=(1.0-fx)*f[idx0]+fx*f[idx1];		
	      v1=(1.0-fx)*f[idx2]+fx*f[idx3];
	      fy=by-(double)j0;
	      
	      if(mode==1)
		g[idx]=(float)((1.0-fy)*v0+fy*v1);
	      else
		g[idx]=(float)((1.0-fy)*v0+fy*v1)*J;
	    }
	}
    }  

  return g;
}

int main(int argc, char **argv)
{
  float *f,*g;
  int npx=2046,npy=4090;

  float pixignr=0;
  double xcen=5000,ycen=4192;
  double xpos=-FLT_MAX,ypos=-FLT_MAX;

  double a[N+1]={0,A1,A2,A3,A4,A5,A6,A7};
  int npx2,npy2,xshift,yshift;
  int i;
  char fnamin[255]="";
  char fnamout[255]="";
  char dtype[255];
  int ignor=INT_MIN;
  float bzero=0.0,bscale=1.0;
  FILE *fpin,*fpout;
  struct imh imhin={""}, imhout={""};
  struct icom icomin={0}, icomout={0};
  int quickmode=0; /* default slow */
  int keepSB=1; /* default keep SB, not flux */





  /* for WCS */


  double b11=1.0,b12=0.0,b21=0.0,b22=1.0;



  /***** parse options ******/

  getargopt opts[20];
  char *files[3]={NULL};
  int n=0;
  int keepflux=0;

  files[0]=fnamin;
  files[1]=fnamout;

  n=0;
  setopts(&opts[n++],"-x=", OPTTYP_DOUBLE , &xpos,
	  "chip offset x from the center(always needed)");
  setopts(&opts[n++],"-y=", OPTTYP_DOUBLE , &ypos,
	  "chip offset y from the center(always needed)");


  setopts(&opts[n++],"-quick", OPTTYP_FLAG , &quickmode,
	  "quick mode (default: quick)",1);
  setopts(&opts[n++],"-keepflux", OPTTYP_FLAG , &keepflux,
	  "keep flux mode (default:keep SB)",1);

  /*
    setopts(&opts[n++],"-reverse", OPTTYP_FLAG , &reverse,
	  "reverse(not impl.)",1);
  */

  setopts(&opts[n++],"-a2=", OPTTYP_DOUBLE , &(a[2]),
	  "a2(default:0)");
  setopts(&opts[n++],"-a3=", OPTTYP_DOUBLE , &(a[3]),
	  "a3(default:4.15755e-10)");
  setopts(&opts[n++],"-a4=", OPTTYP_DOUBLE , &(a[4]),
	  "a4(default:0)");
  setopts(&opts[n++],"-a5=", OPTTYP_DOUBLE , &(a[5]),
	  "a5(default:1.64531e-18)");
  setopts(&opts[n++],"-a6=", OPTTYP_DOUBLE , &(a[6]),
	  "a6(default:0)");
  setopts(&opts[n++],"-a7=", OPTTYP_DOUBLE , &(a[7]),
	  "a7(default:-4.62459e-26)");

  setopts(&opts[n++],"-b11=", OPTTYP_DOUBLE , &(b11),
	  "b11(default:1.0)");
  setopts(&opts[n++],"-b12=", OPTTYP_DOUBLE , &(b12),
	  "b12(default:0.0)");
  setopts(&opts[n++],"-b21=", OPTTYP_DOUBLE , &(b21),
	  "b21(defaulr:0.0)");
  setopts(&opts[n++],"-b22=", OPTTYP_DOUBLE , &(b22),
	  "b22(default:1.0)");

  setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero,
	  "bzero");
  setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale,
	  "bscale");
  setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
	  "data_typ(FITSFLOAT,FITSSHORT...)");

  setopts(&opts[n++],"-pixignr=", OPTTYP_FLOAT , &ignor,
	  "pixignr value");

  setopts(&opts[n++],"",0,NULL,NULL);

  if(parsearg(argc,argv,opts,files,NULL)
     ||fnamout[0]=='\0' || xpos==-FLT_MAX || ypos==-FLT_MAX)
    {
      print_help("Usage: distcorr5 [options] filein fileout",
		 opts,"");
      exit(-1);
    }

  if(keepflux==1) keepSB=0;
 
  xcen=-xpos;
  ycen=-ypos;
  
  /* read FILE */
  if ((fpin= imropen(fnamin,&imhin,&icomin))==NULL) 
    {
      printf("File %s not found !!\n",fnamin);
      exit(-1);
    }
  pixignr=(float)imget_pixignr( &imhin );
  npx=imhin.npx;
  npy=imhin.npy;

  f=(float*)malloc(npx*npy*sizeof(float));
  imrall_tor(&imhin,fpin,f,npx*npy);
	
  /* 2002/04/01 */
  /* only "quick" version is implemented */
  g=distcorr_lintrans_quick3(f,npx,npy,pixignr,xcen,ycen,N,a,
			     b11,b12,b21,b22,
			     &npx2,&npy2,&xshift,&yshift,keepSB);

  /* replace pixignr */
  imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);
  imclose(fpin,&imhin,&icomin);

  imc_fits_set_dtype(&imhout,FITSFLOAT,bzero,bscale);

  if(ignor!=INT_MIN) 
    {
      for(i=0;i<npx2*npy2;i++)
	if (g[i]==pixignr) g[i]=(float)ignor;
      imset_pixignr(&imhout,&icomout,ignor);
    }
  else
    {
      imset_pixignr(&imhout,&icomout,(int)pixignr);
    }
  imhout.npx=npx2;  
  imhout.npy=npy2;
  imhout.ndatx=npx2;  
  imhout.ndaty=npy2;


  imaddhistf(&icomout,"Distortion corrected with center=(%f,%f)",xcen,ycen);
  if (keepSB==1)
    imaddhistf(&icomout," keep-SB mode.");
  else
    imaddhistf(&icomout," keep-flux mode.");
  imaddhistf(&icomout,"  matrix=(%f,%f)(%f,%f)",b11,b12,b21,b22);
  imaddhistf(&icomout,"  coeff:a2=%e a3=%e a4=%e",a[2],a[3],a[4]);
  imaddhistf(&icomout,"  coeff:a5=%e a6=%e a7=%e",a[5],a[6],a[7]);

  /* WCS revise */
  imc_shift_WCS(&icomout,(float)xshift,(float)yshift);

  /* write FILE */
  if ((fpout= imwopen(fnamout,&imhout,&icomout))==NULL) 
    {
      printf("Cannot open file %s !!\n",fnamout);
      exit(-1);
    }

  imwall_rto( &imhout, fpout, g );
  imclose(fpout,&imhout,&icomout);
  free(g);
  return 0;
}

