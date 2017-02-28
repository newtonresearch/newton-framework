/*
	File:		StorePackages.cc

	Contains:	Newton object store packages interface.

	Written by:	Newton Research Group, 2007.
*/

#include "Objects.h"
#include "RefMemory.h"
#include "RSSymbols.h"
#include "LargeBinaries.h"
#include "StorePackages.h"
#include "Relocation.h"
#include "OSErrors.h"


NewtonErr
PkgCompCallback(VAddr instance, void * inBuf, size_t inSize, bool inArg3)
{
	return ((CStorePackageWriter*)instance)->writeCompressedData(inBuf, inSize, inArg3);
}


/*------------------------------------------------------------------------------
	C S t o r e P a c k a g e W r i t e r
------------------------------------------------------------------------------*/

CStorePackageWriter::CStorePackageWriter()
{
	fCompressor = NULL;
	f08 = NULL;
	f2C = NULL;
	f28 = NULL;
}


CStorePackageWriter::~CStorePackageWriter()
{
	if (f08 != NULL)
		delete[] f08;
	if (f2C != NULL)
		delete f2C;
	if (f28 != NULL)
		delete f28;
}


void
CStorePackageWriter::init(CStore * inStore, PSSId inObjectId, size_t inSize, CCallbackCompressor * inCompressor, RelocationHeader * ioRelocHeader, RelocationEntry * ioRelocEntry)
{
	fCompressor = inCompressor;
	f14 = 0;
	if (inCompressor != NULL)
	{
		inCompressor->f10 = PkgCompCallback;
		inCompressor->f14 = this;
	}
	fStore = inStore;
	fObjectId = inObjectId;
	f0C = 0;
	f10 = 1024;
	f08 = new char[1024];
	if (f08 != NULL
	&&  fStore->setObjectSize(inObjectId, (inSize+1023)/1024 * 4) == noErr)
	{
		
		f28 = new CCRelocationGenerator;
		if (f28 != NULL
		&&  f28->init(ioRelocHeader, ioRelocEntry) = noErr)
		{
			f24 = f28->getRelocDataSizeForBlock();
			f2C = new CFrameRelocationGenerator;
		}
	}
}


NewtonErr
CStorePackageWriter::writeChunk(char * inBuf, size_t inSize, bool inArg3)
{
	return noErr;
}


void
CStorePackageWriter::writeCompressedData(void * inBuf, size_t inSize)
{
	long	newSize = 4 + f24 + f10 + inSize;
	if (fStore->setObjectSize(f20, newSize) == noErr)
	{
		long	dataOffset = 4 + f24 + f10;
		if (fStore->write(f20, dataOffset, inBuf, inSize) == noErr)
			f10 += inSize;
	}
}


void
CStorePackageWriter::flush(void)
{
}


void
CStorePackageWriter::abort(void)
{
	PSSId	id;
	for (ArrayIndex i = 0; i < f14; ++i)
	{
		if (fStore->read(fObjectId, i*sizeof(PSSId), &id, sizeof(id)) != noErr)
			break;
		if (fStore->separatelyAbort(id) != noErr)
			break;
	}
}
