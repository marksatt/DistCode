//
//  DistCode.h
//  DistCode
//
//  Created by Mark Satterthwaite on 31/07/2015.
//  Copyright (c) 2015 marksatt. All rights reserved.
//

#ifndef DistCode_DistCode_h
#define DistCode_DistCode_h

#import <Cocoa/Cocoa.h>

@interface DistCode : NSObject
{
@private
	NSTask* DistCCDaemon;
	NSPipe* DistCCPipe;
	
	NSTask* DmucsDaemon;
	NSPipe* DmucsPipe;
	
	NSPipe* LoadAvgPipe;
	NSTask* LoadAvgDaemon;
}
- (BOOL)startup;
- (void)shutdown;
- (BOOL)restart;
- (NSTask*)beginDaemonTask:(NSString*)Path withArguments:(NSArray*)Arguments andPipe:(NSPipe*)Pipe;
- (NSString*)executeTask:(NSString*)Path withArguments:(NSArray*)Arguments;
- (void)startDistccDaemonAsCoordinator:(BOOL)AsCoordinator;
- (void)stopDistccDaemon;
- (void)startDmucsDaemon;
- (void)stopDmucsDaemon;
- (NSDictionary*)updateDmucs;
- (BOOL)startLoadAvg: (NSString*)Server;
- (void)stopLoadAvg;
- (BOOL)startCompileHost;
- (void)stopCompileHost;
- (NSMutableArray*)pumpDistccMon;
- (NSString*)hostName;
- (NSString*)hostProperty;
- (NSMutableDictionary*)hostInfo;
- (void)addHost:(NSString*)Host ToServer:(NSString*)Server withNumCPUS:(unsigned)NumCPUs andPowerIndex:(unsigned)Index;
- (void)removeHost:(NSString*)Host fromServer:(NSString*)Server;
- (BOOL)runCoordinator;
- (BOOL)runCompileHost;
- (BOOL)verboseOutput;
- (NSString*)coordinatorIPAddress;
- (NSNumber*)hostTimeout;
- (void)toggleCompileHost:(BOOL)State;
- (void)install;
@end

#endif
