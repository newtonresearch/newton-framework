/*
	File:		Regions.cc

	Contains:	Region implementation.
					We really donÕt want to be using QuickDraw regions in the 21st century.

	Written by:	Newton Research Group.
*/

#include "Screen.h"


RgnHandle	gCachedRgn;		// 0C1017B0


RgnHandle
NewCachedRgn(void)
{
	if (gCachedRgn != NULL)
	{
		gCachedRgn = NULL;
		return gCachedRgn;
	}

	RgnHandle	rgn = NewRgn();
	if (rgn == NULL)
		OutOfMemory();
	return rgn;
}


void
DisposeCachedRgn(RgnHandle inRgn)
{
	if (gCachedRgn == NULL)
		gCachedRgn = inRgn;
	else if (inRgn != NULL)
		DisposeRgn(inRgn);
}


/*--------------------------------------------------------------------------------
	C R e c t a n g u l a r R e g i o n
--------------------------------------------------------------------------------*/

CRectangularRegion::CRectangularRegion(const Rect * inRect)
{
	fPtr = &fRegion;
	fRegion.rgnSize = sizeof(Region);
	fRegion.rgnBBox = *inRect;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C R e g i o n
--------------------------------------------------------------------------------*/

CRegion::CRegion(CRegionVar & inRgn)
{
	fRegion = inRgn.stealRegion();
}

CRegion::~CRegion()
{
	if (fRegion)
		DisposeCachedRgn(fRegion);
}

RgnHandle
CRegion::stealRegion(void)
{
	RgnHandle	rgn = fRegion;
	fRegion = NULL;
	return rgn;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C R e g i o n S t r u c t
------------------------------------------------------------------------------*/

CRegionStruct::CRegionStruct()
{
	fRegion = NewCachedRgn();
}


CRegionStruct::~CRegionStruct()
{
	DisposeCachedRgn(fRegion);
}


CRegionStruct &
CRegionStruct::operator =(CRegion & inRgn)
{
	DisposeCachedRgn(fRegion);
	fRegion = inRgn;
	return *this;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C R e g i o n V a r
--------------------------------------------------------------------------------*/

void
DisposeRegionVar(void * inData)
{
	delete (CRegionVar *)inData;
}

CRegionVar::CRegionVar()
{
	fRegion = NewCachedRgn();
	fException.header.catchType = kExceptionCleanup;
	fException.object = this;
	fException.function = DisposeRegionVar;
	AddExceptionHandler(&fException.header);
}

CRegionVar::CRegionVar(CRegion & inRgn)
{
	fRegion = inRgn;
	fException.header.catchType = kExceptionCleanup;
	fException.object = this;
	fException.function = DisposeRegionVar;
	AddExceptionHandler(&fException.header);
}

CRegionVar::~CRegionVar()
{
	RemoveExceptionHandler(&fException.header);
}

CRegionVar &
CRegionVar::operator =(CRegion & inRgn)
{
	DisposeCachedRgn(fRegion);
	fRegion = inRgn;
	return *this;
}

RgnHandle
CRegionVar::stealRegion(void)
{
	RgnHandle	rgn = fRegion;
	fRegion = NULL;
	return rgn;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C C l i p p e r
------------------------------------------------------------------------------*/
#include "View.h"

CClipper::CClipper()
{
	fViewRgn = new CRegionStruct;
	x04 = new CRegionStruct;
	fIsVisible = NO;
}

void
CClipper::updateRegions(CView * inView)
{
	Rect	bounds;
	inView->outerBounds(&bounds);

	long	roundness = ((inView->viewFormat & vfRoundMask) >> vfRoundShift) * 2;
	if (roundness)
	{
		OpenRgn();
	//	FrameRoundRect(&bounds, roundness, roundness);
		CloseRgn(*fViewRgn);
	}
	else
		RectRgn(*fViewRgn, &bounds);

	Rect	openBounds;
	openBounds.top = -32767;
	openBounds.left = -32767;
	openBounds.bottom = 32767;
	openBounds.right = 32767;
	RectRgn(*x04, &openBounds);
}

void
CClipper::offset(Point inPt)
{
	OffsetRgn(*fViewRgn, inPt.h, inPt.v);
}

void
CClipper::recalcVisible(RgnHandle inRgn)
{
	DiffRgn(*fViewRgn, inRgn, *x04);
	fIsVisible = EqualRgn(*x04, *fViewRgn);
}

