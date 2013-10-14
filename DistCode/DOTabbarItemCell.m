//
//  DOTabbarItemCell.m
//  DOTabbar
//
//  Created by Dmitry Obukhov on 30.03.13.
//  Copyright (c) 2013 Dmitry Obukhov. All rights reserved.
//

#import "DOTabbarItemCell.h"

@implementation DOTabbarItemCell

#pragma mark - Рисование

- (id)init
{
	self = [super init];
	
	if (self != nil) {
		[self setBezelStyle:NSShadowlessSquareBezelStyle];
		
		[self setImagePosition:NSImageAbove];
		[self setImageScaling:NSImageScaleProportionallyDown];
		
		[self setFont:[NSFont systemFontOfSize:[NSFont smallSystemFontSize]]];
	}
	
	return self;
}

- (void)drawBezelWithFrame:(NSRect)frame inView:(NSView *)controlView
{
	if (self.state == NSOnState) {
		[NSGraphicsContext saveGraphicsState];
		
		[[NSGraphicsContext currentContext] setShouldAntialias:NO];
		
		NSGradient *bgGradient = [[NSGradient alloc] initWithStartingColor:[NSColor colorWithCalibratedWhite:0.82 alpha:0.]
															   endingColor:[NSColor colorWithCalibratedWhite:0.82 alpha:1.0]];
		
		NSGradient* borderGradient = [[NSGradient alloc] initWithStartingColor:[NSColor colorWithCalibratedWhite:0.7 alpha:0.]
																   endingColor:[NSColor colorWithCalibratedWhite:0.7 alpha:1.0]];
		
		[bgGradient drawInRect:frame angle:90.0];
		
		[borderGradient drawInRect:NSMakeRect(NSMinX(frame), NSMinY(frame), 1.0, NSHeight(frame)) angle:90.0];
		[borderGradient drawInRect:NSMakeRect(NSMaxX(frame) - 1, NSMinY(frame), 1.0, NSHeight(frame)) angle:90.0];
		
		[NSGraphicsContext restoreGraphicsState];
	}
}

#pragma mark -

- (NSRect)drawingRectForBounds:(NSRect)theRect
{
	theRect.size.height -= 10;
	theRect.origin.y += 10;
	
	return theRect;
}

@end
