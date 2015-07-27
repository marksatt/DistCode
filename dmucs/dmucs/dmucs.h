#ifndef _DMUCS_MAIN_H_
#define _DMUCS_MAIN_H_


/*
 * dmucs.h: main header file.
 *
 * Author: Victor T. Norman
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

extern bool debugMode;

#define DMUCS_DEBUG(x) if (debugMode) fprintf x
#define strequ(x, y) (strncmp(x, y, strlen(x)) == 0)

/*
 * Ideally, the administrator should set the name of the machine on which
 * the host server will run (and the clients (like 'gethost') will contact)
 * by putting CPPFLAGS=-DSERVER_MACH_NAME=\\\"<hostname>\\\" as an
 * argument to the 'configure' script.
 * E.g., ./configure CPPFLAGS=-DSERVER_MACH_NAME=\\\"mulberry\\\"
 *       if the administrator plans to run the 'dmucs' server on a machine
 *       named mulberry.
 */
#ifndef SERVER_MACH_NAME
#define SERVER_MACH_NAME	"localhost"
#endif

#ifndef SERVER_PORT_NUM
#define SERVER_PORT_NUM 9714
#endif

#include "COSMIC/HDR/sockets.h"

void addFd(Socket *sock);
void removeFd(Socket *sock);


#endif
