/* sprt.c: */
#include <stdio.h>
#include <string.h>
#include "xtdio.h"

/* -------------------------------------------------------------------------
 * Local Definitions:
 */
#define BUFSIZE 261
#define SFTY      5

/* sprt: this routine makes string characters all visible */
#ifdef __PROTOTYPE__
char *sprt(const char *s)
#else	/* __PROTOTYPE__ */
char *sprt(s)
char *s;
#endif	/* __PROTOTYPE__ */
{
static char  buf1[BUFSIZE];
static char  buf2[BUFSIZE];
static char  buf3[BUFSIZE];
static char  buf4[BUFSIZE];
char        *b;
char        *bend;
static char *buf           = buf3;
int          ic;

/* allows up to three sprt()s in one function call */
if(buf == buf1)      buf= buf2;
else if(buf == buf2) buf= buf3;
else if(buf == buf3) buf= buf4;
else                 buf= buf1;

buf[0]= '\0';
bend  = buf + BUFSIZE - SFTY;
if(s) for(b= buf; *s && b < bend; ++s, b+= strlen(b)) {
	ic= (int) *s;
	if(ic < 31)        sprintf(b,"^%c",(char) (ic + 64));
	else if(ic >= 128) sprintf(b,"~%3d",ic);
	else {
		*b    = *s;
		*(b+1)= '\0';
		}
	b+= strlen(b);
	}
return buf;
}

/* --------------------------------------------------------------------- */
