//
//  DOTabBar.m
//  DOTabBar
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

#import "DOTabbar.h"

static NSSize const DefaultItemSize = {56.0, 68.0};
static CGFloat const DefaultGroupTitleHeight = 18.0;
static CGFloat const DefaultIntergroupSpacing = 32.0;

static CGFloat const BottomBorderWidth = 1.0;

static NSDictionary *GroupTitleAttributes;

@implementation DOTabbar {
	NSMutableArray *_matrices;
	NSMutableDictionary *_cachedCells;
	
	BOOL _hasGroupTitles;
	NSMutableDictionary *_groupTitles;
	
	BOOL _needsUpdateIntrinsicContentSize;
	NSSize _intrinsicContentSize;
}

#pragma mark - Initialization

+ (void)initialize
{
	NSMutableParagraphStyle *style = [NSMutableParagraphStyle new];
	[style setAlignment:NSCenterTextAlignment];
	[style setLineBreakMode:NSLineBreakByTruncatingTail];
	
	GroupTitleAttributes = @{
		NSParagraphStyleAttributeName: style,
		NSForegroundColorAttributeName: [NSColor windowFrameColor],
		NSFontAttributeName: [NSFont fontWithName:@"Helvetica" size:12.0]};
}

- (id)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	
	if (self) {
		[self setContentHuggingPriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationVertical];
		[self setContentCompressionResistancePriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationHorizontal];
		[self setContentCompressionResistancePriority:NSLayoutPriorityDefaultHigh forOrientation:NSLayoutConstraintOrientationVertical];
		
		self.itemSize = DefaultItemSize;
		self.intergroupSpacing = DefaultIntergroupSpacing;
	}

	return self;
}

#pragma mark -

- (void)reloadItems
{
	if (_matrices != nil) {
		[_matrices makeObjectsPerformSelector:@selector(removeFromSuperview)];
	}
	
	NSArray *groupIdentifiers = [self.delegate tabbarGroupIdentifiers:self];
	
	_matrices = [NSMutableArray arrayWithCapacity:[groupIdentifiers count]];
	
	_hasGroupTitles = NO;
	_groupTitles = [NSMutableDictionary dictionary];
	
	_cachedCells = [NSMutableDictionary dictionary];
	
	
	for (NSString *groupIdentifier in groupIdentifiers) {
		if ([self.delegate respondsToSelector:@selector(tabbar:titleForGroupIdentifier:)]) {
			NSString *title = [self.delegate tabbar:self titleForGroupIdentifier:groupIdentifier];
			
			if (title != nil) {
				[_groupTitles setObject:title forKey:groupIdentifier];
				_hasGroupTitles = YES;
			}
		}
		
		NSArray *groupItemIdentifiers = [self.delegate tabbar:self itemIdentifiersForGroupIdentifier:groupIdentifier];
		NSMutableArray *groupCells = [NSMutableArray arrayWithCapacity:[groupItemIdentifiers count]];
		
		for (NSString *itemIdentifier in groupItemIdentifiers) {
			NSAssert([_cachedCells objectForKey:itemIdentifier] == nil, @"Duplicated item identifier \"%@\"", itemIdentifier);
			
			NSCell *cell = [self.delegate tabbar:self cellForItemIdentifier:itemIdentifier];
			
			if (cell.identifier == nil) {
				cell.identifier = itemIdentifier;
			}
			
			NSAssert([cell.identifier isEqualToString:itemIdentifier], @"Undefined item identifier \"%@\"", itemIdentifier);
			
			[_cachedCells setObject:cell forKey:itemIdentifier];
			
			[groupCells addObject:cell];
		}
		
		NSMatrix *matrix = [self createMatrixWithIdentifier:groupIdentifier cells:groupCells];
		[_matrices addObject:matrix];
		[self addSubview:matrix];
	}
	
	[self setNeedsUpdateIntrinsicContentSize];
}

- (NSMatrix *)createMatrixWithIdentifier:(NSString *)identifier cells:(NSArray *)cells
{
	NSMatrix *matrix = [[NSMatrix alloc] initWithFrame:NSZeroRect
												  mode:NSRadioModeMatrix
											 cellClass:[NSCell class]
										  numberOfRows:0
									   numberOfColumns:[cells count]];
	
	[matrix setIdentifier:identifier];
	[matrix setAllowsEmptySelection:YES];
	[matrix setIntercellSpacing:NSZeroSize];
	[matrix setTarget:self];
	[matrix setAction:@selector(selectedItemChanged:)];
	
	[matrix addRowWithCells:cells];
	
	// No idea why NSMatrix don't do that
	[cells makeObjectsPerformSelector:@selector(setControlView:) withObject:matrix];
	
	return matrix;
}

#pragma mark - Selection handling

- (void)selectedItemChanged:(id)sender
{
	NSCell *selectedCell = [sender selectedCell];
	
	if (selectedCell != nil) {
		[_matrices makeObjectsPerformSelector:@selector(deselectAllCells)];
		[(NSMatrix *)selectedCell.controlView selectCell:selectedCell];
		
		[self selectItemWithIdentifier:selectedCell.identifier];
		
		if ([self.delegate respondsToSelector:@selector(tabbar:didSelectItemWithIdentifier:)]) {
			[self.delegate tabbar:self didSelectItemWithIdentifier:self.selectedIdentifier];
		}
	}
}

- (void)selectItemWithIdentifier:(NSString *)identifier
{
	if ([identifier isEqualToString:self.selectedIdentifier]) {
		return;
	}
	
	[_matrices makeObjectsPerformSelector:@selector(deselectAllCells)];
	
	NSCell *cell = [_cachedCells objectForKey:identifier];
	
	[self willChangeValueForKey:@"selectedIdentifier"];
	
	if (cell != nil) {
		[(NSMatrix *)cell.controlView selectCell:cell];
		
		_selectedIdentifier = identifier;
	} else {
		_selectedIdentifier = nil;
	}
	
	[self didChangeValueForKey:@"selectedIdentifier"];
}

#pragma mark - Layout

- (NSSize)intrinsicContentSize
{
	[self updateIntrinsicContentSizeIfNeeded];
	return _intrinsicContentSize;
}

- (void)setNeedsUpdateIntrinsicContentSize
{
	_needsUpdateIntrinsicContentSize = YES;
	[self invalidateIntrinsicContentSize];
}

- (void)updateIntrinsicContentSizeIfNeeded
{
	if (_needsUpdateIntrinsicContentSize) {
		[self updateIntrinsicContentSize];
	}
}

- (void)updateIntrinsicContentSize
{
	_intrinsicContentSize = NSMakeSize(0, BottomBorderWidth);
	
	if (_hasGroupTitles) {
		_intrinsicContentSize.height += DefaultGroupTitleHeight;
	}
	
	if ([_matrices count] > 0) {
		_intrinsicContentSize.height += self.itemSize.height;
		
		for (NSMatrix *matrix in _matrices) {
			[matrix setCellSize:self.itemSize];
			[matrix sizeToCells];
			
			_intrinsicContentSize.width += NSWidth(matrix.frame) + self.intergroupSpacing;
		}
		
		// Remove last group spacing
		_intrinsicContentSize.width -= self.intergroupSpacing;
	}
	
	_needsUpdateIntrinsicContentSize = NO;
}

- (void)resizeSubviewsWithOldSize:(NSSize)oldSize
{
	CGFloat availableWidth = NSWidth(self.bounds);
	
	NSPoint offset = NSMakePoint((availableWidth - _intrinsicContentSize.width) *0.5, BottomBorderWidth);
	
	for (NSMatrix *matrix in _matrices) {
		[matrix setFrameOrigin:offset];
		
		offset.x += NSWidth(matrix.frame) + self.intergroupSpacing;
	}
}

#pragma mark - Drawing

- (void)drawRect:(NSRect)dirtyRect
{
	[super drawRect:dirtyRect];
	
	[NSGraphicsContext saveGraphicsState];
	
	// Draw background
	[[NSColor whiteColor] setFill];
	NSRectFill(dirtyRect);
	
	// Draw border
	[[NSColor windowFrameColor] setFill];
	NSRectFill(NSMakeRect(0, 0, NSWidth(self.bounds), BottomBorderWidth));
	
	// Draw groups titles
	if (_hasGroupTitles) {
		for (NSMatrix *matrix in _matrices) {
			NSString *title = [_groupTitles objectForKey:matrix.identifier];
			
			if (title != nil) {
				NSSize titleSize = [title sizeWithAttributes:GroupTitleAttributes];
				NSRect titleRect = [self titleRectForMatrix:matrix];
				
				titleRect.size.height = titleSize.height;
				
				[self drawGroupTitle:title inRect:titleRect];
			}
		}
	}
	
	[NSGraphicsContext restoreGraphicsState];
}

- (void)drawGroupTitle:(NSString *)title inRect:(NSRect)rect
{
	[title drawInRect:NSIntegralRect(rect) withAttributes:GroupTitleAttributes];
}

- (NSRect)titleRectForMatrix:(NSMatrix *)matrix
{
	NSRect titleRect = matrix.frame;
	
	titleRect.origin.y += NSHeight(titleRect);
	titleRect.size.height = DefaultGroupTitleHeight;
	
	return titleRect;
}

#pragma mark - Setters

- (void)setDelegate:(id<DOTabbarDelegate>)delegate
{
	if (delegate != self.delegate) {
		_delegate = delegate;
		
		[self reloadItems];
	}
}

- (void)setSelectedIdentifier:(NSString *)selectedIdentifier
{
	if (![selectedIdentifier isEqualToString:self.selectedIdentifier]) {
		_selectedIdentifier = selectedIdentifier;
		
		[self selectItemWithIdentifier:selectedIdentifier];
	}
}

- (void)setItemSize:(NSSize)itemSize
{
	if (!NSEqualSizes(itemSize, self.itemSize)) {
		_itemSize = itemSize;
		
		[self setNeedsUpdateIntrinsicContentSize];
	}
}

- (void)setIntergroupSpacing:(CGFloat)intergroupSpacing
{
	if (intergroupSpacing != self.intergroupSpacing) {
		_intergroupSpacing = intergroupSpacing;
		
		[self setNeedsUpdateIntrinsicContentSize];
	}
}

@end
