#ifndef _DMUCS_PKT_H_
#define _DMUCS_PKT_H_

/*
 * dmucs_msg.h: definition of a DMUCS packet received by the DMUCS server.
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

#include "COSMIC/HDR/sockets.h"

enum dmucs_req_t {
    HOST_REQ,
    LOAD_AVERAGE_INFORM,
    STATUS_INFORM,
    MONITOR_REQ
};


/*
 * Format of packets that come in to the dmucs server:
 *
 * o host request:   "host <client IP address>
 * o load average:   "load <host IP address> <3 floating pt numbers>"
 * o status message: "status <host IP address> up|down [n <numCpus>]
 *		[p <powerIndex>]"
 * o monistor req:   "monitor <client IP address>"
 */

#include "dmucs_host.h"
#include "dmucs_dprop.h"

class DmucsMsg {
private:
protected:
    struct in_addr clientIp_;
    DmucsDprop dprop_;

    DmucsMsg(struct in_addr clientIp, DmucsDprop dprop) :
	clientIp_(clientIp), dprop_(dprop) {}
	
public:
    /* Factory Method: parseMsg */
    static DmucsMsg *parseMsg(Socket *sock, const char *buf);

    virtual void handle(Socket *sock, const char *buf) = 0;
	
	virtual ~DmucsMsg(){}
};


class DmucsStatusMsg : public DmucsMsg
{
private:
    struct in_addr host_;
    host_status_t status_;
    int numCpus_;
    int powerIndex_;

public:
    DmucsStatusMsg(struct in_addr clientIp, struct in_addr host,
		   host_status_t status, DmucsDprop dprop) :
	DmucsMsg(clientIp, dprop), 
	host_(host), status_(status), numCpus_(1), powerIndex_(1) {}
	virtual ~DmucsStatusMsg(){}
    void handle(Socket *sock, const char *buf);
};


class DmucsRemoteAddMsg : public DmucsMsg
{
private:
	struct in_addr host_;
	host_status_t status_;
	int numCpus_;
	int powerIndex_;
	
public:
	DmucsRemoteAddMsg(struct in_addr clientIp, struct in_addr host,
				   host_status_t status, DmucsDprop dprop, int numcpus, int powerindex) :
	DmucsMsg(clientIp, dprop),
	host_(host), status_(status), numCpus_(numcpus), powerIndex_(powerindex) {}
	virtual ~DmucsRemoteAddMsg(){}
	void handle(Socket *sock, const char *buf);
};


class DmucsLdAvgMsg : public DmucsMsg
{
private:
    struct in_addr host_;
    float ldAvg1_, ldAvg5_, ldAvg10_;

public:
    DmucsLdAvgMsg(struct in_addr clientIp, struct in_addr host,
		  float ldavg1, float ldavg5,
		  float ldavg10, DmucsDprop dprop) :
	DmucsMsg(clientIp, dprop), 
	host_(host), ldAvg1_(ldavg1), ldAvg5_(ldavg5), ldAvg10_(ldavg10) {}
	virtual ~DmucsLdAvgMsg(){}
    void handle(Socket *sock, const char *buf);
};


class DmucsHostReqMsg : public DmucsMsg
{
public:
    DmucsHostReqMsg(struct in_addr clientIp, DmucsDprop dprop) :
	DmucsMsg(clientIp, dprop) {}
	virtual ~DmucsHostReqMsg(){}
    void handle(Socket *sock, const char *buf);
};


class DmucsMonitorReqMsg : public DmucsMsg
{
public:
    DmucsMonitorReqMsg(struct in_addr clientIp, DmucsDprop dprop) :
	DmucsMsg(clientIp, dprop) {};
	virtual ~DmucsMonitorReqMsg(){}
    void handle(Socket *sock, const char *buf);
};


#define BUFSIZE 1024	// largest info we will read from the socket.

#endif
