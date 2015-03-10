/*
	File:		Debug.cc

	Contains:	View debug functions.

	Written by:	Newton Research Group.
*/

#include "RootView.h"

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "RefMemory.h"
#include "FaultBlocks.h"

#include "Application.h"

#include "Geometry.h"
// from DrawShape.h -- but we donÕt want all the QD hassle
extern bool		FromObject(RefArg inObj, Rect * outBounds);
extern Ref		ToObject(const Rect * inBounds);

extern bool gOutlineViews;


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FDebug(RefArg inRcvr, RefArg inView);
Ref	FDebugMemoryStats(RefArg inRcvr);
Ref	FDebugRunUntilIdle(RefArg inRcvr);

Ref	FGetFrameStuff(RefArg inRcvr, RefArg inObj, RefArg inWhat);
Ref	FDV(RefArg inRcvr, RefArg inView);
Ref	FViewAutopsy(RefArg inRcvr, RefArg inWhat);
Ref	FInsetRect(RefArg inRcvr, RefArg ioRect, RefArg inX, RefArg inY);
Ref	FOffsetRect(RefArg inRcvr, RefArg ioRect, RefArg inX, RefArg inY);
}


Ref
FindForm(RefArg inRoot, RefArg inView)
{
	// INCOMPLETE
	return NILREF;
}


Ref
FDebug(RefArg inRcvr, RefArg inView)
{
	return FindForm(gRootView->fContext, inView);
}


Ref
FDebugMemoryStats(RefArg inRcvr)
{
	// this really does nothing
	return NILREF;
}


Ref
FDebugRunUntilIdle(RefArg inRcvr)
{
	gRootView->update();
	gApplication->run();
	while (gApplication->runNextDelayedAction())
	{
		gRootView->update();
	}
	return NILREF;
}



Ref
FGetFrameStuff(RefArg inRcvr, RefArg inObj, RefArg inWhat)
{
	Ref	stuff = NILREF;
	switch (RINT(inWhat))
	{
	case 0:
		if ((ObjectFlags(inObj) & kObjReadOnly) == 0)
			stuff = ((FrameObject *)ObjectPtr(inObj))->map;
		break;
	case 1:
		if (IsFaultBlock(inObj))
			stuff = ((FaultObject *)NoFaultObjectPtr(inObj))->handler;
		break;
	case 2:
		stuff = gApplication->getUndoStack(0);
		break;
	case 3:
		stuff = gApplication->getUndoStack(1);
		break;
	}
	return stuff;
}


Ref
FDV(RefArg inRcvr, RefArg inView)
{
	CView *  view = GetView(inRcvr, inView);
	if (NOTNIL(view))
	{
		view->dump(0);
		for (ArrayIndex i = 0; i < 8; ++i)
		{
			view->hilite(YES);
			Wait(100);
			view->hilite(NO);
			Wait(100);
		}
	}
	else
		REPprintf("No view found (hidden?)");
	return NILREF;
}


Ref
FViewAutopsy(RefArg inRcvr, RefArg inWhat)
{
	if (ISINT(inWhat))
		gSlowMotion = RINT(inWhat);
	else
	{
		gOutlineViews = !gOutlineViews;
		gRootView->dirty();
	}
	return NILREF;
}


Ref
FInsetRect(RefArg inRcvr, RefArg ioRect, RefArg inX, RefArg inY)
{
	Rect  box;
	if (FromObject(ioRect, &box))
	{
		int  dx = RINT(inX);
		int  dy = RINT(inY);
		if (dx | dy)
			InsetRect(&box, dx, dy);
		return ToObject(&box);
	}
	return NILREF;	
}


Ref
FOffsetRect(RefArg inRcvr, RefArg ioRect, RefArg inX, RefArg inY)
{
	Rect  box;
	if (FromObject(ioRect, &box))
	{
		int  dx = RINT(inX);
		int  dy = RINT(inY);
		if (dx | dy)
			OffsetRect(&box, dx, dy);
		return ToObject(&box);
	}
	return NILREF;	
}

