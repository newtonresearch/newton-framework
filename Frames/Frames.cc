/*
	File:		Frames.c

	Contains:	Newton frame functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "RefMemory.h"
#include "Lookup.h"
#include "Symbols.h"
#include "Arrays.h"
#include "Frames.h"

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
#if defined(hasGlobalConstantFunctions)
Ref	FDefineGlobalConstant(RefArg inRcvr, RefArg inTag, RefArg inObj);
Ref	FUnDefineGlobalConstant(RefArg inRcvr, RefArg inTag);
#endif
Ref	FDefGlobalFn(RefArg inRcvr, RefArg inTag, RefArg inObj);
Ref	FGetPath(RefArg inRcvr, RefArg inObj, RefArg inTag);
Ref	FGetSlot(RefArg inRcvr, RefArg inObj, RefArg inTag);
Ref	FHasPath(RefArg inRcvr, RefArg inObj, RefArg inPath);
Ref	FHasSlot(RefArg inRcvr, RefArg inObj, RefArg inTag);
Ref	FHasVar(RefArg inRcvr, RefArg inTag);
Ref	FRemoveSlot(RefArg inRcvr, RefArg inObj, RefArg inTag);
Ref	FIsPathExpr(RefArg inRcvr, RefArg inObj);
}

/*----------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------*/

extern "C" bool	InHeap(Ref ref);

static Ref	GlobalFunctionLookup(Ref fn);

static ArrayIndex FindOffset1(Ref context, Ref tag, Ref * outMap);

		 bool	FrameHasSlotRef(Ref context, Ref tag);
static bool SlowFrameHasSlot(Ref context, Ref tag);
		 Ref	GetFrameSlotRef(Ref context, Ref slot);
static Ref  SlowGetFrameSlot(Ref context, Ref tag);

static ArrayIndex	ConvertToSortedMap(Ref context, ArrayIndex index);
static ArrayIndex	SearchSortedMap(ArrayObject * map, ArrayIndex length, Ref tag);


/*----------------------------------------------------------------------
	F u n c t i o n s
----------------------------------------------------------------------*/
#if defined(hasGlobalConstantFunctions)
extern Ref *		RSgConstantsFrame;

Ref
FDefineGlobalConstant(RefArg inRcvr, RefArg inTag, RefArg inObj)
{
	if (FrameHasSlot(RA(gConstantsFrame), inTag))
		ThrowExFramesWithBadValue(kNSErrConstRedefined, inTag);

	SetFrameSlot(RA(gConstantsFrame), EnsureInternal(inTag), inObj);
	return inTag;
}

Ref
FUnDefineGlobalConstant(RefArg inRcvr, RefArg inTag)
{
	RemoveSlot(RA(gConstantsFrame), inTag);
	return NILREF;
}
#endif

Ref
FDefGlobalFn(RefArg inRcvr, RefArg inTag, RefArg inObj)
{
	if (!IsFunction(inObj))
		ThrowBadTypeWithFrameData(kNSErrNotAFunction, inObj);

	SetFrameSlot(gFunctionFrame, EnsureInternal(inTag), inObj);
	return inTag;
}


/*----------------------------------------------------------------------
	Look up a global function.
	Args:		fnSym		symbol of function
	Return:	Ref		function object
	Throw:	BadType	if fnSym is not a symbol
----------------------------------------------------------------------*/
extern Ref gConstFuncFrame;

static Ref
GlobalFunctionLookup(Ref fnSym)
{
	Ref	fns, func;

	if (!(ISREALPTR(fnSym) && ((SymbolObject *)PTR(fnSym))->objClass == kSymbolClass))
		ThrowBadTypeWithFrameData(kNSErrNotASymbol, fnSym);

	fns = gFunctionFrame;
	gFunctionFrame = NILREF;	// prevent infinite loop via GetFrameSlot
	if (FrameHasSlotRef(fns, fnSym))
	{
		func = GetFrameSlotRef(fns, fnSym);
		gFunctionFrame = fns;
		return func;
	}

#if defined(hasPureFunctionSupport)
	Ref cfns = gConstFuncFrame;
	if (NOTNIL(cfns) && FrameHasSlotRef(cfns, fnSym))
	{
		func = GetFrameSlotRef(cfns, fnSym);
		gFunctionFrame = fns;
		return func;
	}
#endif

	gFunctionFrame = fns;
	return GetFrameSlotRef(gConstNSData->internalFunctions, fnSym);
}

#pragma mark -

/*----------------------------------------------------------------------
	Determine whether an object could be a path expression.
	Args:		inObj		an object
	Return:	YES|NO
----------------------------------------------------------------------*/

bool
IsPathExpr(RefArg inObj)
{
	return ISINT(inObj)
	|| IsSymbol(inObj)
	|| (IsArray(inObj) && EQRef(((ArrayObject *)ObjectPtr(inObj))->objClass, RSYMpathExpr));
}


/*----------------------------------------------------------------------
	Determine whether the path exists in the given frame.
	Args:		context	a frame
				path		the path
	Return:	bool		exists
	Throw:	BadType	if path is not a symbol, int or pathExpr array
				exFrames	if bad pathExpr
----------------------------------------------------------------------*/

bool
FrameHasPath(RefArg context, RefArg path)
{
	if (NOTNIL(context))
	{
		if (IsSymbol(path))
		{
			bool	varExists = NO;
			if (context == gFunctionFrame)
				return NOTNIL(GlobalFunctionLookup(path));
			else if (IsFrame(context))
				GetProtoVariable(context, path, &varExists);
			return varExists;
		}
		else if (ISINT(path))
		{
			if (IsArray(context))
				return ARRAYLENGTH(ObjectPtr(context)) > (ArrayIndex)RVALUE(path);
		}
		else if (IsArray(path))
		{
			if (EQRef(ClassOf(path), RSYMpathExpr))
			{
				Ref	current;
				Ref	pathElement;
				ArrayIndex	pathLen = ARRAYLENGTH(ObjectPtr(path));
				if (pathLen < 1)
					ThrowErr(exFrames, kNSErrEmptyPath);

				current = context;
				for (ArrayIndex i = 0; i < pathLen && NOTNIL(current); ++i)
				{
					pathElement = ((ArrayObject *)ObjectPtr(path))->slot[i];
					if (ISINT(pathElement))
					{
						ArrayIndex pathIndex = RVALUE(pathElement);
						if (IsArray(current) && pathIndex < ARRAYLENGTH(ObjectPtr(current)))
							current = ((ArrayObject *)ObjectPtr(current))->slot[pathIndex];
						else
							return NO;
					}
					else if (IsSymbol(pathElement))
					{
						if (IsFrame(current))
						{
							if (current == gFunctionFrame)
							{
								current = GlobalFunctionLookup(pathElement);
								if (ISNIL(current))
									return NO;
							}
							else
							{
								bool varExists = NO;
								current = GetProtoVariable(context, pathElement, &varExists);
								if (!varExists)
									return NO;
							}
						}
						else
							return NO;
					}
					else
						ThrowExFramesWithBadValue(kNSErrBadSegmentInPath, path);
				}
				return YES;
			}
		}
		else
			ThrowBadTypeWithFrameData(kNSErrNotAPathExpr, path);
	}
	return NO;
}


/*----------------------------------------------------------------------
	Determine the index of the named slot within a frame.
	Args:		context	a frame
				tag		a slot symbol
	Return:	long		index
							-1 => slot doesn’t exist
	Throw:	BadType	if context is not a frame
----------------------------------------------------------------------*/

ArrayIndex
FrameSlotPosition(Ref context, Ref tag)
{
	FrameObject *	fr = (FrameObject *)ObjectPtr(context);
	if (NOTFRAME(fr))
		ThrowBadTypeWithFrameData(kNSErrNotAFrame, MAKEPTR(fr));

	return FindOffset(fr->map, tag);	// look up the name in the map
}


/*----------------------------------------------------------------------
	Determine the index of the named slot within a sorted frame map.
	Args:		map		a frame map
				length	number of slots in the map
				slot		a slot symbol
	Return:	long		index
							-1 => slot doesn’t exist
----------------------------------------------------------------------*/

static ArrayIndex
SearchSortedMap(ArrayObject * map, ArrayIndex length, Ref tag)
{
	int	first = 0;
	int	last = length - 1;
	while (first <= last)
	{
		ArrayIndex i = (first + last) / 2;
		int	cmp = SymbolCompare(map->slot[1 + i], tag);
		if (cmp == 0)
			return i;
		else if (cmp < 0)
			first = i + 1;
		else
			last = i - 1;
	}

	return kIndexNotFound;
}


/*----------------------------------------------------------------------
	Determine the offset (index) of the named slot within a frame.
	These functions cache their results to improve performance.
----------------------------------------------------------------------*/

#define kMaxDepth					16

#define kOffsetCacheShift		 6
#define kOffsetCacheSize		(1 << 5)
#define kOffsetCacheSizeMask	(kOffsetCacheSize - 1)

OffsetCacheItem offsetCache[kOffsetCacheSize];	// 0C107BF4


/*----------------------------------------------------------------------
	Clear the offset cache.
----------------------------------------------------------------------*/

void
FindOffsetCacheClear(void)
{
	for (ArrayIndex i = 0; i < kOffsetCacheSize; ++i)
		offsetCache[i].context = INVALIDPTRREF;
}


/*----------------------------------------------------------------------
	Determine the offset (index) of the named slot within a frame.
	Searches supermaps too, so returns the actual map in which the slot
	was found.
	Args:		context	a frame
				tag		a slot symbol
				outMap	pointer to map Ref in which slot actually exists
	Return:	long		index
							outMap == NILREF => slot doesn’t exist
----------------------------------------------------------------------*/

static ArrayIndex
FindOffset1(Ref context, Ref tag, Ref * outMap)
{
	ArrayObject *	stack[kMaxDepth];
	ArrayObject *	map;
	int		depth = 0;
	long		offset = 0;
	ULong		hash = ((SymbolObject *)PTR(tag))->hash;

	for (map = (ArrayObject *)ObjectPtr(context);
		  NOTNIL(map->slot[0]);
		  map = (ArrayObject *)ObjectPtr(map->slot[0]))
	{
		stack[depth++] = map;
		if (depth == kMaxDepth)
		{
			offset = FindOffset1(map->slot[0], tag, outMap);
			if (NOTNIL(*outMap))
				return offset;
			depth = kMaxDepth - 1;
			break;
		}
	}

	while (depth >= 0)
	{
		ArrayIndex	len = ARRAYLENGTH(map) - 1;

		if (map->objClass & kMapSorted)
		{
			ArrayIndex mapOffset = SearchSortedMap(map, len, tag);
			if (mapOffset == kIndexNotFound)
			{
				offset += len;
			}
			else
			{
				*outMap = MAKEPTR(map);
				return offset + mapOffset;
			}
		}
		else
		{
			Ref	slotSym;
			Ref *	slotPtr = &map->slot[1];
			while (len-- != 0)
			{
				slotSym = *slotPtr++;
				if (slotSym == tag
				||  UnsafeSymbolEqual(slotSym, tag, hash))
				{
					*outMap = MAKEPTR(map);
					return offset;
				}
				offset++;
			}
		}
		if (depth-- > 0)
			map = stack[depth];
	}

	*outMap = NILREF;
	return offset;
}


/*----------------------------------------------------------------------
	Determine the offset (index) of the named slot within a frame.
	Args:		fr			a frame
				tag		a slot symbol
	Return:	long		index
							-1 => slot doesn’t exist
	Throw:	BadType	if tag is not a symbol
----------------------------------------------------------------------*/

ArrayIndex
FindOffset(Ref context, Ref tag)
{
	SymbolObject *	sym = (SymbolObject *)PTR(tag);
	if (!(ISREALPTR(tag) && sym->objClass == kSymbolClass))
		ThrowBadTypeWithFrameData(kNSErrNotASymbol, tag);

	ArrayIndex cacheIndex = ((context + sym->hash) >> kOffsetCacheShift) & kOffsetCacheSizeMask;
	OffsetCacheItem *	cachePtr = gCached.offset;
	ArrayIndex	offset;
	if (context == cachePtr->context
	 && tag == cachePtr->slot)
	{
		offset = cachePtr->offset;
	}
	else
	{
		cachePtr = &offsetCache[cacheIndex];
		if (context == cachePtr->context
		 && tag == cachePtr->slot)
		{
			offset = cachePtr->offset;
			gCached.offset = cachePtr;
		}
		else
		{
			offset = kIndexNotFound;
		}
	}
	if (offset != kIndexNotFound)
		return offset;

	if ((tag == RSYM_proto || UnsafeSymbolEqual(tag, RSYM_proto, k_protoHash))
	&&  (((ArrayObject *)ObjectPtr(context))->objClass & kMapProto) == 0)
		;	// we’re looking for a _proto slot but there’s no proto chain
	else
	{
		Ref	implMap;
		offset = FindOffset1(context, tag, &implMap);
		if (ISNIL(implMap))
			offset = kIndexNotFound;
	}
	cachePtr = &offsetCache[cacheIndex];
	cachePtr->context = context;
	cachePtr->slot = tag;
	cachePtr->offset = offset;
	gCached.offset = cachePtr;

	return offset;
}


#pragma mark -

/*----------------------------------------------------------------------
	Determine whether the named slot exists in the given frame.
	Args:		context	a frame
				tag		a slot symbol
	Return:	bool		exists
	Throw:	BadType	if context is not a frame
----------------------------------------------------------------------*/

bool
FrameHasSlot(RefArg context, RefArg tag)
{
	return FrameHasSlotRef(context, tag);
}


bool
FrameHasSlotRef(Ref context, Ref tag)
{
	if (context == gFunctionFrame)
		return NOTNIL(GlobalFunctionLookup(tag));

	FrameObject * fr = (FrameObject *)FaultCheckObjectPtr(context);
	if (fr == NULL)
	{
		if (InHeap(tag))
			return SlowFrameHasSlot(context, tag);
		fr = (FrameObject *)ObjectPtr(context);
	}
	if (NOTFRAME(fr))
		ThrowBadTypeWithFrameData(kNSErrNotAFrame, context);

	return (FindOffset(fr->map, tag) != kIndexNotFound);
}


static bool
SlowFrameHasSlot(Ref context, Ref tag)
{
	bool	hasSlot = NO;
	
	LockRef(tag);
	unwind_protect
	{
		hasSlot = FrameHasSlotRef(MAKEPTR(ObjectPtr(context)), tag);
	}
	on_unwind
		UnlockRef(tag);
	end_unwind;

	return hasSlot;
}


/*----------------------------------------------------------------------
	Return the value of the path.
	Args:		context	a frame
				path		the path
	Return:	Ref		the slot value
	Throw:	BadType	if path is not a symbol, int or pathExpr array
				exFrames	if bad pathExpr
----------------------------------------------------------------------*/

Ref
GetFramePath(RefArg context, RefArg path)
{
	if (NOTNIL(context))
	{
		if (IsSymbol(path))
		{
			if (context == gFunctionFrame)
				return GlobalFunctionLookup(path);
			else
				return GetProtoVariable(context, path);
		}

		else if (ISINT(path))
		{
			if (IsArray(context))
			{
				ArrayIndex	i = RVALUE(path);
				if (i < ARRAYLENGTH(ObjectPtr(context)))
					return ((FrameObject *)ObjectPtr(context))->slot[i];
				else
					return NILREF;
			}
		}

		else if (IsArray(path))
		{
			if (EQRef(ClassOf(path), RSYMpathExpr))
			{
				Ref	current;
				Ref	pathElement;
				ArrayIndex	i, pathLen = ARRAYLENGTH(ObjectPtr(path));
				if (pathLen < 1)
					ThrowErr(exFrames, kNSErrEmptyPath);

				current = context;
				for (i = 0; i < pathLen && NOTNIL(current); ++i)
				{
					pathElement = ((ArrayObject *)ObjectPtr(path))->slot[i];
					if (ISINT(pathElement))
					{
						ArrayIndex	pathIndex = RVALUE(pathElement);
						if (IsArray(current) && pathIndex < ARRAYLENGTH(ObjectPtr(current)))
							current = ((ArrayObject *)ObjectPtr(current))->slot[pathIndex];
						else
							ThrowExFramesWithBadValue(kNSErrPathFailed, path);
					}
					else if (IsSymbol(pathElement))
					{
						if (current == gFunctionFrame)
							current = GlobalFunctionLookup(pathElement);
						else
							current = GetProtoVariable(current, pathElement);
					}
					else
						ThrowExFramesWithBadValue(kNSErrBadSegmentInPath, path);
				}
				return current;
			}
			else
				ThrowBadTypeWithFrameData(kNSErrNotAPathExpr, path);
		}
	}

	ThrowExFramesWithBadValue(kNSErrPathFailed, path);
	return NILREF;
}


/*----------------------------------------------------------------------
	Return the value of the named slot.
	Args:		context	a frame
				slot		a slot symbol
	Return:	Ref		the slot value
	Throw:	BadType	if context is not a frame
----------------------------------------------------------------------*/

Ref
GetFrameSlot(RefArg context, RefArg slot)
{
	return GetFrameSlotRef(context, slot);
}


Ref
GetFrameSlotRef(Ref context, Ref slot)
{
	if (context == gFunctionFrame)
		return GlobalFunctionLookup(slot);

	FrameObject * fr = (FrameObject *)FaultCheckObjectPtr(context);
	if (fr == NULL)
	{
		if (InHeap(slot))
			return SlowGetFrameSlot(context, slot);
		fr = (FrameObject *)ObjectPtr(context);
	}
	if (NOTFRAME(fr))
		ThrowBadTypeWithFrameData(kNSErrNotAFrame, context);

	ArrayIndex offset = FindOffset(fr->map, slot);
	return (offset == kIndexNotFound) ? NILREF : fr->slot[offset];
}


Ref
UnsafeGetFrameSlot(Ref context, Ref slot, bool * exists)
{
	if (context == gFunctionFrame)
	{
		Ref	result = GlobalFunctionLookup(slot);
		*exists = NOTNIL(result);
		return result;
	}
	
	FrameObject * fr = (FrameObject *)ObjectPtr(context);
	ArrayIndex offset = FindOffset(fr->map, slot);
	if (offset == kIndexNotFound)
	{
		*exists = NO;
		return NILREF;
	}
	else
	{
		*exists = YES;
		return fr->slot[offset];
	}
}


static Ref
SlowGetFrameSlot(Ref context, Ref slot)
{
	Ref	theSlot = NILREF;

	LockRef(slot);
	unwind_protect
	{
		theSlot = GetFrameSlotRef(MAKEPTR(ObjectPtr(context)), slot);
	}
	on_unwind
		UnlockRef(slot);
	end_unwind;

	return theSlot;
}


/*----------------------------------------------------------------------
	Return the symbol of the indexed slot.
	Args:		context	a frame map
				index		a slot index
				mapIndex	slot index within submap
	Return:	Ref		the slot symbol
----------------------------------------------------------------------*/

Ref
GetTag(Ref context, ArrayIndex index, ArrayIndex * mapIndex)
{
	if (ISNIL(context))
	{
		if (mapIndex)
			*mapIndex = 0;
		return NILREF;
	}

	ArrayIndex x = 0;
	FrameMapObject * map = (FrameMapObject *)ObjectPtr(context);
	Ref tag = GetTag(map->supermap, index, &x);	// recurse thru supermaps
	if (x == kIndexNotFound)
	{
		if (mapIndex)
			*mapIndex = kIndexNotFound;
		return tag;
	}

	ArrayIndex mapLength = Length(context) - 1 + x;
	if (mapLength > index)
	{
		if (mapIndex)
			*mapIndex = kIndexNotFound;
		return GetArraySlotRef(context, index - x + 1);
	}
	else
	{
		if (mapIndex)
			*mapIndex = mapLength;
		return NILREF;
	}
}


ArrayIndex
GetMapTags(Ref inMap, SortedMapTag * outTags)
{
	FrameMapObject *	map = (FrameMapObject *)ObjectPtr(inMap);
	Ref			supermap = map->supermap;
	ArrayIndex	numOfTags = 0;
	if (NOTNIL(supermap))
	{
		numOfTags = GetMapTags(supermap, outTags);
		outTags += numOfTags;
	}

	Ref *	slotPtr = &map->slot[0];
	for (ArrayIndex i = 0, count = (map->size - SIZEOF_FRAMEMAPOBJECT) / sizeof(Ref); i < count; ++i, ++outTags)
	{
		outTags->ref = *slotPtr++;
		outTags->index = numOfTags++;
	}
	return numOfTags;
}


int
CompareSymbols_qsort(const void * a, const void * b)
{
	return SymbolCompare(((SortedMapTag*)a)->ref, ((SortedMapTag*)b)->ref);
}


void
GetFrameMapTags(Ref inFrame, SortedMapTag * outTags, bool inSort)
{
	Ref			map = ((FrameObject *)ObjectPtr(inFrame))->map;
	ArrayIndex	numOfTags = GetMapTags(map, outTags);
	if (inSort)
		qsort(outTags, numOfTags, sizeof(SortedMapTag), CompareSymbols_qsort);
}


/*----------------------------------------------------------------------
	Sort the given frame’s map.
	Since nothing grows within this function, Refs may safely be used.
	Args:		context	a frame
				index		a slot index
	Return:	ArrayIndex		that index after sorting
----------------------------------------------------------------------*/

static ArrayIndex
ConvertToSortedMap(Ref context, ArrayIndex index)
{
	Ref			map = ((FrameObject *)ObjectPtr(context))->map;
	ArrayIndex	mapLen = Length(map);
	ArrayIndex	frOffset = Length(context) - mapLen;
	// iterate over map; 
	// slot 0 is supermap so start at 1
	// inner loop goes further so stop 1 short
	for (ArrayIndex index1 = 1; index1 < mapLen - 1; index1++)
	{
		ArrayIndex nextIndex = index1;
		Ref nextTag = GetArraySlotRef(map, index1);
		for (ArrayIndex indexn = index1 + 1; indexn < mapLen; indexn++)
		{
			Ref tagn = GetArraySlotRef(map, indexn);
			if (SymbolCompare(nextTag, tagn) > 0)
			{
				nextIndex = indexn;
				nextTag = tagn;
			}
		}

		if (nextIndex != index1)
		{
			// swap map entries
			Ref	r, r2;
			ArrayIndex	i, i2;
			r = GetArraySlotRef(map, index1);
			SetArraySlotRef(map, nextIndex, r);
			SetArraySlotRef(map, index1, nextTag);

			// swap frame entries
			i = index1 + frOffset;
			i2 = nextIndex + frOffset;
			r2 = GetArraySlotRef(context, i2);
			r = GetArraySlotRef(context, i);
			SetArraySlotRef(context, i, r2);
			SetArraySlotRef(context, i2, r);

			// update return value
			if (index == index1)
				index = nextIndex;
			else if (index == nextIndex)
				index = index1;
		}
	}

	((FrameMapObject *)ObjectPtr(((FrameObject *)ObjectPtr(context))->map))->objClass |= kMapSorted;
	FindOffsetCacheClear();

	return index;
}


/*----------------------------------------------------------------------
	Extend a frame map by a number of slots, sharing the existing frame
	map (as its supermap).
	Args:		inMap			a frame map
				inSize		the number of slots to add
	Return:	Ref			the new frame map
----------------------------------------------------------------------*/

Ref
ExtendSharedMap(RefArg inMap, ArrayIndex inSize)
{
	Ref	superMap = (ComputeMapSize(inMap) != 0) ? (Ref)inMap : NILREF;
	Ref	newMap = AllocateMap(superMap, inSize);
	((FrameMapObject *)ObjectPtr(newMap))->objClass = ((FrameMapObject *)ObjectPtr(inMap))->objClass & kMapProto;

	return newMap;
}


/*----------------------------------------------------------------------
	Shrink a frame map (remove a tag) where the map shares a supermap.
	In this case we need to create a new frame map that shares only the
	highest level supermap.
	Args:		inMap			the frame map
				inImplMap	the frame map actually containing the tag
				index			the index of the tag to remove
	Return:	Ref			the new frame map
----------------------------------------------------------------------*/

Ref
ShrinkSharedMap(RefArg inMap, RefArg inImplMap, ArrayIndex index)
{
	RefVar		superMap(((FrameMapObject *)ObjectPtr(inImplMap))->supermap);
	ArrayIndex	superSize = ISNIL(superMap) ? 0 : ComputeMapSize(superMap);
	ArrayIndex	newSize = ComputeMapSize(inMap) - superSize - 1;
	if (newSize != 0 || ISNIL(superMap))
	{
		RefVar		newMap = AllocateMap(superMap, newSize);
		ArrayIndex	i;
		for (i = superSize; i < index; ++i)
			SetArraySlot(newMap, 1 + i - superSize, GetTag(inMap, i));
		for ( ; i < newSize + superSize; ++i)
			SetArraySlot(newMap, 1 + i - superSize, GetTag(inMap, 1+i));
		return newMap;
	}
	return superMap;
}


/*----------------------------------------------------------------------
	Shrink an array (remove an element).
	Args:		ioArray		an array
				index			the index of the element to remove
	Return:	--				ioArray is modified
----------------------------------------------------------------------*/

void
ShrinkArray(RefArg ioArray, ArrayIndex index)
{
	ArrayObject * obj = (ArrayObject *)ObjectPtr(ioArray);
	ArrayIndex	limit = Length(ioArray) - 1;
	for ( ; index < limit; index++)
		obj->slot[index] = obj->slot[index + 1];

	UnsafeSetArrayLength(ioArray, limit);
}


/*----------------------------------------------------------------------
	Add the named slot.
	Args:		context	a frame
				tag		a slot symbol
	Return:	long		the index of the added slot
----------------------------------------------------------------------*/

ArrayIndex
AddSlot(RefArg context, RefArg tag)
{
	ArrayIndex frIndex = Length(context);
	UnsafeSetArrayLength(context, frIndex + 1);

	RefVar map(((FrameObject *)ObjectPtr(context))->map);
	FrameMapObject * mapPtr = (FrameMapObject *)ObjectPtr(map);
	if ((mapPtr->objClass & kMapShared) || (mapPtr->flags & kObjReadOnly))
	{
		map = ExtendSharedMap(map, 1);
		((FrameObject *)ObjectPtr(context))->map = map;
		SetArraySlot(map, 1, tag);
	}
	else
	{
		ArrayIndex	mapIndex, mapLen = Length(map);
		SetLength(map, mapLen + 1);
		mapPtr = (FrameMapObject *)ObjectPtr(map);	// just moved it (potentially)
		if (mapPtr->objClass & kMapSorted)
		{
			FindOffsetCacheClear();
			ICacheClearFrame(context);
			FrameObject * frPtr = (FrameObject *)ObjectPtr(context);
			for (mapIndex = mapLen - 1; mapIndex > 0; mapIndex--, frIndex--)
			{
				if (SymbolCompare(tag, mapPtr->slot[mapIndex - 1]) >= 0)
					break;
				mapPtr->slot[mapIndex] = mapPtr->slot[mapIndex - 1];
				frPtr->slot[frIndex] = frPtr->slot[frIndex - 1];
			}
			mapPtr->slot[mapIndex] = tag;
		}
		else
		{
			SetArraySlot(map, mapLen, tag);
			if (mapLen > 20)
				frIndex += (ConvertToSortedMap(context, mapLen) - mapLen);	// add diff in index
			ICacheClearFrame(context);
		}
	}

	ICacheClearSymbol(tag, SymbolHash(tag));

	if (EQ(tag, SYMA(_proto)))
		((ArrayObject *)ObjectPtr(map))->objClass |= kMapProto;

	SymbolObject * sym = (SymbolObject *)PTR(tag);
	ArrayIndex cacheIndex = ((context + sym->hash) >> kOffsetCacheShift) & kOffsetCacheSizeMask;
	OffsetCacheItem * cachePtr = &offsetCache[cacheIndex];
	if (map == cachePtr->context
	 && tag == cachePtr->slot)
	{
		cachePtr->offset = frIndex;
		gCached.offset = cachePtr;
	}

	return frIndex;
}


/*----------------------------------------------------------------------
	Remove the named slot.
	Args:		ioContext	a frame
				tag		a slot symbol
	Return:	--			ioContext is modified
	Throw:	BadType	if ioContext is not a frame
				exFrames	if ioContext is read-only
				BadType	if tag is not a symbol
----------------------------------------------------------------------*/

void
RemoveSlot(RefArg ioContext, RefArg tag)
{
	FrameObject * fr = (FrameObject *)ObjectPtr(ioContext);

	if ((fr->flags & kObjMask) != kFrameObject)
		ThrowBadTypeWithFrameData(kNSErrNotAFrame, ioContext);
	if (fr->flags & kObjReadOnly)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, ioContext);
	if (!IsSymbol(tag))
		ThrowBadTypeWithFrameData(kNSErrNotASymbol, tag);

	RefVar	frMap(fr->map);
	RefVar	implMap;
	ArrayIndex	index = FindOffset1(frMap, tag, (Ref*)implMap.h);
	if (NOTNIL(implMap))
	{
		// the slot exists
		FrameMapObject *	frMapPtr = (FrameMapObject *)ObjectPtr(implMap);
		if (Length(implMap) == 2)
		{
			// we’re removing the only slot in the frame
			// (2 => single tag + supermap ref), so unlink the framemap altogether
			if (EQ(implMap, frMap))
			{
				if (NOTNIL(frMapPtr->supermap))
				{
					((FrameObject *)ObjectPtr(ioContext))->map = frMapPtr->supermap;
					goto tagDone;
				}
			}
			else
			{
				// slot is in a supermap somewhere
				bool	wasMapUnlinked = NO;
				RefVar	map(frMap);
				FrameMapObject * mapPtr = (FrameMapObject *)ObjectPtr(map);
				// look for the map whose supermap contains the slot
				while (!EQRef(mapPtr->supermap, implMap))
				{
					map = mapPtr->supermap;
					mapPtr = (FrameMapObject *)ObjectPtr(map);
				}
				if ((mapPtr->objClass & kMapShared) == 0 && (mapPtr->flags & kObjReadOnly) == 0)
				{
					// we can safely modify it
					mapPtr->supermap = frMapPtr->supermap;
					if (SymbolCompare(tag, RSYM_proto) == 0)
					{
					// we’re removing the _proto slot so clear the flag
						mapPtr->objClass &= ~kMapProto;
					}
					wasMapUnlinked = YES;
				}
				if (wasMapUnlinked)
					goto tagDone;
			}
		}

		// at this point we need to shrink the frame map
		if (!EQ(implMap, frMap)
		|| (frMapPtr->objClass & kMapShared) || (frMapPtr->flags & kObjReadOnly))
		{
			// map is shared or read-only so we need to create a new one
			RefVar	newMap = ShrinkSharedMap(frMap, implMap, index);
			if (SymbolCompare(tag, RSYM_proto) != 0 && (ObjectFlags(newMap) & kObjReadOnly) == 0)
			{
				// we weren’t removing the _proto slot so copy kMapProto flag from original map
				((FrameMapObject *)ObjectPtr(newMap))->objClass |= (((FrameMapObject *)ObjectPtr(frMap))->objClass & kMapProto);
			}
			((FrameObject *)ObjectPtr(ioContext))->map = newMap;
		}
		else
		{
			// we can simply shrink the original map
			Ref	superMap = ((FrameMapObject *)ObjectPtr(frMap))->supermap;
			ArrayIndex	superSize = ISNIL(superMap) ? 0 : ComputeMapSize(superMap);
			ShrinkArray(frMap, 1 + index - superSize);
			if (SymbolCompare(tag, RSYM_proto) == 0)
			{
			// we’re removing the _proto slot so clear the flag
				frMapPtr->objClass &= ~kMapProto;
			}
		}

tagDone:
		// remove the slot value
		ShrinkArray(ioContext, index);
		// invalidate the cache
		FindOffsetCacheClear();
		ICacheClear();
	}
}


/*----------------------------------------------------------------------
	Set the value of the path.
	Args:		context	a frame
				path		the path
				value		the value to set
				for1x		do it NOS 1.x style -- not used in the modern Newton environment
	Return:	void		(the object is modified)
	Throw:	exFrames	if path is int but context is not array
				exFrames	if bad pathExpr
				BadType	if path is not symbol, int or pathExpr array
----------------------------------------------------------------------*/

void
SetFramePath(RefArg context, RefArg path, RefArg value)
{
	if (IsSymbol(path))
	{
		SetFrameSlot(context, path, value);
	}

	else if (ISINT(path))
	{
		if (!IsArray(context))
			ThrowExFramesWithBadValue(kNSErrPathFailed, path);
		SetArraySlot(context, RVALUE(path), value);
	}

	else if (IsArray(path) && EQRef(ClassOf(path), RSYMpathExpr))
	{
		ArrayIndex pathLen = ARRAYLENGTH(ObjectPtr(path));
		if (pathLen < 1)
			ThrowErr(exFrames, kNSErrEmptyPath);

		RefVar	current(context);
		RefVar	pathElement;
		ArrayIndex i;
		pathLen--;
		for (i = 0; i < pathLen && NOTNIL(current); ++i)
		{
			pathElement = GetArraySlot(path, i);
			if (ISINT(pathElement))
			{
				// path element is array index => current path must be an array
				if (!IsArray(current))
					ThrowExFramesWithBadValue(kNSErrPathFailed, path);
				current = GetArraySlot(current, RVALUE(pathElement));
			}
			else if (IsSymbol(pathElement))
			{
				// path element is a symbol => current path component must be a frame
				bool	varExists;
/*				if (for1x)
				{
					if (varExists = FrameHasSlot(current, pathElement))	// intentional assignment
						current = GetFrameSlot(current, pathElement);
				}
				else
*/				{
					Ref var = GetProtoVariable(current, pathElement, &varExists);
					if (varExists)
						current = var;
				}
				if (!varExists)
				{
					// current path component does not exist so create one => next path element must be a symbol
					if (ISINT(((ArrayObject *)ObjectPtr(path))->slot[i + 1]))
						ThrowExFramesWithBadValue(kNSErrPathFailed, path);
					RefVar newElement(AllocateFrame());
					SetFrameSlot(current, pathElement, newElement);
					current = newElement;
				}
			}
			else
				ThrowExFramesWithBadValue(kNSErrBadSegmentInPath, path);
		}
		if (ISNIL(current))
			ThrowExFramesWithBadValue(kNSErrPathFailed, path);
		pathElement = GetArraySlot(path, i);	// original says = ((ArrayObject *)ObjectPtr(path))->slot[i];
		if (ISINT(pathElement))
		{
			// final path element is array index => final path component must be an array
			if (!IsArray(current))
				ThrowExFramesWithBadValue(kNSErrPathFailed, path);
			SetArraySlot(current, RVALUE(pathElement), value);
		}
		else
			// final path element is symbol => final path component must be a frame
			SetFrameSlot(current, pathElement, value);
	}

	else
		ThrowExFramesWithBadValue(kNSErrNotAPathExpr, path);
}


/*----------------------------------------------------------------------
	Set the value of the named slot.
	Args:		context	a frame
				tag		a slot symbol
				value		the value to set
	Return:	void		(the object is modified)
	Throw:	BadType	if context is not a frame
				exFrames	if context is read-only
----------------------------------------------------------------------*/

void
SetFrameSlot(RefArg context, RefArg tag, RefArg value)
{
	FrameObject * fr = (FrameObject *)ObjectPtr(context);

	// object must be a writable frame
	if (NOTFRAME(fr))
		ThrowBadTypeWithFrameData(kNSErrNotAFrame, context);
	if (fr->flags & kObjReadOnly)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, context);

	ArrayIndex i = FindOffset(fr->map, tag);
	if (i == kIndexNotFound)
	{
		// slot doesn’t exist in frame, so add it
		i = AddSlot(context, tag);
		// context may have moved, so update pointer
		fr = (FrameObject *)ObjectPtr(context);
	}
	// set the value
	fr->slot[i] = value;
	DirtyObject(context);

	// if parent or proto was set, cache is now invalid
	ULong hash = SymbolHash(tag);
	if (hash == k_parentHash || hash == k_protoHash)
	{
		if (SymbolCompare(tag, RSYM_parent) == 0 || SymbolCompare(tag, RSYM_proto) == 0)
			ICacheClear();
	}
}


/*----------------------------------------------------------------------
	Return the frame map for a frame - and make it shared.
	Args:		inFrame		a frame
	Return:	the frame map
----------------------------------------------------------------------*/

Ref
SharedFrameMap(RefArg inFrame)
{
	Ref					map = ((FrameObject *)ObjectPtr(inFrame))->map;
	FrameMapObject *	mapPtr = (FrameMapObject *)ObjectPtr(map);
	if ((mapPtr->objClass & kMapShared) == 0 && (mapPtr->flags & kObjReadOnly) == 0)
		mapPtr->objClass |= kMapShared;
	return map;
}

#pragma mark -

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FGetPath(RefArg inRcvr, RefArg inObj, RefArg inTag)
{
	return GetFramePath(inObj, inTag);
}


Ref
FGetSlot(RefArg inRcvr, RefArg inObj, RefArg inTag)
{
	return GetFrameSlot(inObj, inTag);
}


Ref
FHasPath(RefArg inRcvr, RefArg inObj, RefArg inPath)
{
	return MAKEBOOLEAN(FrameHasPath(inObj, inPath));
}


Ref
FHasSlot(RefArg inRcvr, RefArg inObj, RefArg inTag)
{
	return MAKEBOOLEAN(FrameHasSlot(inObj, inTag));
}


Ref
FHasVar(RefArg inRcvr, RefArg inTag)
{
	bool  exists = NO;
	if (NOTNIL(inRcvr))
		GetVariable(inRcvr, inTag, &exists, kNoLookup);
	if (!exists)
		exists = FrameHasSlot(gVarFrame, inTag);
	return MAKEBOOLEAN(exists);
}


Ref
FRemoveSlot(RefArg inRcvr, RefArg inObj, RefArg inTag)
{
	ULong flags = ObjectFlags(inObj);

	if ((flags & kObjSlotted) == 0)
		ThrowBadTypeWithFrameData(kNSErrNotAFrameOrArray, inObj);

	if ((flags & kObjFrame) != 0)
		RemoveSlot(inObj, inTag);
	else
		ArrayRemoveCount(inObj, RINT(inTag), 1);

	return inObj;
}


Ref
FIsPathExpr(RefArg inRcvr, RefArg inObj)
{
	return MAKEBOOLEAN(IsPathExpr(inObj));
}


