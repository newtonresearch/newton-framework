/*
	File:		Heap.cp

	Contains:	Heap operations.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "MemMgr.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

Heap		gSkiaHeapBase;		// 0C10107C
Heap		gKernelHeap;		// 0C101080


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

static Heap			NewHeap(VAddr inAddr, Size inSize, Size inPageSize);

static NewtonErr	DestroyVMHeapHelper(Heap inHeap);
static bool			HeapReleaseRequestHandler(Heap inHeap, VAddr * outStart, VAddr * outEnd, bool inDoRelease);

static void			ResurrectSkiaHeap(Heap inHeap);
static void			SetSkiaHeapRefcon(Heap inHeap, void * inRefCon);
static void *		GetSkiaHeapRefcon(Heap inHeap);
static void							SetSkiaHeapSemaphore(Heap inHeap, CULockingSemaphore * inSemaphore);
static CULockingSemaphore *	GetSkiaHeapSemaphore(Heap inHeap);


/*------------------------------------------------------------------------------
	R e f C o n
------------------------------------------------------------------------------*/

void
SetHeapRefCon(void * inRefCon, Heap inHeap)
{ SetSkiaHeapRefcon(inHeap, inRefCon); }

void *
GetHeapRefCon(Heap inHeap)
{ return GetSkiaHeapRefcon(inHeap); }


/*------------------------------------------------------------------------------
	Get the task’s current heap.
	Args:		--
	Return:	the task’s current heap
------------------------------------------------------------------------------*/

Heap
GetHeap(void)
{
	return GetCurrentHeap();
}


/*------------------------------------------------------------------------------
	Set the task’s current heap.
	Args:		inHeap		a heap
	Return:	--
------------------------------------------------------------------------------*/

void
SetHeap(Heap inHeap)
{
	SetCurrentHeap(inHeap);
}


/*------------------------------------------------------------------------------
	Get the task’s current heap.
	Args:		--
	Return:	the task’s current heap
------------------------------------------------------------------------------*/

Heap
GetCurrentHeap(void)
{
	if (gOSIsRunning || gCurrentTaskId != kNoId)
		return TaskSwitchedGlobals()->fCurrentHeap	? TaskSwitchedGlobals()->fCurrentHeap
																	: gKernelHeap;
	else
	 	return gKernelHeap;
}


/*------------------------------------------------------------------------------
	Set the task’s current heap.
	Args:		inHeap		a heap
	Return:	--
------------------------------------------------------------------------------*/

void
SetCurrentHeap(Heap inHeap)
{
	if (gOSIsRunning || gCurrentTaskId != kNoId)
		TaskSwitchedGlobals()->fCurrentHeap = inHeap;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Create a new heap at a known VM address.
	Args:		inAddr		where heap is in VM
				inSize		size of heap
				outHeap		new heap
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
NewHeapAt(VAddr inAddr, Size inSize, Heap * outHeap)
{
	NewtonErr err = noErr;
	XTRY
	{
		Heap	newHeap;
		XFAILNOT(newHeap = NewHeap(inAddr, inSize, kSubPageSize), err = memFullErr;)
		*outHeap = newHeap;
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Create a new heap.
	Args:		inAddr		where heap is in VM
				inSize		size of heap
				inPageSize	
	Return:	new heap
------------------------------------------------------------------------------*/

static Heap
NewHeap(VAddr inAddr, Size inSize, Size inPageSize)
{
	Heap savedHeap = GetCurrentHeap();

	// zero the specified memory and call it the current heap
	Block *	block = (Block *)inAddr;
	SHeap *	newHeap = (SHeap *)((Block *)(block + 1));
	memset((Ptr) inAddr, 0, inPageSize);
	SetCurrentHeap(newHeap);

	// create the heap definition block
	CreatePrivateBlock(block, 3);	// kMM_HeapHeaderBlock?
	block->inuse.size = sizeof(Block) + sizeof(SHeap);

	// set up heap
	newHeap->start = inAddr;
	newHeap->tag = 'skia';
	newHeap->size = inSize;
	newHeap->pageSize = inPageSize;
	newHeap->extent = newHeap->prevMaxExtent = newHeap->pageSize;
	newHeap->maxExtent = newHeap->extent;
	newHeap->isVMBacked = false;
	newHeap->refCon = 0;

	newHeap->end = inAddr + newHeap->extent;

	newHeap->fixedHeap = newHeap;
	newHeap->masterPtrHeap = newHeap;
	newHeap->x9C = newHeap;
	newHeap->relocHeap = newHeap;
	newHeap->wiredHeap = NULL;

	// the entire heap is one huge free block
	Block *	freeBlock = (Block *)((Ptr) newHeap + sizeof(SHeap));
	newHeap->freeList = freeBlock;
	newHeap->freeListTail = freeBlock;
	newHeap->x48 = freeBlock;

	Size	freeBlockSize = newHeap->extent - (sizeof(Block) + sizeof(SHeap)) - sizeof(Block);
	newHeap->free = freeBlockSize;
	freeBlock->free.size = freeBlockSize;
	freeBlock->free.next = NULL;
	freeBlock->free.prev = NULL;

	// create the end-of-heap block
	block = (Block *)((Ptr) freeBlock + freeBlockSize);
	CreatePrivateBlock(block, 4);	// kMM_HeapEndBlock?
	block->inuse.flags |= 0x04;
	block->inuse.size = sizeof(Block);
	block->inuse.link = newHeap;

	newHeap->mpBlockSize = 64;
	newHeap->x80 = 0;
	newHeap->x64 = 0;
	newHeap->freeMPList = NULL;
	newHeap->freeMPCount = 0;	// NOT IN ORIGINAL!

	SetCurrentHeap(savedHeap);
	return newHeap;
}


/*------------------------------------------------------------------------------
	V M   H e a p   O p e r a t i o n s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create a new heap.
	Args:		inDomain			domain for heap (or zero for current env’s default)
				inMaxSize		max size of both Ptr & Handle allocation
				outHeap			new heap
				inOptions		various options
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
NewVMHeap(ObjectId inDomain, Size inMaxSize, Heap * outHeap, ULong inOptions)
{
	NewtonErr	err;
	bool			isRecoverable;
	VAddr			heapStart, heapEnd;

	XTRY
	{
		if (inDomain == 0)
			inDomain = TaskSwitchedGlobals()->fDefaultHeapDomainId;
		isRecoverable = (inOptions & kPersistentHeap) != 0;
		if (isRecoverable)
			inMaxSize += kPageSize;
		XFAIL(err = NewHeapArea(inDomain, 0, inMaxSize, (inOptions & ~kPersistentHeap) | kNHOpts_ExceptionOnNoMem, &heapStart, &heapEnd))

		Size	pageSize = kSubPageSize;
		if (isRecoverable)
		{
			heapStart = ALIGN(heapStart, kPageSize);
			pageSize = kPageSize;
		}
		Heap saveHeap = GetHeap();
		LockHeapRange(heapStart, heapStart + pageSize);
		UnlockHeapRange(heapStart, heapStart + pageSize);
		Heap newHeap = NewHeap(heapStart, heapEnd - heapStart, pageSize);
		SetHeap(saveHeap);

		XFAILNOT(newHeap, err = memFullErr;)
		AddSemaphoreToHeap(newHeap);
		SetHeapIsVMBacked(newHeap);
		SetRemoveRoutine((VAddr)newHeap, HeapReleaseRequestHandler, newHeap);
		*outHeap = newHeap;
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Create a new heap, specifying sizes for Ptrs and Handles.
	Args:		inDomain			domain for heap (or zero for current env’s default)
				inPtrSize		max size of Ptr allocation
				inHandleSize	max size of Handle allocation
				outHeap			new heap
				inOptions		various options
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
NewSegregatedVMHeap(ObjectId inDomain, Size inPtrSize, Size inHandleSize, Heap * outHeap, ULong inOptions)
{
	NewtonErr	err;
	Heap			newHeap = NULL;
	Heap			mpHeap = NULL;
	Heap			handleHeap = NULL;

	XTRY
	{
		// set up pointer heap
		XFAIL(err = NewVMHeap(inDomain, inPtrSize, &newHeap, inOptions))
#if defined(correct)
		if (inHandleSize > 0)
		{
			// set up master pointer block
			XFAIL(err = NewVMHeap(inDomain, inPtrSize/4, &mpHeap, inOptions))
			ClobberHeapSemaphore(mpHeap);
			SetSkiaHeapSemaphore(mpHeap, GetSkiaHeapSemaphore(newHeap));
			SetFixedHeap(mpHeap, newHeap);
			SetMPHeap(newHeap, mpHeap);
			SetMPHeap(mpHeap, mpHeap);
			// set up handle heap
			XFAIL(err = NewVMHeap(inDomain, inHandleSize, &handleHeap, inOptions))
			ClobberHeapSemaphore(handleHeap);
			SetSkiaHeapSemaphore(handleHeap, GetSkiaHeapSemaphore(newHeap));
			SetFixedHeap(handleHeap, newHeap);
			SetMPHeap(handleHeap, mpHeap);
			SetRelocHeap(newHeap, handleHeap);
			SetRelocHeap(handleHeap, handleHeap);
		}
#endif
		*outHeap = newHeap;
	}
	XENDTRY;
	XDOFAIL(err)
	{
		if (newHeap != NULL)
			DestroyVMHeap(newHeap);
	}
	XENDFAIL;
	return err;
}


/*------------------------------------------------------------------------------
	Create a new persistent heap (ie one that lives beyond reboot).
	Args:		inDomain			domain for heap (or zero for current env’s default)
				inMaxSize		max size of both Ptr & Handle allocation
				outHeap			new heap
				inOptions		various options
				inName			its name
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
NewPersistentVMHeap(ObjectId inDomain, Size inMaxSize, Heap * outHeap, ULong inOptions, ULong inName)
{
	NewtonErr		err;
	CDomainDBEntry	domainEntry;

	if (inDomain == kNoId)
		inDomain = TaskSwitchedGlobals()->fDefaultHeapDomainId;
	
	for (ArrayIndex index = 0; MemObjManager::findEntryByIndex(kMemObjDomain, index, &domainEntry, &err); index++)
	{
		if (domainEntry.fId == inDomain)
		{
			if ((err = NewVMHeap(inDomain, inMaxSize, outHeap, inOptions | kPersistentHeap)) == noErr)
			{
				CPersistentDBEntry	entry;
				VAddr						heapStart, heapEnd;
				if ((err = GetHeapAreaInfo((VAddr)*outHeap, &heapStart, &heapEnd)) == noErr)
				{
					entry.init(inName, true, index);
					entry.fHeap = *outHeap;
					entry.fBase = heapStart;
					entry.fSize = heapEnd - heapStart;
					entry.fFlags &= ~0x80;
					if ((err = MemObjManager::registerPersistentNewEntry(inName, &entry)) != noErr)
						DestroyVMHeap(entry.fHeap);
				}
			}
			return err;
		}
	}
	// didn’t find the domain
	return kStackErrBadDomain;
}


/*------------------------------------------------------------------------------
	Delete a persistent heap.
	Args:		inName			its name
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
DeletePersistentVMHeap(ULong inName)
{
	NewtonErr err = noErr;
	XTRY
	{
		CPersistentDBEntry entry;
		XFAIL(err = MemObjManager::findEntryByName(kMemObjPersistent, inName, &entry))
		XFAIL(err = MemObjManager::deregisterPersistentEntry(inName))
		DestroyVMHeap(entry.fHeap);
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Bring a heap at some VAddr back to life after a reboot.
	Args:		inHeap			the heap
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
ResurrectVMHeap(Heap inHeap)
{
	NewtonErr err = noErr;
	if (IsSkiaHeap(inHeap))
	{
		ResurrectSkiaHeap(inHeap);
		if ((err = AddSemaphoreToHeap(inHeap)) == noErr)
			err = SetHeapLimits(GetHeapStart(inHeap), GetHeapStart(inHeap) + GetHeapExtent(inHeap));
		SetRemoveRoutine((VAddr)inHeap, HeapReleaseRequestHandler, inHeap);
	}
	return err;
}


/*------------------------------------------------------------------------------
	Clobber a heap.
	Args:		inHeap			the heap
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
DestroyVMHeap(Heap inHeap)
{
	if (inHeap != NULL)
	{
		SWiredHeapDescr *	wiredHeap;
		ClobberHeapSemaphore(inHeap);
		if ((wiredHeap = GetWiredHeap(inHeap)) != NULL)
		{
			wiredHeap->fHeap->destroy();
			SetWiredHeap(inHeap, NULL);
		}
		if (GetRelocHeap(inHeap) != NULL)
			DestroyVMHeapHelper(GetRelocHeap(inHeap));
		if (GetMPHeap(inHeap) != NULL)
			DestroyVMHeapHelper(GetMPHeap(inHeap));
		if (GetSPHeap(inHeap) != NULL)
			DestroyVMHeapHelper(GetSPHeap(inHeap));
		DestroyVMHeapHelper(inHeap);
	}
	return noErr;
}

static NewtonErr
DestroyVMHeapHelper(Heap inHeap)
{
	if (inHeap != NULL)
		FreePagedMem(GetHeapEnd(inHeap));
	return noErr;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Attach a semaphore a heap.
	Args:		inHeap			the heap
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
AddSemaphoreToHeap(Heap inHeap)
{
	NewtonErr err = noErr;
	XTRY
	{
		CULockingSemaphore *	sem = new CULockingSemaphore;
		XFAILIF(sem == NULL, err = MemError();)
		XFAILIF(err = sem->init(), delete sem;)
		if (IsSafeHeap(inHeap))
			SetSafeHeapSemaphore(inHeap, sem);
		else
			SetSkiaHeapSemaphore(inHeap, sem);
	}
	XENDTRY;
	return err;
}


/*------------------------------------------------------------------------------
	Return a heap’s semaphore.
	Args:		inHeap			the heap
	Return:	its semaphore
------------------------------------------------------------------------------*/

CULockingSemaphore *
GetHeapSemaphore(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetHeap();
	if (IsSafeHeap(inHeap))
		return GetSafeHeapSemaphore(inHeap);
	return GetSkiaHeapSemaphore(inHeap);
}


/*------------------------------------------------------------------------------
	Clobber a heap’s semaphore. Careful! Does not work for safe heaps.
	Args:		inHeap			the heap
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
ClobberHeapSemaphore(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetHeap();

	CULockingSemaphore *	sem;
	if ((sem = GetSkiaHeapSemaphore(inHeap)) != NULL)
	{
		SetSkiaHeapSemaphore(inHeap, NULL);
		delete sem;
	}
	return noErr;
}


/*------------------------------------------------------------------------------
	Clobber a heap.
	Args:		inHeap			the heap
				inVerification	sanity check
				inRecoverable	
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
ZapHeap(Heap inHeap, ULong inVerification, bool inRecoverable)
{
	NewtonErr	err;
	VAddr			heapStart, heapEnd;

	XTRY
	{
		XFAILIF(inVerification != kZapHeapVerification  ||  inHeap == NULL, err = kOSErrBadParameters;)

		err = GetHeapAreaInfo((VAddr)inHeap, &heapStart, &heapEnd);
		if (inRecoverable)
			heapStart = PAGEALIGN(heapStart);
		XFAIL(err)

		Size	pageSize = inRecoverable ? kPageSize : kSubPageSize;
		err = SetHeapLimits(heapStart, heapStart + pageSize);
		LockHeapRange(heapStart, heapStart + pageSize);
		UnlockHeapRange(heapStart, heapStart + pageSize);

		Heap	saveHeap = GetHeap();
		Heap	newHeap = NewHeap(heapStart, heapEnd - heapStart, pageSize);
		SetHeap(saveHeap);
		XFAILNOT(newHeap, err = memFullErr;)
		SetHeapIsVMBacked(newHeap);
		SetRemoveRoutine((VAddr)newHeap, HeapReleaseRequestHandler, newHeap);
	}
	XENDTRY;
	return err;
}


static bool
HeapReleaseRequestHandler(Heap inHeap, VAddr * outStart, VAddr * outEnd, bool inDoRelease)
{
	bool	result = false;
	if (IsSkiaHeap(inHeap))
	{
		if (HeapPtr(inHeap)->prevMaxExtent > HeapPtr(inHeap)->maxExtent)
		{
			if (inDoRelease)
				HeapPtr(inHeap)->prevMaxExtent = HeapPtr(inHeap)->maxExtent;
			*outEnd = HeapPtr(inHeap)->start + HeapPtr(inHeap)->maxExtent;
			result = true;
		}
		else
			*outEnd = HeapPtr(inHeap)->start + HeapPtr(inHeap)->prevMaxExtent;
		*outStart = HeapPtr(inHeap)->start;
	}
	return result;
}


#pragma mark -

/*------------------------------------------------------------------------------
	H e a p   S i z e   C a l c u l a t i o n s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Return the total amount of free space in a heap.
	Args:		inHeap			the heap
	Return:	the amount
------------------------------------------------------------------------------*/

Size
TotalFreeInHeap(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	return HeapPtr(inHeap)->free;
}


/*------------------------------------------------------------------------------
	Return the largest contiguous amount of free space in a heap.
	Args:		inHeap			the heap
	Return:	the amount
------------------------------------------------------------------------------*/

Size
LargestFreeInHeap(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	Size		size = 0;
	Block *	block;
	for (block = HeapPtr(inHeap)->freeList; block != NULL; block = block->free.next)
	{
		if (size < block->free.size)
			size = block->free.size;
	}
	if (size != 0)
		size -= sizeof(Block);
	return size;
}


/*------------------------------------------------------------------------------
	Return the number of free blocks in a heap.
	Args:		inHeap			the heap
	Return:	the amount
------------------------------------------------------------------------------*/

Size
CountFreeBlocks(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	Size		count = 0;
	Block *	block;
	for (block = HeapPtr(inHeap)->freeList; block != NULL; block = block->free.next)
		count++;
	return count;
}


/*------------------------------------------------------------------------------
	Return the total amount of used space in a heap.
	Args:		inHeap			the heap
	Return:	the amount
------------------------------------------------------------------------------*/

Size
TotalUsedInHeap(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	return HeapPtr(inHeap)->extent - HeapPtr(inHeap)->free;
}


/*------------------------------------------------------------------------------
	Return the maximum allowed size of a heap.
	Args:		inHeap			the heap
	Return:	the amount
------------------------------------------------------------------------------*/

Size
MaxHeapSize(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	return HeapPtr(inHeap)->size;
}


/*------------------------------------------------------------------------------
	Return the amount of space in a heap that could be released.
	Args:		inHeap			the heap
	Return:	the amount
------------------------------------------------------------------------------*/

Size
GetHeapReleaseable(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	return HeapPtr(inHeap)->prevMaxExtent - HeapPtr(inHeap)->extent;
}

#pragma mark -

/*------------------------------------------------------------------------------
	S k i a   H e a p   O p e r a t i o n s
------------------------------------------------------------------------------*/

static void
ResurrectSkiaHeap(Heap inHeap)
{
	HeapPtr(inHeap)->prevMaxExtent =
	HeapPtr(inHeap)->maxExtent = HeapPtr(inHeap)->extent;
}


static void
SetSkiaHeapRefcon(Heap inHeap, void * inRefCon)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	HeapPtr(inHeap)->refCon = inRefCon;
}


static void *
GetSkiaHeapRefcon(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	return HeapPtr(inHeap)->refCon;
}


static void
SetSkiaHeapSemaphore(Heap inHeap, CULockingSemaphore * inSemaphore)
{
	HeapPtr(inHeap)->x64 = inSemaphore;
}


static CULockingSemaphore *
GetSkiaHeapSemaphore(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	return HeapPtr(inHeap)->x64;
}

#pragma mark -

/*------------------------------------------------------------------------------
	H e a p   V a l i d a t i o n
------------------------------------------------------------------------------*/

typedef NewtonErr (*VetProcPtr)(Size * outSize, Block * inBlock, int inType, void ** outWhereSmashed);

static NewtonErr	VetHeap(Heap inHeap, void ** outWhereSmashed);
static NewtonErr	BasicVetHeap(SHeap * inHeap, void ** outWhereSmashed);
static NewtonErr	CheckPointer(SHeap * inHeap, Ptr inPtr, bool inIgnoreNULL);
static NewtonErr	VetFreeList(SHeap * inHeap, void ** outWhereSmashed);
static NewtonErr	VetHandles(SHeap * inHeap, void ** outWhereSmashed);
static NewtonErr	WalkEachBlock(SHeap * inHeap, void ** outWhereSmashed, VetProcPtr, Size *);
static NewtonErr	VetBlock(SHeap * inHeap, Block * inBlock, void ** outWhereSmashed);
static NewtonErr	VetDirBlock(SHeap * inHeap, Block * inBlock, void ** outWhereSmashed);
static NewtonErr	VetDynBlock(SHeap * inHeap, Block * inBlock, void ** outWhereSmashed);


/*------------------------------------------------------------------------------
	Check the heap.
	Args:		inHeap				the heap
				outWhereSmashed	address of heap inconsistency
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CheckHeap(Heap inHeap, void ** outWhereSmashed)
{
	NewtonErr err;

	newton_try
	{
		err = VetHeap(inHeap != NULL ? inHeap : GetCurrentHeap(), outWhereSmashed);
	}
	newton_catch(exAbort)
	{
		err = kMemErrExceptionGrokkingHeap;
	}
	end_try;

	return err;
}


static NewtonErr
VetHeap(Heap inHeap, void ** outWhereSmashed)
{
	NewtonErr err;
	XTRY
	{
		SHeap * heap = (SHeap *)inHeap;
		XFAIL(err = BasicVetHeap(heap, outWhereSmashed))
		XFAIL(err = WalkEachBlock(heap, outWhereSmashed, 0, NULL))
		XFAIL(err = VetFreeList(heap, outWhereSmashed))
		err = VetHandles(heap, outWhereSmashed);
	}
	XENDTRY;
	return err;
}


static NewtonErr
BasicVetHeap(SHeap * inHeap, void ** outWhereSmashed)
{
	NewtonErr	err;
	Ptr			p;

	XTRY
	{
		XFAILIF(inHeap->tag != 'skia', err = kMemErrBadHeapHeader;)

		if ((p = inHeap->xA8) != NULL)
			XFAIL(err = CheckPointer(inHeap, p, true))

		if ((p = (Ptr) inHeap->xAC) != NULL)
			XFAIL(err = CheckPointer(inHeap, p, true))

		if ((p = (Ptr) inHeap->freeList) != NULL)
			XFAIL(err = CheckPointer(inHeap, p, true))

		if ((p = (Ptr) inHeap->freeListTail) != NULL)
			XFAIL(err = CheckPointer(inHeap, p, true))

		if ((p = (Ptr) inHeap->x48) != NULL)
			XFAIL(err = CheckPointer(inHeap, p, true))

		XFAILIF(inHeap->free > (inHeap->end - inHeap->start), err = kMemErrBogusFreeCount;)
	}
	XENDTRY;
	return err;
}


static NewtonErr
CheckPointer(SHeap * inHeap, Ptr inPtr, bool inIgnoreNULL)
{
	NewtonErr err = noErr;

	if (inPtr != NULL || !inIgnoreNULL)
	{
		if (inPtr < (Ptr) inHeap || inPtr > (Ptr) inHeap->end)	// sic
			err = kMemErrPointerOutOfHeap;

		if (MISALIGNED(inPtr))
			err = kMemErrUnalignedPointer;
	}

	return err;
}


NewtonErr
vet(Size * outSize, Block * inBlock, int inType, void ** outWhereSmashed)
{
	if (inType == 7)
		*outSize += inBlock->free.size;
	return noErr;
}


static NewtonErr
VetFreeList(SHeap * inHeap, void ** outWhereSmashed)
{
	Block *	block;
	Size	freeSize = 0;

	WalkEachBlock(inHeap, NULL, vet, &freeSize);

	freeSize = 0;
	for (block = inHeap->freeList; block != NULL; block = block->free.next)
	{
		freeSize += block->free.size;
	}
	if (freeSize != inHeap->free)
		return kMemErrFreeSpaceDisagreement2;

	return noErr;
}


static NewtonErr
VetHandles(SHeap * inHeap, void ** outWhereSmashed)
{
	// this really does no checking
	return noErr;
}


static NewtonErr
WalkEachBlock(SHeap * inHeap, void ** outWhereSmashed, VetProcPtr inVet, Size * outSize)
{
	NewtonErr	err;
	bool			wasPrevBlockFree = true;
	Ptr			p;
	Block *		block, * nextBlock;

//printf("\n&%08X walking the heap\n", inHeap);
	for (block = (Block *)inHeap->start; block < (Block *)inHeap->end; block = nextBlock)
	{
		if (outWhereSmashed != NULL)
			*outWhereSmashed = block;
		UByte	flags = block->flags;
		if ((flags & 0x80) == 0)
		{
//printf("&%08X free block, size &%X (%d)\n", block, block->free.size - sizeof(Block), block->free.size - sizeof(Block));
			if (wasPrevBlockFree)
				return kMemErrMisplacedFreeBlock;	// free blocks should be coalesced
			if (flags != 0 || MISALIGNED(block->free.size))
				return kMemErrBogusBlockSize;
			if (block->free.size < sizeof(Block))
				return kMemErrBlockSizeLessThanMinimum;
			if (block->free.size >= 256*MByte)
				return kMemErrPreposterousBlockSize;

			if ((p = (Ptr) block->free.next) != NULL
			&&  CheckPointer(inHeap, p, true) != noErr)
				return kMemErrBadFreelistPointer;

			if ((p = (Ptr) block->free.prev) != NULL
			&&  CheckPointer(inHeap, p, true) != noErr)
				return kMemErrBadFreelistPointer;

			if (inVet != NULL
			&&  (err = inVet(outSize, block, 7, outWhereSmashed)) != noErr)
				return err;

			nextBlock = (Block *)((VAddr) block + block->free.size);
			wasPrevBlockFree = true;			
		}
		else
		{
			UByte	blockType = flags & 0x03;
			if (blockType == 0x01)	// is direct
			{
				if ((flags & 0x10) != 0 && block->inuse.busy == 0xFF)	// is internal
				{
//printf("&%08X internal block, type %d\n", block, block->inuse.type);
					switch (block->inuse.type)
					{
					case 1:
					case 2:
					case 3:
					case 4:
						// do nothing
						break;
					case 5:
						if ((err = VetDirBlock(inHeap, block, outWhereSmashed)) != noErr)
							return err;
						break;
					default:
						return kMemErrBadInternalBlockType;
						break;
					}
				}
				else
				{
//printf("&%08X direct block, size &%X (%d)\n", block, block->inuse.size - sizeof(Block), block->inuse.size - sizeof(Block));
				if ((err = VetDirBlock(inHeap, block, outWhereSmashed)) != noErr)
					return err;
				}

				if (inVet != NULL
				&&  (err = inVet(outSize, block, 3, outWhereSmashed)) != noErr)
					return err;
			}
			else if (blockType == 0x02)	// is indirect
			{
//printf("&%08X indirect block, size &%X (%d)\n", block, block->inuse.size - sizeof(Block), block->inuse.size - sizeof(Block));
				if ((err = VetDynBlock(inHeap, block, outWhereSmashed)) != noErr)
					return err;

				if (inVet != NULL
				&&  (err = inVet(outSize, block, 4, outWhereSmashed)) != noErr)
					return err;
			}
			else
				return kMemErrBogusBlockType;

			nextBlock = (Block *)((VAddr) block + block->inuse.size);
			wasPrevBlockFree = false;
		}

		if (CheckPointer(inHeap, (Ptr) nextBlock, false) != noErr)
			return kMemErrBadForwardMarch;
	}

	return noErr;
}


static NewtonErr
VetBlock(SHeap * inHeap, Block * inBlock, void ** outWhereSmashed)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(inBlock->inuse.size < sizeof(Block), err = kMemErrBlockSizeLessThanMinimum;)
		XFAILIF(MISALIGNED(inBlock->inuse.size), err = kMemErrBogusBlockSize;)
		XFAILIF(inBlock->inuse.size >= 256*MByte, err = kMemErrPreposterousBlockSize;)
	}
	XENDTRY;

	if (outWhereSmashed != NULL)
		*outWhereSmashed = inBlock;

	return err;
}


static NewtonErr
VetDirBlock(SHeap * inHeap, Block * inBlock, void ** outWhereSmashed)
{
	return VetBlock(inHeap, inBlock, outWhereSmashed);
}


static NewtonErr
VetDynBlock(SHeap * inHeap, Block * inBlock, void ** outWhereSmashed)
{
	NewtonErr err = VetBlock(inHeap, inBlock, outWhereSmashed);
	if (err != noErr)
		return err;
	if ((err = CheckPointer(inHeap->masterPtrHeap, (Ptr) inBlock->inuse.link, false)) == noErr)
	{
		if (HeapPtr(inBlock->inuse.link)->start != (VAddr) (inBlock+1))
			err = kMemErrBadMasterPointer;
	}
	if (outWhereSmashed != NULL)
		*outWhereSmashed = inBlock;
	return err;
}


// maybe destined for debug build?

void
SetHeapValidation(Heap inHeap)	// perhaps more args
{ /* this really does nothing */ }

void
ValidateHeap(Heap inHeap, int inArg2)
{ /* this really does nothing */ }

#pragma mark -

/*------------------------------------------------------------------------------
	T a s k - s a f e   H e a p   W a l k i n g
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Get the heap’s seed.
	Args:		inHeap		the heap
	Return:	its seed
------------------------------------------------------------------------------*/

long
HeapSeed(Heap inHeap)
{
	if (inHeap == NULL)
		inHeap = GetCurrentHeap();
	return HeapPtr(inHeap)->seed;
}


/*------------------------------------------------------------------------------
	Task-safe heap walking.
	Seed-based.  Ask the heap for a seed, then pass the seed in on
	every operation.  If the seed changes out from under you, get
	another seed and start over.
	Args:		inHeap					heap to walk
				inSeed					permission to walk heap
				inFromBlock				from block, NULL for first block
				outFoundBlock			next block
				outFoundBlockHandle	block's handle, if any
				outFoundBlockType		block's type (see below)
				outFoundBlockTag		block's tag
				outFoundBlockSize		block's size (apparent size, not true size)
				outFoundBlockOwner	block's owner
	Return:	block type; kMM_HeapEndBlock at the end of the heap
------------------------------------------------------------------------------*/

int
NextHeapBlock(	Heap			inHeap,
					long			inSeed,
					void *		inFromBlock,			// r5
					void **		outFoundBlock,			// r4
					void ***		outFoundBlockHandle,	// r10
					int *			outFoundBlockType,	// r9
					char *		outFoundBlockTag,		// r8
					Size *		outFoundBlockSize,	// r7
					ObjectId *	outFoundBlockOwner)	// r6
{
	Block *	block;
	Size		blockSize;
	int		blockType;

	if (inHeap == NULL)
		inHeap = GetCurrentHeap();	// r1
	if (inSeed != HeapPtr(inHeap)->seed)
		return kMM_HeapSeedFailure;

	if (inFromBlock != NULL)
		block = (Block *)inFromBlock - 1;
	else
		block = (Block *)inHeap - 1;

	if ((block->flags & 0x80) == 0)	// it’s free
		blockSize = block->free.size;
	else
		blockSize = block->inuse.size;
	block = (Block *)((Ptr) block + blockSize);
	if ((VAddr) block >= HeapPtr(inHeap)->end)
		block = (Block *)inFromBlock;
	
	if ((block->flags & 0x80) == 0)	// it’s free
	{
		blockType = kMM_HeapFreeBlock;
		if (outFoundBlock != NULL)
			*outFoundBlock = block + 1;	// return data pointer
		if (outFoundBlockHandle != NULL)
			*outFoundBlockHandle = NULL;
		if (outFoundBlockType != NULL)
			*outFoundBlockType = blockType;
		if (outFoundBlockTag != NULL)
			*outFoundBlockTag = 0;
		if (outFoundBlockSize != NULL)
			*outFoundBlockSize = block->free.size;	// including block header!
		if (outFoundBlockOwner != NULL)
			*outFoundBlockOwner = 0;
	}
	else if ((block->inuse.flags & 0x03) == 0x01)	// it’s a direct block
	{
		if ((block->inuse.flags & 0x10) != 0
		&&  block->inuse.busy == 0xFF)	// it’s an internal block
		{
			// map block type
			switch (block->inuse.type)
			{
			case 1:
			case 2:
				blockType = kMM_InternalBlock;
				break;
			case 3:
				blockType = kMM_HeapHeaderBlock;
				break;
			case 4:
				blockType = kMM_HeapEndBlock;
				break;
			case 5:
				blockType = kMM_HeapMPBlock;
				break;
			default:
				return -1;	// this, of course, should never happen!
			}
			if (outFoundBlock != NULL)
				*outFoundBlock = block + 1;
			if (outFoundBlockHandle != NULL)
				*outFoundBlockHandle = NULL;
			if (outFoundBlockType != NULL)
				*outFoundBlockType = blockType;
			if (outFoundBlockTag != NULL)
				*outFoundBlockTag = 0;
			if (outFoundBlockSize != NULL)
				*outFoundBlockSize = block->inuse.size;
			if (outFoundBlockOwner != NULL)
				*outFoundBlockOwner = 0;
		}
		else	// it’s REALLY a direct block
		{
			blockType = kMM_HeapPtrBlock;
			if (outFoundBlock != NULL)
				*outFoundBlock = block + 1;
			if (outFoundBlockHandle != NULL)
				*outFoundBlockHandle = NULL;
			if (outFoundBlockType != NULL)
				*outFoundBlockType = blockType;
			if (outFoundBlockTag != NULL)
				*outFoundBlockTag = block->inuse.type;
			if (outFoundBlockSize != NULL)
				*outFoundBlockSize = block->inuse.size;
			if (outFoundBlockOwner != NULL)
				*outFoundBlockOwner = block->inuse.owner;
		}
	}
	else	// it’s an indirect block
	{
		blockType = kMM_HeapHandleBlock;
		if (outFoundBlock != NULL)
			*outFoundBlock = block + 1;
		if (outFoundBlockHandle != NULL)
			*outFoundBlockHandle = (void **) block->inuse.link;
		if (outFoundBlockType != NULL)
			*outFoundBlockType = blockType;
		if (outFoundBlockTag != NULL)
			*outFoundBlockTag = block->inuse.type;
		if (outFoundBlockSize != NULL)
			*outFoundBlockSize = block->inuse.size;
		if (outFoundBlockOwner != NULL)
			*outFoundBlockOwner = block->inuse.owner;
	}

	return blockType;
}


/*------------------------------------------------------------------------------
	Count blocks in a heap (of various types).
	Args:		outTotalSize		size of all matching blocks
				outFoundCount		number of matching blocks
				inHeap				heap to visit (NULL for current heap)
				inBlockType			block type filter (zero, kMM_HeapPtrBlock or kMM_HeapHandleBlock)
				inName				name to match (or zero)
				inNameMask			mask of name (or zero)
	Return:	--
------------------------------------------------------------------------------*/

void
CountHeapBlocks(	Size *	outTotalSize,
						Size *	outFoundCount,
						Heap		inHeap,
						int		inBlockType,
						ULong		inName,
						ULong		inNameMask)
{
	for ( ; ; )
	{
		if (inHeap == NULL)
			inHeap = GetCurrentHeap();

		Size		totalSize = 0;
		Size		foundCount = 0;
		long		seed = HeapSeed(inHeap);
		Block *	block = NULL;

		for ( ; ; )
		{
			Size		blockSize;
			ULong		blockOwner;
			int blockType = NextHeapBlock(inHeap, seed, block, (void**)&block, NULL, NULL, NULL, &blockSize, &blockOwner);

			if (blockType == kMM_HeapSeedFailure)
				// wups! try again with the right seed
				break;

			else if (blockType == kMM_HeapEndBlock)
			{
				// that’s it; set totals and return
				if (outTotalSize != NULL)
					*outTotalSize = totalSize;
				if (outFoundCount != NULL)
					*outFoundCount = foundCount;
				return;
			}

			else if ((blockType == kMM_HeapPtrBlock || blockType == kMM_HeapHandleBlock)
				  &&  (inBlockType == 0 || inBlockType == blockType)
				  &&  (blockOwner & inNameMask) == inName)
			{
				// it’s a matching block; count it
				totalSize += blockSize;
				foundCount++;
			}
		}
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	Convert block address to heap address.
	Args:		inVoidStar		block address
	Return:	pointer to heap structure
------------------------------------------------------------------------------*/

Heap
VoidStarToHeap(void * inVoidStar)
{
	return (Heap) ((Block *)inVoidStar + 1);
}

/*------------------------------------------------------------------------------
	Convert heap address to block address.
	Args:		inHeap		heap address
	Return:	pointer to block containing heap structure
------------------------------------------------------------------------------*/

void *
HeaptoVoidStar(Heap inHeap)
{
	return ((Block *)inHeap - 1);	// function isn't in symbol table
}
