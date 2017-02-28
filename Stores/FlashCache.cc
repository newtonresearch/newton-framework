/*
	File:		FlashCache.cc

	Contains:	Flash store lookup cache implementation.
	
	Written by:	Newton Research Group, 2010.
*/

#include "FlashCache.h"
#include "FlashStore.h"


/*------------------------------------------------------------------------------
	Hash a PSSId.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

ULong
HashPSSId(PSSId inObjectId)
{
	return (((inObjectId ^ (inObjectId >> 24)) ^ (inObjectId >> 16)) ^ (inObjectId >> 8));
}


#pragma mark -

/*------------------------------------------------------------------------------
	F l a s h S t o r e L o o k u p C a c h e
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Construct cache.
	FIXME: cache allocation failure
	Args:		inCacheSize		MUST be a power of 2
------------------------------------------------------------------------------*/

CFlashStoreLookupCache::CFlashStoreLookupCache(ArrayIndex inCacheSize)
{
	fCacheSize = inCacheSize;
	fIndex = 0;
	fSetSize = 8;
	fCache = (FlashStoreLookupCacheEntry *)NewPtr(fCacheSize * sizeof(FlashStoreLookupCacheEntry));
	if (fCache)
		memset(fCache, 0, fCacheSize * sizeof(FlashStoreLookupCacheEntry));
}


/*------------------------------------------------------------------------------
	Destroy cache.
------------------------------------------------------------------------------*/

CFlashStoreLookupCache::~CFlashStoreLookupCache()
{
	if (fCache)
		FreePtr((Ptr)fCache);
}


/*------------------------------------------------------------------------------
	Look up entry.
	Args:		inObjectId
				inState
	Return:	directory entry address
------------------------------------------------------------------------------*/

ZAddr
CFlashStoreLookupCache::lookup(PSSId inObjectId, int inState)
{
	ArrayIndex cacheIndex = HashPSSId(inObjectId) & (fCacheSize - 1);
	FlashStoreLookupCacheEntry * entry = fCache + (cacheIndex & ~fSetSize);
	for (ArrayIndex i = 0; i < fSetSize; ++i, ++entry)
	{
		if (entry->matches(inObjectId, inState))
			return entry->fDirEntryAddr;
	}
	return kIllegalZAddr;
}


/*------------------------------------------------------------------------------
	Add an entry.
	Args:		ioObj
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStoreLookupCache::add(CStoreObjRef & ioObj)
{
	add(ioObj.fObj.id, ioObj.getDirEntryOffset(), ioObj.state());
}


/*------------------------------------------------------------------------------
	Add an entry.
	Args:		inObjectId
				inDirAddr
				inState
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStoreLookupCache::add(PSSId inObjectId, ZAddr inDirAddr, int inState)
{
	ArrayIndex cacheIndex = HashPSSId(inObjectId) & (fCacheSize - 1);
	FlashStoreLookupCacheEntry * entry = fCache + (cacheIndex & ~fSetSize);
	for (ArrayIndex i = 0; i < fSetSize; ++i, ++entry)
	{
		if (entry->matches(inObjectId, inState))
		{
			entry->fDirEntryAddr = inDirAddr;
			return;
		}
	}
	// didn’t find an existing entry -- use next sequential entry
	entry = fCache + (cacheIndex & ~fSetSize) + fIndex;
	entry->fId = inObjectId;
	entry->fDirEntryAddr = inDirAddr;
	entry->fState = inState;

	fIndex = (fIndex + 1) & (fSetSize - 1);
}


/*------------------------------------------------------------------------------
	Change an entry.
	Args:		ioObj
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStoreLookupCache::change(CStoreObjRef & ioObj)
{
	change(ioObj.fObj.id, ioObj.getDirEntryOffset(), ioObj.state());
}


/*------------------------------------------------------------------------------
	Change an entry.
	Args:		inObjectId
				inDirOffset
				inState
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStoreLookupCache::change(PSSId inObjectId, ZAddr inDirAddr, int inState)
{
	forget(inObjectId, inState);
	add(inObjectId, inDirAddr, inState);
}


/*------------------------------------------------------------------------------
	Forget an entry.
	Args:		inObjectId
				inState
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStoreLookupCache::forget(PSSId inObjectId, int inState)
{
	ArrayIndex cacheIndex = HashPSSId(inObjectId) & (fCacheSize - 1);
	FlashStoreLookupCacheEntry * entry = fCache + (cacheIndex & ~fSetSize);
	for (ArrayIndex i = 0; i < fSetSize; ++i, ++entry)
	{
		if (entry->fId == inObjectId)
		{
			entry->fId = 0;
			entry->fDirEntryAddr = 0;	// really? not kIllegalZAddr?
		}
	}
}


/*------------------------------------------------------------------------------
	Forget all entries.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CFlashStoreLookupCache::forgetAll(void)
{
	memset(fCache, 0, fCacheSize * sizeof(FlashStoreLookupCacheEntry));
}


#pragma mark -

/*------------------------------------------------------------------------------
	F l a s h S t o r e L o o k u p C a c h e E n t r y
------------------------------------------------------------------------------*/

bool
FlashStoreLookupCacheEntry::matches(PSSId inId, int inState)
{
	if (inId == fId)
	{
		if (inState == fState)
			return true;
		if (inState == 0)
		{
			switch (fState)
			{
			case 3:
			case 10:
			case 4:
			case 11:
			case 6:
			case 13:
				return true;
			}
		}
	}
	return false;
}

