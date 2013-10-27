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
	
	std::string getHostPath(path, pathLen);
	getHostPath = "\"" + getHostPath + "/gethost\"";
	
	std::string distccPath(path, pathLen);
	distccPath = "\"" + distccPath + "/distcc\"";
	
	std::string compileop = getHostPath;
	compileop += " ";
	compileop += distccPath;
	compileop += " xcrun ";
	compileop += name;
	for (int i = 1; i < argc; i++) {
		compileop += " ";
		compileop += (char*)argv[i];
	}
	return system(compileop.c_str());
}

