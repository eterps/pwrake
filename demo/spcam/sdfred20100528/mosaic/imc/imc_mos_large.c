/*
 * ==================================================================================
 *			          File:  imc_mos.c
 * ==================================================================================
 *
 *   User callable functions are:
 *
 *	void   imc_mos_set_add ( mode )
 *	float *imc_mos_read( wx1, wx2, wy1, wy2 )
 *
 *   All other functions are system functions.
 *
 * ==================================================================================
 
 arranged by M.YAGI 95/4/10 -
       
       In original code only ONE mosaic data can be used at one time.
       It is inconvenient.       
       As *.mos files are managed through 'struct mos,'
       new functions always receives that struct and change that struct.


       But user is not permitted to access directly the struct.
       So make dictionary (imhp->fnam) <-> (mos).
       User callables are like this,

 	void   imc_mos_set_add ( imhp, mode )
 	float *imc_mos_read ( imhp, wx1, wx2, wy1, wy2 )
*/
 
/* ... system include files ... */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>

/* ... local include files ... */
#include "imc.h"
#include "stat.h"
#include "sort.h"

/* ... define paramaters ... */

#ifndef N_LINE_BUFFER
int N_L_BUF=200;
#else
int N_L_BUF=N_LINE_BUFFER;
#endif

#ifndef N_LINE_BUFFER_LARGE
int N_L_BUF_L=20;
#else
int N_L_BUF_L=N_LINE_BUFFER_LARGE;
#endif

const int N_BYTE_PIXEL=sizeof(float); /* # of bytes per pixel in buffer */

/* ...In-line functions ...*/

/* This is a function that transforms pixel ix iy
 * coordinates  into virtual xy coordinate */
#define TRANS_X( X, Y, XOFF, SIN, COS ) ( (XOFF) + (COS)*(X) - (SIN)*(Y) ) 
#define TRANS_Y( X, Y, YOFF, SIN, COS ) ( (YOFF) + (SIN)*(X) + (COS)*(Y) )

/* This is a function that transform virtual xy
 *  coordinates into pixel# in a frame */
#define INV_TRANS_X(VX,VY,VXOFF,VYOFF,SIN,COS) ( (COS)*((VX)-(VXOFF)) + (SIN)*((VY)-(VYOFF)) )
#define INV_TRANS_Y(VX,VY,VXOFF,VYOFF,SIN,COS) (-(SIN)*((VX)-(VXOFF)) + (COS)*((VY)-(VYOFF)) )

/* Nearest integer */

#define L_NINT( X )  (int)floor((X)+0.5)

/* Globals */

static MOSMODE default_add=NODIST_NOADD;
static int default_maxadd=0;
static int default_pixignr=INT_MIN;
static int const_add=0;


size_t check_size_t_max(void)
{
  size_t size_t_max;
  /* size_t max ; to be included in configure, in future*/

  if (sizeof(size_t)==sizeof(int))
    {
      size_t_max=INT_MAX;
    }
  else if (sizeof(size_t)==sizeof(short))
    {
      size_t_max=SHRT_MAX;
    }
  else if (sizeof(size_t)==sizeof(long))
    {
      size_t_max=LONG_MAX;
    }

  return size_t_max;
}



/* -------------------------------------------------------------- */
struct mos *imc_mos_get_mosp(struct imh *imhp)
/* --------------------------------------------------------------
 * 
 * Description:
 * ============
 * Now this routine keeps number of struct mos.
 * As imhp->id is not used in *.mos files,
 * this routine put unique number(n) for each imhp->ids at
 * first call and returns mos[n].
 *  
 * --------------------------------------------------------------
 */
{
  static struct mos **mos=NULL;
  static int nmos=0;
  int i;

  /* If imhp is NULL pointer,
     all areas are freed */
  if (imhp==NULL)
    {
      for(i=0;i<nmos;i++)
	{
	  free(mos[i]);
	}
      nmos=0;
      free(mos);
      mos=NULL;
      return NULL;
    }
  
  i=atoi(imhp->id);
  
  /* thus struct imh for mos should have unique id# (1,2,...)
     as STRING */
  
  if (i>0 && i<=nmos)
    {
      /* already allocated */
      return mos[i-1];
    }
  else
    {
      /* New area must be allocated */
      nmos++;
      sprintf(imhp->id,"%d",nmos);
      
      mos=(struct mos **)realloc(mos,sizeof(struct mos *)*(nmos));
      if(mos==NULL)
	{
	  fprintf(stderr,"Cannot allocate memory for mos in imc_mos_get_mosp.\n");
	  exit(-1);
	}

      mos[nmos-1]=(struct mos *)calloc(1,sizeof(struct mos));
      if(mos[nmos-1]==NULL)
	{
	  fprintf(stderr,"Cannot allocate memory for mos[%d] in imc_mos_get_mosp.\n",nmos-1);
	  exit(-1);
	}

      return mos[nmos-1];
    }
}


int imc_mos_select_frame2( struct mos *mosp,
			   int vx1,
			   int vx2,
			   int vy1,
			   int vy2)
{
  int ifrm;
  int nsel=0;

  for( ifrm=0; ifrm<mosp->nfrm; ifrm++ ) 
    {    
      /* Is this frame in the region */
      mosp->frm[ifrm].sel = 0;
      /*
      printf("frmsel:%f %f %f %f\n",mosp->frm[ifrm].vxmin,
      mosp->frm[ifrm].vxmax,
      mosp->frm[ifrm].vymin,
      mosp->frm[ifrm].vymax);
      */
      if( mosp->frm[ifrm].vxmax < vx1 ) continue;
      if( mosp->frm[ifrm].vxmin > vx2 ) continue;
      if( mosp->frm[ifrm].vymax < vy1 ) continue;
      if( mosp->frm[ifrm].vymin > vy2 ) continue;
      /* This frame is in the region */
      mosp->frm[ifrm].sel=1;
      nsel++;
    }
  return nsel;
}

/* -------------------------------------------------------------------------------- */
void imc_mos_select_frame( struct mos *mosp )
/* --------------------------------------------------------------------------------
 *
 * Description:
 * ============
 *  This function determines which mosaic frames are associated with
 *  given window in virtual space.
 *
 * Arguments:
 * ==========
 */
/*
 * --------------------------------------------------------------------------------
 */
{
#if 0
  int	ifrm;		/* loop index for frame# */
  /* loop over real frame */

  mosp->nsel=0;
  for( ifrm=0; ifrm<mosp->nfrm; ifrm++ ) {    
    /* Is this frame in the region */
    mosp->frm[ifrm].sel = 0;
    if( mosp->frm[ifrm].vxmax < mosp->wx1 ) continue;
    if( mosp->frm[ifrm].vxmin > mosp->wx2 ) continue;
    if( mosp->frm[ifrm].vymax < mosp->wy1 ) continue;
    if( mosp->frm[ifrm].vymin > mosp->wy2 ) continue;
    
    /* This frame is in the region */
    mosp->frm[ifrm].sel = 1;
    mosp->nsel++;
  }
#endif
  mosp->nsel=imc_mos_select_frame2(mosp,mosp->wx1,mosp->wx2,
				   mosp->wy1,mosp->wy2);
}

/********************************************************************************/
/********************************************************************************/


int iyrange2(float wy1, float wy2,
	     double cos_rot, double sin_rot, 
	     int npx, int npy,
	     float yoff, 
	     int *iymin, int *iymax)
{
  float y0,y1,y2,y3;

  float ymin,ymax;

  if(fabs(cos_rot)<1.0e-5)
    {
      *iymin=0;
      *iymax=npy-1;
      return 0;
    }

  y0=(wy1-yoff)/cos_rot;
  y1=(wy2-yoff)/cos_rot;
  y2=(wy1-npx*sin_rot-yoff)/cos_rot;
  y3=(wy2-npx*sin_rot-yoff)/cos_rot;

  /*
    printf("y0-3 %f %f %f %f\n",y0,y1,y2,y3);
  */

  ymin=y0;
  if(y1<ymin)ymin=y1;
  if(y2<ymin)ymin=y2;
  if(y3<ymin)ymin=y3;
  if(ymin<0) ymin=0;
 
  ymax=y0;
  if(y1>ymax)ymax=y1;
  if(y2>ymax)ymax=y2;
  if(y3>ymax)ymax=y3;
  if(ymax>npy-1)ymax=(float)(npy-1);
  
  /*
    printf("ymin=%f ymax=%f\n",ymin,ymax);
  */
  *iymin=(int)floor(ymin);
  *iymax=(int)ceil(ymax);

  if(*iymin<=*iymax) return 0;
  else return 1;
}

int iyrange(struct mos *mosp, int ifrm, 
	    double cos_rot, double sin_rot, 
	    float wy1, float wy2,
	    int *iymin, int *iymax)
{
  /* Determine iy range for this virtual space window */
  /*
   * equation of side of Frame
   *  x = x0 + slope * y   for left  side
   *  x = x1 + slope * y   for right side
   */
  float	rymin,rymax;		/* iy range */
  float	x0,x1,slope;		/* parameter for side of frame */
  float	xint_U, xint_L;		/* x of intersection of y=vy1, vy2 and frame side */

  printf("debug:UL %f %f \n",
	 mosp->frm[ifrm].vxUL,mosp->frm[ifrm].vyUL);
  printf("debug:UR %f %f \n",
	 mosp->frm[ifrm].vxUR,mosp->frm[ifrm].vyUR);
  printf("debug:LL %f %f \n",
	 mosp->frm[ifrm].vxLL,mosp->frm[ifrm].vyLL);
  printf("debug:LR %f %f \n",
	 mosp->frm[ifrm].vxLR,mosp->frm[ifrm].vyLR);


  slope = sin_rot / cos_rot;  
  if( slope > 0.0 ) 
    {
      x0= mosp->frm[ifrm].vxUL - slope * mosp->frm[ifrm].vyUL;
      x1= mosp->frm[ifrm].vxLR - slope * mosp->frm[ifrm].vyLR;
      xint_L = x1 + slope * wy1;
      xint_U = x0 + slope * wy2;
    }
  else	
    {
      x0= mosp->frm[ifrm].vxLL - slope * mosp->frm[ifrm].vyLL;
      x1= mosp->frm[ifrm].vxUR - slope * mosp->frm[ifrm].vyUR;
      xint_L = x0 + slope * wy1;
      xint_U = x1 + slope * wy2;
    }
  
  /* transfer vy1 and vy2 back to real iy */

  rymin=-sin_rot*(xint_L-mosp->frm[ifrm].xof)+cos_rot*(wy1-mosp->frm[ifrm].yof);
  rymax=-sin_rot*(xint_U-mosp->frm[ifrm].xof)+cos_rot*(wy2-mosp->frm[ifrm].yof);

  /* limit y range */
  *iymin= rymin - 2.0;
  *iymax= rymax + 2.0;
  if( *iymin < 0 ) *iymin = 0;
  if( *iymax >= mosp->frm[ifrm].npy ) *iymax = mosp->frm[ifrm].npy - 1;
  if(*iymin <= *iymax ) return 0;
  else return 1;
}


float getval_nearest(float fx,float fy, int x0, int x1, 
		     int y0, int y1, float *f, float pixignr, char *fmap)
{
  int ix,iy;
  int npx,id1;

  *fmap=0;
  ix=(int)floor(fx+0.5)-x0;
  iy=(int)floor(fy+0.5)-y0;
  if(ix>=x0 && ix<=x1 && iy>=y0 && iy<=y1)
    {
      npx=x1-x0+1;
      id1=ix+npx*iy;
      *fmap=1;
      return f[id1];
    }
  else
    return pixignr;
}			
 

float getval_interp(float fx,float fy, int x0, int x1, 
		    int y0, int y1, float *f, float pixignr, char *fmap)
{
  int ix,iy;
  int id1,id2,id3,id4;
  int npx;
  float v1,v2,v3,v4;
  float a1,a2,b1,b2;

  *fmap=0;

  if(fx>=x0 && fx<=x1 && fy>=y0 && fy<=y1)
    {
      npx=x1-x0+1;
      ix=(int)floor(fx)-x0;
      iy=(int)floor(fy)-y0;
      a1=fx-ix;
      a2=fy-iy-y0;
      id1=ix+npx*iy;
      id2=id1+1;
      id3=id2+npx;
      id4=id3-1;

      /*2002/07/20*/
      a1=fx-ix;
      a2=fy-iy-y0;
      if(a1==0){id2=id1;id3=id4;}
      if(a2==0){id4=id1;id3=id2;}

      v1=f[id1];
      v2=f[id2];
      v3=f[id3];
      v4=f[id4];
      
      if(v1!=pixignr && v2!=pixignr && v3!=pixignr && v4!=pixignr)
	{
	  /* all 4 pixels around reversed coordinate (float position) 
	     are valid */
	  /* biliniar interpolation! */
	  b1=(v2-v1)*a1+v1;
	  b2=(v3-v4)*a1+v4;
	  /* load data and mark fmap*/
	  *fmap=1;
	  return (b2-b1)*a2+b1;
	}
    }
  return pixignr;
}



/* -------------------------------------------------------------------------- */

void imc_mos_paste_nodist_noadd( 
				struct mos *mosp)

/* -------------------------------------------------------------------------- */
{
  struct  imh imh={""};
  struct  icom icom={0};
  int    ifrm;			/* loop index for virtual x */
  int	ix;		/* x loop index */
  int	iy;			/* loop index for real y */
  int i;

  int	iymin,iymax;		/* iy range */
  double   sin_rot, cos_rot;	/* sin cos of frame rotation */

  FILE	*fp;			/* file pntr for read image frame */

  float	*pix;		/* work area for one line of real frame */

  int	ivx, ivy;	/* virtual x y */
  int	npx_buff;	/* # of x pixs per line in buffer */


  float	*bp;		/* pntr to buffer */

  int wx1,wx2,wy1,wy2;
  int nfrm,npx,npy;
  size_t imgsize;

  float xoff,yoff;
  float pixignr;
  float fact=1.0;
  float xtrans,ytrans;

  /* ... Fill buffer with pixignr(default value) ... */


  imgsize=mosp->buffsize/N_BYTE_PIXEL;

  for(i=0;i<imgsize;i++)
    {
      mosp->vpixp[i]=(float)(mosp->pixignr);
    }

  wx1=mosp->wx1;
  wx2=mosp->wx2;
  wy1=mosp->wy1;
  wy2=mosp->wy2;
  nfrm=mosp->nfrm;
    
  npx_buff=(wx2-wx1)+1;

  /* loop over associated frame */
  for( ifrm=mosp->nfrm-1; ifrm>=0 ; ifrm-- ) 
    {
      /* to overwrite upper one */
      if(!mosp->frm[ifrm].sel)continue;

      /* Determine iy range for this virtual space window */
      cos_rot= mosp->frm[ifrm].cosrot;
      sin_rot= mosp->frm[ifrm].sinrot;
      
      /********* mosp => iymin  *********/
      printf("debug:wy1=%d wy2=%d\n",wy1,wy2);

      xoff=mosp->frm[ifrm].xof;
      yoff=mosp->frm[ifrm].yof;
      npx=mosp->frm[ifrm].npx;
      npy=mosp->frm[ifrm].npy;
      pixignr=(float)mosp->frm[ifrm].pixignr;



      if (iyrange2(wy1, wy2,cos_rot, sin_rot, 
		   npx, npy, yoff, &iymin,&iymax)!=0) continue;

      /*      
      if (iyrange(mosp,ifrm,cos_rot,sin_rot,
		  wy1,wy2,&iymin,&iymax)!=0) continue;
      */

      /* open iraf frame */
      fp = imropen( mosp->frm[ifrm].fnam, &imh, &icom );

      pix=(float*)malloc(npx*N_BYTE_PIXEL);

      fact=1.0/mosp->frm[ifrm].flux;
      
      /* debug */
      /*
	fprintf(stderr,"debug:factor\n");
	fprintf(stderr,"debug:factor=%f\n",fact);
      */
      printf("debug:xoff=%f yoff=%f\n",xoff,yoff);
      printf("debug:yrange:%d-%d\n",iymin,iymax);
      /* here, mapping */
      /* loop over iy of real frame */
      for( iy=iymin; iy<=iymax; iy++ ) 
	{
	  /* read one line */
	  if(imrl_tor(&imh,fp,pix,iy)==0) break;
	  	  
	  xtrans=xoff-sin_rot*(float)iy;
	  ytrans=yoff+cos_rot*(float)iy;
	  if(iy==iymin)
	    {
	      printf("xtrans:%f ytrans:%f\n",xtrans,ytrans);
	      printf("sin_rot:%f cos_rot:%f\n",sin_rot,cos_rot);
	    }
	  for( ix=0; ix<npx ; ix++ ) 
	    {
	      /* shift and  rotation */
	      /* integer value of virtual xy */
	      
	      /*
	      fprintf(stderr,"debug:x=%d y=%d\n",ix,iy);
	      */
	      if (pix[ix]!=pixignr)
		{
		  ivx= (int)floor(xtrans+cos_rot*(float)ix+0.5);
		  ivy= (int)floor(ytrans+sin_rot*(float)ix+0.5);
		  
		  /* check this is in selected range */
		  if(ivx<wx1||ivx>wx2||ivy<wy1||ivy>wy2) continue;
		  
		  /* get buffer pointer for this pixel */
		  /* Sep 7 */

		  /* load data */
		  /* Normalize data here for NODIST_NOADD */
		  bp=mosp->vpixp+npx_buff*(ivy-wy1)+(ivx-wx1);
		  if ((*bp)!=0) 
		    {
		      /************ OK ***********/
		      printf("ERRORRRRRR0\n");
		      exit(-1);
		    }
		  (*bp)=(float)(pix[ix]*fact);
		}
	    } /* end ix loop */
	}
      /* 1999/10/29 */
      /* (void) fclose( fp );*/
      imclose(fp,&imh, &icom );
      free(pix);
    }	
}



void imc_mos_paste_dist_large_main(struct mos *mosp,
				   float (*func)(int,float*,float*,float))
{
  struct  imh imh={""};
  struct  icom icom={0};
  size_t fmapsize;
  int npx_buff,npy_buff,npy_buff0;
  double cos_rot,sin_rot;

  float *pixp=NULL,*bufp=NULL;
  FILE *fp;

  float fx,fy;
  float fx0,fy0;

  float *bp=NULL;
  char *fmapp=NULL;

  int iy;
  int iymin,iymax;
  int ivx,ivy;
  int ivxoff;

  int ifrm;

  int k;

  float *frame=NULL,*framep=NULL;

  int nframes,ifrmsel,nfmax,nfmin;

  int nfrm,nfrmsel,npx,npy;
  int wx1,wx2,wy1,wy2;
  float xoff,yoff,pixignr,pixignr_out;
  int idx,idx0;

  float *buf=NULL;
  float *val=NULL,*weight=NULL;

  size_t buffsize;
  size_t imgsize;
  int nsplit=1;
  int *fidx;
  int splitmax;
  size_t size_t_max;

  wx1=mosp->wx1;
  wx2=mosp->wx2;
  wy1=mosp->wy1;
  wy2=mosp->wy2;
  nfrm=mosp->nfrm;

  /* calc nfrmsel */

  fidx=(int *)malloc(nfrm*sizeof(int));
  nfrmsel=0;
  for( ifrm=0 ; ifrm<nfrm ; ifrm++ ) 
    {
      if( ! mosp->frm[ifrm].sel ) continue;
      fidx[nfrmsel]=ifrm;
      nfrmsel++;
    }

  /* Clear vpix buffer */
  
  /* Allocate median buffer */

  buf=NULL;

  splitmax=16;
  npx_buff = wx2-wx1+1;
  npy_buff = wy2-wy1+1;

  size_t_max=check_size_t_max();

  /* testing */
  /*
    for (nsplit=4;nsplit<splitmax;nsplit*=2)
  */


  for (nsplit=1;nsplit<splitmax;nsplit*=2)
    {
      npy_buff0 = (wy2-wy1)/nsplit+1;
      imgsize=(npx_buff*npy_buff0);
      buffsize=imgsize*N_BYTE_PIXEL;
      if(nfrmsel<size_t_max/buffsize)
	{
	  /* buf=(float*)malloc(buffsize*(size_t)nfrmsel); */
	  /* 2005/01/19 */

	  buf=mosp->medbuf;
	  if(buf!=NULL) break; 
	}
    }

  if (nsplit>splitmax)
    {
      (void) printf("Imc_mos_fill_buffer: Can't Allocate Median Buffer. Stop.\n");
      exit(-1);
    } 

  /* test code */
  printf("debug:split=%d\n",nsplit);
  memset(buf,0,buffsize*(size_t)nfrmsel);
  {
    (void) printf(" %u bytes allocated for input buffer.\n",buffsize*(size_t)nfrmsel);
    (void) printf(" debug: size_t max limit is %u bytes\n",size_t_max);
    (void) printf(" ----------------------------------------------- \n\n");
  }

  printf("debug: %f\n",buf[imgsize*nfrmsel-1]);

  /* fmapp is u_int* but used as char* */

  /* 2005/01/19 already allocated */

  fmapsize = imgsize * nfrmsel * sizeof(char);
  /*
    mosp->fmapp = (u_int *)realloc( mosp->fmapp, fmapsize );
  */
  

  /* ... Allocate memory for Flux map ... */  
  if( mosp->fmapp == NULL ) 
    {
      (void) printf("Imc_mos_fill_buffer: Can't Allocate Flux-map Buffer\n");
      exit(-1);
    } 
  /*
    else if(  mosp->fmapp != old_ptr ) 
    {
    (void) printf(" %u bytes allocated for flux map\n",fmapsize);
    (void) printf(" ----------------------------------------------- \n\n");
    }
  */

  /* Main Loop */
  /* loop over associated frame */

  pixignr_out=(float)mosp->pixignr;  

  val=(float*)malloc(nfrmsel*sizeof(float));
  weight=(float*)malloc(nfrmsel*sizeof(float));

  for(k=0;k<nsplit;k++)
    {
      /* clear map */

      memset(mosp->fmapp,0,fmapsize); /* this function is quicker */

      memset(buf,0,buffsize*(size_t)nfrmsel);
      memset(val,0,nfrmsel*sizeof(float));
      memset(weight,0,nfrmsel*sizeof(float));

      wy1=mosp->wy1+npy_buff0*k;
      wy2=wy1+npy_buff0-1;

      printf("split: %d/%d %d-%d\n",k+1,nsplit,wy1,wy2);
      for( ifrmsel=0 ; ifrmsel<nfrmsel ; ifrmsel++ ) 
	{	  
	  ifrm=fidx[ifrmsel];
	  printf("%s\n",mosp->frm[ifrm].fnam);
	  
	  /* Determine iy range for this virtual space window */
	  cos_rot= mosp->frm[ifrm].cosrot;
	  sin_rot= mosp->frm[ifrm].sinrot;
	  
	  /********* mosp => iymin  *********/
	  xoff=mosp->frm[ifrm].xof;
	  yoff=mosp->frm[ifrm].yof;
	  npx=mosp->frm[ifrm].npx;
	  npy=mosp->frm[ifrm].npy;

	  pixignr=(float)mosp->frm[ifrm].pixignr;

	  if (iyrange2(wy1, wy2, cos_rot, sin_rot, 
		       npx, npy, yoff, &iymin, &iymax)!=0) 
	    continue;


	  /* open frame */
	  fp = imropen( mosp->frm[ifrm].fnam, &imh, &icom );
	  
	  /* read image into buffer */		  
	  /* 2005/01/19 */

	  frame=mosp->frmbuf;
	  memset(frame,0,npx*(iymax-iymin+1)*sizeof(float));


	  if (frame==NULL)
	    {
	      fprintf(stderr,"Cannot allocate memory for frame in imc_mos_paste_dist_med_main!\n");
	      fprintf(stderr,"size=%d\n",npx*(iymax-iymin+1)*sizeof(float));
	      exit(-1);
	    }
	  
	  /* read area */
	  for( iy=iymin; iy<=iymax; iy++ ) 
	    {
	      framep=frame+npx*(iy-iymin);
	      if( imrl_tor( &imh, fp, framep,iy ) == 0 ) 
		{
		  /*ERRROR*/
		  fprintf(stderr,"Cannot read frame at y=%d\n",iy);
		  exit(-1);
		  /* or, ignore */ 
		  /* break; */
		}
	    }
  	  
	  /* Loop over virtual frame y,x */
	  
	  for(ivy=wy1;ivy<=wy2;ivy++)
	    {
	      idx0=npx_buff*(ivy-wy1)*nfrmsel+ifrmsel;

	      /*
		printf("debug: idx0 %d %d %d %d %d %d\n",idx0,
		npx_buff,ivy,wy1,nfrmsel,ifrmsel);
	      */

	      fx0=sin_rot*(ivy-yoff);
	      fy0=cos_rot*(ivy-yoff);


	      for(ivx=wx1;ivx<=wx2;ivx++)	
		{
		  /* get buffer pointer for this pixel */
		  idx=idx0+(ivx-wx1)*nfrmsel;

		  bp=buf+idx;
		  fmapp=(char*)mosp->fmapp+idx;

		  /* reverse mapping virtual coordinate to real frame */
		  fx=cos_rot*(ivx-xoff)+fx0;
		  fy=-sin_rot*(ivx-xoff)+fy0;
		  
		  /* if bp is not pixignr (i.e. valid value)
		     fmapp is marked */
		  if ((*bp)!=0) 
		    {
		      /************ OK ***********/
		      printf("ERRORRRRRR 00 %f %f %f\n",*bp,fx,fy);
		      printf("ERRORRRRRR 00 ivx:%d ivy:%d\n",ivx,ivy);
		      printf("ERRORRRRRR 00 wx1:%d wy1:%d\n",wx1,wy1);

		      exit(-1);
		    }

		  *bp=getval_interp(fx,fy,0,npx-1,iymin,iymax,
				    frame,pixignr,fmapp);

		}
	    } /* end vx vy loop */	
	  
	  /* 1999/10/29 */
	  imclose(fp,&imh,&icom);
	  
	  /* 2005/01/19 */
	  
	}
      
      /* Take weighted median */
      /* or something */
      
      
      nfmax=mosp->maxadd;
      if (nfmax==0)
	nfmax=nfrmsel;

      if (const_add==0)
	nfmin=1;
      else
	nfmin=nfmax;

      for(ivy= wy1; ivy <= wy2; ivy++) 
	{
	  printf("y=%d\n",ivy);
	  ivxoff=npx_buff*(ivy-wy1);

	  for(ivx=wx1;ivx<=wx2;ivx++) 
	    {
	      /* pntr to pixel data */
	      idx= (ivxoff+ivx-wx1)*nfrmsel;
	      pixp  = mosp->vpixp + (ivx-wx1) + npx_buff*(ivy-mosp->wy1);
	      bufp  = buf + idx;
	      fmapp = (char*)mosp->fmapp + idx;
	      
	      /* Find contributed frame */
	      nframes=0;
	      for(ifrmsel=0;ifrmsel<nfrmsel;ifrmsel++)
		{	      
		  ifrm=fidx[ifrmsel];
		  if(fmapp[ifrmsel]==1)
		    {
		      val[nframes]=bufp[ifrmsel];
		      weight[nframes]=mosp->frm[ifrm].flux;
		      if ((val[nframes]!=pixignr_out)&&(weight[nframes]>0))
			nframes++;
		    }
		}
	      /* take weighted median */
	      if(nframes>=nfmin)
		{
		  if(nfmax<nframes)
		    *pixp=func(nfmax,val,weight,pixignr_out);
		  else
		    *pixp=func(nframes,val,weight,pixignr_out);
		}
	      else
		{
		  *pixp=pixignr_out;
		}
	    } /* end ivx */
	} /* end ivy */
    }
  free(val);
  free(weight);
  /* 2005/01/19 */
  /* free(buf); */
}


/*************** coadding funcs **********************/
float addflux(int ndat, float *dat, float *weight, float errval)
{
  int i;
  double w=0;
  
  for(i=0;i<ndat;i++)
    {
      w+=weight[i];
    }
  return w;
}

float meanflux(int ndat, float *dat, float *weight, float errval)
{
  int i;
  double s=0,w=0;
  
  for(i=0;i<ndat;i++)
    {
      s+=dat[i];
      w+=weight[i];
    }
  if(w!=0) return (float)(s/w);
  else return errval;
}

float medianflux(int ndat, float *dat, float *weight, float errval)
{
  int i;
  for(i=0;i<ndat;i++) 
    {
      dat[i]/=weight[i];
    }
  return floatweightedmedian(ndat,dat,weight);
}

/* member */
static float clipfactor=3.0;
static float clipfrac=0.16;
static int clip_min_n=3;
static int clipmean_min_n=1;

int imc_mos_set_clip_param(float k, float f, int n, int m)
{
  if(k>0 && f>0 && f<1 && m<=n)
    {
      clipfactor=k;
      clipfrac=f;
      clip_min_n=n;
      clipmean_min_n=m;
      return 0;
    }
  else 
    return -1;
}

float clipmean(int ndat, float *dat, float *weight, float errval)
{
  int i;
  double s=0,w=0;
  float *a;
  float med,hex,sigma;
  float v,vmin=-FLT_MAX,vmax=FLT_MAX;

  if(ndat<=clipmean_min_n)
    {
      for(i=0;i<ndat;i++) 
	{
	  dat[i]/=weight[i];
	}
      return floatweightedmedian(ndat,dat,weight);
    }

  a=malloc(ndat*sizeof(float));

  if(ndat>=clip_min_n)
    {
      for(i=0;i<ndat;i++)
	{
	  a[i]=dat[i]/weight[i];
	}
      med=floatmedian(ndat,a);
      hex=nth(ndat,a,floor((float)(ndat-1)*clipfrac));
      sigma=med-hex;
      /* a is broken by nth */
      vmin=med-clipfactor*sigma;
      vmax=med+clipfactor*sigma;
    }
  for(i=0;i<ndat;i++)
    {
      v=dat[i]/weight[i];
      if(v>vmin&&v<vmax)
	{
	  s+=dat[i];
	  w+=weight[i];
	}
    }
  free(a);

  if(w!=0) return (float)(s/w);
  else return errval;
}

float outlyer(int ndat, float *dat, float *weight,float errval)
{

  float peak,median;
  int i;

  if(ndat<3) return errval;

  for(i=0;i<ndat;i++)
    dat[i]/=weight[i];
  peak=floatmax(ndat,dat);
  median=floatmedian(ndat,dat);
  
  return (peak-median);
}

































































/* -------------------------------------------------------------------------- */
float *imc_mos_fill_buffer(
     struct mos *mosp, /* pntr to mosaic struct */
     int vx1,	/* smallest virtual x */
     int vx2,	/* largest  virtual x */
     int vy1,	/* smallest virtual y */
     int vy2	/* largest  virtual y */)
{
  int  buffsize ;	/* total buffer size in byte */
  float pixignr;
  int i,nmax;
  float *v;


  /* ... check if mosaic file is open ... */
  
  if( mosp->nfrm <= 0 ) 
    {
      (void) printf("Imc_mos_fill_buffer: nfrm is zero\n");
      return NULL ;
    }

  if( vx1 > vx2 || vy1 > vy2 ) 
    {
      (void) printf("Imc_mos_fill_buffer: Invalid input parametes (%d %d)-(%d %d)\n",
		    vx1,vy1,vx2,vy2);
      return NULL ;
    }

  /* ... Allocate memory space ... */
  
  /* calculate necessary buffer size */
  /* typical size for 16000(npx) x 100(nline) => 6.4M bytes */
  
  buffsize= (vx2-vx1+1) * (vy2-vy1+1) * N_BYTE_PIXEL;

  if(buffsize>mosp->buffsize) 
    {      
      if(mosp->buffsize==0) 
	{
	  /* new buffer should be created */
	  /* 2005/01/19 */
	  free(mosp->vpixp);
	  if( (mosp->vpixp = (float *)malloc(buffsize)) != NULL ) 
	    {
	      (void) printf("\n ------------- Imc_Mos_Fill_Buffer ------------- \n");
	      (void) printf(" %d bytes allocated for",buffsize);
	      (void) printf(" %d x %d pixels of output buffer\n",vx2-vx1+1,vy2-vy1+1);
	      (void) printf(  " ----------------------------------------------- \n\n");
	    }
	  /*
	  else
	    {
	      mosp->buffsize==0;
	    }
	  */
	}      
      else 
	{
	  /* recreate buffer */

	  /* 2005/01/19 */
	  /* already allocated */
	  /* error return */
	  printf("Error: in allocating vpixp\n");
	  exit(-1);
#if 0
	  if( (mosp->vpixp = (float *)realloc(mosp->vpixp,buffsize)) != NULL ) 
	    {
	      (void) printf("\n ------------- Imc_Mos_Fill_Buffer ------------- \n");
	      (void) printf(" %d bytes Re-allocated for",buffsize);
	      (void) printf(" %d x %d pixels\n",vx2-vx1+1,vy2-vy1+1);
	      (void) printf(  " ----------------------------------------------- \n\n");
	    }
	  /*
	  else
	    {
	      mosp->buffsize==0;
	    }
	  */
#endif
	}
    }
  
  /* check buffer pntr */
  if( mosp->vpixp == NULL ) 
    {
      mosp->buff_fill = 0;
      (void) printf("Imc_mos_fill_buffer: Can't Allocate Buffer\n");
      (void) printf(" %d x %d pixels\n",vx2-vx1+1,vy2-vy1+1);
      
      return NULL;
    }
  else 
    {
      mosp->buffsize= buffsize;
      mosp->buff_fill = 1;
      mosp->iy0       = vy1;

    }

  /* ... find actual frame associate with this virtual xy region ... */
   
  mosp->wx1= vx1;
  mosp->wx2= vx2;
  mosp->wy1= vy1;
  mosp->wy2= vy2;

  /*
    (void) imc_mos_select_frame(mosp);
  */
  mosp->nsel=imc_mos_select_frame2(mosp,vx1,vx2,vy1,vy2);

  if( mosp->nsel == 0 ) 
    {
      (void) printf("Imc_mos_fill_buffer:No real frames for vx=%d-%d, vy=%d-%d\n"
		    ,vx1,vx2,vy1,vy2);
      /*2004/12/14*/
      /* fill pixignr */
      nmax=(vx2-vx1+1) * (vy2-vy1+1);
      pixignr=(float)(mosp->pixignr);
      v=mosp->vpixp;
      for(i=0;i<nmax;i++)
	v[i]=pixignr;

      return mosp->vpixp ;
    }


  switch( mosp->mix ) 
    {
    case NODIST_NOADD:
      imc_mos_paste_nodist_noadd(mosp);
      break;
      /*
	case DIST_NOADD:
	imc_mos_paste_dist_noadd(mosp);
	break;
      */

    case CONST_ADD:
      const_add=1;
      /* and go to next block */
    case DIST_ADD:
      imc_mos_paste_dist_large_main(mosp,meanflux);
      break;

    case DIST_MED:
      /*    case CONST_MED: not implemented yet */
      imc_mos_paste_dist_large_main(mosp,medianflux);
      break;

    case DIST_CLIP_ADD:      
      imc_mos_paste_dist_large_main(mosp,clipmean);
      break;

    case DIST_FLUX:
      imc_mos_paste_dist_large_main(mosp,addflux);
      break;

    case DIST_PEAK:
      imc_mos_paste_dist_large_main(mosp,outlyer);
      break;

    default:
      break;
    }
      
  return mosp->vpixp ;
}

int imc_mos_check_fill_buffer(struct imh *imhp,
			      int vxmin, int vxmax,
			      int vymin, int vymax,
			      int nline)
{
  int vy1,vy2,vx1,vx2;
  size_t buffsize;
  size_t frmbufsize;
  int nfrm,nfrmmax=0;
  size_t nfrmlimit;
  size_t size_t_max;
  size_t npx_buff,npy_buff,imgsize;

  struct mos *mosp;
  int retval=0;

  int ifrm;

  if((mosp = imc_mos_get_mosp(imhp))==NULL)
    return -1;

  if(vxmin>vxmax)
    {
      return -1;
    }
  vx1=vxmin;
  vx2=vxmax;

  size_t_max=check_size_t_max();

  npx_buff=(vx2-vx1)+1;
  npy_buff=nline+1;
  imgsize=(npx_buff*npy_buff);
  buffsize=imgsize*N_BYTE_PIXEL;
  nfrmlimit=size_t_max/buffsize+1;
  printf("\ndebug: Checking number of frames...\n");
  for(vy1=vymin;vy1<vymax;vy1+=nline)
    {
      vy2=vy1+nline-1;
      if(vy2>vymax) vy2=vymax;
      nfrm=imc_mos_select_frame2(mosp,vx1,vx2,vy1,vy2);
      if (nfrm>nfrmmax) nfrmmax=nfrm;
      printf("debug: %d-%d: %d\n",vy1,vy2,nfrm);
    }

  printf("debug: nfrmlimit=%d nfrmmax=%d\n\n",nfrmlimit,nfrmmax);
      
  if(nfrmmax+1>nfrmlimit)
    {
      printf("Error: limit exceeded.\n");
      nline=(int)floor((double)size_t_max/
		       (double)(nfrmmax+1)/(double)N_BYTE_PIXEL/
		       (double)npx_buff);
      
      printf("       Recommended nlines < %u (safest estimate)\n",
	     (int)floor((double)size_t_max/
			(double)(nfrmmax+1)/(double)N_BYTE_PIXEL/
			(double)npx_buff));
      retval=-1;
      /*
	printf("size_t %u\n",size_t_max);
	printf("nfrm %u\n",nfrmmax+1);
	printf("npx %u\n",npx_buff);
	printf("npy %u\n",npy_buff);
	printf("byte %u\n",N_BYTE_PIXEL);
      */
    }

  /* recalc */
  npy_buff=nline+1;
  imgsize=(npx_buff*npy_buff);
  buffsize=imgsize*N_BYTE_PIXEL;


  /* check frame npx, npy */
  frmbufsize=0;
  for(ifrm=0;ifrm<mosp->nfrm; ifrm++)
    {
      if (frmbufsize<mosp->frm[ifrm].npx*mosp->frm[ifrm].npy)
	frmbufsize=mosp->frm[ifrm].npx*mosp->frm[ifrm].npy;
    }
  /* 2005/05/05 */
  frmbufsize*=N_BYTE_PIXEL;

  /* malloc check */
  /* and alloc in *.mos 2005/01/19 */
  mosp->medbufsiz=buffsize*nfrmmax;
  mosp->fmappsiz=imgsize*nfrmmax;
  mosp->frmbufsiz=frmbufsize;

  printf("debug: imgsize:%d buffsize:%d fmappsiz:%d medbufsiz:%d\n",imgsize,buffsize,mosp->fmappsiz,mosp->medbufsiz);

  mosp->medbuf=(float*)malloc(buffsize*nfrmmax); /* buf(median) */
  mosp->vpixp=(float*)malloc(buffsize); /* buf(median) */
  mosp->frmbuf=(float*)malloc(frmbufsize); /* frame */
  mosp->fmapp=(u_int*)malloc(imgsize*nfrmmax);  /* fmap */


  if(mosp->fmapp==NULL)
    {
      printf("Error: Cannot allocate memory.\n");
      if (retval!=0)
	printf("nline=%d is still too large for memory allocation.\n",
	       nline);
      printf("Set nlines smaller\n");
      exit(-1);
    }

  return retval;
}

/* -------------------------------------------------------------------------- */
int imrl_tor_mos( 
     struct imh *imhp,       /* (in) ptr to imh struct */
     float      *dp,         /* (out) ptr to pixel array to be filled */
     int       iy)          /* (in) Line # start with 0 */
{
  /* char s[BUFSIZ];   *//* mosaic file name in imh header */
  float *vpix;	/* pntr to buffer (virtual pixel */
  int maxy;	/* max virtual y in buffer */
  struct mos *mosp;	/* pntr to mosaic struct */

  int nline = N_LINE_BUFFER;

  /* ... get mosaic pntr ... */
  mosp= imc_mos_get_mosp(imhp);
  if(mosp==NULL) return -1;
  if( mosp->nfrm <= 0 ) 
    {
      (void) printf("Imrl_tor_mos: nfrm is zero\n");
      return -1;
    }
  
  /* ... fill pixel buffer if needed ... */
  
  /*
  switch(mosp->mix)
    {
    case CONST_ADD:
    case DIST_ADD:
    case DIST_MED:
      nline=N_L_BUF_L;
      break;
    default:
      nline=N_L_BUF;
      break;
    }
  */
  nline=N_L_BUF;

  maxy= mosp->iy0 + nline -1 ;  
  if( mosp->buff_fill == 0 || iy < mosp->iy0 || iy > maxy ) 
    {
      /* buffer has to be filled */
      /* nline should be > 1 */
      maxy= iy + nline - 1;

      printf("y: %d %d\n",iy,maxy);
      if( imc_mos_fill_buffer(mosp, (int)0, (int)(mosp->npx-1), iy, maxy) == NULL ) 
	{
	  /* 2002/02/05 */
	  printf("Error\n");
	  return 0 ;
	}

    }
  
  /* ... copy pixel data to user array ... */
  vpix  = mosp->vpixp + mosp->npx * (iy - mosp->iy0); 
  /*
  printf("debug:read %d\n",iy);
  */

  memcpy(dp,vpix,sizeof(float)*mosp->npx); 
  /*
  printf("debug:  %f\n",dp[0]);
  */

  /* ... Free memory if this is the last line ... */
  if( iy == (mosp->npy - 1) ) (void) imc_mos_free_buffer(mosp);

  return 1 ;
}


/* ------------------------------------------------------------------------- */
int imrall_tor_mos(
/* -------------------------------------------------------------------------
 *
 * Description:
 * ============
 *   This function reads all mosaic file data into a given array.
 *   This function calls imrl_tor_mos() that calls imc_mos_fill_buffer.
 *   Besides the memory space for the array you define, the sapce required 
 *   for this operation is about [the x-size of virtual frame] * N_LINE_BUFFER *
 *   sizeof( float ).
 *
 * Note:
 * =====
 *   Not recommended for large mosaic files unless you have a huge
 *   PHYISICAL memory.
 *
 * Arguments:
 * ==========
 */
        struct imh *imhp,
        float      *pix,
        int       pdim)
/*
 * Revision:
 * =========
 *   93/02/15 --- original creation (M. Sekiguchi)
 *
 * ---------------------------------------------------------------------------
 */
{
  int ivy; 	/* virtual y index */
  float *dp;	/* pntr to float */
  
  /* ... check dimension ... */
  
  if( pdim < (imhp->npy * imhp->npx) ) 
    {
      (void) printf("imrall_tor_mos: given array size too small\n");
      return (int)0 ;
    }
  
  /* ... Loop over virtual y */
  
  for( ivy=0; ivy<imhp->npy; ivy++ ) 
    {      
      /* locate destination pntr */
      dp = pix + (size_t)(ivy * imhp->npx);
      
      /* fill one line */
      (void) imrl_tor_mos( imhp, dp, ivy );
    }  
  return 1 ;
}

int imc_mos_get_shift_old(struct imh *imhp,float *vxmin,float *vxmax, float *vymin, float *vymax)
{
  struct mos *mosp;	/* pntr to mosaic struct */
  int  ifrm;
  float fvxmin, fvxmax, fvymin, fvymax;

  double sin_rot, cos_rot;
  float x_LL;	/* frame x lower left */
  float x_LR;	/* frame x lower right */
  float x_UL;	/* frame x upper left */
  float x_UR;	/* frame x upper right */
  float y_LL;	/* frame y lower left */
  float y_LR;	/* frame y lower right */
  float y_UL;	/* frame y upper left */
  float y_UR;	/* frame y upper right */

  /* ... determine max/min virtual x/y */
  
  *vxmin = 0.0;
  *vxmax = 0.0;
  *vymin = 0.0;
  *vymax = 0.0;

  mosp= imc_mos_get_mosp(imhp);
  if(mosp==NULL) return -1;
    
  
  for( ifrm=0; ifrm < mosp->nfrm; ifrm++ ) 
    {
    /* we assume the frame rotation is small */
      sin_rot= mosp->frm[ifrm].sinrot = sin( mosp->frm[ifrm].rot );
      cos_rot= mosp->frm[ifrm].cosrot = cos( mosp->frm[ifrm].rot );
      
      /* calculate frame x/y of four corner */
      x_LL = 0.0;
      y_LL = 0.0;	
      x_LR = x_LL + mosp->frm[ifrm].npx - 1;		
      y_LR = y_LL;
      
      x_UL = x_LL;		
      y_UL = y_LL + mosp->frm[ifrm].npy - 1;
      x_UR = x_LR;		
      y_UR = y_UL;
      
      
      /* virtual x/y of four corner */
      mosp->frm[ifrm].vxLL= TRANS_X(x_LL,y_LL,mosp->frm[ifrm].xof,sin_rot,cos_rot);
      mosp->frm[ifrm].vxLR= TRANS_X(x_LR,y_LR,mosp->frm[ifrm].xof,sin_rot,cos_rot);
      mosp->frm[ifrm].vxUL= TRANS_X(x_UL,y_UL,mosp->frm[ifrm].xof,sin_rot,cos_rot);
      mosp->frm[ifrm].vxUR= TRANS_X(x_UR,y_UR,mosp->frm[ifrm].xof,sin_rot,cos_rot);
      
      mosp->frm[ifrm].vyLL= TRANS_Y(x_LL,y_LL,mosp->frm[ifrm].yof,sin_rot,cos_rot);
      mosp->frm[ifrm].vyLR= TRANS_Y(x_LR,y_LR,mosp->frm[ifrm].yof,sin_rot,cos_rot);
      mosp->frm[ifrm].vyUL= TRANS_Y(x_UL,y_UL,mosp->frm[ifrm].yof,sin_rot,cos_rot);
      mosp->frm[ifrm].vyUR= TRANS_Y(x_UR,y_UR,mosp->frm[ifrm].yof,sin_rot,cos_rot);

#if 0      
      if( mosp->frm[ifrm].rot > 0.0 ) 
	{     
	  /* virtual x max is lower right corner */
	  /* virtual y max is upper right corner */
	  fvxmax= mosp->frm[ifrm].vxLR;
	  fvymax= mosp->frm[ifrm].vyUR;
	  
	  /* virtual x min is upper left corner */
	  /* virtual y min is lower left corner */
	  fvxmin= mosp->frm[ifrm].vxUL;
	  fvymin= mosp->frm[ifrm].vyLL;
	}   
      else 
	{
	  
	  /* virtual x max is upper right corner */
	  /* virtual y max is upper left corner */
	  fvxmax= mosp->frm[ifrm].vxUR;
	  fvymax= mosp->frm[ifrm].vyUL;
	  
	  /* virtual x min is lower left corner */
	  /* virtual y min is lower right corner */
	  fvxmin= mosp->frm[ifrm].vxLL;
	  fvymin= mosp->frm[ifrm].vyLR;     
	}
#endif
      fvxmax=mosp->frm[ifrm].vxLL;
      fvxmin=mosp->frm[ifrm].vxLL;
      if (fvxmax<mosp->frm[ifrm].vxLR) fvxmax=mosp->frm[ifrm].vxLR;
      else if (fvxmin>mosp->frm[ifrm].vxLR) fvxmin=mosp->frm[ifrm].vxLR;
      if (fvxmax<mosp->frm[ifrm].vxLR) fvxmax=mosp->frm[ifrm].vxLR;
      else if (fvxmin>mosp->frm[ifrm].vxUL) fvxmin=mosp->frm[ifrm].vxUL;
      if (fvxmax<mosp->frm[ifrm].vxLR) fvxmax=mosp->frm[ifrm].vxLR;
      else if (fvxmin>mosp->frm[ifrm].vxUR) fvxmin=mosp->frm[ifrm].vxUR;

      fvymax=mosp->frm[ifrm].vyLL;
      fvymin=mosp->frm[ifrm].vyLL;
      if (fvymax<mosp->frm[ifrm].vyLR) fvymax=mosp->frm[ifrm].vyLR;
      else if (fvymin>mosp->frm[ifrm].vyLR) fvymin=mosp->frm[ifrm].vyLR;
      if (fvymax<mosp->frm[ifrm].vyLR) fvymax=mosp->frm[ifrm].vyLR;
      else if (fvymin>mosp->frm[ifrm].vyUL) fvymin=mosp->frm[ifrm].vyUL;
      if (fvymax<mosp->frm[ifrm].vyLR) fvymax=mosp->frm[ifrm].vyLR;
      else if (fvymin>mosp->frm[ifrm].vyUR) fvymin=mosp->frm[ifrm].vyUR;
      
      mosp->frm[ifrm].vxmin = fvxmin;
      mosp->frm[ifrm].vxmax = fvxmax;
      mosp->frm[ifrm].vymin = fvymin;
      mosp->frm[ifrm].vymax = fvymax;
      
      if( fvxmax > *vxmax ) *vxmax = fvxmax;
      if( fvymax > *vymax ) *vymax = fvymax;
      if( fvxmin < *vxmin ) *vxmin = fvxmin;
      if( fvymin < *vymin ) *vymin = fvymin;
    }
  return 0;
}


int imc_mos_get_shift(struct imh *imhp,float *vxmin,float *vxmax, float *vymin, float *vymax)
{
  struct mos *mosp;	/* pntr to mosaic struct */
  mos_frm *frmp;
  int  ifrm;
  float fvxmin, fvxmax, fvymin, fvymax;

  double sin_rot, cos_rot;

  float x[4],vx[4];	/* frame, virtual x */
  float y[4],vy[4];	/* frame, virtual y */
  float xoff,yoff;
  int i;

  /* ... determine max/min virtual x/y */
  
  *vxmin = 0.0;
  *vxmax = 0.0;
  *vymin = 0.0;
  *vymax = 0.0;

  mosp= imc_mos_get_mosp(imhp);
  if(mosp==NULL) return -1;
    
  for( ifrm=0; ifrm < mosp->nfrm; ifrm++ ) 
    {
    /* we assume the frame rotation is small */

      frmp=&(mosp->frm[ifrm]);
      sin_rot= frmp->sinrot = sin( frmp->rot );
      cos_rot= frmp->cosrot = cos( frmp->rot );

      xoff=frmp->xof;
      yoff=frmp->yof;

      /*
	printf("debug: %s r=%f cos=%f sin=%f xof=%f yof=%f\n",
	mosp->frm[ifrm].fnam,
	frmp->rot,cos_rot,sin_rot,xoff,yoff);
      */

      /* calculate frame x/y of four corner */
      x[0]=x[3]=0.0;
      x[1]=x[2]=frmp->npx - 1;
      y[0]=y[1]=0.0;
      y[2]=y[3]=frmp->npy - 1;
      
      /* virtual x/y of four corners */
      for(i=0;i<4;i++)
	{
	  vx[i]= TRANS_X(x[i],y[i],xoff,sin_rot,cos_rot);
	  vy[i]= TRANS_Y(x[i],y[i],yoff,sin_rot,cos_rot);
	}

      frmp->vxLL=vx[0];
      frmp->vyLL=vy[0];

      frmp->vxLR=vx[1]; 
      frmp->vyLR=vy[1];

      frmp->vxUR=vx[2];
      frmp->vyUR=vy[2];

      frmp->vxUL=vx[3];
      frmp->vyUL=vy[3];

      fvxmin=fvxmax=vx[0];
      fvymin=fvymax=vy[0];
      for(i=1;i<4;i++)
	{
	  if(vx[i]<fvxmin)fvxmin=vx[i];
	  else if(vx[i]>fvxmax)fvxmax=vx[i];
	  if(vy[i]<fvymin)fvymin=vy[i];
	  else if(vy[i]>fvymax)fvymax=vy[i];
	}

      frmp->vxmin = fvxmin;
      frmp->vxmax = fvxmax;
      frmp->vymin = fvymin;
      frmp->vymax = fvymax;


      /*
	printf("debug: %s %f,%f - %f,%f\n",
	mosp->frm[ifrm].fnam,	     
	fvxmin,fvymin, fvxmax,fvymax);
      */

      if( fvxmax > *vxmax ) *vxmax = fvxmax;
      if( fvymax > *vymax ) *vymax = fvymax;
      if( fvxmin < *vxmin ) *vxmin = fvxmin;
      if( fvymin < *vymin ) *vymin = fvymin;
    }
  return 0;
}

int imc_mos_set_shift(struct imh *imhp,float x,float y)
{
  struct mos *mosp;	/* pntr to mosaic struct */
  float virx_off,viry_off;
  int ifrm;

  mosp= imc_mos_get_mosp(imhp);
  if(mosp==NULL) return -1;

  virx_off= mosp->vxoff-x; /* always absolute value from the original *.mos */
  viry_off= mosp->vyoff-y;
  
  /* subtract offset from all virtual xy */
  for( ifrm=0; ifrm < mosp->nfrm; ifrm++ ) 
    {
      mosp->frm[ifrm].vxLL  -= virx_off;
      mosp->frm[ifrm].vxLR  -= virx_off;
      mosp->frm[ifrm].vxUL  -= virx_off;
      mosp->frm[ifrm].vxUR  -= virx_off;
      
      mosp->frm[ifrm].vyLL  -= viry_off;
      mosp->frm[ifrm].vyLR  -= viry_off;
      mosp->frm[ifrm].vyUL  -= viry_off;
      mosp->frm[ifrm].vyUR  -= viry_off;
      
      mosp->frm[ifrm].xof   -= virx_off;
      mosp->frm[ifrm].yof   -= viry_off;
      
      mosp->frm[ifrm].vxmin -= virx_off;
      mosp->frm[ifrm].vxmax -= virx_off;
      
      mosp->frm[ifrm].vymin -= viry_off;
      mosp->frm[ifrm].vymax -= viry_off;
    }

  mosp->vxoff= x;
  mosp->vyoff= y;

  /* npx/npy change */
  mosp->npx-=ceil(virx_off);
  mosp->npy-=ceil(viry_off);
  imhp->npx=imhp->ndatx=mosp->npx;
  imhp->npy=imhp->ndaty=mosp->npy;

  /*
    printf("npx:%d npy:%d\n",mosp->npx,mosp->npy);
  */
  return 0;
}

/* ----------------------------------------------------------------------- */
void  imc_mos_free_buffer(struct mos *mosp )
/* -----------------------------------------------------------------------
 *
 * Description:
 * ============
 *  This function frees the buffer sapce.
 *
 * -----------------------------------------------------------------------
 */
{
  if( mosp->vpixp != NULL ) 
    {
      
      (void) free( mosp->vpixp );
      mosp->vpixp = NULL;
      mosp->buffsize = 0;
      /*
	(void) printf("\n Imrl_tor_mos: Memory freed\n\n");
	*/
    }
  
  if( mosp->cent_sfrm != NULL ) 
    {
      (void) free( mosp->cent_sfrm );
      mosp->cent_sfrm = NULL;
    }
  if( mosp->fmapp != NULL ) 
    {
      (void) free( mosp->fmapp );
      mosp->fmapp = NULL;
    }
  return;
}

/* -------------------------------------------------------------------------- */
void imc_mos_get_vlimit(
/* --------------------------------------------------------------------------
 * 
 * Description: Imc Mosaic Get Virtual xy Limits
 * ============
 * This function calculates virtual coordinates of four corners of
 * real image frame. The results are stored in "mos" structure.
 * The output aurguments returns the max/min value of virtual coordinates
 * for this mosaic frame. (Also, the equations of four sides of each frame
 * in the virtual space are calculated for later use.)
 *
 * After calculating all such parameters, this function re-offsets all
 * virtual xy value so that the smallest virtual xy is zero.
 *
 * Arguments:
 * ==========
 */
			/*	
				struct mos *mosp,
			*/
			/* struct that stores mosaic parameters */
        struct imh  *imhp,      /* (in) ptr to imh header structure */
	float *vxmin,		/* minmum virtual x after re-offset, ie =0 */
	float *vxmax,		/* maximum virtual x after re-offset  */
	float *vymin,		/* minmum virtual y after re-offset, ie =0 */
	float *vymax)		/* maximum virtual x after re-offset  */
/*
 * Revision:
 * =========
 * 1993 Feb - original creation (M. Sekiguchi)
 * 2001 revised by Yagi
 *
 * ----------------------------------------------------------------------------
 */
{
  float x,y;
  imc_mos_get_shift(imhp,vxmin,vymin,vxmax,vymax);
  x=-(*vxmin);
  y=-(*vymin);
  imc_mos_set_shift(imhp,x,y);
  return;
}

/* ----------------------------------------------------------------------------------- */
FILE *imropen_mos(
/* -----------------------------------------------------------------------------------
 *
 * Description:  Imc Read Open for Mosaic
 * ============
 * This function is called by imropen if the file name is "*.mos". This function 
 * reads a *.mos file and load mosaic parameters. Imropen() is called for each image
 * frame to get parameters of each files such as x-size and y-size. Such parameters
 * are stored in a structure called "mos", which is defined in imc.h.
 *
 * Imropen_mos calls imc_mos_get_vlimit() to find out the size of entire virtual
 * mosaic frame. All virtual xy parametes are re-calculated so that the minimum
 * xy begin with 0.
 *
 * Most of contents of imh and icom are copied from the last opened image
 * file. But parameters such as x-size and y-size are overwitten and they 
 * indicate the size of entire virtual frame. For a user, imh and comp look like
 * those for the virtual frame, not for eash small frame.
 * 
 * The retrun value, unlike imropn, is a pointer to *.mos file. 
 *
 * Note:
 * =====
 * This function is a system function. A user should not call this
 * function directory in his/her programs.
 *
 * Arguments:
 * ==========
 */
		  char        *mosnam,    
		  /* (in) iraf file name without .imh or .pix */
		  struct imh  *imhp,      
		  /* (in) ptr to imh header structure */
		  struct icom *comp)      
     /* (in) ptr to comment struct */
     /*
      * Return  value:
      * ==============
      *      File pointer to *.mos file.
      *      Will be NULL pointer if faile to open files.
      *
      * Revision history:
      * =================
      * 1993 Feb - original creation (M.Sekiguchi)
      *
      * ^~------------------------------------------------------------------------------ */
{
  char  line[BUFSIZ];	/* line buffer */
  char  *cp;		/* char pntr */
  int   ifrm;		/* frame loop index */
  float vxmin, vxmax;	/* min-max virtual x in "*.mos": file */
  float vymin, vymax;	/* min-max virtual y in "*.mos": file */
  FILE  *fp;		/* group file pntr */
  FILE  *fp0;
  struct mos *mosp;	/* pntr to mosaic struct */
  int frmnum=20;
  int frmnumstep=20;
  struct imh  imhp0={""};      /* (in) ptr to imh header structure */
  struct icom comp0={0};      /* (in) ptr to comment struct */
  int pixignr;

  /* ... Open mosaic group file ... */
  
  if( ( fp=fopen(mosnam,"r") ) == NULL ) {
    (void) printf( "imropen_mos: can't open %s\n", mosnam );
    return NULL ;
  }
  
  mosp= imc_mos_get_mosp(imhp);
  if(mosp==NULL) return NULL;
  
  /* ... read in frame information ... */
  
  ifrm= 0;

  mosp->pixignr=default_pixignr;
  mosp->frm=(mos_frm*)realloc(mosp->frm,frmnum*sizeof(mos_frm));

  while( fgets( line, BUFSIZ, fp ) != NULL ) 
    {    
      /* check if it's a comment line */
      if( line[0] == '#' || line[0] == '\n') continue; 
      
      /* decode in to parameters */
      
      if ( sscanf( line, "%s%f%f%f%f", mosp->frm[ifrm].fnam,
		    &mosp->frm[ifrm].xof, &mosp->frm[ifrm].yof,
		    &mosp->frm[ifrm].rot, &mosp->frm[ifrm].flux )==5)
	{
          mosp->frm[ifrm].sinrot = sin( mosp->frm[ifrm].rot );
          mosp->frm[ifrm].cosrot = cos( mosp->frm[ifrm].rot );
	  ifrm++;
	}
      else
	{
	  printf("Imropen_mos:line is out of format ... skip.\n[%s]\n",line);
	}

      if (ifrm>=frmnum)
	{ 
	  frmnum+=frmnumstep;
	  mosp->frm=(mos_frm*)realloc(mosp->frm,frmnum*sizeof(mos_frm));
	}
    }
  
  /* check # of frames */
  mosp->nfrm= ifrm;
  if( mosp->nfrm <=0 ) 
    {	    
      (void) printf( "imropen_mos: No frames %s\n", mosnam );
      return  NULL;
    }
  mosp->frm=(mos_frm*)realloc(mosp->frm, mosp->nfrm*sizeof(mos_frm));
  
  /* ... read iraf file of each frame ... */
  
  for( ifrm=0; ifrm < mosp->nfrm; ifrm++ ) 
    {
      /* open iraf files */
      mosp->frm[ifrm].pixp = imropen( mosp->frm[ifrm].fnam, &imhp0, &comp0 );
      mosp->frm[ifrm].npx = imhp0.npx;
      mosp->frm[ifrm].npy = imhp0.npy;
      mosp->frm[ifrm].ndx = imhp0.ndatx;
      mosp->frm[ifrm].ndy = imhp0.ndaty;
      mosp->frm[ifrm].typ = imhp0.dtype;

      /* close iraf files */
      
      pixignr=imget_pixignr(&imhp0);
      mosp->frm[ifrm].pixignr=pixignr;
      if (pixignr!=INT_MIN)
	{
	  mosp->pixignr=pixignr;
	}
      imclose(mosp->frm[ifrm].pixp,&imhp0, &comp0 );
    }
  
  /* ... determine max/min virtual x/y */
  
  /* 2001/12/13  revise */
  mosp->npx = 0;
  mosp->npy = 0;
  mosp->vxoff= 0;
  mosp->vyoff= 0;
  
  imc_mos_get_shift(imhp, &vxmin, &vxmax, &vymin, &vymax );
  printf("%f %f %f %f\n",vxmin,vxmax, vymin, vymax );
  
  imc_mos_set_shift(imhp, -vxmin, -vymin);
  
  /* these should be set later */
  mosp->npx = vxmax - vxmin + 1;
  mosp->npy = vymax - vymin + 1;
  
  /* fake imh header for whole iamge */
  imhp->dtype = DTYPEMOS;
  imhp->naxis = 2;
  imhp->npx   = mosp->npx;
  imhp->npy   = mosp->npy;
  imhp->ndatx = mosp->npx;
  imhp->ndaty = mosp->npy;
  
  (void)printf("\n Assembled virtual image size= %d x %d pixels\n\n",
	       mosp->npx, mosp->npy );
  
  /* ... Rewite imh to indicate this is multiple file ...*/
  
  /* take out directory information */
  if( (cp=strrchr(mosnam,'/')) == NULL ) 
    cp= mosnam; 
  else cp++;
  
  /* ... get pointer to mosaic struct */
  
  memset(imhp->pixnam,0,160);
  memset(imhp->imhnam,0,160);
  (void) imencs0( imhp->imhnam, cp );
  (void) imencs0( imhp->pixnam, cp );
  
  /* ... no idea ... **/
  (void) imencs0( imhp->title, cp );
  (void) imencs0( imhp->institute, "");
  
  /* set default values */
  mosp->maxadd=default_maxadd;
  mosp->mix=default_add;
  mosp->buffsize=0;

  mosp->vpixp=NULL;
  /*
    mosp->fmapp=NULL;
  */

  /* 2002/02/04 sort2frm/frm2sort array */
  /* 2002/02/05 not always needed in all mode/ so move to fill or paste */
  /*
    mosp->sort2frm=(int*)calloc( mosp->nfrm,sizeof(int));
    mosp->frm2sort=(int*)calloc( mosp->nfrm,sizeof(int));
  */
  mosp->sort2frm=NULL;
  mosp->frm2sort=NULL;

  /* 2005/01/19 */
  mosp->medbufsiz=0;
  mosp->fmappsiz=0;

  mosp->medbuf=NULL;
  mosp->frmbuf=NULL;
  mosp->fmapp=NULL;

  /* 2002/02/20 */

  fp0=imropen( mosp->frm[0].fnam, &imhp0, &comp0 );

  if(imc_copyWCS(&comp0,comp)==1)
    {
      imaddhistf(comp,"WCS is copied from '%s'", mosp->frm[0].fnam);
      imc_rotate_WCS(comp, 
		     mosp->frm[0].xof, mosp->frm[0].yof,
		     mosp->frm[0].cosrot,mosp->frm[0].sinrot); 
    }
  imclose(fp0,&imhp0,&comp0);

  return fp;
}

void imc_mos_set_default_maxadd( int maxadd )
{
  default_maxadd=maxadd;
}

void imc_mos_set_nline(int nline)
{
  N_L_BUF=nline;
}

void imc_mos_set_nline_l(int nline)
{
  N_L_BUF_L=nline;
}

void imc_mos_set_pixignr(
			 struct imh *imhp,
			 int pixignr)
{
  struct mos *mosp;
  if((mosp = imc_mos_get_mosp(imhp))!=NULL)
    mosp->pixignr = pixignr;

  return;
}

/* ------------------------------------------------------------ */
void imc_mos_set_maxadd( 
/* ------------------------------------------------------------
 *
 *   This function sets the maximum number of frames to be
 *   added into a pixel.
 *
 * Arguments:
 * ==========
 */
     struct imh  *imhp,
     int maxadd) /* max number of frames to be added */
/*
 * ------------------------------------------------------------
 */
{
  struct mos *mosp;
  if((mosp = imc_mos_get_mosp(imhp))!=NULL)
    mosp->maxadd = maxadd;
  else
    {
      printf("imc_mos_set_maxadd:ERROR cannot access imhp\n");
      exit(-1);
    }
  return;
}


void imc_mos_set_default_add (MOSMODE	mode)
{
  default_add=mode;
}

/* ------------------------------------------------------------- */
void imc_mos_set_add ( 
/* -------------------------------------------------------------
 *
 * Description:
 * ============
 *   This function sets how imc_mos co-add pixels.
 *
 * Arguments:
 * ==========
 */
		      struct imh *imhp,
		      MOSMODE	mode)
/*
 * --------------------------------------------------------------
 */
{
  struct mos *mosp;
  if((mosp = imc_mos_get_mosp(imhp))!=NULL)
    mosp->mix = mode;
  return;
}

/* ----------------------------------------------------------------------------- */
float *imc_mos_read(
/* ----------------------------------------------------------------------------- 
 *
 * Description:
 * ============
 *   This is a user callable funcion which reads mosaic data for the specifiled
 *   virtual space window. It allocates buffer space by itself and returns pointer
 *   to the buffer. No void data are padded between two lines.
 *
 * Condition prior to call:
 * ========================
 *   Imropen() should be called for a "*.mos" file.
 *   If co-add mode needs to be set, imc_mos_set_add() should be called.
 *
 * Arguments:
 * ==========
 */
struct imh *imhp,
	int vx1,	/* smallest virtual x */
	int vx2,	/* largest  virtual x */
	int vy1,	/* smallest virtual y */
	int vy2)	/* largest  virtual y */
/*
 * Return value:
 * =============
 *   A pointer to pixel data.
 *   An Error causes return value to be NULL. 
 *
 * Revision:
 * =========
 *   Original creation --- Feb 27, 1993 (M. Sekiguchi)
 *
 * -------------------------------------------------------------------------------
 */
{
  struct mos *mosp;
  
  /* ... Get pointer to mosaic struct ... */
  
  mosp= imc_mos_get_mosp(imhp);
  
  /* ... check if *.mos file is really read ... */

  if( mosp==NULL || mosp->nfrm < 1 ) {
    (void) printf("Imc_Mos_Read: # of Mosaic Frame < 1\n");
    (void) printf("  Check if imropen() is called or\n");
    (void) printf("  check if input file is really *.mos\n");
    return NULL ;
  }
  /* check */
  if (0!=imc_mos_check_fill_buffer(imhp,vx1,vx2,
				   vy1,vy2,(vy2-vy1+1)))
    {
      printf("Error: nline is not appropriate\n");
      exit(-1);
    }
  
  /* ... Read data ... */
  
  return imc_mos_fill_buffer( mosp, vx1, vx2, vy1, vy2 ) ;
}


int imc_mos_merge_org_header(struct icom *icomout,
			     struct imh *imhin)
{
  struct  imh	imhin2={""};
  struct  icom	icomin2={0};
  FILE *fpin2;
  int i;

  int ncom0=32;  
  char key00[32][9]={
    "SIMPLE  ",
    "BITPIX  ",
    "EXTEND  ",
    "IGNRVAL ",
    "BLANK   ",
    "BZERO   ",
    "BSCALE  ",
    "NAXIS   ",
    "NAXIS1  ",
    "NAXIS2  ",
    "NAXIS3  ",
    "NAXIS4  ",
    /***** WCS *******/

    "CD1_1   ",
    "CD1_2   ",
    "CD2_1   ",
    "CD2_2   ",

    "PC001001",
    "PC001002",
    "PC002001",
    "PC002002",

    "CUNIT1  ",
    "CTYPE1  ",
    "CRPIX1  ",
    "CDELT1  ",

    "CUNIT2  ",
    "CTYPE2  ",
    "CRPIX2  ",
    "CDELT2  ",

    "EQUINOX ",
    "LONPOLE ",
    "LONGPOLE",
    "RADECSYS",
  };
  char **key0;
  struct mos *mosp;

  key0=(char**)malloc(sizeof(char*)*ncom0);
  for(i=0;i<ncom0;i++)
    key0[i]=key00[i];

  mosp=imc_mos_get_mosp(imhin);
  if( (fpin2 = imropen ( mosp->frm[0].fnam, &imhin2, &icomin2 )) == NULL ) 
    {
      printf("Cannot open [%s]\n",mosp->frm[0].fnam);
      exit(-1);
    }

  imaddcomf(icomout,"---------------------------------------------------------------------");

  imaddcomf(icomout,"Following lines are copied from %s",
	    mosp->frm[0].fnam);
  imaddcomf(icomout,"---------------------------------------------------------------------");

  imc_merge_icom(icomout,&icomin2,ncom0,key0);
  imclose(fpin2,&imhin2,&icomin2);  
  imaddcomf(icomout,"---------------------------------------------------------------------");
  return 0;
}

int imc_mos_get_params(struct imh *imhin,
		       float *vxoff,
		       float *vyoff,
		       int *npx,
		       int *npy,
		       MOSMODE *mix,
		       int *maxadd,
		       float *pixignr,
		       int *nfrm,
		       mos_frm **frm)
{
  struct mos *mosp;

  if((mosp = imc_mos_get_mosp(imhin))==NULL)
    return -1;

  if(vxoff!=NULL) *vxoff=mosp->vxoff;
  if(vyoff!=NULL) *vyoff=mosp->vyoff;
  if(npx!=NULL) *npx=mosp->npx;
  if(npy!=NULL) *npx=mosp->npy;
  if(mix!=NULL) *mix=mosp->mix;
  if(maxadd!=NULL) *maxadd=mosp->maxadd;
  if(pixignr!=NULL) *pixignr=(float)(mosp->pixignr);
  if(nfrm!=NULL) *nfrm=mosp->nfrm;
  if(frm!=NULL) *frm=mosp->frm;
 
  return 0;
}
