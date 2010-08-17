#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>

/* revised by YAGI */
#define NMAX 1000000
/* double data[NMAX]; */


int main(int argc,char **argv)
{
  FILE *fp1;
  int i,j;
  int nbin;
  float nbin_1;
  int hist; 
  double center;
  double max,min;
  double lowerv,upperv;
  int rec;
  double total=0;
  double sqtotal=0;
  double mean;
  double width;
  double *data;

  if(argc!=5) {
    fprintf(stderr,"Usage: mkhist [data file] [min] [max] [width]\n");
    exit(-1);
  }

  if( (fp1=fopen(argv[1],"r"))==NULL) {
    fprintf(stderr,"ERROR: file %s not found \n",argv[1]);
    exit(-1);
  }

  rec=0;
  data=(double*)malloc(NMAX*sizeof(double));
  /*
    while( (fscanf(fp1,"%lf",&data[rec]))!=EOF)
    rec++;
  */ /* 2007/07 YAGI v1 */
  while(fscanf(fp1,"%lf",&data[rec])==1)
    {
      rec++;
      /* 2008/01/03 YAGI
	 in fact, this can be much smarter using realloc,
	 but I cannot apply, at least this codeset; it's not mine */
      if (rec>=NMAX)
	{
	  fprintf(stderr, "ERROR: Too many lines in %s (>%d)\n",argv[1],NMAX);
	  exit(-1);
	}
    }

#if 0
  /*it should be more robust against strange lines*/
  char buffer[BUFSIZ];

  while(!feof(fp1))
    {
      if(NULL==fgets(buffer,BUFSIZ,fp)) break;
      if(1==sscanf(buffer,"%lf",&data[rec]))
	{
	  rec++;
	  if(rec>NMAX) break;
	}
    }
#endif 

  if( rec <= 1) {
    fprintf(stderr,"ERROR: %s contains less than 1 column \n",argv[1]);
    exit(-1);
  }

  min=atof(argv[2]);
  max=atof(argv[3]);
  width=atof(argv[4]);

  /* 2008/01/03 YAGI */
  if(width==0)
    {
      fprintf(stderr,"ERROR: width is 0.\n");
      exit(-1);
    }

  nbin_1 = (max-min)/width ;
  nbin= (int) nbin_1 + 1;
  
  upperv=min-0.5*width;
  for(i=0;i<nbin;i++) {
    #if 0
    lowerv=min+((float)i - 0.5)*width;
    upperv=min+((float)i + 0.5)*width;
    #endif
    lowerv = upperv ;
    upperv = lowerv + width ;

    hist=0;
    /* center=(lowerv+upperv)/2.0; */
    center=0.5*(lowerv+upperv);
    
    for(j=0;j<rec;j++) {
      if(data[j] >= lowerv && data[j] < upperv)
	hist++;
    }

#if 0
    if(upperv==max)
      hist++;
#endif

#if 0
    printf("%lf %lf %lf %d\n",lowerv,upperv,center,hist);
#endif
    printf("%lf %ld\n",center,hist);

  }
    

/*
  printf("rec=%d : total=%lf : sqtotal=%lf \n",rec,total,sqtotal); 
  printf("mean=%lf : rms=%lf \n",mean, sqrt(sqtotal/(1.0*rec)-pow(mean,2)) );
*/
  fclose(fp1);
}








