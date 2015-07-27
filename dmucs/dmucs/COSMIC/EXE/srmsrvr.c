/* srmsrvr.c: this program is dangerous, but it does allow one to forcibly
 * remove servers from the PortMaster's desmesne.
 */
#include <stdio.h>
#include "xtdio.h"
#include "sockets.h"

/* ---------------------------------------------------------------------- */

/* main: */
#ifdef __PROTOTYPE__
int main(
  int argc,
  char **argv)
#else	/* __PROTOTYPE__ */
int main(argc,argv)
int argc;
char **argv;
#endif	/* __PROTOTYPE__ */
{
rdcolor();
Sinit();

for(--argc, ++argv; argc > 0; --argc, ++argv) {
	if(Srmsrvr(*argv) != PM_OK)
	  error(XTDIO_WARNING,"failed to remove <%s%s%s>\n",MAGENTA,*argv,GREEN);
	}

#ifdef vms
return 1;
#else
return 0;
#endif
}

/* ---------------------------------------------------------------------- */

