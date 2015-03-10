/*
	File:		ScreenBits.cc

	Contains:	Screen bitmap classes.

	Written by:	Newton Research Group.
*/

#include "DrawShape.h"
#include <FixMath.h>

#include "NewtGlobals.h"

extern int gSlowMotion;


/*------------------------------------------------------------------------------
	C B i t s
------------------------------------------------------------------------------*/

void
DisposeCBits(void * inData)
{
	delete (CBits *)inData;
}

CBits::CBits()
{
	fBitsPort = NULL;
	fIsDirty = NO;
	fPixMap.baseAddr = NULL;
	fIsPixMapCreatedByUs = YES;

	long	stackItem;
	if ((Ptr)this > (Ptr) &stackItem)	// ie on stack
	{
		fException.header.catchType = kExceptionCleanup;
		fException.function = DisposeCBits;
		fException.object = this;
		AddExceptionHandler(&fException.header);
	}
	else
		fException.function = NULL;
}

CBits::~CBits()
{
	if (fException.function)
		RemoveExceptionHandler(&fException.header);
	cleanUp();
}

bool
CBits::init(const Rect * inBounds)
{
	if (EmptyRect(inBounds))
		return NO;

	fPixMap.baseAddr = (Ptr) NewHandle(initBitMap(inBounds, &fPixMap));	// NOTE NewHandle deprecated
	fIsDirty = NO;
	return fPixMap.baseAddr != NULL;
}

void
CBits::init(const PixelMap * inPixmap)
{
	fPixMap = *inPixmap;
	fIsDirty = fIsPixMapCreatedByUs = NO;
}

size_t
CBits::initBitMap(const Rect * inBounds, PixelMap * ioPixmap)
{
	long		pixDepth;
	GetGrafInfo(kGrafPixelDepth, &pixDepth);
	long	rowBytes = ALIGN(((inBounds->right - inBounds->left) * pixDepth), 32) / 8;
	long	numOfRows = inBounds->bottom - inBounds->top;

	ioPixmap->baseAddr = NULL;
	ioPixmap->rowBytes = rowBytes;
	ioPixmap->bounds = *inBounds;
	ioPixmap->pixMapFlags = kPixMapHandle + kPixMapDevScreen + pixDepth;
	ioPixmap->deviceRes.v =
	ioPixmap->deviceRes.h = kDefaultDPI;
	ioPixmap->grayTable = NULL;

	return numOfRows * rowBytes;
}


void
CBits::cleanUp(void)
{
	if (fPixMap.baseAddr)
	{
		endDrawing();
		if (fIsPixMapCreatedByUs)
		{
			DisposeHandle((Handle)fPixMap.baseAddr);
			fIsPixMapCreatedByUs = NO;
		}
		fPixMap.baseAddr = NULL;
	}
}

void
CBits::beginDrawing(Point inOrigin)
{
	if (fBitsPort == NULL)
	{
		fBitsPort = new CBitsPort;
		if (fBitsPort == NULL)
			OutOfMemory();
		fBitsPort->init(this, inOrigin, !fIsDirty);
	}
	else
	{
		setPort();
		SetOrigin(inOrigin.h, inOrigin.v);
	}

	if (gSlowMotion)
		restorePort();
}

void
CBits::endDrawing(void)
{
	if (gSlowMotion)
		copyFromScreen(&fPixMap.bounds, &fPixMap.bounds, srcCopy, NULL);

	if (fBitsPort)
	{
		delete fBitsPort;
		fBitsPort = NULL;
	}
}

void
CBits::draw(const Rect * inBounds, long inTransferMode, RgnHandle inMaskRgn)
{
	draw(&fPixMap.bounds, inBounds, inTransferMode, inMaskRgn);
}

void
CBits::draw(const Rect * inSrcBounds, const Rect * inDstBounds, long inTransferMode, RgnHandle inMaskRgn)
{
	GrafPtr	thePort;
	GetPort(&thePort);
	CopyBits(&fPixMap, &thePort->portBits, inSrcBounds, inDstBounds, inTransferMode, inMaskRgn);
	fIsDirty = YES;
}

void
CBits::copyFromScreen(const Rect * inSrcBounds, const Rect * inDstBounds, long inTransferMode, RgnHandle inMaskRgn)
{
	GrafPtr	thePort;
	GetPort(&thePort);
	CopyBits(&thePort->portBits, &fPixMap, inSrcBounds, inDstBounds, inTransferMode, inMaskRgn);
	fIsDirty = YES;
}

void
CBits::copyIntoBitmap(PixelMap * inPixMap, long inTransferMode, RgnHandle inMaskRgn)
{
	CopyBits(&fPixMap, inPixMap, &fPixMap.bounds, &inPixMap->bounds, inTransferMode, inMaskRgn);
}

void
CBits::fill(long inPattern)
{
	Ptr	bits = (fPixMap.pixMapFlags & kPixMapPtr) ? fPixMap.baseAddr : *(Handle)fPixMap.baseAddr;
	FillLongs(bits, fPixMap.rowBytes * (fPixMap.bounds.bottom - fPixMap.bounds.top), inPattern);
}

void
CBits::setPort(void)
{
	SetPort(fBitsPort->fPort);
}

void
CBits::restorePort(void)
{
	SetPort(fBitsPort->fSavedPort);
}

void
CBits::setBounds(const Rect * inBounds)
{
	fPixMap.bounds = *inBounds;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C B i t s P o r t
------------------------------------------------------------------------------*/

CBitsPort::~CBitsPort()
{
	SetPort(fSavedPort);
	ClosePort(fPort);
	delete fPort;
}


void
CBitsPort::init(CBits * inBits, Point inOrigin, bool inClear)
{
	GrafPtr	thePort;

	fBits = inBits;
	GetPort(&thePort);
	fSavedPort = thePort;
	fPort = new GrafPort;
	if (fPort == NULL)
		OutOfMemory();
	OpenPort(fPort);
	SetPort(fPort);
	SetPortBits(&fBits->fPixMap);
	fPort->portRect = fBits->fPixMap.bounds;
	RectRgn(fPort->visRgn, &fBits->fPixMap.bounds);
	if (inClear)
		fBits->fill(0);
	SetOrigin(inOrigin.h, inOrigin.v);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S a v e S c r e e n B i t s
------------------------------------------------------------------------------*/

void
DisposeCSaveScreenBits(void * inData)
{
	delete (CSaveScreenBits *)inData;
}


CSaveScreenBits::CSaveScreenBits(void)
{
	fPixMap.baseAddr = NULL;

	fException.header.catchType = kExceptionCleanup;
	fException.object = this;
	fException.function = DisposeCSaveScreenBits;
	AddExceptionHandler(&fException.header);
}


CSaveScreenBits::~CSaveScreenBits(void)
{
	if (fPixMap.baseAddr)
		free(fPixMap.baseAddr);
	RemoveExceptionHandler(&fException.header);
}


bool
CSaveScreenBits::allocateBuffers(Rect * inBounds)
{
	bool	isAllocated = YES;
	Rect	bounds;

	newton_try
	{
		if (inBounds == NULL)
		{
			Rect	box = gScreenPixelMap.bounds;	// sic - why not bounds = gScreenPixelMap.bounds ?
			SetRect(&bounds, box.left, box.top, box.right, box.bottom);
			inBounds = &bounds;
		}
		long	pixDepth = PixelDepth(&gScreenPixelMap);
		fPixMap.rowBytes = ALIGN(inBounds->right - inBounds->left, 32) * pixDepth / 8;

		if ((fPixMap.baseAddr = NewPtr((inBounds->bottom - inBounds->top) * fPixMap.rowBytes)) = NULL)
			OutOfMemory();

		fPixMap.bounds = *inBounds;
		fPixMap.pixMapFlags = kPixMapPtr + pixDepth;
		fPixMap.deviceRes.v =
		fPixMap.deviceRes.h = kDefaultDPI;
		fPixMap.grayTable = NULL;
	}
	newton_catch(exRootException)
	{
		isAllocated = NO;
	}
	end_try;

	return isAllocated;
}


void
CSaveScreenBits::saveScreenBits(void)
{
//	CopyBits(&GetCurrentPort()->portBits, &fPixMap, NULL, NULL, srcCopy, NULL);
}


void
CSaveScreenBits::restoreScreenBits(Rect * inBounds, RgnHandle inMask)
{
	if (PtInRect(*(Point*)inBounds, &fPixMap.bounds)
	 || (inBounds->bottom < fPixMap.bounds.bottom && inBounds->right < fPixMap.bounds.right))
		SectRect(inBounds, &fPixMap.bounds, inBounds);

//	CopyBits(&fPixMap, &GetCurrentPort()->portBits, inBounds, inBounds, srcCopy, inMask);
}
