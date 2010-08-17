#ifndef STAT_H
#define STAT_H

#include "sort.h"
extern float floatmin(int ndat,float *dat);
extern float floatmax(int ndat,float *dat);
extern float nth(int ndat,float *dat,float n);
extern float floatmedian(int ndat,float *dat);
extern float floatquartile0(int ndat,float *dat);
extern float floatquartile2(int ndat,float *dat);
extern float floatMAD(int ndat,float *dat);
extern float getTukey2(int ndat,float *a,float *med,float *mad);
extern float Tukey(int ndat,float *dat);
extern float getTukey(int ndat,float *dat,float *med,float *mad);

extern int floatmeanrms(int ndat, float *dat, float *mean, float *sigma);
extern int floatweightedmeanrms(int ndat, float *dat, float *weight,
			 float *mean, float *sigma);
extern float floatweightedmedian(int ndat, float *dat, float *weight);

#ifndef MAD2SIGMA
/* 0.674489750196082 */
#define MAD2SIGMA (1.48260221850560)
#endif 


#endif
