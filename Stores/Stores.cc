/*
	File:		Stores.cc

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Arrays.h"
#include "ROMResources.h"
#include "LargeBinaries.h"
#include "PrecedentsForIO.h"
#include "OSErrors.h"
#include "FaultBlocks.h"
#include "Cursors.h"
#include "SerialNumber.h"
#include "NameServer.h"

#define kStoreVersion	4


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

CStore *	gInRAMStore;		// 0C1016C4
CStore *	gMuxInRAMStore;	// 0C1016C8
CStore *	gCardStore;			// 0C1016CC
CStore *	gMuxCardStore;		// 0C1016D0

Ref		gStores;				// 0C10596C	stores [array]
bool		gNeedsInternalSignatures;	// 0C105970
char *	g0C105974;			// 0C105974	rand_state?
bool		gAskedForFlush;	// 0C105978
bool		gTraceStore;		// 0C10597C


/*------------------------------------------------------------------------------
	U n i o n   S o u p s
------------------------------------------------------------------------------*/

extern Ref		gUnionSoups;			// soup entry cache [weak array]

extern void		AddToUnionSoup(RefArg name, RefArg soup);
extern void		RemoveFromUnionSoup(RefArg name, RefArg soup);


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/
extern size_t sizeof_rand_state(void);
extern void set_rand_state(void * ioState);
extern void get_rand_state(void * ioState);

extern void		InitPackageSoups(void);
extern void		InitHintsHandlers(void);
void		InitExternalFunctions(void);

Ref		RegisterStore(CStore * inStore);
void		RemoveStore(CStore * inStore);

NewtonErr		ReadStoreRootData(CStore * inStore, PSSId inId, StoreRootData * outRootData, size_t * outSize);
extern void		LargeBinariesStoreRemoved(CStoreWrapper * inStoreWrapper);
extern NewtonErr	SetupEphemeralTracker(RefArg inStoreObject, PSSId inEphemeralListId);
extern CStore *	StoreFromWrapper(RefArg inRcvr);


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C"
{
Ref	FGetCardInfo(RefArg inRcvr);
Ref	FGetStores(RefArg inRcvr);
}


/*------------------------------------------------------------------------------
	I n i t i a l i z a t i o n

	InitQueries() creates an empty gStores array.
	InitExternal() adds the internal store to gStores.
	All this is called from CNewtWorld::mainConstructor().
------------------------------------------------------------------------------*/

void
InitQueries(void)
{
	gStores = MakeArray(0);
	AddGCRoot(&gStores);

	gUnionSoups = MakeEntryCache();
	AddGCRoot(&gUnionSoups);

	InitPackageSoups();
}


void
InitExternal(void)
{
	gPrecedentsForWriting = new CPrecedentsForWriting;
	gPrecedentsForReading = new CPrecedentsForReading;

	InitHintsHandlers();

#if !defined(forFramework)
	OpaqueRef		thing, spec;
	CUNameServer	ns;
	ns.lookup("InRAMStore", kRegisteredStore, &thing, &spec);
	RegisterStore((CStore *)thing);
#else
	RegisterStore(gMuxInRAMStore);
#endif

	InitLargeObjects();
	InitExternalFunctions();
}


void
InitExternalFunctions(void)
{ /* this really does nothing */ }


/*------------------------------------------------------------------------------
	Register a Newton store.
	Args:		inStore		the store to be registered
	Return:	a store object
------------------------------------------------------------------------------*/

Ref
RegisterStore(CStore * inStore)
{
	RefVar	store(MakeStoreObject(inStore));
	AddArraySlot(gStores, store);

	newton_try
	{
		RefVar	theSoup;
		RefVar	soupName;
		RefVar	soupObject;
		for (int i = Length(gUnionSoups) - 1; i >= 0; i--)
		{
			theSoup = GetArraySlot(gUnionSoups, i);
			if (NOTNIL(theSoup))
			{
				soupName = GetFrameSlot(theSoup, SYMA(theName));
				if (NOTNIL(soupName))
				{
					soupObject = StoreGetSoup(store, soupName);
					if (NOTNIL(soupObject))
						AddToUnionSoup(theSoup, soupObject);
				}
			}
		}
	}
	cleanup
	{
		RemoveStore(inStore);
	}
	end_try;

	return store;
}


/*------------------------------------------------------------------------------
	S o r t   T a b l e s
------------------------------------------------------------------------------*/

Ref	StoreConvertSoupSortTables(void) { return NILREF; }
const CSortingTable *	StoreGetDirSortTable(RefArg) { return NULL; }
bool	StoreHasSortTables(RefArg) { return false; }
void	StoreSaveSortTable(RefArg, long) {}
void	StoreRemoveSortTable(RefArg, long) {}

/*------------------------------------------------------------------------------
	Remove a Newton store.
	Args:		inStore		the store to be removed
	Return:	--
------------------------------------------------------------------------------*/

static void
AddSortTables(RefArg inStore)
{
	RefVar	sortTables(GetFrameSlot(GetFrameSlot(inStore, SYMA(_proto)), SYMA(sortTables)));
	if (NOTNIL(sortTables))
	{
		ArrayIndex	i, count = Length(sortTables);
		for (i = 0; i < count; i += 3)
		{
			int	sortTableNumber = RINT(GetArraySlot(sortTables, i));
/*			if (gSortTables->getSortTable(sortTableNumber) == 0)
			{
				CStoreWrapper *	storeWrapper = (CStoreWrapper *)GetFrameSlot(inStore, SYMA(store));	// r10
				ULong	sortTableId = RINT(GetArraySlot(sortTables, i+2));
				long	size;
				OSERRIF(storeWrapper->getObjectSize(sortTableId, &size));
				CSortingTable * sortTable = (CSortingTable *)NewPtr(size);
				if (sortTable == NULL)
					OutOfMemory();
				OSERRIF([storeWrapper->store() read: sortTableId buf: sortTable offset: 0 size: size]);
				gSortTables->add(sortTable, true);
			}
			else
				gSortTables->subscribe(sortTableNumber);
*/		}
	}
}

static void
RemoveSortTables(RefArg inStore)
{
	RefVar	sortTables(GetFrameSlot(GetFrameSlot(inStore, SYMA(_proto)), SYMA(sortTables)));
	if (NOTNIL(sortTables))
	{
		ArrayIndex	i, count = Length(sortTables);
		for (i = 0; i < count; i += 3)
		{
			int	sortTableNumber = RINT(GetArraySlot(sortTables, i));
//			gSortTables->unsubscribe(sortTableNumber);
		}
	}
}

void
RemoveStore(CStore * inStore)
{
	RefVar				storeObject;
	CStoreWrapper *	storeWrapper = NULL;
	for (ArrayIndex i = 0, count = Length(gStores); i < count; ++i)
	{
		storeObject = GetArraySlot(gStores, i);
		storeWrapper = (CStoreWrapper *)GetFrameSlot(storeObject, SYMA(store));
		if (storeWrapper->store() == inStore)
		{
			ArrayMunger(gStores, i, 1, RA(NILREF), 0, 0);
			RemoveSortTables(storeObject);
			KillStoreObject(storeObject);
			delete storeWrapper;
			return;
		}
	}
	ThrowOSErr(kNSErrStoreNotRegistered);
}


/*------------------------------------------------------------------------------
	Return store information.
------------------------------------------------------------------------------*/


CStore *
GetInternalStore(void)
{
	return gMuxInRAMStore;
}

void
SetupRandomSignature()
{
	if (g0C105974 == NULL)
	{
		g0C105974 = new char[sizeof_rand_state()];
		if (g0C105974 == NULL)
			OutOfMemory();
		get_rand_state(g0C105974);
	}
}

long
GetRandomSignature(void)
{
	if (g0C105974 == NULL)
		return rand();

	long		signature;
	size_t	randStateSize = sizeof_rand_state();
	char		randState[4], * randStatePtr;
	if (randStateSize >= sizeof(randState))
	{
		randStatePtr = new char[randStateSize];
		if (randStatePtr == NULL)
			OutOfMemory();
	}
	else
		randStatePtr = randState;
	get_rand_state(randStatePtr);
	set_rand_state(g0C105974);
	signature = rand();
	get_rand_state(g0C105974);
	set_rand_state(randStatePtr);
	if (randStatePtr != randState)
		delete[] randStatePtr;
	return signature;
}


NewtonErr
ReadStoreRootData(CStore * inStore, PSSId inId, StoreRootData * outRootData, size_t * outSize)
{
	NewtonErr	err;
	size_t		dummySize;
	if (outSize == NULL)
		outSize = &dummySize;
	*outSize = 0xFFFFFFFF;

	XTRY
	{
		if (inId == 0)
			XFAIL(err = inStore->getRootId(&inId))
		XFAIL(err = inStore->getObjectSize(inId, outSize))
		XFAILIF(*outSize < 20, err = kNSErrNotAFrameStore;)
		if (*outSize >= 24)
			*outSize = 24;
		else
			outRootData->ephemeralsId = 0;
		err = inStore->read(inId, 0, outRootData, *outSize);
	}
	XENDTRY;

	return err;
}


Ref
MakeStoreObject(CStore * inStore)
{
	long				signature;

	SetupRandomSignature();
	CStoreWrapper *	storeWrapper = new CStoreWrapper(inStore);
	if (storeWrapper == NULL)
		OutOfMemory();

	RefVar	storeObject;
	bool		isStoreNew = false;

	newton_try
	{
		RefVar			persistentStoreInfo;
		StoreRootData	storeRoot;
		PSSId				rootId;
		size_t			size;

		OSERRIF(storeWrapper->store()->getRootId(&rootId));
		OSERRIF(storeWrapper->store()->getObjectSize(rootId, &size));
		if (size == 0)
		{
			// store is empty - create default info
			isStoreNew = true;
			CheckWriteProtect(storeWrapper->store());
			OSERRIF(storeWrapper->lockStore());

			newton_try
			{
				storeRoot.signature = 'WALY';
				storeRoot.version = kStoreVersion;

				persistentStoreInfo = Clone(RA(storePersistent));

				// set up index for soup names on this store
				IndexInfo	indexInfo;
				indexInfo.keyType = kKeyTypeString;
				indexInfo.dataType = kDataTypeNormal;
				indexInfo.x10 = 0;
				indexInfo.x19 = 0;
				indexInfo.x14 = 0xFFFFFFFF;
				indexInfo.x18 = 0xFF;
				SetFrameSlot(persistentStoreInfo, SYMA(nameIndex), MAKEINT(CSoupIndex::create(storeWrapper, &indexInfo)));

				if (DefaultSortTableId() != 0)
					SetFrameSlot(persistentStoreInfo, SYMA(dirSortId), MAKEINT(DefaultSortTableId()));

				SetFrameSlot(persistentStoreInfo, SYMA(name), MakeStringFromCString("Untitled"));

				if (inStore == GetInternalStore())
				{
					SerialNumber serialNumber;
					NewtonErr err = GetSerialNumberROMObject()->getSystemSerialNumber(&serialNumber);
					signature = (err == noErr) ? serialNumber.word[1] : 0;
				}
				else
					signature = GetRandomSignature();
				SetFrameSlot(persistentStoreInfo, SYMA(signature), MAKEINT(signature));

				storeRoot.mapTableId = CStoreHashTable::create(storeWrapper->store());
				if ((storeWrapper->fMapTable = new CStoreHashTable(storeWrapper->store(), storeRoot.mapTableId)) == NULL)
					OutOfMemory();
				storeRoot.symbolTableId = CStoreHashTable::create(storeWrapper->store());
				if ((storeWrapper->fSymbolTable = new CStoreHashTable(storeWrapper->store(), storeRoot.symbolTableId)) == NULL)
					OutOfMemory();

				storeRoot.permId = kNoPSSId;
				StorePermObject(persistentStoreInfo, storeWrapper, storeRoot.permId, NULL, NULL);
				OSERRIF(storeWrapper->store()->replaceObject(rootId, (char *)&storeRoot, sizeof(storeRoot)));
				OSERRIF(storeWrapper->unlockStore());
			}
			cleanup
			{
				OSERRIF(storeWrapper->abort());
			}
			end_try;
		}
		else
		{
			// we already have this store - read in its root data
			OSERRIF(ReadStoreRootData(storeWrapper->store(), rootId, &storeRoot, NULL));
			if (storeRoot.signature != 'WALY')
				ThrowOSErr(kNSErrNotAFrameStore);
			if (storeRoot.version > kStoreVersion)
				ThrowOSErr(kNSErrNewStoreFormat);
			if ((storeWrapper->fMapTable = new CStoreHashTable(storeWrapper->store(), storeRoot.mapTableId)) == NULL)
				OutOfMemory();
			if ((storeWrapper->fSymbolTable = new CStoreHashTable(storeWrapper->store(), storeRoot.symbolTableId)) == NULL)
				OutOfMemory();
			persistentStoreInfo = LoadPermObject(storeWrapper, storeRoot.permId, NULL);
		}
		storeObject = Clone(RA(storePrototype));
		SetFrameSlot(storeObject, SYMA(_proto), MakeFaultBlock(RA(NILREF), storeWrapper, storeRoot.permId, persistentStoreInfo));
		SetFrameSlot(storeObject, SYMA(store), (Ref) storeWrapper);
		SetFrameSlot(storeObject, SYMA(soups), MakeEntryCache());
		SetFrameSlot(storeObject, SYMA(version), MAKEINT(kStoreVersion));
		if (isStoreNew)
			StoreSaveSortTable(storeObject, DefaultSortTableId());
		else
			AddSortTables(storeObject);
		SetupEphemeralTracker(storeObject, storeRoot.permId);
	}
	cleanup
	{
		if (storeWrapper != NULL)
			storeWrapper->~CStoreWrapper();
	}
	end_try;

	return storeObject;
}


NewtonErr
SetStoreVersion(RefArg ioStoreObject, ULong inVersion)
{
	NewtonErr		err;
	StoreRootData	storeRoot;
	PSSId				rootId;
	size_t			size;
	CStore *			store = StoreFromWrapper(ioStoreObject);
	XTRY
	{
		XFAIL(err = store->getRootId(&rootId))
		XFAIL(err = ReadStoreRootData(store, rootId, &storeRoot, &size))
		if (storeRoot.version != inVersion)
		{
			storeRoot.version = inVersion;
			XFAIL(err = store->write(rootId, 0, (char *)&storeRoot, size))
		}
		SetFrameSlot(ioStoreObject, SYMA(version), MAKEINT(inVersion));
	}
	XENDTRY;
	return err;
}


NewtonErr
GetStoreVersion(CStore * inStore, ULong * outVersion)
{
	NewtonErr		err;
	StoreRootData	storeRoot;
	size_t			size;
	err = ReadStoreRootData(inStore, 0, &storeRoot, &size);
	if (size == 0)
	{
		*outVersion = 4;
		err = noErr;
	}
	else
		*outVersion = storeRoot.version;
	return err;
}


void
KillStoreObject(RefArg inStoreObject)
{
	SetFrameSlot(inStoreObject, SYMA(_proto), RA(NILREF));

	RefVar soups(GetFrameSlot(inStoreObject, SYMA(soups)));
	for (ArrayIndex i = 0, count = Length(soups); i < count; ++i)
	{
		RefVar	soup(GetArraySlot(soups, i));
		if (NOTNIL(soup))
		{
			RemoveFromUnionSoup(GetFrameSlot(soup, SYMA(theName)), soup);
			SetFrameSlot(soup, SYMA(_proto), RA(NILREF));
			InvalidateCacheEntries(GetFrameSlot(soup, SYMA(cache)));
			EachSoupCursorDo(soup, kSoupRemoved, soup);
			SetArraySlot(soups, i, RA(NILREF));
		}
	}
	LargeBinariesStoreRemoved((CStoreWrapper *)GetFrameSlot(inStoreObject, SYMA(store)));
}


#pragma mark -

/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Return array of all known stores.
	Args:		inRcvr
	Return:	array Ref
----------------------------------------------------------------------------- */

Ref
GetStores(void)
{
	return gStores;
}


Ref
FGetStores(RefArg inRcvr)
{
	return GetStores();
}


/* -----------------------------------------------------------------------------
	Erase a store.
	Args:		inRcvr		the store
	Return:	erased store frame Ref
----------------------------------------------------------------------------- */
extern CStoreWrapper * StoreWrapper(RefArg inRcvr);

Ref
StoreErase(RefArg inRcvr)
{
	NewtonErr err;
	CStoreWrapper * storeWrapper = StoreWrapper(inRcvr);
	CheckWriteProtect(inRcvr);

	// remember where we are in the global list of stores
	ArrayIndex originalStoreIndex = ArrayPosition(gStores, inRcvr, 0, RA(NILREF));
	CStore * store = storeWrapper->store();
	// remove the store from the global list of stores
	RemoveStore(store);
	// reformat it
	err = store->format();
	if (err)
		ThrowOSErr(err);
	// reregister it
	RefVar storeRef(RegisterStore(store));
	ArrayIndex storeIndex;
	if (originalStoreIndex != kIndexNotFound && (storeIndex = ArrayPosition(gStores, storeRef, 0, RA(NILREF))) != kIndexNotFound)
	{
		if (storeIndex != originalStoreIndex)
		{
			// the store is not where it used to be in the global list -- put it back where it was
			SetArraySlotRef(gStores, storeIndex, GetArraySlotRef(gStores, originalStoreIndex));
			SetArraySlotRef(gStores, originalStoreIndex, storeRef);
		}
	}
	return storeRef;
}


/* -----------------------------------------------------------------------------
	Return info for PCMCIA card.

	canonicalCardInfo := {	cardInfoVersion: 0,
									totalSockets: 0,
									totalCards: 0,
									socketInfos: [] };

	Args:		inRcvr
	Return:	info frame Ref
----------------------------------------------------------------------------- */

Ref
FGetCardInfo(RefArg inRcvr)
{
#if defined(correct)
	//sp-64
	ArrayIndex numOfCards = 0;	// sp2C
	RefVar sp28;
	RefVar sp24;
	RefVar sp20;
	RefVar sp1C;
	RefVar sp18;
	RefVar sp14;
	RefVar sp10;
	RefVar sp0C;
	RefVar sp08;
	RefVar sp04;
	int sp00 = 0;

	RefVar cardInfo(DeepClone(RA(canonicalCardInfo)));
	r4 = 0;
	if (NOTNIL(cardInfo))
	{
		//sp-04
		SetFrameSlot(cardInfo, SYMA(cardInfoVersion), MAKEINT(0x00020200));
		sp20 = GetFrameSlot(cardInfo, SYMsocketInfos);
		//sp-08
		sp60 = gCardServer;
		for (ArrayIndex i = 0; i < sp60->f38; ++i)		// sp00
		{
			;
		}
		SetFrameSlot(cardInfo, SYMA(totalSockets), MAKEINT(sp60->f38));
		SetFrameSlot(cardInfo, SYMA(totalCards), MAKEINT(numOfCards));
	}
#else
	RefVar cardInfo(DeepClone(RA(canonicalCardInfo)));
#endif
	return cardInfo;
}


#pragma mark -


Ref
ToObject(CStore * inStore)
{
	RefVar storeObject;
	CStoreWrapper * storeWrapper;
	for (ArrayIndex i = 0, count = Length(gStores); i < count; ++i)
	{
		storeObject = GetArraySlot(gStores, i);
		storeWrapper = (CStoreWrapper *)GetFrameSlot(storeObject, SYMA(store));
		if (storeWrapper->store() == inStore)
			return storeObject;
	}
	return NILREF;
}


void
CheckWriteProtect(CStore * inStore)
{
	bool isRO;
	if (inStore->isROM())
		ThrowOSErr(kNSErrStoreIsROM);
	OSERRIF(inStore->isReadOnly(&isRO));
	if (isRO)
		ThrowOSErr(kStoreErrWriteProtected);
}


bool
StoreWritable(CStore * inStore)
{
	bool isRO;
	if (!(isRO = inStore->isROM()))
		inStore->isReadOnly(&isRO);
	return !isRO;
}

