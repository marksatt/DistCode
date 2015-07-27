/* rdcolor.c: this program initializes color/printer variables using a
 *   rdcolor.dat file if it exists.  Otherwise, default values are used.
 *   The <rdcolor.dat> file uses the following format:
 *	color ...<CR>
 *   where the ellipsis is the escape sequence to be used for the "color".
 *   The following characters have special meaning when preceded by a
 *   backslash:
 *	     \e     =>      escape character
 *	     \f     =>      formfeed character
 *	     \h     =>      backspace character
 *	     \n     =>      newline character
 *	     \t     =>      tab character
 *	     \A - \Z=>      Associated control characters
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "xtdio.h"

/*
 * #define DEBUG_TEST
 * #define DEBUG
 */

/* -------------------------------------------------------------------------
 * Definitions:
 */
#define LINESIZE 80
#define TERMSIZE 20

/* -------------------------------------------------------------------------
 * Global Data:
 */
char *BLACK   = NULL;
char *RED     = NULL;
char *GREEN   = NULL;
char *YELLOW  = NULL;
char *BLUE    = NULL;
char *MAGENTA = NULL;
char *CYAN    = NULL;
char *WHITE   = NULL;

/* since some systems just have to use BLACK, ..., WHITE for themselves... */
char *RD_BLACK   = NULL;
char *RD_RED     = NULL;
char *RD_GREEN   = NULL;
char *RD_YELLOW  = NULL;
char *RD_BLUE    = NULL;
char *RD_MAGENTA = NULL;
char *RD_CYAN    = NULL;
char *RD_WHITE   = NULL;

char *UBLACK     = NULL;
char *URED       = NULL;
char *UGREEN     = NULL;
char *UYELLOW    = NULL;
char *UBLUE      = NULL;
char *UMAGENTA   = NULL;
char *UCYAN      = NULL;
char *UWHITE     = NULL;

char *RVBLACK    = NULL;
char *RVRED      = NULL;
char *RVGREEN    = NULL;
char *RVYELLOW   = NULL;
char *RVBLUE     = NULL;
char *RVMAGENTA  = NULL;
char *RVCYAN     = NULL;
char *RVWHITE    = NULL;

char *CLEAR      = NULL;
char *BOLD       = NULL;
char *NRML       = NULL;

char *PYELLOW    = NULL;
char *PRED       = NULL;
char *PCYAN      = NULL;
char *PBLACK     = NULL;

/* ------------------------------------------------------------------------- */

static struct colorstr{ /* color data structure			*/
	char **color;	/* points to a color variable			*/
	char *name;		/* gives a name to a color				*/
	char *defalt;	/* default color string: nothing		*/
	} colors[]={
	  {&BLACK,		"BLACK",	"\0"},
	  {&RED,		"RED",		"\0"},
	  {&GREEN,		"GREEN",	"\0"},
	  {&YELLOW,		"YELLOW",	"\0"},
	  {&BLUE,		"BLUE",		"\0"},
	  {&MAGENTA,	"MAGENTA",	"\0"},
	  {&CYAN,		"CYAN",		"\0"},
	  {&WHITE,		"WHITE",	"\0"},
	  {&UBLACK,		"UBLACK",	"\0"},
	  {&URED,		"URED",		"\0"},
	  {&UGREEN,		"UGREEN",	"\0"},
	  {&UYELLOW,	"UYELLOW",	"\0"},
	  {&UBLUE,		"UBLUE",	"\0"},
	  {&UMAGENTA,	"UMAGENTA",	"\0"},
	  {&UCYAN,		"UCYAN",	"\0"},
	  {&UWHITE,		"UWHITE",	"\0"},
	  {&RVBLACK,	"RVBLACK",	"\0"},
	  {&RVRED,		"RVRED",	"\0"},
	  {&RVGREEN,	"RVGREEN",	"\0"},
	  {&RVYELLOW,	"RVYELLOW",	"\0"},
	  {&RVBLUE,		"RVBLUE",	"\0"},
	  {&RVMAGENTA,	"RVMAGENTA","\0"},
	  {&RVCYAN,		"RVCYAN",	"\0"},
	  {&RVWHITE,	"RVWHITE",	"\0"},
	  {&BOLD,		"BOLD",		"\0"},
	  {&NRML,		"NRML",		"\0"},
	  {&PYELLOW,	"PYELLOW",	"\0"},
	  {&PRED,		"PRED",		"\0"},
	  {&PCYAN,		"PCYAN",	"\0"},
	  {&PBLACK,		"PBLACK",	"\0"},
	  {&CLEAR,		"CLEAR",	"\0"}
	  };
static int ncolor      = sizeof(colors)/sizeof(struct colorstr);
char      *colorfile[] = {"rdcolor.dat","color.dat"};
static int ncolorfile  = sizeof(colorfile)/sizeof(char *);
static int rdcolor_done= 0;	/* has rdcolor been called? */

/* ------------------------------------------------------------------------- */

/* rdcolor: read the <rdcolor.dat> file */
#ifdef __PROTOTYPE__
void rdcolor(void)
#else	/* __PROTOTYPE__ */
void rdcolor()
#endif	/* __PROTOTYPE__ */
{
FILE *fp    = NULL;
char *cmd   = NULL;
char *new   = NULL;
char *lterm = NULL;
int   icolor;
int   foundterm;

static char  color[LINESIZE];
static char  line[LINESIZE];
static char  newdef[LINESIZE+LINESIZE];
static char *term=NULL;


/* indicate that rdcolor() has been called */
rdcolor_done= 1;

/* set up default colors */
for(icolor= 0; icolor < ncolor; ++icolor) *(colors[icolor].color)= colors[icolor].defalt;

/* attempt to open <rdcolor.dat> file */
for(icolor= 0; icolor < ncolorfile; ++icolor) {
	fp= fopenv(colorfile[icolor],"r","RDCOLOR");
	if(fp) break;
	}

if(!fp) { /* use default values */
	/* set up RD_... colors */
	RD_BLACK  = BLACK;
	RD_RED    = RED;
	RD_GREEN  = GREEN;
	RD_YELLOW = YELLOW;
	RD_BLUE   = BLUE;
	RD_MAGENTA= MAGENTA;
	RD_CYAN   = CYAN;
	RD_WHITE  = WHITE;
	return;
	}

/* attempt to locate correct terminal definition */
#ifdef unix
term= getenv("TERM");
#endif
#ifdef LATTICE
term= "TI";
#endif
#ifdef DESMET
term= "TI";
#endif
#ifdef AZTEC_C
term= "Amiga"
#endif
#ifdef vms
term= "vms";
#endif
#ifdef MSDOS
term= "ibmpc";
#endif

if(term) {  /* search for terminal entry in <rdcolor.dat> file */
	foundterm= 0;
	while(fgets(line,LINESIZE,fp)) if(!isspace(line[0])) {
		srmtrblk(line);
		for(lterm= strtok(line,"|:"); lterm; lterm=strtok((char *) NULL,"|:")) {
			lterm= stpblk(lterm);
			srmtrblk(lterm);
			if(!strcmp(lterm,term)) {
				foundterm= 1;
				break;
				}
			}	/* term|term|term|term: search */
		if(foundterm) break;
		}

	if(!foundterm) { /* couldn't find terminal entry */
		fclose(fp);
		return;
		}
	}

while(fgets(line,LINESIZE,fp)) { /* read color & use cmd sequence */

	/* skip blank lines */
	for(cmd= line; *cmd; ++cmd) if(!isspace(*cmd)) break;
	if(!*cmd) continue;
	if(cmd == line) break; /* finished reading in terminal def'n */

	/* if first non-blank character is a "#", skip it (comment) */
	if(*cmd == '#') continue;

	/* get the color */
	sscanf(cmd,"%s",color);

	/* skip to first white space character */
	for(; *cmd && !isspace(*cmd); ++cmd);
	cmd[strlen(cmd) - 1]= '\0';	/* remove newline character */

	/* skip over tabs following colorname */
	while(*cmd == '\t') ++cmd;

	/* identify color (if known) */
	for(icolor= 0; icolor < ncolor; ++icolor)
	  if(!strcmp(colors[icolor].name,color)) break;
	if(icolor >= ncolor) { /* unknown color */
		error(XTDIO_WARNING, "(rdcolor) ignoring unknown color <%s> in <%s>\n",
		  color,colorfile);
		continue;
		}


	/* copy cmd (new color) over to newdef (new definition), making
	 *  appropriate changes due to backslashes
	 */
	for(new= newdef; *cmd; ++cmd,++new) {
		if(*cmd == '\\') {
			switch(*++cmd) {
			case 'e' : /* escape character		*/
				*new= '\033';
				break;
			case 'f' : /* formfeed character	*/
				*new= '\f';
				break;
			case 'h' : /* backspace character	*/
				*new= (char) 8;
				break;
			case 'n' : /* newline character		*/
				*new= '\n';
				break;
			case 't' : /* tab character		*/
				*new= '\t';
				break;
			default:   /* handles A-Z and other	*/
				if('A' <= *cmd && *cmd <= 'Z') *new= *cmd - 64;
				else *new= *cmd;
				break;
				}
			}
		else *new= *cmd;
		}
	*new= '\0'; /* terminate new definition with null byte */

	/* allocate memory and then set color to colorfile-defined value */
	*colors[icolor].color= (void *) calloc((size_t) (strlen(newdef)+1),sizeof(char));
	if(newdef[0]) strcpy(*colors[icolor].color,newdef);
	else          *colors[icolor].color= "\0";

	}

/* close the colorfile */
fclose(fp);

/* set up RD_... colors */
RD_BLACK  = BLACK;
RD_RED    = RED;
RD_GREEN  = GREEN;
RD_YELLOW = YELLOW;
RD_BLUE   = BLUE;
RD_MAGENTA= MAGENTA;
RD_CYAN   = CYAN;
RD_WHITE  = WHITE;

}

/* ------------------------------------------------------------------------- */

/* rdcputs: read color's put-string function.  Normally, rdcputs acts exactly
 *  like fputs, except that "\255X#" strings are translated to appropriate
 *  colors.
 */
#ifdef __PROTOTYPE__
void rdcputs(
  char *s,
  FILE *fp)
#else	/* __PROTOTYPE__ */
void rdcputs(s,fp)
char *s;
FILE *fp;
#endif	/* __PROTOTYPE__ */
{
int icolor;

if(*s == '\255' && *(s+1) == 'X') { /* translate */
	if(rdcolor_done) {
		/* only try interpreting this if rdcolor() has been called */
		sscanf(s+2,"%d",&icolor);
		fputs(*colors[icolor].color,fp);
		}
	}
else fputs(s,fp);
}

/* ------------------------------------------------------------------------- */
