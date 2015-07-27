/* fopenv.c : this function attempts to open a file based on the
 *   environment variable if an fopen in the current directory fails.
 *   The environment string can have ";" as separators for multiple
 *   path searching.  Returns a NULL pointer if unsuccessful.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "xtdio.h"

/* -----------------------------------------------------------------------
 * Definitions:
 */
#ifdef unix
#define PATHSEP ':'
#else
#define PATHSEP ';'
#endif

/* -----------------------------------------------------------------------
 * Source Code:
 */

/* fopenv: opens file for read/write based on environment variable
 *	char *filename : name of file to be opened
 *	char *ctrl     : "r", "w", etc.
 *	char *env_var  : environment variable to be used
 */
#ifdef __PROTOTYPE__
FILE *fopenv(
  char *filename,
  char *ctrl,
  char *env_var)
#else	/* __PROTOTYPE__ */
FILE *fopenv(filename,ctrl,env_var)
char *filename,*ctrl,*env_var;
#endif	/* __PROTOTYPE__ */
{
char *s,*env,*buf;
int more;
FILE *fp;


/* see if file can be opened in current directory first */
fp= fopen(filename,ctrl);
if(fp) {
	return fp;
	}

/* check out the environment variable */
env= getenv(env_var);

/* attempt to open filename using environment search */
if(env) while(*env) { /* while there's environment-variable info left... */
	for(s= env; *s && *s != PATHSEP; ++s);
	more = *s == PATHSEP;
	*s   = '\0';
	buf  = (void *) calloc((size_t) strlen(env) + strlen(filename) + 2,sizeof(char));
#ifdef vms
	sprintf(buf,"%s%s",env,filename);
#endif
#ifdef unix
	sprintf(buf,"%s/%s",env,filename);
#endif
#ifdef LATTICE
	sprintf(buf,"%s\\%s",env,filename);
#endif
#ifdef DESMET
	sprintf(buf,"%s\\%s",env,filename);
#endif
#ifdef AZTEC_C
	sprintf(buf,"%s/%s",env,filename);
#endif
#ifdef MSDOS
	sprintf(buf,"%s\\%s",env,filename);
#endif

	/* attempt to open file given the path */
	fp= fopen(buf,ctrl);

	/* free up memory */
	free(buf);
	if(fp) { /* successfully opened file */
		return fp;
		}
	if(more) {	/* another path to search */
		*s= PATHSEP;
		env= s + 1;
		}
	else env= s;
	}

/* return NULL pointer, thereby indicating a modest lack of success */
return (FILE *) NULL;
}
