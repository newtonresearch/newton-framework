/*
	File:		CachedReadStore.h

	Contains:	Store interface for cached reads only.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__CACHEDREADSTORE_H)
#define __CACHEDREADSTORE_H 1

#include "Frames.h"
#include "Store.h"

/*------------------------------------------------------------------------------
	C C a c h e d R e a d S t o r e
------------------------------------------------------------------------------*/
#define kUseObjectSize 0xFFFFFFFF

class CCachedReadStore
{
public:
					CCachedReadStore();
					CCachedReadStore(CStore * inStore, PSSId inId, size_t inSize);
					~CCachedReadStore();

	NewtonErr	init(CStore * inStore, PSSId inId, size_t inSize);
	NewtonErr	getDataPtr(size_t inOffset, size_t inSize, void ** outData);
	size_t		getDataSize(void) const;

private:
	char		fStaticBuf[KByte];
	char *	fDataPtr;			//+400
	CStore *	fStore;				//+404
	PSSId		fObjId;				//+408
	size_t	fDataSize;			//+40C
	bool		fIsCached;			//+410
	char *	fAllocdBuf;			//+414;
	size_t	fAllocdBufSize;	//+418
};

inline	size_t	CCachedReadStore::getDataSize(void) const
{ return fDataSize; }

#endif	/* __CACHEDREADSTORE_H */
