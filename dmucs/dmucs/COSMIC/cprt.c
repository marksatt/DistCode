#include <stdio.h>
#include "xtdio.h"

/* --------------------------------------------------------------------- */

/* cprt: print out a character so that it is legible */
#ifdef __PROTOTYPE__
char *cprt(const char c)
#else	/* __PROTOTYPE__ */
char *cprt(c)
char c;
#endif	/* __PROTOTYPE__ */
{
static char buf1[10];
static char buf2[10];
static char buf3[10];
static char buf4[10];
static char buf5[10];
static char *buf=buf1;
int ic;

ic= (int) c;
if(ic < 32)        sprintf(buf,"^%c",(char) (ic + 64));
else if(ic == 127) sprintf(buf,"^<DEL>");
else if(ic >= 128) sprintf(buf,"^<%3d>",ic);
else               sprintf(buf,"%c",c);
if(buf == buf1) {
	buf= buf2;
	return buf1;
	}
else if(buf == buf2) {
	buf= buf3;
	return buf2;
	}
else if(buf == buf3) {
	buf= buf4;
	return buf3;
	}
else if(buf == buf4) {
	buf= buf5;
	return buf4;
	}
else {
	buf= buf1;
	return buf5;
	}
}

/* --------------------------------------------------------------------- */
