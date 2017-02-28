/*
	File:		Iterators.cc

	Contains:	Array and frame slot iterators.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Iterators.h"
#include "ROMResources.h"
#include "Funcs.h"
#include "Frames.h"
#include "Arrays.h"

extern "C" bool	OnStack(void *);


/*----------------------------------------------------------------------
	Map a function against all slots in a frame.
	Args:		context	a frame
				func		the mapping function
				anything	argument for mapping function
	Return:	void		(the object is modified)
----------------------------------------------------------------------*/

void
MapSlots(RefArg context, MapSlotsFunction func, unsigned anything)
{
	if (!(ObjectFlags(context) & kObjSlotted))
		return;

	RefVar	tag, value;
	CObjectIterator	iter(context);
	for ( ; !iter.done(); iter.next())
	{
		tag = iter.tag();
		value = iter.value();
		func(tag, value, anything);
	}
}


extern "C"
Ref
FMap(RefArg inRcvr, RefArg inObj, RefArg inFn)
{
	RefVar	args(MakeArray(2));
	CObjectIterator	iter(inObj);
	for ( ; !iter.done(); iter.next())
	{
		SetArraySlot(args, 0, iter.tag());
		SetArraySlot(args, 1, iter.value());
		DoBlock(inFn, args);
	}
	return NILREF;
}


/*----------------------------------------------------------------------
	O b j e c t I t e r a t o r
----------------------------------------------------------------------*/

void
DisposeObjectIterator(void * iter)
{
	((CObjectIterator *)iter)->~CObjectIterator();
}

#pragma mark -

CObjectIterator::CObjectIterator(RefArg inObj, bool includeSiblings)
{
	if (OnStack(this))	// !!
	{
		x.header.catchType = kExceptionCleanup;
		x.function = DisposeObjectIterator;
		x.object = this;
		AddExceptionHandler(&x.header);
	}
	else
		x.function = NULL;

	if (!(ISPTR(inObj) && (ObjectFlags(inObj) & kObjSlotted)))
		ThrowBadTypeWithFrameData(kNSErrNotAFrameOrArray, inObj);
	
	fObj = inObj;
	fIncludeSiblings = includeSiblings;
	if (ObjectFlags(inObj) & kObjFrame)
		fMapRef = ((FrameObject *)ObjectPtr(inObj))->map;
	fLength = Length(inObj);
	fIndex = -1;
	next();
}


CObjectIterator::~CObjectIterator()
{
	if (x.function != NULL)
		RemoveExceptionHandler(&x.header);
}


void
CObjectIterator::reset(void)
{
	fIndex = -1;
	next();
}


void
CObjectIterator::resetWithObject(RefArg inObj)
{
	fObj = inObj;
	if (ObjectFlags(inObj) & kObjFrame)
		fMapRef = ((FrameObject *)ObjectPtr(inObj))->map;
	else
		fMapRef = NILREF;
	fLength = Length(inObj);
	fIndex = -1;
	next();
}


bool
CObjectIterator::next(void)
{
	ArrayIndex len = Length(fObj);
	if (fLength <= len)
		fIndex++;
	fLength = len;
	if (fIndex < (int)fLength)
	{
		fTag = ISNIL(fMapRef)	? MAKEINT(fIndex)
										: GetTag(fMapRef, fIndex);
		fValue = GetArraySlot(fObj, fIndex);
		return true;
	}
	else
	{
		if (fIncludeSiblings && NOTNIL(fMapRef))
		{
			RefVar proto(GetFrameSlot(fObj, SYMA(_proto)));
			if (NOTNIL(proto))
			{
				resetWithObject(proto);
				return !done();
			}
		}
		fValue = fTag = NILREF;
		return false;
	}
}


bool
CObjectIterator::done(void)
{
	fLength = Length(fObj);
	if (fIncludeSiblings && NOTNIL(fMapRef))
	{
		if (fIndex < (int)fLength)
			return false;
		return ISNIL(GetFrameSlot(fObj, SYMA(_proto)));
	}
	return fIndex >= (int)fLength;
}

#pragma mark -

/*----------------------------------------------------------------------
	P r e c e d e n t s
----------------------------------------------------------------------*/

#define kPrecedentBlockSize	16


CPrecedents::CPrecedents()
{
	fPrecArray = MakeArray(kPrecedentBlockSize);
	fNumEntries = 0;
}


CPrecedents::~CPrecedents()
{ }


ArrayIndex
CPrecedents::add(RefArg inObj)
{
	ArrayIndex	precedentArrayLen = Length(fPrecArray);
	if (fNumEntries == precedentArrayLen)
		SetLength(fPrecArray, precedentArrayLen + kPrecedentBlockSize);
	SetArraySlot(fPrecArray, fNumEntries, inObj);
	return fNumEntries++;
}


ArrayIndex
CPrecedents::find(RefArg inObj)
{
	Ref *	p = ((ArrayObject *)ObjectPtr(fPrecArray))->slot;
	Ptr	objPtr = SetupListEQ(inObj);
	for (ArrayIndex index = 0; index < fNumEntries; index++)
	{
		if (ListEQ(p[index], inObj, objPtr))
			return index;
	}
	return kIndexNotFound;
}


Ref
CPrecedents::get(ArrayIndex index)
{
	return GetArraySlot(fPrecArray, index);
}


void
CPrecedents::set(ArrayIndex index, RefArg inObj)
{
	SetArraySlot(fPrecArray, index, inObj);
}

