/* sktdbg.c: this program supports testing the Sockets library
 *   sktdbg {s|c} sktname
 */
#include <stdio.h>
#include "xtdio.h"
#include "sockets.h"

/* -------------------------------------------------------------------------
 * Local Definitions:
 */
/* #define ansi_escapes  -- experimental, doesn't work yet */

#define BIGBUFSIZE		8192
#define BUFSIZE			128
#define PLAINPROMPT		0
#define BYTESPROMPT		1
#define DYNAMICPROMPT	2

#define TIMEOUT			30L

/* -------------------------------------------------------------------------
 * Prototypes:
 */
#ifdef __PROTOTYPE__
void Taccept(char *buf);
void Tclose(char *buf);
void Tmenu(char *buf);
void Tdebug(char *buf);
void Tpeek(char *buf);
void Ttest(char *buf);
void Tfwinit(char *buf);
void Tfput(char *buf);
void Tgets(char *buf);
void Tprintf(char *buf);
void Tputs(char *buf);
void Tshut(char *buf);
void Tquit(char *buf);
void Tread(char *buf);
void Trmsrvr(char *buf);
void Tscanf(char *buf);
void Ttable(char *buf);
void Twait(char *buf);
void Twrite(char *buf);
void Tisset(char *buf);

#else

extern main();                                    /* sktdbg.c             */
extern void Taccept();                            /* sktdbg.c             */
extern void Tclose();                             /* sktdbg.c             */
extern void Tmenu();                              /* sktdbg.c             */
extern void Tdebug();                             /* sktdbg.c             */
extern void Tpeek();                              /* sktdbg.c             */
extern void Ttest();                              /* sktdbg.c             */
extern void Tfwinit();                            /* sktdbg.c             */
extern void Tfput();                              /* sktdbg.c             */
extern void Tgets();                              /* sktdbg.c             */
extern void Tputs();                              /* sktdbg.c             */
extern void Tshut();                              /* sktdbg.c             */
extern void Tquit();                              /* sktdbg.c             */
extern void Tread();                              /* sktdbg.c             */
extern void Twrite();                             /* sktdbg.c             */
extern void Twait();                              /* sktdbg.c             */
extern void Tscanf();                             /* sktdbg.c             */
extern void Tprintf();                            /* sktdbg.c             */
extern void Ttable();                             /* sktdbg.c             */
extern void Trmsrvr();                            /* sktdbg.c             */
extern void Tisset();                             /* sktdbg.c             */

#endif

/* -------------------------------------------------------------------------
 * Global Data
 */
#ifdef __PROTOTYPE__
struct menu_str{
	char  *cmd;
	void (*test)(char *);
	} menu[]=
#else
struct menu_str{
	char  *cmd;
	void (*test)();
	} menu[]=
#endif
	{{"accept" , Taccept},
	{"close"   , Tclose},
	{"fput"    , Tfput},
	{"fwinit"  , Tfwinit},
	{"get"     , Tgets},
	{"isset"   , Tisset},
	{"menu"    , Tmenu},
	{"peek"    , Tpeek},
	{"printf"  , Tprintf},
	{"put"     , Tputs},
	{"q"       , Tquit},
	{"quit"    , Tquit},
	{"read"    , Tread},
	{"rmsrvr"  , Trmsrvr},
	{"scanf"   , Tscanf},
	{"shutdown", Tshut},
	{"table"   , Ttable},
	{"test"    , Ttest},
	{"wait"    , Twait},
	{"write"   , Twrite},
	#ifdef DEBUG
	{"D"       , Tdebug},
	#endif
	{"?"       , Tmenu}
	};
int nmenu      = sizeof(menu)/sizeof(struct menu_str);
Socket *skt    = NULL;
Socket *server = NULL;

#ifdef ansi_escapes
int pflag= DYNAMICPROMPT;
#else
int pflag= BYTESPROMPT;
#endif

/* -------------------------------------------------------------------------
 * Source Code:
 */

/* main: */
#ifdef __PROTOTYPE__
int main(
  int    argc,
  char **argv)
#else	/* __PROTOTYPE__ */
int main(argc,argv)
int    argc;
char **argv;
#endif	/* __PROTOTYPE__ */
{
char  *b               = NULL;
char   buf[BUFSIZE];
char   cmdbuf[BUFSIZE];
int    icmd;
#ifdef ansi_escapes
int    fd;
Smask  fdmask;
int    newcnt          = 0;
int    oldcnt          = 0;
Smask  sktmask;
#endif

rdcolor(); /* initialize ANSI escape sequences */

if(argc < 3) { /* give usage */
	static char *usage[]={
	 XYELLOW,"sktdbg ",
	 XCYAN,"sktname ",XGREEN,
	 "{",XCYAN,"sScb",XGREEN,"}\n\n",

	 XUWHITE,"Author:\n",XGREEN,"Dr. Charles E. Campbell\n\n",

	 XUWHITE,"Explanation:\n",XGREEN,
	 "This program is designed to test out the Sockets Library.\n",
	 "The ",XCYAN,"sktname",XGREEN," is the name of the socket.\n",
	 "It can be sktname@hostname or just sktname (current host is then used)\n",
	 "The ",XCYAN,"s",XGREEN," yields a server/accept socket.\n",
	 "The ",XCYAN,"c",XGREEN," yields a client socket.\n",
	 "The ",XCYAN,"b",XGREEN," yields a client-wait socket.\n\n",

	 "The Sockets Library was written by Dr. Charles E. Campbell, Jr. and\n",
	 "Terry McRoberts of Goddard Space Flight Center, NASA\n",
	 "",""};
	char **u;

	/* print out explanation */
	for(u= usage; **u || **(u+1); ++u) rdcputs(*u,stdout);
#ifdef vms
	exit(1);
#else
	exit(0);
#endif
	}

Sinit();	/* initialize sockets */

/* open a socket of the appropriate type */
skt= Sopenv(argv[1],argv[2],"SKTDBG");
if(!skt) {
	if(!strcmp(argv[2],"s")) {
		error(XTDIO_WARNING,"unable to Sopen(%s,%s), removing <%s>\n", argv[1],argv[2],argv[1]);
		Srmsrvr(argv[1]);
		skt= Sopenv(argv[1],argv[2],"SKTDBG");
		if(!skt) error(XTDIO_ERROR,"2nd try: unable to Sopen(%s,%s)\n",argv[1],argv[2]);
		else     error(XTDIO_NOTE,"second try to Sopen(%s,%s) succeeded\n",argv[1],argv[2]);
		}
	else error(XTDIO_ERROR,"unable to Sopen(%s,%s)\n",argv[1],argv[2]);
	}

/* print out menu */
fputc('\n',stdout);
Tmenu("");

/* command loop */
while(1) {

	/* get command from user */
	switch(pflag) {

	case PLAINPROMPT:
		printf("%sEnter: %s",CYAN,GREEN);
		break;

	case BYTESPROMPT:
		printf("%s(%s%4d%s bytes) Enter: %s",
		  CYAN,GREEN,Stest(skt),
		  CYAN,GREEN);
		break;

#ifdef ansi_escapes
	case DYNAMICPROMPT:
		/* set up Smaskwait */
		Smaskset((Socket *) NULL);
		fd= fileno(stdin);
		Smaskfdset(fd);
		fdmask= Smaskget();
		Smaskset(skt);
		sktmask= Smaskget();
		printf("%s(%s%4d%s bytes) Enter: %s",
		  CYAN,GREEN,Stest(skt),
		  CYAN,GREEN);
		fflush(stdout);

		while(1) {
			/* block until skt or keyboard sends something */
			Smaskwait();
			Smaskuse(fdmask);
			if(Smasktest()) break;
			Smaskuse(sktmask);

			/* update screen */
			newcnt= Stest(skt);
			if(newcnt == oldcnt) continue;
			oldcnt= newcnt;
			printf("\033[20D%s(%s%4d%s bytes) Enter: %s",
			  CYAN,GREEN,newcnt,
			  CYAN,GREEN);
			fflush(stdout);
			sleep(1);
			}
		break;
#endif	/* ansi_escapes */
		}

	/* get prompt from user */
	fgets(buf,BUFSIZE,stdin);
	srmtrblk(buf);
	b= stpblk(buf);
	if(!*b) continue;

	/* get command from b */
	sscanf(buf,"%s",cmdbuf);
	b= stpnxt(buf,"%s%b");	/* point past command string */

	/* handle option */
	if(*b == '-') {
		switch(b[1]) {

		case 'p':	/* prompt mode control */
#ifdef ansi_escapes
			if(++pflag > DYNAMICPROMPT) pflag= PLAINPROMPT;
#else
			if(++pflag > BYTESPROMPT)   pflag= PLAINPROMPT;
#endif
			break;

		default:
			error(XTDIO_WARNING,"I don't know the <%s%s%s> command\n",
			  MAGENTA,cmdbuf,GREEN);
			break;
			}
		continue;
		}

	/* identify command */
	for(icmd= 0; icmd < nmenu; ++icmd)
	  if(!strcmp(menu[icmd].cmd,cmdbuf)) break;
	if(icmd >= nmenu) {
		error(XTDIO_WARNING,"I don't know the <%s%s%s> command\n",
		  MAGENTA,cmdbuf,GREEN);
		continue;
		}

	/* pass control to command */
	(*menu[icmd].test)(b);
	}
}

/* ------------------------------------------------------------------------- */

/* Taccept: this function allows a server to accept a connection */
#ifdef __PROTOTYPE__
void Taccept(char *buf)
#else	/* __PROTOTYPE__ */
void Taccept(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{

/* If server is null, then the skt *is* a server.  Copy it over.
 * Otherwise, if there's a skt, close it before accepting
 */
if(!server)  {
	if(skt->type == PM_SERVER) {
		server= skt;
		skt   = NULL;
		}
	else {
		error(XTDIO_WARNING,"cannot Saccept with a skt<%s>\n",Sprtskt(skt));
		return;
		}
	}
else if(skt) {
	Sclose(skt);
	skt= NULL;
	}

/* only wait up to TIMEOUT seconds */
if(Stimeoutwait(server,TIMEOUT,0L) <= 0) {
	error(XTDIO_WARNING,"accept timed out (waited %ld seconds)\n",TIMEOUT);
	return;
	}
skt   = Saccept(server);

printf("  %sSaccept%s => <%s%s%s>\n",
  YELLOW,GREEN,MAGENTA,skt? Sprtskt(skt) : "null",GREEN);

}

/* ------------------------------------------------------------------------- */

/* Tclose: this function closes the socket and,
 * for non-accept sockets, exits the program
 */
#ifdef __PROTOTYPE__
void Tclose(char *buf)
#else	/* __PROTOTYPE__ */
void Tclose(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{

switch(skt->type) {

case PM_ACCEPT:
	Sclose(skt);
	skt   = server;
	server= NULL;
	printf("  %sSclose%s: closed accept Socket\n",YELLOW,GREEN);
	printf("              server socket still available\n");
	break;

case PM_CLIENT:
	Sclose(skt);
	printf("  %sSclose%s: closed client Socket\n",YELLOW,GREEN);
#ifdef vms
	exit(1);
#else
	exit(0);
#endif

case PM_SERVER:
	Sclose(skt);
	skt= server= NULL;
	printf("  %sSclose%s: closed server Socket\n",YELLOW,GREEN);
#ifdef vms
	exit(1);
#else
	exit(0);
#endif
	}

}

/* ------------------------------------------------------------------------- */

/* Tmenu: this function print out the commands */
#ifdef __PROTOTYPE__
void Tmenu(char *buf)
#else	/* __PROTOTYPE__ */
void Tmenu(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
int i;


printf("%sSocket Test Commands%s\n",UWHITE,GREEN);
for(i= 0; i < nmenu; ++i) printf("  %s\n",menu[i].cmd);

}

/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */

/* Tpeek: this function does a Speek */
#ifdef __PROTOTYPE__
void Tpeek(char *buf)
#else	/* __PROTOTYPE__ */
void Tpeek(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
static char locbuf[BIGBUFSIZE];
int         buflen;


buflen= Speek(skt,locbuf,BIGBUFSIZE);
printf("  %sSpeek%s=> skt<%s> locbuf<%s> buflen=%d\n",
  YELLOW,GREEN,Sprtskt(skt),sprt(locbuf),buflen);

}

/* ------------------------------------------------------------------------- */

/* Ttest: this function does a STell */
#ifdef __PROTOTYPE__
void Ttest(char *buf)
#else	/* __PROTOTYPE__ */
void Ttest(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
int  buflen;


buflen= Stest(skt);
buf[0]= '\0';
printf("  %sStest%s => skt<%s> %d\n",YELLOW,GREEN,Sprtskt(skt),buflen);

}

/* ------------------------------------------------------------------------- */

/* Tfwinit: sends a PM_FW to the PortMaster */
#ifdef __PROTOTYPE__
void Tfwinit(char *buf)
#else	/* __PROTOTYPE__ */
void Tfwinit(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
int      trycnt;
Socket  *pmskt;
SKTEVENT mesg;


/* begin PortMaster firewall re-initialize */
pmskt= Sopen_clientport(buf,"PMClient",PORTMASTER);

if(!pmskt) {
	freeSocket(pmskt);
	return;
	}

/* send a PM_FWINIT */
for(trycnt= 0; trycnt < PM_MAXTRY; ++trycnt) {

	/* this is where the PM_FWINIT gets sent */
	mesg= htons(PM_FWINIT);
	Swrite(pmskt,(char *) &mesg,sizeof(mesg));

	/* wait for PortMaster to respond with PM_OK (I hope) */
	if(Stimeoutwait(pmskt,20L,0L) < 0) {
		Sclose(pmskt);
		return;
		}

	/* read PortMaster response */
	Sread(pmskt,(char *) &mesg,sizeof(mesg));
	mesg= ntohs(mesg);
	if(mesg != PM_RESEND) break;
	}
if(mesg == PM_OK) printf("PortMaster firewall re-initialized\n");
else              printf("PortMaster isn't handling firewalls\n");

}

/* ------------------------------------------------------------------------- */

/* Tfput: puts the entire contents of a file, line by line, across a Socket */
#ifdef __PROTOTYPE__
void Tfput(char *buf)
#else	/* __PROTOTYPE__ */
void Tfput(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
char *b               = NULL;
char  locbuf[BUFSIZE];
FILE *fp              = NULL;


srmtrblk(buf);
b = stpblk(buf);
if(*b && skt) {
	fp= fopen(b,"r");
	if(!fp) {
		error(XTDIO_WARNING,"unable to open file<%s>\n",sprt(b));
		return;
		}
	while(fgets(locbuf,BUFSIZE,fp)) {
		srmtrblk(locbuf);
		Sputs(locbuf,skt);
		printf("  %sSputs%s(locbuf<%s>,skt<%s>)\n",
		  YELLOW,
		  GREEN,
		  sprt(locbuf),
		  Sprtskt(skt));
		}
	fclose(fp);
	}

}

/* --------------------------------------------------------------------- */

/* Tgets: this function does a Sread */
#ifdef __PROTOTYPE__
void Tgets(char *buf)
#else	/* __PROTOTYPE__ */
void Tgets(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
char locbuf[BUFSIZE];


/* only wait up to TIMEOUT seconds */
if(Stimeoutwait(skt,TIMEOUT,0L) <= 0) {
	error(XTDIO_WARNING,"get timed out (waited %ld seconds)\n",TIMEOUT);
	return;
	}

(void *) Sgets(locbuf,BUFSIZE,skt);

printf("  %sSgets%s: skt<%s> locbuf<%s>\n",
  YELLOW,GREEN,Sprtskt(skt),sprt(locbuf));

}

/* ------------------------------------------------------------------------- */

/* Tputs: this function does a Swrite
 *   Supports \a - \z for control character insertion
 */
#ifdef __PROTOTYPE__
void Tputs(char *buf)
#else	/* __PROTOTYPE__ */
void Tputs(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
char  locbuf[BUFSIZE];
char *backslash;
char *b;
char *lb;


backslash= strchr(buf,'\\');
if(backslash) {
	for(b= buf, lb= locbuf; *b; ++b) {
		*lb= *b;
		if(*b == '\\') {
			++b;
			if     (*b == '\\')             continue;
			else if('a' <= *b && *b <= 'z') *lb= (*b) - 'a' + 1;
			}
		}
	*lb= '\0';
	Sputs(locbuf,skt);
	printf("  %sSputs%s(buf<%s>,skt<%s>)\n",YELLOW,GREEN,sprt(locbuf),Sprtskt(skt));
	}
else {
	Sputs(buf,skt);
	printf("  %sSputs%s(buf<%s>,skt<%s>)\n",YELLOW,GREEN,sprt(buf),Sprtskt(skt));
	}

}

/* ------------------------------------------------------------------------- */

/* Tshut: this function shuts down the PortMaster */
#ifdef __PROTOTYPE__
void Tshut(char *buf)
#else	/* __PROTOTYPE__ */
void Tshut(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
int      trycnt;
Socket  *pmskt;
SKTEVENT mesg;


/* shut down client/accept socket */
printf("  %sSclose%s(skt<%s>)",YELLOW,GREEN,Sprtskt(skt));
Sclose(skt); skt= NULL;

/* if server, shut down server socket */
if(server) {
	printf("  %sSclose%s(server<%s>)",YELLOW,GREEN,Sprtskt(server));
	Sclose(server); server= NULL;
	}

/* begin PortMaster shutdown sequence */
pmskt= Sopen_clientport(buf,"PMClient",PORTMASTER);
if(!pmskt) {
	freeSocket(pmskt);
	return;
	}

/* send a QUIT */
for(trycnt= 0; trycnt < PM_MAXTRY; ++trycnt) {
	mesg= htons(PM_QUIT);
	Swrite(pmskt,(char *) &mesg,sizeof(mesg));
	if(Stimeoutwait(pmskt,20L,0L) < 0) {
		Sclose(pmskt);
		return;
		}
	Sread(pmskt,(char *) &mesg,sizeof(mesg));
	mesg= ntohs(mesg);
	if(mesg == PM_OK) break;
	}

/* finish up termination sequence */
Sputs("PortMaster",pmskt);
Sclose(pmskt);
printf("  %sclosed down%s PortMaster\n",YELLOW,GREEN);

/* ok, exit the program */
#ifdef vms
exit(1);
#else
exit(0);
#endif
}

/* ------------------------------------------------------------------------- */

/* Tquit: this function quits */
#ifdef __PROTOTYPE__
void Tquit(char *buf)
#else	/* __PROTOTYPE__ */
void Tquit(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{

if(skt) {
	printf("  %sSclose%s(skt<%s>)\n",YELLOW,GREEN,Sprtskt(skt));
	Sclose(skt);
	}
if(server) {
	printf("  %sSclose%s(server<%s>)\n",YELLOW,GREEN,Sprtskt(server));
	Sclose(server);
	}

#ifdef vms
exit(1);
#else
exit(0);
#endif
}

/* ------------------------------------------------------------------------- */

/* Tread: this function does a Sread */
#ifdef __PROTOTYPE__
void Tread(char *buf)
#else	/* __PROTOTYPE__ */
void Tread(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
char locbuf[BUFSIZE];
int  buflen;
int  ibuf;


buflen= Sread(skt,locbuf,BUFSIZE);
printf("  %sSread%s: skt<%s> locbuf<",YELLOW,GREEN,Sprtskt(skt));
for(ibuf=0; ibuf < buflen; ++ibuf) printf("%s ",cprt(locbuf[ibuf]));
printf("> buflen=%d\n",buflen);

}

/* ------------------------------------------------------------------------- */

/* Twrite: this function does a Swrite */
#ifdef __PROTOTYPE__
void Twrite(char *buf)
#else	/* __PROTOTYPE__ */
void Twrite(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
int  buflen;


buflen= Swrite(skt,buf,strlen(buf)+1);
printf("  %sSwrite%s: skt<%s> buf<%s> buflen=%d\n",
  YELLOW,GREEN,Sprtskt(skt),sprt(buf),buflen);

}

/* ------------------------------------------------------------------------- */

/* Twait: this function does a Swait */
#ifdef __PROTOTYPE__
void Twait(char *buf)
#else	/* __PROTOTYPE__ */
void Twait(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
int  buflen;


buflen= Swait(skt);
printf("  %sSwait%s: skt<%s> buflen=%d\n",YELLOW,GREEN,Sprtskt(skt),buflen);

}

/* ------------------------------------------------------------------------- */

/* Tscanf: this function tests Sscanf with "%s %d %lf %s" */
#ifdef __PROTOTYPE__
void Tscanf(char *buf)
#else	/* __PROTOTYPE__ */
void Tscanf(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
char   *b             = NULL;
char    buf1[BUFSIZE];
int     i;
double  d;


for(b= strchr(buf,'%'); b; b= strchr(b+1,'%')) {

	/* only wait up to TIMEOUT seconds */
	if(Stimeoutwait(skt,TIMEOUT,0L) <= 0) {
		error(XTDIO_WARNING,"scanf timed out (waited %ld seconds)\n",TIMEOUT);
		return;
		}

	/* skip over 'l' and 'h' modifier */
	if     (b[1] == 'l') ++b;
	else if(b[1] == 'h') ++b;

	/* process data on Socket */
	switch(b[1]) {

	case 'c':
		Sscanf(skt,"%c",buf1);
		printf("  %sSscanf%s with  %%c: rcvd<%s%c%s>\n",
		  YELLOW,CYAN,
		  GREEN,buf1[0],CYAN);
		break;

	case 'd':
		Sscanf(skt,"%d",&i);
		printf("  %sSscanf%s with  %%d: rcvd<%s%d%s>\n",
		  YELLOW,CYAN,
		  GREEN,i,CYAN);
		break;

	case 'f':
		Sscanf(skt,"%lf",&d);
		printf("  %sSscanf%s with %%lf: rcvd<%s%f%s>\n",
		  YELLOW,CYAN,
		  GREEN,d,CYAN);
		break;

	case 's':
		Sscanf(skt,"%s",buf1);
		printf("  %sSscanf%s with  %%s: rcvd<%s%s%s>\n",
		  YELLOW,CYAN,
		  GREEN,sprt(buf1),CYAN);
		break;

	default:
		error(XTDIO_WARNING,"sktdbg doesn't support format code <%s>\n",sprt(b));
		break;
		}
	}

}

/* ------------------------------------------------------------------------- */

/* Tprintf: this function tests Sprintf with "%s %d %s" */
#ifdef __PROTOTYPE__
void Tprintf(char *buf)
#else	/* __PROTOTYPE__ */
void Tprintf(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
int    i = 7;
double d = 8.;


Sprintf(skt,"%s %d %f itworked!",buf,i,d);
printf("  %sSprintf%s: skt<%s> i=%d d=%f itworked!\n",
  YELLOW,GREEN,Sprtskt(skt),i,d);

}

/* ------------------------------------------------------------------------- */

/* Ttable: this function queries the PortMaster for a table, and then
 * prints it out
 */
#ifdef __PROTOTYPE__
void Ttable(char *buf)
#else	/* __PROTOTYPE__ */
void Ttable(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
int       trycnt;
Socket   *pmskt  = NULL;
SKTEVENT  mesg;
SKTEVENT  cnt;


pmskt= Sopen_clientport(buf,"PMClient",PORTMASTER);
if(!pmskt) {
	freeSocket(pmskt);
	return;
	}

/* send a PM_TABLE request */
for(trycnt= 0; trycnt < PM_MAXTRY; ++trycnt) {
	mesg= PM_TABLE;
	mesg= htons(mesg);
	Swrite(pmskt,(char *) &mesg,sizeof(mesg));
	Sread(pmskt,(char *) &mesg,sizeof(mesg));
	mesg= ntohs(mesg);
	if(mesg == PM_OK) break;
	}

/* find out how many entries in table there are */
Sread(pmskt,(char *) &cnt,sizeof(cnt));
cnt= ntohs(cnt);

/* begin to receive table */
for(; cnt > 0; --cnt) {
	Sgets(buf,BUFSIZE,pmskt);
	printf("%2d: %s\n",cnt,buf);
	}

/* close socket to PortMaster */
Sclose(pmskt);

}

/* ------------------------------------------------------------------------- */

/* Trmsrvr: this function removes a server from the PortMaster's PortTable */
#ifdef __PROTOTYPE__
void Trmsrvr(char *buf)
#else	/* __PROTOTYPE__ */
void Trmsrvr(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
SKTEVENT mesg;


mesg= Srmsrvr(buf);
printf("  %sSrmsrvr%s: buf<%s> mesg=%s\n",
  YELLOW,GREEN,sprt(buf),(mesg == PM_OK)? "success" : "failure");

}

/* ------------------------------------------------------------------------- */

/* Tisset: this function tests if Smaskisset works:
 *         it does so by using Smaskwait() with zero timeout
 *         and then invokves Smaskisset() to test the
 *         Socket.  This function is primarily intended to
 *         debug & test the Smaskisset() function.
 */
#ifdef __PROTOTYPE__
void Tisset(char *buf)
#else
void Tisset(buf)
char *buf;
#endif
{
int ret;


Smasktime(0L,1L);
Smaskset(skt);
Smaskwait();
ret= Smaskisset(skt);

printf("  %ssocket %s%s%s read-ready\n",
  GREEN,
  YELLOW,
  ret? "is" : "is not",
  GREEN);

}

/* --------------------------------------------------------------------- */
