/*
	File:		BufferPipe.cc

	Contains:	The CBufferPipe class implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "BufferPipe.h"


/*----------------------------------------------------------------------
	C B u f f e r P i p e
	vt @ 0001B078
----------------------------------------------------------------------*/

CBufferPipe::CBufferPipe()
{
	fGetBuf = NULL;
	fPutBuf = NULL;
	fBufsAreOurs = false;
	f0D = false;
}


CBufferPipe::~CBufferPipe()
{
	if (fBufsAreOurs)
	{
		if (fGetBuf)
			fGetBuf->destroy();
		if (fPutBuf)
			fPutBuf->destroy();
	}
}


// initialisation

void
CBufferPipe::init(size_t inGetBufSize, size_t inPutBufSize)
{
	NewtonErr err = noErr;

	fBufsAreOurs = true;
	f0D = false;

	XTRY
	{
		if (inGetBufSize > 0)
		{
			fGetBuf = CBufferSegment::make();
			XFAILIF(fGetBuf == NULL, err = MemError();)
			XFAIL(err = fGetBuf->init(inGetBufSize))
			fGetBuf->seek(0, kSeekFromEnd);
		}
		if (inPutBufSize > 0)
		{
			fPutBuf = CBufferSegment::make();
			XFAILIF(fPutBuf == NULL, err = MemError();)
			XFAIL(err = fPutBuf->init(inPutBufSize))
		}
	}
	XENDTRY;

	XDOFAIL(err)
	{
		ThrowErr(exPipe, err);
	}
	XENDFAIL;
}


void
CBufferPipe::init(CBufferSegment * inGetBuf, CBufferSegment * inPutBuf, bool inDoFreeBuffer)
{
	fGetBuf = inGetBuf;
	fPutBuf = inPutBuf;
	fBufsAreOurs = inDoFreeBuffer;
	reset();
}


// pipe interface

long
CBufferPipe::readSeek(long inOffset, int inSelector)
{
	if (fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	return fGetBuf->seek(inOffset, inSelector);
}


long
CBufferPipe::readPosition(void) const
{
	if (fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	return fGetBuf->position();
}


long
CBufferPipe::writeSeek(long inOffset, int inSelector)
{
	if (fPutBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	return fPutBuf->seek(inOffset, inSelector);
}


long
CBufferPipe::writePosition(void) const
{
	if (fPutBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	return fPutBuf->position();
}


void
CBufferPipe::readChunk(void * outBuf, size_t & ioSize, bool & outEOF)
{
	if (fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	int result;
	size_t amtRemaining = ioSize;
	bool isEOF = fGetBuf->atEOF();
	outEOF = false;
	if (ioSize > 0 && !f0D)
	{
		result = fGetBuf->copyOut((UByte *)outBuf, amtRemaining);
		if (result == -1)
			isEOF = 1;
		else if (result != 0)
			ThrowErr(exPipe, result);
		while (amtRemaining > 0 && !f0D)
		{
			underflow(amtRemaining, f0D);
			isEOF = false;
			result = fGetBuf->copyOut((UByte *)outBuf + ioSize - amtRemaining, amtRemaining);
			if (result == -1)
				isEOF = 1;
			else if (result != 0)
				ThrowErr(exPipe, result);
		}
	}
	if (f0D)
	{
		if (isEOF)
		{
			f0D = false;
			outEOF = true;
		}
		else
		{
			if (amtRemaining > 0)
			{
				result = fGetBuf->copyOut((UByte *)outBuf, amtRemaining);
				if (result == -1)
				{
					f0D = false;
					outEOF = true;
				}
				else if (result != 0)
					ThrowErr(exPipe, result);
			}
		}
	}
	ioSize -= amtRemaining;
}


void
CBufferPipe::writeChunk(const void * inBuf, size_t inSize, bool inFlush)
{
	if (fPutBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	int result;
	size_t amtRemaining = inSize;
	if (inSize > 0)
	{
		result = fPutBuf->copyIn((const UByte *)inBuf, amtRemaining);
		if (!(result == -1 || result == 0))
			ThrowErr(exPipe, result);
		while (amtRemaining > 0)
		{
			overflow();
			result = fPutBuf->copyIn((const UByte *)inBuf + inSize - amtRemaining, amtRemaining);
			if (!(result == -1 || result == 0))
				ThrowErr(exPipe, result);
		}
	}
	if (inFlush)
		flushWrite();
}


void
CBufferPipe::reset(void)
{
	resetRead();
	resetWrite();
}


void
CBufferPipe::resetRead()
{
	f0D = false;
	if (fGetBuf)
	{
		fGetBuf->reset();
		fGetBuf->seek(0, kSeekFromEnd);
	}
}


void
CBufferPipe::resetWrite()
{
	if (fPutBuf)
		fPutBuf->reset();
}


// get primitives

int
CBufferPipe::peek(bool inArg)
{
	if (fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	int dataChar = fGetBuf->peek();
	if (inArg && dataChar == -1 && !f0D)
	{
		underflow(1, f0D);
		dataChar = fGetBuf->peek();
	}
	return dataChar;
}


int
CBufferPipe::next(void)
{
	if (fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	int dataChar = fGetBuf->next();
	if (dataChar == -1)
	{
		if (f0D)
			f0D = false;
		else
		{
			underflow(1, f0D);
			dataChar = fGetBuf->next();
		}
	}
	return dataChar;
}


int
CBufferPipe::skip(void)
{
	if (fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	int dataChar = fGetBuf->skip();
	if (dataChar == -1)
	{
		if (f0D)
			f0D = false;
		else
		{
			underflow(1, f0D);
			dataChar = fGetBuf->skip();
		}
	}
	return dataChar;
}


int
CBufferPipe::get(void)
{
	if (fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	int dataChar = fGetBuf->get();
	if (dataChar == -1)
	{
		if (f0D)
			f0D = false;
		else
		{
			underflow(1, f0D);
			dataChar = fGetBuf->get();
		}
	}
	return dataChar;
}


// put primitives

int
CBufferPipe::put(int dataByte)
{
	if (fPutBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);

	int dataChar = fPutBuf->put(dataByte);
	if (dataChar == -1)
	{
		overflow();
		dataChar = fPutBuf->put(dataByte);
	}
	return dataChar;
}

