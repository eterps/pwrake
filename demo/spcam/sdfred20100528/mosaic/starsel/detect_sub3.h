#ifndef DETECT_SUB3_H
#define DETECT_SUB3_H


#include "obj3.h"

extern int extract_pixlist(float *pix, int npx, int npy, 
			   int x, int y, 
			   /* absolute coordinate */
			   int id, 
			   float thres, 
			   int x0, int y0, 
			   /* shift from absolute to physical */
			   int *map, pixlist_t *pl);


int detect_full_new3(float *pix,
		     int	npx,
		     int	npy,
		     float thres,
		     int	npix_min,
		     
		     int *nobj_old,
		     pixlist_t ***ob_edge,
		     int x0,
		     int y0,
		     pixlist_t ***ob);
     
extern int detect_new3(float *pix, int npx, int npy, float thres,
		       int npix_min, int x0, int y0, int *map,
		       pixlist_t ***out, int *nedged, pixlist_t ***edged);

extern int detect_simple_array(float *pix, int npx, int npy, float thres,
			       int npix_min, objrec_t ***ob);

/* for compativility */
extern int detect_simple(float *pix, int npx, int npy, float thres,
			 int npix_min, objrec_t **ob);

extern int convert_pixlist(objrec_t *ob,pixlist_t *ob3);
extern int convert_objrec(pixlist_t *ob3,objrec_t *ob);


extern int makeimage_pixlist(float *g, int npx, int npy,
			     int xmin, int ymin, pixlist_t *pl);

extern int clear_pixlist(pixlist_t *ob);
extern int free_pixlist(pixlist_t *ob); 

extern int clear_objrec(objrec_t *ob);
extern int free_objrec(objrec_t *ob) ;
extern int free_objrec_list(objrec_t *ob);

#endif
