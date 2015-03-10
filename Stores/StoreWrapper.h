/*
	File:		StoreWrapper.h

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#if !defined(__STOREWRAPPER_H)
#define __STOREWRAPPER_H 1

#include "DynamicArray.h"
#include "Store.h"
#include "Frames.h"
#include "NodeCache.h"

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C"
{
Ref	StoreAbort(RefArg inRcvr);
Ref	StoreLock(RefArg inRcvr);
Ref	StoreUnlock(RefArg inRcvr);
Ref	StoreCardSlot(RefArg inRcvr);
Ref	StoreCardType(RefArg inRcvr);
Ref	StoreCheckWriteProtect(RefArg inRcvr);
Ref	StoreIsReadOnly(RefArg inRcvr);
Ref	StoreIsValid(RefArg inRcvr);
Ref	StoreHasPassword(RefArg inRcvr);
Ref	StoreSetPassword(RefArg inRcvr, RefArg inPassword);
Ref	StoreGetKind(RefArg inRcvr);
Ref	StoreGetSignature(RefArg inRcvr);
Ref	StoreSetSignature(RefArg inRcvr, RefArg inSignature);
Ref	StoreGetName(RefArg inRcvr);
Ref	StoreSetName(RefArg inRcvr, RefArg inName);
Ref	StoreGetInfo(RefArg inRcvr, RefArg inTag);
Ref	StoreSetInfo(RefArg inRcvr, RefArg inTag, RefArg inValue);
Ref	StoreGetAllInfo(RefArg inRcvr);
Ref	StoreSetAllInfo(RefArg inRcvr, RefArg info);
Ref	StoreGetObjectSize(RefArg inRcvr, RefArg inObj);
Ref	StoreSetObjectSize(RefArg inRcvr, RefArg inObj, RefArg inSize);
Ref	StoreTotalSize(RefArg inRcvr);
Ref	StoreUsedSize(RefArg inRcvr);
Ref	StoreErase(RefArg inRcvr);
Ref	StoreCreateSoup(RefArg inRcvr, RefArg inName, RefArg indexes);
Ref	StoreCheckUnion(RefArg inRcvr);
Ref	StoreGetSoupNames(RefArg inRcvr);
Ref	StoreHasSoup(RefArg inRcvr, RefArg inName);
Ref	StoreGetSoup(RefArg inRcvr, RefArg inName);

Ref	StoreNewObject(RefArg inRcvr);
Ref	StoreNewVBO(RefArg inRcvr, RefArg inArg2);
Ref	StoreNewCompressedVBO(RefArg inRcvr, RefArg inArg2, RefArg inArg3, RefArg inArg4);
Ref	StoreReadObject(RefArg inRcvr, RefArg inArg2, RefArg inArg3);
Ref	StoreWriteObject(RefArg inRcvr, RefArg inArg2, RefArg inArg3, RefArg inArg4);
Ref	StoreWriteWholeObject(RefArg inRcvr, RefArg inArg2, RefArg inArg3, RefArg inArg4);
Ref	StoreDeleteObject(RefArg inRcvr, RefArg inObj);
Ref	StoreSuckPackageFromBinary(RefArg inRcvr, RefArg inBinary, RefArg inParams);
Ref	StoreSuckPackageFromEndpoint(RefArg inRcvr, RefArg inEndpoint, RefArg inParams);
Ref	StoreRestorePackage(RefArg inRcvr);
Ref	StoreRestoreSegmentedPackage(RefArg inRcvr, RefArg inArg2);
}

bool	IsRichString(RefArg inObj);
bool	IsValidStore(const CStore * inStore);
void	CheckWriteProtect(RefArg inRcvr);
long	DefaultSortTableId(void);

typedef uint32_t StoreRef;	// hi word = index into table for PSSId, lo word = offset into block read

/*------------------------------------------------------------------------------
	C S t o r e H a s h T a b l e
------------------------------------------------------------------------------*/

#define kStoreHashTableSize	64

class CStoreHashTableIterator;

class CStoreHashTable
{
public:
	static	PSSId	create(CStore * inStore);

				CStoreHashTable(CStore * inStore, PSSId inId);

	StoreRef	insert(ULong inHash, char * inData, size_t inSize);
	bool		get(StoreRef inRef, char * outData, size_t * ioSize);
	void		abort(void);
	size_t	totalSize(void) const;

private:
	friend class CStoreHashTableIterator;

	PSSId		fId;
	PSSId		fTable[kStoreHashTableSize];
	CStore *	fStore;
};


/*------------------------------------------------------------------------------
	C S t o r e H a s h T a b l e I t e r a t o r
------------------------------------------------------------------------------*/

class CStoreHashTableIterator
{
public:
				CStoreHashTableIterator(CStoreHashTable * inTable);

	void		next(void);
	void		getData(char * outData, size_t * ioSize);

private:
	CStoreHashTable *	fTable;
	ArrayIndex	fIndex;
	PSSId			fId;
	size_t		fTableEntrySize;
	ArrayIndex	fOffset;
	short			fSize;
	bool			fDone;
};


/*------------------------------------------------------------------------------
	C S t o r e W r a p p e r   S u p p o r t
------------------------------------------------------------------------------*/
class CStoreWrapper;

class CEphemeralTracker
{
public:
					CEphemeralTracker();
					~CEphemeralTracker();

	NewtonErr	init(CStoreWrapper * inStoreWrapper, PSSId inId);

	static ArrayIndex	find(PSSId inId, CDynamicArray*);
	static bool	findAndRemove(PSSId inId, CDynamicArray*);

	void			addEphemeral(PSSId inId);
	bool			isEphemeral(PSSId inId);
	void			removeEphemeral(PSSId inId);
	NewtonErr	deleteEphemeral(PSSId inId);
	NewtonErr	deleteEphemeral1(PSSId inId);
	NewtonErr	deleteAllEphemerals(void);
	NewtonErr	deletePendingEphemerals(void);

	NewtonErr	readEphemeralList(void);
	NewtonErr	writeEphemeralList(void);
	void			lockEphemerals(void);
	void			flushEphemerals(void);
	void			abortEphemerals(void);

private:
	CStoreWrapper *	fStoreWrapper;	// +00
	CStore *				fStore;			// +04
	PSSId					fId;				// +08
	CDynamicArray *	f0C;
	CDynamicArray *	f10;
	CDynamicArray *	f14;
public:
	CDynamicArray *	f18;
};


struct MapMap
{
	StoreRef		remote;
	StoreRef		local;
	ArrayIndex	numOfTags;
};

struct SymbolMap
{
	StoreRef		remote;
	StoreRef		local;
};

struct CopyData
{
	MapMap		frameMapping[16];
	ArrayIndex	symbolIndex;
	SymbolMap	symbolMapping[32];
};


/*------------------------------------------------------------------------------
	C S t o r e W r a p p e r
------------------------------------------------------------------------------*/

#define kMapCacheSize		 8
#define kSymbolCacheSize	16

class CStoreWrapper
{
public:
					CStoreWrapper(CStore * inStore);
					~CStoreWrapper(void);

	void			dirty(void);
	void			sparklingClean(void);
	Ref			referenceToMap(StoreRef inRef);
	Ref			referenceToSymbol(StoreRef inRef);
	StoreRef		symbolToReference(RefArg inSymbol);
	StoreRef		addMap(SortedMapTag * inTags, bool inSoupEntry, ArrayIndex * ioNumOfTags, ArrayIndex * outIndices);
	StoreRef		frameToMapReference(RefArg inFrame, bool inSoupEntry, ArrayIndex * ioNumOfTags, ArrayIndex ** outIndices);
	void			startCopyMaps_Symbols(void);
	StoreRef		copyMap(StoreRef inRef, CStoreWrapper * inOtherStore, ArrayIndex * outNumOfTags);
	StoreRef		copySymbol(StoreRef inRef, CStoreWrapper * inOtherStore);
	void			endCopyMaps_Symbols(void);

	NewtonErr	getStoreSizes(size_t * outUsedSize, size_t * outTotalSize);
	NewtonErr	lockStore(void);
	NewtonErr	unlockStore(void);
	NewtonErr	abort(void);

	CStore *		store(void) const;
	CNodeCache *nodeCache(void);

	void			setEphemeralTracker(CEphemeralTracker * inTracker);
	bool			hasEphemerals(void);
	void			addEphemeral(PSSId inId);
	void			removeEphemeral(PSSId inId);
	void			deleteEphemeral(PSSId inId);
	void			flushEphemerals(void);

private:
	friend Ref MakeStoreObject(CStore * inStore);

	CStoreHashTable *		fMapTable;				// +00
	CStoreHashTable *		fSymbolTable;			// +04
	RefStruct				fMapCache;				// +08
	StoreRef					fMapRefCache[kMapCacheSize];			// +0C
	ArrayIndex				fMapRefCacheIndex;	// +2C
	RefStruct				fSymbolCache;			// +30
	StoreRef					fSymbolRefCache[kSymbolCacheSize];	// +34
	CopyData *				fCopyData;				// +74
	ArrayIndex				fSymbolCacheIndex;	// +78
	CStore *					fStore;					// +7C
	CNodeCache				fNodeCache;				// +80
	bool						fIsLocked;				// +90
	CEphemeralTracker *	fTracker;				// +94
};

inline	CStore *		CStoreWrapper::store(void) const
{ return fStore; }

inline	CNodeCache *CStoreWrapper::nodeCache(void)
{ return &fNodeCache; }

inline	void			CStoreWrapper::setEphemeralTracker(CEphemeralTracker * inTracker)
{ fTracker = inTracker; }

inline 	bool			CStoreWrapper::hasEphemerals(void)
{ return fTracker != NULL; }

inline	void			CStoreWrapper::addEphemeral(PSSId inId)
{ fTracker->addEphemeral(inId); }

inline	void			CStoreWrapper::removeEphemeral(PSSId inId)
{ if (fTracker->isEphemeral(inId)) removeEphemeral(inId); }

inline	void			CStoreWrapper::deleteEphemeral(PSSId inId)
{ if (fTracker && fTracker->isEphemeral(inId)) deleteEphemeral(inId); }

inline 	void			CStoreWrapper::flushEphemerals(void)
{ fTracker->flushEphemerals(); }

#endif	/* __STOREWRAPPER_H */
