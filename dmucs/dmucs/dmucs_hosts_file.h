#ifndef _DMUCS_HOST_FILE_H_
#define _DMUCS_HOST_FILE_H_ 1

/*
 * dmucs_hosts_file.h: code to read the hosts-info configuration file.
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

#include <map>
#include <string>

#ifdef PKGDATADIR
const std::string HOSTS_INFO_FILE = std::string(PKGDATADIR) + \
	std::string("/") + std::string("hosts-info");
#else
const std::string HOSTS_INFO_FILE = std::string(getenv("HOME")) + std::string("/.dmucs/hosts-info");
#endif


class DmucsHostsFile
{
public:

    static DmucsHostsFile *getInstance(const std::string &hostsInfoFile);
    bool getDataForHost(const struct in_addr &ipAddr, int *numCpus,
			int *powerIndex) const;
	bool setDataForHost(const struct in_addr ipAddr, int numCpus,
						int powerIndex) const;

    /*
     * This class implements the singleton pattern, as only one instance of
     * it needs to exist, and it should take care to re-read the hostsfile,
     * etc., when necessary.
     */

private:
    // this is private so that it cannot be used (this is a Singleton).
    DmucsHostsFile(const std::string &hostsInfoFile);	
    ~DmucsHostsFile();

    struct info_t {
	int numCpus_;
	int powerIndex_;
	info_t(int ncpus, int pindex) :
	    numCpus_(ncpus), powerIndex_(pindex) {};
    };

    static DmucsHostsFile *instance_;
    std::string hostsInfoFile_;

    /*
     * This maps an IP address (in 32-bit format) to the pair of values:
     * numCpus and powerIndex.
     */
    typedef std::map<unsigned int, info_t> host_info_db_t;
    typedef host_info_db_t::iterator host_info_db_iter_t;

    mutable host_info_db_t db_;
    mutable time_t lastFileChangeTime_;

    void readFileIntoDb() const;

    /* If the file modification time has changed since the last time this
       was called, then return true AND update lastFileChangeTime_ to the
       new modification time. */
    bool hasFileChanged() const;

};

#endif

