#ifndef SKYSB_SUB_H
#define SKYSB_SUB_H

float medfilter_skysb(float *g,int iext,int jext,int width) ;
float medfilter_skysb0(float *g,int iext,int jext,int width) ;
float skysigmadet(float *sigmap,int iext,int jext) ;
int statrj(float *g, int iext,int jext,
	   int is,int ie,int js,int je, 
           float fact, 
	   int ncycle, 
	   float *gmax,float *gmin,float *gmean, float *gsd,int *nrej,
	   float pixignr);
void statsk(float *g,int iext,int jext,float fact,int ncycle,
	    float *gmax,float *gmin,float *gmean,float *gsd,
	    float pixignr);
void skydet2b(float *g,int iext,int jext,float *rmesh,
	      int imesh,int jmesh,
	      int meshi,int meshj,float gmean,float gsd,
	      float fsigm,float *sgmesh,int binflag,
	      float pixignr);
float pxintp(float *g,int iext,int jext,float x,float y);
void skypattern(float *g,int iext,int jext,float *rmesh,int imesh,
		int jmesh,int meshi,int meshj);
void skysub(float *g,int npx,int npy,float *rmesh,int nmesh_x,
	    int nmesh_y,int meshsiz_x,int meshsiz_y,
	    int mode, float pixignr);

#endif
