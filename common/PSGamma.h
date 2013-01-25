#ifndef PSGAMMA_H
#define PSGAMMA_H

#include "PIDefines.h"
#include "PITypes.h"
#include "PIAbout.h"
#include "PIFilter.h"
#include "PIUtilities.h"

typedef struct Parameters
{
	double gamma;
	bool invert;
} Parameters, *ParametersPtr;

typedef struct Data
{
	FilterColor color;
	FilterColor colorArray[4];
	Boolean queryForParameters;
	BufferID dissolveBufferID;
	Ptr gammaBuffer;
	VRect proxyRect;
	float scaleFactor;
	BufferID proxyBufferID;
	Ptr proxyBuffer;
	int32 proxyWidth;
	int32 proxyHeight;
	int32 proxyPlaneSize;
} Data;

extern FilterRecord* gFilterRecord;
extern Data* gData;
extern int16* gResult;
extern Parameters* gParams;
extern SPBasicSuite * sSPBasic;
extern int gammaLut8[0x100];
extern int gammaLut16[0x8001];

void UpdateGammaLut();
void SetupFilterRecordForProxy(void);
void CopyColor(FilterColor& destination, const FilterColor& source);
void SetColor(FilterColor& destination, 
			  const uint8 a, 
			  const uint8 b, 
			  const uint8 c, 
			  const uint8 d);
void CreateDissolveBuffer(const int32 width, const int32 height);
void DeleteDissolveBuffer(void);
void CreateProxyBuffer(void);
extern "C" void ResetProxyBuffer(void);
extern "C" void UpdateProxyBuffer(void);
void DeleteProxyBuffer(void);
int32 DisplayPixelsMode(int16 mode);

#define I8_MULT(a, b, ut)			((ut) = (a) * (b) + 0x80, ((((ut) >> 8) + (ut)) >> 8))
#define I8_LERP(a, b, f, st)		((a) + I8_MULT((b) - (a), (f), (st)))

#define IQ15(d)	((int)((double)(d) * (double)0x8000 + 0.5))
#define  Q15_MULTR(a, b)	((unsigned short)(((int)(a) * (int)(b) + IQ15(0.5)) >> 15))
#define  Q15_LERP(a, b, f)	((a) + Q15_MULTR((b) - (a), (f)))

#define I8_TO_I9(i8)  (((unsigned short)(i8) << 1) | ((i8) >> 7))
#define I8_TO_Q15(i8) (((I8_TO_I9(i8) + 1) >> 1) << 7)

#endif