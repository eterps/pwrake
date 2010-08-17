#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <math.h>
#include <string.h>

#include "imc.h"
#include "getargs.h"

void sorthist(int n, float *value, int *hist)
{
  int i, j, k;
  float x;
  int y;

  for (k=n/2-1;k>=0;k--) 
    {
      i=k;  
      /* keep */
      x=value[i];
      y=hist[i];

      j=2*i+1;
      while (j<=n-1) 
	{
	  if (j<n-1 && value[j]<value[j+1]) j++;
	  if (x >= value[j]) break;
	  /* swap */
	  value[i]=value[j];  
	  hist[i]=hist[j];  
	  i=j;
	  j=2*i+1;
	}
      /* restore */
      value[i]=x;
      hist[i]=y;
    }

  while (n>0) 
    {
      /* keep*/
      x=value[n-1];
      y=hist[n-1];
      value[n-1]=value[0];
      hist[n-1]=hist[0];

      n--;
      i=0;
      j=2*i+1;
      while (j<=n-1) 
	{
	  if (j<n-1 && value[j]<value[j+1]) j++;
	  if (x >= value[j]) break;
	  /* swap */ 
	  value[i]=value[j];  
	  hist[i]=hist[j];  
	  i=j;
	  j=2*i+1;
	}
      /* restore */
      value[i]=x;
      hist[i]=y;
    }
}


/**************************************************************************/

int main(int argc,char *argv[])
{
  struct  imh	imhin={""};
  struct  icom	icomin={0};
  char fnamin[BUFSIZ]="";
  FILE	*fpin;
  float   *dp;
  int iy,ix;
  float pixignr=(float)INT_MIN;

  float imax=(float)INT_MAX,imin=(float)INT_MIN;

  float bzero=FLT_MIN,bscale=-1.0;

  int dtype,pixoff;
  int ymin=-1,ymax=-1;
  int xmin=-1,xmax=-1;




#define BINMAX 65536

/*
#define BINMAX 32768
*/

  int *hist;
  float *value;
  float scale;
  int i,j,l;
  float med=0;
  int quietmode=1;

  float nth=0;
  float frac;
  int inth=0;

  /* for getarg ver3*/

  getargopt opts[20];
  char *files[3]={NULL};
  int n=0;
  int helpflag;

  files[0]=fnamin;

  setopts(&opts[n++],"-imax=", OPTTYP_FLOAT , &imax,
	  "imax");
  setopts(&opts[n++],"-imin=", OPTTYP_FLOAT , &imin,
	  "imin");
  setopts(&opts[n++],"-xmin=", OPTTYP_INT , &xmin, "xmin (default:0)");
  setopts(&opts[n++],"-xmax=", OPTTYP_INT , &xmax, "xmax (default:npx-1)");
  setopts(&opts[n++],"-ymin=", OPTTYP_INT , &ymin, "ymin (default:0)");
  setopts(&opts[n++],"-ymax=", OPTTYP_INT , &ymax, "ymax (default:npy-1)");
  setopts(&opts[n++],"-q",OPTTYP_FLAG,&quietmode,"quiet mode(default)",1);
  setopts(&opts[n++],"-debug",OPTTYP_FLAG,&quietmode,"debug mode",0);
  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamin[0]=='\0')
   {
      fprintf(stderr,"Error: No input file specified!!\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: normmed [option] (infilename)",
		 opts,"");
      exit(-1);
    }
  
  if((fpin = imropen ( fnamin, &imhin, &icomin )) == NULL)
    {
      fprintf(stderr,"Error: Cannot open input file \"%s\"\n", fnamin);
      exit(-1);
    }

  if(xmin<0) xmin=0;
  if(ymin<0) ymin=0;
  if(xmax<0) xmax=imhin.npx-1;
  if(ymax<0) ymax=imhin.npy-1;

  if(xmin>xmax||xmax>imhin.npx-1||
     ymin>ymax||ymax>imhin.npy-1)
    {
      fprintf(stderr,"Error: region (%d,%d)-(%d,%d) is out of (0,0)-(%d,%d).",
	      xmin,ymin,xmax,ymax,imhin.npx-1,imhin.npy-1);
      exit(-1);
    }

  dp=(float*)malloc(imhin.npx*sizeof(float));

  /*
    if(imhin.npx>XMAX)
    {
    fprintf(stderr,"Sorry, npx=%d is too large. max npx allowed is %d\n",
    imhin.npx,XMAX);
    exit(-1);
    }
  */

  /* type check & reset imax,imin*/

#if 1
  if(imax==(float)INT_MAX&&imin==(float)INT_MIN)
    {
      switch(imhin.dtype)
	{
	case DTYPFITS:
	  imc_fits_get_dtype(&imhin,&dtype,  
			     &bzero,&bscale,&pixoff);
	  switch(dtype)
	    {
	    case FITSCHAR:
	      imax=bscale*(float)CHAR_MAX+bzero;
	      imin=bscale*(float)CHAR_MIN+bzero;
	      break;
	    case FITSFLOAT:
	      imax=bscale*(float)INT_MAX+bzero;
	      imin=bscale*(float)INT_MIN+bzero;
	      break;
	    case FITSSHORT:
	      imax=bscale*(float)SHRT_MAX+bzero;
	      imin=bscale*(float)SHRT_MIN+bzero;
	      break;
	    case FITSINT:
	      imax=bscale*(float)INT_MAX+bzero;
	      imin=bscale*(float)INT_MIN+bzero;
	      break;
	    default:
	      break;
	    }	  
	  break;
	case DTYPU2:
	  imax=(float)USHRT_MAX;
	  imin=0;
	  break;
	case DTYPI2:
	  imax=(float)SHRT_MAX;
	  imin=(float)SHRT_MIN;
	  break;
	default:
	  break;      
	}
    }
#endif

  hist=(int*)calloc(BINMAX,sizeof(int));
  value=(float*)malloc(BINMAX*sizeof(float));
  
  if(!quietmode)
    printf("-imax=%f,-imin=%f\n",imax,imin);
  pixignr=(float)imget_pixignr( &imhin );

  /* 1st path, make histogram */

  for(l=0;l<3;l++)
    {

      if(!quietmode)  
	printf("1st path\n");
      scale=(float)(BINMAX-1)/(imax-imin);
      n=0;

      for( iy=ymin; iy<=ymax; iy++ ) 
	{
	  if(!quietmode && iy%100==0)
	    printf("debug:%d n=%d\n",iy,n);

	  (void) imrl_tor( &imhin, fpin, dp, iy );
	  for( ix=xmin; ix<=xmax; ix++ ) 
	    {
	      if(dp[ix]<=imax&&dp[ix]>=imin&&dp[ix]!=pixignr)
		{
		  hist[(int)floor((dp[ix]-imin)*scale)]++;
		  n++;
		}
	    }
	}
      if(!quietmode)
	printf("cumm\n");
      
      if(nth==0) 
	nth=(float)(n+1)*0.5;
      /* nth is 1-origin number */

      /* diff to cummulative */
      if(nth<hist[0])
	i=0;
      else
	{
	  for (i=1;i<BINMAX;i++)
	    {
	      hist[i]+=hist[i-1];
	      if(hist[i]>=nth) break;
	    }
	  nth-=(float)hist[i-1];
	}
      /* the median is in i-th bin, [i/scale+imin,i+1/scale+imin] */
      imax=(float)(i+1)/scale+imin;
      imin+=(float)i/scale;

      if(!quietmode)
	printf("[%g %g] %f / %d\n",imin,imax,nth,hist[i]-hist[i-1]);

      if(hist[i]-hist[i-1]<BINMAX)
	break;

      memset(hist,0,BINMAX*sizeof(int));
    }

  /* imax!=imin check needed ???*/
  if(imax==imin)
    {
      med=imax;
    }
  else
    {
      memset(hist,0,BINMAX*sizeof(int));
      if(!quietmode)
	printf("2nd pass\n");
      /* 2nd path */
      n=0; j=0;
      for( iy=ymin; iy<=ymax; iy++ ) 
	{
	  if(!quietmode&&iy%100==0)
	    printf("debug:%d %d\n",iy,j);
	  (void) imrl_tor( &imhin, fpin, dp, iy );
	  for( ix=xmin; ix<=xmax; ix++ ) 
	    {
	      if(dp[ix]<=imax&&dp[ix]>=imin&&dp[ix]!=pixignr)
		{
		  hist[n]++;
		  value[n]=dp[ix];
		  n++; 
		  j++;
#if 1
		  if(n>=BINMAX) 
		    {
		      /* compress */
		      sorthist(BINMAX,value,hist);
		      for (i=1;i<BINMAX;i++)
			{
			  if(value[i]==value[i-1])
			    {
			      hist[i]+=hist[i-1];
			      value[i-1]=imax+1.0;
			      hist[i-1]=0;
			    }
			}
		      sorthist(BINMAX,value,hist);
		      for (n=BINMAX-1;n>=0;n--)
			{
			  if(hist[n]>0)break;
			}
		      n++;
		      if(n>=BINMAX) 
			{
			  fprintf(stderr,"Cannot compress histogram, exit...\n");
			  exit(-1);
			}
		    }
#endif
		}
	    }
    }
      
      /* compress */
      sorthist(n,value,hist);
      
      for (i=1;i<n;i++)
	{ 
	  hist[i]+=hist[i-1];
	  if(hist[i]>=nth) 
	    {
	      if(!quietmode)
		printf("debug: nth:%f %d/%d %15.15f:%d %15.15f:%d\n",nth,i,n,
		       value[i-1],hist[i-1],
		       value[i],hist[i]);
	      
	      inth=(int)floor(nth);
	      if(hist[i-1]<inth)
		med=value[i];
	      else
		{
		  frac=nth-(float)inth;
		  med=(1.0-frac)*value[i-1]+frac*value[i];
		}
	      break;
	    }
	}
    }

  if(!quietmode)
    printf("median:");
  printf("%f\n",med);

  imclose(fpin,&imhin,&icomin);
  free(dp);
  return 0;
}
