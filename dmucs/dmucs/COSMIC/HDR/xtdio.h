/* xtdio.h:
 * Authors:   Charles E. Campbell, Jr.
 *  		  Terry McRoberts
 * Copyright: Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *            Permission is hereby granted to use and distribute this code,
 *            with or without modifications, provided that this copyright
 *            notice is copied with it. Like anything else that's free,
 *            sockets.h is provided *as is* and comes with no warranty
 *            of any kind, either expressed or implied. By using this
 *            software, you agree that in no event will the copyright
 *            holder be liable for any damages resulting from the use
 *            of this software.
 *  Date:	  Aug 22, 2005
 */
#ifndef XTDIO_H
#define XTDIO_H

/* --------------------------------------------------------------------- */

#ifdef AS400
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
#endif

#ifdef _AIX
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
#endif

#ifdef apollo
# include <string.h>
# include <stdarg.h>
# include <stdlib.h>
#endif

#ifdef __CYGWIN__
# include <stdlib.h>
# include <string.h>
#endif

#ifdef __linux__
# include <string.h>
# include <stdlib.h>
# include <stdio.h>
# include <stdarg.h>
#endif

#ifdef _MSC_VER
# include <string.h>
# include <stdlib.h>
# include <stdio.h>
# include <stdarg.h>
#endif

#ifdef os2
# include <string.h>
# include <stdlib.h>
# include <stdio.h>
# include <stdarg.h>
#endif

#ifdef MSDOS
# include <string.h>
# include <stdlib.h>
# include <stdarg.h>
#endif

/* SCO Unix */
#if defined(M_I386) && defined(M_SYSV)
# include <stdlib.h>
# include <stdarg.h>
#endif

#ifdef __osf__
# include <strings.h>
# include <stdlib.h>
# include <stdarg.h>
#endif

#ifdef sgi
# include <strings.h>
# include <stdlib.h>
# include <stdarg.h>
#endif

#ifdef sun
# include <string.h>
# ifdef __STDC__
#  include <stdlib.h>
#  include <stdarg.h>
# else
#  include "xstdlib.h"
#  include "varargs.h"
# endif
#endif

#ifdef ultrix
# include <strings.h>
# include "xstdlib.h"
# include <varargs.h>
#endif

#ifdef vms
# include <string.h>
# include <stdlib.h>
# include <stdarg.h>
#endif

#ifdef __WIN32__
# include <string.h>
# include <stdlib.h>
# include <stdarg.h>
#endif

#include "rdcolor.h"
#include "setproto.h"

/* ---------------------------------------------------------------------
 * Definitions:
 */

#define XTDIO_SEVERE     0
#define XTDIO_ERROR      1
#define XTDIO_WARNING    2
#define XTDIO_NOTE       3

/* if your compiler supports prototyping, but does *not* support the keyword
 * "const", then move the following line out of this comment
#define const
 */

/* ---------------------------------------------------------------------
 * Typedefs:
 */

/* ---------------------------------------------------------------------
 * Global Variables:
 */
#ifdef __PROTOTYPE__
extern void (*error_exit)(int);
#else
extern void (*error_exit)();
#endif

/* ---------------------------------------------------------------------
 * Prototypes:
 */
# ifdef __PROTOTYPE__

char *cprt(const char);                                /* cprt.c          */
void error(int, char *, ...);                          /* error.c         */
FILE *fopenv(char *, char *, char *);                  /* fopenv.c        */
void outofmem(void *, char *, ...);                    /* outofmem.c      */
void rdcolor(void);                                    /* rdcolor.c       */
void rdcputs(char *, FILE *);                          /* rdcolor.c       */
char *sprt(const char *);                              /* sprt.c          */
void srmtrblk(char *);                                 /* srmtrblk.c      */
char *stpblk(char *);                                  /* stpblk.c        */
char *stpnxt(char *, char *);                          /* stpnxt.c        */
char *strnxtfmt(char *);                               /* strnxtfmt.c     */

# else  /* __PROTOTYPE__ */

extern char *cprt();                                   /* cprt.c          */
extern char *stpnxt();                                 /* stpnxt.c        */
extern char *stpblk();                                 /* stpblk.c        */
extern FILE *fopenv();                                 /* fopenv.c        */
extern char *sprt();                                   /* sprt.c          */
extern char *strnxtfmt();                              /* strnxtfmt.c     */

# endif /* __PROTOTYPE__ */

#endif
