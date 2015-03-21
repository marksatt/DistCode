//
//  IceCode.m
//  DistCode
//
//  Created by Mark Satterthwaite on 23/02/2015.
//  Copyright (c) 2015 marksatt. All rights reserved.
//

#import "IceCode.h"
#include <icecc/comm.h>

NSString* CompileLanguage(CompileJob::Language Lang)
{
	switch (Lang)
	{
		case CompileJob::Lang_C:
		{
			return @"C";
		}
		case CompileJob::Lang_CXX:
		{
			return @"C++";
		}
		case CompileJob::Lang_OBJC:
		{
			return @"Objective-C";
		}
		case CompileJob::Lang_Custom:
		default:
		{
			return @"Custom";
		}
	}
}

NSString* CompileFlags(uint32_t Flags)
{
	NSMutableString* CompileFlags = [NSMutableString new];
	if( Flags != CompileJob::Flag_None )
	{
		if( Flags & CompileJob::Flag_g )
		{
			[CompileFlags appendString:@" -g"];
		}
		if( Flags & CompileJob::Flag_g3 )
		{
			[CompileFlags appendString:@" -g3"];
		}
		if( Flags & CompileJob::Flag_O )
		{
			[CompileFlags appendString:@" -O"];
		}
		if( Flags & CompileJob::Flag_O2 )
		{
			[CompileFlags appendString:@" -O2"];
		}
		if( Flags & CompileJob::Flag_Ol2 )
		{
			[CompileFlags appendString:@" -Ol2"];
		}
	}
	return CompileFlags;
}

@interface IceCode (Private)
- (void)registerConnectionhandler;
- (void)registerMessageHandler;
- (void)handleStatsMessage:(Msg*) Mesg;
- (void)handleRemoteJobBegin:(Msg*)Mesg;
- (void)handleLocalJobBegin:(Msg*)Mesg;
- (void)handleCompileStatusMessage:(Msg*)Mesg;
- (void)handleRemoteJobDone:(Msg*)Mesg;
- (void)handleLocalJobDone:(Msg*)Mesg;
- (void)handleTimerEvent;
@end

@implementation IceCode

+ (IceCode*) iceCodeForNetName:(NSString*)NetName withTimeout:(NSInteger)Timeout andSchedulerName:(NSString*)Scheduler andPort:(NSUInteger)Port
{
	std::string net = [NetName UTF8String];
	std::string sched = [Scheduler UTF8String];
	IceCode* Ice = [IceCode new];
	Ice->Hosts = [NSMutableDictionary new];
	Ice->Jobs = [NSMutableDictionary new];
	Ice->CompleteJobs = [NSMutableDictionary new];
	Ice->HostsArray = [NSMutableArray new];
	Ice->JobsArray = [NSMutableArray new];
	Ice->Port = Port;
	Ice->Timeout = Timeout;
	Ice->Discover = new DiscoverSched(net, (int)Timeout, sched, (int)Port);
	Ice->Scheduler = nullptr;
	Ice->NetName = NetName;
	Ice->SchedulerName = Scheduler;
	Ice->Timer = [NSTimer scheduledTimerWithTimeInterval:10.0 target:Ice selector:@selector(handleTimerEvent) userInfo:nil repeats:YES];
	
	return Ice;
}

- (void)dealloc
{
	if(Timer)
	{
		[Timer invalidate];
		Timer = nullptr;
	}
	[self handleEnd];
}

- (NSString*)hostName
{
	return NetName;
}

- (NSString*)schedulerName
{
	return SchedulerName;
}

- (void)registerConnectionhandler
{
	DiscoverSched* Disc = (DiscoverSched*)Discover;
	assert(Disc);
	
	dispatch_block_t ConnectionHandler = ^{
		if(!Scheduler)
		{
			DiscoverSched* DiscoverObj = (DiscoverSched*)Discover;
			assert(DiscoverObj);
			MsgChannel* Msg = DiscoverObj->try_get_scheduler();
			if ( Msg )
			{
				Scheduler = Msg;
				NetName = [NSString stringWithUTF8String:DiscoverObj->networkName().data()];
				SchedulerName = [NSString stringWithUTF8String:DiscoverObj->schedulerName().data()];
				[self registerMessageHandler];
				Msg->send_msg ( MonLoginMsg() );
				delete DiscoverObj;
				DiscoverObj = nullptr;
				if(Connect)
				{
					dispatch_source_cancel(Connect);
					Connect = nullptr;
				}
			}
		}
	};
	ConnectionHandler();
	
	if(!Scheduler)
	{
		Connect = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, Disc->listen_fd(), 0, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0));
		dispatch_source_set_event_handler(Connect, ConnectionHandler);
		dispatch_resume(Connect);
	}
}

- (void)registerMessageHandler
{
	MsgChannel* Channel = (MsgChannel*)Scheduler;
	if (Channel && !Source)
	{
		Source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, Channel->fd, 0, dispatch_get_main_queue());
		dispatch_source_set_event_handler(Source, ^{
			while (!Channel->read_a_bit() || Channel->has_msg())
			{
				Msg* Mesg = Channel->get_msg();
				if( Mesg )
				{
					switch (Mesg->type)
					{
						case M_MON_GET_CS:
							[self handleCompileStatusMessage:Mesg];
							break;
						case M_MON_JOB_BEGIN:
							[self handleRemoteJobBegin:Mesg];
							break;
						case M_MON_JOB_DONE:
							[self handleRemoteJobDone:Mesg];
							break;
						case M_END:
							[self handleEnd];
							break;
						case M_MON_STATS:
							[self handleStatsMessage:Mesg];
							break;
						case M_MON_LOCAL_JOB_BEGIN:
							[self handleLocalJobBegin:Mesg];
							break;
						case M_JOB_LOCAL_DONE:
							[self handleLocalJobDone:Mesg];
							break;
						default:
							break;
					}
				}
				else
				{
					break;
				}
			}
		});
		dispatch_resume(Source);
	}
}

- (void)handleStatsMessage:(Msg*) Mesg
{
	MonStatsMsg* Stats = reinterpret_cast<MonStatsMsg*>( Mesg );
	NSMutableDictionary* Dict = (NSMutableDictionary*)[Hosts objectForKey:[NSNumber numberWithInt:Stats->hostid]];
	if ( !Dict )
	{
		Dict = [NSMutableDictionary new];
		[Dict setObject:[[NSBundle mainBundle] imageForResource:@"mac_client-512"] forKey:@"ICON"];
		[Dict setObject:[NSNumber numberWithBool:YES] forKey:@"Active"];
		[Dict setObject:@"Online" forKey:@"Status"];
	}
	[Dict setValue:[NSNumber numberWithInt:Stats->hostid] forKey:@"HostID"];
	NSString* StatsData = [NSString stringWithUTF8String: Stats->statmsg.c_str()];
	NSArray* Lines = [StatsData componentsSeparatedByString:@"\n"];
	for (NSString* Line in Lines)
	{
		NSArray* Components = [Line componentsSeparatedByString:@":"];
		if( [Components count] == 2 )
		{
			[Dict setValue:[Components objectAtIndex:1] forKey:[Components objectAtIndex:0]];
		}
	}
	[Hosts setObject:Dict forKey:[NSNumber numberWithInt:Stats->hostid]];
	if(![HostsArray containsObject:Dict])
	{
		[HostsArray addObject:Dict];
	}
}

- (void)handleRemoteJobBegin:(Msg*)Mesg
{
	MonJobBeginMsg* Job = (MonJobBeginMsg*)Mesg;
	NSMutableDictionary* Dict = [Jobs objectForKey:[NSNumber numberWithInt:Job->job_id]];
	if ( !Dict )
	{
		Dict = [NSMutableDictionary new];
	}
	[Dict setValue:[NSNumber numberWithInt:Job->job_id] forKey:@"JobID"];
	[Dict setValue:[NSNumber numberWithInt:Job->hostid] forKey:@"HostID"];
	[Dict setValue:[NSNumber numberWithInt:-1] forKey:@"ClientID"];
	[Dict setValue:[NSNumber numberWithInt:Job->stime] forKey:@"STime"];
	[Dict setValue:@"Remote" forKey:@"Execution"];
	[Jobs setObject:Dict forKey:[NSNumber numberWithInt:Job->job_id]];
	if(![JobsArray containsObject:Dict])
	{
		[JobsArray addObject:Dict];
	}

}

- (void)handleLocalJobBegin:(Msg*)Mesg
{
	MonLocalJobBeginMsg* Job = (MonLocalJobBeginMsg*)Mesg;
	NSMutableDictionary* Dict = [Jobs objectForKey:[NSNumber numberWithInt:Job->job_id]];
	if ( !Dict )
	{
		Dict = [NSMutableDictionary new];
	}
	[Dict setValue:[NSNumber numberWithInt:Job->job_id] forKey:@"JobID"];
	[Dict setValue:[NSNumber numberWithInt:Job->hostid] forKey:@"HostID"];
	[Dict setValue:[NSNumber numberWithInt:-1] forKey:@"ClientID"];
	[Dict setValue:[NSNumber numberWithInt:Job->stime] forKey:@"STime"];
	[Dict setValue:[NSString stringWithUTF8String:Job->file.c_str()] forKey:@"File"];
	[Dict setValue:@"Local" forKey:@"Execution"];
	[Jobs setObject:Dict forKey:[NSNumber numberWithInt:Job->job_id]];
	if(![JobsArray containsObject:Dict])
	{
		[JobsArray addObject:Dict];
	}
}

- (void)handleCompileStatusMessage:(Msg*)Mesg
{
	MonGetCSMsg* CS = (MonGetCSMsg*)Mesg;
	NSMutableDictionary* Dict = [Jobs objectForKey:[NSNumber numberWithInt:CS->job_id]];
	if ( !Dict )
	{
		Dict = [NSMutableDictionary new];
	}
	[Dict setValue:[NSNumber numberWithInt:CS->job_id] forKey:@"JobID"];
	[Dict setValue:[NSNumber numberWithInt:CS->clientid] forKey:@"ClientID"];
	[Dict setValue:[NSString stringWithUTF8String:CS->filename.c_str()] forKey:@"File"];
	[Dict setValue:CompileLanguage(CS->lang) forKey:@"Language"];
	[Dict setValue:CompileFlags(CS->arg_flags) forKey:@"Arguments"];
	if(![Dict valueForKey:@"HostID"])
	{
		[Dict setValue:[NSNumber numberWithInt:-1] forKey:@"HostID"];
	}
	if(![Dict valueForKey:@"Execution"])
	{
		[Dict setValue:@"Unknown" forKey:@"Execution"];
	}
	[Jobs setObject:Dict forKey:[NSNumber numberWithInt:CS->job_id]];
	if(![JobsArray containsObject:Dict])
	{
		[JobsArray addObject:Dict];
	}
}

- (void)handleRemoteJobDone:(Msg*)Mesg
{
	MonJobDoneMsg* Job = (MonJobDoneMsg*)Mesg;
	NSMutableDictionary* Dict = [Jobs objectForKey:[NSNumber numberWithInt:Job->job_id]];
	if ( !Dict )
	{
		Dict = [NSMutableDictionary new];
	}
	[Dict setValue:[NSNumber numberWithInt:Job->job_id] forKey:@"JobID"];
	[Dict setValue:[NSNumber numberWithInt:Job->exitcode] forKey:@"ExitCode"];
	[Dict setValue:[NSNumber numberWithInt:Job->real_msec] forKey:@"RealMS"];
	[Dict setValue:[NSNumber numberWithInt:Job->user_msec] forKey:@"UserMS"];
	[Dict setValue:[NSNumber numberWithInt:Job->sys_msec] forKey:@"SysMS"];
	[Dict setValue:[NSNumber numberWithInt:Job->pfaults] forKey:@"PageFaults"];
	[Dict setValue:[NSNumber numberWithInt:Job->in_compressed] forKey:@"InCompressed"];
	[Dict setValue:[NSNumber numberWithInt:Job->in_uncompressed] forKey:@"InUncompressed"];
	[Dict setValue:[NSNumber numberWithInt:Job->out_compressed] forKey:@"OutCompressed"];
	[Dict setValue:[NSNumber numberWithInt:Job->out_uncompressed] forKey:@"OutUncompressed"];
	[Dict setValue:[NSNumber numberWithLongLong:time(nullptr)] forKey:@"ETime"];
	[CompleteJobs setObject:Dict forKey:[NSNumber numberWithInt:Job->job_id]];
	[Jobs removeObjectForKey:[NSNumber numberWithInt:Job->job_id]];
	if(![JobsArray containsObject:Dict])
	{
		[JobsArray addObject:Dict];
	}
}

- (void)handleLocalJobDone:(Msg*)Mesg
{
	JobLocalDoneMsg* Job = (JobLocalDoneMsg*)Mesg;
	NSMutableDictionary* Dict = [Jobs objectForKey:[NSNumber numberWithInt:Job->job_id]];
	if(Dict)
	{
		[JobsArray removeObject:Dict];
		[Jobs removeObjectForKey:[NSNumber numberWithInt:Job->job_id]];
	}
}

- (void)handleTimerEvent
{
	if (!Scheduler)
	{
		DiscoverSched* DiscoverObj = (DiscoverSched*)Discover;
		if (!DiscoverObj || DiscoverObj->timed_out())
		{
			if( Connect )
			{
				dispatch_source_cancel(Connect);
				Connect = nullptr;
			}
			std::string net = [NetName UTF8String];
			std::string sched = [SchedulerName UTF8String];
			
			Discover = new DiscoverSched(net, (int)Timeout, sched, (int)Port);
			DiscoverObj = (DiscoverSched*)Discover;
			assert(Discover);
			[self registerConnectionhandler];
		}
	}
	else
	{
		NSEnumerator* It = [CompleteJobs keyEnumerator];
		id Key = nullptr;
		uint64_t Now = time(nullptr);
		NSMutableArray* Done = [NSMutableArray new];
		while ( (Key = [It nextObject]) )
		{
			NSDictionary* Job = (NSDictionary*)[CompleteJobs objectForKey:Key];
			uint64_t EndTime = [[Job valueForKey:@"ETime"] longLongValue];
			if ((Now - EndTime) > 60)
			{
				[Done addObject:Key];
				[JobsArray removeObject:Job];
			}
		}
		[CompleteJobs removeObjectsForKeys:Done];
	}
}

- (void)handleEnd
{
	if(Source)
	{
		dispatch_source_cancel(Source);
		Source = nullptr;
	}
	if(Scheduler)
	{
		delete (MsgChannel*)Scheduler;
		Scheduler = nullptr;
	}
	[Hosts removeAllObjects];
	[Jobs removeAllObjects];
	[CompleteJobs removeAllObjects];
	[HostsArray removeAllObjects];
	[JobsArray removeAllObjects];
}

- (NSDictionary*)hosts
{
	return Hosts;
}

- (NSDictionary*)jobs
{
	return Jobs;
}

- (NSDictionary*)completeJobs
{
	return CompleteJobs;
}

- (NSArray*)hostsArray
{
	return HostsArray;
}

- (NSArray*)jobsArray
{
	return JobsArray;
}

@end
