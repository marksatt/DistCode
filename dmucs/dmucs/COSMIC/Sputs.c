/* Sputs.c: this function "puts" a string which Sgets can receive
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Sputs.c is provided *as is* and comes with no warranty
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

/* Sputs: */
#ifdef __PROTOTYPE__
void Sputs(
  char   *buf,
  Socket *skt)
#else
void Sputs(
  buf,
  skt)
char   *buf;
Socket *skt;
#endif
{

/* write out buf and the null byte */
Swrite(skt,buf,(int)strlen(buf)+1);

}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
