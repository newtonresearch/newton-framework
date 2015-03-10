/*
	File:		View.cc

	Contains:	CView implementation.

	Written by:	Newton Research Group.
*/
#include "Quartz.h"

#include "Objects.h"
#include "ObjHeader.h"
#include "Lookup.h"
#include "Funcs.h"
#include "Globals.h"
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

#include "DrawShape.h"
//#include "DrawText.h"

extern CViewList *gEmptyViewList;
extern bool			gOutlineViews;
extern int			gScreenHeight;

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


/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------------
	D a t a

	The tag cache is an array of commonly used slot symbols. CView methods are
	provided which acccept an index into the tag cache, saving symbol lookup
	overhead.
--------------------------------------------------------------------------------*/

const Ref * gTagCache;					// 0C104F58.x00
static int	gUniqueViewId;				// 0C104F58.x04
bool			gSkipVisRegions;			// 0C104F58.x08
//static CRegionStruct * g0C104F64;	// 0C104F58.x0C
//static CRegionStruct * g0C104F68;	// 0C104F58.x10
//static long g0C104F6C;				// 0C104F58.x14


/*--------------------------------------------------------------------------------
	U t i l i t i e s
--------------------------------------------------------------------------------*/

bool
ProtoEQ(RefArg obj1, RefArg obj2)
{
	RefVar	fr(obj1);

	do
	{
		if (EQ(fr, obj2))
			return YES;
	} while (NOTNIL(fr = GetFrameSlot(fr, SYMA(_proto))));

	return NO;
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


bool
Overlaps(const Rect * r1, const Rect * r2)
{
	Rect	rect1 = *r1;
	Rect	rect2 = *r2;

	if (rect1.left == rect1.right)
		rect1.right = rect1.right + 1;
	if (rect1.top == rect1.bottom)
		rect1.bottom = rect1.bottom + 1;

	if (rect2.left == rect2.right)
		rect2.right = rect2.right + 1;
	if (rect2.top == rect2.bottom)
		rect2.bottom = rect2.bottom + 1;

	return Intersects(&rect1, &rect2);
}


bool
Intersects(const Rect * r1, const Rect * r2)
{
	Rect	nullRect;
	return SectRect(r1, r2, &nullRect);
}

#pragma mark -

long
GetFirstNonFloater(CViewList * inList)
{
	CView *			view;
	CBackwardLoop	iter(inList);
	while ((view = (CView *)iter.next()) != NULL
		&&  (view->viewFlags & vFloating) != 0)
		;
	return iter.index();
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C V i e w
--------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clView, CView, CResponder)


/*--------------------------------------------------------------------------------
	Constructor.
	DoesnÕt do anything, initialization is done in init().
--------------------------------------------------------------------------------*/

CView::CView()
{ }


/*--------------------------------------------------------------------------------
	Destructor.
--------------------------------------------------------------------------------*/

CView::~CView()
{ }


/*--------------------------------------------------------------------------------
	Perform a command.
	Args:		inCmd			the command frame
	Return:	bool			the command was performed by this class
--------------------------------------------------------------------------------*/

bool
CView::doCommand(RefArg inCmd)
{
	bool  isHandled;
	if ((isHandled = realDoCommand(inCmd)) == NO && this != fParent)
		isHandled = fParent->doCommand(inCmd);

//	if (ISNIL(isHandled))	// sic
//		isHandled = NO;

	return isHandled;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	Initialize.
	Args:		inContext		the context frame for this view
				inParentView	the parent view
	Return:	--
--------------------------------------------------------------------------------*/

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
			obj = inParentView->buildContext(GetArraySlot(aContext, i+1), YES);
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
			obj = inParentView->buildContext(GetArraySlot(aContext, i+1), YES);
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
	if (inParentView->viewFlags & vWriteProtected)
		setFlags(vWriteProtected);
	viewFlags |= (RINT(getCacheProto(kIndexViewFlags)) & vEverything);

//	Set up viewFormat
	Ref	format = getProto(SYMA(viewFormat));
	if (NOTNIL(format))
		viewFormat = RINT(format) & vfEverything;

//	If weÕre a child of the root view set up a clipper
//	if (inParentView == gRootView && this != gRootView)
//		SetFrameSlot(inContext, SYMA(viewClipper), AddressToRef(new CClipper));

//	Add us to our parentÕs children
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
		if ((viewFlags & vApplication) != 0
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

		if ((viewFlags & vVisible) != 0 && hasVisRgn())
			inParentView->viewVisibleChanged(this, NO);

		if (NOTNIL(obj = getProto(SYMA(declareSelf))))	// intentional assignment
			SetFrameSlot(inContext, obj, inContext);		// obj is a symbol

		addViews(NO);

		if (viewJustify & vjReflow)
		{
			bool  isFirst = YES;
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
					isFirst = NO;
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
		if ((viewFlags & vVisible) != 0 && hasVisRgn())
			inParentView->viewVisibleChanged(this, NO);
	}
	end_try;
}


/*--------------------------------------------------------------------------------
	Delete the view.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::dispose(void)
{
	setFlags(0x90000000);
	removeAllHilites();
	bool	wantPostQuit = NO;

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
//	CClipper * 	viewClipper = clipper();
//	if (viewClipper)
	{
//		delete viewClipper;
		SetFrameSlot(fContext, SYMA(viewClipper), RA(NILREF));
	}

	// remove ourselves from the root view
	gRootView->forgetAboutView(this);

	fTagCacheMaskHi |= 0x00010000;	// sic

	// and finallyÉ
	delete this;
}


/*--------------------------------------------------------------------------------
	Build a view hierarchy in the context of the root view.
	Args:		inProto		the view definition
				inArg2
	Return:	the instantiated view
--------------------------------------------------------------------------------*/

Ref
CView::buildContext(RefArg inProto, bool inArg2)
{
	RefVar	vwClass(GetProtoVariable(inProto, SYMA(viewClass)));
	RefVar	vwProto(inProto);
	RefVar	vwData;
	RefVar	stdForm;
	long		itsClass;
	bool		isInk = NO;

	if (ISNIL(vwClass))
	{
		vwClass = GetProtoVariable(inProto, SYMA(viewStationery));
		if (ISNIL(vwClass) && NOTNIL(GetProtoVariable(inProto, SYMA(ink))))
		{
			vwClass = RSYMpoly;
			isInk = YES;
		}
		if (ISNIL(vwClass))
			ThrowErr(exRootException, -8502);  // NULL view class

		stdForm = GetFrameSlot(gVarFrame, SYMA(stdForms));
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


/*--------------------------------------------------------------------------------
	Perform a command.
	Args:		inCmd		the command frame
	Return:	YES if we handled the command
--------------------------------------------------------------------------------*/
Ref
GetStrokeBundleFromCommand(RefArg inCmd)
{}

bool
CView::realDoCommand(RefArg inCmd)
{
	bool		isHandled = NO;
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
// ¥¥ R e c o g n i t i o n   E v e n t s
	case aeClick:
		if ((viewFlags & vClickable) != 0)
		{
			RefVar args(MakeArray(1));
			SetArraySlot(args, 0, AddressToRef((void *)CommandParameter(inCmd)));
			Ref clik = runCacheScript(kIndexViewClickScript, args);
			if (EQRef(clik, RSYMskip))
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
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
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
			isHandled = NOTNIL(runScript(SYMA(viewGestureScript), args, YES));
			CommandSetResult(inCmd, isHandled);
		}
		newton_catch_all
		{
			CommandSetResult(inCmd, 1);
			isHandled = YES;
		}
		end_try;
		break;

	case aeWord:
		{
		RefVar args(MakeArray(1));
		SetArraySlot(args, 0, AddressToRef((void *)CommandParameter(inCmd)));
		isHandled = NOTNIL(runScript(SYMA(viewWordScript), args));
		CommandSetResult(inCmd, isHandled);
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
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

// ¥¥ K e y   E v e n t s
	case aeKeyUp:
	case aeKeyDown:
	case ae34:
	case ae35:
		handleKeyEvent(inCmd, cmdId, NULL);
		isHandled = YES;
		break;

// ¥¥ V i e w   E v e n t s
	case aeAddChild:
		targetView = addChild(CommandFrameParameter(inCmd));
		gApplication->dispatchCommand(MakeCommand(aeShow, targetView, CommandParameter(inCmd)));
		isHandled = YES;
		break;

	case aeDropChild:
		targetView = (CView *)CommandParameter(inCmd);
		targetView->hide();
		removeChildView(targetView);
		isHandled = YES;
		break;

	case aeHide:
		hide();
		isHandled = YES;
		break;

	case aeShow:
		if (gModalCount == 0 || CommandParameter(inCmd) == 0x00420000)
		{
			show();
			isHandled = YES;
		}
		else
		{
			bool  isNormal = NO;
			if (fParent == gRootView)
			{
				for (targetView = this; targetView != gRootView; targetView = targetView->fParent)
				{
					if (targetView->viewJustify & 0x40000000)
					{
						isNormal = YES;
						break;
					}
				}
			}
			else
				isNormal = YES;
			if (isNormal)
				show();
			else
				ModalSafeShow(this);
			isHandled = YES;
		}
		break;

	case aeScrollUp:
		runScript(SYMA(viewScrollUpScript), RA(NILREF), YES);
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
		isHandled = YES;
		break;

	case aeScrollDown:
		runScript(SYMA(viewScrollDownScript), RA(NILREF), YES);
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
		isHandled = YES;
		break;

	case aeOverview:
		runScript(SYMA(viewOverviewScript), RA(NILREF), YES);
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
		isHandled = YES;
		break;

	case aeRemoveAllHilites:
		removeAllHilites();
		isHandled = YES;
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
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
		isHandled = YES;
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
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
		isHandled = YES;
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
		
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
		isHandled = YES;
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
			isHandled = YES;
		}
		break;

	case aeAddHilite:
		{
		RefVar parm(CommandFrameParameter(inCmd));
		if (IsFrame(parm))
		{
			ArrayAppendInFrame(fContext, SYMA(hilites), GetFrameSlot(parm, SYMA(hilite)));
			dirty();
			isHandled = YES;
		}
		}
		break;

	case aeRemoveHilite:
		removeHilite(CommandFrameParameter(inCmd));
		isHandled = YES;
		break;

	case aeChangeStyle:
		{
		CView * view;
		CListLoop iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
		{
			view->realDoCommand(inCmd);
		}
		gRootView->setParsingUtterance(YES);	// huh?
		isHandled = YES;
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
		gRootView->setParsingUtterance(YES);	// huh?
		isHandled = YES;
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
		SetFrameSlot(RA(gVarFrame), SYMA(lastTextChanged), RA(NILREF));
		isHandled = YES;
		}
		break;
	}

	return isHandled;
}


/*--------------------------------------------------------------------------------
	Find a view given its id.
	Args:		inId			the id
	Return:	CView *
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Do idle processing.
	We run the viewÕs viewIdleScript.
	Args:		inArg			unused
	Return:	int			number of milliseconds until next idle
--------------------------------------------------------------------------------*/

int
CView::idle(int inArg /*unused*/)
{
	int nextIdle = 0;	// kNoTimeout
	Ref result = runCacheScript(kIndexViewIdleScript, RA(NILREF));
	if (NOTNIL(result))
		nextIdle = RINT(result);

	return nextIdle;
}


/*--------------------------------------------------------------------------------
	Determine whether this view is printing.
	Args:		--
	Return:	bool
--------------------------------------------------------------------------------*/

bool
CView::printing(void)
{
#if defined(correct)
	GrafPtr	thePort;
	GetPort(&thePort);
	return (thePort->portBits.pixMapFlags & kPixMapDeviceType) != kPixMapDevScreen;
#else
	return NO;
#endif
}


/*--------------------------------------------------------------------------------
	A c c e s s o r s
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Determine whether this view is in the proto chain of a given view.
	Args:		inContext
	Return:	bool
--------------------------------------------------------------------------------*/

bool
CView::protoedFrom(RefArg inContext)
{
	Ref fr;

	for (fr = inContext; NOTNIL(fr); fr = GetFrameSlot(fr, SYMA(_proto)))
	{
		if (EQRef(fr, inContext))
			return YES;
	}

	return NO;
}


/*--------------------------------------------------------------------------------
	Return a variable in the proto context.
	Args:		inTag			the slot name
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
CView::getProto(RefArg inTag) const
{
	return GetProtoVariable(fContext, inTag);
}


/*--------------------------------------------------------------------------------
	Return a well-known variable in the proto context.
	Args:		index			index into the tag cache for the name of the variable
	Return:	Ref
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Return a well-known variable in the current context.
	Args:		index			index into the tag cache for the name of the variable
	Return:	Ref
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Invalidate the well-known slot cache.
	Args:		index			index into the tag cache for the name of the variable
	Return:	bool			always YES
--------------------------------------------------------------------------------*/

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
	return YES;
}


/*--------------------------------------------------------------------------------
	Return a variable in the current context.
	Args:		inTag			the slot name
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
CView::getVar(RefArg inTag)
{
	return GetVariable(fContext, inTag);
}


/*--------------------------------------------------------------------------------
	Get a slot value.
	Args:		inTag			the slot name
				inClass		the class we expect
	Return:	Ref
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Set a slot value.
	We handle certain slot names specially.
	NOTE	This method was formarly named SetValue().
	Args:		inTag			the slot name
				inValue		the value to set
	Return:	--
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Set a slot in the data frame.
	Args:		inTag			the slot name
				inValue		the value to set
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::setDataSlot(RefArg inTag, RefArg inValue)
{
	SetFrameSlot(dataFrame(), inTag, inValue);
}


/*--------------------------------------------------------------------------------
	Return the data frame.
	Args:		--
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
CView::dataFrame(void)
{
	Ref	data = getCacheProto(kIndexRealData);
	return NOTNIL(data) ? data : (Ref)fContext;
}


/*--------------------------------------------------------------------------------
	Return the view at the top of the view hierarchy.
	This corresponds to the 'window' this view is in.
	Args:		--
	Return:	CView *
--------------------------------------------------------------------------------*/

CView *
CView::getWindowView(void)
{
	CView * view = this;
	while (!view->hasVisRgn())
		view = view->fParent;
	return view;
}


/*--------------------------------------------------------------------------------
	Return a writeable copy of a variable.
	Args:		inTag			variableÕs slot
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
CView::getWriteableVariable(RefArg inTag)
{
	// see if itÕs a proto variable
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


/*--------------------------------------------------------------------------------
	Return a writeable copy of a proto variable.
	Args:		inTag			variableÕs slot
	Return:	Ref
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Return the viewÕs copy protection.
	Args:		--
	Return:	ULong
--------------------------------------------------------------------------------*/

ULong
CView::copyProtection(void)
{
	RefVar var(getVar(SYMA(copyProtection)));
	return NOTNIL(var) ? RINT(var) : cpNoCopyProtection;
}


/*--------------------------------------------------------------------------------
	Transfer the copy protection from another view to this.
	Args:		inView		the other view
	Return:	--
--------------------------------------------------------------------------------*/

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
				RefVar stdForms(GetFrameSlot(gVarFrame, SYMA(stdForms)));
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

/*--------------------------------------------------------------------------------
	V i e w   m a n i p u l a t i o n
--------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Set a custom foreground pattern.
	This really is a pattern, not just a colour -- need to set it in CG.
	Args:		inName		the symbol of the pattern to use
	Return:	YES always 
--------------------------------------------------------------------------------*/

bool
CView::setCustomPattern(RefArg inName)
{
	bool			isDone;
//	CGColorRef	pattern;
	RefVar		patName(getVar(inName));
//	if (NOTNIL(patName) && GetPattern(patName, &isDone, &pattern, YES))
//		SetFgPattern(pattern);
	return YES;
}


/*--------------------------------------------------------------------------------
	Set new viewFlags.
	Args:		inFlags		the flags to set (in addition to whatÕs already set)
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::setFlags(ULong inFlags)
{
	viewFlags |= inFlags;
	if (inFlags & (vVisible | vSelected))
	{
		RefVar	oldFlags(RINT(getCacheProto(kIndexViewFlags)));
		setContextSlot(SYMA(viewFlags), MAKEINT(oldFlags | inFlags));
	}
}


/*--------------------------------------------------------------------------------
	Clear viewFlags.
	Args:		inFlags		the flags to clear
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::clearFlags(ULong inFlags)
{
	viewFlags &= ~inFlags;
	if (inFlags & (vVisible | vSelected))
	{
		RefVar	oldFlags(RINT(getCacheProto(kIndexViewFlags)));
		setContextSlot(SYMA(viewFlags), MAKEINT(oldFlags & ~inFlags));
	}
}


/*--------------------------------------------------------------------------------
	Set the viewÕs origin. All drawing within the view is with respect to this
	origin.
	Args:		inOrigin		the origin
	Return:	--
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Return the viewÕs local origin within its parent.
	Args:		--
	Return:	Point
--------------------------------------------------------------------------------*/

Point
CView::localOrigin(void)
{
	Point		locOrigin = *(Point*)&viewBounds;
	Point		absOrigin = fParent->contentsOrigin();

	locOrigin.h -= absOrigin.h;
	locOrigin.v -= absOrigin.v;
	return locOrigin;
}


/*--------------------------------------------------------------------------------
	Return the viewÕs contentÕs origin.
	Args:		--
	Return:	Point
--------------------------------------------------------------------------------*/

Point
CView::contentsOrigin(void)
{
	Point		cntOrigin = childOrigin();
	cntOrigin.h = viewBounds.left - cntOrigin.h;
	cntOrigin.v = viewBounds.top - cntOrigin.v;
	return cntOrigin;
}


/*--------------------------------------------------------------------------------
	Set up the viewÕs visible region.
	Args:		--
	Return:	the region
--------------------------------------------------------------------------------*/
#if 0
CRegion
CView::setupVisRgn(void)
{
	CView *		view;
	CRegionVar	theRgn;
	GrafPtr		thePort;

	GetPort(&thePort);
	CopyRgn(thePort->visRgn, theRgn);

	for (view = this; view != gRootView; view = view->fParent)
	{
		CClipper *	viewClipper = view->clipper();
		if (viewClipper)
		{
			SectRgn(theRgn, *viewClipper->x04, theRgn);
			break;
		}
		if (viewFlags & vClickable)
		{
			CRectangularRegion	tapRgn(&view->viewBounds);
			SectRgn(theRgn, tapRgn, theRgn);
		}
		DiffRgn(theRgn, getFrontMask(), theRgn);
	}

	return theRgn;
}
#endif

/*--------------------------------------------------------------------------------
	Determine whether we have a visible region.
	The answerÕs YES if we are a child of the root view.
	Args:		--
	Return:	bool
--------------------------------------------------------------------------------*/

bool
CView::hasVisRgn(void)
{
	return fParent == gRootView && this != gRootView;
}


/*--------------------------------------------------------------------------------
	Determine whether this view and all its ancestors back to the root view are
	visible.
	Args:		--
	Return:	bool
--------------------------------------------------------------------------------*/

bool
CView::visibleDeep(void)
{
	CView * view;

	for (view = this; view != gRootView; view = view->fParent)
		if ((view->viewFlags & vVisible) == 0)
			return NO;
	return YES;
}


/*--------------------------------------------------------------------------------
	Build a region encompassing this view and all its ancestorsÕ children.
	Args:		--
	Return:	the region
--------------------------------------------------------------------------------*/
#if 0
CRegion
CView::getFrontMask(void)
{
	CRegionVar	theMask;
	SetEmptyRgn(theMask);

	CView *			view;
	CBackwardLoop	loop(viewChildren);
	while ((view = (CView *)loop.next()) != this && view != NULL)
	{
		if (view->viewFlags & vVisible)
		{
			Rect	bounds;
			if ((view->viewFormat & vfFillMask) || view->hasVisRgn())
			{
				CClipper *	viewClipper = view->clipper();
				if (viewClipper)
					UnionRgn(theMask, *viewClipper->fViewRgn, theMask);
				else
				{
					view->outerBounds(&bounds);
					CRectangularRegion rectRgn(&bounds);
					UnionRgn(theMask, rectRgn, theMask);
				}
			}
			else if (view->viewChildren != NULL)
			{
				CView *		childView;
				CListLoop	iter(view->viewChildren);
				while ((childView = (CView *)iter.next()) != NULL)
				{
					if ((view->viewFlags & vVisible) && (view->viewFormat & vfFillMask))
					{
						view->outerBounds(&bounds);
						CRectangularRegion rectRgn(&bounds);
						UnionRgn(theMask, rectRgn, theMask);
					}
				}
			}
		}
	}
	return theMask;
}


/*--------------------------------------------------------------------------------
	Return this viewÕs clipper. Only children of the root view have a clipper.
	Args:		--
	Return:	CClipper
--------------------------------------------------------------------------------*/

CClipper *
CView::clipper(void)
{
	if (hasVisRgn())
	{
		RefVar viewClipper(GetFrameSlot(fContext, SYMviewClipper));
		if (NOTNIL(viewClipper))
			return (CClipper *)RefToAddress(viewClipper);
	}
	return NULL;
}
#endif

/*--------------------------------------------------------------------------------
	Dirty this view so that it will be redrawn.
	Args:		inBounds		rect within the view to dirty
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::dirty(const Rect * inBounds)
{
	if (visibleDeep())
	{
		Rect	dirtyBounds;
		outerBounds(&dirtyBounds);
		if (inBounds)
			SectRect(inBounds, &dirtyBounds, &dirtyBounds);
		if (!EmptyRect(&dirtyBounds))
		{
			CView *	view = this;
			CView *	topView = NULL;
			while (!view->hasVisRgn())
			{
				if ((topView == NULL) && (view->viewFormat & vfFillMask))
					topView = view;
				view = view->fParent;
				if (view->viewFlags & vClipping)
					SectRect(&view->viewBounds, &dirtyBounds, &dirtyBounds);
				if (EmptyRect(&dirtyBounds))
					return;
			}
#if 0
			CClipper *	viewClipper = view->clipper();
			CRectangularRegion	dirtyRgn(&dirtyBounds);
			CRegionVar	clippedDirtyRgn;
			SectRgn(dirtyRgn, *viewClipper->x04, clippedDirtyRgn);
			if (!EmptyRgn(clippedDirtyRgn))
				gRootView->invalidate(clippedDirtyRgn, topView ? topView : view);
#endif
		}
	}
}


/*--------------------------------------------------------------------------------
	Show the view. ThereÕs no animation but may be a sound.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::show(void)
{
	if ((viewFlags & vVisible) == 0)
	{
		gStrokeWorld.blockStrokes();
		bringToFront();

		CAnimate animation;
		animation.setupPlainEffect(this, YES, 0);

		setFlags(vVisible);
		if (hasVisRgn())
			fParent->viewVisibleChanged(this, YES);
		else
			dirty();

		animation.doEffect(SYMA(showSound));

		gStrokeWorld.unblockStrokes();
		gStrokeWorld.flushStrokes();

		runScript(SYMA(viewShowScript), RA(NILREF));
	}
}


/*--------------------------------------------------------------------------------
	Hide the view. ThereÕs no animation but may be a sound.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::hide(void)
{
	gStrokeWorld.blockStrokes();

	if (ViewContainsCaretView(this))
		gRootView->caretViewGone();
	gRootView->setPopup(this, NO);

	CAnimate animation;
	animation.setupPlainEffect(this, NO, 0);

	runScript(SYMA(viewHideScript), RA(NILREF));

	if (hasVisRgn())
	{
		clearFlags(vVisible);
		fParent->viewVisibleChanged(this, YES);
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


/*--------------------------------------------------------------------------------
	Set the view bounds.
	Args:		inBounds		the bounds
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::setBounds(const Rect * inBounds)
{
	RefVar		justification;

	viewBounds = *inBounds;
	justification = getCacheProto(kIndexViewJustify);
	viewJustify = NOTNIL(justification) ? RINT(justification) : 0;
	justifyBounds(&viewBounds);

/*	CClipper *	viewClipper;
	if (viewClipper = clipper())	// intentional assignment
	{
		Rect	bounds;
		outerBounds(&bounds);		// sic - doesnÕt appear to have any effect
		viewClipper->updateRegions(this);
		fParent->viewVisibleChanged(this, NO);
	}
*/
}


/*--------------------------------------------------------------------------------
	Apply the viewÕs justification to some bounds.
	Args:		ioBounds		the bounds
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::justifyBounds(Rect * ioBounds)
{
	if (this != fParent) // donÕt justify root view bounds
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
		
//L100
		bool needsJustifyH = YES;
		bool needsJustifyV = YES;
		ArrayIndex  index;
		if ((justify & vjSiblingMask) != 0
		&& ((index = fParent->viewChildren->getIdentityIndex(this)) != kIndexNotFound
		 || (index = fParent->viewChildren->count()) > 0)
		&&   index > 0)
		{
			CView *  sibling = (CView *)fParent->viewChildren->at(index - 1);
			ULong		siblingJustifyV;
			if ((siblingJustifyV = justify & vjSiblingVMask) != 0)
			{
				needsJustifyV = NO;
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
//L206
			ULong		siblingJustifyH;
			if ((siblingJustifyH = justify & vjSiblingHMask) != 0)
			{
				needsJustifyH = NO;
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
//L295
		if (fParent == gRootView
		&&  this != gRootView)
		{
		// this is a child of the root view - justify WRT the entire display
			RefVar	displayParams(GetFrameSlot(RA(gVarFrame), SYMA(displayParams)));
			bounds.top = RINT(GetProtoVariable(displayParams, SYMA(appAreaGlobalTop)));
			bounds.left = RINT(GetProtoVariable(displayParams, SYMA(appAreaGlobalLeft)));
			origin = *(Point *)&bounds;
			bounds.bottom = bounds.top + RINT(GetProtoVariable(displayParams, SYMA(appAreaHeight)));
			bounds.right = bounds.left + RINT(GetProtoVariable(displayParams, SYMA(appAreaWidth)));
		}
//L369
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
//L425
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
//L454
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
//L483
		OffsetRect(ioBounds, origin.h, origin.v);
	}
}


void
CView::dejustifyBounds(Rect * ioBounds)
{
	// like above, but in reverse
}


/*--------------------------------------------------------------------------------
	Return the viewÕs bounds including its frame.
	Args:		outBounds		the bounds
	Return:	--
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Recalculate the viewÕs bounds from its viewBounds frame.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Scale the viewÕs bounds.
	Args:		inFromRect
				inToRect
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::scale(const Rect * inFromRect, const Rect * inToRect)
{
/*
	CTransform	xform;
	xform.setup(inFromRect, inToRect, NO);
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


/*--------------------------------------------------------------------------------
	Return the viewÕs origin (with respect to which children are located).
	Args:		outPt		the origin
	Return:	--
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Move the view within its parent.
	Args:		inDelta		the amount to move
	Return:	--
--------------------------------------------------------------------------------*/

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
	simpleOffset(inDelta, NO);

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
		view->simpleOffset(inDelta, YES);
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
	RefVar	context(GetProtoVariable(inView, SYMA(preallocatedContext)));
	if (ISNIL(context))
		context = buildContext(inView, NO);
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


/*--------------------------------------------------------------------------------
	Remove this view from its parent.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::removeView(void)
{
	fParent->removeChildView(this);
}


/*--------------------------------------------------------------------------------
	Remove a child of this view.
	Args:		inView		the child to be removed
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::removeChildView(CView * inView)
{
	if ((inView->viewFlags & vVisible) && (inView->viewFlags & vIsInSetupForm) == 0)
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


/*--------------------------------------------------------------------------------
	Remove all children of this view.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

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
	int	viewIndex = viewChildren->getIdentityIndex(inView);
	int	nonFloaterIndex = GetFirstNonFloater(viewChildren);
	if ((inView->viewFlags & vFloating) != 0
	||  (inView->viewJustify & vIsModal) != 0)
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

	if ((inView->viewFlags & vVisible) == 0)
		return;

	// INCOMPLETE!
//	fParent->dirty();	// TEST
}


void
CView::bringToFront(void)
{
	fParent->reorderView(this, viewChildren->count());
}


CView *
CView::frontMost(void)
{
	if (viewFlags & (vVisible | vApplication))
	{
		CView *			view, * frontMostView;
		CBackwardLoop	iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
			if ((frontMostView = view->frontMost()))	// intentional assignment
				return frontMostView;
	}
	return NULL;
}


CView *
CView::frontMostApp(void)
{
	if ((viewFlags & (vVisible | vApplication)) && !(viewFlags & vFloating))
	{
		CView *			view, * frontMostView;
		CBackwardLoop	iter(viewChildren);
		while ((view = (CView *)iter.next()) != NULL)
			if ((frontMostView = view->frontMostApp()) != NULL)
				return frontMostView;
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


void
CView::childViewMoved(CView * inView, Point newOrigin)
{
}

void
CView::childBoundsChanged(CView * inView, Rect * inBounds)
{
}

void
CView::moveChildBehind(CView * inView, CView * inBehindView)
{
}

int
CView::setChildrenVertical(int inArg1, int inArg2)
{ return 0;
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
//r4: r6 r5 r3 r10 r9
	*outIsClickable = NO;
	CView * theView = NULL;	//r7
	int theDistance;
	if ((theDistance = distance(inPt, inSlop)) != kDistanceOutOfRange)
	{
		if ((viewFlags & vClickable) != 0)
			*outIsClickable = YES;
		if (inRecognitionReqd == 0
		||  (inRecognitionReqd & (viewFlags & vRecognitionAllowed)) != 0)
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
				*outIsClickable = YES;
				break;
			}
		}
	}
	return theView;
}

int
CView::distance(Point inPt1, Point * inSlop)
{
	int theDistance = kDistanceOutOfRange;
	if ((viewFlags & vVisible) != 0)
	{
#if 0
		CClipper * clip;
		if ((clip = clipper()))
			if (PtInRgn(inPt1, /*RgnHandle*/clip.f04))
				// trivial acceptance -- pt is in the clipping region of the view
				return 0;
		else
#endif
		if (inSlop != NULL)
		{
			Rect bounds;
			outerBounds(&bounds);
			InsetRect(&bounds, -inSlop->h, -inSlop->v);
			if (PtInRect(inPt1, &bounds))
				theDistance = CheapDistance(MidPoint(&bounds), inPt1);
		}
		else
		{
			if (insideView(inPt1))
				theDistance = 0;
		}
	}
	return theDistance;
}

void
CView::narrowVisByIntersectingObscuringSiblingsAndUncles(CView * inView, Rect * inBounds)
{
}

void
CView::viewVisibleChanged(CView * inView, bool inVisible)
{
/*
	CRegionVar  sp08;
	Rect			sp00;
	sp00.left = -32767;
	sp00.top = -32767;
	sp00.right = 32766;
	sp00.bottom = 32766;
	RectRgn(&sp08, &sp00);
	vt24();
	// INCOMPLETE

	CView *			view;
	CBackwardLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		;
	}
*/
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
		return NO;

	if (outSpacing)
	{
		Ref	viewSpacing = getProto(SYMA(viewLineSpacing));
		long  spacing = NOTNIL(viewSpacing) ? RINT(viewSpacing) : 1;
		outSpacing->v = spacing;
		outSpacing->h = EQ(inGrid, SYMA(lineGrid)) ? 0 : spacing;
	}
	return YES;
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
	runCacheScript(kIndexViewChangedScript, args, YES);
	if (hasNoScripts)
		setFlags(vNoScripts);

	dirty();
}


#pragma mark Hilites
/*------------------------------------------------------------------------------
	H i l i t i n g
------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Highlight this view.
	Args:		isOn			or not
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::hilite(bool isOn)
{
	if (visibleDeep())
	{
		bool			isCaretOn;
	//	CRegion		vr = setupVisRgn();
	//	CRegionVar	viewVisRgn(vr);
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
		/*			bounds = viewBounds;
					int	inset = (viewFormat & vfInsetMask) >> vfInsetShift;
					int	rounding = (viewFormat & vfRoundMask) >> vfRoundShift;
					InsetRect(&bounds, -inset, -inset);
					if (rounding)
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
		//	GrafPtr	thePort;
		//	GetPort(&thePort);
		//	CopyRgn(viewVisRgn, thePort->visRgn);
		}
		end_unwind;
	}
}


/*--------------------------------------------------------------------------------
	Highlight everything.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

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


/*--------------------------------------------------------------------------------
	Return this viewÕs highlights.
	Args:		--
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
CView::hilites(void)
{
	return getProto(SYMA(hilites));
}


/*--------------------------------------------------------------------------------
	Return this viewÕs first highlight.
	Args:		--
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
CView::firstHilite(void)
{
	RefVar lites(hilites());
	if (NOTNIL(lites) && Length(lites) != 0)
		return GetArraySlot(lites, 0);
	return NILREF;	
}


/*--------------------------------------------------------------------------------
	Determine whether this view is highlighted.
	Args:		--
	Return:	Ref
--------------------------------------------------------------------------------*/

bool
CView::hilited(void)
{
	RefVar lites(hilites());
	return NOTNIL(lites) && Length(lites) != 0;
}


/*--------------------------------------------------------------------------------
	Delete all highlighted data.
	Args:		RefArg		dunno
	Return:	
--------------------------------------------------------------------------------*/

void
CView::deleteHilited(RefArg ignored)
{
	RefVar	cmd(MakeCommand(aeRemoveData, fParent, fId));
	gApplication->dispatchCommand(cmd);
}

	// ¥¥¥
void
CView::removeHilite(RefArg inArg1)
{
}


void
CView::removeAllHilites(void)
{
}


/*--------------------------------------------------------------------------------
	Determine whether this viewÕs completely hilited.
	You could say the view is ALL content data, so weÕll always say YES.
	Args:		inArg			dunno
	Return:	--
--------------------------------------------------------------------------------*/

bool
CView::isCompletelyHilited(RefArg inArg)
{
	return YES;
}


bool
CView::pointInHilite(Point inPt)
{
	// INCOMPLETE
	return NO;
}


/*--------------------------------------------------------------------------------
	Draw this viewÕs hiliting.
	Nothing to do here in the base class.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::drawHiliting(void)
{ }


/*--------------------------------------------------------------------------------
	Draw this viewÕs hilites.
	Nothing to do here in the base class.
	Args:		isOn			or not
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::drawHilites(bool isOn)
{ }


/*--------------------------------------------------------------------------------
	Draw this viewÕs hilited content data.
	You could say the view is ALL content data, so weÕll draw the whole lot.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::drawHilitedData(void)
{
	draw(&viewBounds, NO);
}


#pragma mark Text
/*------------------------------------------------------------------------------
	T e x t
------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Return this viewÕs text style.
	If it doesnÕt have a viewFont return the global userFont.
	Args:		--
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
CView::getTextStyle(void)
{
	RefVar	style(getProto(SYMA(viewFont)));
	if (ISNIL(style))
	{
		if (viewFlags & vReadOnly)
			style = getVar(SYMA(viewFont));
		else
			style = GetPreference(SYMA(userFont));
	}
	return style;
}


/*--------------------------------------------------------------------------------
	Return this viewÕs text style record.
	Args:		outRec		style record to be filled in
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::getTextStyleRecord(StyleRecord * outRec)
{
	CreateTextStyleRecord(getTextStyle(), outRec);
}


#pragma mark Gestures
/*------------------------------------------------------------------------------
	G e s t u r e s
------------------------------------------------------------------------------*/

ULong
CView::clickOptions(void)
{
	return 0x01;
}


void
CView::handleScrub(const Rect * inArg1, long inArg2, CUnit * unit, bool inArg4)
{}


#pragma mark Input
/*------------------------------------------------------------------------------
	I n p u t
------------------------------------------------------------------------------*/

void
CView::setCaretOffset(int * ioX, int * ioY)
{}

void
CView::offsetToCaret(long inArg1, Rect * inArg2)
{}

void
CView::pointToCaret(Point inPt, Rect * inArg2, Rect * inArg3)
{}

ULong
CView::textFlags(void) const
{
	RefVar	flags(getProto(SYMA(textFlags)));
	if (ISINT(flags))
		return RINT(flags);
	return 0;
}


#pragma mark Keyboard
/*------------------------------------------------------------------------------
	K e y b o a r d
------------------------------------------------------------------------------*/

void
CView::buildKeyChildList(CViewList * keyList, long inArg2, long inArg3)
{
	CView *		view;
	CListLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		if ((view->viewFlags & vVisible) != 0)
		{
			view->buildKeyChildList(keyList, inArg2, inArg3);
			if ((view->viewFlags & vReadOnly) == 0)
			{
#if defined(correct)
				if (inArg3 == 0)
				{
					if ((view->textFlags() & vTakesAllKeys) != 0 || view->protoedFrom(RA(protoInputLine)))
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
/*------------------------------------------------------------------------------
	S e l e c t i o n
------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Select this view.
	Set the flag and hilite as appropriate.
	Args:		isOn			select as opposed to deselect
				isUnique		if YES then deselect everything else in the parent
	Return:	--
--------------------------------------------------------------------------------*/

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
			hilite(YES);
		}
		
	}
	else
	{
		if (viewFlags & vSelected)
		{
			clearFlags(vSelected);
			hilite(NO);
		}
	}
}


/*--------------------------------------------------------------------------------
	Deselect all this viewÕs children.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::selectNone(void)
{
	CView *		view;
	CListLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()) != NULL)
	{
		if (view->viewFlags & vSelected)
		{
			view->hilite(NO);
			view->clearFlags(vSelected);
		}
	}
}


/*--------------------------------------------------------------------------------
	Set this viewÕs content selection.
	This really does nothing - plain views have no content.
	It will be overridden in more interesting views.
	Args:		inInfo
				ioX			possibly out args actually
				ioY
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::setSelection(RefArg inInfo, int * ioX, int * ioY)
{ }


/*--------------------------------------------------------------------------------
	Return this viewÕs content selection.
	This really returns NULL - plain views have no content.
	It will be overridden in more interesting views.
	Args:		--
	Return:	Ref
--------------------------------------------------------------------------------*/

Ref
CView::getSelection(void)
{
	return NILREF;
}


/*--------------------------------------------------------------------------------
	Activate this viewÕs content selection.
	This does nothing - plain views have no content - except run the
	viewCaretActivateScript so that more interesting views can call this inherited
	method.
	Args:		--
	Return:	Ref
--------------------------------------------------------------------------------*/

void
CView::activateSelection(bool isOn)
{
	RefVar	args(MakeArray(1));
	SetArraySlot(args, 0, isOn ? RA(TRUEREF) : RA(NILREF));

	runCacheScript(kIndexViewCaretActivateScript, args);
}

#pragma mark -

/*------------------------------------------------------------------------------
	S o u n d   E f f e c t s
------------------------------------------------------------------------------*/

void
CView::soundEffect(RefArg inTag)
{
	RefVar	snd(getVar(inTag));
	if (NOTNIL(snd))
		FPlaySound(fContext, snd);
}


#pragma mark Drawing
/*------------------------------------------------------------------------------
	D r a w i n g
------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Update this view on screen.
	Args:		inRgn			clipping region, no longer used
								ensure the CGContextÕs clipping path is set up on entry
				inView		child view from which to update
								NULL => update this view
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::update(CBaseRegion inRgn, CView * inView)
{
	CView *			view;
// save thePort->visRgn
	newton_try
	{
//		SectRgn(rgn, inRgn, rgn);
		Rect  bounds = viewBounds;	//= inRgn.rgnBBox
		view = (inView != NULL) ? inView : this;
//		if ((view->viewFormat & vfFillMask) == vfNone)
//			EraseRgn(rgn);

		if (inView == NULL || inView == gRootView)
			draw(inRgn, NO);
		else
		{
			CView * view = inView;
			CView * parent = view->fParent;
			for ( ; ; )
			{
				if (view)
					// draw the view and all its later siblings
					parent->drawChildren(inRgn, view);
				parent->postDraw(&bounds);
				if (parent == this || parent == gRootView)
					break;
				else
				{
					// step back up the parent chain
					view = parent;
					parent = view->fParent;
					// try the next sibling
					CViewList * kids = parent->viewChildren;
					view = (CView *)kids->at(kids->getIdentityIndex(view) + 1);
				}
			}
		}
	}
	newton_catch(exRootException)
	{ }
	end_try;
// restore thePort->visRgn
}


/*--------------------------------------------------------------------------------
	Draw this view, clipped to a Rect.
	Args:		inBounds		clipping bounds
				inArg2
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::draw(const Rect * inBounds, bool inArg2)
{
	CRectangularRegion	rectRgn(inBounds);
	draw(rectRgn, inArg2);
}


/*--------------------------------------------------------------------------------
	Draw this view.
	Args:		inRgn			clipping region, no longer used
								was actually a TBaseRegion passed ON THE STACK!
								ensure the CGContextÕs clipping path is set up on entry
				inArg2
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::draw(CBaseRegion inRgn, bool inArg2)
{
	if ((viewFlags & vVisible) == 0 || (viewFlags & vIsInSetup2) != 0)
		if (inArg2 == NO)
			return;

	Rect  sp08 = viewBounds;	// TEST
/*
	Rect  sp08 = inRgn.rgnBBox;
	Rect  sp00;
	outerBounds(&sp00);
	if (RectOverlaps(sp08, sp00))
	{
		CClipper *  r6 = clipper();
		if (r6 && !gSkipVisRegions)
		{
			if (g0C104F6C == 0)
			{
				g0C104F6C = 1;
				g0C104F68 = new CRegionStruct;
				g0C104F64 = g0C104F68;
			}
			SectRgn(r6->f04, inRgn, g0C104F68);
			r0 = !EmptyRgn(g0C104F68);
		}
		else
			r0 = RectInRgn(sp00, inRgn);
		if (r0)
		{
			CRegionStruct *  sp04 = NULL;
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
				if (sp04 == NULL || !EmptyRgn(thePort->visRgn))
*/				{
					if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
						StartDrawing(NULL, NULL);

					preDraw(&sp08);

/*					if ((viewFlags & vClipping) != 0)
					{
						sp04 = new CRegionStruct;
						GrafPtr  thePort;
						GetPort(&thePort);
						CopyRgn(thePort->visRgn, sp04);

						GetPort(&thePort);
						sp008 = thePort->visRgn;
						CRectangularRegion	sp00C(viewBounds);
						SectRgn(sp008, sp00C, sp008);
					}
*/
					realDraw(&sp08);

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

					drawChildren(inRgn, NULL);

/*					if ((viewFlags & vClipping) != 0)
					{
						RgnHandle	theRgn;
						GrafPtr  thePort;
						GetPort(&thePort);
						theRgn = thePort->visRgn; 
						CopyRgn(sp04, theRgn);
						delete sp04;
						sp04 = NULL;
					}
*/
					postDraw(&sp08);

					if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
					{
						StopDrawing(NULL, NULL);
						Wait(gSlowMotion);
					}
				}
			}
			on_unwind
			{
/*				if (sp04)
				{
					RgnHandle	theRgn;
					GrafPtr  thePort;
					GetPort(&thePort);
					theRgn = thePort->visRgn; 
					CopyRgn(sp04, theRgn);
					delete sp04;
				}
				if (sp00)
				{
					RgnHandle	theRgn;
					GrafPtr  thePort;
					GetPort(&thePort);
					theRgn = thePort->visRgn; 
					CopyRgn(sp00, theRgn);
					delete sp00;
				}
*/			}
			end_unwind;
/*		}
	}
*/

	if (gOutlineViews)
	{
		CGContextSaveGState(quartz);
		SetPattern(vfGray);
		SetLineWidth(2);
		StrokeRect(viewBounds);
		CGContextRestoreGState(quartz);
	}
}


/*--------------------------------------------------------------------------------
	Draw this viewÕs children, clipped to a Rect.
	Args:		inBounds		clipping bounds
				inView		child view from which to start drawing
								NULL => draw them all
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::drawChildren(const Rect * inBounds, CView * inView)
{
	CRectangularRegion	rectRgn(inBounds);
	drawChildren(rectRgn, inView);
}


/*--------------------------------------------------------------------------------
	Draw this viewÕs children.
	Args:		inRgn			clipping region, no longer used
								ensure the CGContextÕs clipping path is set up on entry
				inView		child view from which to start drawing
								NULL => draw them all
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::drawChildren(CBaseRegion inRgn, CView * inView)
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
						view->draw(inRgn, NO);
					}
					// else do nothing until we reach the specified child view
				}
				else
					view->draw(inRgn, NO);
			}
		}
		newton_catch(exRootException)
		{ }
		end_try;
	}
}


/*--------------------------------------------------------------------------------
	Draw the viewÕs grid or line pattern, before any other drawing takes place.
	Args:		inBounds
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::preDraw(Rect * inBounds)
{
	if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
		StartDrawing(NULL, NULL);

	if ((viewFormat & (vfLinesMask | vfFillMask)) != 0)
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
				SectRect(inBounds, &bounds, &bounds);
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


/*--------------------------------------------------------------------------------
	Draw the viewÕs content.
	A plain view has no content so we really do nothing here in the base class.
	Args:		inBounds
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::realDraw(Rect * inBounds)
{ }


/*--------------------------------------------------------------------------------
	Draw the viewÕs frame after its content has been drawn.
	Also indicate which button is default if weÕre keyboard operated.
	Args:		inBounds
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::postDraw(Rect * inBounds)
{
	if (gSlowMotion != 0 && viewChildren != gEmptyViewList)
		StartDrawing(NULL, NULL);

	if (viewFlags & vSelected)
		hilite(YES);

	if ((viewFormat & (vfFrameMask | vfShadowMask)) != 0)
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
					DrawPicture(hookPict, &hookBounds, 0);
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
		xform.setup(inSrcBounds, inDstBounds, NO);
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
/*------------------------------------------------------------------------------
	D r a g   a n d   D r o p
------------------------------------------------------------------------------*/

bool
CView::addDragInfo(CDragInfo * info)
{
	bool		done;
	RefVar	args(MakeArray(1));
//	SetArraySlot(args, 0, info->00);
	Ref	result = runScript(SYMA(viewAddDragInfoScript), args, YES, &done);
	return done && NOTNIL(result);
}

int
CView::dragAndDrop(CStroke * inStroke, const Rect * inBounds, const Rect * inArg3, const Rect * inArg4, bool inArg5, const CDragInfo * info, const Rect * inArg7)
{
}

void
CView::drag(CStroke * inStroke, const Rect * inBounds)
{
}

void
CView::drag(const CDragInfo * info, CStroke * inStroke, const Rect * inArg3, const Rect * inArg4, const Rect* inArg5, bool inArg6, Point * inArg7, Point * inArg8, bool * inArg9, bool * inArg10)
{
}

void
CView::alignDragPtToGrid(const CDragInfo * inArg1, Point * inArg2)
{
}

void
CView::dragFeedback(const CDragInfo * inArg1, const Point & inArg2, bool inArg3)
{
}

void
CView::endDrag(const CDragInfo * inArg1, CView * inArg2, const Point * inArg3, const Point * inArg4, const Point * inArg5, bool inArg6)
{
}

void
CView::drawDragBackground(const Rect * inArg1, bool inArg2)
{
}

void
CView::drawDragData(const Rect * inBounds)
{
}

void
CView::dropApprove(CView * inTarget)
{
}

void
CView::getSupportedDropTypes(const Point * inPt)
{
}

void
CView::getClipboardDataBits(Rect * inArea)
{
}

void
CView::findDropView(const CDragInfo & inArg1, const Point * inPt)
{  /* this really does nothing */	}

void
CView::acceptDrop(const CDragInfo & inArg1, const Point * inPt)
{
}

void
CView::targetDrop(const CDragInfo & inArg1, const Point * inPt)
{
}

Ref
CView::getDropData(RefArg inDragType, RefArg inDragRef)
{
	RefVar	args(MakeArray(2));
	SetArraySlot(args, 0, inDragType);
	SetArraySlot(args, 1, inDragRef);
	return runScript(SYMA(viewGetDropDataScript), args, YES, NULL);
}

void
CView::drop(RefArg inArg1, RefArg inArg2, Point * inPt)
{
}

void
CView::dropMove(RefArg inArg1, const Point * inArg2, const Point * inArg3, bool inArg4)
{
}

void
CView::dropRemove(RefArg inArg1)
{
}

bool
CView::dropDone(void)
{
	return NOTNIL(runScript(SYMA(viewDropDoneScript), RA(NILREF), YES, NULL));
}


#pragma mark Scripts
/*------------------------------------------------------------------------------
	S c r i p t s
------------------------------------------------------------------------------*/

/*--------------------------------------------------------------------------------
	Run the viewSetupFormScript.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::setupForm(void)
{
	runScript(SYMA(viewSetupFormScript), RA(NILREF));
}


/*--------------------------------------------------------------------------------
	Run the viewSetupDoneScript.
	Args:		--
	Return:	--
--------------------------------------------------------------------------------*/

void
CView::setupDone(void)
{
	runScript(SYMA(viewSetupDoneScript), RA(NILREF));
}


/*--------------------------------------------------------------------------------
	Run a Newton script.
	Args:		inTag			script name
				inArgs		argument array
				inSelf		as opposed to in proto
				outDone		whether script was actually run (maybe bad parameters
								prevented it)
	Return:	Ref			result of the script
--------------------------------------------------------------------------------*/

Ref
CView::runScript(RefArg inTag, RefArg inArgs, bool inSelf, bool * outDone)
{
	bool isDone = NO;
	Ref  result = NILREF;

	if (!MASKTEST(viewFlags, vNoScripts))
	{
		if (NOTNIL(inSelf ? getVar(inTag) : getProto(inTag)))
		{
			result = inSelf ? DoMessage(fContext, inTag, inArgs)
								 : DoProtoMessage(fContext, inTag, inArgs);
			isDone = YES;
		}
	}
	if (outDone != NULL)
		*outDone = isDone;
	return result;
}


/*--------------------------------------------------------------------------------
	Run a Newton script. Access the script name via index into the tag cache
	to save lookup overhead.
	Args:		index			tag cache index of the script
				inArgs		argument array
				inSelf		as opposed to in proto
				outDone		whether script was actually run (maybe bad parameters
								prevented it)
	Return:	Ref			result of the script
--------------------------------------------------------------------------------*/

Ref
CView::runCacheScript(ArrayIndex index, RefArg inArgs, bool inSelf, bool * outDone)
{
	bool isDone = NO;
	Ref  result = NILREF;

	if (!MASKTEST(viewFlags, vNoScripts))
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
			isDone = YES;
		}
	}
	if (outDone != NULL)
		*outDone = isDone;
	return result;
}


#pragma mark Debug
/*------------------------------------------------------------------------------
	D e b u g
------------------------------------------------------------------------------*/

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

/*------------------------------------------------------------------------------
	C H i l i t e
------------------------------------------------------------------------------*/

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

