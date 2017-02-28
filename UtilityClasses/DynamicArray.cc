/*
	File:		DynamicArray.cc

	Contains:	Interface to the CDynamicArray class.

	Written by:	Newton Research Group.
*/

#include "DynamicArray.h"
#include "ArrayIterator.h"


/*--------------------------------------------------------------------------------
	C D y n a m i c A r r a y
--------------------------------------------------------------------------------*/

void
MoveLow(CDynamicArray * inArray)
{
	// the original accesses fElementSize but then discards it and uses 0
//	size_t r1 = inArray->fElementSize;
	Size blockSize = GetPtrSize(inArray->fArrayBlock);
	// allocate a buffer
	Ptr buf = NewPtr(blockSize);
	if (buf)
	{
		// copy block to the buffer
		memmove(buf, inArray->fArrayBlock, blockSize);
		size_t elementCount = inArray->count();
		// empty the block
		inArray->setArraySize(0);
		// then reallocate it
		inArray->setArraySize(elementCount);
		// and copy its contents back
		memmove(inArray->fArrayBlock, buf, blockSize);
		FreePtr(buf);
	}
}


CDynamicArray::CDynamicArray()
{
	fArrayBlock = NULL;
	fAllocatedSize = 0;
	fChunkSize = CDynamicArray::kDefaultChunkSize;
	fElementSize = CDynamicArray::kDefaultElementSize;
	fSize = 0;
	fIterator = NULL;
}


CDynamicArray::CDynamicArray(size_t inElementSize, ArrayIndex inChunkSize)
{
	fArrayBlock = NULL;
	fAllocatedSize = 0;
	fChunkSize = inChunkSize;
	fElementSize = inElementSize;
	fSize = 0;
	fIterator = NULL;
}



CDynamicArray::~CDynamicArray()
{
	if (fIterator)
		fIterator->deleteArray();
	if (fArrayBlock)
		FreePtr(fArrayBlock);
}


// array manipulation primitives

NewtonErr
CDynamicArray::setArraySize(ArrayIndex inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (inSize == 0)
		{
			if (fArrayBlock)
			{
				FreePtr(fArrayBlock);
				fArrayBlock = NULL;
				fAllocatedSize = 0;
			}
		}
		else
		{
			if (fAllocatedSize < inSize || fAllocatedSize - inSize >= fChunkSize)
			{
			// need to grow the block
				ArrayIndex	newSize;
				if (fChunkSize == 0)
					newSize = inSize;
				else
				{
					newSize = inSize + fChunkSize;
					newSize = newSize - newSize % fChunkSize;
				}
				if (newSize != fAllocatedSize)
				{
					Ptr newBlock = ReallocPtr(fArrayBlock, fElementSize * newSize);
					XFAIL(err = MemError())
					fArrayBlock = newBlock;
					fAllocatedSize = newSize;
				}
			}
		}
	}
	XENDTRY;
	return err;
}


NewtonErr
CDynamicArray::setElementCount(ArrayIndex inSize)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = setArraySize(inSize))
		fSize = inSize;
	}
	XENDTRY;
	return err;
}


void *
CDynamicArray::safeElementPtrAt(ArrayIndex index)
{
	if (fSize > 0 && index != kIndexNotFound && index < fSize)
		return fArrayBlock + fElementSize * index;
	return NULL;
}


NewtonErr
CDynamicArray::getElementsAt(ArrayIndex index, void * outElements, ArrayIndex inCount)
{
	if (inCount > 0)
		memmove(outElements, fArrayBlock + fElementSize * index, fElementSize * inCount);
	return noErr;
}


NewtonErr
CDynamicArray::insertElementsBefore(ArrayIndex index, void * inElements, ArrayIndex inCount)
{
	NewtonErr	err = noErr;

	if (index > fSize)
		index = fSize;
	if (inCount > 0)
	{
		if ((err = setArraySize(fSize + inCount)) == noErr)
		{
			Ptr	srcPtr = fArrayBlock + fElementSize * index;
			Ptr	dstPtr = fArrayBlock + fElementSize * (index + inCount);
			Ptr	endPtr = fArrayBlock + fElementSize * fSize;
			if (index < fSize)
				memmove(dstPtr, srcPtr, endPtr - srcPtr);
			memmove(srcPtr, inElements, fElementSize * inCount);
			fSize += inCount;
			if (fIterator)
				fIterator->insertElementsBefore(index, inCount);
		}
	}
	return err;
}


NewtonErr
CDynamicArray::replaceElementsAt(ArrayIndex index, void  * inElements, ArrayIndex inCount)
{
	if (inCount > 0)
		memmove(fArrayBlock + fElementSize * index, inElements, fElementSize * inCount);
	return noErr;
}


NewtonErr
CDynamicArray::removeElementsAt(ArrayIndex index, ArrayIndex inCount)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (fSize > 0 && inCount > 0)
		{
			Ptr	dstPtr = fArrayBlock + fElementSize * index;
			Ptr	srcPtr = fArrayBlock + fElementSize * (index + inCount);
			Ptr	endPtr = fArrayBlock + fElementSize * fSize;
			if (endPtr > srcPtr)
				memmove(dstPtr, srcPtr, endPtr - srcPtr);
			XFAIL(err = setArraySize(fSize - inCount))
			fSize -= inCount;
			if (fIterator)
				fIterator->removeElementsAt(index, inCount);
		}
	}
	XENDTRY;
	return err;
}


// miscellaneous functions

NewtonErr
CDynamicArray::merge(CDynamicArray * inArray)
{
	if (this->fElementSize != inArray->fElementSize)
		return kUCErrElementSizeMismatch;
	else if (inArray->fSize <= 0)
		return noErr;
	else
		return insertElementsBefore(fSize, inArray->fArrayBlock, inArray->fSize);
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C A r r a y I t e r a t o r
--------------------------------------------------------------------------------*/

CArrayIterator::CArrayIterator()
{
	init();
}


CArrayIterator::CArrayIterator(CDynamicArray * inDynamicArray)
{
	init(inDynamicArray, 0, inDynamicArray->count() - 1, kIterateForward);
}


CArrayIterator::CArrayIterator(CDynamicArray * inDynamicArray, bool inForward)
{
	init(inDynamicArray, 0, inDynamicArray->count() - 1, inForward);
}


CArrayIterator::CArrayIterator(CDynamicArray * inDynamicArray,
		ArrayIndex inLowBound,
		ArrayIndex inHighBound, bool inForward)
{
	init(inDynamicArray, inLowBound, inHighBound, inForward);
}


CArrayIterator::~CArrayIterator()
{
	if (fDynamicArray)
		fDynamicArray->fIterator = removeFromList();
}


void
CArrayIterator::init(void)
{
	fNextLink = fPreviousLink = this;
	fCurrentIndex = fLowBound = fHighBound = kIndexNotFound;
	fIterateForward = kIterateForward;
	fDynamicArray = NULL;
}


void
CArrayIterator::init(CDynamicArray * inDynamicArray,
							ArrayIndex inLowBound,
							ArrayIndex inHighBound, bool inForward)
{
	fNextLink = fPreviousLink = this;
	fDynamicArray = inDynamicArray;
	inDynamicArray->fIterator = appendToList(inDynamicArray->fIterator);
	initBounds(inLowBound, inHighBound, inForward);
}


void
CArrayIterator::initBounds(ArrayIndex inLowBound, ArrayIndex inHighBound, bool inForward)
{
	fHighBound = (fDynamicArray->fSize > 0) ? MINMAX(0, inHighBound, fDynamicArray->fSize - 1) : kIndexNotFound;
	fLowBound = (fHighBound != kIndexNotFound) ? MINMAX(0, inLowBound, fHighBound) : kIndexNotFound;
	fIterateForward = inForward;
	reset();
}


void
CArrayIterator::resetBounds(bool inForward)
{
	fHighBound = (fDynamicArray->fSize > 0) ? fDynamicArray->fSize - 1 : kIndexNotFound;
	fLowBound = (fHighBound != kIndexNotFound) ? 0 : kIndexNotFound;
	fIterateForward = inForward;
	reset();
}


void
CArrayIterator::reset(void)
{
	fCurrentIndex = fIterateForward ? fLowBound : fHighBound;
}


void
CArrayIterator::switchArray(CDynamicArray * inNewArray, bool inForward)
{
	if (fDynamicArray)
	{
		fDynamicArray->fIterator = removeFromList();
		fDynamicArray = NULL;
	}
	init(inNewArray, 0, inNewArray->fSize - 1, inForward);
}


ArrayIndex
CArrayIterator::firstIndex(void)
{
	reset();
	return more() ? fCurrentIndex : kIndexNotFound;
}


ArrayIndex
CArrayIterator::nextIndex(void)
{
	advance();
	return more() ? fCurrentIndex : kIndexNotFound;
}


ArrayIndex
CArrayIterator::currentIndex(void)
{
	return fDynamicArray ? fCurrentIndex : kIndexNotFound;
}


void
CArrayIterator::removeElementsAt(ArrayIndex index, ArrayIndex inCount)
{
	if (fLowBound > index)
		fLowBound -= inCount;
	if (fHighBound >= index)
		fHighBound -= inCount;
	if (fIterateForward)
	{
		if (fCurrentIndex >= index)
			fCurrentIndex -= inCount;
	}
	else
	{
		if (fCurrentIndex > index)
			fCurrentIndex -= inCount;
	}	
	if (fDynamicArray && fNextLink != fPreviousLink)
		fNextLink->removeElementsAt(index, inCount);
}


void
CArrayIterator::insertElementsBefore(ArrayIndex index, ArrayIndex inCount)
{
}


void
CArrayIterator::deleteArray(void)
{
	if (fNextLink != fPreviousLink)
		fNextLink->deleteArray();
	fDynamicArray = NULL;
}


bool
CArrayIterator::more(void)
{
	return fDynamicArray != NULL
		 && fCurrentIndex != kIndexNotFound;
}


void
CArrayIterator::advance(void)
{
	if (fIterateForward)
		fCurrentIndex = fCurrentIndex < fHighBound ? fCurrentIndex + 1 : kIndexNotFound;
	else
		fCurrentIndex = fCurrentIndex > fLowBound ? fCurrentIndex - 1 : kIndexNotFound;
}


CArrayIterator *
CArrayIterator::appendToList(CArrayIterator * inList)
{
	if (inList)
	{
		fPreviousLink = inList;
		fNextLink = inList->fNextLink;
		inList->fNextLink->fPreviousLink = this;
		inList->fNextLink = this;
	}
	return this;
}


CArrayIterator *
CArrayIterator::removeFromList(void)
{
	CArrayIterator *	result = (fNextLink != this) ? fNextLink : NULL;
	fNextLink->fPreviousLink = fPreviousLink;
	fNextLink->fNextLink = fPreviousLink->fNextLink;
	fNextLink = fPreviousLink = this;
	return result;
}
