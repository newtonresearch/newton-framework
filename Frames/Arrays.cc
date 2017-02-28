/*
	File:		Arrays.cc

	Contains:	Newton array functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "ROMResources.h"
#include "RefMemory.h"
#include "Symbols.h"
#include "Arrays.h"
#include "Frames.h"
#include "Funcs.h"
#include "RichStrings.h"
#include "NewtonScript.h"
#include "ItemTester.h"


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FNewWeakArray(RefArg inRcvr, RefArg inLength);
Ref	FArray(RefArg inRcvr, RefArg inLength, RefArg inValue);
Ref	FArrayInsert(RefArg inRcvr, RefArg ioArray, RefArg inObj, RefArg index);
Ref	FArrayMunger(RefArg inRcvr, RefArg a1, RefArg a1start, RefArg a1count,
											 RefArg a2, RefArg a2start, RefArg a2count);
Ref	FAddArraySlot(RefArg inRcvr, RefArg ioArray, RefArg inObj);
Ref	FArrayRemoveCount(RefArg inRcvr, RefArg ioArray, RefArg inStart, RefArg inCount);
Ref	FSetLength(RefArg inRcvr, RefArg ioArray, RefArg inLength);

Ref	FSetAdd(RefArg inRcvr, RefArg ioArray, RefArg inMember, RefArg inUnique);
Ref	FSetRemove(RefArg inRcvr, RefArg ioArray, RefArg inMember);
Ref	FSetContains(RefArg inRcvr, RefArg inArray, RefArg inMember);
Ref	FSetOverlaps(RefArg inRcvr, RefArg inArray, RefArg inArray2);
Ref	FSetUnion(RefArg inRcvr, RefArg ioArray, RefArg inArray2, RefArg inUnique);
Ref	FSetDifference(RefArg inRcvr, RefArg ioArray, RefArg inArray2);

Ref	FTableLookup(RefArg inRcvr, RefArg inTable, RefArg inKey);
}

/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/
void	ArrayAppendInFrame(RefArg frame, RefArg slot, RefArg element);

#if 0
static void ArrayGrowAt(RefArg array, ArrayIndex start, ArrayIndex growCount);
static void ArrayInsertAt(RefArg array, ArrayIndex index, RefArg element);
static bool ArrayRemoveInFrame(RefArg frame, RefArg slot, RefArg element);
#endif
static Ref  GetArraySlotError(Ref obj, ArrayIndex index, ObjHeader * p);
static void SetArraySlotError(Ref obj, ArrayIndex index, ObjHeader * p);


/*------------------------------------------------------------------------------
	Make a new array object.
	In:		length	number of (nil) elements in array
	Return:	Ref		the array object
------------------------------------------------------------------------------*/

Ref
MakeArray(ArrayIndex length)
{
	return AllocateArray(SYMA(array), length);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Add a slot to an array.
	In:		obj		the array
				value		the slot to be added
	Return:	void		(the array object is modified)
------------------------------------------------------------------------------*/

void
AddArraySlot(RefArg ioArray, RefArg inValue)
{
	ArrayIndex	aLen;

	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);
	if (ObjectFlags(ioArray) & kObjReadOnly)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, ioArray);

	aLen = Length(ioArray);
	UnsafeSetArrayLength(ioArray, aLen + 1);
	SetArraySlot(ioArray, aLen, inValue);
}


Ref
ArrayInsert(RefArg ioArray, RefArg inObj, ArrayIndex index)
{
	ArrayIndex  aLen;

	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);
	if (index > (aLen = Length(ioArray)))
		ThrowExFramesWithBadValue(kNSErrOutOfBounds, MAKEINT(index));

	SetLength(ioArray, aLen + 1);
	Ref * p = Slots(ioArray);
	memmove(p+1, p, (aLen - index) * sizeof(Ref));
	p[index] = inObj;
	return inObj;
}


/*------------------------------------------------------------------------------
	Munge two arrays.
	In:		a1			the destination array
				a1start	the first slot to be munged
				a1count	the number of slots to be munged
				a2			the source array
				a2start	the first slot to be munged
				a2count	the number of slots to be munged
	Return:	void		(the destination array object is modified)
------------------------------------------------------------------------------*/

void
ArrayMunger(RefArg a1, ArrayIndex a1start, ArrayIndex a1count,
				RefArg a2, ArrayIndex a2start, ArrayIndex a2count)
{
	ArrayIndex	a1len,  a2len;
	ArrayIndex	a1tail, a2tail;
	int	delta;
	Ref *	a1data;
	Ref *	a2data;

	if ((ObjectFlags(a1) & kObjMask) != kArrayObject)
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, a1);
	if (NOTNIL(a2) && (ObjectFlags(a2) & kObjMask) != kArrayObject)
		ThrowBadTypeWithFrameData(kNSErrNotAnArrayOrNil, a2);
	if (EQ(a1, a2))
		ThrowExFramesWithBadValue(kNSErrObjectsNotDistinct, a1);
	if (ObjectFlags(a1) & kObjReadOnly)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, a1);

	a1len = Length(a1);
	if (a1count == kIndexNotFound)
		a1count = a1len - a1start;

	if (ISNIL(a2))
		a2start = a2count = a2len = 0;
	else
		a2len = Length(a2);
	if (a2count == kIndexNotFound)
		a2count = a2len - a2start;

	a1start = MINMAX(0, a1start, a1len);
	a1tail = a1len - a1start;
	a1count = MINMAX(0, a1count, a1tail);
	a2start = MINMAX(0, a2start, a2len);
	a2tail = a2len - a2start;
	a2count = MINMAX(0, a2count, a2tail);
	delta = a2count - a1count;
	if (delta > 0)
		SetLength(a1, a1len + delta);
	a1data = Slots(a1);
	a2data = ISNIL(a2) ? NULL : Slots(a2);
	if (delta)
		memmove(a1data + a1start, a1data + a1start + a1count, (a1tail - a1count) * sizeof(Ref));
	if (a2count)
		memmove(a1data + a1start, a2data + a2start, a2count * sizeof(Ref));
	if (delta < 0)
		SetLength(a1, a1len + delta);
}


/*------------------------------------------------------------------------------
	Find the index of a slot in an array.
	In:		array		the array
				item		the slot to be found
				start		the starting index for the search
				test		a symbol or function object
	Return:	long		the index of the item
							-1 => the item wasn't found
------------------------------------------------------------------------------*/

ArrayIndex
ArrayPosition(RefArg array, RefArg item, ArrayIndex start, RefArg test)
{
	bool	noTest = ISNIL(test);
	ArrayIndex	count = Length(array);
	if (count != 0)
	{
		start = MINMAX(0, start, count);
		if (noTest)
		{
			for ( ; start < count; start++)
			{
				if (EQRef(item, GetArraySlot(array, start)))
					break;
			}
		}
		else
		{
			RefVar args(MakeArray(2));
			RefVar arg1(NILREF);
			SetArraySlot(args, 0, item);
			for ( ; start < count; start++)
			{
				arg1 = GetArraySlot(array, start);
				SetArraySlot(args, 1, arg1);
				if (DoBlock(test, args))
					break;
			}
		}
		if (start != count)
			return start;
	}
	return kIndexNotFound;
}

#if 0
static void
ArrayGrowAt(RefArg array, ArrayIndex start, ArrayIndex growCount)
{
	ArrayIndex i, count = Length(array);

	if (start > count)
		start = count;
	SetLength(array, count + growCount);
	for (i = count - 1; i >= start; i--)
	{
		RefVar ref(GetArraySlot(array, i));
		SetArraySlot(array, i + growCount, ref);
	}
}


static void
ArrayInsertAt(RefArg array, ArrayIndex index, RefArg element)
{
	ArrayGrowAt(array, index, 1);
	SetArraySlot(array, index, element);
}
#endif

/*------------------------------------------------------------------------------
	Append a slot to an array (being a slot in a frame).
	In:		frame		a frame containingÉ
				tag		Éthis slot
				element	the slot to be appended
	Return:	void		(the destination array object is modified)
------------------------------------------------------------------------------*/

void
ArrayAppendInFrame(RefArg frame, RefArg tag, RefArg element)
{
	RefVar array(GetFrameSlot(frame, tag));
	if (ISNIL(array))
	{
		array = MakeArray(1);
		SetArraySlot(array, 0, element);
		SetFrameSlot(frame, tag, array);
	}
	else
		AddArraySlot(array, element);
}


/*------------------------------------------------------------------------------
	Remove a slot from an array (being a slot in a frame).
	In:		array		the array
				element	the slot to be removed
	Return:	bool		true => slot was removed
							false  => slot wasn't found
------------------------------------------------------------------------------*/

bool
ArrayRemoveInFrame(RefArg frame, RefArg tag, RefArg element)
{
	bool	 result = false;
	RefVar array(GetFrameSlot(frame, tag));
	if (NOTNIL(array) && ArrayRemove(frame, element))
	{
		if (Length(array) == 0)
			SetFrameSlot(frame, tag, RA(NILREF));
		result = true;
	}
	return result;
}


/*------------------------------------------------------------------------------
	Remove a slot from an array.
	In:		array		the array
				element	the slot to be removed
	Return:	bool		true => slot was removed
							false  => slot wasn't found
------------------------------------------------------------------------------*/

bool
ArrayRemove(RefArg array, RefArg element)
{
	if (!IsArray(array))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, array);

	bool	result = false;
	for (ArrayIndex i = 0, count = Length(array); i < count; ++i)
	{
		if (EQRef(element, GetArraySlot(array, i)))
		{
			ArrayRemoveCount(array, i, 1);
			result = true;
			break;
		}
	}
	return result;
}


/*------------------------------------------------------------------------------
	Remove slots from an array.
	In:		array		the array
				start		index of first slot to be removed
				removeCount		number of slots to be removed
	Return:	void		(the array object is modified)
------------------------------------------------------------------------------*/

void
ArrayRemoveCount(RefArg array, ArrayIndex start, ArrayIndex removeCount)
{
	ArrayMunger(array, start, removeCount, RA(NILREF), 0, 0);
}


/*------------------------------------------------------------------------------
	Remove and return the last element of an array.
	In:		array		the array
	Return:	Ref		the element
------------------------------------------------------------------------------*/

Ref
ArrayPop(RefArg array)
{
	ArrayIndex last = Length(array) - 1;
	RefVar ref(GetArraySlot(array, last));
	SetLength(array, last);
	return ref;
}


/*------------------------------------------------------------------------------
	Test whether there are any elements in an array.
	In:		array		the array
	Return:	bool		array state
------------------------------------------------------------------------------*/

bool
ArrayIsEmpty(RefArg array)
{
	return Length(array) == 0;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Throw an array index out of bounds exception.
	In:		obj		the array
				index		index of the slot accessed
				p			pointer to obj (for convenience)
	Return:	Ref		nil
------------------------------------------------------------------------------*/

static Ref
GetArraySlotError(Ref obj, ArrayIndex index, ObjHeader * p)
{
	if (!ISSLOTTED(p))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, obj);

	RefVar	fr(AllocateFrame());
	SetFrameSlot(fr, SYMA(errorCode), MAKEINT(kNSErrOutOfBounds));
	SetFrameSlot(fr, SYMA(value), MAKEPTR(p));
	SetFrameSlot(fr, SYMA(index), MAKEINT(index));
	ThrowRefException(exFramesData, fr);

	return NILREF;	// actually will never return
}


/*------------------------------------------------------------------------------
	Return a slot from an array.
	In:		obj		the array
				slot		index of the slot to be returned
	Return:	Ref		the slot
------------------------------------------------------------------------------*/

Ref
GetArraySlotRef(Ref obj, ArrayIndex index)
{
	ArrayObject * p = (ArrayObject *)ObjectPtr(obj);
	if (ISSLOTTED(p)
	&& index < ARRAYLENGTH(p))
	{
		return p->slot[index];
	}
	else
		return GetArraySlotError(obj, index, (ObjHeader *)p);
}

Ref
GetArraySlot(RefArg obj, ArrayIndex index)
{
	return GetArraySlotRef(obj, index);
}



/*------------------------------------------------------------------------------
	Set a slot in an array.
	In:		obj		the array
				index		index of the slot to be set
				value		value of the slot to be set
	Return:	void		(the array object is modified)
------------------------------------------------------------------------------*/

static void
SetArraySlotError(Ref obj, ArrayIndex index, ObjHeader * p)
{
	if (!ISSLOTTED(p))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, obj);

	else if (index >= ARRAYLENGTH(p))
	{
		RefVar	fr(AllocateFrame());
		SetFrameSlot(fr, SYMA(errorCode), MAKEINT(kNSErrOutOfBounds));
		SetFrameSlot(fr, SYMA(value), MAKEPTR(p));
		SetFrameSlot(fr, SYMA(index), MAKEINT(index));
		ThrowRefException(exFramesData, fr);
	}

	else
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, obj);
}


void
SetArraySlotRef(Ref obj, ArrayIndex index, Ref value)
{
	ArrayObject * p = (ArrayObject *)ObjectPtr(obj);
	if ((p->flags & (kObjSlotted | kObjReadOnly)) == kObjSlotted
	&& index < ARRAYLENGTH(p))
	{
		p->slot[index] = value;
		DirtyObject(obj);
	}
	else
		SetArraySlotError(obj, index, (ObjHeader *)p);
}

void
SetArraySlot(RefArg obj, ArrayIndex index, RefArg value)
{
	SetArraySlotRef(obj, index, value);
}


#pragma mark -
/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FAddArraySlot(RefArg inRcvr, RefArg ioArray, RefArg inObj)
{
	AddArraySlot(ioArray, inObj);
	return inObj;
}


Ref
FNewWeakArray(RefArg inRcvr, RefArg inLength)
{
	return AllocateArray(kWeakArrayClass, RINT(inLength));
}


Ref
FArray(RefArg inRcvr, RefArg inLength, RefArg inValue)
{
	ArrayIndex	aLen = RINT(inLength);
	RefVar		a(MakeArray(aLen));
	if (NOTNIL(inValue))
	{
		Ref * p = Slots(a);
		for (ArrayIndex i = 0; i < aLen; ++i)
			p[i] = inValue;
	}
	return a;
}


Ref
FArrayInsert(RefArg inRcvr, RefArg ioArray, RefArg inObj, RefArg index)
{
	ArrayInsert(ioArray, inObj, RINT(index));
	return ioArray;
}


Ref
FArrayMunger(RefArg inRcvr, RefArg a1, RefArg a1start, RefArg a1count,
									 RefArg a2, RefArg a2start, RefArg a2count)
{
	ArrayMunger(a1, RINT(a1start), NOTNIL(a1count) ? RINT(a1count) : kIndexNotFound,
					a2, RINT(a2start), NOTNIL(a2count) ? RINT(a2count) : kIndexNotFound);
	return a1;
}


Ref
FArrayRemoveCount(RefArg inRcvr, RefArg ioArray, RefArg inStart, RefArg inCount)
{
	ArrayRemoveCount(ioArray, RINT(inStart), RINT(inCount));
	return NILREF;
}


Ref
FSetLength(RefArg inRcvr, RefArg ioArray, RefArg inLength)
{
	SetLength(ioArray, RINT(inLength));
	return ioArray;
}

#pragma mark -

/*------------------------------------------------------------------------------
	S e t   F u n c t i o n s
------------------------------------------------------------------------------*/

Ref
FSetAdd(RefArg inRcvr, RefArg ioArray, RefArg inMember, RefArg inUnique)
{
	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);

	if (ISTRUE(inUnique)
	&&  NOTNIL(FSetContains(inRcvr, ioArray, inMember)))
		return NILREF;

	AddArraySlot(ioArray, inMember);
	return ioArray;
}


Ref
FSetRemove(RefArg inRcvr, RefArg ioArray, RefArg inMember)
{
	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);

	if (ISNIL(ArrayRemove(ioArray, inMember)))
		return NILREF;
	return ioArray;
}


Ref
FSetContains(RefArg inRcvr, RefArg inArray, RefArg inMember)
{
	if (!IsArray(inArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray);

	for (ArrayIndex i = 0, count = Length(inArray); i < count; ++i)
	{
		if (EQRef(inMember, GetArraySlot(inArray, i)))
			return MAKEINT(i);
	}
	return NILREF;
}


Ref
FSetOverlaps(RefArg inRcvr, RefArg inArray, RefArg inArray2)
{
	if (!IsArray(inArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray);
	if (!IsArray(inArray2))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray2);

	ArrayIndex i, count = Length(inArray);
	ArrayIndex j, count2 = Length(inArray2);
	for (i = 0; i < count; ++i)
	{
		for (j = 0; j < count2; ++j)
		{
			if (EQRef(GetArraySlot(inArray2, j), GetArraySlot(inArray, i)))
				return MAKEINT(i);
		}
	}
	return NILREF;
}


Ref
FSetUnion(RefArg inRcvr, RefArg ioArray, RefArg inArray2, RefArg inUnique)
{
	if (ISNIL(ioArray))
	{
		if (ISNIL(inArray2))
			return MakeArray(0);
		if (!IsArray(inArray2))
			ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray2);
		return inArray2;
	}

	if (ISNIL(inArray2))
	{
		if (!IsArray(ioArray))
			ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);
		return Clone(ioArray);
	}

	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);
	if (!IsArray(inArray2))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray2);

	RefVar setUnion;
	ArrayIndex i, count = Length(ioArray);
	ArrayIndex j, count2 = Length(inArray2);
	if (ISTRUE(inUnique))
	{
		setUnion = MakeArray(0);
		for (i = 0; i < count; ++i)
			FSetAdd(inRcvr, setUnion, GetArraySlot(ioArray, i), RA(TRUEREF));
		for (j = 0; j < count2; ++j)
			FSetAdd(inRcvr, setUnion, GetArraySlot(inArray2, j), RA(TRUEREF));
	}
	else
	{
		setUnion = MakeArray(count + count2);
		for (i = 0; i < count; ++i)
			SetArraySlot(setUnion, i, GetArraySlot(ioArray, i));
		for (j = 0; j < count2; ++j, ++i)
			SetArraySlot(setUnion, i, GetArraySlot(inArray2, j));
	}

	return setUnion;
}


Ref
FSetDifference(RefArg inRcvr, RefArg ioArray, RefArg inArray2)
{
	if (ISNIL(ioArray))		// allow nil
		return NILREF;
	if (!IsArray(ioArray))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, ioArray);

	RefVar setDelta(Clone(ioArray));

	if (ISNIL(inArray2))		// allow nil
		return setDelta;
	if (!IsArray(inArray2))
		ThrowBadTypeWithFrameData(kNSErrNotAnArray, inArray2);

	for (ArrayIndex i = 0, count = Length(inArray2); i < count; ++i)
		ArrayRemove(setDelta, GetArraySlot(inArray2, i));

	return setDelta;
}

#pragma mark -

Ref
TableLookup(RefArg inTable, RefArg inKey)
{
	RefVar value;
	ArrayIndex count = Length(inTable);
	for (ArrayIndex i = 0; i < count-2; i += 2)
	{
		if (EQ(GetArraySlot(inTable, i), inKey))
		{
			value = GetArraySlot(inTable, i+1);
			break;
		}
	}
	if (ISNIL(value))
		value = GetArraySlot(inTable, count - 1);
	return value;
}


Ref
FTableLookup(RefArg inRcvr, RefArg inTable, RefArg inKey)
{
	return TableLookup(inTable, inKey);
}

