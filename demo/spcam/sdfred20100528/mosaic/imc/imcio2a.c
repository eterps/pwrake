#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>

/* ... local include files ... */
#include "imc.h"
#include "getargs.h"

/* ------------------------------------------------------------------ */
/* --------------------------------------------------------------------
 *
 *    General Purpose Read/Write Program
 *
 * --------------------------------------------------------------------
 */


int main(int argc,char *argv[])
{
#define NOT_REPLACE -999999.0
  struct  imh	imhin={""}, imhout={""};
  struct  icom	icomin={0}, icomout={0};
  char	fnamin[BUFSIZ]="", fnamout[BUFSIZ]="";
  char temp[BUFSIZ];
  FILE	*fp=NULL, *fp2=NULL;

  float *dp, *dpout;
  float *dpybin,*dpxbin;
  int *ndp;
  int	ix,iy;

  int pixignr=INT_MIN;
  float   rplc_ignor=NOT_REPLACE;
  int maxadd=-1;
  float bzero=0.,bscale=1.;

/*  int pixoff; */
  int  u2off;
  int iouty, ioutx, i, NBINX=1, NBINY=1, NBIN=1;
  int  ymin=0, ymax=INT_MAX;
  int  xmin=0, xmax=INT_MAX;
  char dtype[80]="\0";
  int fitsdtype;
  float fitsmax;

  char modename[20]="";

  int pixoff;

  int n0;
  int nline=-1;

  int xshift=0,yshift=0;
  float vxref=FLT_MAX,vyref=FLT_MAX;

  MOSMODE mosmode=NODIST_NOADD;
  /*
    int dist_clip=0,dist_flux=0,dist_peak=0,dist_add=0,
    dist_med=0, nodist_noadd=0, const_add=0;
  */
  int addorghead=0;
  float vxoff,vyoff;
  int nfrm;

  float sigmaf=0.16,nsigma=3.0;
  int meanmin=0,clipmin=3;

  getargopt opts[30];
  char *files[3]={NULL};
  int n=0;
  int helpflag;



  /*
   * ... Open Input
   */

  files[0]=fnamin;
  files[1]=fnamout;


  setopts(&opts[n++],"-maxadd=", OPTTYP_INT , &maxadd,"(mos)max number of coadd");
  setopts(&opts[n++],"-nline=", OPTTYP_INT , &nline,"(mos)y-width of buffering");

  setopts(&opts[n++],"-nodist_noadd", OPTTYP_FLAG , &mosmode,"(mos)quick mode (default)",NODIST_NOADD);

  setopts(&opts[n++],"-dist_add", OPTTYP_FLAG , &mosmode,"(mos)bilinear weighted mean",DIST_ADD);
  setopts(&opts[n++],"-dist_med", OPTTYP_FLAG , &mosmode,"(mos)bilinear weighted median",DIST_MED);
  setopts(&opts[n++],"-const_add", OPTTYP_FLAG , &mosmode,"(mos)mean of at least maxadd images or blank",CONST_ADD);
  setopts(&opts[n++],"-dist_clip", OPTTYP_FLAG , &mosmode,"(mos)clip mean mode(tesing)",DIST_CLIP_ADD);
  setopts(&opts[n++],"-dist_flux", OPTTYP_FLAG , &mosmode,"(mos)flux map mode(testing)",DIST_FLUX);
  setopts(&opts[n++],"-dist_peak", OPTTYP_FLAG , &mosmode,"(mos)outlyer mode(testing)",DIST_PEAK);


  setopts(&opts[n++],"-vxref=", OPTTYP_FLOAT , &vxref,"(mos) reference pixel x(default: not used (automatic))");
  setopts(&opts[n++],"-vyref=", OPTTYP_FLOAT , &vyref,"(mos) reference pixel y(default: not used(automacic))");
  setopts(&opts[n++],"-addorghead", OPTTYP_FLAG , &addorghead,"(mos) Add original header of reference (default:no)",1);

  setopts(&opts[n++],"-sigmaf=", OPTTYP_FLOAT , &sigmaf,"(mos/clip)sigma estimate fraction (default:0.160)");
  setopts(&opts[n++],"-nsigma=", OPTTYP_FLOAT , &nsigma,"(mos/clip)clipping sigma factor(default:3.0)");
  setopts(&opts[n++],"-clipmin=", OPTTYP_INT , &clipmin,"(mos/clip)minimum # of images for clipping (default:3)");
  setopts(&opts[n++],"-meanmin=", OPTTYP_INT , &meanmin,"(mos/clip)minimum # of images for taking clipped mean(default:not set)");


  setopts(&opts[n++],"-ymin=", OPTTYP_INT , &ymin,"ymin (default:0)");
  setopts(&opts[n++],"-ymax=", OPTTYP_INT , &ymax,"ymax (default:npy-1)");
  setopts(&opts[n++],"-xmin=", OPTTYP_INT , &xmin,"xmin (default:0)");
  setopts(&opts[n++],"-xmax=", OPTTYP_INT , &xmax,"xmax (default:npx-1");

  setopts(&opts[n++],"-bin=", OPTTYP_INT , &NBIN,"binning size(default:1)");
  setopts(&opts[n++],"-binx=", OPTTYP_INT , &NBINX,"binning size(default:1)");
  setopts(&opts[n++],"-biny=", OPTTYP_INT , &NBINY,"binning size(default:1)");

  setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero,
	  "bzero");
  setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale,
	  "bscale");
  setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
	  "datatyp(FITSFLOAT,FITSSHORT...)");
  setopts(&opts[n++],"-pixignr=", OPTTYP_FLOAT , &rplc_ignor,
	  "pixignr value");
  setopts(&opts[n++],"-ignor=", OPTTYP_FLOAT , &rplc_ignor,
	  "pixignr value");

  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamout[0]=='\0')
   {
      fprintf(stderr,"Error: No input file specified!!\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: imcio2 <options> [filein] [fileout]",
		 opts,
		 "");
      exit(-1);
    }

  switch(mosmode)
    {
    case DIST_CLIP_ADD:
      sprintf(modename,"dist_clip");
      imc_mos_set_clip_param(nsigma,sigmaf,clipmin,meanmin);
      break;
    case CONST_ADD:
      if (maxadd<=0) maxadd=1;
      sprintf(modename,"const_add %d",maxadd);
      break;
    case DIST_FLUX:
      sprintf(modename,"dist_flux");
      break;
    case DIST_PEAK:
      sprintf(modename,"dist_peak");
      break;
    case DIST_ADD:
      sprintf(modename,"dist_add");
      break;
    case DIST_MED:
      sprintf(modename,"dist_med");
      break;
    case NODIST_NOADD:
      sprintf(modename,"dist_noadd");
      break;
    default:
      mosmode=NODIST_NOADD;
      sprintf(modename,"nodist_noadd");
      break;
    }
  imc_mos_set_default_add( mosmode );
  if (maxadd>0)
    {
      imc_mos_set_default_maxadd( maxadd );
      printf(" Max # of frames is %d\n", maxadd);
    }

  if(nline<0)
    nline=200;
  
  imc_mos_set_nline(nline);
  imc_mos_set_nline_l(nline);
  
  if( fnamin[0]=='\0' ) 
    {
      (void) printf("\n input IRAF/FITS/MOS file name = " );
      (void) fgets( temp,BUFSIZ,stdin );
      sscanf(temp,"%s",fnamin);
    }
  else
    (void)printf("\n Input = %s\n",fnamin );
  

  if( (fp = imropen ( fnamin, &imhin, &icomin )) == NULL ) 
    {
      exit(-1);
    }

  /* testing */
  if (imhin.dtype==DTYPEMOS)
    {
      if (vxref!=FLT_MAX && vyref!=FLT_MAX)
	{
	  printf("vxref:%f vyref:%f\n",vxref,vyref);
	  imc_mos_set_shift(&imhin, -vxref, -vyref);
	}
    }

  u2off=imget_u2off( &imhin );
  (void) printf("Offset = %d\n",u2off);
  pixignr=imget_pixignr( &imhin );
  (void) printf("Pixignr= %d\n",pixignr);

  if(xmin<0) xshift=-xmin;
  if(ymin<0) yshift=-ymin;

  if( ymax == INT_MAX ) 
    {
      /* ymax is not given */
      ymax = imhin.npy;
    }
  else
    ymax++;

  if( xmax == INT_MAX ) 
    {
      /* ymax is not given */
      xmax = imhin.npx;
    }
  else
    xmax++;

  printf("Output Y range: %d to %d in original frame\n",ymin,(ymax-1));
  printf("Output X range: %d to %d in original frame\n",xmin,(xmax-1));

  if( ymin > (ymax-1) ) 
    {
      printf("Error: Ymin > Ymax\n");
      imclose( fp,&imhin,&icomin );
      exit(-1);
    }
  if( xmin > (xmax-1) ) 
    {
      printf("Error: Xmin > Xmax\n");
      imclose( fp,&imhin,&icomin );
      exit(-1);
    }

  /* 2003/12/23 */
  /* check */
  if (imhin.dtype==DTYPEMOS)
    {
      if (0!=imc_mos_check_fill_buffer(&imhin,xmin,xmax,
				       ymin,ymax,nline))
	{
	  printf("Error: nline is not appropriate\n");
	  exit(-1);
	}
    }
  /*
   * ... Open output
   */
  
  if(fnamout[0]=='\0' ) 
    {
      (void) printf(" output IRAF/FITS file name = " );
      (void) fgets( temp,BUFSIZ,stdin );
      sscanf(temp,"%s",fnamout);
    }
  else
    (void)printf(  " Output= %s\n\n", fnamout );
  
  imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);

  if (NBIN!=1) 
    {
      NBINX=NBIN;
      NBINY=NBIN;
    }
  NBIN=NBINX*NBINY;

  imhout.npx   = (xmax - xmin)/ NBINX;
  imhout.npy   = (ymax - ymin)/ NBINY;
  imhout.ndatx = imhout.npx;
  imhout.ndaty = imhout.npy;

  if (imhout.dtype==DTYPFITS)
    {
      if(dtype[0]!='\0')
	{
	  if (strstr(dtype,"SHORT"))
	    {
	      fitsdtype=FITSSHORT;
	      fitsmax=(float)SHRT_MAX-1.; /* defined in <limits.h> */
	    }
	  else if (strstr(dtype,"INT"))
	    {
	      fitsdtype=FITSINT;
	      fitsmax=(float)INT_MAX-1.; /* defined in <limits.h> */
	    }
	  else if (strstr(dtype,"CHAR"))
	    {
	      fitsdtype=FITSCHAR;
	      fitsmax=(float)CHAR_MAX-1.; /* defined in <limits.h> */
	    }
	  else if (strstr(dtype,"FLOAT"))
	    {
	      fitsdtype=FITSFLOAT;
	    }
	  else
	    {
	      fitsdtype=FITSSHORT;
	      fitsmax=SHRT_MAX-2.; /* defined in <limits.h> */
	    }
	}
      else
	{
	  (void)imc_fits_get_dtype( &imhout, &fitsdtype, NULL, NULL, NULL);
	}
      (void)imc_fits_set_dtype( &imhout, fitsdtype, bzero, bscale );
    }
  else
    {	  
      if (strstr(dtype,"I2"))
	{
	  imhout.dtype=DTYPI2;
	}
      else if (strstr(dtype,"INT"))
	{
	  imhout.dtype=DTYPINT;
	}
      else if (strstr(dtype,"I4"))
	{
	  imhout.dtype=DTYPI4;
	}
      else if (strstr(dtype,"R4"))
	{
	  imhout.dtype=DTYPR4;
	}
      else if (strstr(dtype,"R8"))
	{
	  imhout.dtype=DTYPR8;
	}
      else
	{
	  imhout.dtype=DTYPU2;
	}
      imset_u2off(&imhout,&icomout,u2off);  
    }


  /*  Make comments */

  imaddhistf(&icomout,"made by imcio2 from %-48.48s",fnamin);
  
  if(imhout.npx != imhin.npx ||  imhout.npy != imhin.npy || NBIN>1)
    {
      imaddhistf(&icomout,
		 "  x[%d:%d] y[%d:%d] %dx%d bining",
		 xmin,xmax,ymin,ymax,NBINX,NBINY);
    }   

  (void)imc_fits_get_dtype( &imhout, NULL, NULL, NULL, &pixoff);

  if(rplc_ignor!=NOT_REPLACE)
    {
      imset_pixignr(&imhout, &icomout, (int)rplc_ignor);
    }
  else
    {
      imset_pixignr(&imhout, &icomout, (int)pixignr);
      rplc_ignor=pixignr;
    }

  /* 2002/05/07 WCS change is added */
  imc_shift_WCS(&icomout, (float)xmin, (float)ymin);
  if (NBIN>1)
    {
      /*
	imc_scale_WCS(&icomout, 1.0/((float)NBIN));
      */
      imc_scale_WCS2(&icomout, 1.0/((float)NBINX),1.0/((float)NBINY));
    }


  /** 2004/03/03 *.mos */
  if (imhin.dtype==DTYPEMOS)
    {
      imc_mos_get_params(&imhin,&vxoff,&vyoff,
			 NULL,NULL,NULL,NULL,
			 NULL,&nfrm,NULL);
      imaddhistf(&icomout,"shift by %f %f from original mos",
		 vxoff,vyoff);
      imaddhistf(&icomout,"%d frames are used, %s",
		 nfrm, modename);

      if (addorghead==1)
	imc_mos_merge_org_header(&icomout,&imhin);
    }


  if( (fp2 = imwopen( fnamout, &imhout, &icomout )) == NULL ) 
    {
      imclose( fp,&imhin,&icomin );
      exit(-1);
    }

/*
 * ... Copy line to line
 */
  dp=(float*)malloc(sizeof(float)*imhin.npx);
  if(dp==NULL)
    {
      printf("Image too large. Cannot allocale memory for dp.\n"); 
      imclose( fp,&imhin,&icomin );
      imclose( fp2,&imhout,&icomout );
      exit(-1);
    }



/* Bin */
    if( NBIN > 1 ) 
      {
	/* 1999/02/19 changed */
	printf("\nBinning by %d x %d ...\n", NBINX, NBINY );	

	dpybin=(float*)malloc(sizeof(float)*imhin.npx);  
	dpxbin=(float*)malloc(sizeof(float)*imhout.npx);
	ndp=(int*)malloc(sizeof(int)*imhin.npx);

	if(dpybin==NULL||dpxbin==NULL||ndp==NULL)
	  {
	    printf("Image too large. Cannot allocale memory for ndp.\n"); 
	    imclose( fp,&imhin,&icomin );
	    imclose( fp2,&imhout,&icomout );
	    exit(-1);
	  }

	for( iy=ymin, iouty=0; iouty<imhout.npy; iouty++ ) 
	  {
	    
	    /* clear y buffer */
	    memset(ndp,0,imhin.npx*sizeof(int));	    
	    memset(dpybin,0,imhin.npx*sizeof(float));	    
	    
	    /* bin in y */
	    for(i=0; i<NBINY && iy<ymax; i++ ) 
	      {		
		/* Read a line */
		if(iy>=0)
		  {
		    (void) imrl_tor( &imhin, fp, dp, iy );
		    for( ix=xmin; ix<xmax; ix++) 
		      {
			if((ix>=0) && (dp[ix]!=(float)pixignr))
			  {
			    dpybin[ix+xshift] += dp[ix];
			    ndp[ix+xshift]++;
			  }
		      }
		  }
		iy++;
	      }

	    /* clear x buffer */
	    memset(dpxbin,0,imhout.npx*sizeof(float));

	    /* bin in x */
	    for(ix=xmin, ioutx=0 ; ioutx<imhout.npx; ioutx++ ) 
	      {
		n0=0;
		for(i=0; i<NBINX && ix<xmax; i++) 
		  {
		    dpxbin[ioutx] += dpybin[ix+xshift];
		    n0+=ndp[ix+xshift];
		    ix++;
		  }
/*
		printf("%d %d %f %d\n",ioutx,iouty,dpxbin[ioutx],n0);
*/
		if(n0!=0)
		  {
		    dpxbin[ioutx]/=(float)n0;
		  }
		else
		  {
		    if(rplc_ignor != NOT_REPLACE)
		      dpxbin[ioutx]=rplc_ignor;
		    else
		      dpxbin[ioutx]=(float)pixignr;
		  }

	      }
	    
	    /* output a line */
	    (void) imwl_rto( &imhout, fp2, dpxbin, iouty );
	  }

	free(dpxbin);
	free(dpybin);
	free(ndp);

      }	
    else
      {
	/* No binning */
	dpout=(float*)malloc(sizeof(float)*imhout.npx);

	if(dpout==NULL)
	  {
	    printf("Image too large. Cannot allocale memory for ndp.\n"); 
	    imclose( fp,&imhin,&icomin );
	    imclose( fp2,&imhout,&icomout );
	    exit(-1);
	  }

	/* Sep 28 temp; -> Feb5 */
	{
	  /**/
	  for( iy=ymin; iy<ymax; iy++ ) 
	    {
	      if(iy>=0 && iy<imhin.npy) 
		{
		  imrl_tor( &imhin, fp, dp, iy );
		  for( ix=xmin; ix<xmax; ix++ ) 
		    {
		      if(ix<0||ix>=imhin.npx|| dp[ix] == (float)pixignr)
			dpout[ix-xmin]=rplc_ignor;
		      else
			dpout[ix-xmin]=dp[ix];

		      /*
		      printf("%d %d\n",ix,ix-xmin);
		      */
		    }
		}	
	      else
		for( ix=xmin; ix<xmax; ix++ ) 
		  dpout[ix-xmin]=rplc_ignor;

	      (void) imwl_rto( &imhout, fp2, dpout, (iy-ymin));
	    }
	}
	free(dpout);
      }
    free(dp);
    
    printf(" Image Size is %d x %d\n\n", imhout.npx, imhout.npy );
    
    if (fp!=NULL) {imclose( fp,&imhin,&icomin );}
    if (fp2!=NULL) {imclose( fp2,&imhout,&icomout );}
    
    return 0;
}
  
