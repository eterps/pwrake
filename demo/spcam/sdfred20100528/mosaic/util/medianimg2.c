/*--------------------------------------------------------------
 *
 *     median.c
 *
 *--------------------------------------------------------------
 *
 * Description:
 * ===========
 *   take median of images
 *
 * Revision history:
 * ================
 *   created  May  26  1999  Nakata
 *   revised  Jun  27  1999  Nakata
 *      read counts by each column
 *   revised  Jun  29  1999  Nakata
 *      add rejection sigma
 *
 *   1999/11/10 Yagi arranged
 *  pixignr
 *   2002/11/10 Yagi arranged
 *--------------------------------------------------------------
 */

/* used for filename */
#define   LINMAX    1024

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#include "imc.h"
#include "stat.h"
#include "getargs.h"

/*--------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
  FILE   *fp_input, **fp_in, *fp_out;

  int j;			/* do loop for (px,py) */
  int ncol;				/* image size (X) (pix) */
  int nrow;				/* image size (Y) (pix) */
  int ncol2, nrow2, n1, k, m, p;
  int n_input, n_final;		/* number of input image */
  int ndat;
  char   line[LINMAX], fnamtab[LINMAX]="", fnamout[LINMAX]="";
  char **fnamin;
  float *factor;

  float bzero=0,bscale=-1.0;
  int bzeroset=0;
  int fitsdtype;
  char dtype[80]="";

  float  **pixel,*dat;
  float  *pixignr;
  float temp;
  float  ave,sdev;
  float  sigma, sigma_r=-1;
  int cycle=0;
  int pixignr_out=INT_MIN;

  struct imh  *imhin, imhout={""};
  struct icom *icomin,icomout={0};
  int nopenmin=0, nopenmax=0, nopennum=0;
   
  float  *data;
  int reorderflag=0;
  
  int nmax=1024;
  int nmaxstep=1024;
  /*---- get argv[] ------------------------------------------*/
  int weighted=1;

  int quietmode=1;
  char comment[BUFSIZ];

  /* Ver3. */
  getargopt opts[20];
  char *files[3]={NULL};
  int n=0;
  int helpflag;

  files[0]=fnamtab;
  files[1]=fnamout;

  setopts(&opts[n++],"-nsigm=", OPTTYP_FLOAT , &sigma_r,
	  "n-sigma rejection (default:not used)");
  setopts(&opts[n++],"-niter=", OPTTYP_INT , &cycle,
	  "rejection iteration cycle (default:0)");

  setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero,
	  "bzero");
  setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale,
	  "bscale");
  setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
	  "datatyp(FITSFLOAT,FITSSHORT...)");
  setopts(&opts[n++],"-pixignr=", OPTTYP_INT , &pixignr_out,
	  "pixignr value for output");
  setopts(&opts[n++],"-q",OPTTYP_FLAG,&quietmode,"quiet mode(default)",1);
  setopts(&opts[n++],"-debug",OPTTYP_FLAG,&quietmode,"debug mode",0);

  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamout[0]=='\0')
   {
      fprintf(stderr,"Error: No output file specified!!\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: medianimg2 [option] (infilename(tab)) (outfilename)",
		 opts,"");
      exit(-1);
    }


  /*---- read input file --------------------------------------*/ 

  if((fp_input = fopen( fnamtab, "r")) == NULL) 
    {
      printf("Cannot open input file %s\n",fnamtab);
      exit(1);
    }
   
  fnamin=(char**)malloc(nmax*sizeof(char*));
  factor=(float*)malloc(nmax*sizeof(float));
  if(fnamin==NULL||factor==NULL)
    {
      fprintf(stderr,"Error: Cannot allocate memory, too small memory.\n");
      exit(-1);
    }

  n = 0;
  fnamin[n]=(char*)malloc(LINMAX*sizeof(char));

  while( fgets( line, LINMAX, fp_input ) != NULL && n<nmax) 
    {
      if (sscanf( line, "%s %f", fnamin[n],&factor[n])!=2)
	{
	  sscanf( line, "%s", fnamin[n]);
	  factor[n]=1.0;
	  weighted=0;
	}

      if(!quietmode)
	printf("%s %f\n", fnamin[n],factor[n]);

      if(factor[n]!=0) 
	{
	  n++;
	  if(n>=nmax)
	    {
	      nmax+=nmaxstep;
	      fnamin=(char**)realloc(fnamin,nmax*sizeof(char*));
	      factor=(float*)realloc(factor,nmax*sizeof(float));
	      if(fnamin==NULL||factor==NULL)
		{
		  fprintf(stderr,"Error: Cannot allocate memory, too many files.\n");
		  exit(-1);
		}

	    }
	  fnamin[n]=(char*)malloc(LINMAX*sizeof(char));
	}
    }
  n_input = n;
  fclose(fp_input);
  if(n_input<=0) 
    {
      fprintf(stderr,"Warning: No valid file names in %s\n",fnamtab);
      exit(0);
    }

  /*---- get ncol and nrow -------------------------------------*/ 

  fp_in=(FILE**)malloc(sizeof(FILE*)*n_input);
  imhin=(struct imh*)calloc(n_input,sizeof(struct imh));
  icomin=(struct icom*)calloc(n_input,sizeof(struct icom));

  if(fp_in==NULL||imhin==NULL||icomin==NULL)
    {
      fprintf(stderr,"Error: Cannot allocate memory, too many files.\n");
      exit(-1);
    }

  if( (fp_in[0] = imropen( fnamin[0], &imhin[0], &icomin[0])) == NULL ) 
    {
      (void) fprintf(stderr, " ERROR: Can't open %s.\n", fnamin[0] );
      (void) exit(-1);
    }
  ncol = imhin[0].npx;
  nrow = imhin[0].npy;

  /*---- malloc -----------------------------------------------*/ 

  pixel=(float**)malloc(n_input*sizeof(float*));
  dat=(float*)malloc(n_input*sizeof(float));
  pixignr=(float*)malloc(n_input*sizeof(float));
 
  data   = (float *)malloc( ncol*(n_input+1)*sizeof(float) );
  if (data==NULL)
    {
      fprintf(stderr,"Error: Not enough memory for buffer, NAXIS1 is too large.\n");
      exit(-1);
    }
  /*---- read image -------------------------------------------*/

  /* first open all */

  nopenmin=0; 
  nopenmax=0; 
  nopennum=0; 

  for( n=0; n<n_input; n++) 
    {
      if( (fp_in[n] = imropen( fnamin[n], &imhin[n], &icomin[n] )) 
	  == NULL 
	  /*debug */ 
	  || n%5==0 )
	{
	  /* 2000/09/26 */
	  /* close-all and retry */
	  nopennum=(n-nopenmin)-1;
	  for(n1=nopenmin;n1<n;n1++)
	    {
	      fclose(fp_in[n1]);
	      fp_in[n1]=NULL;
	    }
	  if( (fp_in[n] = imropen( fnamin[n], &imhin[n], &icomin[n] )) 
	     == NULL ) 
	    {
	      (void) fprintf(stderr," ERROR: Can't open %s.\n", fnamin[n] );
	      (void) exit(-1);
	    }
	  nopenmin=n;
	}
      ncol2 = imhin[n].npx;
      nrow2 = imhin[n].npy;
      if ( (ncol != ncol2) || (nrow != nrow2) ) 
	{
	  fprintf(stderr, "Error: image size is not consistent with each other\n");
	  fprintf(stderr, "     %s: %d x %d\n",fnamin[0],ncol,nrow);
	  fprintf(stderr, "     %s: %d x %d\n",fnamin[n],ncol2,nrow2);
	  exit(-1);
	}
      /* 2000/09/26 */

      pixel[n]=data+ncol*(n+1);
      pixignr[n]=imget_pixignr(&imhin[n]);
    }


  /* 2000/09/26 */
  if(nopennum!=0)   
    {
      nopenmax=n_input-1;
      reorderflag=1; /* reversal */
    }

  imh_inherit(&imhin[0],&icomin[0],&imhout,&icomout,fnamout);

  imhout.npx = ncol;
  imhout.npy = nrow;
  imhout.dtype = DTYPFITS;

  /* 2000/07/03 */
  if(bzeroset!=1)
    (void)imc_fits_get_dtype( &imhout, NULL, &bzero, &bscale, NULL);
  if(!quietmode)
    printf("Output bzero:%f bscale:%f\n",bzero,bscale);
  if(dtype[0]=='\0'||(fitsdtype=imc_fits_dtype_string(dtype))==-1)
    (void)imc_fits_get_dtype( &imhout, &fitsdtype, NULL, NULL, NULL);
    
  /* re-set bzero & bscale */
  if(imc_fits_set_dtype(&imhout,fitsdtype,bzero,bscale)==0)
    {
      fprintf(stderr,"Error: Cannot set FITS %s\n",fnamout);
      fprintf(stderr,"Type %d BZERO %f BSCALE %f\n",fitsdtype,bzero,bscale);
      exit(-1);
    }

  if(pixignr_out==INT_MIN)
    pixignr_out=pixignr[0];

  imset_pixignr( &imhout, &icomout, pixignr_out);

  /*  Make comments */
  sprintf(comment,"made by medianimg2 from %s",fnamtab);
  imaddhist(&icomout,comment);

  if( (fp_out = imwopen_fits( fnamout,
			     &imhout, &icomout )) == NULL ) 
    {
      (void) fprintf(stderr, " ERROR: Can't open %s.\n", fnamout );
      (void) exit(-1);
    }


  for( j=0; j<nrow; j++ ) 
    {
      if(!quietmode)
	printf("Now working on nrow = %d\n", j);

      if(nopennum==0)
	for( n=0; n<n_input; n++) 
	  {
	    /*printf("Now working on %s\n", fnamin[n]); */
	    imrl_tor( &imhin[n], fp_in[n], pixel[n], j );
	  }
      else
	{
	  if (reorderflag==1)
	    {
	      for( n=n_input-1; n>=0; n--) 
		{
		  if(n<nopenmin)
		    {
		      for(n1=nopenmin;n1<=nopenmax;n1++)
			fclose(fp_in[n1]);
		      nopenmax=nopenmin-1;
		      nopenmin=nopenmax-nopennum+1;
		      if(nopenmin<0) nopenmin=0;
/*
		      printf("open %d - %d\n",nopenmin,nopenmax);
*/
		      for(n1=nopenmin;n1<=nopenmax;n1++)
			fp_in[n1]=fopen(fnamin[n1],"r");
		    }
/*
		  printf("%d %s\n",n,fnamin[n]);
*/
		  imrl_tor( &imhin[n], fp_in[n], pixel[n], j );
		}
	      reorderflag=0;
	    }
	  else
	    {
	      for( n=0; n<n_input; n++) 
		{
		  if(n>nopenmax)
		    {
		      for(n1=nopenmin;n1<=nopenmax;n1++)
			fclose(fp_in[n1]);
		      nopenmin=nopenmax+1;
		      nopenmax=nopenmin+nopennum-1;
		      if(nopenmax>=n_input) nopenmax=n_input-1;
/*
		      printf("open %d - %d\n",nopenmin,nopenmax);
*/
		      for(n1=nopenmin;n1<=nopenmax;n1++)
			fp_in[n1]=fopen(fnamin[n1],"r");
		    }
/*
		  printf("%d %s\n",n,fnamin[n]);
*/
		  imrl_tor( &imhin[n], fp_in[n], pixel[n], j );
		}
	      reorderflag=1;
	    }
	}

      /*---- get median pixels ------------------------------------*/

      for( k=0; k<ncol; k++ ) 
	{
	  ndat=0;
	  for( n=0; n<n_input; n++) 
	    {
	      if(pixel[n][k]!=pixignr[n])
		{
		  /* 1999/11/11 */
		  dat[ndat]=pixel[n][k]/factor[n];
		  ndat++;
		}
	    }
	  n_final = ndat;

	  /* 2002/11/10 */
	  if(sigma_r>0)
	    {
	      /*---- compute statistical value again 
		with sigma rejection ------*/	      
	      if (weighted==1)
		floatweightedmeanrms(ndat,dat,factor,&ave,&sdev);
	      else
		floatmeanrms(ndat,dat,&ave,&sdev);
	      for( m=0; m<cycle; m++ )
		{
		  if( fabs(sdev) < 0.001 ) break;
		  sigma = sigma_r * sdev;
		  
		  n=ndat;
		  for( p=0; p<n; p++ )
		    {
		      if ( ( dat[p] >= ( ave + sigma ) ) ||
			   ( dat[p] <= ( ave - sigma ) ) )
			{
			  /* not sort but swap */
			  temp=dat[n-1];
			  dat[n-1]=dat[p];
			  dat[p]=temp;
			  p--;
			  n--;
			}
		    }
		  ndat = n;
		  if (weighted==1)
		    floatweightedmeanrms(ndat,dat,factor,&ave,&sdev);
		  else
		    floatmeanrms(ndat,dat,&ave,&sdev);
		}
	      n_final=ndat;
	    }
	  if (n_final>1)
	    {
	      if (weighted==1)
		data[k]=floatweightedmedian(n_final,dat,factor);
	      else
		data[k]=floatmedian(n_final,dat);
	    }
	  else
	    data[k]=pixignr_out;
	}

      /*---- write a median image ----------------------------------*/
      imc_fits_wl( &imhout, fp_out, data, j );
    }

  (void) free(data);
  for( n=0; n<n_input; n++) 
    {
      if(nopennum!=0) fp_in[n]=fopen(fnamin[n],"r");
      imclose(fp_in[n],&imhin[n], &icomin[n]);
    }
  free(fp_in);
  (void) imclose(fp_out,&imhout,&icomout);

  return 0;
}
