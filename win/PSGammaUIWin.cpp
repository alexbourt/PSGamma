#include "PSGamma.h"
#include "PSGammaUI.h"
#include "FilterBigDocument.h"
#include "resource.h"
#include <CommCtrl.h>
#include <math.h>

void GetProxyItemRect(HWND hDlg);
void PaintProxy(HWND hDlg);
void UpdateProxyItem(HWND hDlg);

//

DLLExport BOOL WINAPI GammaProc(HWND hDlg, 
								UINT wMsg, 
								WPARAM wParam, 
								 LPARAM lParam)
{
	int	item, cmd;
	switch (wMsg)
	{
		case WM_INITDIALOG:
			{
				CenterDialog(hDlg);
				// recalculate the proxy and update the pixel data in inData
				GetProxyItemRect(hDlg);
				SetupFilterRecordForProxy();
				CreateProxyBuffer();
				CreateDissolveBuffer(gData->proxyWidth, gData->proxyHeight);

				//SetFocus(GetDlgItem(hDlg, IDC_GAMMASLIDER));

				// set the initial gamma value and slider position & range

				char szGamma[32];
				sprintf_s(szGamma, 32, "%.02f", gParams->gamma);

				// clean up the trailing zeros except for one

				char* pszGamma = szGamma;
				while (*(pszGamma+1)) pszGamma++;
				while (*pszGamma == '0' && *(pszGamma-1) != '.') { *pszGamma = NULL; pszGamma--; }

				SetDlgItemText(hDlg, IDC_GAMMAEDIT, szGamma);

				HWND hwndSlider = GetDlgItem(hDlg, IDC_GAMMASLIDER);
				SendMessage(hwndSlider, TBM_SETRANGE, true, MAKELONG(0, 800));
				SendMessage(hwndSlider, TBM_SETPOS, true, 400);

				SendMessage(hwndSlider, TBM_SETPOS, TRUE, (UINT)(sqrt((gParams->gamma - 0.25) / 3.75) * 800));

				HWND hwndInvert = GetDlgItem(hDlg, IDC_CHECKINVERT);
				SendMessage(hwndInvert, BM_SETCHECK, gParams->invert, 0);

				UpdateGammaLut();
				UpdateProxyItem(hDlg);
			}
			// intentional fall through to the WM_PAINT message
		case WM_PAINT:
			if (*gResult == noErr)
				PaintProxy(hDlg);
			return FALSE;
			break;

		case WM_HSCROLL:
			{
				HWND hwndSlider = GetDlgItem(hDlg, IDC_GAMMASLIDER);

				if ((HWND)lParam == hwndSlider)
				{
					if (LOWORD(wParam) == TB_THUMBTRACK
						|| LOWORD(wParam) == TB_THUMBPOSITION
						|| LOWORD(wParam) == TB_PAGEDOWN
						|| LOWORD(wParam) == TB_PAGEUP)
					{
						HWND hwndSlider = GetDlgItem(hDlg, IDC_GAMMASLIDER);
						
						gParams->gamma = (double)SendMessage(hwndSlider, TBM_GETPOS, false, false) / 800;

						gParams->gamma = gParams->gamma*gParams->gamma * 3.75 + 0.25;

						char szGamma[32];
						sprintf_s(szGamma, 32, "%.02f", gParams->gamma);

						char* pszGamma = szGamma;
						while (*(pszGamma+1)) pszGamma++;
						while (*pszGamma == '0' && *(pszGamma-1) != '.') { *pszGamma = NULL; pszGamma--; }

						SetDlgItemText(hDlg, IDC_GAMMAEDIT, szGamma);

						UpdateGammaLut();
						UpdateProxyItem(hDlg);
					}
				}

				break;
			}

		case WM_COMMAND:
			item = LOWORD (wParam);
			cmd = HIWORD (wParam);
			switch (item)
			{
				case IDC_GAMMAEDIT:
					if (cmd == EN_CHANGE)
					{
						HWND hwndSlider = GetDlgItem(hDlg, IDC_GAMMASLIDER);
						
						char szGamma[32];
						GetDlgItemText(hDlg, IDC_GAMMAEDIT, szGamma, 31);

						gParams->gamma = atof(szGamma);
							
						SendMessage(hwndSlider, TBM_SETPOS, TRUE, (UINT)(sqrt((gParams->gamma - 0.25) / 3.75) * 800));

						UpdateGammaLut();
						UpdateProxyItem(hDlg);
					}
					break;

				case IDC_CHECKINVERT:
					if (cmd == BN_CLICKED)
					{
						HWND hwndInvert = GetDlgItem(hDlg, IDC_CHECKINVERT);
						gParams->invert = SendMessage(hwndInvert, BM_GETCHECK, 0, 0) != FALSE;

						UpdateGammaLut();
						UpdateProxyItem(hDlg);
					}
					break;

				case kDOK:
				case kDCancel:
					if (cmd == BN_CLICKED)
					{
						DeleteProxyBuffer();
						DeleteGammaBuffer();
						EndDialog(hDlg, item);
						return TRUE;
					}
					break;
			}
			return TRUE;
			break;

		default:
			return FALSE;
			break;
	}
	return TRUE;
}

Boolean DoUI(void)
{
	PlatformData* platform = (PlatformData*)(gFilterRecord->platformData);

	INT_PTR result = DialogBoxParam(GetDLLInstance(),
							(LPSTR)"PSGAMMADIALOG",
							(HWND)platform->hwnd,
							(DLGPROC)GammaProc,
							NULL);
	//used by the GammaProc routine, don't let the error go further
	*gResult = noErr;
	return (result == kDOK);
}

void UpdateProxyItem(HWND hDlg)
{
	RECT imageRect;

	if (*gResult == noErr)
	{
		ResetProxyBuffer();
		UpdateProxyBuffer();
		GetWindowRect(GetDlgItem(hDlg, kDProxyItem), &imageRect);
		ScreenToClient(hDlg, (LPPOINT)&imageRect);
		ScreenToClient(hDlg, (LPPOINT)&(imageRect.right));
		InvalidateRect(hDlg, &imageRect, FALSE);
	}
}

void GetProxyItemRect(HWND hDlg)
{
	RECT	wRect;
	
	GetWindowRect(GetDlgItem(hDlg, kDProxyItem), &wRect);	
	ScreenToClient (hDlg, (LPPOINT)&wRect);
	ScreenToClient (hDlg, (LPPOINT)&(wRect.right));

	gData->proxyRect.left = (short)wRect.left;
	gData->proxyRect.top  = (short)wRect.top;
	gData->proxyRect.right = (short)wRect.right;
	gData->proxyRect.bottom = (short)wRect.bottom;
}

//-------------------------------------------------------------------------------
void PaintProxy(HWND hDlg)
{
	PSPixelMap pixels;
	PSPixelMask mask;
	PAINTSTRUCT  ps;
	VRect  itemBounds;
	RECT  wRect;
	POINT	mapOrigin; 
	HDC		hDC;

	// find the proxy in screen cordinates and center the
	// proxy rectangle
	GetWindowRect(GetDlgItem(hDlg, kDProxyItem), &wRect);
	mapOrigin.x = 0;
	mapOrigin.y = 0;
	ClientToScreen(hDlg, &mapOrigin);

	VRect inRect = GetInRect();

	itemBounds.left = (((wRect.right + wRect.left - 
		                        inRect.right + 
								inRect.left) / 
								2) - mapOrigin.x);
	itemBounds.top = (((wRect.bottom + wRect.top - 
		                       inRect.bottom + 
							   inRect.top) / 
							   2) - mapOrigin.y);
	itemBounds.right = (itemBounds.left + 
		                       (inRect.right - 
							   inRect.left));
	itemBounds.bottom = (itemBounds.top + 
		                        (inRect.bottom - 
								inRect.top));
	
	hDC = BeginPaint(hDlg, &ps);	

	wRect.left = itemBounds.left;
	wRect.top = itemBounds.top;
	wRect.right = itemBounds.right;
	wRect.bottom = itemBounds.bottom;

	// paint the black frame with a one pixel space
	// between the image and the frame
	InflateRect(&wRect, 2, 2);
	FrameRect(hDC, &wRect, (HBRUSH)GetStockObject(BLACK_BRUSH));	
	InflateRect(&wRect, -2, -2);
	
	// init the PSPixel map
	pixels.version = 1;
	pixels.bounds.top = inRect.top;
	pixels.bounds.left = inRect.left;
	pixels.bounds.bottom = inRect.bottom;
	pixels.bounds.right = inRect.right;
	pixels.imageMode = DisplayPixelsMode(gFilterRecord->imageMode);
	pixels.rowBytes = gData->proxyWidth;
	pixels.colBytes = 1;
	pixels.planeBytes = gData->proxyPlaneSize;
	pixels.baseAddr = gData->proxyBuffer;

	pixels.mat = NULL;
	pixels.masks = NULL;
	pixels.maskPhaseRow = 0;
	pixels.maskPhaseCol = 0;

	// display the transparency information if it exists
	if (gFilterRecord->isFloating != NULL) 
	{
		mask.next = NULL;
		mask.maskData = gFilterRecord->maskData;
		mask.rowBytes = gFilterRecord->maskRowBytes;
		mask.colBytes = 1;
		mask.maskDescription = kSimplePSMask;
	
		pixels.masks = &mask;
	} 
	else if ((gFilterRecord->inLayerPlanes != 0) && 
		       (gFilterRecord->inTransparencyMask != 0)) 
	{
		mask.next = NULL;
		mask.maskData = ((int8 *) gData->proxyBuffer) + 
			            gData->proxyPlaneSize *
						CSPlanesFromMode(gFilterRecord->imageMode, 0);
		mask.rowBytes = gData->proxyWidth;
		mask.colBytes = 1;
		mask.maskDescription = kSimplePSMask;
	
		pixels.masks = &mask;
	}

	(gFilterRecord->displayPixels)(&pixels, 
		                           &pixels.bounds, 
								   itemBounds.top, 
								   itemBounds.left, 
								   (void*)hDC);

	EndPaint(hDlg, (LPPAINTSTRUCT) &ps);
}

void DoAbout(void)
{
	AboutRecord* aboutRecord = (AboutRecord*)gFilterRecord;
	ShowAbout(aboutRecord);
}
