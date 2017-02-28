/*
	File:		NArray.cc

	Contains:	YAAI (Yet Another Array Implementation).
					Seems to be a simplified version of CDynamicArray.

	Written by:	Newton Research Group, 2009.
*/

#include "NArray.h"

/*------------------------------------------------------------------------------
	N C o m p a r a t o r
------------------------------------------------------------------------------*/

NComparator::NComparator()
{ }

NComparator::~NComparator()
{ }

const void *
NComparator::keyOf(const void * inData) const
{
	return inData;
}

int
NComparator::compareKeys(const void * inKey1, const void * inKey2) const
{
	if (inKey1 < inKey2)
		return -1;
	if (inKey1 > inKey2)
		return 1;
	return 0;
}


#pragma mark -

/*------------------------------------------------------------------------------
	N B l o c k C o m p a r a t o r
------------------------------------------------------------------------------*/

NBlockComparator::NBlockComparator(ArrayIndex inBlockSize)
{
	fBlockSize = inBlockSize;
}

NBlockComparator::~NBlockComparator()
{ }

int
NBlockComparator::compareKeys(const void * inKey1, const void * inKey2) const
{
	return memcmp(inKey1, inKey2, fBlockSize);
}


#pragma mark -

/*------------------------------------------------------------------------------
	N I t e r a t o r
	Implemented in the ROM, but doesn’t appear to be used.
------------------------------------------------------------------------------*/

void
NIterator::insertElements(ArrayIndex index, ArrayIndex inCount)
{}

void
NIterator::removeElements(ArrayIndex index, ArrayIndex inCount)
{}

void
NIterator::deleteArray(void)
{}


#pragma mark -

/*------------------------------------------------------------------------------
	N A r r a y
------------------------------------------------------------------------------*/

NArray::NArray()
{
	fElementSize = kDefaultElementSize;
	fChunkSize = kDefaultChunkSize;
	fCount = 0;
	fAllocatedCount = 0;
	fArrayStorage = NULL;	
	fIter = NULL;
	fAllowShrinkage = true;
}


NArray::~NArray()
{
	if (fIter)
		fIter->deleteArray();
	if (fArrayStorage)
		free(fArrayStorage);
}


NewtonErr
NArray::init(ArrayIndex inElementSize, ArrayIndex inChunkSize, ArrayIndex inCount, bool inAllowShrinkage)
{
	NewtonErr err;
	XTRY
	{
		XFAILIF((inElementSize == 0 || inChunkSize == 0 || inCount == 0), err = kUCErrRangeCheck;)	// original checks ivars!
		fElementSize = inElementSize;
		fChunkSize = inChunkSize;
		fAllowShrinkage = inAllowShrinkage;
		err = setPhysicalCount(inCount);
	}
	XENDTRY;
	return err;
}


NewtonErr
NArray::insertElements(ArrayIndex index, ArrayIndex inCount, const void * inData)
{
	if (inCount == 0)
		return noErr;

	NewtonErr err;
	XTRY
	{
		XFAILIF((index == kIndexNotFound), err = kUCErrRangeCheck;)
		if (index > fCount)
			index = fCount;
		XFAIL(err = setPhysicalCount(fCount + inCount))	// might need to expand the array
		char * blockStart = (char *)fArrayStorage + fElementSize * index;
		char * blockEnd = (char *)fArrayStorage + fElementSize * (index + inCount);
		char * arrayEnd = (char *)fArrayStorage + fElementSize * fCount;
		if (fCount > index)
			memmove(blockEnd, blockStart, arrayEnd - blockStart);		// open up a gap
		memcpy(blockStart, inData, fElementSize * inCount);			// copy the data into it
		fCount += inCount;
		if (fIter)
			fIter->insertElements(index, inCount);
	}
	XENDTRY;
	return err;
}


NewtonErr
NArray::removeElements(ArrayIndex index, ArrayIndex inCount)
{
	if (inCount == 0)
		return noErr;

	NewtonErr err;
	XTRY
	{
		XFAILIF((index == kIndexNotFound || index >= fCount || index + inCount >= fCount), err = kUCErrRangeCheck;)
		char * blockStart = (char *)fArrayStorage + fElementSize * index;
		char * blockEnd = (char *)fArrayStorage + fElementSize * (index + inCount);
		char * arrayEnd = (char *)fArrayStorage + fElementSize * fCount;
		if (blockEnd < arrayEnd)
			memmove(blockStart, blockEnd, arrayEnd - blockEnd);	// close up the gap
		XFAIL(err = setCount(fCount - inCount))
		if (fIter)
			fIter->removeElements(index, inCount);
	}
	XENDTRY;
	return err;
}


NewtonErr
NArray::setCount(ArrayIndex inCount)
{
	NewtonErr err = noErr;
	if (fCount != inCount
	&&  (err = setPhysicalCount(inCount)) == noErr)
		fCount = inCount;
	return err;
}


NewtonErr
NArray::setPhysicalCount(ArrayIndex inCount)
{
	NewtonErr err = noErr;

	if (inCount == 0 && fAllowShrinkage)
	{
		free(fArrayStorage);
		err = MemError();
		fArrayStorage = NULL;
		fAllocatedCount = 0;
	}

	else if (fAllocatedCount < inCount		// need to expand
		  || (fAllowShrinkage && (fAllocatedCount - inCount) >= fChunkSize))	// need to shrink
	{
		ArrayIndex newCount = inCount + (fChunkSize - 1);
		newCount = newCount - newCount % fChunkSize;
//		if (fAllocatedCount < newCount									// redundant, surely?
//		|| (fAllowShrinkage && (fAllocatedCount > newCount)))
		{
			void * newStorage = realloc(fArrayStorage, fElementSize * newCount);
			if (newStorage)
			{
				fAllocatedCount = newCount;
				fArrayStorage = newStorage;
			}
			else
				err = MemError();
		}
	}

	return err;
}


void *
NArray::at(ArrayIndex index) const
{
	if (fCount <= 0 || /*index < 0 ||*/ index >= fCount)
		return NULL;
	return (char *)fArrayStorage + fElementSize * index;
}


ArrayIndex
NArray::contains(const void * inElement) const
{
	NBlockComparator cmp(fElementSize);
	for (ArrayIndex i = 0; i < fCount; ++i)
	{
		const void * key1 = cmp.keyOf(at(i));
		const void * key2 = cmp.keyOf(inElement);	// shouldn’t this be OUTSIDE the loop?
		if (cmp.compareKeys(key1, key2) == 0)
			return i;
	}
	return kIndexNotFound;
}


ArrayIndex
NArray::where(const void * inElement) const
{
	// actually returns logical size
	return fCount;
}

#pragma mark -

/*------------------------------------------------------------------------------
	N S o r t e d A r r a y
------------------------------------------------------------------------------*/

NSortedArray::NSortedArray()
{
	fComparator = NULL;
}

NSortedArray::~NSortedArray()
{ }

NewtonErr
NSortedArray::init(NComparator * inComparator, ArrayIndex inElementSize, ArrayIndex inChunkSize, ArrayIndex inCount, bool inAllowShrinkage)
{
	if (inComparator == NULL)
		return -1;

	fComparator = inComparator;
	return NArray::init(inElementSize, inChunkSize, inCount, inAllowShrinkage);
}


/*------------------------------------------------------------------------------
	Determine whether array contains the given element.
	Args:		inElement
	Return:	index of found element, or kIndexNotFound
------------------------------------------------------------------------------*/

ArrayIndex
NSortedArray::contains(const void * inElement) const
{
	ArrayIndex index = where(inElement);
	if (index < fCount)
	{
		// element sits within the array -- is it actually there?
		const void * key1 = fComparator->keyOf(inElement);
		const void * key2 = fComparator->keyOf(at(index));
		if (fComparator->compareKeys(key1, key2) == 0)
			return index;
	}
	return kIndexNotFound;
}


/*------------------------------------------------------------------------------
	Find where the given element should be within the array.
	Args:		inElement
	Return:	index, even if inElement is not in the array
				fCount => at the end
------------------------------------------------------------------------------*/

ArrayIndex
NSortedArray::where(const void * inElement) const
{
	// perform binary search for element
	int firstIndex = 0;				// r10
	int lastIndex = fCount - 1;	// r8
	int index;
	while (lastIndex >= firstIndex)
	{
		index = (firstIndex + lastIndex) / 2;

		const void * key1 = fComparator->keyOf(inElement);
		const void * key2 = fComparator->keyOf(at(index));
		int cmp = fComparator->compareKeys(key1, key2);

		if (cmp >= 0)
		// object must be in higher range
			firstIndex = index + 1;
		else
		// object must be in lower range
			lastIndex = index - 1;
	}
	return lastIndex + 1;
}

