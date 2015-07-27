/* Smaskwait.c: this file contains code which allows one to wait until data
 *  is present given a mask
 * Authors:      Charles E. Campbell, Jr.
 *               Terry McRoberts
 * Copyright:    Copyright (C) 1999-2005 Charles E. Campbell, Jr. {{{1
 *               Permission is hereby granted to use and distribute this code,
 *               with or without modifications, provided that this copyright
 *               notice is copied with it. Like anything else that's free,
 *               Smaskwait.c is provided *as is* and comes with no warranty
 *               of any kind, either expressed or implied. By using this
 *               software, you agree that in no event will the copyright
 *               holder be liable for any damages resulting from the use
 *               of this software.
 * Date:         Aug 22, 2005
 */
#include <stdio.h>
#include "xtdio.h"
#define  SSLNEEDTIME
#include "sockets.h"

/* ---------------------------------------------------------------------
 * Local Variables:
 */
static int             waitall = 0;   
static fd_set          mask;  			/* mask last-used by Smaskwait, etc		*/
static struct timeval *timeptr = NULL;
static struct timeval  timewait;
static fd_set          rmask;			/* set by Smaskwait						*/
static int             first   = 1;   

/* ---------------------------------------------------------------------
 * Local Prototypes:
 */

/* ---------------------------------------------------------------------
 * Source Code:
 */

/* Smaskwait: this function waits until some Socket specified to the
 * Smaskset() function receives something.
 *
 * Returns: < 0: error
 *          ==0: timeout
 *          > 0: communication data awaiting
 *
 *   NOTE: THE MASK IS NOT RESET TO ZERO - THE MASK IS RETAINED!!!
 */
#ifdef __PROTOTYPE__
int Smaskwait()
#else
    int Smaskwait()
#endif
{
    fd_set wmask;			/* write mask */
    fd_set emask;			/* error mask */
    short  result;


    FD_ZERO(&wmask);
    FD_ZERO(&emask);

    /* test if something is available for reading on the socket.  This form
     * will block (sleep) until something arrives
     */
    rmask  = mask;
    result = select(waitall, &rmask,&wmask,&emask,timeptr);

    if(result < 0) {	/* error */
	return result;
    }

    /* result >  0: socket has something waiting 
     * result == 0: timeout
     */
    return result;
}

/* --------------------------------------------------------------------- */

/* Smaskset: this function sets up the mask
 *  A null skt clears the mask
 */
#ifdef __PROTOTYPE__
void Smaskset(Socket *skt)
#else
    void Smaskset(skt)
    Socket *skt;
#endif
{

    if(first) {
	FD_ZERO(&mask);
	first= 0;
    }

    if(skt) {
	FD_SET(skt->skt,&mask);
	if(skt->skt >= waitall) waitall= skt->skt + 1;
    }
    else {
	waitall= 0;
	FD_ZERO(&mask);
    }

}

/* --------------------------------------------------------------------- */

/* Smaskfdset: this function sets up the mask with a "file descriptor"
 * which may be associated with devices other than Sockets
 */
#ifdef __PROTOTYPE__
void Smaskfdset(int fd)
#else
    void Smaskfdset(fd)
    int fd;
#endif
{

    if(first) {
	FD_ZERO(&mask);
	first= 0;
    }

    if(0 <= fd && fd < FD_SETSIZE) {	/* fd must be a legal file descriptor */
	FD_SET(fd,&mask);
	if(fd >= waitall) waitall= fd + 1;
    }

}

/* --------------------------------------------------------------------- */

/* Smasktime: this function sets up a timeval structure - specifies a
 * time limit for waiting.  Note: if seconds < 0 or useconds < 0,
 * or seconds == useconds == 0, then there will be no timeout waiting.
 */
#ifdef __PROTOTYPE__
void Smasktime(long seconds,long useconds)
#else
    void Smasktime(seconds,useconds)
    long seconds;			/* time in seconds			*/
long useconds;			/* time in micro-seconds	*/
#endif
{

    if(seconds < 0 || useconds < 0 || (seconds == 0 && useconds == 0)) {
	/* no timeout (ie. infinite wait) */
	timewait.tv_sec = 0L;
	timewait.tv_usec= 0L;
	timeptr         = (struct timeval *) NULL;
    }

    else { /* set up specified timeout */
	timewait.tv_sec = seconds;
	timewait.tv_usec= (int)useconds;
	timeptr         = &timewait;
    }

}

/* --------------------------------------------------------------------- */

/* Smasktest: this function allows one to test if any Socket in the
 *  mask has something on it
 * Returns: < 0: error
 *          ==0: timeout
 *          > 0: communication data awaiting
 */
int Smasktest()
{
    int ret;
    struct timeval timewait_hold;
    struct timeval *timeptr_hold=NULL;


    /* save current time info */
    timewait_hold   = timewait;
    timeptr_hold    = timeptr;

    /* set up for new time info */
    timewait.tv_sec = 0L;
    timewait.tv_usec= 0L;
    timeptr         = &timewait;

    ret= Smaskwait();

    /* restore time info */
    timewait= timewait_hold;
    timeptr = timeptr_hold;

    return ret;
}

/* --------------------------------------------------------------------- */

/* Smaskunset: this function sets up the mask
 *  A null skt clears the mask
 */
#ifdef __PROTOTYPE__
void Smaskunset(Socket *skt)
#else
    void Smaskunset(skt)
    Socket *skt;
#endif
{

    if(skt) {
	FD_CLR(skt->skt,&mask);
    }
    else {
	waitall= 0;
	FD_ZERO(&mask);
    }

}

/* --------------------------------------------------------------------- */

/* Smaskunfdset: this function sets up the mask
 *  A null skt clears the mask
 */
#ifdef __PROTOTYPE__
void Smaskunfdset(int fd)
#else
    void Smaskunfdset(fd)
    int fd;
#endif
{

    if(fd) FD_CLR(fd,&mask);
    else {
	waitall= 0;
	FD_ZERO(&mask);
    }

}

/* --------------------------------------------------------------------- */

/* Smaskget: this function returns the current mask */
#ifdef __PROTOTYPE__
Smask Smaskget()
#else
    Smask Smaskget()
#endif
{
    Smask smask;

    smask.mask   = mask;
    smask.waitall= waitall;

    return smask;
}

/* --------------------------------------------------------------------- */

/* Smaskuse: this function sets up the mask with the provided mask */
#ifdef __PROTOTYPE__
void Smaskuse(Smask usermask)
#else
    void Smaskuse(usermask)
    Smask usermask;
#endif
{

    mask   = usermask.mask;
    waitall= usermask.waitall;

}

/* --------------------------------------------------------------------- */

typedef struct MaskStack_str MaskStack;
struct MaskStack_str {
    Smask      mask;
    MaskStack *nxt,*prv;
};
static MaskStack *mshd=NULL,*mstl=NULL;

/* Smaskpush: this function pushes the current mask down on a stack
 *  ALSO: the current mask is *cleared*!!!
 */
void Smaskpush()
{
    MaskStack *m;


    /* allocate */
    m= (MaskStack *) malloc(sizeof(MaskStack));

    /* link */
    if(mstl) mstl->nxt= m;
    else     mshd     = m;
    m->prv= mstl;
    mstl  = m;

    /* initialize */
    mstl->mask.mask   = mask;
    mstl->mask.waitall= waitall;

    Smaskset((Socket *) NULL);

}

/* --------------------------------------------------------------------- */

/* Smaskpop: this function pops the stack (or, if the stack is empty,
 *  it will set the current mask to null)
 */
void Smaskpop()
{

    if(mstl) {
	MaskStack *old=mstl;

	Smaskuse(mstl->mask);

	/* delink current tail */
	if(mstl->prv) mstl->prv->nxt= NULL;
	else          mshd          = NULL;
	mstl= mstl->prv;

	free((char *) old);
    }
    else Smaskset((Socket *) NULL);

}

/* --------------------------------------------------------------------- */

/* Smaskisset: this function tests if a specific Socket is read-ready.
 * Assumption: function is called after a Smaskwait() so that the
 * rmask has been set up.
 */
#ifdef __PROTOTYPE__
int Smaskisset(Socket *skt)
#else
    int Smaskisset(skt)
    Socket *skt;
#endif
{
    return FD_ISSET(skt->skt,&rmask);
}

/* --------------------------------------------------------------------- */

/* Smaskfdisset: this function tests if a specific file descriptor is
 * read-ready
 */
#ifdef __PROTOTYPE__
int Smaskfdisset(int fd)
#else
    int Smaskfdisset(fd)
    int fd;
#endif
{
    return FD_ISSET(fd,&rmask);
}

/* --------------------------------------------------------------------- */


/* ---------------------------------------------------------------------
 * vim: ts=4
 */
