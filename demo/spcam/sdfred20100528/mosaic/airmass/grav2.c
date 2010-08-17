#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

int main(int argc, char *argv[])
{
  char key[10]="";
  FILE *fp;
  char buffer[81]="";
  char val[70]="";
  int i,j;
  char flag=0;
  
  double a1=1.7330e-4, a3=-2.09e-7, a5=7.95e-10;
  int hh,dd,mm;
  float ss;

  double ZD0=-1.,ZD1=-1.;
  double LST0=-1.,LST1=-1.;
  double LST;
  double RA,DEC;
  char telname[BUFSIZ];
  double LATI,ZD,a,b,c,COSA,SINA,E,A;

  if (argc!=2)
    {
      printf("Usage: airmass2 FITS [dtheta]\n");
      exit(-1);
    }

  fp=fopen(argv[1],"r");
  if (NULL==fp)
    {
      printf("Cannot find file \"%s\"\n",argv[1]);
      exit(-1);
    }

  while(!feof(fp))
    {
      if (80!=fread(buffer, 1, 80, fp))
	{
	  /* error */
	  /* printf("EEEEEEEROOOORRR\n"); */
	  exit(-1);
	}
      /*
	buffer[80]='\0';
	printf("%s\n",buffer);
      */

      /*
	if (strncmp("END                                                                     ",buffer,80)==0)
      */
      if (strncmp("END     ",buffer,8)==0)
	{
	  break;
	}
      else if (strncmp("LST-STR =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%*[ \']%2d%*[:]%2d%*[:]%f",&hh,&mm,&ss);
	  LST=(double)hh+(double)mm/60.+ss/3600.;
	}   
      else if (strncmp("ZD-STR  =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%lf",&ZD0);
	}   
      else if (strncmp("ZD-END  =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%lf",&ZD1);
	}   
      else if (strncmp("RA      =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%*[ \']%2d%*[:]%2d%*[:]%f",&hh,&mm,&ss);
	  /* 
	     printf("%d %d %f\n",hh,mm,ss);
	     printf("[%s]\n",buffer+9);
	  */
	  RA=(double)hh+(double)mm/60.+ss/3600.;
	}   
      else if (strncmp("DEC     =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%*[ \']%c%2d%*[:]%2d%*[:]%f",&flag,&dd,&mm,&ss);
	  if (flag=='-')
	    DEC=-((double)dd+(double)mm/60.+ss/3600.);
	  else
	    DEC=((double)dd+(double)mm/60.+ss/3600.);
	}   
      else if (strncmp("TELESCOP=",buffer,9)==0)
	{
	  sscanf(buffer+9,"%s",telname);
	  if ((strncmp(telname,"'Subaru",7)!=0)&&
	      (strncmp(telname,"'SUBARU",7)!=0))
	    {
	      printf(" Error: TELESCOP=%s is not supported. quit.\n",
		     telname);
	      exit(-1);
	    }
	  else
	    {
	      LATI=+19.+49./60.+42.6/3600.;
	    }
	}   
    }

  if (ZD0>0&&ZD1>0)
    {
      ZD=0.5*(ZD0+ZD1);
    }
  /*
    printf("%f %f\n",RA,DEC);
  */
  a=(90.0-LATI)*M_PI/180.0;
  b=ZD*M_PI/180.0;
  c=(90.0-DEC)*M_PI/180.0;
  
  /*
    printf("%f %f %f\n",a,b,c);
  */  
  if(b==0)
    {
      printf(" Error: Cannot calculate. ZD=0.\n");
      exit(-1);
    }
  if(c==0)
    {
      printf(" Error: Cannot calculate. Dec=90.\n");
      exit(-1);
    }
  
  COSA=(cos(a)-cos(b)*cos(c))/(sin(b)*sin(c));
  
  /*
    printf("%f\n",COSA);
  */

  SINA=sqrt(1-COSA*COSA);
  
  /* BUG!! */
  E=RA-LST+24.0;
  while  (E>12.0) {E-=24.0;}
  if(E>0) {SINA=-SINA;}; /* object is EAST */
  
  /*
    printf("%f\n",SINA);
    TANA=SINA/COSA;
  */

  A=atan2(SINA,COSA)/M_PI*180;
  printf("%f %f\n",A,b);
  
  return 0;
}
