/*
	File:		Inker.cc

	Contains:	CInker implementation.
					The inker object responds to interrupts from the tablet hardware
					and inserts pen location samples in the tablet buffer.

	Written by:	Newton Research Group, 2010.
*/

#include "Quartz.h"
#include "QDDrawing.h"
#include "RootView.h"

#include "Inker.h"
#include "Tablet.h"
#include "TabletBuffer.h"
#include "Screen.h"
#include "Funcs.h"
#include "NameServer.h"
#include "VirtualMemory.h"
#include "RSSymbols.h"
#include "Preference.h"
#include "Debugger.h"

extern void		QDStartDrawing(NativePixelMap * inPixmap, Rect * inBounds);
extern void		QDStopDrawing(NativePixelMap * inPixmap, Rect * inBounds);

extern void		BlockLCDActivity(bool doIt);
extern void		BlitToScreens(NativePixelMap * inPixmap, Rect * inSrcBounds, Rect * inDstBounds, int inTransferMode);


extern void		RealStrokeInit(void);
extern int		RealStrokeTime(void);

extern void		InsertArmisticeSamples(void);


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FBusyBoxControl(RefArg inRcvr, RefArg inCtrl);

Ref	FIsTabletCalibrationNeeded(RefArg inRcvr);
Ref	FCalibrateTablet(RefArg inRcvr);
Ref	FGetCalibration(RefArg inRcvr);
Ref	FSetCalibration(RefArg inRcvr, RefArg inCalibrationData);

Ref	FHobbleTablet(RefArg inRcvr);
}


bool			CheckTabletHWCalibration(void);	// <--
NewtonErr	HobbleTablet(void);
void			InkerOffUnhobbled(Rect * inRect);


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

bool	gInkerCalibrated = true;	// 0C101654	original is probably false
CUPort * gTheInkerPort = NULL;	// 0C101658

bool	gCalibrate = false;			// 0C104D20
Heap	gStrokerHeap;					// 0C104D24
bool	gFakeTablet;					// 0C104D28	private
bool	gWireRecog = true;			// 0C104D2C
int	gCount = 0;						// 0C104D30


/* -----------------------------------------------------------------------------
	The inker object responds to interrupts from the tablet hardware and inserts
	pen location samples in the tablet buffer.
----------------------------------------------------------------------------- */

void
BusyBoxSend(int inSelector)
{
	if (gTheInkerPort != NULL)
	{
		CInkerEvent evt(inSelector);
		gTheInkerPort->send(&evt, sizeof(CInkerEvent), 3*kSeconds);
	}
}


CUPort *
InkerPort(void)
{
	if (gTheInkerPort == NULL)
	{
		OpaqueRef thing, spec;
		CUNameServer ns;
		ns.lookup("inkr", kUPort, &thing, &spec);
		gTheInkerPort = new CUPort((ObjectId)thing);
	}
	return gTheInkerPort;
}


void
StartInker(CUPort * inPort)
{
	CInker inker;
	inker.setNewtPort(inPort);
	inker.init('inkr', true, kSpawnedTaskStackSize);
}


void
InkerOff(Rect * inRect)
{
	HobbleTablet();
	InkerOffUnhobbled(inRect);
}

void
InkerOffUnhobbled(Rect * outRect)
{
	size_t replySize;
	CInkerEvent request(7);
	CInkerBoundsEvent reply;
	InkerPort()->sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
	*outRect = reply.bounds();
}


NewtonErr
HobbleTablet(void)
{
	size_t replySize;
	CInkerEvent request(29);
	CInkerBoundsEvent reply;
	return InkerPort()->sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
}


NewtonErr
CalibrateInker(void)
{
	size_t replySize;
	CInkerEvent request(5);
	CInkerReply reply;
	Timeout timeout = 0;
	RefVar sleepTime(GetPreference(SYMA(sleepTime)));
	if (ISINT(sleepTime))
	{
		timeout = RINT(sleepTime);
		if (timeout > 600) timeout = 600;
	}
	request.setParameter(timeout * kSeconds);
	InkerPort()->sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
	if (reply.result() == noErr)
	{
		gInkerCalibrated = true;
//		DoBlock(RA(SaveCalibration), RA(NILREF));		// don’t have that code readily to hand
	}
	return reply.result();
}


void
CheckTabletCalibration(void)
{
	if (!gInkerCalibrated || CheckTabletHWCalibration())
		FCalibrateTablet(RA(NILREF));
}


bool
CheckTabletHWCalibration(void)
{
	size_t replySize;
	CInkerEvent request(33);
	CInkerReply reply;
	return InkerPort()->sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply)) == noErr
		 && reply.result() > 0;
}


void
LoadInkerCalibration(void)
{
	if (NOTNIL(DoBlock(MAKEMAGICPTR(117), RA(NILREF))))
		gInkerCalibrated = true;

#if defined(correct)
	FILE *	f = fopen("bootNoCalibrate", "r");
	if (f != NULL)
	{
		gInkerCalibrated = true;
		fclose(f);
	}
	else
#endif
		CheckTabletCalibration();
}


/* -----------------------------------------------------------------------------
	Draw an ink line into a pixmap.
	Args:		inPt1			from this point
				inPt2			to this point
				outRect		rect enclosing the line drawn
				inPenSize	thickness of pen
				ioPixMap		pixmap into which to draw
	Return:	--
				outRect
----------------------------------------------------------------------------- */

void	InkerLine(const Point inPt1, const Point inPt2, Rect * outRect, const Point inPenSize, const NativePixelMap * ioPixMap);
void
InkerLine(const Point inPt1, const Point inPt2, Rect * outRect, const Point inPenSize)
{
	InkerLine(inPt1, inPt2, outRect, inPenSize, &gScreenPixelMap);
}


void
InkerLine(const Point inPt1, const Point inPt2, Rect * outRect, const Point inPenSize, const NativePixelMap * ioPixMap)
{
	Rect inkyBox;
	// create rect enclosing the ink
	Pt2Rect(inPt1, inPt2, &inkyBox);
	inkyBox.right += inPenSize.h;
	inkyBox.bottom += inPenSize.v;

	if (SectRect(&inkyBox, &ioPixMap->bounds, outRect))
	{
		// ink intersects with pixmap
		Rect box = *outRect;		// sp14
		int boxLeft = box.left;
		int boxRight = box.right;
		ULong rowBytes = ioPixMap->rowBytes;	// sp00
		Point startPt = inPt1;	// sp10
		Point endPt = inPt2;		// sp0C
		float xLeft, xRight;		// r7, r6
		float dX;					// r4
		if (inPt1.v == inPt2.v)
		{
			// horizontal straight line
			// always draw left-right
			int left = (inPt1.h < inPt2.h) ? inPt1.h : inPt2.h;
			xLeft = (float)left + 0.5;
			xRight = (float)boxRight + 0.5;
			dX = 0.0;
		}
		else
		{
			if (inPt1.v > inPt2.v)
			{
				// always draw top-down
				startPt = inPt2;
				endPt = inPt1;
			}
			float r8 = (float)startPt.h + 0.5;
			// dX is amount to offset per row
			dX = (float)(endPt.h - startPt.h) / (float)(endPt.v - startPt.v);
			float r0 = inPenSize.v * dX;
			xLeft = r8 + dX/2;
			xRight = r8 + (float)inPenSize.h + dX/2;
			if (dX >= 0.0)
			{
				// left-right diagonal
				xLeft -= r0;
				if (dX >= 1.0)
					xRight -= 1.0;
				else
					xLeft += dX;
			}
			else
			{
				// right-left diagonal
				xRight -= r0;
				if (dX < -1.0)
					xLeft += 1.0;
				else
					xRight += dX;
			}
			if (box.top != inkyBox.top)
			{
				float r0 = (box.top - inkyBox.top) * dX;
				xLeft += r0;
				xRight += r0;
			}
		}
		//sp-08
		int pixDepth = PixelDepth(ioPixMap);		// spm04
		int pixMask = gScreenConstants.xMask[pixDepth];				// 00,1F,0F,00,07		r8
		int pixShift = gScreenConstants.xShift[pixDepth];			// 00,05,04,00,03		spm00
		int depthShift = gScreenConstants.depthShift[pixDepth];		// 00,03,02,00,01		r9

		ULong xWordOrigin = ioPixMap->bounds.left + ((boxLeft - ioPixMap->bounds.left) & ~pixMask);		// r10

		char * rowPtr	= PixelMapBits(ioPixMap)
							+ (box.top - ioPixMap->bounds.top) * rowBytes	// start of row in pixmap	r9
							+ ((xWordOrigin - ioPixMap->bounds.left) >> depthShift);	// offset to active rect
		xLeft -= xWordOrigin;
		xRight -= xWordOrigin;
		boxLeft -= xWordOrigin;
		boxRight -= xWordOrigin;

		QDStartDrawing((NativePixelMap *)ioPixMap, outRect);
//printf("InkerLine() x:%d-%d y:%d-%d rowOffset=%d byteOffset=%d\n",boxLeft,boxRight, box.top,box.bottom, (box.top - ioPixMap->bounds.top), ((xWordOrigin - ioPixMap->bounds.left) >> depthShift));
		for (int y = box.top; y < box.bottom; ++y)
		{
			int x1 = MAX(boxLeft, xLeft);
			int x2 = MIN(xRight, boxRight);
//printf("x:%d-%d\n",x1,x2);
			if (x1 < x2)
			{
				ULong lBits = 0xFFFFFFFF >> ((x1 & pixMask) * pixDepth);				// r3
				ULong rBits = (x2 & pixMask) == 0 ? 0 : 0xFFFFFFFF << (32 - ((x2 & pixMask) * pixDepth));	// r12
#if defined(hasByteSwapping)
				lBits = BYTE_SWAP_LONG(lBits);
				rBits = BYTE_SWAP_LONG(rBits);
#endif
				ULong wordOffset = (x1 >> pixShift);
				ULong numOfWords = (x2 >> pixShift) - wordOffset;
				ULong * pixPtr = (ULong *)rowPtr + wordOffset;
				if (numOfWords == 0)
				{
					ULong xBits = lBits & rBits;
					*pixPtr |= xBits;
//printf("%08X\n", xBits);
				}
				else
				{
					*pixPtr++ |= lBits;
//printf("%08X", lBits);
					for ( ; numOfWords > 1; --numOfWords) {
						*pixPtr++ = 0xFFFFFFFF;
//printf("FFFFFFFF");
					}
					*pixPtr |= rBits;
//printf("%08X\n", rBits);
				}
			}
			rowPtr += rowBytes;
			xLeft += dX;
			xRight += dX;
		}
		QDStopDrawing((NativePixelMap *)ioPixMap, outRect);
	}
}


#pragma mark - PlainC
/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

Ref
FBusyBoxControl(RefArg inRcvr, RefArg inCtrl)
{
	int  ctrl = RINT(inCtrl);
	if (ctrl >= -2 && ctrl <= 2)
		BusyBoxSend(53 + ctrl);
	return NILREF;
}


Ref
FIsTabletCalibrationNeeded(RefArg inRcvr)
{
	return MAKEBOOLEAN(gTheInkerPort != NULL && CheckTabletHWCalibration());
}


Ref
FCalibrateTablet(RefArg inRcvr)
{
	NewtonErr err = CalibrateInker();
	gRootView->dirty();
	return err != 0 ? MAKEINT(err) : NILREF;
}


Ref
FGetCalibration(RefArg inRcvr)
{
	RefVar calibration;
	CInkerCalibrationEvent request(22);	// size +24
	CInkerCalibrationEvent reply;			// size +24
	size_t replySize = 0;
	InkerPort()->sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
	if (replySize == sizeof(CInkerCalibrationEvent))	// 0x24
	{
		calibration = AllocateBinary(SYMA(calibration), 20);
		CDataPtr data(calibration);
		memmove((char *)data, &request.fCalibration, 20);
	}
	return calibration;
}


Ref
FSetCalibration(RefArg inRcvr, RefArg inCalibrationData)
{
	if (NOTNIL(inCalibrationData) && Length(inCalibrationData) == 20)
	{
		CInkerCalibrationEvent request(23);	// size +24
		CDataPtr data(inCalibrationData);
		memmove(&request.fCalibration, (char *)data, sizeof(Calibration));

		CInkerCalibrationEvent reply;			// size +24
		size_t replySize;
		InkerPort()->sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
	}
	return NILREF;
}


Ref
FHobbleTablet(RefArg rcvr)
{
	HobbleTablet();
	return NILREF;
}


#pragma mark - CLiveInker
/*--------------------------------------------------------------------------------
	C L i v e I n k e r

	Live ink is held in a 64 x 64 pixmap.
--------------------------------------------------------------------------------*/

CLiveInker::CLiveInker()
	:	fPixMem(NULL)
{ }


CLiveInker::~CLiveInker()
{
	if (fPixMem)
		FreePtr(fPixMem);
}


NewtonErr
CLiveInker::init(void)
{
	NewtonErr err = noErr;
	XTRY
	{
	// create 64x64 PixelMap
		int	pixDepth = PixelDepth(&gScreenPixelMap);		// in screen params
		int	depthShift = gScreenConstants.depthShift[pixDepth];
		fPixDepthShift = depthShift;
		fPixMemSize = 64 * (64 >> depthShift);
		XFAILNOT(fPixMem = NewPtr(fPixMemSize), err = kOSErrNoMemory;)

		fPixMap.baseAddr = fPixMem;
		fPixMap.pixMapFlags = kPixMapPtr + pixDepth;
		fPixMap.deviceRes.h =
		fPixMap.deviceRes.v = kDefaultDPI;
		fPixMap.grayTable = NULL;
	}
	XENDTRY;
	return err;
}


void
CLiveInker::resetAccumulator(void)
{
	// invalidate ink bounds
	SetRect(&fInkBounds, 32767, 32767, -32768, -32768);
	// no points in the ink line yet
	fNumOfPts = 0;

	ScreenInfo info;
	GetGrafInfo(kGrafScreen, &info);
	fExtent.x = info.f18;	// 32
	fExtent.y = info.f14;	// 32
}


void
CLiveInker::startLiveInk(void)
{
	mapLCDExtent(&fInkBounds, &fPixMap.bounds);
	fPixMap.rowBytes = (fPixMap.bounds.right - fPixMap.bounds.left) >> fPixDepthShift;
	if (SectRect(&fPixMap.bounds, &gScreenPixelMap.bounds, &fPixMap.bounds))
		memset(fPixMem, 0, (fPixMap.bounds.bottom - fPixMap.bounds.top) * fPixMap.rowBytes);
}


void
CLiveInker::stopLiveInk(void)
{
	BlockLCDActivity(true);
	BlitToScreens(&fPixMap, &fPixMap.bounds, &fPixMap.bounds, 1/*srcOr*/);
	BlockLCDActivity(false);
}


void
CLiveInker::inkLine(const Point inPt1, const Point inPt2, const Point inPenSize)
{
	Rect	inkedBox;
	InkerLine(inPt1, inPt2, &inkedBox, inPenSize, &fPixMap);
}


bool
CLiveInker::addPoint(const Point inLocation, const Point inSize)
{
	LRect pt;
	pt.top = inLocation.v;
	pt.left = inLocation.h;
	pt.bottom = inLocation.v + inSize.v;
	pt.right = inLocation.h + inSize.h;

	if (pt.top >= fInkBounds.top && pt.left >= fInkBounds.left
	&&  pt.right <= fInkBounds.right && pt.bottom <= fInkBounds.bottom)
	{
		// point is totally within our ink bounds
		++fNumOfPts;
		return true;
	}

	Rect bBox;
	bBox.top = MIN(fInkBounds.top, pt.top);
	bBox.left = MIN(fInkBounds.left, pt.left);
	bBox.bottom = MAX(fInkBounds.bottom, pt.bottom);
	bBox.right = MAX(fInkBounds.right, pt.right);

	if (fNumOfPts <= 1 || mapLCDExtent(&bBox, NULL))
	{
		fInkBounds = bBox;
		++fNumOfPts;
		return true;
	}
	return false;
}


bool
CLiveInker::mapLCDExtent(const Rect * inRect, Rect * outRect)
{
	LRect aligned;
	aligned.top = TRUNC(inRect->top, fExtent.y);
	aligned.bottom = ALIGN(inRect->bottom, fExtent.y);
	aligned.left = TRUNC(inRect->left, fExtent.x);
	aligned.right = ALIGN(inRect->right, fExtent.x);

	int pixDepth = PixelDepth(&fPixMap);
	int depthShift = gScreenConstants.depthShift[pixDepth];
	size_t pixMemReqd = (aligned.bottom - aligned.top) * ((aligned.right - aligned.left) >> depthShift);
	if (pixMemReqd <= fPixMemSize)
	{
		// we have enough memory to hold the image in the rect
		if (outRect)
			SetRect(outRect, aligned.left, aligned.top, aligned.right, aligned.bottom);
		return true;
	}
	// rect must be constrained for the memory available
	if (outRect)
		SetRect(outRect, aligned.left, aligned.top, aligned.left+64, aligned.top+64);
	return false;
}


#pragma mark - CInker
/*--------------------------------------------------------------------------------
	C I n k e r
--------------------------------------------------------------------------------*/

CInker::CInker()
{ }


CInker::~CInker()
{ }


size_t
CInker::getSizeOf(void) const
{ return sizeof(CInker); }


NewtonErr
CInker::mainConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CAppWorld::mainConstructor())

		fMessage.init(1);
		iInker();
		CInkerEventHandler * handler = new CInkerEventHandler();
		handler->init(kInkerEventId);
		handler->initIdler(50, kMilliseconds);
		if (!gFakeTablet)
			handler->stopIdle();

		Heap	aHeap;
		XFAIL(NewSegregatedVMHeap(0, 64*KByte, 64*KByte, &aHeap, 0))
		gStrokerHeap = aHeap;
		SetHeap(aHeap);

		RealStrokeInit();

		StackLimits	inkStack;
		LockStack(&inkStack, 0x0300);
	}
	XENDTRY;
	return noErr;
}


void
CInker::iInker(void)
{
	fLiveInker.init();

	// initialize the tablet to the dimensions of the display
	NativePixelMap	pixMap;
	GetGrafInfo(kGrafPixelMap, &pixMap);
	TabInitialize(pixMap.bounds, getMyPort());

	fCurrentPenMode = 2;
	fNextPenMode = 2;

	fEvent.fEventType = kInkerEventId;

	fPenState = kPenUp;

	gFakeTablet = FLAGTEST(gDebuggerBits, kDebuggerUseARMisticeCard);
	if (gFakeTablet)
	{
		for (bool done = false; !done; )
		{
			newton_try
			{
				ARMisticeSamples * samples = &gFakeTabletSamples;
				samples->wrPtr = gFakeTabletSamples.buf;
				samples->rdPtr = gFakeTabletSamples.buf;
				samples->bufStart = gFakeTabletSamples.buf;
				samples->bufEnd = gFakeTabletSamples.buf + kFakeTabletSampleBufSize;
				done = true;
//				0x03380000->x04 = samples;
			}
			newton_catch(exAbort)
			{ /* just catch it */ }
			end_try;
		}
		StartBypassTablet();
	}
}	


void
CInker::setNewtPort(CUPort * inPort)
{
	fPort = inPort;
}


void
CInker::setCurrentPenMode(UChar inPenMode)
{
	fCurrentPenMode = inPenMode;
}


UChar
CInker::getCurrentPenMode(void)
{
	return fCurrentPenMode;
}


void
CInker::setNextPenMode(UChar inPenMode)
{
	fNextPenMode = inPenMode;
}


UChar
CInker::getNextPenMode(void)
{
	return fNextPenMode;
}


bool
CInker::testForCalibrationNeeded(void)
{
	return TabletNeedsRecalibration();
}


void
CInker::presCalibrate(void)
{
//	this really does nothing
}


NewtonErr
CInker::calibrate(ULong)
{
	// do the whole thing with the drawing of the newt and the crosses
	return noErr;
}


void
CInker::getRawPoint(ULong*, ULong*, short, short, ULong)
{
	// draw a cross and return the actual point tapped for calibrate()
}


bool
CInker::convert(void)
{
	if (gCalibrate || InkerBufferEmpty())
		// there’s nothing to convert
		return false;

	TabletSample sample;
	sample.intValue = GetInkerData();
	if (sample.z == kPenDownSample)			// pen is applied
	{
		fPenState = kPenDown;
		fCurrentPenMode = fNextPenMode;
		IncInkerIndex(1);	// ignore the time
	}
	else if (sample.z == kPenUpSample)		// pen is lifted
	{
		fPenState = kPenUp;
		SetInkerData((fCurrentPenMode << 8) | kPenUpSample);
		SetInkerData(*(ULong *)&fInkBounds.top, 2);		// top-left point
		SetInkerData(*(ULong *)&fInkBounds.bottom, 3);	// bottom-right point
		IncInkerIndex(3);
	}
	else if (sample.z <= kPenMaxPressureSample)	// pen is writing
	{
		fPenState = kPenActive;
		fSampleLoc.x = sample.x / kTabScale;
		fSampleLoc.y = sample.y / kTabScale;
	}
	else
	{
		fPenState = kPenError;
		SetInkerData(0x0F);
	}

	// remove sample from the buffer
	IncInkerIndex(1);
	return true;
}


// called from CInkerEventHandler when ink is to be drawn
void
CInker::LCDEntry(void)
{
	Rect inkyBox;
	Point currentPt, prevPt;
	while (convert())	// reads pen x & y
	{
		switch (fPenState)
		{
		case kPenDown:
			fPenLoc.x = -1.0;
			x84 = 0;
			fInkBounds.right = 0;
			fInkBounds.left = 0;
			break;

		case kPenError:
			// this really does nothing
			break;

		case kPenActive:
			if (fCurrentPenMode != 0)
			{
				currentPt.h = fSampleLoc.x + 0.5;
				currentPt.v = fSampleLoc.y + 0.5;
				if (fPenLoc.x == -1.0)
					prevPt = currentPt;
				else
				{
					prevPt.h = fPenLoc.x + 0.5;
					prevPt.v = fPenLoc.y + 0.5;
				}
				if (*(int *)&currentPt != *(int *)&prevPt		// pen has moved
				||  fPenLoc.x == -1.0)								// this is the first point
				{
					drawInk(prevPt, currentPt, &inkyBox, fPenSize);
					UnionRect(&fInkBounds, &inkyBox, &fInkBounds);	// original says JoinRect which does the same thing
				}
			}
			fPenLoc = fSampleLoc;
			break;

		case kPenUp:
			fPenLoc.x = -1.0;
			x84 = 0;
		}
		if (!gWireRecog)
			FlushInkerBuffer();
		if (fPenState != kPenDown  &&  RealStrokeTime() != 0)
			fPort->send(&fMessage, &fEvent, sizeof(CInkerEvent));		// sic -- sizeof(CIdleEvent) ?
	}
	// send idle event to update display
	if (fPenState != kPenDown  &&  RealStrokeTime() != 0)
		fPort->send(&fMessage, &fEvent, sizeof(CInkerEvent));		// sic -- sizeof(CIdleEvent) ?
}


/* -------------------------------------------------------------------------------
	Draw a line of ink.
	Call stack:
	CInkerEventHandler::idleProc()
	-> CInkerEventHandler::inkThem()
		-> gInkWorld->LCDEntry()
			-> CInker::drawInk
	Args:		inPt1			from this point
				inPt2			to this point
				outBounds	box enclosing the drawn ink
				inPenSize	ignored
	Return:	--
------------------------------------------------------------------------------- */

void
CInker::drawInk(Point inPt1, Point inPt2, Rect * outBounds, short inPenSize)
{
	Point penSize;
	penSize.h = penSize.v = fCurrentPenMode;	// inPenSize is ignored

	Point	ptsArray[80];
	ptsArray[0] = inPt1;
	ptsArray[1] = inPt2;
	ArrayIndex ptsIndex = 2;

	fLiveInker.resetAccumulator();
	fLiveInker.addPoint(inPt1, penSize);
	bool isInBounds = fLiveInker.addPoint(inPt2, penSize);
	// if there are more tablet samples pending, add them to our points vector
	while (isInBounds && !InkerBufferEmpty() && ptsIndex < 80)
	{
		// read tablet sample
		TabletSample sample;
		sample.intValue = GetInkerData();
		if (sample.z > kPenMaxPressureSample)
			break;
		FPoint samplePt;
		samplePt.x = sample.x / kTabScale;
		samplePt.y = sample.y / kTabScale;
		// convert to pen coord
		Point penPt;
		penPt.h = samplePt.x + 0.5;
		penPt.v = samplePt.y + 0.5;

		isInBounds = fLiveInker.addPoint(penPt, penSize);
		if (isInBounds)
		{
			IncInkerIndex(1);
			if (*(ULong *)&penPt != *(ULong *)&ptsArray[ptsIndex-1])
			{
				ptsArray[ptsIndex++] = penPt;
				fSampleLoc = samplePt;
			}
		}
	}
	*outBounds = fLiveInker.bounds();

	// ink the line segments
	fLiveInker.startLiveInk();
	for (Point * pt = ptsArray, * ptsLimit = ptsArray+ptsIndex-1; pt < ptsLimit; ++pt)
		fLiveInker.inkLine(pt[0], pt[1], penSize);
	fLiveInker.stopLiveInk();
}


void
CInker::insertionSort(ULong * inArray, ArrayIndex inCount, ULong inItem)
{
	for ( ; inCount > 0; inCount--)
	{
		ULong	item = inArray[inCount - 1];
		inArray[inCount] = item;
		if (item <= inItem)
			break;
	}
	inArray[inCount] = inItem;
}


#pragma mark - CInkerEventHandler
/* -------------------------------------------------------------------------------
	C I n k e r E v e n t H a n d l e r
------------------------------------------------------------------------------- */

void
CInkerEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	UChar penMode;
	switch (((CInkerEvent *)inEvent)->fSelector)
	{
	case 2:
		TabletIdle();
		if (!TabletBufferEmpty())
		{
			inkThem();
			if (gInkWorld->fPenState == kPenUp  &&  TabletBufferEmpty())
			{
				XFAIL(GetTabletState() == kTabletAwake)	// not in XTRY/XENDTRY but does the right thing -- see idleProc
			}
		}
		startIdle();
		break;

	case 4:
		inkThem();
		break;

	case 5:
		((CInkerEvent *)inEvent)->fParameter = gInkWorld->calibrate(((CInkerEvent *)inEvent)->fParameter);
		srand(0x0F181800);
		setReply(sizeof(CInkerEvent), inEvent);
		break;

	case 6:
		gInkWorld->presCalibrate();
		break;

	case 7:	// inker off
		((CInkerBoundsEvent *)inEvent)->fBounds = gInkWorld->fInkBounds;
		penMode = gInkWorld->getCurrentPenMode();
		gInkWorld->setCurrentPenMode(((CInkerEvent *)inEvent)->fSelector - 7);
		((CInkerEvent *)inEvent)->fSelector = penMode + 7;
		setReply(sizeof(CInkerBoundsEvent), inEvent);
		break;

	case 8:
	case 9:
	case 10:
	case 11:
		penMode = gInkWorld->getCurrentPenMode();
		gInkWorld->setCurrentPenMode(((CInkerEvent *)inEvent)->fSelector - 7);
		((CInkerEvent *)inEvent)->fSelector = penMode + 7;
		setReply(sizeof(CInkerEvent), inEvent);
		break;

	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		penMode = gInkWorld->getNextPenMode();
		gInkWorld->setNextPenMode(((CInkerEvent *)inEvent)->fSelector - 12);
		((CInkerEvent *)inEvent)->fSelector = penMode + 12;
		setReply(sizeof(CInkerEvent), inEvent);
		break;

	case 20:
		((CInkerEvent *)inEvent)->fSelector = gInkWorld->getCurrentPenMode() + 7;
		setReply(sizeof(CInkerEvent), inEvent);
		break;

	case 21:
		((CInkerEvent *)inEvent)->fSelector = gInkWorld->getNextPenMode() + 12;
		setReply(sizeof(CInkerEvent), inEvent);
		break;

	case 22:
		GetTabletCalibration(&((CInkerCalibrationEvent *)inEvent)->fCalibration);
		setReply(sizeof(CInkerCalibrationEvent), inEvent);
		break;

	case 23:
		SetTabletCalibration(((CInkerCalibrationEvent *)inEvent)->fCalibration);
		break;

//	case 29:		hobble tablet

	case 33:
		((CInkerEvent *)inEvent)->fParameter = gInkWorld->testForCalibrationNeeded();
		setReply(sizeof(CInkerEvent), inEvent);
		break;
	}
}


void
CInkerEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{ }


void
CInkerEventHandler::idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	TabletIdle();	// does nothing; just returns noErr
	inkThem();		// draw strokes
	XTRY
	{
		if (gFakeTablet)
			InsertArmisticeSamples();
		else
		if (gInkWorld->fPenState == kPenUp  &&  TabletBufferEmpty())
		{
			XFAIL(GetTabletState() == kTabletAwake)
			XFAIL(GetTabletState() == kTabletBypassed && !gFakeTablet)
			XFAIL(GetTabletState() == kTabletAsleep)
		}
		resetIdle();
	}
	XENDTRY;
}


void
CInkerEventHandler::inkThem(void)
{
	gInkWorld->LCDEntry();
}

