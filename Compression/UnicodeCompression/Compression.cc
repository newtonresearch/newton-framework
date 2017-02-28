/*
	File:		Compression.cc

	Contains:	Unicode compression initialization.

	Written by:	Newton Research Group, 2007.
*/

#include "Newton.h"
#include "UnicodeCompression.h"
#include "OSErrors.h"

extern	void		ThrowOSErr(NewtonErr err);

unsigned char gUnicodeLookupTable[32] =
{
	0xBE,0x7E,0x80,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};


/*------------------------------------------------------------------------------
	C U n i c o d e C o m p r e s s o r
------------------------------------------------------------------------------*/

PROTOCOL CUnicodeCompressor : public CCallbackCompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CUnicodeCompressor)

	CUnicodeCompressor *	make(void);
	void			destroy(void);

	NewtonErr	init(void *);
	void			reset(void);
	NewtonErr	writeChunk(void * inSrcBuf, size_t inSrcLen);
	NewtonErr	flush(void);

private:
	NewtonErr	writeRun(void);
	NewtonErr	write(unsigned char inCh);

	friend CCallbackCompressor * NewCompressor(CompressionType inCompression, WriteProcPtr inWriter, VAddr instance);

	WriteProcPtr	fWrite;
	VAddr				fHandler;
	unsigned char	fBuf[128];	// +18
	ArrayIndex		fBufLen;		// +98
	ArrayIndex		fRunLen;		// +9C
	unsigned char	fCodeTable;	// +A0
	unsigned char	fRun[256];	// +A1
};


/*------------------------------------------------------------------------------
	C U n i c o d e C o m p r e s s o r
	Unicode text is compressed into runs of bytes per code table (high byte of
	16-bit Unicode).
	So, ASCII text is rendered as bytes in code table 0
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CUnicodeCompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CUnicodeCompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN18CUnicodeCompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN18CUnicodeCompressor4makeEv - 0b	\n"
"		.long		__ZN18CUnicodeCompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CUnicodeCompressor\"	\n"
"2:	.asciz	\"CCallbackCompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN18CUnicodeCompressor9classInfoEv - 4b	\n"
"		.long		__ZN18CUnicodeCompressor4makeEv - 4b	\n"
"		.long		__ZN18CUnicodeCompressor7destroyEv - 4b	\n"
"		.long		__ZN18CUnicodeCompressor4initEPv - 4b	\n"
"		.long		__ZN18CUnicodeCompressor5resetEv - 4b	\n"
"		.long		__ZN18CUnicodeCompressor10writeChunkEPvm - 4b	\n"
"		.long		__ZN18CUnicodeCompressor5flushEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CUnicodeCompressor)

CUnicodeCompressor *
CUnicodeCompressor::make(void)
{
	reset();
	return this;
}

void
CUnicodeCompressor::destroy(void)
{ }


NewtonErr
CUnicodeCompressor::init(void *)
{
	return noErr;
}


void
CUnicodeCompressor::reset(void)
{
	fBufLen = 0;
	fCodeTable = 0;
	fRunLen = 0;
}


/* -----------------------------------------------------------------------------
	Write a chunk of Unicode text to store, in compressed form.
	Args:		inSrcBuf			Unicode text
				inSrcLen			number of bytes, ie MUST be 2 * number of unichars
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CUnicodeCompressor::writeChunk(void * inSrcBuf, size_t inSrcLen)
{
	NewtonErr err = noErr;

	XTRY
	{
		// MUST be writing a whole number of UniChars
		XFAILIF(ODD(inSrcLen), err = kOSErrBadParameters;)

		UniChar * s = (UniChar *)inSrcBuf;
		for (ArrayIndex i = 0, count = inSrcLen / sizeof(UniChar); i < count; ++i)
		{
			UniChar ch = *s++;
			unsigned char charCode = ch & 0xFF;
			unsigned char codeTable = (ch >> 8) & 0xFF;
			if (fRunLen > 0)
			{
				// we’re in a run
				if (codeTable == fCodeTable)
				{
					// in the same code table -- add char to the run
					fRun[fRunLen++] = charCode;
					if (fRunLen >= 255)
						XFAIL(err = writeRun())
					continue;
				}
				else
					// new code table -- flush the current run
					XFAIL(err = writeRun())
			}
			if ((gUnicodeLookupTable[codeTable/8] & (0x80 >> (codeTable&7))) != 0)
			{
				// start a new codetable run
				fRun[0] = charCode;
				fRunLen = 1;
				fCodeTable = codeTable;
			}
			else
			{
				// write the unicode character individually
				write(codeTable);
				write(charCode);
			}
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
CUnicodeCompressor::flush(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = writeRun())
		err = fWrite(fHandler, fBuf, fBufLen, true);
		fBufLen = 0;
	}
	XENDTRY;
	return err;
}


NewtonErr
CUnicodeCompressor::writeRun(void)
{
	NewtonErr err = noErr;
	if (fRunLen != 0)
	{
		write(fCodeTable);
		write(fRunLen);
		for (ArrayIndex i = 0; i < fRunLen; ++i)
			write(fRun[i]);
		fRunLen = 0;
	}
	return err;
}


/*originally inline*/ NewtonErr
CUnicodeCompressor::write(unsigned char inCh)
{
	NewtonErr err = noErr;
	if (fBufLen >= 128)
	{
		// buffer is full, flush to store
		err = fWrite(fHandler, fBuf, fBufLen, false);
		fBufLen = 0;
	}
	fBuf[fBufLen++] = inCh;
	return err;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C U n i c o d e D e c o m p r e s s o r
----------------------------------------------------------------------------- */

PROTOCOL CUnicodeDecompressor : public CCallbackDecompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CUnicodeDecompressor)

	CUnicodeDecompressor *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	void			reset(void);
	NewtonErr	readChunk(void * inDstBuf, size_t * outDstLen, bool * outDone);

private:
	friend CCallbackDecompressor * NewDecompressor(CompressionType inCompression, ReadProcPtr inReader, VAddr instance);

	ReadProcPtr		fRead;
	VAddr				fHandler;
	unsigned char	fRunLen;		//+18
	ArrayIndex		fRunIndex;	//+1C
	unsigned char	fCodeTable;	//+20
	unsigned char	fRun[256];	//+21
	bool				fIsEOF;		//+121
};


/*------------------------------------------------------------------------------
	C U n i c o d e D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CUnicodeDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CUnicodeDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN20CUnicodeDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN20CUnicodeDecompressor4makeEv - 0b	\n"
"		.long		__ZN20CUnicodeDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CUnicodeDecompressor\"	\n"
"2:	.asciz	\"CCallbackDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN20CUnicodeDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN20CUnicodeDecompressor4makeEv - 4b	\n"
"		.long		__ZN20CUnicodeDecompressor7destroyEv - 4b	\n"
"		.long		__ZN20CUnicodeDecompressor4initEPv - 4b	\n"
"		.long		__ZN20CUnicodeDecompressor5resetEv - 4b	\n"
"		.long		__ZN20CUnicodeDecompressor9readChunkEPvPmPb - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CUnicodeDecompressor)

CUnicodeDecompressor *
CUnicodeDecompressor::make(void)
{
	reset();
	return this;
}


void
CUnicodeDecompressor::destroy(void)
{ }


NewtonErr
CUnicodeDecompressor::init(void * inContext)
{
	return noErr;
}


void
CUnicodeDecompressor::reset(void)
{
	fCodeTable = 0;
	fRunLen = 0;
	fRunIndex = 0;
	fIsEOF = false;
}


/* -----------------------------------------------------------------------------
	Read a chunk of Unicode text from store, and decompress it.
	Args:		inDstBuf			Unicode text
				ioDstLen			number of bytes, ie MUST be 2 * number of unichars
									on exit, number of bytes actually read
				outDone			true => no more text available
	Return:	error code
----------------------------------------------------------------------------- */

NewtonErr
CUnicodeDecompressor::readChunk(void * inDstBuf, size_t * ioDstLen, bool * outDone)
{
	NewtonErr err = noErr;

	XTRY
	{
		// MUST be reading a whole number of UniChars
		XFAILIF(ODD(*ioDstLen), err = kOSErrBadParameters;)

		*outDone = false;

		size_t size;
		unsigned char * s = (unsigned char *)inDstBuf;
		// attempt to fill the whole buffer
		for (ArrayIndex i = 0, count = *ioDstLen / sizeof(UniChar); i < count; ++i)
		{
			if (fIsEOF && fRunLen == 0)
			{
				// no more compressed data, and no run in progress -- we’re done
				*ioDstLen = i * sizeof(UniChar);
				break;
			}
			if (fRunLen > 0)
			{
				// we’re in a codetable run
				// output Unicode char from codetable|charcode bytes
#if defined(hasByteSwapping)
				*s++ = fRun[fRunIndex++];
				*s++ = fCodeTable;
#else
				*s++ = fCodeTable;
				*s++ = fRun[fRunIndex++];
#endif
				if (fRunIndex >= fRunLen)
				{
					// run is done
					fRunLen = 0;
					fRunIndex = 0;
				}
			}
			else
			{
				// read codetable
				size = 1;
				XFAIL(err = fRead(fHandler, &fCodeTable, &size, &fIsEOF))
				// read codetable run length
				size = 1;
				XFAIL(err = fRead(fHandler, &fRunLen, &size, &fIsEOF))

				// output codetable
#if !defined(hasByteSwapping)
				*s++ = fCodeTable;
#endif
				if (gUnicodeLookupTable[fCodeTable/8] & (0x80 >> (fCodeTable&7)))
				{
					// this codetable can handle runs -- read the run
					// the compressor ensures the run will not overrun our buffer limit (256 bytes, 128 Unicode chars)
					size = fRunLen;
					XFAIL(err = fRead(fHandler, &fRun, &size, &fIsEOF))
					// output the first charcode in the run
					*s++ = fRun[fRunIndex++];
					if (fRunIndex >= fRunLen)
					{
						// run is done -- really? immediately?
						fRunLen = 0;
						fRunIndex = 0;
					}
				}
				else
				{
					// it’s an unpopular codetable -- treat the runlen as the charcode
					*s++ = fRunLen;
					fRunLen = 0;
				}
#if defined(hasByteSwapping)
				*s++ = fCodeTable;
#endif
			}
		}
		if (fIsEOF && fRunLen == 0)
			// no more compressed data, and no run in progress -- we’re done
			*outDone = true;
	}
	XENDTRY;

	return err;
}


#pragma mark -
/*------------------------------------------------------------------------------
	C o m p r e s s i o n
------------------------------------------------------------------------------*/

void
InitUnicodeCompression(void)
{
	CUnicodeCompressor::classInfo()->registerProtocol();
	CUnicodeDecompressor::classInfo()->registerProtocol();
}


CCallbackCompressor *
NewCompressor(CompressionType inCompression, WriteProcPtr inWriter, VAddr instance)
{
	NewtonErr err;
	CUnicodeCompressor *	compr = NULL;
	XTRY
	{
		XFAIL(inCompression != kUnicodeCompression)
		compr = (CUnicodeCompressor *)MakeByName("CCallbackCompressor", "CUnicodeCompressor");
		XFAILIF(compr == NULL, ThrowMsg("Couldn’t create compressor.");)
		XFAILIF(err = compr->init(NULL), ThrowOSErr(err);)
		compr->fWrite = inWriter;
		compr->fHandler = instance;
	}
	XENDTRY;
	return compr;
}


CCallbackDecompressor *
NewDecompressor(CompressionType inCompression, ReadProcPtr inReader, VAddr instance)
{
	NewtonErr err;
	CUnicodeDecompressor *	decompr = NULL;
	XTRY
	{
		XFAIL(inCompression != kUnicodeCompression)
		decompr = (CUnicodeDecompressor *)MakeByName("CCallbackDecompressor", "CUnicodeDecompressor");
		XFAILIF(decompr == NULL, ThrowMsg("Couldn’t create decompressor.");)
		XFAILIF(err = decompr->init(NULL), ThrowOSErr(err);)
		decompr->fRead = inReader;
		decompr->fHandler = instance;
	}
	XENDTRY;
	return decompr;
}

