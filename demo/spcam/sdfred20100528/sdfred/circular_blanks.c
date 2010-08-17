/*--------------------------------------------------------------
 *
 *                          circular_blanks.c
 *
 *--------------------------------------------------------------
 *
 * Description:
 * ===========
 *   make circular blanks
 *
 * Revision history:
 * ================
 *   created  Jun   2  2001  Ouchi
 *
 *--------------------------------------------------------------
*/

#define   BLANKLISTMAX 10000

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "imc.h"



/*--------------------------------------------------------------*/

int main( int argc, char *argv[] )
{
   FILE   *fp_in, *fp_out, *fp_inlist;

   int   i, j,          /* do loop for (px,py) */
          ncol,          /* image size (X) (pix) */
          nrow;          /* image size (Y) (pix) */

   char   *input,*output,*inlist;
#if 0
   double ncol_min[BLANKLISTMAX], ncol_max[BLANKLISTMAX],
          nrow_min[BLANKLISTMAX], nrow_max[BLANKLISTMAX];
#endif

   double x[BLANKLISTMAX], y[BLANKLISTMAX], r[BLANKLISTMAX];
   double dist_x, dist_y, dist;

   int    rec,k;
   float blank=-32768.;
   float  *frame;
   struct imh imhin={""},imhout={""};
   struct icom icomin={0},icomout={0};

   /*---- get argv[] ------------------------------------------*/

   if( argc != 5 ) {
     (void) printf("usage: circular_blanks [input image]  ");
     (void) printf("[blanklist( x y radius)]  ");
     (void) printf("[blank_value(SPCAM=-32768)] [output image]\n");
     (void) exit(-1);
   }

   input=argv[1];
   output=argv[4];
   inlist=argv[2];

/*
   ncol_min = atol(argv[2]);
   ncol_max = atol(argv[3]);
   nrow_min = atol(argv[4]);
   nrow_max = atol(argv[5]);
*/
   
   blank    = atof(argv[3]);

 /*---- read the list ---------------------------------------------*/ 

   if( (fp_inlist = fopen( inlist,"r"))==NULL) {
     fprintf(stderr,"ERROR: file %s not found \n",inlist);
     exit(-1);
   }

   rec=0;
   while( (fscanf(fp_inlist,"%lf %lf %lf",
	&x[rec],&y[rec],&r[rec]))!=EOF)
     rec++;
   
   if(rec==0) {
     fprintf(stderr,"ERROR: file %s has an inappropriate form \n",inlist);
     exit(-1);
   }

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
 
   frame = (float *)malloc( ncol*nrow*sizeof(float) );

   /*---- processing the data ----------------------------------*/ 
    
   /*
     for( j=0; j<nrow; j++ ) {
     imrl_tor( &imhin, fp_in, data, j );
     for( i=0; i<ncol; i++ ) {
     frame[ncol*j+i] = data[i];
     }
     }
   */
   imrall_tor(&imhin, fp_in, frame, ncol*nrow);

#if 0
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
#endif

   for( j=0; j<nrow; j++ ) {
     for( i=0; i<ncol; i++ ) {
       for( k=0;k<rec; k++) {
	 dist_x=(i+1)-x[k];
	 dist_y=(j+1)-y[k];
	 if(dist_x <= r[k] && dist_x >= -r[k]   
             && dist_y <= r[k] && dist_y >= -r[k]) {
	   dist=sqrt( pow(dist_x,2) + pow(dist_y,2) );
	   if(dist<=r[k]) {
	     frame[ncol*j+i] = blank;
	     break;
	   }
	 }
       }
     }
   }

   (void) fclose(fp_inlist);
   /*
     (void) fclose(fp_in);
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
     imc_fits_wl( &imhin, fp_out, data, j );
   }
   */
   imwall_rto(&imhout, fp_out, frame);

   (void) free(frame);
   /*
   (void) free(data);
   (void) fclose(fp_out);
   */
   imclose(fp_out,&imhout,&icomout);

   return 0;
}
