#ifndef	MATCH_H
#define	MATCH_H

typedef struct point
{
  int		id;		/* id number of point */
  double	x, y;		/* coordinates of point */
  double	m;		/* magnitude of point */
}	POINT;


typedef struct matched_pair
{
  POINT	        pt1, pt2;
  int		count;
}	MATCH_PAIR;

#define	min(x, y)	( (x) < (y) ? (x) : (y) )
#define	max(x, y)	( (x) > (y) ? (x) : (y) )
#define	sqr(x)		( (x) * (x) )

extern	int	match(POINT*, int, POINT*, int, double, MATCH_PAIR**, int*);
/* return number of matched points
   if matching was not successible return 0 */

extern	void	displacement(MATCH_PAIR*, int, double*, double*,
			     double*, double*);

extern	void	rotation_angle(MATCH_PAIR*, int, double*, double*);

extern	void	flux_ratio(MATCH_PAIR*, int, double*, double*, int*);

extern	void	scale(MATCH_PAIR*, int, double*, double*,
		      double*, double*);

extern	POINT   *read_point_list(char*, int*);

extern  POINT   *make_point_list(POINT*, int, int, int, int*);

extern	int	match2(POINT*, int, POINT*, int, 
		       double, double, double,
		       MATCH_PAIR**, int*);

extern void    sort_point_list(POINT*, int);
#endif
