// ADOBE SYSTEMS INCORPORATED
// Copyright  2009 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this 
// file in accordance with the terms of the Adobe license agreement
// accompanying it.  If you have received this file from a source
// other than Adobe, then your use, modification, or distribution
// of it requires the prior written permission of Adobe.
//-------------------------------------------------------------------------------

#import "DissolveProxyView.h"
#import "PIGeneral.h"
#import "Dissolve.h"
#import "FilterBigDocument.h"
#import "DissolveController.h"

extern void UpdateProxyBuffer(void);
extern void ResetProxyBuffer(void);

extern DissolveController *gDissolveController;

/* Make sure this is unique to you and everyone you might encounter, search for
"Preventing Name Conflicts" or use this link
http://developer.apple.com/mac/library/documentation/UserExperience/Conceptual/PreferencePanes/Tasks/Conflicts.html
*/
@implementation DissolveProxyView

- (id)initWithFrame:(NSRect)frame {
    self = [super initWithFrame:frame];
    if (self) {
		currentColor = [NSColor clearColor];
		NSLog(@"SetColor to clearColor");
    }
    return self; 
}

- (void)layoutSubviews
{
	NSLog(@"in layout");
}

- (BOOL)isFlipped
{
	return YES;
}

- (void)setDispositionColor:(int16)newColor
{
	NSLog(@"start setColor to %d", newColor);
	switch (newColor) {
		case 0:
			currentColor = [NSColor clearColor];
			NSLog(@"end setColor to clearColor");
			break;
		default:
		case 1:
			currentColor = [NSColor blueColor];
			NSLog(@"end setColor to blueColor");
			break;
		case 2:
			currentColor = [NSColor redColor];
			NSLog(@"end setColor to redColor");
			break;
		case 3:
			currentColor = [NSColor greenColor];
			NSLog(@"end setColor to greenColor");
			break;
	}
}

- (void)drawRect:(NSRect)rect {


	NSLog(@"in drawRect, %f %f %f %f", rect.size.width, rect.size.height, rect.origin.x, rect.origin.y);

	Rect r;
	short itemHeight, itemWidth;
	short bufferHeight, bufferWidth;
	PSPixelMap outMap;
	short dstRow;
	short dstCol;
	VRect srcRect;
	VRect inRect, outRect;
	
	itemHeight = (short)rect.size.height;
	itemWidth = (short)rect.size.width;
	
	gData->proxyRect.top = 0;
	gData->proxyRect.left = 0;
	gData->proxyRect.bottom = itemHeight;
	gData->proxyRect.right = itemWidth;
	
	r.top = gData->proxyRect.top;
	r.left = gData->proxyRect.left;
	r.bottom = gData->proxyRect.bottom;
	r.right = gData->proxyRect.right;
	
	SetupFilterRecordForProxy();
	CreateProxyBuffer();
	CreateDissolveBuffer(gData->proxyWidth, gData->proxyHeight);
	
	ResetProxyBuffer();
	UpdateProxyBuffer();
	
	bufferHeight = gData->proxyHeight;
	bufferWidth  = gData->proxyWidth;
	
	/* Set up the output map. */
	
	inRect = GetInRect();

	outMap.version       = 1;
	outMap.bounds.top    = inRect.top;
	outMap.bounds.left   = inRect.left;
	outMap.bounds.bottom = inRect.bottom;
	outMap.bounds.right  = inRect.right;
	outMap.imageMode     = DisplayPixelsMode(gFilterRecord->imageMode);
	outMap.rowBytes      = gData->proxyWidth;
	outMap.colBytes		 = 1;
	outMap.planeBytes = gData->proxyPlaneSize;
	outMap.baseAddr = gData->proxyBuffer;
	
	/* Version 1 fields.  MCH 7/13/94 */
	outMap.mat			= NULL;
	outMap.masks		= NULL;
	outMap.maskPhaseRow = 0;
	outMap.maskPhaseCol = 0;
	
	/* Compute where we are going to display it. */
	
	dstRow = (short)rect.origin.x;
	dstCol = (short)rect.origin.y;
	
	
	if (itemHeight > bufferHeight)
		{
		
		Rect eraseArea;
		eraseArea.top = gData->proxyRect.top;
		eraseArea.left = gData->proxyRect.left;
		eraseArea.bottom = gData->proxyRect.bottom;
		eraseArea.right = gData->proxyRect.right;

		dstRow += (itemHeight - bufferHeight) / 2;
		
		eraseArea.bottom = dstRow;
		
		//if (eraseArea.bottom > eraseArea.top && eraseArea.right  > eraseArea.left)
		//	EraseRect (&eraseArea);
			
		r.top = dstRow;
			
		eraseArea = r;
		eraseArea.top = dstRow + bufferHeight;
	
		//if (eraseArea.bottom > eraseArea.top && eraseArea.right  > eraseArea.left)
		//	EraseRect (&eraseArea);
			
		r.bottom = dstRow + bufferHeight;
		
		itemHeight = bufferHeight;
			
		}
			
	if (itemWidth > bufferWidth)
		{
		
		Rect eraseArea = r;

		dstCol += (itemWidth - bufferWidth) / 2;
		
		eraseArea.right = dstCol;
		
		//if (eraseArea.bottom > eraseArea.top && eraseArea.right  > eraseArea.left)
		//	EraseRect (&eraseArea);
			
		r.left = dstCol;
			
		eraseArea = r;
		eraseArea.left = dstCol + bufferWidth;
	
		//if (eraseArea.bottom > eraseArea.top && eraseArea.right  > eraseArea.left)
		//	EraseRect (&eraseArea);
			
		r.right = dstCol + bufferWidth;
		
		itemWidth = bufferWidth;
			
		}


	/* Compute the source. */
	
	outRect = GetOutRect();

	srcRect.top    = outRect.top;
	srcRect.left   = outRect.left;
	
	if (bufferHeight > itemHeight)
		srcRect.top += (bufferHeight - itemHeight) / 2;
		
	if (bufferWidth > itemWidth)
		srcRect.left += (bufferWidth - itemWidth) / 2;
		
	srcRect.bottom = srcRect.top  + itemHeight;
	srcRect.right  = srcRect.left + itemWidth;
	
	/* Display the data. */
	
	// taken from mondo...i need global rect here?
	PSPlatformContext windowContext;
	// set up global target rectangle for this blit
	CGContextRef cgContext = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort]; 
	windowContext.fCGContextRef = (void *) cgContext;
	windowContext.fScreenRect.top = srcRect.top;
	windowContext.fScreenRect.left = srcRect.left;
	windowContext.fScreenRect.bottom = srcRect.bottom;
	windowContext.fScreenRect.right = srcRect.right;

	if (gFilterRecord->displayPixels != NULL)
		(*(gFilterRecord->displayPixels)) (&outMap, &srcRect, dstRow, dstCol, &windowContext);

	[gDissolveController updateCursor];

	
#if VERSION_1_FILL_WITH_COLOR

	// Set the color in the current graphics context for future draw operations
	[[NSColor blackColor] setStroke];
	[currentColor setFill];
	
	
	
	// Create our drawing path
	NSBezierPath* drawingPath = [NSBezierPath bezierPath];

	// draw a rectangle
	[drawingPath appendBezierPathWithRect: rect];

	// actually draw it
	[drawingPath stroke] ;

	// and fill it
	[drawingPath fill] ;

#endif
	
}

@end

// end DissolveProxyView.m
