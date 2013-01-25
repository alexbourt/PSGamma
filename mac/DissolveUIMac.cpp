// ADOBE SYSTEMS INCORPORATED
// Copyright  1993 - 2002 Adobe Systems Incorporated
// All Rights Reserved
//
// NOTICE:  Adobe permits you to use, modify, and distribute this 
// file in accordance with the terms of the Adobe license agreement
// accompanying it.  If you have received this file from a source
// other than Adobe, then your use, modification, or distribution
// of it requires the prior written permission of Adobe.
//-------------------------------------------------------------------------------
//	Includes
//-------------------------------------------------------------------------------

#include "PIDefines.h"
#include "Dissolve.h"
#include "DissolveUI.h"
#include "DialogUtilities.h"
#include "FilterBigDocument.h"

#if !__LP64__ 

void UpdateProxyItem(DialogPtr dialogPtr);
VRect SetRect(Rect inRect);

/*****************************************************************************/

/* UserItem to draw the data from the output buffer. */

static pascal void ShowOutputBuffer (DialogPtr dp, short item)
	{
	
	Rect r;
	Handle h;
	short itemType;
	short itemHeight, itemWidth;
	short bufferHeight, bufferWidth;
	PSPixelMap outMap;
	short dstRow;
	short dstCol;
	VRect srcRect;
	VRect inRect, outRect;
	
	GetDialogItem (dp, item, &itemType, &h, &r);

	/* do the border */
	PenNormal ();
	InsetRect (&r, -2, -2);
	FrameRect (&r);
	InsetRect (&r, 2, 2); 
	
	itemHeight = r.bottom - r.top;
	itemWidth = r.right - r.left;
	
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
	
	dstRow = r.top;
	dstCol = r.left;
	
	if (itemHeight > bufferHeight)
		{
		
		Rect eraseArea = r;

		dstRow += (itemHeight - bufferHeight) / 2;
		
		eraseArea.bottom = dstRow;
		
		if (eraseArea.bottom > eraseArea.top && eraseArea.right  > eraseArea.left)
			EraseRect (&eraseArea);
			
		r.top = dstRow;
			
		eraseArea = r;
		eraseArea.top = dstRow + bufferHeight;
	
		if (eraseArea.bottom > eraseArea.top && eraseArea.right  > eraseArea.left)
			EraseRect (&eraseArea);
			
		r.bottom = dstRow + bufferHeight;
		
		itemHeight = bufferHeight;
			
		}
			
	if (itemWidth > bufferWidth)
		{
		
		Rect eraseArea = r;

		dstCol += (itemWidth - bufferWidth) / 2;
		
		eraseArea.right = dstCol;
		
		if (eraseArea.bottom > eraseArea.top && eraseArea.right  > eraseArea.left)
			EraseRect (&eraseArea);
			
		r.left = dstCol;
			
		eraseArea = r;
		eraseArea.left = dstCol + bufferWidth;
	
		if (eraseArea.bottom > eraseArea.top && eraseArea.right  > eraseArea.left)
			EraseRect (&eraseArea);
			
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
	
	if (gFilterRecord->displayPixels != NULL)
		(*(gFilterRecord->displayPixels)) (&outMap, &srcRect, dstRow, dstCol, gFilterRecord->platformData);
			
	}

/*****************************************************************************/

UserItemUPP gUserItemShowOutput = NULL;

/*****************************************************************************/

/* Set up an item so that it will display the data in the output portion of
   the filter record. */

static void MakeOutputProxy (DialogPtr dp, short proxyItem)
	{
	
	short itemType;
	Rect r;
	Handle h;

	GetDialogItem (dp, proxyItem, &itemType, &h				  		  , &r);
	gUserItemShowOutput = NewUserItemUPP(ShowOutputBuffer);
	SetDialogItem (dp, proxyItem,  itemType, (Handle) gUserItemShowOutput, &r);
	
	}

/*****************************************************************************/
VRect SetRect(Rect inRect) 
{
	VRect outRect = { 0, 0, 0, 0};
	outRect.top = inRect.top;
	outRect.left = inRect.left;
	outRect.bottom = inRect.bottom;
	outRect.right = inRect.right;
	return outRect;
}
	
/* Display modal dialog, using proxy routines to preview results. */
Boolean DoUI (void)

	{

	long x = 0;
	short item, lastitem;
	short itemType;
	short numberErr;
	Handle h;
	DialogPtr dp;
	DialogTHndl dt;
	
	dt = (DialogTHndl) GetResource ('DLOG', uiID);
	HNoPurge ((Handle) dt);
	
	CenterDialog (dt);

	dp = GetNewDialog (uiID, nil, (WindowPtr) -1);

	Rect proxyRect;
	
	GetDialogItem (dp, kDProxyItem, &itemType, &h, &proxyRect);
	gData->proxyRect = SetRect(proxyRect);
	SetupFilterRecordForProxy ();
	CreateProxyBuffer();
	CreateDissolveBuffer(gData->proxyWidth, gData->proxyHeight);
	UpdateProxyItem(dp);
	MakeOutputProxy (dp, kDProxyItem);
	
	(void) SetDialogDefaultItem (dp, ok);
	(void) SetDialogCancelItem (dp, cancel);
	(void) SetDialogTracksCursor (dp, TRUE);
	
	StuffNumber (dp,
				 kDEditText,
				 gParams->percent);
	
	SetCheckBoxState (dp, kDEntireImage, gParams->ignoreSelection);
	ShowHideItem(dp, kDEntireImage, gFilterRecord->haveMask);
				 	
	SetRadioGroupState (dp, kDFirstRadio, kDLastRadio,  kDFirstRadio + gParams->disposition);
			
	SelectTextItem (dp, kDEditText); 

	ShowWindow (GetDialogWindow(dp));

	do
	{
		
		MoveableModalDialog (dp, gFilterRecord->processEvent, nil, &item);

		if (lastitem != item && item != cancel)
		{ /* we just left this area.  Check to make sure its within bounds. */
			switch (lastitem)
			{
				case kDEditText:
					numberErr = FetchNumber(dp,
										    kDEditText,
										    0,
										    99,
										    &x);
					if (numberErr != noErr)
					{ // shows alert if there's an error
						AlertNumber(dp,
									kDEditText,
									0,
									99,
									&x,
								    AlertID,
								    numberErr);
						item = kDEditText; // stay here
					}
			}
		}
		switch (item)
		{
			case kDEntireImage:
				gParams->ignoreSelection = !gParams->ignoreSelection;
				SetCheckBoxState(dp, kDEntireImage, gParams->ignoreSelection);
				UpdateProxyItem(dp);
				break;

			case kDEditText:
				// grab the number whether it's right or not
				numberErr = FetchNumber(dp, kDEditText, 0, 99, &x);
				if (numberErr == noErr)
				{ // no errors getting the number
					if (gParams->percent != x)
					{ // New number.  Update with it.
						gParams->percent = x;	 
						UpdateProxyItem(dp);
					}
				}
				break;

			default:
				if ((item >= kDFirstRadio) && (item <= kDLastRadio))
				{
					SetRadioGroupState (dp, kDFirstRadio, kDLastRadio, item);
					gParams->disposition = item - kDFirstRadio;
					CopyColor(gData->color, gData->colorArray[gParams->disposition]);
					UpdateProxyItem(dp);
				}
					break;
		}
		lastitem = item;
	}
	while (item != ok && item != cancel);

	DisposeDialog (dp);
	HPurge ((Handle) dt);
	
	if ( gUserItemShowOutput != NULL )
		DisposeUserItemUPP(gUserItemShowOutput);
	if ( gUserItemOutlineOK != NULL )
		DisposeUserItemUPP(gUserItemOutlineOK);
	if ( gUserItemOutlineGroup != NULL )
		DisposeUserItemUPP(gUserItemOutlineGroup);
	if ( gFilterProc != NULL )
		DisposeModalFilterUPP(gFilterProc);

	DeleteProxyBuffer();

	return (item == ok);
}

//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------
//
// UpdateProxyItem
//
// Force the WM_PAINT message to the DissolveProc routine.
//
//-------------------------------------------------------------------------------
void UpdateProxyItem(DialogPtr dialogPtr)
{
	if (*gResult == noErr)
	{
		ResetProxyBuffer();
		UpdateProxyBuffer();
		InvalItem(dialogPtr, kDProxyItem);
	}
}



//-------------------------------------------------------------------------------
//
// DoAbout
//
// Pop a simple about box for this plug in.
//
// NOTE:	The global gFilterRecord is NOT a FilterRecord*. You must cast it to
//			an AboutRecord*. See PIAbout.h
//
//-------------------------------------------------------------------------------
void DoAbout(void)
{
	// AboutRecord* aboutRecord = (AboutRecord*)gFilterRecord;
	ShowAbout(AboutID);
}

#endif // #if !__LP64__ 

// end DissolveUIMac.cpp 