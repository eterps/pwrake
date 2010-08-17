#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "getargs.h"
#include "match_sub.h"
#include "stat.h"
#include "statd.h"

/* Moved from match_sub.c, for this routine is not generic */
POINT	*read_point_list(char *filename, int *npt)
{
    FILE	*fp;
    char	line[BUFSIZ];
    /*
    double	x, y, f,m;
    */
    int		i, num = 0;
    int		start;
    POINT *point_return=NULL;
    int POINTMAX=200,POINTMAXSTEP=200;
    double f;
    double *x,*y,*m;
    int *id;

    if ((fp = fopen(filename, "rt")) == NULL) {
	(void)fprintf(stderr, "read_point_list : %s does not exist!\n", 
		      filename);
	*npt=0;
	return NULL;
    }
    id=(int*)malloc(sizeof(int)*POINTMAX);
    m=(double*)malloc(sizeof(double)*POINTMAX);
    x=(double*)malloc(sizeof(double)*POINTMAX);
    y=(double*)malloc(sizeof(double)*POINTMAX);

    while ( fgets(line, BUFSIZ, fp) != NULL)
      {
	start = 0;
	/* New */
	if (sscanf(line, "%*s %lf %lf %lf", &(x[num]), &(y[num]), &f)==3 
	    && f>0)
	  {
	    m[num]=-2.5*log10(f);
	    id[num]=num;
	    num++;
	  }
	if (feof(fp)) break;
	if(num>=POINTMAX)
	  {
	    POINTMAX+=POINTMAXSTEP;
	    id=(int*)realloc(id,sizeof(int)*POINTMAX);
	    m=(double*)realloc(m,sizeof(double)*POINTMAX);
	    x=(double*)realloc(x,sizeof(double)*POINTMAX);
	    y=(double*)realloc(y,sizeof(double)*POINTMAX);
	  }
      }
    (void)fclose(fp);
        
    /* sorted by m in from smallest */
    heapsort2id(num,m,id);

    *npt = num;

    point_return = (POINT*)malloc((size_t)(*npt)*sizeof(POINT));
    if (point_return == NULL) 
      {
	(void)fprintf(stderr, "point_return : Out of memory\n");
	exit(0);
      }

    for (i = 0; i < *npt; i++) 
      {
	point_return[i].id = i;
	point_return[i].x  = x[id[i]];
	point_return[i].y  = y[id[i]];
	point_return[i].m  = -m[i];
      }
    free(id);
    free(x);
    free(y);
    free(m);

    return point_return;
}

int main(int argc, char **argv)
{
    POINT *pta=NULL, *ptb=NULL;
    POINT *pta_keep=NULL, *ptb_keep=NULL;
    int npa, npb;
    int npa0, npb0;
    MATCH_PAIR *mat_pair=NULL;
    int num=0;
    int nmp=0;
    double dx, dx_sig, dy, dy_sig;
    double dm, dm_sig;
    int nmp_limits=0;
    double perr;

    int i;

    int flag=1;

    int nmax=30,nmin=4;
    double epsilon=1.0;

    double theta_mean, theta_sig;
    double ss,cc,x_old,y_old,x_new,y_new;
    int m;

    char fnamcat1[BUFSIZ]="";
    char fnamcat2[BUFSIZ]="";

    getargopt opts[10];
    char *files[3]={NULL};
    int n=0;
    int helpflag;
    int quietmode=1;

    files[0]=fnamcat1;
    files[1]=fnamcat2;
  
  setopts(&opts[n++],"-nmax=",OPTTYP_INT,&nmax,"max objects to be used (default:30");
  setopts(&opts[n++],"-nmin=",OPTTYP_INT,&nmin,"min objects limit to be used (default:4)");
  setopts(&opts[n++],"-eps=",OPTTYP_FLOAT,&epsilon,"error of position (default:1.0)");
  setopts(&opts[n++],"-quiet",OPTTYP_FLAG,&quietmode,"quiet mode(default)",1);
  setopts(&opts[n++],"-verbose",OPTTYP_FLAG,&quietmode,"verbose mode",0);

  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);
  if(helpflag==1||fnamcat2[0]=='\0')
    {
      print_help("Usage: match_single5 <options> (file1) (file2)\n"
		 "       *.mes = \"# x y f ...\" format, f is flux, not magnitude.",
		 opts,
		 "");
      exit(-1);
    }

    pta = read_point_list(fnamcat1, &npa0);
    ptb = read_point_list(fnamcat2, &npb0);

    if(!quietmode)
      printf("npa = %d npb = %d\n", npa0, npb0);
    if (pta==NULL || ptb==NULL) 
      {
	exit(-1);
      }
    /* 2000/03/16 */ 
    /* copy */

    pta_keep=(POINT*)malloc(npa0*sizeof(POINT));
    ptb_keep=(POINT*)malloc(npb0*sizeof(POINT));

    memcpy(pta_keep,pta,npa0*sizeof(POINT));
    memcpy(ptb_keep,ptb,npb0*sizeof(POINT));

    num=nmax;
    if (num>npa0) num=npa0;
    if (num>npb0) num=npb0;
    
    if(!quietmode)
      printf("%d %f %d\n",num,epsilon,flag);
    
    if (npa0>num) npa=num; else npa=npa0;
    if (npb0>num) npb=num; else npb=npb0;
    memcpy(pta,pta_keep,npa*sizeof(POINT));
    memcpy(ptb,ptb_keep,npb*sizeof(POINT));
    
    match(pta, npa, ptb, npb, epsilon, &mat_pair, &nmp);

    if(!quietmode)
      {
	printf("nmp=%d\n",nmp);
	for(i=0;i<nmp;i++)
	  {
	    printf("%d: (%f,%f)-(%f,%f) %d\n",
		   i,
		   mat_pair[i].pt1.x,mat_pair[i].pt1.y,
		   mat_pair[i].pt2.x,mat_pair[i].pt2.y,
		   mat_pair[i].count);
	  }
      }

    rotation_angle(mat_pair, nmp, &theta_mean, &theta_sig);
    
    /* Recalculate positions with rotation */   
    if (nmp != 0) 
      {
	
	/* Take out Rotation from Points */
	ss = sin(theta_mean);
	cc = cos(theta_mean);
	for (m = 0; m < nmp; m++) 
	  {
	    x_old = mat_pair[m].pt2.x;
	    y_old = mat_pair[m].pt2.y;
	    x_new = x_old * cc + y_old * ss;
	    y_new =-x_old * ss + y_old * cc;
	    mat_pair[m].pt2.x = x_new;
	    mat_pair[m].pt2.y = y_new;
	  }
      }
    
    displacement(mat_pair, nmp, &dx, &dx_sig, &dy, &dy_sig);
    flux_ratio(mat_pair, nmp, &dm, &dm_sig, &nmp_limits);
    if(!quietmode)
      {
	printf("dx = %7.2f +- %7.2f\n", dx, dx_sig);
	printf("dy = %7.2f +- %7.2f\n", dy, dy_sig);
	printf("dt = %7.4f +- %7.4f\n", theta_mean, theta_sig);
	printf("dm = %7.2f +- %7.2f (%d)\n", dm, dm_sig, nmp_limits);
      }
    perr=sqrt(dx_sig*dx_sig+dy_sig*dy_sig);
    /* free mat_pair*/

    free(mat_pair);

    if(!quietmode)    
      printf("%d %f\n",num,epsilon);

    if(nmp_limits<nmin)
      {
	/* clear */
	dx=0;
	dy=0;
	theta_mean=0;
	dm=0;
	dx_sig=-1.0; 
	dy_sig=-1.0;
	theta_sig=-1.0;
	dm_sig=-2.5*log10(2);
      }

    printf("%s %s %8.3f %8.3f %8.4f %8.3f %8.3f %8.3f %8.4f %8.3f %d\n",
	   fnamcat1,fnamcat2,
	   -dx,-dy,-theta_mean,pow(10.0,0.4*dm),
	   dx_sig, dy_sig,theta_sig, 1.0-pow(10.0,-0.4*dm_sig),nmp_limits);

    free(pta);
    free(ptb);
    free(pta_keep);
    free(ptb_keep);

    return 0;
}
