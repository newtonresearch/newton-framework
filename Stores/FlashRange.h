/*
	File:		FlashRange.h

	Contains:	Newton flash memory interface.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__FLASHRANGE_H)
#define __FLASHRANGE_H 1

#include "PSSTypes.h"
#include "Protocols.h"
#include "VirtualMemory.h"


/*------------------------------------------------------------------------------
	C M e m o r y A l l o c a t o r
	Looks a bit experimental.
	As does a lot of this, actually.
------------------------------------------------------------------------------*/

class CMemoryAllocator
{
public:
	virtual void *	allocate(size_t inSize) = 0;
	virtual void	deallocate(void * inBlock) = 0;
};

class CHeapAllocator : public CMemoryAllocator
{
public:
	static CMemoryAllocator * getGlobalAllocator(void);
	virtual void *	allocate(size_t inSize);
	virtual void	deallocate(void * inBlock);
};

class CNoReuseAllocator : public CMemoryAllocator
{
public:
	virtual void *	allocate(size_t inSize);
	virtual void	deallocate(void * inBlock);
private:
	char *	fMem;
	size_t	fSize;
};


/*------------------------------------------------------------------------------
	C B a n k C o n t r o l R e g i s t e r
------------------------------------------------------------------------------*/
enum eMemoryLane
{
	kMemoryLane8Bit1  = 0xFF000000,
	kMemoryLane8Bit2  = 0x00FF0000,
	kMemoryLane8Bit3  = 0x0000FF00,
	kMemoryLane8Bit4  = 0x000000FF,
	kMemoryLane16Bit1 = 0xFFFF0000,
	kMemoryLane16Bit2 = 0x0000FFFF,
	kMemoryLane32Bit  = 0xFFFFFFFF
};


class CBankControlRegister
{
public:
static CBankControlRegister *	getBankControlRegister(void);

	NewtonErr	configureFlashBankDataSize(eMemoryLane inLanes);
	ULong			setBankControlRegister(ULong inArg1, ULong inArg2);

private:
	ULong			f00;
	ULong			f04;
};


/*------------------------------------------------------------------------------
	C F l a s h R a n g e
------------------------------------------------------------------------------*/
#define kNoBlockIndex 0xFFFF

class CFlashDriver;

struct SFlashChipInformation
{
bool	operator==(const SFlashChipInformation& info) const;

	int	x00;
	int	x04;
	int	x08;
	int	x0C;
	int	x10;
	int	x14;
};


class CFlashRange
{
public:
								CFlashRange(CFlashDriver * inDriver, VAddr inFlashAddr, VAddr inReadAddr, VAddr inWriteAddr, eMemoryLane inLanes, const SFlashChipInformation& info, CMemoryAllocator& inAllocator);
	virtual					~CFlashRange();
	virtual void			dlete(CMemoryAllocator&);

	virtual VAddr			startOfBlockWriteVirtualAddress(VAddr inAddr) const = 0;
	virtual void			adjustVirtualAddresses(long inDelta) = 0;
	virtual NewtonErr		doWrite(VAddr inAddr, size_t inLength, char * inBuffer) = 0;
	virtual PAddr			prepareForBlockCommand(VAddr inAddr) = 0;

	void			earlyPrepareForReadingArray(void);
	void			startReadingArray(void);
	void			doneReadingArray(void);

	NewtonErr	read(VAddr inAddr, size_t inLength, char * inBuffer);
	NewtonErr	write(VAddr inAddr, size_t inLength, char * inBuffer);

	VAddr			startOfBlockFlashAddress(VAddr inAddr) const;
	bool			isVirgin(VAddr inAddr, size_t inLength);
	void			resetAllBlocksStatus(void);
	void			lockBlock(VAddr inAddr);
	void			flushDataCache(VAddr inAddr, size_t inLength) const;

	NewtonErr	eraseRange(void);
	NewtonErr	syncErase(VAddr inAddr, size_t inLength);
	NewtonErr	startErase(VAddr inAddr, size_t inLength);
	bool			isEraseComplete(NewtonErr& outErr);

//private:
	CFlashDriver *	fDriver;			//+04
	VAddr			fRangeAddr;			//+08		virtual address of range
	VAddr			fReadVAddr;			//+0C
	VAddr			fWriteVAddr;		//+10
	eMemoryLane	fDataLanes;			//+14		bitmap of bytes in a word 000000FF..FFFFFFFF
	SFlashChipInformation	fFlashChipInfo;	//+18
	size_t		fRangeSize;			//+30
	ULong			fDataFactor;		//+34
	ULong			fDataWidth;			//+38		number of bytes in a word 1..4
	ULong			fBlockSize;			//+3C
	ULong			fEraseAddr;			//+44
	ULong			fEraseLen;			//+48
#if !defined(correct)
private:
	int			fd;
#endif
};


/*------------------------------------------------------------------------------
	C 3 2 B i t F l a s h R a n g e
------------------------------------------------------------------------------*/

class C32BitFlashRange : public CFlashRange
{
public:
								C32BitFlashRange(CFlashDriver * inDriver, VAddr inFlashAddr, VAddr inReadAddr, VAddr inWriteAddr, eMemoryLane inLanes, const SFlashChipInformation& info, CMemoryAllocator& inAllocator);

	virtual VAddr			startOfBlockWriteVirtualAddress(VAddr inAddr) const;
	virtual void			adjustVirtualAddresses(long inDelta);
	virtual NewtonErr		doWrite(VAddr inAddr, size_t inLength, char * inBuffer);
	virtual PAddr			prepareForBlockCommand(VAddr inAddr);
};


#endif	/* __FLASHRANGE_H */
