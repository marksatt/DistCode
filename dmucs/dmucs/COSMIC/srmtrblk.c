#include <stdio.h>
#include <ctype.h>

/* srmtrblk: remove trailing blanks from string s */
#ifdef __PROTOTYPE__
void srmtrblk(char *s)
#else	/* __PROTOTYPE__ */
void srmtrblk(s)
char *s;
#endif	/* __PROTOTYPE__ */
{
char *ss;
for(ss= s; *ss; ++ss);	/* find end of string */
for(--ss; ss >= s && isspace(*ss) && *ss != '\f'; --ss) *ss= '\0';
}
