/*
	File:		QDGeometry.cc

	Contains:	QuickDraw shape functions for Newton.

	Written by:	Newton Research Group.
*/

#include "Geometry.h"
#include "NewtonWidgets.h"
#include <stdarg.h>


const Point	gZeroPoint = { 0, 0 };
const Rect	gZeroRect = { 0, 0, 0, 0 };


/* -----------------------------------------------------------------------------
	P o i n t s
----------------------------------------------------------------------------- */

void
SetPt(Point * ioPt, short h, short v)
{
	ioPt->h = h;
	ioPt->v = v;
}

#pragma mark -

/* -----------------------------------------------------------------------------
	R e c t s
----------------------------------------------------------------------------- */

void
SetRect(Rect * r, short left, short top, short right, short bottom)
{
	r->left = left;
	r->top = top;
	r->right = right;
	r->bottom = bottom;
}


void
SetEmptyRect(Rect * r)
{
	r->left = 0;
	r->top = 0;
	r->right = 0;
	r->bottom = 0;
}


void
OffsetRect(Rect * r, short dh, short dv)
{
	r->left += dh;
	r->top += dv;
	r->right += dh;
	r->bottom += dv;
}


void
InsetRect(Rect * r, short dh, short dv)
{
	r->left += dh;
	r->top += dv;
	r->right -= dh;
	r->bottom -= dv;
}


void
TrimRect(const Rect * src1, const Rect * src2, Rect * dstRect)
{
	if (EmptyRect(src2))
		*dstRect = *src1;
	else if (EmptyRect(src1))
		*dstRect = gZeroRect;
	else
	{
		dstRect->top = (src1->top >= src2->top) ? src2->bottom : src1->top;
		dstRect->bottom = (src1->bottom <= src2->bottom) ? src2->top : src1->bottom;
		dstRect->left = src1->left;
		dstRect->right = src1->right;
	}
}

void
UnionRect(const Rect * src1, const Rect * src2, Rect * dstRect)
{
	if (EmptyRect(src1))
		*dstRect = *src2;
	else if (EmptyRect(src2))
		*dstRect = *src1;
	else
	{
		dstRect->left = MIN(src1->left, src2->left);
		dstRect->top = MIN(src1->top, src2->top);
		dstRect->right = MAX(src1->right, src2->right);
		dstRect->bottom = MAX(src1->bottom, src2->bottom);
	}
}

void
UnionRect(Rect * rect, Point pt)
{
	if (rect->top == -32768)
	{
		rect->top = pt.v;
		rect->left = pt.h;
		rect->bottom = pt.v;
		rect->right = pt.h;
	}
	else
	{
		if (rect->top > pt.v) rect->top = pt.v;
		if (rect->left > pt.h) rect->left = pt.h;
		if (rect->bottom < pt.v) rect->bottom = pt.v;
		if (rect->right  < pt.h) rect->right = pt.h;
	}
}


bool
RSect(Rect * result, long count, const Rect * r1, ...)
{
	Rect *	rn;
	va_list	args;
	va_start(args, r1);

	short	sectLeft = r1->left;
	short	sectTop = r1->top;
	short	sectRight = r1->right;
	short	sectBottom = r1->bottom;
	if (sectRight < sectLeft || sectBottom < sectTop)
		goto fail;

	for (count--; count > 0; count--)
	{
		rn = va_arg(args, Rect*);
		sectLeft = MAX(sectLeft, rn->left);
		sectTop = MAX(sectTop, rn->top);
		sectRight = MIN(sectRight, rn->right);
		sectBottom = MIN(sectBottom, rn->bottom);
		if (sectRight < sectLeft || sectBottom < sectTop)
			goto fail;
	}
	va_end(args);
	SetRect(result, sectLeft, sectTop, sectRight, sectBottom);
	return true;

fail:
	va_end(args);
	SetEmptyRect(result);
	return false;
}


bool
SectRect(const Rect * src1, const Rect * src2, Rect * dstRect)
{
	return RSect(dstRect, 2, src1, src2);
}


bool
Overlaps(const Rect * r1, const Rect * r2)
{
	Rect	rect1 = *r1;
	Rect	rect2 = *r2;

	if (rect1.left == rect1.right)
		rect1.right = rect1.right + 1;
	if (rect1.top == rect1.bottom)
		rect1.bottom = rect1.bottom + 1;

	if (rect2.left == rect2.right)
		rect2.right = rect2.right + 1;
	if (rect2.top == rect2.bottom)
		rect2.bottom = rect2.bottom + 1;

	return Intersects(&rect1, &rect2);
}


bool
Intersects(const Rect * r1, const Rect * r2)
{
	Rect	nullRect;
	return SectRect(r1, r2, &nullRect);
}


bool
EqualRect(const Rect * rect1, const Rect * rect2)
{
	return rect1->left == rect2->left
		 && rect1->top == rect2->top
		 && rect1->right == rect2->right
		 && rect1->bottom == rect2->bottom;
}


bool
EmptyRect(const Rect * rect)
{
	return rect->right <= rect->left || rect->bottom <= rect->top;
}


void
Pt2Rect(Point pt1, Point pt2, Rect * rect)
{
	if (pt2.h < pt1.h)
	{
		rect->left = pt2.h;
		rect->right = pt1.h;
	}
	else
	{
		rect->left = pt1.h;
		rect->right = pt2.h;
	}
	if (pt2.v < pt1.v)
	{
		rect->top = pt2.v;
		rect->bottom = pt1.v;
	}
	else
	{
		rect->top = pt1.v;
		rect->bottom = pt2.v;
	}
}


bool
PtInRect(Point pt, const Rect * rect)
{
	return pt.v >= rect->top && pt.v <= rect->bottom
		 && pt.h >= rect->left && pt.h <= rect->right;
}


Point
MidPoint(const Rect * rect)
{
	Point midPt;
	midPt.h = (rect->left + rect->right) / 2;
	midPt.v = (rect->top + rect->bottom) / 2;
	return midPt;
}


int
CheapDistance(Point inPt1, Point inPt2)
{
	int dh, dv;
	dh = inPt1.h - inPt2.h; if (dh < 0) dh = -dh;
	dv = inPt1.h - inPt2.h; if (dv < 0) dv = -dv;
	return (dh >= dv) ? dh + dv/2 : dv + dh/2;
}


/* -----------------------------------------------------------------------------
	Optimised Rect overlap detection for view update.
----------------------------------------------------------------------------- */

bool
RectsOverlap(const Rect * r1, const Rect * r2)
{
	return r2->right >= r1->left && r1->right >= r2->left
		 && r2->bottom >= r1->top && r1->bottom >= r2->top;
}


bool
RectContains(const Rect * r1, const Rect * r2)	// r1 contains r2
{
	return r2->top >= r1->top && r2->left >= r1->left
		 && r2->bottom <= r1->bottom && r2->right <= r1->right;
}

