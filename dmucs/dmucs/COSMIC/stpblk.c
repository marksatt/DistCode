#include <stdio.h>
#include <ctype.h>
  
/* stpblk: skips blanks (white space) */
#ifdef __PROTOTYPE__
char *stpblk(char *p)
#else	/* __PROTOTYPE__ */
char *stpblk(p)
char *p;
#endif	/* __PROTOTYPE__ */
{
while(isspace(*p)) ++p;
return p;
}
