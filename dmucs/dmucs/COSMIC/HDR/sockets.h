/* sockets.h: this file is the header file for the Simple Sockets Library
 *  Version:	3
 *	Authors:	Charles E. Campbell, Ph.D.
 *  			Terry McRoberts
 *  Date:		Apr 2, 2004
 */
#ifndef SOCKETS_H
#define SOCKETS_H

#ifdef __cplusplus
  extern "C" {
#endif

/* --------------------------------------------------------------------------
 * Standard Include Section
 */
/* for amigas */
#ifdef MCH_AMIGA
  typedef unsigned short u_short;
  typedef unsigned long  fd_set;
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

  /* for ibm's AIX o/s */
#ifdef _AIX
# define SSLNOSETSOCKOPT
# define _NO_BITFIELDS
# include <netinet/in.h>
# include <netinet/ip.h>
struct ip_firstfour {   /* copied from <netinet/ip.h>, who knows why it isn't getting defined!!! */
	u_char  ip_fvhl;
	u_char  ip_ftos;
	u_short ip_flen;
# define ip_fwin ip_flen
	};
# include <netinet/tcp.h>
# include <sys/select.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <stdio.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

#ifdef apollo
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
# include <stdio.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

/* for ibm's AS/400  */
#ifdef AS400
# define STRTSRVR_PGM "*libl/startau17"
/* # define SSLNOSETSOCKOPT  */
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
# include <stdio.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

/* for Apple's MAC OS-X - thanks to Paul Bourke */
#ifdef __APPLE__
# include <sys/types.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <stdarg.h>
# include <netdb.h>
# include <unistd.h>
# include <stdio.h>
# include <stdarg.h>
# include <stdlib.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif 

#ifdef __CYGWIN__
# include <sys/types.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <stdarg.h>
# include <netdb.h>
# include <unistd.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

/* for Linux */
#ifdef __linux
# include <sys/types.h>
# include <sys/select.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <stdarg.h>
# include <netdb.h>
# include <unistd.h>
# include <stdio.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
# ifndef __USE_BSD
  typedef unsigned short u_short;
#endif
#endif

#ifdef MSC
# define SSLNOPEEK
# define SSLNOSETSOCKOPT
# define SSLNEEDTOSHAREPM
# define SSLSKTZERO
# define Sscanf          sktscanf
# define Sprintf         sktprintf
# define Stimeoutwait    sktwait
# define close(skt) sock_close(skt)
# include <stdarg.h>
  typedef unsigned short u_short;
# ifdef IRL_PC
#  include <c:\c600\iptcp\include\sys\types.h>
#  include <c:\c600\iptcp\include\sys\socket.h>
#  include <c:\c600\iptcp\include\netdb.h>
#  include <c:\c600\iptcp\include\netinet\in.h>
#  include <c:\c600\iptcp\include\sys\uio.h>
#  include <c:\c600\iptcp\include\sys\time.h>
#  include <c:\c600\iptcp\include\sys\errno.h>
#  ifdef SSLNEEDTIME
#   include "c:\c600\iptcp\include\sys\time.h"
#  endif
# endif
#endif

/* for Microsoft Developer Studio */
#ifdef _MSC_VER
# define close(s)        closesocket(s)
# include <stdio.h>
# include <winsock2.h>  /* corresponds to version 2.2.x of the WinSock API */
# include <wtypes.h>
  typedef unsigned short u_short;
# ifdef SSLNEEDTIME
#  include <time.h>
# endif
#endif

/* for Watcomm C++ on OS/2        */
#ifdef os2
# define STRTSRVR_PGM "startsrv.cmd"
# include <w:/os2tk45/h/types.h>
/* # include <w:/os2tk45/h/utils.h> */
# include <w:/os2tk45/h/netdb.h>
# include <w:/os2tk45/h/netinet/in.h>
# include <w:/os2tk45/h/netinet/tcp.h>
# include <w:/os2tk45/h/arpa/inet.h>
# include <w:/os2tk45/h/sys/socket.h>
# include <w:/os2tk45/h/sys/select.h>
/* order is important  */
# ifndef TCPV40HDRS
#  include <w:/os2tk45/h/sys/itypes.h>
#  include <w:/os2tk45/h/unistd.h>
#  define _lswap(x)   ((x<<24)|(x>>24)|((x&0xff00)<<8)|((x&0xff0000)>>8))
#  define _bswap(x)   (((unsigned)x<<8)|((unsigned)x>>8))
# endif
# ifdef SSLNEEDTIME
#  include <w:/os2tk45/h/sys/time.h>
# endif
#endif

/* for OSF */
#ifdef __osf__
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

/* for FreeBSD */
#ifdef __FreeBSD__
# include <stdarg.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
# include <stdio.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

/* for SCO Unix's cc compiler */
#if defined(M_I386) && defined(M_SYSV)
# include <sys/types.h>
# include <sys/socket.h>
# include <sys/netinet/in.h>
# include <sys/netinet/tcp.h>
# include <netdb.h>
# include <stdlib.h>
# include <stdarg.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

#ifdef sgi
# include <string.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
# include <sys/uio.h>
# include <unistd.h>
# include <bstring.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
   int select(int,fd_set*,fd_set*,fd_set*,struct timeval *);
# endif
#endif

#ifdef sun
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
# include <stdio.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

#ifdef  ultrix
# include <string.h>
# include <sys/types.h>
# include </usr/sys/h/socket.h>
# include <netinet/in.h>
# include <netinet/tcp.h>
# include <netdb.h>
# include <stdio.h>
# ifdef SSLNEEDTIME
#  include <sys/time.h>
# endif
#endif

#ifdef vms
# define Sscanf  Sktscanf
# define Sprintf Sktprintf
# include <errno.h>
# include <types.h>
# include <socket.h>
# include <in.h>
# include <tcp.h>
# include <netdb.h>
# include <inet.h>
# ifdef OldVMS
#  include <ucx$inetdef.h>
# else
#  include <netinet/in.h>
#  include <netinet/tcp.h>
# endif
# ifdef SSLNEEDTIME
#  include time
# endif
typedef int fd_set; /* at least on my current version of vms I need this, sheesh */
#endif

/* win32 (it works for borland c) */
#ifdef __WIN32__
# define SSLSKTZERO
# define Sscanf          sktscanf
# define Sprintf         sktprintf
# define Stimeoutwait    sktwait
# define close(s)        closesocket(s)
# include <stdio.h>
# include <winsock.h>
# include <wtypes.h>
  typedef unsigned short u_short;
# ifdef SSLNEEDTIME
#  include <time.h>
# endif
#endif

/* --------------------------------------------------------------------------
 * Definitions Section
 */
#define PORTMASTER	((u_short) 1750)		/* standard PortMaster port -- IANA registered */

#ifdef vms
/* typedef unsigned long fd_set; */
# define FD_SET(n,p)	(*p|= (1<<n))
# define FD_CLR(n,p)	(*p&= ~(1<<n))
# define FD_ISSET(n,p)	(*p & (1<<n))
# define FD_ZERO(p)		(*p= 0L)
# define TCP_READ( MSG_SOCK, LINE, NCHARS ) netread( MSG_SOCK, LINE, NCHARS )
# define TCP_CLOSE( MSG_SOCK )              netclose( MSG_SOCK )
# define TCP_WRITE( SOCK, LINE, NCHARS )    netwrite( SOCK, LINE, NCHARS )
#else
# define TCP_READ( MSG_SOCK, LINE, NCHARS ) recv( MSG_SOCK, LINE, NCHARS,0 )
# define TCP_CLOSE( MSG_SOCK )              close( MSG_SOCK )
# define TCP_WRITE( SOCK, LINE, NCHARS )    send( SOCK, LINE, NCHARS,0 )
#endif

#ifdef MSC
# define FD_SET(n,p)     ((p)->fds_bits[0] |= (1<<n))
# define FD_CLR(n,p)     ((p)->fds_bits[0] &= ~(1<<n))
# define FD_ISSET(n,p)   ((p)->fds_bits[0] & (1<<n))
# define FD_ZERO(p)      ((p)->fds_bits[0] = 0)
#endif

/* end of types ============================================================ */

/* PortMaster messages */
#define PM_SERVER     ((u_short) 1)
#define PM_CLIENT     ((u_short) 2)
#define PM_CLOSE      ((u_short) 3)
#define PM_RESEND     ((u_short) 4)
#define PM_QUIT       ((u_short) 5)
#define PM_SORRY      ((u_short) 6)
#define PM_OK         ((u_short) 7)
#define PM_ACCEPT     ((u_short) 8)
#define PM_TABLE      ((u_short) 9)
#define PM_RMSERVER   ((u_short) 10)
#define PM_FWINIT     ((u_short) 11)
#define PM_SHARE      ((u_short) 12)
#define PM_OKSHARE    ((u_short) 13)
#define PM_CLIENTWAIT ((u_short) 14)

#define PM_BIGBUF			1024
#define PM_MAXTRY			20	/* max number of resends to PortMaster	*/
#define PM_MAXREQUESTS		10	/* max pending connects					*/

/* --------------------------------------------------------------------------
 * Typedef Section
 */
typedef int                PrtMstrEvent;
typedef struct Skt_str     Socket;
typedef struct Smask_str   Smask;
typedef u_short            SKTEVENT;
#if !defined(MCH_AMIGA) && !defined(__WIN32__)
  typedef struct sockaddr_in sinpt;
#endif

/* --------------------------------------------------------------------------
 * Data Structures:
 */
struct Skt_str {
	int      skt;		/* skt handle						*/
	SKTEVENT port;		/* associated port					*/
	int      type;		/* PM_SERVER, PM_CLIENT, PM_ACCEPT	*/
	char    *sktname;	/* name of socket					*/
	char    *hostname;	/* name of host						*/
	};

struct Smask_str {
	fd_set   mask;
	unsigned waitall;
	};

#ifdef __cplusplus
	}
#endif

/* ---------------------------------------------------------------------
 * Additional XTDIO Includes:
 */
#include "xtdio.h"

/* ---------------------------------------------------------------------
 * Prototypes:
 */
#ifdef __cplusplus
  extern "C" {
#endif

#ifdef __PROTOTYPE__
/* XTDIO: SKTS routines */
Socket *Saccept(Socket *);                                                                                          /* Saccept.c       */
void Sclose(Socket *);                                                                                              /* Sclose.c        */
char *Sgets( char *,  int,  Socket *);                                                                              /* Sgets.c         */
void Sinit(void);                                                                                                   /* Sinit.c         */
int Smaskwait(void);                                                                                                /* Smaskwait.c     */
void Smaskset(Socket *);                                                                                            /* Smaskwait.c     */
void Smaskfdset(int);                                                                                               /* Smaskwait.c     */
void Smasktime(long,long);                                                                                          /* Smaskwait.c     */
int Smasktest(void);                                                                                                /* Smaskwait.c     */
void Smaskunset(Socket *);                                                                                          /* Smaskwait.c     */
void Smaskunfdset(int);                                                                                             /* Smaskwait.c     */
Smask Smaskget(void);                                                                                               /* Smaskwait.c     */
void Smaskuse(Smask);                                                                                               /* Smaskwait.c     */
void Smaskpush(void);                                                                                               /* Smaskwait.c     */
void Smaskpop(void);                                                                                                /* Smaskwait.c     */
int Smaskisset(Socket *);                                                                                           /* Smaskwait.c     */
int Smaskfdisset(int);                                                                                              /* Smaskwait.c     */
Socket *makeSocket( char *, char *, int);                                                                           /* Smkskt.c        */
void freeSocket(Socket *);                                                                                          /* Smkskt.c        */
Socket *Sopen( char *, char *);                                                                                     /* Sopen.c         */
Socket *Sopen_clientport( char *, char *, u_short);                                                                 /* Sopen.c         */
Socket *Sopenv( char *,  char *,  char *);                                                                          /* Sopenv.c        */
int Speek( Socket *,  void *,  int);                                                                                /* Speek.c         */
unsigned long Speeraddr(Socket *);                                                                                  /* Speeraddr.c     */
char *Speername(Socket *);                                                                                          /* Speername.c     */
int Sprintf( Socket *, char *, ...);                                                                                /* Sprintf.c       */
char *Sprtskt(Socket *);                                                                                            /* Sprtskt.c       */
void Sputs( char *, Socket *);                                                                                      /* Sputs.c         */
int Sread( Socket *,  void *,  int);                                                                                /* Sread.c         */
int Sreadbytes( Socket *,  void *,  int);                                                                           /* Sreadbytes.c    */
SKTEVENT Srmsrvr(char *);                                                                                           /* Srmsrvr.c       */
int Sscanf(Socket *,char *,...);                                                                                    /* Sscanf.c        */
int Stest(Socket *);                                                                                                /* Stest.c         */
int Stimeoutwait(Socket *,long,long);                                                                               /* Stimeoutwait.c  */
int Svprintf( Socket *, char *, ...);                                                                            /* Svprintf.c      */
int Swait(Socket *);                                                                                                /* Swait.c         */
int Swrite( Socket *,  void *,  int);                                                                               /* Swrite.c        */
#else
/* XTDIO: SKTS routines */
extern Socket *Saccept();                                                                                           /* Saccept.c       */
extern void Sclose();                                                                                               /* Sclose.c        */
extern char *Sgets();                                                                                               /* Sgets.c         */
extern void Sinit();                                                                                                /* Sinit.c         */
extern int Smaskwait();                                                                                             /* Smaskwait.c     */
extern void Smaskset();                                                                                             /* Smaskwait.c     */
extern void Smaskfdset();                                                                                           /* Smaskwait.c     */
extern void Smasktime();                                                                                            /* Smaskwait.c     */
extern int Smasktest();                                                                                             /* Smaskwait.c     */
extern void Smaskunset();                                                                                           /* Smaskwait.c     */
extern void Smaskunfdset();                                                                                         /* Smaskwait.c     */
extern Smask Smaskget();                                                                                            /* Smaskwait.c     */
extern void Smaskuse();                                                                                             /* Smaskwait.c     */
extern void Smaskpush();                                                                                            /* Smaskwait.c     */
extern void Smaskpop();                                                                                             /* Smaskwait.c     */
extern int Smaskisset();                                                                                            /* Smaskwait.c     */
extern int Smaskfdisset();                                                                                          /* Smaskwait.c     */
extern Socket *makeSocket();                                                                                        /* Smkskt.c        */
extern void freeSocket();                                                                                           /* Smkskt.c        */
extern Socket *Sopen();                                                                                             /* Sopen.c         */
extern Socket *Sopen_clientport();                                                                                  /* Sopen.c         */
extern Socket *Sopenv();                                                                                            /* Sopenv.c        */
extern int Speek();                                                                                                 /* Speek.c         */
extern unsigned long Speeraddr();                                                                                   /* Speeraddr.c     */
extern char *Speername();                                                                                           /* Speername.c     */
extern int Sprintf();                                                                                               /* Sprintf.c       */
extern char *Sprtskt();                                                                                             /* Sprtskt.c       */
extern void Sputs();                                                                                                /* Sputs.c         */
extern int Sread();                                                                                                 /* Sread.c         */
extern int Sreadbytes();                                                                                            /* Sreadbytes.c    */
extern SKTEVENT Srmsrvr();                                                                                          /* Srmsrvr.c       */
extern int Sscanf();                                                                                                /* Sscanf.c        */
extern int Stest();                                                                                                 /* Stest.c         */
extern int Stimeoutwait();                                                                                          /* Stimeoutwait.c  */
extern int Svprintf();                                                                                              /* Svprintf.c      */
extern int Swait();                                                                                                 /* Swait.c         */
extern int Swrite();                                                                                                /* Swrite.c        */
#endif

# ifdef __cplusplus
	}
# endif
#endif /* #ifndef SOCKETS_H */
