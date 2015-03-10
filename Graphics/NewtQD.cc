/*
	File:		NewtQD.cc

	Contains:	QuickDraw functions for Newton.

	Written by:	Newton Research Group.
*/

#include "Screen.h"
#include "NewtGlobals.h"

extern void	InitQDCompression(void);

extern void InitScreen(void);
extern void SetupScreen(void);
extern void SetupScreenPixelMap(void);
extern void SetScreenInfo(void);
extern void	BlockLCDActivity(BOOL doBlock);
extern void	BlitToScreens(PixelMap *, Rect *, Rect *, long);

static void	InvalidateQDTempBuf(void);
static Ptr	QDNewTempPtr(size_t size);
static void	QDDisposeTempPtr(Ptr p);

Ptr			AllocNewTempBuf(void);
void			DeleteNewTempBuf(Ptr inBuf);

#define NewFakeHandle(h,s) h
#define HLock(h)
#define HUnlock(h)
#define HGetState(h) 0

const int	kBusyBoxWidth  = 32;
const int	kBusyBoxHeight = 32;


BOOL				gIsGrafInited = NO; // 0C105410 immediately follows gGC

RgnHandle		gUniverse;		// 0C1056F0

GrafPort			gScreenPort;	// 0C1067CC;

PatternHandle	gPattern[5];	// 0C107D74
QDGlobals		qd;				// 0C107D88

#if 0
const Region wideOpen = { sizeof(RgnHandle), { -32767, -32767, 32767, 32767 } };
const RgnPtr wideOpenPtr = &wideOpen;		// 003816B0

const char whiteBits[kPatternDataSize] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
const PixelMap whitePixPat = { whiteBits, 1, 0, {0,0,8,8}, kPixMapPtr + kOneBitDepth, {0,0}, NULL};
const PixelMapPtr whitePixPatPtr = &whitePixPat;

const char ltGrayBits[kPatternDataSize] = { 0x88, 0x22, 0x88, 0x22, 0x88, 0x22, 0x88, 0x22 };
const PixelMap ltGrayPixPat = { ltGrayBits, 1, 0, {0,0,8,8}, kPixMapPtr + kOneBitDepth, {0,0}, NULL};
const PixelMapPtr ltGrayPixPatPtr = &ltGrayPixPat;

const char grayBits[kPatternDataSize] = { 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55 };
const PixelMap grayPixPat = { grayBits, 1, 0, {0,0,8,8}, kPixMapPtr + kOneBitDepth, {0,0}, NULL};
const PixelMapPtr grayPixPatPtr = &grayPixPat;

const char dkGrayBits[kPatternDataSize] = { 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD, 0x77, 0xDD };
const PixelMap dkGrayPixPat = { dkGrayBits, 1, 0, {0,0,8,8}, kPixMapPtr + kOneBitDepth, {0,0}, NULL};
const PixelMapPtr dkGrayPixPatPtr = &dkGrayPixPat;

const char blackBits[kPatternDataSize] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
const PixelMap blackPixPat = { blackBits, 1, 0, {0,0,8,8}, kPixMapPtr + kOneBitDepth, {0,0}, NULL};
const PixelMapPtr blackPixPatPtr = &blackPixPat;


/*------------------------------------------------------------------------------
	Initialize the QuickDraw graphics system.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void InitGraf(void)
{
	if (gIsGrafInited)
		return;

	memset(&qd, 0, sizeof(qd));
	qd.x00 = 1;
/*
	gUniverse = NewFakeHandle(&wideOpenPtr, 20);

	gPattern[0] = NewFakeHandle(&whitePixPatPtr, sizeof(PixelMap));
	gPattern[1] = NewFakeHandle(&ltGrayPixPatPtr, sizeof(PixelMap));
	gPattern[2] = NewFakeHandle(&grayPixPatPtr, sizeof(PixelMap));
	gPattern[3] = NewFakeHandle(&dkGrayPixPatPtr, sizeof(PixelMap));
	gPattern[4] = NewFakeHandle(&blackPixPatPtr, sizeof(PixelMap));
*/
	InitScreen();
	InitQDCompression();

/*	NewtGlobals *	newt = GetNewtGlobals();
	newt->graf = (GrafPtr) NewPtr(sizeof(GrafPort));
	if (newt->graf)
	{
		OpenPort(newt->graf);
		InvalidateQDTempBuf();
	//	TPinPad::ClassInfo()->Register();
	//	TGrayShrink::ClassInfo()->Register();
	}
*/	gIsGrafInited = YES;
}
#endif

/*------------------------------------------------------------------------------
	Handle fork creation/destruction for CNewtWorld.
------------------------------------------------------------------------------*/

NewtonErr
InitForkGlobalsForQD(NewtGlobals * inGlobals)
{
	NewtGlobals *	newt = GetNewtGlobals();
	GrafPtr			thePort = (GrafPtr) NewPtr(sizeof(GrafPort));
/*	newt->graf = thePort;
	if (thePort != NULL)
	{
		OpenPort(thePort);
		newt->buf = AllocNewTempBuf();
		if (newt->buf == NULL)
		{
			ClosePort(thePort);
			DisposePtr((Ptr)newt->graf);
		}
	}
*/	return MemError();
}


void
DestroyForkGlobalsForQD(NewtGlobals * inGlobals)
{
/*	if (inGlobals->graf && inGlobals->graf != &gScreenPort)
	{
		ClosePort(inGlobals->graf);
		DisposePtr((Ptr)inGlobals->graf);
	}
*/	if (inGlobals->buf)
		DeleteNewTempBuf(inGlobals->buf);
}


/*------------------------------------------------------------------------------
	Return a pointer to the bitmap in a PixelMap.
	Args:		pixmap		the PixelMap
	Return:  pointer
------------------------------------------------------------------------------*/

Ptr
GetPixelMapBits(PixelMap * pixmap)
{
	Ptr	pixBits = pixmap->baseAddr;
	ULong storage = pixmap->pixMapFlags & kPixMapStorage;
	if (storage == kPixMapOffset)
		return pixBits + (unsigned) pixmap;
	else if (storage == kPixMapHandle)
		return *(Handle) pixBits;
	else if (storage == kPixMapPtr)
		return pixBits;
	return NULL;
}


static void
StartProtectSrcBits(PixelMap * pix, Rect * inBounds)
{
	// this really does nothing
}

static void
StopProtectSrcBits(PixelMap * pix)
{
	// this really does nothing
}


/*------------------------------------------------------------------------------
	F o n t s
------------------------------------------------------------------------------*/
static void	EngineInitSFNT(void);
static void	EngineInitInk(void);

void
InitFonts(void)
{
	EngineInitSFNT();
	EngineInitInk();
}

void
EngineInitSFNT(void)
{
	// this really does nothing
}

void
EngineInitInk(void)
{
	// this really does nothing
}

#pragma mark -

/*------------------------------------------------------------------------------
	B u s y   B o x
------------------------------------------------------------------------------*/

const ULong	gBusyBox1bit[] = {	// 0037AB5C
	0x00000000,
	0x0FFFFFF0,
	0x3FFFFFFC,
	0x3000000C,
	0x60000006,
	0x63FFFFC6,
	0x67FDFE66,
	0x679CFCE6,
	0x679FFDE6,
	0x67CF0FE6,
	0x67FC0726,
	0x64F8F226,
	0x6473B2E6,
	0x67A727E6,
	0x67E60FE6,
	0x67E49FE6,
	0x67F3FFE6,
	0x67F9FFE6,
	0x67FCFFE6,
	0x67FC3FE6,
	0x67FC7FE6,
	0x67FE3FE6,
	0x67FC1FE6,
	0x67FE7FE6,
	0x67FFFFE6,
	0x63FFFFC6,
	0x60000006,
	0x3000000C,
	0x3FFFFFFC,
	0x0FFFFFF0,
	0x00000000,
	0x00000000
};

const ULong	gBusyBox2bit[] = {	// 0037AA5C
	0x00000000,0x00000000,
	0x00FFFFFF,0xFFFFFF00,
	0x0FFFFFFF,0xFFFFFFF0,
	0x0F000000,0x000000F0,
	0x3C000000,0x0000003C,
	0x3C0FFFFF,0xFFFFF03C,
	0x3C3FFFF3,0xFFFC3C3C,
	0x3C3FC3F0,0xFFF0FC3C,
	0x3C3FC3FF,0xFFC3FC3C,
	0x3C3FF0FF,0x00FFFC3C,
	0x3C3FFFF0,0x003F0C3C,
	0x3C30FFC0,0xFF0C0C3C,
	0x3C303F0F,0xCF0CFC3C,
	0x3C3FCC3F,0x0C3FFC3C,
	0x3C3FFC3C,0x00FFFC3C,
	0x3C3FFC30,0xC3FFFC3C,
	0x3C3FFF0F,0xFFFFFC3C,
	0x3C3FFFC3,0xFFFFFC3C,
	0x3C3FFFF0,0xFFFFFC3C,
	0x3C3FFFF0,0x0FFFFC3C,
	0x3C3FFFF0,0x3FFFFC3C,
	0x3C3FFFF0,0x0FFFFC3C,
	0x3C3FFFF0,0x03FFFC3C,
	0x3C3FFFFC,0x3FFFFC3C,
	0x3C3FFFFF,0xFFFFFC3C,
	0x3C0FFFFF,0xFFFFF03C,
	0x3C000000,0x0000003C,
	0x0F000000,0x000000F0,
	0x0FFFFFFF,0xFFFFFFF0,
	0x00FFFFFF,0xFFFFFF00,
	0x00000000,0x00000000,
	0x00000000,0x00000000
};

const ULong	gBusyBox4bit[] = {	// 0037A85C
	0x00000000,0x00000000,0x00000000,0x00000000,
	0x0000FFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000,
	0x00FFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFF00,
	0x00FF0000,0x00000000,0x00000000,0x0000FF00,
	0x0FF00000,0x00000000,0x00000000,0x00000FF0,
	0x0FF000FF,0xFFFFFFFF,0xFFFFFFFF,0xFF000FF0,
	0x0FF00FFF,0xFFFFFF0F,0xFFFFFFF0,0x0FF00FF0,
	0x0FF00FFF,0xF00FFF00,0xFFFFFF00,0xFFF00FF0,
	0x0FF00FFF,0xF00FFFFF,0xFFFFFF0F,0xFFF00FF0,
	0x0FF00FFF,0xFF00FFFF,0x0000FFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFFF00,0x00000FFF,0x00F00FF0,
	0x0FF00F00,0xFFFFF000,0xFFFF00F0,0x00F00FF0,
	0x0FF00F00,0x0FFF00FF,0xF0FF00F0,0xFFF00FF0,
	0x0FF00FFF,0xF0F00FFF,0x00F00FFF,0xFFF00FF0,
	0x0FF00FFF,0xFFF00FF0,0x0000FFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFF00F00,0xF00FFFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFF00FF,0xFFFFFFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFF00F,0xFFFFFFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFFF00,0xFFFFFFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFFF00,0x00FFFFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFFF00,0x0FFFFFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFFFF0,0x00FFFFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFFF00,0x000FFFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFFFF0,0xFFF0FFFF,0xFFF00FF0,
	0x0FF00FFF,0xFFFFFFFF,0xFFFFFFFF,0xFFF00FF0,
	0x0FF000FF,0xFFFFFFFF,0xFFFFFFFF,0xFF000FF0,
	0x0FF00000,0x00000000,0x00000000,0x00000FF0,
	0x00FF0000,0x00000000,0x00000000,0x0000FF00,
	0x00FFFFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFFFF00,
	0x0000FFFF,0xFFFFFFFF,0xFFFFFFFF,0xFFFF0000,
	0x00000000,0x00000000,0x00000000,0x00000000,
	0x00000000,0x00000000,0x00000000,0x00000000
};

void
QDShowBusyBox(PixelMap * ioPixmap)
{
	short left = (qd.pixmap.bounds.right - qd.pixmap.bounds.left - kBusyBoxWidth) / 2;
	SetRect(&ioPixmap->bounds, left, 0, left + kBusyBoxWidth, kBusyBoxHeight);
	switch (ioPixmap->pixMapFlags & kPixMapDepth)
	{
	case 1:
		ioPixmap->baseAddr = (Ptr) &gBusyBox1bit[0];
		break;
	case 2:
		ioPixmap->baseAddr = (Ptr) &gBusyBox2bit[0];
		break;
	case 4:
		ioPixmap->baseAddr = (Ptr) &gBusyBox4bit[0];
		break;
	}
	BlockLCDActivity(YES);
	BlitToScreens(ioPixmap, &ioPixmap->bounds, &ioPixmap->bounds, srcCopy);
	BlockLCDActivity(NO);
}


void
QDHideBusyBox(PixelMap * inPixmap)
{
	BlockLCDActivity(YES);
	BlitToScreens(&qd.pixmap, &inPixmap->bounds, &inPixmap->bounds, 0);
	BlockLCDActivity(NO);
}


/*------------------------------------------------------------------------------
	B u f f e r   M a n a g e m e n t
------------------------------------------------------------------------------*/

#define kTempBufSize 1024

Ptr
AllocNewTempBuf(void)
{
	return NewPtr(kTempBufSize);
}


void
DeleteNewTempBuf(Ptr inBuf)
{
	if (inBuf != (Ptr)-kTempBufSize)
		DisposePtr(inBuf);
}


void
InvalidateQDTempBuf(void)
{
	GetNewtGlobals()->buf = (Ptr)-kTempBufSize;
	GetNewtGlobals()->bufPtr = NULL;
}


Ptr
QDNewTempPtr(size_t inSize)
{
	Ptr p;
	NewtGlobals *	newt = GetNewtGlobals();
	if (inSize < sizeof(long))
		inSize = sizeof(long);
	if (newt && newt->bufPtr + inSize < newt->buf + kTempBufSize)
	{
		p = newt->bufPtr;
		newt->bufPtr = p + LONGALIGN(inSize);
	}
	else
		p = NewPtr(inSize);
	return p;
}


void
QDDisposeTempPtr(Ptr p)
{
	if (p)
	{
		NewtGlobals *	newt = GetNewtGlobals();
		if (newt && p >= newt->buf && p < newt->buf + kTempBufSize)
			newt->bufPtr = p;
		else
			DisposePtr(p);
	}
}


long
QDSafeLock(Handle h)
{
	long state = HGetState(h);
	HLock(h);
	return state;
}

#pragma mark -

/*------------------------------------------------------------------------------
	S o r t i n g
------------------------------------------------------------------------------*/

void
QDQuickSort(Point * pt1, Point * pt2)
{
	;
}

#pragma mark -

/*------------------------------------------------------------------------------
	G r a f P o r t s
------------------------------------------------------------------------------*/

void
OpenPort(GrafPtr ioPort)
{
	ioPort->visRgn = NewRgn();
	ioPort->clipRgn = NewRgn();
	InitPort(ioPort);
}


void
InitPortRgns(GrafPtr ioPort)
{
//	RectRgn(ioPort->visRgn, &qd.pixmap.bounds);
//	CopyRgn(gUniverse, ioPort->clipRgn);
}


void
InitPort(GrafPtr ioPort)
{
	RgnHandle	savedVisRgn;
	RgnHandle	savedClipRgn;

	memset(ioPort, 0, sizeof(GrafPort));
	ioPort->visRgn = savedVisRgn;
	ioPort->clipRgn = savedClipRgn;
	ioPort->portBits = qd.pixmap;
	ioPort->portRect = qd.pixmap.bounds;
	InitPortRgns(ioPort);
	ioPort->fgPat = gPattern[blackPat];
	ioPort->bgPat = gPattern[whitePat];
	ioPort->pnSize.h = 1;
	ioPort->pnSize.v = 1;
	ioPort->pnMode = patCopy;
	SetPort(ioPort);
}


GrafPtr
GetCurrentPort(void)
{
	NewtGlobals *	newt = GetNewtGlobals();
	return /*newt ? newt->graf : */ &gScreenPort;
}


void
GetPort(GrafPtr * outPort)
{
	*outPort = GetCurrentPort();
}


GrafPtr
SetPort(GrafPtr inPort)
{
	GrafPtr			savePort = GetCurrentPort();
	NewtGlobals *	newt = GetNewtGlobals();
/*	if (newt)
		newt->graf = inPort;
*/	return savePort;
}


void
SetPortBits(PixelMap * inBits)
{
	GetCurrentPort()->portBits = *inBits;
}


void
ClosePort(GrafPtr inPort)
{
	DisposeRgn(inPort->visRgn);
	DisposeRgn(inPort->clipRgn);
	DisposePattern(inPort->fgPat);
	DisposePattern(inPort->bgPat);
}


void
SetOrigin(short inH, short inV)
{
	GrafPtr	thePort = GetCurrentPort();
	if (!(thePort->portRect.top == inV && thePort->portRect.left == inH))
	{
		inH -= thePort->portRect.left;
		inV -= thePort->portRect.top;
		OffsetRect(&thePort->portBits.bounds, inH, inV);
		OffsetRect(&thePort->portRect, inH, inV);
		OffsetRgn(thePort->visRgn, inH, inV);
	}
}


#pragma mark -

/*------------------------------------------------------------------------------
	P a t t e r n s
------------------------------------------------------------------------------*/

#include "View.h"

ULong
RGBtoGray(ULong inR, ULong inG, ULong inB, long inNumOfBits, long inDepth)
{
/* This is the Grayscale Conversion according to the Luminance Model
	used by NTSC and JPEG for nonlinear data in a gamma working space:
	Y = 0.299R + 0.587G + 0.114B
*/
	ULong  gray = 19589 * inR
					+ 38443 * inG
					+  7497 * inB;

	return ~gray >> (32 - inNumOfBits);
}


PatternHandle
GetStdGrayPattern(ULong inR, ULong inG, ULong inB)
{
	GrafPtr			thePort = GetCurrentPort();
	PixelMap *		pix = IsPrinterPort(thePort) ? &qd.pixmap : &thePort->portBits;
	long				depth = PixelDepth(pix);
	long				numOfBytes = 8 * depth;
	PatternHandle  thePattern = (PatternHandle) NewHandle(sizeof(PixelMap) + numOfBytes);	// NOTE NewHandle deprecated
	if (thePattern)
	{
		long				gray = RGBtoGray(inR, inG, inB, 16, depth);
		unsigned char  grayByte;
		Ptr				grayPtr;
		pix->baseAddr = (Ptr) sizeof(PixelMap);	// offset to gray data
		pix->rowBytes = numOfBytes / 8;				// 8 rows (of 8 pixels) in a std pattern
		SetRect(&pix->bounds, 0, 0, 8, 8);
		pix->pixMapFlags = kPixMapOffset + depth;
		pix->deviceRes.h =
		pix->deviceRes.h = kDefaultDPI;
		pix->grayTable = NULL;
		grayPtr = (Ptr)pix + sizeof(PixelMap);		// data follows immediately

		switch (depth)
		{
		case 1:
			grayByte = gray ? 0xFF : 0;
			break;
		case 2:
			grayByte = (gray << 6) | (gray << 4) | (gray << 2) | gray;
			break;
		case 4:
			grayByte = (gray << 4) | gray;
			break;
		}

		for ( ; numOfBytes > 0; numOfBytes--)
			*grayPtr++ = grayByte;
	}
	return thePattern;
}


PatternHandle
GetStdPattern(long inPatNo)
{
	if (inPatNo > 4)
		inPatNo = 4;
	return gPattern[inPatNo];
}


BOOL
GetPattern(RefArg inPatNo, BOOL * outGot, PatternHandle * outPat, BOOL inHuh)
{
// INCOMPLETE
	return NO;
}

/*
BOOL
SetPattern(long inPatNo)
{
	BOOL isSet = YES;

	switch (inPatNo)
	{
	case vfWhite:
		SetFgPattern(GetStdPattern(whitePat));
		break;
	case vfLtGray:
		SetFgPattern(GetStdPattern(ltGrayPat));
		break;
	case vfGray:
		SetFgPattern(GetStdPattern(grayPat));
		break;
	case vfDkGray:
		SetFgPattern(GetStdPattern(dkGrayPat));
		break;
	case vfBlack:
		SetFgPattern(GetStdPattern(blackPat));
		break;
	case 12:
		SetFgPattern(GetStdPattern(blackPat));
		break;
	case vfDragger:
		SetFgPattern(GetStdPattern(grayPat));
		break;
	case vfCustom:
		break;
	case vfMatte:
		SetFgPattern(GetStdPattern(grayPat));
		break;
	default:
		isSet = NO;
	}

	return isSet;
}
*/

void
DisposePattern(PatternHandle inPat)
{
	ULong		storage = (*inPat)->pixMapFlags & kPixMapStorage;

	// donÕt dispose a built-in pattern!
	for (int i = 4; i >= 0; i--)
		if (inPat == gPattern[i])
			return;

	if (storage == kPixMapOffset)
		/* donÕt dispose anything */ ;
	else if (storage == kPixMapHandle)
		DisposeHandle((Handle)(*inPat)->baseAddr);
	else
		DisposePtr((*inPat)->baseAddr);
	DisposeHandle((Handle)inPat);
}


void
SetFgPattern(PatternHandle inPat)
{
	GetCurrentPort()->fgPat = inPat;
}


PatternHandle
GetFgPattern(void)
{
	return GetCurrentPort()->fgPat;
}


void
DisposeFgPattern(void)
{
	DisposePattern(GetFgPattern());
	SetFgPattern(GetStdPattern(blackPat));
}
