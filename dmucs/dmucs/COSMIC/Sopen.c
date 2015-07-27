/* Sopen.c: these functions are responsible for opening Sockets
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Sopen.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 *
 *  Routines Include: <Sopen.c>
 *    Socket *Sopen( char *skthost, char *mode)
 *    Socket *Sopen_clientport( char *hostname, char *sktname, u_short port)
 *    static Socket *Sopen_server(char *sktname)
 *    static Socket *Sopen_client( char *hostname, char *sktname, int blocking)
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "sockets.h"

/* ----------------------------------------------------------------------
 * Local Definitions:
 */
#define BUFSIZE		128
#define TIMEOUT 	 20L
#define DFLT_PORT	  0

/* ---------------------------------------------------------------------
 * Local Data:
 */
static unsigned long sslport= DFLT_PORT;

/* ----------------------------------------------------------------------
 * Local Prototypes:
 */
#ifdef __PROTOTYPE__
static Socket *Sopen_server(char *);               /* Sopen.c */
static Socket *Sopen_client( char *, char *, int); /* Sopen.c */
#else
static Socket *Sopen_server();                     /* Sopen.c */
static Socket *Sopen_client();                     /* Sopen.c */
#endif

/* ---------------------------------------------------------------------- */

/* Sopen:
 * prochost: SocketName@Hostname
 * mode    : selects type of socket desired
 *               s=server - one must use Saccept(skt) to receive connects
 *               S=server - like s, but will Srmsrvr() if first attempt fails
 *               c=client - open a client to the specified server (nonblocking)
 *               b=client - open a client, but block until server opens
 */
#ifdef __PROTOTYPE__
Socket *Sopen(
  char *skthost,
  char *mode)
#else
Socket *Sopen(
  skthost,
  mode)
char *skthost;
char *mode;
#endif
{
    char       *at                 = NULL;
    char       *hostname           = NULL;
    int         blocking           = 0;
    Socket     *ret                = NULL;
    static char localhost[BUFSIZE] = "";
    static char sktname[BUFSIZE];

    static int first=1;


    /* do any o/s-orientated initialization that is required */
    if(first) {
        first= 0;
        Sinit();
    }

    /* by default, sslport is always DFLT_PORT *unless* overridden in the mode */
    sslport= DFLT_PORT;
    if(isdigit(mode[1])) {
        sscanf(mode+1,"%lu",&sslport);
    }

    else if(skthost[0] == '$') { /* 2.10e mod: a server named "$####" will use port #### */
        char *s;
        /* $####... will also be accepted as a sslport */
        for(s= skthost + 1; *s && isdigit(*s); ++s) ;
        if(!*s || *s == '@') {
            sscanf(skthost+1,"%lu",&sslport);
        }
    }

    /* call mode-selected server/client socket opening routine */
    switch(*mode) {

    case 's':	/* server */
    case 'S':	/* use Srmsrvr when Sopen_server itself fails */
#ifdef SSLNEEDTOSHAREPM
        /* SSLNEEDTOSHAREPM: this machine needs to share another's PortMaster;
         *  ie. the o/s doesn't support multiple tasks, so a PortMaster cannot
         *      run on this machine concurrently with a user process.  A typical
         *      example of such an "o/s" is pure MSDOS.
         */
        if(!getenv("PMSHARE")) {
            error(XTDIO_WARNING,"this machine must share a PortMaster (set PMSHARE, please)\n");
            ret= NULL;
            return ret;
        }
#endif

        /* sanity check -- must have a server name or a specified port number */
        if((!skthost || !skthost[0]) && sslport == DFLT_PORT) {
            error(XTDIO_WARNING,"with no server name one must specify the port number\n");
            ret= NULL;
            return ret;
        }

        ret= Sopen_server(skthost);
        if(!ret && *mode == 'S') {
            Srmsrvr(skthost);
            ret= Sopen_server(skthost);
        }

        sslport= DFLT_PORT;	/* paranoia time -- insure that sslport will default to DFLT_PORT */
        return ret;

    case 'b':   /* client, block until server becomes available */
        blocking= 1;
        /* deliberate fall through */

    case 'c':	/* client */
        /* parse sktname into sktname and hostname strings */
        at = (skthost) ? strchr(skthost, '@') : NULL;
        if (!at) {	/* fill in missing hostname with current host */
            if(!localhost[0]) { /* get local host name */
                gethostname(localhost,BUFSIZE);
            }
            hostname= localhost;
            strcpy(sktname,skthost? skthost : "");
        }
        else {	/* use hostname specified in skthost */
            strcpy(sktname,skthost);
            sktname[(int) (at - skthost)]= '\0';
            hostname                     = sktname + ((int) (at - skthost)) + 1;
        }

        /* sanity check -- must have a server name or a specified port number */
        if((!skthost || !skthost[0]) && sslport == DFLT_PORT) {
            error(XTDIO_WARNING,"with no server name one must specify the port number\n");
            ret= NULL;
            return ret;
        }

        /* note: Sopen_client will free up hostname/sktname memory if unsuccessful
         * open occurs
         */
        ret    = Sopen_client(hostname,sktname,blocking);
        sslport= DFLT_PORT;	/* paranoia time -- insure that sslport will default to DFLT_PORT */
        return ret;

    default:
        sslport= DFLT_PORT;	/* paranoia time -- insure that sslport will default to DFLT_PORT */
        return (Socket *) NULL;
    }
}

/* ---------------------------------------------------------------------- */

/* Sopen_server: this function opens a socket handle for the SSL socket
 *   function library
 */
#ifdef __PROTOTYPE__
static Socket *Sopen_server(char *sktname)
#else
static Socket *Sopen_server(sktname)
char *sktname;
#endif
{
  char              *at     = NULL;
  char              *pmshare= NULL;
#if defined(_AIX)
  size_t          length;
#elif defined(__gnu_linux__)
  socklen_t       length;
#else
  int             length;
#endif
  int                resend = 0;
  int                status = 1;
  Socket            *skt    = NULL;
  Socket            *pmskt  = NULL;
  SKTEVENT           mesg;
  SKTEVENT           port;
  struct sockaddr_in sin;	/* InterNet socket address structure */
  static char        hostname[BUFSIZE];


  /* check for no "@" in hostname */
  at= sktname? strchr(sktname,'@') : NULL;
  if(at) *at= '\0';

  /* determine if this Sopen is to share a PortMaster, and if so, what
   * host it resides at
   */
  pmshare= getenv("PMSHARE");

  /* make a Socket */
  if(gethostname(hostname,BUFSIZE) < 0) {
    return  (Socket *) NULL;
  }

  skt=  makeSocket(pmshare? pmshare : hostname,sktname? sktname : "",PM_SERVER);
  if(!skt)  {
    return  (Socket *) NULL;
  }

  /* create a socket */
  if((skt->skt=  socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
    freeSocket(skt);
    return (Socket *) NULL;
  }

#ifndef SSLNOSETSOCKOPT
  if(sslport) {	/* allow fixed-port servers to be brought back up quickly */
    status= 1;
    if(setsockopt(skt->skt,SOL_SOCKET,SO_REUSEADDR,(char *) &status,sizeof(status)) < 0) {
    }
  }
#endif	/* SSLNOSETSOCKOPT */

  /*	initialize socket data structure
   *  get port specified by sslport, defaults to any
   *  memset: added by Steve Richards for AS/400
   */
  memset(&sin,0x00,sizeof(struct sockaddr_in));
  sin.sin_family      = AF_INET;
  sin.sin_addr.s_addr = htonl(INADDR_ANY);
  sin.sin_port        = htons((u_short) sslport);

  /*	bind socket data structure to this socket     */
  if(bind(skt->skt,(struct sockaddr *) &sin,sizeof(sin))) {
    freeSocket(skt);
    return (Socket *) NULL;
  }

  /* getsockname fills in the socket structure with information,
   * such as the port number assigned to this socket
   */
  length = sizeof(sin);
  if(getsockname(skt->skt,(struct sockaddr *) &sin,(socklen_t*)&length)) {
    freeSocket(skt);
    return (Socket *) NULL;
  }
  skt->port= ntohs(sin.sin_port);

  /* --- PortMaster Interface --- */
  if(sktname && sktname[0]) {	/* skip if null or empty sktname specified */

    /* Inform PortMaster of socket */
    pmskt= Sopen_clientport(pmshare? pmshare : hostname,"PMClient",PORTMASTER);
    if(!pmskt) {
      shutdown(skt->skt,2);
      close(skt->skt);
      freeSocket(skt);
      return (Socket *) NULL;
    }

    /* keep trying until PortMaster sends PM_OK */
    do {

      /* write a PM_SERVER request to the host's PortMaster */
      if(pmshare) {
        mesg= htons(PM_SHARE);
      }
      else {
        mesg= htons(PM_SERVER);
      }
      Swrite(pmskt,(char *) &mesg,sizeof(mesg));

      /* insure that the PortMaster is talking for up to TIMEOUT seconds */
      if(Stimeoutwait(pmskt,TIMEOUT,0L) < 0) {
        shutdown(pmskt->skt,2);
        close(pmskt->skt);
        freeSocket(pmskt);
        shutdown(skt->skt,2);
        close(skt->skt);
        freeSocket(skt);
        return (Socket *) NULL;
      }

      /* ok, read PortMaster's response */
      if(Sreadbytes(pmskt,(char *) &mesg,sizeof(mesg)) <= 0) {
        perror("");
        shutdown(skt->skt,2);
        close(skt->skt);
        freeSocket(skt);
        return (Socket *) NULL;
      }
      mesg= ntohs(mesg); /* convert to host form */

      /* only try to open a PM_SERVER protocol PM_MAXTRY times -or-
       * if PortMaster says "PM_SORRY" (probably due to firewall violation)
       */
      if(++resend >= PM_MAXTRY || mesg == PM_SORRY) {
        shutdown(pmskt->skt,2);
        close(pmskt->skt);
        freeSocket(pmskt);
        shutdown(skt->skt,2);
#ifdef SSLSKTZERO
        close(skt->skt); skt->skt= 0;
#else
        close(skt->skt); skt->skt= (int) NULL;
#endif
        freeSocket(skt);
        return (Socket *) NULL;
      }
    } while(mesg != PM_OK);

    if(pmshare) {
      /* send PortMaster the hostname for shared PortMaster mode */
      Sputs(hostname,pmskt);
    }

    /* send PortMaster the sktname and port id */
    Sputs(sktname,pmskt);

    port= sin.sin_port;	/* network form of port already */
    Swrite(pmskt,(char *) &port,sizeof(port));

    /* find out if PortMaster received ok */
    if(Stimeoutwait(pmskt,TIMEOUT,0L) < 0) {
      shutdown(pmskt->skt,2);
      close(pmskt->skt);
      freeSocket(pmskt);
      shutdown(skt->skt,2);
      close(skt->skt);
      freeSocket(skt);
      return (Socket *) NULL;
    }
    Sreadbytes(pmskt,(char *) &mesg,sizeof(mesg));

    mesg= ntohs(mesg);
    if(mesg != PM_OK) {
      shutdown(pmskt->skt,2);
      close(pmskt->skt);
      freeSocket(pmskt);
      shutdown(skt->skt,2);
      close(skt->skt);
      freeSocket(skt);
      return (Socket *) NULL;
    }
  }

  /* --- Return to Server Initialization --- */

  /* prepare socket queue for (up to) PM_MAXREQUESTS connection
   * requests
   */
  listen(skt->skt,PM_MAXREQUESTS);

#ifndef SSLNOSETSOCKOPT
  /* turn off TCP's buffering algorithm so small packets don't get
   * needlessly delayed
   */
  if(setsockopt(skt->skt,IPPROTO_TCP,TCP_NODELAY,(char *) &status,sizeof(status)) < 0) {
  }
#endif	/* SSLNOSETSOCKOPT */

  /* let PortMaster know that the new server is up and running
   * by sending it a final PM_OK.
   * Then, close down the socket to the PortMaster.
   */
  if(sktname && sktname[0]) {	/* skip if null or empty sktname specified */
    mesg= htons(PM_OK);
    Swrite(pmskt,(char *) &mesg,sizeof(mesg));
    shutdown(pmskt->skt,2);
    close(pmskt->skt);
#ifdef SSLSKTZERO
    pmskt->skt= 0;
#else
    pmskt->skt= (int) NULL;
#endif
  }

  /* return server socket */
  return skt;
}

/* ---------------------------------------------------------------------- */

/* Sopen_client: */
#ifdef __PROTOTYPE__
static Socket *Sopen_client(
  char *hostname,
  char *sktname,
  int   blocking)
#else
static Socket *Sopen_client(
  hostname,
  sktname,
  blocking)
char *hostname;
char *sktname;
int   blocking;
#endif
{
  char      *pmshare = NULL;		/* PortMaster hostname					*/
  char       skthost[BUFSIZE];	/* client's hostname					*/
  int        resend  = 0;                                  
  Socket    *skt     = NULL;                               
  SKTEVENT   port;
  SKTEVENT   mesg;


  skthost[0]= '\0';

  /* --- PortMaster Interface --- */
  if(sslport == (unsigned long) DFLT_PORT) {

    /* determine if this Sopen is to share a PortMaster, and if so, what
     * host it resides at
     */
    pmshare= getenv("PMSHARE");
	
    /* open a socket to the target host's PortMaster */
    skt= Sopen_clientport(hostname,sktname,PORTMASTER);
    if(!skt && pmshare) skt= Sopen_clientport(pmshare,sktname,PORTMASTER);
    if(!skt) {
      return (Socket *) NULL;
    }
	
    /* go through PortMaster client protocol to get port */
    do {
      mesg= htons(blocking? PM_CLIENTWAIT : PM_CLIENT);
      Swrite(skt,(char *) &mesg,sizeof(mesg));
	
	
      /* get PM_OK or PM_RESEND from PortMaster */
      if(Stimeoutwait(skt,TIMEOUT,0L) < 0) {
        shutdown(skt->skt,2);
        close(skt->skt);
        freeSocket(skt);
        return (Socket *) NULL;
      }
      Sreadbytes(skt,(char *) &mesg,sizeof(mesg));
      mesg= ntohs(mesg);
	
	
      /* too many retries or PM_SORRY
       * (latter usually due to firewall violation)
       */
      if(++resend >= PM_MAXTRY || mesg == PM_SORRY) {
        shutdown(skt->skt,2);
        close(skt->skt);
        freeSocket(skt);
        return (Socket *) NULL;
      }
    } while(mesg != PM_OK);
	
    /* send sktname of desired port */
    Sputs(sktname,skt);
	
    /* get response (PM_OK, PM_OKSHARE, or PM_SORRY) */
    if((blocking && Swait(skt) < 0) || Stimeoutwait(skt,TIMEOUT,0L) < 0) {
      shutdown(skt->skt,2);
      close(skt->skt);
      freeSocket(skt);
      return (Socket *) NULL;
    }
    Sreadbytes(skt,(char *) &mesg,sizeof(mesg));
    mesg= ntohs(mesg);
	
    if(mesg == PM_OKSHARE) {
      if(Stimeoutwait(skt,TIMEOUT,0L) < 0) {
        shutdown(skt->skt,2);
        close(skt->skt);
        freeSocket(skt);
        return (Socket *) NULL;
      }
      Sgets(skthost,BUFSIZE,skt);
    }
    else if(mesg != PM_OK) {
      shutdown(skt->skt,2);
      close(skt->skt);
      freeSocket(skt);
      return (Socket *) NULL;
    }
    else skthost[0]= '\0';
	
    /* get response (server's port #)
     *   If blocking, will wait until the PortMaster sends the information.
     *   If not blocking, wait up to TIMEOUT seconds (20seconds) 
     */
    if(Stimeoutwait(skt,TIMEOUT,0L) < 0) {
      shutdown(skt->skt,2);
      close(skt->skt);
      freeSocket(skt);
      return (Socket *) NULL;
    }
    Sreadbytes(skt,(char *) &mesg,sizeof(mesg));
    port= ntohs(mesg);
	
    /* close the PortMaster client socket */
    shutdown(skt->skt,2);
    close(skt->skt);
    freeSocket(skt);
  }
  else {
    port= (SKTEVENT) sslport;
  }

  /* --- Return to Client Initialization --- */

  /* open a socket to the port returned by the target host's PortMaster */
  skt= Sopen_clientport(skthost[0]? skthost : hostname,sktname,port);
  if(!skt) {
    return (Socket *) NULL;
  }

  return skt;
}

/* ---------------------------------------------------------------------- */

/* Sopen_clientport: this function opens a client socket given a hostname
 * and port
 */
#ifdef __PROTOTYPE__
Socket *Sopen_clientport(
  char   *hostname,
  char   *sktname,
  u_short port)
#else
Socket *Sopen_clientport(
	hostname,
	sktname,
	port)
char *hostname;
char *sktname;
u_short port;
#endif
{
  Socket            *skt        = NULL;
  int                status     = 1;
  static char        localhost[BUFSIZE];
  struct hostent    *hostentptr = NULL;
  struct sockaddr_in sin;				/* InterNet socket address structure */


  /* if hostname is null, then use current hostname */
  if(!hostname || !hostname[0]) {
    gethostname(localhost,BUFSIZE);
    hostname= localhost;
  }

  /* allocate a Socket */
  skt= makeSocket(hostname,sktname,PM_CLIENT);
  if(!skt) {
    return (Socket *) NULL;
  }
  skt->port= port;

  /*	open socket */

  skt->skt= socket(AF_INET,SOCK_STREAM,0);
  if(skt->skt < 0) {
    freeSocket(skt);
    return (Socket *) NULL;
  }

  /*	initialize socket data structure
   *  memset: added by Steve Richards for AS/400
   */
  memset(&sin,0x00,sizeof(struct sockaddr_in));
  sin.sin_family = AF_INET;
  hostentptr     = gethostbyname(hostname);


  if(!hostentptr) {
    freeSocket(skt);
    return (Socket *) NULL;
  }

  /* get host address */
  sin.sin_addr = * ((struct in_addr *) hostentptr->h_addr);
  sin.sin_port = htons((u_short) port);

  /* send port number to server */

  /* connect to remote host */
  if(connect(skt->skt,(struct sockaddr *) &sin,sizeof(sin)) < 0){
    shutdown(skt->skt,2);
    close(skt->skt);
    freeSocket(skt);
    return (Socket *) NULL;
  }

#ifndef SSLNOSETSOCKOPT
  /* turn off TCP's buffering algorithm so small packets don't get
   * needlessly delayed
   */

  if(setsockopt(skt->skt,IPPROTO_TCP,TCP_NODELAY,(char *) &status,sizeof(status))
     < 0) {
  }
#endif	/* SSLNOSETSOCKOPT */

  return skt;
}

/* ---------------------------------------------------------------------
 * vim: ts=4
 */
