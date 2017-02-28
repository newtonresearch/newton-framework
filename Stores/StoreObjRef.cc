/*
	File:		StoreObjRef.cc

	Contains:	Store object reference implementation.
	
	Written by:	Newton Research Group, 2010.
*/
#undef forDebug
#define debugLevel 2

#include "StoreObjRef.h"
#include "FlashStore.h"
#include "FlashBlock.h"
#include "OSErrors.h"

#pragma mark SDirEnt
/*------------------------------------------------------------------------------
	S D i r E n t
	Directory entry?
------------------------------------------------------------------------------*/

bool
SDirEnt::isValid(CFlashStore * inStore)
{
	return zaddr != 0 && zaddr != 0x00FFFFFF	// address is not invalid
		 && x3 == (inStore->fDirtyBits & 1);	// entry is in use
}


bool
SDirEnt::isValidMigratedObjectInfo(int info1, int info2)
{
	return info1 <= 0x3FFF && info2 <= 0x3FF;
	//		 14 bits				  10 bits
	//		 16*KByte			  1*KByte
}


void
SDirEnt::setMigratedObjectInfo(int info1, int info2)
{
	zaddr = (info1 & 0x3FFF) | (info2 << 14);
}


void
SDirEnt::getMigratedObjectInfo(int * outInfo1, int * outInfo2) const
{
	*outInfo1 = zaddr & 0x3FFF;
	*outInfo2 = zaddr >> 14;
}

#if forDebug
void
SDirEnt::print(void)
{
	PRINTF(("SDirEnt: %08X %d%d%d%d%d%d\n", zaddr, x2,x3,x4,x5,x6,x7));
}
#endif


#pragma mark -
#pragma mark StoreObjHeader
/* -----------------------------------------------------------------------------
	S t o r e O b j H e a d e r
	Was SObject.
	Every object in the store is preceded by this header.
----------------------------------------------------------------------------- */
extern bool	IsValidPSSId(PSSId inId);

bool
StoreObjHeader::isValid(CFlashStore * inStore)
{
	return validity != (inStore->fVirginBits & 1)	// ie not virgin
		 && IsValidPSSId(id);
}


#pragma mark -
#pragma mark CStoreObjRef
/* -----------------------------------------------------------------------------
	C S t o r e O b j R e f
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	Assignment constructor.
	Args:		inObj			object to copy
	Return:	ref to this
----------------------------------------------------------------------------- */

CStoreObjRef &
CStoreObjRef::operator=(const CStoreObjRef & inObj)
{
	fObj = inObj.fObj;
	fObjectAddr = inObj.fObjectAddr;
	fDirEntryAddr = inObj.fDirEntryAddr;
	return *this;
}


/* -----------------------------------------------------------------------------
	Clone an object.
	Args:		inState
				inObj			object to clone
				inArg3
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CStoreObjRef::clone(int inState, CStoreObjRef & inObj, bool inArg3)
{
	NewtonErr err;

	// loop until no write error
	for ( ; ; )
	{
		if ((err = cloneEmpty(inState, inObj, inArg3)) == noErr)
			err = copyTo(inObj, 0, fObj.size);
		if (err == kStoreErrWriteError)
		{
			inObj.dlete();
			// and retry
		}
		else
			break;	// success, possibly
	}

	return err;
}


/* -----------------------------------------------------------------------------
	Clone an object (give the new one the same id) and make it empty.
	Args:		inState
				inObj			object to clone
				inArg3
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CStoreObjRef::cloneEmpty(int inState, CStoreObjRef & inObj, bool inArg3)
{
	return cloneEmpty(inState, fObj.size, inObj, inArg3);
}


/* -----------------------------------------------------------------------------
	Clone an object (give the new one the same id) and make it empty.
	Args:		inState
				inSize		size to make the new object
				inObj			object to clone
				inArg4
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CStoreObjRef::cloneEmpty(int inState, size_t inSize, CStoreObjRef & inObj, bool inArg4)
{
	return fStore->addObject(fObj.id, inState, inSize, inObj, inArg4, false);
}


/* -----------------------------------------------------------------------------
	Copy a range of our data to another object.
	Args:		inObj			object to copy into
				inOffset		start of range to copy
				inLen			size of range to copy
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CStoreObjRef::copyTo(CStoreObjRef & inObj, size_t inOffset, size_t inLen)
{
	return fStore->basicCopy(fStore->translate(fObjectAddr + sizeof(StoreObjHeader) + inOffset),
									 fStore->translate(inObj.fObjectAddr + sizeof(StoreObjHeader) + inOffset),
									 inLen);
}


/* -----------------------------------------------------------------------------
	Set...?
	Args:		inObjectAddr			address of object
				inDurEntryAddr			address of its directory entry
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CStoreObjRef::set(ZAddr inObjectAddr, ZAddr inDirEntryAddr)
{
PRINTF(("CStoreObjRef::set(obj=%08X, dir=%08X)\n", inObjectAddr,inDirEntryAddr));
	fObjectAddr = inObjectAddr;
	fDirEntryAddr = inDirEntryAddr;
	if (inObjectAddr == kIllegalZAddr && inDirEntryAddr != kIllegalZAddr)
	{
		SDirEnt dirEntry;
		fStore->basicRead(fStore->translate(inDirEntryAddr), &dirEntry, sizeof(SDirEnt));
#if forDebug
		dirEntry.print();
#endif
		fObjectAddr = MAKE_ADDR(dirEntry.zaddr);
PRINTF(("  obj -> %08X)\n", fObjectAddr));
	}
	return fStore->basicRead(fStore->translate(fObjectAddr), &fObj, sizeof(StoreObjHeader));
}


ULong
CStoreObjRef::findSuperceeded(CStoreObjRef & inObj)
{
	return fStore->lookup(fObj.id, fStore->fIsSRAM ? 5 : 12, inObj);
}


ULong
CStoreObjRef::findSuperceeder(CStoreObjRef & inObj)
{
	return fStore->lookup(fObj.id, fStore->fIsSRAM ? 6 : 13, inObj);
}


NewtonErr
CStoreObjRef::setSeparateTransaction(void)
{
	NewtonErr err = noErr;
	int n = fObj.x2;
	if (n != 0)
	{
		if (n == 2)
			return noErr;
		if (n != 3)			// sic
			return noErr;
		fObj.x2 = 2;
		return rewriteObjHeader();
	}

	CStoreObjRef obj(fStore->fVirginBits, fStore);
	fStore->add(obj);
	if ((err = clone(state(), obj, true)) == noErr)
	{
		dlete();
//		this = obj;
	}
	fStore->remove(obj);

	return err;
}


NewtonErr
CStoreObjRef::clearSeparateTransaction(void)
{
	int n = fObj.x2;
	if (n == 0)
		return noErr;
	if (n != 2)			// sic
		return noErr;
	return rewriteObjHeader();
}


NewtonErr
CStoreObjRef::rewriteObjHeader(void)
{
	return fStore->basicWrite(fStore->translate(fObjectAddr), &fObj, sizeof(StoreObjHeader));
}


ZAddr
CStoreObjRef::getDirEntryOffset(void)
{
	if (fDirEntryAddr == kIllegalZAddr)
	{
		CStoreObjRef obj(fStore->fVirginBits, fStore);
		fStore->add(obj);
		if (fStore->lookup(fObj.id, state(), obj) == noErr)
			fDirEntryAddr = obj.fDirEntryAddr;
		fStore->remove(obj);
	}
	return fDirEntryAddr;
}


NewtonErr
CStoreObjRef::read(void * outBuf, size_t inOffset, size_t inLen)
{
PRINTF(("CStoreObjRef::read(offset=%lu, length=%lu)\n", inOffset,inLen));
	return fStore->basicRead(fStore->translate(fObjectAddr + sizeof(StoreObjHeader) + inOffset), outBuf, inLen);
}


NewtonErr
CStoreObjRef::write(void * inBuf, size_t inOffset, size_t inLen)
{
PRINTF(("CStoreObjRef::write(offset=%lu, length=%lu)\n", inOffset,inLen));
	return fStore->basicWrite(fStore->translate(fObjectAddr + sizeof(StoreObjHeader) + inOffset), inBuf, inLen);
}


NewtonErr
CStoreObjRef::dlete(void)
{
	ZAddr dirEntAddr = getDirEntryOffset();
	if (dirEntAddr != kIllegalZAddr)
		fStore->blockForAddr(dirEntAddr)->zapDirEnt(dirEntAddr);
	fStore->blockForAddr(fObjectAddr)->zapObject(fObjectAddr);
	fObj.id = kNoPSSId;
	return noErr;
}


NewtonErr
CStoreObjRef::setState(int inState)
{
	if (fStore->fIsSRAM)
		fObj.transBits = ~gObjectStateToTransBits[inState];
	else
//		fObj.transBits &= gObjectStateToTransBits[inState];	// certainly looks like that in the source
		fObj.transBits = gObjectStateToTransBits[inState] ^ kVirginBits;
	return fStore->basicWrite(fStore->translate(fObjectAddr), &fObj, sizeof(StoreObjHeader));
}


NewtonErr
CStoreObjRef::setCommittedState(void)
{
	if (fObj.x2 == 2)
		fObj.x2 = 0;
	return setState(fStore->fIsSRAM ? 4 : 11);
}


/*------------------------------------------------------------------------------
	Translation tables.
	Object states:
		0
		1 |  8
		2 |  9
		3 | 10
		4 | 11	committed?
		5 | 12
		6 | 13
		7 | 14	deleted?
------------------------------------------------------------------------------*/

UByte gObjectStateToTransBits[15] =
{
	0x00,
	0x28, 0x08, 0x48, 0x09, 0x0B, 0x0C, 0x19,		// for SRAM		1..7	has 0x08 bit set
	0x20, 0x00, 0x40, 0x01, 0x03, 0x04, 0x11		// for Flash	8..14
};

UByte gObjectTransBitsToState[128] =
{
	0x09, 0x0B, 0x00, 0x0C, 0x0D, 0x0B, 0x00, 0x0C,		// Flash
	0x02, 0x04, 0x00, 0x05, 0x06, 0x04, 0x00, 0x05,		// SRAM
	0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E,		//...
	0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07,

	0x08, 0x0B, 0x00, 0x0C, 0x0D, 0x0B, 0x00, 0x0C,
	0x01, 0x00, 0x00, 0x00, 0x06, 0x04, 0x00, 0x05,
	0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E,
	0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07,

	0x0A, 0x0B, 0x00, 0x0C, 0x0D, 0x0B, 0x00, 0x0C,
	0x03, 0x04, 0x00, 0x05, 0x06, 0x04, 0x00, 0x05,
	0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E,
	0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07,

	0x08, 0x00, 0x00, 0x00, 0x0D, 0x0B, 0x00, 0x0C,
	0x01, 0x00, 0x00, 0x00, 0x06, 0x04, 0x00, 0x05,
	0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E, 0x00, 0x0E,
	0x00, 0x07, 0x00, 0x07, 0x00, 0x07, 0x00, 0x07
};


unsigned int CStoreObjRef::transBits(void) const
{
	unsigned int bits = fObj.transBits ^ (fStore->fVirginBits & 0xFF);
PRINTF(("transBits %d -> ",bits));
	return bits;
}

unsigned int CStoreObjRef::state(void) const
{
	unsigned int st = gObjectTransBitsToState[transBits()];
PRINTF(("state %d\n",st));
	return st;
}

