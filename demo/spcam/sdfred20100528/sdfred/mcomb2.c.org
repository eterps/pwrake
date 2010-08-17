
/*------------------------------------------------------
*
*                        mcomb2.c
*
*Description:
*            make the median combined image
*
*       Created by Masami Ouchi 08/12/99
*       Revised by Masami Ouchi 03/23/2000
*       Revised by Masami Ouchi 04/17/2002
*
*
*-------------------------------------------------------*/

#define   LINE 1000
#define   MAX_INPUT_FRAME 1000

#define   DEFAULT_REJECTION_SIGMA 3.0
#define   DEFAULT_NUMBER_OF_REJECTION 1

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "imc.h"
#include "stat.h"
#include "sort.h"


int main( int argc, char **argv )
{
  int   ncol, nrow;
  
  float *data[MAX_INPUT_FRAME];
  float *frame;
  float *localpix;
  float *localpix2;
  float pixignr=-32768;
  
  
  FILE     *fp_infile;
  FILE     *fp_in[MAX_INPUT_FRAME], *fp_out;
  struct imh imhin[MAX_INPUT_FRAME]={{""}},imhout={""};
  struct icom icomin[MAX_INPUT_FRAME]={{0}},icomout={0};

  char     inlist[LINE];
  char     infile[MAX_INPUT_FRAME][LINE],outfile[LINE];
  int     i,j,k,l,m;
  int     k_frame;
  
  int      number_of_frame;
  float    med;

  /*
  float    sum,stddev;
  */
  float    stddev;
  float    upper,lower;
  double   rejection_sigma=DEFAULT_REJECTION_SIGMA;
  int      number_of_rejection=DEFAULT_NUMBER_OF_REJECTION;

  /* get argv[] */
  if( !(argc == 3 || argc == 5) ) {
    printf(
	"usage: mcomb [list of input files] [output file]\n");
    printf(
	" or (in case rejection sigma and number of rejection are customized)\n");
    printf(
	"usage: mcomb [list of input files] [output file] [rejection sigma] [number of rejection]\n");

    exit (-1);
  }

  if( argc == 5 ) {
    rejection_sigma = atof( argv[3]);
    number_of_rejection = atoi( argv[4]);
  }

  strcpy( inlist, argv[1] );
  strcpy( outfile, argv[2] );

  printf("\n --Processing images contained in %s \n",inlist);

  /* take the bias image names from infile */
  if( (fp_infile = fopen( inlist, "r")) == NULL) {
    printf( " ERROR: Can't open %s. \n",inlist );
    exit(-1);
  }

  number_of_frame=0;
  for( i=0;i<MAX_INPUT_FRAME;i++) 
    if( (fscanf(fp_infile,"%s", infile[i])) == EOF) {
      break;
    } 
    else {
      number_of_frame++;
    }
  printf("Number of input images : %d \n",number_of_frame);

  fclose(fp_infile);

  for( i=0;i<number_of_frame;i++) 
    printf("Input file: %s\n",infile[i]);

  /* open images */
  for( i=0;i<number_of_frame;i++) {
    if( (fp_in[i] = imropen( infile[i], &(imhin[i]), &(icomin[i]) )) 
	== NULL ) {
      printf( " ERROR: Can't open %s. #%d \n", infile[i] , i);
      printf( " Suffix of image must be either .pix or .fits. \n");
      exit(-1);
    }
    if(i==0) {
      ncol = imhin[i].npx;
      nrow = imhin[i].npy;
    } 
    else {
      if(ncol != imhin[i].npx && nrow != imhin[i].npy ){
	printf( "ERROR: The size of image is different \n" );
	exit(-1);
      }
    }
  }

  printf("Image size %ld x %ld \n",ncol,nrow);
  printf("Rejection sigma = %f\n",rejection_sigma);
  printf("Number of rejection = %d [0 means none]\n",number_of_rejection);
  printf("Blank value = %d\n",pixignr);

  /* allocate the memory */
  for(i=0;i<number_of_frame;i++) {
    data[i] = (float *)malloc( ncol*sizeof(float) );
  }

  localpix = (float *)malloc( (number_of_frame+1)*sizeof(float) );
  localpix2 = (float *)malloc( (number_of_frame)*sizeof(float) );

  localpix[0]=0;

  frame = (float *)malloc( ncol*nrow*sizeof(float) );

  /* read the image and take the median of the frames*/
  for( j=0; j<nrow; j++) {
    /* read pixel vlues of one row */
    for(m=0; m<number_of_frame; m++) 
      {
	imrl_tor( &imhin[m], fp_in[m], data[m], j);
      }
    
    /* store the median values into frame[] */
    for( i=0; i<ncol; i++) {


      
      /* store the values of local pixels into localpix[] */
      k=0;
      for(m=0; m<number_of_frame;m++) {
	if(data[m][i] != pixignr ) {  
	  localpix[k]=data[m][i];
	  k++;
	}
      }

      
      if(k==0) {
	med=pixignr;
	goto blank;
      }
      if(k==1) {
	med=localpix[0];
	goto single;
      }

      /* take the median without the rejection */
      /*
	mdian1(localpix, k, &med);
      */
      med=floatmedian(k,localpix);
      k_frame=k;


      /* reject the pixels over sigma*REJECTON_SIGMA */
      for( l=0; l< number_of_rejection; l++) {

	/* old way to derive stddev */
	/*
	  sum=0;
	  for( m=1; m<=k;m++) sum=sum+pow((localpix[m]-med),2);
	  stddev=sqrt(sum/( (float) k ));
	*/

	/* new way to derive stddev */
	/*
	if(k>=14) {
	  for( m=1; m<=k;m++) localpix2[m-1]=localpix[m] ;
	  sort(m-1,localpix2);
	  stddev=med-localpix2[(int) (k/2*0.1587)];
	} else {
	  for( m=1; m<=k;m++) localpix2[m-1]=localpix[m] ;
	  sort(m-1,localpix2);
	  stddev=med-localpix2[0];
	}
	*/

	/*
	  sort(k,localpix);
	  stddev=med-localpix[(int) (k/2*0.1587+1)];
	*/

	stddev=med-nth(k_frame,localpix,floor((float)(k-1)*0.1587));

	upper=med+stddev*rejection_sigma;
	lower=med-stddev*rejection_sigma;

	k=0;
	for( m=0; m<k_frame;m++) 
	  if(localpix[m]>=lower && localpix[m]<=upper) {
	    localpix[k]=localpix[m];
	    k++;
	  } 

	if(k==0) {
	  med=pixignr;
	  goto blank;
	}
	if(k==1) {
	  med=localpix[0];
	  goto single;
	}

	/*
	  mdian1(localpix, k, &med);
	*/
	med=floatmedian(k,localpix);

      }
      
    blank:
    single:
      /* store the median values */
      frame[ncol*j+i] = med;
    }
  }

  /* open the output image file  */
  imh_inherit(&imhin[0],&icomin[0],&imhout,&icomout,outfile);
  imhout.dtype = DTYPFITS;
  imc_fits_set_dtype( &imhout, FITSFLOAT, 0.0, 1.0 );

  for(i=0;i<number_of_frame;i++)
    imclose(fp_in[i],&(imhin[i]),&(icomin[i]));

  if( (fp_out = imwopen_fits( outfile, &imhout, &icomout)) == NULL ){
    printf(" ERROR: Can't open %s.\n",outfile );
    exit(-1);
  }

  /* write an image into an output file */  
  for( j=0; j<nrow; j++) {
    for( i=0; i<ncol; i++) {
      data[0][i] = (float)frame[ncol*j+i];
    }
    imc_fits_wl( &imhout, fp_out, data[0], j);
  }
  
  /* free the memory allocation */
  free(frame);
  for(i=0;i<number_of_frame;i++)
    free(data[i]);
  imclose(fp_out,&imhout,&icomout);

  printf("--Created median combined image is %s\n",outfile);

  return 0;
}






