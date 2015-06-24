/*
	File:		FlashStore.cc

	Contains:	CStore protocol implementation for Flash. And SRAM.

	Written by:	Newton Research Group, 2009.
*/
#undef forDebug
//#define forDebug 1
#define debugLevel 2

#include "LargeObjects.h"
#include "LargeObjectStore.h"
#include "Globals.h"
#include "RDM.h"
#include "FlashStore.h"
#include "FlashIterator.h"
#include "OSErrors.h"
extern void DumpHex(void * inBuf, size_t inLen);

extern CROMDomainManager1K * gROMStoreDomainManager;

extern size_t	InternalStoreInfo(int inSelector);


// these need initializing
size_t			g0C100DD4;
size_t			gInternalBlockSize;			// 0C100DD8
size_t			gMutableBlockSize;			// 0C100DDC
size_t			gInternalFlashStoreSlop;	// 0C100DE0

SCompactState	g0C106324;		// can do this because they have no ctors
CStoreDriver	g0C106358;


// belongs in PSSManager.h

struct PSSStoreInfo
{
	void				clear(void);

	CUAsyncMessage	f00;
	CStore *			f10;
	size_t			f14;
	int				f18;
	void *			f1C;
	CCardHandler *	f28;
	CFlash *			f2C;
	ULong				f30;		// type/signature
	bool				f38;
	bool				f39;
	ULong				f3C;
	CStore *			f40;
// size +50
};

#if defined(forFramework)
/*------------------------------------------------------------------------------
	S t u b s
------------------------------------------------------------------------------*/

void Sleep(Timeout inTimeout) { }
#endif


/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

bool
IsValidPSSId(PSSId inId)
{
	return inId != 0
		&& (inId & (inId - 1)) != 0
		&&  inId != 0x10000000
		&& (~inId & (~inId - 1)) != 0;
}


/*------------------------------------------------------------------------------
	Return the closest integer value greater than the base 2 log of the
	input parameter.
	Args:		inX
	Return:	base 2 log
------------------------------------------------------------------------------*/

ULong
CeilLog2(ULong inX)
{
	ULong log2x = 0;
	for (ULong x = inX; x > 1; x >>= 1)
		log2x++;
	if (inX > (1 << log2x))
		log2x++;
	return log2x;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C F l a s h S t o r e
------------------------------------------------------------------------------*/

/* Translate block number to address/offset */
#define BLOCK(_b) (_b * fBlockSize)
/* Translate address/offset to block number */
#define BLOCK_NUMBER(_a) (_a >> fBlockSizeShift)
/* Iterate over all blocks in the store */
#define FOREACH_BLOCK(_b) \
	for (ZAddr _addr = 0; _addr < storeCapacity(); _addr += fBlockSize) \
	{ \
		CFlashBlock * _b = blockForAddr(_addr);
#define END_FOREACH_BLOCK \
	}


#pragma mark -
/* ----------------------------------------------------------------
	CFlashStore implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CFlashStore::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN11CFlashStore6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN11CFlashStore4makeEv - 0b	\n"
"		.long		__ZN11CFlashStore7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CFlashStore\"	\n"
"2:	.asciz	\"CStore\"	\n"
"3:	.asciz	\"LOBJ\", \"\"	\n"
"		.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN11CFlashStore9classInfoEv - 4b	\n"
"		.long		__ZN11CFlashStore4makeEv - 4b	\n"
"		.long		__ZN11CFlashStore7destroyEv - 4b	\n"
"		.long		__ZN11CFlashStore4initEPvmjjjS0_ - 4b	\n"
"		.long		__ZN11CFlashStore11needsFormatEPb - 4b	\n"
"		.long		__ZN11CFlashStore6formatEv - 4b	\n"
"		.long		__ZN11CFlashStore9getRootIdEPj - 4b	\n"
"		.long		__ZN11CFlashStore9newObjectEPjm - 4b	\n"
"		.long		__ZN11CFlashStore11eraseObjectEj - 4b	\n"
"		.long		__ZN11CFlashStore12deleteObjectEj - 4b	\n"
"		.long		__ZN11CFlashStore13setObjectSizeEjm - 4b	\n"
"		.long		__ZN11CFlashStore13getObjectSizeEjPm - 4b	\n"
"		.long		__ZN11CFlashStore5writeEjmPvm - 4b	\n"
"		.long		__ZN11CFlashStore4readEjmPvm - 4b	\n"
"		.long		__ZN11CFlashStore12getStoreSizeEPmS0_ - 4b	\n"
"		.long		__ZN11CFlashStore10isReadOnlyEPb - 4b	\n"
"		.long		__ZN11CFlashStore9lockStoreEv - 4b	\n"
"		.long		__ZN11CFlashStore11unlockStoreEv - 4b	\n"
"		.long		__ZN11CFlashStore5abortEv - 4b	\n"
"		.long		__ZN11CFlashStore4idleEPbS0_ - 4b	\n"
"		.long		__ZN11CFlashStore10nextObjectEjPj - 4b	\n"
"		.long		__ZN11CFlashStore14checkIntegrityEPj - 4b	\n"
"		.long		__ZN11CFlashStore8setBuddyEP6CStore - 4b	\n"
"		.long		__ZN11CFlashStore10ownsObjectEj - 4b	\n"
"		.long		__ZN11CFlashStore7addressEj - 4b	\n"
"		.long		__ZN11CFlashStore9storeKindEv - 4b	\n"
"		.long		__ZN11CFlashStore8setStoreEP6CStorej - 4b	\n"
"		.long		__ZN11CFlashStore11isSameStoreEPvm - 4b	\n"
"		.long		__ZN11CFlashStore8isLockedEv - 4b	\n"
"		.long		__ZN11CFlashStore5isROMEv - 4b	\n"
"		.long		__ZN11CFlashStore6vppOffEv - 4b	\n"
"		.long		__ZN11CFlashStore5sleepEv - 4b	\n"
"		.long		__ZN11CFlashStore20newWithinTransactionEPjm - 4b	\n"
"		.long		__ZN11CFlashStore23startTransactionAgainstEj - 4b	\n"
"		.long		__ZN11CFlashStore15separatelyAbortEj - 4b	\n"
"		.long		__ZN11CFlashStore23addToCurrentTransactionEj - 4b	\n"
"		.long		__ZN11CFlashStore21inSeparateTransactionEj - 4b	\n"
"		.long		__ZN11CFlashStore12lockReadOnlyEv - 4b	\n"
"		.long		__ZN11CFlashStore14unlockReadOnlyEb - 4b	\n"
"		.long		__ZN11CFlashStore13inTransactionEv - 4b	\n"
"		.long		__ZN11CFlashStore9newObjectEPjPvm - 4b	\n"
"		.long		__ZN11CFlashStore13replaceObjectEjPvm - 4b	\n"
"		.long		__ZN11CFlashStore17calcXIPObjectSizeEllPl - 4b	\n"
"		.long		__ZN11CFlashStore12newXIPObjectEPjm - 4b	\n"
"		.long		__ZN11CFlashStore16getXIPObjectInfoEjPmS0_S0_ - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CFlashStore)

/*------------------------------------------------------------------------------
	Initialize a new store instance.
	Args:		--
	Return:	store
------------------------------------------------------------------------------*/

CFlashStore *
CFlashStore::make(void)
{
	fBlock = NULL;
	fPhysBlock = NULL;
	fLogicalBlock = NULL;
	fCache = NULL;
	fTracker = NULL;
	fLockCount = 0;
	fFlash = NULL;
	fSocketNo = kIndexNotFound;

	return this;
}


/*------------------------------------------------------------------------------
	Destroy a store instance -- dispose buffers and caches.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStore::destroy(void)
{
	// original calls deinit() which does:
	if (fBlock)
		FreePtr((Ptr)fBlock), fBlock = NULL;
	if (fPhysBlock)
		FreePtr((Ptr)fPhysBlock), fPhysBlock = NULL;
	if (fLogicalBlock)
		FreePtr((Ptr)fLogicalBlock), fLogicalBlock = NULL;
	if (fCache)
		delete fCache, fCache = NULL;
	if (fTracker)
		delete fTracker, fTracker = NULL;
#if defined(correct)
	if (fIsStoreRemovable && fSocketNo != kIndexNotFound)
		UnregisterVccOffNotify(fSocketNo);
#endif
}


/*------------------------------------------------------------------------------
	Initialize a store instance with inPSSInfo.
	Called from InitPSSManager().
	inFlags are used to determine if the store is on SRAM, Flash card
	or Internal Flash.
	Args:		inStoreAddr
				inStoreSize
				inArg3
				inSocketNumber
				inFlags
					0x01 =>	store is removable
					0x08 => use RAM as additional store
					0x10 => new internal flash
				inPSSInfo
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::init(void * inStoreAddr, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inPSSInfo)
{
	NewtonErr err = noErr;
	CFlash * flashMem = NULL;	// r6
	PSSStoreInfo * storeInfo = NULL;		// r7

	fSocketNo = inSocketNumber;
	fIsStoreRemovable = (inFlags & 0x01) != 0;
	fIsInternalFlash = (inFlags & 0x10) != 0;

	if (fIsStoreRemovable)
	{
		if (inPSSInfo)
		{
			storeInfo = (PSSStoreInfo *)inPSSInfo;
			flashMem = storeInfo->f2C;
			inStoreSize = storeInfo->f14;
		}
	}
	else if (fIsInternalFlash)
	{
		flashMem = (CFlash *)inPSSInfo;	// actually CNewInternalFlash
		fSocketNo = kIndexNotFound;
	}

	vccOn();
	if (flashMem)
		flashMem->acknowledgeReset();

	fLSN = 0;
	fPhysBlock = NULL;
	fWorkingBlock = NULL;
	fBlock = NULL;
	fLogicalBlock = NULL;
	fCache = NULL;
	f38 = 0;
	fAvEraseCount = 0;
	fIsErasing = NO;
	fEraseBlockAddr = 0;
	fLockCount = 0;
	fObjListTail = NULL;
	fObjListHead = NULL;
	fIsInTransaction = NO;
	f17 = NO;
	fIsFormatReqd = NO;
	f92 = NO;
	fIsFormatting = NO;
	f96 = NO;
	fLockROCount = 0;
	fFlash = flashMem;
	fCardHandler = storeInfo ? storeInfo->f28 : NULL;

	fIsSRAM = fIsStoreRemovable ? (storeInfo->f30 != 'flsh') : !fIsInternalFlash;
	f3E = !fIsSRAM && (flashMem->getAttributes() & 0x10) == 0;
	fUseRAM = inFlags == 0x08;
	fIsROM = storeInfo && storeInfo->f30 == 'rom ';
	fStoreAddr = (char *)inStoreAddr;
	fIsMounted = NO;
	fDirtyBits = ~kVirginBits;
	fVirginBits = kVirginBits;
	fBlockSize = flashMem ? flashMem->getEraseRegionSize() : (fUseRAM ? gInternalBlockSize : gMutableBlockSize);
	fBlockSizeShift = CeilLog2(fBlockSize);
	fBucketSize = kBucketSize;
	f64 = fBlockSize >> 2;
	fBlockSizeMask = fBlockSize - 1;
	f68 = f64 >> 2;
	fBucketCount = fBlockSize >> 11;
	fStoreDriver = NULL;
	fCachedUsedSize = 1;
	fCompact = NULL;

	if (fIsSRAM)
	{
		if (fUseRAM)
		{
			fCompact = &g0C106324;
			fStoreDriver = &g0C106358;
			fStoreDriver->init((char *)inStoreAddr, inStoreSize, (char *)InternalStoreInfo(3), InternalStoreInfo(2));
			inStoreSize += InternalStoreInfo(2);
		}
		else if (fIsStoreRemovable)
		{
			// reserve the last block for the compact state
			inStoreSize = ALIGN(inStoreSize, fBlockSize) - fBlockSize;
			fCompact = (SCompactState *)(fStoreAddr + inStoreSize);
			fStoreDriver = fCompact->inProgress() ? &fCompact->x34 : &fA4;
			for (bool isWP = true; isWP; sendAlertMgrWPBitch(0))
			{
				newton_try
				{
					fStoreDriver->init(fStoreAddr, inStoreSize, NULL, 0);
					isWP = NO;
				}
				newton_catch(exAbort)
				{
					isWP = true;
				}
				end_try;
			}
		}
		else
		{
			fCompact = &g0C106324;
			fStoreDriver = &g0C106358;
			fStoreDriver->init((char *)inStoreAddr, inStoreSize, NULL, 0);
		}
		if (fCompact->inProgress())
		{
			f17 = true;
			f92 = true;
		}
	}

	XTRY
	{
/*
The store consists of a number of 64K blocks. A block is the smallest erasable unit on the store.
We create an array of logical block objects and an array of pointers into that array.
We create an array of physical block objects and point each logical block object to a physical block -- physical blocks can be mapped out as required.
*/
		fNumOfBlocks = (flashMem ? flashMem->getTotalSize() : inStoreSize) / fBlockSize;
		f60 = 28 - CeilLog2(fNumOfBlocks);	// => 256M blocks max

		XFAILNOT(fPhysBlock = (CFlashPhysBlock *)NewPtr(fNumOfBlocks * sizeof(CFlashPhysBlock)), err = MemError();)	// original uses malloc
		XFAILNOT(fBlock = (CFlashBlock **)NewPtr(fNumOfBlocks * sizeof(CFlashBlock *)), err = MemError();)				// original uses malloc
		XFAILNOT(fLogicalBlock = (CFlashBlock *)NewPtrClear(fNumOfBlocks*sizeof(CFlashBlock)), err = MemError();)	// but does use NewPtrClear here
		for (ArrayIndex i = 0; i < fNumOfBlocks; ++i)
			fBlock[i] = &fLogicalBlock[i];
		initBlocks();

		fCache = new CFlashStoreLookupCache(64);

		XFAILNOT(fTracker = new CFlashTracker(128), err = MemError();)

		if (!f92)
		{
			newton_try
			{
				if (err == noErr)
					err = mount();
				if (err == kStoreErrNeedsFormat)
				{
					fIsFormatReqd = true;
					err = noErr;
				}

				if (err == noErr)
					err = recoveryCheck(NO);
				if (err == kStoreErrNeedsFormat)
				{
					fIsFormatReqd = true;
					err = noErr;
				}
			}
			newton_catch(exAbort)
			{
				err = noErr;
			}
			end_try;
		}

		if (fIsStoreRemovable)
		{
			fE5 = NO;
			fD8 = (long)storeInfo->f10;
			fE4 = NO;
			fE8 = fSocketNo;
		}
	}
	XENDTRY;

	vccOff();
	return err;
}


/*------------------------------------------------------------------------------
	Determine whether the store needs to be formattted.
	Original calls internalNeedsFormat() which we implement here.
	Args:		outNeedsFormat
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::needsFormat(bool * outNeedsFormat)
{
	bool isFormatReqd = NO;
	if (!f17)
	{
		if (fIsFormatReqd
		||  blockForAddr(BLOCK(0))->logEntryOffset() == 0
		||  blockForAddr(BLOCK(0))->isVirgin()
		||  blockForAddr(BLOCK(1))->isVirgin())
			// required blocks are still virgin
			isFormatReqd = true;
		else
		{
			FOREACH_BLOCK(block)
				StoreObjHeader obj;
				if (block->isVirgin())
					break;
				if (!block->isReserved()
				&&  block->readObjectAt(block->rootDirAddr(), &obj) == noErr
				&&  obj.id != kRootDirId)
				{
					// block’s directory is bogus
					isFormatReqd = true;
					break;
				}
			END_FOREACH_BLOCK
		}
	}

	*outNeedsFormat = isFormatReqd;
	return noErr;
}


/*------------------------------------------------------------------------------
	Format the store.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::format(void)
{
	NewtonErr err = noErr;
	CTime now = GetGlobalTime();
	vppOn();

	XTRY
	{
		// can’t format if the store is write-protected
		bool isRO;
		isReadOnly(&isRO);
		XFAILIF(isRO, err = kStoreErrWriteProtected;)

		fIsFormatting = true;
		if (fIsSRAM)
		{
			if (fCardHandler)
			{
#if 0		// no CCardHandler implementation yet -- any point?
				ULong status = fCardHandler->cardStatus() & 0x03;
				if (status == 1 || status == 3)
				{
					newton_try
					{
						CUPort cardServer;
						CCardMessage msg;
						size_t replySize;

						CUNameServer ns;
						ULong thing, spec;

						ns.lookup("cdsv", kUPort, &thing, &spec);
						cardServer = (ObjectId)thing;

						msg.messageStuff(0x3C, fSocketNo, 0);
						cardServer.sendRPC(&replySize, &msg, sizeof(CCardMessage), &msg, sizeof(CCardMessage));
						if (err == noErr)
							err = msg.f0C;
					}
					newton_catch_all
					{ }	// should return appropriate error here; original wouldn’t work anyway
					end_try;
				}
				else
					err = kStoreErrBadBattery;
				XFAIL(err)
#endif
			}
		}

		fLockCount = 1;
		if (fFlash)
			fFlash->acknowledgeReset();
		if (fCompact)
		{
			fCompact->init();
			if (fUseRAM)
				fStoreDriver->init(fStoreAddr, storeCapacity(), (char *)InternalStoreInfo(3), InternalStoreInfo(2));
			else
				fStoreDriver->init(fStoreAddr, storeCapacity(), NULL, 0);
		}

		ZAddr logEntryAddr = blockForAddr(0)->logEntryOffset();
		if (logEntryAddr != 0)
			zapLogEntry(logEntryAddr);

		logEntryAddr = 0;
		for ( ; ; )
		{
			SReservedBlockLogEntry logEntry;
			XFAILIF(err = nextLogEntry(logEntryAddr, &logEntryAddr, kAnyBlockLogEntryType, NULL), if (err == kStoreErrNoMoreObjects) err = noErr;)

			bool zapIt = true;
			XFAIL(err = basicRead(logEntryAddr, &logEntry, sizeof(SFlashLogEntry)))
			if (logEntry.fType == kReservedBlockLogEntryType)
			{
				XFAIL(err = basicRead(logEntryAddr, &logEntry, sizeof(SReservedBlockLogEntry)))
				if ((logEntry.f28 & 0x01) != 0)
					zapIt = NO;	// it’s reserved
			}
			if (zapIt)
				XFAIL(zapLogEntry(logEntryAddr))
		}
		XFAIL(err)

		if (!isErased(BLOCK(0)))
			XFAIL(err = syncErase(BLOCK(0)))
		if (!isErased(BLOCK(1)))
			XFAIL(err = syncErase(BLOCK(1)))

		// reset log sequence number
		fLSN = 1;
		// create log entries in first two blocks
		SFlashBlockLogEntry logEntry;
		logEntry.f20 = 0;
		logEntry.fPhysicalAddr = BLOCK(0);
		logEntry.fLogicalAddr = BLOCK(0);
		logEntry.fEraseCount = 0;
		logEntry.f2C = 0;
		logEntry.f30 = 0;
		logEntry.fRandom = rand();
		logEntry.fCreationTime = now.convertTo(kSeconds);
		logEntry.fDirectoryAddr = logEntry.fLogicalAddr + 4;
		logEntry.f44 = 0;
		logEntry.f48 = 0;
		logEntry.f40 = fVirginBits;
		XFAIL(err = addLogEntryToPhysBlock(kFlashBlockLogEntryType, sizeof(SFlashBlockLogEntry), &logEntry, BLOCK(0), NULL))

		logEntry.fPhysicalAddr = BLOCK(1);
		logEntry.fLogicalAddr = BLOCK(1);
		logEntry.fRandom = rand();
		logEntry.fCreationTime = now.convertTo(kSeconds);
		logEntry.fDirectoryAddr = logEntry.fLogicalAddr + 4;
		XFAIL(err = addLogEntryToPhysBlock(kFlashBlockLogEntryType, sizeof(SFlashBlockLogEntry), &logEntry, BLOCK(1), NULL))

		// write root directories in first two blocks
		XFAIL(err = mount())
		XFAIL(err = blockForAddr(BLOCK(0))->writeRootDirectory(NULL))
		XFAIL(err = blockForAddr(BLOCK(1))->writeRootDirectory(NULL))
		XFAIL(err = mount())

		// create store’s root object
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		err = blockForAddr(BLOCK(0))->addObject(kRootId, fIsSRAM ? 4 : 11, 0, obj, NO, NO);
		remove(obj);
		XFAIL(err)

		// reset flags
		fIsInTransaction = NO;
		f17 = NO;
		f92 = NO;
		fTracker->reset();
		fWorkingBlock = NULL;
		fIsFormatReqd = NO;
	}
	XENDTRY;

	fIsFormatting = NO;
	fLockCount = 0;
	vppOff();

	return err;
}


/*------------------------------------------------------------------------------
	Return the PSSId of the store’s root object.
	Args:		outRootId
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::getRootId(PSSId * outRootId)
{
	*outRootId = kRootId;
	return noErr;
}


/*------------------------------------------------------------------------------
	Create a new object in the store.
	Args:		outObjectId
				inSize
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::newObject(PSSId * outObjectId, size_t inSize)
{
	return newObject(outObjectId, NULL, inSize);
}


/*------------------------------------------------------------------------------
	Erase an object from the store.
	We don’t need to do anything special.
	Args:		inObjectId
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::eraseObject(PSSId inObjectId)
{
	return noErr;
}


/*------------------------------------------------------------------------------
	Delete an object from the store.
	Args:		inObjectId
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::deleteObject(PSSId inObjectId)
{
	if (inObjectId == kNoPSSId)
		return noErr;

	NewtonErr err;
	vppOn();

	newton_try
	{
		CStoreObjRef obj2(fVirginBits, this);
		add(obj2);
		CStoreObjRef obj1(fVirginBits, this);
		add(obj1);
		lockStore();

		XTRY
		{
			XFAIL(err = setupForModify(inObjectId, obj2, true, true))
			switch (obj2.state())
			{
			case 3:
			case 10:
				if (obj2.fObj.x2 == 2)
				{
					fTracker->lock();
//					XFAIL(err = obj2.dlete())	sic -- but don’t we want to fTracker->unlock()?
					if ((err = obj2.dlete()) == noErr)
						fTracker->remove(inObjectId);
					fTracker->unlock();
				}
				else
					err = obj2.setState(fIsSRAM ? 7 : 14);
				break;

			case 4:
			case 11:
				if (obj2.fObj.x2 == 2)
				{
					fTracker->lock();
					fTracker->add(inObjectId);
//					XFAIL(err = obj2.setState(fIsSRAM ? 7 : 14))	sic
					err = obj2.setState(fIsSRAM ? 7 : 14);
					fTracker->unlock();
				}
				else
					err = obj2.setState(fIsSRAM ? 7 : 14);
				break;

			case 6:
			case 13:
				XFAIL(err = obj2.findSuperceeded(obj1))
				XFAIL(err = obj2.dlete())
				err = obj1.setState(fIsSRAM ? 7 : 14);
				break;
			}
		}
		XENDTRY;

		fCache->forget(inObjectId, 0);
		unlockStore();
		remove(obj1);
		remove(obj2);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vppOff();
	return err;
}


/*------------------------------------------------------------------------------
	Set the size of an object in store.
	Args:		inObjectId
				inSize
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::setObjectSize(PSSId inObjectId, size_t inSize)
{
	NewtonErr err;
PRINTF(("CFlashStore::setObjectSize(id=%d, size=%lu)\n", inObjectId,inSize));
ENTER_FUNC

	// passing a flag in the size???
	f95 = (inSize & 0x80000000) != 0;
	inSize &= ~0x80000000;

	vppOn();

	newton_try
	{
		CStoreObjRef obj2(fVirginBits, this);
		add(obj2);
		CStoreObjRef obj1(fVirginBits, this);
		add(obj1);

		lockStore();
		XTRY
		{
			XFAIL(err = setupForModify(inObjectId, obj2, true, true))
			if (!f95)
				XFAILIF(inSize > obj2.fObj.size  &&  avail() < storeSlop(), err = kStoreErrStoreFull;)

			if (inSize != obj2.fObj.size)
			{
				bool r8 = (obj2.fObj.x2 != 2);
				switch (obj2.state())
				{
				case 3:
				case 10:
					if (r8)
						fTracker->lock();
					for ( ; ; )
					{
						err = obj2.cloneEmpty(fIsSRAM ? 2 : 9, inSize, obj1, obj2.fObj.x2 == 2);
						if (err == noErr)
							err = obj2.copyTo(obj1, 0, MIN(obj2.fObj.size, inSize));
						if (err == kStoreErrWriteError)
							obj1.dlete();
							// and try again
						else
							break;
					}
					XFAIL(err)
					XFAIL(err = obj2.dlete())
					XFAIL(err = obj1.setState(fIsSRAM ? 3 : 10))
					if (r8)
						fTracker->unlock();
					fCache->change(obj1);
					break;

				case 4:
				case 11:
					if (r8)
					{
						fTracker->lock();
						fTracker->add(inObjectId);
					}
					for ( ; ; )
					{
						err = obj2.cloneEmpty(fIsSRAM ? 1 : 8, inSize, obj1, obj2.fObj.x2 == 2);
						if (err == noErr)
							err = obj2.copyTo(obj1, 0, MIN(obj2.fObj.size, inSize));
						if (err == kStoreErrWriteError)
							obj1.dlete();
							// and try again
						else
							break;
					}
					XFAIL(err)
					XFAIL(err = obj2.setState(fIsSRAM ? 5 : 12))
					XFAIL(err = obj1.setState(fIsSRAM ? 6 : 13))
					if (r8)
						fTracker->unlock();
					fCache->change(obj1);
					break;

				case 6:
				case 13:
					if (r8)
						fTracker->lock();
					for ( ; ; )
					{
						err = obj2.cloneEmpty(fIsSRAM ? 1 : 8, inSize, obj1, obj2.fObj.x2 == 2);
						if (err ==  noErr)
							err = obj2.copyTo(obj1, 0, MIN(obj2.fObj.size, inSize));
						else
							break;
						if (err == kStoreErrWriteError)
							obj1.dlete();
							// and try again
						else
							break;
					}
					XFAIL(err)
					XFAIL(err = obj2.dlete())
					XFAIL(err = obj1.setState(fIsSRAM ? 6 : 13))
					if (r8)
						fTracker->unlock();
					fCache->change(obj1);
					break;
				}
			}
		}
		XENDTRY;
		unlockStore();

		remove(obj1);
		remove(obj2);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vppOff();
EXIT_FUNC
	return err;
}


/*------------------------------------------------------------------------------
	Return the size of an object in store.
	Args:		inObjectId
				outSize
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::getObjectSize(PSSId inObjectId, size_t * outSize)
{
	NewtonErr err;
	vccOn();

	newton_try
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		*outSize = 0;
		err = setupForRead(inObjectId, obj);
		if (err == noErr)
			*outSize = obj.fObj.size;
		remove(obj);
	}
	newton_catch(exAbort)
	{
		vccOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vccOff();
	return err;
}


/*------------------------------------------------------------------------------
	Write data into an object.
	Args:		inObjectId
				inStartOffset
				inBuffer
				inLength
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength)
{
	NewtonErr err;
PRINTF(("CFlashStore::write(id=%d, offset=%lu, length=%lu)\n", inObjectId,inStartOffset,inLength));
ENTER_FUNC
	vppOn();

	newton_try
	{
		CStoreObjRef obj2(fVirginBits, this);
		add(obj2);
		CStoreObjRef obj1(fVirginBits, this);
		add(obj1);
		CStoreObjRef obj0(fVirginBits, this);
		add(obj0);

		lockStore();
		XTRY
		{
			// write into obj2
			XFAIL(err = setupForModify(inObjectId, obj2, true, true))
			size_t objSize = obj2.fObj.size;
			size_t endOffset = inStartOffset + inLength;
			XFAILIF(inStartOffset > objSize || endOffset > objSize, err = kStoreErrObjectOverRun;)
			bool r10 = (obj2.fObj.x2 != 2);
			switch (obj2.state())
			{
			case 3:
			case 6:
				err = obj2.write(inBuffer, inStartOffset, inLength);
				break;

			case 4:
				if (r10)
				{
					fTracker->add(inObjectId);
					fTracker->lock();
				}
				// clone the object
				XFAIL(err = obj2.cloneEmpty(fIsSRAM ? 1 : 8, obj1, obj2.fObj.x2 == 2))
				// copy data before the start
				if (inStartOffset > 0)
					XFAIL(err = obj2.copyTo(obj1, 0, inStartOffset))
				// write the given data
				XFAIL(err = obj1.write(inBuffer, inStartOffset, inLength))
				// copy data after that
				if (obj2.fObj.size > endOffset)
					XFAIL(err = obj2.copyTo(obj1, endOffset, obj2.fObj.size - endOffset))
				XFAIL(err = obj2.setState(fIsSRAM ? 5 : 12))
				XFAIL(err = obj1.setState(fIsSRAM ? 6 : 13))
				if (r10)
					fTracker->unlock();
				fCache->change(obj1);
				break;

			case 5:
				XFAIL(err = obj2.findSuperceeder(obj1))
				err = obj1.write(inBuffer, inStartOffset, inLength);
				break;

			case 10:
				if (isRangeVirgin(obj2.fObjectAddr + sizeof(StoreObjHeader) + inStartOffset, inLength))
				{
					err = obj2.write(inBuffer, inStartOffset, inLength);
					break;
				}

				if (r10)
					fTracker->lock();
				for ( ; ; )
				{
					// clone the object
					XFAIL(err = obj2.cloneEmpty(fIsSRAM ? 2 : 9, obj1, obj2.fObj.x2 == 2))
					// copy data before the start
					XTRY
					{
						if (inStartOffset > 0)
							XFAIL(err = obj2.copyTo(obj1, 0, inStartOffset))
						// write the given data
						XFAIL(err = obj1.write(inBuffer, inStartOffset, inLength))
						// copy data after that
						if (obj2.fObj.size > endOffset)
							XFAIL(err = obj2.copyTo(obj1, endOffset, obj2.fObj.size - endOffset))
					}
					XENDTRY;
					if (err == kStoreErrWriteError)
						obj1.dlete();
						// and try again
					else
						break;
				}
				XFAIL(err)
				XFAIL(err = obj2.dlete())
				XFAIL(err = obj1.setState(fIsSRAM ? 3 : 10))
				if (r10)
					fTracker->unlock();
				fCache->change(obj1);
				break;

			case 11:
				if (r10)
				{
					fTracker->add(inObjectId);
					fTracker->lock();
				}
				for ( ; ; )
				{
					// clone the object
					XFAIL(err = obj2.cloneEmpty(fIsSRAM ? 1 : 8, obj1, obj2.fObj.x2 == 2))
					XTRY
					{
						if (inStartOffset > 0)
							XFAIL(err = obj2.copyTo(obj1, 0, inStartOffset))
						// write the given data
						XFAIL(err = obj1.write(inBuffer, inStartOffset, inLength))
						// copy data after that
						if (obj2.fObj.size > endOffset)
							XFAIL(err = obj2.copyTo(obj1, endOffset, obj2.fObj.size - endOffset))
					}
					XENDTRY;
					if (err == kStoreErrWriteError)
						obj1.dlete();
						// and try again
					else
						break;
				}
				XFAIL(err)
				XFAIL(err = obj2.setState(fIsSRAM ? 5 : 12))
				XFAIL(err = obj1.setState(fIsSRAM ? 6 : 13))
				if (r10)
					fTracker->unlock();
				fCache->change(obj1);
				break;

			case 12:
				XFAIL(err = obj2.findSuperceeder(obj1))
				if (r10)
					fTracker->lock();
				for ( ; ; )
				{
					// clone the object
					XFAIL(err = obj1.cloneEmpty(fIsSRAM ? 1 : 8, obj0, obj2.fObj.x2 == 2))
					XTRY
					{
						if (inStartOffset > 0)
							XFAIL(err = obj1.copyTo(obj0, 0, inStartOffset))
						// write the given data
						XFAIL(err = obj0.write(inBuffer, inStartOffset, inLength))
						// copy data after that
						if (obj1.fObj.size > endOffset)
							XFAIL(err = obj1.copyTo(obj0, endOffset, obj1.fObj.size - endOffset))
					}
					XENDTRY;
					if (err == kStoreErrWriteError)
						obj0.dlete();
						// and try again
					else
						break;
				}
				XFAIL(err)
				XFAIL(err = obj1.dlete())
				XFAIL(err = obj0.setState(fIsSRAM ? 6 : 13))
				if (r10)
					fTracker->unlock();
				fCache->change(obj0);
				break;

			case 13:
				if (isRangeVirgin(obj2.fObjectAddr + sizeof(StoreObjHeader) + inStartOffset, inLength))
				{
					err = obj2.write(inBuffer, inStartOffset, inLength);
					break;
				}

				if (r10)
					fTracker->lock();
				for ( ; ; )
				{
					// clone the object
					XFAIL(err = obj2.cloneEmpty(fIsSRAM ? 1 : 8, obj1, obj2.fObj.x2 == 2))
					// copy data before the start
					XTRY
					{
						if (inStartOffset > 0)
							XFAIL(err = obj2.copyTo(obj1, 0, inStartOffset))
						// write the given data
						XFAIL(err = obj1.write(inBuffer, inStartOffset, inLength))
						// copy data after that
						if (obj2.fObj.size > endOffset)
							XFAIL(err = obj2.copyTo(obj1, endOffset, obj2.fObj.size - endOffset))
					}
					XENDTRY;
					if (err == kStoreErrWriteError)
						obj1.dlete();
						// and try again
					else
						break;
				}
				XFAIL(err)
				XFAIL(err = obj2.dlete())
				XFAIL(err = obj1.setState(fIsSRAM ? 6 : 13))
				if (r10)
					fTracker->unlock();
				fCache->change(obj1);
				break;
			}
		}
		XENDTRY;
		unlockStore();

		remove(obj0);
		remove(obj1);
		remove(obj2);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vppOff();
EXIT_FUNC
	return err;
}


/*------------------------------------------------------------------------------
	Read data from an object.
	Args:		inObjectId
				inStartOffset
				outBuffer
				inLength
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength)
{
	NewtonErr err;
PRINTF(("CFlashStore::read(id=%d, offset=%lu, length=%lu)\n", inObjectId,inStartOffset,inLength));
ENTER_FUNC
	vccOn();

	newton_try
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		XTRY
		{
			XFAIL(err = setupForRead(inObjectId, obj))
			size_t objSize = obj.fObj.size;
			XFAILIF(inStartOffset > objSize, err = kStoreErrObjectOverRun;)
			if (inStartOffset + inLength > objSize)
			{
				err = obj.read(outBuffer, inStartOffset, objSize - inStartOffset);
				if (err == noErr)
					err = kStoreErrObjectOverRun;
			}
			else
				err = obj.read(outBuffer, inStartOffset, inLength);
		}
		XENDTRY;
		remove(obj);
	}
	newton_catch(exAbort)
	{
		vccOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vccOff();
EXIT_FUNC
	return err;
}


/*------------------------------------------------------------------------------
	Determine the store’s usable and used sizes.
	Args:		outTotalSize
				outUsedSize
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::getStoreSize(size_t * outTotalSize, size_t * outUsedSize)
{
	NewtonErr err = noErr;

	// eval size reserved for system in each block
	CFlashBlock * block = blockForAddr(0);
	size_t reservedSize = block->rootDirSize() + (fIsSRAM ? 256 : 1024) + 12;
	// eval total size available to user
	size_t availableSize = storeCapacity() - (fNumOfBlocks * reservedSize) - storeSlop();
	*outTotalSize = availableSize;

	if (fCachedUsedSize != 1)
		*outUsedSize = fCachedUsedSize;

	else
	{
		availableSize = fBlockSize - reservedSize;	// per block now

		vccOn();
		*outUsedSize = 0;
		FOREACH_BLOCK(block)
			if (!block->isVirgin())
				*outUsedSize += (availableSize - block->calcRecoverableBytes());
		END_FOREACH_BLOCK
		vccOff();

		// sanity check
		if (*outUsedSize > *outTotalSize)
			*outUsedSize = *outTotalSize;
		fCachedUsedSize = *outUsedSize;
	}

	return err;
}


/*------------------------------------------------------------------------------
	Determine whether the store is read-only.
	Args:		outIsReadOnly
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::isReadOnly(bool * outIsReadOnly)
{
	vccOn();
	*outIsReadOnly = (fIsStoreRemovable && (fFlash != NULL) && (fFlash->getAttributes() & 0x80)) || isROM() || isWriteProtected();
	vccOff();

	return noErr;
}


/*------------------------------------------------------------------------------
	Lock the store.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::lockStore(void)
{
	fLockCount++;
	return noErr;
}


/*------------------------------------------------------------------------------
	Unlock the store.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::unlockStore(void)
{
	NewtonErr err = noErr;

	if (fLockCount == 1 && fIsInTransaction)
	{
		vppOn();
		newton_try
		{
			markCommitPoint();
			doCommit(true);
			fLockCount = 0;
		}
		newton_catch(exAbort)
		{
			vppOff();
			fObjListHead = NULL;
			fObjListTail = NULL;
		}
		end_try;
		vppOff();
	}

	else if (fLockCount > 0)
		fLockCount--;

	return err;
}


NewtonErr
CFlashStore::abort(void)
{
	NewtonErr err = noErr;
	if (fIsInTransaction)
	{
		vppOn();

		newton_try
		{
			err = doAbort(NO);
		}
		newton_catch(exAbort)
		{
			vppOff();
			fObjListHead = NULL;
			fObjListTail = NULL;
		}
		end_try;

		vppOff();
	}
	fLockCount = 0;
	return err;
}


NewtonErr
CFlashStore::idle(bool * outArg1, bool * outArg2)
{
	return noErr;
}


/*------------------------------------------------------------------------------
	This doesn’t appear to be used.
	Args:		inObjectId
				outNextObjectId
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::nextObject(PSSId inObjectId, PSSId * outNextObjectId)
{
	return noErr;
}


NewtonErr
CFlashStore::checkIntegrity(ULong * inArg1)
{
	return noErr;
}

#pragma mark MuxStore
/*------------------------------------------------------------------------------
	We are not a Mux store.
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::setBuddy(CStore * inStore)
{
	return noErr;
}

bool
CFlashStore::ownsObject(PSSId inObjectId)
{
	return true;
}

VAddr
CFlashStore::address(PSSId inObjectId)
{
	return 0;
}

/*------------------------------------------------------------------------------
	Return a string identifying the kind of store.
	Args:		--
	Return:	C string
------------------------------------------------------------------------------*/

const char *
CFlashStore::storeKind(void)
{
	if (!fIsStoreRemovable)
		return "Internal";

	if (isROM())
		return "Application card";

	if (fIsSRAM)
		return "Storage card";

	return "Flash storage card";
}


NewtonErr
CFlashStore::setStore(CStore * inStore, ObjectId inEnvironment)
{
	return noErr;
}


/*------------------------------------------------------------------------------
	Determine whether this store is equivalent to a hunk of data.
	Args:		inAlienStoreData		block of data that might be a store
				inAlienStoreSize		size of that block
	Return:	true => is the same
------------------------------------------------------------------------------*/

bool
CFlashStore::isSameStore(void * inAlienStoreData, size_t inAlienStoreSize)
{
	NewtonErr err;
	bool isTheSame = true;
	vccOn();

	if (!fIsFormatting)
	{
		newton_try
		{
			ZAddr logEntryAddr = 0;
			for ( ; ; )
			{
				// find a log entry in this store
				XFAILIF(err = nextLogEntry(logEntryAddr, &logEntryAddr, kFlashBlockLogEntryType, inAlienStoreData), if (err == kStoreErrNoMoreObjects) err = noErr;)	// loop exit
				// point to what SHOULD be the same log entry in the alien store data
				SFlashBlockLogEntry * logEntry = (SFlashBlockLogEntry *)((char *)inAlienStoreData + logEntryAddr);
				// entries should match
				ULong signature;
				CFlashBlock * block = blockForAddr(logEntry->fLogicalAddr);
				if (block != NULL
				&& (signature = block->validity()) != 0
				&&  signature != (logEntry->fRandom ^ logEntry->fCreationTime))
				{
					isTheSame = NO;
					break;
				}
			}
		}
		newton_catch(exAbort)
		{
			isTheSame = NO;
		}
		end_try;
	}

	vccOff();
	return isTheSame;
}


/*------------------------------------------------------------------------------
	Return the lock state of the store.
	Args:		--
	Return:	true => locked
------------------------------------------------------------------------------*/

bool
CFlashStore::isLocked(void)
{
	return fLockCount != 0;
}


/*------------------------------------------------------------------------------
	Determine whether the store is application ROM.
	Args:		--
	Return:	true => is ROM
------------------------------------------------------------------------------*/

bool
CFlashStore::isROM(void)
{
	return fIsROM;
}


#pragma mark Power management
/*------------------------------------------------------------------------------
	We don’t manage power.
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::vppOff(void)
{ return noErr; }

NewtonErr
CFlashStore::sleep(void)
{
	return noErr;
}


#pragma mark Transactions
/*------------------------------------------------------------------------------
	Create a new object within the current transaction.
	Args:		outObjectId
				inSize
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::newWithinTransaction(PSSId * outObjectId, size_t inSize)
{
	return newWithinTransaction(outObjectId, inSize, NO);
}


NewtonErr
CFlashStore::startTransactionAgainst(PSSId inObjectId)
{
	NewtonErr err = noErr;
	vppOn();

	newton_try
	{
		CStoreObjRef obj1(fVirginBits, this);
		add(obj1);
		CStoreObjRef obj0(fVirginBits, this);
		add(obj0);

		XTRY
		{
			XFAIL(err = setupForModify(inObjectId, obj1, NO, NO))
			XFAIL(obj1.fObj.x2 == 2)
			fTracker->lock();
			XTRY
			{
				switch (obj1.state())
				{
				case 3:
				case 10:
					XFAIL(err = obj1.setSeparateTransaction())
					break;

				case 6:
				case 13:
					XFAIL(err = obj1.findSuperceeded(obj0))
					XFAIL(err = obj1.setSeparateTransaction())	// original doesn’t XFAIL
					XFAIL(err = obj0.setSeparateTransaction())	// original doesn’t XFAIL
					break;

				case 4:
				case 11:
					XFAIL(err = obj1.setState(fIsSRAM ? 5 : 12))
					XFAIL(err = obj1.clone(fIsSRAM ? 6 : 13, obj0, true))	// original doesn’t XFAIL
					XFAIL(err = obj1.setSeparateTransaction())	// original doesn’t XFAIL
					fCache->change(obj0);
					break;
				}
			}
			XENDTRY;
			fTracker->unlock();
		}
		XENDTRY;

		remove(obj0);
		remove(obj1);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vppOff();
	return err;
}


NewtonErr
CFlashStore::separatelyAbort(PSSId inObjectId)
{
	NewtonErr err = noErr;
	vppOn();

	newton_try
	{
		CStoreObjRef obj2(fVirginBits, this);
		add(obj2);
		CStoreObjRef obj1(fVirginBits, this);
		add(obj1);
		CStoreObjRef obj0(fVirginBits, this);
		add(obj0);

		XTRY
		{
			err = setupForModify(inObjectId, obj2, NO, NO);
			if (err == kStoreErrObjectNotFound)
				err = lookup(inObjectId, fIsSRAM ? 7 : 14, obj2);
			XFAIL(err)
			XFAIL(obj2.fObj.x2 != 2)
			switch (obj2.state())
			{
			case 3:
			case 10:
				XFAIL(err = obj2.dlete())
				break;

			case 6:
				XFAIL(err = obj2.findSuperceeded(obj1))
				XFAIL(err = obj2.dlete())
				XFAIL(err = obj1.clearSeparateTransaction())
				XFAIL(err = obj1.setState(fIsSRAM ? 4 : 11))
				break;

			case 7:
				XFAIL(err = obj2.clearSeparateTransaction())
				XFAIL(err = obj2.setState(fIsSRAM ? 4 : 11))
				break;

			case 13:
				XFAIL(err = obj2.findSuperceeded(obj1))
				XFAIL(err = obj2.dlete())
				XFAIL(err = obj1.clone(fIsSRAM ? 1 : 8, obj0, NO))
				XFAIL(err = obj0.setState(fIsSRAM ? 4 : 11))
				XFAIL(err = obj1.dlete())
				break;

			case 14:
				XFAIL(err = obj2.clearSeparateTransaction())
				XFAIL(err = obj2.clone(fIsSRAM ? 1 : 8, obj1, NO))
				XFAIL(err = obj1.setState(fIsSRAM ? 4 : 11))
				XFAIL(err = obj2.dlete())
				break;
			}
			fCache->forget(inObjectId, 0);
		}
		XENDTRY;

		remove(obj0);
		remove(obj1);
		remove(obj2);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vppOff();
	return err;
}


/*------------------------------------------------------------------------------
	Add an object to the current transaction.
	Args:		inObjectId
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::addToCurrentTransaction(PSSId inObjectId)
{
	NewtonErr err = noErr;
	vppOn();

	newton_try
	{
		CStoreObjRef obj1(fVirginBits, this);
		add(obj1);
		CStoreObjRef obj0(fVirginBits, this);
		add(obj0);
		lockStore();

		XTRY
		{
			err = setupForModify(inObjectId, obj1, true, true);
			if (err == kStoreErrObjectNotFound)
				err = lookup(inObjectId, fIsSRAM ? 7 : 14, obj1);
			XFAIL(err)
			XFAIL(obj1.fObj.x2 != 2)
			switch (obj1.state())
			{
			case 3:
			case 10:
			case 7:
			case 14:
				fTracker->lock();
				if ((err = obj1.clearSeparateTransaction()) == noErr)
					fTracker->add(inObjectId);
				fCache->change(obj1);
				fTracker->unlock();
				break;

			case 6:
			case 13:
				fTracker->lock();
				obj1.findSuperceeded(obj0);
				obj1.clearSeparateTransaction();
				if ((err = obj0.clearSeparateTransaction()) == noErr)
					fTracker->add(inObjectId);
				fCache->change(obj1);
				fTracker->unlock();
				break;
			}
		}
		XENDTRY;

		unlockStore();
		remove(obj0);
		remove(obj1);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vppOff();
	return err;
}


/*------------------------------------------------------------------------------
	Determine whether a separate transaction is in progress.
	Args:		inObjectId
	Return:	true => separate transaction in progress
------------------------------------------------------------------------------*/

bool
CFlashStore::inSeparateTransaction(PSSId inObjectId)
{
	bool yehuhuh = NO;
	vppOn();
	newton_try
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		yehuhuh = lookup(inObjectId, 0, obj) == noErr
				 && obj.fObj.x2 == 2;
		remove(obj);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;
	vppOff();
	return yehuhuh;
}


/*------------------------------------------------------------------------------
	Lock read-only..?
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::lockReadOnly(void)
{
	fLockROCount++;
	return noErr;
}


/*------------------------------------------------------------------------------
	Unlock read-only..?
	Args:		inReset
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::unlockReadOnly(bool inReset)
{
	if (inReset)
		fLockROCount = 0;
	else if (fLockROCount > 0)
		fLockROCount--;
	return noErr;
}


/*------------------------------------------------------------------------------
	Determine whether a transaction is in progress.
	Args:		--
	Return:	true => transaction in progress
------------------------------------------------------------------------------*/

bool
CFlashStore::inTransaction(void)
{
	return fIsInTransaction;
}


/*------------------------------------------------------------------------------
	Create a new object in the store.
	Args:		outObjectId			on return, id of the object created or 0
				inData				object data
				inSize				object size
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::newObject(PSSId * outObjectId, void * inData, size_t inSize)
{
	NewtonErr	err;

	// passing a flag in the size???
	f95 = (inSize & 0x80000000) != 0;
	inSize &= ~0x80000000;

	// object size sanity check
	if (inSize > fBlockSize - KByte)
		return kStoreErrObjectTooBig;

PRINTF(("CFlashStore::newObject(size=%lu)\n", inSize));
ENTER_FUNC
	vppOn();

	newton_try
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		lockStore();

		XTRY
		{
			err = setupForModify(kNoPSSId, *(CStoreObjRef *)0, true, true);
			XFAIL(outObjectId == NULL)
			*outObjectId = 0;
			XFAIL(err)
			if (!f95 && avail() < storeSlop())
			{
				gc();
				XFAILIF(avail() < storeSlop(), err = kStoreErrStoreFull;)
			}
			fTracker->lock();
			XTRY
			{
				XFAIL(err = addObject(kNoPSSId, fIsSRAM ? 3 : 10, inSize, obj, NO, NO))
				if (inData != NULL)
					XFAIL(err = obj.write(inData, 0, inSize))
				*outObjectId = obj.fObj.id;
				fTracker->add(*outObjectId);
			}
			XENDTRY;
			fTracker->unlock();
		}
		XENDTRY;

		unlockStore();
		remove(obj);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vppOff();
PRINTF((" -> objectId=%u\n", *outObjectId));
EXIT_FUNC
	return err;
}


/*------------------------------------------------------------------------------
	Replace the data in an object on the store.
	Args:		inObjectId			id of the object to replace
				inData				new object data
				inSize				new object size
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::replaceObject(PSSId inObjectId, void * inData, size_t inSize)
{
	NewtonErr	err;
PRINTF(("CFlashStore::replaceObject(id=%d, size=%lu)\n", inObjectId,inSize));
ENTER_FUNC
	if ((err = setObjectSize(inObjectId, inSize)) == noErr)
		err = write(inObjectId, 0, inData, inSize);
EXIT_FUNC
	return err;
}


#pragma mark XIP
/*------------------------------------------------------------------------------
	We don’t execute in place.
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::calcXIPObjectSize(long inArg1, long inArg2, long * outArg3)
{
	return kOSErrXIPNotPossible;
}

NewtonErr
CFlashStore::newXIPObject(PSSId * outObjectId, size_t inSize)
{
	return kOSErrXIPNotPossible;
}

NewtonErr
CFlashStore::getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4)
{
	return kOSErrXIPNotPossible;
}


#pragma mark -
#pragma mark Flash-specific


/*------------------------------------------------------------------------------
	Determine the store’s capacity.
	Note that when flash is written to we actually copy to another block,
	so one block must always be reserved.
	Args:		--
	Return:	its size
------------------------------------------------------------------------------*/

size_t
CFlashStore::storeCapacity(void)
{
	size_t capacity = fNumOfBlocks * fBlockSize;
	return fIsSRAM ? capacity : capacity - fBlockSize;
}


/*------------------------------------------------------------------------------
	Determine the free store available.
	Iterate over the blocks summing the yield per block.
	Args:		--
	Return:	its size
------------------------------------------------------------------------------*/

size_t
CFlashStore::avail(void)
{
	size_t storeAvail = 0;

	FOREACH_BLOCK(block)
		storeAvail += block->yield();
	END_FOREACH_BLOCK

	if (storeAvail < storeSlop() && fIsSRAM)
	{
		// we’re at a critically low level -- GC and try again
		// (can only GC SRAM because of the writes involved)
		gc();
		storeAvail = 0;
		FOREACH_BLOCK(block)
			storeAvail += block->yield();
		END_FOREACH_BLOCK
	}

	return storeAvail;
}


/*------------------------------------------------------------------------------
	Return the low-water mark of store space available.
	WAS:		internalStoreSlop()
				but we do return a value for any store
	Args:		--
	Return:	the minimum slop
------------------------------------------------------------------------------*/

size_t
CFlashStore::storeSlop(void)
{
	return fIsInternalFlash ? gInternalFlashStoreSlop : 6*KByte;
}


/*------------------------------------------------------------------------------
	Garbage-Collect.
	Well, actually, shuffle objects up to maximise free space.
	Only for SRAM because of the writes involved.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStore::gc(void)
{
	if (fIsSRAM)
	{
		FOREACH_BLOCK(block)
			if (!block->isVirgin())
				block->compactInPlace();
		END_FOREACH_BLOCK
	}
}


bool
CFlashStore::isWriteProtected(void)
{
	if (fLockROCount > 0)
		return true;

	bool isWP = false;
	if (fIsStoreRemovable)
	{
		if (fFlash)
		{
			touchMe();
			fFlash->getWriteProtected(&isWP);
		}
		else if (fCardHandler)
		{
#if 0		// no CCardHandler implementation yet -- any point?
			newton_try
			{
				touchMe();
				isWP = (fCardHandler->cardStatus() & 0x04) != 0;
			}
			newton_catch_all
			{
				isWP = false;
			}
			end_try;
#endif
		}
	}
	return isWP;
}

#pragma mark Erasure

bool
CFlashStore::isErased(ZAddr inBlockAddr)
{
	return isErased(inBlockAddr, fBlockSize, 0);
}


bool
CFlashStore::isErased(ZAddr inBlockAddr, size_t inBlockSize, ArrayIndex inNumOfBadBytesPermitted)
{
	if (fIsSRAM)
		return false;

	if (fIsInternalFlash)
		return fFlash->isVirgin(inBlockAddr, inBlockSize);

	ArrayIndex failureCount = 0;
	char * p = fStoreAddr + inBlockAddr;
	for ( ; inBlockSize > 0; inBlockSize--, p++)
	{
		if (*p != fVirginBits
		&&  ++failureCount > inNumOfBadBytesPermitted)
			return false;
	}
	return true;
}


NewtonErr
CFlashStore::syncErase(ZAddr inBlockAddr)
{
	NewtonErr err = 1;	// not actually an error, obviously, but gets the loop started
	for (ArrayIndex attempt = 0; err && attempt < 4; attempt++)
	{
		err = noErr;
		if (fIsSRAM)
		{
			newton_try
			{
				fStoreDriver->set(inBlockAddr, fBlockSize, fVirginBits);
			}
			newton_catch_all	// original says newton_catch(0)
			{
				err = kOSErrBusAccess;
			}
			end_try;
		}
		else	// is flash
		{
			XTRY
			{
				XFAIL(err = waitForEraseDone())
				XFAIL(err = startErase(inBlockAddr))
				err = waitForEraseDone();
			}
			XENDTRY;
		}

		if (err)
		{
			Sleep(100*kMilliseconds);
			if (isWriteProtected())
				sendAlertMgrWPBitch(0);
		}
	}
	return err;
}


NewtonErr
CFlashStore::startErase(ZAddr inBlockAddr)
{
	fEraseBlockAddr = inBlockAddr;
	fIsErasing = true;
	return fFlash->erase(inBlockAddr);
}


NewtonErr
CFlashStore::eraseStatus(ZAddr inBlockAddr)
{
	NewtonErr err = fFlash->status(inBlockAddr);
	if (err == 1)
		err = noErr;
	else if (err == 3)
		err = kStoreErrEraseInProgress;
	return err;
}


NewtonErr
CFlashStore::waitForEraseDone(void)
{
	if (fIsSRAM)
		return noErr;

	while (eraseStatus(fEraseBlockAddr) == kStoreErrEraseInProgress)
		/* spin-loop */;
	fIsErasing = false;
	return eraseStatus(fEraseBlockAddr);
}


void
CFlashStore::calcAverageEraseCount(void)
{
	ArrayIndex eraseCount = 0;
	for (ArrayIndex i = 0; i < fNumOfBlocks; ++i)
		eraseCount += fPhysBlock[i].eraseCount();
	fAvEraseCount = eraseCount / fNumOfBlocks;
}


ArrayIndex
CFlashStore::averageEraseCount(void)
{
	return fAvEraseCount;
}

#pragma mark Log Entries

/* -----------------------------------------------------------------------------
	Look for the next log entry: identify a log entry by its signature.
	Args:		inAddr		current log entry
				outAddr		next log entry
				inType		log entry type: fblk,eblk,zblk
				inArg4		SFlashLogEntry to use instead of actual next log entry
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::nextLogEntry(ZAddr inAddr, ZAddr * outAddr, ULong inType, void * inAlienStoreData)
{
	ZAddr addr = (inAddr == 0) ? offsetToLogs() : inAddr + 4;

	// iterate over blocks
	for ( ; ; )
	{
		// iterate over log entries within the block -- we’re searching for the next log entry signature
		for ( ; (addr & fBlockSizeMask) < (fBlockSize - sizeof(SFlashLogEntry)); addr += 4)
		{
			SFlashLogEntry logEntry;
			if (inAlienStoreData)
				memcpy(&logEntry, inAlienStoreData, sizeof(SFlashLogEntry));
			else
				basicRead(addr, &logEntry, sizeof(SFlashLogEntry));
			if (logEntry.isValid(addr)								// found a log entry
			&& (inType == 0 || inType == logEntry.fType))	// of the right type
			{
				*outAddr = addr;
				return noErr;
			}
		}
		// try the next block
		addr = ALIGN(addr, fBlockSize);
		if (addr >= fNumOfBlocks * fBlockSize)
			return kStoreErrNoMoreObjects;
		addr += offsetToLogs();
	}
}


/* -----------------------------------------------------------------------------
	Add a log entry to the block.
	Args:		inType		log entry type: fblk,eblk,zblk
				inSize		log entry size
				ioLogEntry	log entry
				inAddr		address/offset at which to write it
				outAddr		on return, actual address/offset at which written
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::addLogEntryToPhysBlock(ULong inType, size_t inSize, SFlashLogEntry * ioLogEntry, ZAddr inAddr, ZAddr * outAddr)
{
	NewtonErr err;
PRINTF(("CFlashStore::addLogEntryToPhysBlock(type=%08X, size=%lu, addr=%08X)\n", inType,inSize,inAddr));
ENTER_FUNC
	ZAddr addr = inAddr;
	if ((addr & fBlockSizeMask) < offsetToLogs())
		// addr is not in logs area in block 0; must be in another block
		addr = (addr & ~fBlockSizeMask) + offsetToLogs();

	for ( ; ; )
	{
PRINTF(("looking for chunk size=%lu at addr=%08X:\n", inSize,addr));
		addr = findPhysWritable(addr, (addr + fBlockSizeMask) & ~fBlockSizeMask, inSize);	// if addr == 0 you’re in big trouble here
PRINTF((" -> addr=%08X)\n", addr));
		XFAILIF(addr == 0, err = kStoreErrBlockFull;)
		ioLogEntry->fPhysSig = (addr ^ 'dyer');
		ioLogEntry->fPhysSig2 = (~addr ^ 'foo!');
		ioLogEntry->fNewtSig = 'newt';
		ioLogEntry->fType = inType;
		ioLogEntry->fSize = inSize;
		ioLogEntry->fLSN = nextLSN();
		ioLogEntry->fPhysicalAddr = inAddr;		// caller also sets this value
		ioLogEntry->f1C = fVirginBits;
		// write most of it; but not fPhysSig yet
		err = basicWrite(addr+4, (char *)ioLogEntry+4, inSize-4);
		if (err == noErr)
			// that was OK, chances are good we’ll totally succeed so now write fPhysSig
			err = basicWrite(addr, ioLogEntry, 4);
		if (err == kStoreErrWriteError)
			XFAIL(err = zapLogEntry(addr))
			// and try again
		else
		{
			if (outAddr)
				*outAddr = addr;
			break;
		}
	}
EXIT_FUNC
	return err;
}


/* -----------------------------------------------------------------------------
	Zap a log entry.
	We only need to zap the signature for the whole entry to be invalid.
	Args:		inAddr		log entry address/offset
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::zapLogEntry(ZAddr inAddr)
{
	return zap(inAddr, 4);
}


/* -----------------------------------------------------------------------------
	Find a chunk of virgin store with a given range.
	Args:		inStartAddr		address/offset of start of range
				inEndAddr		address/offset of end of range
				inLen				length of chunk to find
	Return:	address/offset of chunk or 0 if no chunk big enough
----------------------------------------------------------------------------- */

ZAddr
CFlashStore::findPhysWritable(ZAddr inStartAddr, ZAddr inEndAddr, size_t inLen)
{
	ZAddr writableAddr = inStartAddr;
	ZAddr limit = inEndAddr - inLen;
	for (ZAddr sampleAddr = inStartAddr; sampleAddr < limit; sampleAddr += 4)
	{
		if (sampleAddr - writableAddr >= inLen)
			// we found enough room!
			return writableAddr;

		// sample the data at this address
		ULong data;
		basicRead(sampleAddr, &data, 4);
		if (data != fVirginBits)
			// in use: start looking afresh
			writableAddr = sampleAddr + 4;
	}
	// no room in the given range
	return 0;
}


#pragma mark More power management
/*------------------------------------------------------------------------------
	We don’t manage power.
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::vppOn(void)
{ return noErr; }
NewtonErr
CFlashStore::vccOn(void)
{ return noErr; }

NewtonErr
CFlashStore::vccOff(void)
{ return noErr; }


#pragma mark Transactions

NewtonErr
CFlashStore::transactionState(int * outState)
{
	NewtonErr err = noErr;

	if (fIsSRAM)
		*outState = fCompact->x08;

	else
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		XTRY
		{
			ULong sp00;
			XFAILIF(err = lookup(kTxnRecordId, 0, obj), if (err == kStoreErrObjectNotFound) { err = noErr; *outState = 0; })
			XFAIL(err = obj.read(&sp00, 0, sizeof(sp00)))
			if (sp00 == 0
			|| (sp00 & (sp00 - 1)) == 0)
				*outState = 2;
			else
				*outState = 1;
		}
		XENDTRY;
		remove(obj);
	}

	return err;
}


/* -----------------------------------------------------------------------------
	Check transaction recovery.
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::recoveryCheck(bool inArg)
{
	NewtonErr err;

	CStoreObjRef obj(fVirginBits, this);
	add(obj);
	vppOn();

	XTRY
	{
		if (f92)
		{
			lowLevelRecovery();
			mount();
		}
		int state;
		XFAIL(err = transactionState(&state))
		if (state == 0 && !f96)
			f17 = NO;
		else
		{
			f17 = true;
			if (inArg)
			{
				bool isRO;
				isReadOnly(&isRO);
				if (!isRO)
				{
					bool r6 = f96;
					switch (state)
					{
					case 0:
						if (r6)
							doAbort(true);
						break;
					case 1:
						doAbort(true);
						break;
					case 2:
						XFAIL(err = doCommit(false))
						if (r6)
							doAbort(true);
						break;
					}
				}
			}
		}
	}
	XENDTRY;

	vppOff();
	remove(obj);
	return err;
}


void
CFlashStore::lowLevelRecovery(void)
{}


/* -----------------------------------------------------------------------------
	Mount the store.
	Initialise blocks, empty the cache, set up log entries, sanity check.
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::mount(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		initBlocks();
		fCache->forgetAll();
		XFAIL(err = scanLogForLogicalBlocks(&f96))
		XFAIL(err = scanLogForErasures())
		XFAIL(err = scanLogForReservedBlocks())
		calcAverageEraseCount();

		bool wasVirginFound = false;
		FOREACH_BLOCK(block)
			if (block->isVirgin())
				wasVirginFound = true;
			else if (wasVirginFound)
				XFAILNOT(block->isReserved(), err = kStoreErrNeedsFormat;)
		END_FOREACH_BLOCK
		XFAIL(err)	// not in original

		fIsMounted = true;
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	Touch the store.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CFlashStore::touchMe(void)
{
	char aByte;
	memcpy(&aByte, fStoreAddr, sizeof(aByte));
}


void
CFlashStore::notifyCompact(CFlashBlock * inBlock)
{
	CStoreObjRef * obj;
	for (obj = fObjListTail; obj != NULL; obj = obj->fPrev)
	{
		if (obj->fObj.isValid(this)
		&& (obj->fObjectAddr & ~fBlockSizeMask) == (inBlock->firstObjAddr() & ~fBlockSizeMask))
		{
			inBlock->lookup(obj->fObj.id, obj->state(), *obj, NULL);
		}
	}
	fA0++;
}


/* -----------------------------------------------------------------------------
	Initialise logical and physical block info objects.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
CFlashStore::initBlocks(void)
{
	for (ArrayIndex i = 0; i < fNumOfBlocks; ++i)
	{
		fPhysBlock[i].init(this, BLOCK(i));		// point physical block to address in store
		fBlock[i]->init(this);
	}
}

NewtonErr
CFlashStore::scanLogForLogicalBlocks(bool * outArg)
{
	NewtonErr err;
	ZAddr logEntryAddr = 0;
PRINTF(("CFlashStore::scanLogForLogicalBlocks()\n"));
ENTER_FUNC
	for ( ; ; )
	{
		// fetch the next flash block log entry
		XFAILIF(err = nextLogEntry(logEntryAddr, &logEntryAddr, kFlashBlockLogEntryType, NULL), if (err == kStoreErrNoMoreObjects) err = noErr;)	// loop exit
		SFlashBlockLogEntry logEntry;
		basicRead(logEntryAddr, &logEntry, sizeof(SFlashBlockLogEntry));
		// validate
		XFAILIF(BLOCK_NUMBER(logEntry.fLogicalAddr) >= fNumOfBlocks, err = kStoreErrNeedsFormat;)
		// ensure our LSN is valid
		if (fLSN < logEntry.fLSN)
			fLSN = logEntry.fLSN;
		if ((logEntry.f40 & 1) == (fDirtyBits & 1))
			fIsROM = true;
		CFlashBlock * block = blockForAddr(logEntry.fLogicalAddr);
		ZAddr blockLogEntryAddr = block->logEntryOffset();
		if (blockLogEntryAddr == 0)
		{
			block->setInfo(&logEntry, outArg);
		}
		else
		{
			SFlashBlockLogEntry blockLogEntry;
			basicRead(blockLogEntryAddr, &blockLogEntry, sizeof(SFlashBlockLogEntry));
			XFAILIF(BLOCK_NUMBER(blockLogEntry.fLogicalAddr) >= fNumOfBlocks, err = kStoreErrNeedsFormat;)
			if (blockLogEntry.fLSN < logEntry.fLSN)
				block->setInfo(&logEntry, NULL);
		}
	}
EXIT_FUNC
	return err;
}


NewtonErr
CFlashStore::scanLogForErasures(void)
{
	NewtonErr err;
	ZAddr logEntryAddr = 0;
PRINTF(("CFlashStore::scanLogForErasures()\n"));
ENTER_FUNC
	for ( ; ; )
	{
		XFAILIF(err = nextLogEntry(logEntryAddr, &logEntryAddr, kEraseBlockLogEntryType, NULL), if (err == kStoreErrNoMoreObjects) err = noErr;)	// loop exit
		SFlashEraseLogEntry logEntry;
		basicRead(logEntryAddr, &logEntry, sizeof(SFlashEraseLogEntry));
		XFAILIF(BLOCK_NUMBER(logEntry.fPhysicalAddr) >= fNumOfBlocks, err = kStoreErrNeedsFormat;)
		if (fLSN < logEntry.fLSN)
			fLSN = logEntry.fLSN;
		CFlashPhysBlock * block = &fPhysBlock[BLOCK_NUMBER(logEntry.fPhysicalAddr)];
		ULong blockLogEntryAddr = block->logEntryOffset();
		if (blockLogEntryAddr == 0)
		{
			block->setInfo(&logEntry);
		}
		else
		{
			SFlashEraseLogEntry blockLogEntry;
			basicRead(blockLogEntryAddr, &blockLogEntry, sizeof(SFlashEraseLogEntry));
			XFAILIF(BLOCK_NUMBER(blockLogEntry.fPhysicalAddr) >= fNumOfBlocks, err = kStoreErrNeedsFormat;)
			if (blockLogEntry.fLSN < logEntry.fLSN)
				block->setInfo(&logEntry);
		}
	}
EXIT_FUNC
	return err;
}


NewtonErr
CFlashStore::scanLogForReservedBlocks(void)
{
	NewtonErr err;
	ZAddr logEntryAddr = 0;
PRINTF(("CFlashStore::scanLogForReservedBlocks()\n"));
ENTER_FUNC
	for ( ; ; )
	{
		XFAILIF(err = nextLogEntry(logEntryAddr, &logEntryAddr, kReservedBlockLogEntryType, NULL), if (err == kStoreErrNoMoreObjects) err = noErr;)	// loop exit
		SReservedBlockLogEntry logEntry;
		basicRead(logEntryAddr, &logEntry, sizeof(SReservedBlockLogEntry));
		if (fLSN < logEntry.fLSN)
			fLSN = logEntry.fLSN;
		CFlashBlock * block = blockForAddr(logEntry.f2C);
		ZAddr blockLogEntryAddr = block->logEntryOffset();
		if (blockLogEntryAddr == 0)
		{
			block->setInfo(&logEntry);
		}
		else
		{
			SReservedBlockLogEntry blockLogEntry;
			basicRead(blockLogEntryAddr, &blockLogEntry, sizeof(SReservedBlockLogEntry));
			if (blockLogEntry.fLSN < logEntry.fLSN)
				block->setInfo(&logEntry);
		}
	}
EXIT_FUNC
	return err;
}


NewtonErr
CFlashStore::newWithinTransaction(PSSId * outObjectId, size_t inSize, bool inArg3)
{
	// object size sanity check
	if (inSize > fBlockSize - KByte)
		return kStoreErrObjectTooBig;

	NewtonErr err = noErr;
PRINTF(("CFlashStore::newWithinTransaction(size=%lu)\n", inSize));
ENTER_FUNC
	vppOn();

	newton_try
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		XTRY
		{
			err = setupForModify(kNoPSSId, *(CStoreObjRef *)0, false, false);
			XFAIL(outObjectId == NULL)
			*outObjectId = 0;
			XFAIL(err)
			if (avail() < storeSlop())
			{
				gc();
				XFAILIF(avail() < storeSlop(), err = kStoreErrStoreFull;)
			}
			XFAIL(err = addObject(kNoPSSId, fIsSRAM ? 3 : 10, inSize, obj, true, inArg3))
			*outObjectId = obj.fObj.id;
			XENDTRY;
		}
		XENDTRY;
		remove(obj);
	}
	newton_catch(exAbort)
	{
		vppOff();
		fObjListHead = NULL;
		fObjListTail = NULL;
	}
	end_try;

	vppOff();
EXIT_FUNC
	return err;
}


/* -----------------------------------------------------------------------------
	Start a new transaction -- create a transaction record object.
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::startTransaction(void)
{
	NewtonErr err = noErr;

	if (fIsSRAM)
		fCompact->x08 = 1;

	else
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		err = addObject(kTxnRecordId, fIsSRAM ? 3 : 10, 4, obj, false, false);
		remove(obj);
	}

	if (err == noErr)
		fIsInTransaction = true;
	return err;
}


/* -----------------------------------------------------------------------------
	End a transaction -- delete its transaction record object.
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::deleteTransactionRecord(void)
{
	NewtonErr err = noErr;

	if (fIsSRAM)
		fCompact->x08 = 0;

	else
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		XTRY
		{
			XFAIL(err = lookup(kTxnRecordId, 0, obj))
			err = obj.dlete();
		}
		XENDTRY;
		XDOFAIL(err == kStoreErrObjectNotFound)
		{
			err = noErr;
		}
		XENDFAIL;
		remove(obj);
	}

	if (err == noErr)
	{
		fIsInTransaction = false;
		f17 = false;
		fTracker->reset();
		f96 = false;
	}
	return err;
}


/* -----------------------------------------------------------------------------
	Mark a commit point in the current transaction -- dirty the first word
	of the transaction record.
	Args:		--
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::markCommitPoint(void)
{
	NewtonErr err = noErr;

	if (fIsSRAM)
		fCompact->x08 = 2;

	else
	{
		CStoreObjRef obj(fVirginBits, this);
		add(obj);
		XTRY
		{
			ULong sp00;
			XFAIL(err = lookup(kTxnRecordId, 0, obj))
			XFAIL(err = obj.read(&sp00, 0, sizeof(sp00)))
			sp00 = fDirtyBits;
			err = obj.write(&sp00, 0, sizeof(sp00));
		}
		XENDTRY;
		remove(obj);
	}

	if (err == noErr)
		f17 = true;
	return err;
}


/* -----------------------------------------------------------------------------
	Commit the current transaction.
	Args:		inArg
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::doCommit(bool inArg)
{
	NewtonErr err = noErr;

	CStoreObjRef obj1(fVirginBits, this);		// f94
	add(obj1);
	CStoreObjRef obj0(fVirginBits, this);		// f78
	add(obj0);

	XTRY
	{
		CFlashIterator iter(this, &obj1, kIterFilterType0);
		bool r6 = false;
		if (inArg && !fTracker->isFull() && !fTracker->isLocked())
		{
			iter.start(fTracker);
			r6 = true;
		}
		while (!iter.done())
		{
			iter.next();
			PSSId objId = obj1.fObj.id;
			switch (obj1.state())
			{
			case 3:
			case 10:
				obj1.setState(fIsSRAM ? 4 : 11);
				fCache->forget(objId, 0);
				break;

			case 5:
			case 12:
				if (obj1.findSuperceeder(obj0) == noErr)
					obj0.setState(fIsSRAM ? 4 : 11);
				obj1.dlete();
				fCache->forget(objId, 0);
				if (BLOCK_NUMBER(obj1.fObjectAddr) != BLOCK_NUMBER(obj0.fObjectAddr))
					blockFor(objId)->objectMigrated(objId, BLOCK_NUMBER(obj0.fObjectAddr));
				break;

			case 6:
			case 13:
				if (obj1.findSuperceeded(obj0) == noErr)
					obj0.dlete();
				obj1.setState(fIsSRAM ? 4 : 11);
				fCache->forget(objId, 0);
				break;

			case 7:
			case 14:
				if (BLOCK_NUMBER(obj1.fObjectAddr) != blockNumberFor(objId))
					blockFor(objId)->zapMigDirEnt(objId);
				obj1.dlete();
				if (r6)
					fTracker->remove(objId);
				fCache->forget(objId, 0);
				break;
			}
		}
	}
	XENDTRY;

	deleteTransactionRecord();
	remove(obj0);
	remove(obj1);

	return err;
}


/* -----------------------------------------------------------------------------
	Abort the current transaction.
	Args:		inArg
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::doAbort(bool inArg)
{
	NewtonErr err = noErr;

	CStoreObjRef obj1(fVirginBits, this);		// f94
	add(obj1);
	CStoreObjRef obj0(fVirginBits, this);		// f78
	add(obj0);

	XTRY
	{
		CFlashIterator iter(this, &obj1, kIterFilterType0);
		while (!iter.done())
		{
			iter.next();
			PSSId objId = obj1.fObj.id;
			switch (obj1.state())
			{
				case 1:
				case 2:
				case 3:
				case 6:
				case 8:
				case 9:
				case 10:
				case 13:
					if (inArg || obj1.fObj.x2 != 2)
					{
						obj1.dlete();
						fCache->forget(objId, 0);
					}
					break;
			}
		}

		long r10 = fA0;
		do
		{
			iter.reset();
			while (!iter.done())
			{
				iter.next();
				PSSId objId = obj1.fObj.id;
				switch (obj1.state())
				{
					case 1:
					case 2:
					case 3:
					case 5:
					case 7:
						if (inArg || obj1.fObj.x2 != 2)
							obj1.setCommittedState();
						break;

					case 12:
					case 14:
						if (inArg || obj1.fObj.x2 != 2)
						{
							XFAIL(err = obj1.clone(fIsSRAM ? 1 : 8, obj0, false))
							XFAIL(err = obj1.dlete())
							XFAIL(err = obj0.setState(fIsSRAM ? 4 : 11))
							fCache->forget(objId, 0);
						}
						break;
				}
				XFAIL(err)
			}
			XFAIL(err)
		}
		while (fA0 != r10);
		XFAIL(err)
		deleteTransactionRecord();
	}
	XENDTRY;

	fCache->forgetAll();
	remove(obj0);
	remove(obj1);

	return err;
}

#pragma mark Object references

/* -----------------------------------------------------------------------------
	Add an object to the store.
	Args:		inObjectId			object’s id
				inState				its state
				inSize				object’s size
				inObj					ref to store object
				inArg4
				inArg5				true => pad objects to fill the block -- for stress testing?
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::addObject(PSSId inObjectId, int inState, size_t inSize, CStoreObjRef & inObj, bool inArg4, bool inArg5)
{
// r4: r6 x r5 r10 sp00 r9
	NewtonErr	err;

	bool doChooseId = inObjectId == kNoPSSId;
	size_t reqdSize = inArg5 ? inSize + 4*KByte : inSize;
PRINTF(("CFlashStore::addObject(id=%d, state=%d, size=%lu)\n", inObjectId,inState,inSize));
ENTER_FUNC

	XTRY
	{
		for ( ; ; )
		{
			// ensure we have a working block
			if (fWorkingBlock == NULL)
			{
				err = chooseWorkingBlock(reqdSize, kIllegalZAddr);
				if (err == kStoreErrStoreFull && fIsSRAM)
				{
					gc();
					err = chooseWorkingBlock(reqdSize, kIllegalZAddr);
				}
				XFAIL(err)
			}

			// caller didn’t supply an id, create one
			if (doChooseId)
				inObjectId = fWorkingBlock->nextPSSId();

			// add the object to the working block
			err = fWorkingBlock->addObject(inObjectId, inState, inSize, inObj, inArg4, inArg5);

			if (err == kStoreErrBlockFull)
			{
				// not enough room in this block, try another
				err = chooseWorkingBlock(reqdSize, kIllegalZAddr);
				if (err == kStoreErrStoreFull && fIsSRAM)
				{
					gc();
					err = chooseWorkingBlock(reqdSize, kIllegalZAddr);
				}
				XFAIL(err)
			}
			else
			{
				XFAIL(err)
				// success!
				fCache->add(inObj);
				if (doChooseId)
					fWorkingBlock->useNextPSSId();
				break;
			}
		}
	}
	XENDTRY;

EXIT_FUNC
	return err;
}


/* -----------------------------------------------------------------------------
	Choose a block that will accommodate the given-sized object.
	Args:		inMinSize		size we’re looking for
				inAddr			address we’d like => block we should use
									kIllegalZAddr => we don’t care which block
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::chooseWorkingBlock(size_t inMinSize, ZAddr inAddr)
{
	NewtonErr err = noErr;
	size_t reqdSize = LONGALIGN(inMinSize) + fBucketSize*sizeof(SDirEnt) + 16;
	CFlashBlock * block;
	ZAddr blockAddr;
	ULong r8;
	ZAddr unusedBlock;
PRINTF(("CFlashStore::chooseWorkingBlock(minSize=%lu, addr=%08X)\n", inMinSize,inAddr));
ENTER_FUNC

	XTRY
	{
		if (inAddr != kIllegalZAddr)
		{
			block = blockForAddr(inAddr);
			if (!block->isVirgin() && block->avail() >= reqdSize)
			{
				fWorkingBlock = block;
EXIT_FUNC
				return noErr;
			}
		}

		block = NULL;
		FOREACH_BLOCK(blok)
			if (blok->isVirgin())
			{
				if (block == NULL)
				{
					block = blok;
					blockAddr = _addr;	// using FOREACH_BLOCK macro detail there
				}
			}
			else if (blok->avail() >= reqdSize)
			{
				fWorkingBlock = blok;
EXIT_FUNC
				return noErr;
			}
		END_FOREACH_BLOCK
		if (block)
		{
			unusedBlock = findUnusedPhysicalBlock();
			if (!isErased(unusedBlock))
				XFAIL(err = syncErase(unusedBlock))
			XFAIL(err = bringVirginBlockOnline(unusedBlock, blockAddr))
			fWorkingBlock = block;
EXIT_FUNC
			return noErr;
		}

		if (fIsSRAM)
		{
			g0C100DD4 += 23;
			ULong firstAddr = fNumOfBlocks/g0C100DD4 * fBlockSize;
			ULong n = firstAddr + fBlockSize;
			if (n >= storeCapacity()) n = 0;
			for ( ; n != firstAddr; )
			{
				block = blockForAddr(n);
				if (block->yield() >= reqdSize)
				{
					XFAIL(err = block->compactInPlace())
					if (block->avail() >= reqdSize)
					{
						fWorkingBlock = block;
EXIT_FUNC
						return noErr;
					}
				}
				n += fBlockSize;
				if (n >= storeCapacity()) n = 0;
			}
EXIT_FUNC
			return kStoreErrStoreFull;
		}

		for (int i = 19; i >= 0; i--)
		{
			bool isBlockAvailable = false;
			ULong topHeuristic = 0;
			blockAddr = kIllegalZAddr;
			unusedBlock = findUnusedPhysicalBlock();

			FOREACH_BLOCK(blok)
				size_t blockYield = blok->yield();
				ULong heuristic = blok->eraseHeuristic(blockYield);
				if (topHeuristic < heuristic)
				{
					topHeuristic = heuristic;
					blockAddr = _addr;	// using FOREACH_BLOCK macro detail there
				}
				if (blockYield >= reqdSize)
					isBlockAvailable = true;
			END_FOREACH_BLOCK
			XFAIL(!isBlockAvailable)

			if (!isErased(unusedBlock))
				XFAIL(err = syncErase(unusedBlock))
			block = blockForAddr(blockAddr);
			block->compactInto(unusedBlock);
			if (!block->isReserved() && block->avail() >= reqdSize)
			{
				fWorkingBlock = block;
EXIT_FUNC
				return noErr;
			}
		}
		XFAIL(err)
		err = kStoreErrStoreFull;
	}
	XENDTRY;

EXIT_FUNC
	return err;
}


/* -----------------------------------------------------------------------------
	Find a spare block.
	Args:		--
	Return:	address/offset of physical block
				kIllegalZAddr => no spare blocks
----------------------------------------------------------------------------- */

ZAddr
CFlashStore::findUnusedPhysicalBlock(void)
{
	for (ZAddr addr = 0, limit = fNumOfBlocks * fBlockSize; addr < limit; addr += fBlockSize)
	{
		if (fPhysBlock[BLOCK_NUMBER(addr)].isSpare())
			return addr;
	}
	return kIllegalZAddr;
}


/* -----------------------------------------------------------------------------
	Initialize a virgin block.
	Args:		inPhysicalAddr		address/offset of physical block
				inLogicalAddr		logical address/offset
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashStore::bringVirginBlockOnline(ZAddr inPhysicalAddr, ZAddr inLogicalAddr)
{
	NewtonErr err;
	CTime now = GetGlobalTime();
PRINTF(("CFlashStore::bringVirginBlockOnline(unusedBlock=%08X, addr=%08X)\n", inPhysicalAddr,inLogicalAddr));
ENTER_FUNC

	XTRY
	{
		CFlashBlock * block = blockForAddr(inLogicalAddr);
		SFlashBlockLogEntry logEntry;
		logEntry.fPhysicalAddr = inPhysicalAddr;
		logEntry.fLogicalAddr = inLogicalAddr;
		logEntry.fEraseCount = 1;
		logEntry.f20 = 0;
		logEntry.f2C = 0;
		logEntry.f30 = 0;
		logEntry.fRandom = rand();
		logEntry.fCreationTime = now.convertTo(kSeconds);
		logEntry.fDirectoryAddr = logEntry.fLogicalAddr + 4;
		logEntry.f44 = 0;
		logEntry.f48 = 0;
		logEntry.f40 = fVirginBits;

		XFAIL(err = block->setInfo(&logEntry, NULL))
		XFAIL(err = block->writeRootDirectory(&logEntry.fDirectoryAddr))
		XFAIL(err = addLogEntryToPhysBlock(kFlashBlockLogEntryType, sizeof(SFlashBlockLogEntry), &logEntry, inPhysicalAddr, NULL))
		XFAIL(err = block->setInfo(&logEntry, NULL))
	}
	XENDTRY;

EXIT_FUNC
	return err;
}


/*------------------------------------------------------------------------------
	Add object ref to linked-list of objects in transaction.
	Args:		inObj
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStore::add(CStoreObjRef & inObj)
{
	inObj.fPrev = fObjListTail;
	inObj.fNext = NULL;

	if (fObjListTail)
		fObjListTail->fNext = &inObj;
	fObjListTail = &inObj;

	if (fObjListHead == NULL)
		fObjListHead = &inObj;
}


/*------------------------------------------------------------------------------
	Remove object ref from linked-list of objects in transaction.
	The original always does:
		obj.fStore->remove(&obj);
	to remove an object reference.
	We use the object’s store in this function to make the call simpler:
		remove(obj);
	Args:		inObj
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStore::remove(CStoreObjRef & inObj)
{
	CFlashStore * objStore = inObj.fStore;
	CStoreObjRef * obj;

	if ((obj = inObj.fNext) != NULL)
		obj->fPrev = inObj.fPrev;
	else
		objStore->fObjListTail = inObj.fPrev;

	if ((obj = inObj.fPrev) != NULL)
		obj->fNext = inObj.fNext;
	else
		objStore->fObjListHead = inObj.fNext;
}


/*------------------------------------------------------------------------------
	Translate logical address/offset to physical address/offset in store.
	(Map to a different block if necessary.)
	Args:		inAddr
	Return:	physical address/offset
------------------------------------------------------------------------------*/

ZAddr
CFlashStore::translate(ZAddr inAddr)
{
PRINTF((" (%08X -> %08X)\n", inAddr,blockForAddr(inAddr)->physAddr()+(inAddr & fBlockSizeMask)));
	return blockForAddr(inAddr)->physAddr() + (inAddr & fBlockSizeMask);
}


/*------------------------------------------------------------------------------
	Validate a PSSId.
	A PSSId is an address within the store, shifted down 2 since objects are
	always long-aligned.
	Args:		inObjectId
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::validateIncomingPSSId(PSSId inObjectId)
{
	return (/*inObjectId > 0 &&*/ blockNumberFor(inObjectId) < fNumOfBlocks) ? noErr : kStoreErrBadPSSId;
}


/*------------------------------------------------------------------------------
	Look up an object reference from its PSSId.
	Args:		inObjectId		object id
				inState			object state
				ioObj				on return, updated store object
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::lookup(PSSId inObjectId, int inState, CStoreObjRef & ioObj)
{
PRINTF(("CFlashStore::lookup(id=%d, state=%d)\n", inObjectId,inState));
ENTER_FUNC
	// look in the cache first
	ZAddr dirAddr;
	if ((dirAddr = fCache->lookup(inObjectId, inState)) != kIllegalZAddr)
	{
PRINTF(("  -> %08X from the cache\n", dirAddr));
		ioObj.set(kIllegalZAddr, dirAddr);
EXIT_FUNC
		return noErr;
	}

	// sanity check -- block must be in use
	NewtonErr err;
	CFlashBlock * block = blockFor(inObjectId);
	if (block->isVirgin())
	{
EXIT_FUNC
		return kStoreErrObjectNotFound;
	}

	// look up in the block
	long sp00;
	err = block->lookup(inObjectId, inState, ioObj, &sp00);

	if (err == kStoreErrObjectNotFound)
	{
		// not there, try..?
PRINTF(("  not in the block\n"));
		if (sp00 >= 0 && (inState <= 0 || inState == 4 || inState == 11))
		{
			block = blockFor(sp00);
			if (!block->isVirgin())
				err = block->lookup(inObjectId, inState, ioObj, NULL);
		}
	}

	if (err == kStoreErrObjectNotFound)
	{
		// still not there, try every other block
PRINTF(("  not in the extra block\n"));
		ZAddr startAddr = block->firstObjAddr();
		for (ZAddr addr = startAddr + fBlockSize; err == kStoreErrObjectNotFound; addr += fBlockSize)
		{
			if (addr > storeCapacity()
			|| blockForAddr(addr)->isVirgin())
				addr = 0;	// reached the last block, restart at the beginning
			if ((addr & ~fBlockSizeMask) == (startAddr & ~fBlockSizeMask))
				break;		// reached the beginning again, exit with kStoreErrObjectNotFound
			err = blockForAddr(addr)->lookup(inObjectId, inState, ioObj, NULL);
		}
	}

	if (err == noErr)
		fCache->add(ioObj);

EXIT_FUNC
	return err;
}


/*------------------------------------------------------------------------------
	Set up for reading an object.
	Args:		inObjectId		object id
				ioObj				on return, ref to store object
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::setupForRead(PSSId inObjectId, CStoreObjRef & ioObj)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (f17)
			XFAIL(err = recoveryCheck(true))
		XFAIL(inObjectId == kNoPSSId)	// not actually a failure
		XFAIL(err = validateIncomingPSSId(inObjectId))
		err = lookup(inObjectId, 0, ioObj);
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Set up for modifying an object.
	Args:		inObjectId
				ioObj
				inArg3		inCurrentTransaction?
				inArg4		startTransaction?
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::setupForModify(PSSId inObjectId, CStoreObjRef & ioObj, bool inArg3, bool inArg4)
{
	NewtonErr err = noErr;
PRINTF(("CFlashStore::setupForModify(id=%d, %u,%u)\n", inObjectId,inArg3,inArg4));
ENTER_FUNC

	XTRY
	{
		XFAILIF(isROM(), err = kStoreErrWriteProtected;)
		XFAILIF(isWriteProtected(), err = f17 ? kStoreErrWPButNeedsRepair : kStoreErrWriteProtected;)
		XFAILIF(inArg3 && fLockCount == 0, err = kStoreErrNotInTransaction;)

		if (fStoreDriver == &fA4)
		{
			fCompact->x34.init(fStoreAddr, fNumOfBlocks * fBlockSize, NULL, 0);
			fStoreDriver = &fCompact->x34;	// huh?
		}

		if (f17)
			XFAIL(err = recoveryCheck(true))
		if (inArg4 && !fIsInTransaction)
			XFAIL(err = startTransaction())
		XFAIL(inObjectId == kNoPSSId)	// not actually a failure
		XFAIL(err = validateIncomingPSSId(inObjectId))
		err = lookup(inObjectId, 0, ioObj);
	}
	XENDTRY;
EXIT_FUNC
	return err;
}


/*------------------------------------------------------------------------------
	Read yer actual bits from the store.
	Args:		inAddr		offset from the start of the store
				outBuf
				inLen
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashStore::basicRead(ZAddr inAddr, void * outBuf, size_t inLen)
{
PRINTF(("CFlashStore::basicRead(addr=%08X, size=%lu)\n", inAddr,inLen));
ENTER_FUNC
	if (fIsROM)
		// original does optimised long/byte copy which amounts to:
		memmove(outBuf, fStoreAddr + inAddr, inLen);

	else if (fUseRAM)		// sic -- fIsSRAM, surely?
		fStoreDriver->read((char *)outBuf, inAddr, inLen);

	else if (fIsInternalFlash)
		fFlash->read(inAddr, inLen, (char *)outBuf);

	else
		memmove(outBuf, fStoreAddr + inAddr, inLen);

#if forDebug
DumpHex(outBuf, inLen);
#endif
EXIT_FUNC
	return noErr;
}


NewtonErr
CFlashStore::basicWrite(ZAddr inAddr, void * inBuf, size_t inLen)
{
	NewtonErr err = noErr;
PRINTF(("CFlashStore::basicWrite(addr=%08X, size=%lu)\n", inAddr,inLen));
ENTER_FUNC
#if forDebug
DumpHex(inBuf, inLen);
#endif

	fCachedUsedSize = 1;	// invalidate

	for ( ; ; )
	{
		newton_try
		{
			if (fIsSRAM)
				fStoreDriver->write((char *)inBuf, inAddr, inLen);

			else if (fIsInternalFlash)
				err = fFlash->write(inAddr, inLen, (char *)inBuf);

			else
			{
				err = fFlash->write(inAddr, inLen, (char *)inBuf);
				// can’t trust the old flash -- check the write worked
				if (err == noErr
				&&  memcmp(fStoreAddr + inAddr, inBuf, inLen) != 0)
					err = kFlashErrWriteFailed;
			}
		}
		newton_catch(exAbort)
		{
			err = kStoreErrWriteProtected;
		}
		end_try;

		if (err == kStoreErrWriteProtected
		||  err == kOSErrWriteProtected)
			sendAlertMgrWPBitch(0);
			// and try again
		else
		{
			if (err == kFlashErrWriteFailed)
				err = kStoreErrWriteError;
			break;
		}
	}

EXIT_FUNC
	return err;
}


NewtonErr
CFlashStore::basicCopy(ZAddr inFromAddr, ZAddr inToAddr, size_t inLen)
{
	NewtonErr err = noErr;
PRINTF(("CFlashStore::basicCopy(from=%08X, to=%08X)\n", inFromAddr,inToAddr));
ENTER_FUNC

	if (fStoreDriver)
	{
		for ( ; ; )
		{
			newton_try
			{
				fStoreDriver->copy(inFromAddr, inToAddr, inLen);
			}
			newton_catch(exAbort)
			{
				err = kStoreErrWriteProtected;
			}
			end_try;

			if (err != noErr)
				sendAlertMgrWPBitch(0);
				// and try again
			else
				break;
		}
	}

	else if (fIsInternalFlash)
		err = fFlash->copy(inFromAddr, inToAddr, inLen);

	else
		err = basicWrite(inToAddr, fStoreAddr + inFromAddr, inLen);

EXIT_FUNC
	return err;
}


NewtonErr
CFlashStore::zap(ZAddr inAddr, size_t inLen)
{
	NewtonErr err = noErr;
PRINTF(("CFlashStore::zap(addr=%08X, size=%lu)\n", inAddr,inLen));
ENTER_FUNC

	if (fIsSRAM)
	{
		bool isDone;
		for (isDone = false; !isDone; )
		{
			newton_try
			{
				fStoreDriver->set(inAddr, inLen, fDirtyBits);
				isDone = true;
			}
			newton_catch(exAbort)
			{
				sendAlertMgrWPBitch(0);
			}
			end_try;
		}
	}

	else if (inLen <= sizeof(ULong))
	{
		ULong zapBuf = fDirtyBits;
		err = basicWrite(inAddr, &zapBuf, inLen);
	}

	else
	{
		UChar zapBuf[64];
		memset(zapBuf, fDirtyBits, 64);
		size_t lenDone;
		for (size_t lenRemaining = inLen; lenRemaining > 0; lenRemaining -= lenDone)
		{
			lenDone = MIN(lenRemaining, 64);
			XFAIL(err = basicWrite(inAddr, zapBuf, lenDone))
		}
	}

EXIT_FUNC
	return err;
}


bool
CFlashStore::isRangeVirgin(ZAddr inAddr, size_t inLen)
{
	if (fIsInternalFlash)
		return fFlash->isVirgin(translate(inAddr), inLen);

	char * p = fStoreAddr + translate(inAddr);
	for ( ; inLen > 0; inLen--)
	{
		if (*p++ != kVirginBits)	// wouldn’t this be better as fVirginBits?
			return false;
	}
	return true;
}

#pragma mark Alerts

bool
CFlashStore::cardWPAlertProc(ULong inArg1, void * inArg2)
{
	bool isWP = false;
	if (fFlash)
		fFlash->getWriteProtected(&isWP);
#if 0		// no CCardHandler implementation yet -- any point?
	else if (fCardHandler)
		isWP = (fCardHandler->cardStatus() & 0x04) == 0;
#endif
	else
		isWP = false;
	return !isWP;
}


NewtonErr
CFlashStore::sendAlertMgrWPBitch(int inAlertSelector)
{
	NewtonErr err = noErr;
#if 0
	CUAsyncMessage sp20;		// is this actually used?
	CUPort alertMgr;
	CUNameServer ns;
	ULong thing, spec;

	Sleep(100*kMilliseconds);

	ns.lookup("alrt", kUPort, &thing, &spec);
	alertMgr = (ObjectId)thing;

	CCardAlertEvent * cardAlert = new CCardAlertEvent;
	if (!fIsInTransaction)
		inAlertSelector = 0;
	if (fIsFormatting)
		inAlertSelector = 1;

	if (inAlertSelector == 0)
		cardAlert->f10->init(cardWPAlertProc, this);		// CCardUnWPAlertDialog
	else if (inAlertSelector == 1)
		cardAlert->f10->init(cardWPAlertProc, this);		// CCardRepairAlertDialog
	cardAlert->f10->setFilterData((void *)fSocketNo);
	sp20.init(true);

	do
	{
		if (fIsInTransaction)
			touchMe();
		err = alertMgr.send(cardAlert, sizeof(cardAlert));
		Sleep(500*kMilliseconds);
		if (fIsInTransaction)
			touchMe();
	} while (isWriteProtected());

	delete cardAlert;
#endif
	return err;
}

