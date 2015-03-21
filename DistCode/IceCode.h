//
//  IceCode.h
//  DistCode
//
//  Created by Mark Satterthwaite on 23/02/2015.
//  Copyright (c) 2015 marksatt. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface IceCode : NSObject
{
@private
	NSInteger Timeout;
	NSUInteger Port;
	void* Discover;
	void* Scheduler;
	dispatch_source_t Connect;
	dispatch_source_t Source;
	NSString* NetName;
	NSString* SchedulerName;
	NSMutableDictionary* Hosts;
	NSMutableDictionary* Jobs;
	NSMutableDictionary* CompleteJobs;
	NSMutableArray* HostsArray;
	NSMutableArray* JobsArray;
	NSTimer* Timer;
}
+ (IceCode*) iceCodeForNetName:(NSString*)NetName withTimeout:(NSInteger)Timeout andSchedulerName:(NSString*)Scheduler andPort:(NSUInteger)Port;
- (NSString*)hostName;
- (NSString*)schedulerName;
- (NSDictionary*)hosts;
- (NSDictionary*)jobs;
- (NSDictionary*)completeJobs;
- (NSArray*)hostsArray;
- (NSArray*)jobsArray;
@end
