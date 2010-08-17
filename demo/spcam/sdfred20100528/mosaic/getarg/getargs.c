#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "getargs.h"

char *getarg(char argv[],char key[])
{
  int l;
  l=strlen(key);
  if(strncmp(argv,key,l)==0) 
    return argv+l;
  else return NULL;
}

char *getarg_exact(char argv[],char key[])
{
  int l,m;
  l=strlen(key);
  m=strlen(argv);
  if ((l==m)&&(strncmp(argv,key,l)==0))
    return argv+l;
  else return NULL;
}

int parsearg(int argc, char **argv, getargopt opts[], 
	     char **filename, char ***endptr)
{
  char *pt;
  int ia,i,j;
  int helpflag=0;
  int flag=0;
  int n=0,nf=0;
  char *argp;
  char buf[BUFSIZ];
  int imax;
  int nread;

  for(ia=1; ia<argc; ia++) 
    {      
      argp=argv[ia];
      if( argp[0]=='-' && isalpha(argp[1]))
	{
	  if (argp[1]=='-') argp++;
	  n=0;
	  flag=0;
	  if((pt=getarg(argp,"-version"))!=NULL||
	     (pt=getarg_exact(argp,"-V"))!=NULL)
	    {
	      printf("Version: %s\n",VERSION_STRING);
	      exit(1);
	    }
	  while(opts[n].name[0] != '\0')
	    {
	      if((pt=getarg(argp,opts[n].name))!=NULL) 
		{
		  switch (opts[n].type)
		    {
		    case OPTTYP_CHAR:
		      *((char*)(opts[n].retval))=pt[0];
		      break;
		    case OPTTYP_INT:
		      *((int*)(opts[n].retval))=atoi(pt);
		      break;
		    case OPTTYP_FLOAT:
		      *((float*)(opts[n].retval))=(float)atof(pt);
		      break;
		    case OPTTYP_DOUBLE:
		      *((double*)(opts[n].retval))=atof(pt);
		      break;
		    case OPTTYP_STRING:
		      sscanf(pt,"%s",(char*)(opts[n].retval));
		      break;
		    case OPTTYP_FLAG:
		      *((int*)(opts[n].retval))=opts[n].flag;
		      break;
		      /****************/
		    case OPTTYP_INTARRAY:
		      imax=opts[n].flag;
		      nread=0;
		      for(i=0;i<imax;i++)
			{
			  if (sscanf(pt,"%[-0-9]%*[^-0-9]%n",buf,&j)==1)
			    {
			      /*
				printf("pt:%s j:%d\n",pt,j);
			      */
			      *((int*)(opts[n].retval)+i)=atoi(buf);
			      nread++;
			      if(j>0) 
				{
				  pt+=j;
				  j=0;
				}
			      else 
				break;
			    }
			  else
			    {
			      break;
			    }
			}
		      *(opts[n].nread)=nread;
		      if (nread==0) 
			helpflag=1;
		      break;
		    case OPTTYP_FLTARRAY:
		    case OPTTYP_DBLARRAY:
		      imax=opts[n].flag;
		      nread=0;
		      for(i=0;i<imax;i++)
			{
			  if (sscanf(pt,"%[-+.0-9eE]%*[^-+.0-9eE]%n",
				     buf,&j)==1)
			    {
			      if(opts[n].type==OPTTYP_FLTARRAY)
				*((float*)(opts[n].retval)+i)=(float)atof(buf);
			      else /* if(opts[n].type==OPTTYP_DBLARRAY) */
				*((double*)(opts[n].retval)+i)=atof(buf);
			      nread++;
			      if(j>0)
				{
				  pt+=j;
				  j=0;
				}
			      else 
				break;
			    }
			  else
			    {
			      break;
			    }
			}
		      *(opts[n].nread)=nread;
		      if (nread==0) 
			helpflag=1;
		      break;
		    default:
		      helpflag=1;
		    }
		  flag=1;
		  /*
		    if(endptr!=NULL) *endptr=(argv+ia+1);
		   */
		  break;
		}
	      n++;
	    }
	  if(flag==0)
	    {
	      printf("Invalid option %s\n",argv[ia]);	      
	      helpflag=1;
	    }
	}
      else
	{
	  if(filename[nf]!=NULL)
	    {
	      sscanf(argp,"%s",filename[nf]);
	      nf++;
	      if(endptr!=NULL) *endptr=(argv+ia+1);
	    }
	}  
    }
  
  return helpflag;
}

int print_help(char *commandline, getargopt opts[], char *otheropts)
{
  int n=0;
  /* defined in getargs.h */
  /*
    enum opttyps {OPTTYP_CHAR,
    OPTTYP_INT,
    OPTTYP_FLOAT,
    OPTTYP_DOUBLE,
    OPTTYP_STRING,
    OPTTYP_FLAG,
    OPTTYP_INTARRAY
    OPTTYP_FLTARRAY
    OPTTYP_DBLARRAY
    };
  */
  char typename[][12]={"%c","%d","%f","%lf","%s","",
		       "%d[,%d]..","%f[,%f]..",
		       "%lf[,%lf].."};

  fprintf(stderr,"%s\n",commandline);
  while(opts[n].name[0] != '\0')
    {
      fprintf(stderr,"       %s%s : %s\n",
	      opts[n].name,
	      typename[opts[n].type],
	      opts[n].comment);
      n++;
    }
  fprintf(stderr,"%s\n",otheropts);
  return 0;
}

int setopts(getargopt *opt,char *name, enum opttyps type, 
	    void *pt, char *comment, ...)
{
  va_list va;

  if(name!=NULL)
    strncpy(opt->name,name,255);
  if(comment!=NULL)
    strncpy(opt->comment,comment,1023);
  else
    opt->comment[0]='\0';

  opt->type=type;
  opt->retval=pt;
  if (type==OPTTYP_FLAG || 
      type==OPTTYP_INTARRAY ||
      type==OPTTYP_FLTARRAY ||
      type==OPTTYP_DBLARRAY )
    {
      va_start(va,comment);
      opt->flag=va_arg(va,int);
      if (type==OPTTYP_INTARRAY ||
	  type==OPTTYP_FLTARRAY ||
	  type==OPTTYP_DBLARRAY )
	{
	  opt->nread=va_arg(va,int*);
	}
      va_end(va);
    }

  return 0;
}
