/* oobrecv.c: tests out receipt of OOB data
 *  Run oobrecv first, then in a second window, run oobsend
 *  Author: Dr. Charles E. Campbell, Jr.
 */
#define _BSD_SIGNALS
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include "sockets.h"

/* ------------------------------------------------------------------------
 * Definitions:
 */
#define BUFSIZE 	256
#define SLEEPTIME	1

/* ------------------------------------------------------------------------
 * Typedefs:
 */

/* ------------------------------------------------------------------------
 * Local Data Structures:
 */

/* ------------------------------------------------------------------------
 * Global Data:
 */
Socket *skt=NULL;

/* ------------------------------------------------------------------------
 * Explanation:
 */

/* ------------------------------------------------------------------------
 * Prototypes:
 */
#ifdef __PROTOTYPE__
int main( int, char **);                               /* oobrecv.c       */
int sigurg_handler(int,int,struct sigcontext *);       /* oobrecv.c       */
#else
int main();                                            /* oobrecv.c       */
int sigurg_handler();                                  /* oobrecv.c       */
#endif

/* ------------------------------------------------------------------------
 * Source Code:
 */

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
char  buf[BUFSIZE];
pid_t rcvpid;
int   sleeptime    = SLEEPTIME;

rdcolor();

if(argc > 1) sscanf(argv[1],"%d",&sleeptime);

/* install SIGURG handler */
signal(SIGURG,(void *) sigurg_handler);

/* poll once per second to open client */
for(; !skt; sleep(1)) skt= Sopen("oob","c");
printf("client to <oob> opened\n");

/* send pid */
rcvpid= getpid();
Sprintf(skt,"%d",rcvpid);

Swait(skt);	/* wait until something shows up */
printf("something has arrived, now sleeping\n");

printf("sleeping for %d seconds\n",sleeptime);
sleep(sleeptime);

Sgets(buf,BUFSIZE,skt);
printf("normal rcvd: buf<%s>\n",sprt(buf));

Sclose(skt);
printf("closed client to <oob>\n");

return 0;
}

/* --------------------------------------------------------------------- */

/* sigurg_handler:  handles SIGURG signals */
#ifdef __PROTOTYPE__
int sigurg_handler(
  int sig,
  int code,
  struct sigcontext *sc)
#else	/* __PROTOTYPE__ */
int sigurg_handler(sig,code,sc)
int sig,code;
struct sigcontext *sc;
#endif	/* __PROTOTYPE__ */
{
char buf[BUFSIZE];
int  cnt;

/* re-install SIGURG handler */
signal(SIGURG,(void *) sigurg_handler);

cnt= recv(skt->skt,buf,BUFSIZE,MSG_OOB);
printf("oob rcvd: cnt=%d buf<%c>\n",cnt,buf[0]);

Sputs("oob recvd",skt);
printf("sent normal<oob recvd>\n");
}

/* --------------------------------------------------------------------- */
