/*--------------------------------------------------------------
 *
 *                          blank2.c
 *
 *--------------------------------------------------------------
 *
 * Description:
 * ===========
 *   tanslate bad pixel to blank
 *
 * Revision history:
 * ================
 *   created  Aug  19  1999  Nakata
 *   revised  Feb   6  2000  Ouchi
 *
 *--------------------------------------------------------------
*/

/*
#define   PI        3.14159265359
*/
#define   LINMAX    256
#define   BLANKLISTMAX 100

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "imc.h"

/* define struct for imh */
struct hdr {
        struct imh obj;
        } hdr;

/* define struct for icom (comments in image-header) */
struct cmt {
        struct icom obj;
        } cmt;

double atof(const char *nptr);

float  *data;
float  *frame;


/*--------------------------------------------------------------*/

int main( int argc, char *argv[] )
{

   void   exit();

   FILE   *fp_in, *fp_out, *fp_inlist;

   long   i, j,          /* do loop for (px,py) */
          ncol,          /* image size (X) (pix) */
          nrow;          /* image size (Y) (pix) */
   char   input[LINMAX],output[LINMAX],inlist[LINMAX];
   double ncol_min[BLANKLISTMAX], ncol_max[BLANKLISTMAX],
          nrow_min[BLANKLISTMAX], nrow_max[BLANKLISTMAX];
   int    rec,k;
   double thres, blank;

   /*---- get argv[] ------------------------------------------*/

   if( argc != 6 ) {
     (void) printf("usage: blank  input (*.fits)  ");
     (void) printf("blankmap[col_min col_max row_min row_max]  ");
     (void) printf("max_value(removed less than it)  blank  output (*.fits)\n");
     (void) exit(-1);
   }

   (void) strcpy( input, argv[1] );
   (void) strcpy( output, argv[5] );
   (void) strcpy( inlist, argv[2] );

/*
   ncol_min = atol(argv[2]);
   ncol_max = atol(argv[3]);
   nrow_min = atol(argv[4]);
   nrow_max = atol(argv[5]);
*/
   
   thres    = atof(argv[3]);
   blank    = atof(argv[4]);

 /*---- read the list ---------------------------------------------*/ 

   if( (fp_inlist = fopen( inlist,"r"))==NULL) {
     fprintf(stderr,"ERROR: file %s not found \n",inlist);
     exit(-1);
   }

   rec=0;
   while( (fscanf(fp_inlist,"%lf %lf %lf %lf",
	&ncol_min[rec],&ncol_max[rec],&nrow_min[rec],&nrow_max[rec]))!=EOF)
     rec++;
   
   if(rec==0) {
     fprintf(stderr,"ERROR: file %s has an inappropriate form \n",inlist);
     exit(-1);
   }

 /*---- read the image ---------------------------------------------*/ 

   if( (fp_in = imropen( input, &hdr.obj, &cmt.obj )) == NULL ) {
     (void) printf(" ERROR: Can't open %s.\n", input );
     (void) printf(" Suffix of image must be either .pix or .fits.\n");
     (void) exit(-1);
   }
   ncol = hdr.obj.npx;
   nrow = hdr.obj.npy;

#if 0
   (void) printf(" nrow     = %4d\n", nrow );
   (void) printf(" ncol     = %4d\n", ncol );
#endif


   /*---- malloc -----------------------------------------------*/ 
 
   data  = (float *)malloc( ncol*sizeof(float) );
   frame = (float *)malloc( ncol*nrow*sizeof(float) );

   /*---- processing the data ----------------------------------*/ 
    
   for( j=0; j<nrow; j++ ) {
     imrl_tor( &hdr.obj, fp_in, data, j );
     for( i=0; i<ncol; i++ ) {
	 frame[ncol*j+i] = data[i];
       }
   }

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
   

   (void) fclose(fp_inlist);
   (void) fclose(fp_in);
 
   /*---- write an output image ----------------------------------*/

   hdr.obj.npx = ncol;
   hdr.obj.npy = nrow;
   hdr.obj.dtype = DTYPFITS;

   (void) imc_fits_set_dtype( &hdr.obj, FITSFLOAT, 0.0, 1.0 );

   if( (fp_out = imwopen_fits( output,
                  &hdr.obj, &cmt.obj )) == NULL ) {
     (void) printf(" ERROR: Can't open %s.\n", output );
     (void) exit(-1);
   }

   for( j=0; j<nrow; j++ ) {
     for( i=0; i<ncol; i++ ) {
       data[i] = (float)(frame[ncol*j+i]);
     }
     imc_fits_wl( &hdr.obj, fp_out, data, j );
   }

   (void) free(frame);
   (void) free(data);

   (void) fclose(fp_out);

   return 0;
}
