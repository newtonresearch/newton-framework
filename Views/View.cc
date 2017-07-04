/*
	File:		View.cc

	Contains:	CView implementation.

	Written by:	Newton Research Group.
*/
#include "Quartz.h"

#include "ObjHeader.h"
#include "Objects.h"
#include "Globals.h"
#include "ROMResources.h"
#include "Lookup.h"
#include "Funcs.h"
#include "Arrays.h"
#include "Strings.h"
#include "Unicode.h"
#include "MagicPointers.h"

#include "RootView.h"
#include "Modal.h"
#include "Notebook.h"
#include "ListLoop.h"
#include "Animation.h"
#include "Sound.h"
#include "Preference.h"

#include "StrokeCentral.h"

#include "Geometry.h"
#include "DrawShape.h"


/* -------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------- */
extern "C" {
Ref	FInRepeatedKeyCommand(RefArg inRcvr);
}

extern bool		EnableFramesFunctionProfiling(bool);	// for debug

extern CView *	BuildView(CView * inView, RefArg inContext);
extern bool		ViewContainsCaretView(CView * inView);
extern void		ModalSafeShow(CView * inView);
extern void		PurgeAreaCache(void);
extern Ref		DoProtoMessage(RefArg rcvr, RefArg msg, RefArg args);
extern Ref		SPrintObject(RefArg inObj);

extern void		ArrayAppendInFrame(RefArg frame, RefArg slot, RefArg element);

extern bool		FromObject(RefArg inObj, short * outNum);
extern bool		FromObject(RefArg inObj, Rect * outBounds);
extern Ref		ToObject(const Rect * inBounds);

extern bool		RectsOverlap(const Rect * r1, const Rect * r2);


/* -------------------------------------------------------------------------------
	D a t a

	The tag cache is an array of commonly used slot symbols. CView methods are
	provided which acccept an index into the tag cache, saving symbol lookup
	overhead.
------------------------------------------------------------------------------- */

extern CViewList *gEmptyViewList;
extern bool			gOutlineViews;
extern int			gScreenHeight;

const Ref * gTagCache;						// 0C104F58.x00
static int	gUniqueViewId = 0;			// 0C104F58.x04
bool			gSkipVisRegions = false;	// 0C104F58.x08
static CRegionStruct * g0C104F64;		// 0C104F58.x0C
static CRegionStruct * gClippedRgn;		// 0C104F58.x10	0C104F68
static bool gIsClippedRgnInitialised = false;	// 0C104F58.x14	0C104F6C
static bool	gInRepKeyCmd = false;		// 0C104F58.x18

/* -------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------- */

bool
ProtoEQ(RefArg obj1, RefArg obj2)
{
	RefVar	fr(obj1);

	do
	{
		if (EQ(fr, obj2))
			return true;
	} while (NOTNIL(fr = GetFrameSlot(fr, SYMA(_proto))));

	return false;
}


bool
SoupEQ(RefArg inArg1, RefArg inArg2)
{
	RefVar uid1(GetProtoVariable(inArg1, SYMA(_uniqueId)));
	RefVar uid2(GetProtoVariable(inArg2, SYMA(_uniqueId)));
	if (NOTNIL(uid1) && NOTNIL(uid2))
		return EQ(uid1, uid2);
	return ProtoEQ(inArg1, inArg2);
	
}


CView *
Exists(CViewList * inList, RefArg inView)
{
	CView *		childView;
	CListLoop	iter(inList);
	while ((childView = (CView *)iter.next()) != NULL)
		if (ProtoEQ(childView->fContext, inView))
			return childView;

	return NULL;
}


Ref
GetCacheContext(RefArg inView)
{
	RefVar	context(GetFrameSlot(inView, SYMA(_cacheContext)));
	if (NOTNIL(context))
	{
		context = Clone(context);
		SetFrameSlot(context, SYMA(_proto), inView);
		SetFrameSlot(context, SYMA(_parent), gRootView->fContext);
	}
	return context;
}

#pragma mark -

long
GetFirstNonFloater(CViewList * inList)
{
	CView *			view;
	CBackwardLoop	iter(inList);
	while ((view = (CView *)iter.next()) != NULL
		&&  FLAGTEST(view->viewFlags, vFloating))
		;
	return iter.index();
}


#pragma mark -
/* -------------------------------------------------------------------------------
	C V i e w
------------------------------------------------------------------------------- */

VIEW_SOURCE_MACRO(clView, CView, CResponder)


/* -------------------------------------------------------------------------------
	Constructor.
	Doesn’t do anything, initialization is done in init().
------------------------------------------------------------------------------- */

CView::CView()
{ }


/* -------------------------------------------------------------------------------
	Destructor.
------------------------------------------------------------------------------- */

CView::~CView()
{ }


/* -------------------------------------------------------------------------------
	Perform a command.
	Args:		inCmd			the command frame
	Return:	bool			the command was performed by this class
------------------------------------------------------------------------------- */

bool
CView::doCommand(RefArg inCmd)
{
	bool  isHandled;
	if ((isHandled = realDoCommand(inCmd)) == false && this != fParent)
		isHandled = fParent->doCommand(inCmd);

//	if (ISNIL(isHandled))	// sic
//		isHandled = false;

	return isHandled;
}

#pragma mark -

/* -------------------------------------------------------------------------------
	Initialize.
	Args:		inContext		the context frame for this view
				inParentView	the parent view
	Return:	--
------------------------------------------------------------------------------- */

void
CView::init(RefArg inContext, CView * inParentView)
{
	viewChildren = gEmptyViewList;
	fParent = inParentView;
	fContext = inContext;
	fTagCacheMaskHi = 0x0000FFFF;
	fTagCacheMaskLo = 0xFFFFFFFF;
	fId = ++gUniqueViewId;
	if (this != inParentView)
		SetFrameSlot(inContext, SYMA(_parent), inParentView->fContext);
	SetFrameSlot(inContext, SYMA(viewCObject), AddressToRef(this));

//	Build allocateContext
	RefVar	obj, tag;
	RefVar	aContext(getCacheProto(kIndexAllocateContext));
	if (NOTNIL(aContext))
	{
		ArrayIndex count = Length(aContext);
		for (ArrayIndex i = 0; i < count; i += 2)
		{
			obj = inParentView->buildContext(GetArraySlot(aContext, i+1), true);
			SetFrameSlot(obj, SYMA(_parent), inContext);
			tag = GetArraySlot(aContext, i);
			setContextSlot(tag, obj);
		}
	}

//	Build stepAllocateContext
	aContext = getCacheProto(kIndexStepAllocateContext);
	if (NOTNIL(aContext))
	{
		ArrayIndex count = Length(aContext);
		for (ArrayIndex i = 0; i < count; i += 2)
		{
			obj = inParentView->buildContext(GetArraySlot(aContext, i+1), true);
			SetFrameSlot(obj, SYMA(_parent), inContext);
			tag = GetArraySlot(aContext, i);
			setContextSlot(tag, obj);
		}
	}

//	Do viewSetupFormScript
	setFlags(vIsInSetupForm);
	setupForm();
	if ((viewFlags & 0x90000000) == 0x90000000)
	{
		clearFlags(0x9000000 + vVisible);
		ThrowErr(exRootException, -8501);	// could not create view
	}
	clearFlags(vIsInSetupForm);

//	Set up viewFlags
	if (FLAGTEST(inParentView->viewFlags, vWriteProtected))
		setFlags(vWriteProtected);
	viewFlags |= (RINT(getCacheProto(kIndexViewFlags)) & vEverything);

//	Set up viewFormat
	Ref	format = getProto(SYMA(viewFormat));
	if (NOTNIL(format))
		viewFormat = RINT(format) & vfEverything;

//	If we’re a child of the root view set up a clipper
	if (inParentView == gRootView && this != gRootView)
		SetFrameSlot(inContext, SYMA(viewClipper), AddressToRef(new CClipper));		// when this view is GC’d viewClipper will be leaked, surely?

//	Add us to our parent’s children
	if (this != inParentView)
		inParentView->addCView(this);

	newton_try
	{
		Rect	bounds;
		int	excess;
		obj = getProto(SYMA(viewBounds));
		if (ISNIL(obj))
			ThrowErr(exRootException, -8505);	// NULL view bounds
		if (!FromObject(obj, &bounds))
			ThrowMsg("bad bounds frame");			// bad view bounds
		setBounds(&bounds);

		// if the view extends beyond the bottom of the display, bring it back on
		if (FLAGTEST(viewFlags, vApplication)
		&& this != gRootView
		&& (viewJustify & vjParentVMask) == vjParentTopV
		&& (excess = viewBounds.bottom - gScreenHeight) > 0)
		{
			bounds.top = bounds.bottom - excess;
			if (bounds.top < 0)
				bounds.top = 0;
			viewBounds.bottom = gScreenHeight;
			Point parentOrigin = *(Point *)&fParent->viewBounds;
			OffsetRect(&bounds, -fParent->viewBounds.left, -fParent->viewBounds.top);
			setBounds(&bounds);
		}

		if (FLAGTEST(viewFlags, vVisible) && hasVisRgn())
			inParentView->viewVisibleChanged(this, false);

		if (NOTNIL(obj = getProto(SYMA(declareSelf))))	// intentional assignment
			SetFrameSlot(inContext, obj, inContext);		// obj is a symbol

		addViews(false);

		if (viewJustify & vjReflow)
		{
			bool  isFirst = true;
			int  justOffset;
			Rect  totalBounds;
			CView *		childView;
			CListLoop	iter(viewChildren);
			while ((childView = (CView *)iter.next()) != NULL)
			{
				Rect  childBounds = childView->viewBounds;
				if (childBounds.left == childBounds.right)
					childBounds.right++;
				if (childBounds.top == childBounds.bottom)
					childBounds.bottom++;
				if (isFirst)
				{
					totalBounds = childBounds;
					isFirst = false;
				}
				else
					UnionRect(&totalBounds, &childBounds, &totalBounds);
			}

			switch (viewJustify & vjParentHMask)
			{
			case vjParentCenterH:
				justOffset = ((viewBounds.left + viewBounds.right) - (totalBounds.left + totalBounds.right)) / 2;
				break;
			case vjParentRightH:
				justOffset = viewBounds.right - totalBounds.right;
				break;
			default:
				justOffset = viewBounds.left - totalBounds.left;
				break;
			}
			OffsetRect(&totalBounds, justOffset, 0);

			switch (viewJustify & vjParentVMask)
			{
			case vjParentCenterV:
				justOffset = ((viewBounds.top + viewBounds.bottom) - (totalBounds.top + totalBounds.bottom)) / 2;
				break;
			case vjParentBottomV:
				justOffset = viewBounds.bottom - totalBounds.bottom;
				break;
			default:
				justOffset = viewBounds.top - totalBounds.top;
				break;
			}
			OffsetRect(&totalBounds, 0, justOffset);

			Rect  dejustifiedBounds;
			viewBounds = totalBounds;
			dejustifyBounds(&dejustifiedBounds);
			setDataSlot(SYMA(viewBounds), ToObject(&dejustifiedBounds));
			recalcBounds();
		}

		//	Do viewSetupDoneScript
		setupDone();
	}
	cleanup
	{
		if (FLAGTEST(viewFlags, vVisible) && hasVisRgn())
			inParentView->viewVisibleChanged(this, false);
	}
	end_try;
}


/* -------------------------------------------------------------------------------
	Delete the view.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::dispose(void)
{
	setFlags(0x90000000);
	removeAllHilites();
	bool	wantPostQuit = false;

	// run the viewQuitScript
	newton_try
	{
		RefVar	quitResult(runScript(SYMA(viewQuitScript), RA(NILREF)));
		wantPostQuit = EQ(quitResult, SYMA(postQuit));
	}
	newton_catch(exRootException)
	{ }
	end_try;

	// NULL out our view object and remove all child views
	setContextSlot(SYMA(viewCObject), RA(NILREF));
	bool	hasChildren = (viewChildren->count() - gEmptyViewList->count() != 0);
	if (hasChildren)
		removeAllViews();

	// run the viewPostQuitScript if viewQuitScript wants it
	if (wantPostQuit)
	{
		newton_try
		{
			runScript(SYMA(viewPostQuitScript), RA(NILREF));
		}
		newton_catch(exRootException)
		{ }
		end_try;
	}

	// NULL out viewChildren
	if (hasChildren && (fParent->viewFlags & 0x90000000) != 0x90000000
	 && FrameHasSlot(fContext, SYMA(viewChildren)))
		setContextSlot(SYMA(viewChildren), RA(NILREF));

	// NULL out allocated view context slots
	RefVar	allocation(getCacheProto(kIndexAllocateContext));
	if (NOTNIL(allocation))
	{
		ArrayIndex count = Length(allocation);
		for (ArrayIndex i = 0; i < count; i += 2)
			setContextSlot(GetArraySlotRef(allocation, i), RA(NILREF));
	}

	// also allocated step context slots
	allocation = getCacheProto(kIndexStepAllocateContext);
	if (NOTNIL(allocation))
	{
		ArrayIndex count = Length(allocation);
		for (ArrayIndex i = 0; i < count; i += 2)
			setContextSlot(GetArraySlotRef(allocation, i), RA(NILREF));
	}

	// delete any view clipper
	CClipper * clip = clipper();
	if (clip)
	{
		delete clip;
		SetFrameSlot(fContext, SYMA(viewClipper), RA(NILREF));
	}

	// remove ourselves from the root view
	gRootView->forgetAboutView(this);

	fTagCacheMaskHi |= 0x00010000;	// sic

	// and finally…
	delete this;
}


/* -------------------------------------------------------------------------------
	Build a view hierarchy in the context of the root view.
	Args:		inProto		the view definition
				inArg2
	Return:	the instantiated view
------------------------------------------------------------------------------- */

Ref
CView::buildContext(RefArg inProto, bool inArg2)
{
	RefVar	vwClass(GetProtoVariable(inProto, SYMA(viewClass)));
	RefVar	vwProto(inProto);
	RefVar	vwData;
	RefVar	stdForm;
	long		itsClass;
	bool		isInk = false;

	if (ISNIL(vwClass))
	{
		vwClass = GetProtoVariable(inProto, SYMA(viewStationery));
		if (ISNIL(vwClass) && NOTNIL(GetProtoVariable(inProto, SYMA(ink))))
		{
			vwClass = SYMA(poly);
			isInk = true;
		}
		if (ISNIL(vwClass))
			ThrowErr(exRootException, -8502);  // NULL view class

		stdForm = GetGlobalVar(SYMA(stdForms));
		stdForm = GetProtoVariable(stdForm, vwClass);
		if (ISNIL(stdForm))
			ThrowErr(exRootException, -8503);  // bad view class

		vwClass = GetVariable(stdForm, SYMA(viewClass));
		vwData = inProto;
		itsClass = RINT(vwClass);
		if (itsClass & 0x00010000)
		{
			if (ObjectFlags(vwProto) & kObjReadOnly)
				vwProto = Clone(vwProto);
			SetFrameSlot(vwProto, SYMA(_proto), stdForm);
		}
		else
			vwProto = stdForm;
	}

	if (!inArg2)
	{
		Ref	vwFlags = GetProtoVariable(vwProto, SYMA(viewFlags));
		if (ISNIL(vwFlags))
			ThrowErr(exRootException, -8504);  // NULL view flags

		long  itsFlags = RINT(vwFlags);
		if ((itsFlags & vVisible) == 0)
			return NILREF;
	}

	RefVar	vwContext(GetCacheContext(vwProto));
	itsClass = RINT(vwClass);
	if (itsClass & 0x00010000)
	{
		vwData = inProto;
		if (ISNIL(vwContext))
			vwContext = Clone(MAKEMAGICPTR(31));
	}
	if (ISNIL(vwContext))
		vwContext = Clone(MAKEMAGICPTR(29));
	if (isInk)
		SetFrameSlot(vwContext, SYMA(viewStationery), SYMA(poly));
	SetFrameSlot(vwContext, SYMA(_proto), vwProto);
	SetFrameSlot(vwContext, SYMA(_parent), gRootView->fContext);
	if (NOTNIL(vwData))
		SetFrameSlot(vwContext, SYMA(realData), vwData);

	return vwContext;
}


/* -------------------------------------------------------------------------------
	Perform a command.
	Args:		inCmd		the command frame
	Return:	true if we handled the command
------------------------------------------------------------------------------- */

Ref
GetStrokeBundleFromCommand(RefArg inCmd)
{
	RefVar parm(GetFrameSlot(inCmd, SYMA(parameter)));
	if (NOTNIL(parm))
	{
		CUnit * unit = (CUnit *)RINT(parm);
		if (unit == NULL)
			parm = NILREF;
		else
		{
			parm = unit->wordInfo();
			SetFrameSlot(inCmd, SYMA(correctInfo), parm);
			if (ISNIL(CommandFrameParameter(inCmd)))
				CommandSetFrameParameter(inCmd, unit->strokes());
		}
	}
	if (ISNIL(parm))
		parm = CommandFrameParameter(inCmd);
	return parm;
}


bool
CView::realDoCommand(RefArg inCmd)
{
	bool		isHandled = false;
	CView *  targetView;
/*
	r6 = SYMA(lastTextChanged);
	r7 = gVarFrame;
	r8 = gApplication;
	r9 = gRootView;
*/

	ULong cmdId = CommandId(inCmd);
	switch (cmdId)
	{
// •• R e c o g n i t i o n   E v e n t s
	case aeClick:
		if (FLAGTEST(viewFlags, vClickable))
		{
			RefVar args(MakeArray(1));
			SetArraySlot(args, 0, AddressToRef((void *)CommandParameter(inCmd)));

DefGlobalVar(SYMA(trace), SYMA(functions));
EnableFramesFunctionProfiling(true);

			RefVar clik(runCacheScript(kIndexViewClickScript, args));
			if (EQ(clik, SYMA(skip)))
			{
				isHandled = 2;		// wooo, tricky!
				CommandSetResult(inCmd, 0);
			}
			else
			{
				isHandled = NOTNIL(clik);
				CommandSetResult(inCmd, isHandled);
			}
		}
		break;

	case aeStroke:
		{
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, AddressToRef((void *)CommandParameter(inCmd)));
		isHandled = NOTNIL(runScript(SYMA(viewStrokeScript), args));
		CommandSetResult(inCmd, isHandled);
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		}
		break;

	case aeScrub:
	case aeCaret:
	case aeLine:
	case aeTap:
	case aeDoubleTap:
	case aeHilite:
		newton_try
		{
			RefVar args(MakeArray(2));
			SetArraySlot(args, 0, AddressToRef((void *)CommandParameter(inCmd)));
			SetArraySlot(args, 1, MAKEINT(cmdId));
			isHandled = NOTNIL(runScript(SYMA(viewGestureScript), args, true));
			CommandSetResult(inCmd, isHandled);
		}
		newton_catch_all
		{
			CommandSetResult(inCmd, 1);
			isHandled = true;
		}
		end_try;
		break;

	case aeWord:
		{
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, AddressToRef((void *)CommandParameter(inCmd)));
		isHandled = NOTNIL(runScript(SYMA(viewWordScript), args));
		CommandSetResult(inCmd, isHandled);
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		}
		break;

	case aeInk:
		{
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, GetStrokeBundleFromCommand(inCmd));
		isHandled = NOTNIL(runCacheScript(kIndexViewRawInkScript, args));
		CommandSetResult(inCmd, isHandled);
		}
		break;

	case aeInkText:
		{
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, GetStrokeBundleFromCommand(inCmd));
		isHandled = NOTNIL(runCacheScript(kIndexViewInkWordScript, args));
		CommandSetResult(inCmd, isHandled);
		}
		break;

// •• K e y   E v e n t s
	case aeKeyUp:
	case aeKeyDown:
	case ae34:
	case ae35:
		handleKeyEvent(inCmd, cmdId, NULL);
		isHandled = true;
		break;

// •• V i e w   E v e n t s
	case aeAddChild:
		targetView = addChild(CommandFrameParameter(inCmd));
		gApplication->dispatchCommand(MakeCommand(aeShow, targetView, CommandParameter(inCmd)));
		isHandled = true;
		break;

	case aeDropChild:
		targetView = (CView *)CommandParameter(inCmd);
		targetView->hide();
		removeChildView(targetView);
		isHandled = true;
		break;

	case aeHide:
		hide();
		isHandled = true;
		break;

	case aeShow:
		if (gModalCount == 0 || CommandParameter(inCmd) == 0x00420000)
		{
			show();
			isHandled = true;
		}
		else
		{
			bool  isNormal = false;
			if (fParent == gRootView)
			{
				for (targetView = this; targetView != gRootView; targetView = targetView->fParent)
				{
					if (FLAGTEST(targetView->viewJustify, vIsModal))
					{
						isNormal = true;
						break;
					}
				}
			}
			else
				isNormal = true;
			if (isNormal)
				show();
			else
				ModalSafeShow(this);
			isHandled = true;
		}
		break;

	case aeScrollUp:
		runScript(SYMA(viewScrollUpScript), RA(NILREF), true);
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		isHandled = true;
		break;

	case aeScrollDown:
		runScript(SYMA(viewScrollDownScript), RA(NILREF), true);
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		isHandled = true;
		break;

	case aeOverview:
		runScript(SYMA(viewOverviewScript), RA(NILREF), true);
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		isHandled = true;
		break;

	case aeRemoveAllHilites:
		removeAllHilites();
		isHandled = true;
		break;

	case aeAddData:
		{
		CView * targetView = addToSoup(CommandFrameParameter(inCmd));
		if (targetView)
		{
			int targetViewId = CommandParameter(inCmd);
			if (targetViewId != 0x08000000)
				targetView->fId = targetViewId;
			CommandSetParameter(inCmd, (long)targetView);
			gApplication->postUndoCommand(aeRemoveData, this, targetView->fId);
			targetView->dirty();
		}
		else
			CommandSetParameter(inCmd, 0);
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		isHandled = true;
		}
		break;

	case aeRemoveData:
		{
		CView * targetView = findId(CommandParameter(inCmd));
		if (targetView)
		{
			int targetViewId = targetView->fId;
			RefVar targetViewData(targetView->dataFrame());
			removeFromSoup(targetView);

			RefVar undoCmd(MakeCommand(aeAddData, this, targetViewId));
			CommandSetFrameParameter(undoCmd, targetViewData);
			gApplication->postUndoCommand(undoCmd);
		}
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		isHandled = true;
		}
		break;

	case aeMoveData:
		{
		Point delta;
		*(short *)&delta.h = CommandIndexParameter(inCmd, 0);
		*(short *)&delta.v = CommandIndexParameter(inCmd, 1);
		move(&delta);

		RefVar undoCmd(MakeCommand(aeMoveChild, fParent, fId));
		CommandSetIndexParameter(undoCmd, 0, -*(short *)&delta.h);
		CommandSetIndexParameter(undoCmd, 1, -*(short *)&delta.v);
		gApplication->postUndoCommand(undoCmd);
		
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		isHandled = true;
		}
		break;

	case aeScaleData:
		if ((viewFlags & (vWriteProtected | vReadOnly)) == 0)
		{
			Rect srcRect;
			*(ULong *)&srcRect.top = CommandIndexParameter(inCmd, 0);
			*(ULong *)&srcRect.bottom = CommandIndexParameter(inCmd, 1);
			Rect dstRect;
			*(ULong *)&dstRect.top = CommandIndexParameter(inCmd, 2);
			*(ULong *)&dstRect.bottom = CommandIndexParameter(inCmd, 3);
			scale(&srcRect, &dstRect);

			RefVar undoCmd(MakeCommand(aeScaleData, this, 0x08000000));
			CommandSetIndexParameter(undoCmd, 0, *(long *)&dstRect.top);
			CommandSetIndexParameter(undoCmd, 1, *(long *)&dstRect.bottom);
			CommandSetIndexParameter(undoCmd, 2, *(long *)&srcRect.top);
			CommandSetIndexParameter(undoCmd, 3, *(long *)&srcRect.bottom);
			gApplication->postUndoCommand(undoCmd);

			fParent->dirty();
			isHandled = true;
		}
		break;

	case aeAddHilite:
		{
		RefVar parm(CommandFrameParameter(inCmd));
		if (IsFrame(parm))
		{
			ArrayAppendInFrame(fContext, SYMA(hilites), GetFrameSlot(parm, SYMA(hilite)));
			dirty();
			isHandled = true;
		}
		}
		break;

	case aeRemoveHilite:
		removeHilite(CommandFrameParameter(inCmd));
		isHandled = true;
		break;

	case aeChangeStyle:
		{
		CView * view;
		CListLoop iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
		{
			view->realDoCommand(inCmd);
		}
		gRootView->setParsingUtterance(true);	// huh?
		isHandled = true;
		}
		break;

	case aeChangePen:
		{
		CList * hilitedSubViews = CList::make();
		if (hilitedSubViews)
		{
			CView * view;
			CListLoop iter(viewChildren);
			// build list of hilited child views
			while ((view = (CView *)iter.next()) != NULL)
			{
				if (view->hilited())
					hilitedSubViews->insertLast(view);
			}
			// change the pen in those views
			CListLoop iter2(hilitedSubViews);
			while ((view = (CView *)iter2.next()) != NULL)
			{
				view->realDoCommand(inCmd);
			}
		}
		gRootView->setParsingUtterance(true);	// huh?
		isHandled = true;
		}
		break;

	case aeMoveChild:
		{
		CView * targetView = findId(CommandParameter(inCmd));
		if (targetView)
		{
			CommandSetId(inCmd, aeMoveData);
			targetView->realDoCommand(inCmd);
		}
		DefGlobalVar(SYMA(lastTextChanged), RA(NILREF));
		isHandled = true;
		}
		break;
	}

	return isHandled;
}


/* -------------------------------------------------------------------------------
	Find a view given its id.
	Args:		inId			the id
	Return:	CView *
------------------------------------------------------------------------------- */

CView *
CView::findId(int inId)
{
	CView *		childView;
	CListLoop	iter(viewChildren);
	while ((childView = (CView *)iter.next()) != NULL)
		if (childView->fId == inId)
			return childView;

	return NULL;
}


/* -------------------------------------------------------------------------------
	Do idle processing.
	We run the view’s viewIdleScript.
	Args:		inId			unused
	Return:	int			number of milliseconds until next idle
------------------------------------------------------------------------------- */

int
CView::idle(int inId /*unused*/)
{
	int nextIdle = 0;	// kNoTimeout
	Ref result = runCacheScript(kIndexViewIdleScript, RA(NILREF));
	if (NOTNIL(result))
		nextIdle = RINT(result);

	return nextIdle;
}


/* -------------------------------------------------------------------------------
	Determine whether this view is printing.
	Args:		--
	Return:	bool
------------------------------------------------------------------------------- */

bool
CView::printing(void)
{
#if defined(correct)
	GrafPtr	thePort;
	GetPort(&thePort);
	return (thePort->portBits.pixMapFlags & kPixMapDeviceType) != kPixMapDevScreen;
#else
	return false;
#endif
}


/* -------------------------------------------------------------------------------
	A c c e s s o r s
------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Determine whether this view is in the proto chain of a given view.
	Args:		inContext
	Return:	bool
------------------------------------------------------------------------------- */

bool
CView::protoedFrom(RefArg inContext)
{
	for (Ref fr = inContext; NOTNIL(fr); fr = GetFrameSlot(fr, SYMA(_proto)))
	{
		if (EQ(fr, inContext))
			return true;
	}

	return false;
}


/* -------------------------------------------------------------------------------
	Return a variable in the proto context.
	Args:		inTag			the slot name
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getProto(RefArg inTag) const
{
	return GetProtoVariable(fContext, inTag);
}


/* -------------------------------------------------------------------------------
	Return a well-known variable in the proto context.
	Args:		index			index into the tag cache for the name of the variable
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getCacheProto(ArrayIndex index)
{
	Ref var;
	ULong * bits, mask;

	if (index < 32)
		bits = &fTagCacheMaskLo;
	else
	{
		index -= 32;
		bits = &fTagCacheMaskHi;
	}
	mask = 1 << index;
	if (!(*bits & mask))
		return NILREF;
	var = getProto(gTagCache[index]);
	*bits = ISNIL(var) ? (*bits & ~mask) : (*bits | mask);
	return var;
}


/* -------------------------------------------------------------------------------
	Return a well-known variable in the current context.
	Args:		index			index into the tag cache for the name of the variable
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getCacheVariable(ArrayIndex index)
{
	Ref var;
	ULong * bits, mask;

	if (index < 32)
		bits = &fTagCacheMaskLo;
	else
	{
		index -= 32;
		bits = &fTagCacheMaskHi;
	}
	mask = 1 << index;
	if (!(*bits & mask))
		return NILREF;
	var = getVar(gTagCache[index]);
	*bits = ISNIL(var) ? (*bits & ~mask) : (*bits | mask);
	return var;
}


/* -------------------------------------------------------------------------------
	Invalidate the well-known slot cache.
	Args:		index			index into the tag cache for the name of the variable
	Return:	bool			always true
------------------------------------------------------------------------------- */

bool
CView::invalidateSlotCache(ArrayIndex index)
{
	ULong * bits;

	if (index < 32)
		bits = &fTagCacheMaskLo;
	else
	{
		index -= 32;
		bits = &fTagCacheMaskHi;
	}
	*bits |= (1 << index);
	return true;
}


/* -------------------------------------------------------------------------------
	Return a variable in the current context.
	Args:		inTag			the slot name
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getVar(RefArg inTag)
{
	return GetVariable(fContext, inTag);
}


/* -------------------------------------------------------------------------------
	Get a slot value.
	Args:		inTag			the slot name
				inClass		the class we expect
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getValue(RefArg inTag, RefArg inClass)
{
	RefVar value;
	RefVar valueClass;

	if (EQ(inTag, SYMA(hilites)) && EQ(inClass, SYMA(offset)))
		return NILREF;
	if (EQ(inTag, SYMA(viewFlags)))
		value = MAKEINT(viewFlags);
	if (ISNIL(value))
		value = getVar(inTag);
	valueClass = ClassOf(value);
	if (NOTNIL(inClass) && IsSubclass(valueClass, inClass))
	{
		if (EQ(inClass, SYMA(string)))
			value = SPrintObject(value);
		else if (EQ(inClass, SYMA(int)))
		{
			if (EQ(valueClass, SYMA(char)))
				;	// no need to do anything
			else if (EQ(valueClass, SYMA(boolean)))
				value = MAKEINT(ISNIL(value) ? 0 : 1);
			else
				value = NILREF;
		}
	}

	return value;
}


/* -------------------------------------------------------------------------------
	Set a slot value.
	We handle certain slot names specially.
	NOTE	This method was formarly named SetValue().
	Args:		inTag			the slot name
				inValue		the value to set
	Return:	--
------------------------------------------------------------------------------- */

void
CView::setSlot(RefArg inTag, RefArg inValue)
{
	if (EQ(inTag, SYMA(viewFlags)))
		viewFlags = RINT(inValue);
	else if (EQ(inTag, SYMA(recConfig)) || EQ(inTag, SYMA(dictionaries)))
		PurgeAreaCache();
	else if (EQ(inTag, SYMA(viewFormat)))
		viewFormat = RINT(inValue);

	setContextSlot(inTag, inValue);

	if (EQ(inTag, SYMA(viewBounds))
	 || EQ(inTag, SYMA(viewFormat))
	 ||(EQ(inTag, SYMA(viewJustify)) && invalidateSlotCache(kIndexViewJustify))
	 || EQ(inTag, SYMA(viewFont)))
		sync();
	changed(inTag);
}

void
CView::setContextSlot(RefArg inTag, RefArg inValue)
{
	SetFrameSlot(fContext, inTag, inValue);
}


/* -------------------------------------------------------------------------------
	Set a slot in the data frame.
	Args:		inTag			the slot name
				inValue		the value to set
	Return:	--
------------------------------------------------------------------------------- */

void
CView::setDataSlot(RefArg inTag, RefArg inValue)
{
	SetFrameSlot(dataFrame(), inTag, inValue);
}


/* -------------------------------------------------------------------------------
	Return the data frame.
	Args:		--
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::dataFrame(void)
{
	Ref	data = getCacheProto(kIndexRealData);
	return NOTNIL(data) ? data : (Ref)fContext;
}


/* -------------------------------------------------------------------------------
	Return the view at the top of the view hierarchy.
	This corresponds to the 'window' this view is in.
	Args:		--
	Return:	CView *
------------------------------------------------------------------------------- */

CView *
CView::getWindowView(void)
{
	CView * view = this;
	while (!view->hasVisRgn())
		view = view->fParent;
	return view;
}


/* -------------------------------------------------------------------------------
	Return a writeable copy of a variable.
	Args:		inTag			variable’s slot
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getWriteableVariable(RefArg inTag)
{
	// see if it’s a proto variable
	RefVar var(getWriteableProtoVariable(inTag));
	if (ISNIL(var))
	{
		// no, get the value
		var = getVar(inTag);
		if (NOTNIL(var))
		{
			// clone it to make it writeable and cache it
			var = Clone(var);
			setDataSlot(inTag, var);
		}
	}
	return var;
}


/* -------------------------------------------------------------------------------
	Return a writeable copy of a proto variable.
	Args:		inTag			variable’s slot
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getWriteableProtoVariable(RefArg inTag)
{
	// see if we already have a writeable copy
	RefVar var(GetFrameSlot(dataFrame(), inTag));
	if (ISNIL(var))
	{
		// no, get the proto value
		var = getProto(inTag);
		if (NOTNIL(var))
		{
			// clone it to make it writeable and cache it
			var = Clone(var);
			setDataSlot(inTag, var);
		}
	}
	return var;
}


/* -------------------------------------------------------------------------------
	Return the view’s copy protection.
	Args:		--
	Return:	ULong
------------------------------------------------------------------------------- */

ULong
CView::copyProtection(void)
{
	RefVar var(getVar(SYMA(copyProtection)));
	return NOTNIL(var) ? RINT(var) : cpNoCopyProtection;
}


/* -------------------------------------------------------------------------------
	Transfer the copy protection from another view to this.
	Args:		inView		the other view
	Return:	--
------------------------------------------------------------------------------- */

void
CView::transferCopyProtection(RefArg inView)
{
	ULong xfrProtection = cpNoCopyProtection;
	ULong selfProtection = copyProtection();
	if (selfProtection & cpReadOnlyCopies)
	{
		ULong roFlags = vNoFlags;
		RefVar vwFlags(GetProtoVariable(inView, SYMA(viewFlags)));
		if (ISNIL(vwFlags))
		{
			RefVar stny(GetProtoVariable(inView, SYMA(viewStationery)));
			if (NOTNIL(stny))
			{
				RefVar stdForms(GetGlobalVar(SYMA(stdForms)));
				stny = GetProtoVariable(stdForms, stny);
				if (NOTNIL(stny))
					vwFlags = GetProtoVariable(stny, SYMA(viewFlags));
			}
		}
		if (NOTNIL(vwFlags))
			roFlags = RINT(vwFlags);
		SetFrameSlot(inView, SYMA(viewFlags), MAKEINT(roFlags | vReadOnly));
		xfrProtection = cpReadOnlyCopies;
	}
	if (selfProtection & cpOriginalOnlyCopies)
		xfrProtection |= cpNoCopies;
	if (selfProtection & cpNewtonOnlyCopies)
		xfrProtection |= cpNewtonOnlyCopies;
	if (xfrProtection != cpNoCopyProtection)
		SetFrameSlot(inView, SYMA(copyProtection), MAKEINT(xfrProtection));
}


#pragma mark -
/* -------------------------------------------------------------------------------
	V i e w   m a n i p u l a t i o n
------------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Set a custom foreground pattern.
	This really is a pattern, not just a colour -- need to set it in CG.
	Args:		inName		the symbol of the pattern to use
	Return:	true always 
------------------------------------------------------------------------------- */

bool
CView::setCustomPattern(RefArg inName)
{
	bool			isDone;
//	CGColorRef	pattern;
	RefVar		patName(getVar(inName));
//	if (NOTNIL(patName) && GetPattern(patName, &isDone, &pattern, true))
//		SetFgPattern(pattern);
	return true;
}


/* -------------------------------------------------------------------------------
	Set new viewFlags.
	Args:		inFlags		the flags to set (in addition to what’s already set)
	Return:	--
------------------------------------------------------------------------------- */

void
CView::setFlags(ULong inFlags)
{
	viewFlags |= inFlags;
	if (FLAGTEST(inFlags, (vVisible | vSelected)))
	{
		RefVar	oldFlags(RINT(getCacheProto(kIndexViewFlags)));
		setContextSlot(SYMA(viewFlags), MAKEINT(oldFlags | inFlags));
	}
}


/* -------------------------------------------------------------------------------
	Clear viewFlags.
	Args:		inFlags		the flags to clear
	Return:	--
------------------------------------------------------------------------------- */

void
CView::clearFlags(ULong inFlags)
{
	viewFlags &= ~inFlags;
	if (FLAGTEST(inFlags, (vVisible | vSelected)))
	{
		RefVar	oldFlags(RINT(getCacheProto(kIndexViewFlags)));
		setContextSlot(SYMA(viewFlags), MAKEINT(oldFlags & ~inFlags));
	}
}


/* -------------------------------------------------------------------------------
	Set the view’s origin. All drawing within the view is with respect to this
	origin.
	Args:		inOrigin		the origin
	Return:	--
------------------------------------------------------------------------------- */

void
CView::setOrigin(Point inOrigin)
{
	RefVar	var;
	short		originX, originY;

	var = getCacheProto(kIndexViewOriginX);
	originX = ISNIL(var) ? 0 : RINT(var);

	var = getCacheProto(kIndexViewOriginY);
	originY = ISNIL(var) ? 0 : RINT(var);

	dirty();
	if (viewChildren != NULL)
	{
		Point		delta;
		delta.h = originX - inOrigin.h;
		delta.v = originY - inOrigin.v;

		CView *	childView;
		CListLoop iter(viewChildren);
		while ((childView = (CView *)iter.next()) != NULL)
		{
			if (childView->hasVisRgn())
				childView->offset(delta);
			else
				childView->simpleOffset(delta, 0);
		}
	}
	invalidateSlotCache(kIndexViewOriginX);
	setContextSlot(SYMA(viewOriginX), MAKEINT(inOrigin.h));

	invalidateSlotCache(kIndexViewOriginY);
	setContextSlot(SYMA(viewOriginY), MAKEINT(inOrigin.v));
}


/* -------------------------------------------------------------------------------
	Return the view’s local origin within its parent.
	Args:		--
	Return:	Point
------------------------------------------------------------------------------- */

Point
CView::localOrigin(void)
{
	Point		locOrigin = *(Point*)&viewBounds;
	Point		absOrigin = fParent->contentsOrigin();

	locOrigin.h -= absOrigin.h;
	locOrigin.v -= absOrigin.v;
	return locOrigin;
}


/* -------------------------------------------------------------------------------
	Return the view’s content’s origin.
	Args:		--
	Return:	Point
------------------------------------------------------------------------------- */

Point
CView::contentsOrigin(void)
{
	Point		cntOrigin = childOrigin();
	cntOrigin.h = viewBounds.left - cntOrigin.h;
	cntOrigin.v = viewBounds.top - cntOrigin.v;
	return cntOrigin;
}


/* -------------------------------------------------------------------------------
	Set up the view’s visible region.
	Args:		--
	Return:	the region
------------------------------------------------------------------------------- */

CRegion
CView::setupVisRgn(void)
{
	CRegionVar visRgn;
	visRgn.fRegion->setRegion(ViewPortRegion());

	for (CView * view = this; view != gRootView; view = view->fParent)
	{
		CClipper * clip = view->clipper();
		if (clip)
		{
			visRgn.fRegion->sectRegion(*clip->x04->fRegion);
//			SectRgn(visRgn, *clip->x04, visRgn);
			break;
		}
		if (FLAGTEST(viewFlags, vClickable))
		{
			CRectangularRegion tapRgn(view->viewBounds);
			visRgn.fRegion->sectRegion(tapRgn);
//			SectRgn(visRgn, tapRgn, visRgn);
		}
		visRgn.fRegion->diffRegion(*getFrontMask().fRegion);
//		DiffRgn(visRgn, getFrontMask(), visRgn);
	}

	return visRgn;
}


/* -------------------------------------------------------------------------------
	Determine whether we have a visible region.
	The answer’s true if we are a child of the root view.
	Args:		--
	Return:	bool
------------------------------------------------------------------------------- */

bool
CView::hasVisRgn(void)
{
	return fParent == gRootView && this != gRootView;
}


/* -------------------------------------------------------------------------------
	Determine whether this view and all its ancestors back to the root view are
	visible.
	Args:		--
	Return:	bool
------------------------------------------------------------------------------- */

bool
CView::visibleDeep(void)
{
	for (CView * view = this; view != gRootView; view = view->fParent)
		if (!FLAGTEST(view->viewFlags, vVisible))
			return false;
	return true;
}


/* -------------------------------------------------------------------------------
	Build a region encompassing this view and all its siblings.
	Args:		--
	Return:	the region
------------------------------------------------------------------------------- */

CRegion
CView::getFrontMask(void)
{
	CRegionVar theMask;
	theMask.fRegion->setEmpty();

	CView * view;
	CBackwardLoop loop(fParent->viewChildren);
	while ((view = (CView *)loop.next()) != NULL && view != this)
	{
		if (FLAGTEST(view->viewFlags, vVisible))
		{
			Rect bBox;
			if ((view->viewFormat & vfFillMask) != vfNone || view->hasVisRgn())
			{
				CClipper * clip = view->clipper();
				if (clip) {
					theMask.fRegion->unionRegion(*clip->fFullRgn->fRegion);
				} else {
					view->outerBounds(&bBox);
					CRectangularRegion rectRgn(bBox);
					theMask.fRegion->unionRegion(rectRgn);
				}
			}
			else if (view->viewChildren != NULL)
			{
				CView * childView;
				CListLoop iter(view->viewChildren);
				while ((childView = (CView *)iter.next()) != NULL)
				{
					if (FLAGTEST(view->viewFlags, vVisible) && (view->viewFormat & vfFillMask) != vfNone)
					{
						view->outerBounds(&bBox);
						CRectangularRegion rectRgn(bBox);
						theMask.fRegion->unionRegion(rectRgn);
					}
				}
			}
		}
	}
	return theMask;
}


/* -------------------------------------------------------------------------------
	Return this view’s clipper. Only children of the root view have a clipper.
	Args:		--
	Return:	CClipper
------------------------------------------------------------------------------- */

CClipper *
CView::clipper(void)
{
	if (hasVisRgn())
	{
		RefVar viewClipper(GetFrameSlot(fContext, SYMA(viewClipper)));
		if (NOTNIL(viewClipper))
			return (CClipper *)RefToAddress(viewClipper);
	}
	return NULL;
}


/* -------------------------------------------------------------------------------
	Dirty this view so that it will be redrawn.
	Args:		inBounds		rect within the view to dirty
	Return:	--
------------------------------------------------------------------------------- */

void
CView::dirty(const Rect * inBounds)
{
if (inBounds) printf("CView<%p>::dirty(inBounds={t:%d,l:%d,b:%d,r:%d})\n", this, inBounds->top, inBounds->left, inBounds->bottom, inBounds->right); else printf("CView<%p>::dirty(inBounds=NULL)\n", this);
if (inBounds != NULL && inBounds->top < 0)
	printf("huh?\n");
	if (visibleDeep())
	{
		// trivial rejection: this view is visible
		Rect	dirtyBounds;
		outerBounds(&dirtyBounds);
		if (inBounds)
			// clip our bounds to the specified rect -- this is dirty
			SectRect(inBounds, &dirtyBounds, &dirtyBounds);
		if (!EmptyRect(&dirtyBounds))
		{
			// trivial rejection: there is something dirty
			CView * view = this;
			CView * topView = NULL;
			while (!view->hasVisRgn())
			{
				if ((topView == NULL) && (view->viewFormat & vfFillMask) != vfNone)
					topView = view;
				view = view->fParent;
				if (FLAGTEST(view->viewFlags, vClipping))
					// clip the dirty bounds to the clipping view
					SectRect(&view->viewBounds, &dirtyBounds, &dirtyBounds);
				if (EmptyRect(&dirtyBounds))
					// trivial rejection
					return;
			}
			// at this point we hasVisRgn so there must be a clipper
			CClipper * clip = view->clipper();
			CRegionVar clippedDirtyRgn;
			CRectangularRegion dirtyRgn(dirtyBounds);
			clippedDirtyRgn.fRegion->setRegion(dirtyRgn);
			clippedDirtyRgn.fRegion->sectRegion(*clip->x04->fRegion);
			if (!clippedDirtyRgn.fRegion->isEmpty())
				gRootView->invalidate(*clippedDirtyRgn.fRegion, topView ? topView : view);
		}
	}
}


/* -------------------------------------------------------------------------------
	Show the view. There’s no animation but may be a sound.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::show(void)
{
	if (!FLAGTEST(viewFlags, vVisible))
	{
		gStrokeWorld.blockStrokes();
		bringToFront();

		CAnimate animation;
		animation.setupPlainEffect(this, true);

		setFlags(vVisible);
		if (hasVisRgn())
			fParent->viewVisibleChanged(this, true);
		else
			dirty();

		animation.doEffect(SYMA(showSound));

		gStrokeWorld.unblockStrokes();
		gStrokeWorld.flushStrokes();

		runScript(SYMA(viewShowScript), RA(NILREF));
	}
}


/* -------------------------------------------------------------------------------
	Hide the view. There’s no animation but may be a sound.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::hide(void)
{
	gStrokeWorld.blockStrokes();

	if (ViewContainsCaretView(this))
		gRootView->caretViewGone();
	gRootView->setPopup(this, false);

	CAnimate animation;
	animation.setupPlainEffect(this, false);

	runScript(SYMA(viewHideScript), RA(NILREF));

	if (hasVisRgn())
	{
		clearFlags(vVisible);
		fParent->viewVisibleChanged(this, true);
	}
	else
	{
		Rect	bounds;
		outerBounds(&bounds);
		clearFlags(vVisible);
		gRootView->smartInvalidate(&bounds);
	}

	animation.doEffect(SYMA(hideSound));

	gStrokeWorld.unblockStrokes();
	gStrokeWorld.flushStrokes();
}


/* -------------------------------------------------------------------------------
	Set the view bounds.
	Args:		inBounds		the bounds
	Return:	--
------------------------------------------------------------------------------- */

void
CView::setBounds(const Rect * inBounds)
{
	RefVar		justification;

	viewBounds = *inBounds;
	justification = getCacheProto(kIndexViewJustify);
	viewJustify = NOTNIL(justification) ? RINT(justification) : 0;
	justifyBounds(&viewBounds);

	CClipper * clip = clipper();
	if (clip)
	{
		Rect bounds;
		outerBounds(&bounds);		// sic - doesn’t appear to have any effect
		clip->updateRegions(this);
		fParent->viewVisibleChanged(this, false);
	}
}


/* -------------------------------------------------------------------------------
	Apply the view’s justification to some bounds.
	Args:		ioBounds		the bounds
	Return:	--
------------------------------------------------------------------------------- */

void
CView::justifyBounds(Rect * ioBounds)
{
	if (this != fParent) // don’t justify root view bounds
	{
		Rect	bounds = fParent->viewBounds;
		if (EmptyRect(&fParent->viewBounds) && (fParent->viewFlags & vIsInSetupForm))
		{
			RefVar	boundsFrame(getProto(SYMA(viewBounds)));
			if (!FromObject(boundsFrame, &bounds))
				ThrowMsg("bad bounds frame");
			fParent->justifyBounds(&bounds);
		}

		Point	offsetOrigin, origin;
		origin = *(Point *)&bounds;
		offsetOrigin = fParent->childOrigin();
		origin.v -= offsetOrigin.v;
		origin.h -= offsetOrigin.h;

		ULong justify;
		if ((viewFlags & vIsInSetupForm) != 0
		&&  (viewFlags & 0x90000000) != 0x90000000)
		{
			RefVar	vwJustify(getCacheProto(kIndexViewJustify));
			justify = NOTNIL(vwJustify) ? RINT(vwJustify) & vjEverything : vjLeftH + vjTopV;
		}
		else
			justify = viewJustify & vjEverything;

		if (justify & vjParentClip)
		{
			origin.v += offsetOrigin.v;
			origin.h = offsetOrigin.h;
		}
		
		bool needsJustifyH = true;
		bool needsJustifyV = true;
		ArrayIndex  index;
		if ((justify & vjSiblingMask) != 0
		&& ((index = fParent->viewChildren->getIdentityIndex(this)) != kIndexNotFound
		 || (index = fParent->viewChildren->count()) > 0)
		&&   index > 0)
		{
			CView *  sibling = (CView *)fParent->viewChildren->at(index - 1);
			ULong		siblingJustifyV = justify & vjSiblingVMask;
			if (siblingJustifyV != 0)
			{
				needsJustifyV = false;
				if (justify & (vjTopRatio + vjBottomRatio))
				{
					long  height = RectGetHeight(sibling->viewBounds);
					if (justify & vjTopRatio)
						ioBounds->top = ioBounds->top * height / 100;
					if (justify & vjBottomRatio)
						ioBounds->bottom = ioBounds->bottom * height / 100;
				}
				switch (siblingJustifyV)
				{
				case vjSiblingCenterV:
					origin.v += (RectGetHeight(sibling->viewBounds) - RectGetHeight(*ioBounds)) / 2;
					break;
				case vjSiblingBottomV:
					origin.v = sibling->viewBounds.bottom;
					break;
				case vjSiblingFullV:
					origin.v = 0;
					ioBounds->top += sibling->viewBounds.top;
					ioBounds->bottom += sibling->viewBounds.bottom;
					break;
				case vjSiblingTopV:
					origin.v = sibling->viewBounds.top;
					break;
				}
			}

			ULong		siblingJustifyH = justify & vjSiblingHMask;
			if (siblingJustifyH != 0)
			{
				needsJustifyH = false;
				if (justify & (vjLeftRatio + vjRightRatio))
				{
					long  width = RectGetWidth(sibling->viewBounds);
					if (justify & vjLeftRatio)
						ioBounds->left = ioBounds->left * width / 100;
					if (justify & vjRightRatio)
						ioBounds->right = ioBounds->right * width / 100;
				}
				switch (siblingJustifyH)
				{
				case vjSiblingCenterH:
					origin.h += (RectGetWidth(sibling->viewBounds) - RectGetWidth(*ioBounds)) / 2;
					break;
				case vjSiblingRightH:
					origin.h = sibling->viewBounds.right;
					break;
				case vjSiblingFullH:
					origin.h = 0;
					ioBounds->left += sibling->viewBounds.left;
					ioBounds->right += sibling->viewBounds.right;
					break;
				case vjSiblingLeftH:
					origin.h = sibling->viewBounds.left;
					break;
				}
			}
		}

		if (fParent == gRootView && this != gRootView)
		{
		// this is a child of the root view - justify WRT the entire display
			RefVar	displayParams(GetGlobalVar(SYMA(displayParams)));
			bounds.top = RINT(GetProtoVariable(displayParams, SYMA(appAreaGlobalTop)));
			bounds.left = RINT(GetProtoVariable(displayParams, SYMA(appAreaGlobalLeft)));
			origin = *(Point *)&bounds;
			bounds.bottom = bounds.top + RINT(GetProtoVariable(displayParams, SYMA(appAreaHeight)));
			bounds.right = bounds.left + RINT(GetProtoVariable(displayParams, SYMA(appAreaWidth)));
		}

		if (justify & (vjLeftRatio + vjRightRatio + vjTopRatio + vjBottomRatio))
		{
			if (needsJustifyH)
			{
				int  width = RectGetWidth(bounds);
				if (justify & vjLeftRatio)
					ioBounds->left = ioBounds->left * width / 100;
				if (justify & vjRightRatio)
					ioBounds->right = ioBounds->right * width / 100;
			}
			if (needsJustifyV)
			{
				int  height = RectGetHeight(bounds);
				if (justify & vjTopRatio)
					ioBounds->top = ioBounds->top * height / 100;
				if (justify & vjBottomRatio)
					ioBounds->bottom = ioBounds->bottom * height / 100;
			}
		}

		if (needsJustifyV)
		{
			switch (justify & vjParentVMask)
			{
			case vjParentCenterV:
				origin.v += (RectGetHeight(bounds) - RectGetHeight(*ioBounds)) / 2;
				break;
			case vjParentBottomV:
				origin.v += RectGetHeight(bounds);
				break;
			case vjParentFullV:
				origin.v = 0;
				ioBounds->top += bounds.top;
				ioBounds->bottom += bounds.bottom;
				break;
			}
		}

		if (needsJustifyH)
		{
			switch (justify & vjParentHMask)
			{
			case vjParentCenterH:
				origin.h += (RectGetWidth(bounds) - RectGetWidth(*ioBounds)) / 2;
				break;
			case vjParentRightH:
				origin.h += RectGetWidth(bounds);
				break;
			case vjParentFullH:
				origin.h = 0;
				ioBounds->left += bounds.left;
				ioBounds->right += bounds.right;
				break;
			}
		}

		OffsetRect(ioBounds, origin.h, origin.v);
	}
}


void
CView::dejustifyBounds(Rect * ioBounds)
{
	// like above, but in reverse
}


/* -------------------------------------------------------------------------------
	Return the view’s bounds including its frame.
	Args:		outBounds		the bounds
	Return:	--
------------------------------------------------------------------------------- */

void
CView::outerBounds(Rect * outBounds)
{
	*outBounds = viewBounds;
	if (viewFormat & vfPenMask)
	{
		// next section was OuterBounds1(outBounds, viewFormat);
		int	frameType = (viewFormat & vfFrameMask) >> vfFrameShift;
		int	penWd = (viewFormat & vfPenMask) >> vfPenShift;
		int	inset = ((viewFormat & vfInsetMask) >> vfInsetShift) + (frameType ? penWd : 0);
		int	shadowWd = (viewFormat & vfShadowMask) >> vfShadowShift;
		if (inset)
			InsetRect(outBounds, -inset, -inset);
		if (shadowWd)
		{
			outBounds->bottom += shadowWd;
			outBounds->right += shadowWd;
		}
		// end of OuterBounds1

		if (this == gRootView->defaultButtonView())
		{
			// allow for default button indicator
			outBounds->top -= 3;
			outBounds->bottom += 3;
		}
	}
}


void
CView::writeBounds(const Rect * inBounds)
{
	Rect  bounds = viewBounds;
	Point origin = fParent->contentsOrigin();
	origin.h = -origin.h;
	origin.v = -origin.v;
	OffsetRect(&bounds, origin.h, origin.v);
	if (!EqualRect(&bounds, inBounds))
	{
		if ((viewFlags & (vWriteProtected + vReadOnly)) == 0)
			setDataSlot(SYMA(viewBounds), ToObject(inBounds));
		setBounds(inBounds);
		if ((viewFlags & (vWriteProtected + vReadOnly)) == 0)
			changed(SYMA(viewBounds));
	}
}


/* -------------------------------------------------------------------------------
	Recalculate the view’s bounds from its viewBounds frame.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::recalcBounds(void)
{
	Rect		bounds;
	RefVar	boundsFrame(getProto(SYMA(viewBounds)));
	if (FromObject(boundsFrame, &bounds))
		setBounds(&bounds);

	CView *		childView;
	CListLoop	iter(viewChildren);
	while ((childView = (CView *)iter.next()) != NULL)
		childView->recalcBounds();
}


/* -------------------------------------------------------------------------------
	Scale the view’s bounds.
	Args:		inFromRect
				inToRect
	Return:	--
------------------------------------------------------------------------------- */

void
CView::scale(const Rect * inFromRect, const Rect * inToRect)
{
/*
	CTransform	xform;
	xform.setup(inFromRect, inToRect, false);
*/

	Rect	bounds = viewBounds;
/*
	CRect::Scale(&xform);

	RefVar	hilite(firstHilite());
	if (NOTNIL(hilite))
	{
		CHilite *	hili = RefToAddress(hilite);
		hili->fBounds.bottom = xform->f00 - bounds.bottom;
		hili->fBounds.right = bounds.right - bounds.left;
	}
*/
	Point		origin =fParent->contentsOrigin();
	OffsetRect(&bounds, -origin.h, -origin.v);

	writeBounds(&bounds);
}


/* -------------------------------------------------------------------------------
	Return the view’s origin (with respect to which children are located).
	Args:		outPt		the origin
	Return:	--
------------------------------------------------------------------------------- */

Point
CView::childOrigin(void)
{
	Point		pt;
	RefVar	coord(getCacheProto(kIndexViewOriginX));
	pt.h = NOTNIL(coord) ? RINT(coord) : 0;
	coord = getCacheProto(kIndexViewOriginY);
	pt.v = NOTNIL(coord) ? RINT(coord) : 0;
	return pt;
}


/* -------------------------------------------------------------------------------
	Move the view within its parent.
	Args:		inDelta		the amount to move
	Return:	--
------------------------------------------------------------------------------- */

void
CView::move(const Point * inDelta)
{
	Rect	newBounds = viewBounds;
	Point	origin = fParent->contentsOrigin();
	OffsetRect(&newBounds, -origin.h, -origin.v);
	OffsetRect(&newBounds, inDelta->h, inDelta->v);
	writeBounds(&newBounds);

	CView *		view;
	CListLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		view->recalcBounds();
	}
	fParent->dirty();
}


void
CView::doMoveCommand(Point inDelta)
{
	RefVar cmd(MakeCommand(aeMoveData, this, 0));
	CommandSetIndexParameter(cmd, 0, inDelta.h);
	CommandSetIndexParameter(cmd, 1, inDelta.v);
	gApplication->dispatchCommand(cmd);
}


void
CView::offset(Point inDelta)
{
	if (inDelta.h == 0 && inDelta.v == 0)
		return;

	bool	hasNoVisRgn = !hasVisRgn();
	if (hasNoVisRgn)
	{
		Rect	bounds;
		outerBounds(&bounds);
		fParent->dirty(&bounds);
	}
	simpleOffset(inDelta, false);

	if (hasNoVisRgn)
		dirty();
	else
		fParent->childViewMoved(this, inDelta);

	if (viewJustify & vIsModal)
		SetModalView(this);
}

void
CView::simpleOffset(Point inDelta, bool inAlways)
{
	if (!inAlways && (viewJustify & vjParentClip))
		return;

	OffsetRect(&viewBounds, inDelta.h, inDelta.v);

	CView *		view;
	CListLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		view->simpleOffset(inDelta, true);
	}
}

bool
CView::insideView(Point inPt)
{
	Rect	bounds;
	outerBounds(&bounds);
	return PtInRect(inPt, &bounds);
}

void
CView::sync(void)
{
	if ((viewFlags & vIsInSetupForm) == 0)
		setupForm();

	RefVar	obj(getProto(SYMA(viewBounds)));
	Rect		protoBounds;
	if (!FromObject(obj, &protoBounds))
		ThrowMsg("bad bounds frame");

	Rect		vwBounds = viewBounds;
	ULong		protoJustify;
	invalidateSlotCache(kIndexViewJustify);
	obj = getCacheProto(kIndexViewJustify);
	protoJustify = NOTNIL(obj) ? RINT(obj) : 0;
	viewJustify = (viewJustify & ~vjEverything) | (protoJustify & vjEverything);

	Rect		bounds = protoBounds;
	justifyBounds(&bounds);
	if (!EqualRect(&bounds, &vwBounds))
	{
		if (RectGetWidth(bounds) == RectGetWidth(vwBounds)
		&&  RectGetHeight(bounds) == RectGetHeight(vwBounds))
		{
		// we just moved
			Point pt;
			pt.h = bounds.left - vwBounds.left;
			pt.v = bounds.top - vwBounds.top;
			offset(pt);
		}
		else
		{
		// we changed size
			outerBounds(&vwBounds);
			fParent->dirty(&vwBounds);
			setBounds(&protoBounds);
			if (viewJustify & vIsModal)
				SetModalView(this);
			dirty();
		}

		CView *		view;
		CListLoop	iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
			view->recalcBounds();
	}
}

Ref
CView::syncScroll(RefArg inWhat, RefArg index, RefArg inupDown)
{ return NILREF;
}

Ref
CView::syncScrollSoup(RefArg inWhat, RefArg inUpDown)
{ return NILREF;
}

CView *
CView::addToSoup(RefArg inChild)
{
	CView *	view;

	RefVar	args(MakeArray(1));
	SetArraySlot(args, 0, inChild);
	RefVar childView(runScript(SYMA(viewAddChildScript), args));
	if (ISNIL(childView))
	{
		RefVar childViews(children());
		if (ISNIL(childViews))
		{
			childViews = MakeArray(1);
			SetArraySlot(childViews, 0, inChild);
			SetFrameSlot(dataFrame(), SYMA(viewChildren), childViews);
		}
		else
			AddArraySlot(childViews, inChild);
		addView(inChild);
	}
	else
		view = findView(IsFrame(childView) ? childView : inChild);

	return view;
}

CView *
CView::addChild(RefArg inContext)
{
	CView *	view;

	if ((view = Exists(viewChildren, inContext)) == NULL)
		view = BuildView(this, inContext);

	return view;
}

CView *
CView::addView(RefArg inView)
{
	RefVar	context(GetProtoVariable(inView, SYMA(preAllocatedContext)));
	if (ISNIL(context))
		context = buildContext(inView, false);
	else
	{
		context = getVar(context);		// read context from named slot
		if ((RINT(GetProtoVariable(context, SYMA(viewFlags))) & vVisible) == 0)
			return NULL;
	}
	if (ISNIL(context))
		return NULL;
	return BuildView(this, context);
}

void
CView::addCView(CView * inView)
{
	CView *  lastChild;

	if (viewChildren == gEmptyViewList)
		viewChildren = (CViewList *)CViewList::make();
	if ((inView->viewFlags & vFloating) != 0
	&& (lastChild = (CView *)viewChildren->last()) != NULL
	&& lastChild->viewFlags & vFloating)
	{
		viewChildren->insertFirst(inView);
		inView->bringToFront();
	}
	else
		viewChildren->insertLast(inView);
}

void
CView::addViews(bool inSync)
{
	bool  isInSetupForm = (viewFlags & vIsInSetupForm) != 0;
	if (!isInSetupForm)
		setFlags(vIsInSetupForm);

	runCacheScript(kIndexViewSetupChildrenScript, RA(NILREF));

	RefVar	viewKids(children());
	ArrayIndex	numOfViewKids = NOTNIL(viewKids) ? Length(viewKids) : 0;

	RefVar	stepKids(getProto(SYMA(stepChildren)));
	ArrayIndex	numOfChildren = numOfViewKids;
	if (NOTNIL(stepKids))
		numOfChildren += Length(stepKids);

	if (numOfChildren > 0)
	{
		RefVar	child;
		CView *  childView;
		if (viewChildren == gEmptyViewList)
			viewChildren = static_cast<CViewList *>(CViewList::make(numOfChildren));
		for (ArrayIndex i = 0; i < numOfChildren; ++i)
		{
			if (i < numOfViewKids)
				child = GetArraySlot(viewKids, i);
			else
				child = GetArraySlot(stepKids, i - numOfViewKids);
			if (!inSync || (childView = Exists(viewChildren, child)) == NULL)
				childView = addView(child);
			if (childView != NULL && inSync)
				childView->setFlags(vIsMarked);
			if (i > 0
			&& (viewJustify & vjReflow) != 0
			&& (childView->viewBounds.bottom - viewBounds.bottom) > RectGetHeight(viewBounds))
			{
			// child is off the bottom of this view so remove it
				SetLength(stepKids, i - numOfViewKids);
				i = numOfChildren;
				removeChildView(childView);
			}
		}

		if (inSync)
		{
			removeUnmarked();

			CView *		view;
			CListLoop	iter(viewChildren);
			while ((view = (CView *)iter.next()) != NULL)
				view->sync();
		}
	}

	if (!isInSetupForm)
		clearFlags(vIsInSetupForm);
}


/* -------------------------------------------------------------------------------
	Remove this view from its parent.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::removeView(void)
{
	fParent->removeChildView(this);
}


/* -------------------------------------------------------------------------------
	Remove a child of this view.
	Args:		inView		the child to be removed
	Return:	--
------------------------------------------------------------------------------- */

void
CView::removeChildView(CView * inView)
{
	if (FLAGTEST(inView->viewFlags, vVisible) && FLAGTEST(inView->viewFlags, vIsInSetupForm))
	{
		if (inView->hasVisRgn())
			inView->hide();
		else
		{
			Rect	bounds;
			inView->outerBounds(&bounds);
			dirty(&bounds);
		}
	}

	ArrayIndex index = viewChildren->getIdentityIndex(inView);
	if (index != kIndexNotFound)
	{
		viewChildren->removeElementsAt(index, 1);
		if (viewChildren->isEmpty())
		{
			delete viewChildren;
			viewChildren = gEmptyViewList;
		}
	}

	inView->dispose();
}


/* -------------------------------------------------------------------------------
	Remove all children of this view.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::removeAllViews(void)
{
	if (viewChildren != gEmptyViewList)
	{
		bool	r8 = ((viewFlags & 0x90000000) != 0x90000000);
		if (!r8)
			setFlags(0x90000000);

		while (!viewChildren->isEmpty())
		{
			CView * view = (CView *)viewChildren->at(0);
			viewChildren->removeElementsAt(0, 1);
			view->dispose();
		}

		if (!r8)
			clearFlags(0x90000000);

		if (viewChildren != gEmptyViewList && viewChildren != NULL)
		{
			delete viewChildren;
			viewChildren = gEmptyViewList;
		}
	}
}

void
CView::removeFromSoup(CView * inView)
{
	RefVar args(MakeArray(1));
	RefVar viewFrame(inView->dataFrame());
	SetArraySlot(args, 0, viewFrame);

	inView->removeView();

	if (ISNIL(runScript(SYMA(viewDropChildScript), args)))
	{
		RefVar kids(children());
		if (NOTNIL(kids))
			ArrayRemove(kids, viewFrame);
	}
}

void
CView::removeUnmarked(void)
{
	CView *		view;
	CListLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		if (view->viewFlags & vIsMarked)
			view->clearFlags(vIsMarked);
		else
		{
			iter.removeCurrent();
			view->dispose();
		}
	}
}


void
CView::reorderView(CView * inView, int index)
{
	int viewIndex = viewChildren->getIdentityIndex(inView);
	int nonFloaterIndex = GetFirstNonFloater(viewChildren);
	if (FLAGTEST(inView->viewFlags, vFloating)
	||  FLAGTEST(inView->viewJustify, vIsModal))
	{
		int	childIndex = viewChildren->count() - 1;	// from the end backwards
		nonFloaterIndex++;
		if (nonFloaterIndex < index)
			nonFloaterIndex = index;
		if (nonFloaterIndex > childIndex)
			nonFloaterIndex = childIndex;
		index = nonFloaterIndex;
		if (this == gRootView)
		{
			if (nonFloaterIndex == childIndex
			&&  childIndex > 0)
				childIndex--;
			do
			{
				if (gRootView->getClipboard((CView *)viewChildren->at(childIndex)) == NULL)
					break;
				if (childIndex > 0)
					index--;
			}
			while (childIndex-- > 0);
		}
	}
	else
	{
		int	validIndex = (index < 0) ? 0 : index;
		index = (validIndex < nonFloaterIndex) ? validIndex : nonFloaterIndex;
	}

	if (viewIndex == index)
		return;

	viewChildren->remove(inView);
	viewChildren->insertAt(index, inView);

	if (!FLAGTEST(inView->viewFlags, vVisible))
		return;

	// INCOMPLETE!
	// calculate the region invalidated by the new arrangement, and invalidate it
	// we can take a brute-force approach
	gRootView->dirty();
}


void
CView::bringToFront(void)
{
	fParent->reorderView(this, viewChildren->count());
}


CView *
CView::frontMost(void)
{
	if (FLAGTEST(viewFlags, vVisible) && FLAGTEST(viewFlags, vApplication))
	{
		CView * view, * frontMostView;
		CBackwardLoop iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
			if ((frontMostView = view->frontMost()) != NULL)
				return frontMostView;
		return this;
	}
	return NULL;
}


CView *
CView::frontMostApp(void)
{
	if (FLAGTEST(viewFlags, vVisible) && FLAGTEST(viewFlags, vApplication) && !FLAGTEST(viewFlags, vFloating))
	{
		CView * view, * frontMostView;
		CBackwardLoop iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
			if ((frontMostView = view->frontMostApp()) != NULL)
				return frontMostView;
		return this;
	}
	return NULL;
}


Ref
CView::children(void)
{
	return getProto(SYMA(viewChildren));
}


int
CView::childrenHeight(ArrayIndex * outNumOfChildren)
{
	int			height = 0;
	ArrayIndex	i = 1;
	CView *		view;
	CListLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		height += (view->viewBounds.bottom - view->viewBounds.top);
		++i;
	}
	if (outNumOfChildren)
		*outNumOfChildren = i;
	return height;
}


Ref
CView::childViewFrames(void)
{
	ArrayIndex numOfKids = viewChildren->count();
	RefVar kids = MakeArray(numOfKids);
	if (numOfKids > 0)
	{
		ArrayIndex	i = 0;
		CView *		view;
		CListLoop	iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
			SetArraySlot(kids, i++, view->fContext);
	}
	return kids;
}


/* -----------------------------------------------------------------------------
	A child view’s position has changed.
	Invalidate display regions as necessary.
	Args:		inView		the view that has moved
								MUST be a child of ours
				inDelta		offset
	Return:	--
----------------------------------------------------------------------------- */

void
CView::childViewMoved(CView * inView, Point inDelta)
{
	if (!inView->hasVisRgn())
		return;

	CRegionVar foregroundMask;
	foregroundMask.fRegion->setEmpty();
	CRegionVar dirty;
	dirty.fRegion->setEmpty();

	// iterate over views, front-back
	bool isBehind = false;
	CView * view;
	CBackwardLoop iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		if (!isBehind)
			isBehind = (view == inView);
		if (FLAGTEST(view->viewFlags, vVisible))
		{
			CClipper * clip = view->clipper();
			if (view == inView)
			{
				// this is the view that moved
				CRectangularRegion viewArea(clip->fFullRgn->fRegion->bounds());
				// add its extent to the region that needs redrawing
				dirty.fRegion->unionRegion(viewArea);
				// update its position
				clip->offset(inDelta);
			}
			if (isBehind)
			{
				clip->recalcVisible(*dirty.fRegion);
				if (view == inView)
				{
					// get the new extent now that the view has moved
					CRectangularRegion viewArea(clip->fFullRgn->fRegion->bounds());
					// that too will need redrawing
					dirty.fRegion->unionRegion(viewArea);
					// but not the views in front of it
					dirty.fRegion->diffRegion(*foregroundMask.fRegion);
					gRootView->invalidate(*dirty.fRegion, NULL);
				}
			}
			// this view is now part of the foreground of views further back
			foregroundMask.fRegion->unionRegion(*clip->fFullRgn->fRegion);
		}
	}
}

void
CView::childBoundsChanged(CView * inView, Rect * inBounds)
{ /* this really does nothing */ }

void
CView::moveChildBehind(CView * inView, CView * inBehindView)
{
	int viewIndex = viewChildren->getIdentityIndex(inView);
	int behindIndex = inBehindView != NULL ? viewChildren->getIdentityIndex(inBehindView) : 0;
	if (viewIndex > behindIndex || --behindIndex > viewIndex)
		reorderView(inView, behindIndex);
}

int
CView::setChildrenVertical(int inTop, int inSpacing)
{
	Point origin = contentsOrigin();
	int y = inTop;
	CView * view;
	CListLoop iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		Rect viewBox = view->viewBounds;
		OffsetRect(&viewBox, -origin.h, -origin.v);
		int viewHt = RectGetHeight(viewBox);
		if (y > viewBox.top)
			viewBox.top = y;
		viewBox.bottom = viewBox.top + viewHt;
		view->setBounds(&viewBox);
		y = viewBox.top + viewHt + inSpacing;
	}
	return y;
}


CView *
CView::findView(RefArg inView)
{
	if (SoupEQ(dataFrame(), inView))
		return this;

	if ((viewFlags & vVisible) == 0)
		return NULL;

	CView * view;
	CListLoop iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		if ((view = view->findView(inView)) != NULL)
			return view;
	}
}

CView *
CView::findView(Point inPt, ULong inRecognitionReqd, Point * inSlop)
{
	int  distance;
	bool  isClickable;
	return findClosestView(inPt, inRecognitionReqd, &distance, inSlop, &isClickable);
}


CView *
CView::findClosestView(Point inPt, ULong inRecognitionReqd, int * outDistance, Point * inSlop, bool * outIsClickable)
{
	*outIsClickable = false;
	CView * theView = NULL;
	int theDistance;
	if ((theDistance = distance(inPt, inSlop)) != kDistanceOutOfRange)
	{
		if (FLAGTEST(viewFlags, vClickable))
			*outIsClickable = true;
		if (inRecognitionReqd == 0  ||  (inRecognitionReqd & (viewFlags & vRecognitionAllowed)) != 0)
		{
			theView = this;
			*outDistance = theDistance;
		}
		// see if we can narrow it down to one of our children
		int closestDistance = kDistanceOutOfRange;
		CView * view;
		CBackwardLoop iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
		{
			int  distance = kDistanceOutOfRange;
			bool  isClickable;
			CView * aView = view->findClosestView(inPt, inRecognitionReqd, &distance, inSlop, &isClickable);
			if (distance < closestDistance)
			{
				closestDistance = *outDistance = distance;
				theView = aView;
			}
			if (isClickable)
			{
				*outIsClickable = true;
				break;
			}
		}
	}
	return theView;
}

int
CView::distance(Point inPt, Point * inSlop)
{
	int theDistance = kDistanceOutOfRange;
	if (FLAGTEST(viewFlags, vVisible))
	{
		CClipper * clip = clipper();
		if (clip) {
			if (clip->x04->fRegion->contains(inPt))
				// trivial rejection -- pt is in the clipping region of the view
				return 0;
		} else if (inSlop != NULL) {
			Rect bounds;
			outerBounds(&bounds);
			InsetRect(&bounds, -inSlop->h, -inSlop->v);
			if (PtInRect(inPt, &bounds))
				theDistance = CheapDistance(MidPoint(&bounds), inPt);
		} else {
			if (insideView(inPt))
				theDistance = 0;
		}
	}
	return theDistance;
}

void
CView::narrowVisByIntersectingObscuringSiblingsAndUncles(CView * inView, Rect * inBounds)
{
	if (inView == NULL)
		inView = gRootView;
#if 0
	//sp-1C
	r8 = sp00 = ViewPortRegion();
	CRegionVar sp04;
	sp04.fRegion->setEmpty();
	//sp-08
	Rect bBox;
	for (CView * view = this; view != inView; view = view->fParent)
	{
		//sp-08
		CView * siblingView;	//r6
		CBackwardLoop iter(view->fParent->viewChildren);
		while ((siblingView = (CView *)iter.next()) != NULL)
		{
			if (inBounds == NULL || (siblingView->outerBounds(&bBox), Intersects(&bBox, inBounds)))
			{
				if ((siblingView->viewFormat & vfFillMask) != vfNone || siblingView->hasVisRgn())
				{
					CClipper * clip = siblingView->clipper();
					if (clip) {
						r8.fRegion->diffRegion(clip->fFullRgn->fRegion);
					} else {
						siblingView->outerBounds(&bBox);
						CRectangularRegion rectRgn(&bBox);
						r8.fRegion->diffRegion(&rectRgn);
					}
					;
				}
			}
		}
	}
#endif
}


/* -----------------------------------------------------------------------------
	A child view’s visibility has changed.
	Invalidate display regions as necessary.
	Args:		inView		the view that has become (in)visible
								MUST be a child of ours
				inVisible	true => is now visible
	Return:	--
----------------------------------------------------------------------------- */

void
CView::viewVisibleChanged(CView * inView, bool inVisible)
{
	CRegionVar foregroundMask;

	Rect bBox;
	bBox.left = -32767;
	bBox.top = -32767;
	bBox.right = 32767;
	bBox.bottom = 32767;
	CRectangularRegion wideOpenRgn(bBox);
	foregroundMask.fRegion->setRegion(wideOpenRgn);

	outerBounds(&bBox);
	CRectangularRegion boundsRgn(bBox);
	foregroundMask.fRegion->diffRegion(boundsRgn);		// foregroundMask is the universe EXCEPT our bounds => clip to this view

	// iterate over views, front-back
	bool isBehind = false;
	CView * view;
	CBackwardLoop iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		if (!isBehind)
		{
			isBehind = (view == inView);
			if (isBehind && inVisible)
			{
				CClipper * clip = inView->clipper();
	//			//sp-24
	//			CRegionVar sp10;
	//			//sp-04
	//			CRectangularRegion viewArea(clip->fFullRgn->fRegion->bounds());	//sp04
	//			CRectangularRegion * sp00 = &viewArea;
	//			DiffRgn(&viewArea, foregroundMask, sp10);
	//			//sp+04
	//			gRootView->invalidate(sp10, FLAGTEST(view->viewFlags, vVisible)? view : this);

				CRegionVar dirty;
				// get the extent of the view that is now visible
				CRectangularRegion viewArea(clip->fFullRgn->fRegion->bounds());
				// it all needs redrawing
				dirty.fRegion->setRegion(viewArea);
				// but not the views in front of it
				dirty.fRegion->diffRegion(*foregroundMask.fRegion);
				gRootView->invalidate(*dirty.fRegion, FLAGTEST(view->viewFlags, vVisible)? view : this);
			}
		}
		if (FLAGTEST(view->viewFlags, vVisible))
		{
			CClipper * clip = view->clipper();
			if (clip)
			{
				if (isBehind)
					clip->recalcVisible(*foregroundMask.fRegion);
				// this view is now part of the foreground of views further back
				foregroundMask.fRegion->unionRegion(*clip->fFullRgn->fRegion);
			}
		}
	}
}


Ref
CView::getRangeText(long inStart, long inEnd)
{
	return NILREF;
}

bool
CView::isGridded(RefArg inGrid, Point * outSpacing)
{
	if (!EQ(inGrid, getProto(SYMA(viewGrid))))
		return false;

	if (outSpacing)
	{
		Ref	viewSpacing = getProto(SYMA(viewLineSpacing));
		long  spacing = NOTNIL(viewSpacing) ? RINT(viewSpacing) : 1;
		outSpacing->v = spacing;
		outSpacing->h = EQ(inGrid, SYMA(lineGrid)) ? 0 : spacing;
	}
	return true;
}

void
CView::changed(RefArg inTag)
{
	changed(inTag, fContext);
}

void
CView::changed(RefArg inTag, RefArg inContext)
{
	if (FrameHasSlot(fContext, SYMA(viewTie)))
	{
		RefVar	args(MakeArray(2));
		SetArraySlot(args, 0, fContext);
		SetArraySlot(args, 1, inTag);
		RefVar	ties(GetFrameSlot(fContext, SYMA(viewTie)));
		ArrayIndex	count = Length(ties);
		for (ArrayIndex i = 0; i < count; i += 2)
		{
			RefVar	msg(GetArraySlot(ties, i+1));
			RefVar	rcvr(GetArraySlot(ties, i));
			DoMessage(rcvr, msg, args);
		}
	}

	RefVar	args(MakeArray(2));
	SetArraySlot(args, 0, inTag);
	SetArraySlot(args, 1, inContext);

	bool hasNoScripts = (viewFlags & vNoScripts) != 0;
	clearFlags(vNoScripts);
	runCacheScript(kIndexViewChangedScript, args, true);
	if (hasNoScripts)
		setFlags(vNoScripts);

	dirty();
}


#pragma mark Hilites
/* -----------------------------------------------------------------------------
	H i l i t i n g
----------------------------------------------------------------------------- */

bool
CView::addHiliter(CUnit * unit)
{
}


void
CView::handleHilite(CUnit * unit, long inArg2, bool inArg3)
{
}


void
CView::globalHiliteBounds(Rect * inBounds)
{
}


void
CView::globalHilitePinnedBounds(Rect * inBounds)
{
}


void
CView::globalHiliteResizeBounds(Rect * inBounds)
{
}


/* -------------------------------------------------------------------------------
	Highlight this view.
	Args:		isOn			or not
	Return:	--
------------------------------------------------------------------------------- */

void
CView::hilite(bool isOn)
{
	if (visibleDeep())
	{
		bool			isCaretOn;
//		CRegion		vr = setupVisRgn();
//		CRegionVar	viewVisRgn(vr);
		Rect			caretBounds = gRootView->getCaretRect();
		Rect			bounds;
		outerBounds(&bounds);
		if ((isCaretOn = Overlaps(&caretBounds, &bounds)))	// intentional assignment
			gRootView->hideCaret();

		unwind_protect
		{
			if (NOTNIL(getCacheProto(kIndexViewHiliteScript)))
			{
				RefVar	args(MakeArray(1));
				if (isOn)
					SetArraySlot(args, 0, RA(TRUEREF));
				if (NOTNIL(runCacheScript(kIndexViewHiliteScript, args)))
				{
					bounds = viewBounds;
					int	inset = (viewFormat & vfInsetMask) >> vfInsetShift;
					int	rounding = (viewFormat & vfRoundMask) >> vfRoundShift;
					InsetRect(&bounds, -inset, -inset);
		/*			if (rounding)
					{
						int	penWd = (viewFormat & vfPenMask) >> vfPenShift;
						if (penWd > 1)
							rounding = MAX(0, rounding - (penWd-1));
						if (rounding)
						{
							rounding *= 2;
							InvertRoundRect(&bounds, rounding, rounding);
						}
						else
							InvertRect(&bounds);
					}
					else
						InvertRect(&bounds);
			*/	}
			}
		}
		on_unwind
		{
			if (isCaretOn)
				gRootView->showCaret();
//			viewVisRgn.fRegion->setRegion(ViewPortRegion());
		}
		end_unwind;
	}
}


/* -------------------------------------------------------------------------------
	Highlight everything.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::hiliteAll(void)
{
	removeAllHilites();
	CHilite *	hili = new CHilite;
	if (hili == NULL)
		OutOfMemory();

	Rect	bounds = viewBounds;
	hili->fBounds.top = 0;
	hili->fBounds.left = 0;
	hili->fBounds.bottom = viewBounds.bottom - viewBounds.top;
	hili->fBounds.right = viewBounds.right - viewBounds.left;

	RefVar	cmd(MakeCommand(aeHilite, this, 0x08000000));
	RefVar	obj(AddressToRef(hili));
	CommandSetFrameParameter(cmd, obj);
	gApplication->dispatchCommand(cmd);
}


/* -------------------------------------------------------------------------------
	Return this view’s highlights.
	Args:		--
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::hilites(void)
{
	return getProto(SYMA(hilites));
}


/* -------------------------------------------------------------------------------
	Return this view’s first highlight.
	Args:		--
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::firstHilite(void)
{
	RefVar lites(hilites());
	if (NOTNIL(lites) && Length(lites) != 0)
		return GetArraySlot(lites, 0);
	return NILREF;	
}


/* -------------------------------------------------------------------------------
	Determine whether this view is highlighted.
	Args:		--
	Return:	bool
------------------------------------------------------------------------------- */

bool
CView::hilited(void)
{
	RefVar lites(hilites());
	return NOTNIL(lites) && Length(lites) != 0;
}


/* -------------------------------------------------------------------------------
	Delete all highlighted data.
	Args:		RefArg		dunno
	Return:	--
------------------------------------------------------------------------------- */

void
CView::deleteHilited(RefArg ignored)
{
	RefVar	cmd(MakeCommand(aeRemoveData, fParent, fId));
	gApplication->dispatchCommand(cmd);
}

	// •••
void
CView::removeHilite(RefArg inArg1)
{
}


void
CView::removeAllHilites(void)
{
}


/* -------------------------------------------------------------------------------
	Determine whether this view’s completely hilited.
	You could say the view is ALL content data, so we’ll always say true.
	Args:		inArg			dunno
	Return:	true, always
------------------------------------------------------------------------------- */

bool
CView::isCompletelyHilited(RefArg inArg)
{
	return true;
}


bool
CView::pointInHilite(Point inPt)
{
	Point localPt;
	localPt.v = inPt.v - viewBounds.top;
	localPt.h = inPt.h - viewBounds.left;

//	CHiliteLoop iter(this);
//	while (iter.next())
//	{
//		if (PtInRect(localPt, iter.box())
//			return true;
//	}
	return false;
}


/* -------------------------------------------------------------------------------
	Draw this view’s hiliting.
	Nothing to do here in the base class.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::drawHiliting(void)
{ }


/* -------------------------------------------------------------------------------
	Draw this view’s hilites.
	Nothing to do here in the base class.
	Args:		isOn			or not
	Return:	--
------------------------------------------------------------------------------- */

void
CView::drawHilites(bool isOn)
{ }


/* -------------------------------------------------------------------------------
	Draw this view’s hilited content data.
	You could say the view is ALL content data, so we’ll draw the whole lot.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::drawHilitedData(void)
{
	draw(viewBounds);
}


#pragma mark Text
/* -----------------------------------------------------------------------------
	T e x t
----------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Return this view’s text style.
	If it doesn’t have a viewFont return the global userFont.
	Args:		--
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getTextStyle(void)
{
	RefVar	style(getProto(SYMA(viewFont)));
	if (ISNIL(style))
	{
		if (FLAGTEST(viewFlags, vReadOnly))
			style = getVar(SYMA(viewFont));
		else
			style = GetPreference(SYMA(userFont));
	}
	return style;
}


/* -------------------------------------------------------------------------------
	Return this view’s text style record.
	Args:		outRec		style record to be filled in
	Return:	--
------------------------------------------------------------------------------- */

void
CView::getTextStyleRecord(StyleRecord * outRec)
{
	CreateTextStyleRecord(getTextStyle(), outRec);
}


#pragma mark Gestures
/* -----------------------------------------------------------------------------
	G e s t u r e s
----------------------------------------------------------------------------- */

ULong
CView::clickOptions(void)
{
	return 0x01;
}


int
CView::handleScrub(const Rect * inScrubbedRect, int inArg2, CUnit * unit, bool inArg4)
{
	if (!FLAGTEST(viewFlags, vWriteProtected|vReadOnly)
	&& (inArg2 == 5 || inArg2 == -1)
	/*&& CoveredBy(viewBounds, inScrubbedRect) > 75*/)
		return 5;
	return 0;
}


#pragma mark Input
/* -----------------------------------------------------------------------------
	I n p u t
	Nothing much to do here.
----------------------------------------------------------------------------- */

void
CView::setCaretOffset(int * ioX, int * ioY)
{ }

void
CView::offsetToCaret(int inArg1, Rect * outRect)
{
	outRect->top = outRect->bottom = -32768;
}

void
CView::pointToCaret(Point inPt, Rect * outRect, Rect * inArg3)
{
	outRect->top = outRect->bottom = -32768;
}

ULong
CView::textFlags(void) const
{
	RefVar flags(getProto(SYMA(textFlags)));
	if (ISINT(flags))
		return RINT(flags);
	return 0;
}


#pragma mark Keyboard
/* -----------------------------------------------------------------------------
	K e y b o a r d
----------------------------------------------------------------------------- */


Ref
FInRepeatedKeyCommand(RefArg inRcvr)
{
	return MAKEBOOLEAN(gInRepKeyCmd);
}


void
CView::buildKeyChildList(CViewList * keyList, long inArg2, long inArg3)
{
	CView *		view;
	CListLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		if (FLAGTEST(view->viewFlags, vVisible))
		{
			view->buildKeyChildList(keyList, inArg2, inArg3);
			if (!FLAGTEST(view->viewFlags, vReadOnly))
			{
#if defined(correct)
				if (inArg3 == 0)
				{
					if (FLAGTEST(view->textFlags(), vTakesAllKeys) || view->protoedFrom(RA(protoInputLine)))
						keyList->insertLast(view);
				}
				else if (inArg3 == 1)
				{
					if (view->derivedFrom(clParagraphView) || view->protoedFrom(RA(protoStaticText)))
						keyList->insertLast(view);
				}
#endif
			}
		}
	}
}


CView *
CView::nextKeyView(CView * inView, long inArg2, long inArg3)
{ return NULL; }

void
CView::handleKeyEvent(RefArg inArg1, ULong inArg2, bool * inArg3)
{}

void
CView::doEditCommand(long inArg)
{}


#pragma mark Selection
/* -----------------------------------------------------------------------------
	S e l e c t i o n
----------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Select this view.
	Set the flag and hilite as appropriate.
	Args:		isOn			select as opposed to deselect
				isUnique		if true then deselect everything else in the parent
	Return:	--
------------------------------------------------------------------------------- */

void
CView::select(bool isOn, bool isUnique)
{
	if (isUnique)
		fParent->selectNone();

	if (isOn)
	{
		if (!(viewFlags & vSelected))
		{
			setFlags(vSelected);
			hilite(true);
		}
		
	}
	else
	{
		if (viewFlags & vSelected)
		{
			clearFlags(vSelected);
			hilite(false);
		}
	}
}


/* -------------------------------------------------------------------------------
	Deselect all this view’s children.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::selectNone(void)
{
	CView *		view;
	CListLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		if (view->viewFlags & vSelected)
		{
			view->hilite(false);
			view->clearFlags(vSelected);
		}
	}
}


/* -------------------------------------------------------------------------------
	Set this view’s content selection.
	This really does nothing - plain views have no content.
	It will be overridden in more interesting views.
	Args:		inInfo
				ioX			possibly out args actually
				ioY
	Return:	--
------------------------------------------------------------------------------- */

void
CView::setSelection(RefArg inInfo, int * ioX, int * ioY)
{ }


/* -------------------------------------------------------------------------------
	Return this view’s content selection.
	This really returns NULL - plain views have no content.
	It will be overridden in more interesting views.
	Args:		--
	Return:	Ref
------------------------------------------------------------------------------- */

Ref
CView::getSelection(void)
{
	return NILREF;
}


/* -------------------------------------------------------------------------------
	Activate this view’s content selection.
	This does nothing - plain views have no content - except run the
	viewCaretActivateScript so that more interesting views can call this inherited
	method.
	Args:		--
	Return:	Ref
------------------------------------------------------------------------------- */

void
CView::activateSelection(bool isOn)
{
	RefVar	args(MakeArray(1));
	SetArraySlot(args, 0, isOn ? RA(TRUEREF) : RA(NILREF));

	runCacheScript(kIndexViewCaretActivateScript, args);
}

#pragma mark -

/* -----------------------------------------------------------------------------
	S o u n d   E f f e c t s
----------------------------------------------------------------------------- */

void
CView::soundEffect(RefArg inTag)
{
	RefVar	snd(getVar(inTag));
	if (NOTNIL(snd))
		FPlaySound(fContext, snd);
}


#pragma mark Drawing
/* -----------------------------------------------------------------------------
	D r a w i n g
----------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Update this view on screen.
	Args:		inUpdateRgn		update region
				inView			child view from which to update
									NULL => update this view
	Return:	--
------------------------------------------------------------------------------- */

void
CView::update(CBaseRegion& inUpdateRgn, CView * inView)
{
//	GrafPtr thePort;
//	GetPort(&thePort);
//	RgnHandle clipRgn = port->clipRgn;
//	CRegionVar savedClipRgn;
//	CopyRgn(clipRgn, savedClipRgn);
	CGContextSaveGState(quartz);
	newton_try
	{
//		clip to inUpdateRgn
//		SectRgn(clipRgn, inUpdateRgn, clipRgn);
		CGContextClipToRect(quartz, MakeCGRect(inUpdateRgn.bounds()));
		CView * view = (inView != NULL) ? inView : this;
//		if ((view->viewFormat & vfFillMask) == vfNone)
//			EraseRgn(clipRgn);

		if (inView == NULL || inView == gRootView)
			draw(inUpdateRgn);
		else
		{
			Rect  bounds = inUpdateRgn.bounds();
			CView * view = inView;
			CView * parent = view->fParent;
			for ( ; ; )
			{
				if (view)
					// draw the view and all its later siblings
					parent->drawChildren(inUpdateRgn, view);
				parent->postDraw(bounds);
				if (parent == this || parent == gRootView)
					break;
				// else step back up the parent chain
				view = parent;
				parent = view->fParent;
				// try the next sibling
				CViewList * kids = parent->viewChildren;
				view = (CView *)kids->at(kids->getIdentityIndex(view) + 1);
			}
		}
	}
	newton_catch(exRootException)
	{ }
	end_try;
//	restore clipping region
//	GetPort(&thePort);
//	clipRgn = port->clipRgn;
//	CopyRgn(savedClipRgn, clipRgn);
	CGContextRestoreGState(quartz);
}


/* -------------------------------------------------------------------------------
	Draw this view, clipped to a Rect.
	Args:		inBounds		clipping bounds
				inForPrint
	Return:	--
------------------------------------------------------------------------------- */

void
CView::draw(Rect& inRect, bool inForPrint)
{
	CRectangularRegion rectRgn(inRect);
	draw(rectRgn, inForPrint);
}


/* -------------------------------------------------------------------------------
	Draw this view.
	Args:		inUpdateRgn		update region
				inForPrint
	Return:	--
------------------------------------------------------------------------------- */

void
CView::draw(CBaseRegion& inUpdateRgn, bool inForPrint)
{
	if (!FLAGTEST(viewFlags, vVisible) || FLAGTEST(viewFlags, vIsInSetup2))
		if (inForPrint == false)
			return;

printf("CView<%p>::draw(region={t:%d,l:%d,b:%d,r:%d})\n", this, inUpdateRgn.bounds().top, inUpdateRgn.bounds().left, inUpdateRgn.bounds().bottom, inUpdateRgn.bounds().right);

	Rect  updateBounds = inUpdateRgn.bounds();
	Rect  bBox;
	outerBounds(&bBox);
	if (RectsOverlap(&updateBounds, &bBox))
	{
		// this view IS somewhere in the update extent
		bool needsUpdate;
		CClipper * clip = clipper();
		if (clip != NULL && !gSkipVisRegions)
		{
			if (!gIsClippedRgnInitialised)
			{
				gIsClippedRgnInitialised = true;
				gClippedRgn = new CRegionStruct;
				// push destructor?
				g0C104F64 = gClippedRgn;
			}
			gClippedRgn->fRegion->setRegion(*clip->x04->fRegion);
			gClippedRgn->fRegion->sectRegion(inUpdateRgn);
//			SectRgn(clip->x04, inUpdateRgn, gClippedRgn);
			needsUpdate = !gClippedRgn->fRegion->isEmpty();
		}
		else
			// more specifically: is this view actually in the update region
			needsUpdate = inUpdateRgn.intersects(&bBox);
		if (needsUpdate)
		{
/*			CRegionStruct *  savedClipRgn = NULL;	//sp04
			CRegionStruct *  sp00 = NULL;
			if (!gSkipVisRegions)
			{
				CRegion	rgn;
				if (hasVisRgn())
				{
					rgn = setupVisRgn();
					sp00 = rgn->stealRegion();
				}
			}
*/
			unwind_protect
			{
/*				GrafPtr  thePort;
				GetPort(&thePort);
				if (savedClipRgn == NULL || !EmptyRgn(thePort->clipRgn))
*/				{
					if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
						StartDrawing(NULL, NULL);

					preDraw(updateBounds);

					if (FLAGTEST(viewFlags, vClipping))
					{
/*						// save the port’s clipRgn
						savedClipRgn = new CRegionStruct;
						GrafPtr thePort;
						GetPort(&thePort);
						CopyRgn(thePort->clipRgn, savedClipRgn);

						// clip the port’s clipRgn to this view’s bounds
						GetPort(&thePort);
						RgnHandle sp008 = thePort->clipRgn;
						CRectangularRegion viewRgn(viewBounds);
						SectRgn(sp008, viewRgn, sp008);
*/						CGContextSaveGState(quartz);
						CGContextClipToRect(quartz, MakeCGRect(viewBounds));
					}

					realDraw(updateBounds);

					if (fTagCacheMaskLo & kIndexViewDrawScript)
					{
						newton_try
						{
							runCacheScript(kIndexViewDrawScript, RA(NILREF));
						}
						newton_catch(exRootException)
						{ }
						end_try;
					}

					drawChildren(inUpdateRgn, NULL);

					if (FLAGTEST(viewFlags, vClipping))
					{
/*						// restore the port’s clipRgn
						GrafPtr thePort;
						GetPort(&thePort);
						RgnHandle theRgn = thePort->clipRgn;
						CopyRgn(savedClipRgn, theRgn);
						delete savedClipRgn, savedClipRgn = NULL;
*/						CGContextRestoreGState(quartz);
					}

					postDraw(updateBounds);

					if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
					{
						StopDrawing(NULL, NULL);
						Wait(gSlowMotion);
					}
				}
			}
			on_unwind
			{
/*				if (savedClipRgn)
				{
					GrafPtr thePort;
					GetPort(&thePort);
					RgnHandle theRgn = thePort->clipRgn;
					CopyRgn(savedClipRgn, theRgn);
					delete savedClipRgn;
				}
				if (sp00)
				{
					GrafPtr thePort;
					GetPort(&thePort);
					RgnHandle theRgn = thePort->clipRgn;
					CopyRgn(sp00, theRgn);
					delete sp00;
				}
*/			}
			end_unwind;
		}
	}

	if (gOutlineViews)
	{
		SetPattern(vfGray);
		SetLineWidth(2);
		StrokeRect(viewBounds);
	}
}


/* -------------------------------------------------------------------------------
	Draw this view’s children, clipped to a Rect.
	-- This is never called. --
	Args:		inRect		area of interest; NULL => draw it all
				inView		child view from which to start drawing
								NULL => draw them all
	Return:	--
------------------------------------------------------------------------------- */

void
CView::drawChildren(Rect& inRect, CView * inView)
{
	CRectangularRegion rectRgn(inRect);
	drawChildren(rectRgn, inView);
}


/* -------------------------------------------------------------------------------
	Draw this view’s children.
	Args:		inUpdateRgn		update region
				inView			child view from which to start drawing
									NULL => draw them all
	Return:	--
------------------------------------------------------------------------------- */

void
CView::drawChildren(CBaseRegion& inUpdateRgn, CView * inView)
{
	if (viewChildren)
	{
		newton_try
		{
			CView *		view;
			CListLoop	iter(viewChildren);
			while ((view = (CView *)iter.next()) != NULL)
			{
				if (inView)
				{
					if (inView == view)
					{
						inView = NULL;
						view->draw(inUpdateRgn);
					}
					// else do nothing until we reach the specified child view
				}
				else
					view->draw(inUpdateRgn);
			}
		}
		newton_catch(exRootException)
		{ }
		end_try;
	}
}


/* -------------------------------------------------------------------------------
	Draw the view’s grid or line pattern, before any other drawing takes place.
	Args:		inRect			area of interest; NULL => draw it all
	Return:	--
------------------------------------------------------------------------------- */

void
CView::preDraw(Rect& inRect)
{
	if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
		StartDrawing(NULL, NULL);

	if (FLAGTEST(viewFormat, vfLinesMask|vfFillMask))
	{
		Rect	frameBounds = viewBounds;
		int	frameType = (viewFormat & vfFrameMask) >> vfFrameShift;
		int	penWd = (viewFormat & vfPenMask) >> vfPenShift;
		int	inset = ((viewFormat & vfInsetMask) >> vfInsetShift) + (frameType ? penWd : 0);
		InsetRect(&frameBounds, -inset, -inset);

		ULong		patNo;
		patNo = viewFormat & vfFillMask; // vfFillShift is zero
		if (SetPattern(patNo))
		{
		// fill entire frame
			int rounding = (viewFormat & vfRoundMask) >> vfRoundShift;
			if (patNo == vfCustom)
				setCustomPattern(SYMA(viewFillPattern));
			FillRoundRect(frameBounds, rounding, rounding);
//			if (patNo == vfCustom)
//				DisposeFgPattern();
		}

		patNo = (viewFormat & vfLinesMask) >> vfLineShift;
		if (SetPattern(patNo))
		{
			int  linespacing;
			RefVar var(getProto(SYMA(viewLineSpacing)));
			if (NOTNIL(var) && (linespacing = RINT(var)) != 0)
			{
				Point origin;
				Point pt1, pt2;
				Rect bounds;
				int h, v;

				if (patNo == vfCustom)
					setCustomPattern(SYMA(viewLinePattern));
				origin = childOrigin();
				v = viewBounds.top + linespacing;
				if (origin.v != 0)
					v -= (origin.v % linespacing);
				bounds = frameBounds;
				SectRect(&inRect, &bounds, &bounds);
				SetLineWidth(1);
				var = getProto(SYMA(viewGrid));
				if (EQ(var, SYMA(squareGrid)))
				{
					for ( ; v <= bounds.bottom; v += linespacing)
					{
						if (v >= bounds.top)
						{
							SetPt(&pt1, viewBounds.left, v+1);
							SetPt(&pt2, viewBounds.right - 1, v+1);
							StrokeLine(pt1, pt2);
						}
					}
					for (h = bounds.left + linespacing; h <= bounds.right; h += linespacing)
					{
						SetPt(&pt1, h, viewBounds.top);
						SetPt(&pt2, h, viewBounds.bottom-1);
						StrokeLine(pt1, pt2);
					}
				}
				else
				{
					for ( ; v <= bounds.bottom; v += linespacing)
					{
						if (v >= bounds.top)
						{
							SetPt(&pt1, viewBounds.left, v+1);
							SetPt(&pt2, viewBounds.right - 1, v+1);
							StrokeLine(pt1, pt2);
						}
					}
				}
//				if (patNo == vfCustom)
//					DisposeFgPattern();
			}
		}
	}

	if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
	{
		StopDrawing(NULL, NULL);
		Wait(gSlowMotion);
	}
}


/* -------------------------------------------------------------------------------
	Draw the view’s content.
	A plain view has no content so we really do nothing here in the base class.
	Args:		inRect			area of interest; NULL => draw it all
	Return:	--
------------------------------------------------------------------------------- */

void
CView::realDraw(Rect& inRect)
{ }


/* -------------------------------------------------------------------------------
	Draw the view’s frame after its content has been drawn.
	Also indicate which button is default if we’re keyboard operated.
	Args:		inRect			area of interest; NULL => draw it all
	Return:	--
------------------------------------------------------------------------------- */

void
CView::postDraw(Rect& inRect)
{
	if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
		StartDrawing(NULL, NULL);

	if (FLAGTEST(viewFlags, vSelected))
		hilite(true);

	if (FLAGTEST(viewFormat, vfFrameMask|vfShadowMask))
	{
		Rect	frameBounds = viewBounds;
		int	frameType = (viewFormat & vfFrameMask) >> vfFrameShift;
		int	penWd = (viewFormat & vfPenMask) >> vfPenShift;
		int	shadowWd;
		int	inset = ((viewFormat & vfInsetMask) >> vfInsetShift) + (frameType ? penWd : 0);
		InsetRect(&frameBounds, -inset, -inset);

		int	rounding = (viewFormat & vfRoundMask) >> vfRoundShift;
		ULong patNo = (viewFormat & vfFrameMask) >> vfFrameShift;

		if (SetPattern(patNo))
		{
// for CG the shadow has to be set up prior to drawing -- this might be better done when filling the view in predraw()
CGContextSaveGState(quartz);
if ((shadowWd = (viewFormat & vfShadowMask) >> vfShadowShift) != 0)
CGContextSetShadow(quartz, CGSizeMake(0.0, -shadowWd), 10.0);
			if (frameType == vfCustom)
				setCustomPattern(SYMA(viewFramePattern));
			SetLineWidth(penWd);

			// frame the bounds rect in the specified pattern (gray)
			if (rounding == 0)
				StrokeRect(frameBounds);
			else
				StrokeRoundRect(frameBounds, rounding, rounding);

			if (frameType == vfMatte || frameType == vfDragger)
			{
			// always frame a dragger in black
				penWd = (this == gRootView->caretSlipView()) ? 4 : 2;
				SetLineWidth(penWd);
				SetPattern(vfBlack);
				if (rounding == 0)
					StrokeRect(frameBounds);
				else
					StrokeRoundRect(frameBounds, rounding, rounding);

				if (frameType == vfDragger)
				{
				// draw the dragger hook
					Rect		hookBounds;
					RefVar	hookPict(MAKEMAGICPTR(691));
					FromObject(GetFrameSlot(hookPict, SYMA(bounds)), &hookBounds);
					OffsetRect(&hookBounds, frameBounds.left + ((frameBounds.right - frameBounds.left) - (hookBounds.right - hookBounds.left))/2, frameBounds.top + 2);
					DrawPicture(hookPict, &hookBounds, vjLeftH+vjTopV, modeMask);
				}
			}
//			if (frameType == vfCustom)
//				DisposeFgPattern();
CGContextRestoreGState(quartz);
		}
#if 0
		if ((penWd = (viewFormat & vfShadowMask) >> vfShadowShift) != 0)
		{
		// draw  a drop shadow
/*			PenNormal();
			PenSize(pen, pen);
			MoveTo(frameBounds.right,			frameBounds.top + pen);
			LineTo(frameBounds.right,			frameBounds.bottom);
			MoveTo(frameBounds.left + pen,	frameBounds.bottom);
			LineTo(frameBounds.right,			frameBounds.bottom);
*/
			Point pt1, pt2;
			SetLineWidth(penWd);
			SetPattern(vfGray);
			SetPt(&pt1, frameBounds.right + penWd/2, frameBounds.top + penWd);
			SetPt(&pt2, frameBounds.right + penWd/2, frameBounds.bottom + penWd/2);
			StrokeLine(pt1, pt2);
			SetPt(&pt1, frameBounds.left, frameBounds.bottom + penWd/2);
			StrokeLine(pt1, pt2);
		}
#endif
		if (this == gRootView->defaultButtonView())
		{
		// draw default keyboard button hilite
			Rect		btnBounds = frameBounds;
			RefVar	btnBoundsObj(getProto(SYMA(_defaultButtonBounds)));
			if (NOTNIL(btnBoundsObj))
			{
				FromObject(btnBoundsObj, &btnBounds);
				OffsetRect(&btnBounds, viewBounds.left, viewBounds.top);
			}
			InsetRect(&btnBounds, 2, 0);
			SetPattern(vfBlack);
	/*		PenSize(1,1);
			MoveTo(btnBounds.left,			btnBounds.top - 2);
			LineTo(btnBounds.right - 1,	btnBounds.top - 2);
			MoveTo(btnBounds.left + 1,		btnBounds.top - 3);
			LineTo(btnBounds.right - 2,	btnBounds.top - 3);
			MoveTo(btnBounds.left,			btnBounds.bottom + 1);
			LineTo(btnBounds.right - 1,	btnBounds.bottom + 1);
			MoveTo(btnBounds.left + 1,		btnBounds.bottom + 2);
			LineTo(btnBounds.right - 2,	btnBounds.bottom + 2);
	*/
			Point pt1, pt2;
			SetLineWidth(1);

			SetPt(&pt1, btnBounds.left,		btnBounds.top - 2);
			SetPt(&pt2, btnBounds.right - 1,	btnBounds.top - 2);
			StrokeLine(pt1, pt2);
			SetPt(&pt1, btnBounds.left + 1,	btnBounds.top - 3);
			SetPt(&pt2, btnBounds.right - 2,	btnBounds.top - 3);
			StrokeLine(pt1, pt2);
			SetPt(&pt1, btnBounds.left,		btnBounds.bottom + 1);
			SetPt(&pt2, btnBounds.right - 1,	btnBounds.bottom + 1);
			StrokeLine(pt1, pt2);
			SetPt(&pt1, btnBounds.left + 1,	btnBounds.bottom + 2);
			SetPt(&pt2, btnBounds.right - 2,	btnBounds.bottom + 2);
			StrokeLine(pt1, pt2);
		}
	}

	if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
	{
		StopDrawing(NULL, NULL);
		Wait(gSlowMotion);
	}
}


void
CView::drawScaledData(const Rect * inSrcBounds, const Rect * inDstBounds, Rect * ioBounds)
{
	if (hilited())
	{
/*
		CTransform	xform;
		xform.setup(inSrcBounds, inDstBounds, false);
		*ioBounds = viewBounds;
		ScaleRect(ioBounds, &xform);	// was CRect::Scale

void
ScaleRect(Rect * ioBounds, const CTransform & inTransform)
{
	ScalePoint((Point*)&ioBounds->top, inTransform);
	ScalePoint((Point*)&ioBounds->bottom, inTransform);
}

void
ScalePoint(Point * ioPoint, const CTransform & inTransform)
{
	INCOMPLETE
}
*/
	}
	else
	{
		ioBounds->top =
		ioBounds->bottom = -32768;
	}
}


#pragma mark Drag & Drop
/* -----------------------------------------------------------------------------
	D r a g   a n d   D r o p
----------------------------------------------------------------------------- */
extern Ref MakePoint(Point inPt);

/* -----------------------------------------------------------------------------
	Add drag info to this view.
	Args:		inDragInfo		array of frames (one frame per dragged item)
	Return:	true => info added
----------------------------------------------------------------------------- */

bool
CView::addDragInfo(CDragInfo * inDragInfo)
{
	bool isDone;
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inDragInfo->f00);
	RefVar result(runScript(SYMA(viewAddDragInfoScript), args, true, &isDone));
	return isDone && NOTNIL(result);
}


/* -----------------------------------------------------------------------------
	Snap drag point to nearest grid, if specified.
----------------------------------------------------------------------------- */
int
AlignToGrid(int inOrd, int inSpacing)
{
	if (inSpacing != 0)
		return ((inOrd + inSpacing/2)/inSpacing) * inSpacing;
	return inOrd;
}

void
AlignPtToGrid(Point * ioPt, Point inSpacing)
{
	ioPt->h = AlignToGrid(ioPt->h, inSpacing.h);
	ioPt->v = AlignToGrid(ioPt->v, inSpacing.v);
}

void
AlignRectToGrid(Rect * ioRect, Point inSpacing)
{
	ioRect->top = AlignToGrid(ioRect->top, inSpacing.v);
	ioRect->left = AlignToGrid(ioRect->left, inSpacing.h);
	ioRect->bottom = AlignToGrid(ioRect->bottom, inSpacing.v);
	ioRect->right = AlignToGrid(ioRect->right, inSpacing.h);
}


void
CView::alignDragPtToGrid(const CDragInfo * inDragInfo, Point * ioPt)
{
	Point spacing;
	if (isGridded(SYMA(squareGrid), &spacing)) {
		AlignPtToGrid(ioPt, spacing);
		return;
	}

	if (isGridded(SYMA(lineGrid), &spacing))
	{
		for (ArrayIndex i = Length(inDragInfo->f00); i > 0; --i)
		{
			if (NOTNIL(inDragInfo->findType(i, SYMA(text)))) {
				AlignPtToGrid(ioPt, spacing);
				return;
			}
		}
	}
}


/* -----------------------------------------------------------------------------
	.
----------------------------------------------------------------------------- */

int
CView::dragAndDrop(CStroke * inStroke, const Rect * inBounds, const Rect * inArg3, const Rect * inArg4, bool inArg5, const CDragInfo * inDragInfo, const Rect * inArg7)
{
//r4: r6 r5 r7 r10 r8 r9
	BusyBoxSend(55);
	inStroke->inkOff(true);
	if (FLAGTEST(copyProtection(), cpNoCopies))
		return 0;
	//sp-20
	Point sp1C;
	Point sp18;
	bool sp14;
	bool sp10;
	int r7 = drag(inDragInfo, inStroke, inBounds, inArg3, inArg7, inArg5, &sp1C, &sp18, &sp14, &sp10);
	//INCOMPLETE
}


/* -----------------------------------------------------------------------------
	.
----------------------------------------------------------------------------- */

void
CView::drag(CStroke * inStroke, const Rect * inBounds)
{
	BusyBoxSend(55);
	inStroke->inkOff(true);
	//INCOMPLETE
}


/* -----------------------------------------------------------------------------
	.
----------------------------------------------------------------------------- */

int
CView::drag(const CDragInfo * inDragInfo, CStroke * inStroke, const Rect * inArg3, const Rect * inArg4, const Rect* inArg5, bool inArg6, Point * inArg7, Point * inArg8, bool * inArg9, bool * inArg10)
{
}


/* -----------------------------------------------------------------------------
	.
----------------------------------------------------------------------------- */

void
CView::endDrag(const CDragInfo * inDragInfo, CView * inArg2, const Point * inArg3, const Point * inArg4, const Point * inArg5, bool inArg6)
{
}


/* -----------------------------------------------------------------------------
	dragFeedbackScript
	Allows a view to give visual feedback while items are dragged over it.
	Args:		inDragInfo		array of frames (one frame per dragged item)
				inPt				current pen point (global coord)
				inShow			true => show the feedback
	Return:	false => no drawing done, don’t call us again
----------------------------------------------------------------------------- */

bool
CView::dragFeedback(const CDragInfo * inDragInfo, const Point & inPt, bool inShow)
{
	RefVar args(MakeArray(3));
	SetArraySlot(args, 0, inDragInfo->f00);
	SetArraySlot(args, 1, MakePoint(inPt));
	SetArraySlot(args, 2, MAKEBOOLEAN(inShow));
	return NOTNIL(runScript(SYMA(viewDragFeedbackScript), args, true));
}


/* -----------------------------------------------------------------------------
	viewDrawDragBackgroundScript
	If supplied, this method draws the image that appears behind the dragged data.
	The default (if this method is missing or if it returns nil) is to use
	the bitmap of the area inside the rectangle defined by bounds XORed with the
	bitmap from ViewDrawDragDataScript. Note that the XOR happens only if copy is nil.
	This method returns a Boolean value.
	Returning non-nil means that this method handled the drawing.
	Returning nil means that the default behavior should take place.
	Args:		inBounds			bounds passed to DragAndDrop
				inCopy			copy passed to DragAndDrop
	Return:	--
----------------------------------------------------------------------------- */

bool
CView::drawDragBackground(const Rect * inBounds, bool inCopy)
{
//	EraseRect(inBounds);
	RefVar args(MakeArray(2));
	SetArraySlot(args, 0, ToObject(inBounds));
	SetArraySlot(args, 1, MAKEBOOLEAN(inCopy));
	return NOTNIL(runScript(SYMA(viewDrawDragBackgroundScript), args, true));
}


/* -----------------------------------------------------------------------------
	viewDrawDragDataScript
	If supplied, this method draws the image that will be dragged.
	The default (if this method is missing) is to use the area of the screen
	inside the rectangle defined by bounds parameter to DragAndDrop.
	This method returns a Boolean value.
	Returning non-nil means that this method handled the drawing.
	Returning nil means that the default behavior should take place.
	Args:		inBounds			bounds passed to DragAndDrop
	Return:	--
----------------------------------------------------------------------------- */

void
CView::drawDragData(const Rect * inBounds)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, ToObject(inBounds));
	if (ISNIL(runScript(SYMA(viewDrawDragDataScript), args, true)))
		drawHilitedData();
}


/* -----------------------------------------------------------------------------
	.
----------------------------------------------------------------------------- */

void
CView::getClipboardDataBits(Rect * inArea)
{
}


/* -----------------------------------------------------------------------------
	viewDropApproveScript
	Provides a way for the view to disallow dropping onto a particular view.
	ViewDropApproveScript returns nil if the drop shouldn’t happen, and non-nil
	if the drop should happen. It is called only if the drop types match up
	with the dragged data and the destView, and is called right before the
	ViewDropScript, ViewDropMoveScript and/or ViewDropRemoveScript methods are called.
	Args:		inTarget				destination view
	Return:	true => the drop should happen
----------------------------------------------------------------------------- */

bool
CView::dropApprove(CView * inTarget)
{
	bool isDone;
	RefVar args(MakeArray(1));
	if (inTarget != NULL)
		SetArraySlot(args, 0, inTarget->fContext);
	RefVar approval(runScript(SYMA(viewDropApproveScript), args, true, &isDone));
	if (!isDone)
		// script failed => allow drop anyway
		approval = TRUEREF;
	return NOTNIL(approval);
}


/* -----------------------------------------------------------------------------
	viewGetDropTypesScript
	Returns an array of symbols; that is, the data types accepted by the view
	at the location currentPoint. For example, 'text or 'picture.
	The array is sorted by priority (preferred type first).
	This method can return nil, meaning no drop is allowed at the current point.
	Args:		inPt				current pen point (global coord)
	Return:	array or boolean
----------------------------------------------------------------------------- */

Ref
CView::getSupportedDropTypes(Point inPt)
{
	bool isDone;
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, MakePoint(inPt));
	RefVar dropTypes(runScript(SYMA(viewGetDropTypesScript), args, true, &isDone));
	if (NOTNIL(dropTypes))
		return dropTypes;
	return MAKEBOOLEAN(isDone);
}


/* -----------------------------------------------------------------------------
	Lets the destination view redirect the drop to a different view.
	Args:		inDragInfo		array of frames (one frame per dragged item)
				inPt				last stroke point (global coord)
	Return:	target view
----------------------------------------------------------------------------- */

CView *
CView::findDropView(const CDragInfo & inDragInfo, Point inPt)
{ /* this really does nothing */ return NULL; }


/* -----------------------------------------------------------------------------
	Accept the dropped data.
	Args:		inDragInfo		array of frames (one frame per dragged item)
				inPt				last stroke point (global coord)
	Return:	target view
----------------------------------------------------------------------------- */

bool
CView::acceptDrop(const CDragInfo & inDragInfo, Point inPt)
{
	RefVar dropTypes(getSupportedDropTypes(inPt));
	if (IsArray(dropTypes))
		return inDragInfo.checkTypes(dropTypes);
	return false;
}


/* -----------------------------------------------------------------------------
	viewFindTargetScript
	Lets the destination view redirect the drop to a different view.
	ViewFindTargetScript returns a view frame of the view that gets the drop messages.
	It is called right after the ViewGetDropTypesScript.
	Args:		inDragInfo		array of frames (one frame per dragged item)
				inPt				last stroke point (global coord)
	Return:	target view
----------------------------------------------------------------------------- */
CView *
FindDropViewDeep(CView * inView, const CDragInfo & inDragInfo, Point inPt)
{
	do {
		if (!FLAGTEST(inView->viewFlags, vWriteProtected+vReadOnly)
		&& inView->acceptDrop(inDragInfo, inPt)) {
			return inView;
		}
	} while ((inView->viewFormat & vfFillMask) == vfNone && !inView->hasVisRgn() && (inView = inView->fParent) != gRootView);
	return NULL;
}

CView *
CView::targetDrop(const CDragInfo & inDragInfo, Point inPt)
{
	CView * view;
	if ((view = gRootView->findView(inPt, vAnythingAllowed, NULL)) != NULL
	&&  (view = FindDropViewDeep(view, inDragInfo, inPt)) != NULL
	&&  (view = view->findDropView(inDragInfo, inPt)) != NULL)
	{
		bool isDone;
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, inDragInfo.f00);
		RefVar target(view->runScript(SYMA(viewFindTargetScript), args, true, &isDone));
		if (isDone)
			view = ISNIL(target)? NULL : GetView(target);
	}
	return view;
}


/* -----------------------------------------------------------------------------
	viewGetDropDataScript
	Called when a destination view that accepts all the dragged items is found.
	ViewGetDropDataScript is called for each item being dragged.
	Args:		inDragType		type dest view wants
				inDragRef		drag reference for item
	Return:	data frame
----------------------------------------------------------------------------- */

Ref
CView::getDropData(RefArg inDragType, RefArg inDragRef)
{
	RefVar args(MakeArray(2));
	SetArraySlot(args, 0, inDragType);
	SetArraySlot(args, 1, inDragRef);
	return runScript(SYMA(viewGetDropDataScript), args, true);
}


/* -----------------------------------------------------------------------------
	viewDropScript
	This message is sent to the destination view for each dragged item.
	Args:		inDropType		type dest view wants
				inDropData		data from source view
				inDropPt			last stroke point (global coord)
	Return:	true => we accepted & handled the drop
----------------------------------------------------------------------------- */

bool
CView::drop(RefArg inDropType, RefArg inDropData, Point inDropPt)
{
	RefVar args(MakeArray(3));
	SetArraySlot(args, 0, inDropType);
	SetArraySlot(args, 1, inDropData);
	SetArraySlot(args, 2, MakePoint(inDropPt));
	return NOTNIL(runScript(SYMA(viewDropScript), args, true));
}


/* -----------------------------------------------------------------------------
	viewDropMoveScript
	This message is sent for each dragged item when dragging and dropping in
	the same view. (In this case, ViewGetDropDataScript and ViewDropScript
	messages are not sent.)
	Args:		inDragRef		drag reference for item
				inOffset			offset of the item
				inLastDragPt	last stroke point (global coord)
				inCopy
	Return:	true => we handled the move
----------------------------------------------------------------------------- */

bool
CView::dropMove(RefArg inDragRef, Point inOffset, Point inLastDragPt, bool inCopy)
{
	RefVar args(MakeArray(4));
	SetArraySlot(args, 0, inDragRef);
	SetArraySlot(args, 1, MakePoint(inOffset));
	SetArraySlot(args, 2, MakePoint(inLastDragPt));
	SetArraySlot(args, 3, MAKEBOOLEAN(inCopy));
	return NOTNIL(runScript(SYMA(viewDropMoveScript), args, true));
}


/* -----------------------------------------------------------------------------
	viewDropRemoveScript
	This message is sent for each dragged item when the copy parameter to
	DragAndDrop is nil.
	Args:		inDragRef		drag reference for item
	Return:	--
----------------------------------------------------------------------------- */

void
CView::dropRemove(RefArg inDragRef)
{
	RefVar args(MakeArray(1));
	SetArraySlot(args, 0, inDragRef);
	runScript(SYMA(viewDropRemoveScript), args, true);
}


/* -----------------------------------------------------------------------------
	viewDropDoneScript
	Sent at the very end of each drag and drop to let the destination view know
	that all specified items have been dropped or moved.
	Args:		--
	Return:	boolean result of script
----------------------------------------------------------------------------- */

bool
CView::dropDone(void)
{
	return NOTNIL(runScript(SYMA(viewDropDoneScript), RA(NILREF), true));
}


#pragma mark Scripts
/* -----------------------------------------------------------------------------
	S c r i p t s
----------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------------
	Run the viewSetupFormScript.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::setupForm(void)
{
	runScript(SYMA(viewSetupFormScript), RA(NILREF));
}


/* -------------------------------------------------------------------------------
	Run the viewSetupDoneScript.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
CView::setupDone(void)
{
	runScript(SYMA(viewSetupDoneScript), RA(NILREF));
}


/* -------------------------------------------------------------------------------
	Run a Newton script.
	Args:		inTag			script name
				inArgs		argument array
				inSelf		as opposed to in proto
				outDone		whether script was actually run (maybe bad parameters
								prevented it)
	Return:	Ref			result of the script
------------------------------------------------------------------------------- */

Ref
CView::runScript(RefArg inTag, RefArg inArgs, bool inSelf, bool * outDone)
{
	bool isDone = false;
	Ref  result = NILREF;

	if (!FLAGTEST(viewFlags, vNoScripts))
	{
		if (NOTNIL(inSelf ? getVar(inTag) : getProto(inTag)))
		{
			result = inSelf ? DoMessage(fContext, inTag, inArgs)
								 : DoProtoMessage(fContext, inTag, inArgs);
			isDone = true;
		}
	}
	if (outDone != NULL)
		*outDone = isDone;
	return result;
}


/* -------------------------------------------------------------------------------
	Run a Newton script. Access the script name via index into the tag cache
	to save lookup overhead.
	Args:		index			tag cache index of the script
				inArgs		argument array
				inSelf		as opposed to in proto
				outDone		whether script was actually run (maybe bad parameters
								prevented it)
	Return:	Ref			result of the script
------------------------------------------------------------------------------- */

Ref
CView::runCacheScript(ArrayIndex index, RefArg inArgs, bool inSelf, bool * outDone)
{
	bool isDone = false;
	Ref  result = NILREF;

	if (!FLAGTEST(viewFlags, vNoScripts))
	{
		ULong * bits;
		if (index < 32)
			bits = &fTagCacheMaskLo;
		else
		{
			index -= 32;
			bits = &fTagCacheMaskHi;
		}
		if ((*bits & (1 << index)) && NOTNIL(inSelf ? getCacheVariable(index) : getCacheProto(index)))
		{
			result = inSelf ? DoMessage(fContext, gTagCache[index], inArgs)
								 : DoProtoMessage(fContext, gTagCache[index], inArgs);
			isDone = true;
		}
	}
	if (outDone != NULL)
		*outDone = isDone;
	return result;
}


#pragma mark - Debug
/* -----------------------------------------------------------------------------
	D e b u g
----------------------------------------------------------------------------- */

Ref
GetNameFromDebugHash(RefArg inHash)
{
	RefVar	unhashFunc;
	RefVar	name;
	RefVar	args(MakeArray(1));

	if (NOTNIL(inHash))
	{
		unhashFunc = GetFrameSlot(gFunctionFrame, SYMA(DebugHashToName));
		if (NOTNIL(unhashFunc))
		{
			SetArraySlot(args, 0, inHash);
			name = DoBlock(unhashFunc, args);
		}
		else if (ISINT(inHash))
		{
			UniChar  str[20];
			IntegerString(RVALUE(inHash), str);
			name = MakeString(str);
		}
	}

	return name;
}

#define DUMPVIEWFLAG(_f) if (viewFlags & _f) REPprintf(" " #_f)
#define DUMPTEXTFLAG(_f) if (txtFlags & _f) REPprintf(" " #_f)

void
CView::dump(ArrayIndex inLevel)
{
	newton_try
	{
		char			buf[4];
		char			nameBuf[64];
		buf[0] = 0;
		nameBuf[0] = 0;
		RefVar	debugName(getProto(SYMA(debug)));
		if (NOTNIL(debugName))
		{
			if (ISINT(debugName))
				debugName = GetNameFromDebugHash(debugName);
			ConvertFromUnicode(GetUString(debugName), nameBuf, 63);
		}
		for (ArrayIndex i = 0; i < inLevel; ++i)
			REPprintf("|");
		REPprintf("%-12.12s", nameBuf);
		for (ArrayIndex i = inLevel; i < 8; ++i)
			REPprintf(" ");
		REPprintf("%4s #%08X", buf, (Ref)fContext);

		// bounds
		REPprintf(" [%3d,%3d,%3d,%3d] %08X", viewBounds.left, viewBounds.top, viewBounds.right, viewBounds.bottom, viewFlags);

		// list view type flags
		DUMPVIEWFLAG(vVisible);
		DUMPVIEWFLAG(vWriteProtected);
		DUMPVIEWFLAG(vReadOnly);
		DUMPVIEWFLAG(vApplication);
		DUMPVIEWFLAG(vCalculateBounds);
		DUMPVIEWFLAG(vClipping);
		DUMPVIEWFLAG(vFloating);

		// list view recognition flags
		if ((viewFlags & vAnythingAllowed) == vAnythingAllowed)
			REPprintf(" vAnythingAllowed");
		else
		{
			DUMPVIEWFLAG(vClickable);
			DUMPVIEWFLAG(vStrokesAllowed);
			DUMPVIEWFLAG(vGesturesAllowed);
			DUMPVIEWFLAG(vCharsAllowed);
			DUMPVIEWFLAG(vNumbersAllowed);
			DUMPVIEWFLAG(vLettersAllowed);
			DUMPVIEWFLAG(vPunctuationAllowed);
			DUMPVIEWFLAG(vShapesAllowed);
			DUMPVIEWFLAG(vMathAllowed);
			DUMPVIEWFLAG(vPhoneField);
			DUMPVIEWFLAG(vDateField);
			DUMPVIEWFLAG(vTimeField);
			DUMPVIEWFLAG(vAddressField);
			DUMPVIEWFLAG(vNameField);
			DUMPVIEWFLAG(vCapsRequired);
			DUMPVIEWFLAG(vCustomDictionaries);
		}
		DUMPVIEWFLAG(vSelected);
		DUMPVIEWFLAG(vClipboard);
		DUMPVIEWFLAG(vNoScripts);
		DUMPVIEWFLAG(vHasIdlerHint);

		ULong txtFlags = textFlags();
		if (txtFlags != 0)
		{
			REPprintf("textFlags: %08X", txtFlags);
			DUMPTEXTFLAG(vWidthIsParentWidth);
			DUMPTEXTFLAG(vNoSpaces);
			DUMPTEXTFLAG(vWidthGrowsWithText);
			DUMPTEXTFLAG(vFixedTextStyle);
			DUMPTEXTFLAG(vFixedInkTextStyle);
			DUMPTEXTFLAG(vAlignToParentLineSpacing);
			DUMPTEXTFLAG(vNoTrackScale);
			DUMPTEXTFLAG(vAllowEmpty);
			DUMPTEXTFLAG(vKeepStylesArray);
			DUMPTEXTFLAG(vExpectingNumbers);
			DUMPTEXTFLAG(vSingleKeyStrokes);
			DUMPTEXTFLAG(vStandAloneBounds);
			DUMPTEXTFLAG(vAlwaysTryKeyCommands);
			DUMPTEXTFLAG(vCallStandardScripts);
			DUMPTEXTFLAG(vTakesCommandKeys);
			DUMPTEXTFLAG(vTakesAllKeys);
			DUMPTEXTFLAG(vTakesNoKeys);
		}
		REPprintf("\n");
		inLevel++;

		CView *		childView;
		CListLoop	iter(viewChildren);
		while ((childView = (CView *)iter.next()) != NULL)
		{
			newton_try
			{
				childView->dump(inLevel);
			}
			newton_catch(exRootException)
			{ }
			end_try;
		}
	}
	newton_catch(exRootException)
	{ }
	end_try;
}

#pragma mark -

/* -----------------------------------------------------------------------------
	C H i l i t e
----------------------------------------------------------------------------- */

CHilite::CHilite()
{ }

CHilite::~CHilite()
{ }

CHilite *
CHilite::clone(void)	// better with a copy constructor ??
{
	CHilite *	clonedHilite = new CHilite;
	clonedHilite->copyFrom(this);
	return clonedHilite;
}

void
CHilite::copyFrom(CHilite * inHilite)	// better with an assignment operator ??
{
	fBounds = inHilite->fBounds;
}

void
CHilite::updateBounds(void)
{ }

bool
CHilite::overlaps(const Rect * inBounds)
{
	return Overlaps(&fBounds, inBounds);
}

bool
CHilite::encloses(const Point inPt)
{
	return PtInRect(inPt, &fBounds);
}
/*
CRegion *
CHilite::area(void)
{
	// INCOMPLETE
	return NULL;
}
*/
// also CHiliteLoop

