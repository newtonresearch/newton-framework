/*
	File:		FakePointers.cc

	Contains:	Pointer allocation based on malloc.

	Written by:	Newton Research Group, 2008.
*/

#include "Newton.h"


/*------------------------------------------------------------------------------
	E r r o r s
------------------------------------------------------------------------------*/
#include "TaskGlobals.h"

inline void
SetMemError(NewtonErr inErr)
{
	TaskSwitchedGlobals()->fMemErr = inErr;
}

#if !usesMacMemMgr
NewtonErr
MemError(void)
{
	return TaskSwitchedGlobals()->fMemErr;
}
#endif

/*------------------------------------------------------------------------------
	P o i n t e r   O p e r a t i o n s
------------------------------------------------------------------------------*/

struct Block
{
	Size				size;		// first b/c it’s 64-bit
	union {
		ObjectId		owner;	// 32-bit
		ULong			name;
	};
	HeapBlockType	type;		// 8-bit
};

/*------------------------------------------------------------------------------
	Create a new pointer.
	Args:		inSize		size required
	Return:	a pointer
------------------------------------------------------------------------------*/

Ptr
NewPtr(Size inSize)
{
	NewtonErr	err;
	Block *	newPtr;

	newPtr = (Block *)malloc(sizeof(Block) + inSize);
	if (newPtr == NULL)
		err = memFullErr;
	else
	{
		newPtr->size = inSize;
		newPtr->name = 0;
		newPtr->type = 0;
		err = noErr;
	}
	SetMemError(err);

	return (newPtr == NULL) ? NULL : (Ptr) (newPtr + 1);
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
	if (inPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		free(block);
		SetMemError(noErr);
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
	Ptr	newPtr = NewPtr(inSize);

	if (inPtr != NULL && newPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		memmove(newPtr, inPtr, MIN(inSize, block->size));
		FreePtr(inPtr);
	}
	if (newPtr == NULL)
	{
		SetMemError(memFullErr);
	}

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
	if (inPtr != NULL)
	{
		Block *	block = ((Block *)inPtr) - 1;
		return block->size;
	}
	return 0;
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
		ObjectId	owner = block->owner;
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
		block->owner = inOwner;
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
		ULong name = block->name;
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
		block->name = inName | 0x80000000;
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
		return block->type;
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
		block->type = inType;
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
	return noErr;
}


/*------------------------------------------------------------------------------
	Unlock a pointer.
	Args:		inPtr			the pointer in question
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
UnlockPtr(Ptr inPtr)
{
	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Wired pointers.
	Behave like normal pointers. (They’re fake!)
------------------------------------------------------------------------------*/
#if 0

Ptr
NewWiredPtr(Size inSize)
{ return NewPtr(inSize); }

void
FreeWiredPtr(Ptr inPtr)
{ FreePtr(inPtr); }

#endif
