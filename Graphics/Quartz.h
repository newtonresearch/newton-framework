/*
	File:		Quartz.h

	Contains:	Quartz core graphics interface for Newton.

	Written by:	Newton Research Group.

	In the Old World they used QuickDraw.
	Drawing is triggered by a call to gRootView->update()
	The call tree:
		CRootView::update(Rect *)		arg specifies area to update, usually NULL => entire display
			CView::update
				CView::draw
					CView::preDraw			grid/line pattern
												if vClipping, close in GrafPort’s clipping region
					CView::realDraw		subclass: view content
					viewDrawScript			NewtonScript drawing content
					CView::drawChildren
						CView::draw			recurse down the view hierarchy
												if vClipping, restore GrafPort’s clipping region
					CView::postDraw		view frame, default button if keyboard connected; frame is drawn OUTSIDE the view bounds so must not be clipped

	Drawing may be clipped to views, but all drawing ends up in the global QuickDraw PixMap representing the display.
	A task, executed every 30ms, blits this PixMap to the hardware.
	The call tree:
		CNewtWorld::mainConstructor
			InitGraf
				InitScreen
					InitScreenTask			creates ScreenUpdateTask
		ScreenUpdateTask					calls itself back in 30ms
			UpdateHardwareScreen
				BlitToScreens				iff dirtyBounds is on screen; StopDrawing() updates these dirtyBounds

	In the New World we use Quartz (CoreGraphics).
	Origin is at bottom-left, not top-left. Coordinates are CGFloat, not short.
		=>	We maintain Point, Rect from the QD world, but all drawing must make this adjustment.
			Encapsulating the adjustment in calls such as StrokeRect(Rect) makes the transition easier.
	Drawing is triggered by gRootView->update() as before.
		Since we’re drawing into an opaque CGContextRef and not a PixMap we can’t do anything tricky with existing pixels.
			No invert => we redraw with an invert style
		We now clip to a path, not a Region => when clipping to a view the path is the view’s frame.
		Patterns in the QD world were in fact used as colours. We now use CGColorRef.
		Text..!
	We don’t need the ScreenUpdateTask.
*/

#if !defined(__QUARTZ_H)
#define __QUARTZ_H 1

#include "QDTypes.h"

#include <CoreGraphics/CoreGraphics.h>


extern CGContextRef	quartz;

extern int				gScreenWidth;
extern int				gScreenHeight;

extern CGColorRef		gBlackColor;
extern CGColorRef		gDarkGrayColor;
extern CGColorRef		gGrayColor;
extern CGColorRef		gLightGrayColor;
extern CGColorRef		gWhiteColor;


extern CGPoint			MakeCGPoint(Point inPoint);
extern CGRect			MakeCGRect(Rect inRect);
extern CGRect			MakeCGRectFrame(Rect inRect, float inLineWd);


#endif	/* __QUARTZ_H */
