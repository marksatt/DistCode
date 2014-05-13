/*
 * dmucs_pkt.cc: code to parse a packet coming into the DMUCS server.
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
#include "dmucs_pkt.h"
#include <exception>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class DmucsBadMsg : public std::exception {};


DmucsMsg *
DmucsMsg::parseMsg(Socket *sock, const char *buffer)
{
    struct in_addr clientIp;
    clientIp.s_addr = Speeraddr(sock);
    char dpropstr[DPROP_MAX_STRLEN + 1];
    dpropstr[0] = '\0';		// empty string

    /*
     * The first word in the buffer must be one of: "host", "load",
     * "status", or "monitor".
     */
    if (strncmp(buffer, "host", 4) == 0) {
	/* The string might not be just "host" in which case it will be
	   followed by a dprop to indicate which sub-database to get the
	   host from. */
	int res = sscanf(buffer, "host %s", dpropstr);
	if (res != 1 && res != 0) {
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
	if (sscanf(buffer, "load %s %f %f %f", machname, &ldavg1,
		   &ldavg5, &ldavg10) != 4) {
	    if (sscanf(buffer, "load %s %f %f %f %s", machname, &ldavg1,
		       &ldavg5, &ldavg10, dpropstr) != 5) {
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
	 * status <host-IP-address> up|down [<dprop>]
	 * NOTE: the host-IP-address MUST be in "dot-notation".
	 */
	char machname[64];
	char state[10];
	if (sscanf(buffer, "status %s %s", machname, state) != 2) {
	    if (sscanf(buffer, "status %s %s %s", machname, state,
		       dpropstr) != 3) {
		fprintf(stderr, "Got a bad status msg!!!\n");
		return NULL;
	    }
	}
	fprintf(stderr, "machname %s, state %s, dprop '%s'\n",
		machname, state, dpropstr);
	struct in_addr host;
	host.s_addr = inet_addr(machname);
	host_status_t status;
	if (strncmp(state, "up", 2) == 0) {
	    status = STATUS_AVAILABLE;
	} else if (strncmp(state, "down", 4) == 0) {
	    status = STATUS_UNAVAILABLE;
	} else {
	    fprintf(stderr, "got unknown state %s\n", state);
	}
	return new DmucsStatusMsg(clientIp, host, status, dpropstr);
    } else if (strncmp(buffer, "monitor", 7) == 0) {
	return new DmucsMonitorReqMsg(clientIp, dpropstr);
    }

    fprintf(stderr, "request not recognized: ->%s<-\n", buffer);
    return NULL;
}
