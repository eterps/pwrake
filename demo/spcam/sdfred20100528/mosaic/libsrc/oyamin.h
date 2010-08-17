#ifndef OYAMIN_H
#define OYAMIN_H

int oyamin2_r(
	       int npar,     /*(I) Number of parameters */
	       double *p,    /*(I/O) Array of parameters [npar]*/
	       double *e,    /*(I) Array of Arrawance [npar]*/
	       double (*func)(int,double*,double,double,double),
	                     /*(I) Function to be minimized */
	       int ncut,     /*(I) Max iteration number */ 
	       int ndata,    /*(I) Number of data */
	       double *xx,   /*(I) Array of data X [ndata]*/
	       double *yy,   /*(I) Array of data Y [ndata]*/
	       double *er,   /*(I) Array of date error [ndata]*/ 
	       double *f,    /*(O) Array of chisq for each data [ndata]*/
	       double *chisq /*(O) Total Chi-squared */
	       );
/* oyamin2_r return values
   0: OK
   1: ncut error
   2: singiular alpha
   3: detivative is 0
  -1: unknown error */


/* another interface, for more than 3 variable function fitting */
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
	     );
/*
   func(npar,p,nvar,var)
   eg. func(npar,p,xx,yy,er) -> func(npar,p,3,v), 
   v[n][0]=xx[n], v[n][1]=yy[n] v[n][2]=er[n];

*/

double mativ2 (
	       int npar,     /*(I)   Dimension of Matrix */
	       double *alpha /*(I/O) Matrix to be inversed */
	       ); 
/* This function returns Determinant of Matrix alpha */

#endif
