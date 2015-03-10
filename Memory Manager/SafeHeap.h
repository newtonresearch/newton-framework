/*
	File:		SafeHeap.h

	Contains:	Safe heap interface.

	Written by:	Newton Research Group.
*/

#if !defined(__SAFEHEAP_H)
#define __SAFEHEAP_H 1

#include "KernelPhys.h"

class CULockingSemaphore;

/*------------------------------------------------------------------------------
	S S a f e H e a p B l o c k
------------------------------------------------------------------------------*/

struct SSafeHeapBlock
{
	SSafeHeapBlock *	next(void);

	unsigned		fDelta : 8;	// diff between allocated size (below) and required size
									// 0xFF => block is free
	unsigned		fSize : 24;	// allocated size
};

extern Size		SafeHeapBlockSize(void * inPtr);


/*------------------------------------------------------------------------------
	S S a f e H e a p P a g e
------------------------------------------------------------------------------*/
struct SWiredHeapDescr;

struct SSafeHeapPage
{
	void					init(ULong, CPhys * inPhys, SSafeHeapPage * inLink);
	void *				alloc(long inAllocSize, long inActualSize);
	void					free(void * inPtr);
	SSafeHeapPage *	firstPage(void);

	virtual void				freePage(void);
	virtual SSafeHeapPage *	getPage(void);

//private:
//	vt
	SSafeHeapPage *		fNext;		// +04	linked list
	SSafeHeapPage *		fPrev;		// +08
	ULong						fTag;			// +0C	'safe' to distinguish from ordinary Heap
	SSafeHeapPage *		fLastPage;	// +10	linked list tail
	SSafeHeapBlock *		fFreeBlock;	// +14	largest free block
	long						fFree;		// +18	free space remaining in this page
	ULong						f1C;
	CLittlePhys *			fPhys;		// +20	physical page
	CULockingSemaphore *	fSem;			// +24

	SWiredHeapDescr *		fDescr;		// +28	possibly refcon -
};												//			should SSafeHeapPage know about SWiredHeapDescr?


/*------------------------------------------------------------------------------
	S W i r e d H e a p D e s c r
------------------------------------------------------------------------------*/
struct SWiredHeapPage;

struct SWiredHeapDescr
{
	NewtonErr			growByOnePage(void);
	NewtonErr			shrinkByOnePage(void);

	VAddr					fStart;	// +00
	VAddr					fEnd;		// +04
	SWiredHeapPage *	fHeap;	// +08	wired heap page backing
	Size					fSize;	// +0C	always page aligned
};


/*------------------------------------------------------------------------------
	S W i r e d H e a p P a g e
------------------------------------------------------------------------------*/

struct SWiredHeapPage : public SSafeHeapPage
{
	static SWiredHeapPage *	make(SWiredHeapDescr * inDescr);	// was new
	NewtonErr					destroy(void);

	virtual void				freePage(void);
	virtual SSafeHeapPage *	getPage(void);
};


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

inline bool					IsSafeHeap(void * inHeap) { return ((SSafeHeapPage*)inHeap)->fTag == 'safe'; }
inline CULockingSemaphore *	GetSafeHeapSemaphore(void * inHeap) { return ((SSafeHeapPage*)inHeap)->fSem; }
inline void							SetSafeHeapSemaphore(void * inHeap, CULockingSemaphore * inSem) { ((SSafeHeapPage*)inHeap)->fSem = inSem; }

extern NewtonErr			InitSafeHeap(SSafeHeapPage ** outHeap);
extern bool					SafeHeapIsEmpty(SSafeHeapPage * inHeap);
extern void *				SafeHeapAlloc(long inSize, SSafeHeapPage * inHeap);
extern void *				SafeHeapRealloc(void * inPtr, Size inSize);
extern void					SafeHeapFree(void * inPtr);
extern SSafeHeapPage **	SafeHeapEndSentinelFor(void * inPtr);

extern NewtonErr			GetNewPageFromPageMgr(void ** outPage, ObjectId * outId, CPhys ** outPhys);
extern void					AddPartialPageToSafeHeap(void * inPartialPage, SSafeHeapPage * inSafeHeap);

#endif	/* __SAFEHEAP_H */
