/*
	File:		QDCompression.cc

	Contains:	QuickDraw PixelMap compression.

	Written by:	Newton Research Group, 2007.
*/

#include "Newton.h"
#include "QDCompression.h"
#include "LargeObjectStore.h"
#include "OSErrors.h"


void
InitQDCompression(void)
{
	CPixelMapCompander::classInfo()->registerProtocol();
}

#pragma mark -

/*------------------------------------------------------------------------------
	C P i x e l M a p C o m p a n d e r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CPixelMapCompander implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CPixelMapCompander::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN18CPixelMapCompander6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN18CPixelMapCompander4makeEv - 0b	\n"
"		.long		__ZN18CPixelMapCompander7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CPixelMapCompander\"	\n"
"2:	.asciz	\"CStoreCompander\"	\n"
"3:	.asciz	\"CLZDecompressor\", \"\"	\n"
"		.asciz	\"CLZCompressor\", \"\"	\n"
"		.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN18CPixelMapCompander9classInfoEv - 4b	\n"
"		.long		__ZN18CPixelMapCompander4makeEv - 4b	\n"
"		.long		__ZN18CPixelMapCompander7destroyEv - 4b	\n"
"		.long		__ZN18CPixelMapCompander4initEP6CStorejjbb - 4b	\n"
"		.long		__ZN18CPixelMapCompander9blockSizeEv - 4b	\n"
"		.long		__ZN18CPixelMapCompander4readEmPcmm - 4b	\n"
"		.long		__ZN18CPixelMapCompander5writeEmPcmm - 4b	\n"
"		.long		__ZN18CPixelMapCompander20doTransactionAgainstEij - 4b	\n"
"		.long		__ZN18CPixelMapCompander10isReadOnlyEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CPixelMapCompander)

CPixelMapCompander *
CPixelMapCompander::make(void)
{
	fDecompressor = NULL;
	fCompressor = NULL;
	fBuffer = NULL;
	fPixMapObj = NULL;
	fIsAllocated = NO;
	return this;
}


void
CPixelMapCompander::destroy(void)
{
	disposeAllocations();
}


NewtonErr
CPixelMapCompander::init(CStore * inStore, PSSId inRootId, PSSId inParmsId, bool inShared, bool inArg5)
{
	NewtonErr	err;

	XTRY
	{
		fStore = inStore;
		fRootId = inRootId;
		fPixMapId = inParmsId;
		fShared = inShared;
		
		if (inShared)
		{
			XFAIL(err = GetSharedLZObjects(&fCompressor, &fDecompressor, &fBuffer, &fBufSize))
		}
		else
		{
			fCompressor = (CCompressor *)MakeByName("CCompressor", "CLZCompressor");
			fDecompressor = (CDecompressor *)MakeByName("CDecompressor", "CLZDecompressor");
			fBufSize = 8 + kSubPageSize;
			fBuffer = NewPtr(fBufSize);
			fIsAllocated = YES;
		}

		XFAILIF(fCompressor == NULL || fDecompressor == NULL || fBuffer == NULL, disposeAllocations(); err = kOSErrNoMemory;)

		if (fIsAllocated)
		{
			XFAIL(err = fCompressor->init(NULL))
			XFAIL(err = fDecompressor->init(NULL))
		}

		size_t	objSize;
		XFAIL(err = fStore->getObjectSize(fPixMapId, &objSize))
		if (objSize > 0)
		{
			fPixMapObj = (PixMapObj *)NewPtr(objSize);
			XFAILNOT(fPixMapObj, err = kOSErrNoMemory;)
			XFAIL(err = inStore->read(fPixMapId, 0, fPixMapObj, objSize))
#if defined(hasByteSwapping)
			fPixMapObj->size = BYTE_SWAP_LONG(fPixMapObj->size);
#if __LP64__
			// byte swap 64-bit long
			ULong tmp, *p;
			p = (ULong *)&fPixMapObj->pixMap.baseAddr;
			tmp = BYTE_SWAP_LONG(p[1]);
			p[1] = BYTE_SWAP_LONG(p[0]);
			p[0] = tmp;
#else
			fPixMapObj->pixMap.baseAddr = (Ptr)BYTE_SWAP_LONG((uint32_t)fPixMapObj->pixMap.baseAddr);
#endif
			fPixMapObj->pixMap.rowBytes = BYTE_SWAP_SHORT(fPixMapObj->pixMap.rowBytes);
			fPixMapObj->pixMap.bounds.top = BYTE_SWAP_SHORT(fPixMapObj->pixMap.bounds.top);
			fPixMapObj->pixMap.bounds.left = BYTE_SWAP_SHORT(fPixMapObj->pixMap.bounds.left);
			fPixMapObj->pixMap.bounds.bottom = BYTE_SWAP_SHORT(fPixMapObj->pixMap.bounds.bottom);
			fPixMapObj->pixMap.bounds.right = BYTE_SWAP_SHORT(fPixMapObj->pixMap.bounds.right);
			fPixMapObj->pixMap.pixMapFlags = BYTE_SWAP_LONG(fPixMapObj->pixMap.pixMapFlags);
			fPixMapObj->pixMap.deviceRes.h = BYTE_SWAP_SHORT(fPixMapObj->pixMap.deviceRes.h);
			fPixMapObj->pixMap.deviceRes.v = BYTE_SWAP_SHORT(fPixMapObj->pixMap.deviceRes.v);
#if defined(QD_Gray)
			fPixMapObj->pixMap.grayTable = (UChar *)BYTE_SWAP_LONG((ULong)fPixMapObj->pixMap.grayTable);
#endif
			fPixMapObj->x20 = BYTE_SWAP_LONG(fPixMapObj->x20);
			fPixMapObj->x24 = BYTE_SWAP_LONG(fPixMapObj->x24);
			fPixMapObj->x28 = BYTE_SWAP_LONG(fPixMapObj->x28);
#endif
			fRowLongs = fPixMapObj->pixMap.rowBytes / sizeof(int32_t);
		}
		f38 = -1;

		PackageRoot	root;
		err = inStore->read(fRootId, 0, &root, sizeof(root));
		fChunksId = root.fDataId;
	}
	XENDTRY;

	return err;
}


void
CPixelMapCompander::disposeAllocations(void)
{
	if (fIsAllocated)
	{
		if (fDecompressor != NULL)
			fDecompressor->destroy();
		if (fCompressor != NULL)
			fCompressor->destroy();
		if (fBuffer != NULL)
			FreePtr(fBuffer);
	}
	else
		ReleaseSharedLZObjects(fCompressor, fDecompressor, fBuffer);

	if (fPixMapObj != NULL)
		FreePtr((Ptr)fPixMapObj);
	fDecompressor = NULL;
	fCompressor = NULL;
	fBuffer = NULL;
	fPixMapObj = NULL;
}


size_t
CPixelMapCompander::blockSize(void)
{
	return kSubPageSize;	// original says kPageSize!
}


NewtonErr
CPixelMapCompander::read(size_t inOffset, char * outBuf, size_t inBufLen, VAddr outData)
{
	NewtonErr	err;
	PSSId			objId;
	size_t		objSize;
	size_t		bufSize;

	XTRY
	{
		// read id of chunk
		XFAIL(err = fStore->read(fChunksId, (inOffset/kSubPageSize)*sizeof(PSSId), &objId, sizeof(PSSId)))
		// get chunk size
		XFAIL(err = fStore->getObjectSize(objId, &objSize))
		if (objSize != 0)
		{
			XFAILIF(objSize > fBufSize, err = kOSErrNoMemory;)
			// read chunk into our buffer
			XFAIL(err = fStore->read(objId, 0, fBuffer, objSize))
			// decompress it into output buffer
			XFAIL(err = fDecompressor->decompress(&bufSize, outBuf, inBufLen, fBuffer, objSize))
			if (fPixMapObj != NULL)
			{
				int rowBytes = fPixMapObj->pixMap.rowBytes;
				fRowLongs = rowBytes / sizeof(int32_t);
				if (f38 != inBufLen)
				{
					int wholeRowBytes = (inBufLen / rowBytes) * rowBytes;
					f3C = wholeRowBytes / sizeof(int32_t) - fRowLongs;
					f40 = inBufLen - wholeRowBytes;
					f38 = inBufLen;
				}
				uint32_t * r6 = (uint32_t *)outBuf;
				uint32_t * r1 = (uint32_t *)outBuf + fRowLongs;
				int bitCount;
				for (bitCount = f3C; bitCount >= 8; bitCount -= 8)
				{
					*r1++ ^= *r6++;
					*r1++ ^= *r6++;
					*r1++ ^= *r6++;
					*r1++ ^= *r6++;
					*r1++ ^= *r6++;
					*r1++ ^= *r6++;
					*r1++ ^= *r6++;
					*r1++ ^= *r6++;
				}
				switch (bitCount)
				{
				case 7:
					*r1++ ^= *r6++;
				case 6:
					*r1++ ^= *r6++;
				case 5:
					*r1++ ^= *r6++;
				case 4:
					*r1++ ^= *r6++;
				case 3:
					*r1++ ^= *r6++;
				case 2:
					*r1++ ^= *r6++;
				case 1:
					*r1 ^= *r6;
				}
			}
		}
		else
			memset(outBuf, 0, inBufLen);
	}
	XENDTRY;

	return err;
}


NewtonErr
CPixelMapCompander::write(size_t inOffset, char * inBuf, size_t inBufLen, VAddr inData)
{
	NewtonErr	err;
	PSSId			objId;
	size_t		bufSize;

	XTRY
	{
		// read id of chunk
		XFAIL(err = fStore->read(fChunksId, (inOffset/kSubPageSize)*sizeof(PSSId), &objId, sizeof(PSSId)))
		if (fPixMapObj == NULL)
		{
			fPixMapObj = (PixMapObj *)NewPtr(sizeof(PixMapObj));
			XFAILNOT(fPixMapObj, err = kOSErrNoMemory;)
			fPixMapObj->size = sizeof(PixMapObj);
			memmove(&fPixMapObj->pixMap, (PixelMap *)inData, sizeof(PixelMap));
			int rowBytes = ((PixelMap *)inData)->rowBytes;
			int wholeRowBytes = (inBufLen / rowBytes) * rowBytes;
			int r2 = wholeRowBytes / sizeof(int32_t);
			fRowLongs = r2;
//			if (PixelDepth(&fPixMapObj->pixMap) == kOneBitDepth)		// can’t make any sense of this
//				fPixMapObj->pixMap.grayTable = r2;
			int r1 = wholeRowBytes / sizeof(int32_t) - fRowLongs;
			f38 = inBufLen;
			f40 = inBufLen - wholeRowBytes;
			f3C = r1;
			XFAIL(err = fStore->setObjectSize(fPixMapId, sizeof(PixMapObj)))
			XFAIL(err = fStore->write(fPixMapId, 0, fPixMapObj, sizeof(PixMapObj)))
		}
		uint32_t * bufPtr = (uint32_t *)inBuf;
		int i;
		for (i = inBufLen / sizeof(int32_t); i > 0; i--)
		{
			if (*bufPtr++ != 0)
				break;
		}
		if (i == 0)
		//	buffer is all zeros
			err = fStore->setObjectSize(objId, 0);
		else
		{
			if (f38 != inBufLen)
			{
				int rowBytes = fPixMapObj->pixMap.rowBytes;
				int wholeRowBytes = (inBufLen / rowBytes) * rowBytes;
				f38 = inBufLen;
				f40 = inBufLen - wholeRowBytes;
				f3C = wholeRowBytes / sizeof(int32_t) - fRowLongs;
			}
			uint32_t * r7 = (uint32_t *)(inBuf + inBufLen - f40);
			uint32_t * r8 = r7 - fRowLongs;
			int bitCount;
			for (bitCount = f3C; bitCount >= 8; bitCount -= 8)
			{
				uint32_t * r1 = r7 - 1;
				uint32_t * r2 = r8 - 1;
				*r1-- ^= *r2--;
				*r1-- ^= *r2--;
				*r1-- ^= *r2--;
				*r1-- ^= *r2--;
				*r1-- ^= *r2--;
				*r1-- ^= *r2--;
				*r1-- ^= *r2--;
				r7 = r1;
				r8 = r2;
				*r1 ^= *r2;
			}
			switch (bitCount)
			{
			case 7:
				*--r7 ^= *--r8;		// watch this pre-decrement!
			case 6:
				*--r7 ^= *--r8;
			case 5:
				*--r7 ^= *--r8;
			case 4:
				*--r7 ^= *--r8;
			case 3:
				*--r7 ^= *--r8;
			case 2:
				*--r7 ^= *--r8;
			case 1:
				*--r7 ^= *--r8;
			}
		}
		// compress data into our buffer
		XFAIL(err = fCompressor->compress(&bufSize, fBuffer, fBufSize, inBuf, inBufLen))
		// replace existing chunk
		err = fStore->replaceObject(objId, fBuffer, bufSize);
	}
	XENDTRY;

	return err;
}


NewtonErr	// just guessing
CPixelMapCompander::doTransactionAgainst(int inArg1, ULong inArg2)
{
	return LODefaultDoTransaction(fStore, fRootId, fChunksId, inArg1, YES);
}


bool
CPixelMapCompander::isReadOnly(void)
{
	return fShared;
}

