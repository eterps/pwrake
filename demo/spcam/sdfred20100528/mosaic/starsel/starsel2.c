/* renewal of psf2.c (12/3) */
/* 1999/11/15, Yagi changed for star selecter */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <limits.h>

#include "imc.h"
#include "lsf.h"

#include "obj3.h"
#include "detect_sub3.h"
#include "moment2d.h"

#include "imopr.h"
#include "getargs.h"

#define BUFSIZE        150

#define frame(x,y,imx,imy) (((x)>= 0)&&((x)<(imx))&&((y)>= 0)&&((y)<(imy)))

#include "apphot.h"

/* as DEBUG */
#define DEBUG 0

static  float   PIXIGNRVAL = -500.0;     /* PIXIGNR value */
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

/*----------------------------------------------------------*/
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

   /* axis ratio test */

   for(p = *ob; p != NULL ; p = q)
     {
       q=p->next;

       /* q=a/b */

       if(p->q>0)
	 {
	   printf("%d %f\n",p->entnum,p->q);
	   value=p->q;
	 }
       else
	 {

	   if( (p->xmax-p->xmin) < (p->ymax-p->ymin) )
	     {
	       value = (float)(p->ymax-p->ymin+1) / (float)(p->xmax-p->xmin+1);
	     }
	   else 
	     {
	       value = (float)(p->xmax-p->xmin+1) / (float)(p->ymax-p->ymin+1);
	     }
	 }
       
       if( value > reject_crit_2 )
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
       if (p->npix<0) /* old version */
	 npix = (float) ((p->xmax-p->xmin+1)*(p->ymax-p->ymin+1));
       else
	 npix = (float) p->npix;

       if( p->npix <= (2*IM_RAD_MAX+1)*(2*IM_RAD_MAX+1))
	 {
	   value = log10( p->peak / npix ) / log10( p->fiso );
	   
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

   vxa=pix[x0-1 + npx*y0];
   vxb=pix[x0+1 + npx*y0];
   vya=pix[x0 + npx*(y0-1)];
   vyb=pix[x0 + npx*(y0+1)];
   vxc=vyc=pix[x0 + npx*y0];

   if( vxa==vxc ||  vxc==vxb || vya==vyc || vyc==vyb )
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

int   crit_test(objrec_t          **ob,   /* detected object */
                int                npx,   /* pixel data size */
                int                npy,
                float           infcri,   /* inferior criterion */
                float           supcri,   /* superior criterion */
		int IM_RAD_MAX)  

/*----------------------------------------------------------*/

{
   float     value;

   value = (float)(*ob)->peak ;

   if( ( value < infcri ) || ( value > supcri ) )
     {
       return 0; /* NG */
     }
   else
     {
       return 1; /* OK */
     }
 }

/*----------------------------------------------------------*/

int   pixignr_check(float         *pix,  /* pix data */
		    int            npx,
		    int            npy,
		    objrec_t      **ob,
		    int IM_RAD_MAX)  /* object record */

/*----------------------------------------------------------*/

{
  int  i, j, x0, y0;

  x0 = (*ob)->ipeak;
  y0 = (*ob)->jpeak;

  
  if (!frame(x0-IM_RAD_MAX-1, y0-IM_RAD_MAX-1, npx, npy)|| 
      !frame(x0+IM_RAD_MAX+1, y0+IM_RAD_MAX+1, npx, npy))
    {
      return 0;
    }  
  
  for(j=y0-(IM_RAD_MAX+1); j<=y0+(IM_RAD_MAX+1); j++)
    for(i=x0-(IM_RAD_MAX+1); i<=x0+(IM_RAD_MAX+1); i++)
      if(( pix[i + j * npx] == PIXIGNRVAL ))
	return 0;

  /* edge test ?? 
  for(j=y0-(IM_RAD_MAX+1); j<=y0+(IM_RAD_MAX+1); j++)
  {
  if( ( pix[x0 - (IM_RAD_MAX +1) + j * npx] == PIXIGNRVAL ) ||
  ( pix[x0 + (IM_RAD_MAX +1) + j * npx] == PIXIGNRVAL ) ){
  return 0;
  }
  }

  
  for(i=x0-(IM_RAD_MAX+1); i<=x0+(IM_RAD_MAX+1); i++)
  {
  if( ( pix[ i + (y0 - (IM_RAD_MAX +1)) * npx] == PIXIGNRVAL ) ||
  ( pix[ i + (y0 + (IM_RAD_MAX +1)) * npx] == PIXIGNRVAL ) )
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


int main(int argc, char** argv)
{
  struct imh	imhin={""};		/* imh header structure */
  struct icom	icomin={0};		/* imh icominent structure */
  struct imh	imhout={""};
  struct icom	icomout={0};
  
  char	fname[BUFSIZ]="";	/* image file name */
  char	fnamcatout[BUFSIZ]="";	/* output catalog file name */
  char	fnampos[BUFSIZ]="";	/* postage stamp file name */
  FILE	*fp_pix;	/* pointer to pix file */
  FILE	*fp;
  float	*pix;		/* pixel array */
  int 	pdim;		/* pixel data size */
  int 	npx, npy;	/* pixel data size */
  objrec_t	*ob, *q;	/* object record */
  objrec_t	*psfob=NULL;	        /* object record used to make PSF */
  int		nobj;		/* number of detected object */
  int         sobj;           /* number of selected object */


  char        line[BUFSIZ];       /* temporary */



  char        basename[BUFSIZ]="";   /* local output file name */
  
  float       infcri=0,supcri=FLT_MAX;  /* selection criteria */
  


  



  
  float	DETECT_LEVEL = 3.0;
  float	DETECT_ISO = 0.0;   
  float	DETECT_ISO_FACTOR = 1.0;   
  
  int	        MINIMUM_PIXEL = 10;
  

  float   REJECT_CRIT_1 = 0.0;     /* Rejection criteria mean-(*)sigma
				      for star selection (Ipeak/Npix test) */
  float   REJECT_CRIT_2 = 0.25;    /* Rejection criteria for star 
				      selection.
				      ((xmax-xmin)/(ymax-ymin) test) */
  /* Needed */
  int     IM_RAD_MAX = 20;         /* Initial PSF Radius and Max Value
				      when radius is undetermined */
  float SKY_SIGMA=0.;
  


  int nsky;
  float sky_mean;
  


  float *tmp;
  int xmin,xmax,ymin,ymax;
  /* 2000/10/21 */
  float aprad=-1.0;
  
  getargopt opts[20];
  char *files[3]={NULL};
  int helpflag=0;
  int n=0;
  
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
  
  setopts(&opts[n++],"-criteria1=", OPTTYP_FLOAT, &REJECT_CRIT_1,
	  "peak-npix criteria (default:0)");
  setopts(&opts[n++],"-aratiomin=", OPTTYP_FLOAT, &REJECT_CRIT_2,
	  "axial ratio minimum limit(default:0.25)");

  setopts(&opts[n++],"-peakmin=", OPTTYP_FLOAT , &infcri,
	  "minimum peak value (default:0)");
  setopts(&opts[n++],"-peakmax=", OPTTYP_FLOAT , &supcri,
	  "maximum peak value (default:10000)");

  setopts(&opts[n++],"-rpsf=", OPTTYP_INT, &IM_RAD_MAX,
	  "maximun radius for PSF (default:20)");

  /* not impl. yet
     setopts(&opts[n++],"-inmes=", OPTTYP_STRING , &fnamcatin2,
     "input catalog name *.mes (default: not used)");
  */

  /*
    setopts(&opts[n++],"-indet=", OPTTYP_STRING , &fnamcatin1,
    "input catalog name *.det (default: not used)");
  */
  setopts(&opts[n++],"-basename=", OPTTYP_STRING , basename,
	  " for output postage stamp name (default: not used)\n");

  setopts(&opts[n++],"-outmes=", OPTTYP_STRING , &fnamcatout,
	  "output catalog name (default: (infile).mes)");
  setopts(&opts[n++],"-aprad=", OPTTYP_FLOAT, &aprad,
	  "aperture flux radius(default:not used)");

  /* not impl.
     setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero,
     "bzero");
     setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale,
	  "bscale");
    setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
    "datatyp(FITSFLOAT,FITSSHORT...)");
    setopts(&opts[n++],"-pixignr=", OPTTYP_INT , &pixignr,
    "pixignr value");
  */

  /* lower compatibility */
  setopts(&opts[n++],"-d", OPTTYP_FLOAT, &DETECT_LEVEL,
	  "same as -nskysigma=");
  setopts(&opts[n++],"-i", OPTTYP_FLOAT, &DETECT_ISO,
	  "same as -iso=");
  setopts(&opts[n++],"-m", OPTTYP_INT, &MINIMUM_PIXEL,
	  "same as -npix=");
  setopts(&opts[n++],"-r", OPTTYP_INT, &IM_RAD_MAX,
	  "same as -rpsf=");
  setopts(&opts[n++],"-s", OPTTYP_FLOAT, &REJECT_CRIT_1,
	  "same as -criteria1=");
  setopts(&opts[n++],"-a", OPTTYP_FLOAT, &REJECT_CRIT_2,
	  "same as -aratiomin=");

  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fname[0]=='\0')
    {
      printf("Error: No input file specified!!\n");
      helpflag=1;
    }    

  if(helpflag==1)
    {
      /* print usage */
      print_help("Usage: starsel2a [options] (imagefile)",
		 opts,"");
      exit(-1);
    }
  

  if(fname[0]=='\0')
    {
      printf("Error: No input file specified!!\n");
      helpflag=1;
    }    
  if(helpflag==1)
    {
      /* print usage */
      print_help("Usage: starsel2a [options] (imagefile)",
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
     PIXIGNRVAL = (float)imget_pixignr(&imhin);

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

    /* inherit */


     /* Detection (or read catalog) */
     /* determine thres if -d is used */
     if(DETECT_ISO<=0)
       {
	 skydet(pix, npx, npy, &nsky, &sky_mean, &SKY_SIGMA, PIXIGNRVAL);
	 DETECT_ISO=SKY_SIGMA*DETECT_LEVEL;
       }
     printf("detect iso=%f npix=%d\n",DETECT_ISO,MINIMUM_PIXEL);
     nobj=detect_simple(pix,npx,npy,DETECT_ISO,
			MINIMUM_PIXEL,&ob);
     printf("%d objects detected\n",nobj);

     printf("Number of detected objects : %d\n", nobj);

     /* Star Selection (rough) */

     /* Determination of the Star Image Center
	and Determination of the Star Peak
	and Star Selection for PSF Making 
	and PIXIGNR check */

     for(q=ob; q != NULL; q=q->next) 
       { 
	 /* crit test */
	 if (q->peak<infcri || q->peak>supcri) continue;
	 if (!pixignr_check(pix, npx, npy, &q, IM_RAD_MAX)) continue;
	 if(det_center(pix, npx, npy, &q))
	   /* The order of the check should be this order */
	   {
	     q->img=(float*)malloc((q->xmax-q->xmin+3)*(q->ymax-q->ymin+3)*
				   sizeof(float));
	     imextra(pix,npx,npy,
		     q->xmin-1,q->ymin-1,q->xmax+1,q->ymax+1,
		     q->img);
	     q->q=ab_r(q->img,(q->xmax-q->xmin+3),(q->ymax-q->ymin+3),
		       DETECT_ISO);
	     mk_psfob(&q, &psfob);	     
	   }
      }	

    if(psfob == NULL)
      {
	printf("No star candidates found\n");
	exit(0);
      }

    /* Select object roughly by Ipeak/Npix and axis ratio */
    
    sobj = select_obj( &psfob, REJECT_CRIT_1, REJECT_CRIT_2, 
		      IM_RAD_MAX);
        
    printf("selected stars = %d\n",sobj);
    if( sobj == 0 || psfob == NULL)
      {
	printf("No star candidates found\n");
	exit(0);
      }
    
    printf("selected object : ");
    
    if (basename[0]!='\0')
      {
	tmp=(float*)malloc(sizeof(float)*(2*IM_RAD_MAX+1)*(2*IM_RAD_MAX+1));
	
	/* here catalogs should be written */
	for(q=psfob; q != NULL; q=q->next)
	  {
	    fprintf(stderr,"%d ", q->entnum); 
	    
	    xmin=(int)floor(q->xc+0.5)-IM_RAD_MAX;
	    xmax=xmin+2*IM_RAD_MAX;
	    ymin=(int)floor(q->yc+0.5)-IM_RAD_MAX;
	    ymax=ymin+2*IM_RAD_MAX;
	    
	    imextra(pix,npx,npy,
		    xmin,ymin,xmax,ymax,tmp);
	    
	    sprintf(fnampos,"%s%d.fits",basename,q->entnum);
	    
	    imh_inherit(&imhin,&icomin,&imhout,&icomout,fnampos);
	    imhout.ndatx = imhout.npx   = 2*IM_RAD_MAX+1;
	    imhout.ndaty = imhout.npy   = 2*IM_RAD_MAX+1;
	    imhout.dtype= DTYPFITS;
	    imc_fits_set_dtype(&imhout,FITSFLOAT,0.0,1.0);

	    (void) sprintf( line, "%-8.8s=%22.4f /  %-45.45s", 
			   "THRESHLD", DETECT_ISO, 
			   "DETECTION THRESHOLD");
	    if( imrep_fits( &icomout, "THRESHLD", line ) == 0 ) 
	      {
		(void) imadd_fits( &icomout , line );
	      }       

	    sprintf(line,"PSF of %s : (%d,%d)-(%d,%d)",		
		    fname,xmin,ymin,xmax,ymax);	
	    imencs(imhout.title,line);
	    
	    sprintf(line,"peak position (%f,%f)",
		    q->xc-(float)xmin,q->yc-(float)ymin);
	    imaddcom(&icomout,line);
	    
	    fp=imwopen(fnampos,&imhout,&icomout);
	    
	    if (fp!=NULL)
	      imwall_rto( &imhout, fp, tmp);
	    imclose(fp,&imhout,&icomout);
	    
	    /* with makePostage, header information vanishes ...
	       makePostage(fnampos,tmp,
	       2*IM_RAD_MAX+1,
	       2*IM_RAD_MAX+1);
	       */
	  }
      }

    imclose(fp_pix,&imhin,&icomin);

    if (fnamcatout[0]=='\0')
      sprintf(fnamcatout,"%s.mes",fname);


    fp=fopen(fnamcatout,"wt");
    if(fp==NULL)
      {
	fprintf(stderr,"Cannot write output catalog \"%s\"\n",
		fnamcatout);
	exit(-1);
      }

    if (aprad<=0) 
      printf("Aperture is not set. Isophotal flux is used\n");

    for(q=psfob; q != NULL; q=q->next)
      {
	/* 2000/10/21 aperture added */
	if(aprad>0)
	  {
	    /* apphot */
	    q->fiso=aperturePhotometry(pix,npx,npy,q->xc,q->yc,aprad);
	  }
	fprintf(fp, "%d %.3f %.3f %.0f %d %.0f %d %d %d %d %d %d\n",
		q->entnum, q->xc, q->yc, q->fiso, 
		q->npix,q->peak, 
		q->xmin, q->xmax, q->ymin, q->ymax, 
		q->ipeak, q->jpeak);
      }
    fclose(fp);

    printf("\n");
    return 0;
}
