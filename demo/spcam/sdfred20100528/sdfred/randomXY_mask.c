#include <stdio.h>
#include <stdlib.h>
#include <math.h>
/* 2008/01/03 YAGI, time() requires time.h */
#include <time.h>

/* 2008/01/03 revised by YAGI */
#define NUMBER_RANDOM 1000000
#define NUMBER_MASK 60000

char mask_shape[NUMBER_MASK];
double mask[NUMBER_MASK][4];
double mask_box[NUMBER_MASK][4];
double mask_circle[NUMBER_MASK][3];
double r[NUMBER_RANDOM][2];

int main(int argc,char **argv)
{
  FILE *fp1;
  long i,j,k;
  char line[100];
  /*
  char mask_shape[300];
  double mask[300][4];
  double mask_box[300][4];
  double mask_circle[300][3];
  double r[60000][2];
  */
  double ic_accum_rr_thetadelta;
  double x1,y1,x2,y2;
  double dx, dy;
  double dist_x,dist_y;
  double dist;
  int number_of_points;
  long rec_mask;
  long rec_mask_box, rec_mask_circle;

  /* read arguments */

  if(argc!=7) {
    fprintf(stderr,"Usage: randomXY_mask [mask file] [x1] [x2] [y1] [y2] [number of random points]\n");
    exit(1);
  }

  if( (fp1=fopen(argv[1],"r"))==NULL) {
    /* 2008/01/03 YAGI ERORR -> ERROR */
    fprintf(stderr,"ERROR: file %s not found \n", argv[1]);
    exit(1);
  }

  /* read inputfiles */

  rec_mask=0;
  while( fgets( line, sizeof(line), fp1 ) != NULL ) { 
      sscanf( line, "%c %lf %lf %lf %lf", &mask_shape[rec_mask],
	      &mask[rec_mask][0],&mask[rec_mask][1],
	      &mask[rec_mask][2],&mask[rec_mask][3] );
   rec_mask++;
   /* 2008/01/03 YAGI */
   /* in fact, this can be much smarter with malloc&realloc */
   if (rec_mask>=NUMBER_MASK)
     {
       fprintf(stderr, "ERROR: Too many lines in %s (>%d)\n",argv[1],
	       NUMBER_MASK);
       exit(-1);
     }
  }


  /* store input parameters */
  x1=atof(argv[2]);
  x2=atof(argv[3]);
  y1=atof(argv[4]);
  y2=atof(argv[5]);
  number_of_points = atof(argv[6]);

  /* 2008/01/03 YAGI */
  if (number_of_points>=NUMBER_RANDOM)
    {
      fprintf(stderr, "ERROR: number of points exceeds limit %d\n",
	      NUMBER_RANDOM);
      exit(-1);
    }

  rec_mask_box=0;
  rec_mask_circle=0;
  /* make masks */
  for(i=0;i<rec_mask;i++) {
    switch ( mask_shape[i] ) {

       case 'b' : mask_box[rec_mask_box][0]=mask[i][0];
	          mask_box[rec_mask_box][1]=mask[i][1];
		  mask_box[rec_mask_box][2]=mask[i][2];
		  mask_box[rec_mask_box][3]=mask[i][3];
		  rec_mask_box++;
		  break;

       case 'c' : mask_circle[rec_mask_circle][0]=mask[i][0];
	          mask_circle[rec_mask_circle][1]=mask[i][1];
		  mask_circle[rec_mask_circle][2]=mask[i][2];
		  rec_mask_circle++;
		  break;

        default : 
	  fprintf(stderr,
		  "ERROR: %s should includes b or c at the first column \n",argv[2]);
	  fprintf(stderr,"insted, %c is found\n",mask_shape[i]);
	  exit(1);
    }
  }

  /*
  printf("#rectangular mask : %d\n",rec_mask_box);
  printf("# xmin     xmax     ymin     ymax\n");


  for(i=0;i<rec_mask_box;i++) {
    printf("# %.3lf %.3lf %.3lf %.3lf\n",mask_box[i][0],mask_box[i][1],
	   mask_box[i][2],mask_box[i][3]);
  }

  printf("#circular mask : %d\n",rec_mask_circle);
  printf("# xcen     ycen     radius \n");
  
  for(i=0;i<rec_mask_circle;i++) {
    printf("# %.3lf %.3lf %.3lf\n",mask_circle[i][0],mask_circle[i][1],
	   mask_circle[i][2]);
  }

  printf("#\n");
  */

  /* make a seed for drand48 */
  srand48( (long) time(NULL) );

  /* generate the random points */
  dx=x2-x1;
  dy=y2-y1;

  /* 2008/01/03 YAGI */
  if (dx==0 || dy==0)
    {
      fprintf(stderr, "ERROR: region size of (%f,%f)-(%f,%f) is 0",
	      x1,y1,x2,y2);
      exit(-1);
    }

  i=0;
  while(i<number_of_points) {
    again :
      r[i][0]=x1+dx*drand48();
      r[i][1]=y1+dy*drand48();
    
      for(j=0;j<rec_mask_box;j++) {
	if(r[i][0]>=mask_box[j][0] && r[i][0]<=mask_box[j][1] 
	   && r[i][1]>=mask_box[j][2] && r[i][1]<=mask_box[j][3]) {
	  goto again ;
	}
      }

      for(j=0;j<rec_mask_circle;j++) {
	dist_x=r[i][0]-mask_circle[j][0];
	dist_y=r[i][1]-mask_circle[j][1];
	if(dist_x <= mask_circle[j][2] && dist_x >= -mask_circle[j][2]   
	     && dist_y <= mask_circle[j][2] && dist_y >= -mask_circle[j][2]) {
	  dist=sqrt( pow(dist_x,2) + pow(dist_y,2) );
	  if(dist<=mask_circle[j][2]) {
	    goto again ;
	  }
	}
      }
      
      i++;
  }

  for(i=0;i<number_of_points;i++) {
    printf("%.2lf %.2lf\n",r[i][0],r[i][1]);
  }

  fclose(fp1);
  return 0;

}


