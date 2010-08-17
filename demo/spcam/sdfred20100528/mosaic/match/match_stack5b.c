#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "getargs.h"

#include "sortd.h"
#include "statd.h"
#include "solve.h"


int adddic(int n, char ***pnames, char *name)
{
  int i;
  char **names;

  names=*pnames;
  for(i=0;i<n;i++)
    {
      if(strcmp(names[i],name)==0) return n;
    }
  n++;
  names=*pnames=(char**)realloc((*pnames),sizeof(char*)*n);
  names[n-1]=(char*)malloc(sizeof(char)*(strlen(name)+1));
  strcpy(names[n-1],name);

  return n;
}

int name2no(int n, char **names, char *name)
{
  int i;

  for(i=0;i<n;i++)
    if(strcmp(names[i],name)==0) return i;

  printf("#%s#\n",name);

  exit(-1);
  return -1;
}


int main(int argc, char *argv[])
{
  FILE *fp; 
  /* log(matrix) file */
  /* 0->0 0->1, ... 0->n */
  
  int i,j,n=0,m,k,dim,dimmat,dimdat,idx;
  double *dx,*dy,*dm,*da; /* data */
  double *ex,*ey,*em,*ea; /* error */

  double dx0,dy0,da0; /* data */
  double ex0,ey0,ea0; /* error */
  
  double df0,ef0;
  double x,y;
  double eps_r=10.0,eps_f=0.5,eps_a=0.005,eps_m;

  int ndat,ndat0=0;

  double *ax,*ay,*am,*aa; /* ans */
  double *ax0,*ay0,*am0,*aa0; /* ans */
  double *mat_x,*mat_y,*mat_m,*mat_a;
  double *dat_x,*dat_y,*dat_m,*dat_a;
  double c,s;
  char **line=NULL;
  char **names=NULL;
  char buffer[BUFSIZ];
  char fnam1[BUFSIZ],fnam2[BUFSIZ],fnamcat[BUFSIZ]="";
  int nline=100,l=0;

  int *connect;
  int nc=1,flag;
  int ctmp;

  double *res=NULL,r0; 
  double *resx,*resy; 
  double *chisq=NULL,chisqmin;
  
  int *id=NULL;



  int quietmode=1;

  int ix1,ix2,id1,id2;
  int idm1, idm2;

  int niter=20;
  int ret;

  getargopt opts[10];
  char *files[3]={NULL};
  int helpflag;
  
  files[0]=fnamcat;

  setopts(&opts[n++],"-dr=",OPTTYP_DOUBLE,&eps_r,"error allowance of position.(default:10.0)");
  setopts(&opts[n++],"-df=",OPTTYP_DOUBLE,&eps_f,"error allowance of flux ratio.(default:0.5)");
  setopts(&opts[n++],"-da=",OPTTYP_DOUBLE,&eps_a,"error allowance of angle.(default:0.005)");
  setopts(&opts[n++],"-niter=",OPTTYP_INT,&niter,"");

  setopts(&opts[n++],"-quiet",OPTTYP_FLAG,&quietmode,"quiet mode(default)",1);
  setopts(&opts[n++],"-verbose",OPTTYP_FLAG,&quietmode,"verbose mode",0);
  setopts(&opts[n++],"",0,NULL,NULL);

  helpflag=parsearg(argc,argv,opts,files,NULL);
  if(helpflag==1||fnamcat[0]=='\0')
    {
      print_help("Usage: match_stack5b <options> (resultfile)",
		 opts,"");
      exit(-1);
    }
  
  /***************************************************************/
  /* read lines */

  fp=fopen(fnamcat,"r");
  if(fp==NULL)
    {
      fprintf(stderr,"Cannot open %s\n",fnamcat);
      exit(-1);
    }
  
  eps_m=fabs(log(eps_f));
  
  line=(char**)malloc(sizeof(char*)*nline);

  l=0; 
  n=0;
  while(1)      
    {
      if(fgets(buffer,BUFSIZ,fp)==NULL) break;
      if (sscanf(buffer,"%s %s",fnam1,fnam2)==2 && 
	  fnam1[0]!='#')
	{
	  n=adddic(n,&names,fnam1);
	  n=adddic(n,&names,fnam2);
	  line[l]=(char*)malloc(sizeof(char)*(strlen(buffer)+1));
	  strcpy(line[l],buffer);
	  l++;
	  if(l>=nline) 
	    {
	      nline*=2;
	      line=(char**)realloc(line,sizeof(char*)*nline);
	    }
	}
    }
  fclose(fp);

  nline=l;
  line=(char**)realloc(line,sizeof(char*)*nline);

  /***************************************************************/
  /* parse line, set data */

  dim=n*n;
  dx=(double*)calloc(dim,sizeof(double));
  dy=(double*)calloc(dim,sizeof(double));
  da=(double*)calloc(dim,sizeof(double));
  dm=(double*)calloc(dim,sizeof(double));

  ex=(double*)calloc(dim,sizeof(double));
  ey=(double*)calloc(dim,sizeof(double));
  ea=(double*)calloc(dim,sizeof(double)); 
  em=(double*)calloc(dim,sizeof(double));

  connect=(int*)calloc(n,sizeof(int));
  
  /* 2000/08/18 */
  /* banpei */
  for(i=0;i<dim;i++)
    em[i]=10000.0;

  for(l=0;l<nline;l++)
    {
      sscanf(line[l],"%s %s %lf %lf %lf %lf %lf %lf %lf %lf %d",
	     fnam1,fnam2,
	     &dx0, &dy0, &da0, &df0, &ex0, &ey0, &ea0, &ef0, &ndat);
      if(ex0>=0&&ef0>=0&&ea0>=0&&ef0>=0)
	{
	  i=name2no(n,names,fnam1);
	  j=name2no(n,names,fnam2);

	  /* 2001/05/22 */
	  /* ndat weight, how ??? */
	  idx=i+j*n;
	  dx[idx]=dx0;
	  ex[idx]=ex0;
	  dy[idx]=dy0;
	  ey[idx]=ey0;
	  da[idx]=da0;
	  ea[idx]=ea0;

	  if (fabs(ef0/df0)<1 && df0>0)
	    {
	      em[idx]=-log(1.0-fabs(ef0/df0));
	      dm[idx]=log(df0);
	    }
	  else
	    {
	      /* error */
	      em[idx]=10000.0;
	      dm[idx]=0.0;
	    }
	  free(line[l]);      
	}
    }
  free(line);

  if(n<1)
    {
      fprintf(stderr,"Error: no valid pairs\n");
      exit(-1);
    }

  /***************************************************************/
  
  ax=(double*)malloc((n-1)*sizeof(double));
  ay=(double*)malloc((n-1)*sizeof(double));
  aa=(double*)malloc((n-1)*sizeof(double));
  am=(double*)malloc((n-1)*sizeof(double));
  ax0=(double*)malloc((n-1)*sizeof(double));
  ay0=(double*)malloc((n-1)*sizeof(double));
  aa0=(double*)malloc((n-1)*sizeof(double));
  am0=(double*)malloc((n-1)*sizeof(double));

  dimdat=(n*(n-1))/2;
  dimmat=n*dimdat;

  mat_x=(double*)calloc(dimmat,sizeof(double));
  mat_y=(double*)calloc(dimmat,sizeof(double));
  mat_a=(double*)calloc(dimmat,sizeof(double));
  mat_m=(double*)calloc(dimmat,sizeof(double));

  dat_x=(double*)calloc(dimdat,sizeof(double));
  dat_y=(double*)calloc(dimdat,sizeof(double));
  dat_a=(double*)calloc(dimdat,sizeof(double));
  dat_m=(double*)calloc(dimdat,sizeof(double));


  /******************************************************************/
  /* check symmetry ?*/
  /* 1st path , a */

  m=0;
  for(i=0;i<n-1;i++)
    {
      for(j=i+1;j<n;j++)
	{
	  /* No xy symmetry check here */

	  /* 2001/03/13 */
	  id1=i+j*n;
	  id2=j+i*n;
	  ix1=i;
	  ix2=j;
	  
	  if(em[id1]==10000.0 && em[id2]!=10000.0) 
	    {
	      /* swap */
	      id1=j+i*n;
	      id2=i+j*n;
	      ix1=j; 
	      ix2=i;
	    }
	  idm1=(ix1-1)+m*(n-1);
	  idm2=(ix2-1)+m*(n-1);


	  if(em[id1]>eps_m)
	    {
	      if(em[id1]!=10000.0) /* banpei */
		{
		  if (!quietmode)
		    {
		      printf("%d %d %s - %s is not used\n",i,j,names[ix1],names[ix2]);
		      printf("%f(mag error) > %f(dm)\n",
			     em[id1],eps_m);
		    }
		}
	      else
		{
		  /* also not used */
		}
	    }
	  else if(ex[id1]*ex[id1]+ey[id1]*ey[id1]>eps_r*eps_r)
	    {
	      if (!quietmode)
		{
		      printf("%d %d %s - %s is not used\n",i,j,names[ix1],names[ix2]);
		  printf("%f(pos error^2) > %f(dr^2)\n",
			 ex[id1]*ex[id1]+ey[id1]*ey[id1],
			 eps_r*eps_r);
		}
	    }
	  else if(ea[id1]>eps_a)	     
	    {
	      if (!quietmode)
		{
		      printf("%d %d %s - %s is not used\n",i,j,names[ix1],names[ix2]);
		  printf("%f(angle error) > %f(da)\n",
			 ea[id1],eps_a);
		}
	    }	     
	  else if(fabs(dm[id1]+dm[id2])>eps_m &&
		  em[id2]!=10000.0) /* banpei */
	    {
	      if (!quietmode)
		{
		  printf("%d %d %s - %s is not used\n",i,j,names[ix1],names[ix2]);
		  printf("Flux information is not symmetric\n");	
		  printf("   ->  : %f (mag)\n",dm[id1]);
		  printf("   <-  : %f (mag)\n",dm[id2]);
		}
	    }
	  else if(fabs(da[id1]+da[id2])>eps_a && em[id2]!=10000.0)
	    {
	      if (!quietmode)
		{
		  printf("%d %d %s - %s is not used\n",i,j,names[ix1],names[ix2]);
		  printf("Angle information is not symmetric\n");	
		  printf("   ->  : %f (angle)\n",da[id1]);
		  printf("   <-  : %f (angle)\n",da[id2]);
		}
	    }
	  else
	    {
	      /* valid value */
	      if(em[id2]!=10000.0)
		{
		  da[id1]=0.5*(da[id1]-da[id2]);
		  ea[id1]=0.5*sqrt(ea[id1]*ea[id1]+
				   ea[id2]*ea[id2]);
		}

	      if(ea[id1]<1.e-7) ea[id1]=1.e-7;
	      
	      mat_a[idm2]=1.0/ea[id1];
	      
	      if(ix1>0)
		{
		  mat_a[idm1]=- mat_a[idm2];
		}
	      dat_a[m]=da[id1]/ea[id1];	      
	      
	      /* 2000/08/18 */

	      /* check connection */
	      if(connect[ix1]!=0)
		{
		  if(connect[ix2]==0)
		    connect[ix2]=connect[ix1];
		  else
		    {
		      ctmp=connect[ix2];
		      for(k=0;k<n;k++)
			if(connect[k]==ctmp)
			  connect[k]=connect[ix1];
		    }
		}
	      else
		{
		  if(connect[ix2]==0)
		    {
		      connect[ix2]=connect[ix1]=nc;
		      nc++;
		    }
		  else
		    connect[ix1]=connect[ix2];
		}
	      m++;
	    }
	}
    }

  /* here, ndat=m */

  /**********************************************************************/

  /* check connection */
  flag=0;
  for(k=0;k<n;k++)
    {
      if(connect[k]==0)
	{
	  printf("Error: %s is not connected to any other frame\n",
		 names[k]);
	  flag=1;
	  /* 2000/08/18, as rejection or partition is not inplemented yet */
	  if(k==0) break;
	}
      else if(connect[k]!=connect[0])
	{
	  printf("Error: %s is not connected to the reference frame %s\n",
		 names[k],names[0]);	  
	  flag=1;
	}
    }

  if(flag==1) 
    {
      /* rejection or partition is not inplemented yet (2000/08/18)*/
      exit(-1);
    }

  if(!quietmode)
    printf("calc angle.\n");

  /* 1st. solve the angle eqn. */
  /* mat_a * aa  = dat_a */

  /*******************************************************************/

  /* 2000/07/18 */
  if(m<n-1)
    {
      /* under determined */
      printf("Error: theta cannot be determined! too few constraints!\n");
      printf("       param=%d eqn=%d\n",n-1,m);
      printf("       all theta are set to be 0\n");
      for(i=0;i<n-1;i++)
	aa[i]=0.0;
    }
  else
    {
      /* main iteration loop */
      ndat=(m+n)/2;
      ndat0=n;

      chisq=(double*)malloc(niter*sizeof(double));
      chisqmin=100000.0;

      res=(double*)malloc(m*sizeof(double));
      id=(int*)malloc(m*sizeof(int));

      for (k=0;k<niter;k++)
	{
	  if(!quietmode)
	    printf("aitrer:k=%d ndat=%d\n",k,ndat);	  
	  chisq[k]=100000.0;

	  ret=solve(n-1,aa,ndat,dat_a,mat_a);

	  if(ret==1)
	    {
	      ndat=(m+ndat+1)/2;
	      continue;
	    }

	  /* here we get angles aa[i] */
	  if(!quietmode)
	    {
	      printf("iter %d\n",k);
	      for(i=0;i<n-1;i++)
		printf("%d a=%f\n",i+1,aa[i]);
	      printf("\n");
	    }

	  /* check chisq */
	  chisq[k]=0;
	  for(j=0;j<m;j++) /* not only used ndat, but also notused*/
	    {
	      id[j]=j;
	      r0=-dat_a[j];
	      for(i=0;i<n-1;i++)
		r0+=aa[i]*mat_a[i+j*(n-1)];
	      res[j]=fabs(r0);
	    }
	  heapsort2id(m,res,id);

	  id_reorder(m,id,sizeof(double),res);

	  chisq[k]=res[ndat-1]/ndat;


	  if (k==0 || chisq[k]<chisq[k-1])
	    {
	      /* OK, good */
	      id_reorder(m,id,(n-1)*sizeof(double),mat_a);
	      id_reorder(m,id,sizeof(double),dat_a);
	      id_reorder(m,id,(n-1)*sizeof(double),mat_x);
	      id_reorder(m,id,sizeof(double),dat_x);
	      id_reorder(m,id,(n-1)*sizeof(double),mat_y);
	      id_reorder(m,id,sizeof(double),dat_y);
	      id_reorder(m,id,(n-1)*sizeof(double),mat_m);
	      id_reorder(m,id,sizeof(double),dat_m);
	    }

	  if (chisq[k]<chisqmin*1.1)
	    {
	      if (chisq[k]<chisqmin) 
		{
		  chisqmin=chisq[k];
		  memcpy(aa0,aa,(n-1)*sizeof(double));
		  ndat0=ndat;
		}

	      if(k==0) /* initial */
		ndat=(m+n)/2;
	      else
		ndat=(m+ndat+1)/2;
	    }
	  else
	    { 
	      /* no good. retry */
	      if(ndat0==ndat) 
		break;
	      else
		ndat=(ndat0+ndat)/2;
	    }
	}
    }

  if(!quietmode)
    printf("recalc %d\n",ndat0);
  memcpy(aa,aa0,(n-1)*sizeof(double));

  /* MASK */
  for(j=ndat0;j<m;j++)
    {
      k=-1;
      for(i=0;i<n-1;i++)
	if(mat_a[i+j*(n-1)]!=0)
	  {
	    if(k==-1) k=i+1;
	    else 
	      {
		em[k+(i+1)*n]=em[(i+1)+k*n]=10000.0;

		if (!quietmode)
		  printf("Mask %s %s\n",names[k],names[i+1]);
		break;
	      }
	  }
      if(i==n && k!=-1)
	{
	  if (!quietmode)
	    printf("Mask %s %s\n",names[k],names[0]);
	  em[k]=em[k*n]=10000.0;
	}
    }

  /***************************************************************/

  /* Second path */
  /* rotate */

  if(!quietmode)
    printf("rotated.\n");

  for(i=1;i<n;i++)
    {
      /* 2000/08/18 !! */
      /* flag check needed ?? +aa or -aa ??? */
      /* => OK */
      c=cos(aa[i-1]);
      s=sin(aa[i-1]); 

      for(j=0;j<n;j++)
	{
	  id1=i+j*n;
	  /* rotate by aa[i]*/
	  x=dx[id1]*c-dy[id1]*s;
	  y=dx[id1]*s+dy[id1]*c;
	  dx[id1]=x;
	  dy[id1]=y;      
	  if(!quietmode)
	    if (x!=0||y!=0)
	      printf("%d %d x=%f y=%f\n",i,j,x,y);
	}  
    }

  /***************************************************************/

  /* clear up connection matrix */
  memset(connect,0,n*sizeof(int));

  m=0;
  for(i=0;i<n-1;i++)
    {
      for(j=i+1;j<n;j++)
	{

	  /* 2001/03/13 */
	  id1=i+j*n;
	  id2=j+i*n;
	  ix1=i;
	  ix2=j;

	  if(em[id1]==10000.0 && em[id2]!=10000.0) 
	    {
	      /* swap */
	      id1=j+i*n;
	      id2=i+j*n;
	      ix1=j; 
	      ix2=i;
	    }

	  idm1=(ix1-1)+m*(n-1);
	  idm2=(ix2-1)+m*(n-1);

	  if(em[id1]>eps_m)
	    {
	      if(em[id1]!=10000.0) /* banpei */
		{
		  if (!quietmode)
		    {
		      printf("%s - %s is not used\n",names[i],names[j]);
		      printf("%f(mag error) > %f(dm)\n",
			     em[id1],eps_m);
		    }
		}
	    }
	  else if(ex[id1]*ex[id1]+ey[id1]*ey[id1]>eps_r*eps_r)
	    {
	      if (!quietmode)
		{
		  printf("%s - %s is not used\n",names[ix1],names[ix2]);
		  printf("%f(pos error^2) > %f(dr^2)\n",
			 ex[id1]*ex[id1]+ey[id1]*ey[id1],
			 eps_r*eps_r);
		}
	    }
	  else if(ea[id1]>eps_a)	     
	    {
	      if (!quietmode)
		{
		  printf("%s - %s is not used\n",names[ix1],names[ix2]);
		  printf("%f(angle error) > %f(da)\n",
			 ea[id1],eps_a);
		}
	    }	     
	  else if(fabs(dm[id1]+dm[id2])>eps_m && em[id2]!=10000.0)
	    {
	      if (!quietmode)
		{
		  printf("%s - %s is not used\n",names[ix1],names[ix2]);
		  printf("Flux information is not symmetric\n");	
		  printf("   ->  : %f (mag)\n",dm[id1]);
		  printf("   <-  : %f (mag)\n",dm[id2]);
		}
	    }
	  else if(fabs(da[id1]+da[id2])>eps_a && em[id2]!=10000.0)
	    {
	      if (!quietmode)
		{
		  printf("%s - %s is not used\n",names[ix1],names[ix2]);
		  printf("Angle information is not symmetric\n");	
		  printf("   ->  : %f (angle)\n",da[id1]);
		  printf("   <-  : %f (angle)\n",da[id2]);
		}
	    }
	  else if(
		  ((dx[id1]+dx[id2])*(dx[id1]+dx[id2])+
		   (dy[id1]+dy[id2])*(dy[id1]+dy[id2])>eps_r*eps_r)
		  && em[id2]!=10000.0)
	    {
	      if (!quietmode)
		{
		  printf("%s - %s is not used\n",names[ix1],names[ix2]);
		  printf("Position information is not symmetric\n");	
		}
	    }
	  else
	    {
	      /* valid value */
	      if(em[id2]!=10000.0)
		{
		  dx[id1]=0.5*(dx[id1]-dx[id2]);
		  dy[id1]=0.5*(dy[id1]-dy[id2]);
		  dm[id1]=0.5*(dm[id1]-dm[id2]); 
		  
		  ex[id1]=0.5*sqrt(ex[id1]*ex[id1]+
				   ex[id2]*ex[id2]);
		  ey[id1]=0.5*sqrt(ey[id1]*ey[id1]+
				   ey[id2]*ey[id2]);
		  em[id1]=0.5*sqrt(em[id1]*em[id1]+
				   em[id2]*em[id2]);
		}

	      if(ex[id1]<0.005) ex[id1]=0.005;
	      if(ey[id1]<0.005) ey[id1]=0.005;
	      if(em[id1]<0.005) em[id1]=0.005;
	      
	      mat_x[idm2]=1.0/ex[id1];
	      mat_y[idm2]=1.0/ey[id1];
	      mat_m[idm2]=1.0/em[id1];

	      /* 2000/08/17 debug */
	      if(i>0)
		{
		  mat_x[idm1]= -mat_x[idm2]; 
		  mat_y[idm1]= -mat_y[idm2]; 
		  mat_m[idm1]= -mat_m[idm2]; 
		}

	      dat_x[m]=dx[id1]/ex[id1];
	      dat_y[m]=dy[id1]/ey[id1];
	      dat_m[m]=dm[id1]/em[id1];

	      /******************* check connection ****************/
	      if(connect[ix1]!=0)
		{
		  if(connect[ix2]==0)
		    connect[ix2]=connect[ix1];
		  else
		    {
		      ctmp=connect[ix2];
		      for(k=0;k<n;k++)
			if(connect[k]==ctmp)
			  connect[k]=connect[ix1];
		    }
		}
	      else
		{
		  if(connect[ix2]==0)
		    {
		      connect[ix2]=connect[ix1]=nc;
		      nc++;
		    }
		  else
		    connect[ix1]=connect[ix2];
		}
	      m++;
	    }
	 
	}
    }

  /***********************************************************************/

  /* check connection */
  flag=0;
  for(k=0;k<n;k++)
    {
      if(connect[k]==0)
	{
	  printf("Error: %s is not connected to any other frame after 1st rejection\n",
		 names[k]);
	  flag=1;
	  /* 2000/08/18, as rejection or partition is not inplemented yet */
	  if(k==0) break;
	}
      else if(connect[k]!=connect[0])
	{
	  printf("Error: %s is not connected to the reference frame %s after rejection\n",
		 names[k],names[0]);	  
	  flag=1;
	}
    }

  if(flag==1) 
    {
      /* rejection or partition is not inplemented yet (2000/08/18)*/
      exit(-1);
    }

  /******************************/

  /* 2000/07/18 */
  /* 2001/05/22 */
  if(m<n-1)
    {
      /* under determined */
      printf("Error: x,y,flux cannot be determined! too few constraints!\n");
      printf("       param=%d eqn=%d\n",n-1,m);
      printf("Exit\n");
      exit(-1);
    }
  else
    {
      /* main iteration loop */
      niter=20;
 
      ndat=(m+n)/2;
      ndat0=n;
      /* 2001/05/21 */
      /* check and reject */
      resx=(double*)malloc(m*sizeof(double));
      resy=(double*)malloc(m*sizeof(double));

      chisqmin=100000.0;

      for (k=0;k<niter;k++)
	{
	  chisq[k]=100000.0;
	  ret=0;
	  if (solve(n-1,ax,ndat,dat_x,mat_x)!=0 ||
	      solve(n-1,ay,ndat,dat_y,mat_y)!=0 ||
	      solve(n-1,am,ndat,dat_m,mat_m)!=0)
	    {
	      ret=1;	  
	      if(!quietmode)	  
		{
		  printf("Error: not solved ndat=%d\n",ndat);
		  printf("X=%d %d %d\n",solve(n-1,ax,ndat,dat_x,mat_x),n-1,ndat);
		  printf("Y=%d %d %d\n",solve(n-1,ax,ndat,dat_x,mat_x),n-1,ndat);
		  printf("M=%d %d %d\n",solve(n-1,ax,ndat,dat_x,mat_x),n-1,ndat);		
		  printf("Error: not solved. \n");
		}

	      if(ndat>=m)
		{
		  /* cannot solve */
		  ndat0=m;
		  break;
		}
	      ndat=(m+ndat+1)/2;
	      if(!quietmode)	  
		printf("Error: not solved. new ndat=%d\n",ndat);
	      continue;
	    }
	  else
	    {
	      /* calc residuals */
	      for(j=0;j<m;j++)
		{
		  id[j]=j;
		  resx[j]=-dat_x[j];
		  resy[j]=-dat_y[j];
		  for(i=0;i<n-1;i++)
		    {
		      resx[j]+=ax[i]*mat_x[i+j*(n-1)];
		      resy[j]+=ay[i]*mat_y[i+j*(n-1)];
		    }
		  res[j]=fabs(resx[j])+fabs(resy[j]);
		}	  
	      
	      /* sort by residual */
	      heapsort2id(m,res,id);

	      id_reorder(m,id,sizeof(double),res);

	      
	      chisq[k]=res[ndat-1]/ndat;
	    }

	  if (k==0 || chisq[k]<chisq[k-1])
	    {
	      /* OK, good */
	      id_reorder(m,id,(n-1)*sizeof(double),mat_x);
	      id_reorder(m,id,sizeof(double),dat_x);
	      id_reorder(m,id,(n-1)*sizeof(double),mat_y);
	      id_reorder(m,id,sizeof(double),dat_y);
	      id_reorder(m,id,(n-1)*sizeof(double),mat_m);
	      id_reorder(m,id,sizeof(double),dat_m);
	    }

	  if (chisq[k]<chisqmin*1.1)
	    {
	      if (chisq[k]<chisqmin) 
		{
		  chisqmin=chisq[k];
		  memcpy(ax0,ax,(n-1)*sizeof(double));
		  memcpy(ay0,ay,(n-1)*sizeof(double));
		  memcpy(am0,am,(n-1)*sizeof(double));
		  ndat0=ndat;
		}
	      if(k==0) /* initial */
		ndat=(m+n)/2;
	      else
		ndat=(m+ndat+1)/2;
	    }
	  else
	    { 
	      if(ndat0==ndat) 
		break;
	      else
		ndat=(ndat0+ndat)/2;
	    }
	}
    }

  if(!quietmode)
    printf("recalc %d\n",ndat0);
  
  /* recalc */
  memcpy(ax,ax0,(n-1)*sizeof(double));
  memcpy(ay,ay0,(n-1)*sizeof(double));
  memcpy(am,am0,(n-1)*sizeof(double));

  /*********************************************************************/
  printf("%s %f %f %f %f\n",names[0],0.0,0.0,0.0,1.0);
  for(i=0;i<n-1;i++)
    {
      printf("%s %f %f %f %f\n",names[i+1],ax[i],ay[i],aa[i],
	     exp(am[i]));
    }

  /* free */

  for(i=0;i<n;i++)
    free(names[i]);

  free(names);
  free(dx);
  free(dy);
  free(da);
  free(dm);
  free(ex);
  free(ey);
  free(ea);
  free(em);
  free(mat_x);
  free(mat_y);
  free(mat_a);
  free(mat_m);
  free(dat_x);
  free(dat_y);
  free(dat_a);
  free(dat_m);
  free(ax);
  free(ay);
  free(aa);
  free(am);
  free(connect);

  return 0;
}
