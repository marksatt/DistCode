/*
 * dmucs_host_state.cc: the DMUCS host state implementation.
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

#include "dmucs_host_state.h"
#include "dmucs_db.h"



DmucsHostStateAvail *DmucsHostStateAvail::instance_ = NULL;


DmucsHostStateAvail *
DmucsHostStateAvail::getInstance()
{
    if (instance_ == NULL) {
	instance_ = new DmucsHostStateAvail();
    }
    return instance_;
}


void
DmucsHostStateAvail::unavail(DmucsHost *host)
{
    /* Remove the CPUs from the cpus database. */
    DmucsDb::getInstance()->delCpusFromTier(host, host->getTier(),
					    host->getIpAddrInt());
    /* Move the host to the overloaded state: remove from availHosts_ and
       add to overloadedHosts_. */
    removeFromDb(host);
    DmucsHostState::changeState(host, DmucsHostStateUnavail::getInstance());
}


void
DmucsHostStateAvail::silent(DmucsHost *host)
{
    /* Remove the CPUs from the cpus database. */
    DmucsDb::getInstance()->delCpusFromTier(host, host->getTier(),
					    host->getIpAddrInt());
    /* Move the host to the overloaded state: remove from availHosts_ and
       add to overloadedHosts_. */
    removeFromDb(host);
    DmucsHostState::changeState(host, DmucsHostStateSilent::getInstance());
}


void
DmucsHostStateAvail::overloaded(DmucsHost *host)
{
    /* Remove the CPUs from the cpus database. */
    DmucsDb::getInstance()->delCpusFromTier(host, host->getTier(),
					    host->getIpAddrInt());
    /* Move the host to the overloaded state: remove from availHosts_ and
       add to overloadedHosts_. */
    removeFromDb(host);
    DmucsHostState::changeState(host, DmucsHostStateOverloaded::getInstance());
}


void
DmucsHostStateAvail::removeFromDb(DmucsHost *host)
{
    DmucsDb::getInstance()->delFromAvailDb(host);
}

void
DmucsHostStateAvail::addToDb(DmucsHost *host)
{
    DmucsDb::getInstance()->addToAvailDb(host);
}


/* ====================================================================== */

DmucsHostStateUnavail *DmucsHostStateUnavail::instance_ = NULL;

DmucsHostStateUnavail *
DmucsHostStateUnavail::getInstance()
{
    if (instance_ == NULL) {
	instance_ = new DmucsHostStateUnavail();
    }
    return instance_;
}

void
DmucsHostStateUnavail::avail(DmucsHost *host)
{
    removeFromDb(host);

    int tier = host->getTier();
    if (tier == 0) {
	DmucsHostState::changeState(host,
				    DmucsHostStateOverloaded::getInstance());
    } else {
	DmucsHostState::changeState(host, DmucsHostStateAvail::getInstance());
    }
}


void
DmucsHostStateUnavail::addToDb(DmucsHost *host)
{
    DmucsDb::getInstance()->addToUnavailDb(host);
}


void
DmucsHostStateUnavail::removeFromDb(DmucsHost *host)
{
    DmucsDb::getInstance()->delFromUnavailDb(host);
}


/* ====================================================================== */

DmucsHostStateSilent *DmucsHostStateSilent::instance_ = NULL;

DmucsHostStateSilent *
DmucsHostStateSilent::getInstance()
{
    if (instance_ == NULL) {
	instance_ = new DmucsHostStateSilent();
    }
    return instance_;
}

void
DmucsHostStateSilent::avail(DmucsHost *host)
{
    removeFromDb(host);
    DmucsHostState::changeState(host, DmucsHostStateAvail::getInstance());
}


void
DmucsHostStateSilent::unavail(DmucsHost *host)
{
    removeFromDb(host);
    DmucsHostState::changeState(host, DmucsHostStateUnavail::getInstance());
}


void
DmucsHostStateSilent::addToDb(DmucsHost *host)
{
    DmucsDb::getInstance()->addToSilentDb(host);
}


void
DmucsHostStateSilent::removeFromDb(DmucsHost *host)
{
    DmucsDb::getInstance()->delFromSilentDb(host);
}



/* ====================================================================== */


DmucsHostStateOverloaded *DmucsHostStateOverloaded::instance_ = NULL;

DmucsHostStateOverloaded *
DmucsHostStateOverloaded::getInstance()
{
    if (instance_ == NULL) {
	instance_ = new DmucsHostStateOverloaded();
    }
    return instance_;
}

void
DmucsHostStateOverloaded::avail(DmucsHost *host)
{
    removeFromDb(host);
    DmucsHostState::changeState(host, DmucsHostStateAvail::getInstance());
}


void
DmucsHostStateOverloaded::unavail(DmucsHost *host)
{
    removeFromDb(host);
    DmucsHostState::changeState(host, DmucsHostStateUnavail::getInstance());
}


void
DmucsHostStateOverloaded::silent(DmucsHost *host)
{
    removeFromDb(host);
    DmucsHostState::changeState(host, DmucsHostStateSilent::getInstance());
}


void
DmucsHostStateOverloaded::addToDb(DmucsHost *host)
{
    DmucsDb::getInstance()->addToOverloadedDb(host);
}


void
DmucsHostStateOverloaded::removeFromDb(DmucsHost *host)
{
    DmucsDb::getInstance()->delFromOverloadedDb(host);
}
