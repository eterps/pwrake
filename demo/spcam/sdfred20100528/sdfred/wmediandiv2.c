/*------------------------------------------------------
*
*                        wmediandiv2.c
*
*Description:
*            divide a sigma-rejected median value from an image
*
*       Created by Masami Ouchi 08/10/99
*       Revised by Masami Ouchi 04/17/2002
*
*
*-------------------------------------------------------*/

#define   REJECTION_SIGMA 3.0
#define   NUMBER_OF_REJECTION 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "imc.h"
#include "stat.h"
#include "sort.h"

int main( int argc, char **argv )
{
  FILE     *fp_in,*fp_out;

  struct imh imhin={""},imhout={""};
  struct icom icomin={0},icomout={0};
  int   ncol, nrow;
  float *data;
  float *frame;

  char  *infile,*outfile;

  int     i,j,k,l;
  int     total_pix;
  int     col1=1,col2=1;
  int     row1=1,row2=1;
  float    med;
  float    stddev;
  float    upper,lower;
  float pixignr=-32768;

  /* get argv[] */
  if( !(argc == 7 || argc == 3) ) {
    fprintf(stderr,
	"usage: wmediandiv [input file] [col1] [col2] [row1] [row2] [output file]\n");
    fprintf(stderr,
	" or (in case a median is dirived from all the image region) \n"); 
    fprintf(stderr,
	"usage: wmediandiv [input file] [output file]\n");
    exit(-1);
  }


  if( argc == 3 ){
    infile=argv[1];
    outfile=argv[2];
  }    
  if( argc == 7) {
    col1=atoi(argv[2]);
    col2=atoi(argv[3]);
    row1=atoi(argv[4]);
    row2=atoi(argv[5]);
    
    infile=argv[1];
    outfile=argv[6];
  }

  /* open image */
  if( (fp_in = imropen( infile, &imhin, &icomin )) == NULL ) {
    printf( " ERROR: Can't open %s. \n", infile );
    printf( " Suffix of image must be either .pix or .fits. \n");
    exit(-1);
  }
  ncol = imhin.npx;
  nrow = imhin.npy;

  if( argc == 3 ){
    col1=1;
    col2=ncol;
    row1=1;
    row2=nrow;
  }
  if( argc == 7 ){
    /* check the size of region */
    if(col1 > col2 || row1 > row2) {
      printf(" ERROR: The region should be specified:[lower col] [upper col] [lower row] [upper low]\n");
      exit(-1);
    }
    if(col2 > ncol || row2 > nrow || col1< 1 || row1 < 1) {
      printf(" ERROR: The image dose not have the specified region [%ld:%ld,%ld:%ld] \n",col1,col2,row1,row2);
      exit(-1);
    }
  }

  printf("\n --Processing %s \n",infile);
  printf("processing region is [%ld:%ld,%ld:%ld] \n",col1,col2,row1,row2);

  printf("Image size %ld x %ld \n",ncol,nrow);

  /* allocate the memory */
  data  = (float *)malloc( ncol*sizeof(float) );
  frame = (float *)malloc( ((col2-col1+1)*(row2-row1+1)+1)*sizeof(float) );

  /* read the image and take the median of the region */

  pixignr=(float)imget_pixignr(&imhin);

  k=0;
  for( j=row1-1; j<row2; j++) {
    /* read pixel vlues of the specified row */
    imrl_tor( &imhin, fp_in, data, j);

    /* store the pixel values of the specifed region */
    for( i=col1-1; i<col2; i++) {
      if(data[i]!=pixignr) {
	frame[k] = data[i];
	k++;
      }
    }
  }

  printf("Calculating the median : %d times, %f sigma rejection...\n",
	 NUMBER_OF_REJECTION, REJECTION_SIGMA);
  printf("Pixels with blank value, %f, are rejected\n", pixignr);

  /* take the median of the specified region */
  /*
    mdian1(frame, k, &med);
  */
  med=floatmedian(k,frame);
  total_pix=k;
  printf("The total number of pixels without blanks : %ld \n",total_pix);

  /* reject the pixels over REJECTION_SIGMA and take the median */

  for( l=0; l< NUMBER_OF_REJECTION; l++) {
    printf("%ld rejection -> med=%f: reject pix=%ld\n",l,med,total_pix-k);

    /* old way to derive stddev */
    /*
      sum=0;
      for( i=1; i<=k;i++) sum=sum+pow((frame[i]-med),2);
      stddev=sqrt(sum/( (float) k ));
    */

    /* new way to derive stddev */
    /*
      sort(k,frame);
      stddev=med-frame[(int) (k/2*0.1587+1)]; <<== WRONG...
    */

    med=floatmedian(k,frame);
    stddev=med-nth(k,frame,floor((float)(k-1)*0.1587));

    upper=med+stddev*REJECTION_SIGMA;
    lower=med-stddev*REJECTION_SIGMA;

    k=0;
    for( i=0; i<total_pix;i++) 
      if(frame[i]>=lower && frame[i]<=upper) {
	frame[k]=frame[i];
	k++;
      } 
    med=floatmedian(k,frame);
  }

  printf("%ld rejection -> med=%f: rejected pix=%ld\n",l,med,total_pix-k);

  if( med==0 ) {
    printf("Can't divide image by the median 0\n");
    exit(-1);
  }
  printf("%s is divided by the median %f  \n",outfile,med);
  printf("Pixel value, %f, is neglected \n", pixignr);

  /* store the header value of the input image */
  /*
  imhout=imhin;
  */
  imh_inherit(&imhin,&icomin,&imhout,&icomout,outfile);

  /* write an image into an output file */
  imhout.npx = ncol;
  imhout.npy = nrow;
  imhout.dtype = DTYPFITS;
  imc_fits_set_dtype( &imhout, FITSFLOAT, 0.0, 1.0 );
  if( (fp_out = imwopen_fits( outfile, &imhout, &icomout)) == NULL ){
    printf(" ERROR: Can't open %s.\n",outfile );
    exit(-1);
  }
  
  for( j=0; j<nrow; j++) {
  
  /* read and write the pixel vlues of the specified row */
    imrl_tor( &imhin, fp_in, data, j);
    for(i=0;i<ncol;i++) {
      if( data[i]!=pixignr ) {
	data[i] = (float)data[i]/med;
      }
    }
    imc_fits_wl( &imhout, fp_out, data, j);
  }
  
  imclose(fp_in,&imhin,&icomin);
  imclose(fp_out,&imhout,&icomout);

  printf("---The median divided image is %s\n\n",outfile);

  free(frame);
  free(data);

  return 0;
}






