/*
 *
 * DistCode -- An OS X GUI & Xcode plugin for the distcc distributed C compiler.
 *
 * Copyright (C) 2013-14 by Mark Satterthwaite
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

//
//  AppDelegate.m
//  DistCode
//
//  Created by Mark Satterthwaite on 30/09/2013.
//  Copyright (c) 2013 marksatt. All rights reserved.
//

#import "AppDelegate.h"
#import <sys/socket.h>
#import <arpa/inet.h>
#import <netdb.h>
#import "OBMenuBarWindow.h"
#import <ServiceManagement/ServiceManagement.h>
#import <Foundation/NSDistributedNotificationCenter.h>

const char *rs_program_name = "distccmon-distcode";
static NSString *kDistCodeGroupIdentifier = @"DistCode";
static NSString *kHostsItemIdentifier = @"Hosts";
static NSString *kMonitorItemIdentifier = @"Monitor";
static NSString *kOptionsItemIdentifier = @"Options";

static NSString* kHostStatus[] = {
	@"Unknown",
	@"Available",
	@"Unavailable",
	@"Overloaded",
	@"Silent"
};

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
		services = [NSMutableArray new];
		DistCCServers = [NSMutableArray new];
		Tasks = [NSMutableArray new];
		DistCodeWrapper = [DistCode new];
		[DistCodeWrapper install];
	}
    return self;
}

- (IBAction)saveDefaults:(id)sender
{
	[[NSUserDefaults standardUserDefaults] synchronize];
	[DistCodeWrapper toggleCompileHost:[[[NSUserDefaults standardUserDefaults] valueForKey:@"RunCompilationHost"] boolValue]];
}

- (void)monitorLoop
{
	NSMutableArray* Objects = [DistCodeWrapper pumpDistccMon];
	[TasksController removeObjects:Tasks];
	[TasksController addObjects:Objects];
}

- (NSMutableDictionary*)getServer:(NSString*)HostName
{
	for (NSMutableDictionary* DistCCDict in DistCCServers)
	{
		if ([[DistCCDict objectForKey:@"HOSTNAME"] isCaseInsensitiveLike:HostName] || [[DistCCDict objectForKey:@"IP"] isCaseInsensitiveLike:HostName])
		{
			return DistCCDict;
		}
	}
	
	NSMutableDictionary* DistCCDict = [NSMutableDictionary new];
	NSMutableArray* DistCCCompilers = [NSMutableArray new];
	NSMutableArray* DistCCSDKs = [NSMutableArray new];
	[DistCCDict setObject:DistCCCompilers forKey:@"COMPILERS"];
	[DistCCDict setObject:DistCCSDKs forKey:@"SDKS"];
	
	struct hostent* Host = gethostbyname([HostName UTF8String]);
	struct in_addr Address;
	memcpy(&Address.s_addr, Host->h_addr_list[0], sizeof(Address.s_addr));
	NSString* IP = [NSString stringWithUTF8String:inet_ntoa(Address)];
	
	[DistCCDict setObject:IP forKey:@"IP"];
	[DistCCDict setObject:HostName forKey:@"HOSTNAME"];
	[DistCCDict setObject:[NSNumber numberWithBool:YES] forKey:@"ACTIVE"];
	[DistCCDict setValue:@"Updating..." forKey:@"STATUS"];
	[DistCCDict setObject:[[NSBundle mainBundle] imageForResource:@"mac_client-512"] forKey:@"ICON"];
	[DistCCDict setValue:@"0" forKey:@"LOADAVG1"];
	[DistCCDict setValue:@"0" forKey:@"LOADAVG5"];
	[DistCCDict setValue:@"0" forKey:@"LOADAVG10"];
	
	[DistCCServers addObject:DistCCDict];
	[DistCCServerController addObject:DistCCDict];
	
	return DistCCDict;
}

- (void)dmucsMonitorHosts:(NSNotification*)Notification
{
	NSDictionary* DistccHosts = Notification.userInfo;
	for (NSDictionary* DistccInfo in DistCCServers)
	{
		NSString* Name = [DistccInfo valueForKey:@"HOSTNAME"];
		if([DistccHosts valueForKey:Name] == nil)
		{
			[DistCCServers removeObject:DistccInfo];
			[DistCCServerController removeObject:DistccInfo];
		}
	}
	
	for(NSDictionary* DistccInfo in DistccHosts.allValues)
	{
		NSString* Name = [DistccInfo valueForKey:@"HostName"];
		NSMutableDictionary* DistCCDict = [self getServer:Name];
		NSInteger Status = [[DistccInfo objectForKey:@"Status"] integerValue];
		BOOL Avail = (Status == 1);
		[DistCCDict setValue:kHostStatus[Status] forKey:@"STATUS"];
		[DistCCDict setValue:[NSNumber numberWithBool:Avail] forKey:@"ACTIVE"];
		[DistCCDict setValue:[DistccInfo valueForKey:@"Cpus"] forKey:@"CPUS"];
		[DistCCDict setValue:[DistccInfo valueForKey:@"Tier"] forKey:@"PRIORITY"];
		NSString* LoadAvg1 = [NSString stringWithFormat:@"%.2f", [[DistccInfo valueForKey:@"LoadAvg1"] floatValue]];
		NSString* LoadAvg5 = [NSString stringWithFormat:@"%.2f", [[DistccInfo valueForKey:@"LoadAvg5"] floatValue]];
		NSString* LoadAvg10 = [NSString stringWithFormat:@"%.2f", [[DistccInfo valueForKey:@"LoadAvg10"] floatValue]];
		[DistCCDict setValue:LoadAvg1 forKey:@"LOADAVG1"];
		[DistCCDict setValue:LoadAvg5 forKey:@"LOADAVG5"];
		[DistCCDict setValue:LoadAvg10 forKey:@"LOADAVG10"];
		
		NSString* DistProp = [DistccInfo valueForKey:@"DistProp"];
		DistProp = [DistProp stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"'"]];
		NSArray* Components = [DistProp componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"="]];
		
		for (NSUInteger i = 0; i < [Components count]-1; i+=2)
		{
			NSString* Key = [Components objectAtIndex:i];
			NSString* Value = [Components objectAtIndex:i+1];
			if ([Key isCaseInsensitiveLike:@"COMPILERS"])
			{
				NSMutableArray* Compilers = [[Value componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@","]] mutableCopy];
				[Compilers removeLastObject];
				[DistCCDict setValue:Compilers forKey:Key];
			}
			else if ([Key isCaseInsensitiveLike:@"SDKS"])
			{
				NSMutableArray* SDKs = [[Value componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@","]] mutableCopy];
				[SDKs removeLastObject];
				[DistCCDict setValue:SDKs forKey:Key];
			}
//			else if(Key && [Key length] > 0 && Value && [Value length] > 0)
//			{
//				[DistCCDict setObject:Value forKey:Key];
//			}
		}
	}
}

//- (void)writeDmucsHostsFile
//{
//	NSString* Path = [NSString stringWithFormat:@"%@/.dmucs/hosts-info", NSHomeDirectory()];
//	FILE* f = fopen([Path UTF8String], "w");
//	NSUInteger SumTasks = 0;
//	for (NSDictionary* DistCCDict in DistCCServers)
//	{
//		NSNumber* Active = [DistCCDict objectForKey:@"ACTIVE"];
//		if([Active boolValue] == YES)
//		{
//			NSString* IP = [DistCCDict objectForKey:@"HOSTNAME"];
//			NSString* CPUs = [DistCCDict objectForKey:@"CPUS"];
//			NSString* Memory = [DistCCDict objectForKey:@"MEMORY"];
//			NSString* Priority = [DistCCDict objectForKey:@"PRIORITY"];
//			NSInteger Pri = [Priority integerValue];
//			
//			NSUInteger NumCPUs = [CPUs integerValue];
//			NSUInteger NumBytes = [Memory integerValue];
//			
//			/* Actually use the memory as the basis - compiler instances in reasonable
//			 codebases typically need ~1GB each so we can't have more running than we
//			 have memory for. Plus the OS will generally need ~2GB for other processes
//			 so account for that. Only if the number of logical CPUs is smaller than
//			 the size of memory in GB will it be the value used. There is never any point
//			 to having more children than there is hardware support for either. */
//			NSUInteger GigaBytes = (NumBytes/(1024*1024*1024));
//			NSUInteger GigaBytesLessOS = (GigaBytes - 2);
//			NSUInteger MaxTasks = NumCPUs <= GigaBytesLessOS ? NumCPUs : GigaBytesLessOS;
//			SumTasks += MaxTasks;
//            
//			NSString* Entry = [NSString stringWithFormat:@"%@ %ld %ld\n", IP, (long)NumCPUs, (long)Pri];
//			fwrite([Entry UTF8String], 1, strlen([Entry UTF8String]), f);
//		}
//	}
//	fclose(f);
//	
//	// Set the number of Xcode build tasks.
//	NSUserDefaults* XcodeDefaults = [[NSUserDefaults alloc] initWithSuiteName:@"com.apple.dt.Xcode"];
//	if (SumTasks)
//	{
//		[XcodeDefaults setInteger: (NSInteger)SumTasks forKey:@"IDEBuildOperationMaxNumberOfConcurrentCompileTasks"];
//		assert([XcodeDefaults integerForKey:@"IDEBuildOperationMaxNumberOfConcurrentCompileTasks"] == SumTasks);
//	}
//	else
//	{
//		// Don't ever use 0, if we get to that use the system default.
//		[XcodeDefaults removeObjectForKey:@"IDEBuildOperationMaxNumberOfConcurrentCompileTasks"];
//	}
//	[XcodeDefaults synchronize];
//}

/*- (BOOL)registerHost:(NSNetService*)NetService withAddress:(NSString*)Address andDetails:(NSString*)Response
{
	BOOL OK = NO;
	char const* SelfName = NULL;
	int Err = dcc_get_dns_domain(&SelfName);
	if(Err != 0 || !SelfName || strlen(SelfName) <= 0)
	{
		return OK;
	}
	NSString* SelfHostName = [NSString stringWithUTF8String:SelfName];
	NSString* LoadAvgPath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"loadavg"];
	NSArray* Components = [Response componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"\n="]];
	if(Components && [Components count])
	{
		NSString* HostName = NetService ? [NetService hostName] : @"localhost";
		HostName = [HostName stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"."]];
		NSMutableDictionary* DistCCDict = [NSMutableDictionary new];
		NSMutableArray* DistCCCompilers = [NSMutableArray new];
		NSMutableArray* DistCCSDKs = [NSMutableArray new];
		[DistCCDict setObject:DistCCCompilers forKey:@"COMPILERS"];
		[DistCCDict setObject:DistCCSDKs forKey:@"SDKS"];
		if(NetService)
			[DistCCDict setObject:NetService forKey:@"SERVICE"];
		[DistCCDict setObject:Address forKey:@"IP"];
		[DistCCDict setObject:HostName forKey:@"HOSTNAME"];
		[DistCCDict setObject:[NSNumber numberWithBool:YES] forKey:@"ACTIVE"];
		[DistCCDict setValue:@"Updating..." forKey:@"STATUS"];
		[DistCCDict setObject:[[NSBundle mainBundle] imageForResource:@"mac_client-512"] forKey:@"ICON"];
        [DistCCDict setValue:@"0" forKey:@"LOADAVG1"];
        [DistCCDict setValue:@"0" forKey:@"LOADAVG5"];
        [DistCCDict setValue:@"0" forKey:@"LOADAVG10"];
		
		for (NSUInteger i = 0; i < [Components count]-1; i+=2)
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
		@try
		{
			if(NetService)
			{
				[DistCCServerController insertObject:DistCCDict atArrangedObjectIndex:[services indexOfObject:NetService]];
			}
			else
			{
				[DistCCServerController addObject:DistCCDict];
			}
		}
		@catch (NSException *exception) {
			NSLog(@"%@", exception);
			[DistCCServerController addObject:DistCCDict];
		}
		[self writeDmucsHostsFile];
		[self addDistCCServer:HostName];
		if(!NetService || ([HostName isCaseInsensitiveLike:SelfHostName]))
		{
			NSPipe* LocalLoadAvgPipe = [NSPipe new];
			NSTask* LocalLoadAvgDaemon = [self beginDaemonTask:LoadAvgPath withArguments:[NSArray new] andPipe:LocalLoadAvgPipe];
			[DistCCDict setObject:LocalLoadAvgDaemon forKey:@"LOADAVG"];
		}
		else if([[[NSUserDefaults standardUserDefaults] valueForKey:@"RunCompilationHost"] boolValue])
		{
			NSPipe* LocalLoadAvgPipe = [NSPipe new];
			NSTask* LocalLoadAvgDaemon = [self beginDaemonTask:LoadAvgPath withArguments:[NSArray arrayWithObjects:@"--server", HostName, nil] andPipe:LocalLoadAvgPipe];
			[DistCCDict setObject:LocalLoadAvgDaemon forKey:@"LOADAVG"];
		}
		OK = YES;
	}
	return OK;
}*/

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

	Browser = [[NSNetServiceBrowser alloc] init];
	[Browser setDelegate:self];
	[Browser searchForServicesOfType:@"_xcodedistcc._tcp" inDomain:@""];
	[self.tabbar selectItemWithIdentifier:kHostsItemIdentifier];
	[DistCCServerController removeObjects:[DistCCServerController arrangedObjects]];
	
	[DistCodeWrapper startup];
	
	MonitorLoopTimer = [NSTimer scheduledTimerWithTimeInterval:1.0f target:self selector:@selector(monitorLoop) userInfo:nil repeats:YES];
	[[NSRunLoop currentRunLoop] addTimer:MonitorLoopTimer forMode:NSEventTrackingRunLoopMode];
	
	NSDistributedNotificationCenter* Notifier = [NSDistributedNotificationCenter defaultCenter];
	[Notifier addObserver:self selector:@selector(dmucsMonitorHosts:) name:@"dmucsMonitorHosts" object:@"DMUCS"];
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
	NSDistributedNotificationCenter* Notifier = [NSDistributedNotificationCenter defaultCenter];
	[Notifier removeObserver:self];
	
	[MonitorLoopTimer invalidate];
	MonitorLoopTimer = nil;

	[Browser stop];
	Browser = nil;
	[DistCodeWrapper shutdown];
	DistCodeWrapper = nil;
    // Set the number of Xcode build tasks.
	NSUserDefaults* XcodeDefaults = [[NSUserDefaults alloc] initWithSuiteName:@"com.apple.dt.Xcode"];
    // Don't ever use 0, if we get to that use the system default.
    [XcodeDefaults removeObjectForKey:@"IDEBuildOperationMaxNumberOfConcurrentCompileTasks"];
	[XcodeDefaults synchronize];
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
		for (NSData* Addr in Addresses)
		{
			struct sockaddr* IP = (struct sockaddr*)[Addr bytes];
			if(IP->sa_family == AF_INET && inet_ntop(IP->sa_family, get_in_addr(IP), Name, INET6_ADDRSTRLEN)!=NULL)
			{
				NSString* Name = [[netService hostName] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"."]];
				[ServicesController addObject:Name];
				break;
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
	[services removeObject:netService];
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
	if([netService hostName])
	{
		NSString* Name = [[netService hostName] stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"."]];
		[ServicesController removeObject:Name];
	}
	[services removeObject:netService];
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
