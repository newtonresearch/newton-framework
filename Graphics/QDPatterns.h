/*
	File:		QDPatterns.h

	Contains:	QuickDraw pattern declarations for Newton.
					QD patterns are actually used as colours (grey levels).

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__QDPATTERNS_H)
#define __QDPATTERNS_H 1

#include "Quartz.h"

typedef	PixelMap **	PatternHandle;						// patterns expressed as handle to PixelMap

enum
{
	whitePat,
	ltGrayPat,
	grayPat,
	dkGrayPat,
	blackPat
};

typedef unsigned char GetPatSelector;

#define	kPatternDataSize 		8						// number of pixels in square pattern definition
#define	kExpPatArraySize		16						// number of longs in expanded pattern array
#define	kExpPatArrayMask		kExpPatArraySize - 1

// Pattern -> Color
//extern CGColorRef		GetStdPattern(int inPatNo);
//extern bool				GetPattern(RefArg inPatNo, bool * ioTakeOwnership, CGColorRef * ioPat, bool inDefault);
extern bool				SetPattern(int inPatNo);

extern ULong			RGBtoGray(ULong inR, ULong inG, ULong inB, int inNumOfBits, int inDepth);

extern CGColorRef		GetStdPattern(int inPatNo);
extern bool				GetPattern(RefArg inPatNo, bool * ioTakeOwnership, CGColorRef * ioPat, bool inDefault);

#endif	/* __QDPATTERNS_H */
