/* talkclient.c: this program implements a client
 *   Author: Dr Charles E Campbell, Jr
 *
 *               Protocol
 *
 *   server->client
 *   m#...message      message from    #>0: client-id  #==0: server
 *   q                 tell client to quit
 *
 *   client->server
 *   m...message       message from client
 *   q                 client is quitting
 *   Q                 client tells server to quit
 */
#include <stdio.h>
#include "xtdio.h"
#include "sockets.h"

/* ------------------------------------------------------------------------
 * Definitions:
 */
#define BUFSIZE	256
#define SERVER	"TalkServer"
#define PROMPT	"Client: "

/* ------------------------------------------------------------------------
 * Typedefs:
 */

/* ------------------------------------------------------------------------
 * Local Data Structures:
 */

/* ------------------------------------------------------------------------
 * Global Data:
 */

/* ------------------------------------------------------------------------
 * Explanation:
 */

/* ------------------------------------------------------------------------
 * Prototypes:
 */

/* ------------------------------------------------------------------------
 * Source Code:
 */

/* main: */
int main(
  int    argc,
  char **argv)
{
Socket *client       = NULL;
char    buf[BUFSIZE];
int     id;
int     result;

rdcolor();

Smaskfdset(fileno(stdin)); /* put stdin into mask        */
fputs(PROMPT,stdout);      /* print prompt               */
fflush(stdout);            /* insure prompt is displayed */

/* attempt to open client at 1-second intervals */
while(1) {

	if(!client) {
		/* note: if server drops out, this client will attempt to
		 * reconnect at one second intervals
		 */
		while(!client) {
			client= Sopen(SERVER,"c");
			if(Smasktest() > 0) {
				fgets(buf,BUFSIZE,stdin);
				srmtrblk(buf);
				if(!strcmp(buf,"q")) exit(0);
				}
			if(!client) sleep(1);
			}
		Smaskset(client);
		}

	Smaskwait();

	result= Stest(client);
	if(result > 0) {
		if(!Sgets(buf,BUFSIZE,client)) {
			error(XTDIO_WARNING,"error on Sgets\n");
			Smaskunset(client);
			Sclose(client);
			client= NULL;
			}

		switch(buf[0]) {

		case 'm':	/* m# message... */
			sscanf(buf+1,"%d",&id);
			if(id == 0) printf("\nserver%s\n",buf+2); /* print message from server         */
			else        printf("\nclient%s\n",buf+1); /* print message from another client */
			fputs(PROMPT,stdout);                     /* print prompt                      */
			fflush(stdout);                           /* insure prompt is displayed        */
			break;

		case 'q':	/* server told this client to quit */
			goto enditnow;

		default:
			error(XTDIO_WARNING,"protocol violation, I don't know what to do with <%s%s%s>\n",
			  YELLOW,
			  sprt(buf),
			  GREEN);
			break;
			}
		}

	else if(result < 0) {
		error(XTDIO_WARNING,"error on Stest\n");
		Smaskunset(client);
		Sclose(client);
		client= NULL;
		}

	else {
		fgets(buf,BUFSIZE,stdin);
		srmtrblk(buf);        /* remove trailing white space */

		fputs(PROMPT,stdout); /* print prompt                */
		fflush(stdout);       /* insure prompt is displayed  */

		if(!strcmp(buf,"q")) {
			Sputs("q",client);
			}
		else if(!strcmp(buf,"Q")) {
			Sputs("Q",client);
			goto enditnow;
			}
		else {
			Sprintf(client,"m%s",buf);
			}
		}
	}

/* that'suh that'sall folks! */
enditnow:
if(client) {
	Smaskunset(client);
	Sclose(client);
	client= NULL;
	}

return 0;
}

/* --------------------------------------------------------------------- */
