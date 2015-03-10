/*
	File:		FlashTracker.cc

	Contains:	Flash usage tracker implementation.
	
	Written by:	Newton Research Group, 2010.
*/

#include "FlashTracker.h"


/*------------------------------------------------------------------------------
	C F l a s h T r a c k e r
	Okay, so it tracks the addition and removal of PSSIds -- but to what end?
------------------------------------------------------------------------------*/

CFlashTracker::CFlashTracker(ArrayIndex inSize)
{
	fSize = inSize;
	fList = (PSSId *)NewPtr(inSize * sizeof(PSSId));
	reset();
}


CFlashTracker::~CFlashTracker()
{
	if (fList)
		FreePtr((Ptr)fList);
}


void
CFlashTracker::reset(void)
{
	fIndex = 0;
	fIsFull = NO;
	fLockCount = 0;
}


void
CFlashTracker::add(PSSId inId)
{
	if (!fIsFull)
	{
		fList[fIndex++] = inId;
		if (fIndex >= fSize)
			fIsFull = YES;
	}
}


void
CFlashTracker::remove(PSSId inId)
{
	for (ArrayIndex i = 0; i < fIndex; ++i)
	{
		if (fList[i] == inId)
		{
			fList[i] = kNoPSSId;
			break;
		}
	}
}


void
CFlashTracker::lock(void)
{
	fLockCount++;
}


void
CFlashTracker::unlock(void)
{
	if (fLockCount > 0)
		fLockCount--;
}

