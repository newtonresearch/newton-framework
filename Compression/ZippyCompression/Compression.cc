/*
	File:		Compression.cc

	Contains:	Zippy compression initialization.

	Written by:	Newton Research Group, 2007.
*/

#include "Newton.h"
#include "ZippyStoreDecompression.h"
#include "OSErrors.h"

void	InitZippyCompression(void);
void	InitZippyDecompression(void);


PROTOCOL CZippyCallbackCompressor : public CCallbackCompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CZippyCallbackCompressor)

	CZippyCallbackCompressor *	make(void);
	void			destroy(void);

	NewtonErr	init(void *);
	void			reset(void);
	NewtonErr	writeChunk(void*, long);
	NewtonErr	flush(void);
};


typedef int ByteAccessor;	// don’t really know

PROTOCOL CZippyCompressor : public CCompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CZippyCompressor)

	CZippyCompressor *	make(void);
	void			destroy(void);

	NewtonErr	init(void *);
	NewtonErr	compress(size_t *, void*, size_t, void*, size_t);
	size_t		estimatedCompressedSize(void*, size_t);

private:
	NewtonErr	initCache(void);
	NewtonErr	compressChunk(size_t *, void*, size_t, void*, size_t);
	NewtonErr	cacheAndCompress(size_t, ByteAccessor*);
	NewtonErr	stuffBits(unsigned char**, int*, int, ByteAccessor);
	NewtonErr	finish(void*, size_t);
	NewtonErr	headerSize(void);
};


#pragma mark -
/*------------------------------------------------------------------------------
	C Z i p p y D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CZippyDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CZippyDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN18CZippyDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN18CZippyDecompressor4makeEv - 0b	\n"
"		.long		__ZN18CZippyDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CZippyDecompressor\"	\n"
"2:	.asciz	\"CDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN18CZippyDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN18CZippyDecompressor4makeEv - 4b	\n"
"		.long		__ZN18CZippyDecompressor7destroyEv - 4b	\n"
"		.long		__ZN18CZippyDecompressor4initEPv - 4b	\n"
"		.long		__ZN18CZippyDecompressor10decompressEPmPvmS1_m - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CZippyDecompressor)

CZippyDecompressor *
CZippyDecompressor::make(void)
{
	return this;
}


void
CZippyDecompressor::destroy(void)
{ }


NewtonErr
CZippyDecompressor::init(void *)
{
	return noErr;
}


NewtonErr
CZippyDecompressor::decompress(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen)
{
	decompressChunk(outSize, inDstBuf, inDstLen, inSrcBuf, inSrcLen);
	return noErr;
}


void
CZippyDecompressor::initCache(void)
{
	fSequence = 0;
	for (ArrayIndex i = 0; i < kZippyCacheSize; ++i)
	{
		fAge[i] = -1;
		fValue[i] = 0;
	}
}


NewtonErr
CZippyDecompressor::decompressChunk(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen)
{
	ZippyHeader * header = (ZippyHeader *)inSrcBuf;
	unsigned char * src = (unsigned char *)(header + 1);
#if defined(hasByteSwapping)
	//	assume header is big-endian - we never create it so it must be from Newton device
	header->size = BYTE_SWAP_LONG(header->size);
	header->flags = BYTE_SWAP_LONG(header->flags);
#endif		
	if (header->flags == 0x00010000)
	{
		// it’s encoded
		uint32_t * dst = (uint32_t *)inDstBuf;
		size_t len = 0;
		ArrayIndex bitIndex = 0;
		uint32_t value;
		unsigned char * srcLimit = src + header->size - 1;
		initCache();
		while (expandValue(&src, &bitIndex, srcLimit, &value))
		{
			len += sizeof(value);
			if (len > inDstLen)
				return ERRBASE_COMPRESSION - 100;
			*dst++ = value;
		}
		*outSize = len;
	}
	else
	{
		// it’s a verbatim copy
		size_t chunkSize = header->size - sizeof(ZippyHeader);
		*outSize = chunkSize;
		memcpy(inDstBuf, src, chunkSize);	// was fast_copy()
	}
	return noErr;
}


size_t
CZippyDecompressor::headerSize(void)
{
	return sizeof(ZippyHeader);
}


size_t
CZippyDecompressor::decompressedLength(void*, size_t)
{
	return kSubPageSize;
}


bool
CZippyDecompressor::expandValue(unsigned char ** ioSrc, ArrayIndex * ioBitIndex, unsigned char * inSrcLimit, uint32_t * outValue)
{
	union LongBytes
	{
		uint8_t uc[4];
		uint32_t ul;
	};

	LongBytes bits;		// sp00
	ArrayIndex index;
	unsigned char * src = *ioSrc;
	int tag, selector;	// r3, r5
	if (src >= inSrcLimit)
	{
		ArrayIndex bitIndex = *ioBitIndex;
		if (bitIndex == 7)
			return NO;
		if (src > inSrcLimit && bitIndex == 0)
			return NO;
		selector = (*src << bitIndex) & 0xC0;
		if (selector == 0xC0)
			return NO;
#if defined(hasByteSwapping)
		bits.uc[3] = src[0];
#else
		bits.uc[0] = src[0];
#endif
	}
	else
	{
#if defined(hasByteSwapping)
		bits.uc[3] = src[0];			// peek at 2 bit tag -- may cross byte boundary
		bits.uc[2] = src[1];
#else
		bits.uc[0] = src[0];
		bits.uc[1] = src[1];
#endif
		bits.ul <<= *ioBitIndex;	// shift to place in bit stream
#if defined(hasByteSwapping)
		tag = bits.uc[3];
#else
		tag = bits.uc[0];
#endif
		selector = tag & 0xC0;
	}

	switch (selector)
	{
	case 0x00:		// set zero word
		*ioBitIndex += 2;			// skip 2 bit tag selector
		if (*ioBitIndex > 7)
		{
			(*ioSrc)++;				// roll over to next byte
			*ioBitIndex &= 0x07;
		}
		*outValue = 0;
		break;

	case 0x80:	// copy from cache -- there are 16 cache entries
		*ioBitIndex += 6;			// skip 2 bit tag selector + 4 bit cache index
		if (*ioBitIndex > 7)
		{
			(*ioSrc)++;				// roll over to next byte
			*ioBitIndex &= 0x07;
		}
		index = (tag >> 2) & 0x0F;
		fAge[index] = ++fSequence;
		*outValue = CANONICAL_LONG(fValue[index]);
		break;

	case 0x40:	// copy 15 bit value from the stream
		{
#if defined(hasByteSwapping)
		bits.uc[3] = src[0];			// peek at next 2 bytes ± 1 byte
		bits.uc[2] = src[1];
		bits.uc[1] = src[2];
#else
		bits.uc[0] = src[0];
		bits.uc[1] = src[1];
		bits.uc[2] = src[2];
#endif

		uint32_t newValue = (bits.ul >> ((7 - *ioBitIndex) + 5)) & 0x3FF8;
		
		*ioBitIndex = (*ioBitIndex + 17) & 0x07;	// skip 2 tag bits + 15 bit value
		if (*ioBitIndex == 0)
			*ioSrc += 3;
		else
			*ioSrc += 2;
		index = (tag >> 2) & 0x0F;
		fAge[index] = ++fSequence;
		fValue[index] = (fValue[index] & ~0x3FF8) | newValue;
		*outValue = CANONICAL_LONG(fValue[index]);
		}
		break;

	case 0xC0:	// copy full 32-bit word from the stream
		{
		int shift = *ioBitIndex + 2;	// will shift past selector bits
#if defined(hasByteSwapping)
		bits.uc[3] = src[0];			// peek at next 3 bytes ± 1 byte
		bits.uc[2] = src[1];
		bits.uc[1] = src[2];
		bits.uc[0] = src[3];
#else
		bits.uc[0] = src[0];
		bits.uc[1] = src[1];
		bits.uc[2] = src[2];
		bits.uc[3] = src[3];
#endif
		bits.ul <<= shift;

		LongBytes word;
		if (*ioBitIndex == 7)
		{
#if defined(hasByteSwapping)
			word.uc[3] = src[2];
			word.uc[2] = src[3];
			word.uc[1] = src[4];
			word.uc[0] = src[5];
#else
			word.uc[0] = src[2];		// we’re on the cusp of the next byte
			word.uc[1] = src[3];
			word.uc[2] = src[4];
			word.uc[3] = src[5];
#endif
			word.ul >>= 7;				// in fact we only need 1 bit of it
			*ioBitIndex = 1;			// skip 2 bit tag
			*ioSrc += 5;				// + 1 word
		}
		else
		{
#if defined(hasByteSwapping)
			word.uc[3] = src[1];
			word.uc[2] = src[2];
			word.uc[1] = src[3];
			word.uc[0] = src[4];
#else
			word.uc[0] = src[1];		// copy the value word
			word.uc[1] = src[2];
			word.uc[2] = src[3];
			word.uc[3] = src[4];
#endif
			word.ul >>= (8 - shift);
			*ioBitIndex = (*ioBitIndex + 2) & 0x07;	// skip 2 bit tag...
			if (*ioBitIndex != 0)
				*ioSrc += 4;			//... + 1 word
			else
				*ioSrc += 5;
		}
#if defined(hasByteSwapping)
		bits.uc[1] = word.uc[1];
		bits.uc[0] = word.uc[0];
#else
		bits.uc[2] = word.uc[2];
		bits.uc[3] = word.uc[3];
#endif

		// update the cache -- replace the LRU (oldest) entry
		int minimumAge = 0x7FFFFFFF;
		for (ArrayIndex i = 0; i < kZippyCacheSize; ++i)
		{
			if (fAge[i] < minimumAge)
			{
				minimumAge = fAge[i];
				index = i;
			}
		}
		fAge[index] = ++fSequence;
		fValue[index] = bits.ul;
		*outValue = CANONICAL_LONG(fValue[index]);
		}
		break;
	}

	return YES;
}


void
CZippyDecompressor::finish(void*, size_t)
{ }


#pragma mark -
/*------------------------------------------------------------------------------
	C Z i p p y S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CZippyStoreDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CZippyStoreDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN23CZippyStoreDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN23CZippyStoreDecompressor4makeEv - 0b	\n"
"		.long		__ZN23CZippyStoreDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CZippyStoreDecompressor\"	\n"
"2:	.asciz	\"CStoreDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN23CZippyStoreDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN23CZippyStoreDecompressor4makeEv - 4b	\n"
"		.long		__ZN23CZippyStoreDecompressor7destroyEv - 4b	\n"
"		.long		__ZN23CZippyStoreDecompressor4initEP6CStorejPc - 4b	\n"
"		.long		__ZN23CZippyStoreDecompressor4readEjPcmm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CZippyStoreDecompressor)

CZippyStoreDecompressor *
CZippyStoreDecompressor::make(void)
{
	fBuffer = NULL;
	fDecompressor = NULL;
	return this;
}


void
CZippyStoreDecompressor::destroy(void)
{
	if (fDecompressor)
		fDecompressor->destroy();
	FreePtr(fBuffer);
}


NewtonErr
CZippyStoreDecompressor::init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer)
{
	NewtonErr err = noErr;

	XTRY
	{
		fStore = inStore;
		XFAILNOT(fBuffer = NewPtr(kSubPageSize), err = kOSErrNoMemory;)
		XFAILNOT(fDecompressor = (CZippyDecompressor *)MakeByName("CDecompressor", "CZippyDecompressor"), err = kOSErrNoMemory;)
	}
	XENDTRY;

	return err;
}


NewtonErr
CZippyStoreDecompressor::read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
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
	C Z i p p y R e l o c S t o r e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CZippyRelocStoreDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CZippyRelocStoreDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN28CZippyRelocStoreDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN28CZippyRelocStoreDecompressor4makeEv - 0b	\n"
"		.long		__ZN28CZippyRelocStoreDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CZippyRelocStoreDecompressor\"	\n"
"2:	.asciz	\"CStoreDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN28CZippyRelocStoreDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN28CZippyRelocStoreDecompressor4makeEv - 4b	\n"
"		.long		__ZN28CZippyRelocStoreDecompressor7destroyEv - 4b	\n"
"		.long		__ZN28CZippyRelocStoreDecompressor4initEP6CStorejPc - 4b	\n"
"		.long		__ZN28CZippyRelocStoreDecompressor4readEjPcmm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CZippyRelocStoreDecompressor)

CZippyRelocStoreDecompressor *
CZippyRelocStoreDecompressor::make(void)
{
	fBuffer = NULL;
	fDecompressor = NULL;
	return this;
}


void
CZippyRelocStoreDecompressor::destroy(void)
{
	if (fDecompressor)
		fDecompressor->destroy();
	FreePtr(fBuffer);
}


NewtonErr
CZippyRelocStoreDecompressor::init(CStore * inStore, PSSId inParmsId, char * inLZWBuffer)
{
	NewtonErr err = noErr;

	XTRY
	{
		fStore = inStore;
		XFAILNOT(fBuffer = NewPtr(kSubPageSize), err = kOSErrNoMemory;)
		XFAILNOT(fDecompressor = (CZippyDecompressor *)MakeByName("CDecompressor", "CZippyDecompressor"), err = kOSErrNoMemory;)
	}
	XENDTRY;

	return err;
}


NewtonErr
CZippyRelocStoreDecompressor::read(PSSId inObjId, char * outBuf, size_t inBufLen, VAddr inBaseAddr)
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

		// read object into our buffer
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

void
InitZippyCompression(void)
{
#if 0
	CZippyCallbackCompressor::classInfo()->registerProtocol();
	CZippyCompressor::classInfo()->registerProtocol();
#endif
	InitZippyDecompression();
}


void
InitZippyDecompression(void)
{
	CZippyDecompressor::classInfo()->registerProtocol();
}
