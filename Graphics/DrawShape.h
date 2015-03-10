/*
	File:		DrawShape.h

	Contains:	Shape drawing declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__DRAWSHAPE_H)
#define __DRAWSHAPE_H 1

#include "QDDrawing.h"


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern bool		FromObject(RefArg inObj, Rect * outBounds);
extern Ref		ToObject(const Rect * inBounds);

extern Rect *  Justify(Rect * ioRect, const Rect * inFrame, ULong inJustify);

extern Rect *	ShapeBounds(RefArg inShape, Rect * outBounds);
extern void		DrawShape(RefArg inShape, RefArg inStyle, Point inOffset);

extern void		DrawPicture(RefArg inIcon, const Rect * inFrame, ULong inJustify);
extern void		DrawBitmap(RefArg inBitmap, const Rect * inFrame);

#endif	/* __DRAWSHAPE_H */
