/*
 *
 * distclang -- A compiler wrapper for executing the distcc distributed C compiler with the dmucs host manager.
 *
 * Copyright (C) 2013-14 by Mark Satterthwaite
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

//
//  main.cpp
//  distclang
//
//  Created by Mark Satterthwaite on 19/09/2013.
//  Copyright (c) 2013 marksatt. All rights reserved.
//

#include <iostream>
#include <string>
#include <unistd.h>
#include <syslog.h>
#include <vector>
#if __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

int main(int argc, const char * argv[])
{
	char const* path = argv[0];
	char const* name = argv[0];
	size_t pathLen = strlen(path);
	for (size_t i = pathLen; i; --i)
	{
		if(path[i-1]=='/')
		{
			name = &path[i];
			pathLen = i-1;
			break;
		}
	}
	
	char *env = getenv("DISTCLANG_RECURSE");
	if(env != NULL)
	{
		fprintf(stderr, "distclang called recursively!\n");
		return -1;
	}
	putenv("DISTCLANG_RECURSE=1");
	
	long timeout = 0;
	
#if __APPLE__
	CFPropertyListRef Value = CFPreferencesCopyAppValue(CFSTR("HostTimeout"), CFSTR("com.marksatt.DistCode"));
	if(Value)
	{
		if(CFGetTypeID(Value) == CFNumberGetTypeID())
		{
			long value = 1;
			CFNumberGetValue((CFNumberRef)Value, kCFNumberLongType, &value);
			timeout = value >= 0 ? value * 60 : value;
		}
		CFRelease(Value);
	}
    else
    {
        timeout = -1;
    }
#endif
	
	char timeoutBuffer[16] = {0};
	snprintf(timeoutBuffer, 16, "%ld", timeout);
	
	std::string getHostPath = std::string(path, pathLen);
	getHostPath += "/gethost";
	std::string distccPath = std::string(path, pathLen);
	distccPath += "/distcc";
	
	char** Args = (char**)alloca(sizeof(char*) * argc + 7);
	int ArgIdx = 0;
	Args[ArgIdx++] = strdup(getHostPath.c_str());
	Args[ArgIdx++] = strdup("--wait");
	Args[ArgIdx++] = timeoutBuffer;
	Args[ArgIdx++] = strdup(distccPath.c_str());
	Args[ArgIdx++] = strdup("xcrun");
	Args[ArgIdx++] = strdup(name);
	for (int i = 1; i < argc; i++) {
		Args[ArgIdx++] = strdup(argv[i]);
	}
	Args[ArgIdx++] = NULL;
	
	int forkret = fork();
    if (forkret == 0) {
		/* child process */
		if (execvp(getHostPath.c_str(), Args) < 0) {
			fprintf(stderr, "execvp failed: %s err %s\n", getHostPath.c_str(), strerror(errno));
			return -1;
		}
		return 0;
    } else if (forkret < 0) {
		fprintf(stderr, "Failed to fork a process!\n");
		return -1;
    }
	
    /* parent process -- just wait for the child */
    int status = 0;
    pid_t pid = -1;
    do
    {
        pid = waitpid(forkret, &status, 0);
    } while (pid == -1 && errno == EINTR);
	
    return WEXITSTATUS(status);
}

