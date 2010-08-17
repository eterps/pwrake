#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "getargs.h"
#include "iscross.h"

int readWCS(POINT *p,FILE *fp)
{
  int NAXIS1=0,NAXIS2=0;
  float CRPIX1=0,CRPIX2=0,CRVAL1=0,CRVAL2=0;
  float CD1_1=0,CD1_2=0,CD2_1=0,CD2_2=0;
  float CDELT1=0, CDELT2=0;
  float PC001001=1.0,PC001002=0.0,PC002001=0.0,PC002002=1.0;

  char buffer[81]="";

  int cdflag=0;

  float cfac;
  float dx,dy,Ax,Ay,Bx,By;
  
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
      else if(strncmp("CDELT1  ",buffer,8)==0)
	{
	  CDELT1=atof(buffer+10);
	}
      else if(strncmp("CDELT2  ",buffer,8)==0)
	{
	  CDELT2=atof(buffer+10);
	}

      else if(strncmp("CD1_1   ",buffer,8)==0)
	{
	  CD1_1=atof(buffer+10);
	  cdflag=1;
	}
      else if(strncmp("CD1_2   ",buffer,8)==0)
	{	  
	  CD1_2=atof(buffer+10);
	}
      else if(strncmp("CD2_1   " ,buffer,8)==0)
	{
	  CD2_1=atof(buffer+10);
	}
      else if(strncmp("CD2_2   ",buffer,8)==0)
	{
	  CD2_2=atof(buffer+10);
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

  if(cdflag!=1)
    {
      CD1_1=CDELT1*PC001001;
      CD1_2=CDELT1*PC001002;
      CD2_1=CDELT2*PC002001;
      CD2_2=CDELT2*PC002002;
    }
  dx=CRPIX1*CD1_1+CRPIX2*CD1_2;
  dy=CRPIX1*CD2_1+CRPIX2*CD2_2;
  
  Ax=NAXIS1*CD1_1;
  Ay=NAXIS1*CD2_1;
  Bx=NAXIS2*CD1_2;
  By=NAXIS2*CD2_2;
  
  p[0].x=CRVAL1-dx/cfac;
  p[0].y=CRVAL2-dy;
  
  p[1].x=CRVAL1+(Ax-dx)/cfac;
  p[1].y=CRVAL2+(Ay-dy);
  
  p[2].x=CRVAL1+(Ax+Bx-dx)/cfac;
  p[2].y=CRVAL2+(Ay+By-dy);
  
  p[3].x=CRVAL1+(Bx-dx)/cfac;
  p[3].y=CRVAL2+(By-dy);
  return 0;
}

int main(int argc, char **argv)
{
  int i,j;
  int ret;
  POINT p1[5];
  POINT p2[5];
  FILE *fp;

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
      print_help("Usage: overlap2 (fits1) (fits2)",
		 opts,
		 "");
      exit(-1);
    }

  /* ADD FITS READER */
  fp=fopen(fnamin1,"rb");
  if (fp==NULL) 
    {
      fprintf(stderr,"Error: Input file %s is not found\n",fnamin1);
      exit(-1);
    }
  readWCS(p1,fp); 
  fclose(fp);

  fp=fopen(fnamin2,"rb");
  if (fp==NULL) 
    {
      fprintf(stderr,"%s is not found\n",fnamin2);
      exit(-1);
    }
  readWCS(p2,fp); 
  fclose(fp);

  p1[4]=p1[0];
  p2[4]=p2[0];
  
  for(i=0;i<4;i++)
    {
      for(j=0;j<4;j++)
	{
	  ret=-1;
	  ret=isCross(&p1[i],&p1[i+1],&p2[j],&p2[j+1]);
	  if(ret==1) 
	    {
	      printf("0\n");
	      exit(0); /*OK*/
	    }
	}
    }
  printf("1\n"); /*NG*/
  exit(1);
}
