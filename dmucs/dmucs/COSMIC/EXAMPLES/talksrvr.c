/* talksrvr.c: this program implements the TalkServer.
 *
 *          The server here also acts as a pass-through: all
 *          other clients get told what any client said.
 *
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
#include <ctype.h>
#include "xtdio.h"
#include "sockets.h"

/* ------------------------------------------------------------------------
 * Definitions:
 */
#define BUFSIZE	256
#define SERVER	"TalkServer"
#define PROMPT	"Server: "

/* ------------------------------------------------------------------------
 * Typedefs:
 */
typedef struct Accept_str Accept;

/* ------------------------------------------------------------------------
 * Local Data Structures:
 */
struct Accept_str {
	int     id;
	Socket *skt;
	Accept *nxt;
	Accept *prv;
	};

/* ------------------------------------------------------------------------
 * Global Data:
 */
Accept *ahd= NULL;		/* accept socket list head */
Accept *atl= NULL;		/* accept socket list tail */

/* ------------------------------------------------------------------------
 * Explanation:
 */

/* ------------------------------------------------------------------------
 * Prototypes:
 */
int main( int, char **);          /* talkserver.c */
void RemoveAccpt(Accept *);       /* talkserver.c */
void SendAllbut(Accept *,char *); /* talkserver.c */

/* ------------------------------------------------------------------------
 * Source Code:
 */

/* main: */
int main(
  int    argc,
  char **argv)
{
Accept     *accpt        = NULL;
Accept     *accptnxt     = NULL;
Socket     *srvr         = NULL;
char        buf[BUFSIZE];
int         didwork      = 1;
int         doquit       = 0;
int         result;
static int  id           = 0;

rdcolor();

/* attempt to open server */
srvr= Sopen(SERVER,"S");

/* this probably happens only when the PortMaster isn't up */
if(!srvr) error(XTDIO_ERROR,"unable to open <%s%s%s> server\n",YELLOW,SERVER,GREEN);
Smaskset(srvr);                     /* enter server into mask */
Smaskfdset(fileno(stdin));          /* enter stdin  into mask */

fputs(PROMPT,stdout);               /* print out prompt       */
fflush(stdout);                     /* force stdout out       */

while(!doquit && didwork) {
	result  = Smaskwait();
	didwork = 0;

	if(result > 0) {                /* if error, returns -1   */

		if(Stest(srvr) > 0) {       /* if error, returns -1   */
			Socket *skt;

			didwork = 1;		/* only indicate didwork when something was done */
			/* accept a new Socket */
			skt= Saccept(srvr);
			if(!skt) {
				error(XTDIO_WARNING,"server tested having data but unable to accept!\n");
				break;
				}
			Smaskset(skt);

			/* allocate and doubly-link in new Accept */
			accpt= (Accept *) malloc(sizeof(Accept));
			if(atl) atl->nxt= accpt;
			else    ahd     = accpt;
			accpt->prv= atl;
			accpt->nxt= NULL;
			atl       = accpt;

			/* initialize */
			accpt->skt= skt;
			accpt->id = ++id;
			Smaskset(accpt->skt);
			}

		else {
			/* check out all accepted clients for any work to do */
			for(accpt= ahd, accptnxt= NULL; accpt && !doquit; accpt= accptnxt) {
				accptnxt= accpt->nxt;		/* note that RemoveAccpt may delete this accpt! */
				if(accpt->skt) {
					result  = Stest(accpt->skt);

					if(result > 0) {
						if(Sgets(buf,BUFSIZE,accpt->skt)) {
							char   *b;
							Accept *snd;
							int     fromid;
							didwork = 1;		/* only indicate didwork when something was done */

							/* translate command from client */
							switch(buf[0]) {

							case 'q':	/* client is quitting  */
								RemoveAccpt(accpt);
								break;

							case 'Q':	/* client told server to quit */
								doquit= 1;
								RemoveAccpt(accpt);
								break;

							case 'm':	/* message from client */
								for(b= buf+1; *b && isspace(*b); ++b) ; /* skip "m ..."   */
								printf("\nclient#%d: %s\n",accpt->id,b);
								SendAllbut(accpt,b);  /* send message to all other clients */
								fputs(PROMPT,stdout); /* print out prompt                  */
								fflush(stdout);       /* force stdout out                  */
								break;
								}
							}
						else RemoveAccpt(accpt);
						}
					else if(result < 0) RemoveAccpt(accpt);
					}
				}
			}

		/* ok, didn't do anything with Sockets, so must be stdin */
		if(!didwork) {
			didwork= 1;

			fgets(buf,BUFSIZE,stdin);        /* get string from stdin       */
			srmtrblk(buf);                   /* remove trailing whitespace  */

			if(!strcmp(buf,"QUIT")) {        /* user command server to QUIT */
				break;
				}

			SendAllbut((Accept *) NULL,buf); /* send to ALL accept Sockets  */
			fputs(PROMPT,stdout);            /* print out prompt            */
			fflush(stdout);                  /* force stdout out            */
			}
		}
	else break;
	}

/* close everything down */
while(ahd) RemoveAccpt(ahd);
Sclose(srvr);

return 0;
}

/* --------------------------------------------------------------------- */

/* RemoveAccpt: this function removes given Accept* from linked list */
void RemoveAccpt(Accept *accpt)
{
/* sanity check */
if(!accpt || !accpt->skt) {
	error(XTDIO_SEVERE,"RemoveAccpt told to remove %s!\n",
	  accpt? "accpt with null socket" : "null accpt");
	}

/* tell client to quit */
Sputs("q",accpt->skt);

/* de-link */
if(accpt->prv) accpt->prv->nxt= accpt->nxt;
else           ahd            = accpt->nxt;
if(accpt->nxt) accpt->nxt->prv= accpt->prv;
else           atl            = accpt->prv;

Smaskunset(accpt->skt); /* remove from mask */
Sclose(accpt->skt);     /* close socket     */
free((char *) accpt);   /* delete Accept    */

}

/* --------------------------------------------------------------------- */

/* SendAllbut: this function sends a message to all clients except the
 * one on the argument list
 */
void SendAllbut(Accept *notme,char *msg)
{
Accept *accpt= NULL;

for(accpt= ahd; accpt; accpt= accpt->nxt) if(accpt != notme) {
	Sprintf(accpt->skt,"m%d %s",notme? notme->id : 0,msg);
	}

}

/* --------------------------------------------------------------------- */
