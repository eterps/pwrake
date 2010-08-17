#ifndef STATD_H
#define STATD_H

#include "sortd.h"

extern int doubleweightedmeanrms(int ndat, double *dat, double *weight,
				 double *mean, double *sigma);
extern int doublemeanrms(int ndat, double *dat, double *mean, double *sigma);
extern double doubleweightedmedian(int ndat, double *dat, double *weight);
extern double doublemedian(int ndat,double *dat);
extern double doublequartile0(int ndat,double *dat);
extern double doublequartile2(int ndat,double *dat);
extern double doubleMAD(int ndat,double *dat);
extern double getTukey_d(int ndat,double *dat,double *med,double *mad);
extern double Tukey_d(int ndat,double *dat);

#ifndef MAD2SIGMA
#define MAD2SIGMA (1.48260221850560)
#endif 

#endif
