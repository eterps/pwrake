#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <string.h>

#include "imc.h"
#include "getargs.h"

int main(int argc,char *argv[])
{
  struct  imh	imhin={""},imhout={""};
  struct  icom	icomin={0},icomout={0};
  char fnamin[BUFSIZ]="";
  char fnamout[BUFSIZ]="";
  FILE	*fpin,*fpout;
  float *dp;
  int npx,npy;
  int iy,ix;
  int pixignr=INT_MIN;
  int pixignr_org=INT_MIN;

  float imax=INT_MAX,imin=INT_MIN;

  float bzero=0,bscale=1.0;
  int bzeroset=0;

  int fitsdtype;
  char dtype[80]="";


  int quietmode=1;

  /* Ver3. */
  getargopt opts[20];
  char *files[3]={NULL};
  int n=0;
  int helpflag;

  files[0]=fnamin;
  files[1]=fnamout;

  setopts(&opts[n++],"-imax=", OPTTYP_FLOAT , &imax,
	  "imax");
  setopts(&opts[n++],"-imin=", OPTTYP_FLOAT , &imin,
	  "imin");

  setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero,
	  "bzero");
  setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale,
	  "bscale");
  setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
	  "datatyp(FITSFLOAT,FITSSHORT...)");
  setopts(&opts[n++],"-pixignr=", OPTTYP_INT , &pixignr,
	  "pixignr value");
  setopts(&opts[n++],"-q",OPTTYP_FLAG,&quietmode,"quiet mode(default)",1);
  setopts(&opts[n++],"-debug",OPTTYP_FLAG,&quietmode,"debugx mode",0);

  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamout[0]=='\0')
   {
      fprintf(stderr,"Error: No output file specified!!\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: uppercut [option] infilename outfilename",
		 opts,"");
      exit(-1);
    }
  
  /*
   * ... Open Input
   */
  fpin = imropen ( fnamin, &imhin, &icomin );
  if (fpin==NULL)
    {
      fprintf(stderr,"Error: Cannot open input file \"%s\"\n",fnamin);
      exit(-1);
    }

  if (strcmp(fnamin,fnamout)==0)
    {
      fprintf(stderr, "Error: Input and Output is the same file!\n");
      imclose(fpin,&imhin,&icomin);
      exit(-1);
    }
  
  if(!quietmode)
    printf("-imax=%f,-imin=%f\n",imax,imin);

  pixignr_org=imget_pixignr( &imhin );
  if (pixignr==(float)INT_MIN)
    {
      pixignr=pixignr_org;
    } 

  imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);

  /* 2000/07/02 */
  if(bzeroset!=1)
    (void)imc_fits_get_dtype( &imhout, NULL, &bzero, &bscale, NULL);
  
  if(!quietmode)
    printf("Output bzero:%f bscale:%f\n",bzero,bscale);

  if(dtype[0]=='\0'||(fitsdtype=imc_fits_dtype_string(dtype))==-1)
    (void)imc_fits_get_dtype( &imhout, &fitsdtype, NULL, NULL, NULL);
    
  /* re-set bzero & bscale */
  if(imc_fits_set_dtype(&imhout,fitsdtype,bzero,bscale)==0)
    {
      fprintf(stderr,"Error\nCannot set FITS %s\n",fnamout);
      fprintf(stderr,"Type %d BZERO %f BSCALE %f\n",fitsdtype,bzero,bscale);
      exit(-1);
    }

  imset_pixignr( &imhout, &icomout, pixignr); 

  if ((fpout = imwopen( fnamout, &imhout, &icomout )) == NULL)
    {
      fprintf(stderr,"Cannot open the output file \"%s\"\n",fnamout);
      imclose(fpin,&imhin,&icomin);
      exit(-1);
    }
   
  npx=imhin.npx;
  npy=imhin.npy;

  dp=(float*)malloc(npx*sizeof(float));
  for( iy=0; iy<npy; iy++ ) 
    {
      (void) imrl_tor( &imhin, fpin, dp, iy );
      for( ix=0; ix<npx; ix++ ) 
	{
	  if(dp[ix]>imax||dp[ix]<imin||dp[ix]==(float)pixignr_org)
	    {
	      dp[ix]=(float)pixignr;
	    }
	}
      (void) imwl_rto( &imhout, fpout, dp, iy );      
    }



  imclose(fpin,&imhin,&icomin);
  imclose(fpout,&imhout,&icomout);
  return 0;
}
