/*
	File:		NArray.h

	Contains:	YAAI (Yet Another Array Implementation).
					Seems to be a simplified version of CDynamicArray.

	Written by:	Newton Research Group, 2009.
*/

#if !defined(__NARRAY_H)
#define __NARRAY_H 1

#include "Newton.h"

/*------------------------------------------------------------------------------
	N C o m p a r a t o r
------------------------------------------------------------------------------*/

class NComparator
{
public:
					NComparator();
	virtual		~NComparator();

	virtual const void *	keyOf(const void * inData) const;
	virtual int		compareKeys(const void * inKey1, const void * inKey2) const;
};


/*------------------------------------------------------------------------------
	N B l o c k C o m p a r a t o r
------------------------------------------------------------------------------*/

class NBlockComparator : public NComparator
{
public:
					NBlockComparator(ArrayIndex inBlockSize);
					~NBlockComparator();

	int			compareKeys(const void * inKey1, const void * inKey2) const;

private:
	ArrayIndex	fBlockSize;
};


/*------------------------------------------------------------------------------
	N I t e r a t o r
------------------------------------------------------------------------------*/

class NIterator
{
public:
	void			insertElements(ArrayIndex index, ArrayIndex inCount);
	void			removeElements(ArrayIndex index, ArrayIndex inCount);
	void			deleteArray(void);
};


/*------------------------------------------------------------------------------
	N A r r a y
------------------------------------------------------------------------------*/

class NArray
{
public:
					NArray();
	virtual		~NArray();

	NewtonErr	init(ArrayIndex inElementSize, ArrayIndex inChunkSize, ArrayIndex inCount, bool inAllowShrinkage);
	NewtonErr	insertElements(ArrayIndex index, ArrayIndex inCount, const void * inData);
	NewtonErr	removeElements(ArrayIndex index, ArrayIndex inCount);
	NewtonErr	setCount(ArrayIndex inCount);
	NewtonErr	setPhysicalCount(ArrayIndex inCount);
	ArrayIndex	count(void);

	virtual void *			at(ArrayIndex index) const;
	virtual ArrayIndex	contains(const void * inElement) const;
	virtual ArrayIndex	where(const void * inElement) const;

protected:
	ArrayIndex	fCount;				// +04	logical size
	ArrayIndex	fAllocatedCount;	// +08	allocated size
	ArrayIndex	fElementSize;		// +0C	element size
	ArrayIndex	fChunkSize;			// +10	number of elements to increase storage by
	void *		fArrayStorage;		// +14	array elements
	NIterator *	fIter;				// +18
	bool			fAllowShrinkage;	// +1C	allow storage to shrink when required
};

inline ArrayIndex NArray::count(void) { return fCount; }

#define kDefaultElementSize 4
#define kDefaultChunkSize 4


/*------------------------------------------------------------------------------
	N S o r t e d A r r a y
------------------------------------------------------------------------------*/

class NSortedArray : public NArray
{
public:
					NSortedArray();
					~NSortedArray();

	NewtonErr	init(NComparator * inComparator, ArrayIndex inElementSize, ArrayIndex inChunkSize, ArrayIndex inCount, bool inAllowShrinkage);

	ArrayIndex	contains(const void * inElement) const;
	ArrayIndex	where(const void * inElement) const;

private:
	NComparator *	fComparator;	// +20
};

#endif	/* __NARRAY_H */
