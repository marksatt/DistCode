/* Speek.c:
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Speek.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#define SSLNEEDTIME
#include "sockets.h"

/* ------------------------------------------------------------------------- */

/* Speek: this function peeks at the socket and returns a buffer full of
 *  whatever is on it
 *
 *  Returns: qty of data in buf
 *           1  (SSLNOPEEK) something on buffer, but cannot peek
 *           0  if nothing
 *          EOF on bad select (socket error)
 *              or on select == 1 (something there) but recv indicates
 *              that nothing is there
 */
#ifdef __PROTOTYPE__
int Speek(
  Socket *skt,      /* socket handle           */
  void   *buf,      /* socket character buffer */
  int     buflen)   /* max length of buffer    */
#else
int Speek(
  skt,              /* socket handle           */
  buf,              /* socket character buffer */
  buflen)           /* max length of buffer    */
Socket *skt;
void   *buf;
int     buflen;
#endif
{
short          result;
struct timeval timeout;
fd_set         rmask;
fd_set         wmask;
fd_set         emask;


/* insure a properly null-byte terminated buffer if nothing is returned */
((char *) buf)[0]= '\0';

/* sanity check */
if(!skt) {
	return 0;
	}

FD_ZERO(&rmask);
FD_SET(skt->skt,&rmask);
FD_ZERO(&wmask);
FD_ZERO(&emask);

timeout.tv_sec = 0;
timeout.tv_usec= 0;

/* select checks if
 * the file descriptor set (fds) has something ready for reading
 */
#ifdef SSLNOPEEK
result = select(skt->skt+1,rmask.fds_bits,wmask.fds_bits,emask.fds_bits,
  &timeout);
#else
result = select(skt->skt+1, &rmask,&wmask,&emask, &timeout);
#endif

if(result < 0) {
	return EOF;
	}

if(result == 0) { /* no descriptors ready for reading */
	return 0;
	}

#ifdef SSLNOPEEK

return 1;

#else

/* test if message available from socket, return qty bytes avail */
if(FD_ISSET(skt->skt,&rmask)) {
	buflen= (int)recv(skt->skt,buf,buflen,MSG_PEEK);
	if(result == 1 && buflen == 0) buflen= EOF;
	return buflen;
	}

/* socket is empty */
return 0;

#endif	/* #ifdef SSLNOPEEK ... #else ... #endif */
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
