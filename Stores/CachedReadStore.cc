/*
	File:		CachedReadStore.cc

	Contains:	Store interface for cached reads only.

	Written by:	Newton Research Group, 2007.
*/

#include "Objects.h"
#include "CachedReadStore.h"
#include "OSErrors.h"

/*------------------------------------------------------------------------------
	C C a c h e d R e a d S t o r e
------------------------------------------------------------------------------*/

CCachedReadStore::CCachedReadStore()
{
	fDataPtr = NULL;
	fAllocdBuf = NULL;
	fIsCached = false;
}


CCachedReadStore::CCachedReadStore(CStore * inStore, PSSId inId, size_t inSize)
{
	fDataPtr = NULL;
	fAllocdBuf = NULL;
	init(inStore, inId, inSize);
}


CCachedReadStore::~CCachedReadStore()
{
	if (fDataPtr != NULL && fDataPtr != fStaticBuf)
		FreePtr(fDataPtr);
	if (fAllocdBuf != NULL)
		FreePtr(fAllocdBuf);
}


NewtonErr
CCachedReadStore::init(CStore * inStore, PSSId inId, size_t inSize)
{
	size_t	size;
	fStore = inStore;
	fObjId = inId;
	if (inSize == kUseObjectSize)
	{
		if (inStore->getObjectSize(inId, &size) != noErr)
			size = 0;
	}
	else
		size = inSize;
	fDataSize = size;
	if (fDataPtr != NULL && fDataPtr != fStaticBuf)
		FreePtr(fDataPtr);
	if (size < KByte)
		fDataPtr = fStaticBuf;
	else
		fDataPtr = NewPtr(size);
	fIsCached = false;
	return noErr;
}


NewtonErr
CCachedReadStore::getDataPtr(size_t inOffset, size_t inSize, void ** outData)
{
	if (fDataPtr != NULL
	&&  inOffset + inSize <= fDataSize)
	{
		if (!fIsCached)
		{
			if (fStore->read(fObjId, 0, fDataPtr, fDataSize) != noErr)
				return kStoreErrObjectOverRun;
			fIsCached = true;
		}
		*outData = fDataPtr + inOffset;
		return noErr;
	}

	if (inSize < KByte)
		*outData = fStaticBuf;
	else
	{
		if (fAllocdBuf == NULL  ||  fAllocdBufSize < inSize)
		{
			char * newBuf;
			if ((newBuf = ReallocPtr(fAllocdBuf, inSize)) == NULL)
				return MemError();
			fAllocdBuf = newBuf;
			fAllocdBufSize = inSize;
		}
		*outData = fAllocdBuf;
	}
	return fStore->read(fObjId, inOffset, *outData, inSize);
}

