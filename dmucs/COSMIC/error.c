/* error.c: this file contains an error function
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "sockets.h"

/* ------------------------------------------------------------------------
 * Definitions:
 */
#define TERMINATE	1

#ifdef vms
void exit(int);
#endif
#ifdef MACOS_X
void exit(int);
#endif
#ifdef MSDOS
extern void exit(int);
#endif

/* ------------------------------------------------------------------------
 * Global Variables:
 */
#ifdef __PROTOTYPE__
void (*error_exit)(int)= exit;	/* user may override this		*/
#else
void (*error_exit)()= exit;		/* user may override this		*/
#endif

/* ------------------------------------------------------------------------
 * Source Code:
 */

/*VARARGS*/
/* error: print out an error message:
 *		error(severity,fmt,...)
 * or	error(severity,errfunc,fmt,...)
 *
 * where
 *
 *	severity: severity of error
 *		XTDIO_SEVERE  : severe error    - will terminate
 *		XTDIO_ERROR   : error           - will terminate
 *		XTDIO_WARNING : warning message
 *		XTDIO_NOTE    : notification
 *
 *	fmt    : printf format specification
 *	...    : arguments for the fmt
 *
 *	errfunc: 
 *	         will be called
 */
#ifdef __PROTOTYPE__
void error(
  int severity,
  char *fmt,
  ...)
#else	/* __PROTOTYPE__ */
void error(severity,fmt,va_alist)
int severity;
char *fmt;
va_dcl
#endif	/* __PROTOTYPE__ */
{
va_list args;
static struct mesgstr {
	char *bgn_color;
	char *message;
	char *end_color;
	int  flags;
	} mesg[]={
	{XRED,    "fatal error",XGREEN,TERMINATE},				/* SEVERE	*/
	{XRED,    "error",      XGREEN,TERMINATE},				/* ERROR	*/
	{XYELLOW, "warning",    XGREEN,0},						/* WARNING	*/
	{XCYAN,   "note",       XGREEN,0}						/* NOTE		*/
	};
static int nmesg= sizeof(mesg)/sizeof(struct mesgstr);

/* severity sanity checks */
if(severity < 0)      severity= 0;
if(severity >= nmesg) severity= XTDIO_SEVERE;

rdcputs(mesg[severity].bgn_color,stderr);
fprintf(stderr,"***%s*** ",mesg[severity].message);
rdcputs(mesg[severity].end_color,stderr);

/* initialize for variable arglist handling */
#ifdef __PROTOTYPE__
va_start(args,fmt);
vfprintf(stderr,fmt,args);
va_end(args);
#else
va_start(args);
vfprintf(stderr,fmt,args);
va_end(args);
#endif

/* terminate */
if(mesg[severity].flags & TERMINATE) (*error_exit)(1);
}

/* ------------------------------------------------------------------------ */

