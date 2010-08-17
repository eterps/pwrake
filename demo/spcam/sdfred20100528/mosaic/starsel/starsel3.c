#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>


#include "imc.h"
#include "lsf.h"
#include "obj3.h"
#include "detect_sub3.h"
#include "imopr.h"

#include "moment2d.h"
#include "getargs.h"
#include "apphot.h"

#define frame(x,y,imx,imy) (((x)>= 0)&&((x)<(imx))&&((y)>= 0)&&((y)<(imy)))

/* as DEBUG */
#define DEBUG 0

float ab_r( 
	   const float  *g,      /* (I) vector for image */
	   const int   iext,    /* (I) x-size of image */
	   const int   jext,    /* (I) y-size of image */
	   const float  dlv     /* (I) threshold (linear) */
	   )
{
  moment2d m={0};
  int xdim=2,ydim=2,ndim=2;
  int i,j;
  float q=0;
  float x2,y2,xy,npix0;
  double P,Q,s,c;

  init_moment2d(&m,xdim,ydim,ndim);
  for(j=0;j<jext;j++)
    for(i=0;i<iext;i++)
      if(g[i+iext*j]>dlv)
	add_moment2d (&m,(float)i,(float)j,1.);
 
  npix0=m.dat[0+(xdim+1)*0];
  x2=m.dat[2+(xdim+1)*0]/npix0;
  y2=m.dat[0+(xdim+1)*2]/npix0;
  xy=m.dat[1+(xdim+1)*1]/npix0;
  free_moment2d(&m);
  /*
      a=sqrt((P+Q)/2.);
      b=sqrt((P-Q)/2.);
  */

  /*
  printf("debug:x2=%f\n",x2);
  printf("debug:y2=%f\n",y2);
  printf("debug:xy=%f\n",xy);
  */

  P=4.*(x2+y2);
  s=2.*xy;
  c=y2-x2;
  Q=4.*sqrt(c*c+s*s);

  q=sqrt((P-Q)/(P+Q));

  /*
    printf("debug:q=%f\n",q);
  */

  
  return q;
}


void   reject_obj(objrec_t **ob,  /* object record */
		  int rej_num)
/*----------------------------------------------------------*/
{
   objrec_t  *p, *q;
   q = *ob;
   for( p = *ob ; p != NULL ; p = p->next )
     {
       if( p->entnum == rej_num )
	 {
	   if( p == *ob )
	     {
	       *ob = p->next;
	     }
	   else
	     {
	       q->next = p->next;
	     }
	   free_objrec(p);
	   return;
	 }
       q = p;
     }
   printf("rejection failed\n");
 }

/*----------------------------------------------------------*/

int   select_obj(objrec_t          **ob,   /* detected object */
                 float    reject_crit_1,   /* rejection criterion for
					      Ipeak/Npix test */
                 float    reject_crit_2,    /* rejection criterion for
					       axis-ratio test */
		 int IM_RAD_MAX)

/*----------------------------------------------------------*/
/* This routine reject the object which have deviated Ipeak/Npix value.
*/
{
   int       sstar = 0, nobj = 0;
   float     value, npix, sval = 0.0 , s2val = 0.0, mean, sigma;
   objrec_t  *p, *q;
   int npixmin;

   npixmin=(2*IM_RAD_MAX+1)*(2*IM_RAD_MAX+1);

   /* axis ratio test */

   for(p = *ob; p != NULL ; p = q)
     {
       q=p->next;

       /* q=a/b */
       if( p->q > reject_crit_2 )
	 {	   
	   sstar++;
	 }
       else
	 {
	   reject_obj(ob, p->entnum);
	 }
     }

   /* Image extent test */

   sstar = 0;

   for(p = *ob; p != NULL ; p = q)
     {
       q=p->next;
       npix = (float) p->npix;

       if( p->npix <= npixmin )
	 {
	   /*
	     value = log10( p->peak / npix ) / log10( p->fiso );
	   */
	   value = log(p->peak/npix)/log(p->fiso);
	   
	   sval  += value;
	   s2val += value*value;
	   sstar++;
	 }
       else
	 {
	   reject_obj(ob, p->entnum);
	 }       
     }
   
   /* Ipeak/Npix test */

   nobj=sstar;
   sstar = 0;

   mean  = sval / (float)nobj;
   sigma = sqrt( s2val / (float)nobj - mean*mean );

   for(p = *ob; p != NULL ; p = q)
     {
       q=p->next;
       npix = (float) p->npix;
       
       value = log10( p->peak / npix ) / log10( p->fiso );
       
       if( value >= mean - reject_crit_1 * sigma )
	 {	   
	   sstar++;
	 }
       else
	 {
	   reject_obj(ob, p->entnum);
	 } 
     }
   return sstar;
 }


/*----------------------------------------------------------*/

int    det_center(float         *pix,  /* pix data */
		  int            npx,
		  int            npy,
		  objrec_t       **p)  /* object record */

/*----------------------------------------------------------*/
{
   int  x0,y0;
   double vxa,vxc,vxb;
   double vya,vyc,vyb;

   x0 = (*p)->ipeak;
   y0 = (*p)->jpeak;
   /*
     printf("debug: %d %d\n",x0,y0);
   */
   vxa=pix[x0-1 + npx*y0];
   vxb=pix[x0+1 + npx*y0];
   vya=pix[x0 + npx*(y0-1)];
   vyb=pix[x0 + npx*(y0+1)];
   vxc=vyc=pix[x0 + npx*y0];

   if( vxa==vxc || vxc==vxb || vya==vyc || vyc==vyb )
     {
       (*p)->xc = (float)(*p)->ipeak;
       (*p)->yc = (float)(*p)->jpeak;
       return 0; /* NG */
     }   
   else if (vxa<=0 || vxc<=0 || vxb<=0 || vya<=0 || vyb<=0)
     {
       (*p)->xc = (float)(*p)->ipeak;
       (*p)->yc = (float)(*p)->jpeak;
       return 0; /* NG */
     }
   else
     {
       (*p)->xc = (((float)x0+0.5)*log(vxc/vxa)+((float)x0-0.5)*log(vxc/vxb))
	 /log(vxc*vxc/vxa/vxb);
       (*p)->yc = (((float)y0+0.5)*log(vyc/vya)+((float)y0-0.5)*log(vyc/vyb))
	 /log(vyc*vyc/vya/vyb);
       return 1; /* OK */
     }
 }

/*----------------------------------------------------------*/

int   pixignr_check(float         *pix,  /* pix data */
		    int            npx,
		    int            npy,
		    objrec_t      **ob,
		    int IM_RAD_MAX,
		    float pixignr
		    )  /* object record */

/*----------------------------------------------------------*/

{
  int  i, j, x0, y0;

  x0 = (*ob)->ipeak;
  y0 = (*ob)->jpeak;

#define frame(x,y,imx,imy) (((x)>= 0)&&((x)<(imx))&&((y)>= 0)&&((y)<(imy)))
  
  if (!frame(x0-IM_RAD_MAX-1, y0-IM_RAD_MAX-1, npx, npy)|| 
      !frame(x0+IM_RAD_MAX+1, y0+IM_RAD_MAX+1, npx, npy))
    {
      return 0;
    }  
  
  for(j=y0-(IM_RAD_MAX+1); j<=y0+(IM_RAD_MAX+1); j++)
    for(i=x0-(IM_RAD_MAX+1); i<=x0+(IM_RAD_MAX+1); i++)
      if(( pix[i + j * npx] == pixignr ))
	return 0;

/* edge test ?? 
  for(j=y0-(IM_RAD_MAX+1); j<=y0+(IM_RAD_MAX+1); j++)
    {
      if( ( pix[x0 - (IM_RAD_MAX +1) + j * npx] == pixignr ) ||
	 ( pix[x0 + (IM_RAD_MAX +1) + j * npx] == pixignr ) ){
	return 0;
      }
    }


    for(i=x0-(IM_RAD_MAX+1); i<=x0+(IM_RAD_MAX+1); i++)
    {
      if( ( pix[ i + (y0 - (IM_RAD_MAX +1)) * npx] == pixignr ) ||
	 ( pix[ i + (y0 + (IM_RAD_MAX +1)) * npx] == pixignr ) )
	{
	  return 0;
	}
    }
*/

  
  return 1;
}

/*----------------------------------------------------------*/

void   mk_psfob(objrec_t  **ob,   /* detected object */
		objrec_t  **psfob)   /* selected object list head
					   to make PSF */
/*----------------------------------------------------------*/
{
   objrec_t  *p,*q;

   p = (objrec_t *) malloc(sizeof(objrec_t));

   if( p == NULL ){
     printf("Cannot allocate memory : psfob \n");
     exit(0);
   }

   p->entnum = (*ob)->entnum;
   p->fiso   = (float) (*ob)->fiso;
   p->peak   = (float) (*ob)->peak;
   p->ipeak  = (*ob)->ipeak;
   p->jpeak  = (*ob)->jpeak;
   p->xmin   = (*ob)->xmin;
   p->xmax   = (*ob)->xmax;
   p->ymin   = (*ob)->ymin;
   p->ymax   = (*ob)->ymax;
   p->xc     = (float) (*ob)->xc;
   p->yc     = (float) (*ob)->yc;
   p->npix   = (*ob)->npix;
   p->next   = NULL;
   /* 1 pix margin */
   p->img=(*ob)->img;
   (*ob)->img=NULL;
   p->map=(*ob)->map;
   (*ob)->map=NULL;
   p->q=(*ob)->q;

   if( *psfob == NULL )
     {
       *psfob = p; /* head */
     }
   else
     {
       for(q = *psfob; q->next != NULL; q = q->next); /* find tail */
       q->next = p;
     }
}

int detectpsf(float *pix, int npx, int npy, 
	      float iso, float npix,
	      int rad,
	      int ndiv,
	      float infcri, float supcri,
	      float crit1,
	      float crit2,
	      float pixignr,
	      objrec_t **psfob)
{
  objrec_t  *ob, ob0, *ob1, *q;	/* object record */
  int i, j, nobj, sobj;
  int yshift;

  if(*psfob!=NULL)
    {
      free_objrec_list(*psfob);
      *psfob=NULL;
    }
  
  nobj=0;
  j=1;
  ob1=&ob0;
  for(i=0;i<ndiv;i++)
    {
      yshift=npy/ndiv*i;
      /*
	printf("%d\n",i);
      */

      nobj+=detect_simple(pix+npx*yshift,npx,npy/ndiv,iso,
			  npix,&ob);
      if (ob!=NULL)
	{
	  ob1->next=ob;
	  ob1=ob1->next;
	  while(ob1->next!=NULL) 
	    {
	      /*
		printf("%d\n",j);
	      */
	      ob1->entnum=j++;
	      ob1->yc+=(float)yshift;
	      ob1->jpeak+=yshift;
	      ob1->ymin+=yshift;
	      ob1->ymax+=yshift;
	      ob1=ob1->next;
	    }
	}
    }
  ob=ob0.next;
  
  printf("detected objects = %d\n", nobj);
  
  if(nobj==0)
    {
      free_objrec_list(ob0.next);
      return -1;
    }
  
  /* Determination of the Star Image Center
     and Determination of the Star Peak
     and Star Selection for PSF Making 
     and pixignr check */
  
  for(q=ob; q != NULL; q=q->next) 
    { 
      /* crit test */
      /*
	printf("debug: id:%d\n",q->entnum);
      */
      
      if (q->peak<infcri || q->peak>supcri) continue;
      if (!pixignr_check(pix, npx, npy, &q, rad, pixignr)) continue;
      if(det_center(pix, npx, npy, &q))
	/* The order of the check should be this order */
	{
	  q->img=(float*)malloc((q->xmax-q->xmin+3)*(q->ymax-q->ymin+3)*
				sizeof(float));
	  imextra(pix,npx,npy,
		  q->xmin-1,q->ymin-1,q->xmax+1,q->ymax+1,
		  q->img);
	  q->q=ab_r(q->img,(q->xmax-q->xmin+3),(q->ymax-q->ymin+3),
		    iso);
	  mk_psfob(&q, psfob);	     
	}
    }	
  free_objrec_list(ob0.next);
  
  if((*psfob) == NULL)
    {
      printf("No star candidates found\n");
      return 0;
    }
  
  /* Select object roughly by Ipeak/Npix and axis ratio */
  sobj = select_obj( psfob, crit1, crit2, rad);
  printf("selected stars = %d\n",sobj);
  if( sobj ==0 ||  (*psfob) == NULL)
    {
      printf("No star candidates found\n");
      /* case */
      /* detect_iso */
      
      return 0;
    }
  return sobj;
}


int main(int argc, char** argv)
{
    struct imh	imhin={""};		/* imh header structure */
    struct icom	icomin={0};		/* imh icominent structure */


    
    char	fname[BUFSIZ]="";	/* image file name */
    char	fnamcatout[BUFSIZ]="";	/* output catalog file name */

    FILE	*fp_pix;	/* pointer to pix file */
    FILE	*fp;
    float	*pix;		/* pixel array */
    int 	pdim;		/* pixel data size */
    int 	npx, npy;	/* pixel data size */
    objrec_t	*psfob=NULL, *q=NULL;	        /* object record used to make PSF */

    int         sobj;           /* number of selected object */



    int         i,k;            /* do loop */



    float       infcri=0,supcri=10000;  /* selection criteria */







    

  
    float	DETECT_LEVEL = 3.0;
    float	DETECT_ISO = 0.0;   
    float	DETECT_ISO_FACTOR = 1.0;   

    int	        MINIMUM_PIXEL = 10;
    


    float   REJECT_CRIT_1 = 0.0;     /* Rejection criteria mean-(*)sigma
					for star selection (Ipeak/Npix test) */

    float   REJECT_CRIT_2 = 0.5;    /* Rejection criteria for star 
					selection.
					((xmax-xmin)/(ymax-ymin) test) */
    /* Needed */
    int     IM_RAD_MAX = 20;         /* Initial PSF Radius and Max Value
					when radius is undetermined */
    float SKY_SIGMA=0.;


    int nsky;
    float sky_mean,sky=FLT_MAX;
    float pixignr;

    int niter=5;
    int nstarmin=50,nstarmax=300;
    int flag;
    int ndiv=10;





    /* 2000/10/21 */
    float aprad=10.0;

  getargopt opts[20];
  int n=0;
  char *files[3]={NULL};
  int helpflag=0;

  files[0]=fname;

  n=0;
  setopts(&opts[n++],"-nskysigma=", OPTTYP_FLOAT , &DETECT_LEVEL,
	  "detection threshold=%%f * skysigma (default:3.0)");
  setopts(&opts[n++],"-iso=", OPTTYP_FLOAT, &DETECT_ISO,
	  "set detection threshold=%%f (default: not used)");
  setopts(&opts[n++],"-niso=", OPTTYP_FLOAT, &DETECT_ISO_FACTOR,
	  "-iso=%%f is multiplied by factor %%f (default: 1.0)");
  setopts(&opts[n++],"-npix=", OPTTYP_INT, &MINIMUM_PIXEL,
	  "minimum connected pixel (default:10)\n");
  setopts(&opts[n++],"-ndiv=", OPTTYP_INT , &ndiv,
	  "number of division of image in detection (default:10)");
  setopts(&opts[n++],"-sky=", OPTTYP_FLOAT , &sky,
	  "background sky (default:estimated from image)");
  setopts(&opts[n++],"-rpsf=", OPTTYP_INT, &IM_RAD_MAX,
	  "maximun radius for PSF (default:20)");



  setopts(&opts[n++],"-criteria1=", OPTTYP_FLOAT, &REJECT_CRIT_1,
	  "peak-npix criteria (default:0)");
  setopts(&opts[n++],"-aratiomin=", OPTTYP_FLOAT, &REJECT_CRIT_2,
	  "axial ratio minimum limit(default:0.5)");

  setopts(&opts[n++],"-peakmin=", OPTTYP_FLOAT , &infcri,
	  "minimum peak value (default:0)");
  setopts(&opts[n++],"-peakmax=", OPTTYP_FLOAT , &supcri,
	  "maximum peak value (default:10000)");

  setopts(&opts[n++],"-outmes=", OPTTYP_STRING , &fnamcatout,
	  "output catalog name (default: (infile).mes)");
  setopts(&opts[n++],"-aprad=", OPTTYP_FLOAT, &aprad,
	  "aperture flux radius(default:10.0)");


  setopts(&opts[n++],"-nmin=", OPTTYP_INT , &nstarmin,
	  "minimum number of output (default:50)");
  setopts(&opts[n++],"-nmax=", OPTTYP_INT , &nstarmax,
	  "maximum number of output (default:300)");
  setopts(&opts[n++],"-niter=", OPTTYP_INT , &niter,
	  "rejection iteration cycle (default:5)");

  /*
    setopts(&opts[n++],"-i", OPTTYP_FLOAT, &DETECT_ISO,
    "");
    setopts(&opts[n++],"-m", OPTTYP_INT, &MINIMUM_PIXEL,
    "");
    setopts(&opts[n++],"-r", OPTTYP_INT, &IM_RAD_MAX,
    "");
    setopts(&opts[n++],"-s", OPTTYP_FLOAT, &REJECT_CRIT_1,
    "(default:0)");
    setopts(&opts[n++],"-a", OPTTYP_FLOAT, &REJECT_CRIT_2,
    "(default:0)");
  */
  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(ndiv<=1) ndiv=1;

  if(fname[0]=='\0')
    {
      printf("Error: No input file specified!!\n");
      helpflag=1;
    }    

  if(helpflag==1)
    {
      /* print usage */
      print_help("Usage: starsel3 [options] (imagefile)",
		 opts,"");
      exit(-1);
    }

    DETECT_ISO*=DETECT_ISO_FACTOR;
    
    /* Image file open to read */
    
    if ((fp_pix = imropen(fname, &imhin, &icomin)) == NULL)
      {
	fprintf(stderr,"Cannot read file %s\n",fname);
	exit(1);
      }
    
    /* Pick up pixignr value */
    pixignr = (float)imget_pixignr(&imhin);
    
    /* Allocaton of pixel data memory */
    
    npx  = imhin.npx;
    npy  = imhin.npy;
    pdim = npx * npy;
    
    pix = (float*) malloc(pdim*sizeof(float));
    
    /* Read pixel data */
    if (imrall_tor(&imhin, fp_pix, pix, pdim) == 0)
      {
	fprintf(stderr,"Cannot read file %s\n",fname);
	exit(1);
      }
    
    imclose(fp_pix,&imhin,&icomin);

    /* Detection */
    /* determine thres if -d is used */
    /*
      printf("sky=%f\n",sky);
    */

    skydet(pix, npx, npy, &nsky, &sky_mean, &SKY_SIGMA, pixignr);

    if(sky==FLT_MAX) 
      sky=sky_mean;

    if(DETECT_ISO<=0)
      {
	DETECT_ISO=SKY_SIGMA*DETECT_LEVEL;
      }
    /* skysb */
    printf("sky=%f\n",sky);
    for(i=0;i<npx*npy;i++)
      if (pix[i]!=pixignr) 
	{
	  pix[i]-=sky;
	}

    /****************************************************************/

    printf("adopted number of PSFs [%d, %d]\n",
	   nstarmin, nstarmax);

    flag=0;
    for(k=0;k<niter;k++)
      {
	printf("\niso=%f npix=%d\n",DETECT_ISO,MINIMUM_PIXEL);
	sobj=detectpsf(pix,npx,npy,
		       DETECT_ISO, 
		       MINIMUM_PIXEL,
		       IM_RAD_MAX,		  
		       ndiv,
		       infcri,supcri,
		       REJECT_CRIT_1,
		       REJECT_CRIT_2,
		       pixignr,
		       &psfob);
	if(sobj<0)
	  {
	    /* no object detected */
	  }
	else if (sobj==0)
	  {
	    /* all objects are rejected */
	    REJECT_CRIT_1-=1.0;
	  }
	else if(sobj>nstarmax)
	  {
	    /* test. change iso */
	    printf("  too many. ");
	    if(flag==0)
	      DETECT_ISO*=2.0;
	    else
	      MINIMUM_PIXEL=(int)ceil((double)MINIMUM_PIXEL*1.5);
	    flag=(flag+1)%2;
	  }
	else if(sobj<nstarmin)
	  {
	    /* test change iso */
	    printf("  too few. ");
	    if(flag==0)
	      MINIMUM_PIXEL=(int)ceil((double)MINIMUM_PIXEL*0.65);
	    else
	      DETECT_ISO*=0.5;
	    flag=(flag+1)%2;
	  }
	else
	  {
	    printf("debug: thres=%f npix=%d crit1=%f\n",DETECT_ISO,MINIMUM_PIXEL,REJECT_CRIT_1);
	    break;
	  }
	if (k<niter-1) 
	  {
	    free_objrec_list(psfob);
	    psfob=NULL;
	    printf("  retrying ...\n");
	  }
	else printf("  iterated %d times\n",niter);
      }

    /****************************************************************/
	
    if (fnamcatout[0]=='\0')
      sprintf(fnamcatout,"%s.mes",fname);
    
    fp=fopen(fnamcatout,"wt");
    if(fp==NULL)
      {
	fprintf(stderr,"Cannot write output catalog \"%s\"\n",
		fnamcatout);
	exit(-1);
      }
    for(q=psfob; q != NULL; q=q->next)
      {
	/* 2000/10/21 aperture added */
	/* apphot */
	q->fiso=aperturePhotometry(pix,npx,npy,q->xc,q->yc,aprad);
	fprintf(fp, "%d %.3f %.3f %.0f %d %.0f %d %d %d %d %d %d\n",
		q->entnum, q->xc, q->yc, q->fiso, 
		q->npix,q->peak, 
		q->xmin, q->xmax, q->ymin, q->ymax, 
		q->ipeak, q->jpeak);
      }
    fclose(fp);

    free_objrec_list(psfob);
    free(pix);
    printf("\n");
    return 0;
}
