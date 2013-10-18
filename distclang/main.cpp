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

int main(int argc, const char * argv[])
{
	char** newArgv = (char**)alloca(argc+2*sizeof(char const*));

	char const* path = argv[0];
	size_t pathLen = strlen(path);
	for (size_t i = pathLen; i; --i)
	{
		if(path[i-1]=='/')
		{
			path = &path[i];
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
	
	const char *envpath;
    if (!(envpath = getenv("PATH")))
		/* strange but true*/
        return 0;
	
	std::string getHostPath(".");
	getHostPath += "/gethost";
	newArgv[0] = (char*)"xcrun";
	newArgv[1] = (char*)path;
	
	for (int i = 1; i < argc; i++) {
		newArgv[i+1] = (char*)argv[i];
	}
	
	int forkret = fork();
    if (forkret == 0) {
		/* child process */
		if (execvp(getHostPath.c_str(), newArgv) < 0) {
			fprintf(stderr, "execvp failed: err %s\n", strerror(errno));
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

