/* outofmem.c */
#include <stdio.h>
#include "sockets.h"

/* ---------------------------------------------------------------------- */

/* outofmem: tests if ptr is null.  If it is, the format and arguments
 * a la printf are put to the stdout and then the program exits
 */
#ifdef __PROTOTYPE__
void outofmem(
  void *ptr,
  char *fmt,
  ...)
#else	/* __PROTOTYPE__ */
void outofmem(ptr,fmt,va_alist)
void *ptr;
char *fmt;
va_dcl
#endif	/* __PROTOTYPE__ */
{
va_list args;

/* check if ptr is not null */
if(ptr) return;

#ifdef __PROTOTYPE__
/* initialize for variable arglist handling */
va_start(args,fmt);

#else	/* __PROTOTYPE__ */

/* initialize for variable arglist handling */
va_start(args);

fmt= va_arg(args,char *);
#endif	/* __PROTOTYPE__ */

fprintf(stderr,"***out of memory*** ");
vfprintf(stderr,fmt,args);
va_end(args);
(*error_exit)(1);
}

/* ---------------------------------------------------------------------- */
