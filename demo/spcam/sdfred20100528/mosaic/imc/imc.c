/* -----------------------------------------------------------------
 *			      imc.c
 * -----------------------------------------------------------------
 * 
 * Descriptions:
 * =============
 *
 *	Imc is an IRAF image io-package for c language.
 *	Imc is not a substitution of IMFORT package but
 *	it provids i/o environmrents suitable for c language.
 *      Imc is very portable and compact, yet very efficient
 *	for IRAF image i/o. It should be run virtually on any
 *	unix platforms.
 *
 * About IRAF format:
 * ==================
 *
 *	An IRAF image are separated into .pix and .imh files.
 *	.imh file has a header which describes what type of
 *	image data they store. The header is followed by 
 *	FITS compatible comments. The size of .imh file is variable
 *	depending on how much comments it stores. .pix also has 
 *	a header which are almost identical to that of .imh file
 *	except that no FIT compatible comments. The pixel data
 *	follows the header.
 *	
 *	.imh file
 *      =========	-------------
 *			 iraf header
 *			-------------
 *			  comments
 *                      -------------
 *	.pix file
 * 	=========	-------------
 *			 iraf header
 *			-------------
 *			 pixel data
 *			-------------
 *
 *	The actual header structure can be seen in imc.h and
 *	imc_head.h. IMFORT routines are mostly for encoding
 *	and decoding parameters in the headers. Imc simply defines
 *	a structure for each header and read the headers as 
 *	structures, so that a user simply access any parameters
 *	by refering structure members. 
 *
 * Usage:
 * ======
 *
 *	If you use the most basic imc routines, imc only returns
 *	a FILE pointer to a pixel file. You can read/write any
 *	types of data using the pointer. 
 *	( imropen, imwopen )
 *
 *	Imc provids routines that read/write all data into/from
 *	local arrays. The local array sould be float. The data
 *	type on the file should be u*2, I*4, or R*4.
 *	( imrall_tor, imwall_rto )
 *
 *	Imc also provids FORTRAN callable routines which read/write
 *	all data.
 *	( imrator, imwarto )
 *
 *	The best examples of usage can be seen in source code of
 *	imrator or imwarto.
 *
 * Function list:
 * ==============
 *
 * FILE *imropen   : open as read
 * FILE *imwopen   : create imh file and open pix file
 *
 * int  imrall_u2u : read all u2 data into u2 array 
 * int  imrall_tor : read all data into r*4 array
 * int  imwall_rto : write all r*4 data
 * int  imrl_tor   : Read a line ( row ) to r*4 array
 * int  imwl_rto   : Write a line (row) from r*4 array
 *
 * void imrau2u    : Fortran callable imropen + imrall_u2u
 * void imrator    : Fortran callable imropen + imrall_tor
 * void imwarto    : Fortran callable imwopen + imwall_rto
 * void imadcm     : Fortran callable imaddcom
 * void imdlcm	   : Fortran callable for deleting a commment
 *
 * void imdecs     : dencode string from iraf/FITS format
 * void imencs     : encode string to iraf/FITS format
 * void imencs0    : encode string with extra \0
 * void imhdump    : print header information
 * void imaddcom   : Add a comment to IRAF/FITS header	
 *
 *  Note:
 *  ====
 *  IMC internally use unused area of imh structure.  This is
 *  the list of usage:
 *
 *	imh.void_2[ VID2SIZ -  8: VID2SIZ -  5 ] : u2 offset
 *	imh.void_2[ VID2SIZ - 23: VID2SIZ - 12 ] : FITS type, zero, scale
 *	imh.void_2[ VID2SIZ - 27: VID2SIZ - 24 ] : offset(?)
 *      imh.void_2[ VID2SIZ - 28 ]               : not used
 *	imh.void_2[ VID2SIZ - 33: VID2SIZ - 29 ] : Ignore pixel value
 *
 *  The u2off is written into an actual IRAF imh file besides FITS
 *  header as a comment.
 *
 *  The Ignore value is also written in to an actual IRAF file.
 *  
 * -----------------------------------------------------------------
 */

/* ... system include files ... */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>

#define TOFF 315500400  /* offset to get JST */

#define WRKSIZ 131072	/* byte size of internal line buffer */

/* define key for u2 pixel offset */
#define PIXOFF_KEY1 0x33
#define PIXOFF_KEY2 0x55

/* define VID2SIZ offset for Ingore pixel value */
#define IGNROFF 29
#define IGNR_KEY 0xaa

/* ... local include files ... */

#include "imc.h"

/* define byte for each data type */
static int psiz[] = {	0,		/*  0:  undefined */
			0,		/*  1:  undefined */
			0,		/*  2:  undefined */
			sizeof(short),	/*  3:  short - unsupported */
			sizeof(int),	/*  4:    int - unsupported */
			sizeof(int),	/*  5:   int */
			sizeof(float),	/*  6:  float */
			0,		/*  7: double - unsupported */
			0,		/*  8: complex -unsupported */
			0,		/*  9:  undefined */
			0,		/* 10:  undefined */
			sizeof(u_short)	/* 11: unsigned short */
		    };

/* define line buffer union */
union {
	short	i2[WRKSIZ/sizeof(short)];	/* for signed short */
	u_short	u2[WRKSIZ/sizeof(short)];	/* for unsigned short */
	int	i4[WRKSIZ/sizeof(int)];		/* for signed int */
	float	r4[WRKSIZ/sizeof(float)];	/* for real*4 */
      }     imcbuf;

/* Yagi added for freeing comp->com */
FILE *imclose ( FILE *fp, 
	       struct imh  *imhp,	
	       struct icom *comp)
{
  int i;
  for( i=0; i<comp->ncom ; i++) 
    {
      if(comp->com[i]!=NULL)free(comp->com[i]);
    }      
  if(comp->com!=NULL) free(comp->com);
  (void) fclose( fp );
/*
  comp->ncom=0;
  comp->com=NULL;
*/
  /* 2003/12/23 */
  free(imhp->buff);

  /* 1999/11/07 */
  memset(imhp,0,sizeof(struct imh));
  memset(comp,0,sizeof(struct icom));

  return NULL;
}


/* ^~--------------------------------------------------------------------- */
FILE *imropen ( 
/* -----------------------------------------------------------------------
 *
 * Description:   <IM>age <R>ead <OPEN>
 * ============
 *
 *	imropen opens both iraf-imh and iraf-pix files, and reads
 *	their header contents into structures. It returns a FILE
 *	pointer where the pixel data starts.
 *	
 *	Note:
 *		1) imh file is closed by imropen.
 *		2) pix file is NOT closed. Someone should close it.
 *		3) FILE pointer IS MOVED to the start of pixel data
 *		   by imropen.
 *
 * Arguments:
 * ==========
 */
	char	    *fnam,	/* (in) iraf file name without .imh or .pix */
	struct imh  *imhp,	/* (in) ptr to imh header structure */
	struct icom *comp)	/* (in) ptr to comment struct */
/*
 * Return  value:
 * ==============
 *	File pointer address where pixel data starts.
 *	Will be NULL pointer if faile to open files.
 *
 * Revision history:
 * =================
 *	Jan 23, 1992 (M. Sekiguchi): modification from messia's iraf_image.c
 *      Apr 24, 1993 (MS): Call imropen_fits() for *.fits
 *
 * ^~------------------------------------------------------------------------------ */
{
#ifndef FORCEFITS
  FILE	*fp;		/* local file pointer */
  struct	pixh pixh;	/* pix header structure */
  char	imhnam[80];	/* imh file name */
  char	pixnam[80];	/* pix file name (logical) */
  char	pixfile[80];	/* pix file name (actual) */
  char	imhdir[80];	/* directory of imh file */
  char	id[80];		/* header id */
  char	*cp;		/* general use */
  int maxcom=IMH_MAXCOM;

  /* 1999/11/02 */
  /* change imh.comm format, from IRAF to FITS */
  char line[IMH_COMSIZE+1];
#endif

/* ... open imh file ... */


  /* check if this is mosaic file */
  if( strstr( fnam, MOSEXTEN ) !=NULL ) {
    return( imropen_mos( fnam, imhp, comp ) );
    /* if mosaic file, imropen_mos will take care all */
  }
  
  /* check if this is fits file */
#ifdef FORCEFITS
    return( imropen_fits( fnam, imhp, comp ) );
#else
  if( strstr( fnam, FITSEXTEN ) !=NULL ) {
    return( imropen_fits( fnam, imhp, comp ) );
    /* if fits file, imropen_fits will take care all */
  }

  
  (void) sprintf( imhnam, "%s%s", fnam, ".imh" );
  if( ( fp=fopen(imhnam,"r") ) == NULL ) {
    (void) printf( "imropen: can't open %s\n", imhnam );
    return( NULL );
  }
  
  if( fread( imhp->id, sizeof(*imhp), (size_t)1, fp ) != 1 ) {
    (void) printf( "imropen: can't read %s\n", imhnam );
    return( NULL );
  }

  /* now read in comments until eof */

  comp->com=(char **)realloc(comp->com,sizeof(char*)*maxcom);
  /* 2000/09/19 */
  if(comp->com==NULL)
    {
      fprintf(stderr,"Cannot allocate memory for comment in imropen\n");
      /* 2000/09/19 */
      /* maxcom should be set smaller and retry*/
      /* but not implemented yet...*/
      exit(-1);
    }

  for( comp->ncom=0; comp->ncom < maxcom; ) 
    {
      comp->com[comp->ncom]=(char*)malloc(sizeof(char)*IMH_COMSIZE);
      /* 2000/09/19 */
      if(comp->com[comp->ncom]==NULL)
	{
	  fprintf(stderr,"Cannot allocate memory for comment line %d in imropen\n",comp->ncom);
	  
	  exit(-1);
	}      
      /* 1999/11/02 */
      if( fread(line,
		(size_t)IMH_COMSIZE, (size_t)1 ,fp) != 1 ) break;
      imdecs(line,(comp->com[comp->ncom]));

      comp->ncom++;
    }
  (void) fclose( fp );
  
  
 /* Restore PIXIGNR from FITS comments */
 (void) imrestore_pixingr( imhp, comp );
  
/* ... open pix file ... */

  /* find out pixel file name */
  (void) imdecs( imhp->pixnam, pixnam );
	
  /* extract imh directory */
  (void) strcpy( imhdir, fnam  );
  if( ( cp=strrchr(imhdir,'/') ) == NULL )
    (void) strcpy( imhdir, "./" );
  else
    *(++cp) = '\0';
  
  /* replace HDR$ with imh directory */
  if( (cp = strstr(pixnam,"HDR$")) != NULL ) {
		cp += 4;
		if( *cp == '/' ) cp++;
		(void) sprintf( pixfile, "%s%s", imhdir, cp );
		}
	else
		(void) sprintf( pixfile, "%s", pixnam );

	/* open pix file */
        if( ( fp=fopen(pixfile,"r") ) == NULL ) {
		(void) printf( "imropen: can't open %s\n", pixfile );
		return( NULL );
		}

	/* read header in */
	if( fread( pixh.id, sizeof(pixh), (size_t)1, fp ) != 1 ) {
		(void) printf( "imropen: can't read %s\n", pixfile );
		return( NULL );
		}

/* ... check header consistency ... */

	(void) imdecs( imhp->id, id );

	if( strcmp( id, "imhdr" ) != 0 ) {
		(void) printf("imropen: wrong header (%s) in imh\n",id);
		return( NULL );
		}

	(void) imdecs( pixh.id, id );
	if( strcmp( id, "impix" ) != 0 ) {
		(void) printf("imropen: wrong header (%s) in pix\n",id);
		return( NULL );
		}

	if( imhp->dtype != pixh.dtype ) {
		(void) printf("imropen: imh & pix different dtype\n" );
		return( NULL );
		}

	if( imhp->naxis != pixh.naxis ) {
		(void) printf("imropen: imh & pix different naxis\n" );
		return( NULL );
		}

	if( imhp->npx != pixh.npx ) {
		(void) printf("imropen: imh & pix different npx\n" );
		return( NULL );
		}

	if( imhp->npy != pixh.npy ) {
		(void) printf("imropen: imh & pix different npy\n" );
		return( NULL );
		}

  return fp;
#endif
}


/* ----------------------------------------------------------------------- */
FILE *imwopen ( 
/* -----------------------------------------------------------------------
 *
 * Description:	 <IM>age <W>rite <OPEN>
 * ============
 *
 *	imwopen writes both iraf-imh and the header portion of iraf-pix
 *	files. It does not fill pixel data but returns a FILE pointer 
 *	where the pixel data should start.
 *	
 *	Note:
 *	     1) imh file is closed by imwopen.
 *	     2) pix file is NOT closed by imwopen. Someone should close it.
 *	     3) FILE pointer IS MOVED to the start of pixel data by imwopen.
 *	     4) imwopen sets the pix directory to "HDR$/".
 *
 * Condition prior to call:
 * ========================
 *
 *	There are many paremeters in imh header to be filled. imwopen fills
 *	most of them for you. You have to fill the following parameters prior
 *	to call imwopen:
 *
 *	imhp->dtype	iraf data type
 *	imhp->npx	x image size
 *	imhp->npy	y image size
 *
 *   +++++++++++
 *   +IMPORTANT+ If you are writing a FITS file, the different data
 *   +++++++++++ type for fits is required. Do the followings prior to
 *		 call imwopen():
 *
 *		   if( !imc_fits_set_dtype(XXX,bzero,bscale) )
 *							 goto error;
 *		 where XXX is one of:
 *				     FITSCHAR  (for signed I*1)
 *				     FITSSHORT (for sigend I*2)
 *				     FITSINT   (for sigend I*4)
 *				     FITSFLOAT (for float  R*4)
 *
 *		 bzero and bscale transfer data as:
 *
 *                     data_in_program = bzero + bscale * data_in_file
 *		 
 *		 imc_fits_set_dtype() returns 0 if XXX is not supported
 *		 type or bscale=0.0.
 *
 *		 However if you read a FITS data and write another FITS
 *		 file with the same data type, it is not necessary to
 *		 call imc_fits_set_dtype().
 *	
 *   If you like to add more comments, please do so using imaddcom.
 *
 * Arguments:
 * ==========
 */
	char	    *fnam,	/* (in) iraf file name without .imh or .pix */
	struct imh  *imhp,	/* (in) ptr to imh header structure */
	struct icom *comp)	/* (in) ptr to comment struct */
/*
 * Return value:
 * =============
 *	File pointer address where pixel data starts.
 *	Will be NULL pointer if faile to open files.
 *
 * Caution:
 * ========
 *	(1) imwopwn DOES OVERWRITE the existing files with the same name.
 *	    Users should check the existing files if needed.
 *
 * Revision history:
 * =================
 *	Jan 29, 1992 (M. Sekiguchi): modification from imropen.
 *	Oct 13, 1992 (M. S): Bug-fix for FITS comments
 *      Apr 24, 1993 (MS): Call imwopen_fits() for *.fits
 *
 * ------------------------------------------------------------------------
 */
{
#ifndef FORCEFITS
  struct pixh pixh;	/* pix header structure */
  FILE	*fp;		/* local file pointer */
  char	imhnam[80];	/* imh file name (logical) */
  char	imhfile[80];	/* imh file name (unix) */
  char	pixnam[80];	/* pix file name (logical) */
  char	pixfile[80];	/* pix file name (unix) */
  char	imhdir[80];	/* directory of imh file */
  char	*cp;		/* general use */
  static char zero[16]="";	/* NULL array */
  char    line[IMH_COMSIZE+1]; 
  int	i,j;		/* for loop */
  int	comlen;
#endif  
  
  /* ... check if this is fits file ... */

#ifdef FORCEFITS
  return( imwopen_fits( fnam, imhp, comp ) );
#else
  if( strstr( fnam, FITSEXTEN ) !=NULL ) {
    return( imwopen_fits( fnam, imhp, comp ) );
    /* if fits file, imwopen_fits will take care all */
  }
  
  /* ... check input parameters ... */
  
  if( imhp->npx > imhp->ndatx ) {
    (void) printf("imwopen: Npix > Ndat in imh header\n");
    return( NULL );
  }
  
  if( imhp->npy > imhp->ndaty ) {
    (void) printf("imwopen: Npix > Ndat in imh header\n");
    return( NULL );
  }
  
  /* ... determine # of axies ... */
  
  if( imhp->npx < 0 ) {
    (void) printf("imwopen: npx < 0\n");
    return( NULL );
  }
  
  if( imhp->npy < 0 ) {
    (void) printf("imwopen: npy < 0\n");
    return( NULL );
  }
  
  if( imhp->npx == 0 && imhp->npy == 0 ) {
    (void) printf("imwopen: both npx and npy are zero\n");
    return( NULL );
  }
  
  imhp->naxis = 0;
  if( imhp->npx > 0 ) imhp->naxis++;	
  if( imhp->npy > 0 ) imhp->naxis++;
  
  /* only naxis <= 2 supported */
  imhp->ukwn1 = 1;
  imhp->ukwn2 = 1;
  imhp->ukwn3 = 1;
  imhp->ukwn4 = 1;
  imhp->ukwn5 = 1;	
  imhp->ukwn6 = 1;	
  imhp->ukwn7 = 1;	
  imhp->ukwn8 = 1;	
  imhp->ukwn9 = 1;	
  imhp->ukwn10= 1;
  
  imhp->off1  = 0x201; 	
  imhp->off2  = 0x201; 	
  imhp->off3  = 0x201; 	
  
  /* ... encode necessary strings ... */
  
  /* separate imh directory and file name */
  /* make pix name and file string */
  (void) strcpy( imhdir, fnam  );
  if((cp=strrchr(imhdir,'/')) == NULL) {
    /* current dir used */
    (void) strcpy(imhdir, "./");
    (void) sprintf(imhnam, "%s%s", fnam, ".imh");
    (void) sprintf(pixnam, "%s%s%s", "HDR$/", fnam, ".pix");
    (void) sprintf(imhfile, "%s%s%s", imhdir, fnam, ".imh");
    (void) sprintf(pixfile, "%s%s%s", imhdir, fnam, ".pix");
  }
  else {
    /* dir path specified */
    cp++;
    (void) sprintf(imhnam, "%s%s", cp, ".imh");
    (void) sprintf(pixnam, "%s%s%s", "HDR$/", cp, ".pix");
    (void) sprintf(imhfile, "%s%s", fnam, ".imh");
    (void) sprintf(pixfile, "%s%s", fnam, ".pix");
    *cp = '\0';
  }
  /*
     (void) printf("Logical imh file= %s\n", imhnam);
     (void) printf("Logical pix file= %s\n", pixnam);
     (void) printf("Unix    imh file= %s\n", imhfile);
     (void) printf("Unix    pix file= %s\n", pixfile);
     */
  
  /* ... encode header id ... */
  
  (void) imencs0( imhp->id, "imhdr" );
  (void) imencs0( imhp->pixnam, pixnam );
  (void) imencs0( imhp->imhnam, imhnam );
  
  /* ... recalculate header size ... */
  
  /* error, sizeof( comp->com ) is size of pointer! 
     i = sizeof( *imhp ) + comp->ncom * sizeof( comp->com );
     */
  i = sizeof( *imhp ) + comp->ncom * IMH_COMSIZE;
  
  if( (i % 16) == 0 ) i = i/4;
  else	i = 4 * (i/16 + 1);
  imhp->imhsize = i;
  
  /* ... change modification date ... */
  
  imhp->tmodify = (int)time((time_t *)0) - TOFF;
  if( imhp->tcreate < TOFF ) 
    {
      /* this date is not reasonable, we will rewrite it */
      imhp->tcreate = imhp->tmodify;
    }	
  
  /* ... copy imh contents to pixh ... */
  
  (void) memcpy( pixh.id, imhp->id, sizeof(pixh) );
  (void) imencs( pixh.id, "pixhdr" );
  
  /* ... encode string to pix header ... */
  
  (void) imencs0( pixh.id, "impix" );
  (void) imencs0( pixh.pixnam, imhnam );
  
  /* ... open imh file ... */
  
  if( ( fp=fopen(imhfile,"w") ) == NULL ) {
    (void) printf( "imwopen: can't open %s\n", imhfile );
    return( NULL );
  }
  
  if( fwrite( imhp->id, sizeof(*imhp), (size_t)1, fp ) != 1 ) {
    (void) printf( "imwopen: can't write %s\n", imhfile );
    return( NULL );
  }
  
  /* now write out comments */
  for( i=0; i<comp->ncom; i++) 
    {
      /* 1999/11/02 */
      
      imencs0(line,comp->com[i]);
	    
      /*   check comment and add terminater string   */
      /*   '\n' should not appear except end of line */
      /*   End of line should be '\n'		       */

      /*  '\0' should not appear in anywhere   */
      /*  charactor is encoded into i=odd      */

      for( j=0; j<IMH_COMSIZE; j++ ) 
	{
	  if( line[j] == '\n' ) 
	    {
	      line[j] = ' ';
	    }
	  else if( j%2==1 && line[j] == '\0' ) 
	    {
	      line[j] = ' ';
	    }
	}
      /* add terminater */
      line[IMH_COMSIZE-1] = '\n';
      line[IMH_COMSIZE] = '\0';

      if( fwrite(line,
		 (size_t)IMH_COMSIZE, (size_t)1 ,fp) != 1 ) 
	{
	  (void) printf( "imwopen: can't write comment\n" );
	  break;
	}
    }
  
  /* we have to write out odd words */
  comlen= 4 + comp->ncom * IMH_COMSIZE;
  comlen= 16 - comlen % 16;
  if( comlen>0 && fwrite(zero,(size_t)comlen,(int)1,fp)!=(int)1 ) {
    (void) printf( "imwopen: can't write odd null comment\n" );
  }
  
  /* now close imh file */
  (void) fclose( fp );
  
  
  /* ... open pix file ... */
  
  /* open pix file */
  if( ( fp=fopen(pixfile,"w") ) == NULL ) {
    (void) printf( "imwopen: can't open %s\n", pixfile );
    return( NULL );
  }
  
  /* write header out */
  if( fwrite( pixh.id, sizeof(pixh), (size_t)1, fp ) != 1 ) {
    (void) printf( "imwopen: can't write %s\n", pixfile );
    return( NULL );
  }
  return( fp );
#endif
}


/* ^~------------------------------------------------------------------------- */
void imdecs( 
/* ---------------------------------------------------------------------------
 *
 * Description: <IM>age <DEC>ode <S>tring
 * ============
 *	Unfortunately, a string is recorded in funny way in an iraf header.
 *	For instants, a string "iraf" is encoded as:
 *
 *		\0, i, \0, r, \0, a, \0, f, \0, \0
 *
 *	imdecs decods a iraf-string ims into a c-string s.
 *	For convenience and safty, imdecs assume sizeof(s) is 80.
 *
 * Arguments:
 * ==========
 */
	char	*ims,		/* (in)  pointer to iraf string */
	char	*s)		/* (out) pointer to c string */
/*
 * Revision history:
 * =================
 *	Jan 23 1992 (M. Sekiguchi): original creation
 *
 * ^~-------------------------------------------------------------------------
 */
{
  int i;
/* Yagi arranged for uninitialized memory protection */
  for(i=0;i<80;i++) 
    {
      if(*(++ims)=='\0')break;
      *(s++) = *(ims++);
    }
  *s = '\0';

}

/* ^~----------------------------------------------------------------------- */
void imencs(
/* -------------------------------------------------------------------------
 *
 * Description:   <IM>age <ENC>ode <S>tring
 * ============
 *	Unfortunately, a string is recorded in funny way in an iraf header.
 *	For instants, a string "iraf" is encoded as:
 *
 *		\0, i, \0, r, \0, a, \0, f, \0, \0
 *
 *	imencs encods a c-string s to a iraf-string ims.
 *	For convenience and safty, imencs assume sizeof(s) is 80.
 *
 * Arguments:
 * ==========
 */
	char	*ims,		/* (out)  pointer to iraf string */
	char	*s)		/* (in) pointer to c string */
/*
 * Revision history:
 * =================
 *	Jan 23 1992 (M. Sekiguchi): original creation
 *	Jun 1996 (M. Yagi): bug fixed for (ims==s)
 *
 * ^~-----------------------------------------------------------------------
 */
{
   int i,j;
	
   for(i=0;i<79;i++) {
     if( s[i] == '\0' ) 
       break;
   }
   for (j=i;j>=0;j--)
     {
       ims[2*j] = '\0';
       ims[2*j+1] = s[j];
     }
}

/* ^~----------------------------------------------------------------------- */
void imencs0( 
/* -------------------------------------------------------------------------
 *
 * Description:   <IM>age <ENC>ode <S>tring
 * ============
 *	Unfortunately, a string is recorded in funny way in an iraf header.
 *	For instants, a string "iraf" is encoded as:
 *
 *		\0, i, \0, r, \0, a, \0, f, \0, \0
 *
 *	imencs encods a c-string s to a iraf-string ims.
 *	For convenience and safty, imencs assume sizeof(s) is 80.
 *
 *	NOTE: imencs0 puts extra '\0' after string.
 *
 * Arguments:
 * ==========
 */
	char	*ims,		/* (out)  pointer to iraf string */
	char	*s)		/* (in) pointer to c string */
/*
 * Revision history:
 * =================
 *	Jan 23 1992 (M. Sekiguchi): original creation
 *	Jun 1996 (M. Yagi): bug fixed for (ims==s)
 *
 * ^~-----------------------------------------------------------------------
 */
{
   int i,j;
	
   for(i=0;i<80;i++) {
     if( s[i] == '\0' ) 
       break;
   }
   ims[2*i]=ims[2*i+1]='\0';
   for (j=i;j>=0;j--)
     {
       ims[2*j] = '\0';
       ims[2*j+1] = s[j];
     }
}


/* ^~----------------------------------------------------------------------- */
void imhdump( 
/* -------------------------------------------------------------------------
 *
 * Description:   <IM>age <H>eader <DUMP>
 * ============
 *	imhdump dumps the contents of imh.
 *
 * Arguments:
 * ==========
 */
	struct imh *imhp,	/* (in) ptr to imh struct */
	struct icom *comp)	/* (in) ptr to comments struct */
/*
 * Revision history:
 * =================
 * 	Jan 27, 1992 (M. Sekiguchi): original creation
 *
 * ^~-------------------------------------------------------------------------
 */
{
  char s[81];	/* string for every thing */
  int   i;	/* for loop */

/* ... dump imh header ... */

	(void) printf("\n IMH HEADER DUMP \n");

	(void) imdecs( imhp->id, s );
	(void) printf( "Header id: %s\n", s );
	(void) printf( "Imh size:  %d\n", imhp->imhsize );
	(void) printf( "Data type: %d\n", imhp->dtype );
	(void) printf( "# of axis: %d\n", imhp->naxis );
	(void) printf( "Image size: %d x %d\n", imhp->npx, imhp->npy );
	(void) printf( "Data  size: %d x %d\n", imhp->ndatx, imhp->ndaty );

	(void) imdecs( imhp->pixnam, s );
	(void) printf( "Pixel file: %s\n", s );
	(void) imdecs( imhp->imhnam, s );
	(void) printf( "Imh file: %s\n", s);
	/* 2002/07/31 */
	/*
	(void) imdecs( imhp->title, s );
	(void) printf( "Title: %s\n", s );
	(void) imdecs( imhp->institute, s );
	(void) printf( "Institute: %s\n", s );
	*/

	(void) printf( "\n" );
	for( i=0; i<comp->ncom; i++ ) 
	  {
	    (void) printf("%s\n",comp->com[i]);
	  }
		
  (void) printf("\n");
}


/* ^~------------------------------------------------------------------------ */
int imrall_tor(
/* -------------------------------------------------------------------------- 
 *
 * Description: <IM>age <R>ead <ALL> data in<TO> <R>eal*4 array
 * ============
 *	imrall_tor reads all data from a pixel file into a given 
 *	real*4 arrray. It can read the following IRAF data types:
 *
 *	unsigned short ( unsigned integer*2 )
 *		 signed int ( integer*4 )
 *		 float	( real*4 )		
 *
 *	and the following fits data types:
 *
 *	signed char  (I*1)
 *	signed short (I*2)
 *	signed int   (I*4)
 *             float (R*4)
 *
 * Arguments:
 * ==========
 */
	struct imh *imhp,	/* (in) ptr to imh struct */
	FILE	   *fp,		/* (in) ptr to pixel file */
	float	   *pix,	/* (out) ptr to pixel array to be filled */
	int	   pdim)	/* (in) dimension of pix */
/*
 * Note:
 * =====
 *
 *	(1) PLEASE NOTE that the pixel data are filled into pix
 *          WITHOUT any gaps between pixel rows. Such choise came from
 *          the fact that C does not allow variable array size as FORTRAN
 *          does. Since the image dimensions are not pre-known and C does
 *          not allow variable array size, when pixel data are filled with
 *	    gaps, these is no use of expression like pix[iy][ix].
 *
 *	(2) If necessary pixel size is larger than pdim, imrall_tor
 *	    immediately returns with 0.
 *
 *	(3) imrall_tor loads pixel data in an internal array whose size is
 *	    64K in u_short. The ndatx should not exceed this size.
 *
 * Revision history:
 * =================
 *	Jan 27, 1992 (M. Sekiguchi): original creation.
 *	Arp 24, 1993 (MS): Completely rewirten using imrall_rl().
 *
 * ^~--------------------------------------------------------------------------
 */ 
{
int	iy;		/* do loop */
int	xdim,ydim;	/* array dimension in x */
float	*dp;		/* ptr to array */
/* char	s[81];		*//* string for header id */

/* ... check if this is a mosaic file ... */

        if( imhp->dtype == DTYPEMOS ) return( imrall_tor_mos( imhp, pix, pdim ) );
                /* if mosaic file,  imrall_tor_mos will take care all */

/* ... check paramerters ... */

	if( imhp->naxis != 2 
	    /* 2004/10/26 */
	    &&
	    imhp->naxis != 1
	    ) {
		(void) printf("imrall_tor: wrong # of axis = %d\n",
			       imhp->naxis);
		return( (int)0 );
		}

	if( pdim < (imhp->npy * imhp->npx) ) {
		(void) printf("imrall_tor: given array size too small\n");
		return( (int)0 );
		}

	xdim = imhp->npx; /* this used to be a function argument */
	ydim = imhp->npy; /* this used to be a function argument */

	if( imhp->npx > imhp->ndatx ) {
		(void) printf("imrall_tor: Npix > Ndat in imh header\n");
		return( (int)0 );
		}
	if( imhp->npy > imhp->ndaty ) {
		(void) printf("imrall_tor: Npix > Ndat in imh header\n");
		return( (int)0 );
		}

/* ... loop over iy ... */

	for( iy=0; iy<ydim; iy++ ) {

	   /* calculate array pntr */
	   dp = pix + iy * xdim;

	   /* read one line */
	   if( !imrl_tor( imhp, fp, dp, iy ) ) goto err;

	   }
	   		 			
return( (int)1 );


/* Jump here if read error */

err:
	(void) printf("imrall_tor: error reading pix data\n");
	return( (int)0 );				
}


/* ------------------------------------------------------------------------ */
int imwall_rto( 
/* ------------------------------------------------------------------------ 
 *
 * Description: <IM>age <W>rite <ALL> <R>eal*4 array <TO> pixel data.
 * ============
 *	imwall_rto writes all real*4 data to a pixel file. 
 *	It can write the following IRAF data types:
 *
 *	unsigned short ( unsigned integer*2 )
 *		 signed int  (	integer*4 )
 *		 float	( real*4 )
 *
 *	and the following fits data types:
 *	
 *	signed char  (I*1)
 *      signed short (I*2)
 *      signed int   (I*4)
 *      float	     (R*4)
 *
 *	When writing data to u2 or i4, the trancation is
 *	performed since C does not do this automatically:
 *
 *      	if( data > i4_max ) data = i4_max
 *		if( data < i4_min ) data = i4_min, or
 *
 *      	if( data > u2_max ) data = u2_max
 *		if( data <      0 ) data = 0
 *
 *	These requires addtional CPU time.
 *	
 * Arguments:
 * ==========
 */
	struct imh *imhp,	/* (in) ptr to imh struct */
	       FILE	   *fp,	/* (in) ptr to pixel file */
	float	   *pix)	/* (in) ptr to pixel data array */
/*
 * Note:
 * =====
 *	(1) pix is assmmed to have all data contiguous, or no data padding
 *	    is necessary. 
 *
 *	(2) imwall_rto loads pixel data in an internal array whose size is
 *	    64K in u_short. The xdim should not exceed this size.
 *
 * Caution:
 * ========
 *
 *	(1) You should correctly fill imhp->ndatx and ndaty with the sizes of
 *	    real*4 array. This has nothing to do with the actual image
 *	    size but the size of the real*4 array when it is DECLARED 
 *	    IN YOUR PROGRAM.
 * 
 * 	(2) imwall_rto writes array data into a pixel file according to:
 *
 *		imhp->npx, imhp->npy,	
 *		imhp->ndatx, imhp->ndaty
 *
 *	Since the ".imh" file or fits header is already written when 
 *	you call imwopen, no one can change or adjust ndatx or ndaty.
 *	Please be carfull not to change ndatx or ndaty after
 *	calling imwopen.
 *
 * Revision history:
 * =================
 *	Feb 1, 1992 (M. Sekiguchi): modification from imrall_tor.
 *	Apr 23, 1993 (MS): Completely rewritten using imwl_rto().
 *
 * -------------------------------------------------------------------------
 */ 
{
int	iy;	/* do loop */
int	xdim;	/* x array size of pix */
int    ydim;	/* y array size of pix */
float	*dp;	/* ptr to array */


/* ... check paramerters ... */

	if( imhp->naxis > 2 ) {
		(void) printf("imwall_rto: wrong # of axis = %d\n",
			       imhp->naxis);
		return( (int)0 );
		}

	xdim = imhp->npx;
	ydim = imhp->npy;

	if( imhp->npx > imhp->ndatx ) {
		(void) printf("imwall_rto: Npix > Ndat in imh header\n");
		return( (int)0 );
		}
	if( imhp->npy > imhp->ndaty ) {
		(void) printf("imwall_rto: Npix > Ndat in imh header\n");
		return( (int)0 );
		}
		 
/* ... write pixel data ... */

	for( iy=0; iy<ydim; iy++ ) {

	   /* locate pixel pntr */
	   dp = pix + iy * xdim;

	   /* write one line */
	   if( !imwl_rto(imhp,fp,dp,iy) ) goto err;

	   }	   
			
return( (int)1 );


/* Jump here if write error */

err:
	(void) printf("imwall_rto: error writing pix data\n");
	return( (int)0 );				
}

/* ^~-------------------------------------------------------------- */
void imadd_fits( 
/* ----------------------------------------------------------------
 *
 * Description:	 <IM>age <ADD> a <FITS> line 
 * ============
 *	imadd_fits adds a FITS line to a comment structure.
 *	Unlike imaddcom, string s will be written as it is.
 *
 * Arguments:
 * ==========
 */
	struct icom *comp,	/* (in/out) ptr to comment struct */
	char	    *s)		/* ptr to your comment string */
/*
 * Revision history:
 * =================
 *	Oct 20 1992 (M. Sekiguchi): Modify from imaddcom
 *	Jun 1996 (M. Yagi): Use sprintf
 *
 * ^~---------------------------------------------------------------
 */
{
  char line[100];	/* FITs line */
/*  int i;*/
   sprintf(line,"%-80.80s%s",s,"              ");
  
  comp->com=(char**)realloc(comp->com,sizeof(char*)*(comp->ncom+1));
  if(comp->com==NULL)
    {
      fprintf(stderr,"Cannot reallocate memory for comment in imaddcom\n");
      /* 2000/09/19 */
      /* maxcom should be set smaller and retry*/
      /* but not implemented yet...*/
      exit(-1);
    }
  comp->com[comp->ncom]=(char*)malloc(sizeof(char)*IMH_COMSIZE);
  if(comp->com[comp->ncom]==NULL)
    {
      fprintf(stderr,"Cannot reallocate memory for new comment in imadd_fits\n");
      /* 2000/09/19 */
      /* maxcom should be set smaller and retry*/
      /* but not implemented yet...*/
      exit(-1);
    }

  /*
  strcpy(comp->com[comp->ncom],line );
  */
  sprintf(comp->com[comp->ncom],"%-80.80s",line);

#if 0
  /* encode string */
  (void) imencs( &comp->com[comp->ncom][0], line );
  
  /* add terminater string */
  comp->com[comp->ncom][IMH_COMSIZE-3] = ' ';	
  comp->com[comp->ncom][IMH_COMSIZE-1] = '\n';	
#endif
  
  comp->ncom++;
}	

/* ------------------------------------------------------------------------ */
void imaddcom(
/* ----------------------------------------------------------------
 *
 * Description:	 <IM>age <ADD> a <COM>ment 
 * ============
 *	imaddcom adds a comment to a comment structure.
 *	After imwopen, it will be written as:
 *
 *	(COMMXXX = 'Here is a your comment.') <- obsolete 
 *      COMMENT  Here is a your comment.
 *
 *	(where XXX is the comment number. The comment is upto
 *	 70 characters.) <- obsolete
 *
 * Arguments:
 * ==========
 */
	struct icom *comp,	/* (in/out) ptr to comment struct */
	char	    *s)		/* ptr to your comment string */
/*
 * Revision history:
 * =================
 *	Feb 1 1992 (M. Sekiguchi): original creation
 *	Oct 13 19912 (M.S): '\n' added at end of each line (bug-fix)
 *             1997  Yagi FITS standard
 *
 * ^~---------------------------------------------------------------
 */
{
  char line[81];	/* FITs line */
  int i,l;
  char head[10];

  /* 2001/05/07 */
  /* folding support */
  /* make FITS format line */

  l=strlen(s);
  sprintf(head,"COMMENT");
  for(i=0;i<=l/70;i++)
    {
      (void) sprintf( line, "%s  %-70.70s",head,s+70*i);
      imadd_fits(comp,line);     
      /* indent */
      sprintf(head,"COMMENT ");
    }  
}

/* ------------------------------------------------------------------------ */

void imaddhist(
	struct icom *comp,	/* (in/out) ptr to comment struct */
	char	    *s)		/* ptr to your comment string */
 /* Made from imaddcom */
{
  char line[81];  /* FITs line */
  int i,l;
  char head[10];

  /* 2001/05/07 */
  /* folding support */
  /* make FITS format line */
  /* and use imadd_fits */

  l=strlen(s);
  sprintf(head,"HISTORY");
  for(i=0;i<=l/70;i++)
    {
      (void) sprintf( line, "%s  %-70.70s",head,s+i*70);
      /*
	printf("debug:[%s]\n",line);
      */
      imadd_fits(comp,line);     
      /* indent */
      sprintf(head,"HISTORY ");
    }
}	


/* ^~------------------------------------------------------------------------ */


int imrl_tor( 
/* --------------------------------------------------------------------------
 *
 * Description: <IM>age <R>ead <L>ine data <TO> <R>eal*4 array
 * ============
 *      imrl_tor reads line data from a pixel file into a given
 *      real*4 arrray. It can read the following data types:
 *
 *	signed short
 *      unsigned short ( unsigned integer*2 )
 *      signed int  ( integer*4 )
 *      float  ( real*4 )
 *
 * Arguments:
 * ==========
 */
        struct imh *imhp,       /* (in) ptr to imh struct */
        FILE       *fp,         /* (in) ptr to pixel file */
        float      *dp,         /* (out) ptr to pixel array to be filled */
        int       iy)          /* (in) Line # start with 0 */
/*
 * Note:
 * =====
 *      Some internal check is (like pix size consistence)
 *      eliminated for speed.
 *
 *
 * Revision history:
 * =================
 *      Oct 1992 (M. Sekiguchi): Modification from imrall_tor.
 *      Apr 1993 (MS): Call imc_fits_rl() for fits file.
 *
 * ^~--------------------------------------------------------------------------
 */
{


/* local variables */
size_t  size;           /* data size (in byte) to read */
int    offset;         /* offset in file pointer */
int    ip;             /* do loop */
int    pixoff;          /* pixel value offset */
/* char	s[80]; */ 


/* ... check if this is a mosaic file ... */

        if( imhp->dtype == DTYPEMOS ) 
                return( imrl_tor_mos( imhp, dp, iy ) );
                /* if mosaic file, imrl_tor_mos will take care all */

/* ... check if this is a fits file ... */

	if( imhp->dtype == DTYPFITS ) {
		return( imc_fits_rl( imhp, fp, dp, iy )	);
		}

/* ... check paramerters ... */

        if( imhp->naxis != 2 
	    /* 2004/10/26 */
	    && imhp->naxis != 1) {
                (void) printf("imrl_tor: wrong # of axis = %d\n",
                               imhp->naxis);
                return( (int)0 );
                }

        if( imhp->npx > imhp->ndatx ) {
                (void) printf("imrl_tor: Npix > Ndat in imh header\n");
                return( (int)0 );
                }
        if( imhp->npy > imhp->ndaty ) {
                (void) printf("imrl_tor: Npix > Ndat in imh header\n");
                return( (int)0 );
                }

        if( imhp->dtype > 11 || psiz[imhp->dtype] == 0 ) {
                (void) printf("imrl_tor: I don't read data type of %d\n",
                 imhp->dtype);
                return( (int)0 );
                }

        if( (imhp->ndatx * psiz[imhp->dtype]) > WRKSIZ ) {
                (void) printf("imrl_tor: data size in x > internal buffer\n");
                return( (int)0 );
                }

/* ... Repositoin file pointer to pix data ... */

        /* ... use ndatx (physical # of data)  ... */
        offset= (int)PIXOFF +  iy * psiz[imhp->dtype] * imhp->ndatx;
        if( fseek( fp, offset, 0 ) != 0 ) {
                (void) printf("imrl_tor: error positioning ptr to pix data\n");
                return( (int)0 );
                }

/* ... read pixel data ... */

        size     = imhp->npx * psiz[imhp->dtype];       /* line size */

        /* ... switch for different data type ... */
        switch ( imhp->dtype ) {

           /* ... data in file are int ... */
           case DTYPINT:
                   if(fread(imcbuf.i4,size,(size_t)1,fp) != 1) goto err;
                   for(ip=0;ip<imhp->npx;ip++,dp++) {*dp = (float)imcbuf.i4[ip];}
                break;

           /* ... data in file are integer*4 ... */
           case DTYPI4:
                   if(fread(imcbuf.i4,size,(size_t)1,fp) != 1) goto err;
                   for(ip=0;ip<imhp->npx;ip++,dp++) {*dp = (float)imcbuf.i4[ip];}
                break;

           /* ... data in file are real*4 ... */
           case DTYPR4:
                   if(fread(dp,size,(size_t)1,fp) != 1) goto err;
                break;

           /* ... data in file are short ... */
           case DTYPI2:
                   if(fread(imcbuf.i2,size,(size_t)1,fp) != 1) goto err;
                   for(ip=0;ip<imhp->npx;ip++,dp++) {*dp = (float)imcbuf.i2[ip];}
                break;

           /* ... data in file are unsigned short ... */
           case DTYPU2:
		/* get pixel offset */
		   pixoff = imget_u2off( imhp );
                   if(fread(imcbuf.u2,size,(size_t)1,fp) != 1) goto err;
		   if( pixoff == 0 ) {
                      for(ip=0;ip<imhp->npx;ip++,dp++) {*dp = (float)imcbuf.u2[ip];}
		      }
		   else {
                      for(ip=0;ip<imhp->npx;ip++,dp++) {
			   *dp = (float)( (int)imcbuf.u2[ip] - pixoff );
			   }
			}
               break;

           /* ... unsupported data type ... */
           default:
                (void)printf("Unsupported data type - Wrong program flow\n");
                return( (int)0 );

        } /* end switch */


return( (int)1 );


/* Jump here if read error */

err:
        (void) printf("imrl_tor: error reading pix data at iy=%d\n", iy);
        return( (int)0 );
}


/* ^~------------------------------------------------------------------------ */
int imwl_rto( 
/* --------------------------------------------------------------------------
 *
 * Description: <IM>age <W>rite <L>ine <R>eal*4 array <TO> pixel data.
 * ============
 *      imwl_rto writes real*4 data in a line to a pixel file.
 *      It can write the following data types:
 *
 *              unsigned short ( unsigned integer*2 )
 *                signed int  (          integer*4 )
 *                       float  (            real*4 )
 *
 *      When writing data to u2 or i4, the trancation is
 *      performed since C does not do this automatically:
 *
 *              if( data > i4_max ) data = i4_max
 *              if( data < i4_min ) data = i4_min, or
 *
 *              if( data > u2_max ) data = u2_max
 *              if( data <      0 ) data = 0
 *
 *      These requires addtional CPU time.
 *
 * Arguments:
 * ==========
 */
        struct imh *imhp,       /* (in) ptr to imh struct */
        FILE       *fp,         /* (in) ptr to pixel file */
        float      *dp,        /* (in) ptr to pixel data array */
        int        iy)         /* (in) line # starting with 0 */
/*
 * Note:
 * =====
 *
 *         imwl_rto loads pixel data in an internal array whose size is
 *          64K in u_short. The xdim should not exceed this size.
 *
 * Caution:
 * ========
 *
 *      imwl_rto writes array data into a pixel file according to:
 *
 *              imhp->npx, imhp->npy,
 *              imhp->ndatx, imhp->ndaty.
 *
 *      Since the ".imh" file is already written when you call
 *      imwopen, no one can change or adjust ndatx or ndaty.
 *      Please be carfull not to change ndatx or ndaty after
 *      calling imwopen.
 *
 * Revision history:
 * =================
 *      Oct 1992 (M. Sekiguchi): modification from imwl_tor.
 *	Apr 1993 (MS): Call imc_fits_wl() for fits file.
 *
 * ^~-------------------------------------------------------------------------
 */
{

#include <limits.h>
#define I4_MAX INT_MAX
#define I4_MIN INT_MIN
#define U2_MAX USHRT_MAX


/* local variables */
size_t  size;           /* data size (in byte) to read */
int    ip;             /* do loop */
int    offset;
float	fdat;
int	pixoff;		/* pixel offset */


/* ... check if this is a fits file ... */

	if( imhp->dtype == DTYPFITS ) {
		return( imc_fits_wl( imhp, fp, dp, iy )	);
		}

/* ... check paramerters ... */

       if( imhp->npx > imhp->ndatx ) {
                (void) printf("imwl_rto: Npix > Ndat in imh header\n");
                return( (int)0 );
                }
        if( imhp->npy > imhp->ndaty ) {
                (void) printf("imwl_rto: Npix > Ndat in imh header\n");
                return( (int)0 );
                }

        if( imhp->dtype > 11 || psiz[imhp->dtype] == 0 ) {
                (void) printf("imwl_rto: I don't write data type of %d\n",
                 imhp->dtype);
                return( (int)0 );
                }

        if( (imhp->npx * psiz[imhp->dtype]) > WRKSIZ ) {
                (void) printf("imwl_rto: im size in x > internal buffer\n");
                return( (int)0 );
                }

/* ... Repositoin file pointer to pix data ... */

        /* ... use ndatx (physical # of data)  ... */
        offset= (int)PIXOFF +  iy * psiz[imhp->dtype] * imhp->ndatx;
        if( fseek( fp, offset, 0 ) != 0 ) {
                (void) printf("imwl_rto: error positioning ptr to pix data\n");
                return( (int)0 );
                }

/* ... read pixel data ... */

        size     = imhp->npx * psiz[imhp->dtype];     /* line size in file*/

        /* ... switch for different data type ... */
        switch ( imhp->dtype ) {


           /* ... data in file are integer*4 ... */
           case DTYPI4:
 	                  for(ip=0;ip<imhp->npx;ip++,dp++) {
			fdat= *dp + 0.5;
                        if( fdat > I4_MAX ) {
                                imcbuf.i4[ip] = I4_MAX;  }
                        else if( fdat < I4_MIN ) {
                                imcbuf.i4[ip] = I4_MIN;  }
                        else    {
                                imcbuf.i4[ip] = (int)fdat; }
                        }
                   if(fwrite(imcbuf.i4,size,(size_t)1,fp) != 1) goto err;
                break;

           /* ... data in file are real*4 ... */
           case DTYPR4:
                   if(fwrite(dp,size,(size_t)1,fp) != 1) goto err;
                break;

           /* ... data in file are unsigned short ... */
           case DTYPU2:
		   /* get pixel offset */
		   pixoff = imget_u2off( imhp );
                   for(ip=0;ip<imhp->npx;ip++,dp++) {
			fdat= *dp + 0.5 + pixoff;
                        if( fdat > U2_MAX ) {
                                imcbuf.u2[ip] = U2_MAX;  }
                        else if( fdat < 0.0 ) {
                                imcbuf.u2[ip] =     0 ;  }
                        else    {
                                imcbuf.u2[ip] = (u_short)fdat; }
                        }
                   if(fwrite(imcbuf.u2,size,(size_t)1,fp) != 1) goto err;
                break;

           /* ... unsupported data type ... */
           default:
                (void)printf("Unsupported data type - Wrong program flow\n");
                return( (int)0 );

        } /* end switch */


return 1;


/* Jump here if write error */

err:
        (void) printf("imwl_rto: error writing pix data\n");
        return 0;
}

/* --------------------------------------------------------------- */
void imset_u2off( 
/* ---------------------------------------------------------------
 *
 * Description:
 * ============
 *	imset_u2off sets offset value for u2 data type. The offset
 *	value is stored as:
 *
 *	imhp->void_2[VID2SIZ-8] = PIXOFF_KEY1
 *	imhp->void_2[VID2SIZ-7] = PIXOFF_KEY2
 *	imhp->void_2[VID2SIZ-6] = upper 8 bit of offset
 *	imhp->void_2[VID2SIZ-5] = lower 8 bit of offset
 *
 *				  Note: VID2SIZ=1000
 *
 *	This location seem to be unused by IRAF.....
 *
 *	imset_u2off also encodes u2 offset information into FITS
 *	header.
 *
 * Argument:
 * =========
 */
	struct imh  *imhp,	/* (in/out) ptr to header struct */
	struct icom *comp,	/* (in/out) ptr to comment struct */
	int u2off)		/* read ADC = ADC in file + u2off */
/*
 *
 * Revision:
 * =========
 *	Original creation - Oct 20 (M.S)
 *	Use imrep_fits - Jun 1996 (M.Yagi)
 * 
 * ----------------------------------------------------------------
 */
{
  char line[IMH_COMSIZE/2];
  u_char *uc;
 
	/* check existing offset */
	if( imget_u2off( imhp ) == u2off ) return;
 
	/* set key words */
	uc = (u_char *)&imhp->void_2[VID2SIZ-8];
	*(uc++) = PIXOFF_KEY1;
	*(uc++) = PIXOFF_KEY2;

	/* set offset value */
	*(uc++) = (u_char)( (u2off & 0xff00) >> 8  );
	*uc     = (u_char)(  u2off & 0xff          );

	/* add comments */
	(void) sprintf( line, "%s%d%s", "U2OFF   =                  ",
			u2off, "  /  REAL_U2 = U2_IN_FILE + OFFSET" );
	if( imrep_fits( comp, "U2OFF   ", line ) == 0 ) {
	  (void) imadd_fits( comp, line );
	}
}

/* --------------------------------------------------------------- */
int imget_u2off( 
/* ---------------------------------------------------------------
 *
 * Description:
 * ============
 *	imget_u2off return offset information from header struct.
 *
 * Argument:
 * =========
 */
	struct imh *imhp)	/* (in/out) ptr to header struct */
/*
 *
 * Revision:
 * =========
 *	Original creation - Oct 20 (M.S)
 * 
 * ----------------------------------------------------------------
 */
{
  int offset;
  u_char *uc;

  /* check key words */
  uc = (u_char *)&imhp->void_2[VID2SIZ-8];
  
  if( ( *uc == PIXOFF_KEY1 ) && ( *(uc+1) == PIXOFF_KEY2 ) )
    
    offset = 0x100 * (int)( *(uc+2) ) +
      (int)( *(uc+3) );
  else  	offset = 0;
  
  return offset;
}

/* --------------------------------------------------------------- */
void imrestore_pixingr( 
/* ---------------------------------------------------------------
 *
 * Description:
 * ============
 * This function restores PIXIGNR. It is read from FITS header
 * and encoded into imhp.
 * 
 * Argument:
 * =========
 */
	struct imh  *imhp,	/* (in/out) ptr to header struct */
	struct icom *comp)	/* (in/out) ptr to comment struct */
/*
 *
 * Revision:
 * =========
 *	Original creation - 96 May (M.S)
 * 
 * ----------------------------------------------------------------
 */
{
  char sss[IMH_COMSIZE/2];
  float bzero,bscale;
  int pixoff;
  int dtype;

  if( imget_fits_value( comp, "BLANK", sss ) == 1 ) 
    {
      /* 1999/02/23 debug */
      /* 1999/09/21 debug for IGNRVAL */
      if( imhp->dtype==DTYPFITS)
	{
	  imc_fits_get_dtype(imhp,&dtype,&bzero,&bscale,&pixoff);
	  imset_pixignr( imhp, comp,
			(int)floor((float)atoi(sss)*bscale+bzero+0.5));
	}
      else
	{
	  (void)imset_pixignr( imhp, comp, atoi( sss ) );
	}
    }
  else
    /* 2004/09/06 bug fix */
    if( imget_fits_value( comp, "IGNRVAL", sss ) == 1 ) 
      {
	if( imhp->dtype==DTYPFITS)
	  {
	    imc_fits_get_dtype(imhp,&dtype,&bzero,&bscale,&pixoff);
	    imset_pixignr( imhp, comp,
			   (int)floor((float)atoi(sss)*bscale+bzero+0.5));
	  }
	else
	  {
	    (void)imset_pixignr( imhp, comp, atoi( sss ) );
	  }
      }
}


/* --------------------------------------------------------------- */
void imset_pixignr( 
/* ---------------------------------------------------------------
 *
 * Description:
 * ============
 * imset_pixignr sets pixel value to be ignored in the annalysis.
 * The value is stored as:
 *
 *	imhp->void_2[VID2SIZ-32] = 
 *	imhp->void_2[VID2SIZ-31] = 
 *	imhp->void_2[VID2SIZ-30] = 
 *	imhp->void_2[VID2SIZ-29] = 
 *	imhp->void_2[VID2SIZ-28] = 
 *
 *				  Note: VID2SIZ=1000
 *
 *	This location seem to be unused by IRAF.....
 *  This routine also encodes the ingore value information into FITS
 *	header.
 *
 * Argument:
 * =========
 */
	struct imh  *imhp,	/* (in/out) ptr to header struct */
	struct icom *comp,	/* (in/out) ptr to comment struct */
	int pixignr)		/* (in) pixel value should be ignored */
/*
 *
 * Revision:
 * =========
 *	Original creation - 96 May (M.S)
 * 
 * ----------------------------------------------------------------
 */
{
  char line[IMH_COMSIZE/2];
  u_char *uc;
  float bzero,bscale;
  int dtype,pixoff;
  int blank;
  
  /*u_int xxx;*/

  /* set key words */
  uc = (u_char *)&imhp->void_2[ VID2SIZ - IGNROFF - 4 ];
  *(uc++) = IGNR_KEY;
  
  /* set offset value */
  *(uc  ) = (u_char)( (pixignr & 0xff000000) >> 24  );
  *(uc+1) = (u_char)( (pixignr & 0x00ff0000) >> 16  );
  *(uc+2) = (u_char)( (pixignr & 0x0000ff00) >>  8  );
  *(uc+3) = (u_char)(  pixignr & 0x000000ff         );
  
  /* add comments */
  /*
     (void) sprintf( line, "%s%11d%s", "IGNRVAL =          ",
     pixignr, " /  PIXVALUE TO BE IGNORED" );
     
     if( imrep_fits( comp, "IGNRVAL", line ) == 0 ) {
     (void) imadd_fits( comp, line );
     }
     */

  /* 98 May, yagi, 'BLANK' is value in raw file in FITS! 
     therefore we need conversion */

   if( imhp->dtype==DTYPFITS)
    {
      imc_fits_get_dtype(imhp,&dtype,&bzero,&bscale,&pixoff);
      blank = (int)floor(((float)pixignr-bzero)/bscale+0.5);
/*
      printf("*** %f %f %f\n",(float)pixignr,bzero,bscale);
*/
    }
   else
     {
       blank=pixignr;
     }

   if( imhp->dtype==DTYPFITS && dtype!=FITSFLOAT)
     {
       (void) sprintf( line, "%s%11d%s", "BLANK   =          ",
		       blank, " /  PIXVALUE TO BE IGNORED" );
       
       if( imrep_fits( comp, "BLANK", line ) == 0 
	   /*     imrep_fits( comp, "IGNRVAL", line ) == 0 */
	   ) 
	 {
	   if(imrep_fits( comp, "IGNRVAL", line ) == 0)
	     {
	       (void) imadd_fits( comp, line );
	     }
	 }
     }
   else 
    {
       (void) sprintf( line, "%s%11d%s", "IGNRVAL =          ",
		       blank, " /  PIXVALUE TO BE IGNORED" );
       if( imrep_fits( comp, "BLANK", line ) == 0 )
	 {
	   if (imrep_fits( comp, "IGNRVAL", line ) == 0)
	     {
	       (void) imadd_fits( comp, line );
	     }
	 }
    }
  if( imhp->dtype==DTYPEMOS ) imc_mos_set_pixignr(imhp,pixignr);

/*
  printf("TEST:: %d\n",pixignr);
*/
}

/* --------------------------------------------------------------- */
int imget_pixignr( 
/* ---------------------------------------------------------------
 *
 * Description:
 * ============
 *  imget_pixignr returns pixel value to be ingored in analysis
 *  header struct. If the value is not defined in the structure,
 *  it returns INT_MIN (-2147483647).
 *
 * Argument:
 * =========
 */
	struct imh *imhp)	/* (in/out) ptr to header struct */
/*
 *
 * Revision:
 * =========
 *	Original creation - 96 May (M.S)
 * 
 * ----------------------------------------------------------------
 */
{
  int pixignr;
  u_char *uc;
  struct mos *mosp;

/*  int dtype,pixoff; */
/*  float bzero,bscale; */
/*  char sss[81];*/

  /* 1999/02/18 added (yagi) */
  if(imhp->dtype==DTYPEMOS)
    { 
      mosp=imc_mos_get_mosp(imhp);
      return mosp->pixignr;
    }

  uc = (u_char *)&imhp->void_2[ VID2SIZ - IGNROFF - 4 ];
  
  if( ( *(uc++) == IGNR_KEY ) ) 
    {
      pixignr = *(uc  ) * 0x01000000 |
	*(uc+1) * 0x00010000 |
	  *(uc+2) * 0x00000100 |
	    *(uc+3) ;	    
    } 
  else 
    {
      pixignr = INT_MIN;		
    } 

  return pixignr;
}
/* ---------------------------------------------------------------------- */
int imget_fits_value( 
/* ----------------------------------------------------------------------
 *
 * Description:
 * ============
 *  This subroutine extract a string from a fits line for a given key.
 *  It is up to a user to convert 'sout' into any type of data.
 *
 * Arguments:
 * ==========
 */
	struct icom *comp, /* (in) ptr to comments struct */
	char *key,         /* (in) Key charactor in fits header */
	char *sout)        /* (out) value for the key */
/*
 * Revision history:
 * =================
 * 1996 May (M. Sekiguchi): original creation
 *
 * ----------------------------------------------------------------------
 */
{
  char s[128],ss[128];	/* string for every thing */
  int   i;	/* for loop */
  char *cp;
  size_t keylen;
  
  keylen= strlen( key );

  for( i=(comp->ncom - 1); i >= 0;  i-- ) 
    {
      /*
	(void) imdecs( &comp->com[i][0], s );
      */
      strcpy(s,comp->com[i]);
      
      /* search for the key */
      if( strncmp( s, key, keylen ) != 0 ) continue;
      
      /* replace / by NULL */
      /* 1999/11/02 buggy, for string may contain /
	 if( (cp=strchr(s,'/')) != NULL ) *cp='\0';
      */
      
      
      /* search for '=' */
      if( (cp=strchr( s, '=' )) == NULL ) continue;
      
      /* search for ''' */
      cp++; sout[0]='\0'; ss[0]='\0';
      (void)sscanf(cp,"%s",ss);
      
      if( ss[0] != '\'' )
	{
	  (void)strcpy( sout, ss );
	} 
      else 
	{
	  /* 2002/07/30 merged */
	  imc_fits_extract_string(ss,sout);
	}
      return 1;		
    }
  return 0;
}

/* -------------------------------------------------------------- */
int imrep_fits( 
/* ----------------------------------------------------------------
 *
 * Description:	 <IM>age <REP>lace a <FITS> line 
 * ============
 *  imrep_fits replaces a FITS line in a comment structure.
 *  The traget line is keyed by "key" string.
 *  Unlike imaddcom, string s will be written as it is.
 *
 * Arguments:
 * ==========
 */
	struct icom *comp, /* (in/out) ptr to comment struct */
	char *key,         /* (in) key for line to be replaced */
	char *s)           /* ptr to your comment string */
/*
 * Return value:
 * =============
 *    1: ok
 *    0: No line is found with the given key
 *
 * Revision history:
 * =================
 * May 1996 (M. Sekiguchi): Original creation
 * Jun 1996 (M. Yagi): Revise (Use sprintf)
 *
 * ---------------------------------------------------------------
 */
{
  char ss[90];	/* FITs line */
  int ic;
  size_t keylen;

  keylen= strlen( key );
  
  for( ic=0; ic < comp->ncom;  ic++ ) 
    {

      /* 1999/11/02 
	 (void) imdecs( &comp->com[ic][0], ss );
      */ 
      strcpy(ss,comp->com[ic]);

      /* search for the key */
      if( strncmp( ss, key, keylen ) != 0 ) continue;
      
      sprintf(comp->com[ic],"%-75.75s%s",s,"              ");

#if 0
      sprintf(line,"%-75.75s%s",s,"              ");
      
      /* encode string */
      (void) imencs( &comp->com[ic][0], line );      
      /* add terminater string */
      comp->com[ic][IMH_COMSIZE-3] = ' ';	
      comp->com[ic][IMH_COMSIZE-1] = '\n';	
#endif
      
      return 1;
    }
  return 0;
}

int imrep_fitsf(struct icom *comp, 
		char *key,char *format, ... )
{
  char line[81]="";
  char format2[81];
  va_list va;

  va_start(va,format);
  sprintf(format2,"%-8.8s= %s",key,format);
  
  vsprintf(line, format2, va);
  va_end(va);

  return imrep_fits(comp,key,line);
}

int imupdate_fitsf(struct icom *comp, 
		   char *key,char *format, ... )
{
  char line[BUFSIZ]="";

  char format2[BUFSIZ]="";
  va_list va;

  va_start(va,format);
  sprintf(format2,"%-8.8s= %s",key,format);
  
  vsprintf(line, format2, va);
  va_end(va);

  if (imrep_fits(comp,key,line)==0)
    imadd_fits(comp,line);
  return 0;
}

void imaddcomf(struct icom *comp, char *format, ... )
{
  char line[BUFSIZ]="";
  va_list va;

  va_start(va,format);
  vsprintf(line, format, va);
  va_end(va);
  imaddcom(comp,line);
  return ;
}

void imaddhistf(struct icom *comp, char *format, ... )
{
  char line[BUFSIZ];
  va_list va;

  va_start(va,format);
  vsprintf(line, format, va);
  va_end(va);

  imaddhist(comp,line);
  return ;
}

int imget_fits_float_value
(struct icom *comp, char *key, float *fout)
{
  char tmp[BUFSIZ];
  if (imget_fits_value(comp,key,tmp)==1)
    {
      *fout=atof(tmp);
      return 1;
    }
  else
    {
      *fout=0;
      return 0;
    }
}

int imget_fits_int_value
(struct icom *comp, char *key, int *iout)
{
  char tmp[BUFSIZ];
  if (imget_fits_value(comp,key,tmp)==1)
    {
      *iout=atoi(tmp);
      return 1;
    }
  else
    {
      *iout=0;
      return 0;
    }
}

void imh_inherit(struct imh *imhin,
		 struct icom *icomin,
		 struct imh *imhout,
		 struct icom *icomout,
		 char* fnamout)
{
  float bzero,bscale;
  int dtype,pixoff;
  int pixignr;
  int u2off;
  int i;

  (void) memcpy( imhout,  imhin,  sizeof(struct imh) );
  (void) memcpy( icomout, icomin, sizeof(struct icom) );


  /*
    printf("%d\n",icomout->ncom);
    printf("%d\n",icomin->ncom);
  */

  if(icomout->ncom>0)
    {
      icomout->com=(char**)malloc(icomout->ncom*sizeof(char*));
      if(icomout->com==NULL)
	{
	  fprintf(stderr,"Cannot allocate memory for comment in imh_inherit\n");
	  exit(-1);
	}
      /*
	memcpy(icomout->com, icomin->com, icomout->ncom*sizeof(char*));
      */

      
      for(i=0;i<icomout->ncom;i++)
	{
	  icomout->com[i]=(char*)malloc(IMH_COMSIZE*sizeof(char));
	  /* 2000/09/19 */
	  if(icomout->com[i]==NULL)
	    {
	      fprintf(stderr,"Cannot allocate memory for comment line %d in imh_inherit\n",i);
	      
	      exit(-1);
	    }      
	  
	  memcpy(icomout->com[i],icomin->com[i],IMH_COMSIZE*sizeof(char));
	}
    }
  pixignr=imget_pixignr(imhin);

/*
  imset_pixignr(imhout,icomout,pixignr);
*/
#ifndef FORCEFITS
  if( strstr( fnamout,FITSEXTEN) != NULL ) 
    {
#endif
      imhout->dtype = DTYPFITS;
      switch ( imhin->dtype ) 
	{
#ifndef FORCEFITS
	case DTYPI2:
	  (void) imc_fits_set_dtype( imhout, FITSSHORT, 0.0, 1.0 );
	  break;
	case DTYPI4:
	  (void) imc_fits_set_dtype( imhout, FITSINT, 0.0, 1.0 );
	  break;
	case DTYPR4:
	  (void) imc_fits_set_dtype( imhout, FITSFLOAT, 0.0, 1.0 );
	  break;
	case DTYPU2:
	  (void) imc_fits_set_dtype( imhout, FITSSHORT, -SHRT_MIN, 1.0 );
	  break;
#endif
	case DTYPEMOS: /* Tenuki */
	  u2off=imget_u2off(imhin);
	  (void) imc_fits_set_dtype( imhout, FITSSHORT, 0.0 , 1.0 );
	  break;
	case DTYPFITS:
	  (void) imc_fits_get_dtype(imhin,&dtype,&bzero,&bscale,&pixoff);
	  (void) imc_fits_set_dtype( imhout, dtype, bzero, bscale);
	  break;
	default:
	  printf("Don't know how to convert input data type to FITS %d\n",
		 imhin->dtype);
	}
      if (imhin->dtype != DTYPFITS)
	{
	  /* 1999/11/03 re-format for fits standard here */	  
	  for(i=0;i<icomout->ncom;i++)
	    {
	      /* IRAF-B/P -> IRAF-B_P */
	      /*
		printf("%s\n",icomout->com[i]);
	      */

	      if(strncmp(icomout->com[i],"IRAF-B/P",8) == 0)
		{
		  icomout->com[i][6]='_'; 		  
		}

	      /*
	      imh_reformat_fits(icomout->com[i]);
	      */
	    }
	  
	}
#ifndef FORCEFITS
    }
  else
    {
      switch ( imhin->dtype ) 
	{
	case DTYPI2:
	case DTYPI4:
	case DTYPR4:
	case DTYPU2:
	  break;
	case DTYPEMOS: /*Tenuki*/
	  imhout->dtype=DTYPU2;
	  break;
	case DTYPFITS:
	  imc_fits_get_dtype(imhin,&dtype,&bzero,&bscale,&pixoff);
	  imset_u2off( imhout, icomout, pixoff );
	  switch (dtype)
	    {
	    case FITSSHORT:
	      if(bzero==0)
		imhout->dtype=DTYPI2;
	      else
		imhout->dtype=DTYPU2;
	      break;
	    case FITSINT:
	      imhout->dtype=DTYPI4;
	      break;
	    case FITSFLOAT:
	      imhout->dtype=DTYPR4;
	      break;
	    default:
	      /* not supported!! */
	      break;
	    }
	}
    }
#endif
}


float *imc_read( 
     struct imh *imhp,
     FILE *fp,
     int vx1,	/* smallest x */
     int vx2,	/* largest  x */
     int vy1,	/* smallest y */
     int vy2)	/* largest  y */
{
  static float *buffer=NULL,*line,*lignr;
  size_t buffsize;
  int npx,npx2,npy,npy2;
  int iy,ix;
  float pixignr;

  npx=imhp->npx;
  npy=imhp->npy;
  if( imhp->dtype == DTYPEMOS ) 
    return( imc_mos_read( imhp, vx1, vx2, vy1, vy2) );
  /*else*/
  pixignr=(float)imget_pixignr(imhp);

  npx2=(vx2 - vx1 + 1);
  npy2=(vy2 - vy1 + 1);
  buffsize= npx2 * npy2 * sizeof(float);

  buffer=(float*)malloc(buffsize);
  if(buffer==NULL)
    {
      printf("Cannot Allocate memory for buffer in imc_read\n");
      return NULL;
    }

  line=(float*)malloc((npx+npx2)*sizeof(float));
  if(line==NULL)
    {
      printf("Cannot Allocate memory for line in imc_read\n");
      free(buffer);
      return NULL;
    }

  lignr=line+npx;
  
  for(ix=0;ix<npx2;ix++) lignr[ix]=pixignr;  

  /* 1999/10/17 */
  /* revised for outside */
  if(vx1<0)
    for(iy=0;iy<npy2;iy++)
      {
	if(iy+vy1<0||iy+vy1>=npy)
	  {
	    for(ix=0;ix<npx2;ix++) 
	      buffer[ix+iy*npx2]=pixignr;
	  }
	else
	  {
	    memcpy(buffer+npx2*iy,lignr,-vx1*sizeof(float));
	    imrl_tor( imhp, fp, line, iy+vy1);
	    memcpy(buffer+npx2*iy-vx1,line,(npx2+vx1)*sizeof(float));
	  }	    
      }
  else if (vx1<npx)
    for(iy=0;iy<npy2;iy++)
      {
	if( iy + vy1< 0 || iy + vy1 >= npy )
	  memcpy(buffer+iy*npx2,lignr,sizeof(float)*npx2);
	else
	  {
	    imrl_tor( imhp, fp, line, iy+vy1);
	    memcpy(buffer+npx2*iy,line+vx1,npx2*sizeof(float));
	  }	    
      }
  else
    for(iy=0;iy<npy2;iy++)
      for(ix=0;ix<npx2;ix++)
	buffer[ix+npx2*iy]=pixignr;

  free(line);
  return buffer;
}


int imc_merge_icom(struct icom *icom1, struct icom *icom2,
		   int ncom0, char **key0)
{
  int i,j;
  char **key1,key2[9]="";
  int flag=0;
  int ncom1,ncom2;
  char *line;

  ncom1=icom1->ncom;
  key1=(char**)malloc(ncom1*sizeof(char*));
  for (j=0;j<ncom1;j++)
    {
      key1[j]=(char*)malloc(9*sizeof(char));
      memcpy(key1[j],icom1->com[j],8);
      key1[j][8]='\0';
    }
  
  ncom2=icom2->ncom;
  for(i=0;i<ncom2;i++)
    {
      line=icom2->com[i];
      if (line[8]!='=') 
	{
	  imadd_fits(icom1,line);
	  continue;
	}
      memcpy(key2,line,8);

      flag=0;
      for (j=0;j<ncom0;j++)
	{
	  if (strncmp(key2,key0[j],8)==0) 
	    {
	      flag=1;
	      break;
	    }
	}
      if(flag==1) continue;

      /* lookup */
      for (j=0;j<ncom1;j++)
	{
	  if (strncmp(key2,key1[j],8)==0) 
	    {
	      flag=1;
	      break;
	    }
	}
      if(flag==0) imadd_fits(icom1,line);
    }

  /* free */
  for (j=0;j<ncom1;j++)
    {
      free(key1[j]);
    }
  free(key1);
  return 0;
}


/* ----------------------- end of imc.c --------------------------- */
