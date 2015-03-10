/*
	File:		FlashStore.h

	Contains:	Flash store declarations.
	
	Written by:	Newton Research Group, 2010.
*/

#if !defined(__FLASHSTORE_H)
#define __FLASHSTORE_H 1

#include "Store.h"
#include "Flash.h"
#include "FlashBlock.h"
#include "FlashCache.h"
#include "FlashTracker.h"
#include "CardEvents.h"

#define kRootDirId		 3
#define kTxnRecordId		23
#define kRootId			39
#define kFirstObjectId	49


/*------------------------------------------------------------------------------
	C F l a s h S t o r e
------------------------------------------------------------------------------*/

class CFlashStore : public CStore
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CFlashStore)
	CAPABILITIES( "LOBJ" "" )

	CFlashStore *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inStoreAddr, size_t inStoreSize, ULong inArg3, ArrayIndex inSocketNumber, ULong inFlags, void * inPSSInfo);
	NewtonErr	needsFormat(bool * outNeedsFormat);
	NewtonErr	format(void);

	NewtonErr	getRootId(PSSId * outRootId);
	NewtonErr	newObject(PSSId * outObjectId, size_t inSize);
	NewtonErr	eraseObject(PSSId inObjectId);
	NewtonErr	deleteObject(PSSId inObjectId);
	NewtonErr	setObjectSize(PSSId inObjectId, size_t inSize);
	NewtonErr	getObjectSize(PSSId inObjectId, size_t * outSize);

	NewtonErr	write(PSSId inObjectId, size_t inStartOffset, void * inBuffer, size_t inLength);
	NewtonErr	read(PSSId inObjectId, size_t inStartOffset, void * outBuffer, size_t inLength);

	NewtonErr	getStoreSize(size_t * outTotalSize, size_t * outUsedSize);
	NewtonErr	isReadOnly(bool * outIsReadOnly);
	NewtonErr	lockStore(void);
	NewtonErr	unlockStore(void);

	NewtonErr	abort(void);
	NewtonErr	idle(bool * outArg1, bool * outArg2);

	NewtonErr	nextObject(PSSId inObjectId, PSSId * outNextObjectId);
	NewtonErr	checkIntegrity(ULong * inArg1);

	NewtonErr	setBuddy(CStore * inStore);
	bool			ownsObject(PSSId inObjectId);
	VAddr			address(PSSId inObjectId);
	const char * storeKind(void);
	NewtonErr	setStore(CStore * inStore, PSSId inEnvironment);

	bool			isSameStore(void * inAlienStoreData, size_t inAlienStoreSize);
	bool			isLocked(void);
	bool			isROM(void);

	NewtonErr	vppOff(void);
	NewtonErr	sleep(void);

	NewtonErr	newWithinTransaction(PSSId * outObjectId, size_t inSize);
	NewtonErr	startTransactionAgainst(PSSId inObjectId);
	NewtonErr	separatelyAbort(PSSId inObjectId);
	NewtonErr	addToCurrentTransaction(PSSId inObjectId);
	bool			inSeparateTransaction(PSSId inObjectId);

	NewtonErr	lockReadOnly(void);
	NewtonErr	unlockReadOnly(bool inReset);
	bool			inTransaction(void);

	NewtonErr	newObject(PSSId * outObjectId, void * inData, size_t inSize);
	NewtonErr	replaceObject(PSSId inObjectId, void * inData, size_t inSize);

	NewtonErr	calcXIPObjectSize(long inArg1, long inArg2, long * outArg3);
	NewtonErr	newXIPObject(PSSId * outObjectId, size_t inSize);
	NewtonErr	getXIPObjectInfo(PSSId inObjectId, unsigned long * outArg2, unsigned long * outArg3, unsigned long * outArg4);


// additional flash methods

	NewtonErr	newWithinTransaction(PSSId * outObjectId, size_t inSize, bool inArg3);
	NewtonErr	startTransaction(void);
	NewtonErr	doAbort(bool inArg);

	NewtonErr	vppOn(void);
	NewtonErr	vccOff(void);
	NewtonErr	vccOn(void);

	size_t		storeCapacity(void);
	size_t		avail(void);
	size_t		storeSlop(void);
	void			gc(void);

	ULong			nextLSN(void);
	NewtonErr	addObject(PSSId inObjectId, int inState, size_t inSize, CStoreObjRef & inObj, bool inArg4, bool inArg5);
	void			add(CStoreObjRef & inObj);
	void			remove(CStoreObjRef & inObj);
	ZAddr			translate(ZAddr inAddr);
	NewtonErr	validateIncomingPSSId(PSSId inObjectId);
	NewtonErr	lookup(PSSId inObjectId, int inState, CStoreObjRef & inObj);
	NewtonErr	setupForRead(PSSId inObjectId, CStoreObjRef & ioObj);
	NewtonErr	setupForModify(PSSId inObjectId, CStoreObjRef & ioObj, bool inArg3, bool inArg4);
	NewtonErr	basicRead(ZAddr inAddr, void * outBuf, size_t inLen);
	NewtonErr	basicWrite(ZAddr inAddr, void * inBuf, size_t inLen);
	NewtonErr	basicCopy(ZAddr inFromAddr, ZAddr inToAddr, size_t inLen);
	NewtonErr	zap(ZAddr inAddr, size_t inLen);
	NewtonErr	chooseWorkingBlock(size_t inMinSize, ZAddr inAddr);
	bool			isRangeVirgin(ZAddr inAddr, size_t inLen);


	NewtonErr	deleteTransactionRecord(void);
	NewtonErr	markCommitPoint(void);
	NewtonErr	doCommit(bool inArg);

	NewtonErr	transactionState(int * outState);
	NewtonErr	recoveryCheck(bool inArg);
	void			lowLevelRecovery(void);
	NewtonErr	mount(void);
	void			touchMe(void);
	void			notifyCompact(CFlashBlock * inBlock);

	void			initBlocks(void);
	NewtonErr	scanLogForLogicalBlocks(bool *);
	NewtonErr	scanLogForErasures(void);
	NewtonErr	scanLogForReservedBlocks(void);

	NewtonErr	addLogEntryToPhysBlock(ULong inIdent, size_t inSize, SFlashLogEntry * ioLogEntry, ZAddr inAddr, ZAddr * outAddr);
	NewtonErr	nextLogEntry(ZAddr inAddr, ZAddr * outAddr, ULong inType, void * inAlienStoreData);
	NewtonErr	zapLogEntry(ZAddr inAddr);
	ZAddr			findPhysWritable(ZAddr inStartAddr, ZAddr inEndAddr, size_t inSize);


	bool			isWriteProtected(void);

	bool			isErased(ZAddr inBlockAddr);
	bool			isErased(ZAddr inBlockAddr, size_t inBlockSize, ArrayIndex inNumOfBadBytesPermitted);
	NewtonErr	syncErase(ZAddr inBlockAddr);
	NewtonErr	startErase(ZAddr inBlockAddr);
	NewtonErr	eraseStatus(ZAddr inBlockAddr);
	NewtonErr	waitForEraseDone(void);
	void			calcAverageEraseCount(void);
	ArrayIndex	averageEraseCount(void);
	ULong			findUnusedPhysicalBlock(void);
	NewtonErr	bringVirginBlockOnline(ULong inUnusedBlock, ULong inBlock);

	ZAddr			offsetToLogs(void) const;

	size_t		bucketSize(void) const;
	size_t		bucketCount(void) const;

	PSSId			PSSIdFor(ArrayIndex inBlockNo, long inBlockOffset) const;
	ArrayIndex	objectNumberFor(PSSId inId) const;
	ArrayIndex	blockNumberFor(PSSId inId) const;
	CFlashBlock *	blockFor(PSSId inId) const;
	CFlashBlock *	blockForAddr(ZAddr inAddr) const;

private:
	friend struct StoreObjHeader;
	friend struct SDirEnt;
	friend class CStoreObjRef;
	friend class CFlashStoreLookupCache;
	friend class CFlashBlock;
	friend class CFlashIterator;

	bool			cardWPAlertProc(ULong inArg1, void * inArg2);
	NewtonErr	sendAlertMgrWPBitch(int inSelector);

	CFlash *				fFlash;				// +10
	bool					fIsMounted;			// +14
	bool					fIsStoreRemovable;// +15
	bool					fIsInTransaction;	// +16
	bool					f17;					// +17
	char *				fStoreAddr;			// +18	base address of ROM store
	ArrayIndex			fSocketNo;			// +1C
	ArrayIndex			fLSN;					// +20	Log Sequence Number
	CFlashPhysBlock *	fPhysBlock;			// +24
	CFlashBlock *		fWorkingBlock;		// +28
	CFlashBlock **		fBlock;				// +2C
	CFlashBlock *		fLogicalBlock;		// +30
	CFlashStoreLookupCache *	fCache;	// +34
	int					f38;
	bool					fIsErasing;			// +3C
	bool					fIsSRAM;				// +3D
	bool					f3E;
	bool					fIsROM;				// +3F
	ULong					fEraseBlockAddr;	// +40
	ArrayIndex			fAvEraseCount;		// +44
	ULong					fDirtyBits;			// +48
	ULong					fVirginBits;		// +4C
	size_t				fBlockSize;			// +50
	ArrayIndex			fNumOfBlocks;		// +54
	ArrayIndex			fBlockSizeShift;	// +58
	ULong					fBlockSizeMask;	// +5C
	ULong					f60;
	ULong					f64;
	ULong					f68;
	size_t				fBucketSize;		// +6C
	ArrayIndex			fBucketCount;		// +70
	int					fLockCount;			// +74
	CCardHandler *		fCardHandler;		// +78
	CStoreObjRef *		fObjListTail;		// +7C
	CStoreObjRef *		fObjListHead;		// +80
	CFlashTracker *	fTracker;			// +84
	CStoreDriver *		fStoreDriver;		// +88
	SCompactState *	fCompact;			// +8C
	bool					fIsFormatReqd;		// +90
	bool					fUseRAM;				// +91
	bool					f92;
	bool					fIsFormatting;		// +94
	bool					f95;
	bool					f96;
	bool					fIsInternalFlash;	// +97
	long					fA0;
	CStoreDriver		fA4;
	int					fLockROCount;		// +D4
	long					fD8;					// +D8	actually CStore *?
	bool					fE4;
	bool					fE5;
	ArrayIndex			fE8;
	size_t				fCachedUsedSize;	// +EC
// size +F0
};


inline ULong			CFlashStore::offsetToLogs(void) const { return fBlockSize - (fIsSRAM ? 256 : 1024); }

inline size_t			CFlashStore::bucketSize(void) const { return fBucketSize; }
inline size_t			CFlashStore::bucketCount(void) const { return fBucketCount; }

inline PSSId			CFlashStore::PSSIdFor(ArrayIndex inBlockNo, long inBlockOffset) const { return (inBlockNo << f60) | inBlockOffset; }
inline ArrayIndex		CFlashStore::objectNumberFor(PSSId inId) const { return inId & ~(0x0FFFFFFF << f60); }
inline ArrayIndex		CFlashStore::blockNumberFor(PSSId inId) const { return inId >> f60; }
inline CFlashBlock *	CFlashStore::blockFor(PSSId inId) const { return fBlock[inId >> f60]; }
inline CFlashBlock *	CFlashStore::blockForAddr(ZAddr inAddr) const { return fBlock[inAddr >> fBlockSizeShift]; }

inline ArrayIndex		CFlashStore::nextLSN(void) { return ++fLSN; }


#endif	/* __FLASHSTORE_H */

