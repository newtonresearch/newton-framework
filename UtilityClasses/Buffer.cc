/*
	File:		Buffer.cc

	Contains:	Buffer implementations.

	Written by:	Newton Research Group, 2007.
*/

#include "Buffer.h"
#include "Pipes.h"

/*----------------------------------------------------------------------
	C M i n B u f f e r
	All pure virtual functions; constructor & destructor do nothing.
----------------------------------------------------------------------*/

CMinBuffer::CMinBuffer()
{ }

CMinBuffer::~CMinBuffer()
{ }

#pragma mark -

/*----------------------------------------------------------------------
	C B u f f e r
	All pure virtual functions; constructor & destructor do nothing.
----------------------------------------------------------------------*/

CBuffer::CBuffer()
{ }

CBuffer::~CBuffer()
{ }

#pragma mark -

/*----------------------------------------------------------------------
	C S h a d o w B u f f e r S e g m e n t
----------------------------------------------------------------------*/

CShadowBufferSegment::CShadowBufferSegment()
{
	fLoBound = 0;
	fIndex = 0;
	fHiBound = 0;
	fBufSize = 0;
}

CShadowBufferSegment::~CShadowBufferSegment()
{ }

NewtonErr
CShadowBufferSegment::init(ObjectId inSharedMemId, long inOffset, long inSize)
{
	NewtonErr err;
	fSharedMem = inSharedMemId;
	err = fSharedMem.getSize(&fBufSize);
	fLoBound = fIndex = inOffset;
	fHiBound = (inSize >= 0) ? inOffset + inSize : fBufSize;
	return err;
}

void
CShadowBufferSegment::reset(void)
{
	fLoBound = fIndex = 0;
	fHiBound = fBufSize;
}

int
CShadowBufferSegment::peek(void)
{
	if (fIndex < fHiBound)
		return getByteAt(fIndex);
	return -1;
}

int
CShadowBufferSegment::next(void)
{
	if (fIndex < fHiBound)
		return getByteAt(++fIndex);
	return -1;
}

int
CShadowBufferSegment::skip(void)
{
	if (fIndex < fHiBound)
		return fIndex++;
	return -1;
}

int
CShadowBufferSegment::get(void)
{
	if (fIndex < fHiBound)
		return getByteAt(++fIndex);
	return -1;
}

UByte
CShadowBufferSegment::getByteAt(long index)
{
	UByte byte;
	size_t len = 0;
	fSharedMem.copyFromShared(&len, &byte, 1, index);
	return byte;
}

size_t
CShadowBufferSegment::getn(UByte * inData, size_t inLength)
{
	size_t len = fHiBound - fIndex;
	if (len > inLength)
		len = inLength;
	fSharedMem.copyFromShared(&len, inData, len, fIndex);
	fIndex += len;
	return len;
}

int
CShadowBufferSegment::copyOut(UByte * inData, size_t & ioLength)
{
	size_t len = getn(inData, ioLength);
	ioLength -= len;
	return (fIndex == fHiBound) ? -1 : 0;
}

int
CShadowBufferSegment::put(int inData)
{
	if (fIndex < fHiBound)
		return putByteAt(inData, ++fIndex);
	return -1;
}

int
CShadowBufferSegment::putByteAt(int inData, long index)
{
	UByte byte = inData;
	if (fSharedMem.copyToShared(&byte, 1, index) == noErr)
		return inData;
	return -1;
}

size_t
CShadowBufferSegment::putn(const UByte * inData, size_t inLength)
{
	size_t len = fHiBound - fIndex;
	if (len > inLength)
		len = inLength;
	fSharedMem.copyToShared((void *)inData, len, fIndex);
	fIndex += len;
	return len;
}

int
CShadowBufferSegment::copyIn(const UByte * inData, size_t & ioLength)
{
	size_t len = putn(inData, ioLength);
	ioLength -= len;
	return (fIndex == fHiBound) ? -1 : 0;
}

size_t
CShadowBufferSegment::getSize(void)
{
	return fHiBound - fLoBound;
}

bool
CShadowBufferSegment::atEOF(void)
{
	return fIndex == fHiBound;
}

long
CShadowBufferSegment::hide(long inCount, int inDirection)
{
	long result;

	if (inCount == 0)
		result = 0;
	else
	{
		if (inDirection == kSeekFromBeginning)
		{
			fLoBound += inCount;
			if (fLoBound < 0)
			{
				result = inCount - fLoBound;
				fLoBound = 0;
			}
			else if (fLoBound > fBufSize)
			{
				result = inCount - (fLoBound - fBufSize);
				fLoBound = fBufSize;
			}
			fIndex = fLoBound;
		}
		else if (inDirection == kSeekFromEnd)
		{
			fHiBound -= inCount;
			if (fHiBound < 0)
			{
				result = inCount + fHiBound;
				fHiBound = 0;
			}
			else if (fHiBound > fBufSize)
			{
				result = inCount + (fHiBound - fBufSize);
				fHiBound = fBufSize;
			}
			fIndex = fHiBound;
		}
		else
			result = 0;
	}

	return result;
}

long
CShadowBufferSegment::seek(long inOffset, int inDirection)
{
	if (inOffset == 0)
	{
		if (inDirection == kSeekFromBeginning)
			fIndex = fLoBound;
		else if (inDirection == kSeekFromEnd)
			fIndex = fHiBound;
	}
	else
	{
		long index;
		if (inDirection == kSeekFromBeginning)
			index = fLoBound + inOffset;
		else if (inDirection == kSeekFromHere)
			index = fIndex + inOffset;
		else if (inDirection == kSeekFromEnd)
			index = fBufSize - inOffset;
		else
			index = fIndex;
		if (index < fLoBound)
			index = fLoBound;
		else if (index > fBufSize)
			index = fBufSize;
		fIndex = index;
	}
	return fIndex - fLoBound;
}

long
CShadowBufferSegment::position(void)
{
	return fIndex - fLoBound;
}

#pragma mark -

/*----------------------------------------------------------------------
	C B a s e R i n g B u f f e r
	All pure virtual functions; constructor & destructor do nothing.
----------------------------------------------------------------------*/

CBaseRingBuffer::CBaseRingBuffer()
{ }

CBaseRingBuffer::~CBaseRingBuffer()
{ }

#pragma mark -

/*----------------------------------------------------------------------
	C R i n g B u f f e r
	At last, a class that can actually be instantiated!
----------------------------------------------------------------------*/

CRingBuffer::CRingBuffer()
{
	fIsBufOurs = false;
	fBuf = NULL;
	fBufEnd = NULL;
	fBufSize = 0;
	fWrPtr = NULL;
	fRdPtr = NULL;
	fIsShared = false;
}

CRingBuffer::~CRingBuffer()
{
	if (fIsBufOurs)
		FreePtr((Ptr)fBuf);
}

NewtonErr
CRingBuffer::init(size_t inSize)
{
	NewtonErr err = noErr;
	XTRY
	{
		if (fIsBufOurs)
			FreePtr((Ptr)fBuf);
		fBufSize = inSize + 1;
		fBuf = (UByte *)NewPtr(fBufSize);
		XFAILIF(fBuf == NULL, err = MemError();)
		fBufEnd = fBuf + fBufSize;
		fRdPtr = fWrPtr = fBuf;
		fIsBufOurs = true;
	}
	XENDTRY;
	return err;
}

NewtonErr
CRingBuffer::makeShared(ULong inPermissions)
{
	NewtonErr err;
	XTRY
	{
		if (!fIsShared)
			XFAIL(err = fSharedMem.init())
		err = fSharedMem.setBuffer(fBuf, fBufSize, inPermissions + kSMemNoSizeChangeOnCopyTo);
	}
	XENDTRY;
	if (err == noErr)
		fIsShared = true;
	return err;
}

NewtonErr
CRingBuffer::unshare(void)
{
	if (fIsShared)
	{
		fSharedMem.destroyObject();
		fIsShared = false;
	}
	return noErr;
}

int
CRingBuffer::peek(void)
{
	if (fRdPtr != fWrPtr)
		return *fRdPtr;
	return -1;
}

int
CRingBuffer::next(void)
{
	if (fRdPtr != fWrPtr)
	{
		if (++fRdPtr == fBufEnd)
			fRdPtr = fBuf;
		if (fRdPtr != fWrPtr)
			return *fRdPtr;
	}
	return -1;
}

int
CRingBuffer::skip(void)
{
	if (fRdPtr != fWrPtr)
	{
		if (++fRdPtr == fBufEnd)
			fRdPtr = fBuf;
		if (fRdPtr != fWrPtr)
			return 0;
	}
	return -1;
}

int
CRingBuffer::get(void)
{
	if (!isEmpty())
	{
		UByte ch = *fRdPtr;
		if (++fRdPtr == fBufEnd)
			fRdPtr = fBuf;
		return ch;
	}
	return -1;
}

size_t
CRingBuffer::getn(UByte * inData, size_t inLength)
{
	size_t len = inLength;
	copyOut(inData, len);
	return inLength - len;
}

size_t
CRingBuffer::getnAt(long inOffset, UByte * inData, size_t inLength)
{
	size_t len = 0;
	if (dataCount() > inOffset)
	{
		UByte * savePtr = fRdPtr;
		fRdPtr += inOffset;
		if (fRdPtr >= fBufEnd)
			fRdPtr -= fBufSize;
		len = copyOut(inData, inLength);
		fRdPtr = savePtr;
	}
	return len;
}

int
CRingBuffer::copyOut(UByte * inData, size_t & ioLength)
{
	if (ioLength > 0)
	{
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computeGetVectors(buf2, buf2Len, buf1, buf1Len);
		if (buf1 != NULL && buf1Len > 0)
		{
			if (buf1Len > ioLength)
				buf1Len = ioLength;
			memmove(inData, buf1, buf1Len);
			inData += buf1Len;
			ioLength -= buf1Len;
			fRdPtr += buf1Len;
			if (fRdPtr == fBufEnd)
				fRdPtr = fBuf;
		}
		if (buf2 != NULL && buf2Len > 0)
		{
			if (buf2Len > ioLength)
				buf2Len = ioLength;
			memmove(inData, buf2, buf2Len);
			ioLength -= buf2Len;
			fRdPtr += buf2Len;
		}
		if (isEmpty())
			return -1;
	}
	return 0;
}

int
CRingBuffer::put(int inData)
{
	if (isFull())
		inData = -1;
	else
	{
		*fWrPtr = inData;
		if (++fWrPtr == fBufEnd)
			fWrPtr = fBuf;
	}
	return inData;
}

size_t
CRingBuffer::putn(const UByte * inData, size_t inLength)
{
	size_t len = inLength;
	copyIn(inData, len);
	return inLength - len;
}

int
CRingBuffer::copyIn(const UByte * inData, size_t & ioLength)
{
	if (ioLength > 0)
	{
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computePutVectors(buf2, buf2Len, buf1, buf1Len);
		if (buf1 != NULL && buf1Len > 0)
		{
			if (buf1Len > ioLength)
				buf1Len = ioLength;
			memmove(buf1, inData, buf1Len);
			inData += buf1Len;
			ioLength -= buf1Len;
			fWrPtr += buf1Len;
			if (fWrPtr == fBufEnd)
				fWrPtr = fBuf;
		}
		if (buf2 != NULL && buf2Len > 0)
		{
			if (buf2Len > ioLength)
				buf2Len = ioLength;
			memmove(buf2, inData, buf2Len);
			ioLength -= buf2Len;
			fWrPtr += buf2Len;
		}
		if (isFull())
			return -1;
	}
	return 0;
}

int
CRingBuffer::copyIn(CPipe * inPipe, size_t & ioLength)
{
	if (ioLength > 0)
	{
		bool isEOF;
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computePutVectors(buf2, buf2Len, buf1, buf1Len);
		if (buf1 != NULL && buf1Len > 0)
		{
			if (buf1Len > ioLength)
				buf1Len = ioLength;
			NewtonErr err = noErr;
			newton_try
			{
				inPipe->readChunk(buf1, buf1Len, isEOF);
			}
			newton_catch(exPipe)
			{
				err = (NewtonErr)(long)CurrentException()->data;
			}
			end_try;
			if (err)
				return 0;
			ioLength -= buf1Len;
			fWrPtr += buf1Len;
			if (fWrPtr == fBufEnd)
				fWrPtr = fBuf;
		}
		if (buf2 != NULL && buf2Len > 0)
		{
			if (buf2Len > ioLength)
				buf2Len = ioLength;
			NewtonErr err = noErr;
			newton_try
			{
				inPipe->readChunk(buf2, buf2Len, isEOF);
			}
			newton_catch(exPipe)
			{
				err = (NewtonErr)(long)CurrentException()->data;
			}
			end_try;
			if (err)
				return 0;
			ioLength -= buf2Len;
			fWrPtr += buf2Len;
		}
		if (isFull())
			return -1;
	}
	return 0;
}

void
CRingBuffer::reset(void)
{
	fRdPtr = fWrPtr = fBuf;
}

size_t
CRingBuffer::getSize(void)
{
	return fBufSize - 1;
}

bool
CRingBuffer::atEOF(void)
{
	return isEmpty() || isFull();
}

bool
CRingBuffer::isFull(void)
{
	UByte * p = fRdPtr;
	if (p == fBuf)
		p = fBufEnd;
	return fWrPtr == (p - 1);
}

bool
CRingBuffer::isEmpty(void)
{
	return fRdPtr == fWrPtr;
}

size_t
CRingBuffer::freeCount(void)
{
	return (fRdPtr > fWrPtr) ? fRdPtr - fWrPtr - 1 : (fRdPtr + fBufSize) - fWrPtr - 1;
}

size_t
CRingBuffer::dataCount(void)
{
	return (fWrPtr >= fRdPtr) ? fWrPtr - fRdPtr : (fWrPtr + fBufSize) - fRdPtr;
}

long
CRingBuffer::updatePutVector(long inLength)
{
	if (inLength > 0)
	{
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computePutVectors(buf2, buf2Len, buf1, buf1Len);
		if (buf1 != NULL && buf1Len > 0)
		{
			if (buf1Len > inLength)
				buf1Len = inLength;
			inLength -= buf1Len;
			fWrPtr += buf1Len;
			if (fWrPtr == fBufEnd)	// shurely it could go over?
				fWrPtr = fBuf;
		}
		if (buf2 != NULL && buf2Len > 0)
		{
			if (buf2Len > inLength)
				buf2Len = inLength;
			inLength -= buf2Len;
			fWrPtr += buf2Len;
		}
	}
	return inLength;
}

long
CRingBuffer::updateGetVector(long inLength)
{
	if (inLength > 0)
	{
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computeGetVectors(buf2, buf2Len, buf1, buf1Len);
		if (buf1 != NULL && buf1Len > 0)
		{
			if (buf1Len > inLength)
				buf1Len = inLength;
			inLength -= buf1Len;
			fRdPtr += buf1Len;
			if (fRdPtr == fBufEnd)	// shurely it could go over?
				fRdPtr = fBuf;
		}
		if (buf2 != NULL && buf2Len > 0)
		{
			if (buf2Len > inLength)
				buf2Len = inLength;
			inLength -= buf2Len;
			fRdPtr += buf2Len;
		}
	}
	return inLength;
}

void
CRingBuffer::computePutVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len)
{
	UByte * breakPtr = (fRdPtr == fBuf) ? fBufEnd - 1 : fRdPtr - 1;

	if (fWrPtr == fRdPtr)
	{
		if (fWrPtr == fBuf || breakPtr == fBuf)
		{
			outBuf1 = NULL;
			outBuf1Len = 0;
		}
		else
		{
			outBuf1 = fBuf;
			outBuf1Len = breakPtr - fBuf;
		}
		if (fWrPtr > breakPtr)
		{
			outBuf2 = fWrPtr;
			outBuf2Len = fBufEnd - fWrPtr;
		}
		else
		{
			outBuf2 = fWrPtr;
			outBuf2Len = breakPtr - fWrPtr;
		}
	}
	else if (fWrPtr == breakPtr)
	{
		outBuf1 = NULL;
		outBuf1Len = 0;
		outBuf2 = NULL;
		outBuf2Len = 0;
	}
	else if (fWrPtr > breakPtr)
	{
		outBuf1 = fBuf;
		outBuf1Len = breakPtr - fBuf;
		outBuf2 = fWrPtr;
		outBuf2Len = fBufEnd - fWrPtr;
	}
	else
	{
		outBuf1 = NULL;
		outBuf1Len = 0;
		outBuf2 = fWrPtr;
		outBuf2Len = breakPtr - fWrPtr;
	}
}

void
CRingBuffer::computeGetVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len)
{
	UByte * breakPtr = (fRdPtr == fBuf) ? fBufEnd - 1 : fRdPtr - 1;

	if (fWrPtr == fRdPtr)
	{
		outBuf1 = NULL;
		outBuf1Len = 0;
		outBuf2 = NULL;
		outBuf2Len = 0;
	}
	else if (fWrPtr == breakPtr)
	{
		if (fWrPtr >= fRdPtr)
		{
			outBuf1 = NULL;
			outBuf1Len = 0;
			outBuf2 = fRdPtr;
			outBuf2Len = fWrPtr - fRdPtr;
		}
		else
		{
			outBuf1 = fBuf;
			outBuf1Len = fWrPtr - fBuf;
			outBuf2 = fRdPtr;
			outBuf2Len = fBufEnd - fRdPtr;
		}
	}
	else if (fRdPtr >= fWrPtr)
	{
		outBuf1 = fBuf;
		outBuf1Len = fWrPtr - fBuf;
		outBuf2 = fRdPtr;
		outBuf2Len = fBufEnd - fRdPtr;
	}
	else
	{
		outBuf1 = NULL;
		outBuf1Len = 0;
		outBuf2 = fRdPtr;
		outBuf2Len = fWrPtr - fRdPtr;
	}
}


#pragma mark -

/*----------------------------------------------------------------------
	C S h a d o w R i n g B u f f e r
----------------------------------------------------------------------*/

CShadowRingBuffer::CShadowRingBuffer()
{
	fBufSize = 0;
	fWrIndex = 0;
	fRdIndex = 0;
	fTempRdIndex = 0;
}

CShadowRingBuffer::~CShadowRingBuffer()
{ }

NewtonErr
CShadowRingBuffer::init(ObjectId inSharedMemId, long inOffset, long inSize)
{
	NewtonErr err;
	fSharedMem = inSharedMemId;
	err = fSharedMem.getSize(&fBufSize);
	fRdIndex = fTempRdIndex = inOffset;
	fWrIndex = inOffset + inSize;
	return err;
}

int
CShadowRingBuffer::peek(void)
{
	if (fRdIndex != fWrIndex)
		return getByteAt(fRdIndex);
	return -1;
}

int
CShadowRingBuffer::next(void)
{
	if (fRdIndex != fWrIndex)
	{
		if (++fRdIndex == fBufSize)
			fRdIndex = 0;
		if (fRdIndex != fWrIndex)
			return getByteAt(fRdIndex);
	}
	return -1;
}

int
CShadowRingBuffer::skip(void)
{
	if (fRdIndex != fWrIndex)
	{
		if (++fRdIndex == fBufSize)
			fRdIndex = 0;
		if (fRdIndex != fWrIndex)
			return 0;
	}
	return -1;
}

int
CShadowRingBuffer::get(void)
{
	if (fRdIndex != fWrIndex)
	{
		int ch = getByteAt(fRdIndex);
		if (++fRdIndex == fBufSize)
			fRdIndex = 0;
		return ch;
	}
	return -1;
}

UByte
CShadowRingBuffer::getByteAt(long inOffset)
{
	UByte byte;
	size_t len = 0;
	fSharedMem.copyFromShared(&len, &byte, 1);
	return byte;
}

size_t
CShadowRingBuffer::getn(UByte * inData, size_t inLength)
{
	size_t len = inLength;
	copyOut(inData, len);
	return inLength - len;
}

int
CShadowRingBuffer::copyOut(UByte * inData, size_t & ioLength)
{
	if (ioLength > 0)
	{
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computeGetVectors(buf2, buf2Len, buf1, buf1Len);
		if (/*buf1 != NULL &&*/ buf1Len > 0)
		{
			if (buf1Len > ioLength)
				buf1Len = ioLength;
			fSharedMem.copyFromShared(&buf1Len, inData, buf1Len);
			inData += buf1Len;
			ioLength -= buf1Len;
			fRdIndex += buf1Len;
			if (fRdIndex == fBufSize)
				fRdIndex = 0;
		}
		if (/*buf2 != NULL &&*/ buf2Len > 0)
		{
			if (buf2Len > ioLength)
				buf2Len = ioLength;
			fSharedMem.copyFromShared(&buf2Len, inData, buf2Len);
			ioLength -= buf2Len;
			fRdIndex += buf2Len;
		}
	}
	return isEmpty() ? -1 : 0;
}

int
CShadowRingBuffer::put(int inData)
{
	long rdIndex = fRdIndex;
	if (rdIndex == 0)
		rdIndex = fBufSize;
	if (fWrIndex == rdIndex - 1)
		return -1;
	putByteAt(inData, fWrIndex);
	if (++fWrIndex == fBufSize)
		fWrIndex = 0;
	return inData;
}

int
CShadowRingBuffer::putByteAt(int inData, long index)
{
	UByte byte = inData;
	if (fSharedMem.copyToShared(&byte, 1, index) == noErr)
		return inData;
	return -1;
}

size_t
CShadowRingBuffer::putn(const UByte * inData, size_t inLength)
{
	size_t len = inLength;
	copyIn(inData, len);
	return inLength - len;
}

int
CShadowRingBuffer::copyIn(const UByte * inData, size_t & ioLength)
{
	if (ioLength > 0)
	{
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computePutVectors(buf2, buf2Len, buf1, buf1Len);
		if (buf1 != NULL && buf1Len > 0)
		{
			if (buf1Len > ioLength)
				buf1Len = ioLength;
			fSharedMem.copyToShared((void *)inData, buf1Len);
			inData += buf1Len;
			ioLength -= buf1Len;
			fWrIndex += buf1Len;
			if (fWrIndex == fBufSize)
				fWrIndex = 0;
		}
		if (buf2 != NULL && buf2Len > 0)
		{
			if (buf2Len > ioLength)
				buf2Len = ioLength;
			fSharedMem.copyToShared((void *)inData, buf2Len);
			ioLength -= buf2Len;
			fWrIndex += buf2Len;
		}
	}
	return isFull() ? -1 : 0;
}

void
CShadowRingBuffer::reset(void)
{
	fWrIndex = fRdIndex = 0;
}

size_t
CShadowRingBuffer::getSize(void)
{
	return fBufSize - 1;
}

bool
CShadowRingBuffer::atEOF(void)
{
	return isEmpty() || isFull();
}

bool
CShadowRingBuffer::isFull(void)
{
	long breakIndex = (fRdIndex == 0) ? fBufSize - 1 : fRdIndex - 1;
	return fWrIndex == breakIndex;
}

bool
CShadowRingBuffer::isEmpty(void)
{
	return fRdIndex == fWrIndex;
}

size_t
CShadowRingBuffer::freeCount(void)
{
	return (fRdIndex > fWrIndex) ? fRdIndex - fWrIndex - 1 : (fRdIndex + fBufSize) - fWrIndex - 1;
}

size_t
CShadowRingBuffer::dataCount(void)
{
	return (fWrIndex >= fRdIndex) ? fWrIndex - fRdIndex : (fWrIndex + fBufSize) - fRdIndex;
}

size_t
CShadowRingBuffer::tempDataCount(void)
{
	return (fWrIndex >= fTempRdIndex) ? fWrIndex - fTempRdIndex : (fWrIndex + fBufSize) - fTempRdIndex;
}

long
CShadowRingBuffer::updatePutVector(long inLength)
{
	if (inLength > 0)
	{
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computePutVectors(buf2, buf2Len, buf1, buf1Len);
		if (/*buf1 != NULL &&*/ buf1Len > 0)
		{
			if (buf1Len > inLength)
				buf1Len = inLength;
			inLength -= buf1Len;
			fWrIndex += buf1Len;
			if (fWrIndex == fBufSize)
				fWrIndex = 0;
		}
		if (/*buf2 != NULL &&*/ buf2Len > 0)
		{
			if (buf2Len > inLength)
				buf2Len = inLength;
			inLength -= buf2Len;
			fWrIndex += buf2Len;
		}
	}
	return inLength;
}

long
CShadowRingBuffer::updateGetVector(long inLength)
{
	if (inLength > 0)
	{
		UByte * buf1, * buf2;
		size_t buf1Len, buf2Len;
		computeGetVectors(buf2, buf2Len, buf1, buf1Len);
		if (/*buf1 != NULL &&*/ buf1Len > 0)
		{
			if (buf1Len > inLength)
				buf1Len = inLength;
			inLength -= buf1Len;
			fRdIndex += buf1Len;
			if (fRdIndex == fBufSize)
				fRdIndex = 0;
		}
		if (/*buf2 != NULL &&*/ buf2Len > 0)
		{
			if (buf2Len > inLength)
				buf2Len = inLength;
			inLength -= buf2Len;
			fRdIndex += buf2Len;
		}
	}
	return inLength;
}

void
CShadowRingBuffer::computePutVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len)
{
	// outBufs aren’t actually used, but we do need to know if they’re non-NULL
	long breakIndex = (fRdIndex == 0) ? fBufSize - 1 : fRdIndex - 1;

	if (fWrIndex == fRdIndex)
	{
		if (fWrIndex == 0 || breakIndex == 0)
		{
			outBuf1 = (UByte *)0;
			outBuf1Len = 0;
		}
		else
		{
			outBuf1 = (UByte *)0;
			outBuf1Len = breakIndex;
		}
		if (fWrIndex > breakIndex)
		{
			outBuf2 = (UByte *)fWrIndex;
			outBuf2Len = fBufSize - fWrIndex;
		}
		else
		{
			outBuf2 = (UByte *)fWrIndex;
			outBuf2Len = breakIndex - fWrIndex;
		}
	}
	else if (fWrIndex == breakIndex)
	{
		outBuf1 = (UByte *)0;
		outBuf1Len = 0;
		outBuf2 = (UByte *)0;
		outBuf2Len = 0;
	}
	else if (fWrIndex > breakIndex)
	{
		outBuf1 = (UByte *)0;
		outBuf1Len = breakIndex;
		outBuf2 = (UByte *)fWrIndex;
		outBuf2Len = fBufSize - fWrIndex;
	}
	else
	{
		outBuf1 = (UByte *)0;
		outBuf1Len = 0;
		outBuf2 = (UByte *)fWrIndex;
		outBuf2Len = breakIndex - fWrIndex;
	}
}

void
CShadowRingBuffer::computeGetVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len)
{
	// outBufs aren’t actually used, but we do need to know if they’re non-NULL
	long breakIndex = (fRdIndex == 0) ? fBufSize - 1 : fRdIndex - 1;

	if (fWrIndex == fRdIndex)
	{
		outBuf1 = (UByte *)0;
		outBuf1Len = 0;
		outBuf2 = (UByte *)0;
		outBuf2Len = 0;
	}
	else if (fWrIndex == breakIndex)
	{
		if (fWrIndex >= fRdIndex)
		{
			outBuf1 = (UByte *)0;
			outBuf1Len = 0;
			outBuf2 = (UByte *)fRdIndex;
			outBuf2Len = fWrIndex - fRdIndex;
		}
		else
		{
			outBuf1 = (UByte *)0;
			outBuf1Len = fWrIndex;
			outBuf2 = (UByte *)fRdIndex;
			outBuf2Len = fBufSize - fRdIndex;
		}
	}
	else if (fRdIndex >= fWrIndex)
	{
		outBuf1 =(UByte *)0;
		outBuf1Len = fWrIndex;
		outBuf2 = (UByte *)fRdIndex;
		outBuf2Len = fBufSize - fRdIndex;
	}
	else
	{
		outBuf1 = (UByte *)0;
		outBuf1Len = 0;
		outBuf2 = (UByte *)fRdIndex;
		outBuf2Len = fWrIndex - fRdIndex;
	}
}

void
CShadowRingBuffer::computeTempGetVectors(UByte *& outBuf1, size_t & outBuf1Len, UByte *& outBuf2, size_t & outBuf2Len)
{
	long breakIndex = (fTempRdIndex == 0) ? fBufSize - 1 : fTempRdIndex - 1;

	if (fWrIndex == fTempRdIndex)
	{
		outBuf1 = (UByte *)0;
		outBuf1Len = 0;
		outBuf2 = (UByte *)0;
		outBuf2Len = 0;
	}
	else if (fWrIndex == breakIndex)
	{
		if (fWrIndex >= fTempRdIndex)
		{
			outBuf1 = (UByte *)0;
			outBuf1Len = 0;
			outBuf2 = (UByte *)fTempRdIndex;
			outBuf2Len = fWrIndex - fTempRdIndex;
		}
		else
		{
			outBuf1 = (UByte *)0;
			outBuf1Len = fWrIndex;
			outBuf2 = (UByte *)fTempRdIndex;
			outBuf2Len = fBufSize - fTempRdIndex;
		}
	}
	else if (fTempRdIndex >= fWrIndex)
	{
		outBuf1 = (UByte *)0;
		outBuf1Len = fWrIndex;
		outBuf2 = (UByte *)fTempRdIndex;
		outBuf2Len = fBufSize - fTempRdIndex;
	}
	else
	{
		outBuf1 = (UByte *)0;
		outBuf1Len = 0;
		outBuf2 = (UByte *)fTempRdIndex;
		outBuf2Len = fWrIndex - fTempRdIndex;
	}
}

