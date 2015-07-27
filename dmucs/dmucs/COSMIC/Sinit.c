/* Sinit.c: this file contains interfaces to initialize and cleanup.
 *  Most o/s's don't need this, but some (WIN32, MSDOS) do.
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Sinit.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#include "sockets.h"

#if defined(MSDOS) || defined(__WIN32__) || defined(_MSC_VER)
static int ssl_init= 0;
#endif

/* --------------------------------------------------------------------- */

/* Sinit: initializes Sockets */
void Sinit()
{

#ifdef MSDOS										//<<-- Old version (changed back)
/* initialize WIN/API DOS Socket library */
if(ssl_init == 0) {
	ssl_init= 1;
	if(socket_init()) error(XTDIO_ERROR,"socket_init() returned -1, cannot start\n");
	}
#endif

#if defined(__WIN32__) || defined(_MSC_VER)					//<<-- Old version (changed back)
if(ssl_init == 0) {
	static WORD    wVersionRequested;
	static WSADATA wsaData;
	int     err;
	ssl_init= 1;
	wVersionRequested= MAKEWORD(1,1);
	err= WSAStartup(wVersionRequested,&wsaData);
	if(err) error(XTDIO_ERROR,"unable to use sockets, needs version 2.0 or better\n");
	if(LOBYTE(wsaData.wVersion) != 1 ||
	   HIBYTE(wsaData.wVersion) != 1) {
		int hiver;
		int lover;
		hiver= LOBYTE(wsaData.wVersion);
		lover= HIBYTE(wsaData.wVersion);
		WSACleanup();
		error(XTDIO_ERROR,"wrong sockets version (wants 1.1, has %d.%d)\n",
		  hiver,lover);
		}
	atexit((void (*)(void)) WSACleanup);
	}
#endif

}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
