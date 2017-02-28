/*
	File:		FlashIterator.cc

	Contains:	Flash iterator implementation.
	
	Written by:	Newton Research Group, 2010.
*/
#undef forDebug
#define debugLevel 2

#include "FlashIterator.h"
#include "FlashStore.h"


/*------------------------------------------------------------------------------
	C F l a s h I t e r a t o r
------------------------------------------------------------------------------*/

CFlashIterator::CFlashIterator(CFlashStore * inStore, CStoreObjRef * inObj, IterFilterType inFilter)
{
	fCurrentObj = inObj;
	fStore = inStore;
	start(inFilter);
}


CFlashIterator::CFlashIterator(CFlashStore * inStore, CStoreObjRef * inObj, ZAddr inRootDirEntAddr, IterFilterType inFilter)
{
	fCurrentObj = inObj;
	fStore = inStore;
	start(inRootDirEntAddr, inFilter);
}


CFlashIterator::CFlashIterator(CFlashStore * inStore, CStoreObjRef * inObj, CFlashBlock * inBlock, IterFilterType inFilter)
{
	fCurrentObj = inObj;
	fStore = inStore;
	start(inBlock->fFirstObjAddr, inFilter);
}


CFlashIterator::CFlashIterator(CFlashStore * inStore, SDirEnt * inDirEnt, ZAddr inRootDirEntAddr)
{
	fCurrentObj = NULL;
	fStore = inStore;
	fDirEnt = inDirEnt;
	fNumOfFreeDirEnts = inStore->bucketSize();
	start(inRootDirEntAddr, kIterFilterType4);
}


#pragma mark -
/*------------------------------------------------------------------------------
	Start iteration.
	Args:
	Return:
------------------------------------------------------------------------------*/

void
CFlashIterator::start(IterFilterType inFilter)
{
	fFilterType = inFilter;
	fState = 0;
	fObjectAddr = 4;
	fRootDirEntAddr = 0;
	f28 = kIllegalZAddr;
	fDirBucketAddr = kIllegalZAddr;
}


void
CFlashIterator::start(ZAddr inRootDirEntAddr, IterFilterType inFilter)
{
//printf("CFlashIterator::start(rootDirEnt=%d, filter=%d)\n", inRootDirEntAddr, inFilter);
	fRootDirEntAddr = inRootDirEntAddr;
	fFilterType = inFilter;
	fState = 0;
	fDirBucketAddr = kIllegalZAddr;
}


void
CFlashIterator::start(CFlashTracker * inTracker)
{
	fTracker = inTracker;
	fFilterType = kIterFilterType5;
	fState = 0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	Look up an object.
	Args:		inObjectId
				inState
				outArg3
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CFlashIterator::lookup(PSSId inObjectId, int inState, int * outArg3)
{
// r4: r7 r6 r5
PRINTF(("CFlashIterator::lookup(id=%d, state=%d)\n", inObjectId, inState));
ENTER_FUNC

	int sp00 = 0;
	ZAddr dirEntAddr = fRootDirEntAddr;	// r8
	ArrayIndex objNo;
	if (outArg3 != NULL)
	{
		*outArg3 = -1;
		objNo = fStore->objectNumberFor(inObjectId);
	}
	for ( ; ; )
	{
		dirEntAddr += sizeof(SDirEnt);
		SDirEnt dirEnt = getDirEnt(dirEntAddr);
		if (ISVIRGINDIRENT(&dirEnt))
		{
PRINTF((" -> kStoreErrObjectNotFound\n"));
EXIT_FUNC
			return kStoreErrObjectNotFound;
		}
		if (dirEnt.isValid(fStore))
		{
			if (ISDIRTYBIT(dirEnt.x6))
			{
				if (outArg3
				&&  *outArg3 < 0
				&&  ISDIRTYBIT(dirEnt.x2))
				{
					int sp00, sp04;
					dirEnt.getMigratedObjectInfo(&sp04, &sp00);
					if (objNo == sp04)
						*outArg3 = sp00;
				}
			}
			else
			{
				ZAddr addr = MAKE_ADDR(dirEnt.zaddr);
				if (addr > fStore->fNumOfBlocks * fStore->fBlockSize)
				{
					// DEBUG: print dirent list
					this->print();
EXIT_FUNC
					return kStoreErrNeedsFormat;
				}
				if (ISDIRTYBIT(dirEnt.x7))
					dirEntAddr = addr + sp00;
				else
				{
					fCurrentObj->set(addr, dirEntAddr);
					if (fCurrentObj->fObj.id == inObjectId)	// we have the object
					{
						if (inState == -1)			// any state will do
						{
//PRINTF(" -> id match\n");
EXIT_FUNC
							return noErr;
						}
						else if (inState == 0)			// many states allowed
						{
							switch (fCurrentObj->state())
							{
								case 3:
								case 10:
								case 4:
								case 11:
								case 6:
								case 13:
//PRINTF(" -> id and selected state match\n");
EXIT_FUNC
									return noErr;
							}
						}
						else if (inState == fCurrentObj->state())	//state must match
						{
//PRINTF(" -> id and state match\n");
EXIT_FUNC
							return noErr;
						}
					}
				}
			}
		}
	}
}


/*------------------------------------------------------------------------------
	Probe for an object, according to the set filter.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashIterator::probe(void)
{
	if (fState == 0)
	{
		switch (fFilterType)
		{
		case kIterFilterType0:
		case kIterFilterType1:
			if (f30 == fStore->fA0)
			{
				fObjectAddr = fRootDirEntAddr;
				f28 = kIllegalZAddr;
				f1C = 4;
			}
			else
			{
				fObjectAddr = (fObjectAddr & ~(fStore->fBlockSize - 1)) + 4;
				f28 = kIllegalZAddr;
				f1C = 4;
			}
			break;

		case kIterFilterType2:
		case kIterFilterType3:
			fObjectAddr = fRootDirEntAddr;
			f28 = kIllegalZAddr;
			f1C = 4;
			break;

		case kIterFilterType4:
			fObjectAddr = kIllegalZAddr;
			fDirBucketAddr = kIllegalZAddr;
			f28 = fRootDirEntAddr;
			f1C = 0;
			break;

		case kIterFilterType5:
			fIndex = -1;
			break;
		}
		fState = 2;
		f30 = fStore->fA0;
	}

	else if (fState == 1)
	{
		fState = 2;
		return;
	}

	switch (fFilterType)
	{
	case kIterFilterType0:
		f28 = kIllegalZAddr;
		for ( ; ; )
		{
			fObjectAddr += f1C;
			if ((fObjectAddr & fStore->fBlockSizeMask) >= fStore->offsetToLogs())
			{
				// out of block
				fObjectAddr = ALIGN(fObjectAddr, fStore->fBlockSize);
				if (fObjectAddr >= fStore->storeCapacity())
					break;
				if (fStore->blockForAddr(fObjectAddr)->isVirgin())
					break;
			}
			fCurrentObj->set(fObjectAddr, kIllegalZAddr);
			if (fCurrentObj->fObj.isValid(fCurrentObj->fStore))
			{
				f1C = ISVIRGINBIT(fCurrentObj->fObj.x61) ? sizeof(StoreObjHeader) + LONGALIGN(fCurrentObj->fObj.size) : 4;
				if (!ISDIRTYBIT(fCurrentObj->fObj.zapped))
				{
					if (fCurrentObj->fObj.id >= 33)
						return;
				}
			}
			else
			{
				if (ISVIRGINOBJECT(fCurrentObj))
				{
					fObjectAddr = ALIGN(fObjectAddr, fStore->fBlockSize);
					if (fObjectAddr >= fStore->storeCapacity())
						break;
					if (fStore->blockForAddr(fObjectAddr)->isVirgin())
						break;
				}
				f1C = 4;
			}
		}
		fState = 3;
		break;

	case kIterFilterType1:
		f28 = kIllegalZAddr;
		for ( ; ; )
		{
			fObjectAddr += f1C;
			if ((fObjectAddr & fStore->fBlockSizeMask) >= fStore->offsetToLogs())
			{
				// out of block
				fObjectAddr = ALIGN(fObjectAddr, fStore->fBlockSize);
				if (fObjectAddr >= fStore->storeCapacity())
					break;
				if (fStore->blockForAddr(fObjectAddr)->isVirgin())
					break;
			}
			fCurrentObj->set(fObjectAddr, kIllegalZAddr);
			if (fCurrentObj->fObj.isValid(fCurrentObj->fStore))
			{
				f1C = ISVIRGINBIT(fCurrentObj->fObj.x61) ? sizeof(StoreObjHeader) + LONGALIGN(fCurrentObj->fObj.size) : 4;
				if (!ISDIRTYBIT(fCurrentObj->fObj.zapped))
				{
					return;
				}
			}
			else
			{
				if (ISVIRGINOBJECT(fCurrentObj))
				{
					fObjectAddr = ALIGN(fObjectAddr, fStore->fBlockSize);
					if (fObjectAddr >= fStore->storeCapacity())
						break;
					if (fStore->blockForAddr(fObjectAddr)->isVirgin())
						break;
				}
				f1C = 4;
			}
		}
		fState = 3;
		break;

	case kIterFilterType2:
		f28 = kIllegalZAddr;
		for ( ; ; )
		{
			fObjectAddr += f1C;
			if ((fObjectAddr & fStore->fBlockSizeMask) >= fStore->offsetToLogs())
				break;	// out of block
			fCurrentObj->set(fObjectAddr, kIllegalZAddr);
			if (fCurrentObj->fObj.isValid(fCurrentObj->fStore))
			{
				f1C = ISVIRGINBIT(fCurrentObj->fObj.x61) ? sizeof(StoreObjHeader) + LONGALIGN(fCurrentObj->fObj.size) : 4;
				if (ISDIRTYBIT(fCurrentObj->fObj.zapped))
				{
					if (fCurrentObj->fObj.id >= 33)
						return;
				}
			}
			else if (*(ULong *)fCurrentObj != fCurrentObj->fStore->fVirginBits)
				f1C = 4;
			else
				break;
		}
		fState = 3;
		break;

	case kIterFilterType3:
		f28 = kIllegalZAddr;
		for ( ; ; )
		{
			fObjectAddr += f1C;
			if ((fObjectAddr & fStore->fBlockSizeMask) >= fStore->offsetToLogs())
				break;	// out of block
			fCurrentObj->set(fObjectAddr, kIllegalZAddr);
			if (fCurrentObj->fObj.isValid(fCurrentObj->fStore))
			{
				f1C = ISVIRGINBIT(fCurrentObj->fObj.x61) ? sizeof(StoreObjHeader) + LONGALIGN(fCurrentObj->fObj.size) : 4;
				if (!ISDIRTYBIT(fCurrentObj->fObj.zapped))
					return;
			}
			else if (!ISVIRGINOBJECT(fCurrentObj))
				f1C = 4;
			else
				break;
		}
		fState = 3;
		break;

	case kIterFilterType4:	// iterate over directory entries
		for ( ; ; )
		{
			f28 += 4;
			*fDirEnt = getDirEnt(f28);
			if (ISVIRGINDIRENT(fDirEnt))
			{
				fState = 3;
				return;
			}
			if (fDirEnt->isValid(fStore))
			{
				if (!ISDIRTYBIT(fDirEnt->x6)
				|| ISDIRTYBIT(fDirEnt->x2))
				{
					if (ISDIRTYBIT(fDirEnt->x7))
					{
						f28 = MAKE_ADDR(fDirEnt->zaddr);
						fNumOfFreeDirEnts += fStore->fBucketSize;
					}
					else
					{
						fNumOfFreeDirEnts--;
						break;
					}
				}
			}
		}
		break;

	case kIterFilterType5:
		for ( ; ; )
		{
			fIndex++;
			if (fIndex >= fTracker->fIndex)
			{
				fState = 3;
				return;
			}
			PSSId objId = fTracker->fList[fIndex];
			if (objId != kNoPSSId
			&&  fStore->lookup(objId, -1, *fCurrentObj) == noErr)
				break;
		}
		break;
	}
}


/*------------------------------------------------------------------------------
	Return the next object in the iterator.
	Args:		--
	Return:	store object ref
------------------------------------------------------------------------------*/

CStoreObjRef *
CFlashIterator::next(void)
{
	probe();
	return fCurrentObj;
}


/*------------------------------------------------------------------------------
	Is the iterator done?
	Args:		--
	Return:	true => no more objects in the iterator
------------------------------------------------------------------------------*/

bool
CFlashIterator::done(void)
{
	bool isDone = false;
	switch (fState)
	{
	case 0:
		f30 = fStore->fA0;
		probe();
		if (fState == 3)
			isDone = true;
		else
			fState = 1;
		break;

	case 2:
		if (f30 != fStore->fA0)
			fState = 0;
		probe();
		if (fState == 3)
			isDone = true;
		else
			fState = 1;
		break;

	case 3:
		isDone = true;
		break;
	}
	return isDone;
}


/*------------------------------------------------------------------------------
	Reset the iterator.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashIterator::reset(void)
{
	fState = 0;
}


#pragma mark -
/*------------------------------------------------------------------------------
	Read a directory entry.
	Args:		inAddr
	Return:	the directory entry (which will be cached in the bucket)
------------------------------------------------------------------------------*/

SDirEnt
CFlashIterator::getDirEnt(ZAddr inAddr)
{
//PRINTF(" getDirEnt(%d)\n", inAddr);
	if (fDirBucketAddr == kIllegalZAddr
	||  inAddr < fDirBucketAddr
	||  inAddr >= fDirBucketAddr + sizeof(fDirBucket))
		// id is not within current bucket
		return readDirBucket(inAddr);
	ArrayIndex index = (inAddr - fDirBucketAddr)/sizeof(SDirEnt);
//PRINTF("  -> #%08X\n", *(ULong *)(fDirBucket + index));
	return fDirBucket[index];
}


/*------------------------------------------------------------------------------
	Read a directory bucket.
	Args:		inAddr
	Return:	the first directory entry in the bucket
------------------------------------------------------------------------------*/

SDirEnt
CFlashIterator::readDirBucket(ZAddr inAddr)
{
	fStore->basicRead(fStore->translate(inAddr), &fDirBucket, sizeof(fDirBucket));
	fDirBucketAddr = inAddr;
//PRINTF("  readDirBucket(%d) -> #%08X\n", inAddr, fDirBucket[0]);
	return fDirBucket[0];
}


#pragma mark -
/*------------------------------------------------------------------------------
	Count unused DirEnts.
	Args:		--
	Return:	the number of unused DirEnts
------------------------------------------------------------------------------*/

ArrayIndex
CFlashIterator::countUnusedDirEnt(void)
{
	while (!done())
		next();
	return fNumOfFreeDirEnts;
}


/*------------------------------------------------------------------------------
	DEBUG
	Print directory entries.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashIterator::print(void)
{
	PRINTF(("--directory entries--\n"));
	ZAddr dirEntAddr = fRootDirEntAddr;
	for ( ; ; )
	{
		dirEntAddr += sizeof(SDirEnt);
		SDirEnt dirEnt = getDirEnt(dirEntAddr);
		ZAddr addr = MAKE_ADDR(dirEnt.zaddr);
		PRINTF(("%08X\n", addr));
		if (ISVIRGINDIRENT(&dirEnt))	// end of directory
		{
			PRINTF(("--end of directory--\n"));
			break;
		}
		if (addr > fStore->fNumOfBlocks * fStore->fBlockSize)
		{
			PRINTF(("address exceeds store size!\n"));
			break;
		}
	}
}

