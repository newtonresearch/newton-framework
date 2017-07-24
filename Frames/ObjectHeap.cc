/*
	File:		ObjectHeap.cc

	Contains:	Newton object heap interface.

	Written by:	Newton Research Group.
*/
#undef forDebug
#define debugLevel 0

#include "Objects.h"
#include "ObjHeader.h"
#include "ObjectHeap.h"
#include "Iterators.h"
#include "Lookup.h"
#include "Symbols.h"
#include "Arrays.h"
#include "Frames.h"
#include "LargeBinaries.h"
#include "Globals.h"
#include "NewtGlobals.h"
#include "RefMemory.h"
#include "ROMResources.h"

extern "C" void	SafelyPrintString(UniChar * str);


/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FLength(RefArg inRcvr, RefArg inObj);
Ref	FReplaceObject(RefArg inRcvr, RefArg inTarget, RefArg inReplacement);
Ref	FClone(RefArg inRcvr, RefArg inObj);
Ref	FDeepClone(RefArg inRcvr, RefArg inObj);
Ref	FTotalClone(RefArg inRcvr, RefArg inObj);
Ref	FEnsureInternal(RefArg inRcvr, RefArg inObj);
Ref	FMakeBinary(RefArg inRcvr, RefArg inSize, RefArg inClass);
Ref	FBinaryMunger(RefArg inRcvr, RefArg a1, RefArg a1start, RefArg a1count,
											  RefArg a2, RefArg a2start, RefArg a2count);
Ref	FGC(RefArg inRcvr);
Ref	FStats(RefArg inRcvr);
Ref	FGetHeapStats(RefArg rcvr, RefArg inOptions);
Ref	FUriah(RefArg inRcvr);
Ref	FUriahBinaryObjects(RefArg inRcvr, RefArg inDoFile);
#if defined(forNTK)
Ref	FVerboseGC(RefArg rcvr, RefArg on);
#endif
}


/* ---------------------------------------------------------------------
	T y p e s
--------------------------------------------------------------------- */

struct DIYGCRegistration
{
	void *		refCon;
	GCProcPtr	mark;
	GCProcPtr	update;
};


struct GCRegistration
{
	void *		refCon;
	GCProcPtr	collect;
};


struct FreeBlock
{
	ObjHeader *	block;
	size_t		size;
};

#define kFreeBlockArraySize 32


/* ---------------------------------------------------------------------
	D e c l a w i n g R a n g e
	Something to do with packages.
--------------------------------------------------------------------- */

class CDeclawingRange
{
public:
				CDeclawingRange(ULong inStart, ULong inEnd, CDeclawingRange * inNext);
	bool		inRange(Ref ref) const;
	bool		inAnyRange(Ref ref) const;
	CDeclawingRange *	next(void) const;

private:
	ULong		rangeStart;
	ULong		rangeEnd;
	CDeclawingRange *	nextRange;
};

inline CDeclawingRange *	CDeclawingRange::next(void) const
{  return nextRange; }

bool
RegisterRangeForDeclawing(ULong start, ULong end)
{
	return gHeap->registerRangeForDeclawing(start, end);
}

void
DeclawRefsInRegisteredRanges(void)
{
	gHeap->declawRefsInRegisteredRanges();
}


/* ---------------------------------------------------------------------
	D a t a
--------------------------------------------------------------------- */
extern OffsetCacheItem offsetCache[];

ObjHeader * gROMSoupData;
ObjHeader * gROMSoupDataSize;


struct GCContext
{
	bool		verbose;						// print flag
	Ref **	roots;						// variable size array of Refs
	DIYGCRegistration *	extObjs;		// variable size array of DIYGCRegistrations
	GCRegistration *		extGC;		// variable size array of GCRegistrations
} gGC
= { true, NULL, NULL, NULL };

CObjectHeap *	gHeap = NULL;
ObjectCache		gCached = { INVALIDPTRREF, NULL, INVALIDPTRREF, 0, offsetCache };
ArrayIndex		gCurrentStackPos = 1;	// top 16 bits is interpreter identifier

bool			gPrintMaps = false;
bool			gUriahROM = false;
bool			gUriahPrintArrays = false;
bool			gUriahSaveOutput = false;


#pragma mark -
/* ---------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
--------------------------------------------------------------------- */

void
SetObjectHeapSize(size_t inSize, bool allocateInTempMemory)
{
	// INCOMPLETE!
}


/*----------------------------------------------------------------------
	Allocate a binary object in the main object heap.
	In:		inClass		class of the new object
				inLength		its length
	Return:	Ref			the binary object
----------------------------------------------------------------------*/

Ref
AllocateBinary(RefArg inClass, ArrayIndex inLength)
{
	return gHeap->allocateBinary(inClass, inLength);
}


/*----------------------------------------------------------------------
	Allocate a new array object in the main object heap.
	In:		inClass		the array class
								nil => 'array
				inLength		number of (nil) elements in array
	Return:	Ref			the array object
----------------------------------------------------------------------*/

Ref
AllocateArray(RefArg inClass, ArrayIndex inLength)
{
	return gHeap->allocateArray(inClass, inLength);
}


/*----------------------------------------------------------------------
	Allocate a new empty frame in the main object heap.
	In:		--
	Return:	Ref			the frame
----------------------------------------------------------------------*/

Ref
AllocateFrame(void)
{
	return gHeap->allocateFrame();
}


/*----------------------------------------------------------------------
	Allocate a new frame with an exising map.
	In:		inMap			a frame map
	Return:	Ref			the frame
----------------------------------------------------------------------*/

Ref
AllocateFrameWithMap(RefArg inMap)
{
	return gHeap->allocateFrameWithMap(inMap);
}


/*----------------------------------------------------------------------
	Allocate a new frame map.
	In:		inSuperMap	a frame map
				numOfTags	number of tags in map
	Return:	Ref			the frame map
----------------------------------------------------------------------*/

Ref
AllocateMap(RefArg inSuperMap, ArrayIndex numOfTags)
{
	return gHeap->allocateMap(inSuperMap, numOfTags);
}


/*----------------------------------------------------------------------
	Allocate a new frame map from an array of tags.
	In:		inSuperMap	a frame map
				tagsArray	array of tags
	Return:	Ref			the frame
----------------------------------------------------------------------*/

Ref
AllocateMapWithTags(RefArg inSuperMap, RefArg tagsArray)
{
	ArrayIndex limit = Length(tagsArray);
	Ref map = AllocateMap(inSuperMap, limit);
	FrameMapObject * mapPtr = (FrameMapObject *)PTR(map);
	ArrayObject * tags = (ArrayObject *)ObjectPtr(tagsArray);
	bool isProtoFlagSet = false;
	mapPtr->objClass = kMapShared;

	for (ArrayIndex i = 0; i < limit; ++i)
	{
		Ref tag = tags->slot[i];
		mapPtr->slot[i] = tag;
		if (!isProtoFlagSet && SymbolCompare(tag, SYMA(_proto)))
		{
			mapPtr->objClass |= kMapProto;
			isProtoFlagSet = true;
		}
	}

	return map;
}


/*----------------------------------------------------------------------
	Compute frame’s map size.
	In:		inMap			a frame map
	Return:	long			the number of tags in this and all supermaps.
----------------------------------------------------------------------*/

ArrayIndex
ComputeMapSize(Ref inMap)
{
	Ref	superMap = ((FrameMapObject *)ObjectPtr(inMap))->supermap;
	ArrayIndex	mapSize = Length(inMap) - 1;
	if (NOTNIL(superMap))
		mapSize += ComputeMapSize(superMap);

	return mapSize;
}


/*----------------------------------------------------------------------
	Set the length of an array object.
	Unsafe in that it doesn’t check the type of Ref passed in.
	It might be a frame.
	In:		obj		the object
				length	number of slots
	Return:	void		(the object is modified)
----------------------------------------------------------------------*/

void
UnsafeSetArrayLength(RefArg obj, ArrayIndex length)
{
	gHeap->unsafeSetArrayLength(obj, length);
}


/*----------------------------------------------------------------------
	Return the length of an array.
	Unsafe in that it doesn’t check the type of Ref passed in.
	In:		p			pointer to array object
	Return:	long		number of slots
----------------------------------------------------------------------*/

ArrayIndex
UnsafeArrayLength(Ptr p)
{
	return ARRAYLENGTH(((ArrayObject *)p));
}


/*----------------------------------------------------------------------
	Return the length of an object (frame/array, binary, large binary).
	In:		obj		the object
	Return:	long		number of slots or size of binary
----------------------------------------------------------------------*/

ArrayIndex
Length(Ref obj)
{
	ArrayIndex	len;
	ObjHeader *	p;

	if (obj == gCached.lenRef)
		return gCached.length;

	if (!(ISPTR(obj)))
		ThrowBadTypeWithFrameData(kNSErrUnexpectedImmediate, obj);

	gCached.lenRef = MAKEPTR(NULL);
	p = ObjectPtr(obj);
	if (gCached.lenRef != INVALIDPTRREF)
		gCached.lenRef = obj;

	if ((p->flags & kObjSlotted) != 0)
		len = ARRAYLENGTH(p);
	else if ((p->flags & kIndirectBinaryObject) != 0)
	{
		gCached.lenRef = INVALIDPTRREF;
		IndirectBinaryObject * vbo = (IndirectBinaryObject *)p;
		return vbo->procs->GetLength(vbo->data);
	}
	else
		len = BINARYLENGTH(p);

	gCached.length = len;
	return len;
}


/*----------------------------------------------------------------------
	Set the length of an object (array, binary, large binary).
	In:		obj		the object
				length	number of slots or size of binary
	Return:	void		(the object is modified)
----------------------------------------------------------------------*/

void
SetLength(RefArg obj, ArrayIndex length)
{
	ObjFlags		flags;
	ObjHeader * p;

	if (IsFrame(obj))
		ThrowBadTypeWithFrameData(kNSErrUnexpectedFrame, obj);
/*	if ((int)length < 0)
		ThrowExFramesWithBadValue(kNSErrNegativeLength, MAKEINT(length)); */

	p = ObjectPtr(obj);
	flags = (ObjFlags) p->flags;
	if ((flags & kObjReadOnly) != 0)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, obj);

	if ((flags & kObjMask) == kIndirectBinaryObject)	// large binary
	{
		IndirectBinaryObject * vbo = (IndirectBinaryObject *)p;
		vbo->procs->SetLength(vbo->data, length);
	}
	else if ((flags & kArrayObject) != 0)				// array
	{
		if (length > kMaxSlottedObjSize)
			ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(length));
		gHeap->unsafeSetArrayLength(obj, length);
	}
	else															// binary
	{
		if (length > kMaxObjSize)
			ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(length));
		gHeap->unsafeSetBinaryLength(obj, length);
	}
}


/*----------------------------------------------------------------------
	Replace one Ref with another.
	Actually, forward references to the new object.
----------------------------------------------------------------------*/

void
ReplaceObject(RefArg target, RefArg replacement)
{
	gHeap->replace(target, replacement);
}


/*----------------------------------------------------------------------
	Clone an object.
----------------------------------------------------------------------*/

Ref
Clone(RefArg obj)
{
	return gHeap->clone(obj);
}


Ref
DeepClone1(RefArg obj, CPrecedents & originals, CPrecedents & clones)
{
	RefVar		objClone;
	ArrayIndex	index;

	if ((index = originals.find(obj)) != kIndexNotFound)
		return clones.get(index);

	originals.add(obj);

	objClone = Clone(obj);
	clones.add(objClone);
	if ((ObjectFlags(obj) & kObjSlotted) != 0)
	{
		RefVar	slot;
		Ref		r;
		for (ArrayIndex i = 0, count = Length(obj); i < count; ++i)
		{
			r = ((ArrayObject *)ObjectPtr(obj))->slot[i];
			if (ISPTR(r))
			{
				slot = r;
				((ArrayObject *)ObjectPtr(obj))->slot[i] = DeepClone1(slot, originals, clones);
			
			}
		}
	}
	return objClone;
}


Ref
DeepClone(RefArg obj)
{
	Ref r = obj;
	if (ISPTR(r))
	{
		CPrecedents	o1, o2;
		r = DeepClone1(obj, o1, o2);
	}
	return r;
}


Ref
TotalClone1(RefArg obj, CPrecedents & originals, CPrecedents & clones, bool doShare)
{
	RefVar	objClone;
	RefVar	slotClone;
	ArrayIndex	 index;

	if ((index = originals.find(obj)) != kIndexNotFound)
		return clones.get(index);
	originals.add(obj);
	
	if (gHeap->inHeap(obj) || ISINROM(obj.h->ref))
	{
		// it’s internal so no need to clone
		if (doShare)
		{
			if (FLAGTEST(ObjectFlags(obj), kObjReadOnly))
			{
				clones.add(obj);
				return NILREF;		//	don’t descend into package or ROM
			}
			else
				objClone = obj;
		}
		else
			objClone = Clone(obj);
	}
	else
	{
		if (IsSymbol(obj))
			objClone = MakeSymbol(SymbolName(obj));
		else
			objClone = Clone(obj);
	}
	clones.add(obj);

	ArrayIndex count;
	if (FLAGTEST(ObjectFlags(obj), kObjSlotted))
		count = Length(obj) + 1;
	else
		count = 1;
	for (int i = 0; i < count; ++i)	// MUST be signed
	{
		slotClone = ((ArrayObject *)ObjectPtr(obj))->slot[i-1];		// start from objClass
		if (ISREALPTR(slotClone))
		{
			slotClone = TotalClone1(slotClone, originals, clones, doShare);
			if (NOTNIL(slotClone))
				((ArrayObject *)ObjectPtr(objClone))->slot[i-1] = slotClone;
		}
	}

	return objClone;
}


Ref
TotalClone(RefArg obj)
{
	Ref r = obj;
	if (ISPTR(r))
	{
		if (IsSymbol(r))
		{
			if (gHeap->inHeap(r) || ISINROM(r))
				// it’s internal so no need to clone
				return r;
			return MakeSymbol(SymbolName(r));
		}
		CPrecedents	o1, o2;
		r = TotalClone1(obj, o1, o2, false);
	}
	return r;
}


Ref
EnsureInternal(RefArg obj)
{
	Ref r = obj;
	if (ISPTR(r))
	{
		if (IsSymbol(r))
		{
			if (gHeap->inHeap(r) || ISINROM(r))
				// it’s already internal
				return r;
			return MakeSymbol(SymbolName(r));
		}
		CPrecedents	o1, o2;
		r = TotalClone1(obj, o1, o2, true);
		if (ISNIL(r))
			r = obj;
	}
	return r;
}


/*----------------------------------------------------------------------
	Munge a binary object.
	In:		a1				a binary object
				a1start		byte offset into a1
				a1len			byte length of section to be munged
				a2				a binary object
				a2start		byte offset into a2
				a2len			byte length of section
	Return:	void			a1 is modified
----------------------------------------------------------------------*/

void
BinaryMunger(RefArg a1, ArrayIndex a1start, ArrayIndex a1count,
				 RefArg a2, ArrayIndex a2start, ArrayIndex a2count)
{
	ArrayIndex	a1len, a2len;
	ArrayIndex	a1tail, a2tail;
	int			delta;
	Ptr			a1data, a2data;

	if ((ObjectFlags(a1) & kObjSlotted) != 0)
		ThrowBadTypeWithFrameData(kNSErrNotABinaryObject, a1);
	if (NOTNIL(a2) && (ObjectFlags(a2) & kObjSlotted) != 0)
		ThrowBadTypeWithFrameData(kNSErrNotABinaryObjectOrNil, a2);
	if (EQ(a1, a2))
		ThrowExFramesWithBadValue(kNSErrObjectsNotDistinct, a1);
	if ((ObjectFlags(a1) & kObjReadOnly) != 0)
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
	a1data = BinaryData(a1);
	a2data = ISNIL(a2) ? NULL : BinaryData(a2);
	if (delta)
		memmove(a1data + a1start, a1data + a1start + a1count, a1tail - a1count);
	if (a2count)
		memmove(a1data + a1start, a2data + a2start, a2count);
	if (delta < 0)
		SetLength(a1, a1len + delta);
}


#pragma mark -
/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FMakeBinary(RefArg inRcvr, RefArg inSize, RefArg inClass)
{
	return AllocateBinary(inClass, RINT(inSize));
}

Ref
FBinaryMunger(RefArg inRcvr, RefArg a1, RefArg a1start, RefArg a1count,
									  RefArg a2, RefArg a2start, RefArg a2count)
{
	BinaryMunger(a1, RINT(a1start), NOTNIL(a1count) ? RINT(a1count) : kIndexNotFound,
					 a2, RINT(a2start), NOTNIL(a2count) ? RINT(a2count) : kIndexNotFound);
	return a1;
}

Ref
FLength(RefArg inRcvr, RefArg inObj)
{
	return MAKEINT(Length(inObj));
}

Ref
FReplaceObject(RefArg inRcvr, RefArg inTarget, RefArg inReplacement)
{
	ReplaceObject(inTarget, inReplacement);
	return NILREF;
}

Ref
FClone(RefArg inRcvr, RefArg inObj)
{
	return Clone(inObj);
}

Ref
FDeepClone(RefArg inRcvr, RefArg inObj)
{
	return DeepClone(inObj);
}

Ref
FTotalClone(RefArg inRcvr, RefArg inObj)
{
	return TotalClone(inObj);
}

Ref
FEnsureInternal(RefArg inRcvr, RefArg inObj)
{
	return EnsureInternal(inObj);
}


#pragma mark -
/*----------------------------------------------------------------------
	R e f H a n d l e s
----------------------------------------------------------------------*/

RefHandle *
AllocateRefHandle(Ref inRef)
{
	return gHeap->allocateRefHandle(inRef);
}


void
DisposeRefHandle(RefHandle * inRefHandle)
{
	gHeap->disposeRefHandle(inRefHandle);
}


void
ClearRefHandles(void)
{
	gHeap->clearRefHandles();
}


#pragma mark -
/*----------------------------------------------------------------------
	S t a c k
----------------------------------------------------------------------*/

void
IncrementCurrentStackPos(void)
{
	++gCurrentStackPos;
}


void
DecrementCurrentStackPos(void)
{
	--gCurrentStackPos;
}


#pragma mark -


/*----------------------------------------------------------------------
	G a r b a g e   C o l l e c t i o n

	GC roots are pointers to refs.
	These refs are updated after garbage collection.
----------------------------------------------------------------------*/

void
AddGCRoot(Ref * inRoot)
{
	if (gGC.roots == NULL)
	{
		size_t allocSize = sizeof(Ref*);
		gGC.roots = (Ref **)NewPtr(allocSize);	// was NewHandle
		if (gGC.roots == NULL)
			OutOfMemory();
		gGC.roots[0] = inRoot;
	}
	else
	{
		Ref ** newRoots;
		size_t prevSize = GetPtrSize((Ptr)gGC.roots);
		newRoots = (Ref **)ReallocPtr((Ptr)gGC.roots, prevSize + sizeof(Ref*));
		if (newRoots == NULL)
			OutOfMemory();
		gGC.roots = newRoots;	
		gGC.roots[prevSize/sizeof(Ref*)] = inRoot;
	}
}


void
RemoveGCRoot(Ref * inRoot)
{
	if (gGC.roots != NULL)
	{
		size_t prevSize = GetPtrSize((Ptr)gGC.roots);
		Ref ** rp = gGC.roots;
		for (ArrayIndex i = 0, count = prevSize/sizeof(Ref*); i < count; ++i, ++rp)
		{
			if (*rp == inRoot)
			{
				if (i < count - 1)
					memmove(rp, rp + 1, (count - 1 - i) * sizeof(Ref*));
				gGC.roots = (Ref **)ReallocPtr((Ptr)gGC.roots, prevSize - sizeof(Ref*));	// need we bother?
				break;
			}
		}
	}
}


void
ClearGCRoots(void)
{
	// not in public jump table
}

#pragma mark -

void *
CommonGCRegister(void ** registrations, size_t gcHandlerSize)
{
	if (*registrations == NULL)
	{
		*registrations = NewPtr(gcHandlerSize);	// was NewHandle
		if (*registrations == NULL)
			ThrowErr(exOutOfMemory, kNSErrOutOfObjectMemory);	// like OutOfMemory() but specifically OutOfObjectMemory
		return *registrations;
	}
	else
	{
		void * newRegs;
		size_t prevSize = GetPtrSize((Ptr)*registrations);
		newRegs = ReallocPtr((Ptr)*registrations, prevSize + gcHandlerSize);
		if (newRegs == NULL)
			ThrowErr(exOutOfMemory, kNSErrOutOfObjectMemory);
		*registrations = newRegs;	
		return (void *)((Ptr)*registrations + prevSize);
	}
}


void
CommonGCUnregister(void ** registrations, size_t gcHandlerSize, void * refCon)
{
	char * p = (char *) *registrations;
	if (p != NULL)
	{
		size_t prevSize = GetPtrSize(p);
		Ptr end = p + prevSize;
		for ( ; p < end; p += gcHandlerSize)
		{
			if (((GCRegistration *)p)->refCon == refCon)
			{
				memmove(p, p + gcHandlerSize, prevSize - ((p + gcHandlerSize) - (char *)*registrations));
				*registrations = ReallocPtr((Ptr)*registrations, prevSize - gcHandlerSize);	// need we bother?
				return;
			}
		}
	}
}

#pragma mark -

void
DIYGCRegister(void * refCon, GCProcPtr markFunction, GCProcPtr updateFunction)
{
	DIYGCRegistration * reg = (DIYGCRegistration *)CommonGCRegister((void **)&gGC.extObjs, sizeof(DIYGCRegistration));
	reg->update = updateFunction;
	reg->mark = markFunction;
	reg->refCon = refCon;
}

void
DIYGCUnregister(void * refCon)
{
	CommonGCUnregister((void **)&gGC.extObjs, sizeof(DIYGCRegistration), refCon);
}

void
DIYGCMark(Ref r)
{
	gHeap->mark(r);
}

Ref
DIYGCUpdate(Ref r)
{
	return gHeap->update(r);
}

#pragma mark -

void
GCRegister(void * refCon, GCProcPtr proc)
{
	GCRegistration * reg = (GCRegistration *)CommonGCRegister((void **)&gGC.extGC, sizeof(GCRegistration));
	reg->collect = proc;
	reg->refCon = refCon;
}

void
GCUnregister(void * refCon)
{
	CommonGCUnregister((void **)&gGC.extGC, sizeof(GCRegistration), refCon);
}

void
ClearGCHooks()
{
	// not in public jump table
}


#pragma mark -
/*----------------------------------------------------------------------
	Garbage Collection.
----------------------------------------------------------------------*/

void
GC(void)
{
	gHeap->GC();
}


Ref
FGC(RefArg inRcvr)
{
	GC();
	return NILREF;
}

#if defined(forNTK)
Ref
FVerboseGC(RefArg rcvr, RefArg on)
{
	bool prevState = gGC.verbose;
	gGC.verbose = NOTNIL(on);
	return MAKEBOOLEAN(prevState);
}
#endif

#pragma mark -
/*----------------------------------------------------------------------
	Heap statistics.
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
	Return the heap bounds.
	Used only by FGetHeapStats.
	Now encapsulated in a CObjectHeap member function.
----------------------------------------------------------------------*/

inline void
HeapBounds(Ptr * start, Ptr * limit)
{
	gHeap->heapBounds(start, limit);
}


inline void
Statistics(size_t * freeSpace, size_t * largestFreeBlock)
{
	gHeap->heapStatistics(freeSpace, largestFreeBlock);
}


Ref
FStats(RefArg inRcvr)
{
	size_t	free, largest;
	gHeap->heapStatistics(&free, &largest);
	REPprintf("Free: %d, Largest: %d\n", free, largest);
	return NILREF;
}


#include "MemMgr.h"
#include "RDM.h"

void
GetActualHeapInfo(Heap inHeap, void ** outStart, void ** outEnd, ArrayIndex * outUsedBlocks, size_t * outFreeSize)
{
#if !defined(forFramework)
	for ( ; ; )
	{
		*outUsedBlocks = 0;
		*outFreeSize = 0;
		long seed = HeapSeed(inHeap);
		void * block = NULL;
		bool r8 = true;
		Size blockSize;
		ObjectId blockOwner;
		int result = NextHeapBlock(inHeap, seed, block, &block, NULL, NULL, NULL, &blockSize, &blockOwner);
		if (result == kMM_HeapSeedFailure)
			continue;
		if (result == kMM_HeapEndBlock)
			return;
		if (r8)
		{
			*outStart = *outEnd = (Ptr)block;
			r8 = false;
		}
		(*outUsedBlocks)++;
		*outEnd = (void *)((Ptr)*outEnd + blockSize);
		if (result == kMM_HeapFreeBlock)
			*outFreeSize += blockSize;
	}
#endif
}


Ref
FGetHeapStats(RefArg rcvr, RefArg inOptions)
{
	RefVar stats(AllocateFrame());

	bool garbageCollectFrames = false;
	bool includeSystemReleasable = false;
	if (NOTNIL(inOptions))
	{
		garbageCollectFrames = NOTNIL(GetFrameSlot(inOptions, MakeSymbol("garbageCollectFrames")));
		includeSystemReleasable = NOTNIL(GetFrameSlot(inOptions, MakeSymbol("includeSystemReleasable")));
	}

#if !defined(forFramework)
	ArrayIndex usedBlocks = 0;
	size_t freeSize = 0;
	size_t largestSize = 0;
	void * heapStart;
	void * heapEnd;
	size_t heapSize;

	GetActualHeapInfo(GetFixedHeap(GetHeap()), &heapStart, &heapEnd, &usedBlocks, &freeSize);
	heapSize = (Ptr)heapEnd - (Ptr)heapStart;
	SetFrameSlot(stats, MakeSymbol("ptrHeapStart"), MAKEINT((VAddr)heapStart>>2));
	SetFrameSlot(stats, MakeSymbol("ptrHeapSize"), MAKEINT(heapSize));
	SetFrameSlot(stats, MakeSymbol("ptrFreeSize"), MAKEINT(freeSize));

	GetActualHeapInfo(GetRelocHeap(GetHeap()), &heapStart, &heapEnd, &usedBlocks, &freeSize);
	heapSize = (Ptr)heapEnd - (Ptr)heapStart;
	SetFrameSlot(stats, MakeSymbol("handleHeapStart"), MAKEINT((VAddr)heapStart>>2));
	SetFrameSlot(stats, MakeSymbol("handleHeapSize"), MAKEINT(heapSize));
	SetFrameSlot(stats, MakeSymbol("handleFreeSize"), MAKEINT(freeSize));

	if (garbageCollectFrames)
		GC();

	HeapBounds((Ptr *)&heapStart, (Ptr *)&heapEnd);
	heapSize = (Ptr)heapEnd - (Ptr)heapStart;
	Statistics(&freeSize, &largestSize);
	SetFrameSlot(stats, MakeSymbol("framesHeapStart"), MAKEINT((VAddr)heapStart>>2));
	SetFrameSlot(stats, MakeSymbol("framesHeapSize"), MAKEINT(heapSize));
	SetFrameSlot(stats, MakeSymbol("framesFreeSize"), MAKEINT(freeSize));

	freeSize = TotalSystemFree();
	if (includeSystemReleasable)
	{
		ULong releasable, stackUsed, pagesUsed;
		GetSystemReleasable(&releasable, &stackUsed, &pagesUsed);
		freeSize += releasable + ROMDomainManagerFreePageCount() * kPageSize;
	}
	SetFrameSlot(stats, MakeSymbol("systemFreeSize"), MAKEINT(freeSize));
#endif

	return stats;
}


Ref
FUriah(RefArg inRcvr)
{
	gHeap->uriah();
	return NILREF;
}


Ref
FUriahBinaryObjects(RefArg inRcvr, RefArg inDoFile)
{
	gHeap->uriahBinaryObjects(NOTNIL(inDoFile));
	return NILREF;
}


#pragma mark -
/*----------------------------------------------------------------------
	Determine whether object is in the heap.
----------------------------------------------------------------------*/

bool
InHeap(Ref r)
{
	return gHeap->inHeap(r);
}


/*----------------------------------------------------------------------
	Determine whether object is on the stack (as opposed to in the heap).
----------------------------------------------------------------------*/

bool
OnStack(const void * obj)
{
	long  here;
	return (const void *)&here < obj && (VAddr)obj < GetNewtGlobals()->stackTop;
}


#pragma mark -


/*----------------------------------------------------------------------
	D e c l a w i n g R a n g e
------------------------------------------------------------------------
	Something to do with refs to packages, I think.
----------------------------------------------------------------------*/

CDeclawingRange::CDeclawingRange(ULong inStart, ULong inEnd, CDeclawingRange * inNext)
{
	nextRange = inNext;
	rangeEnd = inEnd;
	rangeStart = inStart;
}

bool
CDeclawingRange::inRange(Ref ref) const
{
	return (ULong)ref > rangeStart
		&& (ULong)ref <= rangeEnd;
}

bool
CDeclawingRange::inAnyRange(Ref ref) const
{
	const CDeclawingRange *	range;
	for (range = this; range != NULL; range = nextRange)
		if (range->inRange(ref))
			return true;
	return false;
	
}


#pragma mark -
/*----------------------------------------------------------------------
	O b j e c t   H e a p
------------------------------------------------------------------------
	Initialization.
	Allocate heap memory and initialize it.
	Add it to the garbage collector.
----------------------------------------------------------------------*/

CObjectHeap::CObjectHeap(size_t inSize, bool allocateInTempMemory)
{
	size_t heapSize;

	mem = (char *)malloc(inSize + 4);		// to long-align
	heapBase = (ObjHeader *)LONGALIGN(mem);
	heapSize = TRUNC(inSize, 4);
	heapLimit = INC(heapBase, heapSize);
	makeFreeBlock(heapBase, heapSize);
	splitBlock(heapBase, heapBase->size - sizeof(RefHandleBlock));
	refHBlock = (RefHandleBlock *)((Ptr) heapBase + LONGALIGN(heapBase->size));
	refHBlock->size = sizeof(RefHandleBlock);
	refHBlock->flags = kArrayObject;
	refHBlockSize = sizeof(RefHandleBlock);

	RefHandle *	rhp = refHBlock->data;
	for (ArrayIndex i = 0; i < kNumOfHandlesInBlock - 1; ++i, ++rhp) {
		rhp->ref = MAKEINT(i + 1);
		rhp->stackPos = MAKEINT(kIndexNotFound);
	}
	rhp->ref = MAKEINT(kIndexNotFound);
	rhp->stackPos = MAKEINT(kIndexNotFound);
	refHIndex = 0;

	objRoot = NILREF;
	AddGCRoot(&objRoot);

	freeHeap = heapBase;

	inGC = false;
	declaw = NULL;
	inDeclaw = false;
}


/*----------------------------------------------------------------------
	Remove this heap from the garbage collector.
----------------------------------------------------------------------*/

CObjectHeap::~CObjectHeap()
{
	RemoveGCRoot(&objRoot);
	free(mem);
}


#pragma mark -
/*----------------------------------------------------------------------
	R e f H a n d l e
------------------------------------------------------------------------
	The RefHandle provides a mechanism for tracking free/used Refs.
	It contains 2 Refs:
		ref		is the Ref it represents
		stackPos	is the index of the next free RefHandle in an
					allocated block
	If stackPos is kIndexNotFound then this is a free RefHandle and ref is the
	index of the next free one. All very confusing.
----------------------------------------------------------------------*/
#define NUMOFREFHANDLES ((refHBlock->size - sizeof(ObjHeader)) / sizeof(RefHandle))

bool gLogObjectHeapAllocations = false;

RefHandle *
CObjectHeap::allocateRefHandle(Ref inRef)
{
	// point to free RefHandle…
	RefHandle * refh = refHBlock->data + refHIndex;
	// sanity check -- ref handle MUST NOT be in use
	if (refh->stackPos != MAKEINT(kIndexNotFound)) {
		printf("\nCObjectHeap::allocateRefHandle() handle in use!\n");
	}
	// …which points to the next
	refHIndex = RVALUE(refh->ref);

	// populate this RefHandle
	refh->ref = inRef;
	refh->stackPos = MAKEINT(gCurrentStackPos);

if (gLogObjectHeapAllocations) { printf("allocating #%p, ref=%ld stackPos=%ld\n", refh, RVALUE(refh->ref), RVALUE(refh->stackPos)); }

	// The last free RefHandle has a ref of kIndexNotFound.
	// When this is reached it’s time to expand the RefHandle table.
	if (refHIndex == kIndexNotFound) {
		return expandObjectTable(refh);
	}
	return refh;
}


/*----------------------------------------------------------------------
	Return a RefHandle to the pool.
	Mark it as free by setting stackPos = kIndexNotFound, and ref to the next free entry.
	The next free entry is now this RefHandle.
----------------------------------------------------------------------*/

void
CObjectHeap::disposeRefHandle(RefHandle * inRefHandle)
{
	if (inRefHandle == NULL) {	// original doesn’t check
		printf("\nCObjectHeap::disposeRefHandle(NULL)\n");
	}

	int nxFree = refHIndex;
	if (nxFree < 0) {
		nxFree = kIndexNotFound;
	}
if (gLogObjectHeapAllocations) { printf(" disposing #%p, ref=%ld -> %d\n", inRefHandle, RVALUE(inRefHandle->ref), nxFree); }

	inRefHandle->ref = MAKEINT(nxFree);						// point to free RefHandle
	inRefHandle->stackPos = MAKEINT(kIndexNotFound);	// mark this as free
	refHIndex = (ArrayIndex)(inRefHandle - refHBlock->data);	// free RefHandle is now this
}


/*----------------------------------------------------------------------
	Expand the object table. Done when AllocateRefHandle can't find
	any free entries.
	The table always expands by kIncrHandlesInBlock which is 32.
----------------------------------------------------------------------*/

RefHandle *
CObjectHeap::expandObjectTable(RefHandle * inRefHandle)
{
	if (gGC.verbose) {
		REPprintf(">>>> expanding RefHandle block ");
	}
	size_t originalSize = refHBlock->size;
	refHBlockSize = LONGALIGN(originalSize + kIncrHandlesInBlock * sizeof(RefHandle));
	GC();
	// if size hasn't changed then there wasn't enough memory
	if ((refHBlock->size - originalSize) / sizeof(RefHandle) == 0) {
		refHIndex = - (long)(&refHBlock->data[0]) / sizeof(RefHandle);	// sic -- but pointless since we WILL crash hard
		OutOfMemory();
	}
	if (gGC.verbose) {
		REPprintf("<<<< number of RefHandles %d -> %d\n", (originalSize-sizeof(ObjHeader))/sizeof(RefHandle), NUMOFREFHANDLES);
	}
	return inRefHandle;
}


/*----------------------------------------------------------------------
	Clear RefHandles allocated after the position saved in gCurrentStackPos.
----------------------------------------------------------------------*/

void
CObjectHeap::clearRefHandles(void)
{
	ArrayIndex currentInterpreter = gCurrentStackPos >> 16;
	ArrayIndex currentStackPos = gCurrentStackPos & 0xFFFF;
	RefHandle * rhp = refHBlock->data;
	for (ArrayIndex i = 0, count = NUMOFREFHANDLES; i < count - 1; ++i, ++rhp) {	// last RefHandle is reserved
		ArrayIndex x = RVALUE(rhp->stackPos);
		if (x != 0 && (x >> 16) == currentInterpreter && (x & 0xFFFF) > currentStackPos) {
			DisposeRefHandle(rhp);
		}
	}
REPprintf("\n\n");
}


#pragma mark -
/*----------------------------------------------------------------------
	Coalesce free blocks until one of at least the required size
	is found / created.
----------------------------------------------------------------------*/

size_t
CObjectHeap::coalesceFreeBlocks(ObjHeader * freeObj, size_t reqSize)
{
	// trivial rejection of object that isn’t actually free
	if ((freeObj->flags & kObjFree) == 0)
		return 0;

	size_t freeSize = LONGALIGN(freeObj->size);
	if (freeSize < reqSize)
	{
		size_t nextSize;
		ObjHeader * nextObj;
		for (nextObj = INC(freeObj, freeSize); nextObj < heapLimit && (nextObj->flags & kObjFree) != 0; nextObj = INC(nextObj, nextSize))
		{
			nextSize = LONGALIGN(nextObj->size);
			freeSize += nextSize;
		}
		freeObj->size = freeSize;
		// if we coalesced the freeHeap, it’s now this free object
		if (freeHeap > freeObj && freeHeap < INC(freeObj, freeSize))
			freeHeap = freeObj;
	}

	return freeSize;
}


/*----------------------------------------------------------------------
	Find a free block of at least the required size.
----------------------------------------------------------------------*/

ObjHeader *
CObjectHeap::findFreeBlock(size_t reqSize)
{
	ObjHeader * freeObj = freeHeap;
	do {
		size_t freeSize = coalesceFreeBlocks(freeObj, reqSize);
		if (freeSize >= reqSize)
			return freeObj;
		freeObj = INC(freeObj, LONGALIGN(freeObj->size));
		if (freeObj >= heapLimit)
			freeObj = heapBase;
	} while (freeObj != freeHeap);

	return NULL;
}


/*----------------------------------------------------------------------
	Create a free block with the given address and size.
----------------------------------------------------------------------*/

void
CObjectHeap::makeFreeBlock(ObjHeader * obj, size_t newSize)
{
if (newSize == 0)
  printf("CObjectHeap::makeFreeBlock(#%p, %lu)\n", obj, newSize);
	obj->size = LONGALIGN(newSize);
	obj->flags = kObjFree;
	if (newSize > 8)
		obj->gc.stuff = 0;
	else if (newSize > 4)
		*(ULong *)&obj->gc = 0;
}


/*----------------------------------------------------------------------
	Split a block giving it the new (smaller) size.
----------------------------------------------------------------------*/

void
CObjectHeap::splitBlock(ObjHeader * obj, size_t newSize)
{
	size_t alignedNewSize = LONGALIGN(newSize);
	long dSize = LONGALIGN(obj->size) - alignedNewSize;

	if (dSize >= 4)
		makeFreeBlock(INC(obj, alignedNewSize), dSize);
if (newSize == 0)
  printf("CObjectHeap::splitBlock(#%p, %lu)\n", obj, newSize);
	obj->size = newSize;
	if (obj == freeHeap)
	{
		freeHeap = INC(obj, LONGALIGN(obj->size));
		if (freeHeap >= heapLimit)
			freeHeap = heapBase;
	}
}


/*----------------------------------------------------------------------
	Resize a block.
	This may move it in memory.
----------------------------------------------------------------------*/

ObjHeader *
CObjectHeap::resizeBlock(ObjHeader * obj, size_t newSize)
{
	size_t objSize = obj->size;
	size_t alignedObjSize = LONGALIGN(objSize);
	size_t alignedNewSize = LONGALIGN(newSize);
	long dSize = alignedNewSize - alignedObjSize;
if (newSize == 0)
  printf("CObjectHeap::resizeBlock(#%p, %lu)\n", obj, newSize);
	if (dSize == 0)		// size differs only fractionally
	{
		obj->size = newSize;
	}
	else if (dSize < 0)	// new size is smaller
	{
		splitBlock(obj, newSize);
	}
	else						// new size is larger
	{
		ObjHeader * nextObj;
		if ((nextObj = INC(obj, alignedObjSize)) < heapLimit
		&&  coalesceFreeBlocks(nextObj, newSize) >= (size_t)dSize)
		{
			// there's room after this block for obj to expand
			splitBlock(nextObj, dSize);
			obj->size = newSize;
		}
		else
		{
			if (FLAGTEST(obj->flags, kObjLocked))
				ThrowExFramesWithBadValue(kNSErrCouldntResizeLockedObject, MAKEPTR(obj));

			objRoot = MAKEPTR(obj);
			nextObj = allocateBlock(newSize, obj->flags);
			obj = PTR(objRoot);
			objRoot = NILREF;
			
			memmove((Ptr)nextObj + sizeof(ObjHeader), (Ptr)obj + sizeof(ObjHeader), alignedObjSize - sizeof(ObjHeader));
			makeFreeBlock(obj, obj->size);
			obj = nextObj;
		}
	}
	return obj;
}

#pragma mark -

/*----------------------------------------------------------------------
	Allocate a heap block of the given size and with the given flags.
----------------------------------------------------------------------*/

ObjHeader *
CObjectHeap::allocateBlock(size_t inSize, unsigned char flags)
{
	ObjHeader * obj = findFreeBlock(inSize);
	if (obj == NULL)
	{
		GC();
		obj = findFreeBlock(inSize);
		if (obj == NULL)
		{
			objRoot = NILREF;
			ThrowErr(exOutOfMemory, kNSErrOutOfObjectMemory);	// not OutOfMemory()
		}
	}
	splitBlock(obj, inSize);

if (obj->size == 0)
  printf("CObjectHeap::allocateBlock(%lu, %d) creates block of zero size\n", inSize, flags);
	freeHeap = INC(obj, LONGALIGN(obj->size));
	if (freeHeap >= heapLimit)
		freeHeap = heapBase;

	obj->flags = flags;
	obj->gc.stuff = 0;
	return obj;
}


/*----------------------------------------------------------------------
	Allocate a generic object of the given size and with the given flags.
	Initialize memory with nils (for arrays) or zeros (for binaries).
----------------------------------------------------------------------*/

Ref
CObjectHeap::allocateObject(size_t inSize, unsigned char flags)
{
	BinaryObject * obj = (BinaryObject *)allocateBlock(inSize, flags);

	obj->objClass = NILREF;
	if ((flags & kObjSlotted) != 0)
	{
		Ref * p = ((ArrayObject *)obj)->slot;
		for (ArrayIndex i = 0, numSlots = (inSize - SIZEOF_ARRAYOBJECT(0)) / sizeof(Ref); i < numSlots; ++i, ++p)
			*p = NILREF;
	}
	else if (inSize > SIZEOF_BINARYOBJECT)
	{
		memset(obj->data, 0, inSize - SIZEOF_BINARYOBJECT);
	}
	return MAKEPTR(obj);
}


/*----------------------------------------------------------------------
	Allocate a binary object of the given size.
----------------------------------------------------------------------*/

Ref
CObjectHeap::allocateBinary(RefArg inClass, size_t inSize)
{
/*	if (inSize < 0)
		ThrowExFramesWithBadValue(kNSErrNegativeLength, MAKEINT(inSize));
	else */ if (inSize > kMaxObjSize)
		ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inSize));

	Ref	obj = allocateObject(SIZEOF_BINARYOBJECT + inSize, kBinaryObject);
	((BinaryObject *)PTR(obj))->objClass = inClass;

	return obj;
}


/*----------------------------------------------------------------------
	Allocate a large binary object of the given size.
----------------------------------------------------------------------*/

Ref
CObjectHeap::allocateIndirectBinary(RefArg inClass, size_t inSize)
{
	Ref	obj = allocateObject(SIZEOF_INDIRECTBINARYOBJECT + inSize, kIndirectBinaryObject);
	((IndirectBinaryObject *)PTR(obj))->objClass = inClass;

	return obj;
}


/*----------------------------------------------------------------------
	Allocate an array of the given size.
----------------------------------------------------------------------*/

Ref
CObjectHeap::allocateArray(RefArg inClass, ArrayIndex inLength)
{
/*	if (inLength < 0)
		ThrowExFramesWithBadValue(kNSErrNegativeLength, MAKEINT(inLength));
	else */ if (inLength > kMaxSlottedObjSize)
		ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inLength));

	Ref	obj = allocateObject(SIZEOF_ARRAYOBJECT(inLength), kArrayObject);
	((ArrayObject *)PTR(obj))->objClass = inClass;

	return obj;
}


/*----------------------------------------------------------------------
	Allocate an empty frame.
----------------------------------------------------------------------*/

Ref
CObjectHeap::allocateFrame(void)
{
	// obj must be RefVar 'cause map allocation might move it
	RefVar obj(allocateObject(SIZEOF_FRAMEOBJECT(0), kFrameObject));
	Ref theMap = allocateMap(RA(NILREF), 0);
	((FrameObject *)PTR(obj))->map = theMap;

	return obj;
}


/*----------------------------------------------------------------------
	Allocate a frame with the given map.
----------------------------------------------------------------------*/

Ref
CObjectHeap::allocateFrameWithMap(RefArg inMap)
{
	Ref r = allocateObject(SIZEOF_FRAMEOBJECT(ComputeMapSize(inMap)), kFrameObject);
	((FrameObject *)PTR(r))->map = inMap;

	return r;
}


/*----------------------------------------------------------------------
	Allocate a frame map.
----------------------------------------------------------------------*/

Ref
CObjectHeap::allocateMap(RefArg inSuperMap, ArrayIndex inLength)
{
	Ref r = allocateObject(SIZEOF_FRAMEMAPOBJECT(inLength), kArrayObject);
	((FrameMapObject *)PTR(r))->objClass = kMapPlain;
	((FrameMapObject *)PTR(r))->supermap = inSuperMap;

	return r;
}

#pragma mark -

/*----------------------------------------------------------------------
	Resize a heap object.
	If a new object has to be created (ie we can’t adjust the size
	of the original) then forward refs from the old to the new.
----------------------------------------------------------------------*/

void
CObjectHeap::resizeObject(RefArg inObj, size_t inSize)
{
	ObjHeader * oPtr = ObjectPtr(inObj);
	if (oPtr->size != inSize)
	{
		ObjHeader * newObj = resizeBlock(oPtr, inSize);
		ObjHeader * oldObj = ObjectPtr(inObj);
		if (oldObj != newObj)
		{
			splitBlock(oldObj, sizeof(ForwardingObject));
			oldObj->flags = kObjForward;
			((ForwardingObject *)oldObj)->obj = MAKEPTR(newObj);
			if (gCached.ref == inObj)
				gCached.ptr = newObj;
		}
	}
}


/*----------------------------------------------------------------------
	Set the length of an array.
	If the array is lengthened, fill the new elements with NILREF.
	Unsafe in that it doesn’t check the type of Ref passed in.
	It might be a frame.
----------------------------------------------------------------------*/

void
CObjectHeap::unsafeSetArrayLength(RefArg inObj, ArrayIndex inLength)
{
	ArrayObject * obj = (ArrayObject *)ObjectPtr(inObj);
	size_t oldSize = obj->size;
	size_t newSize = SIZEOF_ARRAYOBJECT(inLength);
	if (newSize != oldSize)
	{
		ArrayIndex oldLength = ARRAYLENGTH(obj);
		resizeObject(inObj, newSize);
		obj = (ArrayObject *)ObjectPtr(inObj);
		Ref * p = &obj->slot[oldLength];
		while (oldLength++ < inLength)
			*p++ = NILREF;
		DirtyObject(inObj);
	}
	gCached.lenRef = INVALIDPTRREF;
}


/*----------------------------------------------------------------------
	Set the length of a binary object.
	If the binary is lengthened, fill the new data with zeros.
	Unsafe in that it doesn’t check the type of Ref passed in.
----------------------------------------------------------------------*/

void
CObjectHeap::unsafeSetBinaryLength(RefArg inObj, ArrayIndex inLength)
{
	BinaryObject * obj = (BinaryObject *)ObjectPtr(inObj);
	size_t oldSize = obj->size;
	size_t newSize = SIZEOF_BINARYOBJECT + inLength;
	if (newSize != oldSize)
	{
		resizeObject(inObj, newSize);
		obj = (BinaryObject *)ObjectPtr(inObj);
		if (oldSize < newSize)
			memset((Ptr)obj + oldSize, 0, newSize - oldSize);
		DirtyObject(inObj);
	}
	gCached.lenRef = INVALIDPTRREF;
}


/*----------------------------------------------------------------------
	Clone an object.
----------------------------------------------------------------------*/

Ref
CObjectHeap::clone(RefArg inObj)
{
	if (ISPTR(inObj))
	{
		ObjHeader * clonedObj;
		ObjHeader * obj = ObjectPtr(inObj);
		ObjFlags flags = (ObjFlags) (obj->flags & kObjMask);
		if ((flags & kObjSlotted) == 0)
		{
			if ((flags & kIndirectBinaryObject) != 0)
			{
				IndirectBinaryObject * vbo = (IndirectBinaryObject *)obj;
				return vbo->procs->Clone(vbo->data, ((BinaryObject *)obj)->objClass);
			}
			if (((BinaryObject *)obj)->objClass == kSymbolClass)
				return inObj;
		}
		clonedObj = allocateBlock(obj->size, flags);
		obj = ObjectPtr(inObj);
		memmove(INC(clonedObj, sizeof(ObjHeader)), INC(obj, sizeof(ObjHeader)), obj->size - sizeof(ObjHeader));
		if (flags == kFrameObject)
		{
			FrameMapObject * mapPtr = (FrameMapObject *)ObjectPtr(((FrameObject *)obj)->map);
			if (!(ISSHARED(mapPtr) || ISREADONLY(mapPtr)))
				mapPtr->objClass |= kMapShared;
		}
		return MAKEPTR(clonedObj);
	}
	return inObj;
}


/*----------------------------------------------------------------------
	Replace one object with another.
	Actually, forward references to the new object.
----------------------------------------------------------------------*/

void
CObjectHeap::replace(Ref ref1, Ref ref2)
{
	if (!ISPTR(ref1))
		ThrowBadTypeWithFrameData(kNSErrNotAPointer, ref1);
	if (!ISPTR(ref2))
		ThrowBadTypeWithFrameData(kNSErrNotAPointer, ref2);
	if (ref1 != ref2)
	{
		ForwardingObject * fo = (ForwardingObject *)NoFaultObjectPtr(ref1);
		if (ISREADONLY(fo))
			ThrowExFramesWithBadValue(kNSErrObjectReadOnly, ref1);
		splitBlock((ObjHeader *)fo, sizeof(ForwardingObject));
		fo->flags = kObjForward;
		fo->obj = ref2;
		if (gCached.ref == ref1)
			gCached.ref = INVALIDPTRREF;
		if (gCached.lenRef == ref1)
			gCached.lenRef = INVALIDPTRREF;
		ICacheClear();
	}
}


/*----------------------------------------------------------------------
	Determine whether the given object is within the heap.
----------------------------------------------------------------------*/

bool
CObjectHeap::inHeap(Ref inObj)
{
	if (!ISPTR(inObj))
		_RPTRError(inObj);

	ObjHeader * p = PTR(inObj);
	return p >= heapBase
		 && p < heapLimit;
}


/*----------------------------------------------------------------------
	Register a range of addresses for declawing.
----------------------------------------------------------------------*/

bool
CObjectHeap::registerRangeForDeclawing(ULong start, ULong end)
{
	CDeclawingRange * range = new CDeclawingRange(start, end, declaw);
	if (range)
	{
		declaw = range;
		return true;
	}
	return false;
}


/*----------------------------------------------------------------------
	Declaw refs in all registered address ranges.
----------------------------------------------------------------------*/

void
CObjectHeap::declawRefsInRegisteredRanges(void)
{
	ArrayIndex	i, count;
	SlottedObject *	obj;
	Ref *		p;

	// If no declawing ranges go no further
	if (declaw == NULL)
		return;

	inDeclaw = true;
PRINTF(("CObjectHeap::declawRefsInRegisteredRanges()\n"));
ENTER_FUNC
	// Update all refs in heap
	for (obj = (SlottedObject *)heapBase;
		  obj < (SlottedObject *)heapLimit;
		  obj = (SlottedObject *)INC(obj, LONGALIGN(obj->size)))
	{
		p = obj->slot;
		count = (obj->flags & kObjSlotted) != 0 ? SLOTCOUNT(obj) : 1;
		for (i = 0; i < count; ++i, ++p)
		{
			*p = update(*p);	//	updates map/class too
		}
		if (ISLARGEBINARY(obj))
		{
			IndirectBinaryObject * vbo = (IndirectBinaryObject *)obj;
			vbo->procs->UpdateRef(vbo->data);
		}
	}

	// Update all roots
	if (gGC.roots != NULL)
	{
		Ref ** rp = gGC.roots;
		count = GetPtrSize((Ptr)gGC.roots)/sizeof(Ref*);
PRINTF(("updating %d roots\n", count));
		for (i = 0; i < count; ++i, ++rp)
		{
			**rp = update(**rp);
		}
	}

	// Update all external objects
	if (gGC.extObjs != NULL)
	{
		DIYGCRegistration * regPtr = gGC.extObjs;
		count = GetPtrSize((Ptr)gGC.extObjs)/sizeof(DIYGCRegistration);
PRINTF(("updating %d external objects\n", count));
		for (i = 0; i < count; ++i, ++regPtr)
		{
			regPtr->update(regPtr->refCon);
		}
	}

	// Now finished with declawing ranges so delete them
	while (declaw)
	{
		CDeclawingRange * nextRange = declaw->next();
		delete declaw;
		declaw = nextRange;
	}
EXIT_FUNC
	inDeclaw = false;
}


/*----------------------------------------------------------------------
	Yer actual Garbage Collection.
----------------------------------------------------------------------*/
#define kHeapSize 4*MByte

void
CObjectHeap::GC(void)
{
	ArrayIndex	i, count;
	bool		isTWAMarked;
size_t free, largest;

	if (inGC)
		ThrowErr(exFrames, kNSErrGcDuringGc);
	inGC = true;
PRINTF(("CObjectHeap::GC()\n"));
ENTER_FUNC

	// pointer refs will change so invalidate the lookup cache
	gCached.ref = INVALIDPTRREF;
	gCached.lenRef = INVALIDPTRREF;
	FindOffsetCacheClear();

	if (gGC.verbose) {
		size_t free, largest;
		heapStatistics(&free, &largest);
		REPprintf("\n[ GC! start %ld/%ld...", free, largest);
	}
	
	isTWAMarked = false;
	weakLink = NULL;

	// mark the RefVar handle block
PRINTF(("marking RefHandle block\n"));
	mark(MAKEPTR(refHBlock));
	// mark all roots
	if (gGC.roots != NULL)
	{
		Ref ** rp = gGC.roots;
		count = GetPtrSize((Ptr)gGC.roots)/sizeof(Ref*);
PRINTF(("marking %d roots\n", count));
		for (i = 0; i < count; ++i, ++rp)
		{
			Ref * p = *rp;
			if (*p == gSymbolTable)
				isTWAMarked = true;
			else if (*p != INVALIDPTRREF)
				mark(*p);
		}
heapStatistics(&free, &largest);
if (free > kHeapSize)
REPprintf("wacko free heap size %d!\n",free);
	}

	// mark all external objects
	if (gGC.extObjs != NULL)
	{
		DIYGCRegistration * regPtr = gGC.extObjs;
		count = GetPtrSize((Ptr)gGC.extObjs)/sizeof(DIYGCRegistration);
PRINTF(("marking %d external objects\n", count));
		for (i = 0; i < count; ++i, ++regPtr)
		{
			regPtr->mark(regPtr->refCon);
		}
heapStatistics(&free, &largest);
if (free > kHeapSize)
REPprintf("wacko free heap size %d!\n",free);
	}

	// mark symbols
	if (isTWAMarked)
	{
PRINTF(("\n#### CLEANING SYMBOL TABLE ####\n"));
		GCTWA();
PRINTF(("\n#### MARKING SYMBOL TABLE ####\n"));
		mark(gSymbolTable);
heapStatistics(&free, &largest);
if (free > kHeapSize)
REPprintf("wacko free heap size %d!\n",free);
	}

	// sweep up unreferenced objects
PRINTF(("\n#### CLEANING WEAK ARRAYS ####\n"));
	cleanUpWeakChain();
heapStatistics(&free, &largest);
if (free > kHeapSize)
REPprintf("wacko free heap size %d!\n",free);
	sweepAndCompact();
heapStatistics(&free, &largest);
if (free > kHeapSize)
REPprintf("wacko free heap size %d!\n",free);
	declawRefsInRegisteredRanges();
heapStatistics(&free, &largest);
if (free > kHeapSize)
REPprintf("wacko free heap size %d!\n",free);

	inGC = false;

//	ValidateHeap(GetCurrentHeap(), -1);

	// collect all external objects
	if (gGC.extGC != NULL)
	{
		GCRegistration * regPtr = gGC.extGC;
		count = GetPtrSize((Ptr)gGC.extGC)/sizeof(GCRegistration);
PRINTF(("collecting %d external objects\n", count));
		for (i = 0; i < count; ++i, ++regPtr)
		{
			regPtr->collect(regPtr->refCon);
		}
heapStatistics(&free, &largest);
if (free > kHeapSize)
REPprintf("wacko free heap size %d!\n",free);
	}

	if (gGC.verbose) {
		size_t	free, largest;
		heapStatistics(&free, &largest);
		REPprintf("finish %ld/%ld ]\n", free, largest);
if (free > kHeapSize)
REPprintf("wacko free heap size %d!\n",free);
		uriah();
	}
EXIT_FUNC
}


/*----------------------------------------------------------------------
	Mark the given Ref as referenced, and therefore to be excluded from
	garbage collection.
	Mark all Refs in frames and arrays too, following pointer refs.
	Args:		ref		the ref to be marked
	Return:  --
----------------------------------------------------------------------*/

void
CObjectHeap::mark(Ref ref)
{
	// ref must be a binary object in the heap
	if (!(ISPTR(ref)
	 && ref > (Ref) heapBase
	 && ref < (Ref) heapLimit))
		return;

	Ref	marker = NILREF;
	Ref	link;
	SlottedObject *	obj;
	ArrayIndex	index;
#if debugLevel > 1
bool isIntroduced = false;
#endif

	for ( ; ; )
	{
		// Mark the object and its class (for binary and array objects)
		// or frame map chain (for frames).
		// Class and map are both treated as slot[0].
		while (ISPTR(ref)
		 && (obj = (SlottedObject *)PTR(ref)), obj >= (SlottedObject *)heapBase
		 && obj < (SlottedObject *)heapLimit
		 && (obj->flags & kObjMarked) == 0)
		{
#if debugLevel > 1
if (!isIntroduced) { isIntroduced = true; printf("MARK #%08X ", ref); }
#endif
			obj->flags |= kObjMarked;
			if (obj->slot[0] == kWeakArrayClass)
			{
				// thread object onto head of weakLink chain
				// pointer is stored directly in obj->slot[0]
#if debugLevel > 1
printf("[weak array] ~ #%08X ", weakLink);
#endif
				obj->slot[0] = (Ref) weakLink;
				weakLink = obj;
				break;
			}
			else
			{
				if (ISLARGEBINARY(obj))
				{
					IndirectBinaryObject * vbo = (IndirectBinaryObject *)obj;
#if debugLevel > 1
printf("VBO ");
#endif
					vbo->procs->Mark(vbo->data);
				}
				// Zero the slotIndex in preparation for walking slotted objects
				obj->gc.count.slots = 0;
				// Thread the ref into the marker list, overloading the ref’s slot[0]
				link = marker;
				marker = ref;
				ref = obj->slot[0];
				obj->slot[0] = link;
#if debugLevel > 1
printf("class/map #%08X ", ref);
#endif
			}
		}
#if debugLevel > 1
printf("\n");
#endif

		// If object has no class/map, go no further ---- SURELY WE WANT TO TRY THE NEXT OBJECT IN A FRAME/ARRAY?
		if (ISNIL(marker))
			break;

		// Now unwind the marker list
		obj = (SlottedObject *)PTR(marker);
		index = obj->gc.count.slots;
		link = ref;
		ref = marker;
		marker = obj->slot[index];
		obj->slot[index] = link;

		// If object is slotted, try next slot next
		if ((obj->flags & kObjSlotted) != 0 && ++index < SLOTCOUNT(obj))
		{
			obj->gc.count.slots = index;
			link = marker;
			marker = ref;
			ref = obj->slot[index];
			obj->slot[index] = link;
#if debugLevel > 1
printf("%3d: #%08X -> #%08X ", index, ref, link);
#endif
		}
	}
}


/*----------------------------------------------------------------------
	Garbage Collect T W A.
	Okay, after everything has been marked for retention, check the
	symbol table for unmarked symbols and remove them.
----------------------------------------------------------------------*/

void
CObjectHeap::GCTWA(void)
{
PRINTF(("CObjectHeap::GCTWA()\n"));
ENTER_FUNC
	Ref * p = ((ArrayObject *)ObjectPtr(gSymbolTable))->slot;
	for (ArrayIndex i = 0; i < gSymbolTableSize; ++i, ++p)
	{
		if (ISREALPTR(*p)
		 && inHeap(*p)
		 && (ObjectFlags(*p) & kObjMarked) == 0)
		{
#if debugLevel > 1
printf("#%08X <- nil\n", *p);
#endif
			*p = NILREF;	// original says 0
			gNumSymbols--;
		}
	}
EXIT_FUNC
}


/*----------------------------------------------------------------------
	The weak chain is a linked list of weak arrays.
	The next link pointer is a Ref in slot[0] of the SlottedObject
	which would normally be the objClass.
----------------------------------------------------------------------*/

void
CObjectHeap::cleanUpWeakChain(void)
{
PRINTF(("CObjectHeap::cleanUpWeakChain()\n"));
ENTER_FUNC
	SlottedObject * obj, * nextObj;
	for (obj = weakLink; obj != NULL; obj = nextObj)
	{
		Ref * p = &obj->slot[1];
#if debugLevel > 1
printf("#%08X\n", REF(obj));
#endif
		ArrayIndex	i, count = SLOTCOUNT(obj);
		for (i = 1; i < count; ++i, ++p)
		{
			Ref fref = *p;
			ForwardingObject * fo;
#if debugLevel > 1
printf("%3d: #%08X", i, fref);
#endif
			while (ISREALPTR(fref) && ((fo = (ForwardingObject *)PTR(fref)), (fo->flags & kObjForward) != 0))
			{
				fref = fo->obj;
#if debugLevel > 1
printf(" -> #%08X", fref);
#endif
			}
			if (ISREALPTR(fref)
			 && fref > (Ref) heapBase
			 && fref < (Ref) heapLimit
			 && (PTR(fref)->flags & kObjMarked) == 0)
			{
#if debugLevel > 1
printf(" <- nil");
#endif
				*p = NILREF;
			}
#if debugLevel > 1
printf("\n");
#endif
		}
		nextObj = (SlottedObject *)obj->slot[0];
		obj->slot[0] = kWeakArrayClass;
	}
EXIT_FUNC
}


/*----------------------------------------------------------------------
	Update a ref after objects have moved due to heap compaction.
	For pointer objects, validate and forward as necessary.
	Args:		ref		original pointer ref
	Return:				updated ref
----------------------------------------------------------------------*/

Ref
CObjectHeap::update(Ref ref)
{
	if (inDeclaw)
	{
		if (declaw->inAnyRange(ref))
			return kBadPackageRef;
	}
	else if (ISREALPTR(ref)								// we’re only interested in pointer objects in the heap
		  && ref > (Ref) heapBase
		  && ref < (Ref) heapLimit)
	{
#if debugLevel > 1
printf("UPDATE #%08X ", ref);
#endif
		ObjHeader * obj;
		while (obj = PTR(ref), (obj->flags & kObjForward) != 0)
		{
			Ref fref = ((ForwardingObject*)obj)->obj;
			if (ISREALPTR(fref))
			{
#if debugLevel > 1
printf("> #%08X ", fref);
#endif
				ref = fref;									// and only objects that forward to a pointer object
			}
			else
			{
#if debugLevel > 1
printf("-> #%08X\n", fref);
#endif
				return fref;
			}
		}
		if ((obj->flags & kObjMarked) == 0			// object that isn’t marked…
		 && ref > (Ref) heapBase						// …but is in the heap
		 && ref < (Ref) heapLimit)
		{
#if debugLevel > 1
printf("-> nil\n");
#endif
			return NILREF;									// is no longer required
		}

		else if ((obj->flags & kObjLocked) == 0	// object that isn’t locked…
			  && ref > (Ref) heapBase					// …and is in the heap
			  && ref < (Ref) heapLimit)
		{
#if debugLevel > 1
printf("-> #%08X\n", obj->gc.destRef);
#endif
			return obj->gc.destRef;						// has moved
		}
#if debugLevel > 1
printf("-> #%08X\n", ref);
#endif
	}
	return ref;
}


/*----------------------------------------------------------------------
	Sweep the heap for all unmarked refs and remove them, compacting
	the heap.
	Args:		--
	Return:  --
----------------------------------------------------------------------*/

void
CObjectHeap::sweepAndCompact(void)
{
	FreeBlock	a[kFreeBlockArraySize];
	Ref *			p;
	ObjHeader *	obj;
	UByte			objFlags;
	size_t		objSize;
	ArrayIndex	i, count;
	size_t		currentRefHBlockSize;
	long			refHDelta;
	ArrayIndex	blockIndex = 1;

	a[0].block = heapBase;
	a[0].size = 0;

PRINTF(("CObjectHeap::sweepAndCompact()\n"));
ENTER_FUNC

	// PASS 1
	// Calculate where objects will move to after compaction
	// and store the destination address in the object’s destRef.
	for (obj = heapBase;
		  obj < (ObjHeader *)refHBlock;						// exclude the ref handles block
		  obj = INC(obj, objSize))
	{
		objSize = LONGALIGN(obj->size);
		objFlags = obj->flags;
//printf(" #%lX[%d] ", obj, obj->size);
		if ((objFlags & kObjMarked) != 0 && (objFlags & kObjForward) == 0)
		{
			if ((objFlags & kObjLocked) != 0)
			{
				// object is marked but locked
				obj->gc.count.slots = 0;
				if (blockIndex < kFreeBlockArraySize)		// use next available entry
				{
					a[blockIndex++] = a[0];						// stack the currently free block
				}
				else													// look for empty entry
				{
					for (i = 1; i < blockIndex; ++i)
					{
						if (a[i].size == 0)
						{
							a[i] = a[0];							// stack free block there
							break;
						}
					}
					if (i == blockIndex)							// ie no free entries in array
					{
						obj->gc.count.slots = a[0].size;		// just remember the free size available
					}
				}
				a[0].block = INC(obj, LONGALIGN(obj->size));
				a[0].size = 0;										// point free block on to next
			}
			else
			{
				// object is marked and isn’t locked
				// so move it to the start of the free block
				FreeBlock * freep = &a[0];
				a[0].size += objSize;
				for (i = 1; i < blockIndex; ++i)				// look for a space in the free block stack first
				{
					if (a[i].size >= objSize)
					{
						freep = &a[i];
						break;
					}
				}
				freep->size -= objSize;								// remove block of this size from the free block
				obj->gc.destRef = MAKEPTR(freep->block);		// indicate where this block should end up
				freep->block = INC(freep->block, objSize);	// and update start of free block
//printf("-> #%lX \n", obj->gc.destRef-1);
			}
		}
		else
		{
			// object isn’t marked
			// so expand the free block
			a[0].size += LONGALIGN(obj->size);
//printf("! #%lX[%d] \n", a[0].block, a[0].size);
		}
/*
switch (objFlags & 0x03)
{
case 0:
	printf("binary");
	break;
case 1:
	printf("array");
	break;
case 2:
	printf("large binary");
	break;
case 3:
	printf("frame");
	break;
}
printf(" %s%s%s%s%s%s\n",
			(objFlags & kObjFree) != 0 ?		"free" : " ",
			(objFlags & kObjMarked) != 0 ?	"marked" : " ",
			(objFlags & kObjLocked) != 0 ?	"locked" : " ",
			(objFlags & kObjForward) != 0 ?	"forwarding" : " ",
			(objFlags & kObjReadOnly) != 0 ?	"read-only" : " ",
			(objFlags & kObjDirty) != 0 ?		"dirty" : " ");
*/
	}

	// Expand RefHandles block if requested
	currentRefHBlockSize = LONGALIGN(refHBlock->size);
	refHDelta = refHBlockSize - currentRefHBlockSize;
	if (refHDelta != 0)
	{
		if (refHDelta < a[0].size)
			a[0].size -= refHDelta;		// reduce the free size by the amount requested
		else
		{										// can’t expand by requested amount, but do as much as possible
			refHBlockSize = currentRefHBlockSize + a[0].size;
			refHDelta = a[0].size;
			a[0].size = 0;
		}
	}
	refHBlock->gc.destRef = MAKEPTR(refHBlock);	// this doesn’t move

	// PASS 2
	// Update refs in all objects in the heap
	// This means all object classes, frame maps and array elements
PRINTF(("updating heap objects\n"));
	for (obj = heapBase;
		  obj < heapLimit;						// include the ref handles block
		  obj = INC(obj, objSize))
	{
		objSize = LONGALIGN(obj->size);
		objFlags = obj->flags;
		if ((objFlags & kObjMarked) != 0 && (objFlags & kObjForward) == 0)
		{
			p = ((SlottedObject *)obj)->slot;
			count = (objFlags & kObjSlotted) != 0 ? SLOTCOUNT(obj) : 1;
			for (i = 0; i < count; ++i, ++p)	// slot 0 is class/map
			{
				*p = update(*p);
			}
			if ((objFlags & kObjMask) == kIndirectBinaryObject)
			{
				IndirectBinaryObject * vbo = (IndirectBinaryObject *)obj;
				vbo->procs->UpdateRef(vbo->data);
			}
		}
	}

	// Update all roots
	if (gGC.roots != NULL)
	{
		Ref ** rp = gGC.roots;
		count = GetPtrSize((Ptr)gGC.roots)/sizeof(Ref*);
PRINTF(("updating %d roots\n", count));
		for (i = 0; i < count; ++i, ++rp)
		{
			**rp = update(**rp);
		}
	}

	// Update all external objects
	if (gGC.extObjs != NULL)
	{
		DIYGCRegistration * regPtr = gGC.extObjs;
		count = GetPtrSize((Ptr)gGC.extObjs)/sizeof(DIYGCRegistration);
PRINTF(("updating %d external objects\n", count));
		for (i = 0; i < count; ++i, ++regPtr)
		{
			regPtr->update(regPtr->refCon);
		}
	}

	// PASS 3
	// Compact the heap
	// Move objects to their new coalesced locations
PRINTF(("compacting the heap\n"));
	for (obj = heapBase;
		  obj < heapLimit;
		  obj = INC(obj, objSize))
	{
		objSize = LONGALIGN(obj->size);
		objFlags = obj->flags;
		if ((objFlags & kObjMarked) != 0 && (objFlags & kObjForward) == 0)
		{
			obj->flags = objFlags & ~kObjMarked;
			if ((objFlags & kObjLocked) != 0)
			{
				// if there was free space before the locked block, free it now
//printf(" #%lX locked, freeing #%d\n", obj, obj->gc.count.slots);
				if (obj->gc.count.slots)
					makeFreeBlock(INC(obj, -obj->gc.count.slots), obj->gc.count.slots);
			}
			else	// block wasn't locked so move it to its destination
			{
				ObjHeader * destPtr = PTR(obj->gc.destRef);
				if (destPtr != obj)
				{
//printf(" #%lX -> #%lX\n", obj, destPtr);
					memmove(destPtr, obj, obj->size);
				}
//else
//printf(" #%lX --\n", obj);
				destPtr->gc.stuff = 0;
			}
		}
		else if ((objFlags & kObjMask) == kIndirectBinaryObject)
		{
			IndirectBinaryObject * vbo = (IndirectBinaryObject *)obj;
			vbo->procs->Delete(vbo->data);
		}
//else
//printf(" #%lX -- forwarding\n", obj);
	}

	if (refHDelta)	// RefHandle block needs expanding
	{
PRINTF(("expanding RefHandle block\n"));
		RefHandle * rhPtr;
		refHBlock = (RefHandleBlock *)((Ptr) refHBlock - refHDelta);
		refHBlock->size = refHBlockSize;
		refHBlock->flags = kArrayObject;
		refHBlock->gc.stuff = 0;
		rhPtr = refHBlock->data;
		count = refHDelta / sizeof(RefHandle);
		for (i = 0; i < count - 1; ++i, ++rhPtr)
		{
			rhPtr->ref = MAKEINT(i + 1);
			rhPtr->stackPos = MAKEINT(kIndexNotFound);
		}
		rhPtr->ref = MAKEINT(kIndexNotFound);
		rhPtr->stackPos = MAKEINT(kIndexNotFound);	// not done in the original for some reason
		refHIndex = 0;
	}

	// Free up all free blocks
	for (i = 0; i < blockIndex; ++i)
	{
		if (a[i].size > 0)
			makeFreeBlock(a[i].block, a[i].size);
	}

	freeHeap = a[0].block;
EXIT_FUNC
}


#pragma mark -

/*----------------------------------------------------------------------
	Return stats about the heap.
----------------------------------------------------------------------*/

void
CObjectHeap::heapBounds(Ptr * outStart, Ptr * outLimit)
{
	*outStart = (Ptr) heapBase;
	*outLimit = (Ptr) heapLimit;
}


void
CObjectHeap::heapStatistics(size_t * outFree, size_t * outLargest)
{
	size_t objSize;

	*outFree = *outLargest = 0;
	for (ObjHeader * obj = heapBase; obj < heapLimit; obj = INC(obj, objSize))
	{
		objSize = LONGALIGN(obj->size);
if (obj->size < 4 || obj->size > kHeapSize)
	printf("CObjectHeap::heapStatistics() object has wacko size %ld\n", objSize);
		if ((obj->flags & kObjFree) != 0)
		{
	 		*outFree += objSize;
	 		if (objSize > *outLargest)
	 			*outLargest = objSize;
		}
	}
}


/*----------------------------------------------------------------------
	Print heap contents.
----------------------------------------------------------------------*/

void
CObjectHeap::uriah(void)
{
	size_t	totalSize = 0;
	size_t	totalLockedSize = 0;
	size_t	totalFreeSize = 0;
	size_t	largestFreeSize = 0;
	size_t	intFrag = 0;

	size_t	frameMapSize = 0;
	size_t	frameMapObjSize = 0;
	size_t	symbolSize = 0;
	ArrayIndex	functionCount = 0;

	size_t	literalObjSize = 0;
	size_t	instructionSize = 0;
	size_t	functionSize = 0;
	size_t	literalSize = 0;
	size_t	binarySize = 0;
	ArrayIndex	binaryCount = 0;
	size_t	arraySize = 0;
	ArrayIndex	arrayCount = 0;
	size_t	frameSize = 0;
	ArrayIndex	frameCount = 0;
	ArrayIndex	viewCount = 0;

	Ref		objRef;
	Ref		objClassSym;

	ArrayIndex	i, limit;
	unsigned		objLockCount;
	size_t	objSize, heapSize;

	FILE * f;
	if (gUriahSaveOutput)
		f = fopen("Uriah Output", "w");
	else
		f = stdout;

	ObjHeader * obj;
	ObjHeader * objLimit;
	if (gUriahROM)
	{
		obj = gROMSoupData;
		objLimit = gROMSoupDataSize;
	}
	else
	{
		obj = heapBase;
		objLimit = heapLimit;
	}

	for ( ; obj < objLimit; obj = INC(obj, objSize))
	{
		objSize = LONGALIGN(obj->size);
		if (objSize == 0 || INC(obj, objSize) > objLimit)
		{
			fprintf(f, "#%p wacko size %lu!\n", obj, objSize);
			break;
		}

		totalSize += objSize;
		if ((obj->flags & kObjFree) != 0)
		{
			totalFreeSize += objSize;
			if (objSize > largestFreeSize)
				largestFreeSize = objSize;
		}
		else
		{
if (objSize > 32000)
	objRef = 0;
			intFrag += (objSize - obj->size);
			if ((obj->flags & kObjLocked) != 0)
			{
				totalLockedSize += objSize;
				if ((objLockCount = obj->gc.count.locks) != 0)
					fprintf(f, "#%p locked (count %d)\n", obj, objLockCount);
				else
					fprintf(f, "#%p has lock bit with zero count!\n", obj);
			}
			else
			{
				if ((objLockCount = obj->gc.count.locks) != 0)
					fprintf(f, "#%p lock bit clear with count %d!\n", obj, objLockCount);
			}
			objRef = MAKEPTR(obj);
			objClassSym = NILREF;
			if ((obj->flags & kObjSlotted) != 0)
			{
				if ((obj->flags & kObjFrame) != 0)
				{
					frameMapObjSize += SIZEOF_FRAMEMAPOBJECT(Length(objRef));
					frameSize += objSize;
					frameCount++;
					if (ISNIL(((FrameObject *)obj)->map))
						objClassSym = SYMA(frame);
					else if (FrameHasSlot(objRef, SYMA(viewCObject)))
						viewCount += objSize;
					else if (ISINT(GetFrameSlot(objRef, SYMA(_proto))))	// frame is NTK definition
						objClassSym = SYMA(frame);									// (not in original)
				}
				else
				{
					arraySize += objSize;
					arrayCount++;
					if (gUriahPrintArrays && NOTINT(((ArrayObject *)obj)->objClass) && ((ArrayObject *)obj)->objClass != kFaultBlockClass)
					{
						PrintObject(objRef, 0);
						fprintf(f, "\n");
					}
				}
			}
			else
			{
				binarySize += objSize;
				binaryCount++;
			}
			if (ISNIL(objClassSym))
				objClassSym = ClassOf(objRef);
			if (objClassSym == SYMA(symbol))
				symbolSize += objSize;
			else if (EQ(objClassSym, SYMA(CodeBlock)) || EQ(objClassSym, SYMA(_function)))
			{
				Ref instr = GetFrameSlot(objRef, SYMA(instructions));
				Ref litrl = GetFrameSlot(objRef, SYMA(literals));
				if (NOTNIL(instr))
				{
					functionCount++;
					instructionSize += LONGALIGN(ObjectPtr(instr)->size);
					if (NOTNIL(litrl))
						literalSize += Length(litrl) * sizeof(Ref);
					functionSize += objSize;
				}
				symbolSize += objSize;
			}
			else if (ISINT(objClassSym))	// obj must be a frame map, int is flags
			{
				frameMapSize += objSize;
				if (gPrintMaps)
				{
					fprintf(f, "map #%p ", obj);
					fprintf(f, objClassSym == kMapShared ? "  " : "* ");
					fprintf(f, "sup #%lX ", ((FrameMapObject *)obj)->supermap);
					for (ArrayIndex i = 0, numOfSlots = (obj->size - SIZEOF_FRAMEMAPOBJECT(0)) / sizeof(Ref); i < numOfSlots; ++i)
						fprintf(f, "%s ", BinaryData(((FrameMapObject *)obj)->slot[i]) + sizeof(ULong));
					fprintf(f, "\n");
				}
			}
			else if (objClassSym == SYMA(literals))
			{
				literalObjSize += objSize;
			}
/*
			// What on earth is all this about?
			bool isSlotted = (obj->flags & kObjSlotted) != 0;
			long	size = isSlotted ? SLOTCOUNT(obj) : 1;
			for (i = 0; i < size; ++i)
			{
				size = isSlotted ? SLOTCOUNT(obj) : 1;
			}
*/
		}
	}

	fprintf(f, "total %lu, free %lu, largest %lu, locked %lu, int frag %lu, ext frag %lu\n",
					totalSize, totalFreeSize, largestFreeSize, totalLockedSize,
					intFrag * 1000 / totalSize,
					(totalFreeSize - largestFreeSize) * 1000 / totalSize );
	fprintf(f, "%u scripts: bytecode %lu, literals p:%lu/v:%lu, grand total %lu\n",
					functionCount, functionSize, literalObjSize, literalSize, literalObjSize + functionSize + instructionSize);
	fprintf(f, "frames %lu(%u) maps p:%lu/v:%lu, %u viewframes\n",
					frameSize, frameCount, frameMapObjSize, frameMapSize, viewCount);
	fprintf(f, "arrays %lu(%u)\n",
					arraySize, arrayCount);
	fprintf(f, "binaries %lu(%u), symbols %lu\n",
					binarySize, binaryCount, symbolSize);

	heapSize = (long) heapLimit - (long) heapBase;
	if (heapSize != totalSize)
		fprintf(f, "*** total size should be %lu\n", heapSize);

	limit = (refHBlock->size - sizeof(ObjHeader)) / sizeof(RefHandle);
	i = 0;
	if (refHIndex != kIndexNotFound)
		for ( ; ; ++i)
		{
			if (i >= limit)
			{
				fprintf(f, "*** OT free list corrupted\n");
				break;
			}
			if (RINT(refHBlock->data[i].stackPos) == kIndexNotFound)
				break;
		}
	fprintf(f, "OT: %u used, %u handles\n", i, limit);

	if (gUriahSaveOutput && f)
		fclose(f);
}


/*----------------------------------------------------------------------
	Print just binaries in the heap.
----------------------------------------------------------------------*/

void
CObjectHeap::uriahBinaryObjects(bool doSaveOutput)
{
	CObjectHeap * uriahHeap = new CObjectHeap(4*KByte);
	RefVar	totalsFrame(uriahHeap->allocateFrame());
	RefVar	className;
	Ref		totalThisClass;
	size_t	objSize;

	FILE * f;
	if (doSaveOutput)
		f = fopen("Uriah Strings", "w");
	else
		f = stdout;

	ObjHeader * obj;
	ObjHeader * objLimit;
	if (gUriahROM)
	{
		obj = gROMSoupData;
		objLimit = gROMSoupDataSize;
	}
	else
	{
		obj = heapBase;
		objLimit = heapLimit;
	}

	for ( ; obj->size > 0 && obj < objLimit; obj = INC(obj, LONGALIGN(obj->size)))
	{
		unsigned objFlags = obj->flags;
		if ((objFlags & kObjSlotted) == 0
		&&  (objFlags & kObjFree) == 0
		&&  (objFlags & kObjForward) == 0)
		{
			Ref	r;
			objSize = LONGALIGN(obj->size);
			className = ClassOf(MAKEPTR(obj));
			totalThisClass = GetFrameSlot(totalsFrame, className);
			if (ISNIL(totalThisClass))
				r = MAKEINT(objSize);
			else
				r = MAKEINT(RINT(totalThisClass) + objSize);
			SetFrameSlot(totalsFrame, className, r);
			
			if (IsSubclass(className, SYMA(string)))
			{
				SafelyPrintString(((StringObject *)obj)->str);
				fprintf(f, "\n");
			}
		}
	}

	fprintf(f, "Summary of sizes of binary objects:\n");
	CObjectIterator	iter(totalsFrame);
	for (; !iter.done(); iter.next())
	{
		fprintf(f, "%s: %ld\n", ((SymbolObject *)PTR(iter.tag()))->name, RINT(iter.value()));
	}

	if (doSaveOutput && f)
		fclose(f);

	delete uriahHeap;		// original leaks
}
