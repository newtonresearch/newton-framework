/*
	File:		NewtonMemory.h

	Contains:	Memory interface for Newton build system.

	Written by:	Newton Research Group.
*/

#if !defined(__NEWTONMEMORY_H)
#define __NEWTONMEMORY_H 1
// DONÕT USE MAC MEMORY FUNCTIONS
#define __MACMEMORY__

#if !defined(__NEWTONTYPES_H)
#include "NewtonTypes.h"
#endif

#if defined(__cplusplus)
#define DEFAULT_NIL = 0
extern "C" {
#else
#define DEFAULT_NIL
#endif


// this sucks, but we may already have included the Carbon framework which defines Size as signed
typedef long Size;

typedef unsigned char HeapBlockType;

/*
**	Opaque memory manager types
**
*/
typedef void *	Heap;


/*	----------------------------------------------------------------
**
**	General operations
**
*/
NewtonErr	MemError(void);	/* per-task result of last memory mgr call */


/*	----------------------------------------------------------------
**
**	Ptr operations
**
*/
Ptr			NewPtr(Size inSize);
Ptr			NewNamedPtr(Size inSize, ULong inName);
Ptr			NewPtrClear(Size inSize);
void			FreePtr(Ptr inPtr);					// was DisposPtr
Ptr			ReallocPtr(Ptr inPtr, Size inSize);
Size			GetPtrSize(Ptr inPtr);
Heap			PtrToHeap(Ptr inPtr);


/*	----------------------------------------------------------------
**
**	"Locking" a Ptr has the following semantics:
**
**		o	The PtrÕs guts are accessible to interrupt code.  (Since
**			VM-heaps are possibly backed by compression or some actual
**			storage device, it is unsafe to touch memory from interrupt
**			code without locking it);
**
**			Note that handles cannot be locked.
**
**		o	The Ptr is NOT guaranteed to be physically contiguous (it
**			may cross a page boundary).  It is unsafe to DMA into or
**			out of it.  Use NewWiredPtr if you want to use DMA;
**
**		o	The Ptr is cacheable (or not), as determined by the heap
**			creation parameters;
**
**		o	If you lock a Ptr, you are responsible for unlocking it
**			before you free it;
**
*/
NewtonErr	LockPtr(Ptr inPtr);
NewtonErr	UnlockPtr(Ptr inPtr);


/*	----------------------------------------------------------------
**
**	Wired Ptrs
**
**	These are for use by DMA; a wired pointer block is g'teed to
**	be contiguous in RAM, and will have a fixed physical address
**	for its lifetime.
**
**	Limitations are:
**
**		o	Size limit of a smidgeon less than 4K (assume 32 bytes
**			less than 4K and you won't be disappointed);
**
**		o	The operations are limited to allocate/deallocate in
**			order to keep the implementation flexible (e.g. you
**			must keep track of size of wired ptrs yourself);
**
**		Use wired pointers with care; they gum up the allocation of
**		pages.
**
*/
Ptr			NewWiredPtr(Size inSize);
void			FreeWiredPtr(Ptr inPtr);			// was DisposeWiredPtr


/*	----------------------------------------------------------------
**
**	Handle operations
**
**		Note that HLock returns a pointer (which you should use).
**
*/
Handle		NewHandle(Size inSize);
Handle		NewNamedHandle(Size inSize, ULong inName);
Handle		NewHandleClear(Size inSize);
void			FreeHandle(Handle inHandle);		// was DisposHandle
Size			GetHandleSize(Handle inHandle);
NewtonErr	SetHandleSize(Handle inHandle, Size inSize);
Ptr			HLock(Handle inHandle);
void			HUnlock(Handle inHandle);
NewtonErr	HandToHand(Handle * ioHandle);
Handle		CopyHandle(Handle inHandle);
Heap			HandleToHeap(Handle inHandle);


/*	----------------------------------------------------------------
**
**	Fake handles (e.g. handles to things in ROM)
**
**		DisposeHandle is allowed (destroys the handle);
**		GetHandleSize returns specified size
**		SetHandleSize returns -1
**		HLock is a NOP (returns address)
**		HUnlock is a NOP
**		HandToHand, CopyHandle will result in correct copies
**
*/
Handle		NewFakeHandle(void * inPtr, Size inSize);
bool			IsFakeHandle(Handle inHandle);


/*	----------------------------------------------------------------
**
**	Tagging and naming heap blocks
**
**		Names and tags are mutually exclusive.  If you assign a name
**		to a block in a shared heap, that block will not be automatically
**		freed when the owner task dies.
**
*/
ObjectId		GetPtrOwner(Ptr inPtr);
void			SetPtrOwner(Ptr inPtr, ObjectId inOwner);
ObjectId		GetHandleOwner(Handle inHandle);
void			SetHandleOwner(Handle inHandle, ObjectId inOwner);


/*	----------------------------------------------------------------
**
**	Naming heap blocks
**
*/
ULong			GetPtrName(Ptr inPtr);
void			SetPtrName(Ptr inPtr, ULong inName);
ULong			GetHandleName(Handle inHandle);
void			SetHandleName(Handle inHandle, ULong inName);


/*	----------------------------------------------------------------
**
**	Typing heap blocks
**
*/
HeapBlockType	GetPtrType(Ptr inPtr);
void				SetPtrType(Ptr inPtr, HeapBlockType inType);
HeapBlockType	GetHandleType(Handle inHandle);
void				SetHandleType(Handle inHandle, HeapBlockType inType);


/*	----------------------------------------------------------------
**
**	Heap Operations
**
*/
Heap			GetHeap(void);			// get the taskÕs current heap
void			SetHeap(Heap);			// set the taskÕs current heap

NewtonErr	NewHeapAt(
					VAddr		inAddr,			// where heap is in VM
					Size		inSize,			// size of heap
					Heap *	outHeap);		// new heap

NewtonErr	NewVMHeap(
					ObjectId	inDomain,		// domain for heap (or zero for current envÕs default)
					Size		inMaxSize,		// max size of both Ptr and Handle allocation
					Heap *	outHeap,			// new heap
					ULong		inOptions);		// various options (see below)

NewtonErr	NewSegregatedVMHeap(
					ObjectId	inDomain,		// domain for heap (or zero for current envÕs default)
					Size		inPtrSize,		// max size of Ptr allocation
					Size		inHandleSize,	// max size of Handle allocation
					Heap *	outHeap,			// new heap
					ULong		inOptions);		// various options (see below)

NewtonErr 	NewPersistentVMHeap(
					ObjectId	inDomain,		// domain (zero => default for caller's environment)
					Size		inMaxSize,		// max storage (not TOO huge, since there's static overhead)
					Heap *	outHeap,			// the heap
					ULong		inOptions,		// various options
					ULong		inName);			// name for persistent heap

NewtonErr 	DeletePersistentVMHeap(
					ULong		inName);			// name for persistent heap

NewtonErr	ResurrectVMHeap(Heap inHeap);							// bring a heap at some VAddr back to life after a reboot
NewtonErr	DestroyVMHeap(Heap inHeap DEFAULT_NIL);			// clobber a heap
NewtonErr	ShrinkHeapLeaving(Heap inHeap, Size amountLeftFree);	// attempt to shrink a heap
NewtonErr	AddSemaphoreToHeap(Heap inHeap DEFAULT_NIL);		// attach a semaphore to a heap
NewtonErr	ClobberHeapSemaphore(Heap inHeap DEFAULT_NIL);	// clobber a heap's semaphore (careful!)
NewtonErr	ZapHeap(Heap inHeap, ULong inVerification, bool inRecoverable);	// clobber the specified heap
#define		kZapHeapVerification	'-><-'

Size			TotalFreeInHeap(Heap inHeap DEFAULT_NIL);
Size			LargestFreeInHeap(Heap inHeap DEFAULT_NIL);
Size			CountFreeBlocks(Heap inHeap DEFAULT_NIL);
Size			TotalUsedInHeap(Heap inHeap DEFAULT_NIL);
Size			MaxHeapSize(Heap inHeap DEFAULT_NIL);
Size			GetHeapReleaseable(Heap inHeap DEFAULT_NIL);


/*	----------------------------------------------------------------
**
**	Options for heap creation
**
*/
enum
{
	kHeapNotCacheable	= 0x00000001,		// don't cache heap
	kPersistentHeap	= 0x40000000		// heap is recoverable
};


/*	----------------------------------------------------------------
**
**	System memory availability information
**
**		TotalSystemFree
**
**			The number of bytes currently available for allocation.  This does
**			not include internal (e.g. 1-4K) fragmentation in heaps and stacks.
**
**		MaxPossibleSystemFree
**
**			An empirically determined number that is the maximum number of bytes
**			available for allocation before the user has stored anything.
**
**		SystemRAMSize
**
**			e.g. 512K for original MessagePad
**
*/
Size			TotalSystemFree(void);
Size			SystemRAMSize(void);


/*	----------------------------------------------------------------
**
**	Lightweight heap checking
**
**	 (This will always be available in the product -- it doesn't cost
**	  any RAM, and is fairly small codewise, and its utility is high...).
**
*/
NewtonErr	CheckHeap(
					Heap		inHeap,				// heap to check (NULL for current heap)
					void **	outWhereSmashed);	// if non-NULL and heap is smashed, address of accident


/*	----------------------------------------------------------------
**
**	Memory operations
**
**		If you are in doubt as to what to call to shuffle large
**		amounts of memory around, use memmove.
**
*/
bool		EqualBytes(const void * inSrc1, const void * inSrc2, Size inSize);
void		XORBytes(const void * inSrc1, const void * inSrc2, void * inDest, Size inSize);
void		ZeroBytes(void * inPtr, Size inSize);
void		FillBytes(void * inPtr, Size inSize, UChar inPattern);
void		FillLongs(void * inPtr, Size inSize, ULong inPattern);


/*
**	Task-safe heap walking
**
**		Seed-based.  Ask the heap for a seed, then pass the seed in on
**		every operation.  If the seed changes out from under you, get
**		another seed and start over.
**
**		Returns block type.
**
**		Returns 'kMM_HeapEndBlock' at the end of the heap.
**
*/
long		HeapSeed(Heap inHeap);
int		NextHeapBlock(
				Heap			inHeap,					// heap to walk
				long			inSeed,					// permission to walk heap
				void *		inFromBlock,			// from block, NULL for first block
				void **		outFoundBlock,			// next block
				void ***		outFoundBlockHandle,	// block's handle, if any
				int *			outFoundBlockType,	// block's type (see below)
				char *		outFoundBlockTag,		// block's tag
				Size *		outFoundBlockSize,	// block's size (apparent size, not true size)
				ObjectId *	outFoundBlockOwner);	// block's owner

/*
**	Block types returned by NextHeapBlock
**
*/
enum
{
	kMM_HeapSeedFailure,		// wups -- try again!
	kMM_InternalBlock,		// internal block
	kMM_HeapHeaderBlock,		// heap header
	kMM_HeapPtrBlock,			// Ptr block
	kMM_HeapHandleBlock,		// Handle block (result of NextHeapBlock is the handle)
	kMM_HeapEndBlock,			// end-of-heap
	kMM_HeapMPBlock,			// master-pointer block
	kMM_HeapFreeBlock			// free block
};


/*	----------------------------------------------------------------
**
**	CountHeapBlocks  --  Count blocks in a heap (of various types)
**
*/
void		CountHeapBlocks(
				Size *	outTotalSize,		// size of all matching blocks
				Size *	outFoundCount,		// number of matching blocks
				Heap		inHeap,				// heap to visit (NULL for current heap)
				int		inBlockType,		// block type filter (zero, kMM_HeapPtrBlock or kMM_HeapHandleBlock)
				ULong		inName,				// name to match (or zero)
				ULong		inNameMask);		// mask of name (or zero)


/*	----------------------------------------------------------------
**
**	Heap Refcons and bases
**
*/
void *	GetHeapRefCon(Heap inHeap DEFAULT_NIL);						/* snarf refcon */
void		SetHeapRefCon(void * inRefCon, Heap inHeap DEFAULT_NIL);	/* set refcon */
Heap		VoidStarToHeap(void * inVoidStar);		/* convert base addr to Heap */
void *	HeaptoVoidStar(Heap inHeap);				/* convert Heap to base addr */


/*	----------------------------------------------------------------
**
**	SetMemMgrBreak  --  Set a memory-manager break condition
**
**		This is a little bit like a logic analyzer hooked up to the
**		memory manager.  When one of the conditions you set up here
**		triggers, a DebugStr indicating the condition is hit.
**
**		SetMemMgrBreak(0);						// reset all breaks
**		SetMemMgrBreak('tag!');					// begin tagging allocated blocks with call-chain hashes
**		SetMemMgrBreak('tag?', hash_value);	// break on allocate when call-chain hash matches
**		SetMemMgrBreak('<', size);				// break on requests/resizes less than 'size'
**		SetMemMgrBreak('>=', size);			// break on requests/resizes >= 'size'
**		SetMemMgrBreak('#', Nth );				// break on Nth memory manager call
**		SetMemMgrBreak('&', address);			// break when address is passed into the memory manager
**		SetMemMgrBreak('&out', address);		// break when address is returned from the memory manager
**
*/
void	SetMemMgrBreak(ULong inWhat, ... );


#ifdef forDebug
/*	----------------------------------------------------------------
**
**	Heap operation tracing, heap tracking
**
*/
extern bool gMemTraceOn;

void		MemTraceInit(ULong inMaxNum);
void		MemTraceOn(bool inIsOn);
ULong		MemTraceCaptureStart(int op, ULong p1, ULong p2 DEFAULT_NIL);
void		MemTraceCaptureEnd(ULong, int markNested);
void		MemTraceDump(HammerIORef ref);
Heap		RememberedHeapAt(int n);	/* return a heap we've (probably) remembered */

/*	----------------------------------------------------------------
**
**	Randomly return NULL on allocations
**
*/
void	SetRandomNilReturns(
			Heap	inHeap,			// heap to return NULL on (NULL for current heap)
			ULong	inFreq,			// frequency in 256-based percent zero => never, 256 => always
			ULong	inDelay,			// number of allocations to delay before turning on
			ULong	inDuration);	// number of allocations to run before turning off
#endif	/* forDebug */

#if defined(__cplusplus)
}
#endif

#endif	/* __NEWTONMEMORY_H */
