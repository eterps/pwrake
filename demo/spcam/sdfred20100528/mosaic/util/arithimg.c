#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <float.h>
#include <math.h>

/* ... local include files ... */

#include "imc.h"
#include "getargs.h"

float calc_disable_pixignr(float a,float erra, char*op, float b, float errb)
{
  switch(op[0])
    {
    case '+':
      return a+b;
    case '-':
      return a-b;
    case '*':
    case 'x':
      return a*b;
    case '/':
      if (b!=0)
	return a/b;
      else 
	return erra;
    case '^':
      if(a>=0)
	return (float)pow(a,b);
      else 
	return erra;
    case '=':
      if(op[1]=='=')
	{
	  if(a==b) return 1;
	  else return 0;
	}
    default:
      return a;
    }
}


float calc(float a,float erra, char*op, float b, float errb)
{
  if(a==erra || b==errb) 
    {
      return erra;
    }
  return calc_disable_pixignr(a,erra,op,b,errb);
}

int main(int argc,char *argv[])
{
  struct	imh	imhin1={""}, imhin2={""}, imhout={""};
  struct  icom	icomin1={0}, icomin2={0}, icomout={0};
  char	fnamin1[BUFSIZ]="";
  char	fnamin2[BUFSIZ]="";
  char	fnamout[BUFSIZ]="";
  char op[10]="";
  char dtype[BUFSIZ]="";

  FILE	*fp1, *fp2, *fp3;

  int	npx,npy;
  int i;
  char *endptr;
  double bias;
  float pixignr1=(float)INT_MIN;
  float pixignr2=(float)INT_MIN;
  int pixignr=INT_MIN;
  float *pix1,*pix2;

  float bzero=FLT_MAX,bscale=FLT_MAX;
  int fitsdtype;
  int disable_pixignr=0;

  /* for getarg */
  char comment[BUFSIZ];

  /* Ver3. */
  getargopt opts[20];
  char *files[5]={NULL};
  int n=0;
  int helpflag;

  files[0]=fnamin1;
  files[1]=op;
  files[2]=fnamin2;
  files[3]=fnamout;

  setopts(&opts[n++],"-disable_pixignr", OPTTYP_FLAG , &disable_pixignr,
	  "",1);
  setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero,
	  "bzero");
  setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale,
	  "bscale");
  setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
	  "datatyp(FITSFLOAT,FITSSHORT...)");
  setopts(&opts[n++],"-pixignr=", OPTTYP_INT , &pixignr,
	  "pixignr value");

  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamout[0]=='\0')
   {
      fprintf(stderr,"Error: No output file specified!!\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: arithimg [option] infile1 op {in2filename/number} outfile",
		 opts,"\n       oparator={+,-,*(x),/,^,==}\n");
      exit(-1);
    }


  /*
   * ... Open Input
   */
  if( (fp1 = imropen ( fnamin1, &imhin1, &icomin1 )) == NULL ) 
    {
      printf("Error: Cannot open %s\n",fnamin1);
      return -1;
    }

  npx=imhin1.npx;
  npy=imhin1.npy;
  pix1=(float*)malloc(sizeof(float)*npx*npy);

  if( !imrall_tor(&imhin1,fp1,pix1,npx*npy)) 
    {
      printf("Error: Cannot read %s\n",fnamin1);
      imclose( fp1,&imhin1,&icomin1 );
      free(pix1);
      return -1;
    }

  pixignr1=(float)imget_pixignr( &imhin1 );

  if(op[0]=='\0')
    {
      (void) printf("operator = " );
      (void) fgets( op, 10, stdin);
    }
  
  if(fnamin2[0]=='\0')
    {
      (void) printf(" input image file2 name or number = " );
      (void) fgets( fnamin2, BUFSIZ, stdin);
    }

  bias=strtod(fnamin2,&endptr);
  /*
  if (endptr==fnamin2)
  */
  if (*endptr!='\0')
    {
      /* not a number */
      if( (fp2 = imropen ( fnamin2, &imhin2, &icomin2 )) == NULL ) 
	{
	  printf("Error: Cannot open %s\n",fnamin2);
	  imclose( fp1,&imhin1,&icomin1 );
	  free(pix1);
	  return -1;
	}
      else
	{
	  if(imhin1.npx!=imhin2.npx || imhin1.npy!=imhin2.npy) 
	    {
	      printf("Error: Size of %s and %s differ\n",fnamin1,fnamin2);
	      imclose( fp1,&imhin1,&icomin1 );
	      imclose( fp2,&imhin2,&icomin2 );
	      free(pix1);
	      return -1;
	    }
	  pix2=(float*)malloc(sizeof(float)*npx*npy);
	  if( !imrall_tor(&imhin2,fp2,pix2,npx*npy)) 
	    {
	      printf("Error: Cannot read %s\n",fnamin2);
	      imclose( fp1,&imhin1,&icomin1 );
	      imclose( fp2,&imhin2,&icomin2 );
	      free(pix1);
	      free(pix2);
	      return -1;
	    }
	  pixignr2=(float)imget_pixignr( &imhin2 );
	  if(disable_pixignr==1)
	    for(i=0;i<npx*npy;i++)
	      pix1[i]=calc_disable_pixignr(pix1[i],pixignr1,op,pix2[i],pixignr2);
	  else
	    for(i=0;i<npx*npy;i++)
	      pix1[i]=calc(pix1[i],pixignr1,op,pix2[i],pixignr2);
	  imclose(fp2,&imhin2,&icomin2);
	  printf("pix1 %s pix2\n",op);
	  free(pix2);
	}
    } 
  else
    {
      printf("pix1 %s %f\n",op,bias);
      if(disable_pixignr==1)
	for(i=0;i<npx*npy;i++)
	  pix1[i]=calc_disable_pixignr(pix1[i],pixignr1,op,bias,(float)INT_MIN);
      else
	for(i=0;i<npx*npy;i++)
	  pix1[i]=calc(pix1[i],pixignr1,op,bias,(float)INT_MIN);
    }

  if(fnamout[0]=='\0')
    {
      (void) printf(" output image file name = " );
      (void) fgets( fnamout, BUFSIZ, stdin);
    }

  imh_inherit(&imhin1,&icomin1,&imhout,&icomout,fnamout);
  imclose(fp1,&imhin1,&icomin1);

  /* !! need 2000/07/02 later version of IMC */
  if(dtype[0]=='\0'||(fitsdtype=imc_fits_dtype_string(dtype))==-1)
    (void)imc_fits_get_dtype( &imhout, &fitsdtype, NULL, NULL, NULL);
  /* 2000/07/02 */
  /* 2004/11/05 bug fix */
  if(bzero==FLT_MAX)
    imc_fits_get_dtype( &imhout, NULL, &bzero, NULL, NULL);
  if(bscale==FLT_MAX)
    imc_fits_get_dtype( &imhout, NULL, NULL, &bscale, NULL);

  if(imc_fits_set_dtype(&imhout,fitsdtype,bzero,bscale)==0)
    {
      printf("Error\nCannot set FITS %s\n",fnamout);
      printf("Type %d BZERO %f BSCALE %f\n",fitsdtype,bzero,bscale);      
      exit(-1);
    }	  

  /* replace pixignr1 if needed */
  if(pixignr!=INT_MIN && pixignr!=pixignr1)
    {
      /* 2002/05/23 disable_pixignr check added */
      if (disable_pixignr!=1)
	for(i=0;i<npx*npy;i++)
	  if(pix1[i]==pixignr1) pix1[i]=(float)pixignr;
      imset_pixignr(&imhout, &icomout, pixignr);
    }

  /*  Make comments */
  sprintf(comment,"made by arithimg: %s %s %s",fnamin1,op,fnamin2);
  imaddhist(&icomout,comment);
    
  if( (fp3=imwopen (fnamout, &imhout, &icomout)) == NULL ) 
    {
      printf("Error: Cannot open %s\n",fnamout);
      free(pix1);
      return -1;
    }
  if( !imwall_rto(&imhout,fp3,pix1))
    {
      printf("Error: Cannot write %s\n",fnamout);
      imclose( fp3,&imhout,&icomout );
      free(pix1);
      return -1;
    }
  (void) imclose( fp3,&imhout,&icomout );
  free(pix1);
  return 0;
}
