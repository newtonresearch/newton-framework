/*
	File:		FlashDriver.cc

	Contains:	Newton flash memory driver implementation.

	Written by:	Newton Research Group, 2014.
*/

#include "FlashDriver.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	C F l a s h D r i v e r
	This should be a protocol interface.
------------------------------------------------------------------------------*/

NewtonErr
CFlashDriver::init(CMemoryAllocator&)
{ return noErr; }

void
CFlashDriver::initializeDriverData(CFlashRange& inRange, CMemoryAllocator&)
{}

bool
CFlashDriver::identify(VAddr, ULong, SFlashChipInformation&)
{ return NO; }

void
CFlashDriver::cleanUp(CMemoryAllocator&)
{}

void
CFlashDriver::cleanUpDriverData(CFlashRange& inRange, CMemoryAllocator&)
{}

void
CFlashDriver::startReadingArray(CFlashRange& inRange)
{}

void
CFlashDriver::doneReadingArray(CFlashRange& inRange)
{}

void
CFlashDriver::resetBlockStatus(CFlashRange& inRange, ULong)
{}

NewtonErr
CFlashDriver::startErase(CFlashRange& inRange, PAddr inBlockAddr)
{}

bool
CFlashDriver::isEraseComplete(CFlashRange& inRange, ULong, long&)
{ return YES; }

void
CFlashDriver::lockBlock(CFlashRange& inRange, ULong)
{}

void
CFlashDriver::beginWrite(CFlashRange& inRange, ULong inAddr, ULong inNumOfWords)
{}

void
CFlashDriver::write(ULong inWord, ULong inMask, PAddr inAddr, CFlashRange& inRange)
{}

NewtonErr
CFlashDriver::reportWriteResult(CFlashRange& inRange, ULong inAddr)
{ return noErr; }


#pragma mark -
/*------------------------------------------------------------------------------
	C P s e u d o F l a s h D r i v e r
------------------------------------------------------------------------------*/

NewtonErr
CPseudoFlashDriver::init(CMemoryAllocator& inAllocator)
{ return noErr; }

bool
CPseudoFlashDriver::identify(VAddr inAddr, ULong inLane, SFlashChipInformation& outInfo)
{
	outInfo.x00 = 0;
	outInfo.x04 = 0;
	outInfo.x08 = 0;
	outInfo.x0C = 4;			// bytes/word? ie number of lanes?
	outInfo.x10 = 4*MByte;	// store size?
	outInfo.x14 = 64*KByte;	// block size?

	return YES;
}

NewtonErr
CPseudoFlashDriver::startErase(CFlashRange& inRange, VAddr inBlockAddr)
{
	memset((void *)inBlockAddr, 0xFF, 64*KByte);
	return noErr;
}

void
CPseudoFlashDriver::write(ULong inWord, ULong inMask, VAddr inAddr, CFlashRange& inRange)
{
	ULong existingWord = *(ULong *)inAddr & ~inMask;
	*(ULong *)inAddr = inWord | existingWord;
}

