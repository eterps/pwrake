#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
  char key[10]="";
  FILE *fp;
  char buffer[81]="";
  char val[70]="";
  int i,j,flag=0;

  if (argc!=3)
    {
      printf("\nUsage: getkey KEYWORD filename\n");
      printf("         eg) getkey NAXIS1 test.fits\n\n");
      exit(-1);
    }
  sprintf(key,"%-8.8s=",argv[1]);
  
  /* printf("debug:[%s]\n",key); */

  fp=fopen(argv[2],"r");
  if (NULL==fp)
    {
      printf("Cannot find file \"%s\"\n",argv[2]);
    }

  while(!feof(fp))
    {
      if (80!=fread(buffer, 1, 80, fp))
	{
	  /* error */
	  exit(-1);
	}

      if (strncmp("END                                                                     ",buffer,80)==0)
	exit(0);
      else if (strncmp(key,buffer,9)==0)
	{
	  /* printf("debug:\n%s\n",buffer); */
	  j=0;
	  for(i=10;i<79;i++)
	    {
	      /* printf("debug:%d %c\n",i,buffer[i]); */
	      if('\''==buffer[i])
		{
		  if(flag==0)
		    flag=1;
		  else if('\''==buffer[i+1])
		    {
		      val[j]='\'';
		      j++;
		      i++;
		    }
		  else
		    {
		      val[j]='\0';
		      /* check trailing ' ' */
		      j--;
		      while (j>0)
			{
			  if(val[j]!=' ') break;
			  val[j]='\0';
			  j--;
			}

		      break;
		    }
		}
	      else if(flag==1)
		{
		  val[j]=buffer[i];
		  j++;
		}
	      else if(flag==2)
		{
		  if (buffer[i]!=' ')
		    {
		      val[j]=buffer[i];
		      j++;
		    }
		  else
		    {
		      val[j]='\0';
		      j++;
		      break;
		    }
		}
	      else 
		{
		  if (buffer[i]!=' ')
		    {
		      flag=2;
		      val[j]=buffer[i];
		      j++;
		    }
		}
	    }   

	  printf("%s",val);
	  return 0;
	}
    }
  
  return 0;
}
