#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/*
#define DEBUG
*/

#define   DIFF_CHISQ   0.0001  /* If the diffrence of chi2 between 
                              * two iterations becomes smaller 
                              * than DIFF_CHISQ, the iteration is 
                              * over.
                              */
/* alpha(i,j)=alpha[i+npar*j] */

double mativ2 (
	       int npar,     /*(I)   Dimension of Matrix */
	       double *alpha /*(I/O) Matrix to be inversed */
	       ) /* This function returns Determinant of Matrix alpha */
{
  double *x=NULL;
  int *perm=NULL; /* Array of Indices */
  int iw;
  int i,j,k,l=-1;
  double w,pivot,pivi;
  double max,eps;
  int n;
  double det;

  n=npar;
  det=1.0;

  if((perm=(int*)realloc(perm,sizeof(int)*n))==NULL)
    {
     fprintf(stderr,"Cannot allocate enough area for perm. exit.\n");
     exit(-1);
   }
  if((x=(double*)realloc(x,sizeof(double)*n))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for x. exit.\n");
     exit(-1);
   }

  for(i=0;i<n;i++) perm[i]=i;
  eps=0.0; 

  for(k=0;k<n;k++)
    {
      max=0.0; 
      for(j=k;j<n;j++)
        {
          w=fabs(alpha[k+npar*j]);
          if (w>max)
            {
              max=w;
              l=j;
            }
        }
      if (eps*0.01>=max)
        {
          return 0;
        }
      pivot=alpha[k+npar*l];
      
      det*=pivot;
      pivi=1.0/pivot;
      if(l!=k)
        { 
          iw=perm[k];                                                        
          perm[k]=perm[l];                                                   
          perm[l]=iw;                                                       
          for(i=0;i<n;i++)
            {
              w=alpha[i+npar*k];
              alpha[i+npar*k]=alpha[i+npar*l];
	      alpha[i+npar*l]=w;
            }
        }
      for(j=0;j<n;j++)
        {
          x[j]=alpha[k+npar*j]*pivi;
          alpha[k+npar*j]=x[j];
        }
      
      for(i=0;i<n;i++)
        {
          if(i!=k)
            {
              w=alpha[i+npar*k];
              if(w!=0.)
                {
                  for(j=0;j<n;j++)
                    {
                      if(j!=k) alpha[i+npar*j]-=w*x[j];
                    }
                  alpha[i+npar*k]=-w*pivi;
                }
            }
        }
      alpha[k+npar*k]=pivi;
     if (max*1.0e-6>eps) eps=max*1.0e-6; 
    }
  
  for(i=0;i<n;i++)
    {
      while(1)
        {
          k=perm[i];
          if (k==i)break;
          iw=perm[k];
          perm[k]=perm[i];
          perm[i]=iw;
          for(j=0;j<n;j++)
            {
              w=alpha[i+npar*j];
              alpha[i+npar*j]=alpha[k+npar*j];
              alpha[k+npar*j]=w;
            }
          det=-det;
        }
    }    
  
  return det;
}


int oyamin2b(
	     int npar,     /*(I) Number of parameters */
	     double *p,    /*(I/O) Array of parameters [npar]*/
	     double *e,    /*(I) Array of Parameter's units [npar]*/
	     int ncut,     /*(I) Max iteration number */ 
	     int ndata,    /*(I) Number of data */
	     double *f,    /*(O) Array of chisq for each data [ndata]*/
	     double *chisq, /*(O) Total Chi-squared */
	     double (*func)(int,double*,int,double*),
	     /*(I) Function to be minimized */
	     int narg,
	     double **args
	     )
{
 double *alpha=NULL;
 double *beta=NULL;
 double *xfc=NULL;
 double *deriv=NULL; 
 double *h=NULL;
 double fkgita,chisq1,chisq0;
 double ff,crt;
 int i,j,k;
 int kount;

 /* 2000/03/09 */
 /* eg. args[0][0]=xx[0]
        args[0][1]=yy[0]
        args[0][2]=er[0] ... */

 /***** Memory allocation *****/
 if((alpha=(double*)malloc(npar*npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for alpha. exit.\n");
     exit(-1);
   }
 if((beta=(double*)malloc(npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for beta. exit.\n");
     exit(-1);
   }
 if((xfc=(double*)malloc(npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for xfc. exit.\n");
     exit(-1);
   }
 if((deriv=(double*)malloc(npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for deriv. exit.\n");
     exit(-1);
   }
 if((h=(double*)malloc(npar*npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for h. exit.\n");
     exit(-1);
   }

 /********** Initialize (sakai2) ************/
 fkgita=0.001;
 kount=0;
 
 for(j=0;j<npar;j++) xfc[j]=p[j];
 chisq0=0.0;
 for(i=0;i<ndata;i++)
   {
     f[i]=func(npar,xfc,narg,args[i]);
     chisq0+=f[i]*f[i];
   }

 /* 2000/03/09 for debug */
#ifdef DEBUG
 for(j=0;j<npar;j++)fprintf(stderr,"%g ",xfc[j]);
 fprintf(stderr,"%g %g\n",chisq0,chisq0/ndata);
#endif

 if (ncut==0) 
   {
     fprintf(stderr,"Ncut error??\n");

     free(alpha);free(beta);free(xfc);free(deriv);free(h);
     return 1;
   }
 
 /************ Main Loop **************/

 while(1)
   {
     
     /***** Parameters changed.
       Calculate derivatives (sakai2) *****/

     kount++;

     /* 2000/03/09 */
#ifdef DEBUG
     fprintf(stderr,"kount:%d\n",kount);
#endif

     if (kount>ncut) 
       {
	 fkgita=0.0;
       }
     else
       {
	 for(j=0;j<npar;j++)
	   {
	     beta[j]=0.0; 
	     for(k=0;k<npar;k++)
	       alpha[j+npar*k]=0.0;
	   }
	 
	 for(i=0;i<ndata;i++)
	   {
/*
	     fprintf(stderr,"deriv:%d/%d\n",i,ndata);
*/

	     for(j=0;j<npar;j++)
	       {

/*
		 fprintf(stderr,"      j:%d/%d\n",j,npar);
*/

		 xfc[j]=xfc[j]+e[j];
		 ff=func(npar,xfc,narg,args[i]);
		 deriv[j]=( ff -f[i] )/e[j];
		 xfc[j]=xfc[j]-e[j];
	       }
	     for(j=0;j<npar;j++)
	       {
		 beta[j]-=f[i]*deriv[j];
		 for(k=0;k<=j;k++)
		   alpha[j+npar*k]+=deriv[j]*deriv[k];
	       }
	   }

#ifdef DEBUG
	 /* 2000/03/09 for debug */
	 for(j=0;j<npar;j++)fprintf(stderr,"%g ",xfc[j]);
	 fprintf(stderr,"--\n");
#endif

	 for(j=0;j<npar;j++)
	   {
	     for(k=0;k<=j;k++)
	       alpha[k+npar*j]=alpha[j+npar*k];
	     deriv[j]=(double)sqrt(alpha[j+npar*j]);
	     if(deriv[j]==0.0) 
	       {
		 fprintf(stderr,"derivative is 0\n");
		 free(alpha);free(beta);free(xfc);free(deriv);free(h);
		 return 3;
	       }
	   }
       }
     
     while(1)
       { 
     /************* Only Fkgita changed. 
                    Recalculate Error Matrix (sakai2) *******************/
	 for(j=0;j<npar;j++)
	   {
	     for(k=j+1;k<npar;k++)
	       {
		 h[j+npar*k]=alpha[j+npar*k]/(deriv[j]*deriv[k]);
		 h[k+npar*j]=h[j+npar*k];
	       }
	     h[j+npar*j]=1.0+fkgita;                                        
	   }
     
	 /************** Inversing Error Matrix ******************/

	 if (mativ2(npar,h)==0)
	   {
	     fprintf(stderr,"Not regular ALPHA!\n");

	     free(alpha);free(beta);free(xfc);free(deriv);free(h);
	     return 2;
/*
	     break;
*/
	   }

	 /************ Move parameters (igara2)***************/
	     
	 for(j=0;j<npar;j++)
	   {
	     xfc[j]=p[j];
	     for(k=0;k<npar;k++)
	       {
		 h[j+npar*k]=h[j+npar*k]/(deriv[j]*deriv[k]);
		 xfc[j]+=beta[k]*h[j+npar*k];
	       }
	   }

	 if ((fkgita)==0.0)
	   {
	     *chisq=chisq0;

	     free(alpha);free(beta);free(xfc);free(deriv);free(h);
	     return 0;
             /****** Here is exit *****/
	   } 
	 else
	   {
	     chisq1=0.0;                                                         
	     for(i=0;i<ndata;i++)
	       {
		 f[i]=func(npar,xfc,narg,args[i]);
		 chisq1 += f[i]*f[i];
	       }

#ifdef DEBUG
	     /* 2000/03/09 for debug */
	     for(j=0;j<npar;j++)fprintf(stderr,"%g ",xfc[j]);
	     fprintf(stderr,"%g %g\n",chisq1,chisq1/ndata);
#endif
	 
	     if(chisq0 <(chisq1-DIFF_CHISQ)) 
	       {
		 fkgita*=10.0;
		 continue;
	       }
	     fkgita*=0.1;
	     i=0;
	     if (chisq0-chisq1>1.0) i=1;
	     for(j=0;j<npar;j++)
	       {
		 crt=(double)fabs(xfc[j]-p[j]);
		 if (crt>e[j]) i=1;
		 p[j]=xfc[j];
	       }
	     chisq0=chisq1;
	     
	     if(i>0) 
	       break;
	     else
	       fkgita=0.0;
	   }
	 
       } 
   }
 fflush(stdout);

 free(alpha);free(beta);free(xfc);free(deriv);free(h);
 return -1;
}


/* OLD xx,yy,er style */


int oyamin2_r(
	       int npar,     /*(I) Number of parameters */
	       double *p,    /*(I/O) Array of parameters [npar]*/
	       double *e,    /*(I) Array of Parameter's units [npar]*/
	       double (*func)(int,double*,double,double,double),
	                     /*(I) Function to be minimized */
	       int ncut,     /*(I) Max iteration number */ 
	       int ndata,    /*(I) Number of data */
	       double *xx,   /*(I) Array of data X [ndata]*/
	       double *yy,   /*(I) Array of data Y [ndata]*/
	       double *er,   /*(I) Array of date error [ndata]*/ 
	       double *f,    /*(O) Array of chisq for each data [ndata]*/
	       double *chisq /*(O) Total Chi-squared */
	       )
{
 double *alpha=NULL;
 double *beta=NULL;
 double *xfc=NULL;
 double *deriv=NULL; 
 double *h=NULL;
 double fkgita,chisq1,chisq0;
 double ff,crt;
 int i,j,k;
 int kount;


 /***** Memory allocation *****/
 if((alpha=(double*)malloc(npar*npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for alpha. exit.\n");
     exit(-1);
   }
 if((beta=(double*)malloc(npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for beta. exit.\n");
     exit(-1);
   }
 if((xfc=(double*)malloc(npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for xfc. exit.\n");
     exit(-1);
   }
 if((deriv=(double*)malloc(npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for deriv. exit.\n");
     exit(-1);
   }
 if((h=(double*)malloc(npar*npar*sizeof(double)))==NULL)
   {
     fprintf(stderr,"Cannot allocate enough area for h. exit.\n");
     exit(-1);
   }

 /********** Initialize (sakai2) ************/
 fkgita=0.001;
 kount=0;
 
 for(j=0;j<npar;j++) xfc[j]=p[j];
 chisq0=0.0;
 for(i=0;i<ndata;i++)
   {
     f[i]=func(npar,xfc,(double)xx[i],(double)yy[i],(double)er[i]);
     chisq0+=f[i]*f[i];
   }
 
 if (ncut==0) 
   {
     fprintf(stderr,"Ncut error??\n");

     free(alpha);free(beta);free(xfc);free(deriv);free(h);
     return 1;
   }
 
 /************ Main Loop **************/

 while(1)
   {

 /***** Parameters changed.
        Calculate derivatives (sakai2) *****/

     kount++;
     
     if (kount>ncut) 
       {
	 fkgita=0.0;
       }
     else
       {
	 for(j=0;j<npar;j++)
	   {
	     beta[j]=0.0; 
	     for(k=0;k<npar;k++)
	       alpha[j+npar*k]=0.0;
	   }
	 for(i=0;i<ndata;i++)
	   {
	     for(j=0;j<npar;j++)
	       {
		 xfc[j]=xfc[j]+e[j];
		 ff=func(npar,xfc,(double)xx[i],(double)yy[i],(double)er[i]);
		 deriv[j]=( ff -f[i] )/e[j];
		 xfc[j]=xfc[j]-e[j];
	       }
	     for(j=0;j<npar;j++)
	       {
		 beta[j]-=f[i]*deriv[j];
		 for(k=0;k<=j;k++)
		   alpha[j+npar*k]+=deriv[j]*deriv[k];
	       }
	   }

	 for(j=0;j<npar;j++)
	   {
	     for(k=0;k<=j;k++)
	       alpha[k+npar*j]=alpha[j+npar*k];
	     deriv[j]=(double)sqrt(alpha[j+npar*j]);
	     if(deriv[j]==0.0) 
	       {
		 printf("derivative is 0\n");
		 free(alpha);free(beta);free(xfc);free(deriv);free(h);
		 return 3;
	       }
	   }
       }
     
     while(1)
       { 
     /************* Only Fkgita changed. 
                    Recalculate Error Matrix (sakai2) *******************/
	 for(j=0;j<npar;j++)
	   {
	     for(k=j+1;k<npar;k++)
	       {
		 h[j+npar*k]=alpha[j+npar*k]/(deriv[j]*deriv[k]);
		 h[k+npar*j]=h[j+npar*k];
	       }
	     h[j+npar*j]=1.0+fkgita;                                        
	   }
     
	 /************** Inversing Error Matrix ******************/

	 if (mativ2(npar,h)==0)
	   {
	     printf("Not regular ALPHA!\n");

	     free(alpha);free(beta);free(xfc);free(deriv);free(h);
	     return 2;
/*
	     break;
*/
	   }

	 /************ Move parameters (igara2)***************/
	     
	 for(j=0;j<npar;j++)
	   {
	     xfc[j]=p[j];
	     for(k=0;k<npar;k++)
	       {
		 h[j+npar*k]=h[j+npar*k]/(deriv[j]*deriv[k]);
		 xfc[j]+=beta[k]*h[j+npar*k];
	       }
	   }
	 
	 if ((fkgita)==0.0)
	   {
	     *chisq=chisq0;

	     free(alpha);free(beta);free(xfc);free(deriv);free(h);
	     return 0;
             /****** Here is exit *****/
	   } 
	 else
	   {
	     chisq1=0.0;                                                         
	     for(i=0;i<ndata;i++)
	       {
/*
		 ff=func(npar,xfc,(double)xx[i],(double)yy[i],(double)er[i]);
		 chisq1 += ff*ff;
*/
		 f[i]=func(npar,xfc,(double)xx[i],(double)yy[i],(double)er[i]);
		 chisq1 += f[i]*f[i];
	       }
	     if(chisq0 <(chisq1-1.e-3)) 
	       {
		 fkgita*=10.0;
		 continue;
	       }
	     fkgita*=0.1;
	     i=0;
	     if (chisq0-chisq1>1.0) i=1;
	     for(j=0;j<npar;j++)
	       {
		 crt=(double)fabs(xfc[j]-p[j]);
		 if (crt>e[j]) i=1;
		 p[j]=xfc[j];
	       }
	     chisq0=chisq1;
	     
	     if(i>0) 
	       break;
	     else
	       fkgita=0.0;
	   }
	 
       } 
   }
 fflush(stdout);

 free(alpha);free(beta);free(xfc);free(deriv);free(h);
 return -1;
}
