/*
	File:		Geometry.c

	Contains:	QuickDraw shape functions for Newton.

	Written by:	Newton Research Group.
*/

#include "Geometry.h"
#include "NewtonWidgets.h"

#include <stdarg.h>


const Point	gZeroPoint = { 0, 0 };
const Rect	gZeroRect = { 0, 0, 0, 0 };

// this is all done by the QD.framework
#if 0
/*------------------------------------------------------------------------------
	P o i n t s
------------------------------------------------------------------------------*/

void
SetPt(Point * ioPt, short h, short v)
{
	ioPt->h = h;
	ioPt->v = v;
}

#pragma mark -

/*------------------------------------------------------------------------------
	R e c t s
------------------------------------------------------------------------------*/

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


BOOL
RSect(Rect * result, long count, const Rect * r1, ...)
{
	Rect *	rn;
	va_list	args;
	va_start(args, r1);

	short	sectLeft = r1->right;
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
	return YES;

fail:
	va_end(args);
	SetEmptyRect(result);
	return NO;
}


BOOL
SectRect(const Rect * src1, const Rect * src2, Rect * dstRect)
{
	return RSect(dstRect, 2, src1, src2);
}


BOOL
EqualRect(const Rect * rect1, const Rect * rect2)
{
	return rect1->left == rect2->left
		 && rect1->top == rect2->top
		 && rect1->right == rect2->right
		 && rect1->bottom == rect2->bottom;
}


BOOL
EmptyRect(const Rect * rect)
{
	return rect->right <= rect->left || rect->bottom <= rect->top;
}
#endif

Point
MidPoint(const Rect * rect)
{
	Point midPt;
	midPt.h = (rect->left + rect->right) / 2;
	midPt.v = (rect->top + rect->bottom) / 2;
	return midPt;
}
