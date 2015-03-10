/*
	File:		Pipes.cc

	Contains:	Implementation of pipes.

	Written by:	Newton Research Group.
*/

#include "LargeBinaries.h"
#include "ROMSymbols.h"


/*------------------------------------------------------------------------------
	C P i p e C a l l b a c k
------------------------------------------------------------------------------*/

CPipeCallback::CPipeCallback()
{
	f04 = 0xFFFFFFFF;
	f08 = 0xFFFFFFFF;
}

CPipeCallback::~CPipeCallback()
{ }

#pragma mark -

/*------------------------------------------------------------------------------
	C P i p e
------------------------------------------------------------------------------*/

CPipe::CPipe()
{ }

CPipe::~CPipe()
{ }


void
CPipe::resetRead()
{
	ThrowErr(exPipe, kUCErrNotImplemented);
}


void
CPipe::resetWrite()
{
	ThrowErr(exPipe, kUCErrNotImplemented);
}

/*	Input stream */

const CPipe &
CPipe::operator>>(char & ioValue)
{
	bool		isEOF;
	size_t	length = sizeof(char);
	readChunk(&ioValue, length, isEOF);
	if (isEOF && length < sizeof(char))
		ThrowErr(exPipe, -2);
	return *this;
}


const CPipe &
CPipe::operator>>(unsigned char & ioValue)
{
	bool		isEOF;
	size_t	length = sizeof(unsigned char);
	readChunk(&ioValue, length, isEOF);
	if (isEOF && length < sizeof(unsigned char))
		ThrowErr(exPipe, -2);
	return *this;
}


const CPipe &
CPipe::operator>>(short & ioValue)
{
	bool		isEOF;
	size_t 	length = sizeof(short);
	readChunk(&ioValue, length, isEOF);
	if (isEOF && length < sizeof(short))
		ThrowErr(exPipe, -2);
	ioValue = CANONICAL_SHORT(ioValue);
	return *this;
}


const CPipe &
CPipe::operator>>(unsigned short & ioValue)
{
	bool		isEOF;
	size_t	length = sizeof(unsigned short);
	readChunk(&ioValue, length, isEOF);
	if (isEOF && length < sizeof(unsigned short))
		ThrowErr(exPipe, -2);
	ioValue = CANONICAL_SHORT(ioValue);
	return *this;
}


const CPipe &
CPipe::operator>>(int & ioValue)
{
	bool		isEOF;
	size_t	length = sizeof(int);
	readChunk(&ioValue, length, isEOF);
	if (isEOF && length < sizeof(int))
		ThrowErr(exPipe, -2);
	ioValue = CANONICAL_LONG(ioValue);
	return *this;
}


const CPipe &
CPipe::operator>>(unsigned int & ioValue)
{
	bool		isEOF;
	size_t	length = sizeof(unsigned int);
	readChunk(&ioValue, length, isEOF);
	if (isEOF && length < sizeof(unsigned int))
		ThrowErr(exPipe, -2);
	ioValue = CANONICAL_LONG(ioValue);
	return *this;
}


const CPipe &
CPipe::operator>>(size_t & ioValue)
{
	unsigned int value;
	*this >> value;
	ioValue = value;
	return *this;
}


/*	Output stream */

const CPipe &
CPipe::operator<<(char ioValue)
{
	writeChunk(&ioValue, sizeof(char), NO);
	return *this;
}


const CPipe &
CPipe::operator<<(unsigned char ioValue)
{
	writeChunk(&ioValue, sizeof(unsigned char), NO);
	return *this;
}


const CPipe &
CPipe::operator<<(short ioValue)
{
	short	value = CANONICAL_SHORT(ioValue);
	writeChunk(&value, sizeof(value), NO);
	return *this;
}


const CPipe &
CPipe::operator<<(unsigned short ioValue)
{
	unsigned short	value = CANONICAL_SHORT(ioValue);
	writeChunk(&value, sizeof(value), NO);
	return *this;
}


const CPipe &
CPipe::operator<<(int ioValue)
{
	int	value = CANONICAL_LONG(ioValue);
	writeChunk(&value, sizeof(value), NO);
	return *this;
}


const CPipe &
CPipe::operator<<(unsigned int ioValue)
{
	unsigned int	value = CANONICAL_LONG(ioValue);
	writeChunk(&value, sizeof(value), NO);
	return *this;
}


const CPipe &
CPipe::operator<<(size_t & ioValue)
{
	unsigned int value = ioValue;
	return *this << value;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P t r P i p e
------------------------------------------------------------------------------*/

CPtrPipe::CPtrPipe()
	:	CPipe()
{
	fPtr = NULL;
	fOffset = 0;
	fEnd = 0;
	fCallback = 0;
	fPtrIsOurs = NO;
}

CPtrPipe::~CPtrPipe()
{
	if (fPtrIsOurs)
		FreePtr(fPtr);
}

void
CPtrPipe::init(size_t inSize, CPipeCallback * inCallback)
{
/*	if (inSize < 0)
		ThrowErr(exPipe, kUCErrBadSize);		// original allowed signed inSize */

	char * p = NewPtr(inSize);
	if (p == NULL)
		ThrowErr(exPipe, MemError());
	init(p, inSize, YES, inCallback);
}

void
CPtrPipe::init(void * inPtr, size_t inSize, bool inAssumePtrOwnership, CPipeCallback * inCallback)
{
	fPtr = (Ptr) inPtr;
	fOffset = 0;
	fEnd = inSize;
	fCallback = inCallback;
	fPtrIsOurs = inAssumePtrOwnership;
}

long
CPtrPipe::readSeek(long inOffset, int inSelector)
{
	return seek(inOffset, inSelector);
}

long
CPtrPipe::readPosition(void) const
{
	return fOffset;
}

long
CPtrPipe::writeSeek(long inOffset, int inSelector)
{
	return seek(inOffset, inSelector);
}

long
CPtrPipe::writePosition(void) const
{
	return fOffset;
}

void
CPtrPipe::readChunk(void * outBuf, size_t & ioSize, bool & outEOF)
{
	outEOF = NO;
	if (fEnd - fOffset < (long)ioSize)
		ThrowErr(exPipe, kUCErrUnderflow);
	memmove(outBuf, fPtr + fOffset, ioSize);
	fOffset += ioSize;
}

void
CPtrPipe::writeChunk(const void * inBuf, size_t inSize, bool inFlush)
{
	if (fEnd - fOffset < (long)inSize)
		ThrowErr(exPipe, kUCErrOverflow);
	memmove(fPtr + fOffset, inBuf, inSize);
	fOffset += inSize;
}

void
CPtrPipe::flushRead(void)
{ }

void
CPtrPipe::flushWrite(void)
{ }

void
CPtrPipe::reset(void)
{
	fOffset = 0;
}

void
CPtrPipe::overflow()
{
	ThrowErr(exPipe, kUCErrOverflow);
}

void
CPtrPipe::underflow(long inArg1, bool & ioArg2)
{
	ThrowErr(exPipe, kUCErrUnderflow);
}

long
CPtrPipe::seek(long inOffset, int inSelector)
{
	if (inOffset == 0)
	{
		if (inSelector == SEEK_SET)
			fOffset = 0;
		else if (inSelector == SEEK_END)
			fOffset = fEnd;
	}
	else
	{
		if (inSelector == SEEK_SET)
			fOffset = inOffset;
		else if (inSelector == SEEK_CUR)
			fOffset += inOffset;
		else if (inSelector == SEEK_END)
			fOffset = fEnd - inOffset;
	}
	return fOffset;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C R e f P i p e
------------------------------------------------------------------------------*/

CRefPipe::CRefPipe()
{
	fTheRef = NILREF;
}


CRefPipe::~CRefPipe()
{
	if (NOTNIL(fTheRef))
	{
		UnlockRef(fTheRef);
		fTheRef = NILREF;
	}
}


void
CRefPipe::initSink(size_t inSize, RefArg inStore, CPipeCallback * inCallback)
{
	if (NOTNIL(inStore))
		fTheRef = FLBAlloc(inStore, SYMA(binary), MAKEINT(inSize));
	else
		fTheRef = AllocateBinary(SYMA(binary), inSize);
	LockRef(fTheRef);
	CPtrPipe::init(BinaryData(fTheRef), inSize, NO, inCallback);
}


void
CRefPipe::initSource(RefArg inRef, CPipeCallback * inCallback)
{
	fTheRef = inRef;
	LockRef(fTheRef);
	CPtrPipe::init(BinaryData(fTheRef), Length(fTheRef), NO, inCallback);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S t d I O P i p e
------------------------------------------------------------------------------*/

CStdIOPipe::CStdIOPipe(const char * inFilename, const char * inMode)
	:	CPipe()
{
	if ((fFile = fopen(inFilename, inMode)) == NULL)
		ThrowErr(exPipe, -1);
	fState = 0;
}

CStdIOPipe::~CStdIOPipe()
{
//	flush();
	if (fclose(fFile) != 0)
		ThrowErr(exPipe, -2);
}

long
CStdIOPipe::readSeek(long inOffset, int inSelector)
{
	return seek(inOffset, inSelector);
}

long
CStdIOPipe::readPosition(void) const
{
	return ftell(fFile);
}

long
CStdIOPipe::writeSeek(long inOffset, int inSelector)
{
	return seek(inOffset, inSelector);
}

long
CStdIOPipe::writePosition(void) const
{
	return ftell(fFile);
}

void
CStdIOPipe::readChunk(void * outBuf, size_t & ioSize, bool & outEOF)
{
	if (fState == 2)
		fflush(fFile);
	fState = 1;
	size_t	sizeRead = fread(outBuf, 1, ioSize, fFile);
	if (sizeRead < ioSize)
	{
		if (ferror(fFile))
			ThrowErr(exPipe, -4);
		ioSize = sizeRead;
		outEOF = YES;
		ThrowErr(exPipe, -3);
	}
	outEOF = feof(fFile);
}

void
CStdIOPipe::writeChunk(const void * inBuf, size_t inSize, bool inFlush)
{
	fState = 2;
	if (fwrite(inBuf, 1, inSize, fFile) < inSize)
		ThrowErr(exPipe, -5);
	if (inFlush)
		flush();
}

void
CStdIOPipe::flushRead(void)
{
	flush();
}

void
CStdIOPipe::flushWrite(void)
{
	flush();
}

void
CStdIOPipe::flush(void)
{
	if (fflush(fFile) != 0)
		ThrowErr(exPipe, -5);
}

long
CStdIOPipe::seek(long inOffset, int inSelector)
{
	if (fseek(fFile, inOffset, inSelector) != 0)
		ThrowErr(exPipe, -6);
	return ftell(fFile);
}

void
CStdIOPipe::reset(void)
{
	if (fseek(fFile, 0, SEEK_SET) != 0)
		ThrowErr(exPipe, -6);
}

void
CStdIOPipe::overflow()
{ }

void
CStdIOPipe::underflow(long inArg1, bool & ioArg2)
{ }

