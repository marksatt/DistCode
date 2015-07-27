/* <setproto.h>: determine if prototype using or not
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
#ifndef SETPROTO_H
# define SETPROTO_H

/* --------------------------------------------------------------------- */

# ifndef __PROTOTYPE__

#  ifdef vms
#   ifndef oldvms
#    define __PROTOTYPE__
#   endif

#  else

#   if defined(sgi)       || defined(apollo)    || defined(__STDC__)  || \
       defined(MCH_AMIGA) || defined(_AIX)      || defined(__MSDOS__) || \
	   defined(ultrix)    || defined(__DECC)    || defined(__alpha)   || \
	   defined(__osf__)   || defined(__WIN32__) || defined(__linux__) || \
	   defined(_MSC_VER)  || defined(os2)       || defined(AS400)
#    define __PROTOTYPE__
#   endif
#  endif
# endif

/* --------------------------------------------------------------------- */

#endif	/* SETPROTO_H */
