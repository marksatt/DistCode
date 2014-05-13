/*
 * dmucs_hosts_file.cc: code to read the hosts-info configuration file.
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
#include "dmucs_hosts_file.h"
#include <fstream>
#include <iostream>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <exception>
#include <sys/stat.h>


DmucsHostsFile *DmucsHostsFile::instance_ = NULL;


DmucsHostsFile *DmucsHostsFile::getInstance(const std::string &file)
{
    if (instance_ == NULL) {
	instance_ = new DmucsHostsFile(file);
    }
    return instance_;
}


DmucsHostsFile::DmucsHostsFile(const std::string &hostsInfoFile) :
    lastFileChangeTime_(0),
    hostsInfoFile_(hostsInfoFile)
{
    readFileIntoDb();
    /* We know the file has changed since time 0 -- so just call this
       to get the new time into lastFileChangeTime_. */
    (void) hasFileChanged();
}


void
DmucsHostsFile::readFileIntoDb() const
{
    db_.clear();

    std::ifstream instr(hostsInfoFile_.c_str());
    if (!instr) {
	DMUCS_DEBUG((stderr, "Unable to open hosts-info file \"%s\"\n",
		     hostsInfoFile_.c_str()));
	return;
    }

    for (int lineno = 1; ; lineno++) {

	/*
	 * Each line is: machine-or-ipaddr numcpus power-index.
	 * Comment lines start with #, and these are skipped.  Lines
	 * containing only whitespace are also skipped.
	 */
	char firstChar;
	instr >> firstChar;		// this will skip empty lines.
	if (instr.eof()) {
	    break;
	}
	if (firstChar == '#') {
	    std::string junk;
	    std::getline(instr, junk);
	    continue;
	} else if (! instr.good()) {
	    break;
	} else {
	    instr.putback(firstChar);
	}

	std::string line;
	std::getline(instr, line);

	char machine[256];
	int numcpus, powerIndex;
	if (sscanf(line.c_str(), "%s %d %d", machine, &numcpus,
		   &powerIndex) != 3) {
	    std::cout << "Bad input in line " << lineno << " of file " <<
		hostsInfoFile_ << std::endl;
	    break;
	}

	/*
	 * Convert the machine name into an ip address.
	 */
	unsigned long addr;
	struct hostent *hostent;
	if ((int)(addr = inet_addr(machine)) != -1) {
	    hostent = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	} else {
	    hostent = gethostbyname(machine);
	}
	if (hostent == NULL) {
	    std::cout << "Could not get IP address for machine: " << machine <<
		".  Skipping." << std::endl;
	    continue;
	}

	struct in_addr in;
	memcpy(&in.s_addr, hostent->h_addr_list[0], sizeof(in.s_addr));

	/* Insert the info into the database of host info. */
	host_info_db_t::value_type object(in.s_addr,
					  info_t(numcpus, powerIndex));
	db_.insert(object);
    }
}


bool
DmucsHostsFile::getDataForHost(const struct in_addr &ipAddr, int *numCpus,
			       int *powerIndex) const
{
	bool found = false;
    if (hasFileChanged()) {
	readFileIntoDb();
    }

    /*
     * Be default, we'll assume 1 CPU and a very slow one at that.
     */
    *numCpus = 1;
    *powerIndex = 1;

    host_info_db_iter_t itr = db_.find(ipAddr.s_addr);
	if (itr == db_.end())
	{
		// Not found - try rereading the db & finding it again before assuming 1 CPU
		readFileIntoDb();
		itr = db_.find(ipAddr.s_addr);
	}
    if (itr != db_.end()) {
		*numCpus = itr->second.numCpus_;
		*powerIndex = itr->second.powerIndex_;
		found = true;
    }
	return found;
}



/* If the file modification time has changed since the last time this
   was called, then return true AND update lastFileChangeTime_ to the
   new modification time. */
bool
DmucsHostsFile::hasFileChanged() const
{
    struct stat st;
    if (stat(hostsInfoFile_.c_str(), &st) != 0) {
	return true;		// assume the file has changed...
    }

    if (lastFileChangeTime_ != st.st_ctime) {
	lastFileChangeTime_ = st.st_ctime;
	return true;
    }
    return false;
}
