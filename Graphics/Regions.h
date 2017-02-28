/*
	File:		Regions.h

	Contains:	Region declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__REGIONS_H)
#define __REGIONS_H 1

#include "QDTypes.h"
#include "NewtonExceptions.h"

/* -------------------------------------------------------------------------------
	R e g i o n s
	We don’t use regions for drawing (stroke/fill), quartz doesn’t do them.
------------------------------------------------------------------------------- */
#if 0
struct Rgn
{
	int	size;
	Rect	bBox;
	char	data[];
};
typedef Rgn * RgnPtr, ** RgnHandle;
#endif

/* -------------------------------------------------------------------------------
	U p d a t e   R e g i o n s
	However: areas of the display that need redrawing are gathered into regions.
	We implement them as a list of Rects.
------------------------------------------------------------------------------- */

struct Region
{
	Rect		extent;
	int		count;
	int		allocCount;
	Rect *	rect;
};

/* -------------------------------------------------------------------------------
	C B a s e R e g i o n
	Was synonym for RgnHandle
	CRootView::update definitely returns this from a RegionVar
------------------------------------------------------------------------------- */

class CBaseRegion
{
public:
				CBaseRegion();
				~CBaseRegion();

	void		setEmpty(void);
	bool		isEmpty(void) const;
	Rect		bounds(void) const;
	void		offset(int dx, int dy);
	bool		isEqual(const CBaseRegion& inRgn) const;
	bool		contains(Point inPt) const;
	bool		intersects(const Rect * inRect) const;		// same semantics as RectInRgn?
	CBaseRegion& setRegion(const CBaseRegion& inRgn);
	CBaseRegion& unionRegion(const CBaseRegion& inRgn);
	CBaseRegion& diffRegion(const CBaseRegion& inRgn);
	CBaseRegion& sectRegion(const CBaseRegion& inRgn);
	//forDebug
	void		dump(const char * inTitle) const;

protected:
	void		combine(const CBaseRegion& inRgn, int inSelector);
	void		nonOverlap1(Rect * inRect, int r, int rEnd, int yTop, int yBottom, int inSelector);
	void		nonOverlap2(Rect * inRect, int r, int rEnd, int yTop, int yBottom, int inSelector);
	void		overlap(Rect * inRect1, int r1, int r1End, Rect * inRect2, int r2, int r2End, int yTop, int yBottom, int inSelector);
	int		coalesceBands(int inPreviousBand, int inCurrentBand);
	void		checkMemory(Rect * inRects, ArrayIndex inCount);
	void		updateExtent(void);

	Rect		extent;
	int		count;
	int		allocCount;
	Rect *	rect;
};

// a single Rect that defines an invalid display area that needs drawing
class CRectangularRegion : public CBaseRegion
{
public:
		CRectangularRegion(Rect inRect);
};


/*--------------------------------------------------------------------------------
	C R e g i o n
	Like Ref.
	For passing around a region; returning a region from a function.
--------------------------------------------------------------------------------*/
class CRegionVar;

class CRegion
{
public:
						CRegion(CRegionVar & inRgn);	// HAVE to create a CRegion from a CRegionVar
						~CRegion();

	CBaseRegion *	stealRegion(void);
	CBaseRegion *	fRegion;

protected:
						CRegion();
};


/*------------------------------------------------------------------------------
	C R e g i o n S t r u c t
	Like RefStruct.
	Convenience wrapper for CBaseRegion.
------------------------------------------------------------------------------*/

class CRegionStruct
{
public:
							CRegionStruct();
							~CRegionStruct();

	CRegionStruct &	operator=(CRegion & inRgn);
	CBaseRegion *		fRegion;
};


/*--------------------------------------------------------------------------------
	C R e g i o n V a r
	Like RefVar.
	Convenience wrapper for CBaseRegion: catches exceptions and deletes the region.
--------------------------------------------------------------------------------*/

class CRegionVar
{
public:
						CRegionVar();
						CRegionVar(CRegion & inRgn);
						~CRegionVar();

	CBaseRegion *	stealRegion(void);
	CBaseRegion *	fRegion;
	CRegionVar &	operator=(CRegion & inRgn);

private:
	ExceptionCleanup	fException;
};


/*------------------------------------------------------------------------------
	C C l i p p e r
	Represents the clipping region for a view.
------------------------------------------------------------------------------*/
class CView;

class CClipper
{
public:
				CClipper();

	void		updateRegions(CView * inView);
	void		offset(Point inPt);
	void		recalcVisible(CBaseRegion& inRgn);

	CRegionStruct *	fFullRgn;	// +00
	CRegionStruct *	x04;
	bool					fIsVisible;	// +08
};


extern CBaseRegion& ViewPortRegion(void);


#endif	/* __REGIONS_H */
