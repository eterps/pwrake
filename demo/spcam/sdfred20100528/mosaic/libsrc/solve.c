#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* Singular Value Decomposition (originally from NRC) */

double pythag(double a, double b)
{
  double absa,absb;
  absa=fabs(a);
  absb=fabs(b);
  if (absa > absb) return absa*sqrt(1.0+(absb*absb)/(absa*absa));
  else if (absb==0.0) return 0.0;
  else return absb*sqrt(1.0+(absa*absa)/(absb*absb));
}

void svdcmpN(double **a, int m, int n, double w[], double **v)
{
  int flag,i,its,j,jj,k,l=0,nm=0;
  double anorm,c,f,g,h,s,scale,x,y,z,*rv1;
  int imin;
  double tmp;

  if((rv1=(double *)malloc(n*sizeof(double)))==NULL)
    {
      fprintf(stderr,"Cannot allocate rv1 in stdcmpN(solve.c), ndat=%d\n",n);
      exit(-1);
    }
  
  g=scale=anorm=0.0;
  for (i=0;i<n;i++) {
    l=i+1;
    rv1[i]=scale*g;
    g=s=scale=0.0;
    if (i < m) {
      for (k=i;k<m;k++) scale += fabs(a[k][i]);
      if (scale) {
	for (k=i;k<m;k++) {
	  a[k][i] /= scale;
	  s += a[k][i]*a[k][i];
	}
	f=a[i][i];
	if(f>=0) g=-sqrt(s); else g=sqrt(s);
	h=f*g-s;
	a[i][i]=f-g;
	for (j=l;j<n;j++) {
	  for (s=0.0,k=i;k<m;k++) s += a[k][i]*a[k][j];
	  f=s/h;
	  for (k=i;k<m;k++) a[k][j] += f*a[k][i];
	}
	for (k=i;k<m;k++) a[k][i] *= scale;
      }
    }
    w[i]=scale *g;
    g=s=scale=0.0;
    if (i <m && i != (n-1)) {
      for (k=l;k<n;k++) scale += fabs(a[i][k]);
      if (scale) {
	for (k=l;k<n;k++) {
	  a[i][k] /= scale;
	  s += a[i][k]*a[i][k];
	}
	f=a[i][l];

	if(f>=0) g=-sqrt(s); else g=sqrt(s);
	h=f*g-s;
	a[i][l]=f-g;
	for (k=l;k<n;k++) rv1[k]=a[i][k]/h;
	for (j=l;j<m;j++) {
	  for (s=0.0,k=l;k<n;k++) s += a[j][k]*a[i][k];
	  for (k=l;k<n;k++) a[j][k] += s*rv1[k];
	}
	for (k=l;k<n;k++) a[i][k] *= scale;
      }
    }
    tmp=(fabs(w[i])+fabs(rv1[i]));
    if(anorm<tmp) anorm=tmp;
  }
  for (i=n-1;i>=0;i--) {
    if (i < (n-1)) {
      if (g) {
	for (j=l;j<n;j++)
	  v[j][i]=(a[i][j]/a[i][l])/g;
	for (j=l;j<n;j++) {
	  for (s=0.0,k=l;k<n;k++) s += a[i][k]*v[k][j];
	  for (k=l;k<n;k++) v[k][j] += s*v[k][i];
	}
      }
      for (j=l;j<n;j++) v[i][j]=v[j][i]=0.0;
    }
    v[i][i]=1.0;
    g=rv1[i];
    l=i;
  }

  if(m<n)imin=m; else imin=n;
  for (i=imin-1;i>=0;i--) {
    l=i+1;
    g=w[i];
    for (j=l;j<n;j++) a[i][j]=0.0;
    if (g) {
      g=1.0/g;
      for (j=l;j<n;j++) {
	for (s=0.0,k=l;k<m;k++) s += a[k][i]*a[k][j];
	f=(s/a[i][i])*g;
	for (k=i;k<m;k++) a[k][j] += f*a[k][i];
      }
      for (j=i;j<m;j++) a[j][i] *= g;
    } else for (j=i;j<m;j++) a[j][i]=0.0;
    ++a[i][i];
  }
  for (k=n-1;k>=0;k--) {
    for (its=0;its<30;its++) {
      flag=1;
      for (l=k;l>=0;l--) {
	nm=l-1;
	if ((double)(fabs(rv1[l])+anorm) == anorm) {
	  flag=0;
	  break;
	}
	if ((double)(fabs(w[nm])+anorm) == anorm) break;
      }
      if (flag) {
	c=0.0;
	s=1.0;
	for (i=l;i<=k;i++) {
	  f=s*rv1[i];
	  rv1[i]=c*rv1[i];
	  if ((double)(fabs(f)+anorm) == anorm) break;
	  g=w[i];
	  h=pythag(f,g);
	  w[i]=h;
	  h=1.0/h;
	  c=g*h;
	  s = -f*h;
	  for (j=0;j<m;j++) {
	    y=a[j][nm];
	    z=a[j][i];
	    a[j][nm]=y*c+z*s;
	    a[j][i]=z*c-y*s;
	  }
	}
      }
      z=w[k];
      if (l == k) {
	if (z < 0.0) {
	  w[k] = -z;
	  for (j=0;j<n;j++) v[j][k] = -v[j][k];
	}
	break;
      }
      if (its == 30)
	{
	  printf("no convergence in 30 svdcmp iterations");
	  exit(1);
	}

      x=w[l];
      nm=k-1;
      y=w[nm];
      g=rv1[nm];
      h=rv1[k];
      f=((y-z)*(y+z)+(g-h)*(g+h))/(2.0*h*y);
      g=pythag(f,1.0);
      if(f>=0)
	f=((x-z)*(x+z)+h*((y/(f+g))-h))/x;
      else
	f=((x-z)*(x+z)+h*((y/(f-g))-h))/x;

      c=s=1.0;
      for (j=l;j<=nm;j++) {
	i=j+1;
	g=rv1[i];
	y=w[i];
	h=s*g;
	g=c*g;
	z=pythag(f,h);
	rv1[j]=z;
	c=f/z;
	s=h/z;
	f=x*c+g*s;
	g = g*c-x*s;
	h=y*s;
	y *= c;
	for (jj=0;jj<n;jj++) {
	  x=v[jj][j];
	  z=v[jj][i];
	  v[jj][j]=x*c+z*s;
	  v[jj][i]=z*c-x*s;
	}
	z=pythag(f,h);
	w[j]=z;
	if (z) {
	  z=1.0/z;
	  c=f*z;
	  s=h*z;
	}
	f=c*g+s*y;
	x=c*y-s*g;
	for (jj=0;jj<m;jj++) {
	  y=a[jj][j];
	  z=a[jj][i];
	  a[jj][j]=y*c+z*s;
	  a[jj][i]=z*c-y*s;
	}
      }
      rv1[l]=0.0;
      rv1[k]=f;
      w[k]=x;
    }
  }
  free(rv1);
  return;
}

int solve(int nx,double *a,int nb, double *b,double *mat)
{
  /* mat*a=b -> return a */

 double **A,**V,*w,*z;

 double temp,wmax;
 int i,j,k;
 int flag=0;

 if ((A=(double **)malloc(nb*sizeof(double *)))==NULL) /* matrix */
   {
     fprintf(stderr,"Cannot allocate A in solve, ndat(nb)=%d\n",nb);
     exit(-1);
   }

 if ((V=(double **)malloc(nx*sizeof(double *)))==NULL) /* matrix */
   {
     fprintf(stderr,"Cannot allocate V in solve, ndat(nx)=%d\n",nx);
     exit(-1);
   } 
 if ((w=(double *)calloc(nx,sizeof(double)))==NULL) /*  */
   {
     fprintf(stderr,"Cannot allocate w in solve, ndat(nx)=%d\n",nx);
     exit(-1);
   }

 if ((z=(double *)malloc(nx*sizeof(double)))==NULL) /*  */
   {
     fprintf(stderr,"Cannot allocate z in solve, ndat(nx)=%d\n",nx);
     exit(-1);
   }

 for (i=0;i<nx;i++) 
   {
     if((V[i]=(double *)malloc(nx*sizeof(double)))==NULL)
       {
	 fprintf(stderr,"Cannot allocate V[%d] in solve, ndat(nx)=%d\n",
		 i,nx);
	 exit(-1);
       }
   }

 for (i=0;i<nb;i++) 
   {
     if ((A[i]=(double *)malloc(nx*sizeof(double)))==NULL)
       {
	 fprintf(stderr,"Cannot allocate A[%d] in solve, ndat(nx)=%d\n",
		 i,nx);
	 exit(-1);
       }
    for(j=0;j<nx;j++)
      A[i][j]=mat[j+nx*i];
  }

/*
 fprintf(stderr,"SVD...\n");
*/

 svdcmpN(A,nb,nx,w,V);

/*
 fprintf(stderr,"S Values are...\n");
 for(i=0;i<nx;i++) printf("%g\n",w[i]);
*/

 wmax=w[0];
 for(i=0;i<nx;i++) 
   { 
    for(j=0;j<nx;j++)
       {
	 temp=0.0;
	 for (k=0;k<nx;k++) temp+=V[i][k]*V[j][k];
      }
    if (w[i]>wmax) wmax=w[i];
  }

 for (i=0;i<nx;i++) 
 {
   z[i]=0.0;
   if (w[i]>1.0e-6*wmax) 
     {
       for (j=0;j<nb;j++)
	 z[i]+=A[j][i]*b[j];
       z[i]/=w[i];
     }
   else 
     {
       /* 2000/08/18 supress */
       /*
	 printf("Value %f is ignored.\n",w[i]);
	 printf("This means Standard Object Data is not good.\n");
       */
       flag=1;
     }
 }

 for (i=0;i<nx;i++)
   {
    a[i]=0.0;
    for (j=0;j<nx;j++) a[i]+=V[i][j]*z[j];
  }

  for(i=0;i<nb;i++) free(A[i]);
  free(A);
  for(i=0;i<nx;i++) free(V[i]);
  free(V);
  free(w);  
  free(z);  

 return flag;
}

