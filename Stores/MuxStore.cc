/*
	File:		MuxStore.cc

	Contains:	Newton mux store interface.

	Written by:	Newton Research Group.
*/

#include "MuxStore.h"
#include "VirtualMemory.h"

// Most of the CMuxStore wrappers just need to lock ivar access
// -- but hold on, isn’t this what the monitor is supposed to do?
// #beltandbraces

#if defined(forFramework)
#define SYNCHRONIZE
#define END_SYNCHRONIZE

#else
#define SYNCHRONIZE \
	fLock->acquire(kWaitOnBlock); \
	unwind_protect
#define END_SYNCHRONIZE \
	on_unwind \
		fLock->release(); \
	end_unwind

#endif


#pragma mark Capability
/* -----------------------------------------------------------------------------
	C l a s s   C a p a b i l i t y
----------------------------------------------------------------------------- */

const CClassInfo *
GetStoreClassInfo(const CStore * inStore)
{
	const CClassInfo * storeClassInfo = inStore->classInfo();
	return (storeClassInfo == CMuxStore::classInfo()) ? ((CMuxStore *)inStore)->getStore()->classInfo() : storeClassInfo;
}


bool
CanCreateLargeObjectsOnStore(CStore * inStore)
{
	const CClassInfo * classInfo = GetStoreClassInfo(inStore);
	return classInfo->getCapability('LOBJ') != NULL;
}


#pragma mark CMuxStore
/* -----------------------------------------------------------------------------
	C M u x S t o r e
----------------------------------------------------------------------------- */

/* ----------------------------------------------------------------
	CMuxStore implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CMuxStore::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN9CMuxStore6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN9CMuxStore4makeEv - 0b	\n"
"		.long		__ZN9CMuxStore7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CMuxStore\"	\n"
"2:	.asciz	\"CStore\"	\n"
"3:	.asciz	\"LOBJ\", \"\" \n"
"		.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN9CMuxStore9classInfoEv - 4b	\n"
"		.long		__ZN9CMuxStore4makeEv - 4b	\n"
"		.long		__ZN9CMuxStore7destroyEv - 4b	\n"
"		.long		__ZN9CMuxStore4initEPvmjjjS0_ - 4b	\n"
"		.long		__ZN9CMuxStore11needsFormatEPb - 4b	\n"
"		.long		__ZN9CMuxStore6formatEv - 4b	\n"
"		.long		__ZN9CMuxStore9getRootIdEPj - 4b	\n"
"		.long		__ZN9CMuxStore9newObjectEPjm - 4b	\n"
"		.long		__ZN9CMuxStore11eraseObjectEj - 4b	\n"
"		.long		__ZN9CMuxStore12deleteObjectEj - 4b	\n"
"		.long		__ZN9CMuxStore13setObjectSizeEjm - 4b	\n"
"		.long		__ZN9CMuxStore13getObjectSizeEjPm - 4b	\n"
"		.long		__ZN9CMuxStore5writeEjmPvm - 4b	\n"
"		.long		__ZN9CMuxStore4readEjmPvm - 4b	\n"
"		.long		__ZN9CMuxStore12getStoreSizeEPmS0_ - 4b	\n"
"		.long		__ZN9CMuxStore10isReadOnlyEPb - 4b	\n"
"		.long		__ZN9CMuxStore9lockStoreEv - 4b	\n"
"		.long		__ZN9CMuxStore11unlockStoreEv - 4b	\n"
"		.long		__ZN9CMuxStore5abortEv - 4b	\n"
"		.long		__ZN9CMuxStore4idleEPbS0_ - 4b	\n"
"		.long		__ZN9CMuxStore10nextObjectEjPj - 4b	\n"
"		.long		__ZN9CMuxStore14checkIntegrityEPj - 4b	\n"
"		.long		__ZN9CMuxStore8setBuddyEP6CStore - 4b	\n"
"		.long		__ZN9CMuxStore10ownsObjectEj - 4b	\n"
"		.long		__ZN9CMuxStore7addressEj - 4b	\n"
"		.long		__ZN9CMuxStore9storeKindEv - 4b	\n"
"		.long		__ZN9CMuxStore8setStoreEP6CStorej - 4b	\n"
"		.long		__ZN9CMuxStore11isSameStoreEPvm - 4b	\n"
"		.long		__ZN9CMuxStore8isLockedEv - 4b	\n"
"		.long		__ZN9CMuxStore5isROMEv - 4b	\n"
"		.long		__ZN9CMuxStore6vppOffEv - 4b	\n"
"		.long		__ZN9CMuxStore5sleepEv - 4b	\n"
"		.long		__ZN9CMuxStore20newWithinTransactionEPjm - 4b	\n"
"		.long		__ZN9CMuxStore23startTransactionAgainstEj - 4b	\n"
"		.long		__ZN9CMuxStore15separatelyAbortEj - 4b	\n"
"		.long		__ZN9CMuxStore23addToCurrentTransactionEj - 4b	\n"
"		.long		__ZN9CMuxStore21inSeparateTransactionEj - 4b	\n"
"		.long		__ZN9CMuxStore12lockReadOnlyEv - 4b	\n"
"		.long		__ZN9CMuxStore14unlockReadOnlyEb - 4b	\n"
"		.long		__ZN9CMuxStore13inTransactionEv - 4b	\n"
"		.long		__ZN9CMuxStore9newObjectEPjPvm - 4b	\n"
"		.long		__ZN9CMuxStore13replaceObjectEjPvm - 4b	\n"
"		.long		__ZN9CMuxStore17calcXIPObjectSizeEllPl - 4b	\n"
"		.long		__ZN9CMuxStore12newXIPObjectEPjm - 4b	\n"
"		.long		__ZN9CMuxStore16getXIPObjectInfoEjPmS0_S0_ - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CMuxStore)

/* -----------------------------------------------------------------------------
	Make a CMuxStore.
	Args:		--
	Return:	new instance
----------------------------------------------------------------------------- */

CMuxStore *
CMuxStore::make(void)
{
	XTRY
	{
		fStore = NULL;
		fMonitor = NULL;
#if defined(forFramework)
		fLock = NULL;
#else
		fLock = new CULockingSemaphore;
		XFAIL(fLock == NULL)
		XFAILIF(fLock->init() != noErr, delete fLock; fLock = NULL;)
#endif
	}
	XENDTRY;
	return this;
}


/* -----------------------------------------------------------------------------
	Destroy a CMuxStore.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CMuxStore::destroy(void)
{
	if (fMonitor != NULL)
		delete fMonitor, fMonitor = NULL;
	if (fStore != NULL)
		delete fStore, fStore = NULL;
#if !defined(forFramework)
	if (fLock != NULL)
		delete fLock, fLock = NULL;
#endif
}


/* -----------------------------------------------------------------------------
	The following member functions implement the CMuxStore protocol.
	They use a semaphore to serialise access to the wrapped CStore.
----------------------------------------------------------------------------- */

#pragma mark Initialisation

NewtonErr
CMuxStore::init(void * inStoreData, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inArg6)
{
	return noErr;
}


NewtonErr
CMuxStore::needsFormat(bool * outNeedsFormat)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->needsFormat(outNeedsFormat);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::format(void)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->format();
	END_SYNCHRONIZE;
	return err;
}

#pragma mark Object access

NewtonErr
CMuxStore::getRootId(PSSId * outRootId)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fStore->getRootId(outRootId);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::newObject(PSSId * outObjectId, size_t inSize)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->newObject(outObjectId, inSize);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::eraseObject(PSSId inObjectId)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->eraseObject(inObjectId);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::deleteObject(PSSId inObjectId)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->deleteObject(inObjectId);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::setObjectSize(PSSId inObjectId, size_t inSize)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->setObjectSize(inObjectId, inSize);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::getObjectSize(PSSId inObjectId, size_t * outSize)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fStore->getObjectSize(inObjectId, outSize);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->write(inObjectId, inStartOffset, inBuffer, inLength);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fStore->read(inObjectId, inStartOffset, outBuffer, inLength);
	END_SYNCHRONIZE;
	return err;
}

#pragma mark Store info

NewtonErr
CMuxStore::getStoreSize(size_t * outTotalSize, size_t * outUsedSize)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->getStoreSize(outTotalSize, outUsedSize);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::isReadOnly(bool * outIsReadOnly)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fStore->isReadOnly(outIsReadOnly);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::lockStore(void)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->lockStore();
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::unlockStore(void)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->unlockStore();
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::abort(void)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->abort();
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::idle(bool * outArg1, bool * outArg2)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->idle(outArg1, outArg2);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::nextObject(PSSId inObjectId, PSSId * outNextObjectId)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fStore->nextObject(inObjectId, outNextObjectId);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::checkIntegrity(ULong * inArg1)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->checkIntegrity(inArg1);
	END_SYNCHRONIZE;
	return err;
}

#pragma mark MuxStore

NewtonErr
CMuxStore::setBuddy(CStore * inStore)
{
	return fStore->setBuddy(inStore);
}


bool
CMuxStore::ownsObject(PSSId inObjectId)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fStore->ownsObject(inObjectId);
	END_SYNCHRONIZE;
	return err;
}


VAddr
CMuxStore::address(PSSId inObjectId)
{
	return fStore->address(inObjectId);
}


const char *
CMuxStore::storeKind(void)
{
	return fStore->storeKind();
}


NewtonErr
CMuxStore::setStore(CStore * inStore, PSSId inEnvironment)
{
	NewtonErr err = noErr;
	XTRY
	{
		fStore = inStore;
		XFAILNOT(fMonitor = (CMuxStoreMonitor *)CStoreMonitor::make("CMuxStoreMonitor"), err = MemError();)
#if !defined(forFramework)
		XFAIL(err = fMonitor->startMonitor(1*KByte, inEnvironment, 'Cmux', NO))
#endif
		err = fMonitor->init(fStore);
	}
	XENDTRY;
	return err;
}


bool
CMuxStore::isSameStore(void * inData, size_t inSize)
{
	return fStore->isSameStore(inData, inSize);
}

bool
CMuxStore::isLocked(void)
{
	return fStore->isLocked();
}


bool
CMuxStore::isROM(void)
{
	return fStore->isROM();
}

#pragma mark Power management

NewtonErr
CMuxStore::vppOff(void)
{
	return fStore->vppOff();
}


NewtonErr
CMuxStore::sleep(void)
{
	return fStore->sleep();
}

#pragma mark Transactions

NewtonErr
CMuxStore::newWithinTransaction(PSSId * outObjectId, size_t inSize)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->newWithinTransaction(outObjectId, inSize);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::startTransactionAgainst(PSSId inObjectId)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->startTransactionAgainst(inObjectId);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::separatelyAbort(PSSId inObjectId)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->separatelyAbort(inObjectId);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::addToCurrentTransaction(PSSId inObjectId)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->addToCurrentTransaction(inObjectId);
	END_SYNCHRONIZE;
	return err;
}


bool
CMuxStore::inSeparateTransaction(PSSId inObjectId)
{
	return fStore->inSeparateTransaction(inObjectId);
}


NewtonErr
CMuxStore::lockReadOnly(void)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->lockReadOnly();
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::unlockReadOnly(bool inReset)
{
	return fStore->unlockReadOnly(inReset);
}


bool
CMuxStore::inTransaction(void)
{
	return fStore->inTransaction();
}

#pragma mark Not in the jump table

NewtonErr
CMuxStore::newObject(PSSId * outObjectId, void * inData, size_t inSize)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->newObject(outObjectId, inData, inSize);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::replaceObject(PSSId inObjectId, void * inData, size_t inSize)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->replaceObject(inObjectId, inData, inSize);
	END_SYNCHRONIZE;
	return err;
}

#pragma mark eXecute-In-Place

NewtonErr
CMuxStore::calcXIPObjectSize(long inArg1, long inArg2, long * outArg3)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fStore->calcXIPObjectSize(inArg1, inArg2, outArg3);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::newXIPObject(PSSId * outObjectId, size_t inSize)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fMonitor->newXIPObject(outObjectId, inSize);
	END_SYNCHRONIZE;
	return err;
}


NewtonErr
CMuxStore::getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4)
{
	NewtonErr err = noErr;
	SYNCHRONIZE
		err = fStore->getXIPObjectInfo(inObjectId, outArg2, outArg3, outArg4);
	END_SYNCHRONIZE;
	return err;
}


#pragma mark -
#pragma mark CMuxStoreMonitor
/*------------------------------------------------------------------------------
	C M u x S t o r e M o n i t o r
	The mux store monitor allows thread-safe access to a CStore.
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CMuxStoreMonitor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CMuxStoreMonitor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN16CMuxStoreMonitor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN16CMuxStoreMonitor4makeEv - 0b	\n"
"		.long		__ZN16CMuxStoreMonitor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CMuxStoreMonitor\"	\n"
"2:	.asciz	\"CStoreMonitor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN16CMuxStoreMonitor9classInfoEv - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor4makeEv - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor7destroyEv - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor4initEP6CStore - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor11needsFormatEPb - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor6formatEv - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor9getRootIdEPj - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor9newObjectEPjm - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor11eraseObjectEj - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor12deleteObjectEj - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor13setObjectSizeEjm - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor13getObjectSizeEjPm - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor5writeEjmPvm - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor4readEjmPvm - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor12getStoreSizeEPmS0_ - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor10isReadOnlyEPb - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor9lockStoreEv - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor11unlockStoreEv - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor5abortEv - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor4idleEPbS0_ - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor10nextObjectEjPj - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor14checkIntegrityEPj - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor20newWithinTransactionEPjm - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor23startTransactionAgainstEj - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor15separatelyAbortEj - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor23addToCurrentTransactionEj - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor12lockReadOnlyEv - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor14unlockReadOnlyEb - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor9newObjectEPjPvm - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor13replaceObjectEjPvm - 4b	\n"
"		.long		__ZN16CMuxStoreMonitor12newXIPObjectEPjm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CMuxStoreMonitor)

CMuxStoreMonitor *
CMuxStoreMonitor::make(void)
{
	fStore = NULL;
	return this;
}

void
CMuxStoreMonitor::destroy(void)
{ /* this really does nothing */ }

NewtonErr
CMuxStoreMonitor::init(CStore * inStore)
{
#if defined(correct)
	// lock a KByte of the monitor’s stack
	ULong here;
	VAddr start = TRUNC((VAddr)&here, KByte) - KByte;
	LockHeapRange(start, start + 2*KByte - 1);
#endif
	fStore = inStore;
	return noErr;
}

NewtonErr
CMuxStoreMonitor::needsFormat(bool * outNeedsFormat)
{ return fStore->needsFormat(outNeedsFormat); }

NewtonErr
CMuxStoreMonitor::format(void)
{ return fStore->format(); }

NewtonErr
CMuxStoreMonitor::getRootId(PSSId * outRootId)
{ return fStore->getRootId(outRootId); }

NewtonErr
CMuxStoreMonitor::newObject(PSSId * outObjectId, size_t inSize)
{ return fStore->newObject(outObjectId, inSize); }

NewtonErr
CMuxStoreMonitor::eraseObject(PSSId inObjectId)
{ return fStore->eraseObject(inObjectId); }

NewtonErr
CMuxStoreMonitor::deleteObject(PSSId inObjectId)
{ return fStore->deleteObject(inObjectId); }

NewtonErr
CMuxStoreMonitor::setObjectSize(PSSId inObjectId, size_t inSize)
{ return fStore->setObjectSize(inObjectId, inSize); }

NewtonErr
CMuxStoreMonitor::getObjectSize(PSSId inObjectId, size_t * outSize)
{ return fStore->getObjectSize(inObjectId, outSize); }

NewtonErr
CMuxStoreMonitor::write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength)
{ return fStore->write(inObjectId, inStartOffset, inBuffer, inLength); }

NewtonErr
CMuxStoreMonitor::read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength)
{ return fStore->read(inObjectId, inStartOffset, outBuffer, inLength); }

NewtonErr
CMuxStoreMonitor::getStoreSize(size_t * outTotalSize, size_t * outUsedSize)
{ return fStore->getStoreSize(outTotalSize, outUsedSize); }

NewtonErr
CMuxStoreMonitor::isReadOnly(bool * outIsReadOnly)
{ return fStore->isReadOnly(outIsReadOnly); }

NewtonErr
CMuxStoreMonitor::lockStore(void)
{ return fStore->lockStore(); }

NewtonErr
CMuxStoreMonitor::unlockStore(void)
{ return fStore->unlockStore(); }

NewtonErr
CMuxStoreMonitor::abort(void)
{ return fStore->abort(); }

NewtonErr
CMuxStoreMonitor::idle(bool * outArg1, bool * outArg2)
{ return fStore->idle(outArg1, outArg2); }

NewtonErr
CMuxStoreMonitor::nextObject(PSSId inObjectId, PSSId * outNextObjectId)
{ return fStore->nextObject(inObjectId, outNextObjectId); }

NewtonErr
CMuxStoreMonitor::checkIntegrity(ULong * inArg1)
{ return fStore->checkIntegrity(inArg1); }

NewtonErr
CMuxStoreMonitor::newWithinTransaction(PSSId * outObjectId, size_t inSize)
{ return fStore->newWithinTransaction(outObjectId, inSize); }

NewtonErr
CMuxStoreMonitor::startTransactionAgainst(PSSId inObjectId)
{ return fStore->startTransactionAgainst(inObjectId); }

NewtonErr
CMuxStoreMonitor::separatelyAbort(PSSId inObjectId)
{ return fStore->separatelyAbort(inObjectId); }

NewtonErr
CMuxStoreMonitor::addToCurrentTransaction(PSSId inObjectId)
{ return fStore->addToCurrentTransaction(inObjectId); }

NewtonErr
CMuxStoreMonitor::lockReadOnly(void)
{ return fStore->lockReadOnly(); }

NewtonErr
CMuxStoreMonitor::unlockReadOnly(bool inReset)
{ return fStore->unlockReadOnly(inReset); }

NewtonErr
CMuxStoreMonitor::newObject(PSSId * outObjectId, void * inData, size_t inSize)
{ return fStore->newObject(outObjectId, inData, inSize); }

NewtonErr
CMuxStoreMonitor::replaceObject(PSSId inObjectId, void * inData, size_t inSize)
{ return fStore->replaceObject(inObjectId, inData, inSize); }

NewtonErr
CMuxStoreMonitor::newXIPObject(PSSId * outObjectId, size_t inSize)
{ return fStore->newXIPObject(outObjectId, inSize); }

