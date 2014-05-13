#ifndef _DMUCS_DPROP_H_
#define _DMUCS_DPROP_H_ 1

/*
 * dmucs_dprop.h: the DMUCS Dprop definition.
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

/*
 * A Dprop is a Dintinguishing Property of a host.  The administrator of a
 * DMUCS installation may want to partition her entire set of compilation
 * hosts into multiple subsets, with each host in a subset having a common
 * distinguishing property -- e.g., each subset may:
 *
 * o have a common architecture -- Solaris vs. Linux vs. FreeBsd -- and
 *   thus may be used for only certain compilations.
 * o be available to a certain set of users or a certain set of
 *   compilations.
 */

#include <string>

typedef std::string DmucsDprop;

const char *dprop2cstr(DmucsDprop d);

const int DPROP_MAX_STRLEN =	64;

#endif 
