/*
	File:		InkGeometry.cc

	Contains:	Geometry functions for the recognition subsystem.
					NOTE non-integral operations now use type float rather than Fixed.

	Written by:	Newton Research Group, 2010.
*/

#include "InkGeometry.h"
#include <math.h>


#pragma mark Conversion
/* -----------------------------------------------------------------------------
	Convert Rect to FRect.
	Named FixRect for historical reasons -- we use float these days.
	Args:		outRect		FRect target
				inRect		Rect source
	Return:	--
----------------------------------------------------------------------------- */

void
FixRect(FRect * outRect, Rect * inRect)
{
	outRect->left = (float)inRect->left;
	outRect->top = (float)inRect->top;
	outRect->right = (float)inRect->right;
	outRect->bottom = (float)inRect->bottom;
}


/* -----------------------------------------------------------------------------
	Convert FRect to Rect.
	Named UnfixRect for historical reasons -- we use float these days.
	Args:		inRect		FRect source
				outRect		Rect target
	Return:	--
----------------------------------------------------------------------------- */

void
UnfixRect(FRect * inRect, Rect * outRect)
{
	outRect->left = (short)inRect->left;
	outRect->top = (short)inRect->top;
	outRect->right = (short)inRect->right;
	outRect->bottom = (short)inRect->bottom;
}


#pragma mark Rectangles
/* -----------------------------------------------------------------------------
	Set an empty zero FRect.
	Args:		ioRect		the FRect to empty
	Return:	--
----------------------------------------------------------------------------- */

void
SetRectangleEmpty(FRect * ioRect)
{
	ioRect->left =
	ioRect->top =
	ioRect->right =
	ioRect->bottom = 0.0;
}


/* -----------------------------------------------------------------------------
	Set an empty FRect at a point.
	Args:		ioRect		the FRect to set
				inPt			the point
	Return:	--
----------------------------------------------------------------------------- */

void
SetRectanglePoint(FRect * ioRect, FPoint inPt)
{
	ioRect->left =
	ioRect->right = inPt.x;
	ioRect->top =
	ioRect->bottom = inPt.y;
}


/* -----------------------------------------------------------------------------
	Set an FRect from a set of ordinates.
	Args:		ioRect		the FRect to set
				inLeft...	the ordinates
	Return:	--
----------------------------------------------------------------------------- */

void
SetRectangleEdges(FRect * ioRect, float inLeft, float inTop, float inRight, float inBottom)
{
	ioRect->left = inLeft;
	ioRect->top = inTop;
	ioRect->right = inRight;
	ioRect->bottom = inBottom;
}


/* -----------------------------------------------------------------------------
	Determine whether an FRect is empty.
	Args:		inRect		the FRect to test
	Return:	YES => is empty
----------------------------------------------------------------------------- */

bool
EmptyRectangle(FRect * inRect)
{
	return (inRect->left >= inRect->right) || (inRect->top >= inRect->bottom);
}


/* -----------------------------------------------------------------------------
	Inset an FRect.
	Args:		ioRect		the FRect to inset
				dx, dy		the inset
	Return:	--
----------------------------------------------------------------------------- */

void
InsetRectangle(FRect * ioRect, float dx, float dy)
{
	ioRect->top += dy;
	ioRect->left += dx;
	ioRect->bottom -= dy;
	ioRect->right -= dx;
}


/* -----------------------------------------------------------------------------
	Determine whether an FPoint is enclosed by an FRect.
	Args:		inPt			the FPoint
				inRect		the FRect
	Return:	YES => point lies within the rectangle
----------------------------------------------------------------------------- */

bool
PointInRectangle(FPoint * inPt, FRect * inRect)
{
	return inPt->x >= inRect->left  &&  inPt->x < inRect->right
		 && inPt->y >= inRect->top  &&  inPt->y < inRect->bottom;
}


/* -----------------------------------------------------------------------------
	Calculate the intersection of two FRects.
	Args:		ioutRect					the intersection
				inRect1, inRect2		the FRects
	Return:	YES => the resulting FRect is non-empty
----------------------------------------------------------------------------- */

bool
SectRectangle(FRect * outRect, FRect * inRect1, FRect * inRect2)
{
	float l = inRect1->left;
	float t = inRect1->top;
	float r = inRect1->right;
	float b = inRect1->bottom;
	if (t < b)
	{
		if (inRect2->top > t)
			t = inRect2->top;
		if (inRect2->left > l)
			l = inRect2->left;
		if (inRect2->bottom < b)
			b = inRect2->bottom;
		if (inRect2->right < r)
			r = inRect2->right;
		if (t < b  && l < r)
		{
			SetRectangleEdges(outRect, l, t, r, b);
			return YES;
		}
	}
	SetRectangleEmpty(outRect);
	return NO;
}


/* -----------------------------------------------------------------------------
	Determine the centre of an FRect.
	Args:		inRect		the FRect
				outPt			the centre
	Return:	--
----------------------------------------------------------------------------- */

void
RectangleCenter(FRect * inRect, FPoint * outPt)
{
	outPt->x = (inRect->left + inRect->right) / 2.0;
	outPt->y = (inRect->top + inRect->bottom) / 2.0;
}


/* -----------------------------------------------------------------------------
	Determine the midpoint of two points.
	Args:		inPt1, inPt2		two points in space
				outMidPt				the point midway between those points
	Return:	--
----------------------------------------------------------------------------- */

void
GetMidPoint(FPoint * inPt1, FPoint * inPt2, FPoint * outMidPt)
{
	outMidPt->x = (inPt1->x + inPt2->x) / 2.0;
	outMidPt->y = (inPt1->y + inPt2->y) / 2.0;
}


/* -----------------------------------------------------------------------------
	Calculate the union of two FRects.
	Args:		inRect		source FRect
				ioBox			target FRect
				inDoReplace	YES => replace the target instead of creating a union
	Return:	--
----------------------------------------------------------------------------- */

void
AddRect(FRect * inRect, FRect * ioBox, bool inDoReplace)
{
	if (inDoReplace)
		*ioBox = *inRect;
	else
	{
		if (inRect->top < ioBox->top)
			ioBox->top = inRect->top;
		if (inRect->bottom > ioBox->bottom)
			ioBox->bottom = inRect->bottom;
		if (inRect->left < ioBox->left)
			ioBox->left = inRect->left;
		if (inRect->right > ioBox->right)
			ioBox->right = inRect->right;
	}
}


/* -----------------------------------------------------------------------------
	Calculate the union of a FPoint and an FRect.
	Args:		inPt			source FPoint
				ioRect		target FRect
				inDoReset	YES => replace the target instead of creating a union
	Return:	--
----------------------------------------------------------------------------- */

void
AddPtToRect(FPoint * inPt, FRect * ioRect, bool inReset)
{
	if (inReset)
		SetRectanglePoint(ioRect, *inPt);
	else
	{
		if (inPt->y < ioRect->top)
			ioRect->top = inPt->y;
		else if (inPt->y > ioRect->bottom)
			ioRect->bottom = inPt->y;
		if (inPt->x < ioRect->left)
			ioRect->left = inPt->x;
		else if (inPt->x > ioRect->right)
			ioRect->right = inPt->x;
	}
}


#pragma mark Rectangle Mapping
/* -----------------------------------------------------------------------------
	Scale an FRect to match the proportions of another.
	Args:		inSrcBox		source FRect
				ioDstBox		target FRect
	Return:	ioDstBox
----------------------------------------------------------------------------- */

FRect *
GetMapper(FRect * inSrcBox, FRect * ioDstBox)
{
	// ratio of src height / width
	float srcRatio = (inSrcBox->bottom - inSrcBox->top) / (inSrcBox->right - inSrcBox->left);
	// ratio of dst height / width
	float dstHt = ioDstBox->bottom - ioDstBox->top;
	float dstWd = ioDstBox->right - ioDstBox->left;
	float dstRatio = dstHt / dstWd;
	if (srcRatio >= dstRatio)
	{
		// src is taller; dst is wider; inset the width
		float dx = (dstWd - dstHt / dstRatio) / 2.0;
		ioDstBox->left += dx;
		ioDstBox->right -= dx;
	}
	else
	{
		// src is wider; dst is taller; inset the height
		float dy = (dstHt - dstWd * dstRatio) / 2.0;
		ioDstBox->top += dy;
		ioDstBox->bottom -= dy;
	}
	return ioDstBox;
}


/* -----------------------------------------------------------------------------
	Map an FPoint in a source FRect into a target FRect.
	Args:		ioPt				the FPoint to map
				inSrcBox			source FRect
				inDstBox			target FRect
	Return:	--
----------------------------------------------------------------------------- */

void
MapPoint(FPoint * ioPt, FRect * inSrcBox, FRect * inDstBox)
{
	float srcWd = inSrcBox->right - inSrcBox->left;
	float dstWd = inDstBox->right - inDstBox->left;
	float dx = ioPt->x - inSrcBox->left;
	if (srcWd != dstWd)
		dx = dx * dstWd / srcWd;
	ioPt->x = inDstBox->left + dx;

	float srcHt = inSrcBox->bottom - inSrcBox->top;
	float dstHt = inDstBox->bottom - inDstBox->top;
	float dy = ioPt->y - inSrcBox->top;
	if (srcHt != dstHt)
		dy = dy * dstHt / srcHt;
	ioPt->y = inDstBox->top + dy;
}


#pragma mark Angles - Degrees
/* -----------------------------------------------------------------------------
	Bring an angle into the range -180 .. +180 degrees.
	Args:		inDegrees			the angle
	Return:	the mapped angle
----------------------------------------------------------------------------- */

float
MapDegrees(float inDegrees)
{
	while (inDegrees > 180.0)
		inDegrees -= 360.0;
	while (inDegrees < -180.0)
		inDegrees += 360.0;
	return inDegrees;
}


/* -----------------------------------------------------------------------------
	Calculate the difference between two angles.
	Args:		inAngle1, inAngle2			the angles in degrees
	Return:	the delta angle in degrees
----------------------------------------------------------------------------- */

float
DeltaAngle(float inAngle1, float inAngle2)
{
	return MapDegrees(inAngle2 - inAngle1);
}


/* -----------------------------------------------------------------------------
	Calculate the sum of two angles.
	Args:		inAngle1, inAngle2			the angles in degrees
	Return:	the angle in degrees
----------------------------------------------------------------------------- */

float
AddAngle(float inAngle1, float inAngle2)
{
	return MapDegrees(inAngle1 + inAngle2);
}


/* -----------------------------------------------------------------------------
	Calculate the mid angle of two angles.
	Args:		inAngle1, inAngle2			the angles in degrees
	Return:	the mid angle in degrees
----------------------------------------------------------------------------- */

float
MidAngle(float inAngle1, float inAngle2)
{
	return AddAngle(inAngle1, DeltaAngle(inAngle1, inAngle2) / 2);
}


/* -----------------------------------------------------------------------------
	Calculate the angle between two FPoints.
	Args:		inPt1, inPt2			the points
	Return:	the angle in degrees
----------------------------------------------------------------------------- */
float	PtsToAngle(FPoint * inPt1, FPoint * inPt2, float inRounding);

float
GetSlope(FPoint * inPt1, FPoint * inPt2)
{
	return PtsToAngle(inPt1, inPt2, 1.0);
}


// use this instead -- it’s better named
float
PtsToAngle(FPoint * inPt1, FPoint * inPt2)
{
	float dx = inPt1->x - inPt2->x;
	float dy = inPt1->y - inPt2->y;
	return atan2f(dy, dx) * 180.0 / M_PI;
}


/* -----------------------------------------------------------------------------
	Calculate the angle between two FPoints.
	Args:		inPt1, inPt2			the points
				inRounding
	Return:	the angle in degrees
----------------------------------------------------------------------------- */
float	RoundBy(float inValue, float inRounding);

float
PtsToAngle(FPoint * inPt1, FPoint * inPt2, float inRounding)
{
	if (inPt1 && inPt2)
	{
		float angle;
#if defined(correct)
		if (inPt1->x < inPt2->x)
			angle = AngleFromSlope((inPt1->x - inPt2->x) / (inPt1->y - inPt2->y));
		else if (inPt1->x > inPt2->x)
			angle = -AngleFromSlope((inPt2->x - inPt1->x) / (inPt1->y - inPt2->y));
		else if (inPt1->y >= inPt2->y)
			angle = 0.0;
		else
			angle = 180.0;
#else
		angle = atan2f(inPt1->x - inPt2->x, inPt1->y - inPt2->y) * 180.0 / M_PI;
#endif
		angle = MapDegrees(angle);
		return RoundBy(angle, inRounding);
	}
	return 0.0;
}


/* -----------------------------------------------------------------------------
	Round a float number.
	Args:		inValue			the value
				inRounding
	Return:	the rounded number
----------------------------------------------------------------------------- */

float
RoundBy(float inValue, float inRounding)
{
	long iv = ((inValue + inRounding/2) - 1) / inRounding;
	return (float)iv * inRounding;
}


/* -----------------------------------------------------------------------------
	Convert slope (dx/dy) to angle.
	Args:		inSlope			the slope
	Return:	the angle in degrees
----------------------------------------------------------------------------- */
#if defined(correct)
int	// wanna make this float, I think
AngleFromSlope(float inSlope)
{
	bool isNegative = (inSlope < 0.0);
	if (isNegative)
		inSlope = -inSlope;
	int r12 = inSlope;
	int r2 = fract inSlope;
	if (r12 == 0)
	{
		r12 = qdConstants.x1C2[r2>>10];
		if (qdConstants.x204[r12] < r2)
			r12++;
	}
	else
	{
		 if (r12 >= 8)
			r12 = 83;
		else
			r12 = qdConstants.x2B4[inSlope/8];
		while (qdConstants.x240[r12] < inSlope)
			r12++;
	}

	return isNegative ? 180.0 - r12 : r12;
}
#endif

#pragma mark Radians
/* -----------------------------------------------------------------------------
	Convert degrees to radians.
	Args:		inAngle			the angle in degrees
	Return:	the angle in radians
----------------------------------------------------------------------------- */

float
DegToRad(float inAngle)
{
	return MapDegrees(inAngle) * M_PI / 180.0;
}


/* -----------------------------------------------------------------------------
	Calculate the angle between two FPoints.
	Args:		inPt1, inPt2			the points
	Return:	the angle in radians
----------------------------------------------------------------------------- */

float
PtsToAngleR(FPoint * inPt1, FPoint * inPt2)
{
	float dx = inPt1->x - inPt2->x;
	float dy = inPt1->y - inPt2->y;

#if defined(correct)
	if (dx == 0)
	{
		// it’s a vertical line segment
		if (dy != 0)
		{
			if (dy > 0)
				return 0.0;			// up
			else
				return 4.858413;	// down
		}
	}
	else
	{
		float angle = atanf(dy);
		if (angle > -4.0 && angle < 4.0)
			return angle;
	}
	return 0.0;

#else
	return atan2f(dy, dx);
#endif
}


/* -----------------------------------------------------------------------------
	Bring an angle into the range -π .. +π degrees.
	Args:		ioRadians			the angle
	Return:	--
----------------------------------------------------------------------------- */

void
NORM(float * ioRadians)
{
	if (*ioRadians > M_PI)
	{
		do {
			*ioRadians -= 2*M_PI;
		} while (*ioRadians > M_PI);
	}
	else if (*ioRadians <= -M_PI)
	{
		do {
			*ioRadians += 2*M_PI;
		} while (*ioRadians <= -M_PI);
	}
}
