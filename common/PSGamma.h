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
	BufferID gammaBufferID;
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
extern SPBasicSuite* sSPBasic;
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
void CreateGammaBuffer(const int32 width, const int32 height);
void DeleteGammaBuffer(void);
void CreateProxyBuffer(void);
extern "C" void ResetProxyBuffer(void);
extern "C" void UpdateProxyBuffer(void);
void DeleteProxyBuffer(void);
int32 DisplayPixelsMode(int16 mode);

#define INT15(d)	RND((d) * 0x8000)
#define Q15(d)		(unsigned short)INT15(d)

#define I8_MULT(a, b, ut)	 ((ut) = (a) * (b) + 0x80, ((((ut) >> 8) + (ut)) >> 8))
#define I8_LERP(a, b, f, st) ((a) + I8_MULT((b) - (a), (f), (st)))

#define Q15_MULTR(a, b)		(unsigned short)(((int)(a) * (int)(b) + INT15(0.5)) >> 15)
#define Q15_LERP(a, b, f)	((a) + Q15_MULTR((b) - (a), (f)))

#define I8_TO_I9(i8)  (((unsigned short)(i8) << 1) | ((i8) >> 7))
#define I9_TO_Q8(i9)  (((i9) + 1) >> 1)
#define I8_TO_Q8(i8)  I9_TO_Q8(I8_TO_I9(i8))
#define Q8_TO_Q15(q8) ((q8) << 7)
#define I8_TO_Q15(i8) Q8_TO_Q15(I8_TO_Q8(i8))

#define I8_TO_DOUBLE(i8)	((double)(i8)  / I8(1))
#define Q15_TO_DOUBLE(q15)	((double)(q15) / Q15(1))

#endif