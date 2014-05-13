#ifndef _DMUCS_HOST_STATE_H_
#define _DMUCS_HOST_STATE_H_ 1

/*
 * dmucs_host_state.cc: the DMUCS host state definition.
 *
 * Copyright (C) 2005, 2006  Victor T. Norman
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#include "dmucs_host.h"



/*
 * States of DmucsHosts:
 * o available -- we got a status "avail" message for the host and/or we are
 *   periodically receiving load average messages from it.  (Note: we may
 *   not always receive the status message -- when the host comes up before
 *   the Dmucs server.  In that case, we treat the load average message as
 *   a status "avail" message.
 * o unavailable -- we got a status "unavail" message from the host.  We
 *   may still be receiving load average messages from it, but it has been
 *   made "administratively" unavailable.
 * o overloaded -- the load average messages we receive indicate that the
 *   machine is very busy, and shouldn't be used for compiles.
 * o silent -- we got a status "avail" message from the host, but now we
 *   are not getting any load average messages from it.
 */

class DmucsHostState
{
public:
    virtual void avail(DmucsHost *host) {}
    virtual void unavail(DmucsHost *host) {}
    virtual void silent(DmucsHost *host) {}
    virtual void overloaded(DmucsHost *host) {}
    virtual void addToDb(DmucsHost *host) {}
    virtual void removeFromDb(DmucsHost *host) {}
    virtual const char *dump() { return "Unknown"; }
    virtual int asInt() { return (int) STATUS_UNKNOWN; }

protected:
    void changeState(DmucsHost *host, DmucsHostState *newState) {
	host->changeState(newState);
    }
};


class DmucsHostStateAvail : public DmucsHostState
{
public:
    virtual void unavail(DmucsHost *host);
    virtual void silent(DmucsHost *host);
    virtual void overloaded(DmucsHost *host);
    virtual int asInt() { return (int) STATUS_AVAILABLE; }
    virtual void addToDb(DmucsHost *host);
    virtual void removeFromDb(DmucsHost *host);
    virtual const char *dump() { return "Available"; }

    static DmucsHostStateAvail *getInstance();

private:
    DmucsHostStateAvail() {}

    static DmucsHostStateAvail *instance_;
};

class DmucsHostStateUnavail : public DmucsHostState
{
public:
    virtual void avail(DmucsHost *host);
    virtual int asInt() { return (int) STATUS_UNAVAILABLE; }
    virtual void addToDb(DmucsHost *host);
    virtual void removeFromDb(DmucsHost *host);
    virtual const char *dump() { return "Unavail"; }

    static DmucsHostStateUnavail *getInstance();

private:
    DmucsHostStateUnavail() {}
    
    static DmucsHostStateUnavail *instance_;
};

class DmucsHostStateSilent : public DmucsHostState
{
public:
    virtual void avail(DmucsHost *host);
    virtual void unavail(DmucsHost *host);
    virtual int asInt() { return (int) STATUS_SILENT; };
    virtual void addToDb(DmucsHost *host);
    virtual void removeFromDb(DmucsHost *host);
    virtual const char *dump() { return "Silent"; }

    static DmucsHostStateSilent *getInstance();

private:
    DmucsHostStateSilent() {}
    
    static DmucsHostStateSilent *instance_;
};

class DmucsHostStateOverloaded : public DmucsHostState
{
public:
    virtual void avail(DmucsHost *host);
    virtual void unavail(DmucsHost *host);
    virtual void silent(DmucsHost *host);
    virtual int asInt() { return (int) STATUS_OVERLOADED; }
    virtual void addToDb(DmucsHost *host);
    virtual void removeFromDb(DmucsHost *host);
    virtual const char *dump() { return "Overloaded"; }

    static DmucsHostStateOverloaded *getInstance();

private:
    DmucsHostStateOverloaded() {}
    
    static DmucsHostStateOverloaded *instance_;
};

#endif

