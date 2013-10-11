//
//  main.cpp
//  distclang
//
//  Created by Mark Satterthwaite on 19/09/2013.
//  Copyright (c) 2013 marksatt. All rights reserved.
//

#include <iostream>

int main(int argc, const char * argv[])
{
	int forkret = fork();
    if (forkret == 0) {
		/* child process */
		if (execvp(argv[nextarg], &argv[nextarg]) < 0) {
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

