/* sktsig.c: set up a Socket so that it has a SIGIO interrupt whenever data
 *  appears on the Socket (this is a unix sysv version)
 */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include "xtdio.h"
#include "sockets.h"

/* ------------------------------------------------------------------------
 * Definitions:
 */
#define BUFSIZE	256

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
#ifdef __PROTOTYPE__
int main( int, char **);                         /* sktsig.c */
# ifdef __gnu_linux__
void sigio_handler(int);                         /* sktsig.c */
# else
void sigio_handler(int,int,struct sigcontext *); /* sktsig.c */
# endif
#else
int main();                                      /* sktsig.c */
void sigio_handler();                            /* sktsig.c */
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
char    buf[BUFSIZE];
Socket *srvr         = NULL;
Socket *skt          = NULL;

/* open server */
srvr= Sopen("sktsig","S");
if(!srvr) error(XTDIO_ERROR,"sktsig: cannot open a server <sktsig>\n");
printf("server <sktsig> now available\n");

/* wait for a client */
printf("waiting for a client\n");
skt= Saccept(srvr);
if(!skt) {
	Sclose(srvr);
	error(XTDIO_ERROR,"sktsig: Saccept failed\n");
	}

/* set up SIGIO handler */
signal(SIGIO,sigio_handler);		/* install sigio handler			*/
fcntl(skt->skt,F_SETOWN,getpid());	/* set ownership to self			*/
fcntl(skt->skt,F_SETFL,FASYNC);		/* incoming data generates SIGIO	*/

Sputs("ready to receive",skt);
printf("ready to receive\n");

while(1) {
	pause();						/* wait until a signal arrives		*/
	if(!Sgets(buf,BUFSIZE,skt)) {	/* socket error						*/
		error(XTDIO_WARNING,"sktsig: socket error\n");
		break;
		}
	printf("rcvd buf<%s>\n",sprt(buf));
	Sprintf(skt,"rcvd buf <%s>",buf);
	if(!strcmp(buf,"q")) break;		/* quit on "q"						*/
	}

/* close down Sockets */
Sclose(skt);
Sclose(srvr);

return 0;
}

/* --------------------------------------------------------------------- */

#if defined(__gnu_linux__)
void sigio_handler(int sig)
#elif defined(__PROTOYPE__)
void sigio_handler(
  int                sig, 
  int                code,
  struct sigcontext *sc)  
#else	/* __PROTOTYPE__ */
void sigio_handler(sig,code,sc)
int                sig;
int                code;
struct sigcontext *sc;
#endif	/* __PROTOTYPE__ */
{
signal(SIGIO,sigio_handler);	/* re-install sigio handler */

printf("caught a sigio\n");
}

/* --------------------------------------------------------------------- */
