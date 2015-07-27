/* rdcolor.h: this file specifies that all colors are actually pointers to
 *  color strings.  It is expected that the function "rdcolor()" will
 *  be used to initialize the colors.  "rdcolor()" examines a colorfile
 *  called "color.dat" for color strings: color ...<CR> where the ellipsis
 *  is the desired escape string.  The default value of such color strings
 *  is set for a color ASCII monitor (8 colors) and the IDS Prism printer.
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
#ifndef RDCOLOR_H
#define RDCOLOR_H

#ifndef __GL_GL_H__
extern char *BLACK;
extern char *RED;
extern char *GREEN;
extern char *YELLOW;
extern char *BLUE;
extern char *MAGENTA;
extern char *CYAN;
extern char *WHITE;
#endif

extern char *RD_BLACK;		/* same as BLACK		*/
extern char *RD_RED;		/* same as RED			*/
extern char *RD_GREEN;		/* same as GREEN		*/
extern char *RD_YELLOW;		/* same as YELLOW		*/
extern char *RD_BLUE;		/* same as BLUE			*/
extern char *RD_MAGENTA;	/* same as MAGENTA		*/
extern char *RD_CYAN;		/* same as CYAN			*/
extern char *RD_WHITE;		/* same as WHITE		*/

extern char *UBLACK;
extern char *URED;
extern char *UGREEN;
extern char *UYELLOW;
extern char *UBLUE;
extern char *UMAGENTA;
extern char *UCYAN;
extern char *UWHITE;
extern char *RVBLACK;
extern char *RVRED;
extern char *RVGREEN;
extern char *RVYELLOW;
extern char *RVBLUE;
extern char *RVMAGENTA;
extern char *RVCYAN;
extern char *RVWHITE;
#ifndef VIM__H
extern char *CLEAR;
#endif
extern char *BOLD;
extern char *NRML;
extern char *PYELLOW;
extern char *PRED;
extern char *PCYAN;
extern char *PBLACK;

/* for rdcputs's benefit... */
#define XBLACK		"\255X0"
#define XRED		"\255X1"
#define XGREEN		"\255X2"
#define XYELLOW		"\255X3"
#define XBLUE		"\255X4"
#define XMAGENTA	"\255X5"
#define XCYAN		"\255X6"
#define XWHITE		"\255X7"
#define XUBLACK		"\255X8"
#define XURED		"\255X9"
#define XUGREEN		"\255X10"
#define XUYELLOW	"\255X11"
#define XUBLUE		"\255X12"
#define XUMAGENTA	"\255X13"
#define XUCYAN		"\255X14"
#define XUWHITE		"\255X15"
#define XRVBLACK	"\255X16"
#define XRVRED		"\255X17"
#define XRVGREEN	"\255X18"
#define XRVYELLOW	"\255X19"
#define XRVBLUE		"\255X20"
#define XRVMAGENTA	"\255X21"
#define XRVCYAN		"\255X22"
#define XRVWHITE	"\255X23"
#define XBOLD		"\255X24"
#define XNRML		"\255X25"
#define XPYELLOW	"\255X26"
#define XPRED		"\255X27"
#define XPCYAN		"\255X28"
#define XPBLACK		"\255X29"
#define XCLEAR		"\255X30"
#endif
