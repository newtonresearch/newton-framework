/*
	File:		ROMResources.h

	Contains:	Built-in resource declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__ROMRESOURCES__)
#define __ROMRESOURCES__ 1

#include "RSData.h"
#include "RSSymbols.h"


/* -----------------------------------------------------------------------------
	M e m o r y   M a p
	Although we don’t actually have a ROM, we do have immutable resources:
	symbols, for example. Should we try to detect those?
----------------------------------------------------------------------------- */
#if defined(forMessagePad)
#define ISINROM(_r) (ULong(Ref(_r)) < 0x03800000)
#define NOTINROM(_r) (ULong(Ref(_r)) >= 0x03800000)
#define ISINPACKAGE(_r) (ULong(Ref(_r)) >= 0x60000000 && ULong(Ref(_r)) < 0x68000000)
#else
extern void * gConstNSDataStart, * gConstNSDataEnd;
#define ISINROM(_r) ((void *)Ref(_r) > gConstNSDataStart && (void *)Ref(_r) < gConstNSDataEnd)
#define NOTINROM(_r) ((void *)Ref(_r) < gConstNSDataStart || (void *)Ref(_r) > gConstNSDataEnd)
#define ISINPACKAGE(_r) false
#endif
#define ISRO(_r) (ISINROM(_r) || ISINPACKAGE(_r))


#endif	/* __ROMRESOURCES__ */
