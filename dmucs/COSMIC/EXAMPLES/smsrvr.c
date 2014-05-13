/* smsrvr.c: a real small server example */
#include <stdio.h>
#include "sockets.h"

/* --------------------------------------------------------------------- */

int main()
{
Socket *srvr = NULL;
Socket *skt  = NULL;

srvr= Sopen("SmallServer","s");
skt = Saccept(srvr);

Sprintf(skt,"hello");
Sclose(skt);
Sclose(srvr);

return 0;
}

/* --------------------------------------------------------------------- */
