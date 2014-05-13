/* Svprintf.c:
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Svprintf.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sockets.h"

/* --------------------------------------------------------------------- */

/* Svprintf:
 *
 *   Returns: count of bytes sent, which may be 0
 *
 *   This function is like vprintf; it takes a Socket pointer, a format
 *   string, and an argument list.  Note that it actually "prints" the
 *   string into a local buffer first of PM_BIGBUF bytes (originally 1024
 *   bytes).  So, please insure that you don't put more info than will fit
 *   into PM_BIGBUF bytes!  It then squirts the resulting string through
 *   a call to Swrite.
 */
#ifdef __PROTOTYPE__
int Svprintf(
  Socket *skt,
  char   *fmt,
  ...)
#else
int Svprintf(skt,fmt,args)
Socket *skt;
char   *fmt;
void   *args;
#endif
{
int         ret;
static char buf[PM_BIGBUF];


/* sanity check */
if(!skt) {
	return 0;
	}

#ifdef AS400
ret= vsprintf(buf,fmt,__va_list args);
#elif __APPLE__
	va_list va;
	va_start(va, fmt);
	ret = vsprintf(buf, fmt, va);
	va_end(va);
#else
ret= vsprintf(buf,fmt,(char *) args);
#endif
Swrite(skt,buf,(int)strlen(buf)+1);	/* send the null byte, too */

return ret;
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
