/*
	File:		RecStroke.cc

	Contains:	CRecStroke implementation.
					NOTE change from original of Fixed -> float.
					Also affect FPoint, FRect.

	Written by:	Newton Research Group, 2010.
*/

#include "Quartz.h"
#include "QDDrawing.h"

#include "RecStroke.h"
#include "Transformations.h"

extern void		InkerLine(const Point inPt1, const Point inPt2, Rect * outRect, const Point inPenSize);

extern bool		AcquireStroke(CRecStroke * inStroke);
extern void		ReleaseStroke(void);

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern int	gLastPenTip;			// 0C1008BC

extern bool	gDefaultInk;			// 0C101890

//Heap					gStrokerHeap;		// 0C104D24


#pragma mark Points
/*------------------------------------------------------------------------------
	Transformations between point formats.
------------------------------------------------------------------------------*/

inline void
SetPoint(SamplePt * outSample, FPoint * inPt)
{
	float v;

	v = inPt->x;
	if (v < 0.0) v = 0.0;
	outSample->x = v * kTabScale;

	v = inPt->y;
	if (v < 0.0) v = 0.0;
	outSample->y = v * kTabScale;
}


inline void
SetPoint(SamplePt * outSample, TabPt * inPt)
{
	SetPoint(outSample, (FPoint *)inPt);

	short z = inPt->z;
	if (z > 7) z = 7;
	outSample->zhi = z >> 1;
	outSample->zlo = z;

	outSample->p = inPt->p;
}


#pragma mark CRecStroke
/*------------------------------------------------------------------------------
	C R e c S t r o k e
	A CRecStroke is a dynamic array of SamplePt points.
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Make a new CRecStroke.
	Args:		inNumOfPts			number of points the stroke should hold
	Return:	new CRecStroke
------------------------------------------------------------------------------*/

CRecStroke *
CRecStroke::make(ArrayIndex inNumOfPts)
{
	CRecStroke * stroke = new CRecStroke;
	XTRY
	{
		XFAIL(stroke == NULL)
		XFAILIF(stroke->iStroke(inNumOfPts) != noErr, stroke->release(); stroke = NULL;)
	}
	XENDTRY;
	return stroke;
}


/*------------------------------------------------------------------------------
	Initialize a new CRecStroke.
	Args:		inNumOfPts			number of points the stroke should hold
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CRecStroke::iStroke(ArrayIndex inNumOfPts)
{
	NewtonErr	err;
	if ((err = iDArray(sizeof(SamplePt), inNumOfPts)) == noErr)	// original says iArray
	{
		FRect  box;
		fStartTime = 0;
		fEndTime = 0;
		f40 = 0;
		f3C = 0;
		f44 = 1;
		f46 = 0;
		SetRectangleEmpty(&box);
		fBBox = box;
		setFlags(gLastPenTip << 8);  // pen size
		if (!gDefaultInk)
			setFlags(0x20000000);
		f30 = 0;
		fEvt = 0;
	}

	return err;
}


/*------------------------------------------------------------------------------
	Return the storage size of the dynamic array.
	Args:		--
	Return:	size in bytes
------------------------------------------------------------------------------*/

size_t
CRecStroke::sizeInBytes(void)
{
	return CDArray::sizeInBytes();	// original says CArray
}


/*------------------------------------------------------------------------------
	Dispose the CRecStroke.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CRecStroke::dealloc(void)
{
/*	if (GetPtrSize((Ptr)this) == 0x58)
	{
		Heap  savedHeap = GetHeap();
		SetHeap(gStrokerHeap);
		CDArray::dealloc();
		SetHeap(savedHeap);
	}
	else
*/	// super dealloc
		CDArray::dealloc();
}


/*------------------------------------------------------------------------------
	Add a point from the tablet.
	Args:		inPt					the point to be added
	Return:	error code			non-zero indicates error
------------------------------------------------------------------------------*/

NewtonErr
CRecStroke::addPoint(TabPt * inPt)
{
	NewtonErr err = noErr;
	SamplePt * p;

	if (f44 != 0
	&&  f44 <= ++f46)
	{
		XTRY
		{
			f46 = 0;
			if (count() > 800)	// sic -- 100 * sizeof(SamplePt)? BeginStroke (DrawInk.cc) doesn’t allow more than 100 pts/stroke
				bifurcate();
			if (count() == 0)
			{
				XFAILNOT(p = tryToAddPoint(), err = 1;)
				SetPoint(p, inPt);
				AddPtToRect((FPoint *)inPt, &fBBox, true);
				fBBox.right += 0.001;		// original adds 1 to Fixed
				fBBox.bottom += 0.001;
			}
			else
			{
				getPoint(count()-1);
				XFAILNOT(p = tryToAddPoint(), err = 1;)
				SetPoint(p, inPt);
				AddPtToRect((FPoint *)inPt, &fBBox, false);
			}
		}
		XENDTRY;
	}
	return err;
}


/*------------------------------------------------------------------------------
	Create a new point in the dynamic array.
	Args:		--
	Return:	pointer to the new point entry
------------------------------------------------------------------------------*/

SamplePt *
CRecStroke::tryToAddPoint(void)
{
	SamplePt * p = (SamplePt *)addEntry();
	if (p == NULL)
	{
		// we ran out of memory! limp on a bit longer by halving our memory requirement
		bifurcate();
		p = (SamplePt *)addEntry();
	}
	return p;
}


/*------------------------------------------------------------------------------
	Halve the number of sample points held.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CRecStroke::bifurcate(void)
{
	f44 *= 2;
	if (f44 != 0)
	{
		SamplePt *  srcPt = getPoint(0);
		SamplePt *  dstPt = srcPt;
		ArrayIndex  srci, dsti;
		for (srci = 0, dsti = 0; srci < count(); srci += 2, srcPt += 2, ++dsti, ++dstPt)
			*dstPt = *srcPt;
		cutToIndex(dsti);
		f30 /= 2;
	}
}


/*------------------------------------------------------------------------------
	Finish up the stroke.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CRecStroke::endStroke(void)
{
	setFlags(kDoneStroke);
	compact();
	fBBox.right += 0.001;		// original adds 1 to Fixed
	fBBox.bottom += 0.001;
}


/*------------------------------------------------------------------------------
	Determine whether the stroke is done.
	Args:		--
	Return:	true => stroke is done
------------------------------------------------------------------------------*/

bool
CRecStroke::isDone(void)
{
	return testFlags(kDoneStroke);
}


/*------------------------------------------------------------------------------
	Return a sample point from the dynamic array.
	Args:		index
	Return:	pointer to the sample point entry
------------------------------------------------------------------------------*/

SamplePt *
CRecStroke::getPoint(ArrayIndex index)
{
	return (SamplePt *)getEntry(index);
}


/*------------------------------------------------------------------------------
	Return a point from the dynamic array as a tablet point.
	Args:		index
				outPt
	Return:	--
------------------------------------------------------------------------------*/

void
CRecStroke::getTabPt(ArrayIndex index, TabPt * outPt)
{
	SamplePt *  p = (SamplePt *)getEntry(index);
	outPt->x = SampleX(p);
	outPt->y = SampleY(p);
	outPt->z = SampleP(p);
	outPt->p = p->p;
}


/*------------------------------------------------------------------------------
	Return a point from the dynamic array as an F point.
	Args:		index
				outPt
	Return:	--
------------------------------------------------------------------------------*/

void
CRecStroke::getFPoint(ArrayIndex index, FPoint * outPt)
{
	SamplePt *  p = (SamplePt *)getEntry(index);
	p->getPoint(outPt);
}


/*------------------------------------------------------------------------------
	Offset all the points in the stroke.
	Args:		dx
				dy
	Return:	--
------------------------------------------------------------------------------*/

void
CRecStroke::offset(float dx, float dy)
{
	FPoint pt;
	SamplePt * p = getPoint(0);
	for (ArrayIndex i = 0; i < count(); ++i, ++p)
	{
		p->getPoint(&pt);
		pt.x += dx;
		pt.y += dy;
		SetPoint(p, &pt);
	}
	fBBox.top += dy;
	fBBox.bottom += dy;
	fBBox.left += dx;
	fBBox.right += dx;
}


/*------------------------------------------------------------------------------
	Rotate the stroke about its centre.
	Args:		inDegrees
	Return:	--
------------------------------------------------------------------------------*/

void
CRecStroke::rotate(float inDegrees)
{
	FPoint theCentre;
	RectangleCenter(&fBBox, &theCentre);

	Matrix xformer;
	MxInit(&xformer);
	RotateMatrix(&xformer, -inDegrees, theCentre.x, theCentre.y);
	
	FPoint xformedPt;
	SamplePt * p;
	for (ArrayIndex i = 0; i < count(); ++i, ++p)
	{
		p = getPoint(i);
		p->getPoint(&xformedPt);
		MxTransform(&xformer, &xformedPt);
		SetPoint(p, &xformedPt);
	}
	updateBBox();
}


/*------------------------------------------------------------------------------
	Scale the stroke.
	Args:		dx
				dy
	Return:	--
------------------------------------------------------------------------------*/

void
CRecStroke::scale(float inX, float inY)
{
	Matrix xformer;
	MxInit(&xformer);
	MxScale(&xformer, inX, inY);

	float dx = 0.0, dy = 0.0;
	if (inX < 0.0)
		dx = fBBox.left + fBBox.right;
	if (inY < 0.0)
		dy = fBBox.top + fBBox.bottom;
	if (dx != 0.0 || dy != 0.0)
		MxMove(&xformer, dx, dy);
	
	FPoint xformedPt;
	SamplePt * p;
	for (ArrayIndex i = 0; i < count(); ++i, ++p)
	{
		p = getPoint(i);
		p->getPoint(&xformedPt);
		MxTransform(&xformer, &xformedPt);
		SetPoint(p, &xformedPt);
	}
	updateBBox();
}


void
CRecStroke::map(FRect * inBox)
{
	FRect srcBox = fBBox;
	FRect dstBox = *inBox;
	GetMapper(&srcBox, &dstBox);

	FPoint pt;
	SamplePt * p = getPoint(0);
	for (ArrayIndex i = 0; i < count(); ++i, ++p)
	{
		p->getPoint(&pt);
		MapPoint(&pt, &srcBox, &dstBox);
		SetPoint(p, &pt);		// original doesn’t do negative bounds checks
	}
}


void
CRecStroke::updateBBox(void)
{
	FPoint pt;
	SamplePt * p = getPoint(0);
	for (ArrayIndex i = 0; i < count(); ++i, ++p)
	{
		p->getPoint(&pt);
		AddPtToRect(&pt, &fBBox, i==0);
	}
	fBBox.right += 0.001;		// original adds 1 to Fixed; maybe they meant to add 1.0?
	fBBox.bottom += 0.001;
}


void
CRecStroke::draw(void)
{
	setFlags(0x04000000);
	if (isDone())
		setFlags(0x08000000);
	if (!testFlags(0x20000000))
	{
		Rect  inkedBox;			// sp00
		Point penSize;				// sp08
		Point thisPt, nextPt;	// sp10, sp0C
		bool  wasAcquired = AcquireStroke(this);
		if (count() > 0)
		{
			SamplePt *  p = getPoint(0);
			thisPt.h = (short) (SampleX(p) + 0.5);
			thisPt.v = (short) (SampleY(p) + 0.5);
			p++;
			penSize.h = penSize.v = (fFlags & 0xFF00) >> 8;

CGContextSetLineWidth(quartz, penSize.h);
CGContextBeginPath(quartz);
CGContextMoveToPoint(quartz, thisPt.h, thisPt.v);

			for (ArrayIndex i = 1; i < count(); ++i, ++p)
			{
				nextPt.h = (short) (SampleX(p) + 0.5);
				nextPt.v = (short) (SampleY(p) + 0.5);
				SampleP(p);	// probably assigned to something, but not used
				if (nextPt.h != thisPt.h  ||  nextPt.v != thisPt.v)
				{
CGContextAddLineToPoint(quartz, nextPt.h, nextPt.v);
					InkerLine(thisPt, nextPt, &inkedBox, penSize);
				}
				thisPt = nextPt;
			}
CGContextStrokePath(quartz);
		}
		if (wasAcquired)
			ReleaseStroke();
	}
}


#pragma mark RecStroke Functions
/*------------------------------------------------------------------------------
	CRecStroke manipulation.
------------------------------------------------------------------------------*/

void
DisposeRecStrokes(CRecStroke ** inStrokes)
{
	CRecStroke * stroke;
	for (ArrayIndex i = 0; (stroke = inStrokes[i]) != NULL; ++i)
		stroke->release();
	free(inStrokes);
}


ULong
CountRecStrokes(CRecStroke ** inStrokes)
{
	ULong count = 0;
	while (inStrokes[count] != NULL)
		count++;
	return count;
}


void
OffsetStrokes(CRecStroke ** inStrokes, float dx, float dy)
{
	CRecStroke * stroke;
	for (ArrayIndex i = 0; (stroke = inStrokes[i]) != NULL; ++i)
		stroke->offset(dx, dy);
}


void
GetStrokeRect(CRecStroke * inStroke, Rect * outBounds)
{
	inStroke->getStrokeRect(outBounds);
	if (outBounds->right == outBounds->left)
		outBounds->right++;
	if (outBounds->bottom == outBounds->top)
		outBounds->bottom++;
}


void
UnionBounds(CRecStroke ** inStrokes, Rect * outBounds)
{
	Rect rect;
	CRecStroke * stroke;
	for (ArrayIndex i = 0; (stroke = inStrokes[i]) != NULL; ++i)
	{
		GetStrokeRect(stroke, &rect);
		UnionRect(&rect, outBounds, outBounds);
	}
}


void
InkBounds(CRecStroke ** inStrokes, Rect * outBounds)
{
	UnionBounds(inStrokes, outBounds);
	InsetRect(outBounds, -2, -2);
}


PolygonShape *
AsPolygon(CRecStroke * inStroke)
{
	ArrayIndex count = inStroke->count();
	size_t polySize = sizeof(PolygonShape) + count * sizeof(Point);
	PolygonShape * poly = (PolygonShape *)NewPtr(polySize);	// was NewHandle
	if (poly != NULL)
	{
//		SetPtrName(poly, 'AsPl');
		poly->size = polySize;
		GetStrokeRect(inStroke, &poly->bBox);
		SamplePt * strkPt = inStroke->getPoint(0);
		Point * polyPt = poly->points;
		for (ArrayIndex i = 0; i < count; ++i, ++strkPt, ++polyPt)
		{
			polyPt->h = (SampleX(strkPt) + 0.5) - poly->bBox.left;
			polyPt->v = (SampleY(strkPt) + 0.5) - poly->bBox.top;
		}
	}
	return poly;
}

