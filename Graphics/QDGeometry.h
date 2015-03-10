/*
	File:		QDGeometry.h

	Contains:	Declares Point, Rect manipulators/accessors.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__QDGEOMETRY_H)
#define __QDGEOMETRY_H 1

#include "QDTypes.h"


extern const Point	gZeroPoint;
extern const Rect		gZeroRect;

//#if !defined(__QUARTZ_H)
extern void		SetPt(Point * ioPt, short h, short v);

extern void		SetRect(Rect * r, short left, short top, short right, short bottom);
extern void		SetEmptyRect(Rect * r);
extern void		OffsetRect(Rect * r, short dh, short dv);
extern void		InsetRect(Rect * r, short dh, short dv);
extern void		TrimRect(const Rect * src1, const Rect * src2, Rect * dstRect);
extern void		UnionRect(const Rect * src1, const Rect * src2, Rect * dstRect);
extern bool		SectRect(const Rect * src1, const Rect * src2, Rect * dstRect);
extern bool		EqualRect(const Rect * rect1, const Rect * rect2);
extern bool		EmptyRect(const Rect * rect);
extern void		Pt2Rect(Point pt1, Point pt2, Rect * rect);
extern bool		PtInRect(Point inPt, const Rect * rect);
//#endif
extern Point	MidPoint(const Rect * rect);
extern int		CheapDistance(Point inPt1, Point inPt2);

extern bool		Intersects(const Rect * r1, const Rect * r2);
extern bool		Overlaps(const Rect * r1, const Rect * r2);

inline int		RectGetWidth(Rect r)		{ return r.right - r.left; }
inline int		RectGetHeight(Rect r)	{ return r.bottom - r.top; }


#endif	/* __QDGEOMETRY_H */
