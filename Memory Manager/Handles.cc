/*
	File:		Handles.cc

	Contains:	Memory manager - handles functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "MemMgr.h"

#define usesMacMemMgr 1

#if !usesMacMemMgr
/*------------------------------------------------------------------------------
	H a n d l e   o p e r a t i o n s

	We no longer use Handles.
	There is no need for them in a sufficiently large heap.
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create a new handle.
	Args:		inSize		size required
	Return:	a handle
------------------------------------------------------------------------------*/

Handle
NewHandle(size_t inSize)
{
	if (sMMBreak_Enabled)
	{
		gMemMgrCallCounter++;
		if (gMemMgrCallCounter == sMMBreak_OnCount)
			ReportMemMgrTrap('#');
		if (sMMBreak_LT != 0 && inSize < sMMBreak_LT)
			ReportMemMgrTrap('<');
		else if (inSize >= sMMBreak_GE)
			ReportMemMgrTrap('>=');
	}

	NewtonErr	err;
	void *	whereSmashed;
	bool		hasSemaphore;
	Heap		savedHeap = GetHeap();
	Heap		hndlHeap = GetRelocHeap(GetHeap());

	if (hndlHeap == savedHeap)
		savedHeap = NULL;
	else
		SetHeap(hndlHeap);

	hasSemaphore = GetHeapSemaphore(NULL) != NULL;
	if (hasSemaphore)
		GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
	if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
		ReportSmashedHeap("entering", err, whereSmashed);

	Handle	h= NewIndirectBlock(inSize);
	if (h == NULL)
		err = memFullErr;
	else
	{
		gHandlesUsed += inSize;
		err = noErr;
	}
	TaskSwitchedGlobals()->fMemErr = err;

	if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
		ReportSmashedHeap("leaving", err, whereSmashed);
	if (hasSemaphore)
		GetHeapSemaphore(NULL)->release();

	if (sMMTagBlocksWithCCHash)
		SetHandleName(h, HashCallChain() & ~0x80000000);

	if (sMMBreak_AddressOut != 0 && sMMBreak_AddressOut == (Ptr)h)
		ReportMemMgrTrap('&');

	if (savedHeap != NULL)
		SetHeap(savedHeap);

	return h;
}


/*------------------------------------------------------------------------------
	Create a new named handle.
	Args:		inSize		size required
				inName		its name
	Return:	a handle
------------------------------------------------------------------------------*/

Handle
NewNamedHandle(size_t inSize, ULong inName)
{
	Handle h = NewHandle(inSize);
	if (h != NULL)
		SetHandleName(h, inName);
	return h;
}


/*------------------------------------------------------------------------------
	Create a new handle and zero its contents.
	Args:		inSize		size required
	Return:	a handle
------------------------------------------------------------------------------*/

Handle
NewHandleClear(size_t inSize)
{
	Handle h = NewHandle(inSize);
	if (h != NULL)
	{
		ClearMemory(HLock(h), inSize);
		HUnlock(h);
	}
	return h;
}


/*------------------------------------------------------------------------------
	Dispose of a handle.
	Args:		inH			the handle in question
	Return:	--
------------------------------------------------------------------------------*/

void
FreeHandle(Handle inH)
{
	if (sMMBreak_Enabled)
	{
		gMemMgrCallCounter++;
		if (gMemMgrCallCounter == sMMBreak_OnCount)
			ReportMemMgrTrap('#');
		if (sMMBreak_AddressIn != 0 && sMMBreak_AddressIn == (Ptr)inH)
			ReportMemMgrTrap('&');
	}

	if (IsFakeHandle(inH))
	{
		NewtonErr	err;
		void *	whereSmashed;
		bool	hasSemaphore = GetHeapSemaphore(NULL) != NULL;
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("entering", err, whereSmashed);

		TaskSwitchedGlobals()->fMemErr = noErr;
		DisposeIndirectBlock(inH);

		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("leaving", err, whereSmashed);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->release();
		return;
	}

	if (inH != NULL)
	{
		NewtonErr	err;
		void *	whereSmashed;
		bool		hasSemaphore;
		Heap		savedHeap = GetHeap();
		Heap		hndlHeap = HandleToHeap(inH);

		if (hndlHeap == savedHeap)	// handle in NULL current heap is allowed
			savedHeap = NULL;
		else if (hndlHeap == NULL)	// but not NULL heap otherwise
			return;
		else
			SetHeap(hndlHeap);

		hasSemaphore = GetHeapSemaphore(NULL) != NULL;
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("entering", err, whereSmashed);

		SetBlockBusy(*inH, 0);
		TaskSwitchedGlobals()->fMemErr = noErr;
		size_t size = GetIndirectBlockSize(inH);
		DisposeIndirectBlock(inH);
		gHandlesUsed -= size;
		if (TotalFreeInHeap(NULL) > 512)
			ShrinkHeapLeaving(GetHeap(), 0);

		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("leaving", err, whereSmashed);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->release();

		if (savedHeap != NULL)
			SetHeap(savedHeap);
	}
}


/*------------------------------------------------------------------------------
	Get a handle’s size.
	Args:		inH			the handle in question
	Return:	its size
------------------------------------------------------------------------------*/

size_t
GetHandleSize(Handle inH)
{
	size_t size;

	if (sMMBreak_Enabled)
	{
		gMemMgrCallCounter++;
		if (gMemMgrCallCounter == sMMBreak_OnCount)
			ReportMemMgrTrap('#');
		if (sMMBreak_AddressIn != 0 && sMMBreak_AddressIn == (Ptr)inH)
			ReportMemMgrTrap('&');
	}

	if (IsFakeIndirectBlock(inH))
		return GetFakeIndirectBlockSize(inH);

	if (inH != NULL)
	{
		NewtonErr	err;
		void *	whereSmashed;
		bool	hasSemaphore = GetHeapSemaphore(NULL) != NULL;
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("entering", err, whereSmashed);

		TaskSwitchedGlobals()->fMemErr = noErr;
		size = GetIndirectBlockSize(inH);

		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("leaving", err, whereSmashed);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->release();
	}

	else
		size = 0;

	return size;
}


/*------------------------------------------------------------------------------
	Set a handle’s size.
	Args:		inH			the handle in question
				inSize		its new size
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
SetHandleSize(Handle inH, size_t inSize)
{
	NewtonErr	err;

	if (sMMBreak_Enabled)
	{
		gMemMgrCallCounter++;
		if (gMemMgrCallCounter == sMMBreak_OnCount)
			ReportMemMgrTrap('#');
		if (sMMBreak_AddressIn != 0 && sMMBreak_AddressIn == (Ptr)inH)
			ReportMemMgrTrap('&');
		if (sMMBreak_LT != 0 && inSize < sMMBreak_LT)
			ReportMemMgrTrap('<');
		else if (inSize >= sMMBreak_GE)
			ReportMemMgrTrap('>=');
	}

	if (IsFakeHandle(inH))
		return (TaskSwitchedGlobals()->fMemErr = -1);

	if (inH != NULL)
	{
		void *	whereSmashed;
		bool		hasSemaphore;
		Heap		savedHeap = GetHeap();
		Heap		hndlHeap = HandleToHeap(inH);

		if (hndlHeap == savedHeap)	// handle in NULL current heap is allowed
			savedHeap = NULL;
		else if (hndlHeap == NULL)	// but not NULL heap otherwise
			return (TaskSwitchedGlobals()->fMemErr = -1);
		else
			SetHeap(hndlHeap);

		hasSemaphore = GetHeapSemaphore(NULL) != NULL;
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("entering", err, whereSmashed);

		size_t size = GetIndirectBlockSize(inH);
		if (SetIndirectBlockSize(inH, inSize) == NULL)
			err = memFullErr;
		else
		{
			err = noErr;
			if (size <= inSize)
				gHandlesUsed += (inSize - size);
			else
				gHandlesUsed -= (size - inSize); 
		}

		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("leaving", err, whereSmashed);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->release();

		if (savedHeap != NULL)
			SetHeap(savedHeap);
	}

	return (TaskSwitchedGlobals()->fMemErr = err);
}


/*------------------------------------------------------------------------------
	Get a handle’s owner. (It’s the same as for a pointer.)
	Args:		inH			the handle in question
	Return:	its owner
------------------------------------------------------------------------------*/

ObjectId
GetHandleOwner(Handle inH)
{
	return (inH != NULL) ? GetPtrOwner(*inH) : 0;
}


/*------------------------------------------------------------------------------
	Set a handle’s owner. (It’s the same as for a pointer.)
	Args:		inH			the handle in question
				inOwner		its owner
	Return:	--
------------------------------------------------------------------------------*/

void
SetHandleOwner(Handle inH, ObjectId inOwner)
{
	if (inH != NULL)
		SetPtrOwner(*inH, inOwner);
}


/*------------------------------------------------------------------------------
	Get a handle’s name. (It’s the same as for a pointer.)
	Args:		inH			the handle in question
	Return:	its name
------------------------------------------------------------------------------*/

ULong
GetHandleName(Handle inH)
{
	return (inH != NULL) ? GetPtrName(*inH) : 0;
}


/*------------------------------------------------------------------------------
	Set a handle’s name. (It’s the same as for a pointer.)
	Args:		inH			the handle in question
				inName		its name
	Return:	--
------------------------------------------------------------------------------*/

void
SetHandleName(Handle inH, ULong name)
{
	if (inH != NULL && !sMMTagBlocksWithCCHash)
		SetPtrName(*inH, name);
}


/*------------------------------------------------------------------------------
	Get a handle’s type. (It’s the same as for a pointer.)
	Args:		inH			the handle in question
	Return:	its type
------------------------------------------------------------------------------*/

HeapBlockType
GetHandleType(Handle inH)
{
	return (inH != NULL) ? GetPtrType(*inH) : 0;
}


/*------------------------------------------------------------------------------
	Set a handle’s type. (It’s the same as for a pointer.)
	Args:		inH			the handle in question
				inType		its type
	Return:	--
------------------------------------------------------------------------------*/

void
SetHandleType(Handle inH, HeapBlockType inType)
{
	if (inH != NULL)
		SetPtrType(*inH, inType);
}


/*------------------------------------------------------------------------------
	Return the heap to which a handle belongs.
	Note semantics of next - when handle is allocated it points back to heap.
	Args:		inH			the handle in question
	Return:	a heap
------------------------------------------------------------------------------*/

Heap
HandleToHeap(Handle inH)
{
	return (inH != NULL) ? ((MasterPtr *)inH)->next : NULL;
}


/*------------------------------------------------------------------------------
	Lock a handle so it won’t move in memory.
	We can use its pointer while it’s locked.
	Args:		inH			the handle in question
	Return:	a pointer
------------------------------------------------------------------------------*/

Ptr
HLock(Handle inH)
{
	Ptr	p;

	if (IsFakeHandle(inH))
		return *inH;

	if (inH != NULL)
	{
		NewtonErr	err;
		void *	whereSmashed;
		bool	hasSemaphore = GetHeapSemaphore(NULL) != NULL;
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("entering", err, whereSmashed);

		IncrementBlockBusy(*inH);
		TaskSwitchedGlobals()->fMemErr = noErr;
		p = *inH;

		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("leaving", err, whereSmashed);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->release();
	}

	else
	{
		TaskSwitchedGlobals()->fMemErr = -1;
		p = NULL;
	}

	return p;
}


/*------------------------------------------------------------------------------
	Unlock a handle so it can move in memory again.
	Any pointer derived from the handle may become invalid!
	Args:		inH			the handle in question
	Return:	--
------------------------------------------------------------------------------*/

void
HUnlock(Handle inH)
{
	if (IsFakeHandle(inH))
		return;

	if (inH != NULL)
	{
		NewtonErr	err;
		void *	whereSmashed;
		bool		hasSemaphore;
		Heap		savedHeap = GetHeap();
		Heap		hndlHeap = HandleToHeap(inH);

		if (hndlHeap == savedHeap)	// handle in NULL current heap is allowed
			savedHeap = NULL;
		else if (hndlHeap == NULL)	// but not NULL heap otherwise
			return;
		else
			SetHeap(hndlHeap);

		hasSemaphore = GetHeapSemaphore(NULL) != NULL;
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("entering", err, whereSmashed);

		DecrementBlockBusy(*inH);
		TaskSwitchedGlobals()->fMemErr = noErr;

		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("leaving", err, whereSmashed);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->release();

		if (savedHeap != NULL)
			SetHeap(savedHeap);
	}
}


/*------------------------------------------------------------------------------
	Duplicate a handle.
	Args:		inH			the handle in question
	Return:	a copy of the original handle
------------------------------------------------------------------------------*/

Handle
CopyHandle(Handle inH)
{
	if (inH == NULL || *inH == NULL)
		return NULL;

	size_t size = GetHandleSize(inH);
	Handle newH = NewHandle(size);
	if (newH != NULL)
	{
		memmove(HLock(newH), HLock(inH), size);
		HUnlock(inH);
		HUnlock(newH);
	}
	return newH;
}


/*------------------------------------------------------------------------------
	Copy a handle into a new handle.
	Very similar to CopyHandle().
	Args:		ioH			a pointer to the handle in question
	Return:	error code
				the new handle is return in *ioH
------------------------------------------------------------------------------*/

NewtonErr
HandToHand(Handle * ioH)
{
	if (ioH == NULL || *ioH == NULL)
		return -1;

	size_t size = GetHandleSize(*ioH);
	Handle newH = NewHandle(size);
	if (newH == NULL)
		return -1;

	memmove(HLock(newH), HLock(*ioH), size);
	HUnlock(newH);
	HUnlock(*ioH);
	*ioH = newH;
	return noErr;
}
#endif	/* usesMacMemMgr */

#pragma mark -

/*------------------------------------------------------------------------------
	F a k e   H a n d l e   O p e r a t i o n s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create a fake handle.
	Args:		inAddr		address of (typically ROM) resource to make handle from
				inSize		size of handle
	Return:	fake handle
------------------------------------------------------------------------------*/

Handle
NewFakeHandle(void * inAddr, size_t inSize)
{
	Handle	fakeH;
	Heap		savedHeap = GetHeap();
	Heap		hndlHeap = GetRelocHeap(GetHeap());

	if (hndlHeap == savedHeap)
		savedHeap = NULL;
	else
		SetHeap(hndlHeap);

	bool hasSemaphore = GetHeapSemaphore(NULL) != NULL;
	if (hasSemaphore)
		GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);

	fakeH = NewFakeIndirectBlock(inAddr, inSize);

	if (hasSemaphore)
		GetHeapSemaphore(NULL)->release();

	if (savedHeap != NULL)
		SetHeap(savedHeap);

	return fakeH;
}


/*------------------------------------------------------------------------------
	Determine whether a handle is genuine or fake.
	Args:		inH			the handle in question
	Return:	true => it’s a fake
------------------------------------------------------------------------------*/

bool
IsFakeHandle(Handle inH)
{
	return IsFakeIndirectBlock(inH);
}

