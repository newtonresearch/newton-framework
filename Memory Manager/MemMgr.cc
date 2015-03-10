/*
	File:		MemMgr.cc

	Contains:	Memory management functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "MemMgr.h"

#define usesMacMemMgr 1

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

ArrayIndex	gHandlesUsed;				// 0C101010
ArrayIndex	gPtrsUsed;					// 0C101014
ArrayIndex	gSavedHandlesUsed;		// 0C101018
ArrayIndex	gSavedPtrsUsed;			// 0C10101C

// MemMgr debug
bool			sMMBreak_Enabled;			// 0C10165C
Size			sMMBreak_LT;				// 0C101660
Size			sMMBreak_GE;				// 0C101664
ArrayIndex	sMMBreak_OnCCHash;		// 0C101668
ArrayIndex	sMMBreak_OnCount;			// 0C10166C
Ptr			sMMBreak_AddressIn;		// 0C101670
Ptr			sMMBreak_AddressOut;		// 0C101674

ArrayIndex	gMemMgrCallCounter;		// 0C101678
bool			sMMTagBlocksWithCCHash;	// 0C10167C


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

void		KillBlock(void * inPtr);
int		TrySetSize(Block ** ioBlock, Size inDiff, Size inSize);
void		JumpBlock(Block * inBlock, Block * inLimit);
Block *	FindSmallestSlide(Ptr * arg1, Size arg2, Size arg3);
Block *	LockedBlock(Block * inStart, Block * inEnd);
Block *	SearchFreeList(Size inSize);
Block *	SlideBlocksUp(Block * inBlock, Block * inLimit);
Block *	SlideBlocksDown(Block * inBlock, Block * inLimit);
void		SetFreeChain(Block * inBlock, Block * inPrevBlock, Block * inNextBlock);
void		MoveFreeBlock(Block * inBlock, long inOffset);
void		RemoveFreeBlock(Block * inBlock);

MPPtr		AllocateMasterPointer(Heap inHeap);
void		AllocateMoreMasters(void);
void		FreeMasterPointer(Heap inHeap, MPPtr inMP);

static NewtonErr	ShrinkSkiaHeapLeaving(Heap inHeap, Size inLeavingAmount);


/*------------------------------------------------------------------------------
	H e a p   b l o c k   c r e a t i o n
------------------------------------------------------------------------------*/

void
CreatePrivateBlock(Block * ioBlock, int inType)
{
	ioBlock->inuse.flags = 0x80 | 0x10 | 0x01;
	ioBlock->inuse.delta = 0;
	ioBlock->inuse.busy = 0xFF;
	ioBlock->inuse.type = inType;
	ioBlock->inuse.link = GetCurrentHeap();
}


int
ExtendVMHeap(Heap inHeap, Size inSize)
{
	SHeap *	heap = (SHeap *)inHeap;
	bool		isTailFree = (heap->freeListTail != NULL
					&& (VAddr) heap->freeListTail + heap->freeListTail->free.size + sizeof(Block) == heap->end);
	Size		extendSize = ALIGN(inSize, heap->pageSize);
	Size		newSize = heap->extent + extendSize;
	if (newSize <= heap->size)
	{
		if (heap->isVMBacked)
		{
			heap->maxExtent = newSize;
			if (newSize > heap->prevMaxExtent)
			{
				if (SetHeapLimits(heap->start, heap->start + newSize) == noErr)
				{
					if (LockHeapRange(heap->start + heap->prevMaxExtent, heap->start + heap->maxExtent) == noErr)
					{
						if (UnlockHeapRange(heap->start + heap->prevMaxExtent, heap->start + heap->maxExtent) == noErr)
							heap->prevMaxExtent = heap->maxExtent;
						else
						{
						//	unlock didn’t work
							heap->maxExtent = heap->extent;
							return NO;
						}
					}
					else
					{
					//	lock didn’t work; restore prev heap limits
						SetHeapLimits(heap->start, heap->start + heap->prevMaxExtent);
						heap->maxExtent = heap->extent;
						return NO;
					}
				}
			}
		}
		// heap isn’t VM backed, or we set up VM backing okay
		// move end block
		Block *	blockPtr = (Block *)heap->end - 1;
		*(Block *)((Ptr) blockPtr + extendSize) = *blockPtr;
		// update heap sizes
		heap->extent += extendSize;
		heap->free += extendSize;
		heap->end = heap->start + heap->extent;
		// update free block
		if (isTailFree)
			heap->freeListTail->free.size += extendSize;
		else
		{
			blockPtr = (Block *)(heap->end - extendSize) - 1;
			blockPtr->free.size = extendSize;
			blockPtr->free.prev = heap->freeListTail;
			blockPtr->free.next = NULL;
			if (heap->freeList == NULL)
				heap->freeList = blockPtr;
			if (heap->freeListTail != NULL)
				heap->freeListTail->free.next = blockPtr;
			heap->freeListTail = blockPtr;
			heap->x48 = blockPtr;
		}
		return YES;
	}

	return NO;
}


/*------------------------------------------------------------------------------
	Attempt to shrink a heap.
	Args:		inHeap				the heap
				inLeavingAmount	its new free size
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
ShrinkHeapLeaving(Heap inHeap, Size inLeavingAmount)
{
	NewtonErr	err;
	err = ShrinkSkiaHeapLeaving(inHeap, inLeavingAmount);
	if (inHeap != GetRelocHeap(inHeap))
		err = ShrinkSkiaHeapLeaving(GetRelocHeap(inHeap), inLeavingAmount);
	return err;
}


static NewtonErr
ShrinkSkiaHeapLeaving(Heap inHeap, Size inLeavingAmount)
{
	NewtonErr	err = noErr;
	SHeap *		heap = (SHeap *)inHeap;

	if (inLeavingAmount < sizeof(Block))
		inLeavingAmount = sizeof(Block);

	if (heap->freeListTail != NULL
	&&  ((VAddr) heap->freeListTail + heap->freeListTail->free.size + sizeof(Block)) == heap->end)
	{
		// last free block in list is also last in available memory
		VAddr	newEnd = ALIGN((VAddr)heap->freeListTail + inLeavingAmount + sizeof(Block), heap->pageSize);
		if (newEnd < heap->end)
		{
			Block *	blockPtr = (Block *)newEnd - 1;
			*blockPtr = *(((Block *)heap->end) - 1);
			if (blockPtr != NULL)
				heap->freeListTail = blockPtr;
			else
			{
				heap->freeList = NULL;
				heap->freeListTail = NULL;
			}
			heap->free -= (heap->freeListTail - blockPtr);
			heap->end = newEnd;
			heap->extent = heap->maxExtent = newEnd - heap->start;
		}
	}

	return err;
}


/*------------------------------------------------------------------------------
	Compact a heap.
	Shuffle blocks down to maximise free space.
	Args:		inHeap				the heap
				inBlock				
	Return:	pointer to free space
------------------------------------------------------------------------------*/

void *
CompactHeap(Heap inHeap, void * inBlock)
{
	SHeap *	heap = (SHeap *)inHeap;
	SHeap *	savedHeap = (SHeap *)GetCurrentHeap();

	Block *	freeBlock, * nextFreeBlock, * lockedFreeBlock = NULL;
	Block *	block = (Block *)inBlock;
	if (block != NULL)
		block--;
	SetCurrentHeap(inHeap);

	for (freeBlock = heap->freeList; freeBlock != NULL && (nextFreeBlock = freeBlock->free.next) != NULL; )
	{
		if (LockedBlock(freeBlock, nextFreeBlock) != NULL)
		{
			// note the locked block
			lockedFreeBlock = freeBlock;
			// jump over it
			freeBlock = nextFreeBlock;
		}
		else
		{
			if (block != NULL
			&&  block > freeBlock
			&&  block < nextFreeBlock)
				// our block will be slid down; adjust its address
				block = (Block *)((Ptr) block - freeBlock->free.size);
			// slide blocks down so free blocks coalesce
			SlideBlocksDown(freeBlock, nextFreeBlock);
			// reset free block pointer
			freeBlock = (lockedFreeBlock != NULL) ? lockedFreeBlock->free.next : heap->freeList;
		}
	}

	SetCurrentHeap(savedHeap);
	if (block != NULL)
		block++;
	return block;
}


/*------------------------------------------------------------------------------
	Create a new block in the heap.
	Args:		inSize				size required
	Return:	pointer to block
------------------------------------------------------------------------------*/

Block *
NewBlock(Size inSize)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	Size		reqdSize = sizeof(Block) + inSize;
	int		delta = ~(reqdSize - 1) & 0x03;		// delta w/ long alignment
	int		blockDelta;
	Size		blockSize = reqdSize + delta;
	Block *	theBlock, * freeBlock;

	heap->x48 = heap->freeList;

	// look for a free block of a larger size
	while ((theBlock = heap->x48) == NULL || theBlock->free.size < blockSize)
	{
		freeBlock = SearchFreeList(blockSize);
		if ((freeBlock == NULL || freeBlock->free.size < blockSize)
		 && ExtendVMHeap(heap, blockSize) == NO)
			return NULL;		// can’t find a block of the required size, and can’t extend the heap
	}

	if ((blockDelta = theBlock->free.size - blockSize) > (int) sizeof(Block))
		// there’s room for another block too
		MoveFreeBlock(theBlock, blockSize);
	else
	{
		// there’s not enough room; waste the excess space
		delta = (delta + blockDelta) & 0xFF;
		blockSize = theBlock->free.size;
		RemoveFreeBlock(theBlock);
	}

	heap->free -= blockSize;
	heap->x68 += delta;
	heap->x6C += (delta & ~0x03);

	theBlock->free.size = 0;		// not really - just clear first four heap struct bytes
	theBlock->inuse.delta = delta;
	theBlock->inuse.busy = 0;
	theBlock->inuse.size = blockSize;
	theBlock->inuse.owner = gCurrentTaskId;

	if (heap->x4C == 0)
		heap->x48 = heap->freeList;
	if (heap->x54)
		heap->x54();

	return theBlock;
}


/*------------------------------------------------------------------------------
	Adjust the size of a block in the heap.
	Args:		inBlock				a pointer to adjust; ie block header precedes this
				inSize				its new size
	Return:	new pointer to data in block
------------------------------------------------------------------------------*/

Ptr
SetBlockSize(Ptr inPtr, Size inSize)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();	// sp00

	Block *	theBlock = ((Block *)inPtr) - 1;	// sp08
	Block *	blockPtr = theBlock;						// r9
	int		r10 = 1;

	Size		currSize = blockPtr->inuse.size;			// r8
	Size		reqdSize = inSize + sizeof(Block);		// r0
	Size		extraBytesToAlign = ~(reqdSize - 1) & 0x03;	// r6
	Size		alignedSize = reqdSize + extraBytesToAlign;		// r5
	long		sizeDiff = alignedSize - currSize;		// r4

	if (sizeDiff == 0)
	{
		// block size hasn’t changed, but deltas might have
		heap->x68 -= (blockPtr->inuse.delta - extraBytesToAlign);
		heap->x6C -= TRUNC(blockPtr->inuse.delta, 4);
		blockPtr->inuse.delta = extraBytesToAlign;
		if (heap->x4C == 0)
			heap->x48 = heap->freeList;
		if (heap->x54)
			heap->x54();
		return (Ptr) (blockPtr + 1);
	}

	else if (sizeDiff < 0)
	{
		// block is now smaller
		heap->x68 -= blockPtr->inuse.delta;
		heap->x6C -= TRUNC(blockPtr->inuse.delta, 4);
		sizeDiff = -sizeDiff;
		if (sizeDiff < sizeof(Block))
		{
			// have to subsume the small extra amount into the delta
			extraBytesToAlign += sizeDiff;
			alignedSize += TRUNC(sizeDiff, 4);
		}
		else
		{
			// create a dummy block of the unwanted size and kill it
			Block *	unwantedBlock = (Block *)((Ptr) blockPtr + alignedSize);
			unwantedBlock->inuse.flags = 0;
			unwantedBlock->inuse.size = sizeDiff;
			KillBlock(unwantedBlock + 1);
		}
		heap->x68 += extraBytesToAlign;
		heap->x6C += TRUNC(extraBytesToAlign, 4);
		blockPtr->inuse.size = alignedSize;
		blockPtr->inuse.delta = extraBytesToAlign;
		if (heap->x4C == 0)
			heap->x48 = heap->freeList;
		if (heap->x54)
			heap->x54();
		return (Ptr) (blockPtr + 1);
	}

	else
	{
L62:
		// block is now larger
		if (sizeDiff < heap->free)
		{
			// block will fit within existing heap
L66:
			int	result = TrySetSize(&theBlock, sizeDiff, alignedSize);
			switch (result)
			{
			case 3:	// prev block is free; claim it
//79
				{
					//sp-04
					Block *	nextFreeBlock, * prevFreeBlock;
					Block *	freeBlockNext, * freeBlockPrev;
					Size		prevFreeSize;
					blockPtr = theBlock;
					// follow links to the prev free block
					for (nextFreeBlock = heap->freeList; nextFreeBlock != NULL && nextFreeBlock < blockPtr; nextFreeBlock = nextFreeBlock->free.next)
						prevFreeBlock = nextFreeBlock;
					prevFreeSize = prevFreeBlock->free.size;
					freeBlockNext = prevFreeBlock->free.next;
					freeBlockPrev = prevFreeBlock->free.prev;
					if ((blockPtr->inuse.flags & 0x01) != 0)
					{
						if (heap->x4C != NULL)
							heap->x4C(blockPtr + 1, prevFreeBlock + 1);
					}
					else
						*(MPPtr*)(blockPtr->inuse.link) = (MPPtr) (prevFreeBlock + 1);
//109
					// move our block back into the free space
					memmove(prevFreeBlock, blockPtr, currSize);
					blockPtr = prevFreeBlock;
					blockPtr->inuse.flags &= ~0x04;
					// move the free block past our block; ie their positions are now reversed
					prevFreeBlock = (Block *)((Ptr) prevFreeBlock + currSize);
					prevFreeBlock->free.size = prevFreeSize;
					SetFreeChain(prevFreeBlock, freeBlockPrev, freeBlockNext);
					// keep that mysterious x48 updated
					if (heap->x48 == blockPtr)
						heap->x48 = prevFreeBlock;
					
					nextFreeBlock = prevFreeBlock + prevFreeSize;
					if ((VAddr) nextFreeBlock < heap->end)
					{
						if ((nextFreeBlock->flags & 0x80) != 0)
							nextFreeBlock->inuse.flags |= 0x04;
						else
						{
							// next block is free too; remove our free block from the free list
							nextFreeBlock->free.prev->free.size += nextFreeBlock->free.size;
							SetFreeChain(nextFreeBlock->free.prev, nextFreeBlock->free.prev->free.prev, nextFreeBlock->free.next);
						}
					}
//147
					theBlock = blockPtr;
					//sp+04
				}
				// fall thru…

			case 1:	// next block is free; claim it
//149
				{
					Block *	nextBlock = (Block *)((Ptr) theBlock + currSize);
					Size		extra = nextBlock->free.size - sizeDiff;
					heap->x68 -= theBlock->inuse.delta;
					heap->x6C -= TRUNC(theBlock->inuse.delta, 4);
					if (extra >= sizeof(Block))
					{
						// move the following free block header up to make the room we require
						Block *	newNextBlock = (Block *)((Ptr) nextBlock + sizeDiff);
						// update prev link pointer
						if ((newNextBlock->free.prev = nextBlock->free.prev) == NULL)
							heap->freeList = newNextBlock;
						else
							nextBlock->free.prev->free.next = newNextBlock;
						// update next link pointer
						if ((newNextBlock->free.next = nextBlock->free.next) == NULL)
							heap->freeListTail = newNextBlock;
						else
							nextBlock->free.next->free.prev = newNextBlock;
						// update size
						newNextBlock->free.size = extra;
						if (heap->x48 == NULL)
							heap->x48 = newNextBlock;
					}
					else
					{
//190
						// only just enough room; remove the free block entirely
						RemoveFreeBlock(nextBlock);
						sizeDiff += extra;
						extraBytesToAlign += extra;
						alignedSize += extra;
					}
					heap->free -= sizeDiff;
//200
					heap->x68 += extraBytesToAlign;
					heap->x6C += TRUNC(extraBytesToAlign, 4);
				}
				// fall thru…

			case 5:	// no worries, there’s room within this block
//211
				blockPtr->inuse.size = alignedSize;
//213
				blockPtr->inuse.delta = extraBytesToAlign;
//214
				if (heap->x4C == 0)
					heap->x48 = heap->freeList;
				if (heap->x54)
					heap->x54();
				return (Ptr) (blockPtr + 1);
				break;

			case 4:	// can’t extend this block, but room elsewhere in the heap
//229
				// create new block
				blockPtr = NewBlock(inSize);
				blockPtr->inuse.flags = theBlock->inuse.flags & ~0x04;
				blockPtr->inuse.busy = theBlock->inuse.busy;
				blockPtr->inuse.type = theBlock->inuse.type;
				// copy data from old block
				memmove(&blockPtr->inuse.link, &theBlock->inuse.link, currSize - offsetof(Block, inuse.link));
				// if indirect block, update link to new location
				if ((theBlock->inuse.flags & 0x02) != 0)
					*(MPPtr*)(theBlock->inuse.link) = (MPPtr) (blockPtr + 1);
				// kill old block
				KillBlock(inPtr);
				if (heap->x4C == 0)
					heap->x48 = heap->freeList;
				if (heap->x54)
					heap->x54();
				return (Ptr) (blockPtr + 1);
				break;
			}
		}

//252
		else
		{
			// block is too large for existing heap
			theBlock->inuse.busy += 2;
			bool	r9 = heap->x50 != NULL && r10;
			if (r9)
				r10 = heap->x50(alignedSize);
			else
			{
//270
				if (inSize > heap->free)
				{
					// gotta extend the heap
					if ((heap->x58
					 &&  heap->x58(heap->start, alignedSize - heap->free))
					||  ExtendVMHeap(heap, alignedSize))
						r9 = YES;
					else
					{
						// can’t do it; 
						theBlock->inuse.busy -= 2;
						if (heap->x54)
							heap->x54();
						return NULL;
					}
				}
			}
//290
			theBlock->inuse.busy -= 2;
			if (r9)
				goto L66;
			if (ExtendVMHeap(heap, alignedSize))
				goto L62;
		}
	}
	return NULL;
}

#pragma mark -

/*------------------------------------------------------------------------------
	See if any blocks within a range of blocks is locked.
	Args:		inStart			block AFTER which we start looking for a locked block
				inEnd				block BEFORE which we should find it
	Return:	pointer to locked block
------------------------------------------------------------------------------*/

Block *
LockedBlock(Block * inStart, Block * inEnd)
{
	Block *	block;

	if ((inStart->flags & 0x80) == 0)			// it’s free
		block = (Block *)((Ptr) inStart + inStart->free.size);
	else
		block = (Block *)((Ptr) inStart + inStart->inuse.size);

	 while (block < inEnd)
	{
		if ((block->flags & 0x80) == 0)
			block = (Block *)((Ptr) block + block->free.size);	// it’s free
		else if (block->inuse.busy == 0)
			block = (Block *)((Ptr) block + block->inuse.size);	// it’s used but not locked
		else
			return block;								// it’s busy|locked; return its address
	}

	return NULL;
}


/*------------------------------------------------------------------------------
	Try to set the size of a block.
	Args:		ioBlock			block to adjust; may move!
				inDiff			extra size required for the block
				inSize			total size required for the heap
	Return:	status code		0 => can’t be done
									1 => next block is free and large enough
									2 => 
									3 => prev block is free and large enough
									4 => have to realloc block
									5 => room within the block delta
------------------------------------------------------------------------------*/

int
TrySetSize(Block ** ioBlock, Size inDiff, Size inSize)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();	// r7
	Block *	thisBlock = *ioBlock;	// r8
	Size		thisBlockSize = thisBlock->inuse.size;	// r9
	Block *	nextBlock = (Block *)((Ptr) thisBlock + thisBlockSize);	// sp00

	if (heap->freeListTail >= nextBlock
	&& (nextBlock->flags & 0x80) == 0
	&&  nextBlock->free.size >= inDiff)
		// there’s room after thisBlock
		return 1;

	if ((VAddr) nextBlock < heap->end
	&&  thisBlockSize > inDiff)
	{
		Block *	block;	// r10
		Size		blockSize;
		Size		usedSize = 0;
		Size		freeSize = 0;
		for (block = nextBlock; (VAddr) block < heap->end; )
		{
			if ((block->flags & 0x80) == 0)	// it’s free
			{
				blockSize = block->free.size;
				freeSize += blockSize;
			}
			else if (block->inuse.busy == 0)	// it’s in use but not locked
			{
				blockSize = block->inuse.size;
				usedSize += blockSize;
			}
			else	// it’s locked
				break;
			block = (Block *)((Ptr) block + blockSize);	// move on to next block
			if (usedSize + freeSize >= inDiff)
			{
				// with a bit of shuffling we might expand our block
				if (thisBlockSize >= usedSize
				&&  heap->x4C != NULL
				&&  heap->x48->free.size >= usedSize
				&&  (heap->x48 < thisBlock || heap->x48 >= block))
				{
					JumpBlock(nextBlock, block);	// shift the next block out of the way
					return 1;
				}
				break;
			}
		}
	}
//64
	if (heap->x48 != 0
	&&  heap->x48->free.size >= inSize)
		// there’s room in the x48 block
		return 4;
//73
	Size		r1 = 0;
	bool		r3 = (thisBlock->inuse.flags & 0x04) != 0;
	Block *	r2 = NULL;
	Block *	r0;
	for (r0 = heap->freeList; r0 != NULL && r0 < thisBlock; r0 = r0->free.next)
	{
		r1 += r0->free.size;
		if (heap->x48->free.size < r0->free.size)
			heap->x48 = r0;	// largest free block?
		r2 = r0;
	}
	if (r3 && r2->free.size >= inSize)
		return 3;
	if (heap->x48 != 0 && heap->x48->free.size >= inSize)
		return 4;
//105
	if (heap->free - r1 > inSize)
	{
		for ( ; r0 != NULL; r0 = r0->free.next)
		{
			if (heap->x48->free.size < r0->free.size)
				heap->x48 = r0;	// largest free block?
		}
		if (heap->x48 != 0 && heap->x48->free.size >= inSize)
			return 4;
	}
//125
	if (heap->fixedHeap == heap
	&&  heap->relocHeap != heap)
		return 0;

	if (heap->masterPtrHeap == heap
	&&  heap->relocHeap != heap)
		return 0;

	Ptr		dataPtr = (Ptr) (thisBlock + 1);	// sp00
	Block *	block;	// r0
	if ((block = FindSmallestSlide(&dataPtr, inSize, inDiff)) != NULL)
	{
		*ioBlock = (Block *)dataPtr - 1;
		if (block->free.size >= inDiff
		&& block == (Block *)((Ptr) *ioBlock + thisBlockSize))
			return 1;

		if (block->free.size >= inSize)
		{
			heap->x48 = block;
			return 4;
		}
	}

	return 0;
}


/*------------------------------------------------------------------------------
	Make blocks jump out of the way. Reallocate ’em elsewhere in the heap.
	Args:		inBlock			first block to make jump
				inLimit			block beyond last jumper
	Return:	--
------------------------------------------------------------------------------*/

void
JumpBlock(Block * inBlock, Block * inLimit)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();	// r8

	Block *	block = inBlock;	// r6
	// we MUST have an inuse block, so skip this one if it’s free
	if ((block->flags & 0x80) == 0)
		block = (Block *)((Ptr) block + block->free.size);

	do
	{
		Size		blockSize = block->inuse.size;
		Size		reqdSize = blockSize - block->inuse.delta - sizeof(Block);	// r10

		Block *	nextBlock;
		// create a new block of exactly the right size
		Block *	newBlock = NewBlock(reqdSize);	// r7
		newBlock->inuse.flags = block->inuse.flags & ~0x04;
		newBlock->inuse.busy = block->inuse.busy;
		newBlock->inuse.type = block->inuse.type;

		if ((block->inuse.flags & 0x01) != 0)
		{
			// direct block
			if (heap->x4C != NULL)
				heap->x4C(block + 1, newBlock + 1);
		}
		else
			// indirect block; set up master pointer
			*(MPPtr*)(block->inuse.link) = (MPPtr) (newBlock + 1);

		// copy block data to its new location
		memmove((Ptr)newBlock + 8, (Ptr)block + 8, reqdSize + 8);	// tricky tricky!

		// if next block is free, skip over it
		nextBlock = (Block *)((Ptr) block + blockSize);
		if ((VAddr) nextBlock < heap->end
		&&  (nextBlock->flags & 0x80) == 0)	// it’s free
			blockSize += nextBlock->free.size;

		// kill the original block
		KillBlock(block + 1);

		// block after new block must be free; use it in x48
		heap->x48 = (Block *)((Ptr) newBlock + newBlock->inuse.size);
		// step on to next inuse block
		block = (Block *)((Ptr) block + blockSize);
	} while (block < inLimit);

	if (heap->x48->free.size < inBlock->free.size)
		heap->x48 = inBlock;
}


Block *
FindSmallestSlide(Ptr * arg1, Size arg2, Size arg3)
{
//	slide blocks around to find the required size
//	sp-14
	SHeap *	heap = (SHeap *)GetCurrentHeap();
	Block *	r4 = (arg1 != NULL) ? (Block *)*arg1 - 1 : NULL;
	int		sp0C = 0x7FFFFFFF;
	Block *	freeBlock = heap->freeList;			// r5
	Block *	nextBlock = freeBlock->free.next;	// r6
	if (nextBlock != NULL)
	{
		;
	}
	else
	{
//193
	}
	
	return NULL;
}


/*------------------------------------------------------------------------------
	Slide blocks up into free space, to maximise free space.
	Args:		inLimit		block before which sliding should stop
				inBlock		first free block into which previous are slid up
	Return:	pointer to a free heap block
------------------------------------------------------------------------------*/

Block *
SlideBlocksUp(Block * inLimit, Block * inBlock)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();	// r8

	Block *	thisBlock = inBlock;							// r4
	Block *	prevFreeBlock = thisBlock->free.prev;	// r7
	Block *	nextFreeBlock = thisBlock->free.next;	// r2
//	sp04 = r2;
	Size		r9 = thisBlock->free.size;
	bool		isx48Affected = (thisBlock == heap->x48);

	Block *	r5 = (Block *)((Ptr) inLimit + inLimit->inuse.size);	// r5
	if (thisBlock != r5) do
	{
		Block *	r0;
		Block *	r6 = (r5 > prevFreeBlock) ? r5 : (Block *)((Ptr) prevFreeBlock + prevFreeBlock->free.size);
		Size		r10;
		while ((r0 = (Block *)((Ptr) r6 + (r10 = r6->inuse.size))) < thisBlock)
			r6 = r0;
		thisBlock = (Block *)((Ptr) r6 + r9);
		if ((r6->inuse.flags & 0x01) != 0)
		{
			if (heap->x4C != NULL)
				heap->x4C(r6 + 1, thisBlock + 1);
		}
		else
			*(MPPtr*)(r6->inuse.link) = (MPPtr) (thisBlock + 1);
		memmove(thisBlock, r6, r10);
		thisBlock->inuse.flags |= 0x04;
		thisBlock = (Block *)((Ptr) thisBlock + r10);
		if ((VAddr) thisBlock < heap->end)
			thisBlock->inuse.flags &= ~0x04;
		if ((r6->inuse.flags & 0x04) != 0)
		{
			if (thisBlock == heap->x48)
				isx48Affected = YES;
			thisBlock = prevFreeBlock;
			prevFreeBlock = prevFreeBlock->free.prev;
			r9 += thisBlock->free.size;
		}
		else
			thisBlock = r6;
		thisBlock->free.size = r9;
		SetFreeChain(thisBlock, prevFreeBlock, nextFreeBlock);
	} while (thisBlock < inLimit);

	if (isx48Affected
	||  heap->x48->free.size < thisBlock->free.size)
		heap->x48 = thisBlock;

	return thisBlock;	// must be free
}


/*------------------------------------------------------------------------------
	Slide blocks down into free space, to maximise free space.
	Args:		inBlock		first free block into which following are slid down
				inLimit		block beyond which sliding should stop
	Return:	pointer to a free heap block
------------------------------------------------------------------------------*/

Block *
SlideBlocksDown(Block * inBlock, Block * inLimit)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	Block *	thisBlock = inBlock;	// is free
	Block *	prevFreeBlock = thisBlock->free.prev;
	Block *	nextFreeBlock = thisBlock->free.next;
	Block *	nextBlock = (Block *)((Ptr) thisBlock + thisBlock->free.size);	// will be inuse
	bool		isx48Affected = (thisBlock == heap->x48);

	do
	{
		Size		nextBlockSize = nextBlock->inuse.size;
		Size		freeSize = thisBlock->free.size;
		if ((nextBlock->inuse.flags & 0x02) != 0)		// is indirect?
			*(MPPtr*)(nextBlock->inuse.link) = (MPPtr) (thisBlock + 1);
		nextBlock->inuse.flags &= ~0x04;
		// move the next block down into this free block
		memmove(thisBlock, nextBlock, nextBlockSize);
		// update pointers to skip the moved block
		nextBlock = (Block *)((Ptr) nextBlock + nextBlockSize);
		thisBlock = (Block *)((Ptr) thisBlock + nextBlockSize);
		// what we’re doing is moving the free hole up; so thisBlock is still free, update its size and free chain pointers
		thisBlock->free.size = freeSize;
		if (nextBlock == nextFreeBlock)
		{
			// coalesce free blocks
			thisBlock->free.size = freeSize + nextFreeBlock->free.size;
			nextBlock = (Block *)((Ptr) nextBlock + nextFreeBlock->free.size);
			if (nextFreeBlock == heap->x48)
				isx48Affected = YES;
			nextFreeBlock = nextFreeBlock->free.next;
		}
		SetFreeChain(thisBlock, prevFreeBlock, nextFreeBlock);
	} while (nextBlock < inLimit);

	if (isx48Affected
	||  heap->x48->free.size < thisBlock->free.size)
		heap->x48 = thisBlock;

	return thisBlock;	// must be free
}


/*------------------------------------------------------------------------------
	Search the free list for a block of at least the required size.
	Args:		inSize
	Return:	pointer to a heap block
------------------------------------------------------------------------------*/

Block *
SearchFreeList(Size inSize)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	if (heap->free >= inSize)
	{
		Block *	whereWeStarted = heap->x48;
		Block *	freeBlock = whereWeStarted;
		while ((freeBlock = (freeBlock->free.next != NULL) ? freeBlock->free.next : heap->freeList) != whereWeStarted)
		{
			if (freeBlock->free.size >= inSize)
			{
				heap->x48 = freeBlock;
				return freeBlock;
			}
		}
		if (heap->x5C == 2 || heap->x5C == 1 || heap->x4C != NULL)
			return FindSmallestSlide(NULL, inSize, 0);
	}
	return NULL;
}


/*------------------------------------------------------------------------------
	Set a free block’s prev/next free pointers.
	Args:		ioBlock			the block to update
				inPrevBlock
				inNextBlock
	Return:	--
------------------------------------------------------------------------------*/

void
SetFreeChain(Block * ioBlock, Block * inPrevBlock, Block * inNextBlock)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	// link to previous block
	ioBlock->free.prev = inPrevBlock;
	if (inPrevBlock != NULL)
		inPrevBlock->free.next = ioBlock;
	else
		heap->freeList = ioBlock;

	// link to next block
	ioBlock->free.next = inNextBlock;
	if (inNextBlock != NULL)
		inNextBlock->free.prev = ioBlock;
	else
		heap->freeListTail = ioBlock;
}


/*------------------------------------------------------------------------------
	Move a free block within the heap.
	Args:		inBlock
				inOffset
	Return:	--
------------------------------------------------------------------------------*/

void
MoveFreeBlock(Block * inBlock, long inOffset)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	bool		isx48Affected = (heap->x48 == inBlock);
	Block *	dstBlock = (Block *)((Ptr) inBlock + inOffset);
	dstBlock->free.size = inBlock->free.size - inOffset;
	SetFreeChain(dstBlock, inBlock->free.prev, inBlock->free.next);
	if (isx48Affected)
		heap->x48 = dstBlock;
}


/*------------------------------------------------------------------------------
	Remove a block from the free list.
	Args:		inBlock			MUST be free already
	Return:	--
------------------------------------------------------------------------------*/

void
RemoveFreeBlock(Block * inBlock)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	// remove block from forward chain
	if (inBlock->free.prev != NULL)
		inBlock->free.prev->free.next = inBlock->free.next;
	else
		heap->freeList = inBlock->free.next;

	// remove block from backward chain
	if (inBlock->free.next != NULL)
		inBlock->free.next->free.prev = inBlock->free.prev;
	else
		heap->freeListTail = inBlock->free.prev;

	if (heap->x48 == inBlock)
		heap->x48 = heap->freeList;

	((Block *)((Ptr) inBlock + inBlock->free.size))->flags &= ~0x04;
}


/*------------------------------------------------------------------------------
	Kill a block - return it to the heap.
	Args:		inPtr
	Return:	--
------------------------------------------------------------------------------*/

void
KillBlock(void * inPtr)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	Block *	block = ((Block *)inPtr) - 1;
	Size		blockSize = block->inuse.size;
	Block *	nextBlock = (Block *)((Ptr) block + blockSize);
	Block *	prevBlock = NULL;
	bool		r2 = (block->flags & 0x04) != 0;
	bool		r1 = (VAddr) nextBlock < heap->end;
	bool		nextBlockIsFree = r1 && (nextBlock->flags & 0x80) == 0;	// r0

	heap->x68 -= block->inuse.delta;
	heap->x6C -= TRUNC(block->inuse.delta, 4);
	if (!nextBlockIsFree)
	{
		if (r1)
			nextBlock->flags |= 0x04;
		for (nextBlock = heap->freeList; nextBlock != NULL && nextBlock < block; nextBlock = nextBlock->free.next)
			prevBlock = nextBlock;
	}
	heap->free += blockSize;
	if (r2)
	{
		if (nextBlockIsFree)
		{
			Size		nextBlockSize = nextBlock->free.size;
			prevBlock = nextBlock->free.prev;
			prevBlock->free.size += nextBlockSize;
			RemoveFreeBlock(nextBlock);
			nextBlock = (Block *)((Ptr) nextBlock + nextBlockSize);
			if ((VAddr) nextBlock < heap->end)
				nextBlock->flags |= 0x04;
		}
		prevBlock->free.size += blockSize;
		block = prevBlock;
	}
	else
	{
		block->inuse.owner = 0;			// sic
		block->free.size = blockSize;
		if (nextBlockIsFree)
		{
			SetFreeChain(block, nextBlock->free.prev, nextBlock->free.next);
			block->free.size += nextBlock->free.size;
		}
		else
			SetFreeChain(block, prevBlock, nextBlock);
	}
	if (heap->x48 == NULL || heap->x48 > block)
		heap->x48 = block;
}

#pragma mark -

/*------------------------------------------------------------------------------
	D i r e c t   B l o c k s

	Pointers are built on direct blocks.
------------------------------------------------------------------------------*/

Ptr
NewWeakBlock(Size	inSize)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	heap->x5C |= 0x02;
	return NewDirectBlock(inSize);
}


Ptr
NewDirectBlock(Size inSize)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	Block *	block = NewBlock(inSize);
	if (block != NULL)
	{
		block->inuse.flags = 0x80 | 0x01;
		block->inuse.link = GetCurrentHeap();
		block++;	// point to data beyond block header
	}
	heap->x5C = 0x00;
	return (Ptr) block;
}


void
DisposeDirectBlock(Ptr inBlock)
{
	KillBlock(inBlock);
}


Size
GetDirectBlockSize(Ptr inBlock)
{
	Block *	block = ((Block *)inBlock) - 1;
	return block->inuse.size - sizeof(Block) - block->inuse.delta;
}


Ptr
SetDirectBlockSize(Ptr inBlock, Size inSize)
{
	return SetBlockSize(inBlock, inSize);
}

#pragma mark -

/*------------------------------------------------------------------------------
	I n d i r e c t   B l o c k s

	Handles are built on indirect blocks.
------------------------------------------------------------------------------*/

Handle
NewIndirectBlock(Size inSize)
{
	SHeap *	heap = (SHeap *)GetCurrentHeap();

	Block *	freeBlock = heap->freeListTail;
	if (freeBlock != NULL && freeBlock->free.prev != NULL)
		CompactHeap(heap, NULL);

	MPPtr	mp = AllocateMasterPointer(heap);
	if (mp != NULL)
	{
		Block * block;
		if ((freeBlock = heap->freeListTail) != NULL)
			heap->x48 = freeBlock;
		if ((block = NewBlock(inSize)) != NULL)
		{
			mp->ptr = (Ptr) (block + 1);			// alloc'd mem is just past block header
			mp->next = (MPPtr) heap;				// override semantics of next - point back to heap
			block->inuse.flags = 0x80 | 0x02;	// block is indirect
			block->inuse.link = mp;					// override semantics of link - point back to master pointer
		}
		else
		{
			FreeMasterPointer(heap, mp);
			mp = NULL;
		}
	}
	return (Handle) mp;
}


void
DisposeIndirectBlock(Handle inBlock)
{
	if (!IsFakeIndirectBlock(inBlock))
		DisposeDirectBlock(((MPPtr) inBlock)->ptr);
	FreeMasterPointer(GetCurrentHeap(), (MPPtr) inBlock);
}


Size
GetIndirectBlockSize(Handle inBlock)
{
	if (IsFakeIndirectBlock(inBlock))
		return 0;

	return GetDirectBlockSize(((MPPtr) inBlock)->ptr);
}


Ptr
SetIndirectBlockSize(Handle inBlock, Size inSize)
{
	if (IsFakeIndirectBlock(inBlock))
		return NULL;

	Ptr	p = SetDirectBlockSize(((MPPtr) inBlock)->ptr, inSize);
	if (p != NULL)
		((MPPtr) inBlock)->ptr = p;
	return p;
}

#pragma mark -

/*------------------------------------------------------------------------------
	F a k e   I n d i r e c t   B l o c k   o p e r a t i o n s

	A fake block is like a master pointer, but the next link holds size and
	flags info.
------------------------------------------------------------------------------*/

Handle
NewFakeIndirectBlock(void * inAddr, Size inSize)
{
	MPPtr	mp = AllocateMasterPointer(GetCurrentHeap());
	if (mp != NULL)
	{
		((FakeBlock *)mp)->ptr = (Ptr) inAddr;
		((FakeBlock *)mp)->size = inSize;
		((FakeBlock *)mp)->flags = 0x01;
	}
	return (Handle) mp;
}


bool
IsFakeIndirectBlock(Handle inBlock)
{
	return (inBlock != NULL) && ((FakeBlock *)inBlock)->flags == 0x01;
}


Size
GetFakeIndirectBlockSize(Handle inBlock)
{
	return (inBlock != NULL) ? ((FakeBlock *)inBlock)->size : 0;
}

#pragma mark -

/*------------------------------------------------------------------------------
	A l l o c a t e M a s t e r P o i n t e r
------------------------------------------------------------------------------*/

MPPtr
AllocateMasterPointer(Heap inHeap)
{
	SHeap *	heap = (SHeap *)inHeap;
	MPPtr	mp;

	if ((mp = heap->freeMPList) == NULL)
		AllocateMoreMasters();
	if ((mp = heap->freeMPList) != NULL)
	{
		heap->freeMPCount--;
		heap->freeMPList = mp->next;
		mp->next = NULL;
	}
	return mp;
}


void
AllocateMoreMasters(void)
{
	SHeap * savedHeap = (SHeap *)GetCurrentHeap();
	SHeap * theHeap = savedHeap;
	if (theHeap != theHeap->masterPtrHeap)
	{
		theHeap = (SHeap *)theHeap->masterPtrHeap;
		SetCurrentHeap(theHeap);
	}
	Block *	mpBlock = NewBlock(theHeap->mpBlockSize * sizeof(MasterPtr));
	if (theHeap != savedHeap)
	{
		SetCurrentHeap(savedHeap);
		theHeap = savedHeap;
	}
	if (mpBlock != NULL)
	{
		MasterPtr *	mp = (MasterPtr *)((Block *)mpBlock + 1);
		MasterPtr *	link = NULL;
		theHeap->freeMPCount += theHeap->mpBlockSize;
		CreatePrivateBlock(mpBlock, 5);	// kMM_HeapMPBlock?
		for (int i = 0; i < theHeap->mpBlockSize; i++, mp++)
		{
			mp->ptr = (Ptr)0xC0000000;
			mp->next = link;
			link = mp;
		}
		if ((mp = theHeap->freeMPList) != NULL)
		{
			// there are already master pointers - add the new block to the chain
			while (mp->next != NULL)
				mp = mp->next;
			mp->next = link;
		}
		else
			theHeap->freeMPList = link;
	}
}


void
FreeMasterPointer(Heap inHeap, MasterPtr * inMP)
{
	SHeap *	heap = (SHeap *)inHeap;

	inMP->next = heap->relocHeap->freeMPList;
	inMP->ptr = (Ptr)0xC0000000;
	heap->relocHeap->freeMPList = inMP;
	heap->relocHeap->freeMPCount++;
}

#pragma mark -

/*------------------------------------------------------------------------------
	B l o c k   A c c e s s o r s
------------------------------------------------------------------------------*/

Size
GetBlockPhysicalSize(void * inPtr)
{
	Block *	block = ((Block *)inPtr) - 1;
	return (block->flags & 0x80) != 0 ? block->inuse.size : block->free.size;
}

Heap
GetBlockParent(void * inPtr)
{
	Block *	block = ((Block *)inPtr) - 1;
	return block->inuse.link;
}

unsigned char
GetBlockType(void * inPtr)
{
	Block *	block = ((Block *)inPtr) - 1;
	return block->inuse.type;
}

unsigned char
GetBlockBusy(void * inPtr)
{
	Block *	block = ((Block *)inPtr) - 1;
	return block->inuse.busy;
}

void
SetBlockBusy(void * inPtr, unsigned char inBusyState)
{
	Block *	block = ((Block *)inPtr) - 1;
	block->inuse.busy = inBusyState;
}

void
IncrementBlockBusy(void * inPtr)
{
	Block *	block = ((Block *)inPtr) - 1;
	block->inuse.busy++;
}

void
DecrementBlockBusy(void * inPtr)
{
	Block *	block = ((Block *)inPtr) - 1;
	block->inuse.busy--;
}

unsigned char
GetBlockDelta(void * inPtr)
{
	Block *	block = ((Block *)inPtr) - 1;
	return block->inuse.delta;
}

unsigned char
GetBlockFlags(void * inPtr)
{
	Block *	block = ((Block *)inPtr) - 1;
	return block->inuse.flags;
}

#pragma mark -

/*------------------------------------------------------------------------------
	E r r o r s
------------------------------------------------------------------------------*/
#if !usesMacMemMgr
NewtonErr
MemError(void)
{
	return TaskSwitchedGlobals()->fMemErr;
}
#endif
#pragma mark -

/*------------------------------------------------------------------------------
	D e b u g   E r r o r   R e p o r t i n g
------------------------------------------------------------------------------*/

void
ReportSmashedHeap(const char * inDoingWhat, NewtonErr inErr, void * inWhere)
{
#if defined(correct)
	char report[200];
	sprintf(report,
	// that’s really all it does -- maybe destined for DebugStr() in debug build only?
#else
	printf(
#endif
			"Smashed heap (err %d) at &%p %s MemMgr", inErr, inWhere, inDoingWhat);
}


void
ReportMemMgrTrap(ULong inName)
{
	char c1, c2, c3, c4;
	c1 = inName >> 24;	if (c1 == 0) c1 = ' ';
	c2 = inName >> 16;	if (c2 == 0) c2 = ' ';
	c3 = inName >>  8;	if (c3 == 0) c2 = ' ';
	c4 = inName;			if (c4 == 0) c4 = ' ';
#if defined(correct)
	char report[200];
	sprintf(report,
	// that’s really all it does -- maybe destined for DebugStr() in debug build only?
#else
	printf(
#endif
			"MemMgr trigger '%c%c%c%c'", c1, c2, c3, c4);
}


ULong
HashCallChain(void)
{
//	we don’t hash the function call chain - it would rely on compiler idiom & assembler
	return 0;
}

#pragma mark -

/*------------------------------------------------------------------------------
	G e n e r a l   o p e r a t i o n s
------------------------------------------------------------------------------*/

bool
EqualBytes(const void * src1, const void * src2, Size length)
{
	const char * src1Ptr = (const char *)src1;
	const char * src2Ptr = (const char *)src2;

	while (MISALIGNED(src1Ptr) && length-- > 0)
		if (*src1Ptr++ != *src2Ptr++)
			return NO;
	Size postLength = length & 0x03;
	for (length /= 4; length > 0; length--, src1Ptr += sizeof(long), src2Ptr += sizeof(long))
		if (*(ULong*)src1Ptr != *(ULong*)src2Ptr)
			return NO;
	while (postLength-- > 0)
		if (*src1Ptr++ != *src2Ptr++)
			return NO;

	return YES;
}


void
XORBytes(const void * src1, const void * src2, void * dest, Size length)
{
	// Optimize if all pointers are long aligned
	if (!MISALIGNED(src1)
	 && !MISALIGNED(src2)
	 && !MISALIGNED(dest))
	{
		ULong * src1Ptr = (ULong *)src1;
		ULong * src2Ptr = (ULong *)src2;
		ULong * destPtr = (ULong *)dest;
		for ( ; length >= 32; length -= 32)
		{
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
		}
		for ( ; length >= 4; length -= 4)
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
		if (length-- > 0)
		{
			char * src1ChPtr = (char *)src1Ptr;
			char * src2ChPtr = (char *)src2Ptr;
			char * destChPtr = (char *)destPtr;
			while (length-- > 0)
				*destChPtr++ = *src1ChPtr++ ^ *src2ChPtr++;
		}
	}
	else
	{
		char * src1Ptr = (char *)src1;
		char * src2Ptr = (char *)src2;
		char * destPtr = (char *)dest;
		for ( ; length >= 8; length -= 8)
		{
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
		}
		while (length--)
			*destPtr++ = *src1Ptr++ ^ *src2Ptr++;
	}
}


void
ClearMemory(Ptr p, Size length)
{
	FillBytes(p, length, 0);
}


void
ZeroBytes(void * addr, Size length)
{
	FillLongs(addr, length, 0);
}


void
FillBytes(void * addr, Size length, UChar pattern)
{
	ULong longPattern = (pattern << 24) | (pattern << 16) | (pattern << 8) | pattern;
	FillLongs(addr, length, longPattern);
}


void
FillLongs(void * addr, Size length, ULong pattern)
{
	UChar   byteValue[4];
	UChar * byteAddr, * p;
	ULong * destPtr = (ULong *)addr;

	*(ULong *)byteValue = pattern;
	if (MISALIGNED(addr))	// long align start address
	{
		p = (UChar *)destPtr;
		for (byteAddr = byteValue + ((long) addr & 0x03); MISALIGNED(destPtr) && length != 0; length--)
			*p++ = *byteAddr++;
		destPtr = (ULong *)p;
	}

	Size postLength = length & 0x03;
	for (length /= 4; length >= 8; length -= 8)
	{
		*destPtr++ = pattern;
		*destPtr++ = pattern;
		*destPtr++ = pattern;
		*destPtr++ = pattern;
		*destPtr++ = pattern;
		*destPtr++ = pattern;
		*destPtr++ = pattern;
		*destPtr++ = pattern;
	}
	for ( ; length > 0; length--)
		*destPtr++ = pattern;

	p = (UChar *)destPtr;
	for (byteAddr = byteValue; postLength > 0; postLength--)
		*p++ = *byteAddr++;
}
