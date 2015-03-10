/*
	File:		CircleBuf.cc

	Contains:	Circle buffer implementation.

	Written by:	Newton Research Group, 2009.
*/

#include "CircleBuf.h"
#include "OSErrors.h"

/*------------------------------------------------------------------------------
	C C i r c l e B u f
	A circular FIFO buffer for communications services.
	NewtonErr return values can contain status codes:
		0	OK
		1
		2	buffer is empty
		3	buffer is full
		4	markers are full
------------------------------------------------------------------------------*/

CCircleBuf::CCircleBuf()
{
	fBuf = NULL;
	fMarkers = NULL;
}


CCircleBuf::~CCircleBuf()
{
	deallocate();
}


NewtonErr
CCircleBuf::allocate(ArrayIndex inSize)
{
	return allocate(inSize, NO, eNormalBuffer, 0);
}


NewtonErr
CCircleBuf::allocate(ArrayIndex inSize, int inArg2, EBufferResidence inResidence, UChar inArg4)
{
	//args r5 r6 r8 r9
	NewtonErr err = noErr;

	// if we already have a buffer, deallocate it first
	if (fBuf)
		deallocate();

	// calculate sizes required
	ArrayIndex bufSize = LONGALIGN(inSize);		// r5
	ArrayIndex markerSize = 0;	// r0
	ArrayIndex numOfMarkers = inArg2 + 1;
	if (inArg2 != 0)
		markerSize = numOfMarkers * sizeof(MarkerInfo);
	ArrayIndex allocSize = bufSize + markerSize;

	XTRY
	{
		// allocate the buffer
		if (inResidence == eWiredBuffer)
		{
			// it’s wired -- there’s a page size limit
			if (allocSize > 4032)
			{
				allocSize = 4032;
				bufSize = allocSize - markerSize;
			}
#if defined(correct)
			fBuf = (UByte *)NewWiredPtr(allocSize);
#endif
		}
#if defined(correct)
		else
#endif
			fBuf = (UByte *)NewPtr(allocSize);
		XFAILNOT(fBuf, err = kOSErrNoMemory;)

		// lock it down if required
		if (inResidence == eLockedBuffer)
			XFAIL(err = LockPtr((Ptr)fBuf))
#if defined(correct)
		if (inArg4 & 0x02)
			err = LockHeapRange(this, this+1, (inArg4 & 0x04) != 0);
#endif
	}
	XENDTRY;

	// reset indexes
	fBufferResidence = inResidence;
	f15 = inArg4;
	fGetIndex = 0;
	fPutIndex = 0;
	fBufLen = bufSize;
	f10 = 0;
	fGetMarkerIndex = 0;
	fPutMarkerIndex = 0;
	fMarkers = NULL;
	fNumOfMarkers = 0;

	// set up markers if required
	if (inArg2 != 0)
	{
		fNumOfMarkers = numOfMarkers;
		fMarkers = (MarkerInfo *)(fBuf + bufSize);
		for (ArrayIndex i = 0; i < fNumOfMarkers; ++i)
		{
			fMarkers[i].x00 = -1;
			fMarkers[i].x04 = 0;
		}
	}

	return err;
}


void
CCircleBuf::deallocate(void)
{
	if (fBuf)
	{
#if defined(correct)
		if (fBufferResidence == eWiredBuffer)
			FreeWiredPtr((Ptr)fBuf);
		else
#endif
		{
			if (fBufferResidence == eLockedBuffer)
				UnlockPtr((Ptr)fBuf);
			FreePtr((Ptr)fBuf);
		}
		fBuf = NULL;
#if 0
		if (f15 & 0x02)
			UnlockHeapRange(this, this+1);
#endif
	}
}


void
CCircleBuf::reset(void)
{
	fGetMarkerIndex = 0;
	fPutMarkerIndex = 0;
	fGetIndex = 0;
	fPutIndex = 0;
}


void
CCircleBuf::resetStart(void)
{
	fGetIndex = 0;
}


ArrayIndex
CCircleBuf::bufferCount(void)
{
	ArrayIndex count = fPutIndex - fGetIndex;
	if (fPutIndex < fGetIndex)
		count += fBufLen;
	return count;
}


ArrayIndex
CCircleBuf::bufferSpace(void)
{
	ArrayIndex count = fGetIndex - fPutIndex - 1;
	if (fGetIndex <= fPutIndex)
		count += fBufLen;
	return count;
}


NewtonErr
CCircleBuf::bufferSpace(ArrayIndex inSpaceReqd)
{
	if (fMarkers && markerSpace() == 0)
		return 4;

	return bufferSpace() >= inSpaceReqd ? 0 : 3;
}


ArrayIndex
CCircleBuf::markerCount(void)
{
	ArrayIndex count = fPutMarkerIndex - fGetMarkerIndex;
	if (fPutMarkerIndex < fGetMarkerIndex)
		count += fNumOfMarkers;
	return count;
}


ArrayIndex
CCircleBuf::markerSpace(void)
{
	if (fMarkers == NULL)
		return 0;

	ArrayIndex count = fGetMarkerIndex - fPutMarkerIndex - 1;
	if (fGetMarkerIndex < fPutMarkerIndex)
		count += fNumOfMarkers;
	return count;
}


NewtonErr
CCircleBuf::copyIn(CBufferList * inBuf, ArrayIndex * ioSize)
{
	// copy as much as we can from the CBufferList
	ArrayIndex amtToCopy;
	if ((amtToCopy = bufferSpace()) != 0)
	{
		// our put index splits the available buffer into two parts
		// -- from the index to the end of the buffer
		// -- from the start of the buffer up to the get index
		ArrayIndex amtCopied;
		ArrayIndex spaceAfter = fBufLen - fPutIndex;
		if (fGetIndex == 0)
			spaceAfter--;
		UByte * p = fBuf + fPutIndex;
		if (amtToCopy <= spaceAfter)
			amtCopied = inBuf->getn(p, amtToCopy);
		else
		{
			amtCopied = inBuf->getn(p, spaceAfter);
			if (amtCopied == spaceAfter)
				amtCopied += inBuf->getn(fBuf, amtToCopy - spaceAfter);
		}
		updateEnd(amtCopied);
		*ioSize -= amtCopied;
		if (amtCopied == 0)
			return 5;
	}
	return noErr;
}


NewtonErr
CCircleBuf::copyIn(UByte * inBuf, ArrayIndex * ioSize, bool inArg3, ULong inArg4)
{
	NewtonErr status = noErr;
	ArrayIndex amtToCopy = *ioSize;
	ArrayIndex spaceAvailable = bufferSpace();
	if (amtToCopy > spaceAvailable)
	{
		amtToCopy = spaceAvailable;
		status = 3;
	}
	if (inArg3 && markerSpace() == 0)
		return 4;

	 *ioSize -= amtToCopy;
	 if (spaceAvailable == 0)
		return status;

	ArrayIndex spaceAfter = fBufLen - fPutIndex;
	if (fGetIndex == 0)
		spaceAfter--;
	UByte * p = fBuf + fPutIndex;
	if (amtToCopy <= spaceAfter)
		memmove(p, inBuf, amtToCopy);
	else
	{
		memmove(p, inBuf, spaceAfter);
		memmove(fBuf, inBuf + spaceAfter, amtToCopy - spaceAfter);
	}

	ArrayIndex index = fPutIndex + amtToCopy;
	if (index >= fBufLen)
		index -= fBufLen;
	if (inArg3)
		status = putEOMMark(index, inArg4);
	fPutIndex = index;

	return status;
}


NewtonErr
CCircleBuf::copyOut(CBufferList * outBuf, ArrayIndex * ioSize, ULong * outArg3)
{
	NewtonErr status = noErr;
	ArrayIndex amtToCopy;
	int isBoundByMarker = bufferCountToNextMarker(&amtToCopy);
	if (amtToCopy != 0)
	{
		ArrayIndex amtCopied;
		ArrayIndex spaceAfter = fBufLen - fGetIndex;
		const UByte * p = fBuf + fGetIndex;
		if (amtToCopy <= spaceAfter)
			amtCopied = outBuf->putn(p, amtToCopy);
		else
		{
			amtCopied = outBuf->putn(p, spaceAfter);
			if (amtCopied == spaceAfter)
				amtCopied += outBuf->putn(fBuf, amtToCopy - spaceAfter);
		}
		*ioSize -= amtCopied;
		if (*ioSize == 0)
			status = 6;

		if (isBoundByMarker && amtCopied == amtToCopy)
		{
			status = 1;
			getEOMMark(outArg3);
			updateStart(amtCopied);
			if (f15 & 0x01)
				getAlignLong();
		}
		else
			updateStart(amtCopied);
	}
	return status;
}


NewtonErr
CCircleBuf::copyOut(UByte * outBuf, ArrayIndex * ioSize, ULong * outArg3)
{
	NewtonErr status = noErr;
	ArrayIndex amtToCopy;	// at this point, space available
	int isBoundByMarker = bufferCountToNextMarker(&amtToCopy);
	if (amtToCopy != 0)
	{
		if (amtToCopy > *ioSize)
		{
			amtToCopy = *ioSize;
			isBoundByMarker = 0;
			status = 6;
		}
		*ioSize -= amtToCopy;

		ArrayIndex amtCopied;
		ArrayIndex spaceAfter = fBufLen - fGetIndex;
		UByte * p = fBuf + fGetIndex;
		if (amtToCopy <= spaceAfter)
			memmove(outBuf, p, amtToCopy);
		else
		{
			memmove(outBuf, p, spaceAfter);
			memmove(outBuf + spaceAfter, fBuf, amtToCopy - spaceAfter);
		}
		*ioSize -= amtCopied;
		if (*ioSize == 0)
			status = 6;

		if (isBoundByMarker)
		{
			status = 1;
			getEOMMark(outArg3);
			updateStart(amtCopied);
			if (f15 & 0x01)
				getAlignLong();
		}
		else
			updateStart(amtCopied);
	}
	return status;
}


void
CCircleBuf::updateStart(ArrayIndex inDelta)
{
	ArrayIndex index = fGetIndex + inDelta;
	if (index >= fBufLen)
		index -= fBufLen;
	fGetIndex = index;
}


void
CCircleBuf::updateEnd(ArrayIndex inDelta)
{
	ArrayIndex index = fPutIndex + inDelta;
	if (index >= fBufLen)
		index -= fBufLen;
	fPutIndex = index;
}


void
CCircleBuf::getAlignLong(void)
{
	ULong misalignment = fGetIndex & 0x03;
	if (misalignment)
		updateStart(4 - misalignment);
}


void
CCircleBuf::putAlignLong(void)
{
	ULong misalignment = fPutIndex & 0x03;
	if (misalignment)
		updateEnd(4 - misalignment);
}


NewtonErr
CCircleBuf::putEOM(ULong inArg)
{
	return putEOMMark(fPutIndex, inArg);
}


NewtonErr
CCircleBuf::putNextEOM(ULong inArg)
{
	NewtonErr status;
	if ((status = putEOMMark(f10, inArg)) == 0)
	{
		fPutIndex = f10;
		if (f15 & 0x01)
			putAlignLong();
	}
	return status;
}


NewtonErr
CCircleBuf::putEOMMark(ULong inArg1, ULong inArg2)
{
	ArrayIndex i = fPutMarkerIndex;
	if (++i == fNumOfMarkers)
		i = 0;
	if (i == fGetMarkerIndex)
		return 4;

	fMarkers[fPutMarkerIndex].x00 = inArg1;
	fMarkers[fPutMarkerIndex].x04 = inArg2;
	fPutMarkerIndex = i;
	return noErr;
}


ULong
CCircleBuf::getEOMMark(ULong * outArg)
{
	if (fMarkers == NULL || fGetMarkerIndex == fPutMarkerIndex)
		return -1;

	ULong theMark;
	ArrayIndex i = fGetMarkerIndex;
	if (outArg)
		*outArg = fMarkers[i].x04;
	theMark = fMarkers[i].x00;
	fMarkers[i].x00 = -1;
	if (++i == fNumOfMarkers)
		i = 0;
	fGetMarkerIndex = i;
	return theMark;
}


ULong
CCircleBuf::peekNextEOMIndex(void)
{
	if (fMarkers == NULL)
		return -1;

	return fMarkers[fGetMarkerIndex].x00;
}


ULong
CCircleBuf::peekNextEOMIndex(ULong * outArg)
{
	if (fMarkers == NULL)
		return -1;

	*outArg = fMarkers[fGetMarkerIndex].x04;
	return fMarkers[fGetMarkerIndex].x00;
}


NewtonErr
CCircleBuf::bufferCountToNextMarker(ULong * outCount)
{
	NewtonErr status = noErr;
	ArrayIndex markerIndex = peekNextEOMIndex();
	ArrayIndex theCount = fPutIndex - fGetIndex;
	if (fPutIndex < fGetIndex)
		theCount += fBufLen;		// count of entire buffer
	if (markerIndex != -1)
	{
		// we have a marker
		ArrayIndex countToMarker = markerIndex - fGetIndex;
		if (markerIndex < fGetIndex)
			countToMarker += fBufLen;
		if (countToMarker <= theCount)
		{
			theCount = countToMarker;
			status = 1;
		}
	}
	*outCount = theCount;
	return status;
}


NewtonErr
CCircleBuf::flushToNextMarker(ULong * outMarker)
{
	ULong mark;
	if ((mark = getEOMMark(outMarker)) == -1)
		return 7;

	fGetIndex = mark;
	return noErr;
}


NewtonErr
CCircleBuf::getBytes(CCircleBuf * inBuf)
{
	NewtonErr status;

	ArrayIndex i = fPutIndex;

	ArrayIndex srcStopIndex = inBuf->fPutIndex;
	ArrayIndex srcIndex = inBuf->fGetIndex;
	UByte * srcBuf = inBuf->fBuf;
	ArrayIndex srcBufLen = inBuf->fBufLen;

	for ( ; ; )
	{
		if (srcIndex == srcStopIndex)
		{
			status = 2;	// copy stopped because source is empty
			break;
		}

		fBuf[i] = srcBuf[srcIndex];
		if (++srcIndex == srcBufLen)
			srcIndex = 0;
		if (++i == fBufLen)
			i = 0;

		if (i == fGetIndex)
		{
			status = 3;	// copy stopped because we’re full
			break;
		}
	}
	fPutIndex = i;
	inBuf->fGetIndex = srcStopIndex;

	return status;
}


NewtonErr
CCircleBuf::getNextByte(UByte * outByte)
{
	if (fMarkers)
	{
		ULong sp00;
		return getNextByte(outByte, &sp00);
	}

	ArrayIndex i = fGetIndex;
	if (i == fPutIndex)
		return 2;

	*outByte = fBuf[i];
	if (++i == fBufLen)
		i = 0;
	fGetIndex = i;
	return noErr;
}


NewtonErr
CCircleBuf::getNextByte(UByte * outByte, ULong * outArg2)
{
	ArrayIndex i = fGetIndex;
	if (i == fPutIndex)
		return 2;

	*outByte = fBuf[i];
	if (++i == fBufLen)
		i = 0;
	if (i == peekNextEOMIndex())
	{
		getEOMMark(outArg2);
		fGetIndex = i;
		return 1;
	}
	fGetIndex = i;
	return noErr;
}


NewtonErr
CCircleBuf::peekNextByte(UByte * outByte)
{
	ArrayIndex i = fGetIndex;
	if (i == fPutIndex)
		return 2;

	*outByte = fBuf[i];
	return noErr;
}


NewtonErr
CCircleBuf::peekNextByte(UByte * outByte, ULong * outArg2)
{
	ArrayIndex i = fGetIndex;
	if (i == fPutIndex)
		return 2;

	*outByte = fBuf[i];

	if (++i == fBufLen)
		i = 0;
	if (i == peekNextEOMIndex(outArg2))
		return 1;

	return noErr;
}


NewtonErr
CCircleBuf::peekFirstLong(ULong * outLong)
{
	*outLong = fBuf[fPutIndex];	// as long
	return noErr;
}


NewtonErr
CCircleBuf::putFirstPossible(UByte inByte)
{
	f10 = fPutIndex;
	return putNextPossible(inByte);
}


NewtonErr
CCircleBuf::putNextPossible(UByte inByte)
{
	ArrayIndex i = f10;
	fBuf[i] = inByte;
	if (++i == fBufLen)
		i = 0;
	if (i == fGetIndex)
		return 3;

	f10 = i;
	return noErr;
}


NewtonErr
CCircleBuf::putNextStart(void)
{
	f10 = fPutIndex;
	return noErr;
}


NewtonErr
CCircleBuf::putNextCommit(void)
{
	fPutIndex = f10;
	return noErr;
}


NewtonErr
CCircleBuf::putNextByte(UByte inByte)
{
	ArrayIndex i = fPutIndex;
	fBuf[i] = inByte;
	if (++i == fBufLen)
		i = 0;
	if (i == fGetIndex)
		return 3;

	fPutIndex = i;
	return noErr;
}


NewtonErr
CCircleBuf::putNextByte(UByte inByte, ULong inArg2)
{
	if (markerSpace() == 0)
		return 4;

	ArrayIndex i = fPutIndex;
	fBuf[i] = inByte;
	if (++i == fBufLen)
		i = 0;
	if (i == fGetIndex)
		return 3;

	fPutIndex = i;
	return putEOMMark(i, inArg2);
}


void
CCircleBuf::flushBytes(void)
{
	ULong offset;
	while (getEOMMark(&offset) != -1)
		/* rip through ’em all */;
	fGetIndex = fPutIndex;
}


void
CCircleBuf::DMABufInfo(ULong*, ULong*, unsigned char*, unsigned char*)
{}

void
CCircleBuf::DMAGetInfo(ULong*)
{}

void
CCircleBuf::DMAGetUpdate(ULong)
{}

void
CCircleBuf::DMAPutInfo(ULong*, ULong*)
{}

void
CCircleBuf::DMAPutUpdate(ULong, unsigned char, ULong)
{}

