#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "imc.h"
#include "getargs.h"

int main(int argc,char **argv)
{
 float *g;
 int i;
 struct imh imhin={""};
 struct icom icomin={0};
 FILE *fp;
 int npx,npy;
 int pixignr;
 double total=0.;
 int disable_pixignr=0;
 char fnamin[BUFSIZ]="";


 /* for getarg */
 /* Ver3. */
 getargopt opts[20];
 char *files[3]={NULL};
 int n=0;
 int helpflag;
 
 files[0]=fnamin;
 
 setopts(&opts[n++],"-disable_pixignr",OPTTYP_FLAG,&disable_pixignr,
	 "add pixignr(blank) as a normal pix.",1);
 setopts(&opts[n++],"",0,NULL,NULL);
 
 helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamin[0]=='\0')
   {
      fprintf(stderr,"Error: No input file specified!!\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: total [option] (infilename)",
		 opts,"");
      exit(-1);
    }
 
  /*
   * ... Open Input
   */
 
 if ((fp=imropen(fnamin,&(imhin),&(icomin)))==NULL)
   {
     printf("File %s not found !! ignore. \n",fnamin);
     return -1;
   }
 
 npx=imhin.npx;
 npy=imhin.npy;
 g=(float*)malloc(sizeof(float)*(npy+1)*(npx+1));
 
 imrall_tor(&imhin, fp, g, npx*npy);
 pixignr=imget_pixignr(&imhin);
 
 imclose(fp,&imhin,&icomin);

 if(disable_pixignr==1) 
   for(i=0;i<npx*npy;i++)
     {
       total+=(double)g[i];
     }
 else
   for(i=0;i<npx*npy;i++)
     {
       if(g[i]!=(float)pixignr)
	 total+=(double)g[i];
     }
 
 printf("%f\n",total);
 free(g);
 return 0;
}
