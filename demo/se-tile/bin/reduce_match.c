#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <time.h>

#include "fitsio.h"
#include "wcs.h"
#include "coord.h"

/* Structure used to store relevant */
/* information about a FITS file    */

#define MAXSTR  1000
#define MAXGETS 10000
#define HDRLEN  80000

#define MAXLINE 100000

int  debug = 0;

/* Structure used to store relevant */
/* information about a FITS file    */

struct FitsFile
{
   fitsfile         *fptr;
   long              naxes[2];
   struct WorldCoor *wcs;
   int               sys;
   double            epoch;
   int               clockwise;
};

FILE    *fstatus;


// reduce_match

#define SPACE 0
#define WORD 1

#define COL_LON   15
#define COL_LAT   16
#define COL_ERR   2
#define NCOLS     16

struct CatalogRow {
    int line;
    int near;
    int reject;
    double lon;
    double lat;
    double err;
    double min_ang;
    int min_idx;
};


/***********************************/
/*                                 */
/*  Print out FITS library errors  */
/*                                 */
/***********************************/

void printFitsError(int status)
{
   char status_str[FLEN_STATUS];

   fits_get_errstatus(status, status_str);

   fprintf(fstatus, "[struct stat=\"ERROR\", status=%d, msg=\"%s\"]\n", status, status_str);

   exit(1);
}



/******************************/
/*                            */
/*  Print out general errors  */
/*                            */
/******************************/

void printError(char *msg)
{
   fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"%s\"]\n", msg);
   exit(1);
}



/* stradd adds the string "card" to a header line, and */
/* pads the header out to 80 characters.               */

int stradd(char *header, char *card)
{
   int i;

   int hlen = strlen(header);
   int clen = strlen(card);

   for(i=0; i<clen; ++i)
      header[hlen+i] = card[i];

   if(clen < 80)
      for(i=clen; i<80; ++i)
         header[hlen+i] = ' ';

   header[hlen+80] = '\0';

   return(strlen(header));
}


/**************************************************/
/*                                                */
/*  Read a FITS file and extract some of the      */
/*  header information.                           */
/*                                                */
/**************************************************/

int readFits(char *filename, struct FitsFile *input)
{
   int       status=0;

   char      errstr[MAXSTR];

   int       sys;
   double    epoch;

   char  *input_header;

   int hdu=0;

   double offset=0;


   /*****************************************/
   /* Open the FITS file and get the header */
   /* for WCS setup                         */
   /*****************************************/

   if(fits_open_file(&input->fptr, filename, READONLY, &status))
   {
      sprintf(errstr, "Image file %s missing or invalid FITS", filename);
      printError(errstr);
   }

   if(hdu > 0)
   {
      if(fits_movabs_hdu(input->fptr, hdu+1, NULL, &status))
	 printFitsError(status);
   }

   if(fits_get_image_wcs_keys(input->fptr, &input_header, &status))
      printFitsError(status);


   /************************/
   /* Open the weight file */
   /************************/

   /*
   if(haveWeights)
   {
      if(fits_open_file(&weight.fptr, weightfile, READONLY, &status))
      {
	 sprintf(errstr, "Weight file %s missing or invalid FITS", weightfile);
	 printError(errstr);
      }

      if(hdu > 0)
      {
	 if(fits_movabs_hdu(weight.fptr, hdu+1, NULL, &status))
	    printFitsError(status);
      }
   }
   */


   /****************************************/
   /* Initialize the WCS transform library */
   /****************************************/

   if(debug >= 3)
   {
      printf("Input header to wcsinit() [input.wcs]:\n%s\n", input_header);
      fflush(stdout);
   }

   input->wcs = wcsinit(input_header);

   if(input->wcs == (struct WorldCoor *)NULL)
   {
      fprintf(fstatus, "[struct stat=\"ERROR\", msg=\"Input wcsinit() failed.\"]\n");
      exit(1);
   }

   input->wcs->nxpix += 2 * offset;
   input->wcs->nypix += 2 * offset;

   input->wcs->xrefpix += offset;
   input->wcs->yrefpix += offset;

   input->naxes[0] = input->wcs->nxpix;
   input->naxes[1] = input->wcs->nypix;


   /***************************************************/
   /*  Determine whether these pixels are 'clockwise' */
   /* or 'counterclockwise'                           */
   /***************************************************/

   input->clockwise = 0;

   if((input->wcs->xinc < 0 && input->wcs->yinc < 0)
   || (input->wcs->xinc > 0 && input->wcs->yinc > 0)) input->clockwise = 1;

   if(debug >= 3)
   {
      if(input->clockwise)
         printf("Input pixels are clockwise.\n");
      else
         printf("Input pixels are counterclockwise.\n");
   }


   /*************************************/
   /*  Set up the coordinate transform  */
   /*************************************/

   if (input->wcs->syswcs == WCS_J2000)
   {
      sys   = EQUJ;
      epoch = 2000.;

      if(input->wcs->equinox == 1950.)
         epoch = 1950;
   }
   else if(input->wcs->syswcs == WCS_B1950)
   {
      sys   = EQUB;
      epoch = 1950.;

      if(input->wcs->equinox == 2000.)
         epoch = 2000;
   }
   else if(input->wcs->syswcs == WCS_GALACTIC)
   {
      sys   = GAL;
      epoch = 2000.;
   }
   else if(input->wcs->syswcs == WCS_ECLIPTIC)
   {
      sys   = ECLJ;
      epoch = 2000.;

      if(input->wcs->equinox == 1950.)
      {
         sys   = ECLB;
         epoch = 1950.;
      }
   }
   else
   {
      sys   = EQUJ;
      epoch = 2000.;
   }

   input->sys   = sys;
   input->epoch = epoch;

   return 0;
}


int read_file(char *file, struct FitsFile *input, struct CatalogRow *cat)
{
    char s[MAXGETS];
    FILE *fp;

    int i, count, len, line, nobj, status;
    int apos[1000];
    int alen[1000];
    char buf[100];
    int offscl;
    double x, y;


    struct CatalogRow c;

    if ((fp = fopen(file, "r")) == NULL) {
	fprintf(stderr,"file open error : %s\n",file);
	exit(EXIT_FAILURE);
    }

    line = nobj = 0;

    while (fgets(s, MAXGETS, fp) != NULL) {
	status = SPACE;
	count = 0;
	//puts(s);
	for (i=0; i<MAXGETS; i++) {
	    //putc(s[i],stdout);
	    //printf("i=%d s[i]='%c' count=%d\n",i,s[i],count);
	    if (s[i]=='\0' || s[i]=='#') {
		if (status==WORD) {
		    alen[count] = len;
		    count++;
		}
		break;
	    }
	    switch (s[i]) {
	    case ' ':
	    case '\t':
		if (status==WORD) {
		    alen[count] = len;
		    //strncpy(buf,&s[apos[count]],alen[count]);
		    //buf[alen[count]] = '\0';
		    //printf("count=%i buf=%s\n",count,buf);
		    count++;
		    len = 0;
		    status = SPACE;
		}
	    break;
	    default:
		if (status==SPACE) {
		    apos[count] = i;
		    status = WORD;
		}
		len++;
	    }
	}
	if (count >= NCOLS) {
	    c.line = line;
	    strncpy(buf,&s[apos[COL_LON]],alen[COL_LON]);
	    c.lon = atof(buf);
	    strncpy(buf,&s[apos[COL_LAT]],alen[COL_LAT]);
	    c.lat = atof(buf);
	    strncpy(buf,&s[apos[COL_ERR]],alen[COL_ERR]);
	    c.err = atof(buf);

	    wcs2pix(input->wcs, c.lon, c.lat, &x, &y, &offscl);
	    //printf("lon=%f lat=%f x=%f y=%f offscl=%d\n",c.lon,c.lat,x,y,offscl);
	    /* 0 if within bounds, else off scale */
	    if (offscl==0) {
		//printf("line=%d lon=%f lat=%f x=%f y=%f offscl=%d\n",c.line,c.lon,c.lat,x,y,offscl);
		cat[nobj++] = c;
	    }
	}
	//if (line>2) exit(0);
	line++;
    }
    fclose(fp);

    return nobj;
}


//double tol_ang = 4.0/60/60; // 4 arcsec

void
xmatch(struct CatalogRow *cat1, int n1, struct CatalogRow *cat2, int n2, double tol_ang)
{
    int i, j, line;
    double x1, y1, z1;
    double x2, y2, z2;
    double cos_lat;
    double d;

    for (i=0; i<n1; i++) {
	cat1[i].min_ang = 180;
	cat1[i].min_idx = -1;
    }
    for (j=0; j<n2; j++) {
	cat2[j].min_ang = 180;
	cat2[j].min_idx = -1;
    }

    //while (fgets(s, MAXGETS, fp) != NULL) {
    for (i=0; i<n1; i++) {
	cos_lat = cos(cat1[i].lat*D2R);
	x1 = cos_lat * cos(cat1[i].lon*D2R);
	y1 = cos_lat * sin(cat1[i].lon*D2R);
	z1 = sin(cat1[i].lat*D2R);

	for (j=0; j<n2; j++) {
	    cos_lat = cos(cat2[j].lat*D2R);
	    x2 = cos_lat * cos(cat2[j].lon*D2R);
	    y2 = cos_lat * sin(cat2[j].lon*D2R);
	    z2 = sin(cat2[j].lat*D2R);
	    d = acos(x1*x2 + y1*y2 + z1*z2);
	    if (d < tol_ang) {
		if (d < cat1[i].min_ang) {
		    cat1[i].min_ang = d;
		    cat1[i].min_idx = j;
		}
		if (d < cat2[j].min_ang) {
		    cat2[j].min_ang = d;
		    cat2[j].min_idx = i;
		}
	    }
	}
    }
}


void
reduce_dup(char *ifile, char *ofile, struct CatalogRow *cat1, int n1, struct CatalogRow *cat2, int n2, const int flag)
{
    char s[MAXGETS];
    FILE *fpi, *fpo;
    int reject;

    int i=0, j, line=0, nrej=0, nmatch=0;

    if ((fpi = fopen(ifile, "r")) == NULL) {
	fprintf(stderr,"file open error for read : %s\n",ifile);
	exit(EXIT_FAILURE);
    }

    if ((fpo = fopen(ofile, "w")) == NULL) {
	fprintf(stderr,"file open error for write : %s\n",ofile);
	exit(EXIT_FAILURE);
    }

    while (fgets(s, MAXGETS, fpi) != NULL) {
	reject = 0;
	if (i < n1) {
	    if (line == cat1[i].line) {
		j = cat1[i].min_idx;
		if (j >= 0) {
		    if (cat2[j].min_idx == i) {
			nmatch++;
			//printf("line=%d i=%d j=%d\n", line, i, j);
			// crossmatch cames off
			if (flag) {
			    if (cat1[i].err >= cat2[j].err) {
				reject = 1;
				nrej++;
				//puts(s);
			    }
			} else {
			    if (cat1[i].err > cat2[j].err) {
				reject = 1;
				nrej++;
				//puts(s);
			    }
			}
		    }
		}
		i++;
	    }
	}
	if (!reject) fputs(s, fpo);
	line++;
    }

    fclose(fpi);
    fclose(fpo);

    printf("nmatch=%d\n",nmatch);
    printf("nrej=%d\n",nrej);
}



int main(int argc, char **argv)
{
    struct FitsFile input1, input2;
    char input_file1[MAXSTR];
    char input_file2[MAXSTR];

    struct CatalogRow *cat1, *cat2;
    int n1, n2;

    double tol_ang;

    /***************************************/
    /* Process the command-line parameters */
    /***************************************/

    fstatus = stdout;

    if (argc!=8) {
	fprintf(stderr,"usage: %s distance_in_arcsec input_cat1 input_cat2 fits1 fits2 output_cat1 output_cat2\n",argv[0]);
	exit(1);
    }

    tol_ang = atof(argv[1])/60/60 * D2R;

    strcpy(input_file1, argv[4]);
    strcpy(input_file2, argv[5]);

    readFits(input_file1, &input1);
    readFits(input_file2, &input2);

    cat1 = (struct CatalogRow*)malloc(sizeof(struct CatalogRow*)*MAXLINE);
    cat2 = (struct CatalogRow*)malloc(sizeof(struct CatalogRow*)*MAXLINE);

    n1 = read_file(argv[2], &input2, cat1);
    printf("n1=%d\n",n1);
    n2 = read_file(argv[3], &input1, cat2);
    printf("n2=%d\n",n2);

    xmatch(cat1, n1, cat2, n2, tol_ang);

    reduce_dup(argv[2], argv[6], cat1, n1, cat2, n2, 0);
    reduce_dup(argv[3], argv[7], cat2, n2, cat1, n1, 1);

    return 0;
}
