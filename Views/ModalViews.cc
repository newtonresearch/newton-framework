 /*
	File:		ModalViews.cc

	Contains:	Modal view utilities.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ROMResources.h"
#include "Lookup.h"
#include "Funcs.h"
#include "ArrayIterator.h"
#include "NewtonScript.h"

#include "NewtWorld.h"

#include "RootView.h"
#include "Modal.h"
#include "Recognition.h"
#include "Geometry.h"


extern "C" Ref	FOpenX(RefArg inRcvr);
extern bool	RealOpenX(RefArg inRcvr, bool inHuh);  // ViewUtils.cc


CDynamicArray *	gDelayedShowList;	// 0C101944
bool					gInhibitPopup;		// 0C101948

int		gModalCount = 0;				// 0C105524		should be initialized somewhere



/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FFilterDialog(RefArg inRcvr);
Ref	FModalDialog(RefArg inRcvr);
Ref	FExitModalDialog(RefArg inRcvr);
Ref	FModalState(RefArg inRcvr);

Ref	FGetPopup(RefArg inRcvr);
Ref	FSetPopup(RefArg inRcvr);
Ref	FClearPopup(RefArg inRcvr);
Ref	FDoPopup(RefArg inRcvr, RefArg inPickItems, RefArg inX, RefArg inY, RefArg inContext);
Ref	FDismissPopup(RefArg inRcvr);
}

/*--------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
--------------------------------------------------------------------------------*/

extern bool		FromObject(RefArg inObj, Rect * outBounds);
extern Ref		ToObject(const Rect * inBounds);


#pragma mark Dialogs
/*--------------------------------------------------------------------------------
	F u n c t i o n s
--------------------------------------------------------------------------------*/

Ref
FFilterDialog(RefArg inRcvr)
{
	NSSend(inRcvr, SYMA(StuffModalCommandKeys));
	if (!RealOpenX(inRcvr, true))
		return TRUEREF;

	if (gModalCount++ != 0)
		gRecognitionManager.disableModalRecognition();

	CView *  view = GetView(inRcvr);
	SetModalView(view);

	gRootView->update();
	gNewtWorld->handleEvents(0);

	SetFrameSlot(inRcvr, SYMA(modalState), RA(TRUEREF));
	return NILREF;
}


Ref
FModalDialog(RefArg inRcvr)
{
	NSSend(inRcvr, SYMA(StuffModalCommandKeys));
	if (!RealOpenX(inRcvr, true))
		return TRUEREF;

	gRootView->update();
	gNewtWorld->handleEvents(0);
/*
	CPseudoSyncState  pss;
	if (pss->init() == noErr)
	{
		RefVar	modalContext(inRcvr);
		gModalCount++;
		gRecognitionManager.disableModalRecognition();

		CView *  view = GetView(modalContext);
		if (view->fParent != gRootView)
		{
			view->viewJustify |= vIsModal;
			do view = view->fParent while (view->fParent != gRootView);
			modalContext = view->fContext;
		}
		SetModalView(view);
		SetFrameSlot(modalContext, SYMA(modalState), AddressToRef(&pss));

		bool  huh = false;
		ULong recState = gRecognitionManager.saveRecognitionState(&huh);
		if (huh == false)
			pss->block(0);
		gRecognitionManager.restoreRecognitionState(recState);
		SetFrameSlot(modalContext, SYMA(modalState), RA(NILREF));
		return TRUEREF;
	}
*/	return NILREF;
}


Ref
FModalState(RefArg inRcvr)
{
	return MAKEBOOLEAN(gModalCount > 0);
}


void
SetModalView(CView * inView)
{
	if (inView->viewJustify & vIsModal)
		gRecognitionManager.disableModalRecognition();
	inView->viewJustify |= vIsModal;

	Rect  box;
	inView->outerBounds(&box);
	gRecognitionManager.enableModalRecognition(&box);
}


void
ModalSafeShow(CView * inView)
{
/*
	if (gDelayedShowList == NULL)
		gDelayedShowList = new CDynamicArray;
	gDelayedShowList->insertElementsBefore(0, &inView, 1);
	if (ViewContainsCaretView(inView))
	{
		CRootView *	root = gRootView;
		RefVar		selection = root->getSelection();
		root->holdPendingKeyView(&root->f68->f24, selection);
		root->setKeyView(NULL, 0, 0, false);
	}
*/
}


void
RealExitModalDialog(CView * inView)
{
// INCOMPLETE
}


Ref
FExitModalDialog(RefArg inRcvr)
{
	CView * view = GetView(inRcvr);
	RealExitModalDialog(view);
	return TRUEREF;
}


void
RemoveModalSafeView(CView * inView)
{
	if (gDelayedShowList)
	{
		CArrayIterator	iter(gDelayedShowList);
		for (int index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
		{
			CViewStuff *	p = (CViewStuff *)gDelayedShowList->safeElementPtrAt(index);
			if (p->fView == inView)
			{
				gDelayedShowList->removeElementsAt(index, 1);
				break;
			}
		}
	}
}


#pragma mark Popups
/*--------------------------------------------------------------------------------
	P o p u p s
--------------------------------------------------------------------------------*/

Ref
FGetPopup(RefArg inRcvr)
{
	CView * popup = gRootView->getPopup();
	return popup ? (Ref)popup->fContext : NILREF;
}


Ref
FSetPopup(RefArg inRcvr)
{
	CView * popup = GetView(inRcvr);
	if (popup)
		gRootView->setPopup(popup, true);
	return NILREF;
}


Ref
FClearPopup(RefArg inRcvr)
{
	CView * popup = gRootView->getPopup();
	if (popup)
		gRootView->setPopup(NULL, false);
	return NILREF;
}


Ref
FDoPopup(RefArg inRcvr, RefArg inPickItems, RefArg inX, RefArg inY, RefArg inContext)
{
// r7 r5 r4 r6 r9
	if (Length(inPickItems) == 0 || gInhibitPopup)
	{
		if (!EQ(inContext, RA(TRUEREF)))
		{
			bool exists;
			GetVariable(inContext, SYMA(pickCancelledScript), &exists);
			if (exists)
				DoMessage(inContext, SYMA(pickCancelledScript), RA(NILREF));
		}
		return NILREF;
	}

	CView * view = GetView(inRcvr);		// r7
	Rect bounds;
	bool r8 = true;
	if (IsFrame(inX) && FromObject(inX, &bounds))
	{
		if (view)
			OffsetRect(&bounds, view->viewBounds.left, view->viewBounds.top);
	}
	else
	{
		int x, y;
		if (ISINT(inX)) x = RVALUE(inX); else x = 0;
		if (ISINT(inY)) { y = RVALUE(inY); r8 = false; } else y = 0;
		if (view)
		{
			if (r8)
				view->outerBounds(&bounds);
			else
				bounds = view->viewBounds;
			bounds.top += y;
			if (r8 && ISINT(inX))
				bounds.right += x;
			else
				bounds.left += x;
		}
		else
		{
			r8 = false;
			bounds.top = y;
			bounds.left = x;
			bounds.bottom = y;
			bounds.right = x;
		}
	}

	RefVar popupTemplate(Clone(RA(canonicalPopup)));
	RefVar popupBounds(ToObject(&bounds));
	if (r8)
		SetFrameSlot(popupTemplate, SYMA(info), AddressToRef(view));
	SetFrameSlot(popupTemplate, SYMA(bounds), popupBounds);
	SetFrameSlot(popupTemplate, SYMA(pickItems), inPickItems);
	if (!EQ(inContext, RA(TRUEREF)))
		SetFrameSlot(popupTemplate, SYMA(callbackContext), inContext);

	RefVar popupView(view->buildContext(popupTemplate, true));
	if (gModalCount == 0)
		FOpenX(popupView);
	else
		FFilterDialog(popupView);
	gRecognitionManager.ignoreClicks(kNoTimeout);
	
	return NOTNIL(popupView) ? popupView : popupTemplate;
}


Ref
FDismissPopup(RefArg inRcvr)
{
	CView * popup = gRootView->getPopup();
	if (popup)
		gRootView->setPopup(NULL, true);
	if (gRootView->getPopup() == NULL)
		gInhibitPopup = false;
	return NILREF;
}

