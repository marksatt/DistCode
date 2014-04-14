//
//  DOTabbarItemCell.m
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
