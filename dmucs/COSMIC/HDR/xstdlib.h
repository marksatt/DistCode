/* xstdlib.h: has standard library function prototypes
 * Authors:   Charles E. Campbell, Jr.
 *  		  Terry McRoberts
 * Copyright: Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *            Permission is hereby granted to use and distribute this code,
 *            with or without modifications, provided that this copyright
 *            notice is copied with it. Like anything else that's free,
 *            xstdlib.h is provided *as is* and comes with no warranty
 *            of any kind, either expressed or implied. By using this
 *            software, you agree that in no event will the copyright
 *            holder be liable for any damages resulting from the use
 *            of this software.
 *  Date:	  Aug 22, 2005
 */
#ifndef XSTDLIB_H
#define XSTDLIB_H

#include <sys/types.h>

/* Prototypes for standard library functions */
extern char *calloc();
extern void  exit();
extern char *getenv();
extern char *index();
extern char *malloc();
extern char *strchr();
extern char *strrchr();
#define remove unlink


#endif
