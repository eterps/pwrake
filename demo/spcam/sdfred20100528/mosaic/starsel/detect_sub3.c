#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "imc.h"
#include "paint_sub.h"
#include "sort.h"

#include "detect_sub3.h"


#define NALLOC_MIN 131072

int setobjrec_pix(int x1, int y1, float v, 
		  void *result)
{
  pixlist_t *pl;
  int x,y;

  
  pl=(pixlist_t*)result;

  x=x1+pl->x0;
  y=y1+pl->y0;

  if(pl->xmin>x)pl->xmin=x;
  else if(pl->xmax<x)pl->xmax=x;
  if(pl->ymin>y)pl->ymin=y;
  else if(pl->ymax<y)pl->ymax=y;


  if (pl->npix>=pl->nalloc)
    {
      pl->nalloc+=NALLOC_MIN;
      pl->pixx=(int*)realloc(pl->pixx,pl->nalloc*sizeof(int));
      pl->pixy=(int*)realloc(pl->pixy,pl->nalloc*sizeof(int));
      pl->flux=(float*)realloc(pl->flux,pl->nalloc*sizeof(float));
      if(pl->pixx==NULL || pl->pixy==NULL || pl->flux==NULL)
	{
	  fprintf(stderr,"cannot allocate in setplrec %d\n",pl->nalloc);
	  exit(-1);
	}
    }
  (pl->pixx)[pl->npix]=x;
  (pl->pixy)[pl->npix]=y;
  (pl->flux)[pl->npix]=v;
  pl->npix++;

  /* moments should be counted here*/
  return (pl->npix);
}


int copy_pixlist(pixlist_t *pl,pixlist_t *src)
{
  memcpy(pl,src,sizeof(pixlist_t));
  pl->nalloc=pl->npix;
  
  pl->pixx=(int*)malloc(pl->nalloc*sizeof(int));
  pl->pixy=(int*)malloc(pl->nalloc*sizeof(int));
  pl->flux=(float*)malloc(pl->nalloc*sizeof(float));

  if(pl->pixx==NULL || pl->pixy==NULL || pl->flux==NULL)
    {
      fprintf(stderr,"cannot allocate in copy %d\n",
	      pl->nalloc);
      exit(-1);
    }

  memcpy(pl->pixx,src->pixx,pl->nalloc*sizeof(int));
  memcpy(pl->pixy,src->pixy,pl->nalloc*sizeof(int));
  memcpy(pl->flux,src->flux,pl->nalloc*sizeof(float));

  return 0;
}


int extract_pixlist(float *pix, int npx, int npy, 
		    int x, int y, /* absolute coordinate */
		    int id, 
		    float thres, 
		    int x0, int y0, /* shift from absolute to physical */
		    int *map, pixlist_t *pl)
{
  int npix=0;
  double thres0;

  pl->entnum=id;
  pl->nalloc=NALLOC_MIN;
  pl->x0=x0;
  pl->y0=y0;

  /* initialize */
  pl->npix=0;

  pl->pixx=(int*)malloc(pl->nalloc*sizeof(int));
  pl->pixy=(int*)malloc(pl->nalloc*sizeof(int));
  pl->flux=(float*)malloc(pl->nalloc*sizeof(float));
  if(pl->pixx==NULL || pl->pixy==NULL || pl->flux==NULL)
    {
      fprintf(stderr,"cannot allocate in extract %d\n",pl->nalloc);
      exit(-1);
    }

  pl->xmin=pl->xmax=x;
  pl->ymin=pl->ymax=y;
  
  thres0=(double)thres;
  npix=doflood3r(pix,npx,npy,x-x0,y-y0,
		 flood_greaterthan,
		 setobjrec_pix, pl,
		 map,id,1,&thres0);

  pl->nalloc=pl->npix;
  pl->pixx=(int*)realloc(pl->pixx,(pl->npix)*sizeof(int));
  pl->pixy=(int*)realloc(pl->pixy,(pl->npix)*sizeof(int));
  pl->flux=(float*)realloc(pl->flux,(pl->npix)*sizeof(float));

  return npix;
}

int clear_pixlist(pixlist_t *pl)
{
  if(pl!=NULL)
    {
      if(pl->pixx!=NULL) free(pl->pixx);
      if(pl->pixy!=NULL) free(pl->pixy);
      if(pl->flux!=NULL) free(pl->flux);
    }
  memset(pl,0,sizeof(pixlist_t));
  return 0; 
}

int free_pixlist(pixlist_t *pl) 
{
  if(pl!=NULL)
    {
      clear_pixlist(pl);
      free(pl);
      return 0;
    }
  else return 1;
}

int clear_objrec(objrec_t *ob)
{
  if(ob!=NULL)
    {
      if(ob->map!=NULL) free(ob->map);
      if(ob->img!=NULL) free(ob->img);
    }
  memset(ob,0,sizeof(objrec_t));
  return 0;
}

int free_objrec(objrec_t *ob) 
{
  if(ob!=NULL)
    {
      clear_objrec(ob);
      free(ob);
      return 0;
    }
  else return 1;
}


/**********/
int free_objrec_list(objrec_t *ob) 
{
  objrec_t *p,*q;
  
  for(p=ob;p!=NULL;p=q)
    {
      q=p->next;
      free_objrec(p);
    }
  return 0;
}


int makeimage_pixlist(float *g, int npx, int npy,
		      int xmin, int ymin, pixlist_t *pl)
{
  int i;
  int x0,y0;
  int npix=pl->npix;
  int *px,*py;
  float *pf;

  px=pl->pixx;
  py=pl->pixy;
  pf=pl->flux;

  for(i=0;i<npix;i++)
    {
      x0=px[i]-xmin;
      y0=py[i]-ymin;
      g[x0+npx*y0]=pf[i];
    }

  return 0;
}



int detect_new3(float *pix,
		int npx,
		int npy,
		float thres,
		int npix_min,
		int x0,
		int y0,
		int *map,
		pixlist_t ***out,
		int *nedged,
		pixlist_t ***edged)
{
  int i,j;

  int npix;
  int idx;
  int vxmin,vxmax,vymin,vymax;
  pixlist_t pl0={0};
  pixlist_t **pllist_in;
  pixlist_t **pllist_edge;
  int nobj=0,nobj1=0,nobj2=0;
  int nmax1=180,nmax2=180,nmaxstep=50;

  pllist_in=(pixlist_t**)malloc(nmax1*sizeof(pixlist_t*));
  pllist_edge=(pixlist_t**)malloc(nmax2*sizeof(pixlist_t*));

  vxmin=x0;
  vymin=y0;
  vxmax=x0+npx-1;
  vymax=y0+npy-1;

  for(j=0;j<npy;j++)
    for(i=0;i<npx;i++)
      {
	idx=i+j*npx;
	if(pix[idx]>thres)
	  {
	    if (map[idx]==0)
	      {
		npix=extract_pixlist(pix, npx, npy, 
				     i+x0, j+y0, 
				     nobj+1, thres,
 				     x0, y0, 
				     map, &pl0);
		nobj++;

		/*
		  if (pl0.xmin==vxmin||pl0.ymin==vymin||
		  pl0.xmax==vxmax||
		*/

		/* only y+ edged version */
		if (pl0.ymax==vymax)
		  {
		    pllist_edge[nobj2]=(pixlist_t*)malloc(sizeof(pixlist_t));
		    copy_pixlist(pllist_edge[nobj2],&pl0);
		    nobj2++;
		    if(nobj2>=nmax2)
		      {
			nmax2+=nmaxstep;
			pllist_edge=(pixlist_t**)
			  realloc(pllist_edge,nmax2*sizeof(pixlist_t*));
		      }
		  }
		else if (npix>=npix_min) /* keep */
		  {
		    pllist_in[nobj1]=(pixlist_t*)malloc(sizeof(pixlist_t));
		    copy_pixlist(pllist_in[nobj1],&pl0);

		    nobj1++;
		    if(nobj1>=nmax1)
		      {
			nmax1+=nmaxstep;
			pllist_in=(pixlist_t**)
			  realloc(pllist_in,nmax1*sizeof(pixlist_t*));
		      }
		  }
		clear_pixlist(&pl0);
	      }
	  }
      }

  /* shrink list */
  pllist_in=(pixlist_t**)realloc(pllist_in,nobj1*sizeof(pixlist_t*));
  pllist_edge=(pixlist_t**)realloc(pllist_edge,nobj2*sizeof(pixlist_t*));

  *out=pllist_in;
  *edged=pllist_edge;
  *nedged=nobj2;
  return nobj1;
}


int convert_pixlist(objrec_t *ob,pixlist_t *pl)
{
  int n;
  int id;
  int npx,npy,idx;
  int xmin,xmax,ymin,ymax;
  int *px,*py;
  float *f,v,peak;
  int ipeak=-1,jpeak=-1;
  int npix;
  double xc,yc,fiso;

  id=pl->entnum;
  ob->entnum=id;
  ob->npix=pl->npix;
  
  peak=pl->flux[0];
  xc=0;
  yc=0;
  fiso=0;

  xmin=ob->xmin=pl->xmin;
  xmax=ob->xmax=pl->xmax;
  ymin=ob->ymin=pl->ymin;
  ymax=ob->ymax=pl->ymax;

  px=pl->pixx;
  py=pl->pixy;
  f=pl->flux;
  npix=pl->npix;

  npx=xmax-xmin+1;
  npy=ymax-ymin+1;

  /*
    printf("%d %d %d %d %d %d\n",
    npx,npy, xmin,xmax,ymin,ymax);
  */

  ob->map=(int*)calloc(npx*npy,sizeof(int));
  /*
    printf("%d %d %d %d %d %d\n",
    npx,npy, xmin,xmax,ymin,ymax);
  */

  ob->img=NULL;

  for(n=0;n<npix;n++)
    {
      v=f[n];
      if(peak<v) 
	{
	  peak=v;
	  ipeak=px[n];
	  jpeak=py[n];
	}
      /* map */
      idx=(px[n]-xmin)+npx*(py[n]-ymin);
      ob->map[idx]=id;

      /* moment calculation */
      fiso+=v;
      /*
      printf("debug:fiso=%f\n",fiso);
      */
      /*
	printf("%d %f %f\n",n,xc,fiso);
      */
      xc+=(px[n]-xc)*v/(fiso);
      yc+=(py[n]-yc)*v/(fiso);
    }
  
  ob->peak=peak;
  ob->ipeak=ipeak;
  ob->jpeak=jpeak;
  ob->xc=(float)xc;
  ob->yc=(float)yc;
  ob->fiso=(float)fiso;

  return 0;
}

int detect_simple_array(float *pix,
			int	npx,
			int	npy,
			float	thres,
			int	npix_min,
			objrec_t ***ob)
{
  int *map,n,nobj1,nobj2,id;
  pixlist_t **pl_in,**pl_edge;
  objrec_t **oblist;

  map=(int*)calloc(npx*npy,sizeof(int));
  nobj1=detect_new3(pix,npx,npy,thres,npix_min,0,0,map,
		    &pl_in,&nobj2,&pl_edge);
  free(map);
  for(n=0;n<nobj2;n++)
    {
      if ((pl_edge[n]->npix)<=npix_min)
	{
	  free_pixlist(pl_edge[n]);
	  pl_edge[n]=pl_edge[nobj2-1];
	  nobj2--;
	  n--;
	}
    }

  if(nobj1+nobj2>0)
    {
      oblist=(objrec_t**)malloc((nobj1+nobj2)*sizeof(objrec_t*));
      
      id=1;
      for(n=0;n<nobj1;n++)
	{
	  oblist[n]=(objrec_t*)calloc(1,sizeof(objrec_t));
	  convert_pixlist(oblist[n],pl_in[n]);
	  free_pixlist(pl_in[n]);
	  oblist[n]->entnum=id++;
	  if (n!=0) oblist[n-1]->next=oblist[n];
	  oblist[n]->next=NULL;
	}
      free(pl_in);

      for(n=0;n<nobj2;n++)
	{
	  oblist[n+nobj1]=(objrec_t*)calloc(1,sizeof(objrec_t));
	  convert_pixlist(oblist[n+nobj1],pl_edge[n]);
	  free_pixlist(pl_edge[n]);
	  oblist[n+nobj1]->entnum=id++;
	  if (n+nobj1>0)
	    oblist[n+nobj1-1]->next=oblist[n+nobj1];
	  oblist[n+nobj1]->next=NULL;
	}
      free(pl_edge);
      *ob=oblist;
    }
  else
    {
      printf("NULL\n");
      *ob=NULL;
    }

  /*
    printf("%d %d\n",nobj1,nobj2);
  */
  return (nobj1+nobj2);
}

int detect_simple(float	*pix,
		  int	npx,
		  int	npy,
		  float	thres,
		  int	npix_min,
		  objrec_t **ob)
{
  objrec_t **oblist;
  int nobj;

  nobj=detect_simple_array(pix,npx,npy,thres,npix_min,&oblist);
  if(nobj>0)
    {
      *ob=oblist[0];
      free(oblist);
    }
  else 
    *ob=NULL;

  return nobj;
}

int merge_pixlist(pixlist_t *pl,
		  pixlist_t *src)
{
  int npix0;


  npix0=pl->npix;

  pl->npix+=src->npix;

  if (pl->nalloc<pl->npix)
    {
      pl->nalloc=pl->npix;
      pl->pixx=(int*)realloc(pl->pixx,pl->npix*sizeof(int));
      pl->pixy=(int*)realloc(pl->pixy,pl->npix*sizeof(int));
      pl->flux=(float*)realloc(pl->flux,pl->npix*sizeof(float));
    }

  memcpy((pl->pixx)+npix0,src->pixx,src->npix*sizeof(int));
  memcpy((pl->pixy)+npix0,src->pixy,src->npix*sizeof(int));
  memcpy((pl->flux)+npix0,src->flux,src->npix*sizeof(float));

  if (pl->xmin>src->xmin) pl->xmin=src->xmin;
  if (pl->xmax<src->xmax) pl->xmax=src->xmax;
  if (pl->ymin>src->ymin) pl->ymin=src->ymin;
  if (pl->ymax<src->ymax) pl->ymax=src->ymax;
  
  return 0;
}


int detect_full_new3(float *pix,
		     int	npx,
		     int	npy,
		     float thres,
		     int	npix_min,
		     
		     int *nobj_old,
		     pixlist_t ***pl_edge0,
		     int x0,
		     int y0,
		     pixlist_t ***ppl)
{
  int *map,n,nobj,nobj0,nobj1,nobj2;
  pixlist_t **pl_edge,**pl_edge2,
    **pl_in,**pl_out,*pl;

  int npix,npix0;
  int x,y;
  int id;
  int i,idx;
  int xmin,xmax,ymin,ymax;
  int cflag;
  double thres0;

 
  nobj0=*nobj_old;
  /* 1) extend old */
  /* change id */
  pl_edge=*pl_edge0;

  map=(int*)calloc(npx*npy,sizeof(int));

  for(n=0;n<nobj0;n++)
    {
      /*printf("n=%d \n",n);*/

      pl=pl_edge[n];
      id=n+1;
      pl->entnum=id;
      /* slow, but check all (1st impl.)*/
      npix=pl->npix;
      npix0=npix;

      xmin=pl->xmin;
      xmax=pl->xmax;
      ymin=pl->ymin;
      ymax=pl->ymax;
      
      /* check only adjuscent */

      cflag=0;
      
      /* y+ only version*/
      y=0;

      for(i=0;i<npix;i++)
	{
	  if (pl->pixy[i]==ymax)
	    {
	      x=pl->pixx[i]-x0;
	      idx=x+npx*y;

	      if (pix[idx]>thres)
		{
		  cflag=1;
		  if(map[idx]==id) continue;
		  else if(map[idx]==0)
		    {
		      /* fill */
		      pl->x0=x0;
		      pl->y0=y0;
		      thres0=(double)thres;
		      doflood3r(pix,npx,npy,x,y,
			       flood_greaterthan,
			       setobjrec_pix, pl,
			       map,id,1,&thres0);
		    }
		  else 
		    {
		      /* merge list */
		      /* pl ==>> pl_edge[map[idx]-1] */
		      if (pl_edge[map[idx]-1]!=NULL)
			{
			  /* not absorped by myself */
			  merge_pixlist(pl_edge[n],pl_edge[map[idx]-1]);
			  
			  /* replace */
			  /*
			    printf("debug:%d<-%d\n",id,map[idx]);
			  */
			  free_pixlist(pl_edge[map[idx]-1]);
			  pl_edge[map[idx]-1]=NULL;
			  cflag=2;
			}
		    }
		}
	    }
	}
      npix=pl->npix;

      for(i=0;i<npix;i++)
	{
	  x=(pl->pixx[i])-x0;
	  y=(pl->pixy[i])-y0;
	  if (x>=0&&y>=0&&x<npx&&y<npy)
	    {
	      idx=x+npx*y;
	      map[idx]=id;
	    }
	}
    }

  pl_out=(pixlist_t**)calloc(nobj0,sizeof(pixlist_t*));
  nobj=0;

  /* clear pixlist_edged, and rebuild array */
  for(n=0;n<nobj0;n++)
    {
      if(pl_edge[n]==NULL)
	{
	  /* swap and shrink */
	  pl_edge[n]=pl_edge[nobj0-1];
	  nobj0--;
	  n--;
	  continue;
	}
      if ((pl_edge[n]->ymax)!=y0+npy-1)
	{
	  /* isolated */
	  if (pl_edge[n]->npix>npix_min)
	    {
	      /* keep */
	      pl_out[nobj]=pl_edge[n];
	      pl_out[nobj]->entnum=nobj+1;
	      nobj++;
	    }
	  else
	    {
	      free_pixlist(pl_edge[n]);
	    }
	  /* swap and shrink */
	  pl_edge[n]=pl_edge[nobj0-1];
	  nobj0--;
	  n--;
	}
      
    }

  nobj1=detect_new3(pix,npx,npy,thres,npix_min,x0,y0,map,&pl_in,
		    &nobj2,&pl_edge2);
  
  free(map);

  /*
    printf("C:%d %d %d %d\n",nobj,nobj0,nobj1,nobj2);
  */

  /* merge edge */
  /* no renumber */

  *nobj_old=nobj0+nobj2;
  pl_edge=(pixlist_t**)realloc(pl_edge,(nobj0+nobj2)*sizeof(pixlist_t*));
  memcpy(pl_edge+nobj0,pl_edge2,nobj2*sizeof(pixlist_t*));
  free(pl_edge2);
  *pl_edge0=pl_edge;

  pl_out=(pixlist_t**)realloc(pl_out,(nobj+nobj1)*sizeof(pixlist_t*));
  memcpy(pl_out+nobj,pl_in,nobj1*sizeof(pixlist_t*));
  free(pl_in);  
  nobj+=nobj1;
  *ppl=pl_out;
  
  return nobj;  
}
