#include <stdio.h>
#include <ctype.h>
#include "xtdio.h"

/* ----------------------------------------------------------------------
 * Local Definitions:
 */
#define BUFSIZE 256

/* ---------------------------------------------------------------------- */

/* strnxtfmt: this function returns a pointer to the end of a format code
 *
 * Use:
 *  strnxtfmt(fmt) : initialize and return first single format code string
 *	strnxtfmt((char *) NULL) : use previous fmt string
 *
 *	Returns a pointer to a single format until no more single formats are
 *	left, in which case a NULL pointer is returned.
 */
#ifdef __PROTOTYPE__
char *strnxtfmt(char *fmt)
#else	/* __PROTOTYPE__ */
char *strnxtfmt(fmt)
char *fmt;
#endif	/* __PROTOTYPE__ */
{
char *lb;
static char locbuf[BUFSIZE];
static char *f=NULL;


/* select format string to be processed */
if(!fmt) fmt= f;
else     f  = fmt;

if(!fmt || !*fmt) {
	return (char *) NULL;
	}

/* Set up a single format string from the fmt variable */
for(lb= locbuf; *fmt; ++fmt) {
	if(*fmt == '%') {
		if(fmt[1] == '%') {	/* enter %% pairs into locbuf directly */
			*lb++ = *fmt;
			*lb++ = *++fmt;
			}
		else {	/* decipher format code: [0-9.hlL\-]*[a-zA-Z] */
			for(*lb++= *fmt++; *fmt; ++fmt) {
				if(isdigit(*fmt) ||
				   *fmt == 'l'   || *fmt == 'L' || *fmt == 'h' ||
				   *fmt == '-'   || *fmt == '.') *lb++= *fmt;
				else break;
				}
			if(*fmt) *lb++ = *fmt++;/* more fmt to be available			*/
			else     *lb   = '\0';	/* end of fmt string encountered	*/
			break;
			}
		}
	else *(lb++)= *fmt;
	}
*lb= '\0';	/* terminate locbuf properly */

/* read strnxtfmt for next call */
f= fmt;

return locbuf;
}

/* ----------------------------------------------------------------------- */

