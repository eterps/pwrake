/* See C/C++ Users Journal vol.12 No.8
  && C magazine 95/1 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct shdw 
{
  int lft,rgt;
  int row,par;
  int OK;
  struct shdw *next;
} shadow;

static int currRow;
static shadow *seedShadow;
static shadow *rowHead;
static shadow *pendHead;
static shadow *freeHead;

void freeShadows(shadow *s)
{
  shadow *t;
  while(s!=NULL)
    {
      t=s->next;
      free(s);
      s=t;
    }
}

void newShadow(int slft,int srgt,int srow,int prow)
{
  shadow *new;

  if((new=freeHead)!=NULL)
    {
      freeHead=freeHead->next;
    }
  else if((new=(shadow*)malloc(sizeof(shadow)))==NULL)
    {
      exit(-1);
    }  

  new->lft=slft;
  new->rgt=srgt;
  new->row=srow;
  new->par=prow;
  new->OK=1;
  new->next=pendHead;
  pendHead=new;
}

void makerow(void)
{
  shadow *s,*t,*u;
  t=pendHead;
  pendHead=NULL;

  /*!! 1999/10/29 */
  while((s=t)!=NULL)
    {
      t=t->next;
      if(s->OK!=0)
	{
	  if(rowHead==NULL)
	    {
	      currRow=s->row;
	      s->next=NULL;
	      rowHead=s;
	    }
	  else if(s->row==currRow)
	    {
	      if(s->lft<=rowHead->lft)
		{
		  s->next=rowHead;
		  rowHead=s;
		}
	      else
		{
		  for(u=rowHead;u->next!=NULL;u=u->next)
		    {
		      /*!! 1999/10/29 */
		      if(s->lft<=u->next->lft) break;
		    }
		  s->next=u->next;
		  u->next=s;
		}
	    }
	  else
	    {
	      s->next=pendHead;
	      pendHead=s;
	    }
	}
      else
	{
	  s->next=freeHead;
	  freeHead=s;
	}
    }
}


void clipShadow(int lft,int rgt,int row,shadow *line)
{
  if(lft<(line->lft-1))
    newShadow(lft,line->lft-2,row,line->row);
  if(rgt>(line->rgt+1))
    newShadow(line->rgt+2,rgt,row,line->row);
}


void removeOverlap(shadow *rw)
{
  shadow *chld;
  
  for(chld=pendHead;(chld->row!=rw->par);chld=chld->next);
  /*  if(chld==NULL) return;*/

  clipShadow(chld->lft,chld->rgt,chld->row,rw);
  if(rw->rgt>chld->rgt+1)
    rw->lft=chld->rgt+2;
  else
    rw->OK=0;

  chld->OK=0;
}

void makeShadows(int lft,int rgt)
{
  shadow *p;

  if(currRow > seedShadow->par)
    {
      newShadow(lft,rgt,currRow+1,currRow);
      clipShadow(lft,rgt,currRow-1,seedShadow);
    }
  else if(currRow < seedShadow->par)
    {
      newShadow(lft,rgt,currRow-1,currRow);
      clipShadow(lft,rgt,currRow+1,seedShadow);

    }
  else
    {
      newShadow(lft,rgt,currRow+1,currRow);
      newShadow(lft,rgt,currRow-1,currRow);
    }

  for(p=rowHead;(p!=NULL)&&(p->lft<=rgt);p=p->next)
   {
     if(p->OK!=0) 
       {
	 removeOverlap(p);
       }
   }
}

int visitShadow3r(float *tmp,int npx,int npy,int x,int y,
		  int (*checkfunc)(float, int, void*), 
		  int (*setfunc)(int,int,float,void*),
		  void *result, int *map,int mapval,int npar, void *par)
{
  int col,lft;
  int npix=0;

  for(col=seedShadow->lft;col<=seedShadow->rgt;col++)
    {
      if(col<npx && col>=0 && currRow>=0 && currRow<npy && 
	 checkfunc(tmp[col+npx*currRow],npar,par))
	{
	  if(map[col+npx*currRow]==0) 
	    {
	      npix++;
	      map[col+npx*currRow]=mapval;
	      setfunc(col,currRow,tmp[col+npx*currRow],result);
	    }

	  if((lft=col)==seedShadow->lft)
	    {
	      lft--;
	      while(lft<npx && lft>=0 && currRow>=0 && currRow<npy &&
		    checkfunc(tmp[lft+npx*currRow],npar,par))
		{
		  if(map[lft+npx*currRow]==0)
		    {
		      npix++;
		      map[lft+npx*currRow]=mapval;
		      setfunc(lft,currRow,tmp[lft+npx*currRow],result);
		    }
		  lft--;
		}
	      lft++;
	    }
	  col++;
	  while(col<npx && col>=0 && currRow>=0 && currRow<npy &&
		checkfunc(tmp[col+npx*currRow],npar,par))
	    {
	      if(map[col+npx*currRow]==0)
		{
		  npix++;
		  map[col+npx*currRow]=mapval;
		  setfunc(col,currRow,tmp[col+npx*currRow],result);
		}
	      col++;
	    }
	  makeShadows(lft,col-1);
	}	  
    }
  return npix;
}


int visitShadow3a(float *tmp,int npx,int npy,int x,int y,
		  float thres,
		  int (*setfunc)(int,int,float,void*),
		  void *result, int *map,int mapval)
{
  int col,lft;
  int npix=0;
  int idx;

  for(col=seedShadow->lft;col<=seedShadow->rgt;col++)
    {
      if(col<npx && col>=0 && currRow>=0 && currRow<npy && 
	 tmp[(idx=col+npx*currRow)]>thres)
	{
	  if(map[idx]==0) 
	    {
	      npix++;
	      map[idx]=mapval;
	      setfunc(col,currRow,tmp[idx],result);
	    }

	  if((lft=col)==seedShadow->lft)
	    {
	      lft--;
	      while(lft<npx && lft>=0 && currRow>=0 && currRow<npy &&
		    tmp[(idx=lft+npx*currRow)]>thres)
		{
		  if(map[idx]==0)
		    {
		      npix++;
		      map[idx]=mapval;
		      setfunc(lft,currRow,tmp[idx],result);
		    }
		  lft--;
		}
	      lft++;
	    }
	  col++;
	  while(col<npx && col>=0 && currRow>=0 && currRow<npy &&
		tmp[(idx=col+npx*currRow)]>thres)
	    {
	      if(map[idx]==0)
		{
		  npix++;
		  map[idx]=mapval;
		  setfunc(col,currRow,tmp[idx],result);
		}
	      col++;
	    }
	  makeShadows(lft,col-1);
	}	  
    }
  return npix;
}

int doflood3r(float *tmp,int npx,int npy,int x,int y,
	      int (*checkfunc)(float,int,void*), 
	      int (*setfunc)(int,int,float,void*),
	      void *result, int *map, int mapval, int npar, void *par)
{
  int n=0;

  pendHead=rowHead=freeHead=NULL;

  newShadow(x,x,y,y);

  while(pendHead!=NULL)
    {
      makerow();
      while(rowHead!=NULL)
	{
	  seedShadow=rowHead;
	  rowHead=rowHead->next;
	  if(seedShadow->OK!=0)
	    {
	      n+=visitShadow3r
		(tmp,npx,npy,x,y,
		 checkfunc, 
		 setfunc, result,
		 map,mapval,npar,par);
	    }
	  /* Yagi added */
	  {
	    seedShadow->next=freeHead;
	    freeHead=seedShadow;
	  }
	}
    }
  freeShadows(freeHead); 
  return n;
}


int doflood3a(float *tmp,int npx,int npy,int x,int y,
	      float thres, /* greater than only */
	      int (*setfunc)(int,int,float,void*),
	      void *result, int *map, int mapval)
{
  int n=0;

  pendHead=rowHead=freeHead=NULL;

  newShadow(x,x,y,y);

  while(pendHead!=NULL)
    {
      makerow();
      while(rowHead!=NULL)
	{
	  seedShadow=rowHead;
	  rowHead=rowHead->next;
	  if(seedShadow->OK!=0)
	    {
	      n+=visitShadow3a
		(tmp,npx,npy,x,y,
		 thres,
		 setfunc, result,
		 map,mapval);
	    }
	  /* Yagi added */
	  {
	    seedShadow->next=freeHead;
	    freeHead=seedShadow;
	  }
	}
    }
  freeShadows(freeHead); 
  return n;
}

int donothing(int x, int y, float val, void *result)
{
  return 0;
}

int flood_lessthan(float a, int npar, void *par)
{
  double thres;
  thres=((double*)par)[0];

  return (a<thres);
}

int flood_greaterthan(float a, int npar, void *par)
{
  double thres;

  thres=((double*)par)[0];

  return (a>thres);
}

int flood_greaterequal(float a, int npar, void *par)
{
  double thres;

  thres=((double*)par)[0];

  return (a>=thres);
}

int flood_between(float a, int npar, void *par)
{
  double thres0,thres1;

  thres0=((double*)par)[0];
  thres1=((double*)par)[1];

  return (a>thres0)&&(a<thres1);
}

int doflood(float *tmp,int npx,int npy,int x,int y,float thres,int *map)
{
  double thres0;
  thres0=(double)thres;

  return doflood3r(tmp,npx,npy,x,y,
		   flood_greaterthan,donothing,NULL,
		   map,1,1,&thres0);

}

int visitShadow2(float *tmp,int npx,int npy,int x,int y,
		 int (*func)(float,int,void*), int *map, int npar, void *par)
{
  int col,lft;
  int npix=0;

  for(col=seedShadow->lft;col<=seedShadow->rgt;col++)
    {
      if(col<npx && col>=0 && currRow>=0 && currRow<npy && 
	 func(tmp[col+npx*currRow],npar,par))
	{
	  if(map[col+npx*currRow]==0) npix++;
	  map[col+npx*currRow]=1;

	  if((lft=col)==seedShadow->lft)
	    {
	      lft--;
	      while(lft<npx && lft>=0 && currRow>=0 && currRow<npy &&
		    func(tmp[lft+npx*currRow],npar,par))
		{
		  if(map[lft+npx*currRow]==0)npix++;
		  map[lft+npx*currRow]=1;
		  lft--;
		}
	      lft++;
	    }
	  col++;
	  while(col<npx && col>=0 && currRow>=0 && currRow<npy &&
		func(tmp[col+npx*currRow],npar,par))
	    {
	      if(map[col+npx*currRow]==0)npix++;
	      map[col+npx*currRow]=1;
	      col++;
	    }
	  makeShadows(lft,col-1);
	}	  
    }
  return npix;
}


int doflood2(float *tmp,int npx,int npy,int x,int y,
	     int (*func)(float,int,void*), int *map,int npar, void*par)
{
  int n=0;

  pendHead=rowHead=freeHead=NULL;

  newShadow(x,x,y,y);
  while(pendHead!=NULL)
    {
      makerow();
      while(rowHead!=NULL)
	{
	  seedShadow=rowHead;
	  rowHead=rowHead->next;
	  if(seedShadow->OK!=0)
	    {
	      n+=visitShadow2(tmp,npx,npy,x,y,func,map,npar,par);
	    }/* Yagi added */{
	    seedShadow->next=freeHead;
	    freeHead=seedShadow;
	  }
	}
    }
  freeShadows(freeHead); 
  return n;
}



int clean(float *g, int npx, int npy, float xcen, float ycen, float thres)
{
  int *map,i,npix;
  /* clean up */
  map=(int*)calloc(npx*npy,sizeof(int));
  if(map==NULL)
    {
      fprintf(stderr,"Cannot allocate map in clean(paint_sub.c)\n");
      exit(-1);
    }
  npix=doflood(g,npx,npy,xcen,ycen,thres,map);
  for(i=0;i<npx*npy;i++)
    g[i]*=(float)map[i];
  free(map);
  return npix;
}
