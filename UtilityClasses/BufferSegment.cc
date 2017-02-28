/*
	File:		BufferDegment.cc

	Contains:	The CBufferSegment class implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "BufferSegment.h"

/*----------------------------------------------------------------------
	C B u f f e r S e g m e n t
	vt @ 0001D3A8
----------------------------------------------------------------------*/

CBufferSegment::CBufferSegment()
{
	fBufferIsOurs = false;
	fBuffer = NULL;
	fBufferLimit = NULL;
	fBufSize = 0;
	fLoBound = fHiBound = NULL;
	fBufPtr = NULL;
	fSharedBufIsInitialized = false;
//	fSharedBuf = 0;		// don’t think they really mean this
}


CBufferSegment::~CBufferSegment()
{
	if (fBufferIsOurs && fBuffer != NULL)
		free(fBuffer);
}


NewtonErr
CBufferSegment::init(size_t inSize)
{
	if (fBufferIsOurs && fBuffer != NULL)
		free(fBuffer);

	fBuffer = NewPtr(inSize);
	if (fBuffer == NULL)
		return MemError();

	fBufSize = inSize;
	fBufferLimit = fBuffer + inSize;
	fLoBound = fBuffer;
	fHiBound = fBufferLimit;
	fBufPtr = fBuffer;
	fBufferIsOurs = true;

	return restoreShared(kSMemReadWrite);
}


NewtonErr
CBufferSegment::init(void * inBuffer, size_t inSize, bool inDoFreeBuffer, long inValidOffset, long inValidCount)
{
	if (fBufferIsOurs && fBuffer != NULL)
		free(fBuffer);

	fBufSize = inSize;
	fBuffer = (Ptr)inBuffer;
	fBufferLimit = (Ptr)inBuffer + inSize;
	fLoBound = fBufPtr = (Ptr)inBuffer + inValidOffset;
	Ptr hiBound;
	if (inValidCount >= 0)
	{
		hiBound = fLoBound + inValidCount;
		if (hiBound > fBufferLimit)
			hiBound = fBufferLimit;
	}
	else
		hiBound = fBufferLimit;
	fHiBound = hiBound;
	fBufferIsOurs = inDoFreeBuffer;

	return restoreShared(kSMemReadWrite);
}


// Shared mem methods

NewtonErr
CBufferSegment::makeShared(ULong inPermissions)
{
	NewtonErr err = noErr;
	if (!fSharedBufIsInitialized)
	{
		XTRY
		{
			XFAIL(err = fSharedBuf.init())
			XFAIL(err = fSharedBuf.setBuffer(fBuffer, fBufSize, inPermissions + kSMemNoSizeChangeOnCopyTo))
			fSharedBufIsInitialized = true;
		}
		XENDTRY;
	}
	return err;
}


NewtonErr
CBufferSegment::restoreShared(ULong inPermissions)
{
	NewtonErr err = noErr;
	if (fSharedBufIsInitialized)
	{
		err = fSharedBuf.setBuffer(fBuffer, fBufSize, inPermissions + kSMemNoSizeChangeOnCopyTo);
	}
	return err;
}


NewtonErr
CBufferSegment::unshare(void)
{
	if (fSharedBufIsInitialized)
	{
		fSharedBuf.destroyObject();
		fSharedBufIsInitialized = false;
	}
	return noErr;
}


size_t
CBufferSegment::getPhysicalSize(void)
{
	return fBufSize;
}


NewtonErr
CBufferSegment::setPhysicalSize(size_t inSize)
{
	long offset = fBufPtr - fBuffer;

	char * newBuf = ReallocPtr(fBuffer, inSize);
	if (newBuf == NULL)
		return MemError();

	fBufSize = inSize;
	fBuffer = newBuf;
	fBufferLimit = newBuf + inSize;
	fLoBound = fBuffer;
	fHiBound = fBufferLimit;
	fBufPtr = newBuf + offset;

	return restoreShared(kSMemReadWrite);
}


// Buffer methods

int
CBufferSegment::peek(void)
{
	return (fBufPtr < fHiBound) ? *fBufPtr : -1;
}


int
CBufferSegment::next(void)
{
	return (fBufPtr < fHiBound) ? *(++fBufPtr) : -1;
}


int
CBufferSegment::skip(void)
{
	return (fBufPtr < fHiBound) ? fBufPtr++, 0 : -1;
}


int
CBufferSegment::get(void)
{
	return (fBufPtr < fHiBound) ? *fBufPtr++ : -1;
}


size_t
CBufferSegment::getn(UByte * outBuf, size_t inSize)
{
	if (inSize > 0)
	{
		size_t numOfBytesAvailable = fHiBound - fBufPtr;
		if (inSize > numOfBytesAvailable)
			inSize = numOfBytesAvailable;
		if (inSize > 0)
		{
			memmove(outBuf, fBufPtr, inSize);
			fBufPtr += inSize;
		}
	}
	return inSize;
}


int
CBufferSegment::copyOut(UByte * outBuf, size_t & ioSize)
{
	if (ioSize > 0)
	{
		ioSize -= getn(outBuf, ioSize);
		if (fBufPtr == fHiBound)
			return -1;
	}
	return 0;
}


int
CBufferSegment::put(int inDataByte)
{
	if (fBufPtr >= fHiBound)
		return -1;
	*fBufPtr++ = inDataByte;
	return inDataByte & 0xFF;
}


size_t
CBufferSegment::putn(const UByte * inBuf, size_t inSize)
{
	if (inSize > 0)
	{
		size_t numOfBytesAvailable = fHiBound - fBufPtr;
		if (inSize > numOfBytesAvailable)
			inSize = numOfBytesAvailable;
		if (inSize > 0)
		{
			memmove(fBufPtr, inBuf, inSize);
			fBufPtr += inSize;
		}
	}
	return inSize;
}


int
CBufferSegment::copyIn(const UByte * inBuf, size_t & ioSize)
{
	if (ioSize > 0)
	{
		ioSize -= putn(inBuf, ioSize);
		if (fBufPtr == fHiBound)
			return -1;
	}
	return 0;
}


void
CBufferSegment::reset(void)
{
	fLoBound = fBufPtr = fBuffer;
	fHiBound = fBufferLimit;
}


size_t
CBufferSegment::getSize(void)
{
	return fHiBound - fLoBound;
}


bool
CBufferSegment::atEOF(void)
{
	return fBufPtr == fHiBound;
}


long
CBufferSegment::hide(long inCount, int inDirection)
{
	if (inCount == 0)
		return 0;

	long xs = 0;
	if (inDirection == kSeekFromBeginning)
	{
		fLoBound += inCount;
		if (fLoBound < fBuffer)
		{
			xs = inCount + (fBuffer - fLoBound);
			fLoBound = fBuffer;
		}
		else if (fLoBound > fBufferLimit)
		{
			xs = inCount - (fLoBound - fBufferLimit);
			fLoBound = fBufferLimit;
		}
		fBufPtr = fLoBound;
	}
	else if (inDirection == kSeekFromEnd)
	{
		fHiBound -= inCount;
		if (fHiBound < fBuffer)
		{
			xs = inCount - (fBuffer - fHiBound);
			fHiBound = fBuffer;
		}
		else if (fHiBound > fBufferLimit)
		{
			xs = inCount + (fHiBound - fBufferLimit);
			fHiBound = fBufferLimit;
		}
		fBufPtr = fHiBound;
	}
	else
		return 0;	// should never happen

	return xs;
}


long
CBufferSegment::seek(long inOffset, int inDirection)
{
	if (inOffset == 0)
	{
		if (inDirection == kSeekFromBeginning)
			fBufPtr = fLoBound;
		else if (inDirection == kSeekFromEnd)
			fBufPtr = fHiBound;
	}
	else
	{
		Ptr p;

		if (inDirection == kSeekFromBeginning)
			p = fLoBound + inOffset;
		else if (inDirection == kSeekFromHere)
			p = fBufPtr + inOffset;
		else if (inDirection == kSeekFromEnd)
			p = fHiBound - inOffset;

		if (p < fLoBound)
			p = fLoBound;
		if (p > fHiBound)
			p = fHiBound;

		fBufPtr = p;
	}

	return fBufPtr - fLoBound;
}


long
CBufferSegment::position(void)
{
	return (fBufPtr != NULL) ? fBufPtr - fLoBound : 0;
}

