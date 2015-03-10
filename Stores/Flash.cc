/*
	File:		Flash.cc

	Contains:	Newton flash memory implementation.
					Mapping info: (see https://code.google.com/p/einstein/wiki/MemMapVirtual )
						flash bank 1
						virtual address #30000000 -> #02000000 physical r/o
						virtual address #34000000 -> #02000000 physical
						flash bank 2
						virtual address #30400000 -> #10000000 physical r/o
						virtual address #34400000 -> #10000000 physical

	Written by:	Newton Research Group, 2014.
*/

#include "Flash.h"
#include "OSErrors.h"

#include <sys/fcntl.h>
#include <sys/mman.h>

extern void			PowerOffAndReboot(NewtonErr inError);
extern "C" NewtonErr	AddNewSecPNJT(VAddr inVAddr, PAddr inPhysAddr, ULong inDomain, Perm inPerm, bool inCacheable);

/* -----------------------------------------------------------------------------
	C o n s t a n t s
----------------------------------------------------------------------------- */
#define kFlashBank1ROAddress		0x30000000
#define kFlashBank1RWAddress		0x34000000
#define kFlashBank1PhysAddress	0x02000000

#define kFlashBank2ROAddress		0x30400000
#define kFlashBank2RWAddress		0x34400000
#define kFlashBank2PhysAddress	0x10000000


/* -----------------------------------------------------------------------------
	T y p e s
----------------------------------------------------------------------------- */
struct FlashBlockInfo
{
	UShort	index;
	UShort	type;
};


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */
CNewInternalFlash * gInternalFlash;


#if defined(forFramework)
/* -----------------------------------------------------------------------------
	S t u b s
----------------------------------------------------------------------------- */
void
PowerOffAndReboot(NewtonErr inError)
{}

NewtonErr
Reboot(NewtonErr inError, ULong inRebootType, bool inSafe)
{ return inError; }

NewtonErr
AddNewSecPNJT(VAddr inVAddr, PAddr inPhysAddr, ULong inDomain, Perm inPerm, bool inCacheable)
{ return noErr; }
#endif


#pragma mark -
/* -----------------------------------------------------------------------------
	S F l a s h C h i p I n f o r m a t i o n
----------------------------------------------------------------------------- */

bool
SFlashChipInformation::operator==(const SFlashChipInformation& info) const
{
	return x00 == info.x00
		 && x04 == info.x04
		 && x08 == info.x08
		 && x0C == info.x0C
		 && x10 == info.x10
		 && x14 == info.x14;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C N e w I n t e r n a l F l a s h
	We call it CNewInternalFlash for compatibility with the original,
	but we implement a disk store.
	So we only need a subset of the full protocol methods.
----------------------------------------------------------------------------- */

void
ClobberInternalFlash(void)
{
	CNewInternalFlash internalFlash;
	internalFlash.init(CHeapAllocator::getGlobalAllocator());
	internalFlash.clobber();
	internalFlash.cleanUp();
}


/* ----------------------------------------------------------------
	CNewInternalFlash implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CNewInternalFlash::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN17CNewInternalFlash6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN17CNewInternalFlash4makeEv - 0b	\n"
"		.long		__ZN17CNewInternalFlash7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CNewInternalFlash\"	\n"
"2:	.asciz	\"CFlash\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN17CNewInternalFlash9classInfoEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash4makeEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash7destroyEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash4readEjmPc - 4b	\n"
"		.long		__ZN17CNewInternalFlash5writeEjmPc - 4b	\n"
"		.long		__ZN17CNewInternalFlash5eraseEm - 4b	\n"
"		.long		__ZN17CNewInternalFlash12suspendEraseEjjj - 4b	\n"
"		.long		__ZN17CNewInternalFlash11resumeEraseEj - 4b	\n"
"		.long		__ZN17CNewInternalFlash9deepSleepEj - 4b	\n"
"		.long		__ZN17CNewInternalFlash6wakeupEj - 4b	\n"
"		.long		__ZN17CNewInternalFlash6statusEj - 4b	\n"
"		.long		__ZN17CNewInternalFlash9resetCardEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash16acknowledgeResetEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash15getPhysResourceEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash18registerClientInfoEj - 4b	\n"
"		.long		__ZN17CNewInternalFlash17getWriteProtectedEPb - 4b	\n"
"		.long		__ZN17CNewInternalFlash20getWriteErrorAddressEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash13getAttributesEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash13getDataOffsetEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash12getTotalSizeEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash12getGroupSizeEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash18getEraseRegionSizeEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash16getChipsPerGroupEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash21getBlocksPerPartitionEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash22getMaxConcurrentVppOpsEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash21getEraseRegionCurrentEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash21getWriteRegionCurrentEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash18getEraseRegionTimeEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash18getWriteAccessTimeEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash17getReadAccessTimeEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash13getVendorInfoEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash15getSocketNumberEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash9vppStatusEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash13vppRisingTimeEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash13flashSpecificEjPvj - 4b	\n"
"		.long		__ZN17CNewInternalFlash10initializeEP11CCardSocketP11CCardPCMCIAjj - 4b	\n"
"		.long		__ZN17CNewInternalFlash14suspendServiceEv - 4b	\n"
"		.long		__ZN17CNewInternalFlash13resumeServiceEP11CCardSocketP11CCardPCMCIAj - 4b	\n"
"		.long		__ZN17CNewInternalFlash4copyEjjm - 4b	\n"
"		.long		__ZN17CNewInternalFlash8isVirginEjm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CNewInternalFlash)

CNewInternalFlash *
CNewInternalFlash::make(void)
{
	// this really does nothing
	return this;
}


void
CNewInternalFlash::destroy(void)
{
	cleanUp();
}


NewtonErr
CNewInternalFlash::init(CMemoryAllocator * inAllocator)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = internalInit(inAllocator, kHWNoMMU))
		XFAILNOT(fBlockMap = (UShort *)inAllocator->allocate((fNumOfBlocks-1)*sizeof(UShort)), err = kOSErrNoMemory;)
		err = setupVirtualMappings();
		if (err == kStoreErrNeedsFormat)
			clobber();
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewInternalFlash::findRange(PAddr inAddr, CFlashRange *& outRange)
{
	if (inAddr < fStoreSize)
	{
		for (ArrayIndex i = 0; i < fNumOfRanges; ++i)
		{
			CFlashRange * range = fRange[i];
			if (inAddr < range->fRangeAddr + range->fRangeSize)
			{
				outRange = range;
				return noErr;
			}
		}
	}
	return kFlashErrAddressOutOfRange;
}


NewtonErr
CNewInternalFlash::internalInit(CMemoryAllocator * inAllocator, eInitHWOption inOptions)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = initializeState(inAllocator, inOptions))

		bool bank1, bank2;
		XFAIL(err = flashAllowedLocations(&bank1, &bank2))

		VAddr roAddr = kFlashBank1ROAddress;	// R flash memory, bank 1, virtual address
		VAddr rwAddr = kFlashBank1RWAddress;	// W flash memory, bank 1, virtual address
		// start the flash at a virtual address of zero
		VAddr flashAddr = 0;
		if (bank1)
			XFAIL(err = configureFlashBank(flashAddr, roAddr, rwAddr))
		if (bank2)
			XFAIL(err = configureIOBank(flashAddr, roAddr, rwAddr))
		// the flashAddr is updated by configureXXX() so now indicates the total flash range size
		fStoreSize = flashAddr;

		if (fNumOfRanges > 0)
		{
			fStoreSize -= fBlockSize;

			CFlashRange * range = fRange[0];
			range->fRangeSize -= fBlockSize;
			range->adjustVirtualAddresses(fBlockSize);
			for (ArrayIndex i = 1; i < fNumOfRanges; ++i)	// valid fNumOfRanges is in range 0..2
				fRange[i]->fRangeAddr -= fBlockSize;
		}
		fNumOfBlocks = fStoreSize / fBlockSize;
//		gGlobalsThatLiveAcrossReboot.fReserved[16] = (f58 != 0);						// this doesn’t look right
//		gGlobalsThatLiveAcrossReboot.fReserved[15] = fStoreSize - fBlockSize;

		gInternalFlash = this;
	}
	XENDTRY;
	return err;
}


void
CNewInternalFlash::cleanUp(void)
{
	for (int i = fNumOfRanges - 1; i >= 0; i--)
	{
		CFlashRange * range = fRange[i];
		fRange[i] = NULL;
		range->dlete(*fAllocator);
#if defined(correct)
		range->~CFlashRange();
		fAllocator->deallocate(range);
#else
		delete range;
#endif
	}
	fNumOfRanges = 0;

	for (int i = fNumOfDrivers - 1; i >= 0; i--)
	{
		CFlashDriver * driver = fDriver[i];
		fDriver[i] = NULL;
		driver->cleanUp(*fAllocator);
#if defined(correct)
		driver->~CFlashDriver();
		fAllocator->deallocate(driver);
#else
		delete driver;
#endif
	}
	fNumOfDrivers = 0;

	if (fBlockMap)
	{
		fAllocator->deallocate(fBlockMap);
		fBlockMap = NULL;
	}

/*	if (fSemaphore)
		delete fSemaphore, fSemaphore = NULL; */

	gInternalFlash = NULL;
}


NewtonErr
CNewInternalFlash::flashAllowedLocations(bool * outBank1, bool * outBank2)
{
	NewtonErr err = noErr;

	// let ’em have both barrels
	*outBank1 = YES;
	*outBank2 = YES;

	XTRY
	{
		ULong size;
#if defined(correct)
		FlashBankConfigTable * config = (FlashBankConfigTable *)GetLastRExConfigEntry(kFlashAddressTag, &size);
#else
		// dummy up FlashBankConfigTable
		struct
		{
			ULong version;
			ULong count;
			PAddr table[1];
		} myConfig = { kFlashBankConfigTableVersion, 1, kFlashBank1PhysAddress };
		FlashBankConfigTable * config = (FlashBankConfigTable *)&myConfig;
#endif
		if (config && config->version == kFlashBankConfigTableVersion)
		{
			// we have a valid REx config entry
			// only two entries alowed
			XFAILIF(config->count > 2, err = kFlashErrUnsupportedConfiguration;)
			*outBank1 = NO;
			*outBank2 = NO;
			for (ArrayIndex i = 0; i < config->count; ++i)
			{
				PAddr flashBankAddr = config->table[i];
				if (flashBankAddr == kFlashBank1PhysAddress)
					*outBank1 = YES;
				else if (flashBankAddr == kFlashBank2PhysAddress)
					*outBank2 = YES;
				else
					XFAIL(err = kFlashErrUnsupportedConfiguration)
			}
			XFAIL(err)
		}
		if (*outBank1 || !*outBank2)
		{
			if (*outBank2)
				avoidConflictWithRExInIOSpace(outBank2);
		}
	}
	XENDTRY;
	return err;
}


void
CNewInternalFlash::avoidConflictWithRExInIOSpace(bool * outBank)
{
#if defined(correct)
	for (ArrayIndex i = 0; i < kMaxROMExtensions; ++i)
	{
		if ((PAddr)gGlobalsThatLiveAcrossReboot.fRExPtr[i] == kFlashBank2PhysAddress)
			*outBank = NO;
	}
#endif
}


NewtonErr
CNewInternalFlash::configureFlashBank(VAddr& ioBankAddr, VAddr& inROAddr, VAddr& inRWAddr)
{
	NewtonErr err;

	if (f54)
		// we’re using the MMU -- map the address
		AddNewSecPNJT(kFlashBank1RWAddress, kFlashBank1PhysAddress, 0, kReadWrite, NO);

	fBCR->configureFlashBankDataSize(kMemoryLane32Bit);

	XTRY
	{
		CFlashDriver * driver;
		SFlashChipInformation info;
		if (checkFor4LaneFlash(kFlashBank1RWAddress, info, driver))
		{
			C32BitFlashRange * range;
#if defined(correct)
			XFAILNOT(range = (C32BitFlashRange *)fAllocator->allocate(sizeof(C32BitFlashRange)), err = kOSErrNoMemory;)
			new(range) C32BitFlashRange(driver, ioBankAddr, inROAddr, inRWAddr, kMemoryLane32Bit, info, *fAllocator);
#else
			XFAILNOT(range = new C32BitFlashRange(driver, ioBankAddr, inROAddr, inRWAddr, kMemoryLane32Bit, info, *fAllocator), err = kOSErrNoMemory;)
#endif
			err = addFlashRange(range, ioBankAddr, inROAddr, inRWAddr, kFlashBank1PhysAddress);
		}
	/*	else
		{
			XFAIL(err = configureNot32BitFlashBank())
			etc;
		} */
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewInternalFlash::configureIOBank(VAddr& ioBankAddr, VAddr& inROAddr, VAddr& inRWAddr)
{
	NewtonErr err;

	if (f54)
		// we’re using the MMU -- map the address
		AddNewSecPNJT(inRWAddr, kFlashBank2PhysAddress, 0, kReadWrite, NO);

	XTRY
	{
		CFlashDriver * driver;			// sp18
		SFlashChipInformation info;	// sp00
		if (checkFor4LaneFlash(inRWAddr, info, driver))
		{
			C32BitFlashRange * range;
#if defined(correct)
			XFAILNOT(range = (C32BitFlashRange *)fAllocator->allocate(sizeof(C32BitFlashRange)), err = kOSErrNoMemory;)
			new(range) CFlashRange(driver, ioBankAddr, inROAddr, inRWAddr, kMemoryLane32Bit, info, *fAllocator);
#else
			XFAILNOT(range = new C32BitFlashRange(driver, ioBankAddr, inROAddr, inRWAddr, kMemoryLane32Bit, info, *fAllocator), err = kOSErrNoMemory;)
#endif
			err = addFlashRange(range, ioBankAddr, inROAddr, inRWAddr, kFlashBank2PhysAddress);
		}
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewInternalFlash::addFlashRange(CFlashRange * inRange, VAddr& ioBankAddr, VAddr& inROAddr, VAddr& inRWAddr, PAddr inPhysAddr)
{
	if (fNumOfRanges > 2)
		return kFlashErrUnsupportedConfiguration;

	fRange[fNumOfRanges++] = inRange;
	if (!f58 || inRange->fFlashChipInfo.x08)
		f58 = YES;
	ioBankAddr += inRange->fRangeSize;
	alignAndMapVMRange(inROAddr, inPhysAddr, inRange->fRangeSize, YES, kReadOnly);
	alignAndMapVMRange(inRWAddr, inPhysAddr, inRange->fRangeSize * (inRange->fDataWidth / 4), NO, kReadWrite);
	if (fBlockSize < inRange->fBlockSize)
		fBlockSize = inRange->fBlockSize;
	inRange->resetAllBlocksStatus();
	return noErr;
}


void
CNewInternalFlash::alignAndMapVMRange(VAddr& ioAddr, PAddr inPhysAddr, size_t inSize, bool inCacheable, Perm inPerm)
{
	// align to section
	size_t alignedSize = ALIGN(inSize, kSectionSize);
	// update address to point past this range
	VAddr addr = ioAddr;
	ioAddr += alignedSize;
	if (f54)
	{
		// we’re using the MMU -- map all sections in the range
		for ( ; addr < ioAddr; addr += kSectionSize, inPhysAddr += kSectionSize)
		{
			AddNewSecPNJT(addr, inPhysAddr, 0, inPerm, inCacheable);
		}
	}
}


NewtonErr
CNewInternalFlash::searchForFlashDrivers(void)
{
	NewtonErr err = noErr;
	XTRY
	{
#if defined(correct)
		for (int rExBlock = kMaxROMExtensions - 1; rExBlock >= 0; rExBlock--)
		{
			ArrayIndex i = 0;
			ULong len;
			SFlashDriverEntry * entry;
			while ((entry = (SFlashDriverEntry *)PrimNextRExConfigEntry(rExBlock, 'fdrv', &len, &i)))
			{
				if (entry->fVersion == kFlashDriverVersion)
				{
					const CClassInfo * classInfo = entry->fDriverClassInfo;
					CFlashDriver * instance = (CFlashDriver *)fAllocator->allocate(classInfo->size());
					XFAILNOT(instance, err = kOSErrNoMemory;)
					classInfo->makeAt(instance);
					fDriver[fNumOfDrivers++] = instance;
					XFAIL(err = instance->init(*fAllocator))
					XFAIL(fNumOfDrivers == 6)
				}
			}
			XFAIL(err)
			XFAIL(fNumOfDrivers == 6)
		}
#endif
		if (fNumOfDrivers == 0)
		{
			// make default driver
#if defined(correct)
			const CClassInfo * classInfo = C28F016_SA_SVDriver::classInfo;
			CFlashDriver * instance = (CFlashDriver *)fAllocator->allocate(classInfo->size());
			XFAILNOT(instance, err = kOSErrNoMemory;)
			classInfo->makeAt(instance);
#else
			CFlashDriver * instance = new CPseudoFlashDriver;
			XFAILNOT(instance, err = kOSErrNoMemory;)
#endif
			fDriver[fNumOfDrivers++] = instance;
			err = instance->init(*fAllocator);
		}
	}
	XENDTRY;
	return err;
}


bool
CNewInternalFlash::findDriverAble(CFlashDriver*& outDriver, VAddr inAddr, eMemoryLane inLane, SFlashChipInformation& outInfo)
{
	for (ArrayIndex i = 0; i < fNumOfDrivers; ++i)
	{
		CFlashDriver * driver = fDriver[i];
		if (driver->identify(inAddr, inLane, outInfo))
		{
			outDriver = driver;
			return YES;
		}
	}
	return NO;
}


bool
CNewInternalFlash::checkFor4LaneFlash(VAddr inAddr, SFlashChipInformation& outInfo, CFlashDriver*& outDriver)
{
	CFlashDriver * lane1Driver;
	CFlashDriver * lane2Driver;
	CFlashDriver * lane3Driver;
	CFlashDriver * lane4Driver;
	SFlashChipInformation lane1Info;
	SFlashChipInformation lane2Info;
	SFlashChipInformation lane3Info;
	SFlashChipInformation lane4Info;

	if (findDriverAble(lane1Driver, inAddr, kMemoryLane16Bit1, lane1Info)
	&&  findDriverAble(lane2Driver, inAddr, kMemoryLane16Bit2, lane2Info)
	&&  lane2Driver == lane1Driver
	&&  lane2Info == lane1Info)
	{
		outInfo = lane1Info;
		outDriver = lane1Driver;
		return YES;
	}

	if (findDriverAble(lane1Driver, inAddr, kMemoryLane8Bit2, lane1Info)
	&&  findDriverAble(lane2Driver, inAddr, kMemoryLane8Bit4, lane2Info)
	&&  findDriverAble(lane3Driver, inAddr, kMemoryLane8Bit3, lane3Info)
	&&  findDriverAble(lane4Driver, inAddr, kMemoryLane8Bit1, lane4Info)
	&&  lane2Driver == lane1Driver
	&&  lane3Driver == lane1Driver
	&&  lane4Driver == lane1Driver
	&&  lane2Info == lane1Info
	&&  lane3Info == lane1Info
	&&  lane4Info == lane1Info)
	{
		outInfo = lane1Info;
		outDriver = lane1Driver;
		return YES;
	}
	
	return YES;//NO;
}

bool
CNewInternalFlash::checkFor2LaneFlash(VAddr, SFlashChipInformation&, CFlashDriver*&, eMemoryLane)
{ return NO; }

bool
CNewInternalFlash::checkFor1LaneFlash(VAddr, SFlashChipInformation&, CFlashDriver*&, eMemoryLane)
{ return NO; }


NewtonErr
CNewInternalFlash::initializeState(CMemoryAllocator * inAllocator, eInitHWOption inOptions)
{
	NewtonErr err = noErr;
	XTRY
	{
		fAllocator = inAllocator;
		fNumOfDrivers = 0;
		fNumOfRanges = 0;
		fBlockSize = 0;
		fStoreSize = 0;
		fBlockMap = NULL;
		fNumOfBlocks = 0;
		f64 = kNoBlockIndex;
		f44 = NULL;
		fEraseErr = noErr;
		f58 = NO;
#if defined(correct)
		if (IsSuperMode())
			fSemaphore = NULL;
		else
			XFAILNOT(fSemaphore = new CULockingSemaphore, err = kOSErrNoMemory;)
#endif
		f54 = inOptions;
		fBCR = CBankControlRegister::getBankControlRegister();
		err = searchForFlashDrivers();
	}
	XENDTRY;
	return err;
}


void
CNewInternalFlash::acknowledgeReset(void)
{ /* the original really does nothing */ }


NewtonErr
CNewInternalFlash::read(ZAddr inAddr, size_t inLength, char * inBuffer)
{
//	CULockingSemaphoreGrabber grab(fSemaphore);
	return readWrite(&CFlashRange::read, inAddr, inLength, inBuffer);
}

NewtonErr
CNewInternalFlash::write(ZAddr inAddr, size_t inLength, char * inBuffer)
{
//	CULockingSemaphoreGrabber grab(fSemaphore);
	return readWrite(&CFlashRange::write, inAddr, inLength, inBuffer);
}


NewtonErr
CNewInternalFlash::readWrite(FlashRWProcPtr inRW, ZAddr inAddr, size_t inLength, char * inBuffer)
{
	NewtonErr err = noErr;
	size_t blockLen;
	for (size_t lenRemaining = inLength; lenRemaining > 0; lenRemaining -= blockLen, inAddr += blockLen, inBuffer += blockLen)
	{
		ArrayIndex blockIndex = inAddr / fBlockSize;
		ArrayIndex offset = inAddr % fBlockSize;
		blockLen = fBlockSize - offset;
		if (blockLen > lenRemaining)
			blockLen = lenRemaining;
		// map physical block
		ZAddr mappedAddr = fBlockMap[blockIndex] * fBlockSize + offset;
		XFAIL(err = readWritePhysical(inRW, mappedAddr, blockLen, inBuffer))
	}
	return err;
}


NewtonErr
CNewInternalFlash::readPhysical(ZAddr inAddr, size_t inLength, char * inBuffer)
{
	return readWritePhysical(&CFlashRange::read, inAddr, inLength, inBuffer);
}


NewtonErr
CNewInternalFlash::writePhysical(ZAddr inAddr, size_t inLength, char * inBuffer)
{
	NewtonErr err;
	ArrayIndex numOfAttempts = 0;
	do {
		err = readWritePhysical(&CFlashRange::write, inAddr, inLength, inBuffer);
		if (err == kFlashErrVppLow && ++numOfAttempts == 2)
			PowerOffAndReboot(kOSErrRebootPowerFault);
	} while (err == kFlashErrVppLow);
	return err;
}


NewtonErr
CNewInternalFlash::readWritePhysical(FlashRWProcPtr inRW, ZAddr inAddr, size_t inLength, char * inBuffer)
{
	for (ArrayIndex i = 0; i < fNumOfRanges; ++i)
	{
		CFlashRange * range = fRange[i];
		ZAddr rangeEndAddr = range->fRangeAddr + range->fRangeSize;
		if (inAddr < rangeEndAddr)
		{
			size_t lenAvailable = rangeEndAddr - inAddr;
			if (inLength > lenAvailable)
				return kFlashErrAddressOutOfRange;
			return (range->*inRW)(inAddr, lenAvailable < inLength ? lenAvailable : inLength, inBuffer);
		}
	}
	return kFlashErrAddressOutOfRange;
}


void
CNewInternalFlash::clobber(void)
{
//	CULockingSemaphoreGrabber grab(fSemaphore);
	return internalClobber();
}


void
CNewInternalFlash::internalClobber(void)
{
	NewtonErr err = noErr;

	turnPowerOn();
	XTRY
	{
		// erase data in all ranges to 0xFFFFFFFF
		for (ArrayIndex i = 0; i < fNumOfRanges; ++i)
		{
			XFAILIF(err = fRange[i]->eraseRange(), Reboot();)
		}
		// write 0xXXXX00FF where XXXX is the block index to the first word of every block
		FlashBlockInfo blockInfo;
		blockInfo.type = 0x00FF;
		ZAddr blockAddr = 0;
		for (ArrayIndex i = 0; i < fNumOfBlocks - 1; i++, blockAddr += fBlockSize)
		{
			blockInfo.index = i;
			XFAILIF(err = writePhysical(blockAddr, sizeof(blockInfo), (char *)&blockInfo), Reboot();)
		}
		if (fBlockMap)
			XFAILIF(err = setupVirtualMappings(), Reboot();)
	}
	XENDTRY;
	turnPowerOff();
}


NewtonErr
CNewInternalFlash::gatherBlockMappingInfo(ULong & ioArg1, ULong & ioArg2, ULong & ioArg3, ULong & ioArg4)
{
	NewtonErr err;

	FlashBlockInfo blockInfo;
	blockInfo.index = kNoBlockIndex;
	blockInfo.type = 0xFFFF;

	ArrayIndex numOfBlocks = fStoreSize / fBlockSize;	// sp00
	ArrayIndex lastBlock = numOfBlocks - 1;	// r8

	XTRY
	{
		ZAddr blockAddr = 0;	// r7
		// iterate over all blocks
		for (ArrayIndex i = 0; i < numOfBlocks; i++, blockAddr += fBlockSize)
		{
			// read the first word
			XFAIL(err = readPhysical(blockAddr, sizeof(blockInfo), (char *)&blockInfo))
			if (blockInfo.type == 0x00FF)
			{
				ArrayIndex blockIndex = blockInfo.index;
				if (blockIndex < lastBlock)
				{
					// it’s a valid block
					// ensure it hasn’t already been mapped
					XFAILNOT(fBlockMap[blockIndex] == kNoBlockIndex, err = kStoreErrNeedsFormat;)
					// record its physical index
					fBlockMap[blockIndex] = i;
				}
				else
				{
					XFAILNOT(ioArg3 == kNoBlockIndex, err = kStoreErrNeedsFormat;)		// blockIndex out of range
					ioArg3 = i;
				}
			}
			else if (blockInfo.type == 0xFFFF)
			{
				if (blockInfo.index == kNoBlockIndex)
				{
					XFAILNOT(ioArg1 == kNoBlockIndex, err = kStoreErrNeedsFormat;)
					ioArg1 = i;
				}
			}
			else if (blockInfo.type == 0x000F)
			{
				XFAILNOT(ioArg2 == kNoBlockIndex, err = kStoreErrNeedsFormat;)
				ioArg2 = i;
			}
			else
			{
				XFAILNOT(ioArg3 == kNoBlockIndex, err = kStoreErrNeedsFormat;)			// unrecognised blockInfo.type
				ioArg3 = i;
			}
		}
		XFAIL(err)

		for (ArrayIndex i = 0; i < lastBlock; ++i)
		{
			if (fBlockMap[i] == kNoBlockIndex)
			{
				// there’s an unmapped block
				XFAILNOT(ioArg4 == kNoBlockIndex, err = kStoreErrNeedsFormat;)
				ioArg4 = 0;
			}
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
CNewInternalFlash::setupVirtualMappings(void)
{
	NewtonErr err;

	XTRY
	{
		// initialize the block map: all entries with 0xFFFF (an invalid index)
		for (ArrayIndex i = 0; i < fNumOfBlocks - 1; ++i)
			fBlockMap[i] = kNoBlockIndex;

		ULong sp0C = kNoBlockIndex;
		ULong sp08 = kNoBlockIndex;
		ULong badBlockInfo = kNoBlockIndex;
		ULong unmappedBlock = kNoBlockIndex;
		XFAIL(err = gatherBlockMappingInfo(sp0C, sp08, badBlockInfo, unmappedBlock))

		ArrayIndex r0 = 0;
		if (unmappedBlock != kNoBlockIndex)
			r0 |= 0x08;
		if (badBlockInfo != kNoBlockIndex)
			r0 |= 0x04;
		if (sp08 != kNoBlockIndex)
			r0 |= 0x02;
		if (sp0C != kNoBlockIndex)
			r0 |= 0x01;
		char validity[16] = { '0','1','1','1', '1','0','0','0',
									 '0','0','0','1', '0','0','0','0' };
		XFAILIF(validity[r0] != '1', err = kStoreErrNeedsFormat;)

		if (sp0C != kNoBlockIndex)
			f64 = sp0C;
		if (sp08 != kNoBlockIndex)
		{
			syncErasePhysicalBlock(sp08*fBlockSize);
			f64 = sp08;
		}
		else if (badBlockInfo != kNoBlockIndex)
		{
			syncErasePhysicalBlock(badBlockInfo*fBlockSize);
			f64 = badBlockInfo;
		}
	}
	XENDTRY;

	return err;
}


bool
CNewInternalFlash::internalCheckEraseCompletion(NewtonErr& outErr, eCheckEraseOption inOption)
{
	outErr = fEraseErr;
	if (f44 == NULL)
		return YES;

	for (;;)
	{
		if (f44->isEraseComplete(outErr))
		{
			f44 = NULL;
			if (outErr)
				fEraseErr = outErr;
			return YES;
		}

		if (inOption == kEraseAsync)
			return NO;

		Sleep(5 * kMilliseconds);
	}
}


NewtonErr
CNewInternalFlash::syncErasePhysicalBlock(PAddr inAddr)
{
	NewtonErr err;
	XTRY
	{
		CFlashRange * range;
		XFAIL(err = findRange(inAddr, range))
		internalCheckEraseCompletion(err, kEraseSync);
		XFAIL(err)
		err = range->syncErase(inAddr, fBlockSize);
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewInternalFlash::erase(PAddr inAddr)
{
//	CULockingSemaphoreGrabber grab(fSemaphore);
	NewtonErr err;
	XTRY
	{
		internalCheckEraseCompletion(err, kEraseSync);
		if (err)
		{
			fEraseErr = noErr;
			XFAIL(err = syncErasePhysicalBlock(f64 * fBlockSize))
		}
		ArrayIndex blockNo = inAddr / fBlockSize;		// r7
		ArrayIndex mappedBlockNo = fBlockMap[blockNo];
		PAddr blockBase = mappedBlockNo * fBlockSize;		// r6
		ZAddr r10 = f64 * fBlockSize;

		FlashBlockInfo blockInfo;
		blockInfo.index = blockNo;
		blockInfo.type = 0x00FF;
		XFAIL(err = writePhysical(blockBase, sizeof(blockInfo), (char *)&blockInfo))

		blockInfo.index = blockNo;
		blockInfo.type = 0x00FF;
		XFAIL(err = writePhysical(r10, sizeof(blockInfo), (char *)&blockInfo))

		blockInfo.index = 0;
		blockInfo.type = 0x0000;
		XFAIL(err = writePhysical(blockBase, sizeof(blockInfo), (char *)&blockInfo))

		fBlockMap[blockNo] = f64;
		f64 = mappedBlockNo;

		CFlashRange * range;
		XFAIL(err = findRange(blockBase, range))

		internalVppOn();
		XFAIL(err = range->startErase(blockBase, fBlockSize))
		internalVppOff();
	}
	XENDTRY;
	XDOFAIL(err)
	{
		PowerOffAndReboot(kOSErrRebootPowerFault);
	}
	XENDFAIL;
	return err;
}


NewtonErr
CNewInternalFlash::copyUsingBuffer(ZAddr inFromAddr, ZAddr inToAddr, size_t inLength, void * inBuffer, size_t inBufLen)
{
	NewtonErr err = noErr;
	XTRY
	{
		size_t blockLen;
		for (size_t lenRemaining = inLength; lenRemaining > 0; inFromAddr += blockLen, inToAddr += blockLen, lenRemaining -= blockLen)
		{
			blockLen = lenRemaining < inBufLen ? lenRemaining : inBufLen;
			XFAIL(err = read(inFromAddr, blockLen, (char *)inBuffer))
			XFAIL(err = write(inToAddr, blockLen, (char *)inBuffer))
		}
	}
	XENDTRY;
	return err;
}


NewtonErr
CNewInternalFlash::copy(ZAddr inFromAddr, ZAddr inToAddr, size_t inLength)
{
#define kCopyBufLen 256

	char buffer[kCopyBufLen];
	return copyUsingBuffer(inFromAddr, inToAddr, inLength, buffer, kCopyBufLen);
}


bool
CNewInternalFlash::isVirgin(ZAddr inAddr, size_t inLength)
{
//	CULockingSemaphoreGrabber grab(fSemaphore);
	if (inAddr > fStoreSize - fBlockSize)
		// improbable address
		return NO;

	size_t blockLen;
	for (size_t lenRemaining = inLength; lenRemaining > 0; inAddr += blockLen, lenRemaining -= blockLen)
	{
		ArrayIndex blockNo = inAddr / fBlockSize;
		ArrayIndex blockOffset = inAddr % fBlockSize;	// offset into block
		blockLen = fBlockSize - blockOffset;				// remainder of block
		if (blockLen > lenRemaining)
			blockLen = lenRemaining;

		CFlashRange * range;
		PAddr blockBase = fBlockMap[blockNo] * fBlockSize;
		if (findRange(blockBase, range) != noErr)
			return NO;

		if (blockOffset == 0)
		{
			// don’t look at the first word in the block
			blockOffset = 4;
			blockLen -= 4;
		}
		if (!range->isVirgin(blockBase + blockOffset, blockLen))
			return NO;
	}
	return YES;
}


NewtonErr
CNewInternalFlash::status(ULong)
{
	// this is what the original does
	return 1;
}


ULong
CNewInternalFlash::getAttributes(void)
{
#if 0
	// this is what the original does
	return f58 ? 0x08 : 0x18;
#else
	return 0;
#endif
}


void
CNewInternalFlash::getWriteProtected(bool * outWP)
{
	// this is what the original does
	*outWP = NO;
}


size_t
CNewInternalFlash::getTotalSize(void)
{
	return fStoreSize - fBlockSize;
}


size_t
CNewInternalFlash::getEraseRegionSize(void)
{
	return fBlockSize;
}

#pragma mark Stubs
/*--------------------------------------------------------------------------- */

void
CNewInternalFlash::suspendErase(ULong, ULong, ULong)
{}

void
CNewInternalFlash::resumeErase(ULong)
{}

void
CNewInternalFlash::deepSleep(ULong)
{}

void
CNewInternalFlash::wakeup(ULong)
{}

void
CNewInternalFlash::resetCard(void)
{}

void
CNewInternalFlash::getPhysResource(void)
{}

void
CNewInternalFlash::registerClientInfo(ULong)
{}

void
CNewInternalFlash::getWriteErrorAddress(void)
{}

ULong
CNewInternalFlash::getDataOffset(void)
{ return 0; }

size_t
CNewInternalFlash::getGroupSize(void)
{ return 0; }

ULong
CNewInternalFlash::getChipsPerGroup(void)
{ return 0; }

ULong
CNewInternalFlash::getBlocksPerPartition(void)
{ return 0; }

ULong
CNewInternalFlash::getMaxConcurrentVppOps(void)
{ return 0; }

ULong
CNewInternalFlash::getEraseRegionCurrent(void)
{ return 0; }

ULong
CNewInternalFlash::getWriteRegionCurrent(void)
{ return 0; }

ULong
CNewInternalFlash::getEraseRegionTime(void)
{ return 0; }

ULong
CNewInternalFlash::getWriteAccessTime(void)
{ return 0; }

ULong
CNewInternalFlash::getReadAccessTime(void)
{ return 0; }

ULong
CNewInternalFlash::getVendorInfo(void)
{ return 0; }

int
CNewInternalFlash::getSocketNumber(void)
{}

ULong
CNewInternalFlash::vppStatus(void)
{ return 0; }

ULong
CNewInternalFlash::vppRisingTime(void)
{ return 0; }

void
CNewInternalFlash::flashSpecific(ULong, void*, ULong)
{}

NewtonErr
CNewInternalFlash::initialize(CCardSocket*, CCardPCMCIA*, ULong, ULong)
{ return noErr; }

NewtonErr
CNewInternalFlash::suspendService(void)
{ return noErr; }

NewtonErr
CNewInternalFlash::resumeService(CCardSocket*, CCardPCMCIA*, ULong)
{ return noErr; }


void
CNewInternalFlash::turnPowerOn(void)
{}

void
CNewInternalFlash::turnPowerOff(void)
{}

void
CNewInternalFlash::internalVppOn(void)
{}

void
CNewInternalFlash::internalVppOff(void)
{}

#pragma mark -
/* -----------------------------------------------------------------------------
	C U L o c k i n g S e m a p h o r e G r a b b e r
----------------------------------------------------------------------------- */
#if defined(correct)
CULockingSemaphoreGrabber::CULockingSemaphoreGrabber(CULockingSemaphore * inSemaphore)
{
	fSemaphore = inSemaphore;
	if (fSemaphore)
		fSemaphore->acquire(kWaitOnBlock);
}

CULockingSemaphoreGrabber::CULockingSemaphoreGrabber(CULockingSemaphore * inSemaphore, eNonBlockOption inOption)
{
	if (inSemaphore && inSemaphore->acquire(kNoWaitOnBlock) == noErr)
		fSemaphore = inSemaphore;
	else
		fSemaphore = NULL;
}

CULockingSemaphoreGrabber::~CULockingSemaphoreGrabber()
{
	if (fSemaphore)
		fSemaphore->release();
}

void
CULockingSemaphoreGrabber::doAcquire(CULockingSemaphore * inSemaphore)
{
	if (fSemaphore)
		fSemaphore->release();
	fSemaphore = inSemaphore;
	if (fSemaphore)
		fSemaphore->acquire(kWaitOnBlock);
}
#endif
