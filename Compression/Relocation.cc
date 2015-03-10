/*
	File:		Relocation.cc

	Contains:	Newton object store packages interface.

	Written by:	Newton Research Group, 2010.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Relocation.h"
#include "OSErrors.h"

extern void	ResolveImportRef(Ref * ioRefPtr, void ** outArg2);


/*------------------------------------------------------------------------------
	Fix up frames pointer refs to absolute addresses.
------------------------------------------------------------------------------*/

void
RelocateFramesInPage(FrameRelocationHeader * inHeader, char * inPage, VAddr inAddr, CCRelocator * inRelocator)
{
	ULong pageSize = inHeader->next10 <<2;	// r9
	if (pageSize == 0)
		return;

// for now, don’t even TRY to relocate
	return;

	Ref * refPtr;
	ULong objOffset = inHeader->first10 <<2;	// r6
	ULong offset, relOffset;	// r5
	if (inRelocator)
		relOffset = inRelocator->getTheNextRelocEntry();	// actually the first reloc entry
	else
		relOffset = 0xFFFFFFFF;

	if (inHeader->flags11 != 0)
	{
		long r1 = inHeader->flags8;
		if ((inHeader->flags8 & 0x01) != 0)
		{
//			r2 = 2;
			offset = (r1 == 3) ? 0 : (2 - r1) <<2;	// r8
			Ref * sp00 = (Ref *)(inPage + objOffset);
			if ((inHeader->flags & 0x80) != 0)
				sp00--;
//1C10
			for (refPtr = (Ref *)(inPage + offset); refPtr < sp00; refPtr++, offset += sizeof(Ref))
			{
				while (offset > relOffset)
					relOffset = inRelocator->getTheNextRelocEntry();
				if (offset != relOffset)
				{
					void * spm00 = 0;
					if (ISREALPTR(*refPtr))
						*refPtr += inAddr;
					else if (ISMAGICPTR(*refPtr))
						ResolveImportRef(refPtr, &spm00);
				}
			}
		}
		else if (r1 < 3)
//1C88
		{
			refPtr = (Ref *)(inPage + 8 - (r1<<2));	// r7
			offset = 2 - r1;					// r8
			while (offset > relOffset)
				relOffset = inRelocator->getTheNextRelocEntry();
			if (offset != relOffset)
			{
				void * spm00 = 0;
				if (ISREALPTR(*refPtr))
					*refPtr += inAddr;
				else if (ISMAGICPTR(*refPtr))
					ResolveImportRef(refPtr, &spm00);
			}
		}
	}

//1D00
	while (objOffset < pageSize)
	{
		ObjHeader * objPtr = (ObjHeader *)(inPage + objOffset);	// sp00
		if (objPtr->flags & kObjSlotted)
		{
			ULong pageSizeRemaining;
			if (objOffset + objPtr->size > pageSize
			&&  (pageSizeRemaining = pageSize - objOffset) > sizeof(ObjHeader))
			{
				ULong numOfRefs = (pageSizeRemaining - sizeof(ObjHeader))/sizeof(Ref);	// r0
				refPtr = (Ref *)(objPtr + sizeof(ObjHeader));	// r8
				offset = (char *)refPtr - inPage;	// r7
				while (numOfRefs-- > 0)
				{
					while (offset > relOffset)
						relOffset = inRelocator->getTheNextRelocEntry();
					if (offset != relOffset)
					{
						void * spm00 = 0;
						if (ISREALPTR(*refPtr))
							*refPtr += inAddr;
						else if (ISMAGICPTR(*refPtr))
							ResolveImportRef(refPtr, &spm00);
					}
				}
			}
		}

//1DE8
		else if (objOffset+sizeof(ObjHeader) < pageSize)
		{
			refPtr = (Ref *)(objPtr + sizeof(ObjHeader));	// r7
			offset = (char *)refPtr - inPage;	// r8
			while (offset > relOffset)
				relOffset = inRelocator->getTheNextRelocEntry();
			if (offset != relOffset)
			{
				void * spm00 = 0;
				if (ISREALPTR(*refPtr))
					*refPtr += inAddr;
				else if (ISMAGICPTR(*refPtr))
					ResolveImportRef(refPtr, &spm00);
			}
		}
//1E64
		ULong alignment = ((inHeader->flags & 0x40) != 0) ? 4 : 8;	// OS1 aligned on 4-bytes, OS2 aligned on 8-bytes
		objOffset += ALIGN(objPtr->size, alignment);	// offset to next ref object
	}
}


void
RelocateFramesInPage(FrameRelocationHeader * inHeader, char * inPage, VAddr inAddr)
// inArg3 is long in original
{
	RelocateFramesInPage(inHeader, inPage, inAddr, NULL);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S i m p l e C R e l o c a t o r
------------------------------------------------------------------------------*/

CSimpleCRelocator::CSimpleCRelocator()
{
	fSize = 0;
	fIndex = 0;
	fReloInfo.numOfEntries = 0;
}


CSimpleCRelocator::~CSimpleCRelocator()
{ }


NewtonErr
CSimpleCRelocator::init(CStore * inStore, PSSId inId, size_t * outReloInfoSize)
{
	NewtonErr	err;

	XTRY
	{
		// read C relocation header
		XFAIL(err = inStore->read(inId, 0, &fReloInfo, sizeof(CRelocationHeader)))
#if defined(hasByteSwapping)
		//	assume this is big-endian - we never create it so it must be from Newton device
		fReloInfo.version = BYTE_SWAP_LONG(fReloInfo.version);
		fReloInfo.numOfEntries = BYTE_SWAP_SHORT(fReloInfo.numOfEntries);
		fReloInfo.x08 = BYTE_SWAP_LONG(fReloInfo.x08);
		fReloInfo.baseAddr = BYTE_SWAP_LONG(fReloInfo.baseAddr);
#endif		
		// check we have the right version
		XFAILIF(fReloInfo.version != kReloInfoVersionGrokd, err = kOSErrBadPackageVersion;)
		if (fReloInfo.numOfEntries != 0)
		{
			// read C relocation data (after the header)
			fSize = LONGALIGN(fReloInfo.numOfEntries);
			XFAIL(err = inStore->read(inId, sizeof(CRelocationHeader), fReloIndex, fSize))
		}
		fSize += sizeof(CRelocationHeader);
		*outReloInfoSize = fSize;
	}
	XENDTRY;

	return err;
}


NewtonErr
CSimpleCRelocator::relocate(char * inBuf, VAddr inBaseAddr)
{
	if (fReloInfo.version != kReloInfoVersionGrokd)
		return kOSErrBadPackageVersion;

	ULong * buf = (ULong *)inBuf;
	long offset = inBaseAddr - fReloInfo.baseAddr;
	for (ArrayIndex i = 0; i < fReloInfo.numOfEntries; ++i)
	{
		ULong index = fReloIndex[i];
		buf[index] += offset;
	}
	return noErr;
}


ULong
CSimpleCRelocator::getTheNextRelocEntry(void)
{
	if (fReloInfo.version == kReloInfoVersionGrokd && fIndex < fReloInfo.numOfEntries)
		return fReloIndex[fIndex++] << 2;
	return 0xFFFFFFFF;
}

#pragma mark -

#if 0
/*------------------------------------------------------------------------------
	C C R e l o c a t i o n G e n e r a t o r
------------------------------------------------------------------------------*/

CCRelocationGenerator::CCRelocationGenerator()
{
	f00 = NULL;
	f04 = NULL;
	f08 = -1;
	f0C = 0;
}


CCRelocationGenerator::~CCRelocationGenerator()
{ }


NewtonErr
CCRelocationGenerator::init(RelocationHeader * ioHeader, RelocationEntry * ioEntries)
{
	if ((f00 = ioHeader) != NULL)
	{
		f04 = ioEntries;
		f08 = ioHeader->reserved;
		f14 = ioHeader->baseAddress;
		f10 = ioHeader->pageSize;
	}
	return noErr;
}


ULong
CCRelocationGenerator::getRelocDataSizeForBlock(ULong inBlock)
{
	RelocationHeader *	header = f00;
	RelocationEntry *		entry = f04;

	if (header == NULL)
		return 0;

	ULong	sizeOfOffsets = 0;
	while (entry + header - sizeof(RelocationHeader) > entry) // ??
	{
		sizeOfOffsets = LONGALIGN(entry->offsetCount);
		if (entry->pageNumber < inBlock)
		{
			if (sizeOfOffsets == 0)
				break;
			entry = (RelocationEntry *)((char *)entry + (sizeof(RelocationEntry) + sizeOfOffsets));
		}
		else if (entry->pageNumber == inBlock)
		{
		//	found it
			break;
		}
		else
		{
		//	gone too far and not found it
			sizeOfOffsets = 0;
			break;
		}
	}
	return sizeOfOffsets + 16;
}


long
CCRelocationGenerator::getRelocDataForBlock(ULong inBlock, char ** outData, long * outSize, char ** outArg4)
{
	RelocationHeader *	header = f00;	// r6
	RelocationEntry *		entry = f04;	// r12

	if (header == NULL)
	{
		*outData = NULL;
		*outSize = 0;
		*outArg4 = 0;
		return 0;
	}

	*outData = &header->+08; ??
	*outSize = 16;
	ULong	sizeOfOffsets = 0;
	while (entry + header - sizeof(RelocationHeader) > entry) // ??
	{
		sizeOfOffsets = LONGALIGN(entry->offsetCount);
		if (entry->pageNumber < inBlock)
		{
			if (sizeOfOffsets == 0)
				break;
			entry = (RelocationEntry *)((char *)entry + sizeof(RelocationEntry) + sizeOfOffsets);
		}
		else if (entry->pageNumber == inBlock)
		{
		//	found it
			*outArg4 = (char *)entry + sizeof(RelocationEntry);
			f0C = entry->offsetCount;
			return 0;
		}
		else
		{
		//	gone too far and not found it
			break;
		}
	}
	*outArg4 = 0;
	f0C = 0;
	return 0;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C F r a m e R e l o c a t i o n G e n e r a t o r
------------------------------------------------------------------------------*/

CFrameRelocationGenerator::CFrameRelocationGenerator()
{
	f17 = NO;
	f14 = NO;
	f08 = 0;
	f04 = -1;
	f00 = 0;
}


CFrameRelocationGenerator::CFrameRelocationGenerator(int inArg1)
{
	f17 = (inArg1 != 0);
	f14 = NO;
	f08 = 0;
	f04 = -1;
	f00 = 0;
}


void
CFrameRelocationGenerator::getHeader(FrameRelocationHeader * outHeader)
{
	outHeader->f00 = f00;
	if (f04 == -1)
	{
		f04 = f08;
		if (f10 > 0)
			outHeader->flags12 &= 0x80;
	}
	outHeader->first10 = f00/4;	// :10, :10, :12 bits
	outHeader->next10 = f04/4;
	f04 = -1;
	f08 = 0;
	f00 = 0;
	if (f17)
		f00 |= 0x40;
	if (f10 != 0)
	{
		f00 |= 0x0800;
		f00 &= ~0x0600;
		f00 |= (MIN((f0C - f10)/4, 3) << 9;
		if (f15)
			f00 |= 0x0100;
		else
			f00 &= ~0x0100;
		if (f16)
			f00 |= 0x0080;
		else
			f00 &= ~0x0080;
	}
	else
		f00 &= ~0x0F80;
}


void
CFrameRelocationGenerator::update(long inArg1, char * inArg2, long inArg3, bool inArg4)
{
}
#endif
