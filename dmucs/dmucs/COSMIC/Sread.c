/* Sread.c:
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Sread.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#include "sockets.h"

/* Sread: this function performs a read from a Socket */
#ifdef __PROTOTYPE__
int Sread(
  Socket *skt,		/* socket handle			*/
  void   *buf,		/* socket character buffer	*/
  int     buflen)	/* max length of buffer		*/
#else
int Sread(
  skt,				/* socket handle			*/
  buf,				/* socket character buffer	*/
  buflen)			/* max length of buffer		*/
Socket *skt;
void   *buf;
int     buflen;
#endif
{
int cnt;


/* sanity check */
if(!skt) {
	return -1;
	}

/* read bytes from Socket */
cnt  = (int)recv(skt->skt,(void *) buf,(unsigned) buflen,0);

if(cnt > 0) {	/* "cnt" bytes received		*/
	return cnt;
	}

/* error return */
((char *) buf)[0]= '\0';
return 0;
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
