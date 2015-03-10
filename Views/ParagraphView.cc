/*
	File:		ParagraphView.cc

	Contains:	CParagraphView implementation.

	Written by:	Newton Research Group.
*/

#include "ParagraphView.h"

#include "Globals.h"


/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C"
{
Ref	FExtractTextRange(RefArg inRcvr, RefArg inRangeStart, RefArg inRangeEnd);
Ref	FExtractRangeAsRichString(RefArg inRcvr, RefArg inRangeStart, RefArg inRangeEnd);
Ref	FGetStyleAtOffset(RefArg inRcvr, RefArg inOffset);
Ref	FGetStylesOfRange(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref	FChangeStylesOfRange(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);

Ref	FPointToCharOffset(RefArg inRcvr, RefArg inX, RefArg inY);
Ref	FPointToWord(RefArg inRcvr, RefArg inX, RefArg inY);

Ref	FVoteOnWordUnit(RefArg inRcvr, RefArg inUnit);
}


/*--------------------------------------------------------------------------------
	Return the CParagraphView associated with a NewtonScript view.
	Throw an exception if itÕs not found.
	Args:		inContext	the NS view frame
	Return:	CParagraphView *
--------------------------------------------------------------------------------*/

CParagraphView *
FailGetParagraphView(RefArg inContext)
{
	CParagraphView *  view = (CParagraphView *)GetView(inContext);
	if (!view->derivedFrom(clParagraphView))
		ThrowMsg("not a paragraph view");
	return view;
}


Ref
FExtractTextRange(RefArg inRcvr, RefArg inRangeStart, RefArg inRangeEnd)
{
	CParagraphView * view = FailGetParagraphView(inRcvr);
	return view->extractTextRange(RINT(inRangeStart), RINT(inRangeEnd));
}

Ref
FExtractRangeAsRichString(RefArg inRcvr, RefArg inRangeStart, RefArg inRangeEnd)
{
	CParagraphView * view = FailGetParagraphView(inRcvr);
	return view->extractRangeAsRichString(RINT(inRangeStart), RINT(inRangeEnd));
}

Ref
FGetStyleAtOffset(RefArg inRcvr, RefArg inOffset)
{
	CParagraphView * view = FailGetParagraphView(inRcvr);
	return view->getStyleAtOffset(RINT(inOffset), NULL, NULL);
}

Ref
FGetStylesOfRange(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3)
{
	CView * view = FailGetView(inRcvr);
	//INCOMPLETE
	return NILREF;
}

Ref
FChangeStylesOfRange(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3)
{
	CView * view = FailGetView(inRcvr);
	//INCOMPLETE
	return NILREF;
}

Ref	FPointToCharOffset(RefArg inRcvr, RefArg inX, RefArg inY) {}
Ref	FPointToWord(RefArg inRcvr, RefArg inX, RefArg inY) {}
Ref	FVoteOnWordUnit(RefArg inRcvr, RefArg inUnit) {}


#pragma mark CParagraphView
/*------------------------------------------------------------------------------
	C P a r a g r a p h V i e w

	A view containing mutable text of different styles.
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clParagraphView, CParagraphView, CView)

CParagraphView::~CParagraphView()
{}


/*--------------------------------------------------------------------------------
	Initialize.
	Args:		inProto		the context frame for this view
				inView		the parent view
	Return:	--
--------------------------------------------------------------------------------*/

void
CParagraphView::init(RefArg inProto, CView * inView)
{
	f30 = -1;
	CView::init(inProto, inView);
}


void
CParagraphView::realDraw(Rect * inBounds)
{}


Ref
CParagraphView::extractRangeAsRichString(ArrayIndex inRangeStart, ArrayIndex inRangeEnd) { return NILREF; }

Ref
CParagraphView::extractTextRange(ArrayIndex inRangeStart, ArrayIndex inRangeEnd) { return NILREF; }

Ref
CParagraphView::getStyleAtOffset(ArrayIndex inOffset, long*, long*) { return NILREF; }

void
CParagraphView::flushWordAtCaret(void) {}

