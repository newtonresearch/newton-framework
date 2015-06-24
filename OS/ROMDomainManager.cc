/*
	File:		ROMDomainManager.cc

	Contains:	ROM Domain Manager implementation.
					The ROM Domain Manager manages (large) objects on a store.

	Written by:	Newton Research Group.
*/

#include "Environment.h"
#include "RDM.h"
#include "MemObjManager.h"
#include "PageManager.h"
#include "MemMgr.h"
#include "ListIterator.h"
#include "ItemComparer.h"
#include "CachedReadStore.h"
#include "LargeObjectStore.h"
#include "RAMInfo.h"
#include "SWI.h"
#include "OSErrors.h"

extern bool		IsValidStore(const CStore * inStore);
extern const CClassInfo * GetStoreClassInfo(const CStore * inStore);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/
#include "PSSManager.h"
extern SCardReinsertionTracker	gCardReinsertionTracker;

CROMDomainManager1K *	gROMStoreDomainManager;
size_t						gRDMNumberOfFaults;


/*------------------------------------------------------------------------------
	ROM domain manager parameters for communication with the monitor.
------------------------------------------------------------------------------*/

RDMParams::RDMParams()
{
	fStore = NULL;
	fObjId = 0;
	fAddr = 0;
	fPkgId = 0;
	fRO = true;
	f14 = -1;
	fDirty = false;
}

#pragma mark -

/*------------------------------------------------------------------------------
	L O T r a n s a c t i o n H a n d l e r
------------------------------------------------------------------------------*/
static CItemComparer	fgTxnComparer;
static bool				fgIsTxnComparerInited = false;
inline void * operator new(size_t, CItemComparer * _ic) { return _ic; }

LOTransactionHandler::LOTransactionHandler()
{
	fList = NULL;
	f04 = false;
	f05 = false;
}


void
LOTransactionHandler::free(void)
{
	if (fList)
		delete fList;
}


NewtonErr
LOTransactionHandler::addObjectToTransaction(PSSId inId, CStore * inStore, int inTxnType)
{
	NewtonErr	err = noErr;

	XTRY
	{
		if (inTxnType == 3)
			f05 = true;
		else if (f04)
			break;

		if (fList == NULL)
		{
			// create list for object ids within the transaction
			if (!fgIsTxnComparerInited)
			{
				new (&fgTxnComparer) CItemComparer;
				fgIsTxnComparerInited = true;
			}
			fList = new CSortedList(&fgTxnComparer);
			XFAILIF(err = MemError(), fList = NULL;)
		}

		// add object id to our list
		if (fList->insertUnique((void *)(OpaqueRef)inId)
		&& inTxnType == 1)
			err = inStore->startTransactionAgainst(inId);
	}
	XENDTRY;
	
	return err;
}


void
LOTransactionHandler::setAllInTransaction(void)
{
	f04 = true;
}


bool
LOTransactionHandler::hasTransaction(void)
{
	return f04
		 || (fList && fList->count() > 0);
}


NewtonErr
LOTransactionHandler::endTransaction(bool inArg1, CStore * inStore, PSSId inObjId, PSSId inTableId)
{
	NewtonErr	err = noErr;

	XTRY
	{
		if (fList)
		{
			if (!f04 || f05)
			{
				PSSId				objId;
				CListIterator	iter(fList);
				for (objId = (PSSId)(OpaqueRef)iter.firstItem(); iter.more(); objId = (PSSId)(OpaqueRef)iter.nextItem())
				{
					if (inArg1)
						XFAIL(err = inStore->addToCurrentTransaction(objId))	// will not go to XENDTRY!
					else
						XFAIL(err = inStore->separatelyAbort(objId))		// will not go to XENDTRY!
				}
			}
			delete fList, fList = NULL;
		}
		f05 = false;
		if (f04)
		{
			f04 = false;
			if (err == noErr)
				err = endAllObjectsTransaction(inArg1, inStore, inObjId, inTableId);
		}
	}
	XENDTRY;
	
	return err;
}


NewtonErr
LOTransactionHandler::endAllObjectsTransaction(bool inArg1, CStore * inStore, PSSId inObjId, PSSId inTableId)
{
	NewtonErr	err;
	PackageRoot	root;

	XTRY
	{
		XFAIL(err = inStore->read(inObjId, 0, &root, sizeof(root)))
		if (inTableId == 0)
			inTableId = root.fDataId;
		XFAIL(err = endIndexTableTransaction(inArg1, inStore, inTableId))
		if (inArg1)
		{
			XFAIL(err = inStore->addToCurrentTransaction(inObjId))
			XFAIL(err = inStore->addToCurrentTransaction(root.fCompanderNameId))
			if (root.fCompanderParmsId != 0)
				XFAIL(err = inStore->addToCurrentTransaction(root.fCompanderParmsId))
			err = inStore->addToCurrentTransaction(root.fDataId);
		}
		else
		{
			XFAIL(err = inStore->separatelyAbort(inObjId))
			XFAIL(err = inStore->separatelyAbort(root.fCompanderNameId))
			if (root.fCompanderParmsId != 0)
				XFAIL(err = inStore->separatelyAbort(root.fCompanderParmsId))
			err = inStore->separatelyAbort(root.fDataId);
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
LOTransactionHandler::endIndexTableTransaction(bool inArg1, CStore * inStore, PSSId inTableId)
{
	NewtonErr	err = noErr;

	XTRY
	{
		CCachedReadStore * cachedStore;
		XFAILNOT(cachedStore = new CCachedReadStore(inStore, inTableId, kUseObjectSize), err = kOSErrNoMemory;)
		PSSId *	objId;
		for (ArrayIndex i = 0, count = cachedStore->getDataSize() / sizeof(PSSId); i < count; ++i)
		{
			XFAIL(err = cachedStore->getDataPtr(i*sizeof(PSSId), sizeof(PSSId), (void**)&objId))
			if (inArg1)
				XFAIL(err = inStore->addToCurrentTransaction(*objId))
			else
				XFAIL(err = inStore->separatelyAbort(*objId))
		}
	}
	XENDTRY;

	return err;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P a c k a g e C h u n k
	Encapsulation of a large object on a store.
----------------------------------------------------------------------------- */

PackageChunk::PackageChunk(CStore * inStore, PSSId inObjId)
{
	fStore = inStore;
	fRootId = inObjId;
	fCompanderName = NULL;
	fStoreCompander = NULL;
	fAddr = 0;
	fPkgId = 0;
	fError = noErr;
	f30 = 0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	Initialize the ROM domain manager.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
InitROMDomainManager(void)
{
	NewtonErr err;
	XTRY
	{
		ULong envId, domId;
		gROMStoreDomainManager = new CROMDomainManager1K();
#if !defined(forFramework)
		XFAIL(err = MemObjManager::findDomainId('romc', &domId))
		XFAIL(err = MemObjManager::findEnvironmentId('romc', &envId))
		XFAIL(err = gROMStoreDomainManager->init(envId, 'ROMF', 'ROMP', 2*KByte, 2*KByte))
		gROMStoreDomainManager->addDomain(domId);
#endif
	}
	XENDTRY;
	return err;
}


NewtonErr
RegisterROMDomainManager(void)
{
	InitializeCompression();
	InitializeStoreDecompressors();
	size_t	rdmCacheSize = InternalRAMInfo(kRDMCacheAlloc, kSubPageSize);	// so this is a kSubPageSize multiple
	gROMStoreDomainManager->reset(rdmCacheSize / kSubPageSize, 0);	// reset() wants number of pages in cache
	return noErr;
}


ObjectId
GetROMDomainManagerId(void)
{
#if !defined(forFramework)
	return (ObjectId) gROMStoreDomainManager->fPageMonitor;
#else
	return 0;
#endif
}


CUMonitor *
GetROMDomainUserMonitor(void)
{
#if !defined(forFramework)
	return &gROMStoreDomainManager->fPageMonitor;
#else
	static ObjectId dumdum = 0;
	return (CUMonitor *)&dumdum;
#endif
}

size_t
ROMDomainManagerFreePageCount(void)
{
	size_t	freePageCount;
#if !defined(forFramework)
	gROMStoreDomainManager->fPageMonitor.invokeRoutine(kRDM_FreePageCount, &freePageCount);
#else
	freePageCount = 0;
#endif
	return freePageCount;
}


#pragma mark -
/*------------------------------------------------------------------------------
	Domain address and size accessors.
------------------------------------------------------------------------------*/
#define kDomainSize (4*MByte)

#if defined(forFramework)
#include <sys/mman.h>
#include <mach/vm_statistics.h>
static VAddr baseAddr = 0;
#endif

VAddr
ROMDomainBase(void)
{
#if defined(forFramework)
	if (baseAddr == 0)
		baseAddr = (VAddr)mmap(NULL, kDomainSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, VM_MAKE_TAG(VM_MEMORY_APPLICATION_SPECIFIC_1), 0);
	return baseAddr;

#else
	CDomainInfo info;
	if (MemObjManager::getDomainInfoByName('romc', &info) == noErr)
		return info.base();
	return 0;
#endif
}

size_t
ROMDomainSize(void)
{
#if defined(forFramework)
	return kDomainSize/2;

#else
	CDomainInfo info;
	if (MemObjManager::getDomainInfoByName('romc', &info) == noErr)
		return info.size() / 2;
	return 0;
#endif
}

bool
IsInRDMSpace(VAddr inAddr)
{
	VAddr base = ROMDomainBase();
	return inAddr >= base
		 && inAddr < base + ROMDomainSize();
}


VAddr
XIPDomainBase(void)
{
#if defined(forFramework)
	return baseAddr + kDomainSize/2;

#else
	CDomainInfo info;
	if (MemObjManager::getDomainInfoByName('romc', &info) == noErr)
		return info.base() + info.size() / 2;
	return 0;
#endif
}

size_t
XIPDomainSize(void)
{
	return ROMDomainSize();
}


#pragma mark -
/*------------------------------------------------------------------------------
	C R O M D o m a i n M a n a g e r 1 K

	Handles kSubPageSize pages, hence the 1K.
	Compressed pages always expand to 1K.

	-10061 exception be thrown here.
------------------------------------------------------------------------------*/

CROMDomainManager1K::CROMDomainManager1K()
{
	fNumOfPageTableEntries = 0;
	f5C = 0;
	fNumOfFreePages = 0;
	fNumOfSubPages = 0;
	fNumOfPages = 0;
	f68 = 0;
	fROMBase = ROMDomainBase();
	fROMSize = ROMDomainSize();
	fXIPBase = XIPDomainBase();
	fPackageTable = NULL;
	fPageTable = NULL;
	fBuffer = NULL;
	fBC = CTime(0);
	fIsWritingOutPage = false;
	fCD = false;
	fDecompressor = NULL;
}


CROMDomainManager1K::~CROMDomainManager1K()
{
	if (fPackageTable)
		delete fPackageTable;
	if (fPageTable)
		delete[] fPageTable;
}


/*------------------------------------------------------------------------------
	L a r g e   O b j e c t s
------------------------------------------------------------------------------*/

NewtonErr
CROMDomainManager1K::userRequest(int inSelector, void * ioData)
{
	NewtonErr		err = noErr;
	RDMParams *		params = (RDMParams *)ioData;
	PackageChunk *	chunk;
	ArrayIndex		index;

	switch (inSelector)
	{
	case kRDM_MapLargeObject:
		if ((err = addPackage(params->fStore, params->fObjId, params->fRO, &params->fAddr)) != noErr)
			endSession(params->fStore, params->fObjId);
		break;

	case kRDM_UnmapLargeObject:
		if (params->fPkgId != 0)
			err = endSession(params->fPkgId);
		else if (params->fAddr != 0)
		{
			if ((err = objectToIndex(params->fAddr, &index)) == noErr)
			{
				chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
				params->fStore = chunk->fStore;
				params->fObjId = chunk->fRootId;
				err = endSession(index, true);
			}
		}
		else
			err = endSession(params->fStore, params->fObjId);
		break;

	case kRDM_FreePageCount:
		doAcquireDatabase(false);
		*(size_t *)ioData = (fNumOfPages == 0 || f5C == 0) ? 0 : fNumOfFreePages;
		doReleaseDatabase();
		break;

	case kRDM_4:
		err = setPackageId(params->fStore, params->fObjId, params->fPkgId);
		break;

	case kRDM_IdToStore:
		if ((err = packageToIndex(params->fPkgId, &index)) == noErr)
		{
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
			params->fStore = chunk->fStore;
			params->fObjId = chunk->fRootId;
		}
		break;

	case kRDM_6:
		if ((err = objectToIndex(params->fStore, params->fObjId, &index)) == noErr)
		{
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
			params->fPkgId = chunk->fPkgId;
		}
		break;

	case kRDM_7:
		if ((err = packageToIndex(params->fPkgId, &index)) == noErr)
		{
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
			params->fAddr = chunk->fAddr;
		}
		break;

	case kRDM_FlushLargeObject:
		if (params->fPkgId != 0)
			err = flushCache(params->fPkgId);
		else if (params->fStore != NULL)
			err = flushCache(params->fStore, params->fObjId);
		else if (params->fAddr != 0)
			err = flushCacheByBase(params->fAddr);
		else
			err = kOSErrBadParameters;
		break;

	case kRDM_ResizeLargeObject:
		err = resizeObject(&params->fAddr, params->fAddr, params->fSize, params->f14);
		break;

	case kRDM_AbortObject:
		if (params->fAddr != 0)
		{
			if ((err = objectToIndex(params->fAddr, &index)) == noErr)
			{
				flushCache(index, false);
				chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
				doTransactionAgainstObject(1, chunk, 0, 0);
				endSession(index, true);
			}
		}
		else if (params->fStore != NULL)
		{
			CArrayIterator	iter(fPackageTable);
			for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
			{
				chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
				if (chunk->fStore == params->fStore)
				{
					bool	isValidStore = IsValidStore(chunk->fStore);
					if (!isValidStore)
						params->fStore = NULL;
					flushCache(index, false);
					if (isValidStore && !chunk->fStoreCompander->isReadOnly())
						doTransactionAgainstObject(1, chunk, 0, 0);
					endSession(index, true);
				}
			}
		}
		break;

	case kRDM_CommitObject:
		if (params->fAddr != 0)
		{
			if ((err = objectToIndex(params->fAddr, &index)) == noErr)
			{
				chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
				err = flushCache(index, true);
				if (err == noErr)
					err = doTransactionAgainstObject(2, chunk, 0, 0);
				else
					doTransactionAgainstObject(1, chunk, 0, 0);
				if (err)
					endSession(index, true);
			}
		}
		else if (params->fStore != NULL)
		{
			CArrayIterator	iter(fPackageTable);
			for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
			{
				chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
				if (chunk->fStore == params->fStore)
				{
					err = flushCache(index, true);
					if (err == noErr)
						err = doTransactionAgainstObject(2, chunk, 0, 0);
					else
						doTransactionAgainstObject(1, chunk, 0, 0);
					if (err)
						endSession(index, true);
				}
			}
		}
		break;

	case kRDM_GetLargeObjectInfo:
		if ((err = objectToIndex(params->fAddr, &index)) == noErr)
		{
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
			params->fStore = chunk->fStore;
			params->fObjId = chunk->fRootId;
			params->fPkgId = chunk->fPkgId;
			params->fSize = chunk->fSize;
			params->fRO = chunk->fStoreCompander->isReadOnly();
			params->fDirty = (chunk->f30 || chunk->fTxnHandler.hasTransaction());
		}
		break;

	case kRDM_StoreToVAddr:
		if ((err = objectToIndex(params->fStore, params->fObjId, &index)) == noErr)
		{
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
			params->fAddr = chunk->fAddr;
			params->fSize = chunk->fSize;
		}
		break;

	case kRDM_LargeObjectAddress:
		chunk = getObjectPtr(params->fAddr);
		if (chunk == NULL)
			err = kOSErrItemNotFound;
		else
		{
			params->fAddr = chunk->fAddr;
			params->fStore = chunk->fStore;
			params->fObjId = chunk->fRootId;
			params->fPkgId = chunk->fPkgId;
			params->fSize = chunk->fSize;
			params->fRO = chunk->fStoreCompander->isReadOnly();
			params->fDirty = (chunk->f30 || chunk->fTxnHandler.hasTransaction());
		}
		break;

	case 16:
		if ((err = objectToIndex(params->fAddr, &index)) == noErr)
			err = endSession(index, false);
		break;

	case 17:
		XIPObjectHasMoved(params->fStore, params->fObjId);
		break;

	case 18:
		XIPInvalidateStore(params->fStore);
		break;
	}

	return err;
}


/* -----------------------------------------------------------------------------
	Resize an object.
	Args:		outAddr		new address of resized object
				inAddr		address of object to be resized
				inSize		new size required
				inReq			not used other than for checking: must be < 0
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::resizeObject(VAddr * outAddr, VAddr inAddr, size_t inSize, NewtonErr inReq)
{
	NewtonErr		err;
	ArrayIndex		index;
	PackageChunk *	chunk;
	const CClassInfo *	info;

	*outAddr = 0;
	XTRY
	{
		// find the object in the package table
		XFAIL(err = objectToIndex(inAddr, &index))
		chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
		// must be writable
		XFAILIF(chunk->fStoreCompander->isReadOnly(), err = kOSErrWriteProtected;)
		XFAIL(err = flushCache(index, true))

		PackageChunk	myChunk(*chunk);

		XTRY
		{
			CLrgObjStore *	objStore = NULL;
			// remove object from package table -- it will be added again after resizing
			fPackageTable->removeElementsAt(index, 1);
			if ((info = ClassInfoByName("CLrgObjStore", myChunk.fCompanderName)) != NULL)
			{
				// package has a compander -- let that handle the resizing
				XFAILIF(inReq >= 0, err = kOSErrUnsupportedRequest;)
				XFAILNOT(objStore = (CLrgObjStore *)MakeByName("CLrgObjStore", NULL, myChunk.fCompanderName), err = kOSErrNoMemory;)
				XFAILIF(err = objStore->init(), objStore->destroy();)
				XFAILIF(err = objStore->resize(myChunk.fStore, myChunk.fRootId, inSize), objStore->destroy();)
			}
			else
			{
				// we resize it
				XFAIL(err = mungeObject(&myChunk, MIN(inSize, myChunk.fSize), inSize - myChunk.fSize))
			}
			// update package size
			myChunk.fSize = inSize;
			// create new package entry
			XFAIL(err = allocatePackageEntry(&myChunk, &index))
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
			*outAddr = chunk->fAddr;
			// we no longer need the object store
			if (objStore)
				objStore->destroy();
		}
		XENDTRY;

		XDOFAIL(err)
		{
			doTransactionAgainstObject(1, &myChunk, 0, 0);
			deleteObjectInfo(&myChunk, err);
		}
		XENDFAIL;
	}
	XENDTRY;

	return err;
}


/* -----------------------------------------------------------------------------
	Munge an object -- insert or remove bytes.
	Args:		inChunk		object’s package wrapper
				inStart		offset into object
				inDelta		number of bytes to remove/insert
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::mungeObject(PackageChunk * inChunk, ArrayIndex inStart, int inDelta)
{
	NewtonErr	err;

	size_t		newSize = inChunk->fSize + inDelta;
	ArrayIndex	pageStart, startPageOffset = inStart & ~(kSubPageSize-1);
	int			pageDelta, deltaPageOffset = inDelta & ~(kSubPageSize-1);

	XTRY
	{
		if (startPageOffset == 0 && deltaPageOffset == 0)
		{
			// offset and size change are page aligned, so adjustment is trivial
		//	if (inStart < 0) inStart += (kSubPageSize-1);	// sic
			pageStart = inStart / kSubPageSize;
		//	if (inDelta < 0) inDelta += (kSubPageSize-1);	// sic
			pageDelta = inDelta / kSubPageSize;
		}
		else
		{
			ArrayIndex	numOfPages = (inChunk->fSize + (kSubPageSize-1)) / kSubPageSize;
			ArrayIndex	newNumOfPages = (newSize + (kSubPageSize-1)) / kSubPageSize;

			if (inDelta < 0)
			{
				// actually, we can only remove from the end of the object
				XFAILIF(inStart - inDelta != inChunk->fSize, err = kOSErrUnsupportedRequest;)
				pageStart = newNumOfPages;
				pageDelta = newNumOfPages - numOfPages;
			}
			else
			{
				// actually, we can only add to the end of the object
				XFAILIF(inStart != inChunk->fSize, err = kOSErrUnsupportedRequest;)
				pageStart = numOfPages;
				pageDelta = newNumOfPages - numOfPages;
			}
		}

		if (pageDelta != 0)
		{
			// more than a page has been added/removed -- we need to update the page table
			if (pageDelta > 0)
				XFAIL(err = insertPages(inChunk, pageStart, pageDelta))
			else
				XFAIL(err = removePages(inChunk, pageStart, -pageDelta))
		}
		XFAIL(err = doTransactionAgainstObject(0, inChunk, inChunk->fRootId, 2))
		err = inChunk->fStore->write(inChunk->fRootId, offsetof(LargeObjectRoot, fActualSize), &newSize, sizeof(newSize));
	}
	XENDTRY;

	return err;
}


/* -----------------------------------------------------------------------------
	Insert 1K pages into an object.
	Args:		inChunk		object’s package wrapper
				inOffset		offset (in pages) into object
				inCount		number of pages to insert
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::insertPages(PackageChunk * inChunk, ArrayIndex inOffset, ArrayIndex inCount)
{
	NewtonErr err = noErr;
	PSSId * pageArray = NULL;

	XTRY
	{
		// create array for object’s new data page ids
		XFAIL(inCount > 0)
		ArrayIndex	numOfPages = (inChunk->fSize + (kSubPageSize-1)) / kSubPageSize;
		ArrayIndex limit = inOffset + inCount;
		XFAILIF(inOffset > numOfPages, err = kOSErrBadParameters;)
		ArrayIndex newNumOfPages = numOfPages + inCount;
		ArrayIndex pageArraySize = newNumOfPages * sizeof(PSSId);
		pageArray = new PSSId[pageArraySize]();
		XFAIL(err = MemError())

		// read existing data page ids
		CStore * store = inChunk->fStore;
		XFAIL(err = store->read(inChunk->fDataId, 0, pageArray, numOfPages * sizeof(PSSId)))

		if (inOffset != numOfPages)
			// inserting within the array -- make space by moving existing page ids up
			memmove(pageArray+limit, pageArray+inOffset, (numOfPages-inOffset) * sizeof(PSSId));
		// create new PSSIds and insert them into the id array
		for ( ; inOffset < limit; inOffset++)
		{
			PSSId newId;
			XFAIL(err = store->newWithinTransaction(&newId, 0))
			XFAIL(err = doTransactionAgainstObject(0, inChunk, newId, 1))
			pageArray[inOffset] = newId;
		}
		XFAIL(err)
		XFAIL(err = doTransactionAgainstObject(0, inChunk, inChunk->fDataId, 2))
		// replace the object’s data with the new id array
		err = store->replaceObject(inChunk->fDataId, pageArray, pageArraySize);
	}
	XENDTRY;

	if (pageArray)
		delete[] pageArray;
	return err;
}


/* -----------------------------------------------------------------------------
	Remove pages from an object.
	Args:		inChunk		object’s package wrapper
				inOffset		offset into object
				inCount		number of pages to remove
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::removePages(PackageChunk * inChunk, ArrayIndex inOffset, ArrayIndex inCount)
{
	NewtonErr err = noErr;
	PSSId * pageArray = NULL;

	XTRY
	{
		// create array for object’s new data page ids
		XFAIL(inCount > 0)
		ArrayIndex	numOfPages = (inChunk->fSize + (kSubPageSize-1)) / kSubPageSize;	// sp04
		ArrayIndex limit = inOffset + inCount;	// r7
		XFAILIF(limit > numOfPages, err = kOSErrBadParameters;)
		ArrayIndex newNumOfPages = numOfPages - inCount;	// sp00
		ArrayIndex pageArraySize = numOfPages * sizeof(PSSId);	// r9
		pageArray = new PSSId[pageArraySize]();	// r6
		XFAIL(err = MemError())

		// read existing data page ids
		CStore * store = inChunk->fStore;
		XFAIL(err = store->read(inChunk->fDataId, 0, pageArray, pageArraySize))

		// delete PSSIds from the id array
		for ( ; inOffset < limit; inOffset++)
		{
			PSSId theId = pageArray[inOffset];
			XFAIL(err = doTransactionAgainstObject(0, inChunk, theId, 3))
			XFAIL(err = store->deleteObject(theId))
		}
		XFAIL(err)
		XFAIL(err = doTransactionAgainstObject(0, inChunk, inChunk->fDataId, 2))

		// replace the object’s data with the new id array
		if (limit == newNumOfPages)
			// removing from the end -- just resize the object
			err = store->setObjectSize(inChunk->fDataId, newNumOfPages * sizeof(PSSId));
		else
		{
			// removing from within the array -- close the space by moving existing page ids down
			memmove(pageArray+inOffset, pageArray+limit, (numOfPages-limit) * sizeof(PSSId));
			err = store->replaceObject(inChunk->fDataId, pageArray, newNumOfPages * sizeof(PSSId));
		}
	}
	XENDTRY;

	if (pageArray)
		delete[] pageArray;
	return err;
}


/* -----------------------------------------------------------------------------
	Perform a transaction.
	Args:		inSelector			0..2 == add, abort, commit?
				inChunk
				inId
				inArg4
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::doTransactionAgainstObject(int inSelector, PackageChunk * inChunk, PSSId inId, int inArg4)
{
	NewtonErr	err;

	if ((err = inChunk->fStoreCompander->doTransactionAgainst(inSelector, 0)) == noErr)
	{
		if (inSelector == 0)
			err = inChunk->fTxnHandler.addObjectToTransaction(inId, inChunk->fStore, inArg4);
		else
			err = inChunk->fTxnHandler.endTransaction(inSelector==2, inChunk->fStore, inChunk->fRootId, inChunk->fDataId);
	}

	return err;
}


/* -----------------------------------------------------------------------------
	Find which object the given virtual address lies within.
	Perform a binary search on the object table.
	Args:		inAddr
	Return:  PackageChunk pointer
----------------------------------------------------------------------------- */

PackageChunk *
CROMDomainManager1K::getObjectPtr(VAddr inAddr)
{
	ArrayIndex	numOfChunks = fPackageTable->count();
	if (numOfChunks > 0)
	{
		int  lastIndex = numOfChunks - 1;
		int  firstIndex = 0;
		int  index;
		PackageChunk *	chunk;

		while (lastIndex >= firstIndex)
		{
			index = (firstIndex + lastIndex) / 2;
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);	// r0
			if (inAddr < chunk->fAddr)
			// object must be in lower range
				lastIndex = index - 1;
			else if (inAddr < chunk->fAddr + chunk->fSize)
			// object is in this range!
				return chunk;
			else if (inAddr == chunk->fAddr && chunk->fSize == 0)
			// object address matches exactly and this range has no size...?
				return chunk;
			else
			// object must be in higher range
				firstIndex = index + 1;
		}
	}

// address isn’t within any object
	return NULL;
}


/* -----------------------------------------------------------------------------
	Find the index into the object table for an object address.
	Perform a linear search on the object table.
	Args:		inAddr
				outIndex
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::objectToIndex(VAddr inAddr, ArrayIndex * outIndex)
{
	NewtonErr		err = kOSErrNoSuchPackage;
	PackageChunk *	chunk;
	CArrayIterator iter(fPackageTable);

	for (ArrayIndex index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
	{
		chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
		if (chunk->fAddr == inAddr)
		{
			*outIndex = index;
			err = noErr;
			break;	// original doesn’t break!
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Find the index into the object table for an object given its store/id.
	Perform a linear search on the object table.
	Args:		inAddr
				outIndex
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::objectToIndex(CStore * inStore, PSSId inObjId, ArrayIndex * outIndex)
{
	NewtonErr		err = kOSErrNoSuchPackage;
	PackageChunk *	chunk;
	CArrayIterator iter(fPackageTable);

	for (ArrayIndex index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
	{
		chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
		if (chunk->fStore == inStore && chunk->fRootId == inObjId)
		{
			*outIndex = index;
			err = noErr;
			break;	// original doesn’t break!
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Find the index into the object table for an object given its package id.
	Perform a linear search on the object table.
	Args:		inAddr
				outIndex
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::packageToIndex(PSSId inPkgId, ArrayIndex * outIndex)
{
	NewtonErr		err = kOSErrNoSuchPackage;
	PackageChunk *	chunk;
	CArrayIterator iter(fPackageTable);

	for (ArrayIndex index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
	{
		chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
		if (chunk->fPkgId == inPkgId)
		{
			*outIndex = index;
			err = noErr;
			break;	// original doesn’t break!
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Delete an object’s entire package.
	Args:		inChunk
				inErr
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::deleteObjectInfo(PackageChunk * inChunk, NewtonErr inErr)
{
	const CClassInfo *	info;
	char				companderCleanupName[128];
	NewtonErr		err = inErr;

	if (inChunk->fStore)
	{
		if (inErr == noErr)
			err = doTransactionAgainstObject(2, inChunk, 0, 0);
		else
			doTransactionAgainstObject(1, inChunk, 0, 0);
	}

#if 0
// we don’t implement xxxCleanup companders anywhere, and this is a cause of crashes in NCX
	sprintf(companderCleanupName, "%sCleanup", inChunk->fCompanderName);
	if ((info = ClassInfoByName("CStoreCompander", companderCleanupName)) != NULL)
		inChunk->fStoreCompander->setType(info);
#endif
	inChunk->fStoreCompander->destroy();
	delete[] inChunk->fCompanderName;
	inChunk->fTxnHandler.free();

	return err;
}


/* -----------------------------------------------------------------------------
	Add a package wrapper for an object to its store.
	Args:		inStore			the store on which to create the package
				inObjId			the object to package up
				inReadOnly		true => object is read-only
				outAddr			mapped address of object
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::addPackage(CStore * inStore, PSSId inObjId, bool inReadOnly, VAddr * outAddr)
{
	NewtonErr		err;
	PackageRoot		root;								// sp48
	size_t			companderNameLen;				// sp44
	size_t			objSize;							// sp40
	bool				isRO;								// sp3C
	bool				isRW = false;
	PackageChunk	chunk(inStore, inObjId);	// sp00

	XTRY
	{
		if (GetStoreClassInfo(inStore)->getCapability('LOBJ')
		&&  inStore->inSeparateTransaction(inObjId))
			chunk.fTxnHandler.setAllInTransaction();
		XFAIL(err = inStore->read(inObjId, 0, &root, sizeof(root)))
		if ((root.flags & 0x00020000) == 0
		||  (err = XIPAddPackage(inStore, inObjId, outAddr)) == kOSErrXIPNotPossible)
		{
#if 0
			if (f5C == 0)
			{
				ArrayIndex index;
				ObjectId	pageId;
				XFAIL(err = get(pageId, 2))
				doAcquireDatabase(false);
				addPage(&index, pageId);
				doReleaseDatabase();
			}
#endif
			// verify object’s root
			XFAILIF((root.flags & 0xFFFF) > 2, err = kOSErrBadPackage;)
			// inherit r/w from store and object root
			inStore->isReadOnly(&isRO);
			isRW = (!isRO && (root.flags & 0x00010000) != 0);
			if (inReadOnly)
				isRW = false;

			// read store’s compander name
			chunk.fDataId = root.fDataId;
			XFAIL(err = inStore->getObjectSize(root.fCompanderNameId, &companderNameLen))
			chunk.fCompanderName = new char[companderNameLen+1];
			XFAIL(err = MemError())
			XFAIL(err = inStore->read(root.fCompanderNameId, 0, chunk.fCompanderName, companderNameLen))
			chunk.fCompanderName[companderNameLen] = 0;

			if ((root.flags & 0xFFFF) == 1)	// object is a package
			{
				// calc its size
				XFAIL(err = inStore->getObjectSize(root.fDataId, &objSize))
				objSize = (objSize/sizeof(PSSId)) * kSubPageSize;
				// make a compander for it
				chunk.fStoreCompander = (CStoreCompander *)MakeByName("CStoreCompander", "CStoreCompanderWrapper");
				XFAILNOT(chunk.fStoreCompander, err = kOSErrBadParameters;)
				// free up memory
				releasePagesFromOurWS(4);
				// initialize compander
				// LZ decompressors need a buffer rather than a parms id
				// the original cast LZ buffer pointer to PSSId and used the same arg -- we use two args
				bool	isLZDecomp = strcmp(chunk.fCompanderName, "CLZStoreDecompressor") == 0
									 || strcmp(chunk.fCompanderName, "CLZRelocStoreDecompressor") == 0;
				XFAIL(err = ((CStoreCompanderWrapper *)chunk.fStoreCompander)->init(inStore, chunk.fCompanderName, inObjId, root.fCompanderParmsId, isLZDecomp ? fBuffer : NULL))
// -->		// hack to make packages the right length
				inStore->read(inObjId, offsetof(LargeObjectRoot, fActualSize), &objSize, sizeof(objSize));
				// create package wrapper
				chunk.fSize = objSize;
				err = allocatePackageEntry(&chunk, NULL);
				// return its address
				*outAddr = chunk.fAddr;
			}
			else if ((root.flags & 0xFFFF) == 2)	// it’s actually a LargeObjectRoot
			{
				// calc its size
				inStore->read(inObjId, offsetof(LargeObjectRoot, fActualSize), &objSize, sizeof(objSize));
				// make a compander for it
				chunk.fStoreCompander = (CStoreCompander *)MakeByName("CStoreCompander", chunk.fCompanderName);
				XFAILNOT(chunk.fStoreCompander, err = kOSErrBadParameters;)
				// free up memory
				releasePagesFromOurWS(6);
				// initialize compander
				XFAIL(err = chunk.fStoreCompander->init(inStore, inObjId, root.fCompanderParmsId, !isRW, true))
				// create package wrapper
				chunk.fSize = objSize;
				err = allocatePackageEntry(&chunk, NULL);
				// return its address
				*outAddr = chunk.fAddr;
			}
			else
				XFAIL(err = kOSErrBadParameters)
		}

	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (isRW)
			doTransactionAgainstObject(1, &chunk, 0, 0);
		delete[] chunk.fCompanderName;
		if (chunk.fStoreCompander)
			chunk.fStoreCompander->destroy();
	}
	XENDFAIL;
#if 0
	GenericSWI(71);
#endif

	return err;
}


/* -----------------------------------------------------------------------------
	Allocate an entry for a package wrapper in the fPackageTable.
	In a VM system, faulting in data from backing store would be deferred until
	the large object’s BinaryData is referenced.
	Here we fault in all the data immediately.
	Args:		inChunk		package descriptor
								on return, object’s address is set and descriptor is inserted in fPackageTable
				outIndex		on return, fPackageTable index of entry
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::allocatePackageEntry(PackageChunk * inChunk, ArrayIndex * outIndex)
{
	NewtonErr		err;
	PackageChunk *	chunk;
	ArrayIndex		count;
	ArrayIndex		foundIndex;
	ArrayIndex		entryIndex = 0;
	VAddr				address = 0;
	long				sizeAvailable;
	long				chunkSize;
	bool				isEntryAvailable = false;

	for (count = fPackageTable->count(); count > 0; --count)
	{
		chunk = (PackageChunk *)fPackageTable->elementPtrAt(count-1);
		if ((chunk->fAddr) < fXIPBase)
			break;
	}
	// count==0 => no chunks, or all chunks are in XIP space

	if (count > 0)
	{
		foundIndex = count - 1;
		for (entryIndex = 0; entryIndex < count; ++entryIndex)
		{
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(entryIndex);
			chunkSize = chunk->fSize;
			if (chunkSize == 0)
				chunkSize = kSubPageSize;
			else
				chunkSize = ALIGN(chunkSize, kSubPageSize);
			if (foundIndex > entryIndex)
				sizeAvailable = ((PackageChunk *)fPackageTable->elementPtrAt(entryIndex+1))->fAddr - chunk->fAddr;
			else
				sizeAvailable = fROMSize - (chunk->fAddr - fROMBase);
			sizeAvailable -= chunkSize;
			if (inChunk->fSize < sizeAvailable
			|| (inChunk->fSize != 0 && inChunk->fSize == sizeAvailable))
			{
				isEntryAvailable = true;
				address = chunk->fAddr + chunkSize;
				++entryIndex;
				break;
			}
		}
	}
	else	// this must be the first entry
	{
		if (inChunk->fSize <= fROMSize)
		{
			isEntryAvailable = true;
			entryIndex = 0;
			address = fROMBase;
		}
	}

	if (isEntryAvailable)
	{
		inChunk->fAddr = address;
		err = fPackageTable->insertElementsBefore(entryIndex, inChunk, 1);
#if 1
		// manually fault in the entire package from backing store
		ProcessorState woo;
		for (chunkSize = inChunk->fSize; chunkSize > 0; chunkSize -= kSubPageSize, address += kSubPageSize)
		{
			woo.fAddr = address;	// must be subpage-aligned
			fault(&woo);
		}
#endif
	}
	else
		err = kOSErrNoMemory;

	if (outIndex)
		*outIndex = entryIndex;
	return err;
}


/* -----------------------------------------------------------------------------
	Set an object’s package wrapper.
	Args:		inStore			the store on which to create the package
				inObjId			the object to package up
				inPkgId			its package
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::setPackageId(CStore * inStore, PSSId inObjId, PSSId inPkgId)
{
	NewtonErr		err;
	PackageChunk *	chunk;
	ArrayIndex		index;
	if ((err = objectToIndex(inStore, inObjId, &index)) == noErr)
	{
		chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
		chunk->fPkgId = inPkgId;
	}
}


/* -----------------------------------------------------------------------------
	Acquire the page working set.
	Args:		inArg1
	Return:  --
----------------------------------------------------------------------------- */

void
CROMDomainManager1K::doAcquireDatabase(bool inArg1)
{
#if 0
	if (gPageTracker->pageCount() < 3)
	{
		if (restrictToInternalWorkingSet())
			releasePagesFromOurWS(3);
	}
	if (gPageTracker->pageCount() < 3)
		releasePagesForFaultHandling(3,6);
	fLock.acquire(kWaitOnBlock);
#endif
}


/* -----------------------------------------------------------------------------
	Release the page working set.
	Args:		--
	Return:  --
----------------------------------------------------------------------------- */

void
CROMDomainManager1K::doReleaseDatabase(void)
{
#if 0
	fLock.release();
#endif
}

#pragma mark -

NewtonErr
CROMDomainManager1K::endSession(ArrayIndex index, bool inArg2)
{
	NewtonErr		err;
	PackageChunk *	chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);

	if (chunk->fAddr >= fXIPBase)
		err = XIPEndSession(index);
	else
	{
		err = flushCache(index, inArg2);
		err = deleteObjectInfo(chunk, err);
		fPackageTable->removeElementsAt(index, 1);
#if 0
		GenericSWI(71);
#endif
	}
	return err;
}


NewtonErr
CROMDomainManager1K::flushCache(ArrayIndex index, bool inArg2)
{
	NewtonErr		err;
	PackageChunk *	chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
	VAddr				r10 = (chunk->fAddr - fROMBase)/kSubPageSize;
	VAddr				r9 = ((chunk->fAddr + chunk->fSize + kSubPageSize-1) - fROMBase)/kSubPageSize;

	doAcquireDatabase(false);
	fDA = (inArg2 && chunk->fStore != NULL);
	for (ArrayIndex pageIndex = 0; pageIndex < fNumOfPages; pageIndex++)
	{
		ULong pageAddr;
		unsigned subPageMask = 0;
		PageTableChunk *	pageTableEntry = &fPageTable[pageIndex];
		for (ArrayIndex subPageIndex = 0; subPageIndex < 4; ++subPageIndex)
		{
			if ((pageAddr = pageTableEntry->fPageAddr[subPageIndex]) != 0xFFFF
			&&  pageAddr*4 >= r10		// ••?? comparing pages to subpages
			&&  pageAddr*4 < r9)
				subPageMask |= (1 << subPageIndex);
		}
		if (subPageMask != 0)
		{
			freeSubPages(pageIndex, subPageMask);
			if (!isEmptyPage(pageTableEntry))
			{
				bool	isExtPage = false;
				ObjectId	pageId = pageTableEntry->fId;
#if 0
				if (pageId != 0)
				{
					CUObject	uobj(pageId);
					if (uobj.isExtPage())
						isExtPage = true;
				}
#endif
				if (!isExtPage)
					fNumOfFreePages--;
				f5C--;
				clearTableEntry(pageIndex);
#if !defined(forFramework)
				CUDomainManager::release(pageId);
#endif
			}
		}
	}
	err = chunk->fError;
	chunk->f30 = false;
	fDA = true;
	doReleaseDatabase();
	return err;
}


NewtonErr
CROMDomainManager1K::endSession(CStore * inStore, PSSId inObjId)
{
	NewtonErr	err;
	ArrayIndex	index;
	if ((err = objectToIndex(inStore, inObjId, &index)) == noErr)
		err = endSession(index, true);
	return err;
}


NewtonErr
CROMDomainManager1K::flushCache(CStore * inStore, PSSId inObjId)
{
	NewtonErr	err;
	ArrayIndex	index;
	if ((err = objectToIndex(inStore, inObjId, &index)) == noErr)
		err = flushCache(index, true);
	else if (err == kOSErrNoSuchPackage)
		err = noErr;
	return err;
}


NewtonErr
CROMDomainManager1K::endSession(PSSId inPkgId)
{
	NewtonErr	err;
	ArrayIndex	index;
	if ((err = packageToIndex(inPkgId, &index)) == noErr)
		err = endSession(index, true);
	return err;
}

NewtonErr
CROMDomainManager1K::flushCache(PSSId inPkgId)
{
	NewtonErr	err;
	ArrayIndex	index;
	if ((err = packageToIndex(inPkgId, &index)) == noErr)
		err = flushCache(index, true);
	return err;
}


NewtonErr
CROMDomainManager1K::flushCacheByBase(VAddr inAddr)
{
	NewtonErr	err;
	ArrayIndex	index;
	if ((err = objectToIndex(inAddr, &index)) == noErr)
		err = flushCache(index, true);
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Handle a fault.
	Args:		inState
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
CROMDomainManager1K::fault(ProcessorState * inState)
{
	if (inState->fAddr < fXIPBase)
	{
		gRDMNumberOfFaults++;
		long  addrOffset = inState->fAddr - fROMBase;
		if (!(0 <= addrOffset && addrOffset < fROMSize))
		{
			ThrowErr(exPermissionViolation, kOSErrPermissionViolation);
			return kOSErrNoMemory;
		}

		PackageChunk *	obj = getObjectPtr(inState->fAddr);
		if (obj == NULL)
			ThrowErr(exPermissionViolation, kOSErrPermissionViolation);

		ULong r0 = inState->f48 & 0x0F;
		fD8 = (r0 == 13 || r0 == 15);
		fIsROCompander = obj->fStoreCompander->isReadOnly();

		bool before = gCardReinsertionTracker.f00;
		decompressAndMap(TRUNC(inState->fAddr, kSubPageSize), obj);		// map in memory from compressed backing store
		bool after = gCardReinsertionTracker.f00;
		if (after && !before)
		{
			gCardReinsertionTracker.f04 = obj->fStore;
			gCardReinsertionTracker.f08 = obj->fRootId;
			gCardReinsertionTracker.f0C = inState->fAddr - obj->fAddr;
		}
#if 0
		MonitorFlushSWI(fFaultMonitor);
#endif
	}
	else
		XIPFault(inState);
	return noErr;
}


/*------------------------------------------------------------------------------
	Handle a fault. For XIP (eXecute In Place) flash card memory.
	Args:		inState
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
CROMDomainManager1K::XIPFault(ProcessorState * inState)
{
	if (!(fXIPBase <= inState->fAddr && inState->fAddr < fXIPBase + fROMSize))
	{
		ThrowErr(exPermissionViolation, kOSErrPermissionViolation);
		return kOSErrNoMemory;
	}

	XIPMapInPackageSection(inState->fAddr);
	return noErr;
}


NewtonErr
CROMDomainManager1K::releaseRequest(int inSelector)
{
	NewtonErr  err = kOSErrNoMemory;
	ArrayIndex pageNum;
#if 0
	if (fLock.acquire(kNoWaitOnBlock) == noErr)
	{
		fBC = GetGlobalTime();
		fLock.release();
		if (inSelector = 0 || inSelector == 1)
			fCD = true;
		if ((err = getWorkingSetPage(&pageNum)) == noErr)
			release(pageNum);
		
	}
#else
	err = noErr;
#endif
	fCD = false;

	return err;
}


/*------------------------------------------------------------------------------
	Decompress a page out of backing store and map it into memory.
	Args:		inAddr			address at which to load decompressed data
				inChunk			backing store data descriptor
	Return:  error code
------------------------------------------------------------------------------*/
#if defined(forFramework)
// beccause we don’t subclass from CUDomainManager
NewtonErr
CROMDomainManager1K::remember(VAddr inAddr, ULong inPermissions, PSSId inId, bool inAlwaysTrueAFAICT)
{ return noErr; }
#endif

NewtonErr
CROMDomainManager1K::decompressAndMap(VAddr inAddr, PackageChunk * inChunk)
{
	NewtonErr	err = noErr;
	ArrayIndex	pageIndex;
	ArrayIndex	subPageIndex = (inAddr / kSubPageSize) & 0x03;
	
	if ((err = getSubPage(inAddr, &pageIndex, inChunk)) != noErr)
		ThrowErr(exAbort, err);

	if (pageIndex != -1)	// original says 0xFF
	{
		if ((err = remember(inAddr, makePermissions(pageIndex, subPageIndex, true), fPageTable[pageIndex].fId, true)) == noErr);
		{
			VAddr errAddr = NULL;
			newton_try
			{
				inChunk->fStoreCompander->read(inAddr - inChunk->fAddr, (char *)inAddr, kSubPageSize, inChunk->fAddr);
			}
			newton_catch(exBusError)
			{
				err = kOSErrBusAccess;
				errAddr = (VAddr)_info.exception.data;
			}
			end_try;

			if (err)
			{
				if (errAddr >= inAddr && errAddr < inAddr + kSubPageSize)
				{
					freeSubPages(pageIndex, 15);
					bool  isExt = false;
					if (fPageTable[pageIndex].fId != 0)
					{
#if 0
						CUObject uobj(fPageTable[pageIndex].fId);
						if (uobj.isExtPage())
							isExt = true;
#endif
					}
					if (!isExt)
						fNumOfFreePages--;
					f5C--;
					clearTableEntry(pageIndex);
				}
				else
				{
					releasePageTableEntry(pageIndex);
					if (err)
						ThrowErr(exAbort, err);
				}
			}
		}
		if (err == noErr)
		{
			if ((err = remember(inAddr, makePermissions(pageIndex, subPageIndex, false), fPageTable[pageIndex].fId, true)) == noErr)
			{
#if 0
				if (IsDebuggerPresent())
					InformDebuggerMemoryReloaded(inAddr, kSubPageSize)
#endif
				;
			}
		}
		releasePageTableEntry(pageIndex);
		if (err)
			ThrowErr(exAbort, err);
	}

	return noErr;
}


#pragma mark -
/*------------------------------------------------------------------------------
	X I P
------------------------------------------------------------------------------*/

void
CROMDomainManager1K::XIPMapInPackageSection(VAddr inAddr)
{}


NewtonErr
CROMDomainManager1K::XIPAllocatePackageEntry(PackageChunk * outChunk, long*)
{}


NewtonErr
CROMDomainManager1K::XIPAddPackage(CStore * inStore, PSSId inObjId, VAddr * outAddr)
{ return kOSErrXIPNotPossible; }


NewtonErr
CROMDomainManager1K::XIPEndSession(ArrayIndex index)
{
	NewtonErr	err;
	PackageChunk *	chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
#if 0
	err = forgetPhysMapRange(chunk->fAddr, chunk->f10->fId, chunk->fSize);
#else
	err = noErr;
#endif
	delete chunk->fCompanderName;
#if 0
	if (chunk->f10)
		delete chunk->f10;
#endif
	delete chunk->fStoreCompander;
	fPackageTable->removeElementsAt(index, 1);
	return err;
}


long
XIPChunkInObject(long*, CStore*, unsigned long, unsigned long)
{ return 0; }


void
CROMDomainManager1K::XIPObjectHasMoved(CStore * inStore, PSSId inObjId)
{}


void
CROMDomainManager1K::XIPInvalidateStore(CStore * inStore)
{
	if (!fPackageTable->isEmpty())
	{
		PackageChunk *	chunk;
		long				index;
		CArrayIterator	iter(fPackageTable);
		for (index = iter.firstIndex(); iter.more(); index = iter.nextIndex())
		{
			chunk = (PackageChunk *)fPackageTable->elementPtrAt(index);
#if 0
			if (chunk->fStore == inStore
			&&  chunk->fAddr >= fXIPBase)
				forgetPhysMapRange(chunk->fAddr, chunk->f10->fId, chunk->fSize);
#endif
		}
	}
}


void
XIPInvalidateStore(CStore * inStore)
{
	NewtonErr	err;
	RDMParams	parms;
#if 0
	CUMonitor	um(GetROMDomainUserMonitor());
	parms.fStore = inStore;
	parms.fObjId = 0;
	um.invokeRoutine(kRDM_18, &parms);
#else
	parms.fStore = inStore;
	parms.fObjId = 0;
	gROMStoreDomainManager->userRequest(kRDM_18, &parms);
#endif
}


void
XIPObjectHasMoved(CStore * inStore, PSSId inId)
{
	NewtonErr	err;
	RDMParams	parms;
#if 0
	CUMonitor	um(GetROMDomainUserMonitor());
	parms.fStore = inStore;
	parms.fObjId = inId;
	um.invokeRoutine(kRDM_17, &parms);
#else
	parms.fStore = inStore;
	parms.fObjId = inId;
	gROMStoreDomainManager->userRequest(kRDM_17, &parms);
#endif
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P a g e   T a b l e
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Reset the manager.
	Args:		inNumSubPages		number of 1K subpages for the cache
				inGuardSubPages	number of 1K guard subpages
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::reset(ArrayIndex inNumSubPages, ArrayIndex inGuardSubPages)
{
	NewtonErr	err;
	size_t		lzBufSize;
	ArrayIndex	numOfPages;

	if (fPageTable)
		delete[] fPageTable, fPageTable = NULL;	// was free()!
	if (fPackageTable)
		delete fPackageTable, fPackageTable = NULL;

	fNumOfPageTableEntries = 0;
	f5C = 0;
	fNumOfFreePages = 0;
	fFreeIndex = 0;
	fIsWritingOutPage = false;
	fCD = false;
	fNumOfSubPages = inNumSubPages + inGuardSubPages;
	numOfPages = fNumOfSubPages / kSubPagesPerPage;
	if (numOfPages > kDomainSize / kPageSize)
		numOfPages = kDomainSize / kPageSize;		// limit the page cache size: 4M / 4K => 1024 pages max
	fNumOfPages = numOfPages;
	fC4 = numOfPages / 10;		// low water
	fC8 = numOfPages / 2;		// mid water
	f68 = numOfPages - inGuardSubPages/kSubPagesPerPage;	// danger zone? doesn’t seem to be referenced anywhere

	XTRY
	{
		fPageTable = new PageTableChunk[inNumSubPages];
		XFAIL(err = MemError())
		for (ArrayIndex index = 0; index < fNumOfPages; ++index)
			clearTableEntry(index);
		XFAIL(err = GetSharedLZObjects(NULL, &fDecompressor, &fBuffer, &lzBufSize))
#if defined(correct)
		// sic -- why do it again?
		for (ArrayIndex index = 0; index < fNumOfPages; ++index)
			clearTableEntry(index);
#endif
		fPackageTable = new CDynamicArray(sizeof(PackageChunk));
		XFAIL(err = MemError())
	}
	XENDTRY;

	XDOFAIL(err)
	{
		if (fPackageTable)
			delete fPackageTable;
		delete[] fPageTable;
		return err;
	}
	XENDFAIL;

	f6C[ 0] = 0;
	f6C[ 1] = 1;
	f6C[ 2] = 1;
	f6C[ 3] = 2;

	f6C[ 4] = 1;
	f6C[ 5] = 3;
	f6C[ 6] = 2;
	f6C[ 7] = 4;

	f6C[ 8] = 1;
	f6C[ 9] = 3;
	f6C[10] = 3;
	f6C[11] = 5;

	f6C[12] = 2;
	f6C[13] = 5;
	f6C[14] = 4;
	f6C[15] = 6;

	fAC = 6;

	fB4 = -1;	// original says 0xFFFF
	fB0 = -1;	// original says 0xFFFF
	fDA = true;

	return err;
}


/* -----------------------------------------------------------------------------
	Clear a page table entry.
	Args:		inPageIndex		index into fPageTable
	Return:	--
----------------------------------------------------------------------------- */

void
CROMDomainManager1K::clearTableEntry(ArrayIndex inPageIndex)
{
	if (fPageTable)
	{
		PageTableChunk *	entry = &fPageTable[inPageIndex];
		entry->fId = 0;
		// clear subpages
		entry->fPageAddr[0] = 0xFFFF;
		entry->fPageAddr[1] = 0xFFFF;
		entry->fPageAddr[2] = 0xFFFF;
		entry->fPageAddr[3] = 0xFFFF;
		entry->fIsWritable[0] = false;
		entry->fIsWritable[1] = false;
		entry->fIsWritable[2] = false;
		entry->fIsWritable[3] = false;
	}
}


/* -----------------------------------------------------------------------------
	Add a page table entry.
	Args:		inPageIndex			index into fPageTable
				inAddr				page address
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::addPageTableEntry(ArrayIndex inPageIndex, VAddr inAddr)
{
	ArrayIndex	pageAddr = (inAddr - fROMBase) / kPageSize;	// actually page offset from fROMBase
	ArrayIndex	subPageIndex = (inAddr / kSubPageSize) & 0x03;
	PageTableChunk *	entry = &fPageTable[inPageIndex];
	entry->fPageAddr[subPageIndex] = pageAddr;
	fNumOfPageTableEntries++;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Add a page table entry.
	Args:		inPageIndex			index into fPageTable
				inPageAddr			page offset from fROMBase
				inPermissions		R/W permissions for page
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::addPageTableEntry(ArrayIndex inPageIndex, UShort inPageAddr, ULong inPermissions)
{
	PageTableChunk *	entry = &fPageTable[inPageIndex];
	Perm	perm;

	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex, inPermissions >>= 2)
	{
		if ((perm = (Perm)(inPermissions & 0x03)) != kNoAccess)
		{
			entry->fPageAddr[subPageIndex] = inPageAddr;
			entry->fIsWritable[subPageIndex] = (perm == kReadWrite);
			fNumOfPageTableEntries++;
		}
	}
	return noErr;
}


/* -----------------------------------------------------------------------------
	Release a page table entry.
	Args:		inPageNum
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::releasePageTableEntry(ArrayIndex inPageNum)
{
	doAcquireDatabase(true);
	unrestrictPage(inPageNum);
	doReleaseDatabase();
	return noErr;
}


/* -----------------------------------------------------------------------------
	Release pages from our working set.
	Args:		inNumOfPagesRequired
	Return:	--
----------------------------------------------------------------------------- */

void
CROMDomainManager1K::releasePagesFromOurWS(ArrayIndex inNumOfPagesRequired)
{
	XTRY
	{
#if 0
		while (gPageTracker->pageCount() < inNumOfPagesRequired)
		{
			XFAIL(releaseRequest(2))
		}
#endif
	}
	XENDTRY;
}


ArrayIndex
CROMDomainManager1K::vAddrToPageIndex(VAddr inAddr)
{
	ULong	pageAddr = (inAddr - fROMBase) / kPageSize;	// page offset from fROMBase
	PageTableChunk *	entry;
	for (ArrayIndex pageIndex = 0; pageIndex < fNumOfPages; ++pageIndex)
	{
		entry = &fPageTable[pageIndex];
		for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex)
		{
			if (entry->fPageAddr[subPageIndex] == pageAddr)
				return pageIndex;
		}
	}
	return kIndexNotFound;
}


void
CROMDomainManager1K::getSourcePage(ArrayIndex inPageNum)
{}


bool
CROMDomainManager1K::restrictToInternalWorkingSet(void)
{
#if 0
	if (f5C >= fNumOfPages
	||  gPageTracker->pageCount() < 3)
		return true;

	CTime delta(GetGlobalTime() - fBC);
	CTime	expiry;
	if (f5C < fC4)			// < 10%
		expiry.set(kNoTimeout);
	else if (f5C < fC8)	// < 50%
		expiry.set(100*kMilliseconds);
	else
		expiry.set(400*kMilliseconds);
		
	return delta < expiry;
#else
	return false;
#endif
}


NewtonErr
CROMDomainManager1K::getWorkingSetPage(ULong * outPageNum)
{ return noErr; }


NewtonErr
CROMDomainManager1K::restrictPage(ArrayIndex inPageNum)
{
	if (fB0 == -1)	// original says 0xFF
		fB0 = inPageNum;
	else
		fB4 = inPageNum;
	return noErr;
}


NewtonErr
CROMDomainManager1K::unrestrictPage(ArrayIndex inPageNum)
{
	if (fB0 == inPageNum)
		fB0 = -1;	// original says 0xFF
	if (fB4 == inPageNum)
		fB4 = -1;	// original says 0xFF
	return noErr;
}


bool
CROMDomainManager1K::restrictedPage(ArrayIndex inPageNum)
{
	return inPageNum == fB0
		 || inPageNum == fB4;
}


NewtonErr
CROMDomainManager1K::addPage(ArrayIndex * outIndex, ObjectId inPageId)
{
	PageTableChunk *	entry;
	for (ArrayIndex pageIndex = 0; pageIndex < fNumOfPages; pageIndex++)
	{
		entry = &fPageTable[pageIndex];
		if (!isValidPage(entry))
		{
			bool	isExtPage = false;
			f5C++;
#if 0
			if (inPageId != 0)
			{
				CUObject	uobj(inPageId);
				if (uobj.isExtPage())
					isExtPage = true;
			}
#endif
			if (!isExtPage)
				fNumOfFreePages++;
			entry->fId = inPageId;
			*outIndex = pageIndex;
			return noErr;
		}
	}
	return kOSErrNoMemory;
}


bool
CROMDomainManager1K::isEmptyPage(PageTableChunk * inPageTableEntry)
{
	if (isValidPage(inPageTableEntry))
		return false;

	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex)
	{
		if (inPageTableEntry->fPageAddr[subPageIndex] != 0xFFFF)
			return false;
	}

	return true;
}


bool
CROMDomainManager1K::isValidPage(PageTableChunk * inPageTableEntry)
{
	return inPageTableEntry->fId != 0;
}


bool
CROMDomainManager1K::okIfDirty(ULong inPageIndex)
{
	if (!fCD)
		return true;

	PageTableChunk *	pageTableEntry = &fPageTable[inPageIndex];
	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex)
	{
		if (pageTableEntry->fIsWritable[subPageIndex])
			return false;
	}

	return true;
}


NewtonErr
CROMDomainManager1K::collect(ULong * outPermissions, ULong inSubPageMask, ArrayIndex inSubPageIndex, PageTableChunk * inPageTableEntry)
{
	ULong		pageAddr = inPageTableEntry->fPageAddr[inSubPageIndex];
	*outPermissions = kNoAccess;
	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex, inSubPageMask >>= 1)
	{
		if (inPageTableEntry->fPageAddr[subPageIndex] == pageAddr)
		{
			if ((inSubPageMask & 0x0001) != 0)
			{
				if (inPageTableEntry->fIsWritable[subPageIndex])
				{
					if (fDA)
					{
						remember(fROMBase + pageAddr*kPageSize, 0, inPageTableEntry->fId, true);
						writeOutPage(fROMBase + pageAddr*kPageSize + subPageIndex*kSubPageSize);
					}
					inPageTableEntry->fIsWritable[subPageIndex] = false;
				}
				inPageTableEntry->fPageAddr[subPageIndex] = 0xFFFF;
				fNumOfPageTableEntries--;
			}
			else
				*outPermissions |= inPageTableEntry->fIsWritable[subPageIndex] ? (kReadWrite << (subPageIndex*2)) : (kReadOnly << (subPageIndex*2));
		}
	}

	return noErr;
}


/* -----------------------------------------------------------------------------
	Write a page out to store.
	Args:		inAddr
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::writeOutPage(VAddr inAddr)
{
	NewtonErr	err;
	PSSId			objId;
	size_t		offset;

	PackageChunk *	obj = getObjectPtr(inAddr);
	if (obj == NULL)
		ThrowErr(exAbort, kOSErrPermissionViolation);

	fIsWritingOutPage = true;
	XTRY
	{
		offset = inAddr - obj->fAddr;
		XFAIL(err = obj->fStore->read(obj->fDataId, (offset/kSubPageSize)*sizeof(PSSId), &objId, sizeof(objId)))
		XFAIL(err = doTransactionAgainstObject(0, obj, objId, 2))
		err = obj->fStoreCompander->write(offset, (char *)inAddr, kSubPageSize, obj->fAddr);
	}
	XENDTRY;
	fIsWritingOutPage = false;

	obj->fError = err;
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	S u b   P a g e s
------------------------------------------------------------------------------*/

NewtonErr
CROMDomainManager1K::getSubPage(VAddr inAddr, ArrayIndex * outPageIndex, PackageChunk * inChunk)
{
// IT’S A WHOPPER
	int			sp00;	// actually stores bool
	long			sp04;
	ArrayIndex	sp08;
	ArrayIndex	subPageIndex = (inAddr / kSubPageSize) & 0x03;  // r10
	NewtonErr	err = noErr; // r7

	doAcquireDatabase(true);
	sp00 = restrictToInternalWorkingSet();
#if 0
	sp08 = vAddrToPageIndex(inAddr);
	CULockingSemaphore * sema4 = ;  // r8
	if (sp08 == 255)
	{
		// etc...
	}
	else
	{
	}
//L340:
	if (err == noErr)
		addPageTableEntry(sp08, inAddr);
#endif
	doReleaseDatabase();
	return err;
}


/* -----------------------------------------------------------------------------
	Make a bitmap of subpages of a given page in use in the page table.
	Args:		inPageIndex			index into the page table
				inPageAddr			page address = page offset from fROMBase
	Return:  4-bit bitmap
----------------------------------------------------------------------------- */

ULong
CROMDomainManager1K::makeSubPageBitMap(ArrayIndex inPageIndex, UShort inPageAddr)
{
	ULong					bitmap = 0;
	PageTableChunk *	pageTableEntry = &fPageTable[inPageIndex];

	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex)
	{
		if (pageTableEntry->fPageAddr[subPageIndex] == inPageAddr)
			bitmap |= (1 << subPageIndex);
	}
	return bitmap;
}


/* -----------------------------------------------------------------------------
	Make a bitmap of subpages in use in the page table.
	Args:		inPageIndex			index into the page table
	Return:  4-bit bitmap
----------------------------------------------------------------------------- */

ULong
CROMDomainManager1K::subPageMap(ArrayIndex inPageIndex)
{
	ULong					bitmap = 0;
	PageTableChunk *	pageTableEntry = &fPageTable[inPageIndex];

	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex)
	{
		if (pageTableEntry->fPageAddr[subPageIndex] == 0xFFFF)
			bitmap |= (1 << subPageIndex);
	}
	return bitmap;
}


/* -----------------------------------------------------------------------------
	Search the page table for the first entry with the given subpages in use.
	Args:		outPageIndex		index into the page table
				inSubPageMask		subpages required within the page
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::findSubPage(ArrayIndex * outPageIndex, ULong inSubPageMask)
{
	ArrayIndex	foundPage = kIndexNotFound;
	int	r8 = fAC + 1;
	ULong	subPagesInUseMask;

	for (ArrayIndex pageIndex = 0; pageIndex < fNumOfPages; pageIndex++)
	{
		if (isValidPage(&fPageTable[pageIndex])
		&&  (subPagesInUseMask = (subPageMap(pageIndex) & inSubPageMask)) == inSubPageMask
		&&  f6C[subPagesInUseMask] < r8)
		{
			foundPage = pageIndex;
			if ((r8 = f6C[subPagesInUseMask]) == 0)
				break;
		}
	}

	if (foundPage == kIndexNotFound)
		return kOSErrItemNotFound;

	*outPageIndex = foundPage;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Is specified subpage free?
	Args:		inPageIndex			index into the page table
				inSubPageIndex		subpage within the page
	Return:  error code
----------------------------------------------------------------------------- */

bool
CROMDomainManager1K::subPageFree(ArrayIndex inPageIndex, ArrayIndex inSubPageIndex)
{
	return fPageTable[inPageIndex].fPageAddr[inSubPageIndex] == 0xFFFF;
}


NewtonErr
CROMDomainManager1K::shuffleSubPages(ArrayIndex inFromPage, ArrayIndex inToPage, ULong inSubPageMask)
{
#if !defined(forFramework)
	NewtonErr err;
	ULong permissions = 0;	// 2 bits per subpage
	ULong pageAddr;
	ULong subPageMask = inSubPageMask;
	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex, subPageMask >>= 1)
	{
		if (subPageMask & 0x01)
		{
			// we have a subpage -- remember its details
			PageTableChunk * pageTableEntry = &fPageTable[inFromPage];
			pageAddr = pageTableEntry->fPageAddr[subPageIndex];
			Perm pagePerms = pageTableEntry->fIsWritable[subPageIndex] ? kReadWrite : kReadOnly;
			permissions |= (pagePerms << (subPageIndex*2));
			// clear it
			pageTableEntry->fPageAddr[subPageIndex] = 0xFFFF;
			pageTableEntry->fIsWritable[subPageIndex] = false;
			fNumOfPageTableEntries--;
		}
	}

	// move the page
	EnterAtomic();
	GenericSWI(k72GenericSWI, fROMBase + pageAddr*kPageSize);	// -> CleanPageinDC
	err = copyPhysPg(fPageTable[inToPage].fId, fPageTable[inFromPage].fId, inSubPageMask);
	ExitAtomic();

	if (err == noErr)
	{
		err = remember(fROMBase + pageAddr*kPageSize, permissions, fPageTable[inToPage].fId, true);
		if (err)
			ThrowErr(exAbort, err);
		addPageTableEntry(inToPage, pageAddr, permissions);
	}
	return err;
#else
	return noErr;
#endif
}


/* -----------------------------------------------------------------------------
	Free subpages from a page.
	Args:		inPageIndex			index into the page table
				inSubPageMask		subpages to free within the page
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::freeSubPages(ArrayIndex inPageIndex, ULong inSubPageMask)
{
	NewtonErr	err = noErr;
	ULong			permissions = 0;	// 2 bits per subpage
	ULong			mask = inSubPageMask;
	PageTableChunk *	pageTableEntry = &fPageTable[inPageIndex];
	ULong			pageAddr;	// page offset from fROMBase

	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex, mask >>= 1)
	{
		pageAddr = pageTableEntry->fPageAddr[subPageIndex];
		if ((mask & 0x0001) != 0
		&&  pageAddr != 0xFFFF)
		{
#if !defined(forFramework)
			collect(&permissions, inSubPageMask, subPageIndex, pageTableEntry);
			if (permissions != 0)
				remember(fROMBase + pageAddr*kPageSize, permissions, pageTableEntry->fId, true);
			else
				forget(fROMBase + pageAddr*kPageSize, pageTableEntry->fId);
#endif
		}
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Free subpages from some page, any page.
	Args:		outPageIndex		index into the page table of subpages we freed
				inSubPageMask		subpages to free within the page
	Return:  error code
----------------------------------------------------------------------------- */

NewtonErr
CROMDomainManager1K::freeAnySubPages(ArrayIndex * outPageNum, ULong inSubPageMask)
{
	// start looking into the page table at a random point
	CTime			now(GetGlobalTime());
	ArrayIndex	index = ((ULong)now + fFreeIndex) % fNumOfPages;
	ArrayIndex	firstIndex = index;
	fFreeIndex += 17;
	// must have a valid page
	while (!isValidPage(&fPageTable[index]))
	{
		if (++index == fNumOfPages)
			index = 0;						// wrap around
		if (index == firstIndex)		// back to where we started
			return kOSErrNoMemory;
	}
	*outPageNum = index;
	freeSubPages(index, inSubPageMask);

	return noErr;
}


#pragma mark -
/*------------------------------------------------------------------------------
	P e r m i s s i o n s
	enum Perm => 2 bits per subpage
	so we only use a byte of the ULong type specified for permissions
------------------------------------------------------------------------------*/

/* -----------------------------------------------------------------------------
	Make a permissions word for a subpage.
	Args:		inPageIndex			index into the page table
				inSubPageIndex		subpage
				inSubPageOnly		true => make permission for this subpage only
										false => make permission for all subpages
	Return:  permissions word
----------------------------------------------------------------------------- */

ULong
CROMDomainManager1K::makePermissions(ArrayIndex inPageIndex, ArrayIndex inSubPageIndex, bool inArg3)
{
	ULong					permissions = 0;
	PageTableChunk *	pageTableEntry = &fPageTable[inPageIndex];
	UShort				pageAddr = pageTableEntry->fPageAddr[inSubPageIndex];

	if (!inArg3)
		inSubPageIndex = kIndexNotFound;

	for (ArrayIndex subPageIndex = 0; subPageIndex < kSubPagesPerPage; ++subPageIndex)
	{
		if (pageTableEntry->fPageAddr[subPageIndex] == pageAddr
		&&  subPageIndex != inSubPageIndex)
			permissions |= pageTableEntry->fIsWritable[subPageIndex] ? (kReadWrite << (subPageIndex*2)) : (kReadOnly << (subPageIndex*2));
	}

	return permissions;
}
