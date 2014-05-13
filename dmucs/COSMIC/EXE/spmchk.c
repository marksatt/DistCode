/* spmchk: see if there is a portmaster running on the specified machine
 *  by attempting to open a Socket on it. If the attempt fails, return 1,
 *  otherwise 0.
 *
 *  XTDIO_USAGE: spmchk [machine]
 *
 *  This can be used in a script like so:
 *
 *	spmchk || Spm
 *
 *      -or-
 *
 *	spmchk || (nohup Spm > /dev/null)&
 *
 *  to avoid having Spm print out an error message if it is already
 *  running.
 *
 *  Author:  Marty Olevitch, Dec 1993
 *       (This routine is placed into the public domain.)
 */

#include <stdio.h>
#include <string.h>
#include "sockets.h"

/* ---------------------------------------------------------------------
 * Definitions:
 */
#define BUFSIZE 256

/* ---------------------------------------------------------------------
 * Prototypes:
 */
#ifdef __PROTOTYPE__
int main(int, char **);
#else
extern main();
#endif

/* ---------------------------------------------------------------------
 * Source Code:
 */

#ifdef __PROTOTYPE__
int main(
  int    argc,
  char **argv)
#else
int main(argc, argv)
int    argc;
char **argv;
#endif
{
char    hostnm[BUFSIZE];
Socket *s;

Sinit();

if(argc > 1) strcpy(hostnm, argv[1]);
else         gethostname(hostnm,BUFSIZE);

s = Sopen_clientport(hostnm, "PMClient", PORTMASTER);
if (!s) {
	exit(2);
	}
Sclose(s);

#ifdef vms
exit(1);
#else
exit(0);
#endif
}

/* --------------------------------------------------------------------- */
