/*
	File:		Array.cc

	Contains:	Recognition sub-system array implementation.

	Written by:	Newton Research Group.
*/

#include "RecObject.h"

/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

Ptr	GetCurrent(ArrayIterator * ioIter);
Ptr	GetNext(ArrayIterator * ioIter);

#pragma mark -
#pragma mark CArray

CArray::CArray()
	: fMem(NULL)
{ }


CArray::~CArray()
{ }


CArray *
CArray::make(size_t inElementSize, ArrayIndex inSize)
{
	CArray * array = new CArray;
	XTRY
	{
		XFAIL(array == NULL)
		XFAILIF(array->iArray(inElementSize, inSize) != noErr, array->release(); array = NULL;)
	}
	XENDTRY;
	return array;
}


NewtonErr
CArray::iArray(size_t inElementSize, ArrayIndex inSize)
{
	NewtonErr  err = noErr;
	iRecObject();
	fChunkSize = 6;
	fFree = (inSize == 0) ? 0 : 6;
	fElementSize = inElementSize;
	fSize = inSize;
	XTRY
	{
		XFAILNOT(fMem = ReallocPtr(fMem, (inSize + fFree) * inElementSize), err = MemError();)
	}
	XENDTRY;
	XDOFAIL(err)
	{
		fSize = 0;
	}
	XENDFAIL;
	return err;
}


size_t
CArray::sizeInBytes(void)
{
	size_t thisSize = 0;
	if (fMem)
		thisSize = GetPtrSize(fMem);
	thisSize += (sizeof(CArray) - sizeof(CRecObject));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CRecObject::sizeInBytes() + thisSize;
}


NewtonErr
CArray::copyInto(CRecObject * outObject)
{
	NewtonErr  err;
	XTRY
	{
		XFAILNOT(outObject, err = 1;)
		XFAIL(err = CRecObject::copyInto(outObject))
		XFAILNOT(fMem, err = 1;)
		size_t memSize = GetPtrSize(fMem);
		char * p = NewPtr(memSize);
		memmove(p, fMem, memSize);
		static_cast<CArray *>(outObject)->fMem = p;
		static_cast<CArray *>(outObject)->fElementSize = fElementSize;
		static_cast<CArray *>(outObject)->fSize = err ? 0 : fSize;
		static_cast<CArray *>(outObject)->fFree = fFree;
		static_cast<CArray *>(outObject)->fChunkSize = fChunkSize;
	}
	XENDTRY;
	return err;
}


void
CArray::dealloc(void)
{
	if (fMem)
		FreePtr(fMem), fMem = NULL;
}


void
CArray::dump(CMsg * outMsg)
{
	char  buf[100];

	sprintf(buf, "\n\tes: %ld  cnt: %d  free: %d", fElementSize, fSize, fFree);
	outMsg->msgStr(buf);
	outMsg->msgLF();
}


ArrayIndex
CArray::add(void)
{
	ArrayIndex  index = kIndexNotFound;
	ArrayIndex  newSize = fSize;
	ArrayIndex  newFree = fFree - 1;
	if (fFree != 0
	 || (newFree = fChunkSize, (fMem = ReallocPtr(fMem, (fSize + fChunkSize + 1) * fElementSize)) != NULL)	// no free space, try allocating new chunk
	 || (newFree = 0, (fMem = ReallocPtr(fMem, (fSize + 1) * fElementSize)) != NULL))								// chunk allocation failed; try smaller allocation
	{
			index = newSize;
			newSize++;
			fFlags |= 0x01;
	}
	fSize = newSize;
	fFree = newFree;
	return index;
}


Ptr
CArray::addEntry(void)
{
	return getEntry(add());
}


Ptr
CArray::getEntry(ArrayIndex index)
{
	if (fMem != NULL && index <= fSize)
		return fMem + index * fElementSize;
	return NULL;
}


void
CArray::setEntry(ArrayIndex index, Ptr inData)
{
	Ptr	p;
	if ((p = getEntry(index)) != NULL)
		memmove(p, inData, fElementSize);
}


void
CArray::cutToIndex(ArrayIndex index)
{
	ArrayIndex  tailCount = fSize - index;
	fSize -= tailCount;
	fFree += tailCount;
}


void
CArray::clear(void)
{
	cutToIndex(0);
}


NewtonErr
CArray::reuse(ArrayIndex index)
{
	NewtonErr  err = noErr;

	fFree = 0;
	XTRY
	{
		XFAILNOT(fMem, err = 1;)
		XFAILNOT(fMem = ReallocPtr(fMem, (index + fChunkSize) * fElementSize), err = MemError();)
	}
	XENDTRY;
	XDOFAIL(err)
	{
		index = 0;
	}
	XENDFAIL;
	fSize = index;
	return err;
}


void
CArray::compact(void)
{
// the original uses Handles so it doesn’t matter that memory moves after compact()
// --but it matters now!
// so we won’t bother until we can work out the consequences of each call
/*
	if (fFree != 0)
	{
		fFree = 0;
		if (fMem)
			fMem = ReallocPtr(fMem, fSize * fElementSize);
	}
*/
}


NewtonErr
CArray::load(ULong, ULong, ULong, ULong)
{
	return noErr;
}


NewtonErr
CArray::loadFromSoup(RefArg, RefArg, ULong)
{
// INCOMPLETE
	return 1;
}


NewtonErr
CArray::save(ULong inArg1, ULong inArg2, ULong inArg3, ULong inArg4)
{
	compact();
/*
	Handle	h = MakeHandle(3*sizeof(long));
	if (h)
	{
		long *	p;
		NameHandle(h, 'asav');
		p = (long *)*h;
		*p = fElementSize;
		*(p+1) = fSize;
		*(p+2) = fChunkSize;
		SaveResource(h, inArg1, inArg2, NULL);
		SaveResource(fMem, inArg3, inArg4, NULL);
		Disposehandle(h);
	}
*/
	return 1;
}


Ptr
CArray::getIterator(ArrayIterator * ioIter)
{
	ioIter->fGetNext = GetNext;
	ioIter->fGetCurrent = GetCurrent;
	if (fMem && fSize)
	{
		ioIter->fMem = fMem;
		ioIter->fMemPtr =
		ioIter->fCurrent = fMem;
	}
	else
	{
		ioIter->fMem = NULL;
		ioIter->fMemPtr =
		ioIter->fCurrent = NULL;
	}
	ioIter->fIndex = 0;
	ioIter->fElementSize = fElementSize;
	ioIter->fSize = fSize;
	return ioIter->fCurrent;
}

#pragma mark -

/*------------------------------------------------------------------------------
	A r r a y I t e r a t o r
------------------------------------------------------------------------------*/

Ptr
GetCurrent(ArrayIterator * ioIter)
{
	Ptr	p = ioIter->fMem;
	long  delta = p - ioIter->fMemPtr;
	if (delta)
	{
		ioIter->fMemPtr = p;
		ioIter->fCurrent += delta;
	}

	return ioIter->fCurrent;
}


Ptr
GetNext(ArrayIterator * ioIter)
{
	Ptr	p = ioIter->fMem;
	long  delta = p - ioIter->fMemPtr;
	if (delta)
	{
		ioIter->fMemPtr = p;
		ioIter->fCurrent += delta;
	}

	ioIter->fIndex++;
	ioIter->fCurrent += ioIter->fElementSize;
	return ioIter->fCurrent;
}


#pragma mark -
#pragma mark CDArray
/*------------------------------------------------------------------------------
	C D A r r a y
------------------------------------------------------------------------------*/

CDArray *
CDArray::make(size_t inElementSize, ArrayIndex inSize)
{
	CDArray * array = new CDArray;
	XTRY
	{
		XFAIL(array == NULL)
		XFAILIF(array->iDArray(inElementSize, inSize) != noErr, array->release(); array = NULL;)
	}
	XENDTRY;
	return array;
}


NewtonErr
CDArray::iDArray(size_t inElementSize, ArrayIndex inSize)
{
	NewtonErr	err = iArray(inElementSize, inSize);
//	SetPtrName(fMem, 'dDta');
	return err;
}


ArrayIndex
CDArray::insert(ArrayIndex index)
{
	ArrayIndex	entryIndex;
	XTRY
	{
		ArrayIndex  stopIndex = index + 1;
		Ptr	stopPtr, startPtr = getEntry(index);
		XFAILIF(add() == kIndexNotFound, entryIndex = kIndexNotFound;)
		XFAILIF(startPtr == NULL, entryIndex = fSize - 1;)
		startPtr = getEntry(index);
		stopPtr = getEntry(stopIndex);
		memmove(stopPtr, startPtr, (fSize - stopIndex) * fElementSize);
		entryIndex = index;
	}
	XENDTRY;
	return entryIndex;
}


ArrayIndex
CDArray::insertEntry(ArrayIndex index, Ptr inData)
{
	ArrayIndex  insertIndex = insert(index);
	if (insertIndex != kIndexNotFound)
		memmove(getEntry(index), inData, fElementSize);
	return insertIndex;
}


ArrayIndex
CDArray::insertEntries(ArrayIndex index, Ptr inData, ArrayIndex inCount)
{
	Ptr		startPtr = getEntry(index);
	Ptr		stopPtr = getEntry(index + inCount);
	size_t	dataSize = inCount * fElementSize;
	size_t	moveSize = (fSize - index) * fElementSize;
	size_t	currentSize = (fSize + fChunkSize) * fElementSize;
	if ((fMem = ReallocPtr(fMem, currentSize + dataSize)) != NULL)
	{
		// pointers may have moved!!??
		memmove(stopPtr, startPtr, moveSize);
		memmove(startPtr, inData, dataSize);
		fSize += inCount;
		// no corresponding reduction in fFree!!??
		fFlags |= 0x01;
		return index;
	}
	return kIndexNotFound;
}


ArrayIndex
CDArray::deleteEntry(ArrayIndex index)
{
	return deleteEntries(index, 1);
}


ArrayIndex
CDArray::deleteEntries(ArrayIndex index, ArrayIndex inCount)
{
	if (inCount == 0)
		return 0;
	if (index >= fSize)
		return kIndexNotFound;
	if (index + inCount >= fSize)
		inCount = fSize - index;

	ArrayIndex  stopIndex;
	Ptr	stopPtr, startPtr = getEntry(index);
	if (startPtr != NULL)
	{
		stopIndex = index + inCount;
		stopPtr = getEntry(stopIndex);
		if (stopPtr != NULL)
			memmove(startPtr, stopPtr, (fSize - stopIndex) * fElementSize);
		fSize -= inCount;
		fFree += inCount;
	}
	return index;
}
