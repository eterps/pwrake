/*********************************
 * 1992.12.21                    *
 * Subroutine version of match.c *
 *********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "match_sub.h"
#include "stat.h"
#include "statd.h"

/* Definition of Data Types */

typedef struct triangle
{
  int		v1, v2, v3;	/* number of vertices */
  double	log_p, d_log_p;	/* logarithm of perimeter */
  int		orien;		/* 1 or -1 */
  double	ratio, d_ratio;
  double	cosine, d_cosine;
} TRIANGLE;

typedef struct node
{
  int		id1, id2;
  double	key;
  struct node	*next;
} NODE;

typedef struct matched_triangle
{
  int		id1, id2;
  int		sense;
  double	log_M;
} MATCH_TRI;
/**********************************************************************/

/* Definition of Subroutines */

double length(POINT pt1, POINT pt2)
{
  return sqrt(sqr(pt1.x-pt2.x) + sqr(pt1.y-pt2.y));
}

double length2(POINT pt1, POINT pt2)
{
  return sqr(pt1.x-pt2.x)+sqr(pt1.y-pt2.y);
}

void select_point(POINT pt[], int *num_of_point, double epsilon)
{
  int i,j,k,n;
  double eps2;
  /* remove points whose distance < 3*eps */

  eps2=sqr(3.0*epsilon);

  n=*num_of_point;
  for (i=0; i<n-1; i++) 
    for (j=i+1; j<n; j++)
      { 
	if (length2(pt[i],pt[j])<eps2) 
	  {
	    /* remove pt[j] here */ 
	    for (k=j; k<n-1; k++)
	      pt[k] = pt[k+1];
	    n--;
	  }
      }
  *num_of_point=n;
  /* 2002/02/03 re-ID */
  for (i=0; i<n; i++) 
    pt[i].id=i;

  return;
}

int ccw(POINT pt0, POINT pt1, POINT pt2)
{
  /* 0->1->2 is CCW or CW */
  double	dx1, dx2, dy1, dy2;

  dx1 = pt1.x - pt0.x; 
  dy1 = pt1.y - pt0.y;
  dx2 = pt2.x - pt0.x; 
  dy2 = pt2.y - pt0.y;
  if (dx1*dy2 > dy1*dx2) return 1;	   /* counter-clockwise */
  if (dx1*dy2 < dy1*dx2) return -1;	   /* clockwise */
  return 0;
}


/************************************************************************/


int make_triangle_list(TRIANGLE *tri[],
		       int num_of_pt, POINT pt[], double epsilon)
{
  int		i, j, k, l;
  double	x2, y2, z2, ratio;
  double	max_length2, mid_length2, min_length2;
  double factor;
  double sum;
  TRIANGLE *tri_buffer;
  int n;
  int *id;
  double *rat;
#define RATIO_MAX 10.0 

  n=(size_t)(num_of_pt*(num_of_pt-1)*(num_of_pt-2)/6);
  *tri = (TRIANGLE*) malloc(n*sizeof(TRIANGLE));
  tri_buffer = (TRIANGLE*) malloc(n*sizeof(TRIANGLE));

  rat=(double*)malloc(n*sizeof(double));
  id=(int*)malloc(n*sizeof(int));

  if (*tri == NULL || tri_buffer==NULL) 
    {
      (void)fprintf(stderr, "*tri : Out of memory\n");
      exit(0);
    }
  if (rat==NULL || id==NULL)
    {
      (void)fprintf(stderr, "*tri : Out of memory\n");
      exit(0);
    }

  l = 0;
  for (i=0; i<num_of_pt-2; i++) 
    {
      for (j=i+1; j<num_of_pt-1; j++) 
	{
	  for (k=j+1; k<num_of_pt; k++) 
	    {
	      x2 = length2(pt[j], pt[k]);
	      y2 = length2(pt[k], pt[i]);
	      z2 = length2(pt[i], pt[j]);


	      /* 2004/07/19 some architecture dumps core with old code*/
	      /* the reason is unclear. this is patchy workarounds */
#if 1
	      if (x2<y2&&x2<z2)
		{
		  min_length2=x2;
		  (*tri)[l].v1 = i;
		  if(y2<z2)
		    {
		      (*tri)[l].v2 = j; (*tri)[l].v3 = k;
		      max_length2=z2;
		      mid_length2=y2;
		    }
		  else
		    {
		      (*tri)[l].v2 = k; (*tri)[l].v3 = j;
		      max_length2=y2;
		      mid_length2=z2;
		    }
		}
	      else if(y2<=x2&&y2<z2)
		{
		  min_length2=y2;
		  (*tri)[l].v1 = j;
		  if(x2<z2)
		    {
		      (*tri)[l].v2 = i; (*tri)[l].v3 = k;
		      max_length2=z2;
		      mid_length2=x2;
		    }
		  else
		    {
		      (*tri)[l].v2 = k; (*tri)[l].v3 = i;
		      max_length2=x2;
		      mid_length2=z2;
		    }
		}
	      else
		{
		  min_length2=z2;
		  (*tri)[l].v1 = k;
		  if(x2<y2)
		    {
		      (*tri)[l].v2 = i; (*tri)[l].v3 = j;
		      max_length2=y2;
		      mid_length2=x2;
		    }
		  else
		    {
		      (*tri)[l].v2 = j; (*tri)[l].v3 = i;
		      max_length2=x2;
		      mid_length2=y2;
		    }
		}
#else
	      min_length2 = min(min(x2,y2),z2);
	      max_length2 = max(max(x2,y2),z2);

	      if ((y2==min_length2) && (z2==max_length2)) {
		(*tri)[l].v1 = i; (*tri)[l].v2 = k; (*tri)[l].v3 = j;
	      }
	      else if ((z2==min_length2) && (y2==max_length2)) {
		(*tri)[l].v1 = i; (*tri)[l].v2 = j; (*tri)[l].v3 = k;
	      }
	      else if ((z2==min_length2) && (x2==max_length2)) {
		(*tri)[l].v1 = j; (*tri)[l].v2 = i; (*tri)[l].v3 = k;
	      }
	      else if ((x2==min_length2) && (z2==max_length2)) {
		(*tri)[l].v1 = j; (*tri)[l].v2 = k; (*tri)[l].v3 = i;
	      }
	      else if ((x2==min_length2) && (y2==max_length2)) {
		(*tri)[l].v1 = k; (*tri)[l].v2 = j; (*tri)[l].v3 = i;
	      }
	      else if ((y2==min_length2) && (x2==max_length2)) {
		(*tri)[l].v1 = k; (*tri)[l].v2 = i; (*tri)[l].v3 = j;
	      }
	      mid_length2 = x2+y2+z2-min_length2-max_length2;
#endif
	      ratio = sqrt(max_length2/min_length2);

	      sum=sqrt(x2)+sqrt(y2)+sqrt(z2);
	      (*tri)[l].log_p = log10(sum);
	      (*tri)[l].ratio = ratio;

	      /* cos(v1) */
	      (*tri)[l].cosine
		= (min_length2 + max_length2 - mid_length2)
		  / (2.0 * sqrt(min_length2 * max_length2));

	      (*tri)[l].orien
		= ccw(pt[(*tri)[l].v1], pt[(*tri)[l].v2], pt[(*tri)[l].v3]);

	      factor=sqr(epsilon)*
		(max_length2+min_length2+mid_length2)/
		  (2.0*max_length2*min_length2);

	      /* NEED CHECK (2000/08/18) */
	      (*tri)[l].d_ratio=2.0*sqr((*tri)[l].ratio)*factor;
	      (*tri)[l].d_cosine=2.0*(1.0-sqr((*tri)[l].cosine))*factor
		+3.0*sqr((*tri)[l].cosine*factor);

	      /* 2000/08/18 */
	      (*tri)[l].d_log_p=
		log10((sum+3.*epsilon)/(sum-3.*epsilon));

	      /* 2001/05/20 */
	      tri_buffer[l]=(*tri)[l];
	      id[l]=l;
	      rat[l]=tri_buffer[l].ratio;

	      l++;
	    }
	}
    }

  heapsort2id(l,rat,id);

  for(i=0;i<l;i++)
    {
      (*tri)[i]=tri_buffer[id[i]];
    }
  free(tri_buffer);
  free(rat);
  free(id);
  return l;
}

double max_d_ratio(TRIANGLE tri[], int num_of_tri)
{
  double	max_value;
  int		i;

  max_value = tri[0].d_ratio;
  for (i=1; i<num_of_tri; i++)
    if (tri[i].d_ratio > max_value)
      max_value = tri[i].d_ratio;
  return max_value;
}

int matching_triangle(MATCH_TRI *mat_tri[],
		      TRIANGLE tria[], int nta,
		      TRIANGLE trib[], int ntb)
{
  int		i, j, k, start;
  int		id1=-1, id2=-1;
  double	max_d, res, key;
  TRIANGLE	*A, *B;
  int		na, nb, reverse;

  if (nta <= ntb) 
    {
      A = tria;	na = nta;
      B = trib;	nb = ntb;
      reverse = 0;
    } 
  else 
    {
      A = trib;	na = ntb;
      B = tria;	nb = nta;
      reverse = 1;
    }

  /* na <= nb */

  max_d = sqrt(max_d_ratio(A, na) + max_d_ratio(B, nb));

  *mat_tri = (MATCH_TRI*) calloc((size_t)na, sizeof(MATCH_TRI));

  if (*mat_tri == NULL) 
    {
      (void)fprintf(stderr, "*mat_tri : Out of memory\n");
      exit(-1);
    }

  /* A, B is already sorted by ratio */

  k = 0; 
  start = 0;
  for (i=0; i<na; i++) 
    {
      res = 9999.0;
      for(;start<nb;start++)
	if((A[i].ratio-B[start].ratio)<=max_d)break;

      for (j=start; j<nb; j++) 
	{
	  if ((B[j].ratio-A[i].ratio) > max_d) break;
	  
	  if (A[i].orien == B[j].orien &&   
	      sqr(A[i].ratio -B[j].ratio ) < (A[i].d_ratio +B[j].d_ratio )
	      && sqr(A[i].cosine-B[j].cosine) < (A[i].d_cosine+B[j].d_cosine)
	      /* 2000/08/18 */
	      && fabs(A[i].log_p - B[j].log_p) < (A[i].d_log_p+B[j].d_log_p)
	      )
	    {

	      /* 2000/08/18 */
	      key = sqr(A[i].ratio-B[j].ratio)/sqrt(A[i].d_ratio /B[j].d_ratio)
		* sqr(A[i].cosine-B[j].cosine)/
		  sqrt(A[i].d_cosine/B[j].d_cosine);

	      if (key < res) 
		{
		  id1 = i;
		  id2 = j;
		  res = key;
		}
	    }
	}

      if (res != 9999.0) 
	{
	  if (!reverse) 
	    {
	      (*mat_tri)[k].id1 = id1;
	      (*mat_tri)[k].id2 = id2;
	      (*mat_tri)[k].sense = A[id1].orien * B[id2].orien;
	      (*mat_tri)[k].log_M = A[id1].log_p - B[id2].log_p;
	    } 
	  else
	    {
	      (*mat_tri)[k].id1 = id2;
	      (*mat_tri)[k].id2 = id1;
	      (*mat_tri)[k].sense = B[id2].orien * A[id1].orien;
	      (*mat_tri)[k].log_M = B[id2].log_p - A[id1].log_p;
	    }
	  k++;
	}
    }
  return k;
}

void statics_of_match_tri(int num_of_match, MATCH_TRI mat_tri[],
			  double *mean_log_M, double *sd_log_M,
			  int *nplus, int *nminus)
{
  int	  i;

  float *dat,med,mad;

  dat=(float*)malloc(num_of_match*sizeof(float));

  *nplus = 0; *nminus = 0;

  for (i=0; i<num_of_match; i++) 
    {
      dat[i]=(float)mat_tri[i].log_M;
      /* 2000/08/18 */
      /* sense is always +, see matching_triangle() */
    }

  /* 2000/08/18 */
  (*nplus)=num_of_match;

  getTukey(num_of_match,dat,&med,&mad);
  
  *mean_log_M = (double)med ;
  *sd_log_M = (double)mad/0.6745;

  free(dat);
  return ;
}

int reject_match_list(int num_of_match, MATCH_TRI mat_tri[],
		      double mean, double sd, int factor)
{
  int		i, j;

  for(i=0; i<num_of_match; i++) 
    {
      if (fabs(mat_tri[i].log_M-mean) > (double)factor*sd) 
	{
	  for(j=i; j<num_of_match-1; j++)
	    mat_tri[j] = mat_tri[j+1];
	  num_of_match--;
	  i--;
	}
    }
  return num_of_match;
}

void reject_by_sense(int *num_of_match, MATCH_TRI mat_tri[], int sense)
{
  int		i, j;

  for (i=0; i<*num_of_match; i++)
    if (mat_tri[i].sense == sense) 
      {
	for (j=i; j<*num_of_match-1; j++)
	  mat_tri[j] = mat_tri[j+1];
	(*num_of_match)--;
	i--;
      }
}

int reject_false_match(int *num_of_match, MATCH_TRI mat_tri[])
{
  double	mean, sd;
  int		nplus, nminus;
  int		m_t, m_f;
  int		factor, status;
  int		old_num_of_match;

  while(1)
    {
      old_num_of_match = *num_of_match;

      statics_of_match_tri(*num_of_match, mat_tri,
			   &mean, &sd, &nplus, &nminus);
      m_t = abs(nplus-nminus);
      m_f = *num_of_match - m_t;

      /*
	if (m_f > m_t)
	factor = 1;
	else if (0.1*(float)m_t > (float)m_f)
	factor = 3;
	else
	factor = 2;
      */

      factor=3;

      *num_of_match = reject_match_list(old_num_of_match, mat_tri,
					mean, sd, factor);

      if (*num_of_match == old_num_of_match) 
	{
	  status = 1;
	  break;
	} 
      else if (*num_of_match == 0) 
	{
	  status = 0;
	  return 0;
	} 
    }

  /*
     if (m_f == 0)
     return status;
     else if (nplus > nminus)
     sense = -1;
     else
     sense = 1;
     reject_by_sense(num_of_match, mat_tri, sense);
     */

  return status;
}


/************************************************************************/

/* for pair sort by count */

void sort_pair_list(MATCH_PAIR pair[], int n)
{
  /* sort by ratio */
  int *id;
  double *count;
  MATCH_PAIR *pair_buffer;
  int i;

  count=(double*)malloc(n*sizeof(double));
  id=(int*)malloc(n*sizeof(int));
  pair_buffer=(MATCH_PAIR*)malloc(n*sizeof(MATCH_PAIR));

  for(i=0;i<n;i++)
    {
      id[i]=i;
      pair_buffer[i]=pair[i];
      count[i]=(double)(pair_buffer[i].count);
    }
  heapsort2id(n,count,id);

  for(i=0;i<n;i++)
    {
      pair[i]=pair_buffer[id[n-1-i]];
    }
  
  free(pair_buffer);
  free(count);
  free(id);
}


/************************************************************************/

int is_point_used(NODE *root, int id ,int i)
{
  NODE	*p;

  p = root;
  while (p != NULL) 
    {
      if ((i == 1 && p->id1 == id) || (i == 2 && p->id2 == id))
	return 0;
      p = p->next;
    }
  return 1;
}

int matched_pair(MATCH_PAIR *mat_pair[], int num_of_match,
		 MATCH_TRI mat_tri[], TRIANGLE tria[], TRIANGLE trib[],
		 int npa, int npb, POINT pta[], POINT ptb[])
{
  int		i, count;
  MATCH_PAIR	*pair;
  int idx1,idx2,idx3;
  int *flag_a,*flag_b;

  pair = (MATCH_PAIR*) calloc((size_t)(npa*npb), sizeof(MATCH_PAIR));
  if (pair == NULL) 
    {
      (void)fprintf(stderr, "pair : Out of memory\n");
      exit(-1);
    }

  *mat_pair = (MATCH_PAIR*) calloc((size_t)(npa*npb), sizeof(MATCH_PAIR));
  if (*mat_pair == NULL) 
    {
      (void)fprintf(stderr, "*mat_pair : Out of memory\n");
      exit(-1);
    }
  
  flag_a=(int*)calloc(npa, sizeof(int));
  if (flag_a == NULL) 
    {
      (void)fprintf(stderr, "flag_a : Out of memory\n");
      exit(-1);
    }

  flag_b=(int*)calloc(npb, sizeof(int));
  if (flag_b == NULL) 
    {
      (void)fprintf(stderr, "flag_b : Out of memory\n");
      exit(-1);
    }
   
  for (i = 0; i < num_of_match; i++) 
    {
      idx1=tria[mat_tri[i].id1].v1*npb+trib[mat_tri[i].id2].v1;
      idx2=tria[mat_tri[i].id1].v2*npb+trib[mat_tri[i].id2].v2;
      idx3=tria[mat_tri[i].id1].v3*npb+trib[mat_tri[i].id2].v3;

      pair[idx1].pt1= pta[tria[mat_tri[i].id1].v1];
      pair[idx1].pt2= ptb[trib[mat_tri[i].id2].v1];
      pair[idx1].count++;
      
      pair[idx2].pt1= pta[tria[mat_tri[i].id1].v2];
      pair[idx2].pt2= ptb[trib[mat_tri[i].id2].v2];
      pair[idx2].count++;
      
      pair[idx3].pt1= pta[tria[mat_tri[i].id1].v3];
      pair[idx3].pt2= ptb[trib[mat_tri[i].id2].v3];
      pair[idx3].count++;
    }

  sort_pair_list(pair, npa*npb);

 (*mat_pair)[0] = pair[0];
  count=1;
  for(i=1;i<npa*npb-1;i++)
    {
      /* if not_used, then add */
      if(flag_a[pair[i].pt1.id]==0 &&
	 flag_b[pair[i].pt2.id]==0)
      {
	(*mat_pair)[count] = pair[i];
	flag_a[pair[i].pt1.id]=1;
	flag_b[pair[i].pt2.id]=1;
	count++;
      }
      if (pair[i].count > pair[i+1].count*2) break;
    }
  free(pair);
  free(flag_a);
  free(flag_b);

  return count;
}

int	match(POINT *pta, int npa, POINT *ptb, int npb, double epsilon,
	      MATCH_PAIR *mat_pair[], int *nmp)
{
  int		nta, ntb;	/* number of triangles to be matched */
  int		nmt;		/* number of matched triangles */
  TRIANGLE	*tria, *trib;	/* pointer to triangle array */
  MATCH_TRI	*mat_tri;	/* pointer to matched triangles array */


  /*  Selecting The Points To Be Matched (section II.a)  */
  select_point(pta, &npa, epsilon);
  select_point(ptb, &npb, epsilon);

  /*
    #define WRITE
  */
  
  if (npa < 3 || npb < 3) {
    /*
      (void)fprintf(stderr, "Number of selected points is less than three.\n");
    */
    return -1;
  }

#ifdef	WRITE
  printf("After selection\n");
  printf("Number of points : A - %d, B - %d\n\n", npa, npb);
#endif

  /*  Generating Triangle Lists (section II.b)  */
  nta = make_triangle_list(&tria, npa, pta, epsilon);
  ntb = make_triangle_list(&trib, npb, ptb, epsilon);
  if (nta < 1 || ntb < 1) {
    /*
       (void)fprintf(stderr, "Number of triangles is less than one.\n");
       */
    return -1;
  }

#ifdef	WRITE
  printf("Number of triangles : A - %d, B - %d\n\n", nta, ntb);
#endif

  /*  Matching Triangles (section II.c)  */
  nmt = matching_triangle(&mat_tri, tria, nta, trib, ntb);

#ifdef	WRITE
  printf("Number of matched triangles : %d\n\n", nmt);
#endif

  if (nmt == 0) 
    {
      *nmp = 0;
      *mat_pair = NULL;
      free((void*)tria);
      free((void*)trib);
      free((void*)mat_tri);
      return 0;
    }

  /*  Reducing The Number of False Matches (section II.d)  */
  if (!reject_false_match(&nmt, mat_tri)) 
    {
      *nmp = 0;
      *mat_pair = NULL;
      free((void*)tria);
      free((void*)trib);
      free((void*)mat_tri);
      return 0;
    }

#ifdef	WRITE
  printf("After rejection\n");
  printf("Number of matched triangles : %d\n\n", nmt);

  for(i=0;i<nmt;i++)
    {
      printf("(%f %f)-(%f %f)-(%f %f) == ",
	     pta[tria[mat_tri[i].id1].v1].x,pta[tria[mat_tri[i].id1].v1].y,
	     pta[tria[mat_tri[i].id1].v2].x,pta[tria[mat_tri[i].id1].v2].y,
	     pta[tria[mat_tri[i].id1].v3].x,pta[tria[mat_tri[i].id1].v3].y);
	     
      printf("(%f %f)-(%f %f)-(%f %f)\n",
	     ptb[trib[mat_tri[i].id2].v1].x,ptb[trib[mat_tri[i].id2].v1].y,
	     ptb[trib[mat_tri[i].id2].v2].x,ptb[trib[mat_tri[i].id2].v2].y,
	     ptb[trib[mat_tri[i].id2].v3].x,ptb[trib[mat_tri[i].id2].v3].y);
    }
#endif

  /*  Assigning Matched Points (section II.e)  */
  *nmp = matched_pair(mat_pair, nmt, mat_tri, tria, trib, npa, npb, pta, ptb);

  if ((*mat_pair)[0].count < 1 || *nmp < 3) 
    {
      *nmp = 0;
      free((void*)*mat_pair);
      *mat_pair = NULL;
      free((void*)tria);
      free((void*)trib);
      free((void*)mat_tri);
      return 0;
    }
  
  free((void*)tria);
  free((void*)trib);
  free((void*)mat_tri);
  return *nmp;
}



/************************************************************************/


void	displacement(MATCH_PAIR mat_pair[], int nmp,
		     double *x_mean, double *x_sig, 
		     double *y_mean, double *y_sig)
{
  int		i;

  float *datx,*daty;
  float mad,med;
  
  if (nmp == 0) {
    *x_mean = *y_mean = 0.0;
    *x_sig  = *y_sig  = 0.0;
    return;
  }
  
  datx=(float*)malloc(nmp*sizeof(float));
  daty=(float*)malloc(nmp*sizeof(float));
  
  for (i = 0; i < nmp; i++) 
    {
      datx[i]=(mat_pair[i].pt2.x - mat_pair[i].pt1.x);
      daty[i]=(mat_pair[i].pt2.y - mat_pair[i].pt1.y);
    }
  getTukey(nmp,datx,&med,&mad);
  *x_mean = (double)med ;
  *x_sig  = (double)mad/0.6745;
  
  getTukey(nmp,daty,&med,&mad);    
  *y_mean = (double)med ;
  *y_sig  = (double)mad/0.6745;
  
  free(datx);
  free(daty);

}

double	angle(POINT pt1, POINT pt2)
{
  double	dx, dy;

  dx = pt2.x - pt1.x;
  dy = pt2.y - pt1.y;

  return atan2(dy,dx);
}

void	rotation_angle(MATCH_PAIR mat_pair[], int nmp,
		       double *theta_mean, double *theta_sig)
{
  int		i, j;
  int		count = 0;
  double	d_theta;

  float *dat;
  float med,mad;

  if (nmp == 0) {
    *theta_mean = *theta_sig = 0.0;
    return;
  }
  
  dat=(float*)malloc(nmp*(nmp-1)/2*sizeof(float));

  /* Should be robust estimator HERE!! */

  for (i = 0; i < nmp-1; i++) {
    for (j = i+1; j < nmp; j++) {
      d_theta = angle(mat_pair[i].pt2, mat_pair[j].pt2)
	- angle(mat_pair[i].pt1, mat_pair[j].pt1);
      if (d_theta > M_PI)
	d_theta -= 2.0 * M_PI;
      else if (d_theta < -M_PI)
	d_theta += 2.0 * M_PI;
      dat[count]=(float)d_theta;
      count++;
    }
  }

  getTukey(count,dat,&med,&mad);
  
  *theta_mean = (double)med;
  *theta_sig  = (double)mad/0.6745;
  free(dat);
}

void  flux_ratio(MATCH_PAIR mat_pair[], int nmp,
		 double *dm, double *dm_sig, int *nmp_limits)
{
    int		i;

    double  d_m;

    float *dat,med,mad;

    *nmp_limits=0;
    *dm = *dm_sig = 0.0;

    if (nmp <= 1) return;
    dat=(float*)malloc(nmp*sizeof(float));
    for (i = 0; i < nmp; i++) 
      {
	/* now, no selection here */
	d_m = mat_pair[i].pt2.m - mat_pair[i].pt1.m;
	dat[i]=(float)d_m;
      }

    getTukey(nmp,dat,&med,&mad);

    *dm = med ;
    *dm_sig=mad/0.6745;
    *nmp_limits = nmp;

    free(dat);
}

void	scale(MATCH_PAIR mat_pair[], int nmp,
	      double *dlogsx, double *dlogsx_sig,
	      double *dlogsy, double *dlogsy_sig)
{
  int		i, j;
  int		count = 0;


  double dx1,dy1,dx2,dy2;
  float *datx,*daty,med,mad;

  datx=(float*)malloc(nmp*(nmp-1)/2*sizeof(float));
  daty=(float*)malloc(nmp*(nmp-1)/2*sizeof(float));

  for (i = 0; i < nmp-1; i++) {
    for (j = i+1; j < nmp; j++) {
      dx1=mat_pair[i].pt1.x - mat_pair[j].pt1.x;
      dy1=mat_pair[i].pt1.y - mat_pair[j].pt1.y;
      dx2=mat_pair[i].pt2.x - mat_pair[j].pt2.x;
      dy2=mat_pair[i].pt2.y - mat_pair[j].pt2.y;

      if ((dx2*dx1>0)&&(dy2*dy1>0))
	{
	  datx[count] = log10(dx2/dx1);
	  daty[count] = log10(dy2/dy1);
	  count++;	  
	}
    }
  }
  
  if(count>1)
    {
      getTukey(count,datx,&med,&mad);
      *dlogsx = med;
      *dlogsx_sig  = mad/0.6745;

      getTukey(count,daty,&med,&mad);
      *dlogsy = med;
      *dlogsy_sig  = mad/0.6745;
    }
  else
    {
      *dlogsx = 1.0;
      *dlogsy = 1.0;
      *dlogsx_sig  = -1.;
      *dlogsy_sig  = -1.;
    }

}

/******************************************************************/
