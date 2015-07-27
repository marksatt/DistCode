
/* 
 * Copyright (c) 1994 - 2002 Marconi Communications, Inc. as an unpublished
 * work.
 * 
 * All rights reserved.
 * 
 * File: test.cc - 
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <fstream>
#include <stdio.h>
#include <map>

struct host_info_t {
    int numCpus_;
    int powerIndex_;
    host_info_t(int numcpus, int pindex) :
	numCpus_(numcpus), powerIndex_(pindex) {}
};

typedef std::map<const unsigned int, host_info_t> host_info_db_t;
typedef host_info_db_t::iterator host_info_db_iter_t;

host_info_db_t db_;

#include <iostream>
#include <sstream>

int
main(int argc, char *argv[])
{
#if 0
    std::ifstream instr("/us/norman/hosts-info");
    if (!instr) {
	cout << "not instr" << endl;
	return -1;
    }

    std::string machine;
    int numcpus, pindex;
    for (int lineno = 1; ; lineno++) {
	instr >> machine >> numcpus >> pindex;
	if (instr.eof()) {
	    cout << "EOF" << endl;
	    break;
	}
	if (! instr.good()) {
	    cout << "bad input in line " << lineno << endl;
	    break;
	}
	cout << "machine: " << machine << " numcpus: " << numcpus <<
	    " pindex: " << pindex << endl;

	/*
	 * Convert the machine name into an ip address.
	 */
	unsigned long addr;
	struct hostent *hostent;
	if ((int)(addr = inet_addr(machine.c_str())) != -1) {
	    hostent = gethostbyaddr((char *)&addr, sizeof(addr), AF_INET);
	} else {
	    hostent = gethostbyname(machine.c_str());
	}
	if (hostent == NULL) {
	    cout << "Could not get IP address for machine: " << machine <<
		".  Skipping." << endl;
	    continue;
	}

	struct in_addr in;
	memcpy(&in.s_addr, hostent->h_addr_list[0], sizeof(in.s_addr));

	host_info_db_t::value_type obj(in.s_addr,
				       host_info_t(numcpus, pindex));
	db_.insert(obj);
    }

    for (host_info_db_iter_t itr = db_.begin(); itr != db_.end(); ++itr) {
	printf("address %s --> (numcpus %d, pindex %d)\n",
	       inet_ntoa(*(const struct in_addr *) &itr->first),
	       itr->second.numCpus_,
	       itr->second.powerIndex_);
    }

    return 0;
#endif


    std::ostringstream o;

    for (int i = 1; i < argc; i++) {
	o << argv[i] << " ";
    }
    std::cout << o.str() << std::endl;

    system(o.str().c_str());
}
