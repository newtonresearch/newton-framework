/*
	File:		ObjectHeap.h

	Contains:	Newton object heap interface.

	Written by:	Newton Research Group.
*/

#if !defined(__OBJECTHEAP__)
#define __OBJECTHEAP__ 1

#include "ObjHeader.h"

extern "C" {
bool	InHeap(Ref ref);
bool	OnStack(const void * obj);
void	SetObjectHeapSize(size_t inSize, bool allocateInTempMemory);
}

/*----------------------------------------------------------------------
	O b j e c t H e a p
----------------------------------------------------------------------*/

class CDeclawingRange;
class CObjectHeap
{
public:
					CObjectHeap(size_t inSize, bool allocateInTempMemory = true);
					~CObjectHeap();

	ObjHeader *	allocateBlock(size_t inSize, unsigned char flags);
	Ref			allocateObject(size_t inSize, unsigned char flags);
	Ref			allocateBinary(RefArg inClass, size_t length);
	Ref			allocateIndirectBinary(RefArg inClass, size_t length);
	Ref			allocateArray(RefArg inClass, ArrayIndex length);
	Ref			allocateFrame(void);
	Ref			allocateFrameWithMap(RefArg inMap);
	Ref			allocateMap(RefArg inSuperMap, ArrayIndex length);

	void			makeFreeBlock(ObjHeader * obj, size_t newSize);

	size_t		coalesceFreeBlocks(ObjHeader * obj, size_t inSize);
	ObjHeader *	findFreeBlock(size_t inSize);
	void			splitBlock(ObjHeader * obj, size_t newSize);
	ObjHeader *	resizeBlock(ObjHeader * obj, size_t newSize);
	void			resizeObject(RefArg ref, size_t newSize);

	void			unsafeSetArrayLength(RefArg ref, ArrayIndex newLength);
	void			unsafeSetBinaryLength(RefArg ref, ArrayIndex newLength);

	bool			inHeap(Ref ref);
	Ref			clone(RefArg ref);
	void			replace(Ref, Ref);
	Ref			update(Ref);

	bool			registerRangeForDeclawing(ULong start, ULong end);
	void			declawRefsInRegisteredRanges(void);

	void			GC(void);
	void			GCTWA(void);
	void			cleanUpWeakChain(void);
	void			mark(Ref);
	void			sweepAndCompact(void);

	RefHandle *	allocateRefHandle(Ref inRef);
	void			disposeRefHandle(RefHandle * inRefHandle);
	RefHandle *	expandObjectTable(RefHandle * inRefHandle);
	void			clearRefHandles(void);

// For debug
	void			heapBounds(Ptr * outStart, Ptr * outLimit);
	void			heapStatistics(size_t * outFree, size_t * outLargest);
	void			uriah(void);
	void			uriahBinaryObjects(bool doFile = false);

private:
friend void				FixStartupHeap(void);

#define kNumOfHandlesInBlock 256
#define kIncrHandlesInBlock   32

	struct RefHandleBlock
	{
		OBJHEADER
		RefHandle	data[kNumOfHandlesInBlock];
	}__attribute__((packed));

	char *				mem;				// +04	allocated memory for heap
	ObjHeader *			heapBase;		// +08	long-aligned base of heap
	ObjHeader *			heapLimit;		// +0C	long-aligned end of heap
	ObjHeader *			freeHeap;		// +10	currently free heap object
	RefHandleBlock *	refHBlock;		// x14	ref handles -- at top of heap
	ArrayIndex			refHIndex;		// x18	index of next free ref handle
	Ref					objRoot;			// +1C	object being resized -- needs GC
	size_t				refHBlockSize;	// +20	size of ref handle block -- will grow as necessary
	SlottedObject *	weakLink;		// +24
	bool					inGC;				// +28
	CDeclawingRange *	declaw;			// +2C
	bool					inDeclaw;		// +30
};


extern CObjectHeap *	gHeap;
extern int				gCurrentStackPos;


#endif	/* __OBJECTHEAP__ */
