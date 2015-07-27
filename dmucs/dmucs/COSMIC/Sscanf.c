 /* Sscanf.c:
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Sscanf.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#include <ctype.h>
#include "sockets.h"

/* -----------------------------------------------------------------------
 * Local Definitions:
 */
#define BUFSIZE 256

/* ----------------------------------------------------------------------- */

/* Sscanf: */
/*VARARGS*/
#ifdef __PROTOTYPE__
int Sscanf(Socket *skt,char *fmt,...)
#else
int Sscanf(skt,va_alist)
Socket *skt;
va_dcl
#endif
{
va_list     args;
#ifdef MSDOS
char       *arg;
#else
void       *arg;
#endif
char       *b       = NULL;
char       *bufend  = NULL;
char       *f       = NULL;
char       *snglfmt = NULL;
int         buflen;
int         cnt     = 0;
static char buf[PM_BIGBUF];

#ifndef __PROTOTYPE__
char *fmt;
#endif

#ifdef SSLNOPEEK
char *bb;
#endif


/* sanity check */
if(!skt) {
	return -1;
	}

#ifdef __PROTOTYPE__
va_start(args,fmt);
#else
va_start(args);
fmt = va_arg(args,char *);
#endif

/* process argument string, one argument at a time */
for(f= fmt, buflen= 1; buflen > 0; f= NULL) {

	/* get next argument from stack.  According to the usual
	 * definition of scanf-like functions, it must be a
	 * pointer.  The associated %... format code will tell
	 * Sscanf the type of pointer.
	 */

	arg= va_arg(args,void *);
	if(!arg) break;

	/* get associated snglfmt */
	snglfmt= strnxtfmt(f);
	if(!snglfmt) break;

	/* wait until something shows up on the Socket */
	Swait(skt);

	/* peek at socket data, and keep at it until at least one byte is
	 * not processed by the snglfmt
	 */
#ifdef SSLNOPEEK
	bb= buf;
#endif

    do {

#ifdef SSLNOPEEK
        /* read socket buffer one byte at a time :-( */
        if(recv(skt->skt,bb,(unsigned) 1,0) <= 0) {
            va_end(args);
            return cnt;
            }
         b     = bb;
         buflen= bb - buf + 1;
         if(*(bb++) == '\0') break;


#else	/* #ifdef SSLNOPEEK ... #else ... #endif */

		/* peek at socket buffer */
		buflen     = Speek(skt,buf,PM_BIGBUF-1);
		/* on error, return count of Sscanf'd args	*/
		if(buflen < 0) {
			va_end(args);
			return cnt;
			}
#endif	/* #ifdef SSLNOPEEK ... #else ... #endif */

		/* terminate buffer appropriately with a null byte */
		bufend = buf + buflen;
		*bufend= '\0';

		/* get a pointer to the character in buf just beyond that the
		 * snglfmt will process.  If that character is bufend, then
		 * it is possible that the snglfmt could process more data than
		 * is currently in buf.  Otherwise, b < bufend and so all the
		 * data that snglbuf will process is available.
		 */
		b= stpnxt(buf,snglfmt);
		} while(b && b == bufend);

	/* read (b - buf) bytes from the socket */
	if(b && !*b) ++b;		/* read in null byte, if one is present */
#ifndef SSLNOPEEK
	buflen= Sreadbytes(skt,buf,(int) (b - buf));
#endif
	if(buflen < 0) break;	/* whoops - got an error! */
	buf[buflen]= '\0';		/* terminate buf properly */

	/* sscanf the buf to set up the arg */
	if(sscanf(buf,snglfmt,arg)) ++cnt;
	else                        break;
	}

/* leave Sscanf */
va_end(args);

return cnt;
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
