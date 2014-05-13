/*
 * monitor.cc: the DMUCS monitoring program -- to see how busy the server
 * is, which hosts are allocated, etc.
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
#include "dmucs_host.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fstream>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <iostream>
#include "COSMIC/HDR/sockets.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#if __APPLE__
#include <Foundation/Foundation.h>
#include <Foundation/NSDistributedNotificationCenter.h>
#endif


extern char **environ;
void usage(const char *prog);
void sleep();
void parseResults(const char *resultStr);


#define RESULT_MAX_SIZE		8196
char resultStr[RESULT_MAX_SIZE];
bool debugMode = false;


int
main(int argc, char *argv[])
{
    /*
     * o Open a client socket to the server ip/port.
     * o Send a monitor request.
     * o Read a string in the response.
     * o Parse the string and print out the information in a clean way.
     */

    /*
     * Process command-line arguments.
     *
     * -s <server>, --server <server>: the name of the server machine.
     * -p <port>, --port <port>: the port number to listen on (default: 6714).
     * -D, --debug: debug mode (default: off)
     */
    std::ostringstream serverName;
    serverName << "@" << SERVER_MACH_NAME;
    int serverPortNum = SERVER_PORT_NUM;

    for (int i = 1; i < argc; i++) {
	if (strequ("-s", argv[i]) || strequ("--server", argv[i])) {
	    if (++i >= argc) {
		usage(argv[0]);
		return -1;
	    }
	    serverName.seekp(1);     // remove everything after the first "@".
	    serverName << argv[i] << '\0';
	} else if (strequ("-p", argv[i]) || strequ("--port", argv[i])) {
	    if (++i >= argc) {
		usage(argv[0]);
		return -1;
	    }
	    serverPortNum = atoi(argv[i]);
	} else if (strequ("-D", argv[i]) || strequ("--debug", argv[i])) {
	    debugMode = true;
	} else {
	    usage(argv[0]);
	    return -1;
	}
    }

    std::ostringstream clientPortStr;
    clientPortStr << "c" << serverPortNum;

    char hostname[256];
    if (gethostname(hostname, 256) < 0) {
	fprintf(stderr, "Could not get my hostname\n");
	return -1;
    }
    struct hostent *he = gethostbyname(hostname);
    if (he == NULL) {
	fprintf(stderr, "Could not get my hostname\n");
	return -1;
    }
    struct in_addr in;
    memcpy(&in.s_addr, he->h_addr_list[0], sizeof(in.s_addr));

    while (1) {
	DMUCS_DEBUG((stderr, "doing Sopen with %s, %s\n",
		     serverName.str().c_str(), clientPortStr.str().c_str()));
	Socket *client_sock = Sopen((char *) serverName.str().c_str(),
				    (char *) clientPortStr.str().c_str());
	if (!client_sock) {
	    fprintf(stderr, "Could not open client: %s\n", strerror(errno));
	    Sclose(client_sock);
	    sleep();
	    continue;
	}

	Sputs((char*)"monitor", client_sock);
	resultStr[0] = '\0';
	if (Sgets(resultStr, RESULT_MAX_SIZE, client_sock) == NULL) {
	    fprintf(stderr, "Got error from reading socket.\n");
	    Sclose(client_sock);
	    return -1;
	}
        DMUCS_DEBUG((stderr, "monitor: got -->%s<--\n", resultStr));

	parseResults(resultStr);
	Sclose(client_sock);

	sleep();
    }
}


static void
dumpSummaryInfo(std::ostringstream &availHosts, std::ostringstream &overHosts,
                std::ostringstream &unavailHosts,
                std::ostringstream &silentHosts, std::ostringstream &unkHosts)
{
    if (! availHosts.str().empty())
        std::cout << "Avail: " << availHosts.str() << '\n';
    if (! overHosts.str().empty())
        std::cout << "Overloaded: " << overHosts.str() << '\n';
    if (! unavailHosts.str().empty())
        std::cout << "Unavail: " << unavailHosts.str() << '\n';
    if (! silentHosts.str().empty())
        std::cout << "Silent: " << silentHosts.str() << '\n';
    if (! unkHosts.str().empty())
        std::cout << "Unknown state: " << unkHosts.str() << '\n';
    availHosts.str("");
    overHosts.str("");
    unavailHosts.str("");
    silentHosts.str("");
    unkHosts.str("");
}    


void
parseResults(const char *resultStr)
{
    /*
     * The string will look like this:
     * D: <distinguishingProp> // a string that distinguishes these hosts.
     * H: <ip-addr> <int>      // a host, its ip address, and its state.
     * C <tier>: <ipaddr>/<#cpus>
     *
     * o The state is represented by an integer representing the
     *   host_status_t enum value.
     * o The entire string will end with a \0 (end-of-string) character.
     *
     * We want to parse this information, and print out :
     * <distinguishingProp> hosts:
     * Tier <tier-num>: host/#cpus host/#cpus ...
     * Tier <tier-num>: ...
     * Available Hosts: <host list>.
     * Silent Hosts: host/#cpus ...
     * Unavailable Hosts: host/#cpus ...
     *
     * <repeat above for each distinguishing prop>
     */

    std::istringstream instr(resultStr);
    std::ostringstream unkHosts, availHosts, unavailHosts, overHosts,
        silentHosts;

    while (1) {

        char firstChar;
	instr >> firstChar;
	if (instr.eof()) {
            dumpSummaryInfo(availHosts, overHosts, unavailHosts, silentHosts,
                            unkHosts);
	    break;
	}
	switch (firstChar) {
        case 'D': {

            /* We see a "D" which means we are going to start getting
               information for a new set of hosts.  So, if we have summary
               information on the previous set, print it out now, and
               clear it. */
            dumpSummaryInfo(availHosts, overHosts, unavailHosts, silentHosts,
                            unkHosts);
          
            std::string distProp;
            instr.ignore();		// eat ':'
            instr >> distProp;
            if (distProp == "''")  continue;	// empty distinguishing str
            std::cout << "*** " << distProp << " hosts:" << '\n';
            break;
        }
	case 'H': {
	    std::string ipstr;
	    int state;
	    /* Read in ': <ip-address> <state>' */
	    instr.ignore();		// eat ':'
	    instr >> ipstr >> state;
            std::string hostname = ipstr;
            
	    unsigned int addr = inet_addr(ipstr.c_str());
	    struct hostent *he = gethostbyaddr((char *)&addr, sizeof(addr),
					       AF_INET);
            if (he) {
                hostname = he->h_name;
            }
			else {
				hostname = ipstr;
			}

	    /*
	     * We collect each hostname based on its state, and add it
	     * to an output string.
	     */
		
#if __APPLE__
		NSDistributedNotificationCenter* Notifier = [NSDistributedNotificationCenter defaultCenter];
		NSString* HostName = [NSString stringWithUTF8String: hostname.c_str()];
		NSNumber* StatusNum = [NSNumber numberWithInt:(int)state];
		NSDictionary* Info = [NSDictionary dictionaryWithObjectsAndKeys:HostName, @"HostName", StatusNum, @"Status", nil];
		[Notifier postNotificationName:@"dmucsMonitorHostStatus" object:@"DMUCS" userInfo:Info options:(NSUInteger)NSNotificationPostToAllSessions];
#endif

	    std::ostringstream *ostr;
	    switch (state) {
	    case STATUS_AVAILABLE: ostr = &availHosts; break;
	    case STATUS_UNAVAILABLE: ostr = &unavailHosts; break;
	    case STATUS_OVERLOADED: ostr = &overHosts; break;
	    case STATUS_SILENT: ostr = &silentHosts; break;
	    case STATUS_UNKNOWN:
	    default: ostr = &unkHosts;
	    }
	    *ostr << hostname << ' '; 
	    break;
	}
	case 'C': {

	    std::string line;
	    std::getline(instr, line);

	    std::istringstream linestr(line);
	    
	    int tierNum;
	    linestr >> tierNum;
	    linestr.ignore(2);		// eat the ': '
	    std::string ipAddrAndNCpus;
	    std::cout << "Tier " << tierNum << ": ";

	    // parse one or more "ipaddr/numcpus" fields.
	    while (1) {
		char ipName[32];
		// Read ip address, ending in '/'
		linestr.get(ipName, 32, '/');
		if (! linestr.good()) {
		    break;
		}
		linestr.ignore();		// skip the '/'
		int numCpus;
		linestr >> numCpus;
		linestr.ignore();		// eat ' '

		unsigned int addr = inet_addr(ipName);
		struct hostent *he = gethostbyaddr((char *)&addr,
                                                   sizeof(addr), AF_INET);
                std::string hname;
                if (he == NULL) {
                  hname = ipName;
                } else {
                  hname = he->h_name;
                }
			
#if __APPLE__
		NSDistributedNotificationCenter* Notifier = [NSDistributedNotificationCenter defaultCenter];
		NSString* HostName = [NSString stringWithUTF8String: hname.c_str()];
		NSNumber* TierNum = [NSNumber numberWithInt:(int)tierNum];
		NSNumber* CpusNum = [NSNumber numberWithInt:(int)numCpus];
		NSDictionary* Info = [NSDictionary dictionaryWithObjectsAndKeys:HostName, @"HostName", TierNum, @"Tier", CpusNum, @"Cpus", nil];
		[Notifier postNotificationName:@"dmucsMonitorHostTier" object:@"DMUCS" userInfo:Info options:(NSUInteger)NSNotificationPostToAllSessions];
#endif
			
		std::ostringstream hostname;
		hostname << hname;
		hostname << '/' << numCpus << ' ';
		std::cout << hostname.str();
	    } 
	    std::cout << '\n';
	    break;
	}
	default:
	    std::string line;
	    std::getline(instr, line);
	    std::cout << line << '\n';
	}
    }

    std::cout << '\n' << std::flush;
}


void sleep()
{
    struct timeval t = { 10L, 0L };		// 10 seconds.
    select(0, NULL, NULL, NULL, &t);
}


void
usage(const char *prog)
{
    fprintf(stderr, "Usage: %s [-s|--server <server>] [-p|--port <port>] "
	    "[-D|--debug]\n\n", prog);
}
