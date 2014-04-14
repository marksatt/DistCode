//
//  DOTabbar.h
//  DOTabbar
//
//  Created by Dmitry Obukhov on 30.03.13.
//  Copyright (c) 2013 Dmitry Obukhov. All rights reserved.
//

/*
 Copyright (c) 2013 Dmitry Obukhov
 
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

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