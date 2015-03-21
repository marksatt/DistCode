//
//  main.cpp
//  killice
//
//  Created by Mark Satterthwaite on 08/03/2015.
//  Copyright (c) 2015 marksatt. All rights reserved.
//

#include <stdlib.h>
#include <unistd.h>

int main(int argc, const char * argv[]) {
	setuid(0);
	
	int SysErr = system("killall -9 iceccd");
	return SysErr;
}
