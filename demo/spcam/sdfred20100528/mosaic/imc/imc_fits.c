#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
/* For tcreate (Yagi added )*/
#include <time.h>

/* for test */
/* #define HAVE_FSEEKO */
#ifdef HAVE_FSEEKO
#include <sys/types.h>
#endif

/* ... local include files ... */

#include "imc.h"

/* ... Sun C uses non-standard arguments ... */
/* ... determine byte order ... */

#ifdef BYTE_REORDER
#define BSWAP 1
#else
#define BSWAP 0
#endif

/* ... define constants ... */

#define NBYTEBLK 2880		/* # of bytes per block */
#define MFITSH   720		/* # of default max header line */
#define NCHAFITS 80		/* # of character per FITS line */
#define KEYSIZ   8		/* # of char per for a key word */

#define DTOFF  12		/* offset from last void_2 for dtype */
#define BZOFF  16		/* offset from last void_2 for bzero */
#define BSCOFF 20		/* offset from last void_2 for bscale */
#define PXOFF  24		/* offset from last void_2 for pixoff */

#define EQL_LOC 8
#define SLH_LOC 31
#define MAX_COM NCHAFITS - SLH_LOC - 2
#define MAX_VAL SLH_LOC - EQL_LOC - 2

/* define buffer for io */

/*
#define WRKSIZ 131072
*/


#define CHAR_SIZE sizeof(char)
#define SHORT_SIZE sizeof(short)
#define INT_SIZE sizeof(int)
#define FLOAT_SIZE sizeof(float)

/*
static union {
  char  i1[WRKSIZ/CHAR_SIZE];
  short i2[WRKSIZ/SHORT_SIZE];
  int   i4[WRKSIZ/INT_SIZE];
  float r4[WRKSIZ/FLOAT_SIZE];
} fitsbuff;
*/

#define fitsbuff_i1(a) ((char*)(a))
#define fitsbuff_i2(a) ((short*)(a))
#define fitsbuff_i4(a) ((int*)(a))
#define fitsbuff_r4(a) ((float*)(a))

/* struct to save fits parameters
 * Please note that imc_fits for FITS file is not like
 * imh for IRAF file. We have one imh struct for every
 * each IRAF file, but we have only one imc_fits struct.
 * imc_fits is simply a templary stroage for FITS parameters.
 * All necessary paramters are transfered to imh when a FITS
 * file is opend. 
 */


static struct imc_fits {
  int   bitpix;
  int   naxis;
  int   naxis1;
  int   naxis2;
  int   naxis3;
  int   naxis4;
  char extend[NCHAFITS];
  float bscale;
  float bzero;
  char  obj[NCHAFITS];
  char  origin[NCHAFITS];
  char  date[NCHAFITS];
  char  stime[NCHAFITS];
  char  iraf_name[NCHAFITS];
  char  iraf_type[NCHAFITS];
  int   iraf_bp;
  int   pixoff;
} imc_fits;

/* ---------------------------------------------------------------------- */
FILE *imropen_fits( 
/* ----------------------------------------------------------------------
 *
 * Description:   <IM>age <R>ead <OPEN> <FITS>
 * ============
 *
 *      imropen_fits opens fits files, and reads
 *      their header contents into structures. 
 *	It returns a FILE pointer where the pixel
 *	data starts.
 *
 *      Note:
 *      1) FILE pointer IS MOVED to the start of pixel data
 *         by imropen_fits.
 *
 * Arguments:
 * ==========
 */
        char        *fnam,      /* (in) iraf file name without .imh or .pix */
        struct imh  *imhp,      /* (in) ptr to imh header structure */
        struct icom *comp)      /* (in) ptr to comment struct */
/*
 * Return  value:
 * ==============
 *      File pointer address where pixel data starts.
 *      Will be NULL pointer if faile to open files.
 *
 * Revision history:
 * =================
 *      Mar, 1993 (M. Sekiguchi): modification from imropen()
 *
 * ----------------------------------------------------------------------- */
{
  char *cp;
  char **fh;
  char /* key[KEYSIZ], eq[KEYSIZ],*/ strg[NCHAFITS];
  char imhnam[NCHAFITS], pixnam[NCHAFITS];
  /*  int pixignr; */
  int  i,nlin;
  int  odd, nblk;
  FILE *fp;
  struct tm tc={0};
  /* ... Open FITS file ... */
  /*  char command[BUFSIZ]; obsolate */

  int mfitsh=MFITSH;
  int maxcom=MFITSH;

  /* 2003/11/04 */
  imhp->buff=NULL;

  fh=(char **)malloc(sizeof(char*)*mfitsh);
  if(fh==NULL)
    {
      fprintf(stderr,"Cannot allocate memory for fh in imropen_fits\n");
      /* 2000/09/19 */
      /* mfitsh should be set smaller and retry*/
      /* but not implemented yet...*/
      exit(-1);
    }

  comp->com=(char **)malloc(sizeof(char*)*maxcom);
  /* 2000/09/19 */
  if(comp->com==NULL)
    {
      fprintf(stderr,"Cannot allocate memory for comment in imropen_fits\n");
      /* 2000/09/19 */
      /* maxcom should be set smaller and retry*/
      /* but not implemented yet...*/
      exit(-1);
    }

#if 1
  if( ( fp=fopen(fnam,"r") ) == NULL ) 
    {
      (void) printf( "imropen_fits: can't open %s\n", fnam );
      return NULL;
    }
#endif

/* Yagi modified for zipped file */
/* but fseek does not work well with popen */
/* Maybe whole read into char[] is needed.
   and fseek&fread -> memcpy(result,buffer+pos,size)
   it will be implemented as imropen_fits_zipped()  */
#if 0
  if( ( fp=fopen(fnam,"r") ) == NULL ) 
    {
      sprintf(command,"/bin/zcat %s\n",fnam);
      if((fp=popen(command,"rb"))==NULL)
	{
	  (void) printf( "imropen_fits: can't open %s\n", fnam );
	  return NULL;
	}
      else
	{
	  setbuf(fp,NULL);
	}
    }
#endif

/* ... Read in until "END" ... */

   nlin = 0;
   while( 1 ) {

	/* check # of lines */

	/* read a line */
	cp = fh[nlin]=(char*)malloc((NCHAFITS+1)*sizeof(char));
	if(fh[nlin]==NULL)
	  {
	    fprintf(stderr,"Cannot allocate memory for line %d in imropen_fits\n",nlin);
	    exit(-1);
	  }

        if( fread( cp, (int)NCHAFITS, (size_t)1, fp ) != 1 ) {
	  (void) printf("imropen_fits: can't read %s\n", fnam );
	  return NULL;
	}
	/* check first line */
	if( nlin == 0 ) {
	   if(strncmp(fh[nlin],"SIMPLE  ",(int)KEYSIZ) != 0) {
		(void) printf("imropen_fits: Not a FITS file\n");
		(void) printf("              1st line is not SIMPLE\n");
/*Yagi added for check*/
		printf("%c%c%c%c%c%c%c%c\n",
		       fh[nlin][0],
		       fh[nlin][1],
		       fh[nlin][2],
		       fh[nlin][3],
		       fh[nlin][4],
		       fh[nlin][5],
		       fh[nlin][6],
		       fh[nlin][7]);
		return NULL;
		}
	   }

	/* add strg terminater */
	fh[nlin][NCHAFITS] = '\0';

	/* check last line */
	nlin++;
	if(strncmp(fh[nlin-1],"END     ",(int)KEYSIZ) == 0) break;

	if (nlin>mfitsh)
	  {
	    mfitsh+=MFITSH;
	    fh=(char **)realloc(fh,sizeof(char*)*mfitsh);	    
	    if(fh==NULL && mfitsh!=0)
	      {
		fprintf(stderr,"Cannot reallocate memory for fh in imropen_fits\n");
		/* 2000/09/19 */
		/* mfitsh should be set smaller and retry*/
		/* but not implemented yet...*/
		exit(-1);
	      }

	  }


	} /* end of while */

 /* 
  for(i=0;i<nlin;i++) (void)printf("%s\n",fh[i]); 
 */

/* ... Decode parameters ... */
  
  imc_fits.obj[0]    = '\0';
  imc_fits.origin[0] = '\0';
  imc_fits.date[0]   = '\0';
  imc_fits.stime[0]   = '\0';
  imc_fits.bscale = 1.0;
  imc_fits.bzero  = 0.0;
  imc_fits.iraf_name[0]='\0';
  imc_fits.iraf_type[0]='\0';
  imc_fits.iraf_bp =0;
  comp->ncom = 0;
  comp->com[comp->ncom]=(char*)malloc(sizeof(char)*(IMH_COMSIZE+1));
  if(comp->com[comp->ncom]==NULL)
    {
      fprintf(stderr,"Cannot allocate memory for new comment in imropen_fits\n");
      exit(-1);
    }

  /*
     for(i=0;i<nlin;i++) (void)printf("%s\n",fh[i]); 
     */

   for( i=0; i<nlin-1; i++ ) 
     {
       /* we do not need look at the last line="END" */
       cp=fh[i];

      /* Simple */
      if( strncmp(cp,"SIMPLE  ",(int)KEYSIZ) == 0 ) 
	{
	  (void) sscanf(cp+9,"%s",strg);
	  if( strg[0] != 'T' ) {
	    (void) printf("imropen_fits: Only SIMPLE is supported\n");
	    return NULL ;
	  }
         }

      /* BITPIX */
      else if( strncmp(cp,"BITPIX  ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%d",&imc_fits.bitpix);
	 if( imc_fits.bitpix != FITSCHAR &&
	     imc_fits.bitpix != FITSSHORT &&
	     imc_fits.bitpix != FITSINT &&
	     imc_fits.bitpix != FITSFLOAT ) {
	       (void) printf("imropen_fits: Unsupported BITPIX = %d\n",
			      imc_fits.bitpix);
	       return NULL;
	       }
	 }

      /* NAXIS */
      else if( strncmp(cp,"NAXIS   ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%d",&imc_fits.naxis);
	 /* 2004/10/26 */
	 imc_fits.naxis1=1;
	 imc_fits.naxis2=1;
	}

      /* NAXIS1 */
      else if( strncmp(cp,"NAXIS1  ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%d",&imc_fits.naxis1);
	}

      /* NAXIS2 */
      else if( strncmp(cp,"NAXIS2  ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%d",&imc_fits.naxis2);
	}

     /* NAXIS3 */
      else if( strncmp(cp,"NAXIS3  ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%d",&imc_fits.naxis3);
	}

      /* NAXIS4 */
      else if( strncmp(cp,"NAXIS4  ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%d",&imc_fits.naxis4);
	}

      /* BSCALE */
      else if( strncmp(cp,"BSCALE  ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%e",&imc_fits.bscale);
	}

      /* BZERO */
      else if( strncmp(cp,"BZERO   ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%e",&imc_fits.bzero);
	}

      /* EXTEND */
      else if( strncmp(cp,"EXTEND  ",(int)KEYSIZ) == 0 ) {
	 (void) sscanf(cp+9,"%s",imc_fits.extend);
	}

      /* OBJECT */
      /* 2002/07/31 */
      /*
      else if( strncmp(cp,"OBJECT  ",(int)KEYSIZ) == 0 ) {
	(void) imc_fits_extract_string( cp, imc_fits.obj );
	}
      */

      /* ORIGIN */
      /*
	else if( strncmp(cp,"ORIGIN  ",(int)KEYSIZ) == 0 ) {
	(void) imc_fits_extract_string( cp, imc_fits.origin );
	}
      */

      /* DATE */
      else if( strncmp(cp,"OBS-DATE",(int)KEYSIZ) == 0 ) {
	(void) imc_fits_extract_string( cp, imc_fits.date );
	}

      /* TIME */
      else if( strncmp(cp,"STRT-TIM",(int)KEYSIZ) == 0 ) {
	(void) imc_fits_extract_string( cp, imc_fits.stime );
	}

      /* IRAF-NAME */
      else if( strncmp(cp,"IRAFNAME",(int)KEYSIZ) == 0 ) {
	(void) imc_fits_extract_string( cp, imc_fits.iraf_name );
	(void) sscanf( imc_fits.iraf_name, "%s", imc_fits.iraf_name );
	}

      /* IRAF-TYPE */
      else if( strncmp(cp,"IRAFTYPE",(int)KEYSIZ) == 0 ) {
/*
	 (void) imc_fits_extract_string( cp, imc_fits.iraf_type );
	 (void) imencs( &comp->com[comp->ncom][0], cp );
*/
	 (void) imc_fits_extract_string( comp->com[comp->ncom], imc_fits.iraf_type );
	 	 
	 comp->ncom++;
	 comp->com[comp->ncom]=(char*)malloc(sizeof(char)*(IMH_COMSIZE+1));
	 if(comp->com[comp->ncom]==NULL)
	   {
	     fprintf(stderr,"Cannot allocate memory for new comment in imropen_fits\n");
	     exit(-1);
	   }

       }

      /* IRAF-B/P */
      else if( strncmp(cp,"IRAF-B/P",(int)KEYSIZ) == 0 ) 
	{
	  (void) sscanf(cp+9,"%d",&imc_fits.iraf_bp);
/*
	  (void) imencs( &comp->com[comp->ncom][0], cp );
*/
	  strcpy(comp->com[comp->ncom],cp);
	  comp->ncom++;
	  comp->com[comp->ncom]=(char*)malloc(sizeof(char)*(IMH_COMSIZE+1));
	  if(comp->com[comp->ncom]==NULL)
	    {
	      fprintf(stderr,"Cannot allocate memory for new comment in imropen_fits\n");
	      exit(-1);
	    }

	}

      /* others */
      else {
	/* encode into IRAF comments */
/*
        (void) imencs( &comp->com[comp->ncom][0], cp );
*/
	strcpy(comp->com[comp->ncom], cp );
	comp->ncom++;
	comp->com[comp->ncom]=(char*)malloc(sizeof(char)*(IMH_COMSIZE+1));
	if(comp->com[comp->ncom]==NULL)
	  {
	    fprintf(stderr,"Cannot allocate memory for new comment in imropen_fits\n");
	    exit(-1);
	  }
      }

       if( comp->ncom >= maxcom )
	 {
	   maxcom+=MFITSH;
	   comp->com=realloc(comp->com,sizeof(char*)*maxcom);
	   if(comp->com==NULL)
	     {
	       fprintf(stderr,"Cannot reallocate memory for comment in imropen_fits\n");
	       exit(-1);
	     }
	 }
   } /* end of loop */

  free(comp->com[comp->ncom]);

  /* 2002/07/20 */
  if(comp->ncom>0)
    {
      comp->com=realloc(comp->com,sizeof(char*)*comp->ncom);
      if(comp->com==NULL)
	{
	  fprintf(stderr,"Cannot reallocate memory for comment in imropen_fits\n");
	  exit(-1);
	}
    }
  else
    {
      free(comp->com);
      comp->com=NULL;
    }


/* ... built IRAF header ... */

   (void) sprintf(imhnam,"%s%s",imc_fits.iraf_name,".imh");
   (void) sprintf(pixnam,"%s%s%s","HDR$/",imc_fits.iraf_name,".pix");
   (void) imencs0( imhp->id, "imhdr" );
   (void) imencs0( imhp->pixnam, pixnam );
   (void) imencs0( imhp->imhnam, imhnam );
   /* 2002/07/31 */
   /*
   (void) imencs0( imhp->title, imc_fits.obj );
   (void) imencs0( imhp->institute, imc_fits.origin );
   */

   imhp->naxis = imc_fits.naxis;
   imhp->npx   = imc_fits.naxis1;
   imhp->npy   = imc_fits.naxis2;
   imhp->ukwn1 = 1;
   imhp->ukwn2 = 1;
   imhp->ukwn3 = 1;
   imhp->ukwn4 = 1;
   imhp->ukwn5 = 1;
   imhp->ndatx = imhp->npx;
   imhp->ndaty = imhp->npy;
   imhp->ukwn6 = 1;
   imhp->ukwn7 = 1;
   imhp->ukwn8 = 1;
   imhp->ukwn9 = 1;
   imhp->ukwn10= 1;
   imhp->off1  = 0x201;
   imhp->off2  = 0x201;
   imhp->off3  = 0x201;

   /******************** 96 Jul 10 **********/
   /* how to manage with imh.tcreate ??*/
   /* The unit is in second and offset is from 1980. */
#define TOFF 315500400  /* offset to get JST */
   tc.tm_isdst=-1;
   
   /* Now not considering differences in time 
      between observatory(eg.Chile) and JST */
   
   /*
   tc.tm_sec=0;
   tc.tm_min=0;
   tc.tm_hour=0;
   */
   tc.tm_wday=0;
   tc.tm_yday=1;
   tc.tm_mday=1;
   tc.tm_mon=1;
   tc.tm_year=70;
   /*
     tc.tm_gmtoff=32400L;
   */

   /* 2003/07/22 for glibc-2.2.2 strtok bug */

   sscanf(imc_fits.stime,"%d%*[: ]%d%*[: ]%d",
	  &(tc.tm_hour),&(tc.tm_min),&(tc.tm_sec));
   sscanf(imc_fits.date,"%d%*[- ]%d%*[- ]%d",
	  &(tc.tm_mday),&(tc.tm_mon),&(tc.tm_year));
   tc.tm_mon-=1;

  imhp->tcreate=mktime(&tc)-TOFF;
  /*
    fprintf(stderr,"%s %d\n",asctime(&tc),imhp->tcreate);
  */
  
  /* calculate header size */
  
  /************************************************************/
  /*
    i = sizeof( *imhp ) + comp->ncom * sizeof( comp->com );
  */
  i = sizeof( *imhp ) + comp->ncom * IMH_COMSIZE;
  
  /************************************************************/

   if( (i % 16) == 0 ) i = i/4;
   else                i = 4 * (i/16 + 1);
   imhp->imhsize = i;

   /* data type = fits */
   imhp->dtype = DTYPFITS;

   /* locate pixel data */
   odd = (nlin * NCHAFITS) % NBYTEBLK;
   nblk= (nlin * NCHAFITS) / NBYTEBLK;
   if (odd == 0) imc_fits.pixoff =  nblk    * NBYTEBLK;
   else		 imc_fits.pixoff = (nblk+1) * NBYTEBLK;

   if( fseek(fp,(int)imc_fits.pixoff,SEEK_SET) != 0 ) {
	(void) printf("imropn_fits: Error fseek to pix data\n");
	return NULL;
	}


   /* 2003/11/04 */
   /* encode some parameter into unsed imh location */
   for(i=0;i<4;i++) 
     imhp->void_2[VID2SIZ-PXOFF-i]=fitsbuff_i1(&(imc_fits.pixoff))[i]; 

   (void)imc_fits_set_dtype( imhp, imc_fits.bitpix, 
                             imc_fits.bzero, imc_fits.bscale );


 /* Restore PIXIGNR from FITS comments */
 (void) imrestore_pixingr( imhp, comp );


  /* free is needed */ 
  for(i=0;i<nlin;i++)
    free(fh[i]);
  free(fh);

  return( fp );
}

/* 2000/07/02 */
/* from skysb */
int imc_fits_dtype_string(char *dtype)
{
  int fitsdtype=-1;
  
  if(dtype!=NULL&&dtype[0]!='\0')
    {
      if (strstr(dtype,"SHORT"))
	{
	  fitsdtype=FITSSHORT;
	}
      else if (strstr(dtype,"INT"))
	{
	  fitsdtype=FITSINT;
	}
      else if (strstr(dtype,"CHAR"))
	{
	  fitsdtype=FITSCHAR;
	}
      else if (strstr(dtype,"FLOAT"))
	{
	  fitsdtype=FITSFLOAT;
	}
    }
  return fitsdtype;
}


/* --------------------------------------------------------------- */
int imc_fits_set_dtype( 
/* ---------------------------------------------------------------
 *
 * Description:
 * ============
 *   imc_fits_set_dtype() sets data type, bzero and bscale of
 *   a fits file. Normally if a FITS file is read, these are
 *   set by imropen(). A user has to call imc_fits_set_dtype()
 *   when writing a fits file out.
 *
 * Note: data_in_program = bzero + bscale * data_in_file
 *
 * Argumets:
 * =========
 */
        struct imh *imhp, /* (I/O) pntr to imh strcut */
	int dtype,  	/* (in) fits data type */
	float bzero,	/* (in) fits offset */
	float bscale)	/* (in) fits scale */
/*
 * ---------------------------------------------------------------
 */
{
  int i;

  if( dtype != FITSCHAR  &&
     dtype != FITSSHORT &&
     dtype != FITSINT   &&
     dtype != FITSFLOAT ) return 0;

  if( bscale == 0.0 ) return 0;
  
  /* 
     fitsbuff_i4(imhp->buff)[0] = dtype;
     fitsbuff_r4(imhp->buff)[1] = bzero;
     fitsbuff_r4(imhp->buff)[2] = bscale;
     
     for( i=0; i<12; i++ )
     {
     imhp->void_2[VID2SIZ - DTOFF - i] = fitsbuff_i1(imhp->buff)[i];
     }
  */
  
  for( i=0; i<4; i++ )
    imhp->void_2[VID2SIZ - DTOFF - i] = fitsbuff_i1(&dtype)[i];
  for( i=4 ;i<8; i++ )
    imhp->void_2[VID2SIZ - DTOFF - i] = fitsbuff_i1(&bzero)[i-4];
  for( i=8 ; i<12; i++ )
    imhp->void_2[VID2SIZ - DTOFF - i] = fitsbuff_i1(&bscale)[i-8];

   

  return 1;
}

/* --------------------------------------------------------------- */
   void imc_fits_get_dtype(
/* ---------------------------------------------------------------
 *
 * Description:
 * ============
 *   imc_fits_set_dtype() reads data type, bzero and bscale of
 *   a fits file from imh. Normally if a FITS file is read, these
 *   are set by imropen().
 *
 * Argumets:
 * =========
 */
        struct imh *imhp,  /* (in) pntr to imh strcut */
	int *dtype,  	   /* (out) fits data type */
	float *bzero,	   /* (out) fits offset */
	float *bscale,	   /* (out) fits scale */
	int *pixoff)       /* (out) fits pixel data offset */
/*
 * ---------------------------------------------------------------
 */
{
  int i;
  char buff[16]="";

  for( i=0; i<16; i++ )
    {
      fitsbuff_i1(buff)[i] = imhp->void_2[VID2SIZ - DTOFF - i];
    }

  /* 2000/07/02 revise, check NULL */
  if(dtype!=NULL)
    *dtype  = (int)fitsbuff_i4(buff)[0];

  if(bzero!=NULL)
    *bzero  = (float)fitsbuff_r4(buff)[1];

  if(bscale!=NULL)
    *bscale = (float)fitsbuff_r4(buff)[2];

  if(pixoff!=NULL)
    *pixoff = (int)fitsbuff_i4(buff)[3];
  
  return;
}

/* --------------------------------------------------------------- */
FILE *imwopen_fits (
/* ---------------------------------------------------------------
 *
 * Description:  <IM>age <W>rite <OPEN>
 * ============
 *
 *      imwopen_fits() writes the header portion of a fits
 *      file. It does not fill pixel data but returns a FILE
 *      pointer where the pixel data should start.
 *
 *      Note:
 *      0) The File name SHOULD BE xxx.fits
 *      1) pix file is NOT closed by imwopen. Someone should close it.
 *      2) FILE pointer IS MOVED to the start of pixel data by imwopen.
 *
 * Condition prior to call:
 * ========================
 *
 *      There are many paremeters in imh header to be filled. 
 *	imwopen_fits() fills most of them for you. You have to
 *      fill the following parameters prior to call imwopen:
 *
 *      imhp->dtype     iraf data type
 *      imhp->npx       x image size
 *      imhp->npy       y image size
 *
 *      If you like to add more comments, please do so using
 *      imaddcom.
 *
 * Arguments:
 * ==========
 */
        char        *fnam,      /* (in) file name WITH .fits */
        struct imh  *imhp,      /* (in) ptr to imh header structure */
        struct icom *comp)      /* (in) ptr to comment struct */
/*
 * Return value:
 * =============
 *      File pointer address where pixel data starts.
 *      Will be NULL pointer if faile to open files.
 *
 * Caution:
 * ========
 *      (1) imwopwn DOES OVERWRITE the existing files with the 
 *	    same name. Users should check the existing files 
 *          if needed.
 *
 * ---------------------------------------------------------------
 */
{
  char /**cp,*/ zero=' ';
  char **fh;

  char temp[NCHAFITS+1];
  int i, nlin;
  int dtype;
  int odd, nzero;
  int pixoff;
#ifdef HAVE_FSEEKO
  off_t nblk;
  off_t filloff;
  off_t size;
  off_t total_size;
#else
  int nblk;
  int filloff;
  int size;
  int total_size;
#endif
  float bzero;
  float bscale;
  FILE *fp;
  int mfitsh=MFITSH;
  /* Yagi added */
  /*
    char key[10],val[80],com[80];
    float fval;
    int ival;
  */

  /* 2003/11/04 */
  imhp->buff=NULL;

  fh=(char**)malloc(sizeof(char*)*mfitsh);
  if(fh==NULL)
    {
      fprintf(stderr,"Cannot allocate memory for fh in imwopen_fits\n");
      /* 2000/09/19 */
      /* mfitsh should be set smaller and retry*/
      /* but not implemented yet...*/
      exit(-1);
    }

  for(i=0;i<mfitsh;i++)
    {
      fh[i]=(char*)malloc(sizeof(char)*(NCHAFITS+1));
      if(fh[i]==NULL)
	{
	  fprintf(stderr,"Cannot allocate memory for fh[%d] in imwopen_fits\n",i);
	  exit(-1);
	}


      /* fill space */
      memset((&fh[i][0]),' ',(NCHAFITS+1));
    }
  /*
   * ... check input paramters
   */
  
  /* ... check FITS date type ... */
  if( imhp->dtype != DTYPFITS ) {
    (void) printf("Imwopen_fits: wrong imh.dtype\n");
    return NULL;
  }
  
#ifndef FORCEFITS
  /* ... check file name ... */
  if( strstr( fnam, FITSEXTEN ) == NULL ) {
    (void) printf("Imwopen_fits: file name should be xx.fits\n");
    return NULL;
  }
#endif
  
  /* ... check npx & npy ... */
  if( imhp->npx < 0 || imhp->npy < 0 ) {
    (void) printf("Imwopen_fits: npx or npy < 0\n");
    return NULL;
  }
  
  if( imhp->npx == 0 && imhp->npy == 0 ) {
    (void) printf("Imwopen_fits: npx = npy =0\n");
    return NULL;
  }
  
  /* ... Check dtype ... */
  (void)imc_fits_get_dtype(imhp,&dtype,&bzero,&bscale,&pixoff);
  /* printf("debug: 2006-08-02 %f %f\n",bzero,bscale); */
  
  if( dtype != FITSCHAR  &&
     dtype != FITSSHORT &&
     dtype != FITSINT   &&
     dtype != FITSFLOAT ) {
    (void) printf("Imwopen_fits: Unkown BITPIX %d\n",dtype);
    (void) printf("Make sure to use pointer of imhd for imc_fits_set_dtype()\n");
    return NULL;
  }
  
  /* ... Determine Naxis ... */
  imhp->naxis = 0;
  if( imhp->npx > 0 ) imhp->naxis++ ;
  if( imhp->npy > 0 ) imhp->naxis++ ;
  
  
  /*
   * ... make FITS header
   */
  
  
  nlin = 0;
  (void)imc_fits_mklog  (fh[nlin++],"SIMPLE",(int)1,"FITS STANDARD");
  (void)imc_fits_mkint  (fh[nlin++],"BITPIX",dtype,"FITS BITS/PIXEL");
  (void)imc_fits_mkint  (fh[nlin++],"NAXIS",(int)imhp->naxis,"NUMBER OF AXES");
  (void)imc_fits_mkint  (fh[nlin++],"NAXIS1",(int)imhp->npx,"NUMBER OF PIX IN 1ST AXIS");
  (void)imc_fits_mkint  (fh[nlin++],"NAXIS2",(int)imhp->npy,"NUMBER OF PIX IN 2ND AXIS");
  
  /* 2006/08/02 */
  /* here we should add EXTEND keyword */
  (void)imc_fits_mklog  (fh[nlin++],"EXTEND",(int)0,"FITS dataset may contain extensions");
  /* write only NON-EXTEND FITS */

  if(dtype!=FITSFLOAT || bscale!=1.0 || bzero!=0.0)
    {
      (void)imc_fits_mkfloat(fh[nlin++],"BSCALE",bscale,
			     "REAL = FILE*BSCALE + BZERO");
      (void)imc_fits_mkfloat(fh[nlin++],"BZERO",bzero,
			     "REAL = FILE*BSCALE + BZERO");
    }
  /* 2002/07/31 */
  /*
    (void)imdecs( imhp->title, strg );
    (void)imc_fits_mkstrg (fh[nlin++],"OBJECT",strg,"Name of object");
    (void)imdecs( imhp->institute, strg );
    (void)imc_fits_mkstrg (fh[nlin++],"ORIGIN",strg,"Origin");
  */

  /* add iraf comments */
  for(i=0; i<comp->ncom; i++) {
    /* format check needed here (98/06/24)*/
    /*
       (void) imdecs( comp->com[i], strg);
       (void) sprintf( fh[nlin++], "%-80.80s", strg);
    */
    (void) sprintf( fh[nlin++], "%-80.80s", comp->com[i]);
  }
  
  /* ternimate line */
  memset(temp,' ',NCHAFITS);
  (void) sprintf( fh[nlin++], "END%77.77s", temp);
  
  /*
   * ... Open & write to file
   */
  
  if( (fp=fopen(fnam,"w")) == NULL ) {
    (void) printf("Imwopen_fits: can not open %s\n",fnam);
    return NULL;
  }
  
  for( i=0; i<nlin; i++ ) 
    {
      if( fwrite(fh[i],(int)NCHAFITS,(size_t)1,fp) != 1 ) 
	{
	  (void) printf("Imwopen_fits: error writing header\n");
	  return NULL;
	}
    }
  
  /*
   * ... Locate pixel data
   */
  
  odd = (nlin * NCHAFITS) % NBYTEBLK;
  nblk= (nlin * NCHAFITS) / NBYTEBLK;
  if (odd == 0) pixoff =  nblk    * NBYTEBLK;
  else          pixoff = (nblk+1) * NBYTEBLK;
  nzero= pixoff - nlin * NCHAFITS;
  
  /*
    fprintf(stderr,"%d %d %d\n",pixoff,nblk,nlin);
    */
  
  /* write empty data to fill block */
  for( i=0; i<nzero; i++ ) {
    if( fwrite(&zero,(size_t)1,(size_t)1,fp) != 1 ) {
      (void) printf("Imwopen_fits: error filling block\n");
      return NULL;
    }
  }
  
  /*
   * ... Fill Dummy Pix Data
   */
  
  /* pix data byte size */
#ifdef HAVE_FSEEKO
  size = (off_t)(abs(dtype)/8) * (off_t)(imhp->npx) * (off_t)(imhp->npy);
  odd  = size % (off_t)NBYTEBLK;
  nblk = size / (off_t)NBYTEBLK;

  /* debug */
  /*
    printf("debug: sizeof(off_t)=%d\n",sizeof(off_t));
    printf("debug: size=%f\n",(double)size);
    printf("debug: odd=%f\n",(double)odd);
    printf("debug: nblk=%f\n",(double)nblk);
  */
#else
  size = abs(dtype)/8 * imhp->npx * imhp->npy;
  odd  = size % NBYTEBLK;
  nblk = size / NBYTEBLK;
#endif

  /* 2007/05/01 revised */
  
#ifdef HAVE_FSEEKO
  total_size = (off_t)(nblk+1) * (off_t)NBYTEBLK;
#else
  total_size = (nblk+1) * NBYTEBLK;
#endif

  zero='\0';

  if( odd != 0 ) 
    {
#ifdef HAVE_FSEEKO
  filloff = (off_t)pixoff + size;
  if( fseeko(fp,filloff,SEEK_SET) != 0 ) 
#else
    filloff = pixoff + size;
  if( fseek(fp,filloff,SEEK_SET) != 0 ) 
#endif
    {
      (void) printf("Imwopen_fits: Error in fseek to dummy data\n");
      return NULL;
    }
      /* wirte zeros */
      nzero      = total_size - size;
      for( i=0; i<nzero; i++ ) {
	if( fwrite(&zero,(int)1,(size_t)1,fp) != 1 ) {
	  (void) printf("Imwopen_fits: error filling dummy data\n");
	  return NULL;
	}
      }
    } /* end if */
  else
    {
      /* put last data */
#ifdef HAVE_FSEEKO
  filloff = (off_t)pixoff+size-1;
  if( fseeko(fp,filloff,SEEK_SET) != 0 ) 
#else
    filloff = pixoff+size-1;
  if( fseek(fp,filloff,SEEK_SET) != 0 ) 
#endif
    {
      (void) printf("Imwopen_fits: Error in fseek to dummy data\n");
      return NULL;
    }
      if( fwrite(&zero,(int)1,(size_t)1,fp) != 1 ) {
	(void) printf("Imwopen_fits: error filling dummy data\n");
	return NULL;
      }
    }

  /* relocate to first real data */
  if( fseek(fp,pixoff,SEEK_SET) != 0 ) {
    (void) printf("Imwopen_fits: Error in fseek to 1st data\n");
    for(i=0;i<mfitsh;i++)
      free(fh[i]);
    free(fh);
    return NULL;
  }
  
  /*
   * ... Fill some parametes
   */
  
  for(i=0;i<4;i++) 
    imhp->void_2[VID2SIZ-PXOFF-i]=fitsbuff_i1(&pixoff)[i];

  /* free fh */

  for(i=0;i<mfitsh;i++)
    free(fh[i]);
  free(fh);
  
  return( fp );
}


/* ------------------------------------------------------------------------ */
int imc_fits_rl( 
/* --------------------------------------------------------------------------
 *
 * Description: <IMC> <FITS> <R>ead <L>ine
 * ============
 *      imc_fits_rl reads line data from a FITS file into a given
 *      real*4 arrray. It can read the following data types:
 *
 *      signed short ( integer*2 )
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
 * Revision history:
 * =================
 *
 * --------------------------------------------------------------------------
 */
{
  size_t size;
  int dtype;
  int pixoff;
  int nbyte;
  int ix;
#ifdef HAVE_FSEEKO
  off_t offset;
#else
  int offset;
#endif
  
  float *rp;
  float bzero;
  float bscale;
  int pixignr;
  
  /*
   * ... Check Input Parameters
   */
  
  /* ... check if this is a fits type ... */
  if( imhp->dtype != DTYPFITS ) {
    (void)printf("Imc_fits_rl: This is not FITS type\n");
    return 0;
  }
  
  if( imhp->npx == 0 ) {
    (void) printf("Imc_fits_rl: npx=0\n");
    return 0;
  }
  
  (void)imc_fits_get_dtype( imhp, &dtype, &bzero, &bscale, &pixoff );
  nbyte= abs(dtype/8);
  
  size = imhp->npx * nbyte;
  
  /*
    
    if( size > WRKSIZ ) {
    (void) printf("Imc_fits_rl: Internal buffer too small\n");
    return 0;
    }
  */
  
  
  /* 2003/11/04 */
  imhp->buff=realloc(imhp->buff,size);
  
  /* ... Reposition file pointer ... */
  
  /* offset is in byte */
#ifdef HAVE_FSEEKO
  offset = (off_t)pixoff + (off_t)iy * (off_t)nbyte * (off_t)(imhp->ndatx);
  if( fseeko(fp,offset,SEEK_SET) != 0 ) 
#else
    offset = pixoff + iy * nbyte * imhp->ndatx;
  if( fseek(fp,offset,SEEK_SET) != 0 ) 
#endif
    {
      (void) printf("Imc_fits_rl: Error in fseeking\n");
      printf("pixoffset=%d offset=%ld\n",pixoff,(long)offset);
      return 0;
    }
  
  /*
   * ... Read Data In
   */
  
  pixignr=imget_pixignr(imhp);
  
  switch ( dtype ) {
    
    /* ============ */ 
  case FITSCHAR:
    /* ============ */ 

      if( fread((imhp->buff),size,(size_t)1,fp) != 1 ) goto err;
      if( bscale == 1.0 ) {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
            *rp = bzero + (float)fitsbuff_i1(imhp->buff)[ix];
         }
      else {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
            *rp = bzero + bscale * (float)fitsbuff_i1(imhp->buff)[ix];
         }
      break;

  /* ============ */ 
   case FITSSHORT:
  /* ============ */ 

      if( fread((imhp->buff),size,(size_t)1,fp) != 1 ) goto err;
      if(BSWAP) (void)imc_fits_swap16((imhp->buff),(int)(size/SHORT_SIZE)); 
      if( bscale == 1.0 ) {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
	    *rp = bzero + (float)fitsbuff_i2(imhp->buff)[ix];
	 }
      else {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
	    *rp = bzero + bscale * (float)fitsbuff_i2(imhp->buff)[ix];
         }
      break;

  /* ============ */ 
   case FITSINT:
  /* ============ */ 

      if( fread((imhp->buff),size,(size_t)1,fp) != 1 ) goto err;
      if(BSWAP) (void)imc_fits_swap32((imhp->buff),(int)(size/INT_SIZE)); 
      if( bscale == 1.0 ) {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
	    *rp = bzero + (float)fitsbuff_i4(imhp->buff)[ix];
	 }
      else {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
	    *rp = bzero + bscale * (float)fitsbuff_i4(imhp->buff)[ix];
         }
      break;

  /* ============ */ 
   case FITSFLOAT:
  /* ============ */ 

      if( fread((imhp->buff),size,(size_t)1,fp) != 1 ) goto err;
      if(BSWAP) (void)imc_fits_swap32((imhp->buff),(int)(size/FLOAT_SIZE)); 
      if( bscale == 1.0 ) {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
	    *rp = bzero + (float)fitsbuff_r4(imhp->buff)[ix];
	 }
      else {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
	    *rp = bzero + bscale * (float)fitsbuff_r4(imhp->buff)[ix];
	 }
      break;

 /* ====== */
   default:
 /* ====== */

      (void) printf("Imc_fits_rl: Unsupported data type = %d\n", dtype);
      return 0;
      /* break; */

   } /* end switch */

return 1;

err:
(void) printf("Imc_fits_rl: Error reading pixel data\n");
return 0;
}

/* ------------------------------------------------------------------------ */
int imc_fits_wl(
/* --------------------------------------------------------------------------
 *
 * Description: <IMC> <FITS> <W>rite <L>ine
 * ============
 *      imc_fits_wl() writes real*4 data in a line to a pixel file.
 *      It can write the following data types:
 *
 *	signed char  ( integer*1 )
 *      signed short ( integer*2 )
 *      signed int  ( integer*4 )
 *      float  ( real*4 )
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
        int         iy)         /* (in) line # starting with 0 */
/*
 * Note:
 * =====
 *      imc_fits_wl() loads pixel data in an internal array 
 *      whose size is 64K in u_short. The xdim should not exceed
 *      this size.
 *
 * Caution:
 * ========
 *      imwl_fits_wl() writes data into a pixel file according to:
 *
 *              imhp->npx, imhp->npy,
 *              imhp->ndatx, imhp->ndaty.
 *
 *      Since the FITS header is already written when you call
 *      imwopen_fits(), no one can change or adjust ndatx or ndaty.
 *      Please be carfull not to change ndatx or ndaty after
 *      calling imwopen_fits().
 *
 * Revision history:
 * =================
 *    Apr 25, 93: (Maki Sekiguchi) Original creation
 *
 * -----------------------------------------------------------------
 */
{
size_t  size;           /* data size (in byte) to read */

#ifdef HAVE_FSEEKO
off_t pntr_offset;
#else
int pntr_offset;
#endif

int pixignr;
int dtype;
int pixoff;
int ix;
int nbyte;
float *rp;
float bzero;
float bscale;
float bzero_up;

/*
 * ... Check Input Parameters
 */

/* ... check FITS date type ... */
if( imhp->dtype != DTYPFITS ) {
    (void) printf("Imc_fits_wl: wrong imh.dtype\n");
    return( (int)0 );
    }

if( imhp->npx == 0 ) {
   (void) printf("Imc_fits_wl:npx =0\n");
   return( (int)0 );
   }

/* ... Check dtype ... */
(void)imc_fits_get_dtype(imhp,&dtype,&bzero,&bscale,&pixoff);

if( dtype != FITSCHAR  &&
    dtype != FITSSHORT &&
    dtype != FITSINT   &&
    dtype != FITSFLOAT ) {
   (void) printf("Imc_fits_wl: Unknown BITPIX %d",dtype);
   return( (int)0 );
   }

if( bscale == 0.0 ) {
   (void) printf("Imc_fits_wl: Bscale=0\n");
   return( (int)0 );
   }

/*... check work area size */
nbyte= abs(dtype/8);
size = imhp->npx * nbyte;
bzero_up = bzero - 0.5; 

/*
if( size > WRKSIZ ) {
   (void) printf("Imc_fits_wl: Internal buffer too small\n");
   return( (int)0 );
   }
*/
 imhp->buff=realloc(imhp->buff,size);

/*
 * ... Locate pixel pntr
 */

/* offset is in byte */
#ifdef HAVE_FSEEKO
pntr_offset = (off_t)pixoff + (off_t)iy * (off_t)nbyte * (off_t)(imhp->npx);
if( fseeko(fp,pntr_offset,SEEK_SET) != 0 ) 
#else
pntr_offset = pixoff + iy * nbyte * imhp->npx;
if( fseek(fp,pntr_offset,SEEK_SET) != 0 ) 
#endif
  {
   (void) printf("Imc_fits_rl: Error in fseeking\n");
   return( (int)0 );
   }

/*
 * ... Write Data Out
 */
pixignr=imget_pixignr(imhp);

switch ( dtype ) {
  
  /* ============*/
   case FITSCHAR:
  /* ============*/

      if( bscale == 1.0 ) {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
	     fitsbuff_i1(imhp->buff)[ix] = (char)floor( *rp-bzero_up );
         }
      else {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
            fitsbuff_i1(imhp->buff)[ix] = (char)floor( (*rp-bzero)/bscale+0.5);
         }
      if( fwrite((imhp->buff),size,(size_t)1,fp) != 1 ) goto err;
      break;

  /* ============*/
   case FITSSHORT:
  /* ============*/

      if( bscale == 1.0 ) {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
            fitsbuff_i2(imhp->buff)[ix] = (short)floor( (*rp-bzero_up) );
         }
      else {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
            fitsbuff_i2(imhp->buff)[ix] = (short)floor( (*rp-bzero)/bscale+0.5 );
         }
      if(BSWAP) (void)imc_fits_swap16((imhp->buff),(int)(size/SHORT_SIZE)); 
      if( fwrite((imhp->buff),size,(size_t)1,fp) != 1 ) goto err;
      break;

  /* ============*/
   case FITSINT:
  /* ============*/

      if( bscale == 1.0 ) {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
            fitsbuff_i4(imhp->buff)[ix] = (int)floor(*rp-bzero_up);
         }
      else {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
            fitsbuff_i4(imhp->buff)[ix] = (int)floor( (*rp-bzero)/bscale+0.5);
         }
      if(BSWAP) (void)imc_fits_swap32((imhp->buff),(int)(size/INT_SIZE)); 
      if( fwrite((imhp->buff),size,(size_t)1,fp) != 1 ) goto err;
      break;

  /* ============*/
   case FITSFLOAT:
  /* ============*/

      if( bscale == 1.0 ) {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
            fitsbuff_r4(imhp->buff)[ix] = (float)(*rp - bzero);
         }
      else {
         for(ix=0,rp=dp;ix<imhp->npx;ix++,rp++)
	   {
	     fitsbuff_r4(imhp->buff)[ix] = (float) ((*rp - bzero) / bscale);
	   }
         }
      if(BSWAP) 
	(void)imc_fits_swap32((imhp->buff),(int)(size/FLOAT_SIZE));
      if( fwrite((imhp->buff),size,(size_t)1,fp) != 1 ) goto err;
      break;

  /* ======*/
   default:
  /* ======*/

      (void) printf("Imc_fits_wl: Unsupported data type = %d\n", dtype);
      return 0;
      /* break; */

   } /* end switch */

return 1;

err:
(void) printf("Imc_fits_wl: Error writing pixel data\n");
return( (int)0 );
}

		
/* --------------------------------------------------------------- */
void imc_fits_extract_string( 
/* ---------------------------------------------------------------
 *
 * Arguments:
 * ==========
 */
	char *fl,	/* (in) fits line */
	char *s)	/* (out) extracted string */
/*
 *
 * ---------------------------------------------------------------
 */
{
  char *cp1, *cp2, *cp3, *cp;
  
  /* default, copy all */

  /* 2002/07/30 revised*/

  /* 1999/11/02 changed */
  cp1= strchr( fl, '\'' );
  if(cp1!=NULL)
    {
      cp2= strchr( cp1+1, '\'' );
      while(cp2!=NULL)
	{
	  cp3=strchr( cp2+1, '\'' );
	  if(cp3==NULL)
	    {
	      *cp2= '\0';
	      cp2=NULL;
	    }
	  else
	    {
	      cp2++;
	      if(cp3==cp2)
		{
		  /* memmove */
		  for(;*cp3!='\0';cp3++)
		    *(cp3)=*(cp3+1);
		}
	    }
	}
      cp=cp1+1;
    }
  else
    cp=fl;

  (void)strcpy( s, cp);
  return;
}



/* ----------------------------------------------------------- */
void imc_fits_mkint( 
/* -----------------------------------------------------------
 *
 */
	char *line,	/* (out) fits line */
	char *key,	/* (in) key */
	int  val,	/* (in) integer value */
	char *com)	/* (in) comment */
/*
 * -----------------------------------------------------------
 */
{
  sprintf(line,"%-8.8s= %20d / %-47.47s",key,val,com);
}
   
/* ----------------------------------------------------------- */
void imc_fits_mkfloat(
/* -----------------------------------------------------------
 *
 */
	char *line,	/* (out) fits line */
	char *key,	/* (in) key */
	float val,	/* (in) float value */
	char *com)	/* (in) comment */
/*
 * -----------------------------------------------------------
 */
{
  sprintf(line,"%-8.8s= %20E / %-47.47s",key,val,com);
}
   
/* ----------------------------------------------------------- */
void imc_fits_mklog(
/* -----------------------------------------------------------
 *
 */
	char *line,	/* (out) fits line */
	char *key,	/* (in) key */
	int val,	/* (in) logical */
	char *com)	/* (in) comment */
/*
 * -----------------------------------------------------------
 */
{
   if( val )
     sprintf(line,"%-8.8s= %20.20s / %-47.47s",key,"T",com);
   else
     sprintf(line,"%-8.8s= %20.20s / %-47.47s",key,"F",com);
 }

/* ----------------------------------------------------------- */
void imc_fits_mkstrg_val(
/* -----------------------------------------------------------
 *
 */
	char *line,	/* (out) fits value */
	char *strg)	/* (in) string */
/*
 * -----------------------------------------------------------
 */
{
  /* 2002/07/31 '' should be treated ... */
  char tmp[160];
  int i,j;

  j=0;
  for(i=0;i<80;i++)
    {
      if (strg[i]=='\0')
	{
	  tmp[j]='\0';
	  break;
	}
      if (strg[i]<0x20) 
	tmp[j++]=' ';
      else if(strg[i]!='\'') 
	tmp[j++]=strg[i];
      else
	{ 
	  tmp[j++]='\'';
	  tmp[j++]='\'';
	}
    }
  sprintf(line,"%c%-8s%c ",'\'',tmp,'\'');
  return;
}

/* ----------------------------------------------------------- */
void imc_fits_mkstrg(
/* -----------------------------------------------------------
 *
 */
	char *line,	/* (out) fits line */
	char *key,	/* (in) key */
	char *strg,	/* (in) string */
	char *com)	/* (in) comment */
/*
 * -----------------------------------------------------------
 */
{
  /* 2002/07/30 '' should be treated ... */

  char tmp[2*NCHAFITS];
  char tmp2[2*NCHAFITS];
  char tmp3[161];


  imc_fits_mkstrg_val(strg,tmp3);
  
  sprintf(tmp,"%-8.8s= %s ",key,tmp3);
  sprintf(tmp2,"%-30.80s / %-47.47s",tmp,com);
  
  sprintf(line,"%80.80s",tmp2);
}


/* -------------------------------------------- */
void imc_fits_swap16 (
/* -------------------------------------------- */
void *dat,
int nword)
{
char *cf, *ct;
int i;
short *idat, buf;

   ct  = (char *)&buf;
   for(i=0,idat=dat; i<nword; i++,idat++) {

	cf  = (char *)idat;

	ct[0] = cf[1];
	ct[1] = cf[0];

	*idat = buf;

   }
}


/* -------------------------------------------- */
void imc_fits_swap32 (
/* -------------------------------------------- */
		      void *dat,
		      int nword)
{
char *cf, *ct;
int i;
int *idat, buf;

   ct  = (char *)&buf;
   for(i=0,idat=dat; i<nword; i++,idat++) {

	cf  = (char *)idat;

	ct[0] = cf[3];
	ct[1] = cf[2];
	ct[2] = cf[1];
	ct[3] = cf[0];

	*idat = buf;

   }
}

int imh_reformat_fits(char *line)
{
  char key[9]="",strg[81]="",com[50]="";
  char quote;
  int pos;
  char cp[IMH_COMSIZE]="";

  /* logical */
  /* not needed now */
  strncpy(cp,line,80);
  cp[80]='\0';

  /* 2002/05/07 */
  if (strncmp(cp,"COMMENT ",8)==0 ||
      strncmp(cp,"HISTORY ",8)==0)
    {
      sprintf(line,"%-80.80s",cp);
      return 1;
    }

  /*string*/
  if(sscanf(cp,"%[-A-Z0-9_ ]%*[=] %*[\']%[^\']%c%n",key,strg,&quote,&pos)==3)
    {
      sscanf(cp+pos,"%*[ /]%[\040-\177]",com);
      imc_fits_mkstrg (line,key,strg,com);     
      return 1;
    }
  /*null string*/
  if(sscanf(cp,"%[-A-Z0-9_ ]%*[=] %[\']%n",key,strg,&pos)==2)
    {
      sscanf(cp+pos,"%*[ /]%[\040-\177]",com);
      imc_fits_mkstrg (line,key," ",com);     
      return 1;
    }
  /*number*/
  else 
    if(sscanf(cp,"%[-A-Z0-9_ ]%*[=] %[-+0-9.E]%n",key,strg,&pos)==2)
      {
	sscanf(cp+pos,"%*[ /]%[\040-\177]",com);
	/* make line here...*/
	sprintf(line,"%-8.8s= %20.20s / %-47.47s",key,strg,com);
	return 2;
      }
  else
    {
      sscanf(cp,"%[-A-Z0-9_ ]%*[=] %[^/]%*[ /]%[\040-\177]",
	     key,strg,com);
      imc_fits_mkstrg (line,key,strg,com);     
      return -1;
    }
}
