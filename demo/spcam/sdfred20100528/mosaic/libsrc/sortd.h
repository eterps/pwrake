#ifndef SORTD_H
#define SORTD_H

#include "sort.h"

extern void shellsort_d(int n, double a[]);
extern void mos_heapsort_d(int n,double a[]);
extern void heapsort_reverse_d(int n,double a[]);
extern void heapsort2_d(int n,double key[],double val[]);
extern void heapsort2id(int n, double key[], int id[]);
extern void id_reorder(int ndat,int id[],size_t siz,void *data);
extern double doublemin(int ndat,double *dat);
extern double doublemax(int ndat,double *dat);
extern double nthd(int ndat,double *dat,double n);
extern int *makeidlist(int n);

#endif
