/*
	File:		TextView.cc

	Contains:	CTextView implementation.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "Lookup.h"
#include "RichStrings.h"
#include "Unicode.h"
#include "MagicPointers.h"
#include "Preference.h"

#include "TextView.h"
#include "Geometry.h"
#include "DrawText.h"

extern void		GetStyleFontInfo(StyleRecord * inStyle, FontInfo * outFontInfo);


/*------------------------------------------------------------------------------
	C T e x t V i e w

	A view containing immutable text of one fixed style.
------------------------------------------------------------------------------*/
VIEW_SOURCE_MACRO(clTextView, CTextView, CView)


/*--------------------------------------------------------------------------------
	Initialize.
	Args:		inProto		the context frame for this view
				inView		the parent view
	Return:	--
--------------------------------------------------------------------------------*/

void
CTextView::init(RefArg inProto, CView * inView)
{
	CView::init(inProto, inView);

//	RefVar	mode(getProto(SYMA(viewTransferMode)));
//	fTextTransferMode = NOTNIL(mode) ? RINT(mode) : srcOr;
}


/*------------------------------------------------------------------------------
	Draw the text.  Really.
	Args:		inRect	the rect in which to draw
	Return:	--
------------------------------------------------------------------------------*/

void
CTextView::realDraw(Rect& inRect)
{
	RefVar	text(getVar(SYMA(text)));
	if (NOTNIL(text))
	{
		CRichString richStr(text);
		if (richStr.length() > 0)
		{
			ULong		vwJustify = viewJustify & vjEverything;
			if (vwJustify & vjOneLineOnly)
			{
				StyleRecord style;
				CreateTextStyleRecord(getVar(SYMA(viewFont)), &style);

				TextOptions options;
				options.alignment = ConvertToFlush(vwJustify, &options.justification);
				options.width = RectGetWidth(viewBounds);
			//	options.transferMode = fTextTransferMode;

				FontInfo		fntInfo;
				GetStyleFontInfo(&style, &fntInfo);

				FPoint		location;
				location.x = viewBounds.left;
				location.y = viewBounds.top + fntInfo.ascent - 1;
				int  leading = RectGetHeight(viewBounds) - (fntInfo.ascent + fntInfo.descent);

				switch (viewJustify & vjVMask)
				{
				case vjTopV:
					{
						Ref	viewLineSpacing = getProto(SYMA(viewLineSpacing));
						if (NOTNIL(viewLineSpacing))
							location.y = viewBounds.top + RINT(viewLineSpacing);
					}
					break;
				case vjCenterV:
					location.y += leading / 2 + 1;
					break;
				case vjBottomV:
					location.y += leading + 1;
					break;
			//	case: vjFullV:
			//		break;
				}
				DrawRichString(richStr, 0, richStr.length(), &style, location, &options, NULL);
			}
			else
				TextBox(richStr, getVar(SYMA(viewFont)), &viewBounds, vwJustify & vjHMask, vwJustify & vjVMask /*,fTextTransferMode*/);
		}
	}
}
