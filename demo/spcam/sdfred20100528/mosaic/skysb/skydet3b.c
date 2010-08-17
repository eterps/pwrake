#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include "imc.h"
#include "getargs.h"
#include "oyamin.h"
#include "stat.h"
#include "postage.h"

#include "skysb_sub.h"

int main(int argc,char **argv)
{
  char fnamin[BUFSIZ]="";
  char fnamout[BUFSIZ]="";

  struct icom icomin={0};
  struct imh imhin={""};

  float fact,fsigm;
  float gmax,gmean,gmin,gsd;
  int   npx,npy;
  int nmesh_x,nmesh_y;
  int   meshsiz_x=0,meshsiz_y=0,meshsiz_xy=0;
  int   ncycle;
  float pixignr;
  float *g;
  float *rmesh;
  float *sgmesh;


  float bscale=1.;



  FILE *fpin;
  float msky,msigm;

  int dtypein;
  int binflag;

  /***** parse options ******/
  getargopt opts[20];
  char *files[3]={NULL};
  int n=0;

  files[0]=fnamin;
  files[1]=fnamout;

  n=0;
  setopts(&opts[n++],"-mesh=", OPTTYP_INT , &meshsiz_xy,
	  "mesh size (default: image size)");
  setopts(&opts[n++],"-meshx=", OPTTYP_INT , &meshsiz_x,
	  "mesh x-size (default: image size)");
  setopts(&opts[n++],"-meshy=", OPTTYP_INT , &meshsiz_y,
	  "mesh y-size (default: image size)");
  /* not impl. yet */
  /*
  setopts(&opts[n++],"-xmin=", OPTTYP_INT , &xmin,
	  "");
  setopts(&opts[n++],"-xmax=", OPTTYP_INT , &xmax,
	  "");
  setopts(&opts[n++],"-ymin=", OPTTYP_INT , &ymin,
	  "");
  setopts(&opts[n++],"-ymax=", OPTTYP_INT , &ymax,
	  "");
  */
  setopts(&opts[n++],"",0,NULL,NULL);

  if(parsearg(argc,argv,opts,files,NULL)||fnamin[0]=='\0')
    {
      print_help("Usage: skydet [options] filein [fileout]",
		 opts,"");
      exit(-1);
    }
  

  /********* main loop *************/
  
  /*
   *** DATA INPUT ***
   */
    
  /**** read image ****/
  if ((fpin= imropen(fnamin,&imhin,&icomin))==NULL) 
    {
      fprintf(stderr,"File %s not found !!\n",fnamin);
      exit(-1);
    }
  
  pixignr=imget_pixignr( &imhin );
  
  if(imhin.dtype==DTYPFITS)
    {
      imc_fits_get_dtype( &imhin, &dtypein, NULL, &bscale, NULL );
      if(dtypein==FITSFLOAT || bscale!=1.0)
	binflag=0;
      else
	binflag=1;	    
    }
  else
    {
      if(imhin.dtype==DTYPR4 || imhin.dtype==DTYPR8)
	binflag=0;
      else
	binflag=1;
    }

  npx=imhin.npx;
  npy=imhin.npy;
  
  if(meshsiz_xy!=0) 
    {
      meshsiz_x=meshsiz_xy;
      meshsiz_y=meshsiz_xy;
    }

  if(meshsiz_x<1) meshsiz_x=npx;
  if(meshsiz_y<1) meshsiz_y=npy;
  
  g=(float*)malloc(sizeof(float)*npx*npy);
  if (g==NULL) 
    { 
      printf("Error cannot allocate image buffer"); 
      exit(-1);
    }
  
  if(!imrall_tor(&imhin,fpin,g,npx*npy)) 
    {
      fprintf(stderr,"Error. cannot read data.\n");
      exit(-1);
    }
      
  imclose(fpin,&imhin,&icomin);

  /*
   *** SATAISTICS
   */
    
  fact=2.0;
  ncycle=2;
  statsk(g,npx,npy,fact,ncycle,&gmax,&gmin,&gmean,&gsd,pixignr);

  printf("\n =>  GMAX GMIN GMEAN GSD= %f %f %f %f \n",gmax,gmin,gmean,gsd);

  /*
 *** SKY DETERMINATION AND SUBTRACTION ***
 */
    
  nmesh_x=(int)(((float)npx*2.)/meshsiz_x)-1; 
  if(nmesh_x<1) nmesh_x=1;  
  nmesh_y=(int)(((float)npy*2.)/meshsiz_y)-1; 
  if(nmesh_y<1) nmesh_y=1;
  
  rmesh=(float*)malloc(sizeof(float)*nmesh_x*nmesh_y);
  sgmesh=(float*)malloc(sizeof(float)*nmesh_x*nmesh_y);
  
  /* 97/09/12 Yagi */
  fsigm=70.0; /* default */

  skydet2b(g,npx,npy,rmesh,
	   nmesh_x,nmesh_y,
	   meshsiz_x,meshsiz_y,
	   gmean,gsd,fsigm,sgmesh,binflag,
	   pixignr);
  
  if(nmesh_x*nmesh_y>2)
    {
      msigm=skysigmadet(sgmesh,nmesh_x,nmesh_y);
      msky=medfilter_skysb(rmesh,nmesh_x,nmesh_y,1); 
    }
  else if (nmesh_x*nmesh_y==2)	
    {
      msky=0.5*(rmesh[0]+rmesh[1]);
      msigm=0.5*(sgmesh[0]+sgmesh[1]);
    }
  else if(nmesh_x*nmesh_y==1)
    {
      msky=rmesh[0];
      msigm=sgmesh[0];	 
    }
  else 
    {
      msky=0;/* Error */
      msigm=-1.0; /* Error */
    }
  
  printf("%g ",msky);
  printf("%g\n",msigm);

  if(fnamout[0]!='\0') 
    {
      skypattern(g,npx,npy,rmesh,nmesh_x,nmesh_y,meshsiz_x,meshsiz_y);
      makePostage(fnamout,g,npx,npy);
    }
  
  return 0;
}
