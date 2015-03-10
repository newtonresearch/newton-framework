/*
	File:		SafeHeap.cc

	Contains:	Safe heap functions.

	Written by:	Newton Research Group.
*/

#include "Newton.h"
#include "UserGlobals.h"
#include "MemArch.h"
#include "PageManager.h"
#include "MemMgr.h"
#include "MMU.h"
#include "SafeHeap.h"

#include <new>


#define kSafeHeapAvailable	(kPageSize - 0x30)
#define kSafeHeapSentinel	(kPageSize - 4)

extern CPageManager *	gThePageManager;

extern VAddr	GetNextKernelVAddr(void);


NewtonErr
GetNewPageFromPageMgr(void ** outPage, ObjectId * outId, CPhys ** outPhys)
{
	NewtonErr err = noErr;
	XTRY
	{
		VAddr pageAddr;

		if (gThePageManager != NULL)
		{
			ObjectId	id;
			XFAIL(err = gThePageManager->get(id, gCurrentTaskId, 2))
			pageAddr = GetNextKernelVAddr();
			*outPage = (void *)pageAddr;
			XFAILIF(err = CUDomainManager::remember(gKernelDomainId, pageAddr, 0xFF, id, YES), gThePageManager->release(id);)
			*outId = id;
			*outPhys = NULL;
		}

		else
		{
		//	no page manager yet
			CLittlePhys *	physPage = gPageTracker->take();
			pageAddr = GetNextKernelVAddr();
			*outPage = (void *)pageAddr;
			RememberMappingUsingPAddr(pageAddr, 0xFF, physPage->base(), YES);
			*outId = kNoId;
			*outPhys = physPage;
		}
	}
	XENDTRY;
	return noErr;
}


void
AddPartialPageToSafeHeap(void * inPartialPage, SSafeHeapPage * inSafeHeap)
{
	if (inPartialPage != NULL
	&&  kPageSize - PAGEOFFSET(inPartialPage) > 0x2C)
	{
		SSafeHeapPage *	page = new (inPartialPage) SSafeHeapPage;
		page->init(0, NULL, inSafeHeap);
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	Initialize a safe heap.
	A safe heap is a linked list of pages (4K in size), and permanently resident.
	Objects in a safe heap cannot cross page boundaries.
	Args:		outHeap			the heap
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
InitSafeHeap(SSafeHeapPage ** outHeap)
{
	NewtonErr	err;
	void *		heapPage;
	ULong			sp04;
	CPhys *		sp00;

	if ((err = GetNewPageFromPageMgr(&heapPage, &sp04, &sp00)) == noErr)
	{
		SSafeHeapPage *	page = new (heapPage) SSafeHeapPage;
		page->init(sp04, sp00, NULL);
		*outHeap = page;
	}
	return err;
}


/*------------------------------------------------------------------------------
	Determine whether the safe heap is completely empty.
	Args:		inHeap			the heap
	Return:	YES => it’s empty
------------------------------------------------------------------------------*/

bool
SafeHeapIsEmpty(SSafeHeapPage * inSafeHeap)
{
	return inSafeHeap->fNext == NULL && inSafeHeap->fFree == kSafeHeapAvailable;
}


/*------------------------------------------------------------------------------
	Allocate a block on the safe heap.
	Args:		inSize
				inSafeHeap
	Return:	pointer to block; NULL => no room
------------------------------------------------------------------------------*/

void *
SafeHeapAlloc(long inSize, SSafeHeapPage * inSafeHeap)
{
	void *	ptr;
	long		size = sizeof(SSafeHeapBlock) + LONGALIGN(inSize);
	if (size <= kSafeHeapAvailable) do
	{
		SSafeHeapPage *	page;
		for (page = inSafeHeap->fLastPage; page != NULL; page = page->fPrev)
		{
			if ((ptr = page->alloc(size, inSize)) != NULL)
				return ptr;
		}
	} while (inSafeHeap->getPage() != NULL);	// try to get another page
	return NULL;
}


/*------------------------------------------------------------------------------
	Reallocate a block on the safe heap.
	Args:		inPtr		a block
				inSize	new size required
	Return:	pointer to moved block; NULL => no room
------------------------------------------------------------------------------*/

void *
SafeHeapRealloc(void * inPtr, Size inSize)
{
	void *	newPtr = SafeHeapAlloc(inSize, (SSafeHeapPage *)gKernelHeap);
	if (inPtr != NULL && newPtr != NULL)
	{
		// allocated size might not be same as original size
		Size	newSize = SafeHeapBlockSize(inPtr);
		memmove(newPtr, inPtr, inSize < newSize ? inSize : newSize);
		SafeHeapFree(inPtr);
	}
	return newPtr;
}


/*------------------------------------------------------------------------------
	Free a block on the safe heap.
	Args:		inPtr		a block
	Return:	--
------------------------------------------------------------------------------*/

void
SafeHeapFree(void * inPtr)
{
	if (inPtr != NULL)
	{
		SSafeHeapPage * page = *SafeHeapEndSentinelFor(inPtr);		
		page->free(inPtr);
	}
}


/*------------------------------------------------------------------------------
	Return the safe heap’s page end sentinel.
	Args:		inPtr		a block
	Return:	safe heap page owner for the page
------------------------------------------------------------------------------*/

SSafeHeapPage **
SafeHeapEndSentinelFor(void * inPtr)
{
	return (SSafeHeapPage **) (TRUNC(inPtr, kPageSize) + kSafeHeapSentinel);
}

#pragma mark -

/*------------------------------------------------------------------------------
	S S a f e H e a p B l o c k

	The safe heap block header contains only size information, packed into a
	long.
------------------------------------------------------------------------------*/

SSafeHeapBlock *
SSafeHeapBlock::next(void)
{
	Ptr	nextBlock = (Ptr) this + fSize;
	if (PAGEOFFSET(nextBlock) >= kSafeHeapSentinel)	// why PAGEOFFSET it?
		nextBlock = NULL;
	return (SSafeHeapBlock *)nextBlock;
}


Size
SafeHeapBlockSize(void * inPtr)
{
	if (inPtr != NULL)
	{
		SSafeHeapBlock *	block = (SSafeHeapBlock *)inPtr - 1;
		return block->fSize - block->fDelta;
	}
	return 0;
}

#pragma mark -

/*------------------------------------------------------------------------------
	S S a f e H e a p P a g e

	A safe heap consists of safe heap pages. Each safe heap page is exactly
	kPageSize in size, and page aligned. Objects within the heap cannot cross
	page boundaries, which limits their size to just under 4K.

	Each page contains a SSafeHeapPage object, typically at its start. This
	points to a linked list of SSafeHeapBlocks within the page. The last word
	in the page is a pointer to the SSafeHeapPage object controlling the page.
------------------------------------------------------------------------------*/

void
SSafeHeapPage::init(ULong inArg1, CPhys * inPhys, SSafeHeapPage * inLink)
{
	int	pageSpaceRemaining = kPageSize - PAGEOFFSET(this);
	if (pageSpaceRemaining >= 0x3C)	// MUST NOT wrap over page boundary
	{
		fNext = NULL;
		fPrev = (inLink != NULL) ? inLink->fLastPage : NULL;
		fTag = 'safe';
		fLastPage = this;
		fFree = pageSpaceRemaining - sizeof(SSafeHeapPage) - sizeof(SSafeHeapBlock);
		fSem = NULL;
		f1C = inArg1;
		fPhys = static_cast<CLittlePhys *>(inPhys);
		SSafeHeapBlock *	block = (SSafeHeapBlock *)((Ptr) this + sizeof(SSafeHeapPage));
		block->fDelta = 0xFF;
		block->fSize = fFree;
		fFreeBlock = block;
		fDescr = (inLink != NULL) ? inLink->fDescr : NULL;
		*SafeHeapEndSentinelFor(this) = this;
		if (inLink != NULL)
		{
			inLink->fLastPage->fNext = this;
			for ( ; inLink != NULL; inLink = inLink->fNext)
				inLink->fLastPage = this;
		}
	}
}


void *
SSafeHeapPage::alloc(long inAllocSize, long inActualSize)
{
	SSafeHeapBlock *	block;
	SSafeHeapBlock *	prevFreeBlock;

	if (inAllocSize <= fFree)
	{
		block = fFreeBlock;
		if (block != NULL
		&&  block->fSize >= inAllocSize)
			goto shortcut;

		prevFreeBlock = NULL;
		for (block = (SSafeHeapBlock *)(this + 1); block != NULL;  block = block->next())
		{
			if (block->fDelta == 0xFF)
			{
			//	block is free
				if (prevFreeBlock != NULL)
				{
				//	we had a free block before - coalesce it with this one
					prevFreeBlock->fSize += block->fSize;
					fFreeBlock = block = prevFreeBlock;
				}
shortcut:
				if (block->fSize < inAllocSize)
				//	block is too small, but free, so mark it for coalescence
					prevFreeBlock = block;
				else
				{
					if (block->fSize - inAllocSize < 0x0C)
					{
					//	leftover is too small to bother with - waste it
						block->fDelta = block->fSize - inActualSize;
						fFree -= block->fSize;
						if (block == fFreeBlock)
							fFreeBlock = NULL;
					}
					else
					{
					//	create a free block for the leftover
						SSafeHeapBlock *	freeBlock = (SSafeHeapBlock *)((Ptr) block + inAllocSize);
						freeBlock->fDelta = 0xFF;
						freeBlock->fSize = block->fSize - inAllocSize;
						fFreeBlock = freeBlock;
						block->fDelta = inAllocSize - inActualSize;
						block->fSize = inActualSize;
						fFree -= inAllocSize;
					}
					return block + 1;	// return pointer to mem past SSafeHeapBlock header
				}
			}
			else
			//	block is in use
				prevFreeBlock = NULL;
		}
	}
	// didn’t find anything big enough
	return NULL;
}


void
SSafeHeapPage::free(void * inPtr)
{
	SSafeHeapBlock *	block = (SSafeHeapBlock *)inPtr - 1;
	block->fDelta = 0xFF;
	fFree += block->fSize;

	if (fFreeBlock == NULL)
		fFreeBlock = block;
	else
	{
		if (fFreeBlock == block->next())
		{
		//	next block is free too - coalesce
			block->fSize += fFreeBlock->fSize;
			fFreeBlock = block;
		}
		else if (block == fFreeBlock->next())
		{
		//	prev block is free too - coalesce
			fFreeBlock->fSize += block->fSize;
		}
		else if (fFreeBlock->fSize < block->fSize)
		//	freed block is bigger
			fFreeBlock = block;
	}

	if (fFree == kSafeHeapAvailable	// this page is now all free
	&&  fPrev != NULL						// but there’s another page
	&&  ALIGNED(this, kPageSize))
	{
	//	we can free this page
		fPrev->fNext = fNext;			// unlink it from prev page
		if (fNext != NULL)
			fNext->fPrev = fPrev;		// unlink it from next page
		else
			for (SSafeHeapPage * page = firstPage(); page != NULL; page = page->fNext)
				page->fLastPage = fPrev;
		freePage();
	}
}


SSafeHeapPage *
SSafeHeapPage::firstPage(void)
{
	SSafeHeapPage *	page;
	for (page = this; page->fPrev != NULL; page = page->fPrev)
		;
	return page;
}


SSafeHeapPage *
SSafeHeapPage::getPage(void)
{
	NewtonErr			err;
	SSafeHeapPage *	page;
	ULong					sp04;
	CPhys *				sp00;

	if (fSem != NULL)
		fSem->release();
	err = GetNewPageFromPageMgr((void **) &page, &sp04, &sp00);
	if (fSem != NULL)
		fSem->acquire(kWaitOnBlock);

	if (err != noErr)
		page = NULL;
	else
	{
		page = new (page) SSafeHeapPage;
		page->init(sp04, sp00, this);
	}
	return page;
}


void
SSafeHeapPage::freePage(void)
{
	if (f1C != 0)
	{
		CUDomainManager::forget(gKernelDomainId, (VAddr) this, f1C);
		gThePageManager->release(f1C);
	}
	else
		gPageTracker->put(fPhys);
}

#pragma mark -

/*------------------------------------------------------------------------------
	S W i r e d H e a p D e s c r

	The wired heap descriptor describes the heap dimensions and points to
	wired heap backing pages.
------------------------------------------------------------------------------*/

NewtonErr
SWiredHeapDescr::growByOnePage(void)
{
	NewtonErr	err;
	VAddr			limit = fStart + fSize;
	if ((err = SetHeapLimits(fStart, limit + kPageSize)) == noErr)
	{
		if ((err = LockHeapRange(limit, limit + kPageSize, YES)) == noErr)
			fSize += kPageSize;
		else
			SetHeapLimits(fStart, limit - kPageSize);
	}
	return err;
}


NewtonErr
SWiredHeapDescr::shrinkByOnePage(void)
{
	NewtonErr	err;
	VAddr			limit = fStart + fSize;
	if ((err = UnlockHeapRange(limit - kPageSize, limit)) == noErr)
	{
		err = SetHeapLimits(fStart, limit - kPageSize);
		fSize -= kPageSize;
	}
	return err;
}

#pragma mark -

/*------------------------------------------------------------------------------
	S W i r e d H e a p P a g e
------------------------------------------------------------------------------*/

SWiredHeapPage *
SWiredHeapPage::make(SWiredHeapDescr * inDescr)
{
	inDescr->fStart = PAGEALIGN(inDescr->fStart);
	inDescr->fHeap = NULL;
	inDescr->fSize = 0;

	if (inDescr->growByOnePage() != noErr)
		return NULL;

	SWiredHeapPage *	page = new ((void *)inDescr->fStart) SWiredHeapPage;
	page->init(0, NULL, NULL);
	page->fDescr = inDescr;
	return page;
}


NewtonErr
SWiredHeapPage::destroy(void)
{
	FreePagedMem(fDescr->fStart);
	return noErr;
}


SSafeHeapPage *
SWiredHeapPage::getPage(void)
{
	if (fDescr->growByOnePage() != noErr)
		return NULL;

	SWiredHeapPage *	page = new ((void *)(fDescr->fStart + fDescr->fSize - kPageSize)) SWiredHeapPage;
	page->init(0, NULL, this);
	return page;
}


void
SWiredHeapPage::freePage(void)
{
	VAddr	pageTop, heapTop = 0;
	SWiredHeapPage *	heap = (SWiredHeapPage *)firstPage();
	SWiredHeapPage *	page;
	for (page = (SWiredHeapPage *)firstPage(); page != NULL; page = (SWiredHeapPage *)page->fNext)
	{
		pageTop = (VAddr) page + kPageSize;
		if (heapTop < pageTop)
			heapTop = pageTop;
	}
	while (heap->fDescr->fStart + heap->fDescr->fSize > heapTop)
		heap->fDescr->shrinkByOnePage();
}

#pragma mark -

/*------------------------------------------------------------------------------
	W i r e d   P o i n t e r s
------------------------------------------------------------------------------*/

Ptr
NewWiredPtr(Size inSize)
{
	Ptr	p = NULL;

	bool	hasSemaphore = (GetHeapSemaphore(NULL) != NULL);
	if (hasSemaphore)
		GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);

	SWiredHeapDescr * descr;
	if ((descr = GetWiredHeap(GetCurrentHeap())) != NULL)
		p = (Ptr) SafeHeapAlloc(inSize, descr->fHeap);
	else
	{
	//	have to create a wired heap
		VAddr start, end;
		if (NewHeapArea(TaskSwitchedGlobals()->fDefaultHeapDomainId, 0, 32*KByte, 1, &start, &end) == noErr)
		{
			descr = (SWiredHeapDescr *)NewDirectBlock(sizeof(SWiredHeapDescr));
			if (descr != NULL)
			{
				gPtrsUsed += inSize;
				IncrementBlockBusy(descr);
				descr->fStart = PAGEALIGN(start);
				descr->fEnd = end;
				descr->fHeap = SWiredHeapPage::make(descr);
				if (descr->fHeap != NULL)
				{
					SetWiredHeap(GetCurrentHeap(), descr);
					p = (Ptr) SafeHeapAlloc(inSize, descr->fHeap);
				}
				else
				{
					DecrementBlockBusy(descr);
					DisposeDirectBlock((Ptr) descr);
					gPtrsUsed -= inSize;
				}
			}
			else
			{
				FreePagedMem(start);
			}
		}
	}

	if (hasSemaphore)
		GetHeapSemaphore(NULL)->release();

	return p;
}


void
FreeWiredPtr(Ptr inPtr)
{
	bool	hasSemaphore;
	if ((hasSemaphore = (GetHeapSemaphore(NULL) != NULL)))
		GetHeapSemaphore(NULL)->acquire(kWaitOnBlock);

	SWiredHeapDescr * descr;
	if ((descr = GetWiredHeap(GetCurrentHeap())) != NULL)
	{
		SafeHeapFree(descr);
		if (SafeHeapIsEmpty(descr->fHeap))
		{
			descr->fHeap->destroy();
			Size size = GetDirectBlockSize((Ptr) descr);
			DecrementBlockBusy((Ptr) descr);
			DisposeDirectBlock((Ptr) descr);
			gPtrsUsed -= size;
			SetWiredHeap(GetCurrentHeap(), NULL);
		}
	}

	if (hasSemaphore)
		GetHeapSemaphore(NULL)->release();
}
