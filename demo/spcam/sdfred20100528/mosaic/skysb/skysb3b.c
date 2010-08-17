#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <float.h>

#include "imc.h"
#include "oyamin.h"
#include "stat.h"

#include "getargs.h"
#include "skysb_sub.h"

int main(int argc,char **argv)
{
  float fact,fsigm;
  float gmax,gmean,gmin,gsd;
  int   npx,npy,meshsiz_x,meshsiz_y;

  int   nmesh_x=0,nmesh_y=0,mesh=0;
  int   ncycle;
  char fnamin[BUFSIZ]="";
  char fnamout[BUFSIZ]="";
  struct icom icomin={0},icomout={0},icomref={0};
  struct imh imhin={""},imhout={""},imhref={""};               

  int i,j;

  float *g,*g2;
  float *rmesh;
  float *sgmesh;

  char fnamref[BUFSIZ]="";
  char buffer[BUFSIZ];
  char skymap[BUFSIZ]="";

  float bzero=0.,bscale=1.;
  float bzero_new=FLT_MIN,bscale_new=FLT_MIN;
  int bzeroset=0;

  int fitsdtype;

  FILE *fpin,*fpout,*fpref;
  FILE *fmap;
  int mode;
  char dtype[80]="";


  float msky,msigm;

  int dtypein;
  int binflag;
  float ignor=(float)INT_MIN;
  float imax=(float)INT_MIN;
  float imin=(float)INT_MAX;

  int pixignr;


  getargopt opts[20];
  char *files[3]={NULL};
  int n=0;
  int helpflag;

  /***************************************/

  files[0]=fnamin;
  files[1]=fnamout;

  setopts(&opts[n++],"-mesh=", OPTTYP_INT , &mesh,
	  "mesh size (default:full image)");
  setopts(&opts[n++],"-meshx=", OPTTYP_INT , &nmesh_x,
	  "mesh x-size (default:not used)");
  setopts(&opts[n++],"-meshy=", OPTTYP_INT , &nmesh_y,
	  "mesh y-size (default:not used)");

  setopts(&opts[n++],"-skyref=", OPTTYP_STRING , &fnamref,
	  "sky determination reference (default:use input image)");

  setopts(&opts[n++],"-imax=", OPTTYP_FLOAT , &imax,
	  "flux upper limit to estimate sky(default:unlimited)");
  setopts(&opts[n++],"-imin=", OPTTYP_FLOAT , &imin,
	  "flux lower limit to estimate sky(default:unlimited)");

  /* */

  setopts(&opts[n++],"-bzero=", OPTTYP_FLOAT , &bzero_new,
	  "bzero");
  setopts(&opts[n++],"-bscale=", OPTTYP_FLOAT , &bscale_new,
	  "bscale");
  setopts(&opts[n++],"-dtype=", OPTTYP_STRING , dtype,
	  "datatyp(FITSFLOAT,FITSSHORT...)");
  setopts(&opts[n++],"-pixignr=", OPTTYP_FLOAT , &ignor,
	  "pixignr value");

  /* To be implemented in next major version */
  /*
    setopts(&opts[n++],"-fit", OPTTYP_FLAG , &fit_param,
    "fitting (default:no)",1);
  */

  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);

  if(fnamout[0]=='\0')
   {
      fprintf(stderr,"Error: No input file specified!!\n");
      helpflag=1;
    }
  if(helpflag==1)
    {
      print_help("Usage: skysb3b <options> [filein] [fileout]",
		 opts,
		 "");
      exit(-1);
    }

  if(nmesh_x==0) nmesh_x=mesh;
  if(nmesh_y==0) nmesh_y=mesh;

  if(bzero_new!=FLT_MIN)
    {
      bzero=bzero_new;
      bzeroset=1;
    }
  if(bscale_new!=FLT_MIN)
    {
      bscale=bscale_new;
      bzeroset=1;
    }

  /*
    printf("debug:nmesh_x=%d nmesh_y=%d\n",nmesh_x,nmesh_y);
  */
  
  mode=1;     /* default */
  fsigm=70.0; /* default */


  /*
 *** DATA INPUT ***
 */
  
  /**** read image ****/
  fprintf(stderr,"%s\n",fnamin);  
  if ((fpin= imropen(fnamin,&imhin,&icomin))==NULL) 
    {
      printf("File %s not found !!\n",fnamin);
      exit(-1);
    }
  
  pixignr=(int)imget_pixignr( &imhin );
  
  printf("pixignr_in=%f\n",(float)pixignr);
  
  if(imhin.dtype==DTYPFITS)
    {
      imc_fits_get_dtype( &imhin, &dtypein, NULL, NULL, NULL);
      if(dtypein==FITSFLOAT)
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


      if(nmesh_x<1) nmesh_x=npx;
      if(nmesh_y<1) nmesh_y=npy;

      g=(float*)malloc(sizeof(float)*npx*npy);
      g2=(float*)malloc(sizeof(float)*npx*npy);
      if (g==NULL||g2==NULL) { printf("Error cannot allocate image buffer"); exit(-1);}
      
      if(imrall_tor(&imhin,fpin,g2,npx*npy)) 
	printf("npx, npy =%d %d \n",npx,npy);
      else {
	printf("Error. cannot read data.\n");
	exit(-1);
      }

      /* 2002/05/21 */
      if(fnamref[0]=='\0') 
	{
	  memcpy(g,g2,sizeof(float)*npx*npy);	  
	}
      else
	{
	  if ((fpref=imropen(fnamref,&imhref,&icomref))==NULL) 
	    {
	      printf("Reference File %s not found !!\n",fnamref);
	      exit(-1);
	    }
	  if (npx!=imhref.npx||npy!=imhref.npy)
	    {
	      printf("Error: Ref. size (%dx%d) differ from input (%dx%d)\n",
		     imhref.npx,imhref.npy,npx,npy);
	      exit(-1);
	    }
	  if(! imrall_tor(&imhref,fpref,g,npx*npy)) 
	    {
	      printf("Error: cannot read ref data.\n");
	      exit(-1);
	    }
	  imclose(fpref,&imhref,&icomref);
	}
      
      /* 2001/07/19 preformat (masking) */
      
      if(imax>(float)INT_MIN||imin<(float)INT_MAX)
	for(i=0;i<npx*npy;i++)
	  if(g[i]>imax||g[i]<imin) g[i]=pixignr;
      
      /*
     *** SATAISTICS
     */
      
      printf("Calculating statistics \n");

      /* TODO */
      /* to be tunable ?*/
      fact=2.0;
      ncycle=2;
      statsk(g,npx,npy,fact,ncycle,&gmax,&gmin,&gmean,&gsd,pixignr);
      
      printf("\n");
      printf(" =>  GMAX GMIN GMEAN GSD= %f %f %f %f \n",gmax,gmin,gmean,gsd);
      
      /*
       *** SKY DETERMINATION AND SUBTRACTION ***
       */

      printf("Sky subtraction under way\n");

      meshsiz_x=(int)((npx*2.)/nmesh_x)-1; /* meshsiz_x > 0 */
      if(meshsiz_x<1) meshsiz_x=1;

      meshsiz_y=(int)((npy*2.)/nmesh_y)-1; /* meshsiz_y > 0 */
      if(meshsiz_y<1) meshsiz_y=1;

      rmesh=(float*)malloc(sizeof(float)*meshsiz_x*meshsiz_y);
      sgmesh=(float*)malloc(sizeof(float)*meshsiz_x*meshsiz_y);

      /* 97/09/12 Yagi */
      skydet2b(g,npx,npy,rmesh,meshsiz_x,meshsiz_y,
	       nmesh_x,nmesh_y,gmean,gsd,fsigm,sgmesh,binflag,pixignr);

      if(meshsiz_x*meshsiz_y>2)
	{
	  /* 3X3 median filter */
	  /* 
	     msky=skysigmadet(rmesh,meshsiz_x,meshsiz_y);
	  */
	  msky=medfilter_skysb
	    (rmesh,meshsiz_x,meshsiz_y,1);
	  msigm=skysigmadet(sgmesh,meshsiz_x,meshsiz_y);
	}
      else if (meshsiz_x*meshsiz_y==2)	
	{
	  msky=0.5*(rmesh[0]+rmesh[1]);
	  msigm=0.5*(sgmesh[0]+sgmesh[1]);
	}
      else if(meshsiz_x*meshsiz_y==1)
	{
	  msky=rmesh[0];
	  msigm=sgmesh[0];	 
	}
      else 
	{
	  msky=0;/* Error */
	  msigm=-1.0; /* Error */
	}

      printf("SSB-EST %g\n",msky);
      printf("SSGM-EST %g\n",msigm);

      /* Write skymap to file*/
      
      if(skymap[0]!='\0') 
	{
	  if(skymap[0]=='-')
	    {
	      fmap=stdout;
	      printf("mapfile:stdout\n");
	      fprintf(stderr,"mapfile:stdout\n");
	    }
	  else
	    {
	      printf("mapfile:%s\n",skymap);
	      fprintf(stderr,"mapfile:%s\n",skymap);
	      fmap=fopen(skymap,"a");
	    }

	  if(fmap==NULL) 
	    {
	      printf("Cannot write to \"%s\" -- writeing to stdout.\n",skymap);
	      fprintf(stderr,
		      "Cannot write to \"%s\" -- writeing to stdout.\n",skymap);
	      fmap=stdout;
	    }

	  fprintf(fmap,"# %s\n",fnamin);
	  fprintf(fmap,"# %d %d # (mesh size)\n",nmesh_x,nmesh_y);
	  fprintf(fmap,"# %d %d # meshsiz_x meshsiz_y\n",meshsiz_x,meshsiz_y);
	  fprintf(fmap,"# %g %g # msky msigma\n",msky,msigm);
	  for(j=0;j<meshsiz_y;j++)
	    {
	      for(i=0;i<meshsiz_x;i++)
		{
		  fprintf(fmap,"%d %d %6.2f\n",i,j,rmesh[i+meshsiz_x*j]);
		}
	    }
	  if(fmap!=stdout) fclose(fmap);
	}


      /*
       *** prepare for output ***
       */  

      /* Now sky subtract */

      skysub(g2,npx,npy,rmesh,meshsiz_x,meshsiz_y,
	     nmesh_x,nmesh_y,mode,pixignr);
      free(rmesh);
      free(sgmesh);      

      statsk(g2,npx,npy,fact,ncycle,&gmax,&gmin,&gmean,&gsd,pixignr);
      
      /***** set output format *******/

      fprintf(stderr,"%s\n",fnamout);

      imh_inherit(&imhin,&icomin,&imhout,&icomout,fnamout);
      imclose(fpin,&imhin,&icomin);

      if (imhout.dtype==DTYPFITS)
	{
	  if(dtype[0]=='\0'||(fitsdtype=imc_fits_dtype_string(dtype))==-1)
	    (void)imc_fits_get_dtype( &imhout, &fitsdtype, NULL, NULL, NULL);

	  /* 2000/07/02 */
	  if(bzeroset!=1)
	    (void)imc_fits_get_dtype( &imhout, NULL, &bzero, &bscale, NULL);

	  /* re-set bzero & bscale */
	  if(imc_fits_set_dtype(&imhout,fitsdtype,bzero,bscale)==0)
	    {
	      printf("Error\nCannot set FITS %s\n",fnamout);
	      printf("Type %d BZERO %f BSCALE %f\n",fitsdtype,bzero,bscale);
	      exit(-1);
	    }
	}
      else
	{	  
	  if (strstr(dtype,"I2"))
	    {
	      imhout.dtype=DTYPI2;
	    }
	  else if (strstr(dtype,"INT"))
	    {
	      imhout.dtype=DTYPINT;
	    }
	  else if (strstr(dtype,"I4"))
	    {
	      imhout.dtype=DTYPI4;
	    }
	  else if (strstr(dtype,"R4"))
	    {
	      imhout.dtype=DTYPR4;
	    }
	  else if (strstr(dtype,"R8"))
	    {
	      imhout.dtype=DTYPR8;
	    }
	  else
	    {
	      imhout.dtype=DTYPU2;
	      imset_u2off(&imhout,&icomout,500);  
	    }
	}
    

      if(ignor!=(float)INT_MIN)
	{
	  imset_pixignr(&imhout, &icomout, (int)ignor);
	  /* replace pixignr */
	  for(i=0;i<imhout.npx*imhout.npy;i++)
	    if(g2[i]==(float)pixignr) g2[i]=(float)ignor;
	  pixignr=ignor;
	}
      else
	{
	  imset_pixignr(&imhout, &icomout, pixignr);
	}

      /**** output image ****/


      imaddhistf(&icomout,
		 "Sky subtracted by skysb3b, mesh %3dx%3d sky%7.2f+%7.2f",
		 nmesh_x,nmesh_y,msky,msigm);
      
      if (imget_fits_value(&icomout, "SSBMSH-I", buffer )==0)
	{
	  imupdate_fitsf(&icomout, "SSBMSH-I", "%20d / %-46.46s", 
		      nmesh_x , "SKY MESH X PIXEL" );
	  imupdate_fitsf(&icomout, "SSBMSH-J", "%20d / %-46.46s", 
		      nmesh_y , "SKY MESH Y PIXEL" );
	  imupdate_fitsf(&icomout, "SSB-EST", "%20.4f / %-46.46s", 
		      msky, "TYPICAL SKY ADC ESTIMATED" );
	  imupdate_fitsf(&icomout, "SSGM-EST", "%20.4f / %-46.46s", 
		      msigm, "TYPICAL SKY SIGMA ADC ESTIMATED" );
	}                  
	  
      if ((fpout= imwopen(fnamout,&imhout,&icomout))==NULL) 
	{
	  printf("Cannot open file %s !!\n",fnamout);
	  exit(-1);
	}

      imwall_rto( &imhout, fpout, g2 );
     
      free(g);
      imclose(fpout,&imhout,&icomout);


      /*
	printf("\n");
	printf("                 --- skysb3b  PROGRAM ENDED ---\n");
      */
  return 0;
}
