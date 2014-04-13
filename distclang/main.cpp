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
	
	std::string getHostPath = std::string(path, pathLen);
	getHostPath += "/gethost";
//	std::string pumpPath = std::string(path, pathLen);
//	pumpPath += "/pump";
	std::string distccPath = std::string(path, pathLen);
	distccPath += "/distcc";
	
	char** Args = (char**)alloca(sizeof(char*) * argc + 5);
	int ArgIdx = 0;
	Args[ArgIdx++] = strdup(getHostPath.c_str());
	//Args[ArgIdx++] = strdup(pumpPath.c_str());
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
    (void) wait(&status);
	
    return WEXITSTATUS(status);
}

