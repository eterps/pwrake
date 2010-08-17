/* ---------------------------------------------------------------------------
 *		                 imc.h
 *  --------------------------------------------------------------------------
 *
 *  This include file defines structures of iraf image and header file.
 *
 *                                                     Maki Sekiguchi
 * ---------------------------------------------------------------------------
 */

#ifndef IMC_H
#define IMC_H

#include <stdio.h>

#ifndef u_short 
#define u_short unsigned short
#endif
#ifndef u_char 
#define u_char unsigned char
#endif
#ifndef u_int
#define u_int unsigned int
#endif

/********************************************
 *                                          *
 *          Define constants                *
 *                                          *
 ********************************************/


#define CHASIZ	160
#define VID2SIZ	1000
#define VID3SIZ 292
#define IMH_MAXCOM	128	/* max # of comment line allowed */
#define IMH_COMSIZE	162	/* sizeof( one comment line ) */

#define FITS_MAXLINE    36

#define PIXOFF	1024	/* pixel data offset in pix file */

typedef enum {
  DTYPI2=3,	/* iraf data type for signed short */
  DTYPINT=4,	/* iraf data type for int (normally log) */
  DTYPI4=5,	/* iraf data type for signed int (integer*4) */
  DTYPR4=6,	/* iraf data type for float (real*4) */
  DTYPR8=7,	/* iraf data type for double (real*8) */
  DTYPCMP=8,	/* iraf data type for complex */
  DTYPU2=11,	/* iraf data type for unsigned short */
  DTYPFITS=-1,	/* FITS data type, See struct imc_fits */
  DTYPEMOS=-11    /* *.mos type */
} DTYP;

#define FITSCHAR   8	/* FITS data type for signed I*1 */
#define FITSSHORT 16	/* FITS data type for signed I*2 */
#define FITSINT   32    /* FITS data type for sigend I*4 */
#define FITSFLOAT -32   /* FITS data type for float R*4 */

#define IMC_FITS_INTFORM "%20d / %-47.47s"
#define IMC_FITS_FLOATFORM "%20E / %-47.47s"
#define IMC_FITS_LOGICALFORM "%-20.20s / %-47.47s"
#define IMC_FITS_STRINGFORM "\'%-8s\' / %-47.47s"

/********************************************
 *                                          *
 *   Define structure for iraf imh file     *
 *                                          *
 ********************************************/

struct imh {
					/* start_addres   size      contents */
#include "imc_head.h"
		 			/* 0x000	  0x2dc     common header */
        char	title[CHASIZ];		/* 0x2dc	  0xa0	    Image title */
	char	institute[CHASIZ];	/* 0x37c	  0xa0	    Institution */
	char    void_2[VID2SIZ]; 	/* 0x41c          0x3e8     not used */
					/* 0x804	   */
  /* 2003/11/04*/
  char *buff;
};


/********************************************
 *                                          *
 *  Define structure for pixel file header  *
 *                                          *
 ********************************************/

struct pixh {
		 			/* start_addres   size      contents */
#include "imc_head.h"
		  	 		/* 0x000	  0x2dc     common header */
	char	void_3[VID3SIZ];	/* 0x2dc	  0x124	    not used */	  
					/* 0x400			     */
};


/********************************************
 *                                          *
 *    Structure for comment strage          *
 *                                          *
 ********************************************/

struct icom {
	int	ncom;				/* # of filled comment lines */
	char	**com;	/* comments in iraf format */
};


/**********************************
 *                                *
 *  Global struct for mosaic file *
 *                                *
 **********************************/

#if 0 /* to be deleted */
#if 0
#define MAXFRM 1024        /* max num of mos frames */
#else
#define MAXFRM 512        /* max num of mos frames */
#endif
#endif 


#define MOSEXTEN ".mos"   /* mosaic (multiple image) file extension */
#define FITSEXTEN ".fit" /* fits file extension */

/* Select how pixels are assembled */

typedef enum{
  NODIST_NOADD=0,
    /* NODIST_ADD=1, */
    DIST_NOADD=3, 
    DIST_ADD=3, 
    CONST_ADD=10,
    NODIST_MED=11,
    DIST_MED=13,
    CONST_MED=14,

    DIST_CLIP_ADD=15, 

    DIST_FLUX=40, 
    DIST_PEAK=41,
    /*
    DIST_ADD_LARGE=23, 
    CONST_ADD_LARGE=30,
    DIST_MED_LARGE=33,
    CONST_MED_LARGE=34
    */

} MOSMODE;

typedef	       struct {
  char   fnam[BUFSIZ];	/* file name of each frame */
  int   sel;		/* =0 Not selected */
  float  xof; 		/* position offsets lower-left vx */
  float  yof; 		/* position offsets lower-left vy */
  float  vxmin;		/* minimum x in virtual space */
  float  vxmax;		/* maximum x in virtual space */
  float  vymin;		/* minimum y in virtual space */
  float  vymax;		/* maximum y in virtual space */
  float  vxLL;		/* virtual x of lower left */
  float  vyLL;		/* virtual y of lower left */
  float  vxLR;		/* virtual x of lower right */
  float  vyLR;		/* virtual y of lower right */
  float  vxUL;		/* virtual x of upper left */
  float  vyUL;		/* virtual y of upper left */
  float  vxUR;		/* virtual x of upper right */
  float  vyUR;		/* virtual y of upper right */
  float  slp_L;		/* left   side x= slp*y + cnst */
  float  cnst_L;		/* left   side x= slp*y + cnst */
  float  slp_R;		/* right  side x= slp*y + cnst */
  float  cnst_R;		/* right  side x= slp*y + cnst */
  float  slp_T;		/* top    side y= slp*x + cnst */
  float  cnst_T;		/* top    side y= slp*x + cnst */
  float  slp_B;		/* bottom side y= slp*x + cnst */
  float  cnst_B;		/* bottom side y= slp*x + cnst */
  float  rot;		/* rotation */
  float  sinrot;		/* sin(rot) */
  float  cosrot;		/* cos(rot) */
  float  flux; 		/* flux factor */
  
  int   npx;		/* x frame size in its frame */
  int   npy;		/* y frame size in its frame */
  int   ndx;		/* # of x data in its frame */
  int   ndy;		/* # of y data in its frame */
  int   typ;		/* data type */
  int   sky;		/* sky value in electron */
  int   pixignr;	/* pix value to be ignored */
  FILE   *pixp;		/* pixel file pntr */
} mos_frm;

struct mos {
  /* parameters for whole virtual image */
  int nfrm;	  /* # of frames used */
  float vxoff;      /* vx= pixel# - vxoff */
  float vyoff;	  /* vy= pixel# - vyoff */
  int npx;	  /* # of x pixel */
  int npy;	  /* # of y pix */
  int nsel;	  /* # of selected frame */
  int wx1;	  /* selected window smallest x */
  int wx2;	  /* selected window largest  x */
  int wy1;	  /* selected window smallest y */
  int wy2;	  /* selected window largest  y */	
  MOSMODE mix;	  /* !=0 pixel data will be coadded */
  int maxadd;	/* max# of frames to be coadded */
  int   pixignr;	/* pix value to be ignored */
  
  /* parameters for buffer information */
  int buffsize;	/* current buffer size in byte */
  float *vpixp;	/* pntr to virtual pixel = (float *)buffp */
  int iy0;	/* y-line# of first line in buffer */
  int buff_fill; /* != 0 means buffer is filled */

  /* 1999/06/16 */
  /* 32 bit field required */
  /*  unsigned short *fmapp; *//* pntr to flux map */
  unsigned int *fmapp;
  int *sort2frm; /* Sort# to fram# */
  int *frm2sort; /* Frame# to sort# */
  int *cent_sfrm;	/* estimated center sort# */
  mos_frm *frm;
  /* parameters for each real frame */	

  /* 2005/01/19 */
  int medbufsiz;
  int frmbufsiz;
  int fmappsiz;

  float *frmbuf;
  float *medbuf;
};


/********************************************
 *                                          *
 *          Declare functions               *
 *                                          *
 ********************************************/

extern  FILE    *imropen(char *fnam, struct imh  *imhp, struct icom *comp);
extern  FILE    *imwopen(char *fnam, struct imh  *imhp, struct icom *comp);
extern FILE *imclose ( FILE *fp, struct imh  *imhp, struct icom *comp);

extern  void    imhdump(struct imh  *imhp, struct icom *comp);
extern  int     imrall_tor(struct imh  *imhp, FILE *fp, float *pix, int pdim);
extern  int     imwall_rto(struct imh  *imhp, FILE *fp, float *pix);
extern  void    imdecs(char *ims, char *s);
extern  void    imencs(char *ims, char *s);
extern  void    imencs0(char *ims, char *s);
extern  int     imrl_tor(struct imh  *imhp, FILE *fp, float *dp, int iy);
extern  int     imwl_rto(struct imh  *imhp, FILE *fp, float *dp, int iy);
extern  void    imadd_fits( struct icom *comp, char *s);
extern  void    imset_u2off(struct imh  *imhp, struct icom *comp,int u2off);
extern  int    imget_u2off(struct imh  *imhp);

extern  void    imc_mos_set_add(struct imh *imhp, MOSMODE mode);
extern  void    imc_mos_set_maxadd(struct imh *imhp, int maxadd);
extern  void    imset_pixignr(  struct imh *imhp, struct icom *comp, int pixignr  );
extern  int    imget_pixignr(  struct imh *imhp  );
extern  int     imget_fits_value(  struct icom *comp, char *key, char *sout  );
extern  int     imrep_fits( struct icom *comp, char *key, char *s  );
/* 97 Feb */ 
extern float *imc_read(struct imh *imhp, FILE *fp, int, int, int, int );

extern int imc_mos_set_shift(struct imh *imhp,float x,float y);
extern int imc_mos_get_shift(struct imh *imhp,float *vxmin,float *vxmax, float *vymin, float *vymax);

/* 2002/04 */
extern int imrep_fitsf(struct icom *comp, char *key,char *format, ... );
extern int imupdate_fitsf(struct icom *comp, char *key, char *format, ... );
extern void imaddcomf(struct icom *,char *,...);
extern void imaddhistf(struct icom *,char *,...);


extern void imh_inherit(struct imh *imhin,
			struct icom *icomin,
			struct imh *imhout,
			struct icom *icomout,char* fnamout);

extern void imc_mos_set_default_add(MOSMODE);
extern void imc_mos_set_default_maxadd(int);
extern void imc_mos_set_pixignr(struct imh *,int);
extern void imaddcom(struct icom *,char *);
extern void imaddhist(struct icom *,char *);
extern void imc_mos_set_nline(int);
extern void imc_mos_set_nline_l(int);

extern int imget_fits_float_value
(struct icom *comp, char *key, float *fout);
extern int imget_fits_int_value
(struct icom *comp, char *key, int *iout);


extern int imc_mos_set_clip_param
(float factor, float frac, int nmin_clip, int nmin_mean);

extern  int	imc_fits_set_dtype(struct imh *imhp,int dtype,  
				   float bzero,float bscale);
extern  void    imc_fits_get_dtype(struct imh *imhp,int *dtype,  
				   float *bzero,float *bscale, int *pixoff);
extern int imc_fits_dtype_string(char *dtype);

extern int imh_reformat_fits(char *line);

int imc_merge_icom(struct icom *icom1, struct icom *icom2,
		   int ncom0, char **key0);

int imc_mos_get_params(struct imh *imhin,
		       float *vxoff,
		       float *vyoff,
		       int *npx,
		       int *npy,
		       MOSMODE *mix,
		       int *maxadd,
		       float *pixignr,
		       int *nfrm,
		       mos_frm **frm);

/* imc_WCS */
extern int imc_getWCS(struct icom *icom, 
		      char *crval1,
		      char *crval2,
		      char *equinox,
		      char *cdelt1,
		      char *cdelt2,
		      char *longpole,
		      char *ctype1,
		      char *ctype2,
		      char *cunit1,
		      char *cunit2,
		      char *crpix1,
		      char *crpix2,
		      char *cd11,
		      char *cd12,
		      char *cd21,
		      char *cd22);
extern int imc_checkWCS(struct icom *icom);
extern int imc_copyWCS(struct icom *icomin, struct icom *icomout);

extern int imc_shift_WCS(struct icom *icom,float dx,float dy);

/* dx and dy should be checked  2002/05/07 */
extern int imc_rotate_WCS(struct icom *icom, 
                          float dx, float dy, double cos_rot, double sin_rot);
extern int imc_scale_WCS(struct icom *icom, float scale);
extern int imc_scale_WCS2(struct icom *icom, 
			  float scalex,
			  float scaley);

/****************/


/********************************************************
 *                                                      *
 * following functions related to mosaic multiple files *
 * A user should NOT call these functions directly      *
 *                                                      *
 ********************************************************/

extern  FILE    *imropen_mos(char *mosnam, struct imh *imhp,struct icom *comp);
extern  int     imrall_tor_mos(struct imh  *imhp, float *pix, int pdim);
extern  int     imrl_tor_mos(struct imh  *imhp, float *dp, int iy);
extern  struct mos *imc_mos_get_mosp(struct imh *);
extern  void    imc_mos_get_vlimit(struct imh *imhp, 
                                   float *vxmin, float *vxmax,
                                   float *vymin, float *vymax);

extern  float   *imc_mos_fill_buffer(struct mos *mosp,
                                     int vx1, int vx2, 
                                     int vy1, int vy2);
extern  int     imc_mos_check_fill_buffer(struct imh *imhp,
                                          int vxmin, int vxmax,
                                          int vymin, int vymax,
                                          int nline);
extern  void    imc_mos_free_buffer(struct mos *mosp);
extern  void    imc_mos_select_frame( struct mos *mosp );
extern  void    imc_mos_norm_flux( struct mos *mosp  );
/*
extern  int     imc_mos_xbound();
extern  void    imc_mos_cal_flux();
extern  void    imc_mos_vsort();
*/

extern  float  *imc_mos_read(struct imh *imhp,
                                     int vx1, int vx2, 
                                     int vy1, int vy2);
extern  void    imc_sort_frame(  struct mos *mosp);
extern  int    imc_mark_fmap(struct mos *mosp, int ifrm, 
			     int ivx1, unsigned int *fmap);
extern  int    imc_mos_get_nframes(  unsigned int fmap );
extern  void   imrestore_pixingr(  struct imh *imhp, struct icom *comp  );

int imc_mos_merge_org_header(struct icom *icomout,
			     struct imh *imhin);

/********************************************************
 *                                                      *
 * following functions related to FITS files            *
 * A user should NOT call these functions directly      *
 *                                                      *
 ********************************************************/

extern  FILE    *imropen_fits(char *fnam, struct imh  *imhp, struct icom *comp);
extern  FILE    *imwopen_fits(char *fnam, struct imh  *imhp, struct icom *comp );
extern  int	imc_fits_rl(struct imh  *imhp, FILE *fp, float *dp, int iy );
extern  int	imc_fits_wl(struct imh  *imhp, FILE *fp, float *dp, int iy );
extern  void    imc_fits_extract_string(char *fl,char *s );
extern  void    imc_fits_mkint(char *line, char *key, int val, char *com );
extern  void	imc_fits_mkfloat( char *line, char *key, float val , char *com);
extern  void	imc_fits_mklog(char *line, char *key, int val, char *com );
extern  void	imc_fits_mkstrg(char *line, char *key, char *strg, char *com );
extern  void    imc_fits_mkstrg_val(char *line,char *strg);
extern  void    imc_fits_swap16(void *dat,int nword );
extern  void    imc_fits_swap32(void *dat,int nword );



#endif

/* ------------------------------- end of imc.h ------------------------------ */
