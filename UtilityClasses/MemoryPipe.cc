/*
	File:		MemoryPipe.cc

	Contains:	The CMemoryPipe class implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "MemoryPipe.h"


/*----------------------------------------------------------------------
	C M e m o r y P i p e
	vt @ 0001A684
----------------------------------------------------------------------*/

CMemoryPipe::CMemoryPipe()
{ }


CMemoryPipe::~CMemoryPipe()
{ }


// pipe interface

void
CMemoryPipe::flushRead(void)
{
	if (fGetBuf)
		fGetBuf->seek(0, kSeekFromEnd);
}


void
CMemoryPipe::flushWrite(void)
{
	if (fPutBuf)
		fPutBuf->reset();
}


void
CMemoryPipe::reset(void)
{
	CBufferPipe::reset();
	if (fGetBuf)
		fGetBuf->seek(0, kSeekFromBeginning);
}


void
CMemoryPipe::overflow(void)
{
	if (fPutBuf)
		fPutBuf->reset();
}


void
CMemoryPipe::underflow(long inArg1, bool & ioArg2)
{
	ioArg2 = false;
}
