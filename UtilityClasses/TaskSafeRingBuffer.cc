/*
	File:		TaskSafeRingBuffer.cc

	Contains:	TaskSafeRingBuffer implementation.

	Written by:	Newton Research Group, 2007.
*/

#include "TaskSafeRingBuffer.h"
#include "KernelTasks.h"
#include "AppWorld.h"
#include "OSErrors.h"

/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

extern CTask *			gCurrentTask;


/*----------------------------------------------------------------------
	C T a s k S a f e R i n g B u f f e r
----------------------------------------------------------------------*/

CTaskSafeRingBuffer::CTaskSafeRingBuffer()
{
	fBufLen = 0;
	fBuf = NULL;
	fBufEnd = NULL;
	fPutPtr = NULL;
	fGetPtr = NULL;
	f18 = NULL;
	f1C = NULL;
	fIsBufOurs = NO;
	fLockCount = 0;
	fCurrentTask = 0;	// kNoId
	fIsThreaded = NO;
	f20 = noErr;
	f24 = noErr;
}


CTaskSafeRingBuffer::~CTaskSafeRingBuffer()
{
	if (fIsBufOurs && fBuf != NULL)
		delete fBuf;	// free()?
	if (f1C)
		delete f1C;
	if (f18)
		delete f18;
}


NewtonErr
CTaskSafeRingBuffer::init(size_t inBufLen, bool inThreaded)
{
	NewtonErr	err = noErr;
	XTRY
	{
		fIsBufOurs = YES;
		fIsThreaded = inThreaded;
		fBufLen = inBufLen + 1;
		fBuf = (UByte *)NewNamedPtr(fBufLen, 'trng');
		XFAILNOT(fBuf, err = MemError();)
		fBufEnd = fBuf + fBufLen;
		fPutPtr = fGetPtr = fBuf;

		f18 = new CULockingSemaphore;
		XFAILNOT(f18, err = MemError();)
		f18->init();

		f1C = new CULockingSemaphore;
		XFAILNOT(f1C, err = MemError();)
		f1C->init();
	}
	XENDTRY;
	return err;
}


void
CTaskSafeRingBuffer::pause(Timeout inDelay)
{
	if (fIsThreaded)
	{
		NewtonErr	err;
		if ((err = gAppWorld->fork(NULL)) == noErr)
			gAppWorld->releaseMutex();
		else
			ThrowErr(exPipe, err);
	}
	Sleep(inDelay);
	if (fIsThreaded)
		gAppWorld->acquireMutex();
}


void
CTaskSafeRingBuffer::acquire(void)
{
	f1C->acquire(kWaitOnBlock);
	if (fCurrentTask != *gCurrentTask)
	{
		f1C->release();
		f18->acquire(kWaitOnBlock);
		f1C->acquire(kWaitOnBlock);
		fCurrentTask = *gCurrentTask;
	}
	else if (fLockCount == 0)
	{
		f18->acquire(kWaitOnBlock);
		fCurrentTask = *gCurrentTask;
	}
	fLockCount++;
	f1C->release();
}


void
CTaskSafeRingBuffer::checkGetSignal(void)
{
	NewtonErr	err = f20;
	if (err != noErr)
	{
		f20 = noErr;
		ThrowErr(exPipe, err);
	}
}


void
CTaskSafeRingBuffer::checkPutSignal(void)
{
	NewtonErr	err = f24;
	if (err != noErr)
	{
		f24 = noErr;
		ThrowErr(exPipe, err);
	}
}


void
CTaskSafeRingBuffer::release(void)
{
	f1C->acquire(kWaitOnBlock);
	if (fLockCount == 1)
	{
		f18->release();
		fCurrentTask = 0;	// kNoId
	}
	fLockCount--;
	f1C->release();
}

#pragma mark -

int
CTaskSafeRingBuffer::peek(void)
{
	int	ch;
	checkGetSignal();
	acquire();
	ch = (fGetPtr != fPutPtr) ? *fGetPtr : -1;
	release();
	return ch;
}


int
CTaskSafeRingBuffer::next(void)
{
	int	ch;
	checkGetSignal();
	acquire();
	if (fGetPtr != fPutPtr)
	{
		fGetPtr++;
		if (fGetPtr == fBufEnd)
			fGetPtr = fBuf;
		ch = (fGetPtr != fPutPtr) ? *fGetPtr : -1;
	}
	else
		ch = -1;
	release();
	return ch;
}


int
CTaskSafeRingBuffer::skip(void)
{
	int	status;
	checkGetSignal();
	acquire();
	if (fGetPtr != fPutPtr)
	{
		fGetPtr++;
		if (fGetPtr == fBufEnd)
			fGetPtr = fBuf;
		status = (fGetPtr != fPutPtr) ? 0 : -1;
	}
	else
		status = -1;
	release();
	return status;
}


int
CTaskSafeRingBuffer::get(void)
{
	int	ch;
	checkGetSignal();
	acquire();
	if (fGetPtr != fPutPtr)
	{
		ch = *fGetPtr++;
		if (fGetPtr == fBufEnd)
			fGetPtr = fBuf;
	}
	else
		ch = -1;
	release();
	return ch;
}


UByte
CTaskSafeRingBuffer::getCompletely(Timeout interval, Timeout inTimeout)
{
	int		ch;
	CTime		expiryTime(TimeFromNow(inTimeout));
	checkGetSignal();
	while ((ch = get()) == -1)
	{
		pause(interval);
		if (inTimeout != kNoTimeout
		&& GetGlobalTime() > expiryTime)
			ThrowErr(exPipe, kOSErrMessageTimedOut);
	}
	return ch;
}


size_t
CTaskSafeRingBuffer::getn(UByte * inBuf, size_t inLen)
{
	size_t	n = inLen;
	checkGetSignal();
	copyOut(inBuf, n);
	return inLen - n;
}


void
CTaskSafeRingBuffer::getnCompletely(UByte * inBuf, size_t inLen, Timeout interval, Timeout inTimeout)
{
	CTime		expiryTime(TimeFromNow(inTimeout));
	size_t	nRemaining = inLen;
	size_t	nDone = 0;
	while (nRemaining > 0)
	{
		checkGetSignal();
		copyOut(inBuf + nDone, nRemaining);
		if (nRemaining > 0)
		{
			nDone = inLen - nRemaining;
			pause(interval);
			if (inTimeout != kNoTimeout
			&& GetGlobalTime() > expiryTime)
				ThrowErr(exPipe, kOSErrMessageTimedOut);
		}
	}
}


int
CTaskSafeRingBuffer::copyOut(UByte * ioBuf, size_t & ioLen)
{
	int result = 0;
	if (ioLen > 0)
	{
		UByte *	p1;
		size_t	n1;
		UByte *	p2;
		size_t	n2;
		acquire();
		computeGetVectors(p1, n1, p2, n2);
		if (p2 != NULL && n2 > 0)
		{
			if (ioLen < n2)
				n2 = ioLen;
			memmove(ioBuf, p2, n2);
			ioBuf += n2;
			ioLen -= n2;
			fGetPtr += n2;
			if (fGetPtr == fBufEnd)
				fGetPtr = fBuf;
		}
		if (p1 != NULL && n1 > 0)
		{
			if (ioLen < n1)
				n1 = ioLen;
			memmove(ioBuf, p1, n1);
			ioLen -= n1;
			fGetPtr += n1;
		}
		if (fGetPtr == fPutPtr)
			result = -1;
		release();
	}
	return result;
}

#pragma mark -

int
CTaskSafeRingBuffer::put(int inCh)
{
	checkPutSignal();
	acquire();
	if (!isFull())
	{
		*fPutPtr++ = inCh;
		if (fPutPtr == fBufEnd)
			fPutPtr = fBuf;
	}
	else
		inCh = -1;
	release();
	return inCh;
}


void
CTaskSafeRingBuffer::putCompletely(int inCh, Timeout interval, Timeout inTimeout)
{
	int		ch;
	CTime		expiryTime(TimeFromNow(inTimeout));
	checkPutSignal();
	while ((ch = put(inCh)) == -1)
	{
		pause(interval);
		if (inTimeout != kNoTimeout
		&& GetGlobalTime() > expiryTime)
			ThrowErr(exPipe, kOSErrMessageTimedOut);
	}
	return ch;
}


size_t
CTaskSafeRingBuffer::putn(const UByte * inBuf, size_t inLen)
{
	size_t	n = inLen;
	checkPutSignal();
	copyIn(inBuf, n);
	return inLen - n;	// length still remaining
}


void
CTaskSafeRingBuffer::putnCompletely(const UByte * inBuf, size_t inLen, Timeout interval, Timeout inTimeout)
{
	CTime		expiryTime(TimeFromNow(inTimeout));
	size_t	nRemaining = inLen;
	size_t	nDone = 0;
	while (nRemaining > 0)
	{
		checkPutSignal();
		copyIn(inBuf + nDone, nRemaining);
		if (nRemaining > 0)
		{
			nDone = inLen - nRemaining;
			pause(interval);
			if (inTimeout != kNoTimeout
			&& GetGlobalTime() > expiryTime)
				ThrowErr(exPipe, kOSErrMessageTimedOut);
		}
	}
}


int
CTaskSafeRingBuffer::copyIn(const UByte * ioBuf, size_t & ioLen)
{
	int result = 0;
	if (ioLen > 0)
	{
		UByte *	p1;
		size_t	n1;
		UByte *	p2;
		size_t	n2;
		acquire();
		computePutVectors(p1, n1, p2, n2);
		if (p2 != NULL && n2 > 0)
		{
			if (ioLen < n2)
				n2 = ioLen;
			memmove(p2, ioBuf, n2);
			ioBuf += n2;
			ioLen -= n2;
			fPutPtr += n2;
			if (fPutPtr == fBufEnd)
				fPutPtr = fBuf;
		}
		if (ioLen > 0 && p1 != NULL && n1 > 0)
		{
			if (ioLen < n1)
				n1 = ioLen;
			memmove(p1, ioBuf, n1);
			ioLen -= n1;
			fPutPtr += n1;
		}
		if (isFull())
			result = -1;
		release();
	}
	return result;
}

#pragma mark -

void
CTaskSafeRingBuffer::reset(void)
{
	checkPutSignal();
	checkGetSignal();
	acquire();
	fPutPtr = fGetPtr = fBuf;
	release();
}


size_t
CTaskSafeRingBuffer::getSize(void)
{
	size_t theSize;
	acquire();
	theSize = fBufLen - 1;
	release();
	return theSize;
}


bool
CTaskSafeRingBuffer::atEOF(void)
{
	return isEmpty() || isFull();
}


bool
CTaskSafeRingBuffer::isFull(void)
{
	bool full;
	UByte *	p;
	checkPutSignal();
	checkGetSignal();
	acquire();
	p = fGetPtr;
	if (p == fBuf)
		p = fBufEnd;
	p--;
	full = (p == fPutPtr);
	release();
	return full;
}


bool
CTaskSafeRingBuffer::isEmpty(void)
{
	bool empty;
	checkPutSignal();
	checkGetSignal();
	acquire();
	empty = (fGetPtr == fPutPtr);
	release();
	return empty;
}


size_t
CTaskSafeRingBuffer::freeCount(void)
{
	size_t	count;
	checkPutSignal();
	checkGetSignal();
	acquire();
	count = fGetPtr - fPutPtr - 1;
	if (fGetPtr <= fPutPtr)
		count += fBufLen;
	release();
	return count;
}


size_t
CTaskSafeRingBuffer::dataCount(void)
{
	size_t	count;
	checkPutSignal();
	checkGetSignal();
	acquire();
	count = fPutPtr - fGetPtr;
	if (fPutPtr < fGetPtr)
		count += fBufLen;
	release();
	return count;
}

#pragma mark -

void
CTaskSafeRingBuffer::computePutVectors(UByte *& outArg1, size_t & outArg2, UByte *& outArg3, size_t & outArg4)
{
	UByte * p = (fGetPtr == fBuf) ? fBufEnd - 1 : fGetPtr - 1;
	if (fPutPtr == fGetPtr)
	{
		if (fPutPtr == fBuf
		||  fPutPtr == p)
		{
			outArg1 = NULL;
			outArg2 = 0;
		}
		else
		{
			outArg1 = fBuf;
			outArg2 = p - fBuf;
		}
		if (fPutPtr > p)
		{
			outArg3 = fPutPtr;
			outArg4 = fBufEnd - fPutPtr;
		}
		else
		{
			outArg3 = fPutPtr;
			outArg4 = p - fPutPtr;
		}
	}
	else if (fPutPtr == p)
	{
		outArg1 = NULL;
		outArg2 = 0;
		outArg3 = NULL;
		outArg4 = 0;
	}
	else if (fPutPtr < p)
	{
		outArg1 = NULL;
		outArg2 = 0;
		outArg3 = fPutPtr;
		outArg4 = p - fPutPtr;
	}
	else
	{
		outArg1 = fBuf;
		outArg2 = p - fBuf;
		outArg3 = fPutPtr;
		outArg4 = fBufEnd - fPutPtr;
	}
}


void
CTaskSafeRingBuffer::computeGetVectors(UByte *& outArg1, size_t & outArg2, UByte *& outArg3, size_t & outArg4)
{
	UByte * p = (fGetPtr == fBuf) ? fBufEnd - 1 : fGetPtr - 1;
	if (fGetPtr == fPutPtr)
	{
		outArg1 = NULL;
		outArg2 = 0;
		outArg3 = NULL;
		outArg4 = 0;
	}
	else if (fPutPtr == p)
	{
		if (fPutPtr < fGetPtr)
		{
			outArg1 = fBuf;
			outArg2 = fPutPtr - fBuf;
			outArg3 = fGetPtr;
			outArg4 = fBufEnd - fGetPtr;
		}
		else
		{
			outArg1 = NULL;
			outArg2 = 0;
			outArg3 = fGetPtr;
			outArg4 = fPutPtr - fGetPtr;
		}
	}
	else
	{
		if (fGetPtr < fPutPtr)
		{
			outArg1 = NULL;
			outArg2 = 0;
			outArg3 = fGetPtr;
			outArg4 = fPutPtr - fGetPtr;
		}
		else
		{
			outArg1 = fBuf;
			outArg2 = fPutPtr - fBuf;
			outArg3 = fGetPtr;
			outArg4 = fBufEnd - fGetPtr;
		}
	}
}


long
CTaskSafeRingBuffer::updatePutVector(long)
{ return 0; }


long
CTaskSafeRingBuffer::updateGetVector(long)
{ return 0; }

#pragma mark -

/*----------------------------------------------------------------------
	C T a s k S a f e R i n g P i p e
----------------------------------------------------------------------*/

CTaskSafeRingPipe::CTaskSafeRingPipe()
{
	fRingBuf = NULL;
	fIsBufOurs = NO;
	fRetryInterval = 0;
	fTimeout = 250*kMilliseconds;
}


CTaskSafeRingPipe::~CTaskSafeRingPipe()
{
	if (fIsBufOurs && fRingBuf != NULL)
		delete fRingBuf;
}


void
CTaskSafeRingPipe::init(CTaskSafeRingBuffer * inRingBuf, bool inTakeRingBufOwnership, Timeout inTimeout, Timeout interval)
{
	fRingBuf = inRingBuf;
	fIsBufOurs = inTakeRingBufOwnership;
	fRetryInterval = interval;
	fTimeout = inTimeout;
	reset();
}


void
CTaskSafeRingPipe::init(size_t inBufLen, Timeout inTimeout, Timeout interval, bool inArg4)
{
	NewtonErr	err;
	CTaskSafeRingBuffer * ringBuf;
	if ((ringBuf = new CTaskSafeRingBuffer) == NULL)
		ThrowErr(exPipe, MemError());
	if ((err = ringBuf->init(inBufLen, inArg4)) != noErr)
		ThrowErr(exPipe, err);
	fRingBuf = ringBuf;
	fIsBufOurs = YES;
	fTimeout = inTimeout;
	fRetryInterval = interval;
}


long
CTaskSafeRingPipe::readSeek(long inOffset, int inSelector)
{ return 0; }


long
CTaskSafeRingPipe::readPosition(void) const
{ return 0; }


long
CTaskSafeRingPipe::writeSeek(long inOffset, int inSelector)
{ return 0; }


long
CTaskSafeRingPipe::writePosition(void) const
{ return 0; }


void
CTaskSafeRingPipe::readChunk(void * outBuf, size_t & ioSize, bool & ioEOF)
{
	if (ioSize == 1)
		*(UByte*)outBuf = fRingBuf->getCompletely(fTimeout, fRetryInterval);
	else
		fRingBuf->getnCompletely((UByte *)outBuf, ioSize, fTimeout, fRetryInterval);
}


void
CTaskSafeRingPipe::writeChunk(const void * inBuf, size_t inSize, bool inFlush)
{
	if (inSize == 1)
		fRingBuf->putCompletely(*(UByte*)inBuf, fTimeout, fRetryInterval);
	else
		fRingBuf->putnCompletely((const UByte *)inBuf, inSize, fTimeout, fRetryInterval);
}


void
CTaskSafeRingPipe::flushRead(void)
{ }


void
CTaskSafeRingPipe::flushWrite(void)
{ }


void
CTaskSafeRingPipe::reset(void)
{
	if (fRingBuf)
		fRingBuf->reset();
}


void
CTaskSafeRingPipe::overflow()
{ }


void
CTaskSafeRingPipe::underflow(long, bool&)
{ }

