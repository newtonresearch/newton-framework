/*
	File:		PackageStore.cc

	Contains:	Newton package store interface.
					The package store contains read-only data
					and is also known as a store part.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "PackageStore.h"
#include "PartHandler.h"
#include "Unicode.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

Ref gPackageStores;	//	package stores [array]


#pragma mark Initialisation
/*------------------------------------------------------------------------------
	I n i t i a l i s a t i o n
------------------------------------------------------------------------------*/

/* -----------------------------------------------------------------------------
	Initialise package stores array and prepare to receive package store parts.
	Args:		--
	Return	--
----------------------------------------------------------------------------- */

void
InitPackageSoups(void)
{
	gPackageStores = MakeArray(0);
	AddGCRoot(&gPackageStores);

	CPackageStore::classInfo()->registerProtocol();
#if 0
	CPackageStorePartHandler pkgStorePartHandler;
	pkgStorePartHandler.init('soup');
#endif
}


#pragma mark Plain C
/*------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FGetPackageStores(RefArg inRcvr);
Ref	FGetPackageStore(RefArg inRcvr, RefArg inName);
}


/* -----------------------------------------------------------------------------
	Return all stores loaded from packages.
	Args:		inRcvr
	Return	array ref
----------------------------------------------------------------------------- */

Ref
FGetPackageStores(RefArg inRcvr)
{
	return gPackageStores;
}


/* -----------------------------------------------------------------------------
	Return package store with the given name.
	Args:		inRcvr
				inName
	Return	store frame ref
				nil => no store with that name
----------------------------------------------------------------------------- */

Ref
FGetPackageStore(RefArg inRcvr, RefArg inName)
{
	RefVar	store;
	RefVar	storeName;
	for (ArrayIndex i = 0, count = Length(gPackageStores); i < count; ++i)
	{
		store = GetArraySlot(gPackageStores, i);
		storeName = StoreGetName(store);
		if (CompareStringNoCase(GetUString(storeName), GetUString(inName)) == 0)
			return store;
	}
	return NILREF;
}


#pragma mark -
#pragma mark CPackageStore
/*------------------------------------------------------------------------------
	C P a c k a g e S t o r e
	A very simple store implementation.
	The store is simply a block of memory within a package.
------------------------------------------------------------------------------*/
/* ----------------------------------------------------------------
	CPackageStore implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CPackageStore::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN13CPackageStore6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN13CPackageStore4makeEv - 0b	\n"
"		.long		__ZN13CPackageStore7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CPackageStore\"	\n"
"2:	.asciz	\"CStore\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN13CPackageStore9classInfoEv - 4b	\n"
"		.long		__ZN13CPackageStore4makeEv - 4b	\n"
"		.long		__ZN13CPackageStore7destroyEv - 4b	\n"
"		.long		__ZN13CPackageStore4initEPvmjjjS0_ - 4b	\n"
"		.long		__ZN13CPackageStore11needsFormatEPb - 4b	\n"
"		.long		__ZN13CPackageStore6formatEv - 4b	\n"
"		.long		__ZN13CPackageStore9getRootIdEPj - 4b	\n"
"		.long		__ZN13CPackageStore9newObjectEPjm - 4b	\n"
"		.long		__ZN13CPackageStore11eraseObjectEj - 4b	\n"
"		.long		__ZN13CPackageStore12deleteObjectEj - 4b	\n"
"		.long		__ZN13CPackageStore13setObjectSizeEjm - 4b	\n"
"		.long		__ZN13CPackageStore13getObjectSizeEjPm - 4b	\n"
"		.long		__ZN13CPackageStore5writeEjmPvm - 4b	\n"
"		.long		__ZN13CPackageStore4readEjmPvm - 4b	\n"
"		.long		__ZN13CPackageStore12getStoreSizeEPmS0_ - 4b	\n"
"		.long		__ZN13CPackageStore10isReadOnlyEPb - 4b	\n"
"		.long		__ZN13CPackageStore9lockStoreEv - 4b	\n"
"		.long		__ZN13CPackageStore11unlockStoreEv - 4b	\n"
"		.long		__ZN13CPackageStore5abortEv - 4b	\n"
"		.long		__ZN13CPackageStore4idleEPbS0_ - 4b	\n"
"		.long		__ZN13CPackageStore10nextObjectEjPj - 4b	\n"
"		.long		__ZN13CPackageStore14checkIntegrityEPj - 4b	\n"
"		.long		__ZN13CPackageStore8setBuddyEP6CStore - 4b	\n"
"		.long		__ZN13CPackageStore10ownsObjectEj - 4b	\n"
"		.long		__ZN13CPackageStore7addressEj - 4b	\n"
"		.long		__ZN13CPackageStore9storeKindEv - 4b	\n"
"		.long		__ZN13CPackageStore8setStoreEP6CStorej - 4b	\n"
"		.long		__ZN13CPackageStore11isSameStoreEPvm - 4b	\n"
"		.long		__ZN13CPackageStore8isLockedEv - 4b	\n"
"		.long		__ZN13CPackageStore5isROMEv - 4b	\n"
"		.long		__ZN13CPackageStore6vppOffEv - 4b	\n"
"		.long		__ZN13CPackageStore5sleepEv - 4b	\n"
"		.long		__ZN13CPackageStore20newWithinTransactionEPjm - 4b	\n"
"		.long		__ZN13CPackageStore23startTransactionAgainstEj - 4b	\n"
"		.long		__ZN13CPackageStore15separatelyAbortEj - 4b	\n"
"		.long		__ZN13CPackageStore23addToCurrentTransactionEj - 4b	\n"
"		.long		__ZN13CPackageStore21inSeparateTransactionEj - 4b	\n"
"		.long		__ZN13CPackageStore12lockReadOnlyEv - 4b	\n"
"		.long		__ZN13CPackageStore14unlockReadOnlyEb - 4b	\n"
"		.long		__ZN13CPackageStore13inTransactionEv - 4b	\n"
"		.long		__ZN13CPackageStore9newObjectEPjPvm - 4b	\n"
"		.long		__ZN13CPackageStore13replaceObjectEjPvm - 4b	\n"
"		.long		__ZN13CPackageStore17calcXIPObjectSizeEllPl - 4b	\n"
"		.long		__ZN13CPackageStore12newXIPObjectEPjm - 4b	\n"
"		.long		__ZN13CPackageStore16getXIPObjectInfoEjPmS0_S0_ - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CPackageStore)

/* -----------------------------------------------------------------------------
	Make an empty package store.
	Args:		--
	Return	--
----------------------------------------------------------------------------- */

CPackageStore *
CPackageStore::make(void)
{
	fStoreData = NULL;
	fStoreSize = 0;
	fLockCounter = 0;
	return this;
}


/* -----------------------------------------------------------------------------
	Destroy the package store.
	Nothing to do -- it’s read-only.
	Args:		--
	Return	--
----------------------------------------------------------------------------- */

void
CPackageStore::destroy(void)
{ }


/* -----------------------------------------------------------------------------
	Initialize the package store.
	It has no physical (hardware( attributes -- simply a block of memory.
	Args:		inStoreData
				inStoreSize
				inArg3
				inSocketNumber
				inFlags
				inArg6
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::init(void * inStoreData, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inArg6)
{
	fStoreData = (PackageStoreData *)inStoreData;
	fStoreSize = inStoreSize;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Does this store require formatting?.
	Never -- it’s read-only.
	Args:		outNeedsFormat
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::needsFormat(bool * outNeedsFormat)
{
	*outNeedsFormat = NO;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Format the package store.
	Cannot be done -- it’s read-only.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::format(void)
{
	return kStoreErrWriteProtected;
}


/* -----------------------------------------------------------------------------
	Check the integrity of the store.
	What can possibly go wrong -- it’s read-only.
	Args:		inArg1
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::checkIntegrity(ULong * inArg1)
{
	return noErr;
}


#pragma mark Store info
/* -----------------------------------------------------------------------------
	Return the store’s root object id.
	Args:		outRootId
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::getRootId(PSSId * outRootId)
{
	*outRootId = fStoreData->rootId;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Return the store size.
	Args:		outTotalSize
				outUsedSize
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::getStoreSize(size_t * outTotalSize, size_t * outUsedSize)
{
	*outTotalSize = fStoreSize;
	*outUsedSize = fStoreSize;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Return the kind of store.
	Args:		--
	Return	C string
----------------------------------------------------------------------------- */

const char *
CPackageStore::storeKind(void)
{
	return "Package";
}


/* -----------------------------------------------------------------------------
	Is this store read-only?
	Hell, yeah!
	Args:		outIsReadOnly
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::isReadOnly(bool * outIsReadOnly)
{
	*outIsReadOnly = YES;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Is this store in ROM?
	Well, yes, kinda.
	Args:		--
	Return	YES => it’s in ROM
----------------------------------------------------------------------------- */

bool
CPackageStore::isROM(void)
{
	return YES;
}


/* -----------------------------------------------------------------------------
	Lock the store.
	Locking REQUIRES an implementation.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::lockStore(void)
{
	fLockCounter++;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Unlock the store.
	Locking REQUIRES an implementation.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::unlockStore(void)
{
	fLockCounter--;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Is this store locked?
	Locking REQUIRES an implementation.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

bool
CPackageStore::isLocked(void)
{
	return fLockCounter != 0;
}


#pragma mark MuxStore
/* -----------------------------------------------------------------------------
	Set the store’s buddy.
	Args:		inStore
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::setBuddy(CStore * inStore)
{
	return noErr;
}


/* -----------------------------------------------------------------------------
	Set the store.
	Args:		inStore
				inEnvironment
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::setStore(CStore * inStore, ObjectId inEnvironment)
{
	return noErr;
}


/* -----------------------------------------------------------------------------
	Is this the same store?
	Args:		inData
				inSize
	Return	YEs => is the same store
----------------------------------------------------------------------------- */

bool
CPackageStore::isSameStore(void * inData, size_t inSize)
{
	return noErr;
}


#pragma mark Object access
/* -----------------------------------------------------------------------------
	Create a new object in the store.
	Args:		outObjectId
				inSize
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::newObject(PSSId * outObjectId, size_t inSize)
{
	return kStoreErrWriteProtected;
}


/* -----------------------------------------------------------------------------
	Erase an object from the store.
	Args:		inObjectId
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::eraseObject(PSSId inObjectId)
{
	return kStoreErrWriteProtected;
}


/* -----------------------------------------------------------------------------
	Delete an object from the store.
	Args:		inObjectId
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::deleteObject(PSSId inObjectId)
{
	return kStoreErrWriteProtected;
}


/* -----------------------------------------------------------------------------
	Set the size of an object in the store.
	Args:		inObjectId
				inSize
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::setObjectSize(PSSId inObjectId, size_t inSize)
{
	return kStoreErrWriteProtected;
}


/* -----------------------------------------------------------------------------
	Return the size of an object in the store.
	Args:		inObjectId
				outSize
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::getObjectSize(PSSId inObjectId, size_t * outSize)
{
	if (inObjectId > fStoreData->numOfObjects)
		return kStoreErrBadPSSId;

	*outSize = fStoreData->offsetToObject[inObjectId+1]
				- fStoreData->offsetToObject[inObjectId];
	return noErr;
}


/* -----------------------------------------------------------------------------
	Return the id of the next object in the store.
	N/A for this store.
	Args:		inObjectId
				outNextObjectId
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::nextObject(PSSId inObjectId, PSSId * outNextObjectId)
{
	*outNextObjectId = 0;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Does this store own the given object?
	Args:		inObjectId
	Return	YES => store owns the object
----------------------------------------------------------------------------- */

bool
CPackageStore::ownsObject(PSSId inObjectId)
{
	return YES;
}


/* -----------------------------------------------------------------------------
	Return the address of an object in the store.
	Args:		inObjectId
	Return	address
----------------------------------------------------------------------------- */

VAddr
CPackageStore::address(PSSId inObjectId)
{
	return 0;
}


/* -----------------------------------------------------------------------------
	Read an object in the store.
	Args:		inObjectId			object’s id
				inStartOffset		offset into object
				outBuffer			buffer to receive object data
				inLength				size of object to read -- may not be whole object
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength)
{
	if (inObjectId > fStoreData->numOfObjects)
		return kStoreErrBadPSSId;

	size_t objSize = fStoreData->offsetToObject[inObjectId+1]
						- fStoreData->offsetToObject[inObjectId];
	if (inStartOffset > objSize)
		return kStoreErrObjectOverRun;

	NewtonErr err = noErr;
	if (inStartOffset + inLength > objSize)
	{
		err = kStoreErrObjectOverRun;		// but read what we can anyway
		inLength = objSize - inStartOffset;
	}
	memmove(outBuffer, (char *)fStoreData + fStoreData->offsetToObject[inObjectId] + inStartOffset, inLength);
	return err;
}


/* -----------------------------------------------------------------------------
	Write an object to the store.
	Args:		inObjectId			object’s id
				inStartOffset		offset into buffer for object data
				inBuffer				object data
				inLength				size of object data
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength)
{
	return kStoreErrWriteProtected;
}


#pragma mark Not in the jump table
NewtonErr
CPackageStore::newObject(PSSId * outObjectId, void * inData, size_t inSize)
{
	return kStoreErrWriteProtected;
}

NewtonErr
CPackageStore::replaceObject(PSSId inObjectId, void * inData, size_t inSize)
{
	return kStoreErrWriteProtected;
}

NewtonErr
CPackageStore::newWithinTransaction(PSSId * outObjectId, size_t inSize)
{
	return newObject(outObjectId, inSize);
}

NewtonErr
CPackageStore::startTransactionAgainst(PSSId inObjectId)
{
	return noErr;
}

NewtonErr
CPackageStore::addToCurrentTransaction(PSSId inObjectId)
{
	return noErr;
}

NewtonErr
CPackageStore::abort(void)
{
	fLockCounter = 0;
	return noErr;
}

NewtonErr
CPackageStore::separatelyAbort(PSSId inObjectId)
{
	return noErr;
}

bool
CPackageStore::inTransaction(void)
{
	return isLocked();
}

bool
CPackageStore::inSeparateTransaction(PSSId inObjectId)
{
	return NO;
}


NewtonErr
CPackageStore::lockReadOnly(void)
{
	return noErr;
}


NewtonErr
CPackageStore::unlockReadOnly(bool inReset)
{
	return noErr;
}


#pragma mark eXecute-In-Place
/* -----------------------------------------------------------------------------
	eXecute-In-Place methods.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::newXIPObject(PSSId * outObjectId, size_t inSize)
{
	return kOSErrXIPNotPossible;
}

NewtonErr
CPackageStore::calcXIPObjectSize(long inArg1, long inArg2, long * outArg3)
{
	return kOSErrXIPNotPossible;
}

NewtonErr
CPackageStore::getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4)
{
	return kOSErrXIPNotPossible;
}


#pragma mark Power management
/* -----------------------------------------------------------------------------
	Idle the store.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::idle(bool * outArg1, bool * outArg2)
{
	*outArg1 = NO;
	*outArg2 = NO;
	return noErr;
}


/* -----------------------------------------------------------------------------
	Send the store to sleep.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::sleep(void)
{
	return noErr;
}


/* -----------------------------------------------------------------------------
	Power off the card.
	Args:		--
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStore::vppOff(void)
{
	return noErr;
}

#if 0
#pragma mark -
#pragma mark CPackageStorePartHandler
/*------------------------------------------------------------------------------
	C P a c k a g e S t o r e P a r t H a n d l e r
------------------------------------------------------------------------------*/
#include "PartHandler.h"
#include "ROMSymbols.h"
#include "Stores.h"


CPackageStorePartHandler::CPackageStorePartHandler()
{ }


/* -----------------------------------------------------------------------------
	Install the package store part.
	Args:		inPartId
				inType
				info
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStorePartHandler::install(const PartId & inPartId, SourceType inType, PartInfo * info)
{
	NewtonErr err;

	XTRY
	{
		// make a CPackageStore
		CStore * store;
		XFAILNOT(store = (CStore *)MakeByName("CStore", "CPackageStore"), err = kOSErrNoMemory;)
		XFAILIF(IsStream(inType), err = kOSErrBadPackage;)						// v
		// init the store with package data
		XFAIL(err = store->init(getSourcePtr(), info->size, 0, 0, 0, 0))	// on failure, shouldn’t we be destroying the store?

		newton_try
		{
			// add a store object to the global array
			AddArraySlot(gPackageStores, MakeStoreObject(store));
		}
		newton_catch(exRootException)
		{
			store->destroy();
			err = kOSErrBadPackage;
		}
		end_try;

		if (err == noErr)
			setRemoveObjPtr((RemoveObjPtr)store);
	}
	XENDTRY;

	return err;
}


/* -----------------------------------------------------------------------------
	Remove the package store part.
	Args:		inPartId
				inType
				inContext
	Return	error code
----------------------------------------------------------------------------- */

NewtonErr
CPackageStorePartHandler::remove(const PartId & inPartId, PartType inType, RemoveObjPtr inContext)
{
	RefVar storeObject;
	CStoreWrapper * storeWrapper = NULL;
	for (ArrayIndex i = 0, count = Length(gPackageStores); i < count; ++i)
	{
		storeObject = GetArraySlot(gPackageStores, i);
		storeWrapper = (CStoreWrapper *)GetFrameSlot(storeObject, SYMA(store));
		if (storeWrapper->store() == (CStore *)inContext)
			break;
	}
	if (i == count)
		// we looked at every package store but we didn’t find the one we’re after
		return -10400;

	// remove store from global array
	ArrayMunger(gPackageStores, i, 1, RA(NILREF), 0, 0);
	// kill the store
	KillStoreObject(storeObject);
	if (storeWrapper)
		delete storeWrapper;

	return noErr;
}
#endif
