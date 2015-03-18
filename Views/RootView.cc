/*
	File:		RootView.cc

	Contains:	CRootView implementation.

	Written by:	Newton Research Group.
*/
#include "Quartz.h"

#include "Objects.h"
#include "Lookup.h"
#include "Funcs.h"
#include "Globals.h"
#include "Arrays.h"
#include "NewtonGestalt.h"
#include "ListLoop.h"
#include "ArrayIterator.h"
#include "MagicPointers.h"
#include "Sound.h"
#include "Preference.h"

#include "RootView.h"
#include "ParagraphView.h"
#include "Clipboard.h"
#include "Screen.h"
#include "Modal.h"
#include "Notebook.h"
#include "Animation.h"
#include "Recognition.h"
#include "QDGeometry.h"

extern CGImageRef LoadPNG(const char * inName);

CViewList *		gEmptyViewList = NULL;		// 0C101930
CRootView *		gRootView = NULL;				// 0C101934
bool				gKeyboardConnected = NO;	// 0C101938		result of undocumented gestalt
bool				gOutlineViews = YES;			// 0C10193C
int				gSlowMotion = 0;				// 0C101940		delay after drawing in milliseconds

extern bool		gNewtIsAliveAndWell;

#define		kCaretBoxWidth		11
#define		kCaretBoxHeight	12


/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/


extern long		LSearch(RefArg inArray, RefArg inItem, RefArg inStart, RefArg inTest, RefArg inKey);
extern ULong	TextOrInkWordsEnabled(CView * inView);


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C"
{
Ref	FGetRoot(RefArg inRcvr);
}


Ref
FGetRoot(RefArg inRcvr)
{
	return (gRootView != NULL) ? gRootView->fContext.h->ref : NILREF;
}


#pragma mark -
/*--------------------------------------------------------------------------------
	U t i l i t i e s
--------------------------------------------------------------------------------*/

void
ClearHardKeymap(void)
{}


bool
ViewContainsCaretView(CView * inView)
{
	CView *	caretView;

	if ((caretView = gRootView->caretView()))		// intentional assignment
	{
		for ( ; caretView != gRootView; caretView = caretView->fParent)
		{
			if (inView == caretView)
				return YES;
		}
	}
	return NO;
}


Rect
CaretPointToRect(Point inCaretPt)
{
	Rect caretRect;
	if (inCaretPt.v == -32768)
		caretRect.top = caretRect.bottom = -32768;
	else
	{
		caretRect.left = inCaretPt.h - kCaretBoxWidth/2;
		caretRect.right = caretRect.left + kCaretBoxWidth;
		caretRect.top = inCaretPt.v;
		caretRect.bottom = caretRect.top + kCaretBoxHeight;
	}
	return caretRect;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	C R o o t V i e w
--------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clView, CRootView, CView)


/*--------------------------------------------------------------------------------
	Constructor.
	DoesnÕt do anything, initialization is done in init().
--------------------------------------------------------------------------------*/

CRootView::CRootView()
{ }


/*--------------------------------------------------------------------------------
	Destructor.
--------------------------------------------------------------------------------*/

CRootView::~CRootView()
{
	if (x40)
		delete x40;
	if (x34)
		delete x34;
//	if (fCaret)
//		delete fCaret;
}


/*--------------------------------------------------------------------------------
	Perform a command.
	Args:		inCmd		the command frame
	Return:	YES if we handled the command
--------------------------------------------------------------------------------*/

bool
CRootView::realDoCommand(RefArg inCmd)
{
	bool  isHandled = NO;

	switch(CommandId(inCmd))
	{
	case aeKeyboardEnable:
		{
			bool  isEnabled = CommandParameter(inCmd);
			gKeyboardConnected = isEnabled;
			if (isEnabled)
				ClearHardKeymap();
			else
				checkForCaretRemoval();
			if (fPopup != NULL)
				sync();
			dirty();
			isHandled = YES;
		}
		break;

	case aeStartHilite:
		hiliter((CUnit*)CommandParameter(inCmd), (CView*)CommandReceiver(inCmd));
		isHandled = YES;
		break;
	case aeAddData:
		{
			CView *  theView = addView(CommandFrameParameter(inCmd));
			int		valu, clipboardLimit = 1;
			RefVar	rClipboardDepth(GetPreference(SYMA(clipboardDepth)));
			if (ISINT(rClipboardDepth) && (valu = RVALUE(rClipboardDepth)) > 0)
				clipboardLimit = valu;
			if (theView->derivedFrom(clClipboard))
			{
				if (ISNIL(fClipboardViews))
					fClipboardViews = MakeArray(0);
				ArrayInsert(fClipboardViews, theView->fContext, 0);
				int numOfClipboards = Length(fClipboardViews);
				if (numOfClipboards > clipboardLimit)
				{
					// too many clipboards -- remove the oldest
					CView * cbView = GetView(GetArraySlot(fClipboardViews, --numOfClipboards));
					RefVar	cmd(MakeCommand(aeRemoveData, this, cbView->fId));
					gApplication->dispatchCommand(cmd);
				}
				if (numOfClipboards > 1)
				{
					CView * cbView = GetView(GetArraySlot(fClipboardViews, 1));
					SetFrameSlot(cbView->fContext, SYMA(viewFillPattern), RA(NILREF));
				}
				FPlaySound(RA(NILREF), RA(addSound));	// ROM_addSound via RS
			}
			else // itÕs the clipboard icon
			{
				if (ISNIL(fClipboardIcons))
					fClipboardIcons = MakeArray(0);
				ArrayInsert(fClipboardIcons, theView->fContext, 0);
				int numOfIcons = Length(fClipboardIcons);
				if (numOfIcons > clipboardLimit)
				{
					// too many clipboard icons -- remove the oldest
					CView * cbView = GetView(GetArraySlot(fClipboardIcons, --numOfIcons));
					RefVar	cmd(MakeCommand(aeRemoveData, this, cbView->fId));
					gApplication->dispatchCommand(cmd);
				}
				if (numOfIcons > 1)
				{
					CView * cbView = GetView(GetArraySlot(fClipboardIcons, 1));
					cbView->setSlot(SYMA(viewFillPattern), MAKEINT(0x10999999));	// huh?
				}
				theView->dirty();
			}
			gApplication->postUndoCommand(aeRemoveData, this, fId);
			isHandled = YES;
		}
		break;

	case aeRemoveData:
		{
			CView *  theView = findId(CommandParameter(inCmd));
			if (theView->derivedFrom(clClipboard) || (theView->viewFlags & vNoScripts) != 0)
			{
				RefVar	frame(theView->dataFrame());
				SetFrameSlot(frame, SYMA(bits), RA(NILREF));
				if (theView->derivedFrom(clClipboard))
				{
					ArrayRemove(fClipboardViews, theView->fContext);
					if (Length(fClipboardViews) == 0)
						fClipboardViews = NILREF;
				}
				else // itÕs the clipboard icon
				{
					ArrayRemove(fClipboardIcons, theView->fContext);
					if (Length(fClipboardIcons) == 0)
						fClipboardIcons = NILREF;
					else
					{
						theView = GetView(GetArraySlot(fClipboardIcons, 0));
						theView->setSlot(SYMA(viewFillPattern), MAKEINT(0x10000000));
					}
				}
				removeChildView(theView);
				RefVar	cmd(MakeCommand(aeAddData, this, theView->fId));
				CommandSetFrameParameter(cmd, frame);
				gApplication->postUndoCommand(cmd);
			}
			isHandled = YES;
		}
		break;

	default:
		isHandled = CView::realDoCommand(inCmd);
		break;
	}

	return isHandled;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	Initialize.
	Args:		inProto		the context frame for this view
				inView		the parent view - will always be NULL for the root view
	Return:	--
--------------------------------------------------------------------------------*/

void
CRootView::init(RefArg inProto, CView * inView)
{
	x34 = new CViewStuff[3];
	x40 = new CDynamicArray(sizeof(IdleViewInfo));
	InitCorrection();
	gEmptyViewList = static_cast<CViewList *>(CViewList::make());
	fKeyboards = MakeArray(0);
	fSelectionStack = MakeArray(0);

#if defined(correct)
	// see whether keyboard is plugged in
	ULong			result;
	CUGestalt	gestalt;
	if (gestalt.gestalt(kGestalt_Extended_Base+10, &result, sizeof(result)) == noErr
	&& result != 0)	// actually first byte of result -- maybe cast to (bool)
		gKeyboardConnected = YES;
#endif
	
	RefVar context(Clone(gConstNSData->rootContext));	// originally RA(rootContext)
	SetFrameSlot(context, SYMA(_proto), inProto);		// RA(rootProto) = SYS_rootProto = @287
	CView::init(context, this);

	// initialize caret
	Rect	caretBox;
	SetRect(&caretBox, 0, 0, kCaretBoxWidth, kCaretBoxHeight);
#if defined(correct)
	fCaret = new CBits;
	fCaret->init(&caretBox);
#else
	fCaret = LoadPNG("caret");
#endif

	dirty();
}


#pragma mark -
/*--------------------------------------------------------------------------------
	V i e w   M a n i p u l a t i o n
--------------------------------------------------------------------------------*/

void
CRootView::dirty(const Rect * inRect)
{
	Rect dirtyBounds = (inRect != NULL) ? *inRect : viewBounds;
	CRectangularRegion rgn(&dirtyBounds);
	invalidate(rgn, NULL);
}


void
CRootView::invalidate(const CBaseRegion inRgn, CView * inView)
{
	if (gSlowMotion)
	{
		StartDrawing(NULL, NULL);
//		InvertRgn(inRgn);					// flash this view in inverse
		StopDrawing(NULL, NULL);
	}

	if (inView == NULL || inView == this)
	{
		// invalidating in the root view context -- collect all invalid regions into x34[0] and NULL out the rest
		x34[0].fView = this;
//		UnionRgn(x34[0].fRegion, inRgn, x34[0].fRegion);	// add invalid region to view(0)
		for (ArrayIndex i = 1; i < 3; ++i)
		{
			if (x34[i].fView != NULL)
			{
//				UnionRgn(x34[0].fRegion, x34[i].fRegion, x34[0].fRegion); // add invalid region of view(n) to view(0)
				x34[i].fView = NULL;				// view(n) is no longer needed
//				SetEmptyRgn(x34[i].fRegion);
			}
		}
	}
	else
	{
		CView * commonParent[3] = {NULL,NULL,NULL};
		for (ArrayIndex i = 0; i < 3; ++i)
		{
			CView * parentView;
			CView * theView = x34[i].fView;
			if (theView == NULL)
			{
				// we found an empty slot, use it for the invalidated view and record its invalid region
				x34[i].fView = inView;
//				UnionRgn(x34[i].fRegion, inRgn, x34[i].fRegion)
				return;
			}
			else if (theView == this)
			{
				// add invalid region to root view
//				UnionRgn(x34[i].fRegion, inRgn, x34[i].fRegion);
				return;
			}
			else
			{
				parentView = getCommonParent(theView, inView);
				commonParent[i] = parentView;
				if (x34[i].fView == parentView || inView == parentView)
				{
					x34[i].fView = commonParent[i];
//					UnionRgn(x34[i].fRegion, inRgn, x34[i].fRegion);	// add invalid region to common parent view
					return;
				}
			}
		}

		for (ArrayIndex i = 0; i < 3; ++i)
		{
			if (commonParent[i] != this)
			{
				x34[i].fView = commonParent[i];
//				UnionRgn(x34[i].fRegion, inRgn, x34[i].fRegion);
				for (ArrayIndex i1 = i+1; i1 < 3 && commonParent[i1] != NULL; i1++)
				{
					if (commonParent[i] == commonParent[i1])
					{
						// views share a parent -- coalesce regions and remove view
//						UnionRgn(x34[i].fRegion, x34[i1].fRegion, x34[i].fRegion);
						for (ArrayIndex i2 = i1; i2 < 2; i2++)
						{
							commonParent[i2] = commonParent[i2+1];
							x34[i2].fView = x34[i2+1].fView;
//							RgnHandle rgn = x34[i2+1].fRegion;	// swap regions
//							x34[i2+1].fRegion = x34[i2].fRegion;
//							x34[i2].fRegion = rgn;
						}
						commonParent[2] = NULL;
						x34[2].fView = NULL;
//						SetEmptyRgn(x34[2].fRegion);
						i1--;
					}
				}
				return;
			}
		}
		
		// when all else fails -- view(0) is the root view with the invalidated region; the others are NULL
		x34[0].fView = this;
//		UnionRgn(x34[0].fRegion, inRgn, x34[0].fRegion)
		for (ArrayIndex i = 1; i < 3; ++i)
		{
//			UnionRgn(x34[0].fRegion, x34[i].fRegion, x34[0].fRegion)
			x34[i].fView = NULL;
//			SetEmptyRgn(x34[i].fRegion);
		}
	}
}


void
CRootView::validate(const CBaseRegion inRgn)
{
	for (ArrayIndex i = 0; i < 3; ++i)
	{
#if 0
		RgnHandle	rgn = x34[i].fRegion;
		DiffRgn(rgn, inRgn, rgn);
#endif
	}
}


void
CRootView::smartInvalidate(const Rect *)
{}


void
CRootView::smartScreenDirty(const Rect * inRect)
{
	UnionRect(&fInkyRect, inRect, &fInkyRect);
}


bool
CRootView::needsUpdate(void)
{
	if (EmptyRect(&fInkyRect) && caretValid(NULL))
	{
		// no new ink or caret change, so further checking needed...
		CView *	defaultButton;
		CView *	caretSlip;
		findDefaultButtonAndCaretSlip(fCaretView, &defaultButton, &caretSlip);
		if (fCaretSlip == caretSlip && fDefaultButton == defaultButton)
		{
			for (ArrayIndex i = 0; i < 3; ++i)
				if (x34[i].fView != NULL)
					return YES;
			return NO;
		}
	}
	return YES;
}


void
CRootView::update(Rect * inRect)
{
	if (inRect != NULL)
	{
		// invalidate the area specified so we update it later
		CRectangularRegion	rgn(inRect);
		invalidate(rgn, NULL);
	}

	if (needsUpdate())
	{
		Point pt;
		bool  updateCaret = NO;
		if (!caretValid(&pt))
			updateCaret = YES;
		updateDefaultButtonAndCaretSlip();

		if (gSlowMotion == 0)
			StartDrawing(NULL, NULL);

		if (!updateCaret && fIsCaretUp)
		{
			// caret is up -- if itÕs in any of our views of interest it may need updating
			Rect  caretRect = getCaretRect();
			for (ArrayIndex i = 0; i < 3; ++i)
			{
				if (x34[i].fView != NULL
			/*	&&  RectInRgn(&caretRect, x34[i].fRegion)*/)
				{
					updateCaret = YES;
					break;
				}
			}
		}

		if (updateCaret && fIsCaretUp)
			restoreBitsUnderCaret();

		if (!EmptyRect(&fInkyRect))
		{
			StartDrawing(NULL, &fInkyRect);
			StopDrawing(NULL, &fInkyRect);	// ensures fInkyRect is blitted to h/w screen
			fInkyRect = gZeroRect;				// original sets Õem manually
		}

		CRegionVar  updateRgn;
		for (ArrayIndex i = 0; i < 3; ++i)
		{
			if (x34[i].fView != NULL)
			{
/*				GrafPtr	thePort;
				GetPort(&thePort);
				DiffRgn(x34[i].fRegion, thePort->visRgn, updateRgn);

				RgnHandle viewRgn = x34[i].fRegion;
				x34[i].fRegion = updateRgn;
				updateRgn = viewRgn;
	
				if (EmptyRgn(x34[i].fRegion))
					x34[i].fView = NULL;
				CView::update(updateRgn, x34[i].fView);
*/			// else
				CView::update(updateRgn, x34[i].fView);
				x34[i].fView = NULL;
			}
		}

		if (updateCaret && caretEnabled())
			drawCaret(pt);

		if (gSlowMotion == 0)
			StopDrawing(NULL, NULL);		// blit to h/w screen
	}
}


#pragma mark -
/*--------------------------------------------------------------------------------
	D r a w i n g
--------------------------------------------------------------------------------*/

void
CRootView::realDraw(Rect * inRect)
{
	// if the application layer is not up yet, just show the splash screen
	if (!gNewtIsAliveAndWell)
		((CNotebook *)gApplication)->drawSplashScreen();
}


void
CRootView::postDraw(Rect * inRect)
{
	// if current live stroke intersects the rect, redraw it
	gRecognitionManager.update(inRect);
}


#pragma mark -
/*--------------------------------------------------------------------------------
	V i e w   M a n i p u l a t i o n
--------------------------------------------------------------------------------*/

void
CRootView::removeAllViews(void)
{
	while (getClipboard())
		removeClipboard();

	CView::removeAllViews();
}


void
CRootView::forgetAboutView(CView * inView)
{
	if (inView == fHilitedView)
		fHilitedView = NULL;

	if (inView == fCaretView)
		caretViewGone();

	if (inView == fPopup)
		setPopup(inView, NO);

	if (inView == fCaretSlip)
		fCaretSlip = NULL;
	else if (inView == fDefaultButton)
		fDefaultButton = NULL;

	unregisterKeyboard(inView->fContext);

	if (MASKTEST(viewFlags, 0x20000000))
		removeAllIdlers(inView);

	for (ArrayIndex i = 0; i < 3; ++i)
		if (x34[i].fView == inView)
			x34[i].fView = inView->fParent;

	if (MASKTEST(viewJustify, 0x40000000))	// sic
		RealExitModalDialog(inView);
	else if (gModalCount)
		RemoveModalSafeView(inView);

	gRecognitionManager.removeClickView(inView);
}


CView *
CRootView::getCommonParent(CView * inView1, CView * inView2)
{
	CView *	theParent;
	CView *	view1, * view2;
	for (view1 = inView1; view1 != NULL; )
	{
		for (view2 = inView2; view2 != NULL; )
		{
			if (view2 == view1)
				return view1;	// the common ancestor
			// walk back up the parent chain
			theParent = view2->fParent;
			view2 = (view2 == theParent) ? NULL : theParent;
		}
		// walk back up the parent chain
		theParent = view1->fParent;
		view1 = (view1 == theParent) ? NULL : theParent;
	}
	return NULL;
}


CView *
CRootView::getFrontmostModalView(void)
{
	CView *			view;
	CBackwardLoop	iter(viewChildren);
	while ((view = (CView *)iter.next()))	// intentional assignment
	{
		if (MASKTEST(view->viewFlags, vVisible)
		 && NOTNIL(view->getProto(SYMA(modalState))))
			return view;
	}
	return NULL;
}

#pragma mark -
/*--------------------------------------------------------------------------------
	I d l i n g
--------------------------------------------------------------------------------*/

IdlingItem *
CRootView::getIdlingView(CView * inView)
{
	for (IdlingItem * idler = fIdlingList, * prev = NULL; idler != NULL; prev = idler, idler = idler->fNext)	// original inits prev = this
		if (idler->fView == inView)
			return prev;
	return NULL;
}


void
CRootView::unlinkIdleView(CView * inView)
{
	IdlingItem * prev = getIdlingView(inView);
	if (prev)
	{
		prev->fNext = prev->fNext->fNext;
	}
}


void
CRootView::addIdler(CView * inView, Timeout inTimeout, ULong inId)
{
	if (inTimeout == kNoTimeout)
		removeIdler(inView, inId);
	else
	{
		IdleViewInfo info;
		info.fView = inView;
		info.fId = inId;
		info.fTime = TimeFromNow((inTimeout*kMilliseconds));
		bool hasExistingIdler = false;
		CArrayIterator	iter(x40);
		for (ArrayIndex index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
		{
			IdleViewInfo * idler = (IdleViewInfo *)x40->safeElementPtrAt(index);
			if (idler->fView == inView && idler->fId == inId)
			{
				// this view is already idling - update its wakeup time
				idler->fTime = info.fTime;
				hasExistingIdler = true;
				break;
			}
		}
		if (!hasExistingIdler)
			x40->addElement(&info);
		setFlags(0x20000000);
		gApplication->updateNextIdleTime(info.fTime);
	}
}


ULong
CRootView::removeIdler(CView * inView, ULong inId)
{
	ULong	timeToRun = 0;
	CArrayIterator	iter(x40);
	for (ArrayIndex index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
	{
		IdleViewInfo *	idler = (IdleViewInfo *)x40->safeElementPtrAt(index);
		if (idler->fView == inView && idler->fId == inId)
		{
			CTime delta = GetGlobalTime() - idler->fTime;
			if (delta > CTime(0))	// idler has expired - return by how much - for debug?
				timeToRun = delta;
			x40->removeElementsAt(index, 1);
			unlinkIdleView(inView);
			break;
		}
	}
	return timeToRun;
}


void
CRootView::removeAllIdlers(CView * inView)
{
	CArrayIterator	iter(x40);
	for (ArrayIndex index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
	{
		IdleViewInfo *	idler = (IdleViewInfo *)x40->safeElementPtrAt(index);
		if (idler->fView == inView)
		{
			x40->removeElementsAt(index, 1);
			unlinkIdleView(inView);
		}
	}
	clearFlags(0x20000000);
}


CTime
CRootView::idleViews(void)
{
	// original compacts viewChildren and x40 CDynamicArrays

//sp-28
	IdleViewInfo sp18;
	sp18.fView = (CView *)0x7FFFFFFF;
	sp18.fId = 0;
	sp18.fTime = GetGlobalTime();

	CTime sp10(10, kMilliseconds);

	// insert NULL view in list of idling views
	IdlingItem sp08;
	sp08.fNext = fIdlingList;
	sp08.fView = NULL;
	fIdlingList = &sp08;
#if 0
//sp-1C
	size_t r9 = x40->count();
	CArrayIterator	iter(x40);
	for (ArrayIndex index = iter.firstIndex(); iter.more() && r9-- > 0; index = iter.nextIndex())
	{
		IdleViewInfo *	idler = (IdleViewInfo *)x40->safeElementPtrAt(index);	// r5
		IdlingItem * r0 = getIdlingView(idler->fView);
		if (r0 == NULL)
		{
//sp-10
			CTime spr00(idler->fTime);
			CTime spr08 = spr00 - sp10;
//sp+08
			if (spr00 > sp18.fTime)
			{
				long r6 = 0;
				sp08.fView = idler->fView;
				newton_try
				{
					r6 = idler->fView->idle(idler->fId);	// r6 = time to next idle in ms
				}
				newton_catch_all
				{ }
				end_try;
			}
			IdleViewInfo *	r5 = (IdleViewInfo *)x40->safeElementPtrAt(iter.currentIndex());
			bool r7 = (getIdlingView(sp08.fView) != NULL);
			if (r6)
			{
//sp-08
				CTime spr08(r6, kMilliseconds);
				r6 = r8 = &r5->fTime;
//sp-08
				spr00 = spr08 + r5->fTime;
				if (r8 == NULL)
					r8 = new CTime;
				*r8 = spr00;
//sp+08
				if (sp18.fTime < r6)
				{
//sp-08
					CTime spm00 = spr00 + sp10;
					if (r6 == NULL)
						r6 = new CTime;
					*r6 = spr00;
				}
			}
			else
			{
				x40->removeElementsAt(r8, 1);
				r7 = NO;
			}
			if (r7)
			{
				if (sp18.fTime < r5->fTime)
					sp18.fTime = r5->fTime;
			}
		}
//sp+08
	}
	unlinkIdleView(sp08.fView);
	if (!caretValid(NULL))
		sp18 = sp20;
	//sp-10
	IdleViewInfo spr00;
	spr00.fView = 0x7FFFFFFF;
	spr00.fId = 0;
	if (spr00 == sp18)
	{
		spr00.fTime = 0;
		sp18 = spr00.fTime;
	}

	return sp18;
#endif
}

#pragma mark -

/*--------------------------------------------------------------------------------
	P o p u p
--------------------------------------------------------------------------------*/

void
CRootView::setPopup(CView * inView, bool inShow)
{
	if (inShow)
	{
		RefVar	cmd(MakeCommand(aeDropChild, inView, 0x08000000));
		gApplication->dispatchCommand(cmd);
	}
	else if (inView == fPopup)	// remove it
	{
		Ref	popup = GetFrameSlot(fPopup->fContext, SYMA(popup));
		fPopup = NOTNIL(popup) ? (CView *)RefToAddress(GetFrameSlot(popup, SYMA(viewCObject))) : NULL;
	}
}


bool
CRootView::getRemoteWriting(void)
{
	return NOTNIL(GetPreference(SYMA(remoteWriting)));
}

#pragma mark -

/*--------------------------------------------------------------------------------
	H i l i t i n g
--------------------------------------------------------------------------------*/

void
CRootView::hiliter(CUnit*, CView *)
{}

void
CRootView::setHilitedView(CView * inView)
{
	if (fHilitedView != inView)
	{
		if (fHilitedView != NULL)
		{
			RefVar	cmd(MakeCommand(aeRemoveAllHilites, inView, 0x08000000));
			gApplication->dispatchCommand(cmd);
		}
		fHilitedView = inView;
	}
}


bool
CRootView::setPreserveHilites(bool inDoIt)
{
	bool	oldValue = fPreserveHilites;
	fPreserveHilites = inDoIt;
	return oldValue;
}

#pragma mark -

/*--------------------------------------------------------------------------------
	I n p u t
--------------------------------------------------------------------------------*/

void
CRootView::showCaret(void)
{
	if (--fHideCaretCount < 0)
		fHideCaretCount = 0;
}


void
CRootView::hideCaret(void)
{
	if (fIsCaretUp)
		restoreBitsUnderCaret();
	fHideCaretCount++;
}


void
CRootView::dirtyCaret(void)
{
	if (fIsCaretUp)
	{
		Rect	caretRect = getCaretRect();
		smartInvalidate(&caretRect);
		fIsCaretUp = NO;
	}
}


#if defined(correct)
void
DrawCaretBits(const Rect * inRect, bool inArg2)
{
	Rect  box = *inRect;
	InsetRect(&box, 1, 1);
	StartDrawing(NULL, NULL);
	if (inArg2)
	{
		DrawBitmap(pixelmapObjectClassBits, inRect, srcCopy);
		DrawBitmap(pixelmapObjectClassBitsMask, &box, srcXor);
	}
	else
	{
		DrawBitmap(pixelmapObjectClassBits, inRect, srcXor);
		DrawBitmap(pixelmapObjectClassBitsMask, &box, srcCopy);
	}
	StopDrawing(NULL, NULL);
}
#endif


CView *
GetCaretClipView(CView * inView)
{
	CView * clipView = inView;
	if (inView->derivedFrom(clParagraphView))
	{
		clipView = ((CParagraphView *)inView)->getEnclosingEditView();
		if (clipView == NULL)
			for (clipView = inView->fParent; clipView != gRootView; clipView = clipView->fParent)
			{
				if ((clipView->viewFlags & vFloating) != 0 || (clipView->viewFlags & vClipping) != 0)
					break;
			}
	}
	return clipView;
}


void
CRootView::drawCaret(Point inPt)
{
	if (fCaretView && x70 == NULL && fHideCaretCount == 0)
	{
		if (inPt.v == -32768)
		{
			fIsCaretUp = NO;
			fCaretPt = inPt;
			x90 = fCaretView;
			return;
		}
		Rect caretRect = CaretPointToRect(inPt);
		Rect visCaretRect = caretRect;

#if defined(correct)
		GrafPtr thePort;
		GetPort(&thePort);
		Rect sp00 = thePort->portBits.bounds;
		SectRect(&sp00, &visCaretRect, &visCaretRect);
		if (!EmptyRect(&visCaretRect))
		{
			Rect screenRect;
			screenRect.left = visCaretRect.left - sp00.left;
			screenRect.top = visCaretRect.top - sp00.top;
			screenRect.right = screenRect.left + (visCaretRect.right - visCaretRect.left);
			screenRect.bottom = screenRect.top + (visCaretRect.bottom - visCaretRect.top);
			fCaret->copyFromScreen(&visCaretRect, &screenRect, 0, NULL);
		}
#endif

		CView * caretView = GetCaretClipView(fCaretView);

#if defined(correct)
		CRegionVar savedVisRgn(caretView->setupVisRgn());
		narrowVisByIntersectingObscuringSiblingsAndUncles(caretView, &visCaretRect);
		DrawCaretBits(&caretRect, NO);
#else
		CGContextDrawImage(quartz, MakeCGRect(caretRect), fCaret);
#endif

		fIsCaretUp = YES;
		fCaretPt = inPt;
		x90 = fCaretView;

#if defined(correct)
		GetPort(&thePort);
		CopyRgn(savedVisRgn, thePort->visRgn);
#endif
	}
}


bool
CRootView::caretEnabled(void)
{
	if (fCaretView == NULL || x70 == NULL)
		return NO;
	return getRemoteWriting() ? YES : keyboardActive();
}


bool
CRootView::caretValid(Point * outPt)
{
	if (outPt)
		outPt->v = -32768;

	if (fHideCaretCount > 0)
		return YES;

	if (!caretEnabled())
		return fIsCaretUp == NO;

	Point	caretPt = getCaretPoint();
	if (outPt)
		*outPt = caretPt;
	if (caretPt.v == -32768
	 && (fIsCaretUp == NO || fCaretPt.v == -32768))
		return YES;

	if (fCaretView->visibleDeep())
		return fIsCaretUp && (x90 == fCaretView && *(long*)outPt == *(long*)&caretPt);

	return YES;
}


void
CRootView::checkForCaretRemoval(void)
{
	if (fCaretView
	 && fCaretView->derivedFrom(clEditView)
	 && !TextOrInkWordsEnabled(fCaretView)
	 && !keyboardActive())
		setKeyView(NULL, 0, 0, NO);
}


bool
CRootView::doCaretClick(CUnit * inUnit)
{
	if (fIsCaretUp && fPopup == NULL)
	{
		Rect caretRect = gRootView->getCaretRect();
		caretRect = CaretPointToRect(gRootView->fCaretPt);		// huh?
		Rect testRect = caretRect;
		InsetRect(&testRect, -2, -2);
		CStroke * strok = inUnit->stroke();	// r7
		if (PtInRect(strok->firstPoint(), &testRect))
		{
			// INCOMPLETE
			// Region twiddling stuff
		}
	}
	return NO;
}


Point
CRootView::getCaretPoint(void)
{
	Point	caretPt;
	Rect	caretRect;
	fCaretView->offsetToCaret(fId, &caretRect);
	if (caretRect.top == -32768)
		caretPt.v = -32768;
	else
	{
		caretPt.h = caretRect.left + 1;
		caretPt.v = caretRect.top;
	}
	return caretPt;
}


Rect
CRootView::getCaretRect(void)
{
	Rect	caretRect;
	if (fIsCaretUp)
		caretRect = CaretPointToRect(fCaretPt);
	else
		caretRect.top = caretRect.bottom = -32768;
	return caretRect;
}


void
CRootView::restoreBitsUnderCaret(void)
{
	if (fIsCaretUp)
	{
		Rect	caretRect = getCaretRect();

		PixelMap	caretMap;
		Rect		caretBox;
		caretBox.top = 0;
		caretBox.left = 0;
		caretBox.bottom = kCaretBoxHeight;
		caretBox.right = kCaretBoxWidth;
/*
		GrafPtr	thePort;
		GetPort(&thePort);
		RgnHandle	savedVisRgn = thePort->visRgn;

		RgnHandle	caretRgn = NewRgn();
		GetGrafInfo(kGrafPixelMap, &caretMap);
		RectRgn(caretRgn, &caretBox);

		GetPort(&thePort);
		thePort->visRgn = caretRgn;
		fCaret->draw(&caretMap.bounds, &caretBox, 0, NULL);
		
		GetPort(&thePort);
		thePort->visRgn = savedVisRgn;
		DisposeRgn(caretRgn);
*/		fIsCaretUp = NO;
	}
}


void
CRootView::caretViewGone(void)
{
	RefVar	selection(popSelection());
	if (NOTNIL(selection))
	{
		RefVar	view(GetFrameSlot(selection, SYMA(view)));
		RefVar	info(GetFrameSlot(selection, SYMA(info)));
		setKeyViewSelection(GetView(view), info, NO);
	}
	else
		setKeyViewSelection(NULL, RA(NILREF), NO);
}


#pragma mark -
/*--------------------------------------------------------------------------------
	K e y b o a r d
--------------------------------------------------------------------------------*/

void
CRootView::registerKeyboard(RefArg inKbd, ULong inStatus)
{
	ArrayIndex kbdIndex = getKeyboardIndex(inKbd);
	if (kbdIndex == kIndexNotFound)
	{
		kbdIndex = Length(fKeyboards);
		SetLength(fKeyboards, kbdIndex+2);
		SetArraySlot(fKeyboards, kbdIndex, inKbd);
	}
//	else keyboard is already registered, just update its id
	SetArraySlot(fKeyboards, kbdIndex+1, MAKEINT(inStatus));
}


bool
CRootView::unregisterKeyboard(RefArg inKbd)
{
	ArrayIndex kbdIndex = getKeyboardIndex(inKbd);
	if (kbdIndex == kIndexNotFound)
		return NO;

	ArrayRemoveCount(fKeyboards, kbdIndex, 2);
	if (fCaretView != NULL && fCaretView->derivedFrom(clParagraphView))
		((CParagraphView *)fCaretView)->flushWordAtCaret();
	checkForCaretRemoval();
	return YES;
}


ArrayIndex
CRootView::getKeyboardIndex(RefArg inKbd)
{
	ArrayIndex kbdIndex = kIndexNotFound;
	ArrayIndex count = Length(fKeyboards);
	for (ArrayIndex i = 0; i < count; i += 2)	// each entry in fKeyboards occupies two slots
	{
		if (EQ(GetArraySlot(fKeyboards, i), inKbd))
		{
			kbdIndex = i;
			break;
		}
	}
	return kbdIndex;
}


void
CRootView::connectPassthruKeyboard(bool inIsConnected)
{
	fIsPassthruKeyboardConnected = inIsConnected;
	if (!inIsConnected)
		checkForCaretRemoval();
}


bool
CRootView::keyboardConnected(void)
{
	return gKeyboardConnected || fIsPassthruKeyboardConnected;
}


bool
CRootView::commandKeyboardConnected(void)
{
	return gKeyboardConnected;
}


bool
CRootView::keyboardActive(void)
{
	if (gKeyboardConnected)
		return YES;

	if (NOTNIL(fKeyboards))
	{
		ArrayIndex count = Length(fKeyboards);
		for (ArrayIndex i = 1; i < count; i += 2)	// weÕre examining the status slot
		{
			if ((RINT(GetArraySlot(fKeyboards, i)) & 0x04) != 0)
				return YES;
		}
	}
	return NO;
}


void
CRootView::findDefaultButtonAndCaretSlip(CView * inView, CView ** outDefaultButton, CView ** outCaretSlip)
{
	CView * defaultButton = NULL;
	CView * caretSlip = NULL;
	if (inView != NULL && commandKeyboardConnected())
	{
		CView * vw = GetView(GetVariable(inView->fContext, SYMA(_defaultButton)));
		if (vw)
			defaultButton = vw;
		for (vw = inView; vw != this; vw = vw->fParent)
		{
			ULong frame = vw->viewFormat & vfFrameMask;
			if (frame == vfFrameMatte || frame == vfFrameDragger)
			{
				caretSlip = vw;
				break;
			}
		}
			
	}
	*outDefaultButton = defaultButton;
	*outCaretSlip = caretSlip;
}


void
CRootView::updateDefaultButtonAndCaretSlip(void)
{
	CView *	defaultButton;
	CView *	caretSlip;
	findDefaultButtonAndCaretSlip(fCaretView, &defaultButton, &caretSlip);
	if (fDefaultButton != defaultButton)
	{
		if (fDefaultButton)
			fDefaultButton->dirty();
		fDefaultButton = defaultButton;
		if (fDefaultButton)
			fDefaultButton->dirty();
	}
	if (fCaretSlip != caretSlip)
	{
		if (fCaretSlip)
			fCaretSlip->dirty();
		fCaretSlip = caretSlip;
		if (fCaretSlip)
			fCaretSlip->dirty();
	}
}


Ref
CRootView::handleKeyIn(ULong inKeyCode, bool inArg2, CView * inTargetView)
{
	RefVar target;
	if (inTargetView)
		target = inTargetView->fContext;
	if (inKeyCode == 0x3B || inKeyCode == 0x3A || inKeyCode == 0x38 || inKeyCode == 0x39)	// cmd alt shift capslock
	{
		ArrayIndex count = Length(fKeyboards);
		for (ArrayIndex i = 0; i < count; i += 2)	// each entry in fKeyboards occupies two slots
		{
			RefVar kbd(GetArraySlot(fKeyboards, i));
			if ((RINT(GetArraySlot(fKeyboards, i+1)) & 0x01) != 0
			&&  !EQ(kbd, target))
			{
				CView * kbdView = GetView(kbd);
				if (kbdView)
					kbdView->dirty();
			}
		}
	}
	return target;
}


void
CRootView::setKeyViewSelection(CView * inView, RefArg inInfo, bool inArg3)
{
	bool r2 = (fCaretView == NULL) || (fCaretView->viewFlags & (vIsInSetupForm | vIsInSetup2)) == (vIsInSetupForm | vIsInSetup2);
	if (inArg3 && !r2 && inView != fCaretView)
		pushSelection(fCaretView, fCaretView->getSelection());

	int x = 0;
	int y = 0;
	if (inView)
		inView->setSelection(inInfo, &x, &y);
	commonSetKeyView(inView, x, y);
}


void
CRootView::setKeyView(CView * inView, int inX, int inY, bool inArg4)
{
	bool r4 = (fCaretView == NULL) || (fCaretView->viewFlags & (vIsInSetupForm | vIsInSetup2)) == (vIsInSetupForm | vIsInSetup2);
	if (!r4 && inView != fCaretView)
		pushSelection(fCaretView, fCaretView->getSelection());
	if (inView)
	{
		if (inArg4 && !r4 && fCaretView->derivedFrom(clParagraphView))
			((CParagraphView *)fCaretView)->flushWordAtCaret();
		inView->setCaretOffset(&inX, &inY);
	}
	commonSetKeyView(inView, inX, inY);
}


void
CRootView::commonSetKeyView(CView * inView, int inX, int inY)
{
	// itÕs a relative biggy
}


bool
CRootView::restoreKeyView(CView * inView)
{
	ArrayIndex stackIndex;
	CView * keyView = findRestorableKeyView(inView, &stackIndex);
	if (keyView == NULL)
		return NO;
	setKeyViewSelection(keyView, GetArraySlot(fSelectionStack, stackIndex+1), YES);
	return YES;
}


CView *
CRootView::findRestorableKeyView(CView * inView, ArrayIndex * outIndex)
{
	int count = Length(fSelectionStack);
	for (int i = count - 2; i >= 0; i -= 2)
	{
		RefVar seln(GetArraySlot(fSelectionStack, i));
		CView * keyView = GetView(seln);
		if (keyView != NULL && keyView != gRootView)
		{
			for (CView * vw = keyView; vw != gRootView; vw = vw->fParent)
			{
				if (vw == inView)
				{
					if (outIndex)
						*outIndex = i;
					return keyView;
				}
			}
		}
	}
	return NULL;
}


void
CRootView::holdPendingKeyView(RefArg inKeyView, RefArg inKeyViewSelection)
{
	fPendingKeyView = inKeyView;
	fPendingKeyViewSelection = inKeyViewSelection;
}


void
CRootView::activatePendingKeyView(void)
{
	CView * keyView = GetView(fPendingKeyView);
	if (keyView)
		setKeyViewSelection(keyView, fPendingKeyViewSelection, YES);
	fPendingKeyView = NILREF;
	fPendingKeyViewSelection = NILREF;
}


#pragma mark -
/*--------------------------------------------------------------------------------
	S e l e c t i o n
--------------------------------------------------------------------------------*/

Ref
CRootView::getSelectionStack(void)
{
	return fSelectionStack;
}


void
CRootView::cleanSelectionStack(CView * inView, bool inPrune)
{
	RefVar	viewContext;
	if (inView)
		viewContext = inView->fContext;
	for (ArrayIndex index = 0; index < Length(fSelectionStack); )
	{
		RefVar	selectionContext(GetArraySlot(fSelectionStack, index));
		if (ISNIL((GetProtoVariable(selectionContext, SYMA(viewCObject))))
		 || EQ(viewContext, selectionContext))
			ArrayRemoveCount(fSelectionStack, index, 2);
		else
			index += 2;
	}
	if (inPrune && Length(fSelectionStack) > 20)
		ArrayRemoveCount(fSelectionStack, 0, 10);
}


void
CRootView::pushSelection(CView * inView, RefArg inInfo)
{
	int	count;

	cleanSelectionStack(inView, YES);
	count = Length(fSelectionStack);
	SetLength(fSelectionStack, count + 2);
	SetArraySlot(fSelectionStack, count, inView->fContext);
	SetArraySlot(fSelectionStack, count+1, inInfo);
}


Ref
CRootView::popSelection(void)
{
	if (Length(fSelectionStack) == 0)
		return NILREF;

	cleanSelectionStack(NULL, NO);
	RefVar	caretInfo;
	int		count = Length(fSelectionStack);
	if (count > 0)
	{
		caretInfo = Clone(RA(canonicalCaretInfo));
		SetFrameSlot(caretInfo, SYMA(view), GetArraySlot(fSelectionStack, count - 2));
		SetFrameSlot(caretInfo, SYMA(info), GetArraySlot(fSelectionStack, count - 1));
		ArrayRemoveCount(fSelectionStack, count - 2, 2);
	}
	return caretInfo;
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C l i p b o a r d
--------------------------------------------------------------------------------*/

void
CRootView::addClipboard(RefArg inArg1, RefArg inArg2)
{
	RefVar	cmd(MakeCommand(aeAddData, this, 0x08000000));

	CommandSetFrameParameter(cmd, inArg1);
	gApplication->dispatchCommand(cmd);

	CommandSetFrameParameter(cmd, inArg2);
	gApplication->dispatchCommand(cmd);
}


Ref
CRootView::getClipboardIcons(void)
{
	return gRootView->fClipboardIcons;
}


CView *
CRootView::getClipboard(void)
{
	if (NOTNIL(fClipboardViews) && Length(fClipboardViews) > 0)
	{
		RefVar	clipboard(GetArraySlot(fClipboardViews, 0));
		return GetView(clipboard);
	}
	return NULL;
}


CView *
CRootView::getClipboardIcon(void)
{
	if (NOTNIL(fClipboardIcons) && Length(fClipboardIcons) > 0)
	{
		RefVar	clipboardIcon(GetArraySlot(fClipboardIcons, 0));
		return GetView(clipboardIcon);
	}
	return NULL;
}


CView *
CRootView::getClipboard(CView * inIconView)
{
	if (NOTNIL(fClipboardIcons) && Length(fClipboardIcons) > 0)
	{
		int	index = LSearch(fClipboardIcons, inIconView->fContext, MAKEINT(0), SYMA(_3D), RA(NILREF));	// '=
		if (index >= 0)
		{
			RefVar	clipboard(GetArraySlot(fClipboardViews, index));
			return GetView(clipboard);
		}
	}
	return NULL;
}


CView *
CRootView::getClipboardIcon(CClipboardView * inClipboardView)
{
	if (NOTNIL(fClipboardViews) && Length(fClipboardViews) > 0)
	{
		int	index = LSearch(fClipboardViews, inClipboardView->fContext, MAKEINT(0), SYMA(_3D), RA(NILREF));	// '=
		if (index >= 0)
		{
			RefVar	clipboardIcon(GetArraySlot(fClipboardIcons, index));
			return GetView(clipboardIcon);
		}
	}
	return NULL;
}


void
CRootView::removeClipboard(void)
{
	if (NOTNIL(fClipboardViews) && Length(fClipboardViews) > 0)
	{
		CView * vw = GetView(GetArraySlot(fClipboardViews, 0));
		if (vw)
			gApplication->dispatchCommand(MakeCommand(aeRemoveData, this, vw->fId));
	}

	if (NOTNIL(fClipboardIcons) && Length(fClipboardIcons) > 0)
	{
		CView * vw = GetView(GetArraySlot(fClipboardIcons, 0));
		if (vw)
			gApplication->dispatchCommand(MakeCommand(aeRemoveData, this, vw->fId));
	}
}

