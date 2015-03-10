/*
	File:		InkGeometry.h

	Contains:	Geometry functions for the recognition subsystem.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__RECGEOMETRY_H)
#define __RECGEOMETRY_H 1

#include "InkTypes.h"


void		FixRect(FRect * outRect, Rect * inRect);
void		UnfixRect(FRect * inRect, Rect * outRect);

inline float RectangleWidth(FRect * inRect)  { return inRect->right - inRect->left; }
inline float RectangleHeight(FRect * inRect) { return inRect->bottom - inRect->top; }

void		SetRectangleEmpty(FRect * ioRect);
void		SetRectanglePoint(FRect * ioRect, FPoint inPt);
bool		EmptyRectangle(FRect * inRect);
void		InsetRectangle(FRect * ioRect, float dx, float dy);
bool		PointInRectangle(FPoint * inPt, FRect * inRect);
bool		SectRectangle(FRect * outRect, FRect * inRect1, FRect * inRect2);
void		RectangleCenter(FRect * inRect, FPoint * outPt);
void		GetMidPoint(FPoint * inPt1, FPoint * inPt2, FPoint * outMidPt);
void		AddPtToRect(FPoint * inPt, FRect * ioRect, bool inReset);
void		AddRect(FRect * inRect, FRect * ioBox, bool inDoReplace);

FRect *	GetMapper(FRect * inSrcBox, FRect * ioDstBox);
void		MapPoint(FPoint * ioPt, FRect * inSrcBox, FRect * inDstBox);

float		DegToRad(float inDegrees);
float		MapDegrees(float inDegrees);
float		DeltaAngle(float inAngle1, float inAngle2);
float		AddAngle(float inAngle1, float inAngle2);
float		MidAngle(float inAngle1, float inAngle2);
float		PtsToAngle(FPoint * inPt1, FPoint * inPt2);
//float		GetSlope(FPoint * inPt1, FPoint * inPt2);
//float		AngleFromSlope(float inSlope);
//float		PtsToAngle(FPoint * inPt1, FPoint * inPt2, float inRounding);
//float		RoundBy(float inValue, float inRounding);

float		PtsToAngleR(FPoint * inPt1, FPoint * inPt2);
void		NORM(float * ioRadians);


#endif	/* __RECGEOMETRY_H */
