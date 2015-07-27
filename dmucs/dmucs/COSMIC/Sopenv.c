/* Sopenv.c : this function attempts to open a Socket based on an
 *   environment variable containing machine names separated by colons.
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Sopenv.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "xtdio.h"
#include "sockets.h"

/* -----------------------------------------------------------------------
 * Definitions:
 */
#define MACHINE_SEP	':'
#define BUFSIZE		256

/* -----------------------------------------------------------------------
 * Source Code:
 */

/* Sopenv: opens Socket for server/client based on environment variable
 */
#ifdef __PROTOTYPE__
Socket *Sopenv(
  char *sktname,		/* name of server					*/
  char *ctrl,			/* "s", "c"							*/
  char *env_var)		/* environment variable to be used	*/
#else
Socket *Sopenv(sktname,ctrl,env_var)
char *sktname; /* name of server                  */
char *ctrl;    /* "s", "c"                        */
char *env_var; /* environment variable to be used */
#endif
{
char        *at           = NULL;
char        *env          = NULL;
char        *s            = NULL;
int          more         = 0;
Socket      *skt          = NULL;
static char  buf[BUFSIZE];


/* see if Socket can be opened on current machine, first */
skt= Sopen(sktname,ctrl);
if(skt) {
	return skt;
	}

/* cannot open server on a different host
 *  ie. using Sopenv to open a server doesn't give any
 *      value added because it doesn't make any sense to
 *      "search" a path of machines to do it.
 */
else if(!strcmp(ctrl,"s") || !strcmp(ctrl,"S")) {
	return (Socket *) NULL;
	}

/* look for '@' in string and null it */
at= (char *) strchr(sktname,'@');
if(at) *at= '\0';

/* check out the environment variable */
if(!env_var || !*env_var) env_var= "SKTPATH";
env= getenv(env_var);

/* attempt to open connection to Socket server using environment search */
if(env) while(*env) { /* while there's environment-variable info left... */
	for(s= env; *s && *s != MACHINE_SEP; ++s);
	more= *s == MACHINE_SEP;
	*s  = '\0';
	sprintf(buf,"%s@%s",sktname,env);

	/* attempt to open Socket given machine name */
	skt= Sopen(buf,ctrl);
	if(more) *s= MACHINE_SEP;

	if(skt) { /* successfully opened Socket */
		if(at) *at= '@';
		return skt;
		}
	if(more) {	/* another path to search */
		*s = MACHINE_SEP;
		env= s + 1;
		}
	else env= s;
	}

/* restore '@' if it was present */
if(at) *at= '@';

/* return NULL pointer, thereby modestly indicating a lack of success */
return (Socket *) NULL;
}

/* =======================================================================
 * Test Code:
 */
#ifdef DEBUG_TEST
#ifdef unix
void *sbrk(int);
#endif

/* main: */
int main(int argc,char **argv)
{
Socket *skt    = NULL;
#ifdef unix
char *memstart = NULL;
char *memory   = NULL;
#endif

rdcolor();


#ifdef unix
memstart= NULL;
#endif

do {
	skt= Sopenv("SOPENV","c","SKTPATH");
	++cnt;
#ifdef unix
	memory= sbrk(0);
	if(!memstart) memstart= memory;
	else if(memory > memstart)
	  error(XTDIO_WARNING,"memory leak: ([memory=%dd] - [memstart=%dd])= %d\n",
	  (int) memory,(int) memstart,memory - memstart);
#endif
	} while(!skt);
Sclose(skt);
printf("took %d tries to open the SOPENV client\n",cnt);

return 0;
}
#endif	/* DEBUG_TEST */

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
