#ifndef _DMUCS_DB_H_
#define _DMUCS_DB_H_ 1

/*
 * dmucs_db.h: the DMUCS database object definition
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

#include <set>
#include <map>
#include <list>
#include "dmucs_host.h"
#include <pthread.h>
#include <stdio.h>
#include "COSMIC/HDR/sockets.h"


class DmucsDpropDb
{
private:
    struct DmucsHostCompare {
	bool operator () (DmucsHost *lhs, DmucsHost *rhs) const;
    };
    typedef std::set<DmucsHost *, DmucsHostCompare> dmucs_host_set_t;
    typedef dmucs_host_set_t::iterator dmucs_host_set_iter_t;

    /* Store a list of available cpu ipaddresses -- there will usually be
       more than one instance of a cpu (ipaddress) in the list, as even on
       a single cpu machine we can do more than one compile.

       Each list of IP addresses is a "tier" -- a set of cpus with
       approximately equivalent computational power.  We then have a map
       of these tiers (lists), indexed by an integer, where the lower the
       integer, the less powerful the cpus in that tier. */
    typedef std::list<unsigned int> dmucs_cpus_t;
    typedef dmucs_cpus_t::iterator dmucs_cpus_iter_t;
    typedef std::map<int, dmucs_cpus_t> dmucs_avail_cpus_t;
    typedef dmucs_avail_cpus_t::iterator dmucs_avail_cpus_iter_t;
    typedef dmucs_avail_cpus_t::reverse_iterator dmucs_avail_cpus_riter_t;


    /* This is a mapping from sock address to host ip address -- the socket
       of the connection from the "gethost" application to the dmucs server,
       and the hostip of the cpu assigned to the "gethost" application. */
    typedef std::map<const Socket *, const unsigned int>
    		dmucs_assigned_cpus_t;
    typedef dmucs_assigned_cpus_t::iterator dmucs_assigned_cpus_iter_t;

    /* 
     * Databases of hosts.
     * o a collection of available hosts, sorted by tier.
     * o a collection of assigned hosts.
     * o a collection of silent hosts.
     * o a collection of overloaded hosts.
     *
     * o a collectoin of available (unassigned) cpus.
     * o a collection of assigned cpus.
     */

    DmucsDprop		dprop_;		// the common dprop for all hosts here.
    dmucs_host_set_t 	allHosts_;	// all known hosts are here.

    dmucs_host_set_t	availHosts_;	// avail hosts are also here.
    dmucs_host_set_t	unavailHosts_;	// unavail hosts are also here.
    dmucs_host_set_t 	silentHosts_;	// silent hosts are also here
    dmucs_host_set_t	overloadedHosts_;// overloaded hosts are here.

    dmucs_avail_cpus_t	availCpus_;	// unassigned cpus are here.
    dmucs_assigned_cpus_t assignedCpus_; // assigned cpus are here.

    /* Statistics */
    int numAssignedCpus_;	/* the # of assigned CPUs during a collection
				   period */
    int numConcurrentAssigned_; /* the max number of assigned CPUs at one
				   time. */
    

public:

    DmucsDpropDb(DmucsDprop dprop) :
        dprop_(dprop), numAssignedCpus_(0), numConcurrentAssigned_(0) {}

    DmucsHost * getHost(const struct in_addr &ipAddr);
    bool 	haveHost(const struct in_addr &ipAddr);
    unsigned int getBestAvailCpu();
    void	assignCpuToClient(const unsigned int clientIp,
				  const Socket *cpuIp);
    void 	moveCpus(DmucsHost *host, int oldTier, int newTier);
    int 	delCpusFromTier(int tier, unsigned int ipAddr);

    void 	addNewHost(DmucsHost *host);
    void	releaseCpu(const Socket *sock);

    void 	addToHostSet(dmucs_host_set_t *theSet, DmucsHost *host);
    void 	delFromHostSet(dmucs_host_set_t *theSet, DmucsHost *host);
    void 	addCpusToTier(int tierNum,
                              const unsigned int ipAddr, const int numCpus);

    void 	addToAvailDb(DmucsHost *host);
    void 	delFromAvailDb(DmucsHost *host);
    void 	addToOverloadedDb(DmucsHost *host);
    void 	delFromOverloadedDb(DmucsHost *host);
    void 	addToSilentDb(DmucsHost *host);
    void 	delFromSilentDb(DmucsHost *host);
    void 	addToUnavailDb(DmucsHost *host);
    void 	delFromUnavailDb(DmucsHost *host);
    
    void	handleSilentHosts();
    std::string	serialize();
    void	getStatsFromDb(int *served, int *max, int *totalCpus);
    void	dump();
};

class MutexMonitor
{
 public:
    MutexMonitor(pthread_mutex_t *m) : m_(m)
    {
	pthread_mutex_lock(m_);
    }
    ~MutexMonitor()
    {
	pthread_mutex_unlock(m_);
    }
 private:
    pthread_mutex_t *m_;
};


class DmucsDb
{
private:
    typedef std::map<DmucsDprop, DmucsDpropDb> dmucs_dprop_db_t;
    typedef dmucs_dprop_db_t::iterator dmucs_dprop_db_iter_t;

    /* A map of dmucs databases, indexed by the distinguishing property of
       hosts in the system. */
    dmucs_dprop_db_t  dbDb_;		

    /* A mapping of socket to distinguishing property -- so that when a
       host is released and all we have is the socket information, we can
       figure out which DpropDb to put the host back into. */
    typedef std::map<const Socket *, DmucsDprop> dmucs_sock_dprop_db_t;
    typedef dmucs_sock_dprop_db_t::iterator dmucs_sock_dprop_db_iter_t;

    dmucs_sock_dprop_db_t sock2DpropDb_;

    static DmucsDb *instance_;
    static pthread_mutexattr_t attr_;
    static pthread_mutex_t mutex_;

    DmucsDb();
    virtual ~DmucsDb() {}

public:
    static DmucsDb *getInstance();

    DmucsHost *getHost(const struct in_addr &ipAddr, DmucsDprop dprop) {
	MutexMonitor m(&mutex_);
	dmucs_dprop_db_iter_t itr = dbDb_.find(dprop);
	if (itr == dbDb_.end()) {
	    throw DmucsHostNotFound();
	}
	return itr->second.getHost(ipAddr);
    }
    bool haveHost(const struct in_addr &ipAddr, DmucsDprop dprop)  {
	MutexMonitor m(&mutex_);
	dmucs_dprop_db_iter_t itr = dbDb_.find(dprop);
	if (itr == dbDb_.end()) {
	    return false;
	}
	return itr->second.haveHost(ipAddr);
    }
    unsigned int getBestAvailCpu(DmucsDprop dprop) {
	MutexMonitor m(&mutex_);
	dmucs_dprop_db_iter_t itr = dbDb_.find(dprop);
	if (itr == dbDb_.end()) {
            fprintf(stderr, "nothing in this db!: dprop %s\n",
                         dprop2cstr(dprop));
	    return 0L;		// 32-bits of zeros = 0.0.0.0 
	}
	return itr->second.getBestAvailCpu();
    }
    void assignCpuToClient(const unsigned int clientIp,
                           const DmucsDprop dprop,
                           const Socket *sock);
    void moveCpus(DmucsHost *host, int oldTier, int newTier) {
	MutexMonitor m(&mutex_);
	// Assume the DmucsDpropDb is definitely there.
	return dbDb_.find(host->getDprop())->second.moveCpus(host, oldTier,
							     newTier);
    }
    int delCpusFromTier(DmucsHost *host,
                        int tier, unsigned int ipAddr) {
	MutexMonitor m(&mutex_);
	// Assume the DmucsDpropDb is definitely there.
	return dbDb_.find(host->getDprop())->second.delCpusFromTier(tier,
								    ipAddr);
    }

    void addNewHost(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	DmucsDprop dprop = host->getDprop();
	dmucs_dprop_db_iter_t itr = dbDb_.find(dprop);
	if (itr == dbDb_.end()) {
	    std::pair<dmucs_dprop_db_iter_t, bool> status =
		dbDb_.insert(std::make_pair(dprop, DmucsDpropDb(dprop)));
	    if (!status.second) {
		fprintf(stderr, "%s: could not make a new Dprop-specific db\n",
			__func__);
		return;
	    }
	    itr = status.first;
	}
	return itr->second.addNewHost(host);
    }
    void addToAvailDb(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	return dbDb_.find(host->getDprop())->second.addToAvailDb(host);
    }
    void delFromAvailDb(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	return dbDb_.find(host->getDprop())->second.delFromAvailDb(host);
    };
    void addToOverloadedDb(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	return dbDb_.find(host->getDprop())->second.addToOverloadedDb(host);
    }
    void delFromOverloadedDb(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	return dbDb_.find(host->getDprop())->second.delFromOverloadedDb(host);
    }
    void addToSilentDb(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	return dbDb_.find(host->getDprop())->second.addToSilentDb(host);
    }
    void delFromSilentDb(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	return dbDb_.find(host->getDprop())->second.delFromSilentDb(host);
    }
    void addToUnavailDb(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	return dbDb_.find(host->getDprop())->second.addToUnavailDb(host);
    }
    void delFromUnavailDb(DmucsHost *host) {
	MutexMonitor m(&mutex_);
	return dbDb_.find(host->getDprop())->second.delFromUnavailDb(host);
    }

    void releaseCpu(const Socket *sock);

    void handleSilentHosts() {
	MutexMonitor m(&mutex_);
	for (dmucs_dprop_db_iter_t itr = dbDb_.begin();
             itr != dbDb_.end(); ++itr) {
	    itr->second.handleSilentHosts();
	}
    }
    std::string	serialize() {
	MutexMonitor m(&mutex_);
	std::string res;
	for (dmucs_dprop_db_iter_t itr = dbDb_.begin();
	     itr != dbDb_.end(); ++itr) {
	    res += itr->second.serialize();
	}
	return res;
    }
    void getStatsFromDb(int *served, int *max, int *totalCpus) {
	MutexMonitor m(&mutex_);
        int t_serv, t_max, t_total;
        *served = 0; *max = 0; *totalCpus = 0;
	for (dmucs_dprop_db_iter_t itr = dbDb_.begin();
	     itr != dbDb_.end(); ++itr) {
	    itr->second.getStatsFromDb(&t_serv, &t_max, &t_total);
            *served += t_serv;
            *max += t_max;
            *totalCpus += t_total;
	}
    }
    void	dump();
};


inline bool
DmucsDpropDb::DmucsHostCompare::operator() (DmucsHost *lhs,
					    DmucsHost *rhs) const
{
    // Just do pointer comparison.
    return (lhs->getIpAddrInt() < rhs->getIpAddrInt());
}

#endif


