/*
	File:		ROMResources.h

	Contains:	Built-in resource declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__ROMRESOURCES__)
#define __ROMRESOURCES__ 1

#include "ROMSymbols.h"

#define DEFSTR(name) \
extern Ref * RS##name;
#define DEFMPSTR(name, str) \
extern Ref * RS##name;
#include "StringDefs.h"
#undef DEFMPSTR
#undef DEFSTR

extern Ref * RSslotCacheTable;

#define DEFFRAME(name, ref) \
extern Ref * RS##name;
#define DEFFRAME1(name, tag1, value1) \
extern Ref * RS##name;
#define DEFFRAME2(name, tag1, value1, tag2, value2) \
extern Ref * RS##name;
#define DEFFRAME3(name, tag1, value1, tag2, value2, tag3, value3) \
extern Ref * RS##name;
#define DEFFRAME4(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4) \
extern Ref * RS##name;
#define DEFFRAME5(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5) \
extern Ref * RS##name;
#define DEFPROTOFRAME5(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5) \
extern Ref * RS##name;
#define DEFFRAME6(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6) \
extern Ref * RS##name;
#define DEFFRAME7(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7) \
extern Ref * RS##name;
#define DEFFRAME8(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8) \
extern Ref * RS##name;
#define DEFFRAME10(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10) \
extern Ref * RS##name;
#define DEFFRAME17(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10, tag11, value11, tag12, value12, tag13, value13, tag14, value14, tag15, value15, tag16, value16, tag17, value17) \
extern Ref * RS##name;
#define DEFPROTOFRAME10(name, tag1, value1, tag2, value2, tag3, value3, tag4, value4, tag5, value5, tag6, value6, tag7, value7, tag8, value8, tag9, value9, tag10, value10) \
extern Ref * RS##name;
#include "FrameDefs.h"
#undef DEFFRAME
#undef DEFFRAME1
#undef DEFFRAME2
#undef DEFFRAME3
#undef DEFFRAME4
#undef DEFFRAME5
#undef DEFPROTOFRAME5
#undef DEFFRAME6
#undef DEFFRAME7
#undef DEFFRAME10
#undef DEFPROTOFRAME10


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
#define ISINROM(_r) NO
#define NOTINROM(_r) YES
#define ISINPACKAGE(_r) NO
#endif
#define ISRO(_r) (ISINROM(_r) || ISINPACKAGE(_r))


#endif	/* __ROMRESOURCES__ */
