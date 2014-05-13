#include <stdio.h>
#include <ctype.h>
#include "xtdio.h"

/* ------------------------------------------------------------------
 * Definitions Section:
 */
#define MAXCNT 65535

/* ------------------------------------------------------------------
 * Source Code:
 */

/* stpnxt: returns a pointer to the next argument in the given string,
 *	assuming a %d, %s, %f, etc. code was used to interpret the
 *	present argument.
 */
#ifdef __PROTOTYPE__
char *stpnxt(
  char *s,			/* source string	*/
  char *fmt)		/* format control string (ie. format control code string) */
#else
char *stpnxt(s,fmt)
char *s;			/* source string	*/
char *fmt;			/* format control string (ie. format control code string) */
#endif
{
char *ss;
int   cmax;

/* if s is NULL return NULL */
if(s == NULL) {
	return (char *) NULL;
	}

/* process control string */
for(ss= s; *s && *fmt ; ++fmt, ss= s) {
	/* scan code processing */
	if(*fmt == '%') {
		++fmt;

		/* %[-]###.: handle predecessor stuff */
		if(*fmt == '-') ++fmt;	
		if(*fmt && isdigit(*fmt)) {
			sscanf(fmt,"%d",&cmax);
			while(*fmt && (isdigit(*fmt) || *fmt == '.')) ++fmt;
			}
		else cmax= MAXCNT;

		/* %...l.: remove the 'l' from consideration
		 * %...h.: remove the 'h' from consideration
		 */
		if(*fmt == 'l' || *fmt == 'h') ++fmt;

		/* handle the format code character */
		switch(*fmt) {

		/* %s: string processing */
		case 's':
			while(*s && isspace(*s)) ++s;
			while(*s && !isspace(*s)) ++s;
			break;

		/* %c: character processing */
		case 'c':
			if(*s) ++s;
			break;

		/* %x: hexadecimal processing */
		case 'x':
			s= stpblk(s);
			ss= s;
			if(*s == '+' || *s == '-') ++s;
			if(s[0] == '0' && s[1] == 'x') s+= 2;
			while(*s && (isdigit(*s) || (*s >= 'a' && *s <= 'f') || (*s >= 'A' && *s <= 'F'))) ++s;
			break;

		/* %o: octal processing */
		case 'o':
			while(*s && isspace(*s)) ++s;
			ss= s;
			if(*s == '+' || *s == '-') ++s;
			while(*s && *s >= '0' && *s <= '7') ++s;
			break;

		/* %d, %e, %f, %g, %u: integer and floating point number processing */
		case 'd':
		case 'e':
		case 'f':
		case 'g':
		case 'u':
			while(*s && isspace(*s)) ++s;
			ss= s;
			if(*s == '-' || *s == '+') ++s;
			while(*s && isdigit(*s)) ++s;
			if(*fmt == 'd') break;
			if(*s == '.') ++s;
			while(*s && isdigit(*s)) ++s;
			if(*s == 'e') { /* exponent handling */
				++s;
				if(*s == '-' || *s == '+') ++s;
				while(isdigit(*s) && *s) ++s;
				}
			break;

		/* %%: single character processing */
		case '%':
			while(*s && isspace(*s)) ++s;
			if(*s != *fmt) {
				return s;
				}
			break;

		/* %b: slight extension, skips over blanks */
		case 'b':
			while(*s && isspace(*s)) ++s;
			break;

		/* unknown */
		default:
			error(XTDIO_WARNING,"unknown stpnxt code <%s%c%s>\n",
			  RD_MAGENTA,
			  *fmt,
			  RD_GREEN);
			break;
			}
		}

	/* white space in control string skipping */
	else if(*fmt && isspace(*fmt)) continue;

	/* non-white space: must match string */
	else {
		while(*s && isspace(*s)) ++s;
		if(*s != *fmt) {
			return s;
			}
		else {	/* s and c match, so continue forth */
			++s;
			continue;
			}
		}
	if(s <= ss) {
		return ss;
		}
	}	/* c: loop */

return s;
}

/* --------------------------------------------------------------------- */
