/* Speername.c: this function gets the name of the peer
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Speername.c is provided *as is* and comes with no warranty
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

/* ---------------------------------------------------------------------
 * Definitions:
 */
#define PEERBUF	65

/* --------------------------------------------------------------------- */

/* Speername: get peername (a string) */
#ifdef __PROTOTYPE__
char *Speername(Socket *skt)
#else
char *Speername(skt)
Socket *skt;
#endif
{
static struct sockaddr_in  sktname;
static char                buf1[PEERBUF];
static char                buf2[PEERBUF];
static char                buf3[PEERBUF];
static char               *buf           = buf1;
#if defined(_AIX)
static size_t              namelen       = sizeof(struct sockaddr);
#elif defined(__gnu_linux__)
static socklen_t           namelen       = sizeof(struct sockaddr);
#else
static int                 namelen       = sizeof(struct sockaddr);
#endif


if     (buf == buf1) buf= buf2;
else if(buf == buf2) buf= buf3;
else                 buf= buf1;

/* sanity check */
if(!skt) strcpy(buf,"null socket");

/* attempt to get information from peer */
else if(getpeername(skt->skt,(struct sockaddr *) &sktname,(socklen_t*)&namelen) == -1) {
	strcpy(buf,"unknown");
	}
else {
	struct hostent *host;

	host= gethostbyaddr((void *) &(sktname.sin_addr.s_addr),sizeof(struct in_addr),AF_INET);

	if(host && host->h_name) strcpy(buf,host->h_name);
	else if(!host)           strcpy(buf,"unable to get host");
	else                     strcpy(buf,"unknown hostname");
	}

return buf;
}

/* ===================================================================== */
#ifdef DEBUG_TEST

/* main: test routine begins here... */
int main(int argc,char **argv)
{
Socket *srvr = NULL;
Socket *skt  = NULL;

rdcolor();				/* initialize color names (GREEN, RED, etc.)    */

srvr= Sopen("GetPeerTest","S");
if(!srvr) error(XTDIO_ERROR,"unable to open <GetPeerTest> server\n");

skt = Saccept(srvr);

printf("client<%s> accepted, peer is <%s>\n",Sprtskt(skt),Speername(skt));

Sclose(skt);
Sclose(srvr);

return 0;
}

#endif	/* DEBUG_TEST */

/* =====================================================================
 * vim: ts=4
 */
