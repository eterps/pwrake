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
  int i,j,flag=0;
  char instname[BUFSIZ];
  
  double a1=1.7330e-4, a3=-2.09e-7, a5=7.95e-10;

  double ZD0=-1.,ZD1=-1.;
  double INR0=-1000.0,INR1=-1000.0;
  double sec2z, tan2z;
  double a,b,z,t,PA,cP,sP,ct,st,b11,b12,b21,b22;

  if (argc<2)
    {
      printf("Usage: airmass2 FITS [dtheta]\n");
      exit(-1);
    }

  if (argc==2) 
    {
      t=0;
    }
  else
    {
      t=atof(argv[2]);
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
      else if (strncmp("ZD-STR  =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%lf",&ZD0);
	}   
      else if (strncmp("ZD-END  =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%lf",&ZD1);
	}   
      else if (strncmp("INR-STR =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%lf",&INR0);
	}   
      else if (strncmp("INR-END =",buffer,9)==0)
	{
	  sscanf(buffer+9,"%lf",&INR1);
	}   
      else if (strncmp("INSTRUME=",buffer,9)==0)
	{
	  sscanf(buffer+9,"%s",instname);
	  if (strncmp(instname,"'SuprimeCam'",12)!=0)
	    {
	      printf(" Error: INSTRUME=%s is not supported. quit.\n",
		     instname);
	      exit(-1);
	    }
	}   
    }

  /* printf("debug0\n"); */

  if (INR0>-360&&INR1>-360)
    {
      PA=(0.5*(INR0+INR1)-90.)/180.*M_PI;
    }
  else
    {
      exit(1);
    }
  
  /* printf("debug1\n"); */

  if (ZD0>0&&ZD1>0)
    {
      z=0.5*(ZD0+ZD1)/180.*M_PI;
    }
  else
    {
      exit(1);
    }

  /* printf("debug2\n"); */
  /*
    # printf("%f %f\n",PA/PI*180,z);
  */

  /*
    ## r(") = A1*tan(z) + A3*tan^3(z) + A5*tan^5(z) 
    ## r(rad) = a1*tan(z) + a3*tan^3(z) + a5*tan^5(z) 
    ## dr/dz=a1*sec^2(z)+3*a3*tan^2(z)*sec^2(z)+5*a5*tan^4(z)*sec^2(z)
    ## a=1-dr/dz
  */
  
  sec2z=1/cos(z)/cos(z);
  tan2z=sin(z)*sin(z)*sec2z;
  
  /* printf("%f %f\n",sqrt(sec2z),sqrt(tan2z)); */
  
  b=(a1+tan2z*(3.*a3+5.*a5*tan2z))*sec2z;
  a=1.-b;
  
  cP=cos(PA);
  sP=sin(PA);
  
  ct=cos(t);
  st=sin(t);
  
  b11=ct*(cP*cP+a*sP*sP)-st*b*sP*cP;
  b12=ct*b*sP*cP-st*(a*cP*cP+sP*sP);
  b21=st*(cP*cP+a*sP*sP)+ct*b*sP*cP;
  b22=st*b*sP*cP+ct*(a*cP*cP+sP*sP);
  
  printf("-b11=%g -b12=%g -b21=%g -b22=%g\n",
	 b11,b12,b21,b22);
  
  return 0;
}
