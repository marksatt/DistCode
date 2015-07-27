/* multiskt.c: this program can open up multiple accepts on one server */
#include <stdio.h>
#include "xtdio.h"
#include "sockets.h"

/* ----------------------------------------------------------------------
 * Definitions:
 */
#define BUFSIZE 128

/* ----------------------------------------------------------------------
 * Typedefs:
 */
typedef struct sktlist_str SktList;


/* ----------------------------------------------------------------------
 * Data Structures:
 */
struct sktlist_str {
    int      id;
    Socket  *skt;
    SktList *nxt,*prv;
    };

/* ----------------------------------------------------------------------
 * Global Data:
 */
Socket  *srvr = NULL;
SktList *shd  = NULL;
SktList *stl  = NULL;

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
    char     mesg[BUFSIZE];
    Socket  *skt        = NULL;
    SktList *sktlist    = NULL;
    SktList *sktlistnxt = NULL;

    rdcolor();

    /* open server */
    srvr= Sopen("multiskt","s");
    if(!srvr) {
	/* if "multiskt" did not open, this code (rudely) attempts to remove
	 * any servers by that name and then re-open
	 */
	Srmsrvr("multiskt");
	srvr= Sopen("multiskt","s");
	if(!srvr) error(XTDIO_ERROR,"unable to open <multiskt> as server\n");
    }

    /* ok, read stuff into skt messages (so long as there are clients) */
    Smaskset(srvr);
    do {

	/* block until a Socket needs service */
	Smaskwait();

	/* if client awaiting connection, go ahead and connect to it */
	if(Stest(srvr)) {
	    skt= Saccept(srvr);
	    if(!skt) {
		error(XTDIO_WARNING,"unable to accept\n");
		continue;
            }

	    /* allocate a doubly-linked SktList */
	    sktlist= (SktList *) malloc(sizeof(SktList));
	    if(stl) stl->nxt= sktlist;
	    else    shd     = sktlist;
	    sktlist->prv= stl;
	    sktlist->nxt= NULL;
	    stl         = sktlist;

	    /* initialize the SktList */
	    sktlist->id = stl->prv? (stl->prv->id + 1) : 1;
	    sktlist->skt= skt;

	    /* set up Smaskwait mask -- all Sockets entered into this mask will
	     * be waited upon as a group
	     */
	    Smaskset(sktlist->skt);
        }

	else {    /* process communication from a client */
	    sktlistnxt= NULL;

	    /* scan all clients loop */
	    for(sktlist= shd; sktlist; sktlist= sktlistnxt) {
		sktlistnxt= sktlist->nxt;

		/* is something waiting on this Socket? */
		if(Stest(sktlist->skt)) {

		    /* something waiting on this Socket, get it */
		    Sgets(mesg,BUFSIZE,sktlist->skt);

		    /* report on message */
		    printf("skt%d: mesg<%s>\n",sktlist->id,mesg);

		    /* send message back */
		    Sprintf(sktlist->skt,"received <%s>\n",sprt(mesg));

		    /* sktlist is quitting on receipt of q */
		    if(!strcmp(mesg,"q")) {

			/* close down Socket */
			Sclose(sktlist->skt);
			Smaskunset(sktlist->skt);

			/* remove sktlist entry from linked list */
			if(sktlist->prv) sktlist->prv->nxt= sktlist->nxt;
			else             shd              = sktlist->nxt;
			if(sktlist->nxt) sktlist->nxt->prv= sktlist->prv;
			else             stl              = sktlist->prv;

			/* free up former sktlist memory */
			free((char *) sktlist);
                    }    /* quit                             */
                }        /* get a message from a client      */
            }            /* sktlist loop                     */
        }                /* client communications processing */
    } while(shd);

    /* close down the server */
    Sclose(srvr);
    return 0;
}

/* ---------------------------------------------------------------------- */
