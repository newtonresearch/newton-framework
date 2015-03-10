/*
	File:		EndpointPipe.cc

	Contains:	Endpoint pipe implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "EndpointPipe.h"


CEndpointPipe::CEndpointPipe()
{
	fEndpoint = NULL;
	fTimeout = kNoTimeout;
	fFraming = NO;
	fCallback = NULL;
	fNumOfBytesRead = 0;
	fNumOfBytesWritten = 0;
	isAborted = NO;
}


CEndpointPipe::~CEndpointPipe()
{ }


void
CEndpointPipe::init(CEndpoint * inEndpoint, size_t inGetBufSize, size_t inPutBufSize, Timeout inTimeout, bool)
{ /* this really does nothing */ }


NewtonErr
CEndpointPipe::init(CEndpoint * inEndpoint, size_t inGetBufSize, size_t inPutBufSize, Timeout inTimeout, bool inFraming, CPipeCallBack * inCalback)
{
	init(inGetBufSize, inPutBufSize);
	fEndpoint = inEndpoint;
	fTimeout = inTimeout;
	fFraming = inFraming;
	fCallback = inCallback;
}


NewtonErr
CEndpointPipe::addToAppWorld(void)
{
	NewtonErr err;
	
	if (fEndpoint == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);
	if (isAborted)
		ThrowErr(exPipe, kOSErrCallAborted);

	if (err = fEndpoint->addToAppWorld())
		ThrowErr(exPipe, err);

	return err;
}


NewtonErr
CEndpointPipe::removeFromAppWorld(void)
{
	NewtonErr err;

	if (fEndpoint == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);
	if (isAborted)
		ThrowErr(exPipe, kOSErrCallAborted);

	if (err = fEndpoint->removeFromAppWorld())
		ThrowErr(exPipe, err);

	return err;
}


void
CEndpointPipe::resetRead(void)
{
	fNumOfBytesRead = 0;
	CBufferPipe::resetRead();
}


void
CEndpointPipe::resetWrite(void)
{
	fNumOfBytesWritten = 0;
	CBufferPipe::resetWrite();
}


void
CEndpointPipe::flushRead(void)
{
	if (fEndpoint == NULL || fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);
	if (isAborted)
		ThrowErr(exPipe, kOSErrCallAborted);

	fGetBuf->reset();
	fGetBuf->seek(0, kSeekFromEnd);
	f0D = NO;
}


void
CEndpointPipe::flushWrite(void)
{
	if (fEndpoint == NULL || fPutBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);
	if (isAborted)
		ThrowErr(exPipe, kOSErrCallAborted);

	// hide the unused part of the buffer
	ULong posn = fPutBuf->position();
	ULong bufSize = fPutBuf->getSize();
	if (posn < bufSize)
		fPutBuf->hide(bufSize - posn, kSeekFromEnd);

	// send the buffer from its beginning
	fPutBuf->seek(0, kSeekFromBeginning);
	ULong flags = fFraming ? 2 : 0;
	if (err = fEndpoint->nSnd(fPutBuf, flags, fTimeout))
	{
		fPutBuf->seek(0, kSeekFromEnd);
		ThrowErr(exPipe, err);
	}
	
	fPutBuf->reset();
}


void
CEndpointPipe::abort(void)
{
	if (fEndpoint)
	{
		if (fEndpoint->isPending(1))
			fEndpoint->nAbort(YES);
	}
	isAborted = YES;
}


void
CEndpointPipe::overflow(void)
{
	NewtonErr err;

	if (fEndpoint == NULL || fPutBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);
	if (isAborted)
		ThrowErr(exPipe, kOSErrCallAborted);

	ULong bufSize = fPutBuf->getSize();
	fPutBuf->seek(0, kSeekFromBeginning);
	ULong flags = fFraming ? 3 : 1;
	if (err = fEndpoint->nSnd(fPutBuf, flags, fTimeout))
	{
		fPutBuf->seek(0, kSeekFromEnd);
		ThrowErr(exPipe, err);
	}
	else
		fNumOfBytesWritten += bufSize;
	fPutBuf->reset();
	if (fCallback && (err = fCallback->cbFunc(fNumOfBytesRead, fNumOfBytesWritten)))
		ThrowErr(exPipe, err);
}


void
CEndpointPipe::underflow(long inArg1, bool & ioArg2)
{
	NewtonErr err;

	if (fEndpoint == NULL || fGetBuf == NULL)
		ThrowErr(exPipe, kUCErrNotInitialized);
	if (isAborted)
		ThrowErr(exPipe, kOSErrCallAborted);

	ULong flags = fFraming ? 2 : 0;
	fGetBuf->reset();
	if (inArg1 < fGetBuf->getSize())		// check this cmp
		inArg1 = fGetBuf->getSize();

	if (err = fEndpoint->nRcv(fGetBuf, inArg1, &flags, fTimeout))
	{
		fGetBuf->seek(0, kSeekFromEnd);
		ThrowErr(exPipe, err);
	}
	else
		fNumOfBytesRead += fGetBuf->getSize();
	ioArg2 = ((flags & 0x01) == 0);
	fGetBuf->seek(0, kSeekFromBeginning);
	if (fCallback && (err = fCallback->cbFunc(fNumOfBytesRead, fNumOfBytesWritten)))
		ThrowErr(exPipe, err);
}


void
CEndpointPipe::setTimeout(Timeout inTimeout)
{
	fTimeout = inTimeout;
}


Timeout
CEndpointPipe::getTimeout(void)
{
	return fTimeout;
}


void
CEndpointPipe::useFraming(bool inFraming)
{
	fFraming = inFraming;
}


bool
CEndpointPipe::usingFraming(void)
{
	return fFraming;
}
