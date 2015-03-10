/*
	File:		LZDecompressor.h

	Contains:	LZ decompression class.
	
	Written by:	Newton Research Group, 2006.
					With a little help from Philz, and a disassembly of the DILs.
*/

#if !defined(__LZDECOMPRESSOR_H)
#define __LZDECOMPRESSOR_H

#include "Compression.h"
#include "PushPopBits.h"


PROTOCOL CLZDecompressor : public CDecompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CLZDecompressor)

	CLZDecompressor *	make(void);
	void				destroy(void);

	NewtonErr		init(void * inContext);
	NewtonErr		decompress(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);

private:
	void				decompressChunk(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);
	void				decompressBlock(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);
	void				codeword_dec_bin(size_t * outCopyLen, size_t * outCopyOffset, size_t * outLiteralLen, long inLen);
	unsigned char	decode_copy_length_bin_huff4(void);
	size_t			decode_lit_len_bin(void);
	size_t			decode_offset_bin(long);

	CPushPopper		fBitStack;			// +10
	long				fDecodeCase;		// +2C
	size_t			fDataSize;			// +30
	size_t			fSrcUsed;			// +34
	bool				fBinaryFlag;		// +38
	bool				fLitFlag;			// +39
	bool				fMemFlag;			// +39
};


#endif	/* __LZDECOMPRESSOR_H */
