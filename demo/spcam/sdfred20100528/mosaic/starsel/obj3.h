#ifndef OBJ_H
#define OBJ_H

typedef struct objrec3 pixlist_t;

struct objrec3 {			/* OBJECT PARAMETTERS DATA STRUCTURE */
  int entnum;
  int nalloc;
  int npix;
  int xmin, xmax, ymin, ymax;
  int *pixx;
  int *pixy;
  int x0,y0;
  float *flux;
};

typedef struct objrec objrec_t;

/* compatibility, need moment2d merge */
struct objrec {			/* OBJECT PARAMETTERS DATA STRUCTURE */
	int	entnum;		/* entry number */
	float	xc, yc;		/* intesity weithed image center */
	int	npix;		/* pixel number */
	float	fiso;		/* isophotal flux */
	float	peak;		/* peak value */
	int	ipeak, jpeak;	/* peak position */
	int	xmin, xmax;	/* image extent */
	int	ymin, ymax;

	float	Mxx;		/* Moment parameter */
	float	Myy;		/* Moment parameter */
	float	Cin;
	float	SB;
	float	a;		/* major axis */
	float	b;		/* minor axis */
	float	q;		/* axis ratio */
	float	pa;		/* Position Angle */
	int	blend;		/* blended : 1 ; single : 0 */

	int	*map;		/* object 1/0 map */
	float thres;
	float *img;
	objrec_t *next;
};

#endif
