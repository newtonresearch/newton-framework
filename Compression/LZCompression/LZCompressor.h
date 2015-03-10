/*
	File:		LZCompressor.h

	Contains:	LZ compression class.
	
	Written by:	Newton Research Group, 2006.
					With a little help from Philz, and a disassembly of the DILs.
*/

#if !defined(__LZCOMPRESSOR_H)
#define __LZCOMPRESSOR_H

#include "Newton.h"
#include "Compression.h"
#include "PushPopBits.h"


struct TTNode
{
	short		start;	// +00
	short		level;	// +02
	short		length;	// +04
	TTNode *	child;	// +08
	TTNode *	parent;	// +0C
	TTNode *	sibling;	// +10
};


PROTOCOL CLZCompressor : public CCompressor
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CLZCompressor)

	CLZCompressor *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	NewtonErr	compress(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);
	size_t		estimatedCompressedSize(void * inSrcBuf, size_t inSrcLen);

private:
	NewtonErr	setHeader(void * inBuf, size_t inBufLen);
	size_t		headerSize(void);

	NewtonErr	compressChunk(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);
	NewtonErr	compressBlock(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen);
	void			finish(void * inBuf, size_t inBufLen);
	void			codeword_gen_bin(size_t inCopyLen, long, size_t inLiteralLen, unsigned char*, size_t inLen);
	void			encode_copy_length_bin_huff4(long inLen);
	void			encode_lit_len_bin(long inLen);
	void			encode_offset_bin(long inParam, unsigned long b);

	TTNode *		talloc(void);

	friend void	TreeSearch(unsigned char * inSrcBuf, unsigned char * inSrcPtr, size_t inSrcLen, size_t * outCopyLen, /*+14*/long * outOffset, /*+18*/TTNode * inThisNode, /*+1C*/TTNode * inNodes, /*+20*/long inArg20, /*+24*/CLZCompressor * inCompressor);
	friend void	AddFirstChild(unsigned char inArg1, TTNode * inNode, TTNode * inChild, long inArg4, unsigned long inArg5, CLZCompressor * inCompressor);
	friend void	AddressANode(unsigned char inArg1, TTNode * inNode1, TTNode * inNode2, TTNode * inNode3, long inArg5, long inArg6, CLZCompressor * inCompressor);
	friend void	InsertANode(unsigned char inArg1, TTNode * inNode1, TTNode * inNode2, TTNode * inNode3, long inArg5, long inArg6, long inArg7, CLZCompressor * inCompressor);


	bool				fFreeze;				// +10
	size_t			fMaxNode;			// +14
	TTNode *			fNode[256];			// +18	a node per byte
	size_t			fNumNodes;			// +420
	size_t			fNodeCount;			// +424
	long				fEncodeCase;		// +428
	bool				fBinaryFlag;		// +42C
	bool				fLitFlag;			// +42D
	bool				fMemFlag;			// +42E
	CPushPopper *	fBitStack;			// +430
	TTNode *			fAvailNode;			// +434	malloc’d buffer
};


#endif	/* __LZCOMPRESSOR_H */
