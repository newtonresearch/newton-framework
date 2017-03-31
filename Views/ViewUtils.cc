/*
	File:		ViewUtils.cc

	Contains:	View utilities.

	Written by:	Newton Research Group.
*/

#include "DrawShape.h"
#include "DrawText.h"
#include "Geometry.h"

#include "ObjHeader.h"
#include "Objects.h"
#include "Globals.h"
#include "ROMResources.h"
#include "Lookup.h"
#include "Funcs.h"
#include "Arrays.h"
#include "ListLoop.h"
#include "Strings.h"
#include "UStringUtils.h"
#include "MagicPointers.h"
#include "NewtonScript.h"

#include "RootView.h"
#include "Modal.h"
#include "ErrorNotify.h"
#include "Notebook.h"
#include "Clipboard.h"
#include "PictureView.h"
#include "KeyboardView.h"
#include "TextView.h"
#include "ParagraphView.h"
#include "PickView.h"
#include "GaugeView.h"
#include "ViewStubs.h"

#include "Sound.h"
#include "Stroke.h"
extern CStroke *	StrokeFromRef(RefArg inRef);


/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C"
{
Ref	FBuildRecConfig(RefArg inRcvr);
Ref	FExtractData(RefArg inRcvr, RefArg inViews, RefArg inSepStr, RefArg inPrintLength);
Ref	FGetView(RefArg inRcvr, RefArg inView);
Ref	FGetId(RefArg inRcvr, RefArg inView);
Ref	FGetFlags(RefArg inRcvr, RefArg inView);
Ref	FGetTextFlags(RefArg inRcvr, RefArg inView);
Ref	FBuildContext(RefArg inRcvr, RefArg inView);
Ref	FAddView(RefArg inRcvr, RefArg inParentView, RefArg inChildTemplate);
Ref	FAddStepView(RefArg inRcvr, RefArg inParentView, RefArg inChildTemplate);
Ref	FRemoveView(RefArg inRcvr, RefArg inParentView, RefArg inChildView);
Ref	FRemoveStepView(RefArg inRcvr, RefArg inParentView, RefArg inChildView);
Ref	FRefreshViews(RefArg inRcvr);
Ref	FOpenX(RefArg inRcvr);
Ref	FCloseX(RefArg inRcvr);
Ref	FToggleX(RefArg inRcvr);
Ref	FShowX(RefArg inRcvr);
Ref	FHideX(RefArg inRcvr);
Ref	FSyncViewX(RefArg inRcvr);
Ref	FRedoChildrenX(RefArg inRcvr);
Ref	FSyncChildrenX(RefArg inRcvr);
Ref	FSyncScrollX(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref	FLayoutTableX(RefArg inRcvr, RefArg inTableDef, RefArg inColStart, RefArg inRowStart);
Ref	FLayoutVerticallyX(RefArg inRcvr, RefArg inChildViews, RefArg inStart);
Ref	FFormatVertical(RefArg inRcvr, RefArg inArg1, RefArg inArg2);
Ref	FSetupIdleX(RefArg inRcvr, RefArg inFreq);
Ref	FSetValue(RefArg inRcvr, RefArg inView, RefArg inTag, RefArg inValue);
Ref	FSetOriginX(RefArg inRcvr, RefArg inX, RefArg inY);
Ref	FSetBounds(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom);
Ref	FRelBounds(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inWidth, RefArg inHeight);
Ref	FLocalBoxX(RefArg inRcvr);
Ref	FGlobalBoxX(RefArg inRcvr);
Ref	FGlobalOuterBoxX(RefArg inRcvr);
Ref	FVisibleBox(RefArg inRcvr);
Ref	FGetDrawBoxX(RefArg inRcvr);
Ref	FDirtyBoxX(RefArg inRcvr, RefArg inRect);
Ref	FDirtyX(RefArg inRcvr);
Ref	FParentX(RefArg inRcvr);
Ref	FChildViewFramesX(RefArg inRcvr);
Ref	FMoveBehindX(RefArg inRcvr, RefArg inView);
Ref	FHiliteOwner(RefArg inRcvr);
Ref	FGetHiliteOffsets(RefArg inRcvr);
Ref	FDropHilites(RefArg inRcvr);
Ref	FHiliteViewChildren(RefArg inRcvr, RefArg inUnit);
Ref	FSetHiliteX(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref	FSetHiliteNoUpdateX(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref	FHiliteX(RefArg inRcvr, RefArg inDoIt);
Ref	FHiliteUniqueX(RefArg inRcvr, RefArg inDoIt);
Ref	FHiliter(RefArg inRcvr, RefArg inUnit);
Ref	FTrackHiliteX(RefArg inRcvr, RefArg inUnit);
Ref	FTrackButtonX(RefArg inRcvr, RefArg inUnit);
Ref	FOffsetView(RefArg inRcvr, RefArg indx, RefArg indy);

Ref	FDragX(RefArg inRcvr, RefArg inUnit, RefArg inDragBounds);
Ref	FDragAndDrop(RefArg inRcvr, RefArg inUnit, RefArg inBounds, RefArg inLimitBounds, RefArg inCopy, RefArg inDragInfo);
Ref	FDragAndDropLtd(RefArg inRcvr, RefArg inUnit, RefArg inBounds, RefArg inLimitBounds, RefArg inCopy, RefArg inDragInfo);

Ref	FSetClipboard(RefArg inRcvr, RefArg inClipbd);
Ref	FGetClipboard(RefArg inRcvr);
Ref	FGetClipboardIcon(RefArg inRcvr);
Ref	FClipboardCommand(RefArg inRcvr, RefArg inCmd);

Ref	FDoDrawing(RefArg inRcvr, RefArg inMsg, RefArg inArgs);
Ref	FInvertRect(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom);

Ref	FGetRemoteWriting(RefArg inRcvr);
Ref	FSetRemoteWriting(RefArg inRcvr, RefArg inArg);
Ref	FGetCaretInfo(RefArg inRcvr);
Ref	FSetCaretInfo(RefArg inRcvr, RefArg inArg1, RefArg inArg2);
Ref	FPositionCaret(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref	FCaretRelativeToVisibleRect(RefArg inRcvr, RefArg inArg);
Ref	FShowCaret(RefArg inRcvr);
Ref	FHideCaret(RefArg inRcvr);
Ref	FInsertItemsAtCaret(RefArg inRcvr, RefArg inArg);
Ref	FViewContainsCaretView(RefArg inRcvr, RefArg inArg);
Ref	FGetInkWordInfo(RefArg inRcvr, RefArg inArg);

Ref	FGetAlternates(RefArg inRcvr, RefArg inArg1, RefArg inArg2);

Ref	FHandleInsertItems(RefArg inRcvr, RefArg inItems);
Ref	FHandleInkWord(RefArg inRcvr, RefArg inWord);
Ref	FHandleRawInk(RefArg inRcvr, RefArg inInk);
}

/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/
extern Ref		Stringer(RefArg obj);

bool	RealOpenX(RefArg inRcvr, bool inHuh);


#pragma mark Warning
/*--------------------------------------------------------------------------------
	U t i l i t i e s
--------------------------------------------------------------------------------*/

void
BadWickedNaughtyNoot(int inErr)
{
	NSCallGlobalFn(SYMA(BadWickedNaughtyNoot), MAKEINT(inErr), RA(NILREF));
}


#pragma mark Commands
/*--------------------------------------------------------------------------------
	C o m m a n d s
	
	Commands are messages (frames) sent to CResponders.
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Create a new command frame.
	Args:		inId		the id of the command
				inRcvr	the intended receiver
				inArg		any arg the command requires
	Return:	Ref		the command frame

	SYS_command @433 = 
	{
		id: nil,
		result: 0,
		parameter: nil,
		receiver: nil,
		frameParameter: nil,
		params: nil,
		undo: nil
	}
--------------------------------------------------------------------------------*/

Ref
MakeCommand(ULong inId, CResponder * inRcvr, OpaqueRef inArg)
{
	RefVar	cmd(Clone(RA(protoCommand)));
	RefVar	rcvr;
	if (inRcvr == gApplication)
		rcvr = SYMA(application);
	else
		rcvr = static_cast<CView*>(inRcvr)->fContext;

	SetFrameSlot(cmd, SYMA(id), MAKEINT(inId));
	SetFrameSlot(cmd, SYMA(receiver), rcvr);
	SetFrameSlot(cmd, SYMA(parameter), MAKEINT(inArg));
	return cmd;
}


/*--------------------------------------------------------------------------------
	Extract the receiver from a command frame.
	Args:		inCmd		the command frame
	Return:	CResponder *
--------------------------------------------------------------------------------*/

CResponder *
CommandReceiver(RefArg inCmd)
{
	RefVar	rcvr(GetFrameSlot(inCmd, SYMA(receiver)));
	if (EQ(rcvr, SYMA(application)))
		return gApplication;
	return GetView(rcvr);
}

#pragma mark -

/*--------------------------------------------------------------------------------
	U t i l i t i e s
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Notify the user of an error.
	Args:		inErr			the error number
				inErrLevel	the error level
	Return:	--
--------------------------------------------------------------------------------*/

void
ErrorNotify(NewtonErr inErr, NotifyLevel inErrLevel)
{
	RefVar	args(MakeArray(3));
	SetArraySlotRef(args, 0, MAKEINT(inErrLevel));
	SetArraySlotRef(args, 1, MAKEINT(inErr));
	DoMessage(gRootView->fContext, SYMA(Notify), args);
}


/*--------------------------------------------------------------------------------
	Notify the user of an error.
	Args:		inErr			the error number
				inErrLevel	the error level
	Return:	--
--------------------------------------------------------------------------------*/

void
ActionErrorNotify(NewtonErr inErr, NotifyLevel inErrLevel)
{
	RefVar	args(MakeArray(3));
	SetArraySlotRef(args, 0, MAKEINT(inErrLevel));
	SetArraySlotRef(args, 1, MAKEINT(inErr));
	DoMessage(gRootView->fContext, SYMA(ActionNotify), args);
}


/*--------------------------------------------------------------------------------
	Return the CView associated with a NewtonScript view.
	Args:		inContext	NS view frame
				inView		specific view within context
	Return:	CView *
--------------------------------------------------------------------------------*/

CView *
GetFrontCommandKeyView(void)
{
	CView *  view;
	for (ArrayIndex i = gRootView->viewChildren->count(); i > 0; i--)
	{
		view = (CView *)gRootView->viewChildren->at(i);
		if ((view->viewFlags & vVisible)
		&&  (view->textFlags() & 0x4000))
			return view;
	}
	return NULL;
}


CView *
GetKeyReceiver(RefArg inContext, RefArg inView)
{
	CView *  view = GetView(inContext, inView);
	if (view == NULL)
		view = gRootView->caretView();
	return view;
}


CView *
GetView(RefArg inContext, RefArg inView)
{
	CView *  view = NULL;
	if (gRootView)
	{
		if (IsSymbol(inView))
		{
			if (EQ(inView, SYMA(viewFrontMost)))
				view = gRootView->frontMost();
			else if (EQ(inView, SYMA(viewFrontMostApp)))
				view = gRootView->frontMostApp();
			else if (EQ(inView, SYMA(viewFrontKey)))
			{
				if ((view = gRootView->caretView()) == NULL)
				{
					CView *  keyView;
					if ((keyView = GetFrontCommandKeyView()) != NULL)
					{
						if ((keyView->textFlags() & 0x8000)
						||  ((keyView = gRootView->findRestorableKeyView(keyView, NULL)) != NULL
						 && (keyView->textFlags() & 0x8000) != 0))
							view = keyView;
					}
				}
				if (view == NULL
				&& (view = gRootView->nextKeyView(NULL, 0, 0)) == NULL)
					view = gRootView;
			}
			else if (EQ(inView, SYMA(viewFrontCommandKey)))
			{
				if ((view = gRootView->caretView()) == NULL
				&&  (view = GetFrontCommandKeyView()) == NULL)
					view = gRootView;
			}
		}
		else if (ISNIL(inView))
		{
			RefVar  context;
			if (NOTNIL(inContext))
				context = GetProtoVariable(inContext, SYMA(preallocatedContext));
			if (NOTNIL(context))
				context = GetVariable(inContext, context);
			view = GetView(context);
		}
		else if (FrameHasSlot(inView, SYMA(viewCObject)))
			view = GetView(inView);
		else
			view = gRootView->findView(inView);
	}
	return view;
}


/*--------------------------------------------------------------------------------
	Return the CView associated with a NewtonScript view.
	Args:		inContext	the NS view frame
	Return:	CView *
--------------------------------------------------------------------------------*/

CView *
GetView(RefArg inContext)
{
	if (NOTNIL(inContext))
	{
		RefVar	view(GetVariable(inContext, SYMA(viewCObject)));
		if (NOTNIL(view))
			return (CView *)RefToAddress(view);
	}
	return NULL;
}


/*--------------------------------------------------------------------------------
	Return the CView associated with a NewtonScript view.
	Throw an exception if it’s not found.
	Args:		inContext	the NS view frame
	Return:	CView *
--------------------------------------------------------------------------------*/

CView *
FailGetView(RefArg inContext)
{
	CView *  view = GetView(inContext);
	if (view == NULL)
		ThrowMsg("NULL view");
	return view;
}

CView *
FailGetView(RefArg inContext, RefArg inView)
{
	CView *  view = GetView(inContext, inView);
	if (view == NULL)
		ThrowMsg("NULL view");
	return view;
}


/*--------------------------------------------------------------------------------
	Build a view structure.

	Args:		inView		the parent CView
				inContext	the view frame
	Return:	CView *
--------------------------------------------------------------------------------*/

CView *
BuildView(CView * inView, RefArg inContext)
{
	CView * theView;
	int	vwClass = RINT(GetProtoVariable(inContext, SYMA(viewClass)));
	switch (vwClass)
	{
	case clView:
		theView = new CView;
		break;

//	case clRootView:		can’t be built, it’s part of the system

	case clPictureView:
		theView = new CPictureView;
		break;

	case clEditView:
		theView = new CEditView;
		break;

//	case clContainerView:	// OS1

	case clKeyboardView:
		theView = new CKeyboardView;
		break;

	case clMonthView:
		theView = new CMonthView;
		break;

	case clParagraphView:
		{
			ULong vwFlags = RINT(GetProtoVariable(inContext, SYMA(viewFlags)))
							  & (vReadOnly + vCalculateBounds + vGesturesAllowed + vClipboard);
			if (vwFlags == vReadOnly
			&& ISNIL(GetProtoVariable(inContext, SYMA(styles)))
			&& ISNIL(GetProtoVariable(inContext, SYMA(tabs))))
				theView = new CTextView;
			else
				theView = new CParagraphView;
		}
		break;

	case clPolygonView:
		theView = new CPolygonView;
		break;

//	case clDataView:		can’t be built, it’s a base class for other views

	case clMathExpView:
		theView = new CMathExpView;
		break;

	case clMathOpView:
		theView = new CMathOpView;
		break;

	case clMathLineView:
		theView = new CMathLineView;
		break;

//	case clScheduleView:	// OS1

	case clRemoteView:
		theView = new CRemoteView;
		break;

//	case clTrainView,		// OS1
//	case clDayView:		// OS1

	case clPickView:
		theView = new CPickView;
		break;

	case clGaugeView:
		theView = new CGaugeView;
		break;

//	case clRepeatingView:	// OS1

	case clPrintView:
		theView = new CPrintView;
		break;

	case clMeetingView:
		theView = new CMeetingView;
		break;

	case clSliderView:
		theView = new CSliderView;
		break;

//	case clCharBoxView:		// OS1

	case clTextView:
		theView = new CTextView;
		break;

	case clListView:
		theView = new CListView;
		break;

//	case cl100:

	case clClipboard:
		theView = new CClipboardView;
		break;

//	case cl102:
//	case cl103:
//	case clLibrarian:

	case clOutline:
		theView = new COutlineView;
		break;

//	case cl106:

	case clHelpOutline:
		theView = new CHelpOutlineView;
		break;

	case clTXView:
		theView = new CTXView;
		break;

	default:
		theView = NULL;
	}

	if (theView == NULL)
		ThrowErr(exRootException, -8471);

	newton_try
	{
		theView->init(inContext, inView);
	}
	cleanup
	{
		theView->removeView();
	}
	end_try;

	return theView;
}

#pragma mark -
Ref
ExtractRichStringFromParaSlots(RefArg inText, RefArg inStyles, ArrayIndex inOffset, ArrayIndex inLength)
{
// INCOMPLETE
	return NILREF;
}


Ref
PolygonDescription(RefArg inView)
{
	return NOTNIL(GetProtoVariable(inView, SYMA(ink))) ? *RSinkName : *RSshapeName;
}


/*--------------------------------------------------------------------------------
	Extract data from a list of views.
	Args:		rcvr
				inViews			array of view objects
				inSepStr			separator string
				inPrintLength	max length of string to generate
	Return:	Ref		string describing the views
--------------------------------------------------------------------------------*/

Ref
FExtractData(RefArg rcvr, RefArg inViews, RefArg inSepStr, RefArg inPrintLength)
{
	if (ISNIL(inViews) || Length(inViews) == 0)
		return MakeString(gEmptyString);		// 0C101754

	ArrayIndex numOfViews = Length(inViews);
	RefVar thisView, prevView;
	RefVar viewStny;
	RefVar viewText;

	ArrayIndex sepStrLen = Length(inSepStr) / sizeof(UniChar) - 1;
	ArrayIndex strPrtLen = 0;
	ArrayIndex strMaxLen = RINT(inPrintLength);

	RefVar viewDescr(MakeArray(numOfViews*2-1));		// view description / separator strings
	ArrayIndex viewIndex = 0;								// incremented as we process views
	ArrayIndex viewStrIndex = 0;							// incremented as we add strings to viewDescr

	RefVar viewObject;
	if (numOfViews < 40)
	{
		// sort views by position, top - bottom of display
		RefVar path(AllocateArray(SYMA(pathExpr), 2));
		SetArraySlot(path, 0, SYMA(viewBounds));
		SetArraySlot(path, 1, SYMA(top));
		viewObject = Clone(inViews);
		SortArray(viewObject, SYMA(_3C), path);	// '<
	}
	else
	{
		viewObject = inViews;
	}

	// PASS 1
	// extract text from paras
	prevView = NILREF;
	for (ArrayIndex i = 0; i < numOfViews && strPrtLen < strMaxLen; ++i)
	{
		thisView = GetArraySlot(viewObject, i);
		if (thisView != prevView)
		{
			viewStny = GetProtoVariable(thisView, SYMA(viewStationery));
			if (ISNIL(viewStny) || EQ(viewStny, SYMA(para)))
			{
				viewText = GetProtoVariable(thisView, SYMA(text));
				if (NOTNIL(viewText))
				{
					CDataPtr strData(viewText);
					UniChar * str = (UniChar *)(char *)strData;
					ArrayIndex strLen = Ustrlen(str);
					ArrayIndex strAvailableLen = MIN(strMaxLen - strPrtLen, strLen);
					RefVar descr(ExtractRichStringFromParaSlots(GetProtoVariable(thisView, SYMA(text)), GetProtoVariable(thisView, SYMA(styles)), 0, strAvailableLen));
					SetArraySlot(viewDescr, viewStrIndex++, descr);
					CRichString rStr(descr);
					strPrtLen += rStr.length();
					if (++viewIndex < numOfViews)
					{
						SetArraySlot(viewDescr, viewStrIndex++, inSepStr);
						strPrtLen += sepStrLen;
					}
				}
			}
		}
		prevView = thisView;
	}

	// PASS 2
	// extract ink from views
	prevView = NILREF;
	for (ArrayIndex i = 0; i < numOfViews && strPrtLen < strMaxLen; ++i)
	{
		thisView = GetArraySlot(viewObject, i);
		if (thisView != prevView)
		{
			viewStny = GetProtoVariable(thisView, SYMA(viewStationery));
			if (NOTNIL(GetProtoVariable(thisView, SYMA(ink)))  ||  EQ(viewStny, SYMA(poly)))
				viewText = PolygonDescription(thisView);
			else if (ISNIL(viewStny)  ||  EQ(viewStny, SYMA(para)))
				viewText = NILREF;
			else
				viewText = RSdataName;
			if (NOTNIL(viewText))
			{
				SetArraySlot(viewDescr, viewStrIndex++, viewText);
				strPrtLen += (Length(viewText) / sizeof(UniChar) - 1);
				if (++viewIndex < numOfViews)
				{
					SetArraySlot(viewDescr, viewStrIndex++, inSepStr);
					strPrtLen += sepStrLen;
				}
			}
		}
		prevView = thisView;
	}

	RefVar result = Stringer(viewDescr);

//	replace newlines and tabs in text with spaces
	CRichString richStr(result);
	CDataPtr strData(result);
	UniChar * s = (UniChar *)(char *)strData;
	for (ArrayIndex i = 0; i < richStr.length(); ++i, ++s)
	{
		UniChar ch = *s;
		if (ch == 0x0D || ch == 0x09)
			*s = ' ';
	}

	return result;
}


/*--------------------------------------------------------------------------------
	Get a view.
	Args:		inRcvr	the C view
	Return:	Ref		the NS view context
--------------------------------------------------------------------------------*/

Ref
FGetView(RefArg inRcvr, RefArg inView)
{
	CView *	view = GetView(inRcvr, inView);
	if (view)
		return view->fContext;
	return NILREF;
}


Ref
FGetId(RefArg inRcvr, RefArg inView)
{
	CView *	view = GetView(inRcvr, inView);
	if (view)	// original doesn’t check
		return MAKEINT(view->fId);
	return NILREF;
}


/*--------------------------------------------------------------------------------
	Get a view’s flags.
	Args:		inRcvr
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
FGetFlags(RefArg inRcvr, RefArg inView)
{
	int		flags = 0;
	CView *	view = GetView(inRcvr, inView);
	if (view)
		flags = view->viewFlags;
	return MAKEINT(flags);
}


Ref
FGetTextFlags(RefArg inRcvr, RefArg inView)
{
	int		flags = 0;
	CView *	view = GetView(inView);
	if (view)
		flags = view->textFlags();
	return MAKEINT(flags);
}


/*--------------------------------------------------------------------------------
	Build view context.
	Args:		inRcvr
				inView
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
FBuildContext(RefArg inRcvr, RefArg inTemplate)
{
	CView * view = GetView(inRcvr);
	if (view == NULL)
		view = gRootView;
	return view->buildContext(inTemplate, true);
}


Ref
CommonAddView(RefArg inRcvr, RefArg inParentView, RefArg inChildTemplate, RefArg inTag)
{
	CView * parentView = FailGetView(inRcvr, inParentView);
	RefVar childContext(parentView->buildContext(inChildTemplate, true));
	CView * childView = NOTNIL(childContext) ? BuildView(parentView, childContext) : NULL;
	if (childView)
	{
		RefVar children(parentView->getProto(inTag));
		if (ISNIL(children))
		{
			children = AllocateArray(inTag, 1);
			SetArraySlot(children, 0, inChildTemplate);
		}
		else
			AddArraySlot(children, inChildTemplate);
		return childContext;
	}
	return NILREF;
}


Ref
FAddView(RefArg inRcvr, RefArg inParentView, RefArg inChildTemplate)
{
	return CommonAddView(inRcvr, inParentView, inChildTemplate, SYMA(viewChildren));
}


Ref
FAddStepView(RefArg inRcvr, RefArg inParentView, RefArg inChildTemplate)
{
	return CommonAddView(inRcvr, inParentView, inChildTemplate, SYMA(stepChildren));
}


Ref
CommonRemoveView(RefArg inRcvr, RefArg inParentView, RefArg inChildView, RefArg inTag)
{
	int bad = 0;
	XTRY
	{
		CView * parentView = FailGetView(inRcvr, inParentView);
		XFAILIF((parentView->viewFlags & (vIsInSetupForm | vIsInSetup2)) == (vIsInSetupForm | vIsInSetup2), bad = 4711;)

		CView * childView = GetView(inRcvr, inChildView);
		if (childView == NULL)
		{
			BadWickedNaughtyNoot(4712);
			CListLoop iter(parentView->viewChildren);
			CView * aChild;
			while ((aChild = (CView *)iter.next()))
			{
				if (GetFrameSlot(aChild->fContext, SYMA(_proto)) == inChildView)
				{
					childView = aChild;
					break;
				}
			}
		}
		if (childView == NULL)
			ThrowMsg("NULL view");

		XFAILIF((childView->viewFlags & (vIsInSetupForm | vIsInSetup2)) == (vIsInSetupForm | vIsInSetup2), bad = 4713;)

		RefVar theTemplate(GetFrameSlot(childView->fContext, SYMA(_proto)));
		parentView->removeChildView(childView);
		RefVar children(parentView->getProto(inTag));
		XFAILIF(IsReadOnly(children), bad = 4714;)
		ArrayRemove(children, theTemplate);
	}
	XENDTRY;

	XDOFAIL(bad)
	{
		BadWickedNaughtyNoot(bad);
	}
	XENDFAIL;

	return NILREF;
}


Ref
FRemoveView(RefArg inRcvr, RefArg inParentView, RefArg inChildView)
{
	return CommonRemoveView(inRcvr, inParentView, inChildView, SYMA(viewChildren));
}


Ref
FRemoveStepView(RefArg inRcvr, RefArg inParentView, RefArg inChildView)
{
	return CommonRemoveView(inRcvr, inParentView, inChildView, SYMA(stepChildren));
}


Ref
FRefreshViews(RefArg inRcvr)
{
	gRootView->update();
	return TRUEREF;
}


/*--------------------------------------------------------------------------------
	Open a view.
	Root view :_Open() -> native function FOpenX
	Args:		inRcvr	the C view
	Return:	Ref		the NS view context
--------------------------------------------------------------------------------*/

Ref
FOpenX(RefArg inRcvr)
{
	RealOpenX(inRcvr, false);
	return TRUEREF;
}


bool
RealOpenX(RefArg inRcvr, bool inHuh)
{
	bool isOK = true;
	CView * view = GetView(inRcvr);
	if (view)
	{
		if (!FLAGTEST(view->viewFlags, vVisible))
		{
			RefVar cmd(MakeCommand(aeShow, view, inHuh ? 0x00420000 : 0x08000000));
			gApplication->dispatchCommand(cmd);
		}
		else
			isOK = false;
	}
	else
	{
		RefVar parent(GetProtoVariable(inRcvr, SYMA(_parent)));
		view = FailGetView(parent);
		RefVar cmd(MakeCommand(aeAddChild, view, inHuh ? 0x00420000 : 0x08000000));
		CommandSetFrameParameter(cmd, inRcvr);
		gApplication->dispatchCommand(cmd);
	}
	return isOK;
}


/*--------------------------------------------------------------------------------
	Close the receiver.
	Args:		inRcvr		a view context
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FCloseX(RefArg inRcvr)
{
	CView *  view = GetView(inRcvr);
	if (view)
	{
		if (FLAGTEST(view->viewFlags, vIsInSetupForm)
		&&  (view->viewFlags & 0x90000000) != 0x90000000)
			view->setFlags(0x90000000);
		else if ((view->viewFlags & 0x90000000) != 0x90000000)
		{
			RefVar	cmd(MakeCommand(aeDropChild, view->fParent, (long)view));	// sic
			gApplication->dispatchCommand(cmd);
		}
		else
			BadWickedNaughtyNoot(4715);
	}
	return TRUEREF;
}


/*--------------------------------------------------------------------------------
	Toggle the receiver.
	Args:		inRcvr		a view context
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FToggleX(RefArg inRcvr)
{
	CView *  view = GetView(inRcvr);
	if (view)
	{
		if ((view->viewFlags & 0x90000000) != 0x90000000)
		{
			RefVar	cmd(MakeCommand(aeDropChild, view->fParent, (long)view));	// sic
			gApplication->dispatchCommand(cmd);
		}
		else
			BadWickedNaughtyNoot(4716);
		return NILREF;
	}
	else
	{
		view = FailGetView(GetProtoVariable(inRcvr, SYMA(_parent)));
		if ((view->viewFlags & 0x90000000) != 0x90000000)
		{
			RefVar	cmd(MakeCommand(aeAddChild, view, 0x08000000));
			CommandSetFrameParameter(cmd, inRcvr);
			gApplication->dispatchCommand(cmd);
		}
		else
			BadWickedNaughtyNoot(4717);
	}
	return TRUEREF;
}


/*--------------------------------------------------------------------------------
	Show the receiver.
	Args:		inRcvr		a view context
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FShowX(RefArg inRcvr)
{
	CView *  view = FailGetView(inRcvr);
	if ((view->viewFlags & 0x90000000) != 0x90000000)
	{
		RefVar	cmd(MakeCommand(aeShow, view, 0x08000000));
		gApplication->dispatchCommand(cmd);
	}
	else
		BadWickedNaughtyNoot(4718);
	return TRUEREF;
}


/*--------------------------------------------------------------------------------
	Hide the receiver.
	Args:		inRcvr		a view context
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FHideX(RefArg inRcvr)
{
	CView *  view = FailGetView(inRcvr);
	if ((view->viewFlags & 0x90000000) != 0x90000000)
	{
		RefVar	cmd(MakeCommand(aeHide, view, 0x08000000));
		gApplication->dispatchCommand(cmd);
		if (gModalCount)
			RemoveModalSafeView(view);
	}
	else
		BadWickedNaughtyNoot(4719);
	return TRUEREF;
}


/*--------------------------------------------------------------------------------
	Sync the receiver - rebuild its viewChildren.
	Args:		inRcvr		a view context
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FSyncViewX(RefArg inRcvr)
{
	CView *  view = FailGetView(inRcvr);
	view->sync();
	return TRUEREF;
}


/*--------------------------------------------------------------------------------
	Redisplay the receiver’s viewChildren.
	Args:		inRcvr		a view context
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FRedoChildrenX(RefArg inRcvr)
{
	CView *  view = FailGetView(inRcvr);
	if (view->viewFlags & vIsInSetupForm)
	{
		BadWickedNaughtyNoot(4720);
		// call us back later, after setupForm is done
		gApplication->addDelayedAction(inRcvr, SYMA(RedoChildren), RA(NILREF), RA(NILREF));
	}
	else
	{
		newton_try
		{
			view->removeAllViews();
			view->addViews(false);
			view->dirty();
		}
		cleanup
		{
			view->clearFlags(vIsInSetupForm | vIsInSetup2);
		}
		end_try;
	}
	return TRUEREF;
}


/*--------------------------------------------------------------------------------
	Redisplay the receiver’s viewChildren.
	Args:		inRcvr		a view context
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FSyncChildrenX(RefArg inRcvr)
{
	CView *  view = FailGetView(inRcvr);
	if (view->viewFlags & vIsInSetupForm)
	{
		BadWickedNaughtyNoot(4721);
		// call us back later, after setupForm is done
		gApplication->addDelayedAction(inRcvr, SYMA(SyncChildren), RA(NILREF), RA(NILREF));
	}
	else
	{
		newton_try
		{
			view->addViews(true);
		}
		cleanup
		{
			view->clearFlags(vIsInSetupForm | vIsInSetup2);
		}
		end_try;
	}
	return TRUEREF;
}


Ref
FSyncScrollX(RefArg inRcvr, RefArg inWhat, RefArg index, RefArg inUpDown)
{
	CView *  view = FailGetView(inRcvr);
	if ((ObjectFlags(inWhat) & kObjFrame) != 0)
		// must be a soup cursor frame
		return view->syncScrollSoup(inWhat, inUpDown);
	// must be an array of view templates
	return view->syncScroll(inWhat, index, inUpDown);
}


/*--------------------------------------------------------------------------------
	Lay out a table of view templates.
	Args:		inRcvr		a view context
				inTableDef
				inColStart
				inRowStart
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FLayoutTableX(RefArg inRcvr, RefArg inTableDef, RefArg inColStart, RefArg inRowStart)
{
	CView *  view = FailGetView(inRcvr);
// it’s a whopper
	return NILREF;
}


/*--------------------------------------------------------------------------------
	Lay out a column of view templates.
	Args:		inRcvr			a view context
				inChildViews	an array of view templates
				inStart			index into array
	Return:	array of view templates that fit within the receiver view
--------------------------------------------------------------------------------*/

Ref
FLayoutVerticallyX(RefArg inRcvr, RefArg inChildViews, RefArg inStart)
{
	CView *  view = FailGetView(inRcvr);
	int		viewHt = RectGetHeight(view->viewBounds);
	int		index = RINT(inStart);
	int		limit = Length(inChildViews);
	bool		allCollapsed = NOTNIL(GetProtoVariable(inRcvr, SYMA(allCollapsed)));
	Ref		collapsedHeight = GetVariable(inRcvr, SYMA(collapsedHeight));
	int		collapsedHt = NOTNIL(collapsedHeight) ? RINT(collapsedHeight) : 0;
	RefVar	column(MakeArray(0));
	RefVar	child;
	int		ht, childHt;
	for (ht = 0; ht < viewHt && index < limit; ht += childHt)
	{
		child = GetArraySlot(inChildViews, index++);
		childHt = RINT(GetProtoVariable(child, SYMA(height)));
		if (allCollapsed || NOTNIL(GetProtoVariable(child, SYMA(collapsed))))
			childHt = collapsedHt;
		AddArraySlot(column, child);
	}
	return column;
}


Ref
FFormatVertical(RefArg inRcvr, RefArg inBounds, RefArg inArg2)
{
	CView *  view = FailGetView(inRcvr);
	Rect bounds;
	if (!FromObject(inBounds, &bounds))
		ThrowMsg("not a rectangle");
	int r2 = 0;
	if (NOTNIL(inArg2))
	{
		ArrayIndex numOfChildren;
		int viewHt = view->childrenHeight(&numOfChildren);
		r2 =  (numOfChildren == 0) ? 0 : RectGetHeight(bounds) - (RectGetHeight(bounds) / numOfChildren);
	}
	return MAKEINT(view->setChildrenVertical(bounds.top + r2, r2));
}


#pragma mark -

Ref
FSetOriginX(RefArg inRcvr, RefArg inX, RefArg inY)
{
	CView *  view = FailGetView(inRcvr);
	Point newOrigin;
	newOrigin.h = RINT(inX);
	newOrigin.v = RINT(inY);
	view->setOrigin(newOrigin);
	return NILREF;
}


#pragma mark -
/*--------------------------------------------------------------------------------
	Set up an idler.
	Args:		inRcvr		a view context
				inFreq		idle frequency in milliseconds
	Return:	NILREF
--------------------------------------------------------------------------------*/

Ref
FSetupIdleX(RefArg inRcvr, RefArg inFreq)
{
	CView *  view = GetView(inRcvr);
	if (view)
		gRootView->addIdler(view, RINT(inFreq), 0);
	return NILREF;
}


/*--------------------------------------------------------------------------------
	Set a frame value.
	Args:		inRcvr		a view context
				inView
				inTag
				inValue
	Return:	NILREF
--------------------------------------------------------------------------------*/

Ref
FSetValue(RefArg inRcvr, RefArg inFrame, RefArg inTag, RefArg inValue)
{
	CView *  view = GetView(inRcvr, inFrame);
	if (view)
		view->setSlot(inTag, inValue);
	else
		SetFrameSlot(inFrame, inTag, inValue);
	return NILREF;
}


/*--------------------------------------------------------------------------------
	Set up a bounds rect.
	Args:		inRcvr		a view context
				inLeft
				inTop
				inRight
				inBottom
	Return:	bounds frame
--------------------------------------------------------------------------------*/

Ref
SetRectFrame(short inLeft, short inTop, short inRight, short inBottom)
{
	Rect  box;
	box.left = inLeft;
	box.top = inTop;
	box.right = inRight;
	box.bottom = inBottom;
	return ToObject(&box);
}


Ref
FSetBounds(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom)
{
	return SetRectFrame(RINT(inLeft), RINT(inTop), RINT(inRight), RINT(inBottom));
}


Ref
FRelBounds(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inWidth, RefArg inHeight)
{
	int  left = RINT(inLeft);
	int  top = RINT(inTop);
	return SetRectFrame(left, top, left + RINT(inWidth), top + RINT(inHeight));
}


bool
CommonBox(RefArg inRcvr, Rect * outBox)
{
	CView *  view = FailGetView(inRcvr);
	if (view->viewFlags & vIsInSetupForm)
	{
		if (FromObject(view->getProto(SYMA(viewBounds)), outBox))
			view->justifyBounds(outBox);
		else
			return false;
	}
	else
		*outBox = view->viewBounds;
	return true;
}


/*--------------------------------------------------------------------------------
	Return the bounds of the receiver in local (top-left = 0,0) coordinates.
	Args:		inRcvr		a view context
	Return:	buonds frame
--------------------------------------------------------------------------------*/

Ref
FLocalBoxX(RefArg inRcvr)
{
	Rect  box;
	if (CommonBox(inRcvr, &box))
	{
		box.right = box.right - box.left;
		box.left = 0;
		box.bottom = box.bottom - box.top;
		box.top = 0;
		return ToObject(&box);
	}
	return NILREF;
}


/*--------------------------------------------------------------------------------
	Return the bounds of the receiver in global coordinates.
	Args:		inRcvr		a view context
	Return:	buonds frame
--------------------------------------------------------------------------------*/

Ref
FGlobalBoxX(RefArg inRcvr)
{
	Rect  box;
	if (CommonBox(inRcvr, &box))
	{
		return ToObject(&box);
	}
	return NILREF;
}


/*--------------------------------------------------------------------------------
	Return the outer bounds of the receiver in global coordinates.
	Args:		inRcvr		a view context
	Return:	buonds frame
--------------------------------------------------------------------------------*/

Ref
FGlobalOuterBoxX(RefArg inRcvr)
{
	CView *  view = FailGetView(inRcvr);
	Rect		box;
	view->outerBounds(&box);
	return ToObject(&box);
}


/*--------------------------------------------------------------------------------
	Return the visible bounds of the receiver in global coordinates.
	Args:		inRcvr		a view context
	Return:	buonds frame
--------------------------------------------------------------------------------*/

Ref
FVisibleBox(RefArg inRcvr)
{
	CView *  view = FailGetView(inRcvr);
	Rect		box;
	// the original intersects with the GrafPort->portRect
	// what’s the Quartz equivalent?
	SectRect(&view->viewBounds, &box, &box);
	return ToObject(&box);
}


/*--------------------------------------------------------------------------------
	Return the bounds we can draw into.
	Args:		inRcvr		a view context
	Return:	buonds frame
--------------------------------------------------------------------------------*/

Ref
FGetDrawBoxX(RefArg inRcvr)
{
	Rect		box;
	// the original returns GrafPort->portRect
	// what’s the Quartz equivalent?
	return ToObject(&box);
}


/*--------------------------------------------------------------------------------
	Dirty an area of the receiver.
	Args:		inRcvr		a view context
				inRect		the area to dirty
	Return:	NILREF
--------------------------------------------------------------------------------*/

Ref
FDirtyBoxX(RefArg inRcvr, RefArg inRect)
{

	CView *  view = GetView(inRcvr);
	Rect		box;
	if (view && FromObject(inRect, &box))
	{
		view->dirty(&box);
	}
	return NILREF;
}


/*--------------------------------------------------------------------------------
	Dirty the receiver.
	Args:		inRcvr		a view context
	Return:	TRUEREF
--------------------------------------------------------------------------------*/

Ref
FDirtyX(RefArg inRcvr)
{
	CView *  view = GetView(inRcvr);
	if (view)
	{
		view->dirty();
	}
	return TRUEREF;
}


Ref
FParentX(RefArg inRcvr)
{
	return GetProtoVariable(inRcvr, SYMA(_parent));
}


Ref
FChildViewFramesX(RefArg inRcvr)
{
	CView *  view = FailGetView(inRcvr);
	return view->childViewFrames();
}


Ref
FMoveBehindX(RefArg inRcvr, RefArg inBehindView)
{
	if (!EQ(inRcvr, inBehindView))
	{
		CView *  view = FailGetView(inRcvr);
		CView *  viewParent = view->fParent;
		if (view != viewParent)
		{
			if (ISNIL(inBehindView))
				view->bringToFront();
			else
			{
				if (EQ(inBehindView, SYMA(skip)))
				{
					ULong saveFlags = view->viewFlags;
					view->viewFlags |= vFloating;
					view->bringToFront();
					view->viewFlags = saveFlags;
				}
				else
				{
					CView *  behindView = FailGetView(inBehindView);
					if (behindView->fParent == viewParent)
						viewParent->moveChildBehind(view, behindView);
				}
			}
		}
	}
	return TRUEREF;
}


Ref
FHiliteOwner(RefArg inRcvr)
{
	CView * view = gRootView->hilitedView();
	return (view != NULL) ? (Ref)view->fContext : NILREF;
}


Ref
FGetHiliteOffsets(RefArg inRcvr)
{
	CView * view = gRootView->hilitedView();
	return (view != NULL) ? (Ref)view->getValue(SYMA(hilites), SYMA(offset)) : NILREF;
}


Ref
FDropHilites(RefArg inRcvr)
{
	CView *  view = GetView(inRcvr);
	if (view && gRootView->hilitedView())
	{
		//INCOMPLETE
	}
	return NILREF;
}


Ref
FHiliteViewChildren(RefArg inRcvr, RefArg inUnit)
{
	CView *  view = FailGetView(inRcvr);
	return MAKEBOOLEAN(view->addHiliter((CUnit *)RefToAddress(inUnit)));
}


Ref
FSetHiliteX(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3)
{
	RefVar isHilited(FSetHiliteNoUpdateX(inRcvr, inArg1, inArg2, inArg3));
	if (NOTNIL(isHilited))
		gRootView->update();
	return isHilited;
}


Ref
FSetHiliteNoUpdateX(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3)
{
	CDataView *  view = (CDataView *)GetView(inRcvr);
	if (view && view->derivedFrom(clDataView))
	{
		if (NOTNIL(inArg3))
		{
			CView * editView;
			if (view->derivedFrom(clParagraphView) && (editView = view->getEnclosingEditView()) != NULL)
				editView->removeAllHilites();
			else
				view->removeAllHilites();
		}
		view->hiliteText(RINT(inArg1), RINT(inArg2) - RINT(inArg3), true);
		return TRUEREF;
	}
	return NILREF;
}


Ref
FHiliteX(RefArg inRcvr, RefArg inDoIt)
{
	CView *  view = GetView(inRcvr);
	if (view)
		view->select(NOTNIL(inDoIt), false);
	return TRUEREF;
}


Ref
FHiliteUniqueX(RefArg inRcvr, RefArg inDoIt)
{
	CView *  view = GetView(inRcvr);
	if (view)
		view->select(NOTNIL(inDoIt), true);
	return TRUEREF;
}


Ref
FHiliter(RefArg inRcvr, RefArg inUnit)
{
	CView *  view = FailGetView(inRcvr);
	RefVar	cmd(MakeCommand(aeStartHilite, view, (int)inUnit));
	gApplication->dispatchCommand(cmd);
	return NILREF;
}


Ref
FTrackHiliteX(RefArg inRcvr, RefArg inUnit)
{
	CStroke * strok = NULL;
	if (NOTNIL(inUnit))
	{
		strok = StrokeFromRef(inUnit);
		strok->inkOff(true);
	}

	if (ISNIL(GetVariable(inRcvr, SYMA(buttonPressedScript))))
		BusyBoxSend(53);

	bool exists;
	RefVar snd(GetProtoVariable(inRcvr, SYMA(_sound), &exists));
	if (exists && NOTNIL(snd))
		FPlaySound(inRcvr, snd);
	else
		FClicker(inRcvr);

	CView * view = FailGetView(inRcvr);
	bool isSelected = (view->viewFlags & vSelected) != 0;
	Point slop = {10,10};

	// non-stroke vars
	Point viewMidPt = MidPoint(&view->viewBounds);
	ArrayIndex count = 0;

	bool isPressed, wasPressed = false;
	for (bool isStrokeDone = false; !isStrokeDone; )
	{
		Point strokPt;
		if (strok)
			strokPt = strok->finalPoint();
		else
			strokPt = viewMidPt;
		isPressed = view->distance(strokPt, &slop) != kDistanceOutOfRange;
		if (isPressed != wasPressed)
		{
			isSelected = !isSelected;
			view->select(isSelected, false);
			wasPressed = isPressed;
		}
		else
		{
			Wait(20);	// milliseconds - was 1 tick
		}
		if (isPressed)
		{
			RefVar undocumentedFeature = DoMessageIfDefined(inRcvr, SYMA(buttonPressedScript), RA(NILREF), NULL);
			if (NOTNIL(undocumentedFeature) && NOTNIL(GetProtoVariable(inRcvr, SYMA(newt_feature))))
			{
				BusyBoxSend(54);
				return undocumentedFeature;
			}
		}
		count++;
		if ((strok && strok->isDone()) || (!strok && count == 2))
			isStrokeDone = true;
	}

	BusyBoxSend(54);
	return MAKEBOOLEAN(isPressed);
}


Ref
FTrackButtonX(RefArg inRcvr, RefArg inUnit)
{
	RefVar isClicked;
	unwind_protect
	{
		isClicked = FTrackHiliteX(inRcvr, inUnit);
		if (NOTNIL(isClicked))
			DoMessage(inRcvr, SYMA(buttonClickScript), RA(NILREF));
	}
	on_unwind
	{
		CView *  view = GetView(inRcvr);
		if (view)
			view->select(false, false);
	}
	end_unwind;
	return isClicked;
}


Ref
FTieViews(RefArg inRcvr, RefArg inMainView, RefArg inDependentView, RefArg inMethodName)
{
	CView *  view = FailGetView(inRcvr);
	RefVar viewContext(view->fContext);
	if (FrameHasSlot(viewContext, SYMA(viewTie)))
	{
		// add to it
		RefVar viewTie(GetFrameSlot(viewContext, SYMA(viewTie)));
		AddArraySlot(viewTie, inDependentView);
		AddArraySlot(viewTie, inMethodName);
	}
	else
	{
		// create it
		RefVar viewTie(MakeArray(2));
		SetArraySlot(viewTie, 0, inDependentView);
		SetArraySlot(viewTie, 1, inMethodName);
		SetFrameSlot(viewContext, SYMA(viewTie), viewTie);
	}
	return TRUEREF;
}


Ref
FOffsetView(RefArg inRcvr, RefArg indX, RefArg indY)
{
	CView *  view = FailGetView(inRcvr);
	Point		delta;
	delta.h = RINT(indX);
	delta.v = RINT(indY);
	view->offset(delta);
	return TRUEREF;
}


#pragma mark Drag & Drop

void
GetAppAreaBounds(Rect * outBounds)
{
	RefVar displayParams(GetGlobalVar(SYMA(displayParams)));
	outBounds->top = RINT(GetProtoVariable(displayParams, SYMA(appAreaGlobalTop)));
	outBounds->left = RINT(GetProtoVariable(displayParams, SYMA(appAreaGlobalLeft)));
	outBounds->bottom = outBounds->top + RINT(GetProtoVariable(displayParams, SYMA(appAreaHeight)));
	outBounds->right = outBounds->left + RINT(GetProtoVariable(displayParams, SYMA(appAreaWidth)));
}


Ref
FDragX(RefArg inRcvr, RefArg inUnit, RefArg inDragBounds)
{
	CView *  view = FailGetView(inRcvr);
	CStroke * strok = StrokeFromRef(inUnit);
	strok->inkOff(true);
	Rect bounds;
	if (NOTNIL(inDragBounds))
		FromObject(inDragBounds, &bounds);
	else
		GetAppAreaBounds(&bounds);
	view->drag(strok, &bounds);
	return TRUEREF;
}


Ref
FDragAndDrop(RefArg inRcvr, RefArg inUnit, RefArg inBounds, RefArg inLimitBounds, RefArg inCopy, RefArg inDragInfo)
{
	CView *  view = FailGetView(inRcvr);
	Rect bounds, dragLimitBounds;
	FromObject(inBounds, &bounds);
	if (NOTNIL(inLimitBounds))
		FromObject(inLimitBounds, &dragLimitBounds);
	CDragInfo dragInfo(inDragInfo);
	return MAKEINT(view->dragAndDrop(StrokeFromRef(inUnit), &bounds, NOTNIL(inLimitBounds) ? &dragLimitBounds : NULL, NULL, NOTNIL(inCopy), &dragInfo, NULL));
}


Ref
FDragAndDropLtd(RefArg inRcvr, RefArg inUnit, RefArg inBounds, RefArg inLimitBounds, RefArg inCopy, RefArg inDragInfo)
{
	CView *  view = FailGetView(inRcvr);
	Rect bounds, dragLimitBounds;
	FromObject(inBounds, &bounds);
	// it’s several pages
	return NILREF;
}

#pragma mark Clipboard

Ref	FSetClipboard(RefArg inRcvr, RefArg inClipbd) { return NILREF; }
Ref	FGetClipboard(RefArg inRcvr) { return NILREF; }
Ref	FGetClipboardIcon(RefArg inRcvr) { return NILREF; }
Ref	FClipboardCommand(RefArg inRcvr, RefArg inCmd) { return NILREF; }

#pragma mark Drawing

Ref	FDoDrawing(RefArg inRcvr, RefArg inMsg, RefArg inArgs) { return NILREF; }

Ref	FInvertRect(RefArg inRcvr, RefArg inLeft, RefArg inTop, RefArg inRight, RefArg inBottom)
{
	Rect rect;
//	GetGlobalRect(&rect, inLeft, inTop, inRight, inBottom);
//	InvertRect(&rect);
	return NILREF;
}


Ref	FGetRemoteWriting(RefArg inRcvr) { return NILREF; }
Ref	FSetRemoteWriting(RefArg inRcvr, RefArg inArg) { return NILREF; }
Ref	FGetCaretInfo(RefArg inRcvr) { return NILREF; }
Ref	FSetCaretInfo(RefArg inRcvr, RefArg inArg1, RefArg inArg2) { return NILREF; }
Ref	FPositionCaret(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3) { return NILREF; }	// edit view
Ref	FCaretRelativeToVisibleRect(RefArg inRcvr, RefArg inArg) { return NILREF; }	// para view
Ref	FShowCaret(RefArg inRcvr) { return NILREF; }
Ref	FHideCaret(RefArg inRcvr) { return NILREF; }
Ref	FInsertItemsAtCaret(RefArg inRcvr, RefArg inArg) { return NILREF; }
Ref	FViewContainsCaretView(RefArg inRcvr, RefArg inArg) { return NILREF; }
Ref	FGetInkWordInfo(RefArg inRcvr, RefArg inArg) { return NILREF; }

Ref	FGetAlternates(RefArg inRcvr, RefArg inArg1, RefArg inArg2) { return NILREF; }

#pragma mark Ink

Ref
anon00170E90(RefArg inRcvr, int inSelector, RefArg inArg)
{ return NILREF; }

Ref
FHandleInsertItems(RefArg inRcvr, RefArg inItems)
{
	return MAKEBOOLEAN(anon00170E90(inRcvr, 77, inItems));
}

Ref
FHandleInkWord(RefArg inRcvr, RefArg inWord)
{
	return MAKEBOOLEAN(anon00170E90(inRcvr, 24, inWord));
}

Ref
FHandleRawInk(RefArg inRcvr, RefArg inInk)
{
	return MAKEBOOLEAN(anon00170E90(inRcvr, 21, inInk));
}

