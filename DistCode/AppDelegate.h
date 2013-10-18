//
//  AppDelegate.h
//  Distcc
//
//  Created by Mark Satterthwaite on 30/09/2013.
//  Copyright (c) 2013 marksatt. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DOTabbar.h"
@class OBMenuBarWindow;

@interface AppDelegate : NSObject <NSApplicationDelegate, NSNetServiceBrowserDelegate, NSNetServiceDelegate, DOTabbarDelegate>
{
@public
	IBOutlet NSArrayController* DistCCServerController;
	NSMutableArray* DistCCServers;
@protected
    // Keeps track of services handled by this delegate
    NSMutableArray *services;
	NSTask* DistCCDaemon;
	NSTask* DmucsDaemon;
	NSTask* LoadAvgDaemon;
	NSPipe* DistCCPipe;
	NSPipe* DmucsPipe;
	NSPipe* LoadAvgPipe;
}

@property (assign) IBOutlet OBMenuBarWindow *window;
@property (weak) IBOutlet DOTabbar *tabbar;
@property (weak) IBOutlet NSTabView *TabView;

- (IBAction)toggleLaunchAtLogin:(id)sender;
- (IBAction)toggleRunCompilationHost:(id)sender;

// Other methods
- (void)netServiceDidResolveAddress:(NSNetService *)netService;
// Sent if resolution fails
- (void)netService:(NSNetService *)netService didNotResolve:(NSDictionary *)errorDict;
- (BOOL)addressesComplete:(NSArray *)addresses forServiceType:(NSString *)serviceType;
- (void)handleError:(NSNumber *)error withService:(NSNetService *)service;

- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didFindDomain:(NSString *)domainName moreComing:(BOOL)moreDomainsComing;
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didRemoveDomain:(NSString *)domainName moreComing:(BOOL)moreDomainsComing;
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didFindService:(NSNetService *)netService moreComing:(BOOL)moreServicesComing;
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didRemoveService:(NSNetService *)netService moreComing:(BOOL)moreServicesComing;
- (void)netServiceBrowserWillSearch:(NSNetServiceBrowser *)netServiceBrowser;
- (void)netServiceBrowser:(NSNetServiceBrowser *)netServiceBrowser didNotSearch:(NSDictionary *)errorInfo;
- (void)netServiceBrowserDidStopSearch:(NSNetServiceBrowser *)netServiceBrowser;
@end
