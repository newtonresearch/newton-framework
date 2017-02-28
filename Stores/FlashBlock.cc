/*
	File:		FlashBlock.cc

	Contains:	Flash block implementation.
					A flash block is the unit that can be erased and rewritten -
					typically 64K in size.
					A CFlashBlock represents such a block.

	Written by:	Newton Research Group, 2010.
*/
#undef forDebug
//#define forDebug 1
#define debugLevel 2

#include "FlashStore.h"
#include "FlashIterator.h"


#pragma mark SFlashLogEntry
/* -----------------------------------------------------------------------------
	S F l a s h L o g E n t r y
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Is this log entry valid?
	Check the signatures.
	Args:		--
	Return:	true => log entry looks good
----------------------------------------------------------------------------- */

bool
SFlashLogEntry::isValid(ZAddr	inAddr)
{
	return fPhysSig == (inAddr ^ 'dyer')
		 && fPhysSig2 == (~inAddr ^ 'foo!')
		 && fNewtSig == 'newt'
		 && fSize < 256;
}


/* -----------------------------------------------------------------------------
	Return the physical address/offset in flash of the log entry.
	Args:		--
	Return:	true => signature is good
----------------------------------------------------------------------------- */

ZAddr
SFlashLogEntry::physOffset(void)
{
	return fPhysSig ^ 'dyer';
}


#pragma mark -
#pragma mark CFlashPhysBlock
/* -----------------------------------------------------------------------------
	C F l a s h P h y s B l o c k
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Initialise.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

NewtonErr
CFlashPhysBlock::init(CFlashStore * inStore, ZAddr inPhysAddr)
{
	fStore = inStore;
	f04 = kIllegalZAddr;
	fBlockAddr = inPhysAddr;
	fLogEntryOffset = 0;
	fEraseCount = 1;
	f14 = false;
	fIsReserved = false;
}


void
CFlashPhysBlock::setInfo(SFlashBlockLogEntry * info)
{
	fLogEntryOffset = info->physOffset();
	fEraseCount = info->fEraseCount;
	f04 = info->fLogicalAddr;
	f14 = false;
	fIsReserved = false;
PRINTF(("CFlashPhysBlock::setInfo(fLogEntryOffset=%08X, fEraseCount=%d, f04=%08X)\n", fLogEntryOffset,fEraseCount,f04));
}


void
CFlashPhysBlock::setInfo(SFlashEraseLogEntry * info)
{
	fLogEntryOffset = info->physOffset();
	fEraseCount = info->f20;
	f04 = kIllegalZAddr;
	f14 = !info->f24;
	fIsReserved = false;
PRINTF(("CFlashPhysBlock::setInfo(fLogEntryOffset=%08X, fEraseCount=%d, f14=%d) for SFlashEraseLogEntry\n", fLogEntryOffset,fEraseCount,f14));
}


void
CFlashPhysBlock::setInfo(SReservedBlockLogEntry * info)
{
	fLogEntryOffset = info->physOffset();
	fEraseCount = info->fEraseCount;
	f04 = info->f2C;
	f14 = false;
	fIsReserved = true;
PRINTF(("CFlashPhysBlock::setInfo(fLogEntryOffset=%08X, fEraseCount=%d, f04=%08X) for SReservedBlockLogEntry\n", fLogEntryOffset,fEraseCount,f04));
}


void
CFlashPhysBlock::setSpare(CFlashPhysBlock * inBlock, ULong inArg2)
{
PRINTF(("CFlashPhysBlock::setSpare(block=%p)\n", inBlock));
ENTER_FUNC
	XTRY
	{
		VAddr physAddr = inBlock->getPhysicalOffset();
		if (fIsReserved)
		{
			SReservedBlockLogEntry logEntry;
			fStore->basicRead(fLogEntryOffset, &logEntry, sizeof(SReservedBlockLogEntry));
			logEntry.fPhysicalAddr = physAddr;
			logEntry.fEraseCount = inBlock->eraseCount();
			XFAIL(fStore->addLogEntryToPhysBlock('zblk', sizeof(SReservedBlockLogEntry), &logEntry, physAddr, NULL))
			fStore->blockForAddr(f04)->setInfo(&logEntry);
		}
		else
		{
			SFlashBlockLogEntry logEntry;
			fStore->basicRead(fLogEntryOffset, &logEntry, sizeof(SFlashBlockLogEntry));
			logEntry.fDirectoryAddr = inArg2;
			logEntry.fPhysicalAddr = physAddr;
			logEntry.fEraseCount = inBlock->eraseCount();
			XFAIL(fStore->addLogEntryToPhysBlock('fblk', sizeof(SFlashBlockLogEntry), &logEntry, physAddr, NULL))
			fStore->blockForAddr(f04)->setInfo(&logEntry, NULL);
		}
		XFAIL(fStore->zapLogEntry(fLogEntryOffset))

		SFlashEraseLogEntry logEntry;
		logEntry.fPhysicalAddr = fBlockAddr;
		logEntry.f20 = fEraseCount + 1;
		logEntry.f24 = true;
		XFAIL(fStore->addLogEntryToPhysBlock('eblk', sizeof(SFlashEraseLogEntry), &logEntry, physAddr, NULL))
		setInfo(&logEntry);

		fStore->syncErase(fBlockAddr);
	}
	XENDTRY;

EXIT_FUNC
}


#pragma mark -
#pragma mark CFlashBlock
/* -----------------------------------------------------------------------------
	C F l a s h B l o c k
	A CFlashBlock instance keeps track of the objects stored within a 64K block
	of flash memory.
	Each object is preceded by a StoreObjHeader defining its state.
----------------------------------------------------------------------------- */
#define XCRITICAL(er) if (er) return er;

/* Translate address/offset to block number */
#define BLOCK_NUMBER(_a) (_a >> fStore->fBlockSizeShift)

extern ULong		HashPSSId(PSSId inObjectId);
extern bool			IsValidPSSId(PSSId inObjectId);
extern size_t		InternalStoreInfo(int inSelector);

/* -----------------------------------------------------------------------------
	Initialize block information.
	Args:		inStore
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::init(CFlashStore * inStore)
{
	fStore = inStore;
	fFirstObjAddr = 0;
	fPhysBlockAddr = kIllegalZAddr;
	fDirBase = 0;
	fNextObjAddr = 0;
	fZappedSize = 0;
	fAvailableId = 0;
	fValidity = 0;	// original doesn’t bother
	return noErr;
}


/* -----------------------------------------------------------------------------
	Write the root directory for the block.
	The root directory contains refs to all objects within the block.
	Args:		outAddr		on return, the address/offset of the root directory object
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::writeRootDirectory(ZAddr * outAddr)
{
	NewtonErr err;
PRINTF(("CFlashBlock::writeRootDirectory()\n"));
ENTER_FUNC
	CStoreObjRef obj(fStore->fVirginBits, fStore);	// original says 0 for fStore->fVirginBits
	fStore->add(obj);
	err = addObject(kRootDirId, 0, rootDirSize(), obj, false, false);
	if (outAddr)
		*outAddr = obj.fObjectAddr;
PRINTF(("-> %08X\n", obj.fObjectAddr));
	fStore->remove(obj);
EXIT_FUNC
	return err;
}


/* -----------------------------------------------------------------------------
	Set address parameters for the block.
	Args:		info			log entry defining the block’s place
				outHuh		on return, huh?
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::setInfo(SFlashBlockLogEntry * info, bool * outHuh)
{
	NewtonErr err;

	fFirstObjAddr = info->fLogicalAddr;
	fPhysBlockAddr = info->fPhysicalAddr;
	fDirBase = info->fDirectoryAddr;
	fNextObjAddr = info->fLogicalAddr + 4;
	fZappedSize = 0;
	fValidity = info->fRandom ^ info->fCreationTime;
	if (fAvailableId == 0)
		fAvailableId = kFirstObjectId + (BLOCK_NUMBER(fFirstObjAddr) << fStore->f60);
PRINTF(("CFlashBlock::setInfo(fPhysBlockAddr=%08X, fDirBase=%08X, fFirstObjAddr = %08X)\n", fPhysBlockAddr,fDirBase,fFirstObjAddr));
ENTER_FUNC

	physBlock()->setInfo(info);

	XTRY
	{
		// iterate over all objects in the block
		ZAddr objAddr = fFirstObjAddr;
		for ( ; ; )
		{
			// read object header
			StoreObjHeader obj;
			XFAILIF(err = nextObject(objAddr, &objAddr, true), if (err == kStoreErrNoMoreObjects) err = noErr;)	// loop exit
			XFAIL(err = readObjectAt(objAddr, &obj))
PRINTF(("object = { id:%d, size:%d, x2:%d }%s\n", obj.id, obj.size, obj.x2, (obj.zapped==0)?" ZAPPED!":""));
			// step to next
			fNextObjAddr = objAddr + sizeof(StoreObjHeader) + LONGALIGN(obj.size);
			// update zapped size
			if (ISDIRTYBIT(obj.zapped))
				fZappedSize += (sizeof(StoreObjHeader) + LONGALIGN(obj.size));
			else
			{
				if (outHuh && obj.x2 == 2)
					*outHuh = true;
			}
			// ensure next available id is valid
			if (obj.id >= kFirstObjectId)
			{
				CFlashBlock * block = fStore->blockFor(obj.id);
				if (block->fAvailableId <= obj.id)
					block->fAvailableId = obj.id + 1;
			}
		}
	}
	XENDTRY;

EXIT_FUNC
	return err;
}


/* -----------------------------------------------------------------------------
	Set address parameters for the RESERVED block.
	Args:		info			log entry defining the block’s place
				outHuh		on return, huh?
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::setInfo(SReservedBlockLogEntry * info)	// called by NewReservedBlock()
{
	fFirstObjAddr = info->f2C;
	fPhysBlockAddr = info->fPhysicalAddr;
PRINTF(("CFlashBlock::setInfo(fPhysBlockAddr=%08X, ffFirstObjAddr = %08X)\n", fPhysBlockAddr,fFirstObjAddr));
ENTER_FUNC
	physBlock()->setInfo(info);
EXIT_FUNC
	return noErr;
}


NewtonErr
CFlashBlock::lookup(PSSId inId, int inState, CStoreObjRef& ioObj, int * outArg4)
{
	if (isReserved())
	{
		if (outArg4)
			*outArg4 = -1;
		return kStoreErrObjectNotFound;
	}

	CFlashIterator iter(fStore, &ioObj, rootDirEnt(inId), kIterFilterType2);
	return iter.lookup(inId, inState, outArg4);
}


/* -----------------------------------------------------------------------------
	Add an object to the block.
	Args:		inObjectId			object’s id
				inState				its state
				inSize				object’s size
				inObj					on return, valid store object ref
				inArg5
				inArg6				true => pad objects to fill 4K -- for stress testing?
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::addObject(PSSId inObjectId, int inState, size_t inSize, CStoreObjRef& ioObj, bool inArg5, bool inArg6)
{
	NewtonErr err = noErr;
PRINTF(("CFlashBlock::addObject(id=%d, state=%d, size=%lu) phys block=%08X\n", inObjectId,inState,inSize,fPhysBlockAddr));
ENTER_FUNC

	XTRY
	{
		XFAILIF(isReserved(), err = kStoreErrBlockFull;)

		size_t objSize;
		ZAddr dirAddr = kIllegalZAddr;
		if (inObjectId >= 17)	// object ids <= 16 are directories?
			XFAIL(err = addDirEnt(inObjectId, kIllegalZAddr, &dirAddr, NULL))
		objSize = sizeof(StoreObjHeader) + LONGALIGN(inSize);
		size_t padding;
		if (inArg6)
		{
			size_t rawObjSize = sizeof(StoreObjHeader) + (inSize & 0x0FFF);
			padding = LONGALIGN(ALIGN(fNextObjAddr + rawObjSize, 4*KByte) - (fNextObjAddr + rawObjSize));
		}
		else
			padding = 0;
		size_t reqdSize = objSize + padding;
//		sp10 = inSize & 0x0000FFFF;	// size; max 64K
//		sp0C = inObjectId & 0x0FFFFFFF;	// id?
retry:
		XFAILIF(reqdSize > avail(), err = kStoreErrBlockFull;)
		if (padding != 0)
		{
			err = fStore->zap(fStore->translate(fNextObjAddr), padding);
			fNextObjAddr += padding;
			XFAIL(err)
		}
		// create store object ref
		memset(&ioObj.fObj, fStore->fVirginBits, sizeof(StoreObjHeader));
		ioObj.fObj.id = inObjectId;
		ioObj.fObj.size = inSize;
		ioObj.fObj.transBits = gObjectStateToTransBits[inState] ^ fStore->fVirginBits;	// original just negates it
		ioObj.fObj.validity = fStore->fDirtyBits;
		if (inArg5)
			ioObj.fObj.x2 = 2;
		if (inArg6)
			ioObj.fObj.x7 = fStore->fDirtyBits;
		if (inObjectId < kFirstObjectId && fStore->fIsSRAM)
			fStore->fStoreDriver->set(fStore->translate(fNextObjAddr) + sizeof(StoreObjHeader), inSize, fStore->fVirginBits);
PRINTF(("writing StoreObjHeader\n"));
		NewtonErr wrErr = basicWrite(fNextObjAddr, &ioObj.fObj, sizeof(StoreObjHeader));
		if (wrErr)
		{
			// write failed! zap what we tried to write
			err = fStore->zap(fStore->translate(fNextObjAddr), sizeof(StoreObjHeader));
			// try the following bit of the block
			fNextObjAddr += sizeof(StoreObjHeader);
			XFAIL(err)
			if (!inArg6 && wrErr == kStoreErrWriteError)
				goto retry;
			XFAIL(err = wrErr)
		}

		ZAddr objAddr = fNextObjAddr;
		fNextObjAddr += objSize;
		if (dirAddr != kIllegalZAddr)
		{
retry2:
			err = setDirEntOffset(dirAddr, objAddr);
			if (err == kStoreErrWriteError)
			{
				// try somewhere else
				err = addDirEnt(inObjectId, kIllegalZAddr, &dirAddr, NULL);
				if (err == kStoreErrBlockFull)
					zapObject(objAddr);
				else
					goto retry2;
			}
		}
		if (err == noErr)
			ioObj.set(objAddr, dirAddr);
	}
	XENDTRY;

EXIT_FUNC
	return err;
}


/* -----------------------------------------------------------------------------
	Return header info for an object at an address in flash.
	Args:		inAddr		address/offset in block of object
				outObject	on return, object header
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::readObjectAt(ZAddr inAddr, StoreObjHeader * outObject)
{
	return fStore->basicRead(fStore->translate(inAddr), outObject, sizeof(StoreObjHeader));
}


/* -----------------------------------------------------------------------------
	Destroy an object at an address in flash.
	Args:		inAddr		address/offset in block of object
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::zapObject(ZAddr inAddr)
{
	NewtonErr err;
	XTRY
	{
		StoreObjHeader obj;
		ZAddr physAddr = fStore->translate(inAddr);	// really? don’t we translate in basicWrite() too?
		// read object header
		XFAIL(err = readObjectAt(inAddr, &obj))
		// validate it
		XFAIL(!obj.isValid(fStore))
		XFAIL(!ISVIRGINBIT(obj.x61))
		// accumulate size of zapped objects
		fZappedSize += (sizeof(StoreObjHeader) + LONGALIGN(obj.size));
		// mark the object as zapped
		obj.zapped = (fStore->fDirtyBits & 1);
		// write the header back
		err = basicWrite(physAddr, &obj, sizeof(StoreObjHeader));
	}
	XENDTRY;
	return err;
}


/* -----------------------------------------------------------------------------
	Add a directory object at an address in flash.
	Args:		inId			id of object
				inAddr		address/offset in block of object
				outDirAddr	on return, address/offset in block of directory
				outDirEnt	on return, copy of directory entry
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::addDirEnt(PSSId inId, ZAddr inAddr, ZAddr * outDirAddr, SDirEnt * outDirEnt)
{
	NewtonErr err;
	SDirEnt dirEnt;
	ZAddr dirAddr, rootAddr = rootDirEnt(inId);			// sp00
PRINTF(("CFlashBlock::addDirEnt(id=%u, addr=%08X)\n", inId,inAddr));
ENTER_FUNC

loop:
	for (dirAddr = rootAddr + 4; dirAddr < rootAddr + bucketSize() * sizeof(SDirEnt) - 8; dirAddr += sizeof(SDirEnt))
	{
PRINTF(("  %08X ?", dirAddr));
		err = readDirEntAt(dirAddr, &dirEnt);
		XCRITICAL(err)
		if (ISVIRGINDIRENT(&dirEnt)
		|| (fStore->fIsSRAM && !dirEnt.isValid(fStore))
		|| (ISDIRTYBIT(dirEnt.x6) && ISDIRTYBIT(dirEnt.x2)))
		{
			*(ULong*)&dirEnt = fStore->fVirginBits;
			if (inAddr != kIllegalZAddr)
				dirEnt.zaddr = MAKE_OFFSET(inAddr);
			dirEnt.x3 = fStore->fDirtyBits & 1;	// mark in-use
			err = basicWrite(dirAddr, &dirEnt, sizeof(SDirEnt));
			if (err == kStoreErrBlockFull)
				zapDirEnt(dirAddr);
			else
			{
				XCRITICAL(err)
				if (outDirAddr)
					*outDirAddr = dirAddr;
				if (outDirEnt)
					*outDirEnt = dirEnt;
PRINTF(("  -> %08X\n", dirAddr));
EXIT_FUNC
				return noErr;
			}
		}
		else if (dirEnt.isValid(fStore) && !ISDIRTYBIT(dirEnt.x6) && ISDIRTYBIT(dirEnt.x7))
		{
			// start with new root
			rootAddr = MAKE_ADDR(dirEnt.zaddr);
PRINTF(("  -- new root %08X\n", rootAddr));
			goto loop;
		}
	}

	for (dirAddr = rootAddr + bucketSize() * sizeof(SDirEnt) - 8; dirAddr < rootAddr + bucketSize() * sizeof(SDirEnt); dirAddr += sizeof(SDirEnt))
	{
PRINTF(("x %08X ?", dirAddr));
		err = readDirEntAt(dirAddr, &dirEnt);
		XCRITICAL(err)
		if (dirEnt.isValid(fStore) && !ISDIRTYBIT(dirEnt.x6) && ISDIRTYBIT(dirEnt.x7))
		{
			rootAddr = MAKE_ADDR(dirEnt.zaddr);
PRINTF(("x -- new root %08X\n", rootAddr));
			goto loop;
		}
	}

	err = extendDirBucket(rootAddr + bucketSize() * sizeof(SDirEnt) - 8, &rootAddr);
	XCRITICAL(err)
PRINTF(("  ---- extending directory %08X\n", rootAddr));
	goto loop;

EXIT_FUNC
	return err;
}


/* -----------------------------------------------------------------------------
	Read directory entry at an address in flash.
	Args:		inAddr		address/offset in block of directory entry
				outDirEnt	on return, copy of the directory entry
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::readDirEntAt(ZAddr inAddr, SDirEnt * outDirEnt)
{
	return fStore->basicRead(fStore->translate(inAddr), outDirEnt, sizeof(SDirEnt));
}


/* -----------------------------------------------------------------------------
	Destroy a directory entry at an address in flash.
	Args:		inAddr		address/offset in block of directory entry
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::zapDirEnt(ZAddr inAddr)
{
	return fStore->zap(fStore->translate(inAddr), sizeof(SDirEnt));
}


/* -----------------------------------------------------------------------------
	Update directory entry at an address in flash with object address/offset.
	Args:		inAddr		address/offset in block of directory entry
				inObjAddr	address/offset in block of object
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::setDirEntOffset(ZAddr inAddr, ZAddr inObjAddr)
{
	SDirEnt dirEnt;
PRINTF(("CFlashBlock::setDirEntOffset(dir=%08X, obj=%08X)\n", inAddr,inObjAddr));
ENTER_FUNC
	readDirEntAt(inAddr, &dirEnt);
	dirEnt.zaddr = MAKE_OFFSET(inObjAddr);
EXIT_FUNC
	return basicWrite(inAddr, &dirEnt, sizeof(SDirEnt));
}


/* -----------------------------------------------------------------------------
	Return the address/offset of the root directory entry.
	Args:		inId			id of the directory entry
	Return:	address/offset
----------------------------------------------------------------------------- */

ZAddr
CFlashBlock::rootDirEnt(PSSId inId)
{
	return fDirBase + sizeof(StoreObjHeader) + (HashPSSId(inId) & (bucketCount() - 1)) * bucketSize() * sizeof(SDirEnt);
}


/* -----------------------------------------------------------------------------
	Return the size of the root directory.
	Args:		--
	Return:	size
----------------------------------------------------------------------------- */

size_t
CFlashBlock::rootDirSize(void)
{
	return 4 + bucketCount() * bucketSize() * sizeof(SDirEnt);
}


/* -----------------------------------------------------------------------------
	Extend a directory entry bucket.
	Args:		inAddr		address/offset in block of directory entry
				outAddr		on return, address/offset in block of extended directory
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::extendDirBucket(ZAddr inAddr, ZAddr * outAddr)
{
	NewtonErr err;
	// begin transaction
	CStoreObjRef obj(fStore->fVirginBits, fStore);	// original says 0 for fStore->fVirginBits
	fStore->add(obj);
	XTRY
	{
		// add an object holding a bucket full of directory entries
		XFAIL(err = addObject(kRootDirId, 0, bucketSize()*sizeof(SDirEnt), obj, false, false))

		// return its address/offset
		ZAddr addr = obj.fObjectAddr + sizeof(StoreObjHeader);
		if (outAddr)
			*outAddr = addr;

		// create a zapped directory entry
		SDirEnt dirEnt;
		*(ZAddr*)&dirEnt = fStore->fVirginBits;
		dirEnt.zaddr = MAKE_OFFSET(addr);
		dirEnt.x7 = fStore->fDirtyBits & 1;
		dirEnt.x3 = fStore->fDirtyBits & 1;	// mark in-use
		// overwrite the original
		if ((err = basicWrite(inAddr, &dirEnt, sizeof(SDirEnt))) == kStoreErrWriteError)
		{
			zapDirEnt(inAddr);
			inAddr += sizeof(SDirEnt);
			XFAILIF(err = basicWrite(inAddr, &dirEnt, sizeof(SDirEnt)), err = kStoreErrBlockFull;)
		}
	}
	XENDTRY;
	// end transaction
	fStore->remove(obj);
	return err;
}


NewtonErr
CFlashBlock::addMigDirEnt(long inArg1, long inArg2)
{
	NewtonErr err;
	XTRY
	{
		SDirEnt dirEnt;
		ZAddr dirAddr;
		XFAIL(err = addDirEnt(fStore->PSSIdFor(BLOCK_NUMBER(fFirstObjAddr), inArg1), kIllegalZAddr, &dirAddr, &dirEnt))
		dirEnt.setMigratedObjectInfo(inArg1, inArg2);
		dirEnt.x2 = fStore->fDirtyBits & 1;
		dirEnt.x6 = fStore->fDirtyBits & 1;
		XFAILIF(err = basicWrite(dirAddr, &dirEnt, sizeof(SDirEnt)), zapDirEnt(dirAddr);)
	}
	XENDTRY;
	return err;
}


NewtonErr
CFlashBlock::zapMigDirEnt(ZAddr inAddr)
{
	if (!isVirgin() && !isReserved())
	{
		ULong objNo = fStore->objectNumberFor(inAddr);
		SDirEnt dirEnt;
		CFlashIterator iter(fStore, &dirEnt, rootDirEnt(inAddr));
		while (!iter.done())
		{
			iter.next();
			if (ISDIRTYBIT(dirEnt.x2))
			{
				int sp00, sp04;
				dirEnt.getMigratedObjectInfo(&sp04, &sp00);
				if (sp04 == objNo)
					return zapDirEnt(iter.getf28());
			}
		}
	}
	return noErr;
}


void
CFlashBlock::objectMigrated(ZAddr inAddr, long inArg2)
{
	if (!isVirgin())
	{
		ULong objNo = fStore->objectNumberFor(inAddr);
		if (SDirEnt::isValidMigratedObjectInfo(objNo, inArg2))
		{
			if (zapMigDirEnt(inAddr) == noErr
			&&  inArg2 != BLOCK_NUMBER(fFirstObjAddr))
				addMigDirEnt(objNo, inArg2);
		}
	}
}


/* -----------------------------------------------------------------------------
	Return the address/offset of the next object in the block.
	Args:		inAddr			current object
				outAddr			on return, address/offset of next object
				inAllObjects	true => return every object encountered
									false => return only zapped objects -- don’t think this is ever used
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CFlashBlock::nextObject(ZAddr inAddr, ZAddr * outAddr, bool inAllObjects)
{
	NewtonErr err;
	StoreObjHeader obj;
	bool tryThis = false;
	ZAddr addr = inAddr;
	if ((addr & fStore->fBlockSizeMask) == 0)
	{
		addr = fFirstObjAddr + 4;
		tryThis = true;
	}

	// loop until we reach the end of the block
	while (addr < endOffset())
	{
		err = readObjectAt(addr, &obj);
		XCRITICAL(err)
		if (obj.isValid(fStore))
		{
			if (tryThis
			&& (inAllObjects || ISDIRTYBIT(obj.zapped)))
			{
				*outAddr = addr;
				return noErr;
			}
			if (ISVIRGINBIT(obj.x61))
			{
				addr += (sizeof(StoreObjHeader) + LONGALIGN(obj.size));
				tryThis = true;
			}
			else
				addr += 4;
		}
		else if (obj.id == (fStore->fVirginBits & 0x0FFFFFFF))
			// object is virgin -- no more objects
			break;
		else
			addr += 4;
	}
	return kStoreErrNoMoreObjects;
}


// can compact SRAM but not flash
NewtonErr
CFlashBlock::compactInto(ULong)
{}

NewtonErr
CFlashBlock::compactInto(CFlashBlock*)
{}

NewtonErr
CFlashBlock::compactInPlace(void)
{
	startCompact(fStore->fCompact);
	continueCompact(fStore->fCompact);
	fStore->mount();
	fStore->notifyCompact(this);
	return noErr;
}


NewtonErr
CFlashBlock::startCompact(SCompactState * ioState)
{
	NewtonErr err = noErr;
	bool isDone;
	for (isDone = false; !isDone; )
	{
		newton_try
		{
			ioState->x34.init(fStore->fStoreAddr, fStore->fNumOfBlocks * fStore->fBlockSize, fStore->fUseRAM ? (char *)InternalStoreInfo(3) : NULL, fStore->fUseRAM ? InternalStoreInfo(2) : 0);
			fStore->fStoreDriver = &ioState->x34;
			ioState->refCount = 0;
			ioState->x10 = fFirstObjAddr;
			ioState->x0C = fStore->translate(fFirstObjAddr);
			ioState->x20 = ioState->x18 = rootDirSize() + 12;
			ioState->x1C = 0;
			ioState->x24 = 0;
			ioState->refCount = 1;
			isDone = true;
		}
		newton_catch(exAbort)
		{
			fStore->sendAlertMgrWPBitch(0);
		}
		end_try;
	}
	return noErr;
}


NewtonErr
CFlashBlock::continueCompact(SCompactState * ioState)
{
	NewtonErr err = noErr;
	bool isDone;
	for (isDone = false; !isDone; )
	{
		newton_try
		{
			err = realContinueCompact(ioState);
			isDone = true;
		}
		newton_catch(exAbort)
		{
			fStore->sendAlertMgrWPBitch(0);
		}
		end_try;
	}
	return err;
}


NewtonErr
CFlashBlock::realContinueCompact(SCompactState * ioState)
{}


bool
CFlashBlock::isVirgin(void)
{
	return logEntryOffset() == 0;
}


bool
CFlashBlock::isReserved(void)
{
	CFlashPhysBlock * block = physBlock();
	return block ? block->isReserved() : false;
}


PSSId
CFlashBlock::nextPSSId(void)
{
	while (!IsValidPSSId(fAvailableId))
		++fAvailableId;
	return fAvailableId;
}


PSSId
CFlashBlock::useNextPSSId(void)
{
	return fAvailableId++;
}


NewtonErr
CFlashBlock::basicWrite(ZAddr inAddr, void * inBuf, size_t inLen)
{
	return fStore->basicWrite(fStore->translate(inAddr), inBuf, inLen);
}


ArrayIndex
CFlashBlock::eraseCount(void)
{
	CFlashPhysBlock * block = physBlock();
	return block ? block->eraseCount() : 0;
}


ULong
CFlashBlock::eraseHeuristic(ULong inArg)
{
	int r0 = fStore->averageEraseCount() - eraseCount();
	ULong r1 = inArg / KByte;
	return r0 * r0 * r0 + r1 * r1;
}


size_t
CFlashBlock::bucketSize(void)
{
	return fStore->bucketSize();
}


ArrayIndex
CFlashBlock::bucketCount(void)
{
	return fStore->bucketCount();
}


size_t
CFlashBlock::avail(void)
{
	if (isReserved())
		return 0;

	size_t availableBytes = fStore->offsetToLogs();
	if (!isVirgin())
		availableBytes -= (fNextObjAddr - fFirstObjAddr);
	return availableBytes;
}


size_t
CFlashBlock::yield(void)
{
	if (isReserved())
		return 0;

	return avail() + fZappedSize;
}


size_t
CFlashBlock::calcRecoverableBytes(void)
{
	if (isReserved())
		return 0;

	size_t numOfUnusedDirEnt = 0;
	size_t bucketLen = bucketSize() * sizeof(SDirEnt);
	ZAddr r7 = fDirBase + bucketLen;
	for (int i = bucketCount() - 1; i >= 0; i--, r7 += bucketLen)
	{
		SDirEnt dirEnt;
		ZAddr r6 = r7;
		for (int j = 1; j >= 0; j--, r6 += sizeof(SDirEnt))
		{
			XFAIL(readDirEntAt(r6, &dirEnt))
			if (dirEnt.isValid(fStore) && !ISDIRTYBIT(dirEnt.x6) && ISDIRTYBIT(dirEnt.x7))
			{
				CFlashIterator iter(fStore, &dirEnt, MAKE_ADDR(dirEnt.zaddr));
				numOfUnusedDirEnt += iter.countUnusedDirEnt();
				break;
			}
		}
	}
	return yield()
		  + numOfUnusedDirEnt * sizeof(SDirEnt)
		  + (numOfUnusedDirEnt/bucketSize()) * 8;
}


ZAddr
CFlashBlock::endOffset(void)
{
	return fFirstObjAddr + fStore->offsetToLogs();
}


ZAddr
CFlashBlock::logEntryOffset(void)
{
	CFlashPhysBlock * block = physBlock();
	return block ? block->logEntryOffset() : 0;
}


/* -----------------------------------------------------------------------------
	Return the physical block represented by this logical block.
	Args:		--
	Return:	logical block or NULL if no physical block set
----------------------------------------------------------------------------- */

CFlashPhysBlock *
CFlashBlock::physBlock(void)
{
	if (fPhysBlockAddr == kIllegalZAddr)
		return NULL;
	return &fStore->fPhysBlock[BLOCK_NUMBER(fPhysBlockAddr)];
}


#pragma mark -
#pragma mark SCompactState
/* -----------------------------------------------------------------------------
	S C o m p a c t S t a t e
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Initialise state.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
SCompactState::init(void)
{
	memset(this, 0, sizeof(SCompactState));
	refCount = 0;
	signature1 = 'bltg';
	signature2 = 'zarf';
}


/* -----------------------------------------------------------------------------
	Is this state valid?
	Check the signatures.
	Args:		--
	Return:	true => signature is good
----------------------------------------------------------------------------- */

bool
SCompactState::isValid(void)
{
	return signature1 == 'bltg' && signature2 == 'zarf';
}


/* -----------------------------------------------------------------------------
	Is compaction in progress?
	Args:		--
	Return:	true => compaction in progress
----------------------------------------------------------------------------- */

bool
SCompactState::inProgress(void)
{
	return isValid() && refCount != 0;
}
