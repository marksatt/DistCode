//
//  DOTabbar.h
//  DOTabbar
//
//  Created by Dmitry Obukhov on 30.03.13.
//  Copyright (c) 2013 Dmitry Obukhov. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DOTabbarItemCell.h"

@protocol DOTabbarDelegate;

@interface DOTabbar : NSView

@property (nonatomic, weak) IBOutlet id<DOTabbarDelegate> delegate;

@property (nonatomic, strong) NSString *selectedIdentifier;

@property (nonatomic, unsafe_unretained) NSSize itemSize;
@property (nonatomic, unsafe_unretained) CGFloat intergroupSpacing;

- (void)reloadItems;
- (void)selectItemWithIdentifier:(NSString *)identifier;

@end

@protocol DOTabbarDelegate <NSObject>

- (NSArray *)tabbarGroupIdentifiers:(DOTabbar *)tabbar;
- (NSArray *)tabbar:(DOTabbar *)tabbar itemIdentifiersForGroupIdentifier:(NSString *)identifier;
- (NSCell *)tabbar:(DOTabbar *)tabbar cellForItemIdentifier:(NSString *)itemIdentifier;

@optional

- (NSString *)tabbar:(DOTabbar *)tabbar titleForGroupIdentifier:(NSString *)identifier;
- (void)tabbar:(DOTabbar *)tabbar didSelectItemWithIdentifier:(NSString *)identifier;

@end