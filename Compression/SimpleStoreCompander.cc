/*
	File:		SimpleStoreCompander.cc

	Contains:	Simple store compression protocol.
					Simple => actually no compression at all.

	Written by:	Newton Research Group, 2007.
*/

#include "SimpleStoreCompander.h"
#include "OSErrors.h"

extern NewtonErr	LODefaultDoTransaction(CStore * inStore, PSSId inId, PSSId, int, bool);


/*------------------------------------------------------------------------------
	C S i m p l e S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CSimpleStoreDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CSimpleStoreDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN24CSimpleStoreDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN24CSimpleStoreDecompressor4makeEv - 0b	\n"
"		.long		__ZN24CSimpleStoreDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CSimpleStoreDecompressor\"	\n"
"2:	.asciz	\"CStoreDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN24CSimpleStoreDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN24CSimpleStoreDecompressor4makeEv - 4b	\n"
"		.long		__ZN24CSimpleStoreDecompressor7destroyEv - 4b	\n"
"		.long		__ZN24CSimpleStoreDecompressor4initEP6CStorejPc - 4b	\n"
"		.long		__ZN24CSimpleStoreDecompressor4readEjPcmm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CSimpleStoreDecompressor)

CSimpleStoreDecompressor *
CSimpleStoreDecompressor::make(void)
{
	return this;
}


void
CSimpleStoreDecompressor::destroy(void)
{ }


NewtonErr
CSimpleStoreDecompressor::init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer)
{
	fStore = inStore;
	return noErr;
}


NewtonErr
CSimpleStoreDecompressor::read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
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
		XFAIL(err = fStore->read(inObjId, sizeof(relocHeader), outBuf, objSize < inBufLen ? objSize : inBufLen))

		// this is the simple store decompressor -- no decompression required

		// relocate frame refs
		RelocateFramesInPage(&relocHeader, outBuf, inBaseAddr);
	}
	XENDTRY;

	return err;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C S i m p l e R e l o c S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CSimpleRelocStoreDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CSimpleRelocStoreDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN29CSimpleRelocStoreDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN29CSimpleRelocStoreDecompressor4makeEv - 0b	\n"
"		.long		__ZN29CSimpleRelocStoreDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CSimpleRelocStoreDecompressor\"	\n"
"2:	.asciz	\"CStoreDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN29CSimpleRelocStoreDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN29CSimpleRelocStoreDecompressor4makeEv - 4b	\n"
"		.long		__ZN29CSimpleRelocStoreDecompressor7destroyEv - 4b	\n"
"		.long		__ZN29CSimpleRelocStoreDecompressor4initEP6CStorejPc - 4b	\n"
"		.long		__ZN29CSimpleRelocStoreDecompressor4readEjPcmm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CSimpleRelocStoreDecompressor)

CSimpleRelocStoreDecompressor *
CSimpleRelocStoreDecompressor::make(void)
{
	return this;
}


void
CSimpleRelocStoreDecompressor::destroy(void)
{ }


NewtonErr
CSimpleRelocStoreDecompressor::init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer)
{
	fStore = inStore;
	return noErr;
}


NewtonErr
CSimpleRelocStoreDecompressor::read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
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

		XFAIL(err = fStore->read(inObjId, relocInfoSize + sizeof(relocHeader), outBuf, objSize - sizeof(relocHeader) - relocInfoSize))

		// this is the simple store decompressor -- no decompression required

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
	C S i m p l e S t o r e C o m p a n d e r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CSimpleStoreCompander implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CSimpleStoreCompander::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN21CSimpleStoreCompander6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN21CSimpleStoreCompander4makeEv - 0b	\n"
"		.long		__ZN21CSimpleStoreCompander7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CSimpleStoreCompander\"	\n"
"2:	.asciz	\"CStoreCompander\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN21CSimpleStoreCompander9classInfoEv - 4b	\n"
"		.long		__ZN21CSimpleStoreCompander4makeEv - 4b	\n"
"		.long		__ZN21CSimpleStoreCompander7destroyEv - 4b	\n"
"		.long		__ZN21CSimpleStoreCompander4initEP6CStorejjbb - 4b	\n"
"		.long		__ZN21CSimpleStoreCompander9blockSizeEv - 4b	\n"
"		.long		__ZN21CSimpleStoreCompander4readEmPcmm - 4b	\n"
"		.long		__ZN21CSimpleStoreCompander5writeEmPcmm - 4b	\n"
"		.long		__ZN21CSimpleStoreCompander20doTransactionAgainstEij - 4b	\n"
"		.long		__ZN21CSimpleStoreCompander10isReadOnlyEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CSimpleStoreCompander)

CSimpleStoreCompander *
CSimpleStoreCompander::make(void)
{
	fStore = NULL;
	fIsReadOnly = false;
	return this;
}


void
CSimpleStoreCompander::destroy(void)
{ }


NewtonErr
CSimpleStoreCompander::init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5)
{
	fStore = inStore;
	fRootId = inRootId;
	fIsReadOnly = inShared;

	PackageRoot	root;
	inStore->read(fRootId, 0, &root, sizeof(root));
	fChunksId = root.fDataId;

	return noErr;
}


size_t
CSimpleStoreCompander::blockSize(void)
{
	return kSubPageSize;
}


NewtonErr
CSimpleStoreCompander::read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
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
			// read chunk into our buffer
			XFAIL(err = fStore->read(objId, 0, outBuf, objSize < inBufLen ? objSize : inBufLen))
		else
			memset(outBuf, 0, inBufLen);
	}
	XENDTRY;

	return err;
}


NewtonErr
CSimpleStoreCompander::write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inBaseAddr)
{
	NewtonErr	err;
	PSSId			objId;

	XTRY
	{
		// read id of chunk
		XFAIL(err = fStore->read(fChunksId, (inOffset/kSubPageSize)*sizeof(PSSId), &objId, sizeof(PSSId)))
		// replace existing chunk
		err = fStore->replaceObject(objId, inBuf, inBufLen);
	}
	XENDTRY;

	return err;
}


NewtonErr
CSimpleStoreCompander::doTransactionAgainst(int inArg1, ULong inArg2)
{
	return LODefaultDoTransaction(fStore, fRootId, fChunksId, inArg1, true);
}


bool
CSimpleStoreCompander::isReadOnly(void)
{
	return fIsReadOnly;
}
