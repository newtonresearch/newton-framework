/*
	File:		QDDrawing.h

	Contains:	QuickDraw interface to drawing functions.
					Declares Rect, RoundRect, etc Frame/Paint Stroke/Fill functions.
					(Stroke rather than Frame, Fill rather than Paint. More descriptive.)
					QDDrawing.cc will implement these using CG.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__QDDRAWING_H)
#define __QDDRAWING_H 1

#include "QDGeometry.h"


typedef void (*OffsetFunc)(ShapePtr, short, short);
typedef void (*StrokeFunc)(ShapePtr);
typedef void (*FillFunc)(ShapePtr);


// don’t think we need these, but they’re frequently used in the original
extern void		StartDrawing(PixelMap * inPixmap, Rect * inBounds);
extern void		StopDrawing(PixelMap * inPixmap, Rect * inBounds);

// might need this while you’re drawing
extern void		BusyBoxSend(int inCmd);


extern void		LineNormal(void);

extern void		SetLineWidth(short inWidth);
extern short	LineWidth(void);

extern void		StrokeLine(Point inFrom, Point inTo);

extern void		StrokeRect(Rect inBox);
extern void		FillRect(Rect inBox);

extern void		StrokeRect(const Rect * inBox);
extern void		FillRect(const Rect * inBox);

extern void		StrokeRoundRect(Rect inBox, short inOvalWd, short inOvalHt);
extern void		FillRoundRect(Rect inBox, short inOvalWd, short inOvalHt);

extern void		StrokeOval(const Rect * inBox);
extern void		FillOval(const Rect * inBox);

extern void		KillPoly(PolygonShape * inShape);
extern void		OffsetPoly(PolygonShape * inShape, short dx, short dy);
extern void		StrokePoly(PolygonShape * inShape);
extern void		FillPoly(PolygonShape * inShape);

extern void		OffsetRgn(RegionShape * inShape, short dx, short dy);
extern void		StrokeRgn(RegionShape * inShape);
extern void		FillRgn(RegionShape * inShape);

extern void		StrokeArc(WedgeShape * inShape, short inArc1, short inArc2);
extern void		FillArc(WedgeShape * inShape, short inArc1, short inArc2);

extern Ptr		GetPixelMapBits(PixelMap * pixmap);


#endif	/* __QDDRAWING_H */
