#include <stdio.h>
#include <stdlib.h> 
#include <math.h>
#include <limits.h>
#include <string.h>

float aperturePhotometry(float *pix, int npx,int npy,
			 float xcen,float ycen,float r)
{
 int xmin,xmax,ymin,ymax;
 float xc[4],yc[4];
 int x,y;
 float temp,x0,y0;
 float x1,x2;
 float th1,th2;
 int i,flag;
 float factor;
 double sum;
 
 if (r<=0.) return 0.;

 xmin=(int)floor(xcen-r-.5);
 ymin=(int)floor(ycen-r-.5);
 xmax=(int)floor(xcen+r+.5);
 ymax=(int)floor(ycen+r+.5);
 
 if((xmin<0)||(ymin<0)||(xmax>npx)||(ymax>npy))
   {
     printf("ERROR\n");
     return -1.;
   }
 sum=0.;
 for(y=ymin;y<=ymax;y++)
   {
     for(x=xmin;x<=xmax;x++)
       {
         x0=x-xcen;y0=y-ycen;
         if(x0<0)x0=-x0;
         if(y0<0)y0=-y0;
         if (x0>y0) 
	   {
             temp=x0;x0=y0;y0=temp;
           }
         xc[0]=xc[1]=x0-.5;
         xc[2]=xc[3]=x0+.5;
         yc[0]=yc[3]=y0-.5;
         yc[1]=yc[2]=y0+.5;
	 /*  
	    1 2   
	    0 3
	    */
         flag=0;
         for(i=0;i<4;i++)
           if (xc[i]*xc[i]+yc[i]*yc[i]<r*r) flag++;
         switch(flag)
           {
           case 0:
             if ((r>y0-0.5)&&(x0>-.5)&&(x0<.5))
               {
                 x1=-sqrt(r*r-(y0-.5)*(y0-.5));
                 th1=asin(x1/r);
                 x2=-x1;
                 th2=-th1;               
                 factor=r*r/2.*(th2-th1)+
                   .5*x2*sqrt(r*r-x2*x2)-
                     .5*x1*sqrt(r*r-x1*x1)
                       -(y0-.5)*(x2-x1);
               }
             else factor=0.; /* TENUKI !!!! */
             break;
           case 4:
           factor=1.;
             break;
           case 1:
             x1=x0-.5;
             th1=asin(x1/r);
             x2=sqrt(r*r-(y0-.5)*(y0-.5));
             th2=asin(x2/r);
             factor=r*r/2.*(th2-th1)+
               .5*x2*sqrt(r*r-x2*x2)-
                 .5*x1*sqrt(r*r-x1*x1)
                   -(y0-.5)*(x2-x1);
             break;
           case 3:
             x1=sqrt(r*r-(y0+.5)*(y0+.5));
             th1=asin(x1/r);
             x2=x0+.5;
             th2=asin(x2/r);
             factor=r*r/2.*(th2-th1)+
               .5*x2*sqrt(r*r-x2*x2)-
                 .5*x1*sqrt(r*r-x1*x1)
                   -(y0-.5)*(x2-x1)+1.*(x1-x0+.5);
             break;
           case 2:
             x1=x0-.5;
             th1=asin(x1/r);
             x2=x0+.5;
             th2=asin(x2/r);
             factor=r*r/2.*(th2-th1)+
               .5*x2*sqrt(r*r-x2*x2)-
                 .5*x1*sqrt(r*r-x1*x1)
                   -(y0-.5)*1.;
             break;
           default:
             exit(-1);
           }
         sum+=(double)(factor*pix[y*npx+x]);
       }
   }
 return (float)sum;
}

