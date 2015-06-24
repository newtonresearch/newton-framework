/*
	File:		LargeBinaries.cc

	Contains:	Large (virtual) binary object functions.

	Written by:	Newton Research Group.
*/

#include "LargeBinaries.h"
#include "ObjectHeap.h"
#include "Globals.h"
#include "OSErrors.h"
#include "DynamicArray.h"
#include "FaultBlocks.h"
#include "RefMemory.h"
#include "PackageParts.h"
#include "ROMResources.h"

extern void	InstantiateLargeObject(VAddr * ioAddr);	// HACK

extern bool	IsPackageHeader(Ptr inData, size_t inSize);

extern bool	RegisterRangeForDeclawing(ULong start, ULong end);
extern void	DeclawRefsInRegisteredRanges(void);


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

Ref	FindLargeBinaryInCache(CStoreWrapper * inStoreWrapper, PSSId inId);
bool	RegisterLargeBinaryForDeclawing(const LBData * inData);

size_t		LBLength(void * inData);
char *		LBDataPtr(void * inData);
void			LBSetLength(void * inData, size_t inLength);
Ref			LBClone(void * inData, Ref inClass);
void			LBDelete(void * inData);
void			LBSetClass(void * inData, RefArg inClass);
void			LBMark(void * inData);
void			LBUpdate(void * inData);


/*------------------------------------------------------------------------------
	D a t a
	S t a n d a r d   V B O   P r o c s
	Don’t know if this is the right kind of initialization.
------------------------------------------------------------------------------*/

const IndirectBinaryProcs  gLBProcs	// 0C1010A0
= { LBLength, LBDataPtr, LBSetLength, LBClone, LBDelete, LBSetClass, LBMark, LBUpdate };

Ref					gLBCache;		// 0C1010C0
CDynamicArray *	gLBStores;		// 0C1010C4


/*------------------------------------------------------------------------------
	I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/

void
InitLargeObjects(void)
{
	gLBCache = MakeEntryCache();
	AddGCRoot(&gLBCache);
	if ((gLBStores = new CDynamicArray(sizeof(LBStoreInfo), 1)) == NULL)	// elementSize, chunkSize
		OutOfMemory();
}


Ref
AllocateLargeBinary(RefArg inClass, StoreRef inClassRef, size_t inSize, CStoreWrapper * inStoreWrapper)
{
	RefVar	obj(gHeap->allocateIndirectBinary(inClass, sizeof(LBData)));

	IndirectBinaryObject * objPtr = (IndirectBinaryObject *)ObjectPtr(obj);
	objPtr->procs = &gLBProcs;

	LBData * largeBinary = (LBData *)&objPtr->data;
	largeBinary->fStoreIndex = kIndexNotFound;
	largeBinary->setStore(inStoreWrapper);
	largeBinary->fId = 0;	// kNoId
	largeBinary->fEntry = NILREF;
	largeBinary->fSize = inSize;
	largeBinary->fClass = inClassRef;
	largeBinary->fAddr = 0;

	return obj;
}


void
AbortLargeBinaries(RefArg inEntry)
{
	bool	isRegisteredForDeclawing = NO;
	for (int i = Length(gLBCache) - 1; i >= 0; i--)
	{
		Ref	entry = GetArraySlot(gLBCache, i);
		if (NOTNIL(entry))
		{
			CStoreWrapper * storeWrapper;
			IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(entry);
			LBData *		largeBinary = (LBData *)&objPtr->data;
			if (largeBinary->fAddr != 0
			&&  largeBinary->isSameEntry(inEntry)
			&&  (storeWrapper = largeBinary->getStore()) != NULL)
			{
				OSERRIF(AbortObject(storeWrapper->store(), *largeBinary));
				if (RegisterLargeBinaryForDeclawing(largeBinary))
					isRegisteredForDeclawing = YES;
				largeBinary->fAddr = 0;
			}
		}
	}
	if (isRegisteredForDeclawing)
		DeclawRefsInRegisteredRanges();
}


void
CommitLargeBinary(RefArg inObj)
{
	IndirectBinaryObject * objPtr = (IndirectBinaryObject *)ObjectPtr(inObj);
	LBData *		largeBinary = (LBData *)&objPtr->data;
	OSERRIF(CommitObject(largeBinary->fAddr));

	CStoreWrapper * storeWrapper = largeBinary->getStore();
	PSSId				objId = *largeBinary;
	RefVar	obj(FindLargeBinaryInCache(storeWrapper, objId));
	if (ISNIL(obj))
		PutEntryIntoCache(gLBCache, inObj);

	storeWrapper->removeEphemeral(objId);
}

// cf cache functions in Soups.cc
Ref
FindLargeBinaryInCache(CStoreWrapper * inStoreWrapper, PSSId inId)
{
	Ref	entry;
	for (ArrayIndex i = 0, count = Length(gLBCache); i < count; ++i)
	{
		entry = GetArraySlot(gLBCache, i);
		if (NOTNIL(entry))
		{
			IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(entry);
			LBData *		largeBinary = (LBData *)&objPtr->data;
			if (*largeBinary == inId
			&&  largeBinary->getStore() == inStoreWrapper)
				return entry;
		}
	}
	return NILREF;
}


bool
RegisterLargeBinaryForDeclawing(const LBData * inData)
{
	LBData *	largeBinary = const_cast<LBData*>(inData);
	if (largeBinary->fAddr != 0
	&& (!LargeObjectAddressIsValid(largeBinary->fAddr)
	  || IsPackageHeader((char*)largeBinary->fAddr, sizeof(PackageDirectory))))
		return RegisterRangeForDeclawing(largeBinary->fAddr, largeBinary->fAddr + largeBinary->fSize);

	return NO;
}


void
LargeBinariesStoreRemoved(CStoreWrapper * inStoreWrapper)
{
	RefVar	entry;
	bool		isRegisteredForDeclawing = NO;
	ArrayIndex count = Length(gLBCache);
	for (ArrayIndex i = 0; i < count; ++i)
	{
		entry = GetArraySlot(gLBCache, i);
		if (NOTNIL(entry))
		{
			IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(entry);
			LBData *		largeBinary = (LBData *)&objPtr->data;
			if (largeBinary->getStore() == inStoreWrapper)
			{
				if (RegisterLargeBinaryForDeclawing(largeBinary))
					isRegisteredForDeclawing = YES;
				largeBinary->fAddr = 0;
			}
		}
	}
	count = gLBStores->count();
	for (ArrayIndex i = 0; i < count; ++i)
	{
		LBStoreInfo *	storeInfo = (LBStoreInfo *)gLBStores->safeElementPtrAt(i);
		if (storeInfo->store == inStoreWrapper)
		{
			AbortObjects(inStoreWrapper->store());
			storeInfo->store = NULL;
		}
	}
	if (isRegisteredForDeclawing)
		DeclawRefsInRegisteredRanges();
}


Ref
LoadLargeBinary(CStoreWrapper * inStoreWrapper, PSSId inId, StoreRef inClassRef)
{
	RefVar	obj(FindLargeBinaryInCache(inStoreWrapper, inId));
	if (ISNIL(obj))
	{
		VAddr			addr;
		OSERRIF(MapLargeObject(&addr, inStoreWrapper->store(), inId, NO));
		size_t size = ObjectSize(addr);
		RefVar objClass(inStoreWrapper->referenceToSymbol(inClassRef));

		obj = AllocateLargeBinary(objClass, inClassRef, size, inStoreWrapper);

		IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(obj);
		LBData *		largeBinary = (LBData *)&objPtr->data;
		largeBinary->fId = inId;
		largeBinary->fAddr = addr;
		PutEntryIntoCache(gLBCache, obj);
	}
	return obj;
}


Ref
DuplicateLargeBinary(RefArg inObj, CStoreWrapper * inStoreWrapper)
{
	IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(inObj);
	LBData largeBinary = *(LBData *)&objPtr->data;
	CStoreWrapper *	storeWrapper;
	if ((storeWrapper = largeBinary.getStore()) == NULL)
		ThrowOSErr(kNSErrInvalidStore);
	CheckWriteProtect(storeWrapper->store());

	RefVar objClass(ClassOf(inObj));
	RefVar obj(AllocateLargeBinary(objClass, inStoreWrapper->symbolToReference(objClass), Length(inObj), inStoreWrapper));
	objPtr = (IndirectBinaryObject *)ObjectPtr(obj);

	inStoreWrapper->lockStore();
	newton_try
	{
		OSERRIF(DuplicateLargeObject(&largeBinary.fId, storeWrapper->store(), largeBinary, inStoreWrapper->store()));
		OSERRIF(MapLargeObject(&largeBinary.fAddr, storeWrapper->store(), largeBinary, NO));
		storeWrapper->addEphemeral(largeBinary);
	}
	cleanup
	{
		OSERRIF(inStoreWrapper->abort());
	}
	end_try;
	inStoreWrapper->unlockStore();

	return obj;
}


void
FinalizeLargeObjectWrites(CStoreWrapper * inStoreWrapper, CDynamicArray * inObjs, CDynamicArray * inObjsWritten)
{
	if (inObjs != NULL)
	{
		for (int i = inObjs->count() - 1; i >= 0; i--)
		{
			bool isWritten = NO;
			PSSId objId = *(PSSId *)inObjs->safeElementPtrAt(i);
			if (inObjsWritten != NULL)
			{
				for (int j = inObjsWritten->count() - 1; j >= 0; j--)
				{
					if (objId == *(PSSId *)inObjsWritten->safeElementPtrAt(i))
					{
						isWritten = YES;
						break;
					}
				}
			}
			if (!isWritten)
				DeleteLargeBinary(inStoreWrapper, objId);
		}
		inStoreWrapper->flushEphemerals();
	}
}


void
DeleteLargeBinary(CStoreWrapper * inStoreWrapper, PSSId inId)
{
	RefVar	obj(FindLargeBinaryInCache(inStoreWrapper, inId));
	if (NOTNIL(obj))
	{
		IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(obj);
		LBData * largeBinary = (LBData *)&objPtr->data;
		largeBinary->fEntry = NILREF;
	}
	else
		DeleteLargeObject(inStoreWrapper->store(), inId);
}


Ref
BreakLargeObjectToEntryLink(PSSId inId, CStoreWrapper * inStoreWrapper)
{
	RefVar	obj(FindLargeBinaryInCache(inStoreWrapper, inId));
	if (NOTNIL(obj))
	{
		IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(obj);
		LBData * largeBinary = (LBData *)&objPtr->data;
		largeBinary->fEntry = NILREF;
	}

	return obj;
}


/*------------------------------------------------------------------------------
	Determine whether object is large binary.
	Args:		r		an object
	Return	YES	if it is
------------------------------------------------------------------------------*/

bool
IsLargeBinary(Ref r)
{
	return ISPTR(r)
	&& !(ObjectFlags(r) & kObjSlotted)
	&& ((IndirectBinaryObject *)ObjectPtr(r))->procs == &gLBProcs;
}

#pragma mark -

/*------------------------------------------------------------------------------
	L B D a t a
------------------------------------------------------------------------------*/

CStoreWrapper *
LBData::getStore(void)	const
{
	if (fStoreIndex == kIndexNotFound)
		return NULL;

	LBStoreInfo * storeInfo = (LBStoreInfo *)gLBStores->safeElementPtrAt(fStoreIndex);
	return storeInfo->store;
}


void
LBData::setStore(CStoreWrapper * inStore)
{
	if (inStore == NULL)
	{
		LBStoreInfo *	storeInfo = (LBStoreInfo *)gLBStores->safeElementPtrAt(fStoreIndex);
		storeInfo->store = NULL;
		storeInfo->refCount--;
		fStoreIndex = kIndexNotFound;
		return;
	}

	ArrayIndex slot = kIndexNotFound;
	ArrayIndex numOfStores = gLBStores->count();
	for (ArrayIndex i = 0; i < numOfStores; ++i)
	{
		LBStoreInfo *	storeInfo = (LBStoreInfo *)gLBStores->safeElementPtrAt(i);
		if (storeInfo->store == inStore)
		{
			storeInfo->store = inStore;
			storeInfo->refCount++;
			fStoreIndex = i;
			return;
		}
		if (storeInfo->refCount == 0)
			slot = i;
	}
	if (slot != kIndexNotFound)
	{
		LBStoreInfo *	storeInfo = (LBStoreInfo *)gLBStores->safeElementPtrAt(slot);
		storeInfo->store = inStore;
		storeInfo->refCount = 1;
		fStoreIndex = slot;
	}
	else
	{
		LBStoreInfo		storeInfo = { inStore, 1 };
		gLBStores->insertElementsBefore(numOfStores, &storeInfo, 1);
		fStoreIndex = numOfStores;
	}
}


bool
LBData::isSameEntry(Ref r)
{
	if (IsFaultBlock(r))
		r = ((FaultObject *)NoFaultObjectPtr(r))->object;
	Ref	rr = fEntry;
	if (IsFaultBlock(rr))
		rr = ((FaultObject *)NoFaultObjectPtr(rr))->object;
	return NOTNIL(r) && NOTNIL(rr) && r == rr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	L a r g e   B i n a r y   O b j e c t   P r o c s
	C interface to C++
------------------------------------------------------------------------------*/

size_t
LBLength(void * inData)
{
	LBData *	largeBinary = reinterpret_cast<LBData*>(inData);
	return largeBinary->fSize;
}


char *
LBDataPtr(void * inData)
{
	LBData *	largeBinary = reinterpret_cast<LBData*>(inData);
	CStoreWrapper *	storeWrapper;
	if ((storeWrapper = largeBinary->getStore()) == NULL)
		ThrowOSErr(kNSErrInvalidStore);

	if (largeBinary->fAddr == 0)
	{
		NewtonErr	err;
		if ((err = MapLargeObject(&largeBinary->fAddr, storeWrapper->store(), *largeBinary, NO)) != noErr)
			ThrowOSErr(err);
	}
#if 0
	// before the implementation of CFlashStore we malloc’d large binaries lazily
	if (memcmp((void *)largeBinary->fAddr, "SIMY", 4) == 0)
	{
		// it’s a placeholder -- need to expand it
		// LB has been allocated but has no data -- we need to allocate space for the data
		// bear in mind the LB may be compressed and we will need to write data back to the LB at some point
		InstantiateLargeObject(&largeBinary->fAddr);
	}
#endif
	return (char *)largeBinary->fAddr;
}


void
LBSetLength(void * inData, size_t inLength)
{
	NewtonErr	err;
	LBData *	largeBinary = reinterpret_cast<LBData*>(inData);
	if (largeBinary->getStore() == NULL)
		ThrowOSErr(kNSErrInvalidStore);

	if ((err = ResizeLargeObject(&largeBinary->fAddr, largeBinary->fAddr, inLength, -1)) != noErr)
		ThrowOSErr(err);
	largeBinary->fSize = inLength;
}


Ref
LBClone(void * inData, Ref inClass)
{
	NewtonErr	err;
	LBData	clonedBinary = *reinterpret_cast<LBData*>(inData);
	CStoreWrapper *	storeWrapper;
	if ((storeWrapper = clonedBinary.getStore()) == NULL)
		ThrowOSErr(kNSErrInvalidStore);
	CheckWriteProtect(storeWrapper->store());

	RefVar	obj(AllocateLargeBinary(inClass, clonedBinary.fClass, clonedBinary.fSize, storeWrapper));
	IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(obj);
	LBData *		largeBinary = (LBData *)&objPtr->data;

	if ((err = DuplicateLargeObject(&largeBinary->fId, storeWrapper->store(), clonedBinary, storeWrapper->store())) != noErr)
		ThrowOSErr(err);
	if ((err = MapLargeObject(&largeBinary->fAddr, storeWrapper->store(), *largeBinary, NO)) != noErr)
		ThrowOSErr(err);

	storeWrapper->addEphemeral(*largeBinary);

	return obj;
}

// this function patched in System Update 717260
void
LBDelete(void * inData)
{
	LBData *	largeBinary = reinterpret_cast<LBData*>(inData);
	CStoreWrapper * storeWrapper = largeBinary->getStore();
	largeBinary->setStore(NULL);

	if (storeWrapper != NULL)
	{
		if (IsValidStore(storeWrapper->store()))
		{
			storeWrapper->deleteEphemeral(*largeBinary);
			AbortObject(storeWrapper->store(), *largeBinary);
			RegisterLargeBinaryForDeclawing(largeBinary);
		}
		else
			AbortObject(storeWrapper->store(), *largeBinary);
	}
}


void
LBSetClass(void * inData, RefArg inClass)
{
	LBData *	largeBinary = reinterpret_cast<LBData*>(inData);
	CStoreWrapper * storeWrapper;
	if ((storeWrapper = largeBinary->getStore()) == NULL)
		ThrowOSErr(kNSErrInvalidStore);

	if (largeBinary->fAddr == 0)
		MapLargeObject(&largeBinary->fAddr, storeWrapper->store(), *largeBinary, NO);
	OSERRIF(storeWrapper->lockStore());
	newton_try
	{
		largeBinary->fClass = storeWrapper->symbolToReference(inClass);
	}
	cleanup
	{
		OSERRIF(storeWrapper->abort());		// should we really be throwing in here?
	}
	end_try;
	OSERRIF(storeWrapper->unlockStore());
}


void
LBMark(void * inData)
{
	LBData *	largeBinary = reinterpret_cast<LBData*>(inData);
	DIYGCMark(largeBinary->fEntry);
}


void
LBUpdate(void * inData)
{
	LBData *	largeBinary = reinterpret_cast<LBData*>(inData);
	largeBinary->fEntry = DIYGCUpdate(largeBinary->fEntry);
}

#pragma mark -

Ref
FLBAllocCompressed(RefArg inStoreWrapper, RefArg inClass, RefArg inLength, RefArg inCompanderName, RefArg inCompanderParms)
{
	CStoreWrapper *	storeWrapper;
	CStore *				store;
	const char *		companderName;
	void *		companderParms;
	size_t		companderParmSize;
	PSSId			objId;
	VAddr			objAddr;

	GC();
	storeWrapper = (CStoreWrapper *)GetFrameSlot(inStoreWrapper, SYMA(store));
	store = storeWrapper->store();
	CheckWriteProtect(store);
	if (!storeWrapper->hasEphemerals())
		ThrowErr(exStore, kNSErrOldStoreFormat);

	companderName = NOTNIL(inCompanderName) ? BinaryData(ASCIIString(inCompanderName)) : "CSimpleStoreCompander";
	companderParms = NULL;
	companderParmSize = 0;
	if (NOTNIL(inCompanderParms))
	{
		companderParms = BinaryData(inCompanderParms);
		companderParmSize = Length(inCompanderParms);
	}

	OSERRIF(storeWrapper->lockStore());
	newton_try
	{
		NewtonErr	err;
		OSERRIF(CreateLargeObject(&objId, store, RINT(inLength), companderName, companderParms, companderParmSize));
		if ((err = MapLargeObject(&objAddr, store, objId, NO)) != noErr)
		{
			DeleteLargeObject(store, objId);
			ThrowOSErr(err);
		}
	}
	newton_catch_all
	{ }
	end_try;
	OSERRIF(storeWrapper->unlockStore());

	// do WrapLargeObject stuff
	storeWrapper->addEphemeral(objId);

	RefVar	obj(AllocateLargeBinary(inClass, storeWrapper->symbolToReference(inClass), RINT(inLength), storeWrapper));
	IndirectBinaryObject * objPtr = (IndirectBinaryObject *)ObjectPtr(obj);
	LBData * largeBinary = (LBData *)&objPtr->data;
	largeBinary->fId = objId;
	largeBinary->fAddr = objAddr;
	return obj;
}


Ref
FLBAlloc(RefArg inStoreWrapper, RefArg inClass, RefArg inLength)
{
	return FLBAllocCompressed(inStoreWrapper, inClass, inLength, RA(NILREF), RA(NILREF));
}


Ref
FLBRollback(RefArg inContext, RefArg inBinary)
{
	IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(inBinary);
	LBData *		largeBinary = (LBData *)&objPtr->data;
	CStoreWrapper *	storeWrapper;
	if ((storeWrapper = largeBinary->getStore()) == NULL)
		ThrowOSErr(kNSErrInvalidStore);
	OSERRIF(AbortObject(storeWrapper->store(), *largeBinary));
	if (RegisterLargeBinaryForDeclawing(largeBinary))
		DeclawRefsInRegisteredRanges();
	largeBinary->fAddr = 0;
	return NILREF;
}


void
MungeLargeBinary(RefArg inBinary, long inOffset, long inLength)
{
	IndirectBinaryObject *  objPtr = (IndirectBinaryObject *)ObjectPtr(inBinary);
	LBData *		largeBinary = (LBData *)&objPtr->data;
	CStoreWrapper *	storeWrapper;
	if ((storeWrapper = largeBinary->getStore()) == NULL)
		ThrowOSErr(kNSErrInvalidStore);
	OSERRIF(ResizeLargeObject(&largeBinary->fAddr, largeBinary->fAddr, inLength, inOffset));
	largeBinary->fSize = inLength;
}
