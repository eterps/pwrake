#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "getargs.h"
#include "iscross.h"

double readHeader(POINT *p,FILE *fp,
		  double *scalex, 
		  double *scaley, 
		  double *ra, 
		  double *dec,
		  double *cosfactor, 
		  double *theta,
		  double *exptime)
{
  int NAXIS1=0,NAXIS2=0;
  double CRPIX1=0,CRPIX2=0,CRVAL1=0,CRVAL2=0;
  double CD1_1=0,CD1_2=0,CD2_1=0,CD2_2=0;

  /* 2002/06/05 */
  double CDELT1=0, CDELT2=0;
  double PC001001=1.0,PC001002=0.0,PC002001=0.0,PC002002=1.0;

  char buffer[81]="";

  int cdflag=0;

  double cfac;
  double dx,dy,Ax,Ay,Bx,By;
  double sx,sy;

  rewind(fp);
  while(1)
    {
      fread(buffer,1,80,fp);
      buffer[80]='\0';

      if(strncmp("END     ",buffer,8)==0)
	{
	  break;
	}
      else if(strncmp("NAXIS1  ",buffer,8)==0)
	{
	  NAXIS1=atoi(buffer+10);
	}
      else if(strncmp("NAXIS2  ",buffer,8)==0)
	{
	  NAXIS2=atoi(buffer+10);
	}
      else if(strncmp("CRPIX1  ",buffer,8)==0)
	{
	  CRPIX1=atof(buffer+10);
	}
      else if(strncmp("CRPIX2  ",buffer,8)==0)
	{
	  CRPIX2=atof(buffer+10);
	}
      else if(strncmp("CRVAL1  ",buffer,8)==0)
	{
	  CRVAL1=atof(buffer+10);
	}
      else if(strncmp("CRVAL2  ",buffer,8)==0)
	{
	  CRVAL2=atof(buffer+10);
	}
      else if(strncmp("CD1_1   ",buffer,8)==0)
	{
	  CD1_1=atof(buffer+10);
	  cdflag=1;
	}
      else if(strncmp("CD1_2   ",buffer,8)==0)
	{	  
	  CD1_2=atof(buffer+10);
	  cdflag=1;
	}
      else if(strncmp("CD2_1   " ,buffer,8)==0)
	{
	  CD2_1=atof(buffer+10);
	  cdflag=1;
	}
      else if(strncmp("CD2_2   ",buffer,8)==0)
	{
	  CD2_2=atof(buffer+10);
	  cdflag=1;
	}
      else if(strncmp("EXPTIME ",buffer,8)==0)
	{
	  *exptime=atof(buffer+10);
	}
      else if(strncmp("CDELT1  ",buffer,8)==0)
	{
	  CDELT1=atof(buffer+10);
	}
      else if(strncmp("CDELT2  ",buffer,8)==0)
	{
	  CDELT2=atof(buffer+10);
	}
      else if(strncmp("PC001001",buffer,8)==0)
	{
	  PC001001=atof(buffer+10);
	}
      else if(strncmp("PC001002",buffer,8)==0)
	{	  
	  PC001002=atof(buffer+10);
	}
      else if(strncmp("PC002001",buffer,8)==0)
	{
	  PC002001=atof(buffer+10);
	}
      else if(strncmp("PC002002",buffer,8)==0)
	{
	  PC002002=atof(buffer+10);
	}

    }
  
  cfac=cos(M_PI/180.0*CRVAL2);
  if (cfac<0.001) cfac=0.001;

  if(cdflag!=1)
    {
      CD1_1=CDELT1*PC001001;
      CD1_2=CDELT1*PC001002;
      CD2_1=CDELT2*PC002001;
      CD2_2=CDELT2*PC002002;
    }

  dx=CRPIX1*CD1_1+CRPIX2*CD1_2;
  dy=CRPIX1*CD2_1+CRPIX2*CD2_2;
  
  /*
    printf("%f %f\n",dx,dy);
  */
  Ax=NAXIS1*CD1_1;
  Ay=NAXIS1*CD2_1;
  Bx=NAXIS2*CD1_2;
  By=NAXIS2*CD2_2;
  
  sx=sqrt(CD1_1*CD1_1+CD1_2*CD1_2);
  sy=sqrt(CD2_1*CD2_1+CD2_2*CD2_2);
  /*
    printf("%f %f\n",sx,sy);
  */

  /*
    -(c*dx0+s*dy0)*0.5*(cfac1+cfac2),(-s*dx0+c*dy0)
  */

  /*
  printf("p0x %f %f %f\n",CRVAL1-dx/cfac,CRVAL1*cfac-dx,cfac);
  */

  p[0].x=-dx/sx;
  p[0].y=-dy/sy;

  p[1].x=(Ax-dx)/sx;
  p[1].y=(Ay-dy)/sy;
  
  p[2].x=(Ax+Bx-dx)/sx;
  p[2].y=(Ay+By-dy)/sy;
  
  p[3].x=(Bx-dx)/sx;
  p[3].y=(By-dy)/sy;

  /*
  theta=0.5*(atan2(CD1_2,-CD1_1)+atan2(CD2_1,CD2_2));
  */
  /* 2002/03/06 bug fix */
  /*
  printf("%e %e\n",CD1_2,CD1_1);
  */
  *theta=atan2(-CD1_2,-CD1_1);
  *scalex=sx;
  *scaley=sy;

  *ra=CRVAL1;
  *dec=CRVAL2;

  *cosfactor=cfac;
  return 0;
}

int main(int argc, char **argv)
{
  POINT p1[5];
  POINT p2[5];
  FILE *fp;

  double dx0,dy0,c,s;
  double t1=0,t2=0;
  double sx1,sy1,cfac1;
  double sx2,sy2,cfac2;
  double ra1,dec1;
  double ra2,dec2;
  double e1,e2;
  double dra,ddec;

  /*
    if (argc<3)
    {
    printf("Usage: %s FITS1 FITS2\n",argv[0]);
    exit(-1);
    }
  */
  char	fnamin1[BUFSIZ]="", fnamin2[BUFSIZ]="";
  getargopt opts[5];
  char *files[3]={NULL};
  int helpflag;

  files[0]=fnamin1;
  files[1]=fnamin2;

  setopts(&opts[0],"",0,NULL,NULL);
  helpflag=parsearg(argc,argv,opts,files,NULL);
  if(fnamin2[0]=='\0') helpflag=1;
  if(helpflag==1)
    {
      print_help("Usage: estmatch (file1) (file2)",
		 opts,
		 "");
      exit(-1);
    }

  /* ADD FITS READER */
  fp=fopen(fnamin1,"rb");
  if (fp==NULL) 
    {
      fprintf(stderr,"%s is not found\n",fnamin1);
      exit(-1);
    }
  readHeader(p1,fp,&sx1,&sy1,&ra1,&dec1,&cfac1,&t1,&e1); 
  fclose(fp);

  fp=fopen(fnamin2,"rb");
  if (fp==NULL) 
    {
      fprintf(stderr,"%s is not found\n",fnamin2);
      exit(-1);
    }
  readHeader(p2,fp,&sx2,&sy2,&ra2,&dec2,&cfac2,&t2,&e2); 
  fclose(fp);


  /*
  printf("%15.15f %15.15f\n",t1,t2);
  */

  /* 2001/06/20 debug */

  dra=(ra2-ra1);
  while (dra>180.0) dra-=180.0;
  while (dra<-180.0) dra+=180.0;

  dra/=sx1/cfac1;
  ddec=(dec2-dec1)/sy1;

  dx0=(p2[0].x-p1[0].x)+dra;
  dy0=(p2[0].y-p1[0].y)+ddec;

  c=cos(t1);
  s=sin(t1);
  /*
  printf("%f %f\n",p1[0].x,p2[0].x);
  */
  
  /*
    if(e1<1.e-6) e1=1.e-6;
  */
  if(e2<1.e-6) e2=1.e-6;

  printf("%f %f %f %f\n",-(c*dx0+s*dy0),
	 (-s*dx0+c*dy0),t1-t2,e2/e1);
  exit(0);
}  


