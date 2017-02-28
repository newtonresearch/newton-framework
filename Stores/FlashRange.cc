/*
	File:		FlashRange.cc

	Contains:	Newton flash memory range implementation.

	Written by:	Newton Research Group, 2014.
*/

#include "FlashDriver.h"
#include "OSErrors.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
extern "C" const char * StoreBackingFile(const char * inStoreName);

#pragma mark -
/* -----------------------------------------------------------------------------
	C H e a p A l l o c a t o r
----------------------------------------------------------------------------- */
static CMemoryAllocator * gHeapAllocator = NULL;

CMemoryAllocator *
CHeapAllocator::getGlobalAllocator(void)
{
	if (gHeapAllocator == NULL)
		gHeapAllocator = new CHeapAllocator;
	return gHeapAllocator;
}

void *
CHeapAllocator::allocate(size_t inSize)
{ return NewPtr(inSize); }	// original uses new()

void
CHeapAllocator::deallocate(void * inBlock)
{ return FreePtr((Ptr)inBlock); }	// original uses delete()


#pragma mark -
/* -----------------------------------------------------------------------------
	C N o R e u s e A l l o c a t o r
	There’s no init function, but someone’s gotta init fMem|fSize.
----------------------------------------------------------------------------- */

void *
CNoReuseAllocator::allocate(size_t inSize)
{
	if (inSize > fSize)
		return NULL;
	char * p = fMem;
	fMem = p + inSize;
	fSize -= inSize;
	return p;
}

void
CNoReuseAllocator::deallocate(void * inBlock)
{ }


#pragma mark -
/*------------------------------------------------------------------------------
	C B a n k C o n t r o l R e g i s t e r
------------------------------------------------------------------------------*/
ULong	g0F241000;								// bank control hardware register
static CBankControlRegister	gBCR;		// 0C1008C8		can do this b/c it has no ctor

CBankControlRegister *
CBankControlRegister::getBankControlRegister()
{
	if (gBCR.f04 == 0)
		gBCR.f04 = 1;
	return &gBCR;
}

NewtonErr
CBankControlRegister::configureFlashBankDataSize(eMemoryLane inLane)
{
	ULong arg1;
	if (inLane == kMemoryLane32Bit)
		arg1 = 0x0000;
	else if (inLane == kMemoryLane8Bit1)
		arg1 = 0x0400;
	else if (inLane == kMemoryLane16Bit1)
		arg1 = 0x0200;
	else if (inLane == kMemoryLane8Bit3)
		arg1 = 0x0500;
	else if (inLane == kMemoryLane16Bit2)
		arg1 = 0x0300;
	else
		return kFlashErrUnsupportedConfiguration;		// original says +10555

	setBankControlRegister(arg1, 0x0700);
	return noErr;
}

ULong
CBankControlRegister::setBankControlRegister(ULong inArg1, ULong inArg2)
{
	g0F241000 = ((inArg2 & ~g0F241000) & 0x7FF) | inArg1;
	return g0F241000;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C F l a s h R a n g e
------------------------------------------------------------------------------*/

CFlashRange::CFlashRange(CFlashDriver * inDriver, VAddr inFlashAddr, VAddr inReadAddr, VAddr inWriteAddr, eMemoryLane inLanes, const SFlashChipInformation& info, CMemoryAllocator& inAllocator)
{
	fDriver = inDriver;
	fRangeAddr = inFlashAddr;	// base address of range; first range starts at 0
	fReadVAddr = inReadAddr;
	fWriteVAddr = inWriteAddr;
	fDataLanes = inLanes;
	fFlashChipInfo = info;
	fEraseLen = 0;

	ArrayIndex numOfLanes = 0;
	ULong laneMask = 0xFF;
	for (ArrayIndex i = 0; i < 4; i++, laneMask <<= 8)		// kMaxMemoryLanes?
	{
		if ((inLanes & laneMask) != 0)
			numOfLanes++;
	}
	fDataFactor = numOfLanes / info.x0C;
	fRangeSize = info.x10 * fDataFactor;
	fBlockSize = info.x14 * fDataFactor;
	fDataWidth = numOfLanes;

	inDriver->initializeDriverData(*this, inAllocator);

#if !defined(correct)
	const char * fpath = StoreBackingFile("Internal");
	// might want to delete this if it already exists so we always start from scratch
	bool isNew = false;
	fd = open(fpath, O_RDWR);
	if (fd < 0)
	{
		// create new backing file EINVAL
		fd = open(fpath, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU|S_IRWXG);
		if (fd < 0)
		{
			printf("FAILED TO CREATE STORE BACKING FILE!\n");
			exit(1);
		}
		ftruncate(fd, fRangeSize);
		isNew = true;
	}
	fReadVAddr = fWriteVAddr = (VAddr)mmap(NULL, fRangeSize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_SHARED, fd, 0);
//	DEBUG: always start with a virgin store
	if (isNew)
		memset((void *)fWriteVAddr, 0xFF, fRangeSize);	// set virgin bits
	close(fd);
#endif
}


CFlashRange::~CFlashRange()
{ }


void
CFlashRange::dlete(CMemoryAllocator & inAllocator)
{
	fDriver->cleanUpDriverData(*this, inAllocator);

#if !defined(correct)
	//unmap disk file
	if (fd != -1)
		munmap((void *)fReadVAddr, fRangeSize);
#endif
}


void
CFlashRange::earlyPrepareForReadingArray(void)
{
	startReadingArray();
}

void
CFlashRange::startReadingArray(void)
{
	CBankControlRegister * bcr = CBankControlRegister::getBankControlRegister();
	bcr->configureFlashBankDataSize(kMemoryLane32Bit);
	fDriver->startReadingArray(*this);
	if (fDataLanes != kMemoryLane32Bit)
		bcr->configureFlashBankDataSize(fDataLanes);
}

void
CFlashRange::doneReadingArray(void)
{
	CBankControlRegister * bcr = CBankControlRegister::getBankControlRegister();
	bcr->configureFlashBankDataSize(kMemoryLane32Bit);
	fDriver->doneReadingArray(*this);
}


NewtonErr
CFlashRange::read(VAddr inAddr, size_t inLength, char * inBuffer)
{
	VAddr addr = fReadVAddr + (inAddr - fRangeAddr);
	startReadingArray();
	memmove(inBuffer, (void *)addr, inLength);
	doneReadingArray();
	return noErr;
}


NewtonErr
CFlashRange::write(VAddr inAddr, size_t inLength, char * inBuffer)
{
	NewtonErr err = noErr;
	XTRY
	{
		VAddr addr = fReadVAddr + (inAddr - fRangeAddr);
		flushDataCache(addr, inLength);

		size_t blockLen;
		for (size_t lenRemaining = inLength; lenRemaining > 0; lenRemaining -= blockLen, inAddr += blockLen, inBuffer += blockLen)
		{
			blockLen = lenRemaining;
			if (startOfBlockFlashAddress(inAddr) + fBlockSize < inAddr + lenRemaining)
				blockLen -= (inAddr - startOfBlockFlashAddress(inAddr));
			XFAIL(err = doWrite(inAddr, blockLen, inBuffer))
		}

	}
	XENDTRY;
	return err;
}


VAddr
CFlashRange::startOfBlockFlashAddress(VAddr inAddr) const
{
	return fRangeAddr + fBlockSize * ((inAddr - fRangeAddr) / fBlockSize);
}


bool
CFlashRange::isVirgin(VAddr inAddr, size_t inLength)
{
	startReadingArray();
	VAddr addr = fReadVAddr + (inAddr - fRangeAddr);
	size_t longLen;
	size_t byteLen = inLength;
	bool isSo = true;
	// check bytes up to 32-bit alignment
	for ( ; (addr & 0x03) != 0 && byteLen != 0; addr++, byteLen--)
		if (*(uint8_t *)addr != 0xFF)
		{
			isSo = false;
			break;
		}
	if (isSo)
	{
		longLen = TRUNC(byteLen, 4);
		// check 32-bit words
		for (size_t count = longLen; count != 0; addr += 4, count -= 4)
			if (*(uint32_t *)addr != 0xFFFFFFFF)
			{
				isSo = false;
				break;
			}
	}
	if (isSo)
	{
		byteLen -= longLen;
		// check bytes to end of range
		for ( ; byteLen != 0; addr++, byteLen--)
			if (*(uint8_t *)addr != 0xFF)
			{
				isSo = false;
				break;
			}
	}
	doneReadingArray();
	return isSo;
}


void
CFlashRange::resetAllBlocksStatus(void)
{
	for (VAddr blockAddr = fRangeAddr; blockAddr < fRangeAddr + fRangeSize; blockAddr += fBlockSize)
	{
		fDriver->resetBlockStatus(*this, prepareForBlockCommand(blockAddr));
	}
}


void
CFlashRange::lockBlock(VAddr inAddr)
{
	fDriver->lockBlock(*this, prepareForBlockCommand(inAddr));
}


void
CFlashRange::flushDataCache(VAddr inAddr, size_t inLength) const
{
#if defined(correct)
	if (IsSuperMode())
		CleanRangeInDCSWIGlue(inAddr, inAddr+inLength);
	else
		GenericSWI(k73GenericSWI, inAddr, inAddr+inLength);
#endif
}


NewtonErr
CFlashRange::eraseRange(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		for (VAddr blockAddr = fRangeAddr; blockAddr < fRangeAddr + fRangeSize; blockAddr += fBlockSize)
		{
			XFAIL(err = syncErase(blockAddr, fBlockSize))
		}
	}
	XENDTRY;
	return err;
}


NewtonErr
CFlashRange::syncErase(VAddr inAddr, size_t inLength)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAIL(err = startErase(inAddr, inLength))
#if defined(correct)
		while (!isEraseComplete(err))
		{
			if (IsSuperMode())
				ShortTimerDelay(1*kMilliseconds);
			else
				Sleep(5*kMilliseconds);
		};
#endif
	}
	XENDTRY;
	return err;
}


NewtonErr
CFlashRange::startErase(VAddr inAddr, size_t inLength)
{
	NewtonErr err = noErr;
	XTRY
	{
		VAddr addr = fReadVAddr + (inAddr - fRangeAddr);
		flushDataCache(addr, inLength);
		fEraseAddr = inAddr;
		fEraseLen = inLength;
		for (size_t lenRemaining = inLength; lenRemaining > 0; lenRemaining -= fBlockSize, inAddr += fBlockSize)
		{
			XFAILIF(err = fDriver->startErase(*this, prepareForBlockCommand(inAddr)), fEraseLen = 0;)
		}
	}
	XENDTRY;
	return err;
}


bool
CFlashRange::isEraseComplete(NewtonErr& outErr)
{ return true; }


#pragma mark -
/*------------------------------------------------------------------------------
	C 3 2 B i t F l a s h R a n g e
------------------------------------------------------------------------------*/

C32BitFlashRange::C32BitFlashRange(CFlashDriver * inDriver, VAddr inFlashAddr, VAddr inReadAddr, VAddr inWriteAddr, eMemoryLane inLanes, const SFlashChipInformation& info, CMemoryAllocator& inAllocator)
	:	CFlashRange(inDriver, inFlashAddr, inReadAddr, inWriteAddr, inLanes, info, inAllocator)
{ }


ULong
FetchReallyUnalignedWord(char * inBuf)
{
	ULong misAlignment = (VAddr)inBuf & 3;
	ULong * ptr = (ULong *)TRUNC(inBuf, 4);
#if defined(hasByteSwapping)
	ULong word1 = *ptr >> (misAlignment * 8);
	ULong word2 = *(ptr+1) << ((4 - misAlignment) * 8);
#else
	ULong word1 = *ptr << (misAlignment * 8);
	ULong word2 = *(ptr+1) >> ((4 - misAlignment) * 8);
#endif
/*	original does this, but redundant if shifting unsigned words, surely?
	if (misAlignment == 1)
		word2 &= 0x000000FF;
	else if (misAlignment == 2)
		word2 &= 0x0000FFFF;
	else if (misAlignment == 3)
		word2 &= 0x00FFFFFF; */
	return word1 | word2;
}


VAddr
C32BitFlashRange::startOfBlockWriteVirtualAddress(VAddr inAddr) const
{
	return fWriteVAddr + fBlockSize * (inAddr - fWriteVAddr)/fBlockSize;
}


void
C32BitFlashRange::adjustVirtualAddresses(long inDelta)
{
	fReadVAddr += inDelta;
	fWriteVAddr += inDelta;
}


NewtonErr
C32BitFlashRange::doWrite(VAddr inAddr, size_t inLength, char * inBuffer)
{
//r4: r7 r6 r5
	CBankControlRegister::getBankControlRegister()->configureFlashBankDataSize(kMemoryLane32Bit);

	VAddr vAddr = fWriteVAddr + (inAddr - fRangeAddr);	//r10
	ULong misAlignment = vAddr & 0x03;
	ULong bytesInFirstWord = 4 - misAlignment;	// r8
	ULong numOfWords = (bytesInFirstWord == 0) ? 0 : 1;	// r3	also means num of writes
	if (inLength > bytesInFirstWord)
	{
		ULong delta = inLength - bytesInFirstWord;
		numOfWords += delta / 4;				// num of bytes -> words
		if ((delta & 0x03) != 0)
			numOfWords++;
	}

	VAddr addr = TRUNC(vAddr, 4);	// always write whole words

	fDriver->beginWrite(*this, addr, numOfWords);
	if (misAlignment)
	{
		// there are bytes preceding the first aligned word in the source
		// fetch word from the source
		ULong word = MISALIGNED(inBuffer) ? FetchReallyUnalignedWord(inBuffer) : *(ULong *)inBuffer;
		// generate a byte mask for the misaligned bytes
		ULong mask;
#if defined(hasByteSwapping)
		word <<= misAlignment * 8;
		mask = 0xFFFFFFFF << (misAlignment * 8);
		if (inLength < bytesInFirstWord)
		{
			if (inLength == 1 && bytesInFirstWord != 2)
			{
				word &= 0x0000FFFF;
				mask &= 0x0000FFFF;
			}
			else
			{
				word &= 0x00FFFFFF;
				mask &= 0x00FFFFFF;
			}
			bytesInFirstWord = inLength;
		}
#else
		word >>= misAlignment * 8;
		mask = 0xFFFFFFFF >> (misAlignment * 8);
		if (inLength < bytesInFirstWord)
		{
			if (inLength == 1 && bytesInFirstWord != 2)
			{
				word &= 0xFFFF0000;
				mask &= 0xFFFF0000;
			}
			else
			{
				word &= 0xFFFFFF00;
				mask &= 0xFFFFFF00;
			}
			bytesInFirstWord = inLength;
		}
#endif
		fDriver->write(word, mask, addr, *this);
		inLength -= bytesInFirstWord;
		addr += 4;
		inBuffer += bytesInFirstWord;
	}

	// write aligned words
	size_t wordAlignedLen = TRUNC(inLength,4);
	inLength -= wordAlignedLen;
	for ( ; wordAlignedLen > 0; wordAlignedLen -= 4, addr += 4, inBuffer += 4)
	{
		// fetch word from the source
		ULong word = MISALIGNED(inBuffer) ? FetchReallyUnalignedWord(inBuffer) : *(ULong *)inBuffer;
		// write it, no masking
		fDriver->write(word, 0xFFFFFFFF, addr, *this);
	}

	if (inLength)
	{
		// there’s a sub-word remaining
		// fetch word from the source
		ULong word = MISALIGNED(inBuffer) ? FetchReallyUnalignedWord(inBuffer) : *(ULong *)inBuffer;
		// generate a byte mask for the bytes in the 32-bit word
#if defined(hasByteSwapping)
		ULong mask = 0xFFFFFFFF >> ((4 - inLength) * 8);
#else
		ULong mask = 0xFFFFFFFF << ((4 - inLength) * 8);
#endif
		// write it
		fDriver->write(word & mask, mask, addr, *this);
	}

	return fDriver->reportWriteResult(*this, startOfBlockWriteVirtualAddress(vAddr));
}


VAddr
C32BitFlashRange::prepareForBlockCommand(VAddr inAddr)
{
	CBankControlRegister::getBankControlRegister()->configureFlashBankDataSize(kMemoryLane32Bit);
	return fWriteVAddr + (inAddr - fRangeAddr);
}

