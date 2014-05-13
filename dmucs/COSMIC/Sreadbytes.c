/* Sreadbytes.c:
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Sreadbytes.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#include "sockets.h"

/* --------------------------------------------------------------------- */

/* Sreadbytes: this function performs a socket read.
 *  Gets buflen bytes, so long as read is working.
 *  Otherwise, (read fails with a -1), the number
 *  of bytes received will be returned.
 */
#ifdef __PROTOTYPE__
int Sreadbytes(
  Socket *skt,		/* socket handle			*/
  void   *buf,		/* socket character buffer	*/
  int     buflen)	/* max length of buffer		*/
#else
int Sreadbytes(
  skt,				/* socket handle			*/
  buf,				/* socket character buffer	*/
  buflen)			/* max length of buffer		*/
Socket *skt;
void   *buf;
int     buflen;
#endif
{
int cnt;
int rcnt;


/* sanity check */
if(!skt) {
	return -1;
	}

/* get buflen bytes, no matter the wait */
for(cnt= 0; cnt < buflen; cnt+= rcnt) {
	rcnt= (int)recv(skt->skt,(void *) (((char *)buf)+cnt),(unsigned) (buflen-cnt),0);
	if(rcnt < 0) break;
	}

return cnt;
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
