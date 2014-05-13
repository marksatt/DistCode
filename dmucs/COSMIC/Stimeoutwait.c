/* Stimeoutwait.c: this file contains code which allows one to wait until data
 *  is present at the Socket (this will block)
 *
 *  Returns: number of bytes of data awaiting perusal
 *	         or -1 if unable to "select" the socket
 *	         or -2 if timeout occurs
 *
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Stimeoutwait.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#define SSLNEEDTIME
#include "sockets.h"

/* ---------------------------------------------------------------------
 * Source Code:
 */

#ifdef __PROTOTYPE__
int Stimeoutwait(Socket *skt,long seconds,long useconds)
#else
int Stimeoutwait(skt,seconds,useconds)
Socket *skt;
long    seconds;
long    useconds;
#endif
{
short          result;
int            ret;
fd_set         emask;
fd_set         rmask;
fd_set         wmask;
struct timeval timeout;
static char    buf[PM_BIGBUF];


/* sanity check */
if(!skt) {
	return -1;
	}

FD_ZERO(&rmask);
FD_SET(skt->skt,&rmask);
FD_ZERO(&wmask);
FD_ZERO(&emask);

/* test if something is available for reading on the socket.  This form
 * will block (sleep) until something arrives
 */
timeout.tv_sec = seconds;
timeout.tv_usec= (int)useconds;

#ifdef SSLNOPEEK


result= select(skt->skt+1,
        rmask.fds_bits,wmask.fds_bits,emask.fds_bits, &timeout);

#else	/* #ifdef SSLNOPEEK  ... #else ... #endif */

result= select(skt->skt+1, &rmask,&wmask,&emask, &timeout);
#endif	/* #ifdef SSLNOPEEK  ... #else ... #endif */

/* socket error */
if(result < 0) {
	return -1;
	}

/* timed out */
else if(result == 0) {
    return -2;
    }

/* server sockets return the select result */
if(skt->type == PM_SERVER) {
	return result;
	}

#ifdef SSLNOPEEK
return 1;

#else	/* #ifdef SSLNOPEEK ... #else ... #endif */

if(FD_ISSET(skt->skt,&rmask)) {
	ret= (int)recv(skt->skt,buf,PM_BIGBUF-1,MSG_PEEK);
	if(result == 1 && ret == 0) ret= EOF;
	return ret;
	}

/* socket is empty */
return 0;
#endif	/* #ifdef SSLNOPEEK ... #else ... #endif */
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
