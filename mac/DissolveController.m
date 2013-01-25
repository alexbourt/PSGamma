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

#import "DissolveController.h"
#import "DissolveProxyView.h"

DissolveController *gDissolveController = NULL;

/* Make sure this is unique to you and everyone you might encounter, search for
"Preventing Name Conflicts" or use this link
http://developer.apple.com/mac/library/documentation/UserExperience/Conceptual/PreferencePanes/Tasks/Conflicts.html
*/

// get the current value and force an update
@implementation DissolveTextField
	
- (void)keyUp:(NSEvent *)theEvent 
{
	NSLog(@"start keyUp, %d", [theEvent keyCode]);
	[gDissolveController updateAmountValue];
	[gDissolveController updateProxy];
	NSLog(@"end keyUp, %d", gParams->percent);
}

@end

/* Make sure this is unique to you and everyone you might encounter, search for
"Preventing Name Conflicts" or use this link
http://developer.apple.com/mac/library/documentation/UserExperience/Conceptual/PreferencePanes/Tasks/Conflicts.html
*/

// controller for the entire dialog
@implementation DissolveController

+ (DissolveController *) dissolveController 
{
    return gDissolveController;
}


- (id) init 
{
    self = [super init];

    // initialize NSApplication, using an entry point that is specific to 
	// bundles this is a no-op for Cocoa apps, but is required by Carbon apps
    NSApplicationLoad();

	amountValue = [NSString stringWithFormat:@"%d", gParams->percent];
	
    if (![NSBundle loadNibNamed:@"DissolveDialog" owner:self]) 
	{
        NSLog(@"failed to load DissolveDialog xib");
    }
    
	gDissolveController = self;

    [textField setStringValue:amountValue];

	switch (gParams->disposition) 
	{
		case 0: // clear
			[dispositionClear setState:true];
			[dispositionCool setState:false];
			[dispositionHot setState:false];
			[dispositionSick setState:false];
			break;
		case 2: // hot
			[dispositionClear setState:false];
			[dispositionCool setState:false];
			[dispositionHot setState:true];
			[dispositionSick setState:false];
			break;
		case 3: // sick
			[dispositionClear setState:false];
			[dispositionCool setState:false];
			[dispositionHot setState:false];
			[dispositionSick setState:true];
			break;
		default:
		case 1: // cool
			[dispositionClear setState:false];
			[dispositionCool setState:true];
			[dispositionHot setState:false];
			[dispositionSick setState:false];
		break;
	}
	
	NSLog(@"Trying to set initial disposition");

	[(DissolveProxyView*)proxyPreview setDispositionColor:gParams->disposition];

	NSLog(@"Trying to set setNeedsDisplay");

	[proxyPreview setNeedsDisplay:YES];

	NSLog(@"Done with init");

    return self;
}

- (int) showWindow 
{
    [dissolveWindow makeKeyAndOrderFront:nil];
	int b = [[NSApplication sharedApplication] runModalForWindow:dissolveWindow];
	[dissolveWindow orderOut:self];
	return b;
}

- (NSString *) getAmountValue 
{
	return amountValue;
}

- (void) updateAmountValue 
{
	amountValue = [textField stringValue];
	NSLog(@"updateAmountValue dissolve %@", amountValue);
	gParams->percent = [amountValue intValue];
	NSLog(@"Percent after updateAmountValue: %d", gParams->percent);
}

- (IBAction) okPressed: (id) sender 
{
	amountValue = [textField stringValue];
	[NSApp stopModalWithCode:1];
	NSLog(@"after nsapp stopmodal");
}

- (IBAction) cancelPressed: (id) sender 
{
	NSLog(@"cancel pressed");
	[NSApp stopModalWithCode:0];
	NSLog(@"after nsapp abortmodal");
}

- (IBAction) clearPressed: (id) sender 
{
	NSLog(@"clear pressed");
	gParams->disposition = 0;
	[gDissolveController updateProxy];
}

- (IBAction) coolPressed: (id) sender 
{
	NSLog(@"cool pressed");
	gParams->disposition = 1;
	[gDissolveController updateProxy];
}

- (IBAction) hotPressed: (id) sender 
{
	NSLog(@"hot pressed");
	gParams->disposition = 2;
	[gDissolveController updateProxy];
}

- (IBAction) sickPressed: (id) sender 
{
	NSLog(@"sick pressed");
	gParams->disposition = 3;
	[gDissolveController updateProxy];
}

- (void) updateProxy 
{
	CopyColor(gData->color, gData->colorArray[gParams->disposition]);
	[(DissolveProxyView*)proxyPreview setDispositionColor:gParams->disposition];
	[proxyPreview setNeedsDisplay:YES];
}

- (void) updateCursor
{

	NSLog(@"Trying to update cursor cursor suite");
	// gFilterRecord->abortProc();

	sPSUIHooks->SetCursor(kPICursorArrow);
	
	NSLog(@"Tried to update cursor 6");
}

@end

/* Carbon entry point and C-callable wrapper functions*/
OSStatus initializeCocoaDissolve(void) 
{
	[[DissolveController alloc] init];
    return noErr;
}

OSStatus orderWindowFrontDissolve(void) 
{
    int okPressed = [[DissolveController dissolveController] showWindow];
    return okPressed;
}

// end DissolveController.m
