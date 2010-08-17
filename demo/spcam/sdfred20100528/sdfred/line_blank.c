/*--------------------------------------------------------------
 *
 *                          line_blank.c
 *
 *--------------------------------------------------------------
 *
 * Description:
 * ===========
 *   blank satellite trails
 *
 * Revision history:
 * ================
 *   created  Dec   9  2000  Masami Ouchi
 *
 *--------------------------------------------------------------
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "imc.h"


/*--------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
   FILE   *fp_in, *fp_out;
   float  *data;
   float  *frame;

   int   i, j,          /* do loop for (px,py) */
          ncol,          /* image size (X) (pix) */
          nrow;          /* image size (Y) (pix) */
   char   *input,*output;
   double x1,y1,x2,y2,width;
   double a,b,d;
   double b_low,b_up;

   float blank=-32768.;
   struct imh imhin={""},imhout={""};
   struct icom icomin={0},icomout={0};


   /*---- get argv[] ------------------------------------------*/

   if( argc != 9 ) {
     (void) printf("usage: line_blank  [input image]  ");
     (void) printf("[x1] [y1] [x2] [y2] [width]  ");
     (void) printf("[blank_value(SPCAM=-32768)] [output image]\n");
     (void) exit(-1);
   }

   /*
     (void) strcpy( input, argv[1] );
     (void) strcpy( output, argv[8] );
   */
   input=argv[1];
   output=argv[8];
   
   x1 = atoi(argv[2]);
   y1 = atoi(argv[3]);
   x2 = atoi(argv[4]);
   y2 = atoi(argv[5]);
   width = atoi(argv[6]);
   blank = atof(argv[7]);

 /*---- read the image ---------------------------------------------*/ 

   if( (fp_in = imropen( input, &imhin, &icomin )) == NULL ) {
     (void) printf(" ERROR: Can't open %s.\n", input );
     (void) printf(" Suffix of image must be either .pix or .fits.\n");
     (void) exit(-1);
   }
   ncol = imhin.npx;
   nrow = imhin.npy;

#if 0
   (void) printf(" nrow     = %4d\n", nrow );
   (void) printf(" ncol     = %4d\n", ncol );
#endif


   /*---- malloc -----------------------------------------------*/ 
 
   /*
   data  = (float *)malloc( ncol*sizeof(float) );
   */
   frame = (float *)malloc( ncol*nrow*sizeof(float) );

   /*---- processing the data ----------------------------------*/ 
    
   /* store the data */
   /*
     for( j=0; j<nrow; j++ ) {
     imrl_tor( &imhin, fp_in, data, j );
     for( i=0; i<ncol; i++ ) {
     frame[ncol*j+i] = data[i];
     }
     }
   */
   imrall_tor(&imhin,fp_in,frame,ncol*nrow);

   /* calculate coefficients of lines which surround the region */

   if( (x1-x2)==0 ) {
     fprintf(stderr,"ERROR: cannot define a, since x1-x2=0 \n");
     exit(-1);
   }

   a=(y1-y2)/(x1-x2);
   b=y1-a*x1;
   d=width/2.;

   b_low=b-d*sqrt(a*a+1.);
   b_up=b+d*sqrt(a*a+1.);

   /* substitute the blank value in the specified region */

   for( j=0; j<nrow; j++ ) {
       for( i=0; i<ncol; i++ ) {
	 if ( (  (j+1)-(a*(i+1)+b_low) >= 0 ) && ( (j+1)-(a*(i+1)+b_up) <= 0)) 
	   frame[ncol*j+i] = (float)blank;
       }
     }

   /*
   for( k=0; k<rec; k++) {
     for( j=0; j<nrow; j++ ) {
       for( i=0; i<ncol; i++ ) {
	 if ( ( i+1 >= ncol_min[k] ) && ( i+1 <= ncol_max[k] ) &&
	     ( j+1 >= nrow_min[k] ) && ( j+1 <= nrow_max[k] ) &&
	     ( frame[ncol*j+i] <= thres ) ) 
	   frame[ncol*j+i] = blank;
       }
     }
   }
   */

 
   /*---- write an output image ----------------------------------*/

   imh_inherit(&imhin,&icomin,&imhout,&icomout,output);
   imclose(fp_in,&imhin,&icomin);

   imhout.npx = ncol;
   imhout.npy = nrow;
   imhout.dtype = DTYPFITS;

   (void) imc_fits_set_dtype( &imhout, FITSFLOAT, 0.0, 1.0 );

   if( (fp_out = imwopen_fits( output,
                  &imhout, &icomout )) == NULL ) {
     (void) printf(" ERROR: Can't open %s.\n", output );
     (void) exit(-1);
   }

   /*
     for( j=0; j<nrow; j++ ) {
     for( i=0; i<ncol; i++ ) {
       data[i] = (float)(frame[ncol*j+i]);
     }
     imc_fits_wl( &imhout, fp_out, data, j );
     }
   */
   imwall_rto(&imhout, fp_out, frame);

   (void) free(frame);

   imclose(fp_out,&imhout,&icomout);

   return 0 ;
}
