/*
	File:		Pointers.cc

	Contains:	Memory manager - pointer functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "MemMgr.h"

/*------------------------------------------------------------------------------
	P o i n t e r   O p e r a t i o n s

	The original Newton system uses malloc() extensively. Presumably this made it
	easy to port the system to any C runtime for testing. The ROM defines malloc()
	as a call to NewPtr().

	There are cases where we need to know the size of an allocated block. Sadly,
	malloc_size does not return the actual size, just a minimum size so we still
	need GetPtrSize() and therefore NewPtr().
	To remove NewPtr/free errors, we make more extensive use of NewPtr(),
	reserving malloc() for genuine host platform interface.
	Our implementation of NewPtr() calls malloc() but adds size/name info to an
	allocated block.
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create a new pointer.
	Args:		inSize		size required
	Return:	a pointer
------------------------------------------------------------------------------*/

Ptr
NewPtr(Size inSize)
{
	Ptr newPtr;

	if (sMMBreak_Enabled)
	{
		gMemMgrCallCounter++;
		if (gMemMgrCallCounter == sMMBreak_OnCount)
			ReportMemMgrTrap('#');
		if (sMMBreak_OnCCHash != 0
		&& (sMMBreak_OnCCHash & ~0x80000000) == (HashCallChain() & ~0x80000000))
			ReportMemMgrTrap('hash');
		if (sMMBreak_LT != 0 && inSize < sMMBreak_LT)
			ReportMemMgrTrap('<');
		else if (inSize >= sMMBreak_GE)
			ReportMemMgrTrap('>=');
	}

	if (!gOSIsRunning || IsSafeHeap(GetHeap()))
	{
		// OS isn’t running yet, or heap is safe
		bool		hasSemaphore = (GetHeapSemaphore(NULL) != NULL);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);

		newPtr = (Ptr) SafeHeapAlloc(inSize, (SSafeHeapPage *)gKernelHeap);
		if (newPtr != NULL)
			gPtrsUsed += inSize;

		if (hasSemaphore)
			GetHeapSemaphore(NULL)->release();
	}
	else
	{
		NewtonErr	err;
		void *	whereSmashed;
		bool		hasSemaphore = (GetHeapSemaphore(NULL) != NULL);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("entering", err, whereSmashed);

		newPtr = NewDirectBlock(inSize);
		if (newPtr == NULL)
			err = memFullErr;
		else
		{
			gPtrsUsed += inSize;
			IncrementBlockBusy(newPtr);
			err = noErr;
		}
		TaskSwitchedGlobals()->fMemErr = err;

		if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
			ReportSmashedHeap("leaving", err, whereSmashed);
		if (hasSemaphore)
			GetHeapSemaphore(NULL)->release();

		if (sMMTagBlocksWithCCHash)
			SetPtrName(newPtr, HashCallChain() & ~0x80000000);
	}

	if (sMMBreak_AddressOut != 0 && sMMBreak_AddressOut == newPtr)
		ReportMemMgrTrap('&');

	return newPtr;
}


/*------------------------------------------------------------------------------
	Create a new named pointer.
	Args:		inSize		size required
				inName		its name
	Return:	a pointer
------------------------------------------------------------------------------*/

Ptr
NewNamedPtr(Size inSize, ULong inName)
{
	Ptr p = NewPtr(inSize);
	if (p != NULL)
		SetPtrName(p, inName);
	return p;
}


/*------------------------------------------------------------------------------
	Create a new pointer and zero its contents.
	Args:		inSize		size required
	Return:	a pointer
------------------------------------------------------------------------------*/

Ptr
NewPtrClear(Size inSize)
{
	Ptr p = NewPtr(inSize);
	if (p != NULL)
		memset(p, 0, inSize);
	return p;
}


/*------------------------------------------------------------------------------
	Dispose of a pointer.
	Args:		inPtr			the pointer in question
	Return:	--
------------------------------------------------------------------------------*/

void
FreePtr(Ptr inPtr)
{
	if (sMMBreak_Enabled)
	{
		gMemMgrCallCounter++;
		if (gMemMgrCallCounter == sMMBreak_OnCount)
			ReportMemMgrTrap('#');
		if (sMMBreak_AddressIn != 0 && sMMBreak_AddressIn == inPtr)
			ReportMemMgrTrap('&');
	}

	if (inPtr != NULL)
	{
		if (IsSafeHeap(GetHeap()))
		{
			bool		hasSemaphore = (GetHeapSemaphore(NULL) != NULL);
			if (hasSemaphore)
				GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);

			Size size = SafeHeapBlockSize(inPtr);
			SafeHeapFree(inPtr);
			gPtrsUsed -= size;
			TaskSwitchedGlobals()->fMemErr = noErr;		// original doesn’t do this!

			if (hasSemaphore)
				GetHeapSemaphore(NULL)->release();
		}

		else
		{
			NewtonErr	err;
			void *	whereSmashed;
			Heap		prevHeap = GetHeap();
			Heap		pntrHeap = PtrToHeap(inPtr);

			if (pntrHeap == prevHeap)	// pointer in NULL current heap is allowed
				prevHeap = NULL;
			else if (pntrHeap == NULL)	// but not NULL heap otherwise
				return;
			else
				SetHeap(pntrHeap);

			bool		hasSemaphore = (GetHeapSemaphore(NULL) != NULL);
			if (hasSemaphore)
				GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
			if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
				ReportSmashedHeap("entering", err, whereSmashed);

			Size size = GetDirectBlockSize(inPtr);
			DecrementBlockBusy(inPtr);
			DisposeDirectBlock(inPtr);
			gPtrsUsed -= size;
			TaskSwitchedGlobals()->fMemErr = noErr;
			ShrinkHeapLeaving(GetHeap(), 0);

			if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
				ReportSmashedHeap("leaving", err, whereSmashed);
			if (hasSemaphore)
				GetHeapSemaphore(NULL)->release();

			if (prevHeap != NULL)
				SetHeap(prevHeap);
		}
	}
}


/*------------------------------------------------------------------------------
	Set a pointer’s size.
	(The function formerly known as SetPtrSize.)
	Args:		inPtr			the pointer in question
				inSize		its new size
	Return:	error code
------------------------------------------------------------------------------*/

Ptr
ReallocPtr(Ptr inPtr, Size inSize)
{
	Ptr	newPtr;

	if (sMMBreak_Enabled)
	{
		gMemMgrCallCounter++;
		if (gMemMgrCallCounter == sMMBreak_OnCount)
			ReportMemMgrTrap('#');
		if (sMMBreak_AddressIn != 0 && sMMBreak_AddressIn == inPtr)
			ReportMemMgrTrap('&');
		if (sMMBreak_LT != 0 && inSize < sMMBreak_LT)
			ReportMemMgrTrap('<');
		else if (inSize >= sMMBreak_GE)
			ReportMemMgrTrap('>=');
	}

	if (inPtr != NULL)
	{
		if (IsSafeHeap(GetHeap()))
		{
			bool		hasSemaphore = (GetHeapSemaphore(NULL) != NULL);
			if (hasSemaphore)
				GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);

			newPtr = (Ptr) SafeHeapRealloc(inPtr, inSize);
			TaskSwitchedGlobals()->fMemErr = noErr;		// original doesn’t do this!

			if (hasSemaphore)
				GetHeapSemaphore(NULL)->release();
		}
		else
		{
			Heap prevHeap = GetHeap();
			Heap pntrHeap = PtrToHeap(inPtr);

			if (pntrHeap == prevHeap)	// pointer in NULL current heap is allowed
				prevHeap = NULL;
			else if (pntrHeap == NULL)	// but not NULL heap otherwise
				return NULL;
			else
				SetHeap(pntrHeap);

			NewtonErr	err;
			void *	whereSmashed;
			bool		hasSemaphore = (GetHeapSemaphore(NULL) != NULL);
			if (hasSemaphore)
				GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);
			if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
				ReportSmashedHeap("entering", err, whereSmashed);

			Size prevSize = GetDirectBlockSize(inPtr);
			DecrementBlockBusy(inPtr);
			newPtr = (Ptr) SetDirectBlockSize(inPtr, inSize);
			if (newPtr == NULL)
			{
				TaskSwitchedGlobals()->fMemErr = memFullErr;
				IncrementBlockBusy(inPtr);
			}
			else
			{
				TaskSwitchedGlobals()->fMemErr = noErr;
				IncrementBlockBusy(newPtr);
				if (prevSize <= inSize)
					gPtrsUsed += (inSize - prevSize);
				else
					gPtrsUsed -= (prevSize - inSize); 
			}

			if ((gNewtConfig & 0x40) != 0 && (err = CheckHeap(NULL, &whereSmashed)) != noErr)
				ReportSmashedHeap("leaving", err, whereSmashed);
			if (hasSemaphore)
				GetHeapSemaphore(NULL)->release();

			if (prevHeap != NULL)
				SetHeap(prevHeap);
		}
	}

	else
		newPtr = NewPtr(inSize);

	if (sMMBreak_AddressOut != 0 && sMMBreak_AddressOut == newPtr)
		ReportMemMgrTrap('&');

	return newPtr;
}


/*------------------------------------------------------------------------------
	Get a pointer’s size.
	Args:		inPtr			the pointer in question
	Return:	its size
------------------------------------------------------------------------------*/

Size
GetPtrSize(Ptr inPtr)
{
	if (sMMBreak_Enabled)
	{
		gMemMgrCallCounter++;
		if (gMemMgrCallCounter == sMMBreak_OnCount)
			ReportMemMgrTrap('#');
		if (sMMBreak_AddressIn != 0 && sMMBreak_AddressIn == inPtr)
			ReportMemMgrTrap('&');
	}

	return (inPtr != NULL) ? GetDirectBlockSize(inPtr) : 0;
}


/*------------------------------------------------------------------------------
	Get a pointer’s owner.
	Args:		inPtr			the pointer in question
	Return:	its owner
------------------------------------------------------------------------------*/

ObjectId
GetPtrOwner(Ptr inPtr)
{
	if (inPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		ObjectId	owner = block->inuse.owner;
		if ((owner & 0x80000000) == 0)
			return owner;
	}
	return 0;
}


/*------------------------------------------------------------------------------
	Set a pointer’s owner.
	Args:		inPtr			the pointer in question
				inOwner		its owner
	Return:	--
------------------------------------------------------------------------------*/

void
SetPtrOwner(Ptr inPtr, ObjectId inOwner)
{
	if (inPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		block->inuse.owner = inOwner;
	}
}


/*------------------------------------------------------------------------------
	Get a pointer’s name.
	Args:		inPtr			the pointer in question
	Return:	its name
------------------------------------------------------------------------------*/

ULong
GetPtrName(Ptr inPtr)
{
	if (inPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		ULong name = block->inuse.name;
		if (name & 0x80000000)
			return name & ~0x80000000;
		// else name is a call chain hash
	}
	return 0;
}


/*------------------------------------------------------------------------------
	Set a pointer’s name.
	Args:		inPtr			the pointer in question
				inName		its name
	Return:	--
------------------------------------------------------------------------------*/

void
SetPtrName(Ptr inPtr, ULong inName)
{
	if (inPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		block->inuse.name = inName | 0x80000000;
	}
}


/*------------------------------------------------------------------------------
	Get a pointer’s type.
	Args:		inPtr			the pointer in question
	Return:	its type
------------------------------------------------------------------------------*/

HeapBlockType
GetPtrType(Ptr inPtr)
{
	if (inPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		return block->inuse.type;
	}
	return 0;
}


/*------------------------------------------------------------------------------
	Set a pointer’s type.
	Args:		inPtr			the pointer in question
				inType		its type
	Return:	--
------------------------------------------------------------------------------*/

void
SetPtrType(Ptr inPtr, HeapBlockType inType)
{
	if (inPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		block->inuse.type = inType;
	}
}


/*------------------------------------------------------------------------------
	Return the heap to which a pointer belongs.
	Args:		inPtr			the pointer in question
	Return:	a heap
------------------------------------------------------------------------------*/

Heap
PtrToHeap(Ptr inPtr)
{
	if (inPtr != NULL)
	{
		Block *	block = (Block *)inPtr - 1;
		return block->inuse.link;
	}
	return NULL;
}


/*------------------------------------------------------------------------------
	Lock a pointer so its guts are available to interrupt code.
	Args:		inPtr			the pointer in question
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
LockPtr(Ptr inPtr)
{
	return LockHeapRange((VAddr) inPtr, (VAddr) inPtr + GetPtrSize(inPtr));
}


/*------------------------------------------------------------------------------
	Unlock a pointer.
	Args:		inPtr			the pointer in question
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
UnlockPtr(Ptr inPtr)
{
	return UnlockHeapRange((VAddr) inPtr, (VAddr) inPtr + GetPtrSize(inPtr));
}
