/*
	File:		Animation.cc

	Contains:	Animation implementation.

	Written by:	Newton Research Group.
*/

#include "RootView.h"

#include "ROMResources.h"
#include "NewtonTime.h"
#include "MagicPointers.h"
#include "Sound.h"
#include "Preference.h"

#include "Screen.h"
#include "Animation.h"
#include "Geometry.h"
#include "DrawShape.h"


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FEffectX(RefArg rcvr, RefArg effect, RefArg offScreen, RefArg sound, RefArg methodName, RefArg methodParameters);
Ref	FSlideEffectX(RefArg rcvr, RefArg contentOffset, RefArg viewOffset, RefArg sound, RefArg methodName, RefArg methodParameters);
Ref	FRevealEffectX(RefArg rcvr, RefArg distance, RefArg bounds, RefArg sound, RefArg methodName, RefArg methodParameters);
Ref	FDeleteX(RefArg rcvr, RefArg methodName, RefArg methodParameters);
Ref	FDoScrubEffect(RefArg rcvr, RefArg inArg);
}

Ref
FEffectX(RefArg rcvr, RefArg effect, RefArg offScreen, RefArg sound, RefArg methodName, RefArg methodParameters)
{ return NILREF; }

Ref
FSlideEffectX(RefArg rcvr, RefArg contentOffset, RefArg viewOffset, RefArg sound, RefArg methodName, RefArg methodParameters)
{ return NILREF; }

Ref
FRevealEffectX(RefArg rcvr, RefArg distance, RefArg bounds, RefArg sound, RefArg methodName, RefArg methodParameters)
{ return NILREF; }

Ref
FDeleteX(RefArg rcvr, RefArg methodName, RefArg methodParameters)
{ return NILREF; }

Ref
FDoScrubEffect(RefArg rcvr, RefArg inArg)
{ return NILREF; }


/*------------------------------------------------------------------------------
	C A n i m a t e
------------------------------------------------------------------------------*/

void
DisposeCAnimate(void * inData)
{
	delete (CAnimate *)inData;
}


/*------------------------------------------------------------------------------
	Constructor.
	Set up an exception handler while the CAnimate is in scope.
	Mask out effects if preferred.
------------------------------------------------------------------------------*/

CAnimate::CAnimate()
{
	fContext = new RefStruct;	// why not make this a member rather than a pointer?
	fA5 = false;

	fException.header.catchType = kExceptionCleanup;
	fException.object = this;
	fException.function = DisposeCAnimate;
	AddExceptionHandler(&fException.header);

	Ref	noFX = GetPreference(SYMA(noFX));
	fMask = NOTNIL(noFX) ? ~RINT(noFX) : 0xFFFFFFFF;
}


/*------------------------------------------------------------------------------
	Destructor.
	Remove the exception handler as we leave scope.
------------------------------------------------------------------------------*/

CAnimate::~CAnimate()
{
	RemoveExceptionHandler(&fException.header);
	DisposeRefHandle(fContext->h);
}


/*------------------------------------------------------------------------------
	Perform an effect.
	Don’t bother with effects if we’re doing a view autopsy.
	Args:		inSoundEffect
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::doEffect(RefArg inSoundEffect)
{
	if (!FLAGTEST(fMask, 1 << fEffect) && fView != NULL)
	{
		gRootView->smartInvalidate(&fView->viewBounds);
		gRootView->update();
	}

	bool	isAnything = !EmptyRect(&f74);
	if (fA5 && (!isAnything /*|| fSaveScreen.allocateBuffers(&f74)*/))
	{
		if (gSlowMotion == 0)
			StartDrawing(NULL, NULL);

		Rect caretBounds = gRootView->getCaretRect();
		if (isAnything)
		{
			if (!EmptyRect(&f6C))
			{
//				CRectangularRegion	viewRgn(&fView->viewBounds);
//				gRootView->validate(viewRgn);
			}
			gRootView->update();

			// if the effect affects the caret, restore it before…
			Rect caretClip = caretBounds;
			SectRect(&f74, &caretClip, &caretClip);
			if (!EmptyRect(&caretClip))
				gRootView->restoreBitsUnderCaret();

			// …saving what’s on screen before we start
//			fSaveScreen.saveScreenBits();
		}

		// if the effect affects the caret, it’ll need redrawing after
		SectRect(&fxBox, &caretBounds, &caretBounds);
		if (!EmptyRect(&caretBounds))
			gRootView->dirtyCaret();

		if (gSlowMotion == 0)
		{
			switch (fEffect)
			{
			case 0:
			case kSlideEffect:
				multiEffect(inSoundEffect);
				break;
			case kTrashEffect:
				crumpleEffect();
				break;
			case kPoofEffect:
				poofEffect();
				break;
			}

			StopDrawing(NULL, NULL);
		}
	}

	else
		if (NOTNIL(*fContext))
			PlaySound(*fContext, inSoundEffect);
}


/*------------------------------------------------------------------------------
	Perform a multi-stage effect.
	Args:		inSoundEffect
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::multiEffect(RefArg inSoundEffect)
{
}


/*------------------------------------------------------------------------------
	Perform the crumpled-piece-of-paper effect.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::crumpleEffect(void)
{
}


void
CAnimate::crumpleSprite(Rect *, Rect *)
{
}


/*------------------------------------------------------------------------------
	Perform the puff-of-smoke effect.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::poofEffect(void)
{
	PlaySound(*fContext, ROM_poof);

	DrawPicture(ROM_cloud1, &fxBox, vjFullH + vjFullV, modeMask);
	StopDrawing(NULL, NULL);
	Wait(100);	// milliseconds - was 2 ticks

	StartDrawing(NULL, NULL);
//	fSaveScreen.restoreScreenBits(&f74, NULL);
	DrawPicture(ROM_cloud2, &fxBox, vjFullH + vjFullV, modeMask);
	StopDrawing(NULL, NULL);
	Wait(100);

	StartDrawing(NULL, NULL);
//	fSaveScreen.restoreScreenBits(&f74, NULL);
	DrawPicture(ROM_cloud3, &fxBox, vjFullH + vjFullV, modeMask);
	StopDrawing(NULL, NULL);
	Wait(100);

	StartDrawing(NULL, NULL);
//	fSaveScreen.restoreScreenBits(&f74, NULL);

	fView->dirty(&f74);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Set up a plain effect.
	Args:		inView		view in which to perform
				inArg2		
				inFX			fx definition (as defined by fx… in View.h)
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::setupPlainEffect(CView * inView, bool inArg2, int inFX)
{
	if (FLAGTEST(fMask, (1 << kPlainEffect)))
	{
		inView->outerBounds(&fxBox);

//		GrafPtr	thePort;
//		GetPort(&thePort);
//		Rect *	portBounds = &thePort->portRect;
//		if (RectGetHeight(fxBox) > RectGetHeight(*portBounds)
//		 || RectGetWidth(fxBox) > RectGetWidth(*portBounds))
//			SectRect(portBounds, &fxBox, &fxBox);

		preSetup(inView, kPlainEffect);

		if (inFX)
			fxDef = inFX;
		else
		{
			RefVar	viewFX(inView->getProto(SYMA(viewEffect)));
			fxDef = NOTNIL(viewFX) ? RINT(viewFX) : 0;
		}
		if (fxDef)
		{
			Rect	effectBounds;
			fA4 = inArg2;
			effectBounds = inArg2 ? fxBox : gZeroRect;	// actually just zeroes it out
			postSetup(&fxBox, &effectBounds, &fxBox);
		}
	}

	else
	{
		// plain effect is not in mask
		fView = inView;
		*fContext = inView->fContext;
	}
}


/*------------------------------------------------------------------------------
	Set up the trash effect.
	Args:		inView		view in which to perform
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::setupTrashEffect(CView * inView)
{
	if (FLAGTEST(fMask, (1 << kTrashEffect)))
	{
		inView->outerBounds(&fxBox);
		if (fxBox.bottom > gScreenHeight)
			fxBox.bottom = gScreenHeight;

		preSetup(inView, kTrashEffect);
		Rect	zeroBounds = gZeroRect;
		postSetup(&fxBox, &zeroBounds, &fxBox);

		if (f74.bottom - f74.top < 72)
		{
			short	boundTop = f74.bottom - 72;
			f74.top = (boundTop < 0) ? 0 : boundTop;
		}
		f74.bottom = gScreenHeight;
		f74.right = gScreenWidth;
	}

	else
	{
		// trash effect is not in mask
		fView = inView;
		*fContext = inView->fContext;
	}
}


/*------------------------------------------------------------------------------
	Set up a slide effect.
	Args:		inView		view in which to perform
				inBounds		area to slide
				inOffsetV	vertical offset
				inOffsetH	horizontal offset to next slide increment
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::setupSlideEffect(CView * inView, const Rect * inBounds, int inOffsetV, int inOffsetH)
{
	if (FLAGTEST(fMask, (1 << kSlideEffect)))
	{
		Rect	slideBounds = *inBounds;
		fxBox = *inBounds;

		preSetup(inView, kSlideEffect);
		long	slideHeight = inBounds->bottom - inBounds->top;
		f90 = slideHeight;
		fA4 = (inOffsetH >= 0);
		if (inOffsetH > 0)
		{
			if (inOffsetV > 0)
			{
				fxBox.bottom += inOffsetV;
				slideBounds.top = slideBounds.bottom - inOffsetV;
			}
			else
			{
				fxBox.top += inOffsetV;
	//			slideBounds.bottom = ?? - inOffsetV;
			}
		}

	//L65
		Rect	moreBounds = fxBox;
		if (inOffsetH == 0)
		{
			f98 = slideHeight;
			if (inOffsetV > 0)
			{
				fxBox.bottom += inOffsetV;
				slideBounds.top = slideBounds.bottom - inOffsetV;
			}
			else
			{
				fxBox.top += inOffsetV;
	//			slideBounds.bottom = ?? - inOffsetV;
			}
		}

	//L97
		else if (inOffsetH < 0)
		{
			f90 -= inOffsetV;
			if (inOffsetV > 0)
			{
				moreBounds.top = moreBounds.bottom + inOffsetV;
			}
			else
			{
				moreBounds.bottom = slideBounds.bottom + inOffsetV;
			}
		}

	//L114
		fxDef = ((inOffsetV > 0) != (inOffsetH >= 0)) ? fxUp : fxDown;
		postSetup(inBounds, &moreBounds, &slideBounds);

		if (inOffsetH != 0)
		{
			f7C = f6C;
			Point	nxLoc;
			nxLoc.h = fxBox.left;
			nxLoc.v = fxBox.bottom - (slideBounds.top - moreBounds.bottom);
			f84 = nxLoc;
		}
		else
		{
			OffsetRect(&f7C, 0, inOffsetV);
			f84 = *(Point*)&fxBox;
		}
	}

	else
	{
		// slide effect is not in mask
		fView = inView;
		*fContext = inView->fContext;
	}
}


/*------------------------------------------------------------------------------
	Set up the puff-of-smoke effect.
	Args:		inView		view in which to perform
				inBounds		area to be poofed
	Return:	--
------------------------------------------------------------------------------*/
#define kMinPoofWidth 89
#define kMinPoofHeight 54

void
CAnimate::setupPoofEffect(CView * inView, const Rect * inBounds)
{
	if (FLAGTEST(fMask, 1 << kPoofEffect))
	{
		fxBox = *inBounds;

		// don’t let the poof be too small - we want to see it!
		if (inBounds->right - inBounds->left < kMinPoofWidth
		||  inBounds->bottom - inBounds->top < kMinPoofHeight)
		{
			fxBox.left = (inBounds->left + inBounds->right)/2 - 44;
			fxBox.right = fxBox.left + kMinPoofWidth;
			fxBox.top = (inBounds->top + inBounds->bottom)/2 - 27;
			fxBox.bottom = fxBox.top + kMinPoofHeight;
		}

		preSetup(inView, kPoofEffect);
		Rect	zeroBounds = gZeroRect;
		postSetup(&fxBox, &zeroBounds, &fxBox);
	}

	else
	{
		// poof effect is not in mask
		fView = inView;
		*fContext = inView->fContext;
	}
}


/*------------------------------------------------------------------------------
	Set up a drag effect.
	Args:		inView		view in which to perform
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::setupDragEffect(CView * inView)
{
	fMask = 0xFFFFFFFF;
	setupPlainEffect(inView, false, fxDrawerEffect);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Set up common to all effects.
	Args:		inView		view in which to perform
				inWhat		effect number
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::preSetup(CView * inView, EffectKind inWhat)
{
	fView = inView;
	fEffect = inWhat;
	fA4 = false;
	f90 = 0;
	f94 = 0;
	f98 = 0x7FFE;
	*fContext = inView->fContext;
}


/*------------------------------------------------------------------------------
	Set up common to all effects.
	Args:		inBnd1		bounds…
				inBnd2		bounds…
				inBnd3		bounds…
	Return:	--
------------------------------------------------------------------------------*/

void
CAnimate::postSetup(const Rect * inBnd1, const Rect * inBnd2, const Rect * inBnd3)
{
	f6C = *inBnd2;
	TrimRect(inBnd1, inBnd2, &f74);
	fA5 = !EmptyRect(inBnd1);
	if (!fA5)
		return;

#if 0
	fA5 = fBits.init(inBnd3);
	if (!fA5)
		OutOfMemory();

	CRegion	mask = fView->getFrontMask();
	f60 = mask;
	CView *	view;
	for (view = fView->fParent; view != gRootView; view = view->fParent)
	{
		CRegion		mask = view->getFrontMask();
		CRegionVar	sp04(&mask);
		UnionRgn(f60, sp04, f60);
	}
//L73
// INCOMPLETE

//000435CC
		Rect	caretBounds;
		gRootView->getCaretRect(&caretBounds);
		SectRect(inBnd3, &caretBounds, &caretBounds);
		if (!EmptyRect(&caretBounds))
		{
			gRootView->restoreBitsUnderCaret();
			gRootView->dirtyCaret();
		}
//L168
		fBits.copyFromScreen(&sp54, &sp54, modeCopy, NULL);
		if (!EmptyRgn(sp18))	// CBaseRegion
		{
			fBits.beginDrawing(*(Point*)inBnd3);
			gSkipVisRegions = true;
			fView->update(sp18, NULL);
			gSkipVisRegions = false;
			fBits.endDrawing();
		}
//L196
		TrimRect(inBnd3, inBnd1, r8);
	}
	else
//L211
		*r8 = *r7;

//L213
	f84 = *(Point*)r8;
	fBits.setBounds(&fxBox);
#endif
}
