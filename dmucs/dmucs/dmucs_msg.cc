/*
 * dmucs_msg.cc: code to parse a packet coming into the DMUCS server.
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

#include "dmucs.h"
#include "dmucs_msg.h"
#include "dmucs_db.h"
#include <exception>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

extern std::string hostsInfoFile;



DmucsMsg *
DmucsMsg::parseMsg(Socket *sock, const char *buffer)
{
    struct in_addr clientIp;
    clientIp.s_addr = (in_addr_t)Speeraddr(sock);
    char dpropstr[DPROP_MAX_STRLEN + 1];
    dpropstr[0] = '\0';		// empty string

    /*
     * The first word in the buffer must be one of: "host", "load",
     * "status", or "monitor".
     */
    if (strncmp(buffer, "host", 4) == 0) {
        /* The string is "host <clientIpAddr> [<typeStr>]" where the
           typeStr is an optional string that is the distinguishing property
           of the host the client wants. */
        char cliIpStr[64];
	int res = sscanf(buffer, "host %s %s", cliIpStr, dpropstr);
	if (res != 2 && res != 1) {
	    fprintf(stderr, "Got a bad host request message ->%s<--\n",buffer);
	    return NULL;
	}
	return new DmucsHostReqMsg(clientIp, dpropstr);
    } else if (strncmp(buffer, "load", 4) == 0) {

	/* The buffer must hold:
	 * load <host-IP-address> <3 floating pt numbers>
	 * followed by an optional <dprop>.
	 */
	char machname[64];
	float ldavg1, ldavg5, ldavg10;
        if (sscanf(buffer, "load %s %f %f %f %s", machname, &ldavg1,
                   &ldavg5, &ldavg10, dpropstr) != 5) {
            if (sscanf(buffer, "load %s %f %f %f", machname, &ldavg1,
                       &ldavg5, &ldavg10) != 4) {
                fprintf(stderr, "Got a bad load avg msg!!!\n");
		return NULL;
	    }
	}
	struct in_addr host;
	host.s_addr = inet_addr(machname);
	DMUCS_DEBUG((stderr, "host %s: ldAvg1 %2.2f, ldAvg5 %2.2f, "
		     "ldAvg10 %2.2f, dprop '%s'\n",
		     machname, ldavg1, ldavg5, ldavg10, dpropstr));
	return new DmucsLdAvgMsg(clientIp, host,
				  ldavg1, ldavg5, ldavg10, dpropstr);
    } else if (strncmp(buffer, "status", 6) == 0) {
	/* The buffer must hold:
	 * status <host-IP-address> add|up|down [<dprop>] [numcpus] [poweridx]
	 * NOTE: the host-IP-address MUST be in "dot-notation".
	 */
	char machname[64];
	char state[10];
	int numcpus = 1;
	int poweridx = 1;
		if (sscanf(buffer, "status %s %s %s %d %d", machname, state, dpropstr, &numcpus, &poweridx)!= 5) {
			dpropstr[0] = '\0';
			if (sscanf(buffer, "status %s %s %d %d", machname, state, &numcpus, &poweridx)!= 4) {
				if (sscanf(buffer, "status %s %s %s", machname, state, dpropstr)!= 3) {
					if (sscanf(buffer, "status %s %s", machname, state) != 2) {
						fprintf(stderr, "Got a bad status msg!!!\n");
						return NULL;
				}
			}
		}
	}
	fprintf(stderr, "machname %s, state %s, dprop '%s'\n",
		machname, state, dpropstr);
	struct in_addr host;
	host.s_addr = inet_addr(machname);
	host_status_t status = STATUS_UNKNOWN;
	if (strncmp(state, "add", 2) == 0) {
		status = STATUS_AVAILABLE;
	} else if (strncmp(state, "up", 2) == 0) {
	    status = STATUS_AVAILABLE;
	} else if (strncmp(state, "down", 4) == 0) {
	    status = STATUS_UNAVAILABLE;
	} else {
	    fprintf(stderr, "got unknown state %s\n", state);
	}
	if (strncmp(state, "add", 2) == 0) {
		return new DmucsRemoteAddMsg(clientIp, host, status, dpropstr, numcpus, poweridx);
	} else {
		return new DmucsStatusMsg(clientIp, host, status, dpropstr);
	}
    } else if (strncmp(buffer, "monitor", 7) == 0) {
	return new DmucsMonitorReqMsg(clientIp, dpropstr);
    }

    fprintf(stderr, "request not recognized: ->%s<-\n", buffer);
    return NULL;
}


void
DmucsHostReqMsg::handle(Socket *sock, const char *buf)
{
    DMUCS_DEBUG((stderr, "Got host request: -->%s<--\n", buf));

    DmucsDb *db = DmucsDb::getInstance();
    unsigned int cpuIpAddr = 0;

    cpuIpAddr = db->getBestAvailCpu(dprop_);
    if(cpuIpAddr > 0) {
	std::string resolved_name =
	    DmucsHost::resolveIp2Name(cpuIpAddr, dprop_);

	fprintf(stderr, "Giving out %s\n", resolved_name.c_str());

	db->assignCpuToClient(cpuIpAddr, dprop_, sock);
#if 0
	fprintf(stderr, "The databases are now:\n");
	db->dump();
#endif

    }
    else {
	/* getBestAvailCpu() might return 0, when there are
	   no more available CPUs.  We send 0.0.0.0 to the client
	   but we don't record it as an assigned cpu. */
        fprintf(stderr, "!!!!!      Out of hosts in db \"%s\"   !!!!!\n",
                dprop2cstr(dprop_));
        fprintf(stderr, "!!!!!  Some other error: %s!!!!!\n",
                strerror(errno));
    }

    struct in_addr c;
    c.s_addr = cpuIpAddr;
    Sputs(inet_ntoa(c), sock);
}


void
DmucsLdAvgMsg::handle(Socket *sock, const char *buf)
{
    DmucsDb *db = DmucsDb::getInstance();
    DMUCS_DEBUG((stderr, "Got load average mesg\n"));

	std::string hostname;
	DmucsHost *host = db->getHost(host_, dprop_);
	
    if(host) {
		
		hostname = host->getName();
        host->updateTier(ldAvg1_, ldAvg5_, ldAvg10_);
		/* If the host hasn't been explicitly made unavailable,
		 then make it available.  If the host is overloaded
		 but isn't anymore, then make it available. */
        if (host->isSilent() ||
            (host->isOverloaded() && host->getTier() != 0)) {
			host->avail();      // make sure the host is available
		}
    } else {
		host = DmucsHost::createHost(host_, dprop_, hostsInfoFile);
		if(host)
		{
			hostname = host->getName();
			host->updateTier(ldAvg1_, ldAvg5_, ldAvg10_);
			fprintf(stderr, "New host available: %s/%d, tier %d, type %s\n",
					host->getName().c_str(), host->getNumCpus(), host->getTier(),
					dprop2cstr(dprop_));
		}
    }
	
    removeFd(sock);
}


void
DmucsStatusMsg::handle(Socket *sock, const char *buf)
{
    DmucsDb *db = DmucsDb::getInstance();
    if (status_ == STATUS_AVAILABLE) {
	if (db->haveHost(host_, dprop_)) {
	    /* Make it available (if it wasn't). */
	    db->getHost(host_, dprop_)->avail();
	} else {
	    /* A new host is available! */
	    DMUCS_DEBUG((stderr, "Creating new host %s, type %s\n",
			 inet_ntoa(host_), dprop2cstr(dprop_)));
		DmucsHost *newHost = DmucsHost::createHost(host_, dprop_, hostsInfoFile);
		newHost->avail();
	}
    } else {    // status is unavailable.
        DmucsHost * host = db->getHost(host_, dprop_);
		if(host) {
			host->unavail();
		}
    }
    removeFd(sock);
}


void
DmucsRemoteAddMsg::handle(Socket *sock, const char *buf)
{
	DmucsDb *db = DmucsDb::getInstance();
	if (status_ == STATUS_AVAILABLE) {
		if (db->haveHost(host_, dprop_)) {
			/* Make it available (if it wasn't). */
			db->getHost(host_, dprop_)->avail();
		} else {
			/* A new host is available! */
			DMUCS_DEBUG((stderr, "Creating new host %s, type %s, cpus %d, index %d\n",
						 inet_ntoa(host_), dprop2cstr(dprop_), numCpus_, powerIndex_));
			DmucsHost *newHost = DmucsHost::appendHost(host_, dprop_, numCpus_, powerIndex_, hostsInfoFile);
			if(newHost)
			{
				newHost->avail();
			}
		}
	} else {    // status is unavailable.
        DmucsHost* host = db->getHost(host_, dprop_);
        if(host) {
			host->unavail();
		}
	}
	removeFd(sock);
}

void
DmucsMonitorReqMsg::handle(Socket *sock, const char *buf)
{
    std::string str = DmucsDb::getInstance()->serialize();
    Sputs((char *) str.c_str(), sock);
    removeFd(sock);
}
