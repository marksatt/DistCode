//
//  DistCode.m
//  DistCode
//
//  Created by Mark Satterthwaite on 31/07/2015.
//  Copyright (c) 2015 marksatt. All rights reserved.
//

#import "DistCode.h"
#import "distcc.h"
#import "mon.h"
#import "util.h"

@interface DistCode (private)
+ (NSComparisonResult)compareVersion:(NSString*)versionOne toVersion:(NSString*)versionTwo;
@end

@implementation DistCode (private)

+ (NSComparisonResult)compareVersion:(NSString*)versionOne toVersion:(NSString*)versionTwo {
	NSArray* versionOneComp = [versionOne componentsSeparatedByString:@"."];
	NSArray* versionTwoComp = [versionTwo componentsSeparatedByString:@"."];
	
	NSInteger pos = 0;
	
	while ([versionOneComp count] > pos || [versionTwoComp count] > pos) {
		NSInteger v1 = [versionOneComp count] > pos ? [[versionOneComp objectAtIndex:pos] integerValue] : 0;
		NSInteger v2 = [versionTwoComp count] > pos ? [[versionTwoComp objectAtIndex:pos] integerValue] : 0;
		if (v1 < v2) {
			return NSOrderedAscending;
		}
		else if (v1 > v2) {
			return NSOrderedDescending;
		}
		pos++;
	}
	
	return NSOrderedSame;
}

@end

@implementation DistCode

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

- (void)startDistccDaemonAsCoordinator:(BOOL)AsCoordinator
{
	NSString* DistccDPath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"distccd"];
	DistCCPipe = [NSPipe new];
	if(AsCoordinator)
	{
		DistCCDaemon = [self beginDaemonTask:DistccDPath withArguments:[NSArray arrayWithObjects:@"--daemon", @"--no-detach", @"--zeroconf", @"--allow", @"0.0.0.0/0", nil] andPipe:DistCCPipe];
	}
	else
	{
		DistCCDaemon = [self beginDaemonTask:DistccDPath withArguments:[NSArray arrayWithObjects:@"--daemon", @"--no-detach", @"--allow", @"0.0.0.0/0", nil] andPipe:DistCCPipe];
	}
}

- (void)stopDistccDaemon
{
	if(DistCCDaemon)
	{
		[DistCCDaemon terminate];
		[DistCCDaemon waitUntilExit];
		DistCCDaemon = nil;
		DistCCPipe = nil;
	}
}

- (void)startDmucsDaemon
{
	NSString* DmucsPath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"dmucs"];
	DmucsPipe = [NSPipe new];
	DmucsDaemon = [self beginDaemonTask:DmucsPath withArguments:[NSArray new] andPipe:DmucsPipe];
}

- (void)stopDmucsDaemon
{
	if(DmucsDaemon)
	{
		[DmucsDaemon terminate];
		[DmucsDaemon waitUntilExit];
		DmucsDaemon = nil;
		DmucsPipe = nil;
	}
}

- (void)startDmucsMonitor
{
	NSString* DmucsMonPath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"monitor"];
	DmucsMonPipe = [NSPipe new];
	DmucsMonDaemon = [self beginDaemonTask:DmucsMonPath withArguments:[NSArray arrayWithObjects:@"--server", [self coordinatorIPAddress], nil] andPipe:DmucsMonPipe];
}

- (void)stopDmucsMonitor
{
	if(DmucsMonDaemon)
	{
		[DmucsMonDaemon terminate];
		[DmucsMonDaemon waitUntilExit];
		DmucsMonDaemon = nil;
		DmucsMonPipe = nil;
	}
}

- (BOOL)startLoadAvg: (NSString*) HostName
{
	NSString* SelfHostName = [self hostName];
	if(SelfHostName)
	{
		NSString* LoadAvgPath = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"loadavg"];
		NSString* HostProp = [self hostProperty];
		if(([HostName isCaseInsensitiveLike:SelfHostName]))
		{
			LoadAvgPipe = [NSPipe new];
			LoadAvgDaemon = [self beginDaemonTask:LoadAvgPath withArguments:[NSArray arrayWithObjects:@"--type", HostProp, nil] andPipe:LoadAvgPipe];
		}
		else if([[[NSUserDefaults standardUserDefaults] valueForKey:@"RunCompilationHost"] boolValue])
		{
			LoadAvgPipe = [NSPipe new];
			LoadAvgDaemon = [self beginDaemonTask:LoadAvgPath withArguments:[NSArray arrayWithObjects:@"--server", HostName, @"--type", HostProp, nil] andPipe:LoadAvgPipe];
		}
		return YES;
	}
	else
	{
		return NO;
	}
}

- (void)stopLoadAvg
{
	if(LoadAvgDaemon)
	{
		[LoadAvgDaemon terminate];
		[LoadAvgDaemon waitUntilExit];
		LoadAvgPipe = nil;
		LoadAvgDaemon = nil;
	}
}

- (NSMutableArray*)pumpDistccMon
{
	struct dcc_task_state *list;
	int ret;
	
	static bool TraceSet = false;
	if(!TraceSet)
	{
		TraceSet = true;
		dcc_set_trace_from_env();
	}
	
	NSMutableArray* Objects = [NSMutableArray new];
	
	struct dcc_task_state *i;
	
	if ((ret = dcc_mon_poll(&list)))
		return Objects;
	
	for (i = list; i; i = i->next) {
		NSMutableDictionary* Object = [NSMutableDictionary new];
		[Object setObject:[NSNumber numberWithUnsignedLong:i->cpid] forKey:@"PID"];
		[Object setObject:[NSString stringWithUTF8String:dcc_get_phase_name(i->curr_phase)] forKey:@"Phase"];
		[Object setObject:[NSString stringWithUTF8String:i->file] forKey:@"File"];
		[Object setObject:[NSString stringWithUTF8String:i->host] forKey:@"Host"];
		[Object setObject:[NSNumber numberWithInt:i->slot] forKey:@"CPU"];
		[Objects addObject:Object];
	}
	
	dcc_task_state_free(list);
	
	return Objects;
}

- (NSString*)hostName
{
	char SelfName[1024];
	int ret = gethostname(SelfName, sizeof(SelfName));
	if (ret != 0)
		return nil;
	return [NSString stringWithUTF8String:SelfName];
}

- (void)addHost:(NSString*)Host ToServer:(NSString*)Server withNumCPUS:(unsigned)NumCPUs andPowerIndex:(unsigned)Index
{
	NSString* HostProp = [self hostProperty];
	NSString* Path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"addhost"];
	NSString* Task = [NSString stringWithFormat:@"\"%@\" --server %@ --host %@ --type %@ --add %d %d", Path, Server, Host, HostProp, NumCPUs, Index];
	int Err = system([Task UTF8String]);
	assert(Err == 0);
}

- (void)removeHost:(NSString*)Host fromServer:(NSString*)Server
{
	NSString* Path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"remhost"];
	NSString* Task = [NSString stringWithFormat:@"\"%@\" --server %@ --host %@", Path, Server, Host];
	int Err = system([Task UTF8String]);
	assert(Err == 0);
}

- (NSString*)hostProperty
{
	NSMutableString* HostProp = [NSMutableString new];
	NSMutableDictionary* HostInfo = [self hostInfo];
	
	char Buffer[256];
	char Buffer1[32];
	char Buffer2[32];

//	[HostProp appendString:@"PLATFORM="];
//	
//	NSString* Platform = [NSProcessInfo processInfo].operatingSystemVersionString;
//	[HostProp appendString:[NSString stringWithFormat:@"%ld.%ld.%ld", [NSProcessInfo processInfo].operatingSystemVersion.majorVersion, [NSProcessInfo processInfo].operatingSystemVersion.minorVersion, (long)[NSProcessInfo processInfo].operatingSystemVersion.patchVersion]];
//	if(sscanf([Platform UTF8String], "Version %*s %*s %[^)]", Buffer))
//	{
//		[HostProp appendString:[NSString stringWithFormat:@"-%s", Buffer]];
//	}
//	
//	[HostProp appendString:[NSString stringWithFormat:@"=MEMORY=%llu", ([NSProcessInfo processInfo].physicalMemory / 1024 / 1024)]];
	
	
	NSMutableArray* Compilers = [HostInfo valueForKey:@"COMPILERS"];
	[HostProp appendString:@"COMPILERS="];
	for(NSDictionary* Compiler in Compilers)
	{
		NSString* CompilerName = [Compiler valueForKey:@"name"];
		
		if(sscanf([CompilerName UTF8String], "Apple LLVM version %*s %*[(]%[^)]", Buffer))
		{
			[HostProp appendString:[NSString stringWithFormat:@"%s,", Buffer]];
		}
	}
	
	NSMutableArray* SDKs = [HostInfo valueForKey:@"SDKS"];
	[HostProp appendString:@"=SDKS="];
	for(NSDictionary* SDK in SDKs)
	{
		NSString* SDKName = [SDK valueForKey:@"name"];
		
		if(![SDKName containsString:@"simulator"] && sscanf([SDKName UTF8String], "%s %s %s", Buffer, Buffer1, Buffer2) == 3)
		{
			[HostProp appendString:[NSString stringWithFormat:@"%s-%s-%s,", Buffer, Buffer1, Buffer2]];
		}
	}
	
	return HostProp;
}

- (NSMutableDictionary*)hostInfo
{
	NSString* Path = [[NSBundle mainBundle] pathForAuxiliaryExecutable:@"distccd"];
	NSString* Response = [self executeTask:Path withArguments:[NSArray arrayWithObjects:@"--host-info", nil]];
	NSRange Range = [Response rangeOfString:@"SYSTEM"];
	if(Range.location != NSNotFound)
	{
		NSArray* Components = [Response componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"\n="]];
		if(Components && [Components count])
		{
			NSMutableDictionary* DistCCDict = [NSMutableDictionary new];
			NSMutableArray* DistCCCompilers = [NSMutableArray new];
			NSMutableArray* DistCCSDKs = [NSMutableArray new];
			[DistCCDict setObject:DistCCCompilers forKey:@"COMPILERS"];
			[DistCCDict setObject:DistCCSDKs forKey:@"SDKS"];
			
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
			
			return DistCCDict;
		}
	}
	
	return nil;
}

- (BOOL)runCoordinator
{
	return ![[[NSUserDefaults standardUserDefaults] valueForKey:@"CoordinatorMode"] boolValue];
}

- (BOOL)runCompileHost
{
	return [[[NSUserDefaults standardUserDefaults] valueForKey:@"RunCompilationHost"] boolValue];
}

- (BOOL)verboseOutput
{
	return [[[NSUserDefaults standardUserDefaults] valueForKey:@"VerboseOutput"] boolValue];
}

- (NSString*)coordinatorIPAddress
{
	return [self runCoordinator] ? @"localhost" : (NSString*)[[NSUserDefaults standardUserDefaults] valueForKey:@"CoordinatorIP"];
}

- (NSNumber*)hostTimeout
{
	return (NSNumber*)[[NSUserDefaults standardUserDefaults] valueForKey:@"HostTimeout"];
}

- (BOOL)startup
{
	if([self runCoordinator] || [self coordinatorIPAddress])
	{
		if([self runCoordinator])
		{
			[self startDmucsDaemon];
		}
		
		[self startDmucsMonitor];
		
		if([self runCompileHost])
		{
			return [self startCompileHost];
		}
		else
		{
			return YES;
		}
	}
	else
	{
		return NO;
	}
}

- (void)shutdown
{
	if([self runCompileHost])
	{
		[self stopCompileHost];
	}
	
	[self stopDmucsMonitor];
	[self stopDmucsDaemon];
	[self stopDistccDaemon];
}

- (BOOL)restart
{
	[self shutdown];
	return [self startup];
}

- (BOOL)startCompileHost
{
	[self startDistccDaemonAsCoordinator:[self runCoordinator]];
	
	NSDictionary* HostInfo = [self hostInfo];
	NSString* CPUs = [HostInfo objectForKey:@"CPUS"];
	NSString* Priority = [HostInfo objectForKey:@"PRIORITY"];
	unsigned NumCPUs = (unsigned)[CPUs integerValue];
	unsigned Pri = (unsigned)[Priority integerValue];
	
	id MaxJobs = [[NSUserDefaults standardUserDefaults] valueForKey:@"MaxJobs"];
	if(MaxJobs && [MaxJobs integerValue] > 0)
	{
		NumCPUs = (unsigned)[MaxJobs integerValue];
	}
	
	[self addHost:[self hostName] ToServer:[self coordinatorIPAddress] withNumCPUS:NumCPUs andPowerIndex:Pri];
	
	return [self startLoadAvg:[self coordinatorIPAddress]];
}

- (void)stopCompileHost
{
	[self stopLoadAvg];
	[self removeHost:[self hostName] fromServer:[self coordinatorIPAddress]];
	[self stopDistccDaemon];
}

- (void)toggleCompileHost:(BOOL)State
{
	if(DistCCDaemon && !State)
	{
		[self stopCompileHost];
	}
	else if(State && !DistCCDaemon)
	{
		[self startCompileHost];
	}
}

- (void)install
{
	NSString* Path = [NSString stringWithFormat:@"%@/.dmucs", NSHomeDirectory()];
	if([[NSFileManager defaultManager] fileExistsAtPath:Path isDirectory:NULL] == NO)
	{
		[[NSFileManager defaultManager] createDirectoryAtPath:Path withIntermediateDirectories:NO attributes:nil error:nil];
	}
	NSString* HostsPath = [NSString stringWithFormat:@"%@/.dmucs/hosts-info", NSHomeDirectory()];
	[[NSFileManager defaultManager] removeItemAtPath:HostsPath error:nil];
	
	NSArray* Paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES);
	Path = [NSString stringWithFormat:@"%@/Developer/Shared/Xcode/Plug-ins", [Paths objectAtIndex:0]];
	NSString* PluginLink = [NSString stringWithFormat:@"%@/Distcc 3.2.xcplugin", Path];
	if([[NSFileManager defaultManager] fileExistsAtPath:PluginLink isDirectory:NULL] == NO)
	{
		[[NSFileManager defaultManager] createDirectoryAtPath:Path withIntermediateDirectories:YES attributes:nil error:nil];
		NSString* PluginPath = [[NSBundle mainBundle] pathForResource:@"Distcc 3.2" ofType:@"xcplugin"];
		[[NSFileManager defaultManager] copyItemAtPath:PluginPath toPath:PluginLink error:nil];
	}
	else
	{
		NSBundle* CurrentBundle = [NSBundle bundleWithPath:PluginLink];
		NSString* CurrentVersion = [[CurrentBundle infoDictionary] objectForKey:@"CFBundleVersion"];
		NSString* PluginPath = [[NSBundle mainBundle] pathForResource:@"Distcc 3.2" ofType:@"xcplugin"];
		NSBundle* NewBundle = [NSBundle bundleWithPath:PluginPath];
		NSString* NewVersion = [[NewBundle infoDictionary] objectForKey:@"CFBundleVersion"];
		if([DistCode compareVersion:CurrentVersion toVersion:NewVersion] == NSOrderedAscending)
		{
			[[NSFileManager defaultManager] removeItemAtPath:PluginLink error:nil];
			[[NSFileManager defaultManager] copyItemAtPath:PluginPath toPath:PluginLink error:nil];
		}
	}
	
	if([[NSFileManager defaultManager] fileExistsAtPath:PluginLink] == true)
	{
		NSString* InfoPath = [NSString stringWithFormat:@"%@/Contents/Info.plist", PluginLink];
		NSData* InfoData = [NSData dataWithContentsOfFile: InfoPath];
		NSPropertyListFormat InfoFormat = NSPropertyListXMLFormat_v1_0;
		NSPropertyListMutabilityOptions InfoOptions = NSPropertyListMutableContainersAndLeaves;
		NSError* InfoError = nil;
		id PropertyList = [NSPropertyListSerialization propertyListWithData:InfoData options: InfoOptions format: &InfoFormat error: &InfoError];
		if ( PropertyList != nil )
		{
			NSMutableDictionary* Dictionary = [NSMutableDictionary new];
			NSMutableArray* UUIDs = [Dictionary valueForKey:@"DVTPlugInCompatibilityUUIDs"];
			NSString* XcodePathResult = [self executeTask:@"/usr/bin/xcode-select" withArguments:[NSArray arrayWithObjects:@"-p", nil]];
			NSString* XcodePath = [XcodePathResult stringByTrimmingCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"\n \r"]];
			NSString* XcodeInfoPath = [[XcodePath stringByDeletingLastPathComponent] stringByAppendingPathComponent: @"Info.plist"];
			NSData* XcodeData = [NSData dataWithContentsOfFile: XcodeInfoPath];
			
			NSPropertyListFormat XcodeFormat = InfoFormat;
			NSPropertyListMutabilityOptions XcodeOptions = NSPropertyListImmutable;
			NSError* XcodeError = nil;
			id XcodePropertyList = [NSPropertyListSerialization propertyListWithData:XcodeData options: XcodeOptions format: &XcodeFormat error:&XcodeError];
			if ( XcodePropertyList != nil )
			{
				NSMutableDictionary* XcodeDictionary = [NSMutableDictionary new];
				NSString* UUID = [XcodeDictionary valueForKey:@"DVTPlugInCompatibilityUUID"];
				if ( [UUIDs containsObject:UUID] == false )
				{
					[UUIDs addObject:UUID];
					[Dictionary setValue:UUIDs forKey:@"DVTPlugInCompatibilityUUIDs"];
					NSData* NewDict = [NSPropertyListSerialization dataFromPropertyList:Dictionary format: InfoFormat errorDescription: nil];
					if ( NewDict != nil )
					{
						[NewDict writeToFile:InfoPath atomically: true];
					}
				}
			}
		}
	}
}

@end

