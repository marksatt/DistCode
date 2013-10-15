//
//  AppDelegate.m
//  Distcc
//
//  Created by Mark Satterthwaite on 30/09/2013.
//  Copyright (c) 2013 marksatt. All rights reserved.
//

#import "AppDelegate.h"
#import <sys/socket.h>
#import <arpa/inet.h>
#import "OBMenuBarWindow.h"

static NSString *kDistCodeGroupIdentifier = @"DistCode";
static NSString *kHostsItemIdentifier = @"Hosts";
static NSString *kMonitorItemIdentifier = @"Monitor";
static NSString *kOptionsItemIdentifier = @"Options";

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

NSNetServiceBrowser* Browser = nil;
@implementation AppDelegate

- (id)init
{
    self = [super init];
    if (self) {
        services = [[NSMutableArray alloc] init];
		DistCCServers = [NSMutableArray new];
		DistCCPipe = [NSPipe new];
		DmucsPipe = [NSPipe new];
		NSString* Path = [NSString stringWithFormat:@"%@/.dmucs", NSHomeDirectory()];
		[[NSFileManager defaultManager] createDirectoryAtPath:Path withIntermediateDirectories:NO attributes:nil error:nil];
	}
    return self;
}

- (void)startDmucs
{
	NSString* DmucsPath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"dmucs"];
	DmucsDaemon = [self beginDaemonTask:DmucsPath withArguments:[NSArray new] andPipe:DmucsPipe];
}

- (void)stopDmucs
{
	[DmucsDaemon terminate];
	[DmucsDaemon waitUntilExit];
	DmucsDaemon = nil;
}

- (void)startDistcc
{
	NSString* DistccDPath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"distccd"];
	DistCCDaemon = [self beginDaemonTask:DistccDPath withArguments:[NSArray arrayWithObjects:@"--daemon", @"--no-detach", @"--zeroconf", @"--allow", @"0.0.0.0/0", nil] andPipe:DistCCPipe];
}

- (void)stopDistcc
{
	[DistCCDaemon terminate];
	[DistCCDaemon waitUntilExit];
	DistCCDaemon = nil;
}

- (void)writeDmucsHostsFile
{
	NSString* Path = [NSString stringWithFormat:@"%@/.dmucs/hosts-info", NSHomeDirectory()];
	FILE* f = fopen([Path UTF8String], "w");
	for (NSDictionary* DistCCDict in DistCCServers)
	{
		NSNumber* Active = [DistCCDict objectForKey:@"ACTIVE"];
		if([Active boolValue] == YES)
		{
			NSString* IP = [DistCCDict objectForKey:@"IP"];
			NSString* CPUs = [DistCCDict objectForKey:@"CPUS"];
			NSString* Priority = [DistCCDict objectForKey:@"PRIORITY"];
			NSString* Entry = [NSString stringWithFormat:@"%@ %@ %@\n", IP, CPUs, Priority];
			fwrite([Entry UTF8String], 1, strlen([Entry UTF8String]), f);
		}
	}
	fclose(f);
}

- (NSTask*)beginDaemonTask:(NSString*)Path withArguments:(NSArray*)Arguments andPipe:(NSPipe*)Pipe
{
	NSTask* Task = [NSTask new];
	[Task setLaunchPath: Path];
	[Task setArguments: Arguments];
	[Task setStandardOutput: Pipe];
	[Task launch];
	return Task;
}

- (NSString*)executeTask:(NSString*)Path withArguments:(NSArray*)Arguments
{
	NSTask* Task = [[NSTask alloc] init];
	[Task setLaunchPath: Path];
	[Task setArguments: Arguments];
	
	NSPipe* Pipe = [NSPipe pipe];
	[Task setStandardOutput: Pipe];
	
	NSFileHandle* File = [Pipe fileHandleForReading];
	
	[Task launch];
	[Task waitUntilExit];
	
	NSData* Data = [File readDataToEndOfFile];
	return [[NSString alloc] initWithData: Data encoding: NSUTF8StringEncoding];
}

- (void)addDistCCServer:(NSString*)ServerIP
{
	NSString* Path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"addhost"];
	NSString* Task = [NSString stringWithFormat:@"\"%@\" -ip %@", Path, ServerIP];
	int Err = system([Task UTF8String]);
	assert(Err == 0);
}

- (void)removeDistCCServer:(NSString*)ServerIP
{
	NSString* Path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"remhost"];
	NSString* Task = [NSString stringWithFormat:@"\"%@\" -ip %@", Path, ServerIP];
	int Err = system([Task UTF8String]);
	assert(Err == 0);
}

- (BOOL)registerHost:(NSNetService*)NetService withAddress:(NSString*)Address andDetails:(NSString*)Response
{
	BOOL OK = NO;
	NSArray* Components = [Response componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"\n="]];
	if(Components && [Components count])
	{
		NSString* HostName = NetService ? [NetService hostName] : @"localhost";
		NSMutableDictionary* DistCCDict = [NSMutableDictionary new];
		NSMutableArray* DistCCCompilers = [NSMutableArray new];
		NSMutableArray* DistCCSDKs = [NSMutableArray new];
		[DistCCDict setObject:DistCCCompilers forKey:@"COMPILERS"];
		[DistCCDict setObject:DistCCSDKs forKey:@"SDKS"];
		[DistCCDict setObject:NetService forKey:@"SERVICE"];
		[DistCCDict setObject:Address forKey:@"IP"];
		[DistCCDict setObject:HostName forKey:@"HOSTNAME"];
		[DistCCDict setObject:[NSNumber numberWithBool:YES] forKey:@"ACTIVE"];
		[DistCCDict setObject:[[NSBundle mainBundle] imageForResource:@"mac_client-512"] forKey:@"ICON"];
		for (NSUInteger i = 0; i < [Components count]; i+=2)
		{
			NSString* Key = [Components objectAtIndex:i];
			NSString* Value = [Components objectAtIndex:i+1];
			if ([Key isCaseInsensitiveLike:@"COMPILER"])
			{
				[DistCCCompilers addObject:[NSDictionary dictionaryWithObjectsAndKeys:Value, @"name", nil]];
			}
			else if ([Key isCaseInsensitiveLike:@"SDK"])
			{
				[DistCCSDKs addObject:[NSDictionary dictionaryWithObjectsAndKeys:Value, @"name", nil]];
			}
			else if(Key && [Key length] > 0 && Value && [Value length] > 0)
			{
				[DistCCDict setObject:Value forKey:Key];
			}
		}
		if(NetService)
		{
			[DistCCServerController insertObject:DistCCDict atArrangedObjectIndex:[services indexOfObject:NetService]];
		}
		else
		{
			[DistCCServerController addObject:DistCCDict];
		}
		[self writeDmucsHostsFile];
		[self addDistCCServer:Address];
		OK = YES;
	}
	return OK;
}

- (BOOL)registerLocalhost
{
	BOOL OK = NO;
	NSString* Path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"distccd"];
	NSString* Response = [self executeTask:Path withArguments:[NSArray arrayWithObjects:@"--host-info", nil]];
	NSRange Range = [Response rangeOfString:@"SYSTEM"];
	if(Range.location != NSNotFound)
	{
		Response = [Response substringFromIndex:Range.location];
		OK = [self registerHost:nil withAddress:@"localhost" andDetails:Response];
	}
	
	return OK;
}

- (void)unregisterLocalhost
{
	for (NSDictionary* DistCCDict in DistCCServers)
	{
		if ([[DistCCDict objectForKey:@"ADDRESS"] isCaseInsensitiveLike:@"localhost"])
		{
			[DistCCServerController removeObject:DistCCDict];
			break;
		}
	}
	[self writeDmucsHostsFile];
	[self removeDistCCServer:@"localhost"];
}

// Sent when addresses are resolved
- (void)netServiceDidResolveAddress:(NSNetService *)netService
{
    // Make sure [netService addresses] contains the
    // necessary connection information
	BOOL OK = NO;
    if ([self addressesComplete:[netService addresses]
				 forServiceType:[netService type]]) {
        NSArray* Addresses = [netService addresses];
		char Name[INET6_ADDRSTRLEN];
		NSString* Path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"distcc"];
		for (NSData* Addr in Addresses)
		{
			struct sockaddr* IP = (struct sockaddr*)[Addr bytes];
			if(inet_ntop(IP->sa_family, get_in_addr(IP), Name, INET6_ADDRSTRLEN)!=NULL)
			{
				NSString* Address = [NSString stringWithFormat:@"%s", Name];
				NSString* Response = [self executeTask:Path withArguments:[NSArray arrayWithObjects:@"--host-info", Address, nil]];
				OK = [self registerHost:netService withAddress:Address andDetails:Response];
				if(OK)
				{
					break;
				}
			}
		}
    }
	if(OK == NO)
	{
		[services removeObject:netService];
	}
}

// Sent if resolution fails
- (void)netService:(NSNetService *)netService didNotResolve:(NSDictionary *)errorDict
{
    [self handleError:[errorDict objectForKey:NSNetServicesErrorCode] withService:netService];
	NSUInteger Index = [services indexOfObject:netService];
	if(Index != NSNotFound)
	{
		if([DistCCServers count] > Index)
		{
			NSMutableDictionary* DistCCDict = [DistCCServers objectAtIndex:Index];
			if(DistCCDict && [[DistCCDict objectForKey:@"SERVICE"] isEqualTo:netService])
			{
				[self removeDistCCServer:[DistCCDict objectForKey:@"IP"]];
				[DistCCServerController removeObject:DistCCDict];
				[self writeDmucsHostsFile];
			}
		}
		[services removeObject:netService];
	}
}

// Verifies [netService addresses]
- (BOOL)addressesComplete:(NSArray *)addresses forServiceType:(NSString *)serviceType
{
    // Perform appropriate logic to ensure that [netService addresses]
    // contains the appropriate information to connect to the service
    return YES;
}

// Error handling code
- (void)handleError:(NSNumber *)error withService:(NSNetService *)service
{
    NSLog(@"An error occurred with service %@.%@.%@, error code = %d",
		  [service name], [service type], [service domain], [error intValue]);
    // Handle error here
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
	// Insert code here to initialize your application
	NSImage* Image = [NSImage imageNamed:@"vpn-512"];
	[Image setSize:NSMakeSize(22.f, 22.f)];
    self.window.menuBarIcon = Image;
    self.window.highlightedMenuBarIcon = Image;
    self.window.hasMenuBarIcon = YES;
    self.window.attachedToMenuBar = YES;
    self.window.isDetachable = YES;
	
	NSString* DefaultsPath = [[NSBundle mainBundle] pathForResource:@"Defaults" ofType:@"plist"];
	if (DefaultsPath)
	{
		[[NSUserDefaults standardUserDefaults] registerDefaults:[[NSDictionary alloc] initWithContentsOfFile:DefaultsPath]];
	}

	NSNumber* RunCompilationHost = [[NSUserDefaults standardUserDefaults] valueForKey:@"RunCompilationHost"];
	if([RunCompilationHost boolValue] == YES)
	{
		[self startDistcc];
	}
	else
	{
		[self registerLocalhost];
	}
	[self startDmucs];
	
	Browser = [[NSNetServiceBrowser alloc] init];
	[Browser setDelegate:self];
	[Browser searchForServicesOfType:@"_xcodedistcc._tcp" inDomain:@""];
	[self.tabbar selectItemWithIdentifier:kHostsItemIdentifier];
}
- (void)applicationWillTerminate:(NSNotification *)notification
{
	[Browser stop];
	Browser = nil;
	if(DmucsDaemon)
	{
		[self stopDmucs];
	}
	if(DistCCDaemon)
	{
		[self stopDistcc];
	}
}
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didFindDomain:(NSString *)domainName moreComing:(BOOL)moreDomainsComing
{
	
}
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didRemoveDomain:(NSString *)domainName moreComing:(BOOL)moreDomainsComing
{
	
}
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didFindService:(NSNetService *)netService moreComing:(BOOL)moreServicesComing
{
	[services addObject:netService];
	[netService setDelegate:self];
	[netService scheduleInRunLoop:[NSRunLoop mainRunLoop] forMode:NSDefaultRunLoopMode];
	[netService resolveWithTimeout:15.0];
}
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didRemoveService:(NSNetService *)netService moreComing:(BOOL)moreServicesComing
{
	NSUInteger Index = [services indexOfObject:netService];
	if(Index != NSNotFound)
	{
		if([DistCCServers count] > Index)
		{
			NSMutableDictionary* DistCCDict = [DistCCServers objectAtIndex:Index];
			if(DistCCDict && [[DistCCDict objectForKey:@"SERVICE"] isEqualTo:netService])
			{
				[self removeDistCCServer:[DistCCDict objectForKey:@"IP"]];
				[DistCCServerController removeObject:DistCCDict];
				[self writeDmucsHostsFile];
			}
		}
		[services removeObject:netService];
	}
}
- (void)netServiceBrowserWillSearch:(NSNetServiceBrowser *)netServiceBrowser
{
	
}
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didNotSearch:(NSDictionary *)errorInfo
{
	
}
- (void)netServiceBrowserDidStopSearch:(NSNetServiceBrowser *)netServiceBrowser
{
	
}

- (void)setViewForIdentifier:(NSString *)identifier
{
	[self.TabView selectTabViewItemWithIdentifier:identifier];
}

#pragma mark - DOTabBarDelegate

- (void)tabbar:(DOTabbar *)tabbar didSelectItemWithIdentifier:(NSString *)identifier
{
	[self setViewForIdentifier:identifier];
}

- (NSArray *)tabbarGroupIdentifiers:(DOTabbar *)tabbar
{
	return @[kDistCodeGroupIdentifier];
}

- (NSString *)tabbar:(DOTabbar *)tabbar titleForGroupIdentifier:(NSString *)identifier
{
	return nil;
}

- (NSArray *)tabbar:(DOTabbar *)tabbar itemIdentifiersForGroupIdentifier:(NSString *)identifier
{
	if ([identifier isEqualToString:kDistCodeGroupIdentifier]) {
		return @[kHostsItemIdentifier, kMonitorItemIdentifier, kOptionsItemIdentifier];
	} else {
		return nil;
	}
}

- (NSCell *)tabbar:(DOTabbar *)tabbar cellForItemIdentifier:(NSString *)itemIdentifier
{
	DOTabbarItemCell *cell = [DOTabbarItemCell new];
	
	if ([itemIdentifier isEqualToString:kHostsItemIdentifier]) {
		cell.title = kHostsItemIdentifier;
		cell.image = [NSImage imageNamed:@"computer"];
		[cell.image setSize:NSMakeSize(40.f, 40.f)];
	} else if ([itemIdentifier isEqualToString:kMonitorItemIdentifier]) {
		cell.title = kMonitorItemIdentifier;
		cell.image = [NSImage imageNamed:@"utilities-system-monitor"];
		[cell.image setSize:NSMakeSize(40.f, 40.f)];
	} else if ([itemIdentifier isEqualToString:kOptionsItemIdentifier]) {
		cell.title = kOptionsItemIdentifier;
		cell.image = [NSImage imageNamed:@"applications-system"];
		[cell.image setSize:NSMakeSize(40.f, 40.f)];
	}
	
	return cell;
}
@end
