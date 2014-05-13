/* smclient.c: this file contains a small client example */
#include <stdio.h>
#include "sockets.h"

/* --------------------------------------------------------------------- */

int main()
{
Socket *skt      = NULL;
char    buf[128];

do {
	skt= Sopen("SmallServer","c");
	if(!skt) sleep(1);
	} while(!skt);

Sscanf(skt,"%s",buf);
printf("buf<%s>\n",buf);
Sclose(skt);

return 0;
}
/* --------------------------------------------------------------------- */
