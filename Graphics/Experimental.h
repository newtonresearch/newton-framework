/*
	File:		Experimental.h

	Contains:	Thoughts for organising Newton graphics functions.
					The main problem is Newton’s QuickDraw legacy which we don’t really want to propagate.
					However, if we are to use view data extracted from the ROM we’ll need to use Rects etc,
					short sized ordinates, and a drawing origin of top-left.
					There should be a layer that transforms these values to Core Graphics / Quartz.
					(With a view possibly to OpenGL?)
					So we’ll keep the old QD geometry but implement it ourselves.
					We definitely want to ditch the old headers that deprecate QD.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__EXPERIMENTAL_H)
#define __EXPERIMENTAL_H 1

/*------------------------------------------------------------------------------
	All graphics functions will need standard NewtonTypes.h
	But these should no longer include QD types.
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	QDTypes.h
	New header.
	Declare Point, Rect, etc here and not in NewtonTypes.
	Declare in C++ style; means all sources will need to be C++ even if they only use plain C.
------------------------------------------------------------------------------*/
#include "NewtonTypes.h"

// Font/Text structures elsewhere -- QDText.h?
// Patterns/colours elsewhere -- QDPatterns.h?


/*------------------------------------------------------------------------------
	Paths.h
	New header.
	Declare paths -- as yet undiscovered usage.
------------------------------------------------------------------------------*/
#include "QDTypes.h"


/*------------------------------------------------------------------------------
	QDGeometry.h
	Declares Point, Rect manipulators/accessors.
------------------------------------------------------------------------------*/
#include "QDTypes.h"


/*------------------------------------------------------------------------------
	QDDrawing.h
	Declares Rect, RoundRect, etc Frame/Paint Stroke/Fill functions.
	Say Stroke rather than Frame, Fill rather than Paint. More descriptive.
	QDDrawing.cc will implement these using CG.
------------------------------------------------------------------------------*/
#include "QDTypes.h"


#endif	/* __EXPERIMENTAL_H */
