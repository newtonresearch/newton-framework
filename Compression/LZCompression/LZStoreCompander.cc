/*
	File:		LZStoreCompander.cc

	Contains:	Store compression protocol.

	Written by:	Newton Research Group.
*/

#include "LZStoreCompander.h"
#include "OSErrors.h"

extern NewtonErr	LODefaultDoTransaction(CStore * inStore, PSSId inId, PSSId, int, bool);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

CCompressor *		gLZCompressor;
int					gLZRefCount;
CDecompressor *	gLZDecompressor;
char *				gLZSharedBuffer;


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

NewtonErr
GetSharedLZObjects(CCompressor ** outCompressor, CDecompressor ** outDecompressor, char ** outBuffer, size_t * outBufLen)
{
	NewtonErr	err = noErr;

	if (outCompressor)
	{
		*outCompressor = NULL;
		if (gLZCompressor == NULL)
		{
			gLZCompressor = (CCompressor *)MakeByName("CCompressor", "CLZCompressor");
			if (gLZCompressor)
			{
				gLZRefCount = 1;
				err = gLZCompressor->init(NULL);
			}
			else
				err = kOSErrNoMemory;
			if (err)
			{
				if (gLZRefCount == 1)
					gLZRefCount = 0;
				if (gLZCompressor)
				{
					delete gLZCompressor;
					gLZCompressor = NULL;
				}
				return err;
			}
		}
		else
			gLZRefCount++;
		*outCompressor = gLZCompressor;
	}

	if (outDecompressor)
		*outDecompressor = gLZDecompressor;
	if (outBuffer)
		*outBuffer = gLZSharedBuffer;
	if (outBufLen)
		*outBufLen = 0x0520;

	return err;
}

void
ReleaseSharedLZObjects(CCompressor * inCompressor, CDecompressor * inDecompressor, char * inBuffer)
{
	if (inCompressor)
	{
		if (--gLZRefCount == 0)
		{
			gLZCompressor->destroy();
			gLZCompressor = NULL;
		}
	}
}


#pragma mark -
/*------------------------------------------------------------------------------
	C L Z S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CLZStoreDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CLZStoreDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN20CLZStoreDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN20CLZStoreDecompressor4makeEv - 0b	\n"
"		.long		__ZN20CLZStoreDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CLZStoreDecompressor\"	\n"
"2:	.asciz	\"CStoreDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN20CLZStoreDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN20CLZStoreDecompressor4makeEv - 4b	\n"
"		.long		__ZN20CLZStoreDecompressor7destroyEv - 4b	\n"
"		.long		__ZN20CLZStoreDecompressor4initEP6CStorejPc - 4b	\n"
"		.long		__ZN20CLZStoreDecompressor4readEjPcmm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CLZStoreDecompressor)

CLZStoreDecompressor *
CLZStoreDecompressor::make(void)
{
	fBuffer = NULL;
	fDecompressor = NULL;
	return this;
}


void
CLZStoreDecompressor::destroy(void)
{
	if (fDecompressor)
		fDecompressor->destroy();
}


NewtonErr
CLZStoreDecompressor::init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer)
{
	NewtonErr	err = noErr;

	XTRY
	{
		fStore = inStore;
		fBuffer = inLZWBuffer;	// not original: see ROMDomainManager.cc, CROMDomainManager1K::addPackage -- LZ compression requires a buffer which was formerly passed in the parmsId
		if (fBuffer)
		{
			XFAILNOT(fDecompressor = (CLZDecompressor *)MakeByName("CDecompressor", "CLZDecompressor"), err = kOSErrNoMemory;)
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
CLZStoreDecompressor::read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
{
	NewtonErr	err;
	FrameRelocationHeader	relocHeader;
	size_t		objSize;

	XTRY
	{
		// read FrameRelocationHeader
		XFAIL(err = fStore->read(inObjId, 0, &relocHeader, sizeof(relocHeader)))
		XFAIL(err = fStore->getObjectSize(inObjId, &objSize))

		objSize -= sizeof(relocHeader);

		// read object into our buffer
		XFAIL(err = fStore->read(inObjId, sizeof(relocHeader), fBuffer, objSize))

		// decompress it into client buffer
		// ignore inBufLen -- outBuf MUST be kSubPageSize in size
		fDecompressor->decompress(&objSize, outBuf, kSubPageSize, fBuffer, objSize);

		// relocate frame refs
		RelocateFramesInPage(&relocHeader, outBuf, inBaseAddr);
	}
	XENDTRY;

	return err;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C L Z R e l o c S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CLZRelocStoreDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CLZRelocStoreDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN25CLZRelocStoreDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN25CLZRelocStoreDecompressor4makeEv - 0b	\n"
"		.long		__ZN25CLZRelocStoreDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CLZRelocStoreDecompressor\"	\n"
"2:	.asciz	\"CStoreDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN25CLZRelocStoreDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN25CLZRelocStoreDecompressor4makeEv - 4b	\n"
"		.long		__ZN25CLZRelocStoreDecompressor7destroyEv - 4b	\n"
"		.long		__ZN25CLZRelocStoreDecompressor4initEP6CStorejPc - 4b	\n"
"		.long		__ZN25CLZRelocStoreDecompressor4readEjPcmm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CLZRelocStoreDecompressor)

CLZRelocStoreDecompressor *
CLZRelocStoreDecompressor::make(void)
{
	fBuffer = NULL;
	fDecompressor = NULL;
	return this;
}


void
CLZRelocStoreDecompressor::destroy(void)
{
	if (fDecompressor)
		fDecompressor->destroy();
}


NewtonErr
CLZRelocStoreDecompressor::init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer)
{
	NewtonErr	err = noErr;

	XTRY
	{
		fStore = inStore;
		fBuffer = inLZWBuffer;	// see ROMDomainManager.cc
		if (fBuffer)
		{
			XFAILNOT(fDecompressor = (CLZDecompressor *)MakeByName("CDecompressor", "CLZDecompressor"), err = kOSErrNoMemory;)
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
CLZRelocStoreDecompressor::read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
{
	NewtonErr	err;
	FrameRelocationHeader	relocHeader;
	size_t		objSize;

	XTRY
	{
		// object is prefixed w/ C relocation info
		size_t relocInfoSize;
		CSimpleCRelocator relocator;
		XFAIL(err = relocator.init(fStore, inObjId, &relocInfoSize))
		
		// read FrameRelocationHeader
		XFAIL(err = fStore->read(inObjId, relocInfoSize, &relocHeader, sizeof(relocHeader)))
		XFAIL(err = fStore->getObjectSize(inObjId, &objSize))

		objSize -= (sizeof(relocHeader) + relocInfoSize);

		XFAIL(err = fStore->read(inObjId, relocInfoSize + sizeof(relocHeader), fBuffer, objSize))

		// decompress it into client buffer
		// ignore inBufLen -- outBuf MUST be kSubPageSize in size
		fDecompressor->decompress(&objSize, outBuf, kSubPageSize, fBuffer, objSize);

		// relocate C refs
		relocator.relocate(outBuf, inBaseAddr);
		// relocate frame refs
		RelocateFramesInPage(&relocHeader, outBuf, inBaseAddr);
	}
	XENDTRY;

	return err;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C L Z S t o r e C o m p a n d e r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CLZStoreCompander implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CLZStoreCompander::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN17CLZStoreCompander6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN17CLZStoreCompander4makeEv - 0b	\n"
"		.long		__ZN17CLZStoreCompander7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CLZStoreCompander\"	\n"
"2:	.asciz	\"CStoreCompander\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN17CLZStoreCompander9classInfoEv - 4b	\n"
"		.long		__ZN17CLZStoreCompander4makeEv - 4b	\n"
"		.long		__ZN17CLZStoreCompander7destroyEv - 4b	\n"
"		.long		__ZN17CLZStoreCompander4initEP6CStorejjbb - 4b	\n"
"		.long		__ZN17CLZStoreCompander9blockSizeEv - 4b	\n"
"		.long		__ZN17CLZStoreCompander4readEmPcmm - 4b	\n"
"		.long		__ZN17CLZStoreCompander5writeEmPcmm - 4b	\n"
"		.long		__ZN17CLZStoreCompander20doTransactionAgainstEij - 4b	\n"
"		.long		__ZN17CLZStoreCompander10isReadOnlyEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CLZStoreCompander)

CLZStoreCompander *
CLZStoreCompander::make(void)
{
	fBuffer = NULL;
	fDecompressor = NULL;
	fCompressor = NULL;
	fIsAllocated = NO;
	return this;
}


void
CLZStoreCompander::destroy(void)
{
	if (fIsAllocated)
	{
		if (fDecompressor != NULL)
			fDecompressor->destroy();
		if (fCompressor != NULL)
			fCompressor->destroy();
		if (fBuffer != NULL)
			free(fBuffer);
	}
	else
		ReleaseSharedLZObjects(fCompressor, fDecompressor, fBuffer);
}


NewtonErr
CLZStoreCompander::init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5)
{
	NewtonErr	err = noErr;

	XTRY
	{
		fStore = inStore;
		fRootId = inRootId;
		if (inShared)
		{
			size_t	bufSize;
			XFAIL(err = GetSharedLZObjects(&fCompressor, &fDecompressor, &fBuffer, &bufSize))
		}
		else
		{
			fCompressor = (CCompressor *)MakeByName("CCompressor", "CLZCompressor");
			fDecompressor = (CDecompressor *)MakeByName("CDecompressor", "CLZDecompressor");
			fBuffer = new char[0x520];
			fIsAllocated = YES;
			XFAILIF(fCompressor == NULL || fDecompressor == NULL || fBuffer == NULL, err = kOSErrNoMemory;)
		}

		PackageRoot	root;
		err = inStore->read(fRootId, 0, &root, sizeof(root));
		fChunksId = root.fDataId;
	}
	XENDTRY;

	return err;
}


size_t
CLZStoreCompander::blockSize(void)
{
	return kSubPageSize;
}


NewtonErr
CLZStoreCompander::read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
{
	NewtonErr	err;
	PSSId			objId;
	size_t		objSize;

	XTRY
	{
		// read id of chunk
		XFAIL(err = fStore->read(fChunksId, (inOffset/kSubPageSize)*sizeof(PSSId), &objId, sizeof(PSSId)))		// get chunk size
		XFAIL(err = fStore->getObjectSize(objId, &objSize))
		if (objSize != 0)
		{
			// read chunk into our buffer
			XFAIL(err = fStore->read(objId, 0, fBuffer, objSize))
			// decompress it into output buffer
			err = fDecompressor->decompress(&inBufLen, outBuf, inBufLen, fBuffer, objSize);
		}
		else
			memset(outBuf, 0, inBufLen);
	}
	XENDTRY;

	return err;
}


NewtonErr
CLZStoreCompander::write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inBaseAddr)
{
	NewtonErr	err;
	PSSId			objId;

	XTRY
	{
		// read id of chunk
		XFAIL(err = fStore->read(fChunksId, (inOffset/kSubPageSize)*sizeof(PSSId), &objId, sizeof(PSSId)))
		// compress data into our buffer
		XFAIL(err = fCompressor->compress(&inBufLen, fBuffer, 0x520, inBuf, inBufLen))
		// replace existing chunk
		err = fStore->replaceObject(objId, fBuffer, inBufLen);
	}
	XENDTRY;

	return err;
}


NewtonErr
CLZStoreCompander::doTransactionAgainst(int inArg1, ULong inArg2)
{
	return LODefaultDoTransaction(fStore, fRootId, fChunksId, inArg1, YES);
}


bool
CLZStoreCompander::isReadOnly(void)
{
	return fCompressor == NULL;
}
