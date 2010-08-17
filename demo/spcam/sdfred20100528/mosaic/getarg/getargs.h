#ifndef GETARGS_H
#define GETARGS_H

extern char *getarg(char argv[],char key[]);
extern char *getarg_exact(char argv[],char key[]);

enum opttyps {OPTTYP_CHAR,
	      OPTTYP_INT,
	      OPTTYP_FLOAT,
	      OPTTYP_DOUBLE,
	      OPTTYP_STRING,
              OPTTYP_FLAG,
	      OPTTYP_INTARRAY,
	      OPTTYP_FLTARRAY,
	      OPTTYP_DBLARRAY
};

typedef struct opts
{
  char name[256];
  enum opttyps type;
  char comment[1024];
  void *retval;
  int flag;
  int hidden;
  int *nread;
} getargopt ;

extern int parsearg(int argc, char **argv, getargopt opts[], 
		    char **filename, char ***endptr);

extern int setopts(getargopt *opt,char *name, enum opttyps type, void*pt,
		   char *comment, ...);
extern int print_help(char *commandline, getargopt opts[], char *otheropts);

#endif


