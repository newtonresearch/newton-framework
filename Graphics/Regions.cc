/*
	File:		Regions.cc

	Contains:	Screen update region implementation.
					We really donÕt want to be using QuickDraw regions in the 21st century.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Regions.h"
#include "Geometry.h"

/* -------------------------------------------------------------------------------
	C B a s e R e g i o n
------------------------------------------------------------------------------- */
#define kInitialCount 10

enum {
	opUnion = 1,
	opDiff,
	opSect
};


/* -----------------------------------------------------------------------------
	In the QuickDraw world there was an idiom for getting the viewportÕs region.
	Presumably this was screen/print area sized.
	We have collapsed that into a function call.
	Args:		--
	Return:	a region covering the display
----------------------------------------------------------------------------- */

CBaseRegion&
ViewPortRegion(void)
{
#if 0
	GrafPtr thePort;
	GetPort(&thePort);
	return thePort->visRgn;
#else
	static CRectangularRegion * visRgn = NULL;
	if (visRgn == NULL) {
		Rect bBox;
		SetRect(&bBox, 0, 0, 320, 480);
		visRgn = new CRectangularRegion(bBox);
	}
	return *visRgn;
#endif
}


/* -----------------------------------------------------------------------------
	C B a s e R e g i o n
----------------------------------------------------------------------------- */

CBaseRegion::CBaseRegion()
{
	allocCount = kInitialCount;
	rect = new Rect[kInitialCount];
	setEmpty();
}

CBaseRegion::~CBaseRegion()
{
	if (rect) {
		delete[] rect;
	}
}

void
CBaseRegion::setEmpty(void)
{
	extent = gZeroRect;
	count = 0;
}

bool
CBaseRegion::isEmpty(void) const
{
	return count == 0;
}

Rect
CBaseRegion::bounds(void) const
{
	return extent;
}

void
CBaseRegion::offset(int dx, int dy)
{
	Rect * r = rect;
	for (ArrayIndex i = 0; i < count; ++i, ++r) {
		OffsetRect(r, dx, dy);
	}
	OffsetRect(&extent, dx, dy);
}

bool
CBaseRegion::isEqual(const CBaseRegion& inRgn) const
{
	if (count == inRgn.count && EqualRect(&extent, &inRgn.extent)) {
		Rect * r1 = rect, * r2 = inRgn.rect;
		for (ArrayIndex i = 0; i < count; ++i, ++r1, ++r2) {
			if (!EqualRect(r1, r2)) {
				return false;
			}
		}
		return true;
	}
	return false;
}


/* -----------------------------------------------------------------------------
	Determine whether our region contains a point.
	Args:		inPt			a point
	Return:	true => the point is within our region
----------------------------------------------------------------------------- */

bool
CBaseRegion::contains(Point inPt) const
{
	if (PtInRect(inPt, &extent)) {
		Rect * r = rect;
		for (ArrayIndex i = 0; i < count; ++i, ++r) {
			if (PtInRect(inPt, r)) {
				return true;
			}
		}
	}
	return false;
}


/* -----------------------------------------------------------------------------
	Determine whether our region intersects a rectangle.
	Args:		inRect		a rectangle
	Return:	true => the rectangle partially overlaps our region
----------------------------------------------------------------------------- */

bool
CBaseRegion::intersects(const Rect * inRect) const
{
	if (RectsOverlap(&extent, inRect)) {
		Rect * r = rect;
		for (ArrayIndex i = 0; i < count; ++i, ++r) {
			if (RectsOverlap(r, inRect)) {
				return true;
			}
		}
	}
	return false;
}


/* -----------------------------------------------------------------------------
	Become a clone of another region.
	Args:		inRgn			another region
	Return:	this
----------------------------------------------------------------------------- */

CBaseRegion &
CBaseRegion::setRegion(const CBaseRegion& inRgn)
{
	extent = inRgn.extent;
	count = inRgn.count;
	allocCount = inRgn.allocCount;

	if (rect)
		delete[] rect;
	rect = new Rect[allocCount];
	memcpy(rect, inRgn.rect, count*sizeof(Rect));

	return *this;
}


/* -----------------------------------------------------------------------------
	Become the union of our region with another region.
	This means: add the other region to our region.
	Args:		inRgn			another region
	Return:	this
----------------------------------------------------------------------------- */

CBaseRegion&
CBaseRegion::unionRegion(const CBaseRegion& inRgn)
{
	// Trivial case #1. Region is this or empty
	if (this == &inRgn || inRgn.isEmpty()) {
		return *this;
	}

	// Trivial case #2. This is empty
	if (isEmpty()) {
		return setRegion(inRgn);
	}

	// Trivial case #3. This region covers the specified one
	if (count == 1 && RectContains(&extent, &inRgn.extent)) {
		return *this;
	}

	// Trivial case #4. The specified region covers this one
	if (inRgn.count == 1 && RectContains(&inRgn.extent, &extent)) {
		return setRegion(inRgn);
	}

	combine(inRgn, opUnion);

	// Update extent
	extent.top = MIN(extent.top, inRgn.extent.top);
	extent.left = MIN(extent.left, inRgn.extent.left);
	extent.bottom = MAX(extent.bottom, inRgn.extent.bottom);
	extent.right = MAX(extent.right, inRgn.extent.right);

	return *this;
}


/* -----------------------------------------------------------------------------
	Become the difference of our region with another region.
	This means: subtract the other region from our region.
	Args:		inRgn			another region
	Return:	this
----------------------------------------------------------------------------- */

CBaseRegion&
CBaseRegion::diffRegion(const CBaseRegion& inRgn)
{
	// trivial rejection
	if (!isEmpty() && !inRgn.isEmpty() && RectsOverlap(&extent, &inRgn.extent)) {
		combine(inRgn, opDiff);
		updateExtent();
	}
	return *this;
}


/* -----------------------------------------------------------------------------
	Become the intersection of our region with another region.
	Args:		inRgn			another region
	Return:	this
----------------------------------------------------------------------------- */

CBaseRegion&
CBaseRegion::sectRegion(const CBaseRegion& inRgn)
{
	// trivial rejection
	if (!isEmpty() && !inRgn.isEmpty() && RectsOverlap(&extent, &inRgn.extent)) {
		combine(inRgn, opSect);
		updateExtent();
	} else {
		setEmpty();
	}
	return *this;
}


/* -----------------------------------------------------------------------------
	Combine our region with another in some way.
	Args:		inRgn			another region
				inSelector	how to combine them
	Return:	--
----------------------------------------------------------------------------- */
ArrayIndex gRectCount = 0;
ArrayIndex gRectAllocCount = 0;
Rect * gRect = NULL;

void
CBaseRegion::checkMemory(Rect * inRects, ArrayIndex inCount)
{
	if (inRects == NULL) {
		if (inCount > gRectAllocCount) {
			gRectAllocCount = inCount * 2;
			Rect * newList = new Rect[gRectAllocCount];
			if (gRectCount > 0)
				memcpy(newList, gRect, gRectCount*sizeof(Rect));
			if (gRect)
				delete[] gRect;
			gRect = newList;
		}
	} else {
		if (inCount > allocCount) {
			allocCount = inCount * 2;
			Rect * newList = new Rect[allocCount];
			if (count > 0)
				memcpy(newList, rect, count*sizeof(Rect));
			if (rect)
				delete[] rect;
			rect = newList;
		}
	}
}


void
CBaseRegion::combine(const CBaseRegion& inRgn, int inSelector)
{
//static const char * ops[4] = { "<OUCH!>", "union", "diff", "sect" };
//printf("CBaseRegion::combine(%s)\n", ops[inSelector]);
//dump("this"); inRgn.dump("with");
	// lock access to gRect
	int r1 = 0, r1End = count;
	int r2 = 0, r2End = inRgn.count;
	gRectCount = 0;

	int yTop = 0, yBottom = extent.top < inRgn.extent.top ? extent.top : inRgn.extent.top;
	int currentBand, previousBand = 0;
	int r1BandEnd, r2BandEnd;
	int top, bottom;

	// Main loop
	do {
		currentBand = gRectCount;

		// Find end of the current r1 band (same top)
		for (r1BandEnd = r1 + 1; r1BandEnd != r1End && rect[r1BandEnd].top == rect[r1].top; ++r1BandEnd) {
		  ;
		}
		// Find end of the current r2 band
		for (r2BandEnd = r2 + 1; r2BandEnd != r2End && rect[r2BandEnd].top == rect[r2].top; ++r2BandEnd) {
		  ;
		}

		// First handle non-intersection band if any
		if (rect[r1].top < inRgn.rect[r2].top) {
			top    = MAX(rect[r1].top,    yBottom);
			bottom = MIN(rect[r1].bottom, inRgn.rect[r2].top);

			if (top != bottom) {
				nonOverlap1(rect, r1, r1BandEnd, top, bottom, inSelector);
			}
			yTop = inRgn.rect[r2].top;
		} else if (inRgn.rect[r2].top < rect[r1].top) {
			top    = MAX(inRgn.rect[r2].top,    yBottom);
			bottom = MIN(inRgn.rect[r2].bottom, rect[r1].top);

			if (top != bottom) {
				nonOverlap2 (inRgn.rect, r2, r2BandEnd, top, bottom, inSelector);
			}
			yTop = rect[r1].top;
		} else {
			yTop = rect[r1].top;
		}

		// Then coalesce if possible
		if (gRectCount != currentBand) {
			previousBand = coalesceBands(previousBand, currentBand);
		}
		currentBand = gRectCount;

		// Check if this is an intersecting band
		yBottom = MIN(rect[r1].bottom, inRgn.rect[r2].bottom);
		if (yBottom > yTop) {
			overlap(rect, r1, r1BandEnd, inRgn.rect, r2, r2BandEnd, yTop, yBottom, inSelector);
		}
		// Coalesce again
		if (gRectCount != currentBand) {
		  previousBand = coalesceBands(previousBand, currentBand);
		}

		// If we're done with a band, skip forward in the region to the next band
		if (rect[r1].bottom == yBottom) r1 = r1BandEnd;
		if (inRgn.rect[r2].bottom == yBottom) r2 = r2BandEnd;

	} while (r1 != r1End && r2 != r2End);

	currentBand = gRectCount;

	//
	// Deal with whichever region still has rectangles left
	//
	if (r1 != r1End) {
		do {
			for (r1BandEnd = r1; r1BandEnd < r1End && rect[r1BandEnd].top == rect[r1].top; ++r1BandEnd) {
			  ;
			}
			top = MAX(rect[r1].top, yBottom);
			bottom = rect[r1].bottom;
		  
			nonOverlap1(rect, r1, r1BandEnd, top, bottom, inSelector);
			r1 = r1BandEnd;
		  
		} while (r1 != r1End);
	} else if (r2 != r2End) {
		do {
			for (r2BandEnd = r2; r2BandEnd < r2End && rect[r2BandEnd].top == rect[r2].top; ++r2BandEnd) {
			  ;
			}

		  top = MAX(inRgn.rect[r2].top, yBottom);
		  bottom = inRgn.rect[r2].bottom;
		  
		  nonOverlap2(inRgn.rect, r2, r2BandEnd, top, bottom, inSelector);
		  r2 = r2BandEnd;
		} while (r2 != r2End);
	}

	// Coalesce again
	if (currentBand != gRectCount) {
		coalesceBands(previousBand, currentBand);
	}

	// Copy the work region into this
	checkMemory(rect, gRectCount);
	memcpy(rect, gRect, gRectCount*sizeof(Rect));
	count = gRectCount;

	// unlock access to gRect
//dump("-->");
}


void
CBaseRegion::nonOverlap1(Rect * inRect, int r, int rEnd, int yTop, int yBottom, int inSelector)
{
	if (inSelector == opUnion || inSelector == opDiff) {
		// add global rects
		ArrayIndex i;
		for (i = gRectCount; r != rEnd; ++i, ++r) {
			checkMemory(NULL, i + 1);

			gRect[i].top = yTop;
			gRect[i].bottom = yBottom;
			gRect[i].left = inRect[r].left;
			gRect[i].right = inRect[r].right;
		}
		gRectCount = i;
	}
}



void
CBaseRegion::nonOverlap2(Rect * inRect, int r, int rEnd, int yTop, int yBottom, int inSelector)
{
	if (inSelector == opUnion) {
		// add global rects
		ArrayIndex i;
		for (i = gRectCount; r != rEnd; ++i, ++r) {
			checkMemory(NULL, i + 1);

			gRect[i].top = yTop;
			gRect[i].bottom = yBottom;
			gRect[i].right = inRect[r].right;
			gRect[i].left = inRect[r].left;
		}
		gRectCount = i;
	}
}


void
CBaseRegion::overlap(Rect * inRect1, int r1, int r1End, Rect * inRect2, int r2, int r2End, int yTop, int yBottom, int inSelector)
{
	int i = gRectCount;

	//
	// UNION
	//
	if (inSelector == opUnion) {
		while (r1 != r1End && r2 != r2End) {
			if (inRect1[r1].left < inRect2[r2].left) {
				if (gRectCount > 0 &&
					 gRect[i-1].top == yTop &&
					 gRect[i-1].bottom == yBottom &&
					 gRect[i-1].right >= inRect1[r1].left) {
					if (gRect[i-1].right < inRect1[r1].right)
						gRect[i-1].right = inRect1[r1].right;
				} else {
					checkMemory(NULL, gRectCount + 1);
					
					gRect[i].top = yTop;
					gRect[i].bottom = yBottom;
					gRect[i].left = inRect1[r1].left;
					gRect[i].right = inRect1[r1].right;
					++i;
					++gRectCount;
				}
				++r1;
			} else {
				if (gRectCount > 0 &&
					 gRect[i-1].top == yTop &&
					 gRect[i-1].bottom == yBottom &&
					 gRect[i-1].right >= inRect2[r2].left) {
					if (gRect[i-1].right < inRect2[r2].right)
						gRect[i-1].right = inRect2[r2].right;
				} else {
					checkMemory(NULL, gRectCount + 1);

					gRect[i].top = yTop;
					gRect[i].bottom = yBottom;
					gRect[i].left = inRect2[r2].left;
					gRect[i].right = inRect2[r2].right;
					++i;
					++gRectCount;
				}
				++r2;
			}
		}

		if (r1 != r1End) {
			do {
				if (gRectCount > 0 &&
					 gRect[i-1].top == yTop &&
					 gRect[i-1].bottom == yBottom &&
					 gRect[i-1].right >= inRect1[r1].left) {
					if (gRect[i-1].right < inRect1[r1].right)
					  gRect[i-1].right = inRect1[r1].right;
				} else {
					checkMemory(NULL, gRectCount + 1);

					gRect[i].top = yTop;
					gRect[i].bottom = yBottom;
					gRect[i].left = inRect1[r1].left;
					gRect[i].right = inRect1[r1].right;
					++i;
					++gRectCount;
				}
				++r1;
			} while (r1 != r1End);
		} else {
			while (r2 != r2End) {
				if (gRectCount > 0 &&
					 gRect[i-1].top == yTop &&
					 gRect[i-1].bottom == yBottom &&
					 gRect[i-1].right >= inRect2[r2].left) {
					if (gRect[i-1].right < inRect2[r2].right) {
						gRect[i-1].right = inRect2[r2].right;
					}
				} else {
					checkMemory(NULL, gRectCount + 1);

					gRect[i].top = yTop;
					gRect[i].bottom = yBottom;
					gRect[i].left = inRect2[r2].left;
					gRect[i].right = inRect2[r2].right;
					++i;
					++gRectCount;
				}
				++r2;
			}
		}

	//
	// SUBTRACT
	//
	} else if (inSelector == opDiff) {
		int x1 = inRect1[r1].left;

		while (r1 != r1End && r2 != r2End) {
			if (inRect2[r2].right <= x1) {
				++r2;
			} else if (inRect2[r2].left <= x1) {
				x1 = inRect2[r2].right;
				if (x1 >= inRect1[r1].right) {
					++r1;
					if (r1 != r1End) x1 = inRect1[r1].left;
				} else {
					++r2;
				}
			} else if (inRect2[r2].left < inRect1[r1].right) {
				checkMemory(NULL, gRectCount + 1);
				 
				gRect[i].top = yTop;
				gRect[i].bottom = yBottom;
				gRect[i].left = x1;
				gRect[i].right = inRect2[r2].left;
				++i;
				++gRectCount;

				x1 = inRect2[r2].right;
				if (x1 >= inRect1[r1].right) {
					++r1;
					if (r1 != r1End)
						x1 = inRect1[r1].left;
					else
						++r2;
				}
			} else {
				if (inRect1[r1].right > x1) {
					checkMemory(NULL, gRectCount + 1);
					
					gRect[i].top = yTop;
					gRect[i].bottom = yBottom;
					gRect[i].left = x1;
					gRect[i].right = inRect1[r1].right;
					++i;
					++gRectCount;
				}
				 
				++r1;
				if (r1 != r1End)
					x1 = inRect1[r1].left;
			}
		}
		while (r1 != r1End) {
			checkMemory(NULL, gRectCount + 1);
			 
			gRect[i].top = yTop;
			gRect[i].bottom = yBottom;
			gRect[i].left = x1;
			gRect[i].right = inRect1[r1].right;
			++i;
			++gRectCount;

			r1++;
			if (r1 != r1End) {
				x1 = inRect1[r1].left;
			}
		}

	//
	// INTERSECT
	//
	} else if (inSelector == opSect) {
		// iterate over both sets of rects
		while (r1 != r1End && r2 != r2End) {
			// ->  <-
			int x1 = MAX(inRect1[r1].left, inRect2[r2].left);
			int x2 = MIN(inRect1[r1].right, inRect2[r2].right);

			if (x1 < x2) {
				// thereÕs an intersection -- add it to the global list
				checkMemory(NULL, gRectCount + 1);

				gRect[i].top = yTop;
				gRect[i].bottom = yBottom;
				gRect[i].left = x1;
				gRect[i].right = x2;
				++i;
				++gRectCount;
			}

			if (inRect1[r1].right < inRect2[r2].right) {
				// r1 is leftmost
				r1++;
			} else if (inRect2[r2].right < inRect1[r1].right) {
				// r2 is leftmost
				r2++;
			} else {
				// same
				r1++;
				r2++;
			}
		}
	}
}



/**
* Corresponds to miCoalesce in Region.c of X11.
*/
int
CBaseRegion::coalesceBands(int previousBand, int currentBand)
{
	int r1   = previousBand;
	int r2   = currentBand;
	int rEnd = gRectCount;

	// Number of rectangles in prevoius band
	int nRectanglesInPreviousBand = currentBand - previousBand;

	// Number of rectangles in current band
	int nRectanglesInCurrentBand  = 0;
	int y = gRect[r2].top;
	int r;
	for (r = r2; r != rEnd && gRect[r].top == y; ++r) {
		nRectanglesInCurrentBand++;
	}

	// If more than one band was added, we have to find the start
	// of the last band added so the next coalescing job can start
	// at the right place.
	if (r != rEnd) {
		for (--rEnd; gRect[rEnd-1].top == gRect[rEnd].top; --rEnd) {
			;
		}
		rEnd -= 4;
		while (gRect[rEnd-1].top == gRect[rEnd].top)
			--rEnd;

		currentBand = rEnd - gRectCount;
		rEnd = gRectCount;
	}

	if (nRectanglesInCurrentBand == nRectanglesInPreviousBand && nRectanglesInCurrentBand != 0) {

		// The bands may only be coalesced if the bottom of the previous
		// band matches the top of the current.
		if (gRect[r1].bottom == gRect[r2].top) {

			// Chek that the bands have boxes in the same places
			do {
				if ((gRect[r1].left != gRect[r2].left) || (gRect[r1].right != gRect[r2].right))
					return currentBand; // No coalescing
			 
				++r1;
				++r2;
			 
				nRectanglesInPreviousBand--;
			} while (nRectanglesInPreviousBand != 0);

			//
			// OK, the band can be coalesced
			//

			// Adjust number of rectangles and set pointers back to start
			gRectCount -= nRectanglesInCurrentBand;
			r1 -= nRectanglesInCurrentBand << 2;
			r2 -= nRectanglesInCurrentBand << 2;        

			// Do the merge
			do {
				gRect[r1].bottom = gRect[r2].bottom;
				++r1;
				++r2;
				nRectanglesInCurrentBand--;
			} while (nRectanglesInCurrentBand != 0);

			// If only one band was added we back up the current pointer
			if (r2 == rEnd) {
				currentBand = previousBand;
			} else {
				do {
					gRect[r1] = gRect[r2];
					r1++;
					r2++;
				} while (r2 != rEnd);
			}
		}
	}

	return currentBand;
}



/* -----------------------------------------------------------------------------
	Update the extent from the rects of our region.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CBaseRegion::updateExtent(void)
{
	if (count == 0) {
		extent = gZeroRect;
	} else {
		// Y values
		extent.top = rect[0].top;
		extent.bottom = rect[count-1].bottom;

		// X values initialize
		extent.left = rect[0].left;
		extent.right = rect[0].right;

		// Scan all rectangles for extreme X values
		Rect * r = rect+1;
		for (ArrayIndex i = 1; i < count; ++i, ++r) {
			if (r->left < extent.left) extent.left = r->left;
			if (r->right > extent.right) extent.right = r->right;
		}
	}
}


/* -----------------------------------------------------------------------------
	forDebug
	Print a description of the region.
	Args:		inTitle			something like "Region--"
	Return:	--
----------------------------------------------------------------------------- */

void
CBaseRegion::dump(const char * inTitle) const
{
	if (inTitle) {
		printf("%s\n", inTitle);
	}
	printf("Extent t:%d, l:%d, b:%d, r:%d\n", extent.top, extent.left, extent.bottom, extent.right);
	printf("%d rectangle%s--\n", count, count==1?"":"s");
	Rect * r = rect;
	for (ArrayIndex i = 0; i < count; ++i, ++r) {
		printf("  t:%d, l:%d, b:%d, r:%d\n", r->top, r->left, r->bottom, r->right);
	}
}


#pragma mark -
/* -------------------------------------------------------------------------------
	C R e c t a n g u l a r R e g i o n
------------------------------------------------------------------------------- */

CRectangularRegion::CRectangularRegion(Rect inRect)
{
	if (EmptyRect(&inRect)) {
		extent = gZeroRect;
		count = 0;
		rect = NULL;
	} else {
		extent = inRect;
		count = 1;
		rect = new Rect[1];
		rect[0] = inRect;
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C a c h e d R e g i o n
----------------------------------------------------------------------------- */
CBaseRegion * gCachedRgn;		// 0C1017B0

CBaseRegion *
NewCachedRgn(void)
{
	if (gCachedRgn != NULL) {
		CBaseRegion * rgn = gCachedRgn;
		gCachedRgn = NULL;
		return rgn;
	}

	CBaseRegion * rgn = new CBaseRegion;
	if (rgn == NULL) {
		OutOfMemory();
	}
	return rgn;
}

void
DisposeCachedRgn(CBaseRegion * inRgn)
{
	if (gCachedRgn == NULL) {
		gCachedRgn = inRgn;
	} else if (inRgn != NULL) {
		delete inRgn;
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C R e g i o n S t r u c t
	Convenience wrapper for CBaseRegion.
----------------------------------------------------------------------------- */

CRegionStruct::CRegionStruct()
{
	fRegion = NewCachedRgn();
}


CRegionStruct::~CRegionStruct()
{
	DisposeCachedRgn(fRegion);
}


CRegionStruct &
CRegionStruct::operator=(CRegion & inRgn)
{
	DisposeCachedRgn(fRegion);
	fRegion = inRgn.fRegion;
	return *this;
}


#pragma mark -
/* -------------------------------------------------------------------------------
	C R e g i o n V a r
	Convenience wrapper for CBaseRegion: catches exceptions and deletes the region.
------------------------------------------------------------------------------- */

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
	fRegion = inRgn.stealRegion();
	fException.header.catchType = kExceptionCleanup;
	fException.object = this;
	fException.function = DisposeRegionVar;
	AddExceptionHandler(&fException.header);
}

CRegionVar::~CRegionVar()
{
	RemoveExceptionHandler(&fException.header);
	if (fRegion) {
		DisposeCachedRgn(fRegion);
	}
}

CRegionVar &
CRegionVar::operator=(CRegion & inRgn)
{
	DisposeCachedRgn(fRegion);
	fRegion = inRgn.stealRegion();
	return *this;
}

CBaseRegion *
CRegionVar::stealRegion(void)
{
	CBaseRegion * rgn;
	rgn = fRegion, fRegion = NULL;
	return rgn;
}


#pragma mark -
/* -------------------------------------------------------------------------------
	C R e g i o n
------------------------------------------------------------------------------- */

CRegion::CRegion()
{
	fRegion = NULL;
}

CRegion::CRegion(CRegionVar & inRgn)
{
	fRegion = inRgn.stealRegion();
}

CRegion::~CRegion()
{
	if (fRegion)
		DisposeCachedRgn(fRegion);
}

CBaseRegion *
CRegion::stealRegion(void)
{
	CBaseRegion * rgn;
	rgn = fRegion, fRegion = NULL;
	return rgn;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C C l i p p e r
----------------------------------------------------------------------------- */
#include "View.h"

CClipper::CClipper()
{
	fFullRgn = new CRegionStruct;
	x04 = new CRegionStruct;
	fIsVisible = false;
}

void
CClipper::updateRegions(CView * inView)
{
	Rect bBox;
	inView->outerBounds(&bBox);

//	int roundness = ((inView->viewFormat & vfRoundMask) >> vfRoundShift) * 2;
//	if (roundness) {
//		OpenRgn();
//		FrameRoundRect(&bBox, roundness, roundness);
//		CloseRgn(*fFullRgn);
//	} else {
		CRectangularRegion bRgn(bBox);
		fFullRgn->fRegion->setRegion(bRgn);
//		RectRgn(*fFullRgn, &bBox);
//	}

	Rect wideOpenBox;
	wideOpenBox.top = -32767;
	wideOpenBox.left = -32767;
	wideOpenBox.bottom = 32767;
	wideOpenBox.right = 32767;
	CRectangularRegion wideOpenRgn(wideOpenBox);
	x04->fRegion->setRegion(wideOpenRgn);
//	RectRgn(*x04, &openBox);
}

void
CClipper::offset(Point inPt)
{
	fFullRgn->fRegion->offset(inPt.h, inPt.v);
//	OffsetRgn(*fFullRgn, inPt.h, inPt.v);
}

void
CClipper::recalcVisible(CBaseRegion& inRgn)
{
	x04->fRegion->setRegion(*fFullRgn->fRegion);
	x04->fRegion->diffRegion(inRgn);
	fIsVisible = x04->fRegion->isEqual(*fFullRgn->fRegion);
//	DiffRgn(*fFullRgn, inRgn, *x04);
//	fIsVisible = EqualRgn(*x04, *fFullRgn);
}

