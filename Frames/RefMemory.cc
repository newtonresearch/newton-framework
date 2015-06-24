/*
	File:		RefMemory.cc

	Contains:	Ref Memory interface.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "RefMemory.h"
#include "Frames.h"
#include "Arrays.h"
#include "LargeBinaries.h"
#include "ROMResources.h"
#include "ROMData.h"

extern ObjHeader * ObjectPtr1(Ref inObj, long inTag, bool inDoFaultCheck);


/*------------------------------------------------------------------------------
	Newton stores magic pointers in a sized C array.

struct MagicPointerTable
{
	ULong	numOfPointers;
	Ref	magicPointer[872];
};

const MagicPointerTable magicPointers = {872};		// 01D80000

	We generate magic pointers as a NS array. so the code is slightly different.
------------------------------------------------------------------------------*/


#define kMPTableShift	12
#define kMPTableMask		0x3FFFF000
#define kMPIndexMask		0x00000FFF

//	gRExImportTables;		// 0C107BC4

/*------------------------------------------------------------------------------
	Return an object pointer given a magic pointer Ref.
	Args:		r				Ref of the object
	Return:	ObjHeader *	pointer to the object
	Throw:	exFrames		if Ref is not a magic pointer
------------------------------------------------------------------------------*/

ObjHeader *
ResolveMagicPtr(Ref r)
{
	// RVALUE but unsigned, then extract table and index
	ULong rValue = r >> kRefTagBits;
	ArrayIndex table = rValue >> kMPTableShift;
	ArrayIndex index = rValue & kMPIndexMask;

	if (table == 0)		// table 0 is yer actual magic pointers
	{
#if defined(correct)
		if (index < magicPointers.numOfPointers)
		{
			return ObjectPtr(magicPointers.magicPointer[index]);
		}
#else
		ArrayObject *  mp = (ArrayObject *)PTR(gConstNSData->magicPointers);
		if (index < ARRAYLENGTH(mp))
		{
			return ObjectPtr(mp->slot[index]);
		}
#endif
	}
	else if (table == 1)	// table 1 is globals - vars & functions
	{
		if (index == 1)
		{
			return ObjectPtr(gVarFrame);
		}
		else if (index == 2)
		{
			return PTR(gFunctionFrame);
		}
	}
	else if (table < 10)	// remaining tables are unit import/export
	{
		/*
		if (ODD(table))
		{
			r3 = gRExImportTables[(table>>1) - 1];	// has 8 x longs
			if (index < r3)
			{
				r0 = gRExImportTables[(table>>1) - 1].x00;	// element size 8
				return PTR(r0[index]);		// element size 4
			}
		}
		else
		{
			r3 = (table>>1) - 1;
			...
			return PTR();
		}
		*/
	}		

	ThrowExFramesWithBadValue(kNSErrBadMagicPointer, r);
	return NULL;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Return a pointer to an object for a Ref.
	Args:		r				Ref whose pointer is required
	Return:	ObjHeader *	the object pointer
------------------------------------------------------------------------------*/

ObjHeader *
ObjectPtr(Ref r)
{
	long tag = RTAG(r);
	if (tag == kTagPointer)
	{
		if (ISRO(r))
			return PTR(r);
		if (r == gCached.ref)
			return gCached.ptr;
	}
	else if (tag == kTagMagicPtr)
	{
		return ResolveMagicPtr(r);
	}
	return ObjectPtr1(r, tag, NO);
}


ObjHeader *
FaultCheckObjectPtr(Ref r)
{
	long tag = RTAG(r);
	if (tag == kTagPointer)
	{
		if (ISRO(r))
			return PTR(r);
		if (r == gCached.ref)
			return gCached.ptr;
	}
	else if (tag == kTagMagicPtr)
	{
		return ResolveMagicPtr(r);
	}
	return ObjectPtr1(r, tag, YES);	// only difference from ObjectPtr()
}


/*------------------------------------------------------------------------------
	Return a pointer to an object for a Ref.
	This differs subtly from plain ObjectPtr, but not sure why.
	Args:		r				Ref whose pointer is required
	Return:	ObjHeader *	the object pointer
------------------------------------------------------------------------------*/

ObjHeader *
NoFaultObjectPtr(Ref r)
{
	long tag = RTAG(r);
	if (tag == kTagPointer)
	{
		return PTR(ForwardReference(r));
	}
	else if (tag == kTagMagicPtr)
	{
		return ResolveMagicPtr(r);
	}
	return ObjectPtr1(r, tag, NO);
}


/*------------------------------------------------------------------------------
	If the given Ref is forwarding, return the actual Ref.
	Args:		r			Ref thatÕs potentially forwarding
	Return:	Ref		of actual object
------------------------------------------------------------------------------*/

static Ref ForwardReference1(ForwardingObject * foPtr);

Ref
ForwardReference(Ref r)
{
	if (ISREALPTR(r))
	{
		ForwardingObject * foPtr = (ForwardingObject *)PTR(r);
		if (foPtr->flags & kObjForward)
			return ForwardReference1(foPtr);
	}
	return r;
}


/*------------------------------------------------------------------------------
	Return the actual Ref of a forwarding object.
	Args:		fo			object thatÕs forwarding
	Return:	Ref		of actual object
------------------------------------------------------------------------------*/

static Ref
ForwardReference1(ForwardingObject * foPtr)
{
	Ref ref, fr = INVALIDPTRREF;
	ForwardingObject * fo = foPtr;

	do {
		ref = fo->obj;
		if (ISREALPTR(ref))
		{
			fo = (ForwardingObject *)PTR(ref);
		}
		else
		{
			fr = ref;
			fo = (ForwardingObject *)NoFaultObjectPtr(ref);
		}
	} while (fo->flags & kObjForward);
	ref = MAKEPTR(fo);
	if (fr == INVALIDPTRREF)
		fr = ref;
	foPtr->obj = fr;
	return ref;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Return the flags byte of an object.
	Args:		r				Ref of the object
	Return:	unsigned		the flags
------------------------------------------------------------------------------*/

unsigned
ObjectFlags(Ref r)
{
	return ObjectPtr(r)->flags;
}


/*------------------------------------------------------------------------------
	Mark an object as dirty.
	Args:		t		Ref to be made dirty
	Return:	void
------------------------------------------------------------------------------*/

void
DirtyObject(Ref r)
{
	ObjHeader * oPtr = ObjectPtr(r);

	if ((oPtr->flags & kObjReadOnly) == 0)
		oPtr->flags |= kObjDirty;
}


/*------------------------------------------------------------------------------
	Mark an object as clean.
	Args:		t		Ref to be made clean
	Return:	void
------------------------------------------------------------------------------*/

void
UndirtyObject(Ref r)
{
	ObjHeader * oPtr = ObjectPtr(r);

	if ((oPtr->flags & kObjReadOnly) == 0)
		oPtr->flags &= ~kObjDirty;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Lock a ref.
	Args:		r		Ref to be locked
	Return:	void
------------------------------------------------------------------------------*/

void
LockRef(Ref r)
{
	ObjHeader * oPtr = ObjectPtr(r);

	if (oPtr->flags & kObjReadOnly)
		return;
	if (oPtr->gc.count.locks == 0xFF)
		return;
	if (oPtr->gc.count.locks++ == 0)
		oPtr->flags |= kObjLocked;
}


/*------------------------------------------------------------------------------
	Unlock a ref.
	Args:		r		Ref to be unlocked
	Return:	void
------------------------------------------------------------------------------*/

void
UnlockRef(Ref r)
{
	ObjHeader * oPtr = ObjectPtr(r);

	if (oPtr->flags & kObjReadOnly)
		return;
	if (oPtr->gc.count.locks == 0xFF)
		return;
	if (--oPtr->gc.count.locks == 0)
		oPtr->flags &= ~kObjLocked;
}


/*------------------------------------------------------------------------------
	Return a pointer to the data of a binary object.
	Args:		r			Ref of the object
	Return:	char *	pointer to the data
------------------------------------------------------------------------------*/

Ptr
BinaryData(Ref r)
{
	BinaryObject * oPtr = (BinaryObject *)ObjectPtr(r);
	if (ISLARGEBINARY(oPtr))
	{
		IndirectBinaryObject * vbo = (IndirectBinaryObject *)oPtr;
		return vbo->procs->GetData(vbo->data);
	}
	return oPtr->data;
}


/*------------------------------------------------------------------------------
	Return a pointer to the slots in a frame or array object.
	Args:		r			Ref of the object
	Return:	Ref *		pointer to the slots
	Throw:	BadType	if Ref is not an array
------------------------------------------------------------------------------*/

Ref *
Slots(Ref r)
{
	ArrayObject * oPtr = (ArrayObject *)ObjectPtr(r);
	if (!(oPtr->flags & kObjSlotted))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, r);

	return oPtr->slot;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Return a pointer to the data of a binary object, locking the Ref first.
	Args:		r			Ref of the object
	Return:	char *	pointer to the data
------------------------------------------------------------------------------*/

Ptr
LockedBinaryPtr(RefArg obj)
{
	LockRef(obj);
	return BinaryData(obj);
}


/*------------------------------------------------------------------------------
	Unlock a RefArg (previously locked by LockedBinaryPtr).
	Args:		r		RefArg to be unlocked
	Return:	void
------------------------------------------------------------------------------*/

void
UnlockRefArg(RefArg obj)
{
	UnlockRef(obj);
}

#pragma mark -

/*------------------------------------------------------------------------------
	B i n a r y   O b j e c t   E q u a l i t y   T e s t
------------------------------------------------------------------------------*/

Ref
BinEqual(RefArg a, RefArg b)
{
	if (!IsBinary(a))
		ThrowBadTypeWithFrameData(kNSErrNotABinaryObject, a);
	if (!IsBinary(b))
		ThrowBadTypeWithFrameData(kNSErrNotABinaryObject, b);
	ArrayIndex  len;
	return (len = Length(a)) == Length(b)
		 && memcmp(BinaryData(a), BinaryData(b), len) == 0;
}

extern "C"
Ref
FBinEqual(RefArg inRcvr, RefArg a, RefArg b)
{
	return MAKEBOOLEAN(BinEqual(a, b));
}

#pragma mark -

/*------------------------------------------------------------------------------
	O b j e c t   E q u a l i t y   T e s t
------------------------------------------------------------------------------*/

static bool
EQ1(Ref a, Ref b)
{
	ObjHeader * obj1, * obj2;

	if (ISREALPTR(a))
	{
		obj1 = PTR(a);
		if (obj1->flags & kObjForward)
			obj1 = PTR(ForwardReference(a));
	}
	else
		obj1 = NoFaultObjectPtr(a);

	if (ISREALPTR(b))
	{
		obj2 = PTR(b);
		if (obj2->flags & kObjForward)
			obj2 = PTR(ForwardReference(b));
	}
	else
		obj2 = NoFaultObjectPtr(b);

	if (obj1 == obj2)
		return YES;
/*
	if (a < x0071FC4C && b < x0071FC4C)		// x0071FC4C is "RExBlock"
		return NO;
*/
//	There are some loose symbols in the ROM - they arenÕt in the symbol table
//	so we have to make a manual check for them.
	return (((SymbolObject *)obj1)->objClass == kSymbolClass && ((SymbolObject *)obj2)->objClass == kSymbolClass
	    &&  ((SymbolObject *)obj1)->hash == ((SymbolObject *)obj2)->hash
	    &&  symcmp(((SymbolObject *)obj1)->name, ((SymbolObject *)obj2)->name) == 0);
}


bool
EQRef(Ref a, Ref b)
{
	return (a == b) ? YES : ((a & b & kTagPointer) ? EQ1(a,b) : NO);
}

#pragma mark -

/*------------------------------------------------------------------------------
	L i s t  E q u a l i t y   T e s t
------------------------------------------------------------------------------*/

Ptr
SetupListEQ(Ref obj)
{
	ObjHeader * objPtr = NULL;
	if (ISPTR(obj))
	{
		if (ISREALPTR(obj))
		{
			objPtr = PTR(obj);
			if (objPtr->flags & kObjForward)
				return (Ptr) PTR(ForwardReference(obj));
		}
		else
			return (Ptr) NoFaultObjectPtr(obj);
	}
	return (Ptr) objPtr;
}


static bool
ListEQ1(Ref a, Ref b, Ptr bPtr)	// very similar to EQ1
{
	ObjHeader * aPtr = PTR(a);
	if (ISREALPTR(a))
	{
		if (aPtr->flags & kObjForward)
			aPtr = PTR(ForwardReference(a));
	}
	else
		aPtr = NoFaultObjectPtr(a);
	if (aPtr == (ObjHeader *)bPtr)
		return YES;
/*
	if (a < x0071FC4C && b < x0071FC4C)		// x0071FC4C is "RExBlock"
		return NO;
*/
	return (((SymbolObject *)aPtr)->objClass == kSymbolClass && ((SymbolObject *)bPtr)->objClass == kSymbolClass
	    &&  ((SymbolObject *)aPtr)->hash == ((SymbolObject *)bPtr)->hash
	    &&  symcmp(((SymbolObject *)aPtr)->name, ((SymbolObject *)bPtr)->name) == 0);
}


bool
ListEQ(Ref a, Ref b, Ptr bPtr)
{
	return (a == b) ? YES : ((a & b & kTagPointer) ? ListEQ1(a,b,bPtr) : NO);
}
