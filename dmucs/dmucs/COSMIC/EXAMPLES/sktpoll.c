/* sktpoll: this program illustrates sleep-polls for clients */
#include <stdio.h>
#include "xtdio.h"
#include "sockets.h"

/* --------------------------------------------------------------------- */

/* main: begin here */
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
Socket *client=NULL;

client= Sopenv("sktpoll","c","SKTPATH");

while(!client) {
	sleep(1);
	client= Sopenv("sktpoll","c","SKTPATH");
	}

Sputs("hello",client);

Sclose(client);
}

/* --------------------------------------------------------------------- */

