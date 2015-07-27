/* Smkskt.c: this function makes a Socket
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Smkskt.c is provided *as is* and comes with no warranty
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

/* makeSocket: this function allocates a Socket */
#ifdef __PROTOTYPE__
Socket *makeSocket(
  char *hostname,
  char *sktname,
  int   sktType)
#else
Socket *makeSocket(hostname, sktname, sktType)
char *hostname;
char *sktname;
int   sktType;
#endif
{
Socket *skt= NULL;


/* allocate a Socket */
skt= (Socket *) malloc(sizeof(Socket));
outofmem((void *) skt,
  "makeSocket: while allocating a Socket for <%s:%s>\n",
  hostname,sktname);

/* initialize Socket */
skt->hostname= (char *) calloc((size_t) strlen(hostname)+1,sizeof(char));
outofmem((void *) skt->hostname,
  "makeSocket: while allocating string for <%s>\n",hostname);
strcpy(skt->hostname,hostname);

if(!sktname) sktname= "";
skt->sktname= (char *) calloc((size_t) strlen(sktname)+1,sizeof(char));
outofmem((void *) skt->sktname,"makeSocket: while allocating string for <%s>\n",sktname);
strcpy(skt->sktname,sktname);

/* set port and skt to zero */
skt->port= skt->skt= 0;
skt->type= sktType;

/* return Socket */
return skt;
}

/* ------------------------------------------------------------------------ */

/* freeSocket: this function frees up a Socket */
#ifdef __PROTOTYPE__
void freeSocket(Socket *skt)
#else
void freeSocket(skt)
Socket *skt;
#endif
{

if(skt) {
	if(skt->hostname) free(skt->hostname);
	if(skt->sktname)  free(skt->sktname);
	skt->skt      = 0;
	skt->port     = 0;
	skt->type     = 0;
	skt->hostname = NULL;
	skt->sktname  = NULL;
	free((char *) skt);
	}

}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
