/*--------------------------------------------------------------
 *
 *                        mask_for_AGX.c
 *
 *--------------------------------------------------------------
 *
 * Description:
 * ===========
 *   Reads a fits image 
 *   and replaces a region shadowed by AG-probe with magic number.
 *
 *   Region of j_limit <= j <= jmax is masked.
 *
 * Usage:      
 * =====
 *   mask_for_AGX  input  output  j_limit  magic
 *
 * Revision history:
 * ================
 *   created        Apr  4  2002  Shimasaku
 *   minor revision Apr 21  2002  Ouchi
 *
 *--------------------------------------------------------------
*/

#define   LINMAX   500

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "imc.h"


/*--------------------------------------------------------------*/

int main(int argc, char *argv[])
{

   FILE   *fp_im_in,    /* input fits file */
          *fp_im_out;   /* output fits file */

   int   i, j,         /* do loop for (px,py) */
          j_limit,
          ncol,         /* size of column (X) */
          nrow;         /* size of row (Y) */

   float  pixignr=-32768,pixignr_out=-32768;
   /*
   float  *data;
   */
   float  *frame;
   struct imh imhin={""},imhout={""};
   struct icom icomin={0},icomout={0};

   /*---- get argv[] ------------------------------------------*/

   if( argc != 5 ) {
      (void) printf("usage: mask_for_AGX  [input]  [output]  [j_limit]  ");
      (void) printf("[blank value(SPCAM=-32768)]\n");
      exit(-1);
      }
   j_limit = atol(argv[3]);
   pixignr_out = atof(argv[4]);

   /*---- read fits image file --------------------------------*/ 

   if( (fp_im_in = imropen_fits( argv[1], &imhin, &icomin )) 
           == NULL ) {
      (void) printf(" ERROR: Can't open %s.\n", argv[1] );
      (void) exit(-1);
      }

   ncol = imhin.npx;
   nrow = imhin.npy;

#if 0
   (void) printf(" nrow     = %4d\n", nrow );
   (void) printf(" ncol     = %4d\n", ncol );
#endif

   /*
   data  = (float *)malloc( ncol*sizeof(float) );
   */
   frame = (float *)malloc( ncol*nrow*sizeof(float) );

   /*---- mask ------------------------------------------------*/ 

#if 0
   for( j=0; j<nrow; j++ ) {
      imc_fits_rl( &imhin, fp_im_in, data, j );

      if( j < j_limit ) {
         for( i=0; i<ncol; i++ ) {
            frame[ncol*j+i] = data[i];
            }
         }
      else {
         for( i=0; i<ncol; i++ ) {
            frame[ncol*j+i] = pixignr;
            }
         }
      }
#endif
   if(j_limit>nrow) j_limit=nrow;

   for( j=0; j<j_limit; j++ ) {
      imc_fits_rl( &imhin, fp_im_in, frame+ncol*j, j );
   }
   for( j=j_limit; j<nrow ; j++ ) {
     for( i=0; i<ncol; i++ ) {
       frame[ncol*j+i] = pixignr;
     }
   }

   /*
     (void) fclose(fp_im_in);
   */

   /*---- write image -----------------------------------------*/ 

   /*
     (void) memcpy( (void *)&imhout, (void *)&imhin,
     sizeof(imhin) );
     (void) memcpy( (void *)&icomout, (void *)&icomin,
     sizeof(icomin) );
   */

   imh_inherit(&imhin,&icomin,&imhout,&icomout,argv[2]);
   imclose(fp_im_in,&imhin,&icomin);

   imhout.dtype = DTYPFITS;
   (void) imc_fits_set_dtype( &imhout, FITSFLOAT, 0.0, 1.0 );

   imhout.npx = ncol;
   imhout.npy = nrow;

   if( (fp_im_out = imwopen_fits( argv[2], &imhout, &icomout ))
           == NULL ) {
      (void) printf(" ERROR: Can't open %s.\n", argv[2] );
      (void) exit(-1);
      }

#if 0
   for( j=0; j<nrow; j++ ) {

     /*
       for( i=0; i<ncol; i++ ) {
       data[i] = frame[ncol*j+i];
       }
      imc_fits_wl( &imhout, fp_im_out, data, j );
     */
     imc_fits_wl( &imhout, fp_im_out, frame+ncol*j, j );     
      }
#endif
   imwall_rto(&imhout, fp_im_out, frame);

   /*
     (void) fclose(fp_im_out);
   */
   imclose(fp_im_out,&imhout,&icomout);

   /*
   (void) free(data);
   */
   (void) free(frame);

   return 0;

}

