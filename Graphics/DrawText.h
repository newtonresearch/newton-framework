/*
	File:		DrawText.h

	Contains:	Text style declarations.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__DRAWTEXT_H)
#define __DRAWTEXT_H 1

#include "InkTypes.h"
#include "TextStyles.h"
#include "RichStrings.h"


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern void		TextBox(CRichString & inStr, RefArg inFont, const Rect * inRect, ULong inJustifyH, ULong inJustifyV /*, long inTransferMode*/);
extern void		DrawRichString(CRichString & inStr, ULong inStart, size_t inLength, StyleRecord * inStyle, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * inBoundsInfo);
extern void		DrawTextOnce(void * inText, size_t inLength, StyleRecord ** inStyles, short * inRuns, FPoint inPt, TextOptions * inOptions, TextBoundsInfo * inBoundsInfo);


#endif	/* __DRAWTEXT_H */
