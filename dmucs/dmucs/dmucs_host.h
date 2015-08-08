#ifndef _DMUCS_HOST_H_
#define _DMUCS_HOST_H_ 1

/*
 * dmucs_host.h: the DMUCS host definition.
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

#include <sys/types.h>
#include <time.h>
#include <exception>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include "dmucs_dprop.h"


enum host_status_t {
    STATUS_UNKNOWN = 0,
    STATUS_AVAILABLE = 1,
    STATUS_UNAVAILABLE,
    STATUS_OVERLOADED,
    STATUS_SILENT
};

class DmucsHostState;


#define DMUCS_HOST_SILENT_TIME	60	/* if we don't hear from a host for
					   60 seconds, we consider it to be
					   silent, and we remove it from the
					   list of available hosts. */

class DmucsHost
{
private:
    /* dProp_: indicates which kind of host this is.  Users can use
       Dmucs to maintain collections of multiple types of hosts -- e.g.,
       hosts that will only compile solaris binaries vs. those that will only
       compile linux binaries.  Or, hosts that are reserved for certain
       segments of users vs. hosts that are available to everyone.  We call
       this user-defined distinction a "distinguishing property" or "dprop"
       of a host. */
    DmucsHostState *	state_;
    struct in_addr 	ipAddr_;
    DmucsDprop		dprop_;
    std::string		resolvedName_;
    int 		ncpus_;
    int			pindex_;
    float		ldavg1_, ldavg5_, ldavg10_;
    time_t		lastUpdate_;

    friend class DmucsHostState;
    void changeState(DmucsHostState *state);

public:
    DmucsHost(const struct in_addr &ipAddr, DmucsDprop dprop,
	      const int numCpus, const int powerIndex);

    void updateTier(float ldAvg1, float ldAvg5, float ldAvg10);

    void avail();
    void unavail();
    void silent();
    void overloaded();

    static DmucsHost *createHost(const struct in_addr &ipAddr,
				  const DmucsDprop dprop,
				  const std::string &hostsInfoFile);
	static DmucsHost *appendHost(const struct in_addr &ipAddr,
					const DmucsDprop dprop,
					int numCpus,
					int powerIndex,
					const std::string &hostsInfoFile);

    const int getStateAsInt() const;
    int getTier() const;
    int calcTier(float ldavg1, float ldavg5, float ldavg10, int pindex) const;
    const std::string &getName();
    const DmucsDprop getDprop() const { return dprop_; }

	void getLoadAvg(float& ldAvg1, float& ldAvg5, float& ldAvg10) { ldAvg1 = ldavg1_; ldAvg5 = ldavg5_; ldAvg10 = ldavg10_; }
    unsigned int getIpAddrInt() const { return ipAddr_.s_addr; }
    int getNumCpus() const { return ncpus_; }
    bool seemsDown() const;
    bool isUnavailable() const;
    bool isSilent() const;
    bool isOverloaded() const;

    static std::string resolveIp2Name(unsigned int ipAddr, DmucsDprop dprop);
    static const std::string &getName(std::string &resolvedName,
				      const struct in_addr &ipAddr);

    void dump();
};



class DmucsNoMoreHosts : public std::exception
{
    // TODO
};

class DmucsHostNotFound : public std::exception
{
    // TODO
};


#endif
