/*
	File:		StoreWrapper.cc

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Arrays.h"
#include "ROMResources.h"
#include "UStringUtils.h"
#include "FaultBlocks.h"
#include "Indexes.h"
#include "Soups.h"
#include "UnionSoup.h"
#include "LargeBinaries.h"
#include "CachedReadStore.h"
#include "PrecedentsForIO.h"
#include "PSSManager.h"
#include "OSErrors.h"

#define k_uniqueIdHash 0xF33529EB
#define k_modTimeHash  0x6AC9BF7E

DeclareException(exBadType, exRootException);

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

extern bool		gAskedForFlush;
extern Ref		gPackageStores;


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" {
ULong	SymbolHashFunction(const char * name);
int	SymbolCompare(Ref sym1, Ref sym2);
}
extern CStore *	GetInternalStore(void);
extern Ref		FourCharToSymbol(ULong inFourChar);
extern unsigned long	RealClock(void);
extern long		GetRandomSignature(void);
extern Ref		GetStores(void);
extern bool		StoreWritable(CStore * inStore);
extern Ref		BreakLargeObjectToEntryLink(PSSId inId, CStoreWrapper * inStoreWrapper);
void				AskForFlush(bool inFlush);


/*------------------------------------------------------------------------------
	P l a i n   C   S t o r e   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

CStoreWrapper *
StoreWrapper(RefArg inRcvr)
{
	if (ISNIL(GetFrameSlot(inRcvr, SYMA(_proto))))
		ThrowErr(exStore, kNSErrInvalidStore);
	return (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));
}

CStore *
StoreFromWrapper(RefArg inRcvr)
{
	if (ISNIL(GetFrameSlot(inRcvr, SYMA(_proto))))
		ThrowErr(exStore, kNSErrInvalidStore);
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));
	return storeWrapper->store();
}


Ref
StoreAbort(RefArg inRcvr)
{
	CStoreWrapper * storeWrapper = StoreWrapper(inRcvr);
	OSERRIF(storeWrapper->store()->abort());
	return NILREF;
}

Ref
StoreLock(RefArg inRcvr)
{
	CStoreWrapper * storeWrapper = StoreWrapper(inRcvr);
	OSERRIF(storeWrapper->lockStore());
	return MAKEBOOLEAN(storeWrapper->store()->isLocked());
}

Ref
StoreUnlock(RefArg inRcvr)
{
	CStoreWrapper * storeWrapper = StoreWrapper(inRcvr);
	OSERRIF(storeWrapper->unlockStore());
	return MAKEBOOLEAN(storeWrapper->store()->isLocked());
}

Ref
StoreCardSlot(RefArg inRcvr)
{
#if !defined(forFramework)
	const PSSStoreInfo * info;
	CStore * store = StoreFromWrapper(inRcvr);
	if (store != NULL
	&&  (info = GetStorePSSInfo(store)) != NULL)
		return MAKEINT(info->f18);
#endif
	return NILREF;
}

Ref
StoreCardType(RefArg inRcvr)
{
#if !defined(forFramework)
	const PSSStoreInfo * info;
	CStore * store = StoreFromWrapper(inRcvr);
	if (store != NULL
	&&  (info = GetStorePSSInfo(store)) != NULL)
	{
		return FourCharToSymbol(info->f30);
	}
#endif
	return NILREF;
}

void
CheckWriteProtect(RefArg inRcvr)
{
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));
	CheckWriteProtect(storeWrapper->store());
}

Ref
StoreCheckWriteProtect(RefArg inRcvr)
{
	CheckWriteProtect(inRcvr);
	return NILREF;
}

Ref
StoreIsReadOnly(RefArg inRcvr)
{
	bool					isRO;
	CStoreWrapper *	storeWrapper = StoreWrapper(inRcvr);
	OSERRIF(storeWrapper->store()->isReadOnly(&isRO));
	return MAKEBOOLEAN(isRO);
}

Ref
StoreIsValid(RefArg inRcvr)
{
	if (NOTNIL(GetFrameSlot(inRcvr, SYMA(_proto))))
	{
		CStoreWrapper *	storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));
		if (IsValidStore(storeWrapper->store()))
			return TRUEREF;
		for (int i = Length(gPackageStores) - 1; i >= 0; i--)
		{
			if (EQ(inRcvr, GetArraySlot(gPackageStores, i)))
				return TRUEREF;
		}
	}
	return NILREF;
}

Ref
StoreGetPasswordKey(CStore * inStore)
{
	/* INCOMPLETE */
	return NILREF;
}

Ref
StoreGetPasswordKey(RefArg inRcvr)
{
	CStoreWrapper * storeWrapper = StoreWrapper(inRcvr);
	return StoreGetPasswordKey(storeWrapper->store());
}

Ref
StoreHasPassword(RefArg inRcvr)
{
	return MAKEBOOLEAN(NOTNIL(StoreGetPasswordKey(inRcvr)));
}

bool
CheckStorePassword(CStore * inStore, RefArg inPassword)
{ return true; }

Ref
StoreSetPassword(RefArg inRcvr, RefArg inOldPassword, RefArg inNewPassword)
{
	CStoreWrapper * storeWrapper = StoreWrapper(inRcvr);	//r8
	CStore * store = storeWrapper->store();
	if (store != GetInternalStore() && CheckStorePassword(store, inOldPassword) == 0) {
		CheckWriteProtect(store);
		/* INCOMPLETE */
	}
	return NILREF;
}

Ref
StoreGetKind(RefArg inRcvr)
{
	CStore * store = StoreFromWrapper(inRcvr);
	return MakeStringFromCString(store->storeKind());
}

Ref
StoreGetSignature(RefArg inRcvr)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);
	return GetFrameSlot(proto, SYMA(signature));
}

Ref
StoreSetSignature(RefArg inRcvr, RefArg inSignature)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);
	StoreCheckWriteProtect(inRcvr);
	SetFrameSlot(proto, SYMA(signature), inSignature);
	WriteFaultBlock(proto);
	return inSignature;
}

Ref
StoreGetName(RefArg inRcvr)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);
	return GetFrameSlot(proto, SYMA(name));
}

Ref
StoreSetName(RefArg inRcvr, RefArg inName)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);
	StoreCheckWriteProtect(inRcvr);
	SetFrameSlot(proto, SYMA(name), inName);
	WriteFaultBlock(proto);
	return inName;
}

Ref
StoreGetInfo(RefArg inRcvr, RefArg inTag)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);
	if (FrameHasSlot(proto, SYMA(info)))
	{
		RefVar	info(GetFrameSlot(proto, SYMA(info)));
		return GetFrameSlot(info, inTag);
	}
	return NILREF;
}

Ref
StoreSetInfo(RefArg inRcvr, RefArg inTag, RefArg inValue)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);
	StoreCheckWriteProtect(inRcvr);
	RefVar	tag(EnsureInternal(inTag));
	RefVar	value(TotalClone(inValue));
	if (FrameHasSlot(proto, SYMA(info)))
	{
		RefVar	info(GetFrameSlot(proto, SYMA(info)));
		SetFrameSlot(info, tag, value);
	}
	else
	{
		RefVar	info(AllocateFrame());
		SetFrameSlot(info, tag, value);
		SetFrameSlot(proto, SYMA(info), info);
	}
	WriteFaultBlock(proto);
	return NILREF;
}

Ref
StoreGetAllInfo(RefArg inRcvr)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);
	return Clone(GetFrameSlot(proto, SYMA(info)));
}

Ref
StoreSetAllInfo(RefArg inRcvr, RefArg info)
{
	if (!IsFrame(info))
		ThrowErr(exBadType, kNSErrNotAFrame);

	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);
	StoreCheckWriteProtect(inRcvr);
	SetFrameSlot(proto, SYMA(info), TotalClone(info));
	WriteFaultBlock(proto);
	return NILREF;
}


/*------------------------------------------------------------------------------
	Soups
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create a new soup on a store.
	Args:		inRcvr		the store object
				inName		name of the soup to create
				indexes		indexes for the soup
	Return:	a soup frame
------------------------------------------------------------------------------*/

Ref
StoreCreateSoup(RefArg inRcvr, RefArg inName, RefArg indexes)
{
	if (!IsString(inName))
		ThrowBadTypeWithFrameData(kNSErrNotAString, inName);
	if (IsRichString(inName))
		ThrowBadTypeWithFrameData(kNSErrNotAPlainString, inName);

	UniChar *	name;
	size_t		nameLen = Length(inName) / sizeof(UniChar);
	if (nameLen > 40)
	{
		RefVar	errFrame(AllocateFrame());
		SetFrameSlot(errFrame, SYMA(errorCode), MAKEINT(kNSErrInvalidSoupName));
		SetFrameSlot(errFrame, SYMA(value), inName);
		ThrowRefException(exFramesData, errFrame);
	}

	if (ISTRUE(StoreHasSoup(inRcvr, inName)))
		ThrowErr(exStore, kNSErrDuplicateSoupName);

	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);

	CStoreWrapper *	storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));
	CheckWriteProtect(storeWrapper->store());

	// create the soup object that holds info we need to be persistent
	RefVar	soupObj(Clone(RA(plainSoupPersistent)));
	SetFrameSlot(soupObj, SYMA(class), SYMA(diskSoup));
	SetFrameSlot(soupObj, SYMA(lastUId), MAKEINT(0));
	SetFrameSlot(soupObj, SYMA(signature), MAKEINT(GetRandomSignature()));
	ULong timeNow = RealClock() & 0x1FFFFFFF;
	SetFrameSlot(soupObj, SYMA(indexesModTime), MAKEINT(timeNow));
	SetFrameSlot(soupObj, SYMA(infoModTime), MAKEINT(timeNow));

	RefVar	theSoup;
	storeWrapper->lockStore();
	newton_try
	{
		NewtonErr	err;
		PSSId			objId = kNoPSSId;
		// add indexes
		AddNewSoupIndexes(soupObj, inRcvr, indexes);
		// make the soup object persistent
		StorePermObject(soupObj, storeWrapper, objId, NULL, NULL);

		// get the index for soup names
		CSoupIndex	soupIndex;
		soupIndex.init(storeWrapper, RINT(GetFrameSlot(proto, SYMA(nameIndex))), StoreGetDirSortTable(inRcvr));

		// create key for soup name
		SKey nameKey;
		UniChar * soupName = GetUString(inName);
		nameKey.set(Ustrlen(soupName)*sizeof(UniChar), soupName);
		// create key for soup PSS id
		SKey	nameData;
		nameData = (int)objId;
		// add that info to the soup name index
		if ((err = soupIndex.add(&nameKey, &nameData)) != noErr)
			ThrowErr(exStore, err > 0 ? kNSErrInternalError : err);

		theSoup = StoreGetSoup(inRcvr, inName);
		AddToUnionSoup(inName, theSoup);
	}
	cleanup
	{
		storeWrapper->abort();
		DeleteEntryFromCache(GetFrameSlot(inRcvr, SYMA(soups)), theSoup);
	}
	end_try;
	storeWrapper->unlockStore();
	return theSoup;
}


Ref
StoreCheckUnion(RefArg inRcvr)
{
	RefVar allStores(GetStores());		// r8
	RefVar store;
	RefVar badSoups;							// sp10
	int i, count = Length(allStores);	// r5, sp04
	for (i = count - 1; i >= 0; --i)
	{
		store = GetArraySlot(allStores, i);
		if (StoreHasSortTables(store))
			break;
	}
	if (i >= 0)
	{
		// at least one of the stores has sort tables
		CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));	// sp00
		CSoupIndex * indexes = new CSoupIndex[count];		// sp08
		if (indexes == NULL)
			OutOfMemory();
		newton_try
		{
			ArrayIndex theIndex = kIndexNotFound;		// sp0C
			for (i = 0; i < count; ++i)
			{
				store = GetArraySlot(allStores, i);
				RefVar proto(GetFrameSlot(store, SYMA(_proto)));
				indexes[i].init(StoreWrapper(store), RINT(GetFrameSlot(proto, SYMA(nameIndex))), StoreGetDirSortTable(store));
				if (EQ(store, inRcvr))
					theIndex = i;
			}
			// shouldn’t we check that theIndex != kIndexNotFound?
//			indexes[theIndex].search(1, NULL, NULL, CheckUnionStopFn, &storeWrapper, NULL, NULL);
		}
		cleanup
		{
			delete[] indexes;
		}
		end_try;
		delete[] indexes;
		return badSoups;	// @541:StoreMounted() expects an array of ‘incompatible’ soups
	}
	return NILREF;
}


Ref
StoreGetSoupNames(RefArg inRcvr)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));
	CSoupIndex soupIndex;
	soupIndex.init(storeWrapper, RINT(GetFrameSlot(proto, SYMA(nameIndex))), StoreGetDirSortTable(inRcvr));

	RefVar	soupNames(MakeArray(0));
	SKey	nameKey;
	SKey	nameData;	// will be the soup id
	NewtonErr err;
	for (err = soupIndex.first(&nameKey, &nameData); err == 0; err = soupIndex.next(&nameKey, &nameData, 0, &nameKey, &nameData))
	{
		AddArraySlot(soupNames, SKeyToKey(nameKey, SYMA(string), NULL));
	}
	return soupNames;
}


PSSId
StoreGetSoupId(RefArg inRcvr, RefArg inName)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);

	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));
	CSoupIndex soupIndex;
	soupIndex.init(storeWrapper, RINT(GetFrameSlot(proto, SYMA(nameIndex))), StoreGetDirSortTable(inRcvr));

	SKey nameKey;
	UniChar * name = GetUString(inName);
	nameKey.set(Ustrlen(name)*sizeof(UniChar), name);

	PSSId soupId;
	if (soupIndex.find(&nameKey, NULL, (SKey *)&soupId, false) == 0)
		return soupId;

	return 0;
}


Ref
StoreHasSoup(RefArg inRcvr, RefArg inName)
{
	return MAKEBOOLEAN(StoreGetSoupId(inRcvr, inName) != 0);
}


Ref
StoreGetSoup(RefArg inRcvr, RefArg inName)
{
	RefVar	proto(GetFrameSlot(inRcvr, SYMA(_proto)));
	if (ISNIL(proto))
		ThrowErr(exStore, kNSErrInvalidStore);

	// first look in the cache
	RefVar soups(GetFrameSlot(inRcvr, SYMA(soups)));
	RefVar theSoup(FindSoupInCache(soups, inName));
	if (ISNIL(theSoup))
	{
		// it’s not there; look for the persistent soup object on store
		// we’ll need to find it with a CSoupIndex
		CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(inRcvr, SYMA(store));
		CSoupIndex	soupIndex;
		soupIndex.init(storeWrapper, RINT(GetFrameSlot(proto, SYMA(nameIndex))), StoreGetDirSortTable(inRcvr));
		RefVar soupObj;

		// create key for soup name
		SKey nameKey;
		UniChar * name = GetUString(inName);
		nameKey.set(Ustrlen(name)*sizeof(UniChar), name);

		// soup name data is its id
		PSSId soupId;
		if (soupIndex.find(&nameKey, &nameKey, (SKey *)&soupId, false) == 0)
		{
			// we found the persistent soup object; load it
			soupObj = LoadPermObject(storeWrapper, soupId, NULL);
			soupObj = MakeFaultBlock(RA(NILREF), storeWrapper, soupId, soupObj);
			// create soup object for the NewtonScript world
			theSoup = Clone(RA(plainSoupPrototype));
			SetFrameSlot(theSoup, SYMA(_proto), soupObj);
			SetFrameSlot(theSoup, SYMA(TStore), (Ref)storeWrapper);
			SetFrameSlot(theSoup, SYMA(storeObj), inRcvr);
			// use the name key as the soup’s name
			RefVar keyObj(SKeyToKey(nameKey, SYMA(string), NULL));
			SetClass(keyObj, SYMA(string_2Enohint));
			SetFrameSlot(theSoup, SYMA(theName), keyObj);
			SetFrameSlot(theSoup, SYMA(cache), MakeEntryCache());
			SetFrameSlot(theSoup, SYMA(cursors), MakeEntryCache());
			CreateSoupIndexObjects(theSoup);
			RefVar nextUId(GetFrameSlot(soupObj, SYMA(lastUId)));
			if (ISNIL(nextUId))
			{
				// no cached lastUId; look for the last uid on store
				ULong uid;
				CSoupIndex * indexObj = GetSoupIndexObject(theSoup, 0);
				if (indexObj->last((SKey *)&uid, (SKey *)NULL) != 0)
					ThrowErr(exStore, kNSErrInternalError);
				nextUId = MAKEINT(uid + 1);
			}
			SetFrameSlot(theSoup, SYMA(indexNextUId), nextUId);
			// cache this soup object
			PutEntryIntoCache(soups, theSoup);
		}
	}

	return theSoup;
}


/*------------------------------------------------------------------------------
	Store objects interface
------------------------------------------------------------------------------*/

Ref
StoreNewObject(RefArg inRcvr, RefArg inSize)
{
	PSSId		objId;
	size_t	size = RINT(inSize);
	OSERRIF(StoreFromWrapper(inRcvr)->newObject(&objId, size));
	return MAKEINT(objId);
}


Ref
StoreDeleteObject(RefArg inRcvr, RefArg inObjId)
{
	PSSId		objId = RINT(inObjId);
	OSERRIF(StoreFromWrapper(inRcvr)->deleteObject(objId));
	return NILREF;
}


Ref
StoreGetObjectSize(RefArg inRcvr, RefArg inObjId)
{
	PSSId		objId = RINT(inObjId);
	size_t	size;
	OSERRIF(StoreFromWrapper(inRcvr)->getObjectSize(objId, &size));
	return MAKEINT(size);
}

Ref
StoreSetObjectSize(RefArg inRcvr, RefArg inObjId, RefArg inSize)
{
	PSSId		objId = RINT(inObjId);
	size_t	size = RINT(inSize);
	OSERRIF(StoreFromWrapper(inRcvr)->setObjectSize(objId, size));
	return NILREF;
}


Ref
StoreUsedSize(RefArg inRcvr)
{
	size_t	used, total;
	OSERRIF(StoreWrapper(inRcvr)->getStoreSizes(&used, &total));
	return MAKEINT(used);
}


Ref
StoreTotalSize(RefArg inRcvr)
{
	size_t	used, total;
	OSERRIF(StoreWrapper(inRcvr)->getStoreSizes(&used, &total));
	return MAKEINT(total);
}


Ref	StoreNewVBO(RefArg inRcvr, RefArg inArg2) { return NILREF; }
Ref	StoreNewCompressedVBO(RefArg inRcvr, RefArg inArg2, RefArg inArg3, RefArg inArg4) { return NILREF; }
Ref	StoreReadObject(RefArg inRcvr, RefArg inArg2, RefArg inArg3) { return NILREF; }
Ref	StoreWriteObject(RefArg inRcvr, RefArg inArg2, RefArg inArg3, RefArg inArg4) { return NILREF; }
Ref	StoreWriteWholeObject(RefArg inRcvr, RefArg inArg2, RefArg inArg3, RefArg inArg4) { return NILREF; }
Ref	StoreRestorePackage(RefArg inRcvr) { return NILREF; }
Ref	StoreRestoreSegmentedPackage(RefArg inRcvr, RefArg inArg2) { return NILREF; }


#pragma mark -
/*------------------------------------------------------------------------------
	C S t o r e H a s h T a b l e
------------------------------------------------------------------------------*/

PSSId
CStoreHashTable::create(CStore * inStore)
{
	PSSId		tableId;
	PSSId		table[kStoreHashTableSize];
	memset(table, 0, sizeof(table));
	OSERRIF(inStore->newObject(&tableId, table, sizeof(table)));
	return tableId;
}


CStoreHashTable::CStoreHashTable(CStore * inStore, PSSId inId)
{
	fId = inId;
	fStore = inStore;
	OSERRIF(inStore->read(inId, 0, fTable, sizeof(fTable)));
}


StoreRef
CStoreHashTable::insert(ULong inHash, const char * inData, size_t inSize)
{
	ArrayIndex	index = inHash & 0x3F;	// r10
	PSSId		objId = fTable[index];	// r1, sp00
	size_t	prefixedSize = sizeof(short) + inSize;
	int		offset = 0;	// r7
	bool		weAlreadyHaveThisData = false;	// r9

	if (objId == 0)
	{
		OSERRIF(fStore->newObject(&objId, prefixedSize));
		OSERRIF(fStore->write(fId, index * sizeof(PSSId), &objId, sizeof(objId)));
	}
	else
	{
		size_t	objSize;
		OSERRIF(fStore->getObjectSize(objId, &objSize));
		if (objSize > inSize)
		{
			CCachedReadStore	cacheStore(fStore, objId, objSize);
			uint16_t itemSize;
			for ( ; offset < objSize; offset += (sizeof(itemSize) + itemSize))
			{
				void *	dataPtr;		//sp-04
				OSERRIF(cacheStore.getDataPtr(offset, sizeof(itemSize), &dataPtr));
				if (offset & 1)
					memmove(&itemSize, dataPtr, sizeof(itemSize));
				else
					itemSize = *(uint16_t *)dataPtr;
				if (itemSize == inSize)
				{
					OSERRIF(cacheStore.getDataPtr(offset + sizeof(uint16_t), inSize, &dataPtr));
					if (memcmp(dataPtr, inData, inSize) == 0)
					{
						weAlreadyHaveThisData = true;
						break;
					}
				}
			}
		}
		if (!weAlreadyHaveThisData)
		{
			OSERRIF(fStore->setObjectSize(objId, objSize + prefixedSize));
			offset = objSize;
		}
	}

	if (!weAlreadyHaveThisData)
	{
		if (prefixedSize <= 1024)
		{
			// use static buffer and one write operation
			char	buf[1024];
			*(uint16_t *)buf = inSize;
			memmove(buf + sizeof(uint16_t), inData, inSize);
			OSERRIF(fStore->write(objId, offset, buf, prefixedSize));
		}
		else
		{
			// use two writes
			uint16_t	itemSize = inSize;
			OSERRIF(fStore->write(objId, offset, &itemSize, sizeof(itemSize)));
			OSERRIF(fStore->write(objId, offset + sizeof(itemSize), (void *)inData, inSize));
		}
	}

	fTable[index] = objId;
	return (index << 16) + offset;
}


bool
CStoreHashTable::get(StoreRef inRef, char * outData, size_t * ioSize)
{
	PSSId			objId = fTable[inRef >> 16];
	ArrayIndex	offset = inRef & 0xFFFF;
	uint16_t		objSize;
	OSERRIF(fStore->read(objId, offset, &objSize, sizeof(objSize)));
//printf("CStoreHashTable::get(objId=%d, offset=%d) objSize=%d\n", objId, offset, objSize);
	bool	isMyBufBigEnough = (*ioSize >= objSize);
	*ioSize = objSize;
	if (isMyBufBigEnough)
		OSERRIF(fStore->read(objId, offset + sizeof(objSize), outData, objSize));
	return isMyBufBigEnough;
}


void
CStoreHashTable::abort(void)
{
	OSERRIF(fStore->read(fId, 0, fTable, sizeof(fTable)));
}


size_t
CStoreHashTable::totalSize(void) const
{
	size_t size = 256;	// start off with the size of the id table itself
	for (ArrayIndex i = 0; i < kStoreHashTableSize; ++i)
	{
		size_t	objSize = 0;
		PSSId		objId = fTable[i];
		if (objId != 0)
			OSERRIF(fStore->getObjectSize(objId, &objSize));
		size += objSize;
	}
	return size;
}


#pragma mark -

/*------------------------------------------------------------------------------
	C S t o r e H a s h T a b l e I t e r a t o r
------------------------------------------------------------------------------*/

CStoreHashTableIterator::CStoreHashTableIterator(CStoreHashTable * inTable)
{
	fTable = inTable;
	fIndex = kIndexNotFound;
	fTableEntrySize = 0;
	fOffset = 0;
	fSize = 0;
	fDone = false;
	next();
}


void
CStoreHashTableIterator::next(void)
{
	if (!fDone)
	{
		fOffset += (sizeof(short) + fSize);
		while (fOffset >= fTableEntrySize)
		{
			fOffset = 0;
			if (fIndex == kIndexNotFound)
				fIndex = 0;
			else if (++fIndex == kStoreHashTableSize)
			{
				fDone = true;
				return;
			}
			fId = fTable->fTable[fIndex];
			if (fId != 0)
				OSERRIF(fTable->fStore->getObjectSize(fId, &fTableEntrySize));
			else
				fTableEntrySize = 0;
		}
		OSERRIF(fTable->fStore->read(fId, fOffset, &fSize, sizeof(short)));
	}
}


void
CStoreHashTableIterator::getData(char * outData, size_t * ioSize)
{
	size_t		reqSize = *ioSize;
	if (reqSize > fSize)
		reqSize = fSize;
	OSERRIF(fTable->fStore->read(fId, fOffset + sizeof(short), outData, reqSize));
	*ioSize = reqSize;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S t o r e W r a p p e r
------------------------------------------------------------------------------*/

CStoreWrapper::CStoreWrapper(CStore * inStore)
{
	fIsLocked = false;
	fStore = inStore;
	fMapTable = NULL;
	fSymbolTable = NULL;

	fMapCache = AllocateArray(kWeakArrayClass, kMapCacheSize);
	for (ArrayIndex i = 0; i < kMapCacheSize; ++i)
		fMapRefCache[i] = -1;
	fMapRefCacheIndex = 0;

	fSymbolCache = AllocateArray(kWeakArrayClass, kSymbolCacheSize);
	for (ArrayIndex i = 0; i < kSymbolCacheSize; ++i)
		fSymbolRefCache[i] = -1;
	fSymbolCacheIndex = 0;
	fCopyData = NULL;

	fTracker = NULL;
}

CStoreWrapper::~CStoreWrapper()
{
	if (fMapTable)
		delete fMapTable;
	if (fSymbolTable)
		delete fSymbolTable;
	if (fTracker)
		delete fTracker;
}

void
CStoreWrapper::dirty(void)
{
	AskForFlush(true);
	if (!fIsLocked)
	{
		fIsLocked = true;
		lockStore();
	}
}

void
CStoreWrapper::sparklingClean(void)
{
	if (fIsLocked)
	{
		fIsLocked = false;
		unlockStore();
	}
}

void
AskForFlush(bool inFlush)
{
	gAskedForFlush = inFlush;
}


StoreRef
CStoreWrapper::addMap(SortedMapTag * inMap, bool inSoupEntry, ArrayIndex * ioNumOfTags, ArrayIndex * outIndices)
{
	ArrayIndex	numOfTags = *ioNumOfTags;
	char *		mapBuf = (char *)(outIndices + numOfTags);
	ArrayIndex	bufLen = sizeof(short);
	ULong			totalHash = 0;
	*ioNumOfTags = 0;
	for (int i = numOfTags - 1; i >= 0; i--, inMap++)
	{
		Ref	tag = inMap->ref;
		ULong	symHash = SymbolHash(tag);
		if (!(symHash == k_protoHash && SymbolCompare(tag, SYMA(_proto)) == 0)									// don’t add _proto slot
		&&  (!inSoupEntry || !((symHash == k_uniqueIdHash && SymbolCompare(tag, SYMA(_uniqueId)) == 0)	// don’t add soup entry slots
									|| (symHash == k_modTimeHash && SymbolCompare(tag, SYMA(_modTime)) == 0))))
		{
			totalHash = (totalHash ^ symHash) >> *ioNumOfTags;	// should be ROR
			const char * symName = SymbolName(tag);
			size_t symLen = strlen(symName) + 1;
			memmove(mapBuf + bufLen, symName, symLen);
			bufLen += symLen;
			*outIndices++ = inMap->index;
			(*ioNumOfTags)++;
		}
	}
	*(short *)mapBuf = *ioNumOfTags;
	return fMapTable->insert(totalHash, mapBuf, bufLen);
}


StoreRef
CStoreWrapper::frameToMapReference(RefArg inFrame, bool inSoupEntry, ArrayIndex * ioNumOfTags, ArrayIndex ** outIndices)
{
	StoreRef			mapRef;
	ArrayIndex		bufSize;
	ArrayIndex		numOfTags = Length(inFrame);
	SortedMapTag	tagBuf[32];
	SortedMapTag *	tag, * tags;
	if (numOfTags > 32)
	{
		tags = new SortedMapTag[numOfTags];
		if (tags == NULL)
			OutOfMemory();
	}
	else
		tags = tagBuf;

	GetFrameMapTags(inFrame, tags, !IsFunction(inFrame) && !FrameHasSlot(inFrame, SYMA(_implementor)));
	bufSize = numOfTags;	// nul-terminators
	for (tag = tags; tag < tags + numOfTags; tag++)
		bufSize += strlen(SymbolName(tag->ref));
	*outIndices = new ArrayIndex[numOfTags + (bufSize+sizeof(ArrayIndex)+1)/sizeof(ArrayIndex)];
	if (*outIndices == NULL)
		OutOfMemory();

	mapRef = addMap(tags, inSoupEntry, &numOfTags, *outIndices);

	if (tags != tagBuf)
		delete tags;

	*ioNumOfTags = numOfTags;
	return mapRef;
}


Ref
CStoreWrapper::referenceToMap(StoreRef inRef)
{
	RefVar	frameMap;
	ArrayIndex	index = kIndexNotFound;
	for (ArrayIndex i = 0; i < kMapCacheSize; ++i)
	{
		if (inRef == fMapRefCache[i])
		{
			index = i;
			frameMap = GetArraySlot(fMapCache, index);
			break;
		}
	}
	if (ISNIL(frameMap))
	{
		// frame map is (short) number of tags followed by nul-terminated tag names
		size_t	bufLen = 1024;
		char		buf[1024];
		char *	mapBuf = buf;
		char *	tag;
		ArrayIndex	numOfTags;
		if (fMapTable->get(inRef, mapBuf, &bufLen) == 0)
		{
			// not enough room in our static buffer -- alloc a bigger one
			mapBuf = new char[bufLen];
			if (mapBuf == NULL)
				OutOfMemory();
			fMapTable->get(inRef, mapBuf, &bufLen);
		}
		numOfTags = *(short *)mapBuf;
		tag = mapBuf + sizeof(short);
		frameMap = MakeArray(numOfTags);
		for (ArrayIndex i = 0; i < numOfTags; ++i)
		{
			SetArraySlot(frameMap, i, MakeSymbol(tag));
			tag += (strlen(tag) + 1);
		}
		if (mapBuf != buf)
			delete[] mapBuf;

		frameMap = AllocateMapWithTags(RA(NILREF), frameMap);

		if (index == kIndexNotFound)
		{
			index = fMapRefCacheIndex;
			if (++fMapRefCacheIndex == kMapCacheSize)
				fMapRefCacheIndex = 0;
		}
		SetArraySlot(fMapCache, index, frameMap);
		fMapRefCache[index] = inRef;
	}

	return frameMap;
}


StoreRef
CStoreWrapper::symbolToReference(RefArg inSymbol)
{
	const char * symName = SymbolName(inSymbol);
	return fSymbolTable->insert(SymbolHash(inSymbol), symName, strlen(symName));
}


Ref
CStoreWrapper::referenceToSymbol(StoreRef inRef)
{
	RefVar	sym;
	ArrayIndex	index = kIndexNotFound;
	for (ArrayIndex i = 0; i < kSymbolCacheSize; ++i)
	{
		if (fSymbolRefCache[i] == inRef)
		{
			index = i;
			sym = GetArraySlot(fSymbolCache, index);
			break;
		}
	}
	if (ISNIL(sym))
	{
		size_t	symLen = 256;
		char		symName[256];
		fSymbolTable->get(inRef, symName, &symLen);
		symName[symLen] = 0;
		sym = MakeSymbol(symName);
		if (index == kIndexNotFound)
		{
			index = fSymbolCacheIndex;
			if (++fSymbolCacheIndex == kSymbolCacheSize)
				fSymbolCacheIndex = 0;
		}
		SetArraySlot(fSymbolCache, index, sym);
		fSymbolRefCache[index] = inRef;
	}

	return sym;
}


void
CStoreWrapper::startCopyMaps_Symbols(void)
{
	fCopyData = new CopyData;
	if (fCopyData == NULL)
		OutOfMemory();

	for (ArrayIndex i = 0; i < 16; ++i)
		fCopyData->frameMapping[i].remote = -1;
	for (ArrayIndex i = 0; i < 32; ++i)
		fCopyData->symbolMapping[i].remote = -1;
	fCopyData->symbolIndex = 0;
}


void
CStoreWrapper::endCopyMaps_Symbols(void)
{
	if (fCopyData != NULL)
	{
		delete fCopyData;
		fCopyData = NULL;
	}
}


StoreRef
CStoreWrapper::copyMap(StoreRef inRef, CStoreWrapper * inOtherStore, ArrayIndex * outNumOfTags)
{
	ArrayIndex	index = kIndexNotFound;
	StoreRef	mapRef;
	MapMap *	p = fCopyData->frameMapping;
	for (ArrayIndex i = 0; i < 16; ++i, p++)
	{
		StoreRef	ref = p->remote;
		if (ref == inRef)
		{
			*outNumOfTags = p->numOfTags;
			return p->local;
		}
		if (ref == -1)
		{
			index = i;
			break;
		}
	}

	// frame map is (short) number of tags followed by nul-terminated tag names
	size_t	bufLen = 1024;
	char		buf[1024];
	char *	mapBuf = buf;
	const char *	tag;
	ULong		totalHash;
	if (inOtherStore->fMapTable->get(inRef, mapBuf, &bufLen) == 0)
	{
		// not enough room in our static buffer -- alloc a bigger one
		mapBuf = new char[bufLen];
		if (mapBuf == NULL)
			OutOfMemory();
		inOtherStore->fMapTable->get(inRef, mapBuf, &bufLen);
	}
	*outNumOfTags = *(short *)mapBuf;
	tag = mapBuf + sizeof(short);
	totalHash = 0;
	for (ArrayIndex i = 0; i < *outNumOfTags; ++i)
	{
		totalHash = (totalHash ^ SymbolHashFunction(tag)) >> i;	// should be ROR
		tag += (strlen(tag) + 1);
	}
	mapRef = fMapTable->insert(totalHash, mapBuf, bufLen);

	if (mapBuf != buf)
		delete[] mapBuf;
	if (index != kIndexNotFound)
	{
		p->remote = inRef;
		p->local = mapRef;
		p->numOfTags = *outNumOfTags;
	}

	return mapRef;
}


StoreRef
CStoreWrapper::copySymbol(StoreRef inRef, CStoreWrapper * inOtherStore)
{
	StoreRef	symRef;
	SymbolMap *	p = fCopyData->symbolMapping;
	for (ArrayIndex i = 0; i < 32; ++i, ++p)
	{
		StoreRef	ref = p->remote;
		if (ref == inRef)
			return p->local;
		if (ref == -1)
			break;
	}

	symRef = symbolToReference(inOtherStore->referenceToSymbol(inRef));
	p = fCopyData->symbolMapping + fCopyData->symbolIndex;
	p->remote = inRef;
	p->local = symRef;
	if (++fCopyData->symbolIndex == 32)
		fCopyData->symbolIndex = 0;

	return symRef;
}


NewtonErr
CStoreWrapper::getStoreSizes(size_t * outUsedSize, size_t * outTotalSize)
{
	GC();
	return store()->getStoreSize(outTotalSize, outUsedSize);
}


NewtonErr
CStoreWrapper::lockStore(void)
{
	if (fTracker)
		fTracker->lockEphemerals();

	return store()->lockStore();
}


NewtonErr
CStoreWrapper::unlockStore(void)
{
	if (fTracker)
		fTracker->flushEphemerals();

	NewtonErr	err = store()->unlockStore();

	if (fTracker
	&&  !fTracker->f18->isEmpty()
	&&  !store()->isLocked())
		 fTracker->deletePendingEphemerals();

	return err;
}


NewtonErr
CStoreWrapper::abort(void)
{
	NewtonErr	err;

	if (fTracker)
		fTracker->flushEphemerals();
	fNodeCache.clear();
	err = fStore->abort();
	fMapTable->abort();
	fSymbolTable->abort();

	return err;
}


#pragma mark -

/*------------------------------------------------------------------------------
	C E p h e m e r a l T r a c k e r
------------------------------------------------------------------------------*/
extern bool CanCreateLargeObjectsOnStore(CStore * inStore);

NewtonErr
EnsureStoreHasEphemeralTracker(RefArg inStoreObject, PSSId inEphemeralListId)
{ return noErr; }


NewtonErr
SetupEphemeralTracker(RefArg inStoreObject, PSSId inEphemeralListId)
{
	NewtonErr			err = noErr;
	CStoreWrapper *	storeWrapper = (CStoreWrapper *)GetFrameSlot(inStoreObject, SYMA(store));
	RefVar				proto(GetFrameSlot(inStoreObject, SYMA(_proto)));
	if (CanCreateLargeObjectsOnStore(storeWrapper->store()))
	{
		RefVar	ephemerals(GetFrameSlot(proto, SYMA(ephemerals)));
		if (ISNIL(ephemerals))
		{
#if 0
			if ((err = EnsureStoreHasEphemeralTracker(inStoreObject, inEphemeralListId)) == noErr)
				ephemerals = GetFrameSlot(proto, SYMephemerals);
#else
			ephemerals = MAKEINT(1023);
#endif
		}
		CEphemeralTracker *	tracker = new CEphemeralTracker;
		tracker->init(storeWrapper, RINT(ephemerals));
		storeWrapper->setEphemeralTracker(tracker);
		tracker->deleteAllEphemerals();
	}
	else
		err = kNSErrWrongStoreVersion;
	return err;
}


CEphemeralTracker::CEphemeralTracker()
{
	f0C = NULL;
	f14 = NULL;
	f18 = NULL;
	f10 = NULL;
}

CEphemeralTracker::~CEphemeralTracker()
{
	if (f0C != NULL)
		delete f0C;
	if (f14 != NULL)
		delete f14;
	if (f18 != NULL)
		delete f18;
	if (f10 != NULL)
		delete f10;
}

NewtonErr
CEphemeralTracker::init(CStoreWrapper * inStoreWrapper, PSSId inObjectId)
{
	fStoreWrapper = inStoreWrapper;
	fStore = inStoreWrapper->store();
	fId = inObjectId;
	f0C = new CDynamicArray;
	f14 = new CDynamicArray;
	f18 = new CDynamicArray;
	f10 = new CDynamicArray;
	return noErr;
}

ArrayIndex
CEphemeralTracker::find(PSSId inId, CDynamicArray * inArray)
{
	ArrayIndex count = inArray->count();
	for (ArrayIndex i = 0; i < count; ++i)
	{
		if (*(PSSId *)inArray->safeElementPtrAt(i) == inId)
			return i;
	}
	return kIndexNotFound;
}

bool
CEphemeralTracker::findAndRemove(PSSId inId, CDynamicArray * inArray)
{
	ArrayIndex i;
	if ((i = find(inId, inArray)) != kIndexNotFound)
	{
		inArray->removeElementsAt(i, 1);
		return true;
	}
	return false;
}

void
CEphemeralTracker::addEphemeral(PSSId inId)
{
	if (CEphemeralTracker::find(inId, f0C) != kIndexNotFound)
		f0C->addElement(&inId);
}

bool
CEphemeralTracker::isEphemeral(PSSId inId)
{
	return CEphemeralTracker::find(inId, f0C) != kIndexNotFound
		 || CEphemeralTracker::find(inId, f14) != kIndexNotFound
		 || CEphemeralTracker::find(inId, f18) != kIndexNotFound;
}

void
CEphemeralTracker::removeEphemeral(PSSId inId)
{
	if (CEphemeralTracker::findAndRemove(inId, f0C)
	||  CEphemeralTracker::findAndRemove(inId, f14))
		f10->addElement(&inId);
}

NewtonErr
CEphemeralTracker::deleteEphemeral(PSSId inId)
{
	return deleteEphemeral1(inId);
}

NewtonErr
CEphemeralTracker::deleteEphemeral1(PSSId inId)
{
	NewtonErr	err = noErr;

	if (fStore->inTransaction()
	||  !StoreWritable(fStore))
	{
		f18->addElement(&inId);
		if (!CEphemeralTracker::findAndRemove(inId, f0C))
			CEphemeralTracker::findAndRemove(inId, f14);
	}
	else
	{
		fStore->lockStore();
		if ((err = DeleteLargeObject(fStore, inId)) == kStoreErrObjectNotFound)
			fStore->abort();
		else
		{
			if (!CEphemeralTracker::findAndRemove(inId, f0C)
			&&  !CEphemeralTracker::findAndRemove(inId, f14))
				CEphemeralTracker::findAndRemove(inId, f18);
			err = writeEphemeralList();
		}
		fStore->unlockStore();
	}

	return err;
}

NewtonErr
CEphemeralTracker::deleteAllEphemerals(void)
{
	NewtonErr	err = noErr;
	while (f14->count() > 0)
	{
		if ((err = deleteEphemeral1(*(PSSId *)f14->safeElementPtrAt(0))) != noErr)
			break;
	}
	return err;
}

NewtonErr
CEphemeralTracker::deletePendingEphemerals(void)
{
	NewtonErr	err = noErr;
	if (StoreWritable(fStore))
		while (f14->count() > 0)
		{
			if ((err = deleteEphemeral1(*(PSSId *)f18->safeElementPtrAt(0))) != noErr)
				break;
		}
	return err;
}

NewtonErr
CEphemeralTracker::readEphemeralList(void)
{
	size_t	size;
	fStore->getObjectSize(fId, &size);
	f14->setElementCount(size / sizeof(PSSId));
	return fStore->read(fId, 0, (char *)f10, size);
}

NewtonErr
CEphemeralTracker::writeEphemeralList(void)
{
	return fStore->replaceObject(fId, f10, f14->count() * sizeof(PSSId));
}

void
CEphemeralTracker::lockEphemerals(void)
{
	if (!fStore->isLocked())
	{
		if (f18->count() > 0)
			deletePendingEphemerals();
		f0C->removeAll();
		f10->removeAll();
	}
}

void
CEphemeralTracker::flushEphemerals(void)
{
	if (f0C->count() > 0
	||  f10->count() > 0)
	{
		NewtonErr	err;
		f14->merge(f0C);
		err = writeEphemeralList();
		f0C->removeAll();
		if (err != noErr)
		{
			abortEphemerals();
			ThrowOSErr(err);
		}
		f10->removeAll();
	}
}

void
CEphemeralTracker::abortEphemerals(void)
{
	f0C->removeAll();
	readEphemeralList();
	for (ArrayIndex i = 0; i < f10->count(); ++i)
		BreakLargeObjectToEntryLink(*(PSSId *)f10->safeElementPtrAt(i), fStoreWrapper);
	f10->removeAll();
}
