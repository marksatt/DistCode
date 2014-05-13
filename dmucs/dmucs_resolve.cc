/*
 * dmucs_resolve.cc: Helperfunction to reslove host names
 *
 * Copyright (C) 2006  Patrik Olesen
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

#include "config.h"
#include <string>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <netdb.h>
#include <pthread.h>

#ifndef HAVE_GETHOSTBYADDR_R
#ifdef HAVE_GETHOSTBYADDR
static pthread_mutex_t gethost_mutex;
static int gethost_mutex_inited = 0;
#endif /* HAVE_GETHOSTBYADDR */
#endif /* !HAVE_GETHOSTBYADDR_R */

const std::string &
getHostName(std::string &resolvedName, const struct in_addr &ipAddr)
{
    int res8 = 0;
    struct hostent he, *res = 0;
    char buffer[128];

#if HAVE_GETHOSTBYADDR_R
#if HAVE_GETHOSTBYADDR_R_7_ARGS
    res = gethostbyaddr_r((char *)&(ipAddr.s_addr), sizeof(ipAddr.s_addr),
			  AF_INET, &he, buffer, 128, &myerrno);
#elif HAVE_GETHOSTBYADDR_R_8_ARGS
    res8 = gethostbyaddr_r((char *)&(ipAddr.s_addr), sizeof(ipAddr.s_addr),
			   AF_INET, &he, buffer, 128, &res, &myerrno);
#else
#error HELP -- do not know how to compile gethostbyaddr_r
#endif /* HAVE_GETHOSTBYADDR_R_X_ARGS */
#elif HAVE_GETHOSTBYADDR
	/* Buffer used to make it thread safe */
    if (gethost_mutex_inited == 0) {
    	pthread_mutex_init(&gethost_mutex,0);
        gethost_mutex_inited = 1;
    }
    pthread_mutex_lock(&gethost_mutex);
    res = gethostbyaddr((char *)&(ipAddr.s_addr), sizeof(ipAddr.s_addr),
			AF_INET);
    if (res != NULL) {
	strncpy(buffer, res->h_name, sizeof(buffer));
	buffer[sizeof(buffer) - 1] = '\0';
	he.h_name = buffer;
    }
    pthread_mutex_unlock(&gethost_mutex);
#else
#error HELP -- do not know how to compile gethostbyaddr
#endif

    resolvedName = (res == NULL || res8 != 0) ?
	inet_ntoa(ipAddr) : he.h_name;

    return resolvedName;
}
