#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "imc.h"
#include "oyamin.h"
#include "stat.h"
#include "sort.h"

#include "skysb_sub.h"

float medfilter_skysb0(float *g,int npx,int npy,int width) 
{
 float *h; 
 int imax,jmax,n;
 int i,j,i0,j0;
 float *dat;
 double s,ss;
 float median;
 float mad,T;


 imax=npx+2*width;
 jmax=npy+2*width;

 if(npx*npy<=2) 
   {
     if(npx*npy==2)
       {
	 return 0.5*(g[0]+g[1]);
       }
     else if(npx*npy==1)
       {
	 return g[0];
       }
     else
       return 0;
   }

 h=(float*)malloc(sizeof(float)*npx*npy);
 dat=(float*)malloc(sizeof(float)*(4*width*(width+1)+1));

 n=0;
 s=ss=0;
 /* calculate mean ,etc.*/

 memcpy(h,g,npx*npy*sizeof(float));

 for(i=0;i<npx;i++)
   for(j=0;j<npy;j++)
     {
       n=0;	     
       for(i0=i-width;i0<=i+width;i0++)
	 for(j0=j-width;j0<=j+width;j0++)
	   {
	     if ((i0>=0) && (j0>=0) && (i0<npx) && (j0<npy))
	       {
		 dat[n]=h[i0+npx*j0];
		 n++;
	       }
	   }
       /*
       j0=j;
       for(i0=i-1;i0<=i+1;i0++)
	 if ((i0>=0) && (j0>=0) && (i0<npx) && (j0<npy))
	   {
	     dat[n]=h[i0+npx*j0];
	     n++;
	   }
       i0=i;
       for(j0=j-1;j0<=j+1;j0++)
	 if ((i0>=0) && (j0>=0) && (i0<npx) && (j0<npy))
	   {
	     dat[n]=h[i0+npx*j0];
	     n++;
	   }
       */

       /* 
	  g[i+npx*j]=floatmedian(n,dat);
       */
       g[i+npx*j]=Tukey(n,dat);
     }

 /* calculate mean ,etc.*/

 T=getTukey(npx*npy,g,&median,&mad);

 free(dat);
 free(h);
 return T;
}

float medfilter_skysb(float *g,int npx,int npy,int width) 
{
 float *h; 
 int imax,jmax,n,n0;
 int i,j,i0,j0;
 float *dat,temp;
 double s,ss;
 float mean,sigma;
 float median;
 float mad,T;
 float x;

 imax=npx+2*width;
 jmax=npy+2*width;

 if(npx*npy<=2) return 0.;

 h=(float*)malloc(sizeof(float)*npx*npy);
 dat=(float*)malloc(sizeof(float)*(4*width*(width+1)+1));

 n=0;
 s=ss=0;

 /* calculate mean ,etc.*/

 T=getTukey(npx*npy,g,&median,&mad);

 for(i=0;i<npx;i++)
   {
     for(j=0;j<npy;j++)
       {
	 s+=g[i+npx*j];
	 ss+=(g[i+npx*j]-median)*(g[i+npx*j]-median);
	 n++;
       }
   }

 mean=s/n;
 sigma=sqrt((ss/n-(median-mean)*(median-mean))*n/(n-1));

 /*
   printf("sky-estimate :: raw-mean:%g raw-median:%g raw-T:%g ",mean,median,T);
   printf("raw-sigma:%g raw-madbased-sigma:%g  \n",sigma,mad/0.6745);
 */

 /* sigma~=mad/0.6745 */
 /* Irwin wrote mad=sigma/0.6745, but it is wrong,
    for P(mad/sigma)=0.75 in gaussian. */
 
 /* 5 sigma rejection and replace with mean */

 s=0;
 ss=0;
 n=0;
 for(i=0;i<npx;i++)
   {
     for(j=0;j<npy;j++)
       {
	 h[i+npx*j]=g[i+npx*j];

	 if(fabs(g[i+npx*j]-T)> 5.*sigma)
	   {
	     h[i+npx*j]=T;
	     /* local median .. now not used*/
	     /*
	       n1=0;
	       for(i0=i-width;i0<=i+width;i0++)
	       for(j0=j-width;j0<=j+width;j0++)
	       {
	       if ((i0>=0) && (j0>=0) && (i0<npx) && (j0<npy))
	       {
	       dat[n1]=g[i0+npx*j0];
	       for(n0=n1;n0>0;n0--)
	       if(dat[n0]>dat[n0-1])
	       {
	       temp=dat[n0-1];
	       dat[n0-1]=dat[n0];
	       dat[n0]=temp;
	       }
	       n1++;
	       }
	       }
	       if (n1%2==1) h[i+npx*j]=dat[(n1-1)/2];
	       else h[i+npx*j]=0.5*(dat[n1/2]+dat[n1/2-1]);
	     */
	   }
       }
   }


 for(i=0;i<npx;i++)
   {
     for(j=0;j<npy;j++)
       {
	 n=0;	     
	 for(i0=i-width;i0<=i+width;i0++)
	   for(j0=j-width;j0<=j+width;j0++)
	     {
	       if ((i0>=0) && (j0>=0) && (i0<npx) && (j0<npy))
		 {
		   dat[n]=h[i0+npx*j0];
		   for(n0=n;n0>0;n0--)
		     if(dat[n0]>dat[n0-1])
		       {
			 temp=dat[n0-1];
			 dat[n0-1]=dat[n0];
			 dat[n0]=temp;
		       }
		   n++;
		 }
	     }
	 if (n%2==1) g[i+npx*j]=dat[(n-1)/2];
	 else g[i+npx*j]=0.5*(dat[n/2]+dat[n/2-1]);
       }
   }

 /* calculate mean ,etc.*/

 T=getTukey(npx*npy,g,&median,&mad);

 /* 1999/11/04 fixed */
 n=0;
 for(i=0;i<npx;i++)
   {
     for(j=0;j<npy;j++)
       {
	 n++;
	 x=(g[i+npx*j]-s);
	 s+=x/(float)n;
	 ss+=(n-1)*x*x/(float)n;
       }
   }
 mean=s;
 sigma=sqrt(ss/(n-1));
 /*
   printf("5 sigma rejection done\n");
   printf("sky-estimate(after rejection):: mean:%g median:%g T:%g ",mean,median,T);
   printf("sigma:%g mad-based-sigma:%g \n",sigma,mad/0.6745);
 */

 free(dat);
 free(h);

 return T;
}

float skysigmadet(float *sigmap,int npx,int npy) 
{ 

  float median;
  float mad,T;
  float *h; 
  int i,n=0;

  h=(float*)malloc(sizeof(float)*npx*npy);

  for(i=0;i<npx*npy;i++)
    if (sigmap[i]>0) 
      h[n++]=sigmap[i];

  if(n>0)
    T=getTukey(n,h,&median,&mad);
  else
    median=T=mad=0.;
  /*
    printf("skysigma estimate :: median:%g T:%g\n",median,T);
  */
  free(h);
  return T;
}

int statrj(float *g, int npx,int npy, /* image */
	   int is,int ie,int js,int je, /*region*/
           float fact, /* factor parameter */
	   int ncycle, /* # cycle */
	   float *gmax,float *gmin,float *gmean, float *gsd,int *nrej,
	   float pixignr)
{
  /* statistic of region*/
  double v;
  int np,j,i;
  int ndat;
  float *dat;
  float T,median,mad;

  *gmax=*gmin=*gmean=*gsd=0.;
  
  if(is<0 || ie>=npx || js<0 || je>=npy || is>ie || js>je) return 1;

  np=(ie-is+1)*(je-js+1);
  /*  if (np<=1) return 1; not needed, already rejected */ 

  dat=(float*)malloc(np*sizeof(float));

  /* 2004/01/26 replaced with Tukey parameters */

  *gmax=-FLT_MAX;
  *gmin=FLT_MAX;
  ndat=0;
  for(j=js;j<=je;j++)
    for(i=is;i<=ie;i++)
      {
	v=g[i+npx*j];
	if (v!=pixignr)
	  {
	    dat[ndat++]=v;
	    if (*gmax<v) *gmax=v;
	    if (*gmin>v) *gmin=v;
	  }
      }

  *nrej=np-ndat;
  if (ndat<=2) 
    {
      free(dat);
      return 1; /* valid pixel is less than or equal to 2*/
    }
  T=getTukey(ndat,dat,&median,&mad);
  *gmean=T;
  *gsd=mad/0.6745;

  /*** N-SIGMA REJECTION  (NCYCLE-TIMES) ***/
  free(dat);
  return 0;
}

#define STATPOS (1./16.)
void statsk(float *g,int npx,int npy,float fact,int ncycle,
	    float *gmax,float *gmin,float *gmean,float *gsd,
	    float pixignr)
{
 /* statistic for 4 region */

 int is[4],js[4];
 float smax[4],smin[4],smean[4],ssd[4];
 int nrej[4],kret[4];

 int idimh,jdimh;
 int i,n,nsmall;

 idimh=(int)((float)npx*STATPOS); /* npx > 0 */
 jdimh=(int)((float)npy*STATPOS); /* npy > 0 */

 is[0]=idimh;
 is[2]=idimh;
 is[1]=npx-3*idimh;
 is[3]=npx-3*idimh;

 js[0]=jdimh;
 js[1]=jdimh;
 js[2]=npy-3*jdimh;
 js[3]=npy-3*jdimh;

 /*
   for(i=0;i<4;i++)
   printf("%d %d\n",is[i]+idimh,js[i]+jdimh);
 */

 /*
  printf("%10s %10s %10s %10s %8s %8s\n",
  "MEAN","SD","MAX","MIN","NREJ","KRET");
 */

 for(n=0;n<4;n++)
   {
     kret[n]=statrj(g,npx,npy,is[n],is[n]+2*idimh,js[n],
		    js[n]+2*jdimh,fact,ncycle,
		    &(smax[n]),&(smin[n]),&(smean[n]),
		    &(ssd[n]),&(nrej[n]),
		    pixignr);
     /*
       printf("%10.2f %10.2f %10.2f %10.2f %8d %8d\n",smean[n],ssd[n],
       smax[n],smin[n],nrej[n],kret[n]);
     */
   }                                                                   

 if(kret[0]*kret[1]*kret[2]*kret[3]!=0)
   {
     printf("ERROR AT STATSK \n");
     for(i=0;i<4;i++) printf("%d ",kret[i]);
     printf("\n");
     exit(-1);
   }
 nsmall=0;

 /* 2003/03/25 bug fix */
 /*
  *gmean=smean[0];
  *gsd=ssd[0];
  *gmin=smin[0];
  *gmax=smax[0];
  */
 *gmean=FLT_MAX;
 *gsd=FLT_MAX;

 /* 2004/01/26 smallest gmean => smallest gsd selection */
 for(n=0;n<4;n++)
   {
     if(ssd[n]<(*gsd)&&kret[n]==0)
       {
         nsmall=n;
         *gmean=smean[n];
         *gsd=ssd[n];
         *gmin=smin[n];
         *gmax=smax[n];
       }
   }
 return ;
}

/***********************/

double errgauss(int npar,double *p,double xx,double yy,double er)
{
/* For oyamin2_r format. err of gauss-fit*/
 double v;
 double yy0,xx0,xx2;

 xx0=(xx-p[0])/p[2];
 xx2=xx0*xx0;
 yy0=p[1]*exp(-0.5*xx2);

 v=(yy-yy0)/er;

 return v;
}

#define NPAR 3

void skydet2b(float *g,int npx,int npy,float *rmesh,
	      int nmesh_x,int nmesh_y,
	      int meshsiz_x,int meshsiz_y,
	      float gmean,float gsd,
	      float fsigm,float *sgmesh,int binflag,
	      float pixignr)
/* if binflag==1, binwidth fixed to be integer */
{

/* 97/Feb */
 int nh;
 double vmin;
 int i,im,jm,j,n,npar=3,nhg;
 double hunit,huinv;
 int ihunit=0;
 static int histsiz=0;
 static int offset0;
 static int offset1;
 double sigm,rmode,ymode; 

 int *khist;
 double *xm,*er,*f,*ym;
 double *ym2;

 double chisq;
 int ncut;
 double p[NPAR],e[NPAR];/* parameter*/
 int n0,n1,nmax,kmax;
 int flag,flag0;
 int n00;
 int mm;





/* This routine is VERY complicated, even for me, code writer */

 nh=(int)(3.5*fsigm*gsd); /* nh >0 */
 if (meshsiz_x*meshsiz_y<2.*nh || nh<10) nh=(int)(meshsiz_x*meshsiz_y*0.5);
                          /* 10 is arbitarly */

 printf("%d %d\n",meshsiz_x,meshsiz_y);
 printf("%f %f nh=%d\n",fsigm,gsd,nh);

 if(binflag==1)
   {
     hunit=floor(4.0*fsigm*gsd/(float)nh+0.5); /* hunit >0 */
     ihunit=(int)hunit;
   }
 else
   {
      hunit=4.0*fsigm*gsd/(float)nh; /* hunit >0 */
   }
 huinv=1./(float)hunit; 

 /* hunit is fixed to be integer, for quantized image.
    This routine will have to get some parameter
    if hunit must be fixed as integer, referring 
    imh.dtype. Todo. */
 
 nhg=nh; /* nh is histogram #bin */
 offset0=0;
 offset1=nh*(nmesh_x+1); 

 /*
   printf("NH:%d\n",nhg);
 */

 xm=(double*)malloc(sizeof(double)*nh);
 er=(double*)malloc(sizeof(double)*nh);
 f=(double*)malloc(sizeof(double)*nh);
 ym=(double*)malloc(sizeof(double)*nh);

 
 for(i=0;i<nhg;i++)
   {
     xm[i]=(double)i;
     er[i]=1.0;
   }

 histsiz=(nmesh_x+1)*2*nh;
 khist=(int*)malloc(sizeof(int)*(nmesh_x+1)*2*nh);
 if(khist==NULL) {printf("Cannot allocate address\n"); exit(-1);}

 if(binflag==1)
   {
     if(ihunit%2==0)
       vmin=(float)floor(gmean-fsigm*gsd+.5)+0.5;
     else
       vmin=(float)floor(gmean-fsigm*gsd+.5);
   }
 else
   {
     vmin=gmean-fsigm*gsd;
   }
 printf("HUNIT = %g\n",hunit);
 printf("VMIN = %g\n",vmin);

 /* vmin is floor of histo x 

          area 0   area1         area(nh-1)
  -----+---------+-------- ...  +---------+------> g[]
     vmin    vmin+hunit            vmin+hunit*(nh)
 */


/*******************
  Filling histogram procedure,
 *******************

   This routine uses khist[ ] buffer 

   1) count histogram of 1/4 region of a mesh 

    j
    3  00 00 00 00
    2  00 00 00 00      Each 00 represents 1/4 mesh.
    1  00 00 00 00    
0  >0  XX 00 00 00
off     0  1  2  3  i

      And fill khist[0](#of pix g=min) ...khist[nh-1](#of pix g=max)

      | khist[n]
      |              is a histogram 1/4 mesh.  
      |
      |------------>
                  n

   2) Next, count next(i+1) area 

    j
    3  00 00 00 00
    2  00 00 00 00      Each 00 represents 1/4 mesh.
    1  00 00 00 00    
0  >0  -- XX 00 00
        0  1  2  3  i

    into khist[0+nh]...khist[nh-1+nh]. nh is a kind of i-offset.

   3) repeat until i=nmesh_x-1 to fill ... khist[nh*(nmesh_x+1)-1].

    j
    3  00 00 00 ... 00
    2  00 00 00 ... 00      Each 00 represents 1/4 mesh.
    1  00 00 00 ... 00    
0  >0  -- -- -- ... XX
        0  1  2    nmesh_x  i

   4) Next, set i=0 and get j=1 data.

    j
    3  00 00 00 00
    2  00 00 00 00      Each 00 represents 1/4 mesh.
1  >1  XX 00 00 00    
0  >0  -- -- -- --
        0  1  2  3  i

    into khist[0+offset1]...khist[nh-1+offset1]. 
    offset1=nh*(nmesh_x+1)

   5) Now add (0,0),(0,1),(1,0),(1,1) histograms into (0,0) khist to
      get histogram of first mesh.
      and ... get j=0,1 submesh histo.
      
     repeat filling to khist[nh*(nmesh_x+1)-1+offset1]

    j
    3  00 00 00 ... 00
    2  00 00 00 ... 00      Each 00 represents 1/4 mesh.
1  >1  -- -- -- ... XX    
0  >0  -- -- -- ... --
        0  1  2    nmesh_x  i


   6) Swap offset0 (initially =0) and offset1.
      Oh, khist[0+offset0] khist[nh*(nmesh_x+1)-1+offset0] is already filled !
      just as before process 4)
   
    j
    3  00 00 00 ... 00
    2  00 00 00 ... 00      Each 00 represents 1/4 mesh.
0  >1  -- -- -- ... --    
1  >0  -- -- -- ... --
        0  1  2    nmesh_x  i
   
   7) Clear khist[0+offset1] ... khist[nh*(nmesh_x+1)-1+offset1]

    j
    3  00 00 00 ... 00
    2  00 00 00 ... 00      Each 00 represents 1/4 mesh.
0  >1  -- -- -- ... --    
    0  00 00 00 ... 00
        0  1  2    nmesh_x  i

   8) Fill j=2 histo into offset=1 area.

    j
    3  00 00 00 ... 00
1  >2  -- -- XX ... 00      Each 00 represents 1/4 mesh.
0  >1  -- -- -- ... --    
    0  00 00 00 ... 00
        0  1  2    nmesh_x  i

   9) repeats for j.
*/

 for(jm=0;jm<nmesh_y+1;jm++)
   {
     for(i=0;i<nmesh_x+1;i++)
       for(n=0;n<nh;n++)
	 khist[n+nh*i+offset0]=0;

     for(im=0;im<nmesh_x+1;im++)
       {
	 /* im,jm is subscript of submesh i,j */

	 for(j=(int)(jm*meshsiz_y/2.);j<(int)((jm+1)*meshsiz_y/2.);j++)
	   {
	     for(i=(int)(im*meshsiz_x/2.);i<(int)((im+1)*meshsiz_x/2.);i++)
	       {
		 if( i<npx && j<npy && g[i+npx*j]!=pixignr )
		   {
		     mm=(int)floor((g[i+npx*j]-vmin)*huinv+0.5);
		     if(mm>=0 && mm<nh)
		       {
			 khist[mm+nh*im+offset0]=khist[mm+nh*im+offset0]+1;
		       }
		   }
	       }
	   }
	 
	 if(jm!=0 && im!=0)
	   {	 
	     /* jm-1,im-1 is filled */

	     kmax=nmax=0;
	     nhg=nh;


	     for(n=0;n<nh;n++)
	       {
		 /* merging histogram into one */
		 khist[n+nh*(im-1)+offset1]+=khist[n+nh*(im)+offset1]+
		   khist[n+nh*(im-1)+offset0]+khist[n+nh*(im)+offset0];
	       }

/*
        |    _kmax_______        
        |              | |
        |
        |          |           |  _
        |         _|           |_| |
	|________|      ^          |__________
  ym []          0   ..       
khist[]          n0  .. nmax  ..    n1
    
*/
/*--------------------------------------------------------------------------*/


	     /* peak finding & */

	     /* 2004/01/26 */
	     /* threshold should be higher */

	     flag0=(int)(meshsiz_x*meshsiz_y/nh);
	     /*
	       printf("flag0=%d\n",flag0);
	     */
	     flag=flag0;


	     for(n=0;n<nh-2;n++)
	       {
#ifdef DEBUGOUT		 
		 printf("debug:%f %d\n",vmin+hunit*((float)n),khist[n+nh*(im-1)+offset1]);
#endif
		 /* find first rise (at least 2 column >flag0 ) of histogram */		 
		 if(khist[n+nh*(im-1)+offset1]>flag)
		   {
		     if(flag>flag0)break;
		     else flag=khist[n+nh*(im-1)+offset1];
		   }
		 else
		   flag=flag0;
	       }		   

	     n0=n;
	     ym[0]=(double)khist[n0+nh*(im-1)+offset1];
	     nmax=n;
	     kmax=khist[n0+nh*(im-1)+offset1];

	     flag=flag0;

	     for(n=nh-1;n>n0;n--)
	       {
		 /* find last fall (at least 2 column >0 ) of histogram */
		 if(khist[n+nh*(im-1)+offset1]>flag)
		   {
		     if(flag>flag0)break;
		     else flag=khist[n+nh*(im-1)+offset1];
		   }
		 else
		   flag=flag0;
	       }		   

	     n1=n;

	     /***************************************************/

	     if(khist[n+nh*(im-1)+offset1]>kmax)
	       {
		 kmax=khist[n+nh*(im-1)+offset1];
		 nmax=n;
	       }

	     ym[n1-n0]=(double)khist[n1+nh*(im-1)+offset1];

	     for(n=n0+1;n<n1;n++)
	       {
		 if(khist[n+nh*(im-1)+offset1]>kmax)
		   {
		     kmax=khist[n+nh*(im-1)+offset1];
		     nmax=n;
		   }
		 ym[n-n0]=(double)
		   khist[(n+nh*(im-1)+offset1)];
	       }	     	     

	     /* set ym2, instead of shift and copying ym into ym2 */
	     /* cut (nmax +- 0.5 nhg) area into ym2[] 
                nhg=min(nmax-n0,n1-nmax) 
		and move n0 to ym2 zero point, 
                  i.e. ym2[0]=khist[n0] */

#if 1
	     if(nmax-n0<=n1-nmax-1)
	       {
		 nhg=nmax-n0;

		 ym2=ym;
		 ym2+=(int)(0.5*nhg);
		 n00=n0+(int)(0.5*nhg);
	       }
	     else
	       {
		 nhg=n1-nmax;
		 ym2=ym;
		 ym2+=n1-n0-(int)(1.5*nhg)-1;
		 n00=(n1-(int)(1.5*nhg)-1);
	       }
#else
	     ym2=ym;
	     nhg=n1-n0+1;
	     n00=n0;
#endif
	     /*
	       printf("nmax=%d\n",nmax);
	     */

	     if (nmax<nh-2)
	       rmode=vmin+hunit*(nmax);
	     else
	       rmode=0;
	     sigm=0;

	     /* This value(histogram peak point) 
		is used if fitting is not good */

	     if(nhg>4)
	       { 

#if 1
		   p[0]=.5*(double)nhg-1.;
		   p[1]=(double)kmax;
		   p[2]=(double)(nhg/4.);
#else
		 /* rev */
		 p[0]=(double)(nmax-n00);
		 p[1]=(double)kmax;
		 p[2]=(double)(nhg/4.);		   
#endif
		 if(p[2]>fsigm/hunit*3.)
		   p[2]=fsigm/hunit*3.;

#ifdef DEBUGOUT
		 printf("debug: x00=%f\n",vmin+hunit*((float)n00));
		 printf("debug: %f %f %f\n",vmin+hunit*(p[0]+(float)n00),p[1],p[2]);
#endif
		 e[0]=0.01;
		 e[1]=20.0; /* empirical */
		 e[2]=0.1;
		 ncut=10;
		 
		 /* debug out */
#undef DEBUGOUT
#ifdef DEBUGOUT
		 for(kkk=0;kkk<nhg;kkk++)
		   {
		     printf("%f %.0f\n",vmin+hunit*(xm[kkk]+(float)n00),
			    ym2[kkk]);
		   }
#endif
		 if(oyamin2_r(npar,p,e,errgauss,ncut,nhg,xm,ym2,er,f,&chisq)==0)
		   {
		     /* fit and get ymode,xmode and sigm0*/
		     if(p[0]<0)
		       {
			 rmode=vmin+hunit*(float)n00;
		       }
		     else if(p[0]>=nhg)
		       {
			 rmode=vmin+hunit*(float)(nhg+n00);
		       }
		     else
		       {
			 /*
			   xmode=(float)(p[0]+(double)n00);
			   ymode=(float)(p[1]);
			   sigm0 =(float)(p[2]);*/
			 /* return xmode to real unit 
			    HIST = ( REAL - vmin )/ hunit */
			 /*
			   rmode=vmin+hunit*(xmode);
			   sigm=sigm0*hunit;
			 */
			 ymode=(float)(p[1]);
			 rmode=vmin+hunit*((float)(p[0]+(double)n00));
			 sigm=hunit*(float)(p[2]);
			 if(sigm<0)sigm=-sigm;
		       }
		   }
	       }

	     /*
	       printf("%d %d %f %f %f\n",im,jm,rmode,sigm,ymode);
	     */

	     rmesh[(im-1)+nmesh_x*(jm-1)]=(float)rmode;    	     
	     sgmesh[(im-1)+nmesh_x*(jm-1)]=(float)sigm;
	   }
       }

     /* swap offsets */
     if (offset0==0)
       {
	 offset0=nh*(nmesh_x+1);
	 offset1=0;
       }
     else
       {
	 offset1=nh*(nmesh_x+1);
	 offset0=0;
       }

   }

/***** main LOOP end *****/

 free(khist);
 free(ym);
 free(f);
 free(xm);
 free(er);

 return;
}

float pxintp(float *g,int npx,int npy,float x,float y)
{
 /* pixel interpolation */
  int i1,i2,j1,j2;
  float dx1,dx2,dy1,dy2;
  float f;
 
  if (npx<1||npy<1) return 0; /* error */

  i1=(int)floor(x);
  if(i1>=npx-1)i1=npx-2;
  if(i1<0)i1=0;
  i2=i1+1;

  j1=(int)floor(y);
  if(j1>=npy-1)j1=npy-2;
  if(j1<0)j1=0;
  j2=j1+1;

  dx1=x-(float)i1;
  dy1=y-(float)j1;
  dx2=1.-dx1;
  dy2=1.-dy1;

  /* 2000/09/13 i2,j2 check needed here!! */
  if(i2>npx-1) 
    {
      i2=i1;
      dx1=1; 
      dx2=0;
    }
  if(j2>npy-1) 
    {
      j2=j1;
      dy1=1; 
      dy2=0;
    }

  f=dx2*dy2*g[i1+npx*j1]+dx1*dy2*g[i2+npx*j1]+
    dx2*dy1*g[i1+npx*j2]+dx1*dy1*g[i2+npx*j2];
  return f;
}

void skypattern(float *g,int npx,int npy,float *rmesh,int nmesh_x,
		int nmesh_y,int meshsiz_x,int meshsiz_y)
{
  int i,j;
  float rmfi,rmfj;
  float x,y;
  
  rmfi=1./(float)(meshsiz_x/2); /* 2./meshsize */
  rmfj=1./(float)(meshsiz_y/2);

  for(j=0;j<npy;j++)
    {
      for(i=0;i<npx;i++)
	{
	  if(nmesh_x>1)
	    x=((float)i+.5)*rmfi-1.;
	  else
	    x=0;
	  if(nmesh_y>1)
	    y=((float)j+.5)*rmfj-1.;
	  else
	    y=0;
	  
	  g[i+npx*j]=pxintp(rmesh,nmesh_x,nmesh_y,x,y);
	}
   }
 return ;
}

void skysub(float *g,int npx,int npy,float *rmesh,int nmesh_x,
	    int nmesh_y,int meshsiz_x,int meshsiz_y,
	    int mode, float pixignr)
{
 int i,j;
 float rmfi,rmfj;
 float x,y,sk;

 rmfi=1./(float)(meshsiz_x/2); /* 2./meshsize */
 rmfj=1./(float)(meshsiz_y/2);

 
 for(j=0;j<npy;j++)
   {
     for(i=0;i<npx;i++)
       {
	 if(nmesh_x>1)
	   x=((float)i+.5)*rmfi-1.;
	 else
	   x=0;
	 if(nmesh_y>1)
	   y=((float)j+.5)*rmfj-1.;
	 else
	   y=0;
	 
	 sk=pxintp(rmesh,nmesh_x,nmesh_y,x,y);
	 
	 if (g[i+npx*j]!=(float)pixignr)
	   {
	     switch(mode)
	       {
	       case 0: /* photo */
		 g[i+npx*j]=(g[i+npx*j]-sk)/sk;
		 break; 
	       case 1: /* CCD */
		 g[i+npx*j]=(g[i+npx*j]-sk);
		 break;
	       case 2: /* sky */
		 g[i+npx*j]=sk;
		 break;
	       default:
		 break;
	       }
	   };
       }
   }
 return ;
}
 
