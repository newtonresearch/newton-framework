/*
	File:		Flash.h

	Contains:	Newton flash memory interface.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__FLASH_H)
#define __FLASH_H 1

#include "PSSTypes.h"
#include "Protocols.h"
#include "VirtualMemory.h"
#include "FlashDriver.h"

class CCardSocket;
class CCardPCMCIA;

/* -----------------------------------------------------------------------------
	C U L o c k i n g S e m a p h o r e G r a b b e r
----------------------------------------------------------------------------- */
class CULockingSemaphore;

class CULockingSemaphoreGrabber
{
public:
enum eNonBlockOption
{
	kOption0,
	kOption1
};
			CULockingSemaphoreGrabber(CULockingSemaphore * inSemaphore);
			CULockingSemaphoreGrabber(CULockingSemaphore * inSemaphore, eNonBlockOption inOption);
			~CULockingSemaphoreGrabber();

	void	doAcquire(CULockingSemaphore * inSemaphore);

private:
	CULockingSemaphore * fSemaphore;
};


/*------------------------------------------------------------------------------
	C F l a s h
	P-class interface.
	Interface to the flash hardware, typically on a PCMCIA card, but also internal.
	Implementations are TFlashAMD, TFlashSeries2, TNewInternalFlash.
------------------------------------------------------------------------------*/

PROTOCOL CFlash : public CProtocol
{
public:
	static CFlash *	make(const char * inName);
	void			destroy(void);

	NewtonErr	read(ZAddr inAddr, size_t inLength, char * inBuffer);
	NewtonErr	write(ZAddr inAddr, size_t inLength, char * inBuffer);

	NewtonErr	erase(ZAddr);
	void			suspendErase(ULong, ULong, ULong);
	void			resumeErase(ULong);

	void			deepSleep(ULong);
	void			wakeup(ULong);
	NewtonErr	status(ULong);

	void			resetCard(void);
	void			acknowledgeReset(void);
	void			getPhysResource(void);
	void			registerClientInfo(ULong);

	void			getWriteProtected(bool * outWP);
	void			getWriteErrorAddress(void);
	ULong			getAttributes(void);
	ULong			getDataOffset(void);
	size_t		getTotalSize(void);
	size_t		getGroupSize(void);
	size_t		getEraseRegionSize(void);

	ULong			getChipsPerGroup(void);
	ULong			getBlocksPerPartition(void);

	ULong			getMaxConcurrentVppOps(void);
	ULong			getEraseRegionCurrent(void);
	ULong			getWriteRegionCurrent(void);
	ULong			getEraseRegionTime(void);
	ULong			getWriteAccessTime(void);
	ULong			getReadAccessTime(void);

	ULong			getVendorInfo(void);
	int			getSocketNumber(void);

	ULong			vppStatus(void);
	ULong			vppRisingTime(void);

	void			flashSpecific(ULong, void*, ULong);

	NewtonErr	initialize(CCardSocket*, CCardPCMCIA*, ULong, ULong);
	NewtonErr	suspendService(void);
	NewtonErr	resumeService(CCardSocket*, CCardPCMCIA*, ULong);

	NewtonErr	copy(ZAddr inFromAddr, ZAddr inToAddr, size_t inLength);
	bool			isVirgin(ZAddr inAddr, size_t inLength);
};


/* -----------------------------------------------------------------------------
	C N e w I n t e r n a l F l a s h
	P-class implementation.
----------------------------------------------------------------------------- */
typedef NewtonErr (CFlashRange::*FlashRWProcPtr)(VAddr inAddr, size_t inLength, char * inBuffer);

PROTOCOL CNewInternalFlash : public CFlash
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CNewInternalFlash)

	CNewInternalFlash *	make(void);
	void			destroy(void);

// ---- initialization
	enum eInitHWOption
	{
		kHWNoMMU,
		kHWUseMMU
	};
	enum eCheckEraseOption
	{
		kEraseAsync,
		kEraseSync
	};

	NewtonErr	init(CMemoryAllocator * inAllocator);
	NewtonErr	internalInit(CMemoryAllocator * inAllocator, eInitHWOption inOptions);
	NewtonErr	initializeState(CMemoryAllocator * inAllocator, eInitHWOption inOptions);
// ----

	NewtonErr	read(ZAddr inAddr, size_t inLength, char * inBuffer);
	NewtonErr	write(ZAddr inAddr, size_t inLength, char * inBuffer);

	void			clobber(void);
	void			cleanUp(void);
	NewtonErr	erase(PAddr);
	void			suspendErase(ULong, ULong, ULong);
	void			resumeErase(ULong);

	void			deepSleep(ULong);
	void			wakeup(ULong);
	NewtonErr	status(ULong);

	void			resetCard(void);
	void			acknowledgeReset(void);
	void			getPhysResource(void);
	void			registerClientInfo(ULong);

	void			getWriteProtected(bool * outWP);
	void			getWriteErrorAddress(void);
	ULong			getAttributes(void);
	ULong			getDataOffset(void);
	size_t		getTotalSize(void);
	size_t		getGroupSize(void);
	size_t		getEraseRegionSize(void);

	ULong			getChipsPerGroup(void);
	ULong			getBlocksPerPartition(void);

	ULong			getMaxConcurrentVppOps(void);
	ULong			getEraseRegionCurrent(void);
	ULong			getWriteRegionCurrent(void);
	ULong			getEraseRegionTime(void);
	ULong			getWriteAccessTime(void);
	ULong			getReadAccessTime(void);

	ULong			getVendorInfo(void);
	int			getSocketNumber(void);

	ULong			vppStatus(void);
	ULong			vppRisingTime(void);

	void			flashSpecific(ULong, void*, ULong);

	NewtonErr	initialize(CCardSocket*, CCardPCMCIA*, ULong, ULong);
	NewtonErr	suspendService(void);
	NewtonErr	resumeService(CCardSocket*, CCardPCMCIA*, ULong);

	NewtonErr	copy(ZAddr inFromAddr, ZAddr inToAddr, size_t inLength);
	bool			isVirgin(ZAddr inAddr, size_t inLength);

private:
	friend class CFlashRange;
	NewtonErr	readPhysical(ZAddr inAddr, size_t inLength, char * inBuffer);
	NewtonErr	writePhysical(ZAddr inAddr, size_t inLength, char * inBuffer);
	NewtonErr	readWrite(FlashRWProcPtr inRW, ZAddr inAddr, size_t inLength, char * inBuffer);
	NewtonErr	readWritePhysical(FlashRWProcPtr inRW, ZAddr inAddr, size_t inLength, char * inBuffer);
	NewtonErr	setupVirtualMappings(void);
	NewtonErr	gatherBlockMappingInfo(ULong & ioArg1, ULong & ioArg2, ULong & ioArg3, ULong & ioArg4);
	NewtonErr	flashAllowedLocations(bool * outArg1, bool * outArg2);
	void			avoidConflictWithRExInIOSpace(bool * outBank);
	void			alignAndMapVMRange(VAddr& ioAddr, PAddr inPhysAddr, size_t inSize, bool inCacheable, Perm inPerm);
	NewtonErr	configureFlashBank(VAddr& ioBankAddr, VAddr& inROAddr, VAddr& inRWAddr);
	NewtonErr	configureIOBank(VAddr& ioBankAddr, VAddr& inROAddr, VAddr& inRWAddr);
	NewtonErr	addFlashRange(CFlashRange * inRange, VAddr& ioBankAddr, VAddr& inROAddr, VAddr& inRWAddr, PAddr inPhysAddr);
	NewtonErr	findRange(PAddr inAddr, CFlashRange *& outRange);

	bool			checkFor4LaneFlash(VAddr, SFlashChipInformation&, CFlashDriver*&);
	bool			checkFor2LaneFlash(VAddr, SFlashChipInformation&, CFlashDriver*&, eMemoryLane);
	bool			checkFor1LaneFlash(VAddr, SFlashChipInformation&, CFlashDriver*&, eMemoryLane);
	NewtonErr	searchForFlashDrivers(void);
	bool			findDriverAble(CFlashDriver*&, VAddr, eMemoryLane, SFlashChipInformation&);

	NewtonErr	syncErasePhysicalBlock(PAddr inAddr);
	bool			internalCheckEraseCompletion(NewtonErr& outErr, eCheckEraseOption inOption);

	NewtonErr	copyUsingBuffer(ZAddr inFromAddr, ZAddr inToAddr, size_t inLength, void * inBuffer, size_t inBufLen);

	void			internalClobber(void);
	void			turnPowerOn(void);
	void			turnPowerOff(void);
	void			internalVppOn(void);
	void			internalVppOff(void);

	CMemoryAllocator *	fAllocator;	//+10
	ArrayIndex		fNumOfRanges;		//+14
	size_t			fStoreSize;			//+18
	size_t			fBlockSize;			//+1C
	CFlashDriver *	fDriver[6];			//+20
	CFlashRange *	fRange[2];			//+38
	int				f40;
	CFlashRange *	f44;
	NewtonErr		fEraseErr;			//+48
	ArrayIndex		fNumOfDrivers;		//+4C
	CBankControlRegister * fBCR;		//+50
	eInitHWOption	f54;					//+54
	bool				f58;
	UShort *			fBlockMap;			//+5C
	ArrayIndex		fNumOfBlocks;		//+60
	ULong				f64;	// possibly UShort: it’s an entry in the fBlockMap
//	CULockingSemaphore *	fSemaphore;	//+68
//size+6C
};


#endif	/* __FLASH_H */
