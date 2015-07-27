/*
 * gethost.cc: The program that send a loadavg message to the DMUCS
 * server periodically.
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
#include "COSMIC/HDR/sockets.h"
#include <time.h>
#include <sys/time.h>
#include <errno.h>


extern char **environ;
void usage(const char *prog);
void sleep();


bool debugMode = false;

int
main(int argc, char *argv[])
{
    /*
     * o Open a client socket to the server ip/port.
     * o Send my IP address in a host request.
     * o Read an IP address in a response.
     * o Assign the value DISTCC_HOSTS to the IP address in the env.
     * o Use execve to run the command passed in, with its args, on the
     *   command line.
     * o Wait for the command to finish.
     * o Close the client socket.
     */


    /*
     * Process command-line arguments.
     *
     * -s <server>, --server <server>: the name of the server machine.
     * -p <port>, --port <port>: the port number to listen on (default: 6714).
     * -t <distinguishing-type-str>: a string that indicates which type
     *    of host the compilation machine is.
     * -D, --debug: debug mode (default: off)
     */
    std::ostringstream serverName;
    serverName << "@" << SERVER_MACH_NAME;
    int serverPortNum = SERVER_PORT_NUM;
    std::string distingProp = "";

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
	} else if (strequ("-t", argv[i]) || strequ("--type", argv[i])) {
	    if (++i >= argc) {
		usage(argv[0]);
		return -1;
	    }
	    distingProp = argv[i];
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

	DMUCS_DEBUG((stderr, "got socket: %s\n", Sprtskt(client_sock)));

	FILE *output = popen("uptime", "r");
	if (output == NULL) {
	    fprintf(stderr, "Failed to get load avg\n");
	    Sclose(client_sock);
	    sleep();
	    continue;
	}
	char buf[1024];
	char ldStr[32];
	if (fgets(buf, 1024, output) == NULL) {
	    strcpy(ldStr," 0.0 0.0 0.0");
	} else {
	    float ldAvg1 = 0.0, ldAvg5 = 0.0, ldAvg10 = 0.0;

	    std::istringstream ist(buf);

	    std::string word;
	    bool found = false;
	    while (ist >> word) {
		if (word == "average:" || word == "averages:") {
		    found = true;
		    break;
		}
	    }
	    if (found) {
		ist >> ldAvg1;
		ist.get();		// eat the ','
		ist >> ldAvg5;
		ist.get();		// eat the ','
		ist >> ldAvg10;
	    } else {
		fprintf(stderr, "Couldn't read loadavg results -->%s<--\n",
			buf);
	    }
	    sprintf(ldStr, " %2.2f %2.2f %2.2f", ldAvg1, ldAvg5, ldAvg10);
	}
	pclose(output);

	std::string clientReqStr = "load " + std::string(inet_ntoa(in)) +
            std::string(ldStr) + std::string(" ") + distingProp;
	DMUCS_DEBUG((stderr, "Writing -->%s<-- to the server\n",
                     clientReqStr.c_str()));

	Sputs((char *) clientReqStr.c_str(), client_sock);
	Sclose(client_sock);

	sleep();
    }
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
	    "[-t|--type <str>] [-D|--debug]\n\n", prog);
}
