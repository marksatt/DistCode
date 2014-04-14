/*
 *
 * DistCode -- An OS X GUI & Xcode plugin for the distcc distributed C compiler.
 *
 * Copyright (C) 2013-14 by Mark Satterthwaite
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301,
 * USA.
 */

//
//  AppDelegate.h
//  DistCode
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
	IBOutlet NSArrayController* TasksController;
	NSMutableArray* DistCCServers;
	NSMutableArray* Tasks;
@protected
    // Keeps track of services handled by this delegate
    NSMutableArray *services;
	NSTask* DistCCDaemon;
	NSTask* DmucsDaemon;
	NSTask* DmucsMonDaemon;
	NSPipe* DistCCPipe;
	NSPipe* DmucsPipe;
	NSPipe* DmucsMonPipe;
	NSTimer* MonitorLoopTimer;
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
