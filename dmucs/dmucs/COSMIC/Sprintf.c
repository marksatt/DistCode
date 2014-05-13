/* Sprintf.c:
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Sprintf.c is provided *as is* and comes with no warranty
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

/* Sprintf: */
/*VARARGS*/
#ifdef __PROTOTYPE__
int Sprintf(
  Socket *skt,
  char *fmt,
   ...)
#else
int Sprintf(skt,va_alist)
Socket *skt;
va_dcl
#endif
{
va_list     args;
int         ret;
static char buf[PM_BIGBUF];

#ifndef __PROTOTYPE__
char *fmt=NULL;
#endif


/* sanity check */
if(!skt) {
	return 0;
	}

#ifdef __PROTOTYPE__
va_start(args,fmt);
#else
va_start(args);
fmt = va_arg(args,char *);
#endif
fflush(stdout);

ret= vsprintf(buf,fmt,args);
Swrite(skt,buf,(int)strlen(buf)+1);	/* send the null byte, too */
va_end(args);

return ret;
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
