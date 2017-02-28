/*
	File:		FlashBlock.h

	Contains:	Flash block declarations.
	
	Written by:	Newton Research Group, 2010.
*/

#if !defined(__FLASHBLOCK_H)
#define __FLASHBLOCK_H 1

#include "StoreObjRef.h"
#include "CompactState.h"

class CFlashStore;

/*------------------------------------------------------------------------------
	S F l a s h L o g E n t r y
------------------------------------------------------------------------------*/
#define kAnyBlockLogEntryType			0
#define kFlashBlockLogEntryType		'fblk'
#define kReservedBlockLogEntryType	'zblk'
#define kEraseBlockLogEntryType		'eblk'

struct SFlashLogEntry
{
	bool		isValid(ZAddr inAddr);
	ZAddr		physOffset(void);

	ULong		fPhysSig;		// +00
	ULong		fPhysSig2;		// +04
	ULong		fNewtSig;		// +08
	ULong		fType;			// +0C
	size_t	fSize;			// +10
	ULong		fLSN;				// +14
	ZAddr		fPhysicalAddr;	// +18	block address/offset
	ULong		f1C;				// +1C	virgin bits
// size+20?
};

struct SReservedBlockLogEntry : public SFlashLogEntry
{
	ULong		f20;
	ULong		fEraseCount;	// +24
	ULong		f28;
	ULong		f2C;
// size+30
};

struct SFlashBlockLogEntry : public SFlashLogEntry
{
	ULong		f20;
	ULong		fEraseCount;	// +24
	ZAddr		fLogicalAddr;	// +28
	ULong		f2C;
	ULong		f30;
	ULong		fRandom;			// +34
	ULong		fCreationTime;	// +38	in seconds
	ZAddr		fDirectoryAddr;// +3C
	ULong		f40;
	ULong		f44;
	ULong		f48;
// size+4C
};

struct SFlashEraseLogEntry : public SFlashLogEntry
{
	ULong		f20;	// eraseCount
	bool		f24;
// size+28
};


/*------------------------------------------------------------------------------
	C F l a s h P h y s B l o c k
------------------------------------------------------------------------------*/

class CFlashPhysBlock
{
public:
	NewtonErr	init(CFlashStore * inStore, ZAddr inPhysAddr);

	void			setInfo(SFlashBlockLogEntry * info);
	void			setInfo(SFlashEraseLogEntry * info);
	void			setInfo(SReservedBlockLogEntry * info);
	ZAddr			logEntryOffset(void) const;
	ZAddr			getPhysicalOffset(void) const;	// address in store of this block

	ArrayIndex	eraseCount(void) const;

	void			setSpare(CFlashPhysBlock * inBlock, ULong);
	bool			isSpare(void) const;

	bool			isReserved(void) const;

private:
	CFlashStore *	fStore;
	ZAddr				f04;
	ZAddr				fBlockAddr;			// +08
	ZAddr				fLogEntryOffset;	// +0C
	ArrayIndex		fEraseCount;		// +10
	bool				f14;					// isErased?
	bool				fIsReserved;		// +15
};


inline ZAddr		CFlashPhysBlock::logEntryOffset(void) const { return fLogEntryOffset; }
inline ZAddr		CFlashPhysBlock::getPhysicalOffset(void) const { return fBlockAddr; }
inline ArrayIndex	CFlashPhysBlock::eraseCount(void) const { return fEraseCount; }
inline bool			CFlashPhysBlock::isSpare(void) const { return f04 == 0xFFFFFFFF; }
inline bool			CFlashPhysBlock::isReserved(void) const { return fIsReserved; }


/*------------------------------------------------------------------------------
	C F l a s h B l o c k
------------------------------------------------------------------------------*/

class CFlashBlock
{
public:
	NewtonErr	init(CFlashStore * inStore);

	NewtonErr	writeRootDirectory(ZAddr * outAddr);
	NewtonErr	setInfo(SFlashBlockLogEntry * info, bool*);
	NewtonErr	setInfo(SReservedBlockLogEntry * info);
	NewtonErr	lookup(PSSId inId, int inState, CStoreObjRef& ioObj, int*);

	NewtonErr	addObject(PSSId inId, int inState, size_t inSize, CStoreObjRef& ioObj, bool inArg5, bool inArg6);
	NewtonErr	zapObject(ZAddr inAddr);
	NewtonErr	readObjectAt(ZAddr inAddr, StoreObjHeader * outObject);
	NewtonErr	nextObject(ZAddr inAddr, ZAddr * outAddr, bool inArg3);

	NewtonErr	addDirEnt(PSSId, ULong, ULong*, SDirEnt * outDirEnt);
	NewtonErr	zapDirEnt(ULong inAddr);
	NewtonErr	readDirEntAt(ULong inAddr, SDirEnt * outDirEnt);
	NewtonErr	setDirEntOffset(ULong, ULong);

	ZAddr			rootDirEnt(PSSId);
	size_t		rootDirSize(void);
	NewtonErr	extendDirBucket(ULong, ULong*);

	NewtonErr	addMigDirEnt(long, long);
	NewtonErr	zapMigDirEnt(ULong);
	void			objectMigrated(ULong, long);

	NewtonErr	compactInto(ULong);
	NewtonErr	compactInto(CFlashBlock*);
	NewtonErr	compactInPlace(void);
	NewtonErr	startCompact(SCompactState*);
	NewtonErr	continueCompact(SCompactState*);
	NewtonErr	realContinueCompact(SCompactState*);

	bool			isVirgin(void);
	bool			isReserved(void);
	PSSId			nextPSSId(void);
	PSSId			useNextPSSId(void);

	ArrayIndex	eraseCount(void);
	ULong			eraseHeuristic(ULong);
	size_t		bucketSize(void);
	ArrayIndex	bucketCount(void);

	size_t		avail(void);
	size_t		yield(void);
	size_t		calcRecoverableBytes(void);
	ZAddr			endOffset(void);
	ZAddr			logEntryOffset(void);
	CFlashPhysBlock *	physBlock(void);
	ZAddr			physAddr(void) const;
	ZAddr			rootDirAddr(void) const;

	NewtonErr	basicWrite(ZAddr inAddr, void * inBuf, size_t inLen);

	ZAddr			firstObjAddr(void) const;
	ULong			validity(void) const;

private:
	friend class CFlashIterator;

	CFlashStore *	fStore;					//+00
	ZAddr				fFirstObjAddr;			//+04
	ZAddr				fPhysBlockAddr;		//+08
	ZAddr				fDirBase;				//+0C
	ZAddr				fNextObjAddr;			//+10
	size_t			fZappedSize;			//+14
	PSSId				fAvailableId;			//+18
	ULong				fValidity;				//+1C
// size+20
};

inline ZAddr		CFlashBlock::physAddr(void) const { return fPhysBlockAddr; }
inline ZAddr		CFlashBlock::rootDirAddr(void) const { return fDirBase; }
inline ZAddr		CFlashBlock::firstObjAddr(void) const { return fFirstObjAddr; }
inline ULong		CFlashBlock::validity(void) const { return fValidity; }


#endif	/* __FLASHBLOCK_H */
