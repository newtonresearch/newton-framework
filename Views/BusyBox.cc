/*
	File:		BusyBox.cc

	Contains:	System busy indicator.

	Written by:	Newton Research Group.
*/

#include "Quartz.h"
#include "RootView.h"
#include "BusyBox.h"

#include "Objects.h"
#include "AppWorld.h"

extern CGImageRef LoadPNG(const char * inName);

#define kBusyBoxWidth 32
#define kBusyBoxHeight 32


/* -----------------------------------------------------------------------------
	C B u s y B o x
----------------------------------------------------------------------------- */

CBusyBox::CBusyBox()
	:	CTimerElement(static_cast<CAppWorld*>(GetGlobals())->fTimers, 0)
{
	fVisible = 0;

#if defined(correct)
	int screenDepth;

	GetGrafInfo(kGrafPixelDepth, &screenDepth);
	fPix.rowBytes = screenDepth * 4 /*bytes*/;	// busybox image is 32 bits wide
	fPix.pixMapFlags = kPixMapPtr + screenDepth;
	fPix.deviceRes.h =
	fPix.deviceRes.v = kDefaultDPI;
#else
	fImage = LoadPNG("busy");
#endif
}


CBusyBox::~CBusyBox()
{
	CGImageRelease(fImage);
}


void
CBusyBox::doCommand(long cmd)
{
	switch (cmd)
	{
	case 51:
		showBusyBox();
		break;
	case 52:
		hideBusyBox();
		break;
	case 54:
		hideBusyBox();
		prime(1*kSeconds);
		fVisible = -1;
		break;
	case 53:
	case 55:
		hideBusyBox();
		cancel();
		break;
	}
}


void
CBusyBox::timeout(void)
{
	showBusyBox();
}


void
CBusyBox::showBusyBox(void)
{
	if (!fVisible)
	{
#if defined(correct)
		QDShowBusyBox(&fPix);
#else
		CGRect  r;
		r.origin.x = (gScreenWidth - kBusyBoxWidth)/2;
		r.origin.y = gScreenHeight - kBusyBoxHeight;
		r.size.width = kBusyBoxWidth;
		r.size.height = kBusyBoxHeight;

		CGContextDrawImage(quartz, r, fImage);
#endif
	}
	fVisible = 1;
}


void
CBusyBox::hideBusyBox(void)
{
	if (fVisible)
	{
#if defined(correct)
		QDHideBusyBox(&fPix);
#else
		Rect box;
		box.top = 0;
		box.left = (gScreenWidth - kBusyBoxWidth)/2;
		box.bottom = box.top + kBusyBoxHeight;
		box.right = box.left + kBusyBoxWidth;
		gRootView->update(&box);
#endif
	}
	fVisible = 0;
}
