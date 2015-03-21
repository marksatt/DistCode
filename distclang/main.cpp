/*
 *
 * distclang -- A compiler wrapper for executing the icecc distributed compiler.
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
#include <CoreFoundation/CoreFoundation.h>

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
	
	std::string iceccPath = std::string(path, pathLen);
	iceccPath += "/icecc";
	
	char** Args = (char**)alloca(sizeof(char*) * argc + 1);
	int ArgIdx = 0;
	Args[ArgIdx++] = strdup(iceccPath.c_str());
	Args[ArgIdx++] = strdup(name);
	for (int i = 1; i < argc; i++) {
		Args[ArgIdx++] = strdup(argv[i]);
	}
	Args[ArgIdx++] = NULL;
	
	char* XcodeToolchainPath = NULL;
	CFPropertyListRef Value = CFPreferencesCopyAppValue(CFSTR("XcodeToolchainPath"), CFSTR("com.marksatt.DistCode"));
	if(Value)
	{
		if(CFGetTypeID(Value) == CFStringGetTypeID())
		{
			CFIndex BufferSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength((CFStringRef)Value), kCFStringEncodingASCII) + 1;
			XcodeToolchainPath = (char*)malloc(BufferSize);
			CFStringGetCString((CFStringRef)Value, XcodeToolchainPath, BufferSize, kCFStringEncodingASCII);
		}
		CFRelease(Value);
	}
	
	char const* Path = getenv("PATH");
	if(XcodeToolchainPath)
	{
		if ( Path )
		{
			char* NewPath = (char*)realloc(strdup(Path), strlen(Path) + 1 + strlen(XcodeToolchainPath) + 1);
			Path = strcat(strcat(NewPath, ":"), XcodeToolchainPath);
		}
		else
		{
			Path = XcodeToolchainPath;
		}
		setenv("PATH", Path, 1);
		free(XcodeToolchainPath);
		free(const_cast<char*>(Path));
	}
	
	char* IceccDebugLevel = strdup("debug");
	Value = CFPreferencesCopyAppValue(CFSTR("IceccDebug"), CFSTR("com.marksatt.DistCode"));
	if(Value)
	{
		if(CFGetTypeID(Value) == CFNumberGetTypeID())
		{
			CFIndex Level = 0;
			if(CFNumberGetValue((CFNumberRef)Value, kCFNumberCFIndexType, &Level))
			{
				switch (Level)
				{
					case 1:
						IceccDebugLevel = strdup("warnings");
						break;
					case 2:
						IceccDebugLevel = strdup("info");
						break;
					case 3:
						IceccDebugLevel = strdup("debug");
						break;
					default:
						break;
				}
			}
			
			CFIndex BufferSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength((CFStringRef)Value), kCFStringEncodingASCII) + 1;
			XcodeToolchainPath = (char*)malloc(BufferSize);
			CFStringGetCString((CFStringRef)Value, IceccDebugLevel, BufferSize, kCFStringEncodingASCII);
		}
		CFRelease(Value);
	}
	
	if(IceccDebugLevel)
	{
		setenv("ICECC_DEBUG", IceccDebugLevel, 1);
		free(IceccDebugLevel);
	}
	
	char* IceccLogFile = strdup("/Users/marksatt/Library/Logs/icecc.log");
	Value = CFPreferencesCopyAppValue(CFSTR("IceccLogFile"), CFSTR("com.marksatt.DistCode"));
	if(Value)
	{
		if(CFGetTypeID(Value) == CFStringGetTypeID())
		{
			CFIndex BufferSize = CFStringGetMaximumSizeForEncoding(CFStringGetLength((CFStringRef)Value), kCFStringEncodingASCII) + 1;
			XcodeToolchainPath = (char*)malloc(BufferSize);
			CFStringGetCString((CFStringRef)Value, IceccLogFile, BufferSize, kCFStringEncodingASCII);
		}
		CFRelease(Value);		
	}
	
	if(IceccLogFile)
	{
		setenv("ICECC_LOGFILE", IceccLogFile, 1);
		free(IceccLogFile);
	}
	
	int forkret = fork();
	if (forkret == 0) {
		/* child process */
		if (execvp(iceccPath.c_str(), Args) < 0) {
			fprintf(stderr, "execvp failed: %s err %s\n", iceccPath.c_str(), strerror(errno));
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
