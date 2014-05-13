/* Speeraddr.c: this function gets the name of the peer
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Speeraddr.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#include "sockets.h"

/* ---------------------------------------------------------------------
 * Definitions:
 */
#define PEERBUF	65

/* --------------------------------------------------------------------- */

/* Speeraddr: get peer IP address */
#ifdef __PROTOTYPE__
unsigned long Speeraddr(Socket *skt)
#else
unsigned long Speeraddr(skt)
Socket *skt;
#endif
{
unsigned long          ret;
static struct sockaddr sktname;
#if defined(_AIX)
static size_t          namelen = sizeof(struct sockaddr);
#elif defined(__gnu_linux__)
static socklen_t       namelen = sizeof(struct sockaddr);
#else
static int             namelen = sizeof(struct sockaddr);
#endif



if(!skt) ret= 0;

else if(getpeername(skt->skt,&sktname,(socklen_t*)&namelen) == -1) {
	ret= 0;
	}

else {
	union {
		unsigned char byte[4];
		unsigned long i;
		} u;
	u.byte[0]     = sktname.sa_data[2];
	u.byte[1]     = sktname.sa_data[3];
	u.byte[2]     = sktname.sa_data[4];
	u.byte[3]     = sktname.sa_data[5];
	ret           = u.i;
	}

return ret;
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

printf("client<%s> accepted, peer is %lu\n",Sprtskt(skt),Speeraddr(skt));

Sclose(skt);
Sclose(srvr);

return 0;
}

#endif	/* DEBUG_TEST */

/* =====================================================================
 * vim: ts=4
 */
