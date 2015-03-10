/*
	File:		FlashCache.h

	Contains:	Flash store lookup cache declarations.
	
	Written by:	Newton Research Group, 2010.
*/

#if !defined(__FLASHCACHE_H)
#define __FLASHCACHE_H 1

#include "StoreObjRef.h"

/*------------------------------------------------------------------------------
	F l a s h S t o r e L o o k u p C a c h e E n t r y
------------------------------------------------------------------------------*/

struct FlashStoreLookupCacheEntry
{
	bool matches(PSSId inObjectId, int inState);

	PSSId		fId;
	ZAddr		fDirEntryAddr;
	int		fState;
};


/*------------------------------------------------------------------------------
	F l a s h S t o r e L o o k u p C a c h e
	8-way set associative cache.
------------------------------------------------------------------------------*/

class CFlashStoreLookupCache
{
public:
					CFlashStoreLookupCache(ArrayIndex inCacheSize);
					~CFlashStoreLookupCache();

	ZAddr			lookup(PSSId inObjectId, int inState);
	void			add(CStoreObjRef & ioObj);
	void			add(PSSId inObjectId, ZAddr inDirAddr, int inState);
	void			change(CStoreObjRef & ioObj);
	void			change(PSSId inObjectId, ZAddr inDirAddr, int inState);
	void			forget(PSSId inObjectId, int inState);
	void			forgetAll(void);

private:
	FlashStoreLookupCacheEntry *	fCache;
	ArrayIndex	fCacheSize;		// must be a power of 2
	ArrayIndex	fIndex;
	ArrayIndex	fSetSize;
};


#endif	/* __FLASHCACHE_H */
