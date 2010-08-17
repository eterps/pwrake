#ifndef SORT_H
#define SORT_H

#define radixsort(a,b) mos_radixsort(a,b)
#define heapsort(a,b) mos_heapsort(a,b)
extern void mos_radixsort(int n, unsigned int a[]);
extern void mos_heapsort(int n,float a[]);

extern void shellsort(int n, float a[]);
extern void heapsort_reverse(int n,float a[]);

extern void heapsort2(int n,float key[],float val[]);

extern float floatmin(int ndat,float *dat);
extern float floatmax(int ndat,float *dat);
extern float nth(int ndat,float *dat,float n);
#endif
