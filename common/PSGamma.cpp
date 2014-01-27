#include "PSGamma.h"
#include "PSGammaUI.h"
#include "PSGammaScripting.h"
#include "PSGammaRegistry.h"
#include "FilterBigDocument.h"
#include <time.h>
#include <math.h>

FilterRecord* gFilterRecord = NULL;
intptr_t* gDataHandle = NULL;
int16* gResult = NULL; // all errors go here
SPBasicSuite* sSPBasic = NULL;

Data* gData = NULL;
Parameters* gParams = NULL;

int gammaLut8[0x100];
int gammaLut16[0x8001];

void DoAbout(void);
void DoParameters(void);
void DoPrepare(void);
void DoStart(void);
void DoContinue(void);
void DoFinish(void);

void DoFilter(void);
void CalcProxyScaleFactor(void);
void ConvertRGBColorToMode(const int16 imageMode, FilterColor& color);
void ScaleRect(VRect& destination, const int16 num, const int16 den);
void ShrinkRect(VRect& destination, const int16 width, const int16 height);
void CopyRect(VRect& destination, const VRect& source);
void LockHandles(void);
void UnlockHandles(void);
void CreateParametersHandle(void);
void InitParameters(void);
void CreateDataHandle(void);
void InitData(void);

void AdjustRectangleGamma(
	void* data, 
	int32 dataRowBytes, 
	void* mask, 
	int32 maskRowBytes, 
	VRect tileRect, 
	uint8 color,
	int32 depth);

//

DLLExport MACPASCAL void PluginMain(const int16 selector,
								    FilterRecordPtr filterRecord,
								    intptr_t * data,
								    int16 * result)
{
	// update our global parameters
	gFilterRecord = filterRecord;
	gDataHandle = data;
	gResult = result;

	if (selector == filterSelectorAbout)
	{
		sSPBasic = ((AboutRecord*)gFilterRecord)->sSPBasic;
	}
	else
	{
		sSPBasic = gFilterRecord->sSPBasic;

		if (gFilterRecord->bigDocumentData != NULL)
			gFilterRecord->bigDocumentData->PluginUsing32BitCoordinates = true;
	}

	// do the command according to the selector
	switch (selector)
	{
		case filterSelectorAbout:
			DoAbout();
			break;
		case filterSelectorParameters:
			DoParameters();
			break;
		case filterSelectorPrepare:
			DoPrepare();
			break;
		case filterSelectorStart:
			DoStart();
			break;
		case filterSelectorContinue:
			DoContinue();
			break;
		case filterSelectorFinish:
			DoFinish();
			break;
		default:
			break;
	}
	
	// unlock our handles used by gData and gParams
	if (selector != filterSelectorAbout)
		UnlockHandles();
}

void DoParameters(void)
{
	if (gFilterRecord->parameters == NULL)
		CreateParametersHandle();
	if ((*gDataHandle) == 0)
		CreateDataHandle();
	if (*gResult == noErr)
	{
		LockHandles();
		InitParameters();
		InitData();
	}
}

void DoPrepare(void)
{
	if (gFilterRecord->parameters != NULL && (*gDataHandle) != 0)
		LockHandles();
	else
	{
		if (gFilterRecord->parameters == NULL)
			CreateParametersHandle();
		if ((*gDataHandle) == 0)
			CreateDataHandle();
		if (*gResult == noErr)
		{
			LockHandles();
			InitParameters();
			InitData();
		}
	}
	
	// we don't need any buffer space
	gFilterRecord->bufferSpace = 0; 

	// give as much memory back to Photoshop as you can
	// we only need a tile per plane plus the maskData
	// inTileHeight and inTileWidth are invalid at this
	// point. Assume the tile size is 256 max.
	VRect filterRect = GetFilterRect();
	int32 tileHeight = filterRect.bottom - filterRect.top;
	int32 tileWidth = filterRect.right - filterRect.left;
	if (tileHeight > 256)
		tileHeight = 256;
	if (tileWidth > 256)
		tileWidth = 256;

	int32 tileSize = tileHeight * tileWidth;
	int32 planes = gFilterRecord->planes;
	if (gFilterRecord->maskData != NULL)
		planes++;
	// duplicate because we have two copies, inData and outData
	planes *= 2;

	int32 totalSize = tileSize * planes;
	// this is worst case and can be dropped considerably
	if (gFilterRecord->maxSpace > totalSize)
		gFilterRecord->maxSpace = totalSize;
}

void DoStart(void)
{
	LockHandles();

	// see if we have any information in the Photoshop registry
	ReadRegistryParameters();

	// save parameters
	double lastGamma = gParams->gamma;
	bool lastInvert = gParams->invert;

	// does the user want a dialog
	Boolean isOK = true;
	Boolean displayDialog;
	OSErr err = ReadScriptParameters(&displayDialog);
	err; // turn off compiler warning for now

	// run the dialog on the specific OS
	if (/*!err &&*/ displayDialog)
		isOK = DoUI();

	// we know we have enough information to run without next time
	gData->queryForParameters = false;

	if (isOK)
	{
		// the main processing routine
		DoFilter();
	}
	else
	{
		// restore if the user hit cancel
		gParams->gamma = lastGamma;
		gParams->invert = lastInvert;
		*gResult = userCanceledErr;
	}
}

void DoContinue(void)
{
	VRect zeroRect = { 0, 0, 0, 0 };

	SetInRect(zeroRect);
	SetOutRect(zeroRect);
	SetMaskRect(zeroRect);
}

void DoFinish(void)
{
	LockHandles();
	WriteScriptParameters();
	WriteRegistryParameters();
}

void DoFilter(void)
{
	int32 tileHeight = gFilterRecord->outTileHeight;
	int32 tileWidth = gFilterRecord->outTileWidth;

	if (tileWidth == 0 || tileHeight == 0)
	{
		*gResult = filterBadParameters;
		return;
	}

	VRect filterRect = GetFilterRect();
	int32 rectWidth  = filterRect.right - filterRect.left;
	int32 rectHeight = filterRect.bottom - filterRect.top;

	UpdateGammaLut();
	CreateGammaBuffer(tileWidth, tileHeight);

	// round up to the nearest horizontal and vertical tile count
	int32 tilesVert = (tileHeight - 1 + rectHeight) / tileHeight;
	int32 tilesHoriz = (tileWidth - 1 + rectWidth) / tileWidth;

	// Fixed numbers are 16.16 values 
	// the first 16 bits represent the whole number
	// the last 16 bits represent the fraction
	gFilterRecord->inputRate = (int32)1 << 16;
	gFilterRecord->maskRate  = (int32)1 << 16;
 
	// variables for the progress bar
	int32 progressTotal = tilesVert * tilesHoriz;
	int32 progressDone = 0;

	// loop through each tile makeing sure we don't go over the bounds
	// of the rectHeight or rectWidth
	for (int32 vertTile = 0; vertTile < tilesVert; vertTile++)
	{
		for (int32 horizTile = 0; horizTile < tilesHoriz; horizTile++)
		{
			VRect filterRect = GetFilterRect();
			VRect inRect = GetInRect();

			inRect.top = vertTile * tileHeight + filterRect.top;
			inRect.left = horizTile * tileWidth + filterRect.left;
			inRect.bottom = inRect.top + tileHeight;
			inRect.right = inRect.left + tileWidth;

			if (inRect.bottom > rectHeight)
				inRect.bottom = rectHeight;
			if (inRect.right > rectWidth)
				inRect.right = rectWidth;

			SetInRect(inRect);

			// duplicate what's in the inData with the outData
			SetOutRect(inRect);
			
			// get the maskRect if the user has given us a selection
			if (gFilterRecord->haveMask)
			{
				SetMaskRect(inRect);
			}

			for (int16 plane = 0; plane < gFilterRecord->planes; plane++)
			{
				// we want one plane at a time, small memory foot print is good
				gFilterRecord->outLoPlane = gFilterRecord->inLoPlane = plane;
				gFilterRecord->outHiPlane = gFilterRecord->inHiPlane = plane;
	
				// update the gFilterRecord with our latest request
				*gResult = gFilterRecord->advanceState();
				if (*gResult != noErr) return;

				// muck with the pixels in the outData buffer
				uint8 color = 255;
				int16 expectedPlanes = CSPlanesFromMode(gFilterRecord->imageMode, 0);

				if (plane < expectedPlanes)
					color = gData->color[plane];

				AdjustRectangleGamma(
					gFilterRecord->outData,
					gFilterRecord->outRowBytes,
					gFilterRecord->maskData,
					gFilterRecord->maskRowBytes,
					GetOutRect(), 
					color,
					gFilterRecord->depth);
			}

			// uh, update the progress bar
			gFilterRecord->progressProc(++progressDone, progressTotal);
			
			// see if the user is impatient or didn't mean to do that
			if (gFilterRecord->abortProc())
			{
				*gResult = userCanceledErr;
				return;
			}
		}
	}
	DeleteGammaBuffer();
}

void AdjustRectangleGamma(void* data, 
						  int32 dataRowBytes, 
						  void* mask, 
						  int32 maskRowBytes, 
						  VRect tileRect, 
						  uint8 color,
						  int32 depth)
{
	uint8*  pixel     = (uint8*)data;
	uint16* bigPixel  = (uint16*)data;
	float*  fPixel    = (float*)data;
	uint8*  maskPixel = (uint8*)mask;

	int32 rectHeight = tileRect.bottom - tileRect.top;
	int32 rectWidth  = tileRect.right - tileRect.left;

	const float gamma = (float)(
		gParams->invert
		? gParams->gamma
		: 1 / gParams->gamma);

	int t;
	uint16 bigMask;
	float fpix, fmask;

	for (int32 y = 0; y < rectHeight; y++)
	{
		for (int32 x = 0; x < rectWidth; x++)
		{
			if (!maskPixel)
			{
				if (depth == 32)
					*fPixel = min(pow(*fPixel, gamma), 1);
				else if (depth == 16)
					*bigPixel = gammaLut16[*bigPixel];
				else
					*pixel = gammaLut8[*pixel];
			}
			else if (*maskPixel)
			{
				if (depth == 32)
				{
					fpix = *fPixel;
					fmask = (float)*maskPixel / 0xFF;
					*fPixel = fpix + fmask * (min(pow(fpix, gamma), 1) - fpix);
				}
				else if (depth == 16)
				{
					uint16 newBigPixel = gammaLut16[*bigPixel];
					bigMask = I8_TO_Q15(*maskPixel);
					*bigPixel = Q15_LERP(*bigPixel, newBigPixel, bigMask);
				}
				else
				{
					uint8 newPixel = gammaLut8[*pixel];
					*pixel = I8_LERP(*pixel, newPixel, *maskPixel, t);
				}
			}

			pixel++;
			bigPixel++;
			fPixel++;

			if (maskPixel != NULL)
				maskPixel++;
		}

		pixel    += (dataRowBytes - rectWidth);
		bigPixel += (dataRowBytes / 2 - rectWidth);
		fPixel   += (dataRowBytes / 4 - rectWidth);

		if (maskPixel != NULL)
			maskPixel += (maskRowBytes - rectWidth);
	}
}

void CreateParametersHandle(void)
{
	gFilterRecord->parameters = gFilterRecord->handleProcs->newProc
											(sizeof(Parameters));
	if (gFilterRecord->parameters == NULL)
		*gResult = memFullErr;
}

void InitParameters(void)
{
	gParams->gamma = 1;
	gParams->invert = false;
}

void CreateDataHandle(void)
{
	Handle h = gFilterRecord->handleProcs->newProc(sizeof(Data));
	if (h != NULL)
		*gDataHandle = (intptr_t)h;
	else
		*gResult = memFullErr;
}

void InitData(void)
{
	CopyColor(gData->colorArray[0], gFilterRecord->backColor);
	SetColor(gData->colorArray[1], 0, 0, 255, 0);
	SetColor(gData->colorArray[2], 255, 0, 0, 0);
	SetColor(gData->colorArray[3], 0, 255, 0, 0);
	for(int a = 1; a < 4; a++)
		ConvertRGBColorToMode(gFilterRecord->imageMode, gData->colorArray[a]);
	//CopyColor(gData->color, gData->colorArray[gParams->disposition]);
	gData->proxyRect.left = 0;
	gData->proxyRect.right = 0;
	gData->proxyRect.top = 0;
	gData->proxyRect.bottom = 0;
	gData->scaleFactor = 1.0;
	gData->queryForParameters = true;
	gData->gammaBufferID = NULL;
	gData->gammaBuffer = NULL;
	gData->proxyBufferID = NULL;
	gData->proxyBuffer = NULL;
	gData->proxyWidth = 0;
	gData->proxyHeight = 0;
	gData->proxyPlaneSize = 0;
}

void UpdateGammaLut()
{
	const double invGamma = 
		  gParams->invert
		? gParams->gamma
		: 1 / gParams->gamma;

	// 8-bit

	for (int i = 0; i < 0x100; i++)
		gammaLut8[i] = I8(pow(I8_TO_DOUBLE(i), invGamma));

	// 16-bit

	for (int i = 0; i <= Q15(1); i++)
		gammaLut16[i] = Q15(pow(Q15_TO_DOUBLE(i), invGamma));
}

void CreateGammaBuffer(const int32 width, const int32 height)
{
	BufferProcs *bufferProcs = gFilterRecord->bufferProcs;

	bufferProcs->allocateProc(width * height, &gData->gammaBufferID);
	gData->gammaBuffer = bufferProcs->lockProc(gData->gammaBufferID, true);
}

void DeleteGammaBuffer(void)
{
	if (gData->gammaBufferID != NULL)
	{
		gFilterRecord->bufferProcs->unlockProc(gData->gammaBufferID);
		gFilterRecord->bufferProcs->freeProc(gData->gammaBufferID);
		gData->gammaBufferID = NULL;
		gData->gammaBuffer = NULL;
	}
}

void SetupFilterRecordForProxy(void)
{
	CalcProxyScaleFactor();

	SetInRect(GetFilterRect()); // gFilterRecord->inRect = gFilterRecord->filterRect;
	
	VRect tempRect = GetInRect();
	
	ScaleRect(tempRect, 1, (int16)gData->scaleFactor);

	SetInRect(tempRect);
	
	SetMaskRect(GetInRect()); // gFilterRecord->maskRect = gFilterRecord->inRect;

	// Fixed numbers are 16.16 values 
	// the first 16 bits represent the whole number
	// the last 16 bits represent the fraction
	gFilterRecord->inputRate = (int32)gData->scaleFactor << 16;
	gFilterRecord->maskRate = (int32)gData->scaleFactor << 16;
 
	gFilterRecord->inputPadding = 255;
	gFilterRecord->maskPadding = gFilterRecord->inputPadding;
	
	gData->proxyWidth = gData->proxyRect.right - gData->proxyRect.left;
	gData->proxyHeight = gData->proxyRect.bottom - gData->proxyRect.top;
	gData->proxyPlaneSize = gData->proxyWidth * gData->proxyHeight;
}

void CalcProxyScaleFactor(void)
{
	int32 filterHeight, filterWidth, itemHeight, itemWidth;
	VPoint fraction;
	
	VRect filterRect = GetFilterRect();
	
	// we place a black frame around the proxy and leave a black
	// between the frame and the actual pixel data
	ShrinkRect(gData->proxyRect, 2, 2);

	filterHeight = filterRect.bottom - filterRect.top;
	filterWidth  = filterRect.right  - filterRect.left;
	
	itemHeight = (gData->proxyRect.bottom - gData->proxyRect.top);
	itemWidth  = (gData->proxyRect.right  - gData->proxyRect.left);
	
	// make sure the proxy isn't bigger than the image after the calculation
	// this will make the proxy half the size for images smaller than the proxy
	if (itemHeight > filterHeight)
		itemHeight = filterHeight;
		
	if (itemWidth > filterWidth)
		itemWidth = filterWidth;
	
	fraction.h = ((filterWidth + itemWidth) / itemWidth);
	fraction.v = ((filterHeight + itemHeight) / itemHeight);

	// calculate the scale factor based on the smaller of height or width
	if (fraction.h > fraction.v) 
		gData->scaleFactor = ((float)filterWidth + (float)itemWidth) /
		                      (float)itemWidth;
	else
		gData->scaleFactor = ((float)filterHeight + (float)itemHeight) / 
		                      (float)itemHeight;

	CopyRect(gData->proxyRect, filterRect);	
	ScaleRect(gData->proxyRect, 1, (int16)gData->scaleFactor);

	// recalculate the scale factor based on the actual proxy size
	// this will reduce rounding errors in the proxy view
	if (fraction.h > fraction.v) 
		gData->scaleFactor = (float)filterWidth / 
		                      (float)(gData->proxyRect.right  - 
							   gData->proxyRect.left);
	else
		gData->scaleFactor = (float)filterHeight / 
							  (float)(gData->proxyRect.bottom - 
							   gData->proxyRect.top);
}

void ConvertRGBColorToMode(const int16 imageMode, FilterColor& color)
{
	if (imageMode != plugInModeRGBColor)
	{
		ColorServicesInfo	csInfo;

		csInfo.selector = plugIncolorServicesConvertColor;
		csInfo.sourceSpace = plugIncolorServicesRGBSpace;
		csInfo.reservedSourceSpaceInfo = NULL;
		csInfo.reservedResultSpaceInfo = NULL;
		csInfo.reserved = NULL;
		csInfo.selectorParameter.pickerPrompt = NULL;
		csInfo.infoSize = sizeof(csInfo);

		csInfo.resultSpace = CSModeToSpace(gFilterRecord->imageMode);
		for (int16 a = 0; a < 4; a++)
			csInfo.colorComponents[a] = color[a];

		if (!(gFilterRecord->colorServices(&csInfo)))
			for (int16 b = 0; b < 4; b++)
				color[b] = (int8)csInfo.colorComponents[b];
	}				   
}

void LockHandles(void)
{
	if (gFilterRecord->parameters == NULL || (*gDataHandle) == 0)
	{
		*gResult = filterBadParameters;
		return;
	}
	gParams = (Parameters*)gFilterRecord->handleProcs->lockProc
				(gFilterRecord->parameters, TRUE);
	gData = (Data*)gFilterRecord->handleProcs->lockProc
		        ((Handle)*gDataHandle, TRUE);
	if (gParams == NULL || gData == NULL)
	{
		*gResult = memFullErr;
		return;
	}
}

void UnlockHandles(void)
{
	if ((*gDataHandle) != 0)
		gFilterRecord->handleProcs->unlockProc((Handle)*gDataHandle);
	if (gFilterRecord->parameters != NULL)
		gFilterRecord->handleProcs->unlockProc(gFilterRecord->parameters);
}

void ScaleRect(VRect& destination, const int16 num, const int16 den)
{
	if (den != 0)
	{
		destination.left = (int16)((destination.left * num) / den);
		destination.top = (int16)((destination.top * num) / den);
		destination.right = (int16)((destination.right * num) / den);
		destination.bottom = (int16)((destination.bottom * num) / den);
	}
}

void ShrinkRect(VRect& destination, const int16 width, const int16 height)
{
	destination.left = (int16)(destination.left + width);
	destination.top = (int16)(destination.top + height);
	destination.right = (int16)(destination.right - width);
	destination.bottom = (int16)(destination.bottom - height);
}

void CopyRect(VRect& destination, const VRect& source)
{
	destination.left = source.left;
	destination.top = source.top;
	destination.right = source.right;
	destination.bottom = source.bottom;
}

void CopyColor(FilterColor& destination, const FilterColor& source)
{
	for (int a = 0; a < sizeof(FilterColor); a++)
		destination[a] = source[a];
}

void SetColor(FilterColor& destination, 
			  const uint8 a, 
			  const uint8 b, 
			  const uint8 c, 
			  const uint8 d)
{
	destination[0] = a;
	destination[1] = b;
	destination[2] = c;
	destination[3] = d;
}

void CreateProxyBuffer(void)
{
	int32 proxySize = gData->proxyPlaneSize * gFilterRecord->planes;
	gFilterRecord->bufferProcs->allocateProc(proxySize, &gData->proxyBufferID);
	gData->proxyBuffer = gFilterRecord->bufferProcs->lockProc(gData->proxyBufferID, true);
}

extern "C" void ResetProxyBuffer(void)
{
	uint8* proxyPixel = (uint8*)gData->proxyBuffer;

	if (proxyPixel != NULL)
	{
		for (int16 plane = 0; plane < gFilterRecord->planes; plane++)
		{
			gFilterRecord->inLoPlane = plane;
			gFilterRecord->inHiPlane = plane;
			
			*gResult = gFilterRecord->advanceState();
			if (*gResult != noErr) return;
			
			uint8* inPixel = (uint8*)gFilterRecord->inData;
			
			for (int32 y = 0; y < gData->proxyHeight; y++)
			{
				uint8* start = inPixel;

				for (int32 x = 0; x < gData->proxyWidth; x++)
				{
					if (gFilterRecord->depth == 32)
					{
						float* reallyBigPixel = (float*)inPixel;
						if (*reallyBigPixel > 1.0 )
							*reallyBigPixel = 1.0;
						if (*reallyBigPixel < 0.0 )
							*reallyBigPixel = 0.0;
						*proxyPixel = (uint8)(*reallyBigPixel * 255);
						inPixel+=4;
					} else 	if (gFilterRecord->depth == 16)
					{
						uint16* bigPixel = (uint16*)inPixel;
						*proxyPixel = (uint8)(*bigPixel * 10 / 1285);
						inPixel+=2;
					}
					else
					{
						*proxyPixel = *inPixel;
						inPixel++;
					}
					proxyPixel++;
				}

				inPixel = start + gFilterRecord->inRowBytes;
			}
		}
	}
}

extern "C" void UpdateProxyBuffer(void)
{
	Ptr localData = gData->proxyBuffer;

	if (localData != NULL)
	{
		for (int16 plane = 0; plane < gFilterRecord->planes; plane++)
		{
			uint8 color = 255;
			uint16 expectedPlanes = CSPlanesFromMode(gFilterRecord->imageMode, 0); 
			if (plane < expectedPlanes)
				color = gData->color[plane];
			AdjustRectangleGamma(localData, 
								 gData->proxyWidth, 
								 gFilterRecord->maskData, 
								 gFilterRecord->maskRowBytes, 
								 gData->proxyRect, 
								 color,
								 8);
			localData += (gData->proxyPlaneSize);
		}
	}
}

void DeleteProxyBuffer(void)
{
	gFilterRecord->bufferProcs->unlockProc(gData->proxyBufferID);
	gFilterRecord->bufferProcs->freeProc(gData->proxyBufferID);
	gData->proxyBufferID = NULL;
	gData->proxyBuffer = NULL;
}

int32 DisplayPixelsMode(int16 mode)
{
	int32 returnMode = mode;
	switch (mode)
	{
		case plugInModeGray16:
		case plugInModeGray32:
			returnMode = plugInModeGrayScale;
			break;
		case plugInModeRGB96:
		case plugInModeRGB48:
			returnMode = plugInModeRGBColor;
			break;
		case plugInModeLab48:
			returnMode = plugInModeLabColor;
			break;
		case plugInModeCMYK64:
			returnMode = plugInModeCMYKColor;
			break;
		case plugInModeDeepMultichannel:
			returnMode = plugInModeMultichannel;
			break;
		case plugInModeDuotone16:
			returnMode = plugInModeDuotone;
			break;
	}
	return (returnMode);
}