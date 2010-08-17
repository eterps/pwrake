#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "imc.h"
#include "stat.h"
#include "getargs.h"

#define XMAX 2200
#define YMAX 4200
#define OSMAX XMAX

int main(int argc,char *argv[])
{
  struct imh imhin={""},imhout={""};
  struct icom icomin={0},icomout={0};

  char fnamin[100]="";
  char fnamout[100]="";
  char tmp[100]="";

  FILE *fp,*fp2;
  float dp[XMAX];
  float data[OSMAX]; /* temporary */
  int ix,iy;
  int xmin=-1,xmax=-1,ymin=-1,ymax=-1;
  int offst;
  float bzero=0,bscale=1.0,bzero_new=FLT_MIN,bscale_new=FLT_MIN;
  float blank_new=FLT_MIN;
  float pixignr=INT_MIN;
  int fitsdtype=-1;

  int efpmin1,efprng1,efpmin2,efprng2;

  int j=0;
  char dtype[BUFSIZ]="";

  int fit_param=-1;
  float os[4096];
  int notrim=0;

  getargopt opts[20];
  char *files[3]={NULL};
  int n=0;
  int helpflag;

  files[0]=fnamin;
  files[1]=fnamout;

  setopts(&opts[n++],"-xmin=", OPTTYP_INT , &xmin,
	  "effective region (default:0)");
  setopts(&opts[n++],"-xmax=", OPTTYP_INT , &xmax,
	  "effective region (default:npx-1)");
  setopts(&opts[n++],"-ymin=", OPTTYP_INT , &ymin,
	  "effective region (default:0)");
  setopts(&opts[n++],"-ymax=", OPTTYP_INT , &ymax,
	  "effective region (default:npy-1)");
  setopts(&opts[n++],"-notrim", OPTTYP_FLAG , &notrim,
	  "not trim off(default:trim)",1);
  /* */

  setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero_new,
	  "bzero");
  setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale_new,
	  "bscale");
  setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
	  "datatyp(FITSFLOAT,FITSSHORT...)");
  setopts(&opts[n++],"-pixignr=", OPTTYP_FLOAT , &blank_new,
	  "pixignr value");
  /*
    setopts(&opts[n++],"-fit", OPTTYP_FLAG , &fit_param,
    "fitting (default:no)",1);
  */
  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamout[0]=='\0')
   {
      fprintf(stderr,"Error: No input file specified!!\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: osmed4 <options> [filein] [fileout]",
		 opts,
		 "");
      exit(-1);
    }

  /*
   * ... Open Input
   */

  (void)printf("\n Input = %s\n",fnamin );

  if( (fp = imropen ( fnamin, &imhin, &icomin )) == NULL ) 
    {
      print_help("Usage: %s <options> [filein] [fileout]",
		 opts,
		 "");
      exit(-1);
    }

  imget_fits_value(&icomin,"EFP-MIN1",tmp);
  efpmin1=atoi(tmp);
  imget_fits_value(&icomin,"EFP-MIN2",tmp);
  efpmin2=atoi(tmp);
  imget_fits_value(&icomin,"EFP-RNG1",tmp);
  efprng1=atoi(tmp);
  imget_fits_value(&icomin,"EFP-RNG2",tmp);
  efprng2=atoi(tmp);


#if 0 /* included in imc_WCS 2002/05/07 */
  /* 2000/09/26 */
  imget_fits_value(&icomin,"CRPIX1",tmp);
  crpix1=atof(tmp);
  imget_fits_value(&icomin,"CRPIX2",tmp);
  crpix2=atof(tmp);
#endif

  if(xmin<0&&xmax<0&&ymin<0&&ymax<0)
    {
      xmin=efpmin1-1;
      ymin=efpmin2-1;
      xmax=xmin+efprng1-1;
      ymax=ymin+efprng2-1;
    }

  /*
     printf("xmin %d xmax %d ymin %d ymax %d\n",
	  xmin,xmax,ymin,ymax);
	  */

  if (xmin<0) xmin=0;
  if (xmax>imhin.npx-1||xmax<xmin) xmax=imhin.npx-1;
  if (ymin<0) ymin=0;
  if (ymax>imhin.npy-1||ymax<ymin) ymax=imhin.npy-1;

  printf("(%d,%d) - (%d,%d)\n",
	 xmin,ymin,xmax,ymax);

  /* output */
  
  imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);
  if(!notrim)
    {
      imhout.npx   = (xmax - xmin + 1);
      imhout.npy   = (ymax - ymin + 1);
      imhout.ndatx = imhout.npx;
      imhout.ndaty = imhout.npy;
    }
  

  /* !! need 2000/07/02 or later version of IMC */

  if(dtype[0]=='\0')
    imc_fits_get_dtype( &imhout, &fitsdtype, &bzero, &bscale, &offst );
  else
    if ((fitsdtype=imc_fits_dtype_string(dtype))==-1)
      {
	printf("\nWarning: Unknown fits dtype %s. Inheriting original.\n",dtype);
	imc_fits_get_dtype( &imhout, &fitsdtype, &bzero, &bscale, &offst );
      }

  if(bzero_new!=FLT_MIN)
    {
      bzero=bzero_new;
    }
  if(bscale_new!=FLT_MIN)
    {
      bscale=bscale_new;
    }

  
  (void)imc_fits_set_dtype( &imhout, fitsdtype, bzero, bscale );

  if(blank_new!=FLT_MIN)
    imset_pixignr(&imhout,&icomout,(int)blank_new);    
  else
    {
      pixignr=(float)imget_pixignr(&imhin);
      imset_pixignr(&imhout,&icomout,(int)pixignr);
    }

  if (fit_param>0)
    {
      imaddhist(&icomout,"OSMED4: Overscan value is smoothed");
    }
  else
    {
      imaddhist(&icomout,"OSMED4: Overscan median is subtracted line by line.");
    }

  if (!notrim)
    {
      imaddhistf(&icomout,"OSMED4: And extracted [%d:%d,%d:%d] (0-origin)",
		 xmin,xmax,ymin,ymax);
    }

  (void)printf("\n Output = %s\n",fnamout );

  /* 2000/09/26 EFP-MIN revise */
  /* 2001/09/17 debug */
  if(efpmin1-xmin<1)
    {
      efprng1-=(xmin+1-efpmin1);
      efpmin1=xmin+1;
    }
  if(efpmin1-1+efprng1-1>=xmax)
    {
      efprng1=xmax-(efpmin1-1)+1;
    }
  if(efpmin2-ymin<1)
    {
      efprng2-=(ymin+1-efpmin2);
      efpmin2=ymin+1;
    }
  if(efpmin2-2+efprng2-1>=ymax)
    {
      efprng2=ymax-(efpmin2-1)+1;
    }

  if(!notrim)
    {


      imupdate_fitsf(&icomout, "EFP-MIN1",IMC_FITS_INTFORM,
		    efpmin1-xmin,
		    "Start position of effective frame in axis-1");

      imupdate_fitsf(&icomout, "EFP-MIN2",IMC_FITS_INTFORM,
		    efpmin2-ymin,
		    "Start position of effective frame in axis-2");

      imupdate_fitsf(&icomout, "EFP-RNG1",IMC_FITS_INTFORM,
		    xmax-xmin+1,
		    "Range of effective frame in axis-1");

      imupdate_fitsf(&icomout, "EFP-RNG2",IMC_FITS_INTFORM,
		    ymax-ymin+1,
		    "Range of effective frame in axis-2");

#if 0      
      /* 2000/09/26 CRPIX revise */
      imupdate_fitsf(&icomout, "CRPIX1",
		    "%20.1f / %-47.47s",
		    crpix1-(float)xmin,
		    "Reference pixel coordinate system in axis1");


      imupdate_fitsf(&icomout, "CRPIX2",
		    "%20.1f / %-47.47s",
		    crpix2-(float)ymin,
		    "Reference pixel coordinate system in axis2");
#else
      imc_shift_WCS(&icomout,(float)xmin,(float)ymin);
#endif
    }

  for( iy=0; iy<imhin.npy; iy++ ) 
    {
      j=0;
      (void) imrl_tor( &imhin, fp, dp, iy );
      for( ix=0; ix<xmin; ix++ ) 
	{
	  data[j++]=dp[ix];
	}
      for(ix=xmax+1;ix<imhin.npx;ix++)
	{
	  data[j++]=dp[ix];
	}
      os[iy]=floatmedian(j,data);
    }

  /* fit, if needed, not implemented */

  /**** OPEN output ****/

  if( (fp2 = imwopen( fnamout, &imhout, &icomout )) == NULL )
    {
      (void) imclose(fp,&imhin,&icomin);
      print_help("Usage: %s <options> [filein] [fileout]",
		 opts,
		 "");
      exit(-1);
    }
  if (notrim)
    {
      xmin=0;
      xmax=imhin.npx-1;
    }

  for( iy=0; iy<imhin.npy; iy++ ) 
    {
      j=0;
      (void) imrl_tor( &imhin, fp, dp, iy );
      for(ix=xmin;ix<=xmax;ix++)
	dp[ix]-=os[iy];
      (void) imwl_rto( &imhout, fp2, dp+xmin, iy);
    }

  (void) imclose(fp,&imhin,&icomin);  
  (void) imclose(fp2,&imhout,&icomout);
  return 0;
}

