/*
	File:		LZDecompressor.cc

	Contains:	LZ decompression class.
	
	Written by:	Newton Research Group, 2006.
					With a little help from Philz, and a disassembly of the DILs.
*/

#include "LZDecompressor.h"
#include "PushPopBits.h"
#include "CopyOffset_bin.h"
#include "LZCompressionData-j.h"

void
InitLZDecompression(void)
{
	CLZDecompressor::classInfo()->registerProtocol();
}

#pragma mark -

/*------------------------------------------------------------------------------
	C L Z D e c o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CLZDecompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CLZDecompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN15CLZDecompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN15CLZDecompressor4makeEv - 0b	\n"
"		.long		__ZN15CLZDecompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CLZDecompressor\"	\n"
"2:	.asciz	\"CDecompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN15CLZDecompressor9classInfoEv - 4b	\n"
"		.long		__ZN15CLZDecompressor4makeEv - 4b	\n"
"		.long		__ZN15CLZDecompressor7destroyEv - 4b	\n"
"		.long		__ZN15CLZDecompressor4initEPv - 4b	\n"
"		.long		__ZN15CLZDecompressor10decompressEPmPvmS1_m - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CLZDecompressor)

CLZDecompressor *
CLZDecompressor::make(void)
{
	return this;
}


void
CLZDecompressor::destroy(void)
{ }


NewtonErr
CLZDecompressor::init(void * inContext)
{
	return noErr;
}


/*------------------------------------------------------------------------------
	Decompress LZ encoded data into a buffer.
	Args:		outSize		returns actual size of decompressed data
				inDstBuf		where to decompress into
				inDstLen		size of that buffer
				inSrcBuf		the LZ encoded data
				inSrcLen		size of that data
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CLZDecompressor::decompress(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen)
{
	decompressChunk(outSize, inDstBuf, inDstLen, inSrcBuf, inSrcLen);
	return noErr;
}


/*------------------------------------------------------------------------------
	Decompress LZ encoded data into a buffer.
	The data is prefixed with its total compressed length, and comprises blocks
	of compressed data which decompress to 1K blocks.
	Args:		outSize		returns actual size of decompressed data
				inDstBuf		where to decompress into
				inDstLen		size of that buffer
				inSrcBuf		the LZ encoded data
				inSrcLen		size of that data
	Return:	--
------------------------------------------------------------------------------*/

void
CLZDecompressor::decompressChunk(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen)
{
	unsigned char *	srcPtr = (unsigned char *)inSrcBuf + sizeof(uint32_t);	// first block is prefixed with its length
	size_t	srcLen = inSrcLen;
	unsigned char *	dstPtr = (unsigned char *)inDstBuf;
	size_t	dstLen = inDstLen;

	size_t	totalSize = 0;
	size_t	doneSize = 0;
	bool		isFirstBlock = YES;
	fSrcUsed = 0;
	fDataSize = 1;		// anything non-zero to start loop

	while (fSrcUsed != 2*kSubPageSize && fDataSize > 0)
	{
		if (isFirstBlock)
		{
			isFirstBlock = NO;
			fDataSize = CANONICAL_LONG(*(uint32_t *)inSrcBuf) - sizeof(uint32_t);	// first block is prefixed with its length
			srcLen = fDataSize + sizeof(uint32_t);	// include the length word
			if (fDataSize > 0)
				decompressBlock(&doneSize, dstPtr, dstLen, srcPtr, srcLen - sizeof(uint32_t));
		}
		else
			if (fDataSize > 0)
				decompressBlock(&doneSize, dstPtr, dstLen, srcPtr, srcLen);
		if (fSrcUsed > fDataSize)
			fSrcUsed = fDataSize;	// ensure we didn’t claim to read more bytes than were available
		fDataSize -= fSrcUsed;
		totalSize += doneSize;
		dstPtr += doneSize;
		srcPtr += fSrcUsed;
		srcLen -= fSrcUsed;
	}
	*outSize = totalSize;
}


/*------------------------------------------------------------------------------
	Decompress LZ encoded data into a buffer.
	Args:		outSize		returns actual size of decompressed data
				inDstBuf		where to decompress into
				inDstLen		size of that buffer
				inSrcBuf		the LZ encoded data
				inSrcLen		size of that data
	Return:	--
------------------------------------------------------------------------------*/

void
CLZDecompressor::decompressBlock(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen)
{
	unsigned char *	srcPtr =  (unsigned char *)inSrcBuf + sizeof(uint32_t);
	unsigned char *	srcEndPtr =  (unsigned char *)inSrcBuf + inSrcLen;
	unsigned char *	dstPtr = (unsigned char *)inDstBuf;
	size_t	totalSize = 0;
	size_t	copyLength;
	size_t	copyOffset;
	size_t	literalLength;

	fLitFlag = YES;
	fDecodeCase = 10;

	if (*(unsigned char *)inSrcBuf == 1)
	{
		// block is uncompressed - copy it verbatim to output
		copyLength = (fDataSize > kSubPageSize) ? kSubPageSize : fDataSize - sizeof(uint32_t);
		memmove(inDstBuf, srcPtr, copyLength);	// original does fast_copy(src, dst, len)
		*outSize = copyLength;
		fSrcUsed = copyLength + sizeof(uint32_t);	// include length word
	}
	else
	{
		fBitStack.setReadBuffer(srcPtr, inSrcLen - sizeof(uint32_t));	// original doesn’t bother reducing length by size of length prefix
		fBinaryFlag = YES;	// never used
		while (srcPtr <= srcEndPtr
			&& totalSize < kSubPageSize
			&& ((srcPtr - (unsigned char *)inSrcBuf) < fDataSize
			 || ((srcPtr - (unsigned char *)inSrcBuf) == 0 && (fBitStack.getBitOffset() & 0x03) == 0x03)))
		{
			codeword_dec_bin(&copyLength, &copyOffset, &literalLength, totalSize);
			if (copyLength > 0)
			{
				// copy previously decompressed bytes
				if (totalSize + copyLength <= kSubPageSize)	// sanity check - don’t exceed block buffer
				{
					totalSize += copyLength;
					unsigned char * copyPtr = dstPtr - copyOffset;
					for (int i = copyLength; i > 0; i -= 8)
					{
						switch (i)
						{
						default:
						case 0: *dstPtr++ = *copyPtr++;
						case 7: *dstPtr++ = *copyPtr++;
						case 6: *dstPtr++ = *copyPtr++;
						case 5: *dstPtr++ = *copyPtr++;
						case 4: *dstPtr++ = *copyPtr++;
						case 3: *dstPtr++ = *copyPtr++;
						case 2: *dstPtr++ = *copyPtr++;
						case 1: *dstPtr++ = *copyPtr++;
						}
					}
				}
			}
			else if (literalLength > 0)
			{
				if ((srcPtr - (unsigned char *)inSrcBuf) < fDataSize)	// sanity check
				{
					// copy literally from the source
					fBitStack.popString(dstPtr, literalLength);
					totalSize += literalLength;
					dstPtr += literalLength;
				}
			}
			srcPtr = (unsigned char *)inSrcBuf + sizeof(uint32_t) + (fBitStack.getByteCount() - fBitStack.getBitOffset()/8);
		}
		*outSize = totalSize;
		fSrcUsed = sizeof(uint32_t) + (fBitStack.getByteCount() - fBitStack.getBitOffset()/8);
	}
}


/*------------------------------------------------------------------------------
	Decode the next codeword in the LZ stream.
	Args:		outCopyLen			if codeword is to copy, length to copy
				outCopyOffset										offset to data
				outLiteralLen		if codeword is literal, length of literal to follow
				inLen					length of LZ stream
	Return:	--
------------------------------------------------------------------------------*/

void
CLZDecompressor::codeword_dec_bin(size_t * outCopyLen, size_t * outCopyOffset, size_t * outLiteralLen, long inLen)
{
	size_t copyLen = decode_copy_length_bin_huff4();
	if (copyLen == 0 && fLitFlag)
	{
		*outLiteralLen = decode_lit_len_bin();
		fLitFlag = (*outLiteralLen >= 63);
	}
	else
	{
		copyLen += 2;
		if (!fLitFlag)
			copyLen++;
		fLitFlag = YES;
		*outCopyOffset = decode_offset_bin(inLen);
	}
	*outCopyLen = copyLen;
}


/*------------------------------------------------------------------------------
	Decode the length of a literal.
	Args:		-
	Return:	length
------------------------------------------------------------------------------*/

size_t
CLZDecompressor::decode_lit_len_bin(void)
{
	size_t		literalLen = 1;
	uint32_t		bits = fBitStack.popFewBits(1);
	if (bits != 0)
	{
		bits = fBitStack.popFewBits(2);
		if (bits == 0)
			literalLen = 2;
		else if (bits == 1)
			literalLen = 3;
		else if (bits == 2)
			literalLen = 4 + fBitStack.popFewBits(2);
		else if (bits == 3)
		{
			bits = fBitStack.popFewBits(4);
			if (/*bits >= 0 &&*/ bits <= 7)
				literalLen = 8 + bits;
			else if (bits >= 8 && bits <= 11)
				literalLen = 16 + (bits - 8)*4 + fBitStack.popFewBits(2);
			else if (bits >= 12 && bits <= 15)
				literalLen = 32 + (bits - 12)*8 + fBitStack.popFewBits(3);
		}
	}
	return literalLen;
}


/*------------------------------------------------------------------------------
	Decode the length of previously decompressed data to copy.
	Args:		-
	Return:	length
------------------------------------------------------------------------------*/

unsigned char
CLZDecompressor::decode_copy_length_bin_huff4(void)
{
	unsigned char v;
	int bits;

	v = fBitStack.popFewBits(8);
	bits = LZCopyBits[v];
	if (bits > 8)
		return CopyValue[v] + fBitStack.popBits(bits - 8);

	fBitStack.restoreBits(8 - bits);
	return CopyValue[v];
}


/*------------------------------------------------------------------------------
	Decode the offset to previously decompressed data to copy.
	Args:		inParam
	Return:	offset
------------------------------------------------------------------------------*/

size_t
CLZDecompressor::decode_offset_bin(long inParam)
{
	size_t offset;

	switch (fDecodeCase)
	{
	case 10:
		if ((offset = decode_offset_case10_bin(inParam, &fBitStack)) < 0x0015) break;
		fDecodeCase = 9;
	case 9:
		if ((offset = decode_offset_case9_bin(inParam, &fBitStack)) < 0x002A) break;
		fDecodeCase = 8;
	case 8:
		if ((offset = decode_offset_case8_bin(inParam, &fBitStack)) < 0x0054) break;
		fDecodeCase = 7;
	case 7:
		if ((offset = decode_offset_case7_bin(inParam, &fBitStack)) < 0x00A8) break;
		fDecodeCase = 6;
	case 6:
		if ((offset = decode_offset_case6_bin(inParam, &fBitStack)) < 0x0150) break;
		fDecodeCase = 5;
	case 5:
		if ((offset = decode_offset_case5_bin(inParam, &fBitStack)) < 0x02A0) break;
		fDecodeCase = 4;
	case 4:
		if ((offset = decode_offset_case4_bin(inParam, &fBitStack)) < 0x0540) break;
		fDecodeCase = 3;
	case 3:
		if ((offset = decode_offset_case3_bin(inParam, &fBitStack)) < 0x0A80) break;
		fDecodeCase = 2;
	case 2:
		if ((offset = decode_offset_case2_bin(inParam, &fBitStack)) < 0x1500) break;
		fDecodeCase = 1;
	case 1:
		if ((offset = decode_offset_case1_bin(inParam, &fBitStack)) < 0x2A00) break;
	default:
		offset = 0;
	}
	return offset;
}
