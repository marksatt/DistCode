/* Swrite.c: this file contains the Swrite() function
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Swrite.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#include "sockets.h"

/* Swrite: this function performs a socket write of a string.
 *  It will write out the requested bytes (up to a null byte),
 *  even if the socket buffer is full.
 *
 *  If the write fails, it will return the negative of the number
 *  of bytes actually transmitted.  Otherwise, it will return the
 *  number of bytes transmitted, which ought to be the same as buflen
 *  if life is just.
 */
#ifdef __PROTOTYPE__
int Swrite(
	   Socket *skt,		/* socket handle			*/
	   void   *buf,		/* socket character buffer	*/
	   int     buflen)	/* length of buffer			*/
#else
int Swrite(
	       skt,				/* socket handle		*/
	       buf,				/* socket character buffer	*/
	       buflen)			/* length of buffer			*/
Socket *skt;
void   *buf;
int     buflen;
#endif
{
    int cnt;
    int wcnt;

    /* sanity check */
    if (!skt) {
	return -1;
    }

    /* write buflen bytes, no matter the wait */
    for (cnt = 0; cnt < buflen; cnt += wcnt) {
	wcnt = (int)send(skt->skt, (void *) (((char *) buf) + cnt),
		    (unsigned) (buflen - cnt), 0);
	if (wcnt < 0) {
	    return cnt;
	}
    }
    return cnt;
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
