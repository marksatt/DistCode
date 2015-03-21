//
//  main.cpp
//  DistServerDaemon
//
//  Created by Mark Satterthwaite on 22/01/2015.
//  Copyright (c) 2015 marksatt. All rights reserved.
//

#include <Security/Authorization.h>
#include <Security/AuthorizationTags.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, const char * argv[])
{
	if( argc >= 3 )
	{
		if ( !strcmp(argv[1], "EXEC") )
		{
			// Paraphrased from http://developer.apple.com/documentation/Security/Conceptual/authorization_concepts/03authtasks/chapter_3_section_4.html
			OSStatus myStatus;
			AuthorizationFlags myFlags = kAuthorizationFlagDefaults;
			AuthorizationRef myAuthorizationRef;
			
			myStatus = AuthorizationCreate(NULL, kAuthorizationEmptyEnvironment, myFlags, &myAuthorizationRef);
			if (myStatus != errAuthorizationSuccess)
				return myStatus;
			
			AuthorizationItem myItems = {kAuthorizationRightExecute, 0, NULL, 0};
			AuthorizationRights myRights = {1, &myItems};
			myFlags = kAuthorizationFlagDefaults |
			kAuthorizationFlagInteractionAllowed |
			kAuthorizationFlagPreAuthorize |
			kAuthorizationFlagExtendRights;
			
			myStatus = AuthorizationCopyRights (myAuthorizationRef, &myRights, NULL, myFlags, NULL );
			if (myStatus != errAuthorizationSuccess)
				return myStatus;
			
			char* myToolPath = strdup(argv[0]);
			char* myArguments[] = {strdup("AUTH"), strdup(argv[2]), strdup(argv[3]), strdup(argv[4]), NULL};
			FILE *myCommunicationsPipe = NULL;
			
			myFlags = kAuthorizationFlagDefaults;
			myStatus = AuthorizationExecuteWithPrivileges(myAuthorizationRef, myToolPath, myFlags, myArguments, &myCommunicationsPipe);
			
			return myStatus;
		}
		else if ( !strcmp(argv[1], "AUTH") )
		{
			setuid(0);
			
			char const* Format = "/usr/bin/xcrun install_name_tool -change @loader_path/../lib/liblzo2.2.dylib \"%s\" \"%s\"";
			unsigned long Size = strlen(Format) + strlen(argv[3]) + strlen(argv[2]) + strlen(argv[4]) + 1;
			char* Buffer = (char*)calloc(Size, 1);
			
			sprintf(Buffer, Format, argv[3], argv[2]);
			int SysErr = system(Buffer);
			
			struct passwd* root = getpwnam("root");
			struct group* wheel = getgrnam("wheel");
			uid_t user = root ? root->pw_uid : getuid();
			gid_t group = wheel ? wheel->gr_gid : getgid();
			int OwnErr = chown(argv[2], user, group);
			
			sprintf(Buffer, "/bin/chmod 6711 \"%s\"", argv[2]);
			
			int ModErr = system(Buffer);
			
			int OwnKill = chown(argv[4], user, group);
			sprintf(Buffer, "/bin/chmod 6711 \"%s\"", argv[4]);
			
			int ModKill = system(Buffer);
			
			if(SysErr || OwnErr || ModErr || OwnKill || ModKill)
			{
				fprintf(stderr, "LZO: %d, iceccd: %d %d, killice: %d %d\n", SysErr, OwnErr, ModErr, OwnKill, ModKill);
				return (SysErr | OwnErr | ModErr | OwnKill | ModKill);
			}
			
			return 0;
		}
	}
}
