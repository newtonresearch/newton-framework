/*
	File:		Stroke.cc

	Contains:	Public Stroke implementation.

	Written by:	Newton Research Group.
*/

#include "RootView.h"

#include "Transformations.h"
#include "Stroke.h"

// from Ink.cc
extern void		AdjustForInk(Rect * ioBounds);

// from Inker.cc
extern void		InkerOff(Rect * inRect);
extern void		InkerOffUnhobbled(Rect * inRect);


#pragma mark CStroke
/*--------------------------------------------------------------------------------
	C S t r o k e
	Was TStrokePublic.
--------------------------------------------------------------------------------*/

CStroke *
CStroke::make(CRecStroke * inStroke, bool inTakeStrokeOwnership)
{
	return new CStroke(inStroke, inTakeStrokeOwnership);
}


CStroke::CStroke(CRecStroke * inStroke, bool inTakeStrokeOwnership)
{
	f00 = 0;
	fStroke = inStroke;
	fStrokeIsOurs = inTakeStrokeOwnership;
	fBounds.left = fBounds.right = -32768;		// fBounds was a TRect -- this is its ctor
}


CStroke::~CStroke(void)
{
	if (fStrokeIsOurs)
		fStroke->release();
}


size_t
CStroke::count(void)
{
	return fStroke->count();
}


bool
CStroke::isDone(void)
{
	return fStroke->isDone();
}


ULong
CStroke::downTime(void)
{
	return fStroke->startTime();
}


ULong
CStroke::upTime(void)
{
	return fStroke->endTime();
}


void
CStroke::bounds(Rect * outBounds)
{
	fStroke->getStrokeRect(outBounds);
	outBounds->right++;
	outBounds->bottom++;
}


Point
CStroke::getPoint(ArrayIndex index)
{
	Point pt;
	SamplePt * p;
	bool wasAcquired = AcquireStroke(fStroke);
	ArrayIndex numOfPoints = fStroke->count();
	if (index >= numOfPoints)
		index = numOfPoints - 1;
	p = fStroke->getPoint(index);
	if (wasAcquired)
		ReleaseStroke();

	pt.h = (short) (SampleX(p) + 0.5);
	pt.v = (short) (SampleY(p) + 0.5);

	return pt;
}


Point
CStroke::firstPoint(void)
{
	return getPoint(0);
}


Point
CStroke::finalPoint(void)
{
	return getPoint(count() - 1);
}


void
CStroke::inkOn(void)
{ /* this really does nothing */ }


void
CStroke::inkOff(bool inArg1)
{
	inkOff(inArg1, YES);
}


void
CStroke::inkOff(bool inArg1, bool inHobbled)
{
	if (!fStroke->testFlags(0x20000000))
	{
		bool wasAcquired = AcquireStroke(fStroke);
		fStroke->setFlags(0x20000000);
		if (wasAcquired)
			ReleaseStroke();

		if (!fStroke->isDone())
		{
			if (inHobbled)
				InkerOff(&fBounds);
			else
				InkerOffUnhobbled(&fBounds);
		}
		Rect inkyRect;
		getInkedRect(&inkyRect);

		if (inArg1)
		{
			if (!fStroke->testFlags(0x04000000))
			{
//				StartDrawing(NULL, &inkyRect);
//				StopDrawing(NULL, &inkyRect);
			}
			else
				gRootView->smartInvalidate(&inkyRect);
		}
		else
			gRootView->smartScreenDirty(&inkyRect);
	}
}


void
CStroke::getInkedRect(Rect * outBounds)
{
	if (fBounds.left == -32768)
	{
		GetStrokeRect(fStroke, &fBounds);
		AdjustForInk(&fBounds);
	}
	*outBounds = fBounds;
}


void
CStroke::invalidate(void)
{
	if (!fStroke->testFlags(0x20000000))
	{
		Rect inkyRect;
		getInkedRect(&inkyRect);
#if 0
		gRootView->smartInvalidate(&inkyRect);
#endif
	}
}


#pragma mark -
/*--------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
--------------------------------------------------------------------------------*/

extern "C"
{
}

