/*
	File:		PictureView.cc

	Contains:	CPictureView implementation.

	Written by:	Newton Research Group.
*/

#include "PictureView.h"

#include "ROMResources.h"
#include "Strings.h"
#include "Iterators.h"
#include "DrawShape.h"


/*------------------------------------------------------------------------------
	C P i c t u r e V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clPictureView, CPictureView, CView)


/*------------------------------------------------------------------------------
	Return the click options this view supports.
	Args:		--
	Return:	ULong
------------------------------------------------------------------------------*/

ULong
CPictureView::clickOptions(void)
{
	return 0x03;
}


/*------------------------------------------------------------------------------
	Highlight the picture.
	If the picture has a mask then use that.
	Args:		inHilite		hilite - true or false
	Return:	--
------------------------------------------------------------------------------*/

void
CPictureView::hilite(bool inHilite)
{
	RefVar icon(getValue(SYMA(icon), RA(NILREF)));

	if (NOTNIL(icon) && IsFrame(icon) && FrameHasSlot(icon, SYMA(mask)) && visibleDeep())
	{
//		CRegion		vr = setupVisRgn();
//		CRegionVar	viewVisRgn(vr);
		unwind_protect
		{
			ULong justify = viewJustify & vjEverything;
			if (justify == 0 && ISNIL(getCacheProto(kIndexViewJustify))) {
				justify = vjCenterH | vjCenterV;
			}
			DrawPicture(icon, &viewBounds, justify, -modeXor);	// XOR the mask
		}
		on_unwind
		{
//			GrafPtr  thePort;
//			GetPort(&thePort);
//			CopyRgn(viewVisRgn, thePort->visRgn);
		}
		end_unwind;
	}

	else
		CView::hilite(inHilite);
}


/*------------------------------------------------------------------------------
	Draw the pictureÕs highlights?
	Args:		inHilite		hilite - true or false
	Return:	--
------------------------------------------------------------------------------*/

void
CPictureView::drawHilites(bool inHilite)
{
	if (inHilite == false)
		CView::hilite(true);
}


/*------------------------------------------------------------------------------
	Draw the picture.  Really.
	Args:		inRect	the rect in which to draw
	Return:	--
------------------------------------------------------------------------------*/

void
CPictureView::realDraw(Rect& inRect)
{
//	Ignore the given Rect and use our viewBounds instead
	drawUsingRect(&viewBounds);
}


/*------------------------------------------------------------------------------
	Draw the picture into a rect.
	Args:		inRect	the rect in which to draw
	Return:	--
------------------------------------------------------------------------------*/

void
CPictureView::drawUsingRect(const Rect * inRect)
{
	RefVar icon(getValue(SYMA(icon), RA(NILREF)));
	if (NOTNIL(icon))
	{
		ULong justify = viewJustify & vjEverything;
		if (justify == 0 && ISNIL(getCacheProto(kIndexViewJustify))) {
			justify = vjCenterH | vjCenterV;
		}
		RefVar transferMode(getProto(SYMA(viewTransferMode)));
		int mode = ISNIL(transferMode) ? modeCopy : RVALUE(transferMode);
		DrawPicture(icon, inRect, justify, mode);
	}
}


/*------------------------------------------------------------------------------
	Draw the picture and scale up or down to fit the specified rect.
	Args:		inRect1	the rect in which to draw
				inRect2
				outRect3
	Return:	--
------------------------------------------------------------------------------*/

void
CPictureView::drawScaledData(const Rect * inSrcBounds, const Rect * inDstBounds, Rect * ioBounds)
{
	CView::drawScaledData(inSrcBounds, inDstBounds, ioBounds);
	drawUsingRect(ioBounds);
}


/*------------------------------------------------------------------------------
	Add picture drag info.
	Args:		inDragInfo
	Return:	--
------------------------------------------------------------------------------*/

bool
CPictureView::addDragInfo(CDragInfo * outDragInfo)
{
//	if (!CView::addDragInfo(outDragInfo))
//		outDragInfo->addDragItem(SYMA(picture), fContext, STRRdrawing);
	return true;
}


/*------------------------------------------------------------------------------
	Return picture drop data.
	Args:		inArg1
				inArg2
	Return:	--
------------------------------------------------------------------------------*/

Ref
CPictureView::getDropData(RefArg inArg1, RefArg inArg2)
{
	RefVar	dropData(CView::getDropData(inArg1, inArg2));
	if (ISNIL(dropData))
	{
		dropData = AllocateFrame();
		Rect  bounds = viewBounds;
		Point origin;
		origin.h = -bounds.left;
		origin.v = -bounds.top;
		OffsetRect(&bounds, origin.h, origin.v);
		SetFrameSlot(dropData, SYMA(viewBounds), ToObject(&bounds));
		SetFrameSlot(dropData, SYMA(icon), DeepClone(getProto(SYMA(icon))));
	}
	return dropData;
}

