/*
	File:		Geometry.h

	Contains:	QuickDraw shape declarations for Newton.

	Written by:	Newton Research Group.
*/

#if !defined(__GEOMETRY_H)
#define __GEOMETRY_H 1

#include "NewtonTypes.h"


extern const Point	gZeroPoint;
extern const Rect		gZeroRect;

#if defined(__cplusplus)
extern "C" {
#endif

void		SetPt(Point * ioPt, short h, short v);

void		SetRect(Rect * r, short left, short top, short right, short bottom);
void		SetEmptyRect(Rect * r);
void		OffsetRect(Rect * r, short dh, short dv);
void		InsetRect(Rect * r, short dh, short dv);
void		TrimRect(const Rect * src1, const Rect * src2, Rect * dstRect);
void		UnionRect(const Rect * src1, const Rect * src2, Rect * dstRect);
#if 0
bool		SectRect(const Rect * src1, const Rect * src2, Rect * dstRect);
bool		EqualRect(const Rect * rect1, const Rect * rect2);
bool		EmptyRect(const Rect * rect);
bool		PtInRect(Point inPt, const Rect * rect);
#endif
Point		MidPoint(const Rect * rect);

bool		Intersects(const Rect * r1, const Rect * r2);
bool		Overlaps(const Rect * r1, const Rect * r2);

#if defined(__cplusplus)
}
#endif

inline int
RectGetWidth(Rect inRect)
{ return inRect.right - inRect.left; }

inline int
RectGetHeight(Rect inRect)
{ return inRect.bottom - inRect.top; }

#endif	/* __GEOMETRY_H */
