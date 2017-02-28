/*
	File:		BufferList.cc

	Contains:	The CBufferList class implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "BufferList.h"

/*----------------------------------------------------------------------
	C B u f f e r L i s t
----------------------------------------------------------------------*/

CBufferList::CBufferList()
{
	fList = NULL;
	fIter = NULL;
	fBuffer = NULL;
	fLoBound = kIndexNotFound;
	fCurrentIndex = kIndexNotFound;
	fHiBound = kIndexNotFound;
	fBuffersAreOurs = false;
	fListIsOurs = false;
}


CBufferList::~CBufferList()
{
	if (fList && fListIsOurs)
	{
		if (fBuffersAreOurs)
		{
			fIter->resetBounds();
			CBuffer * buf;
			for (buf = (CBuffer *)fIter->firstItem(); fIter->more(); buf = (CBuffer *)fIter->nextItem())
			{
				if (buf)
					delete buf;
			}
		}
		delete fList;
	}
	if (fIter)
		delete fIter;
}


NewtonErr
CBufferList::init(bool inDeleteSegments)
{
	NewtonErr err = noErr;

	XTRY
	{
		fBuffersAreOurs = inDeleteSegments;
		fListIsOurs = true;
		fList = new CList;
		XFAILIF(fList == NULL, err = MemError();)
		fIter = new CListIterator(fList);
		XFAILIF(fIter == NULL, err = MemError();)
		resetMark();	// original doesn’t do this
	}
	XENDTRY;

	return err;
}


NewtonErr
CBufferList::init(CList * bufList, bool inDeleteSegments)
{
	NewtonErr err = noErr;

	XTRY
	{
		fBuffersAreOurs = inDeleteSegments;
		fListIsOurs = false;
		fList = bufList;
		fIter = new CListIterator(fList);
		XFAILIF(fIter == NULL, err = MemError();)
		resetMark();
	}
	XENDTRY;

	return err;
}


// Buffer methods

int
CBufferList::peek(void)
{
	int value = fBuffer->peek();
	if (value == -1)
	{
		if (nextSegment())
			value = fBuffer->peek();
	}
	return value;
}


int
CBufferList::next(void)
{
	int value = fBuffer->next();
	if (value == -1)
	{
		if (nextSegment())
			value = fBuffer->next();
	}
	return value;
}


int
CBufferList::skip(void)
{
	int p = fBuffer->skip();
	if (p == -1)
	{
		if (nextSegment())
			p = fBuffer->skip();
	}
	return p;
}


int
CBufferList::get(void)
{
	int value = fBuffer->get();
	if (value == -1)
	{
		if (nextSegment())
			value = fBuffer->get();
	}
	return value;
}


size_t
CBufferList::getn(UByte * outBuf, size_t inSize)
{
	size_t numOfBytesGot = fBuffer->getn(outBuf, inSize);
	while (numOfBytesGot < inSize)
	{
		if (!nextSegment())
			break;
		numOfBytesGot += fBuffer->getn(outBuf + numOfBytesGot, inSize - numOfBytesGot);
	}
	return numOfBytesGot;
}


int
CBufferList::copyOut(UByte * outBuf, size_t & ioSize)
{
	ioSize -= fBuffer->getn(outBuf, ioSize);
	if (fCurrentIndex == fHiBound
	&&  fBuffer->atEOF())
		return -1;
	return 0;
}


int
CBufferList::put(int inDataByte)
{
	int result = fBuffer->put(inDataByte);
	if (result == -1)
	{
		if (nextSegment())
			result = fBuffer->put(inDataByte);
	}
	return result;
}


size_t
CBufferList::putn(const UByte * inBuf, size_t inSize)
{
	size_t numOfBytesPut = fBuffer->putn(inBuf, inSize);
	while (numOfBytesPut < inSize)
	{
		if (!nextSegment())
			break;
		numOfBytesPut += fBuffer->putn(inBuf + numOfBytesPut, inSize - numOfBytesPut);
	}
	return numOfBytesPut;
}


int
CBufferList::copyIn(const UByte * inBuf, size_t & ioSize)
{
	ioSize -= fBuffer->putn(inBuf, ioSize);
	if (fCurrentIndex == fHiBound && fBuffer->atEOF())
		return -1;
	return 0;
}


void
CBufferList::reset(void)
{
	fIter->resetBounds();
	CBuffer * buf;
	for (buf = (CBuffer *)fIter->firstItem(); fIter->more(); buf = (CBuffer *)fIter->nextItem())
	{
		buf->reset();
	}
	resetMark();
}


void
CBufferList::resetMark(void)
{
	fHiBound = fList->count() - 1;
}


size_t
CBufferList::getSize(void)
{
	if (fList->count() == 1)
		return fBuffer->getSize();

	size_t totalSize = 0;
	fIter->resetBounds();
	fIter->initBounds(fLoBound, fHiBound, kIterateForward);

	CBuffer * buf;
	for (buf = (CBuffer *)fIter->firstItem(); fIter->more(); buf = (CBuffer *)fIter->nextItem())
	{
		totalSize += buf->getSize();
	}
	return totalSize;
}


bool
CBufferList::atEOF(void)
{
	if (fCurrentIndex != fHiBound)
		return false;
	return fBuffer->atEOF();
}


long
CBufferList::hide(long inCount, int inDirection)
{
	if (inCount == 0)
		return 0;

	if (fList->count() == 1)
		return fBuffer->hide(inCount, inDirection);

	fIter->resetBounds();

	if (inDirection == kSeekFromBeginning)
	{
		if (inCount > 0)
			fIter->initBounds(fLoBound, fList->count() - 1, kIterateForward);
		else
			fIter->initBounds(0, fLoBound, kIterateBackward);
	}
	else if (inDirection == kSeekFromEnd)
	{
		if (inCount > 0)
			fIter->initBounds(0, fHiBound, kIterateBackward);
		else
			fIter->initBounds(fHiBound, fList->count() - 1, kIterateForward);
	}
	else
		return 0;	// should never happen

	long countRemaining = inCount;
	CBuffer * buf;
	for (buf = (CBuffer *)fIter->firstItem(); fIter->more(); buf = (CBuffer *)fIter->nextItem())
	{
		countRemaining -= buf->hide(countRemaining, inDirection);
		if (countRemaining == 0)
			break;
	}
	fCurrentIndex = fIter->currentIndex();
	if (inDirection == kSeekFromBeginning)
		fLoBound = fCurrentIndex;
	else
		fHiBound = fCurrentIndex;
	selectSegment(fCurrentIndex);

	if (inDirection == kSeekFromEnd)
		fBuffer->seek(0, kSeekFromEnd);

	return inCount - countRemaining;
}


long
CBufferList::seek(long inOffset, int inDirection)
{
	if (fList->count() == 1)
		return fBuffer->seek(inOffset, inDirection);

	long overallPosition = 0;
	if (inOffset == 0)
	{
		if (inDirection == kSeekFromBeginning)
			selectSegment(fLoBound);
		else if (inDirection == kSeekFromEnd)
		{
			selectSegment(fHiBound);
			fBuffer->seek(0, 1);
		}
	}
	else
	{
		size_t totalSize = getSize();
		overallPosition = position();
		if (inDirection == kSeekFromBeginning)
			overallPosition = inOffset;
		else if (inDirection == kSeekFromHere)
			overallPosition += inOffset;
		else if (inDirection == kSeekFromEnd)
			overallPosition = totalSize - inOffset;

		if (overallPosition < 0)
			overallPosition = 0;
		else if (overallPosition > totalSize)
			overallPosition = totalSize;

		fIter->resetBounds();
		fIter->initBounds(fLoBound, fHiBound, kIterateForward);

		long thePosition = overallPosition;
		CBuffer * buf;
		for (buf = (CBuffer *)fIter->firstItem(); fIter->more(); buf = (CBuffer *)fIter->nextItem())
		{
			thePosition -= buf->getSize();
			if (thePosition <= 0)
				break;
		}
		if (thePosition < 0)
			thePosition = -thePosition;

		selectSegment(fIter->currentIndex());
		fBuffer->seek(thePosition, kSeekFromEnd);
	}

	return overallPosition;
}


long
CBufferList::position(void)
{
	if (fList->count() == 1
	||  fLoBound == fCurrentIndex)
		return fBuffer->position();

	long thePosition = 0;
	fIter->resetBounds();
	fIter->initBounds(fLoBound, fCurrentIndex - 1, kIterateForward);

	CBuffer * buf;
	for (buf = (CBuffer *)fIter->firstItem(); fIter->more(); buf = (CBuffer *)fIter->nextItem())
	{
		thePosition += buf->getSize();
	}
	return thePosition + fBuffer->position();
}


// List methods

CBuffer *
CBufferList::at(ArrayIndex index)
{ return (CBuffer *)fList->at(index); }


CBuffer *
CBufferList::first(void)
{ return (CBuffer *)fList->first(); }


CBuffer *
CBufferList::last(void)
{ return (CBuffer *)fList->last(); }


NewtonErr
CBufferList::insert(CBuffer * inItem)
{ return insertLast(inItem); }


NewtonErr
CBufferList::insertBefore(ArrayIndex index, CBuffer * inItem)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->insertBefore(index, inItem))
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::insertAt(ArrayIndex index, CBuffer * inItem)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->insertAt(index, inItem))
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::insertFirst(CBuffer * inItem)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->insertFirst(inItem))
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::insertLast(CBuffer * inItem)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->insertLast(inItem))
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::remove(CBuffer * inItem)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->remove(inItem))
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::removeAt(ArrayIndex index)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->removeAt(index))
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::removeFirst(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->removeFirst())
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::removeLast(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->removeLast())
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::removeAll(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->removeAll())
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::deleteItem(CBuffer * inItem)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fList->remove(inItem))
		if (inItem)
			delete inItem;
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::deleteAt(ArrayIndex index)
{
	NewtonErr err;
	XTRY
	{
		CBuffer * item = (CBuffer *)fList->at(index);
		XFAIL(err = fList->removeAt(index))
		if (item)
			delete item;
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::deleteFirst(void)
{
	NewtonErr err;
	XTRY
	{
		CBuffer * item = (CBuffer *)fList->at(0);
		XFAIL(err = fList->removeFirst())
		if (item)
			delete item;
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::deleteLast(void)
{
	NewtonErr err;
	XTRY
	{
		CBuffer * item = (CBuffer *)fList->last();
		XFAIL(err = fList->removeLast())
		if (item)
			delete item;
		resetMark();
	}
	XENDTRY;
	return err;
}


NewtonErr
CBufferList::deleteAll(void)
{
	NewtonErr err;
	XTRY
	{
		fIter->resetBounds();
		CBuffer * buf;
		for (buf = (CBuffer *)fIter->firstItem(); fIter->more(); buf = (CBuffer *)fIter->nextItem())
		{
			if (buf)
				delete buf;
		}

		XFAIL(err = fList->removeAll())
		resetMark();
	}
	XENDTRY;
	return err;
}


ArrayIndex
CBufferList::getIndex(CBuffer * inItem)
{
	return fList->getIdentityIndex(inItem);
}


void
CBufferList::selectSegment(ArrayIndex index)
{
	fCurrentIndex = index;
	fBuffer = (CBuffer *)fList->at(index);
	fBuffer->seek(0, kSeekFromBeginning);
}


bool
CBufferList::nextSegment(void)
{
	int index = fCurrentIndex;
	if (index < fHiBound)
	{
		selectSegment(++index);
		return true;
	}
	return false;
}

