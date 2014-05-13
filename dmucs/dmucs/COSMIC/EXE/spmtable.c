/* Spmtable: this program prints out a list of servers currently on the
 * requested PortMaster
 *
 * spmtable machine machine machine ...
 * spmtable
 * spmtable -ENVVARNAME
 */
#include <stdio.h>
#include "xtdio.h"
#include "sockets.h"

/* ----------------------------------------------------------------------
 * Definitions:
 */
#define BUFSIZE			256
#define MACHINE_SEP		':'
#define MAXTIME			5L		/* max time for PM_TABLE response, in sec	*/

/* ----------------------------------------------------------------------
 * Prototypes:
 */
#ifdef __PROTOTYPE__
int main(int,char **);                            /* Spmtable.c           */
void printPMtable(char *);                        /* Spmtable.c           */
#else
extern main();                                    /* spmtable.c           */
extern void printPMtable();                       /* spmtable.c           */
#endif


/* ---------------------------------------------------------------------- */

/* main: */
#ifdef __PROTOTYPE__
int main(int argc,char **argv)
#else
int main(argc,argv)
int argc;
char **argv;
#endif
{
char localhost[BUFSIZE];

rdcolor();

Sinit();

if(argc > 1) {
	for(++argv; argc > 1; --argc, ++argv) {	/* for every machine mentioned on the cmd line	*/
		if(argv[0][0] == '-') {				/* report on SKTPATH machines					*/
			char *envvar= argv[0] + 1;
			char *machines;
			char *mend;

			/* look up the SKTPATH */
			machines= getenv(*envvar? envvar : "SKTPATH");
			if(!machines) {
				error(XTDIO_WARNING,"unable to getenv(%s%s%s)\n",
				  YELLOW,sprt(*envvar? envvar : "SKTPATH"),GREEN);
				continue;
				}

			/* make a multiple machine report */
			printf("  %sMultiple Machine PortMaster Tables\n",WHITE);
			printf("    %s%s%s<%s%s%s>%s\n\n",
			  CYAN,*envvar? envvar : "SKTPATH",YELLOW,
			  GREEN,machines,YELLOW,NRML);
			mend= strchr(machines,MACHINE_SEP);
			while(mend) {
				*mend   = '\0';
				printPMtable(machines);
				*mend   = MACHINE_SEP;
				machines= mend + 1;
				mend    = strchr(machines,MACHINE_SEP);
				}
			if(machines) printPMtable(machines);
			printf("%s\n",NRML);
			}
		else printPMtable(*argv);			/* report on mentioned machine					*/
		}
	}
else {										/* report on local machine						*/
	char *pmshare= NULL;
	pmshare= getenv("PMSHARE");
	if(pmshare) printPMtable(pmshare);		/* if PMSHARE, default to its PortMaster		*/
	else {
		gethostname(localhost,BUFSIZE);		/* otherwise, use localhost's PortMaster		*/
		printPMtable(localhost);
		}
	}

#ifdef vms
return 1;
#else
return 0;
#endif
}

/* ---------------------------------------------------------------------- */

/* printPMtable: this function queries the PortMaster for a table, and then
 * prints it out
 */
#ifdef __PROTOTYPE__
void printPMtable(char *buf)
#else	/* __PROTOTYPE__ */
void printPMtable(buf)
char *buf;
#endif	/* __PROTOTYPE__ */
{
char     locbuf[BUFSIZE];
int      icnt;
int      itry;
Socket  *pmskt;
SKTEVENT mesg;
SKTEVENT cnt;


/* open a client socket to the PortMaster */
pmskt= Sopen_clientport(buf,"PMClient",PORTMASTER);
if(!pmskt) {
	error(XTDIO_WARNING,"unable to connect to <%s%s%s>'s PortMaster\n",
	  RVCYAN,buf,GREEN);
	return;
	}

/* send a PM_TABLE request */
for(itry= 0; itry < PM_MAXTRY; ++itry) {
	mesg= PM_TABLE;
	mesg= htons(mesg);
	Swrite(pmskt,(char *) &mesg,sizeof(mesg));

	/* wait for response */
	if(Stimeoutwait(pmskt,MAXTIME,0L) < 0) {
		Sclose(pmskt);
		error(XTDIO_WARNING,"%s's PortMaster is not responding\n",sprt(buf));
		return;
		}
	Sread(pmskt,(char *) &mesg,sizeof(mesg));

	/* translate reponse */
	mesg= ntohs(mesg);
	if     (mesg == PM_OK)    break;
	else if(mesg == PM_SORRY) {
		error(XTDIO_WARNING,"%s's PortMaster is refusing connections\n",sprt(buf));
		return;
		}
	}

/* find out how many entries in table there are */
if(Stimeoutwait(pmskt,MAXTIME,0L) < 0) {
	Sclose(pmskt);
	error(XTDIO_WARNING,"%s's PortMaster is not responding with entry count\n",sprt(buf));
	return;
	}
Sread(pmskt,(char *) &cnt,sizeof(cnt));
cnt= ntohs(cnt);

if(cnt) {
	printf("\n%sCurrent %s%s%s Portmaster Servers\n",
	  UWHITE,
	  RVCYAN, buf, UWHITE);
	
	/* begin to receive table */
	for(icnt= 1; cnt > 0; --cnt, ++icnt) {
		if(Stimeoutwait(pmskt,MAXTIME,0L) < 0) {
			error(XTDIO_WARNING,"%s abruptly ceased sending server names\n",sprt(buf));
			break;
			}
		if(!Sgets(locbuf,BUFSIZE,pmskt)) {
			error(XTDIO_WARNING,"%s disconnected unexpectedly!\n",sprt(buf));
			break;
			}
		printf("%s%2d: %s%s\n",CYAN,icnt,GREEN,locbuf);
		}
	puts(NRML);
	}
else printf("%s%s%s's PortMaster currently has no servers%s\n",RVCYAN,buf,GREEN,NRML);

/* close socket to PortMaster */
Sclose(pmskt);

}

/* --------------------------------------------------------------------- */
