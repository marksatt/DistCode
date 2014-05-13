/* sktfork.c: this program illustrates use of fork under Unix SysV with a
 *            simple server.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "xtdio.h"
#include "sockets.h"

/* ------------------------------------------------------------------------
 * Definitions:
 */
#define BUFSIZE	256

/* ------------------------------------------------------------------------
 * Globals:
 */
int childstat= 0;

/* ------------------------------------------------------------------------
 * Prototypes:
 */
int main( int, char **);      /* sktfork.c       */
void sigchld_handler(void);   /* sktfork.c       */

/* ------------------------------------------------------------------------
 * Source Code:
 */

/* main: */
int main(
  int    argc,
  char **argv)
{
    int     sktcnt = 0;
    int     istat;
    int     ret;
    Socket *srvr   = NULL;
    Socket *nskt   = NULL;

    if (argc <= 1) {
	fprintf(stderr,"***usage*** sktfork srvrname\n");
	exit(1);
    }

    srvr = Sopen(argv[1],"s");
    if (!srvr) {
	fprintf(stderr, "***error*** unable to open <%s> server\n", argv[1]);
	exit(1);
    }

    /* install a SIGCHLD handler */
    sigset(SIGCHLD, sigchld_handler);

    while(1) {
	printf("Wait for new socket to be accepted\n");
	nskt = Saccept(srvr);
	if (!nskt) {	/* presumably I just got a SIGCHLD */
	    printf("WEXITSTATUS(childstat=%d)=%d\n", childstat,
		   WEXITSTATUS(childstat));
	    if (WEXITSTATUS(childstat) == 1) break;
	    continue;
	} else {
	    ++sktcnt;
	    printf("normal new skt: sktcnt=%d\n", sktcnt);
	}
	ret = fork();

	if (ret == 0) {
	    /* child process */
	    char    buf[BUFSIZE];
	    Socket *skt;

	    printf("child() skt<%s>", Sprtskt(nskt));
	    skt = nskt;
	    nskt = NULL;

	    if (!Sgets(buf, BUFSIZE, skt)) {
		fprintf(stderr, "***error*** unable to Sgets\n");
		_exit(0);
	    }
	    Sputs("goodbye!", skt);
	    printf("got <%s>, goodbye!\n", buf);
	    Sclose(skt);
	    skt= NULL;

	    if (!strcmp(buf, "Q")) {
		printf("return: child 1\n");
		_exit(1);
	    }
	    printf("return: child 0\n");
	    _exit(0);
	} else if (ret == -1) {
	    fprintf(stderr, "***error*** unable to fork\n");
	    if (nskt) Sclose(nskt);
	    if (srvr) Sclose(srvr);
	    srvr = nskt = NULL;
	    break;
	}
    }

    printf("closing down server: sktcnt=%d\n", sktcnt);
    if (srvr) Sclose(srvr);
    srvr = NULL;

    return 0;
}

/* --------------------------------------------------------------------- */

/* sigchld_handler: this function */
void sigchld_handler()
{
    wait(&childstat);
}

/* --------------------------------------------------------------------- */
