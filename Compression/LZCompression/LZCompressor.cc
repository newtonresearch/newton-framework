/*
	File:		LZCompressor.cc

	Contains:	LZ compression class.
	
	Written by:	Newton Research Group, 2007.
*/

#include "LZCompressor.h"
#include "CopyOffset_bin.h"
#include "LZCompressionData-j.h"
#include "OSErrors.h"


extern void	InitLZDecompression(void);

void
InitLZCompression(void)
{
#if 0
//	we don’t have this yet
	CLZCallbackCompressor::classInfo()->registerProtocol();
#endif
	CLZCompressor::classInfo()->registerProtocol();

	InitLZDecompression();
}


/*------------------------------------------------------------------------------
	T r e e S e a r c h
------------------------------------------------------------------------------*/

void	TreeSearch(unsigned char * inSrcBuf, unsigned char * inSrcPtr, size_t inSrcLen, size_t * outCopyLen, /*+14*/long * outOffset, /*+18*/TTNode * inThisNode, /*+1C*/TTNode * inNodes, /*+20*/long inArg20, /*+24*/CLZCompressor * inCompressor);
void	AddFirstChild(unsigned char index, TTNode * inNode, TTNode * inChild, long inStart, unsigned long inLength, CLZCompressor * inCompressor);
void	ExtendAChild(TTNode * inNode, TTNode * inNode2, long inStart, long inArg4);
void	UpdateANode(TTNode * inNode);
void	AddASibling(TTNode * inNode, TTNode * inSibling, long inStart, long inLevel);
void	AddressANode(unsigned char index, TTNode * inNode1, TTNode * inNode2, TTNode * inNode3, long inStart, long inLevel, CLZCompressor * inCompressor);
void	InsertANode(unsigned char index, TTNode * inNode1, TTNode * inNode2, TTNode * inNode3, long inStart, long inLength, long inLevel, CLZCompressor * inCompressor);


void
TreeSearch(unsigned char * inSrcBuf, unsigned char * inSrcPtr, size_t inSrcLen, size_t * outCopyLen, /*+14*/long * outOffset, /*+18*/TTNode * inThisNode, /*+1C*/TTNode * inNodes, /*+20*/long inArg20, /*+24*/CLZCompressor * inCompressor)
{
/*	r5 = inSrcBuf;
	r4 = inSrcPtr;
	r6 = inSrcLen;
	r10 = inCompressor;
*/
	TTNode *	sp04 = NULL;
	int		state = (inSrcPtr == inSrcBuf) ? 4 : 0;	// r9
	size_t	srcUsed = inSrcPtr - inSrcBuf;		// r8
	size_t	srcRemaining = inSrcLen - srcUsed;	// r0
	size_t	level = MIN(srcRemaining, 0x40);		// sp00	assume the most
	TTNode *	lr;
	if (!inCompressor->fFreeze)
		lr = inCompressor->talloc();
	TTNode *	r12 = inThisNode->child;
	TTNode *	r2;
	unsigned char * srcEnd = inSrcBuf + inSrcLen;	// r1
	int len;

	while (state == 0)
	{
		r2 = r12->parent;
		if (r2 == inThisNode)
		{
			if (inCompressor->fNode[*inSrcPtr] != NULL)
			{
				r12 = inCompressor->fNode[*inSrcPtr];
//E1E0 -->
				if (r12->length == 1)
				{
					if (r12->child != NULL)
						r12 = r12->child;
					else
						state = 5;	// no children; extend this one
				}
				else if (r12->length > 1)
				{
//E20C
					for (len = 1; len < r12->length; len++)
					{
						int r0 = r12->parent->level;
						if (srcUsed + r0 + len <= inSrcLen
						&&  inSrcPtr + r0 + len < srcEnd)
						{
							if (inSrcBuf[r12->start + r0 + len] == inSrcPtr[r0 + len])
							{
								if (r12->length - 1 == len)
								{
									if (r12->child != NULL)
									{
										r12 = r12->child;
										len = r12->length;
									}
									else
									{
										state = 5;	// no children; extend this one
										break;
									}
								}
							}
							else
							{
								len--;
								state = 3;
								break;
							}
						}
						else
						{
							len--;
							state = 8;
							break;
						}
					}
				}
// <--
			}
			else
			{
				state = 6;
			}
		}
		else
		{
//E178
			int r0 = r2->level;	// offset?
			if (srcUsed + r0 <= inSrcLen
			&&  inSrcPtr + r0 < srcEnd)
			{
				if (inSrcBuf[r12->start + r0] == inSrcPtr[r0])
				{
					if (r2->child != r12)
					{
						sp04->sibling = r12->sibling;
						r12->sibling = r12->parent->child;
						r12->parent->child = r12;
					}
//E1E0 -->
					if (r12->length == 1)
					{
						if (r12->child != NULL)
							r12 = r12->child;
						else
							state = 5;	// no children; extend this one
					}
					else if (r12->length > 1)
					{
//E20C
						for (len = 1; len < r12->length; len++)
						{
							int r0 = r12->parent->level;
							if (srcUsed + r0 + len <= inSrcLen
							&&  inSrcPtr + r0 + len < srcEnd)
							{
								if (inSrcBuf[r12->start + r0 + len] == inSrcPtr[r0 + len])
								{
									if (r12->length - 1 == len)
									{
										if (r12->child != NULL)
										{
											r12 = r12->child;
											len = r12->length;
										}
										else
										{
											state = 5;	// no children; extend this one
											break;
										}
									}
								}
								else
								{
									len--;
									state = 3;
									break;
								}
							}
							else
							{
								len--;
								state = 8;
								break;
							}
						}
					}
// <--
				}
				else
				{
//E300
					if (r12->sibling != NULL)
					{
						sp04 = r12;
						r12 = r12->sibling;
					}
					else
					{
						state = 2;	// no siblings; add one
					}
				}
			}
			else
			{
				state = 7;	// beyond the end of the source
			}
		}
	}

//E2AC
	switch (state)
	{
	case 2:
		*outCopyLen = r12->parent->level;
		*outOffset = srcUsed - r12->parent->start;
		if (!inCompressor->fFreeze)
			AddASibling(r12, lr, srcUsed, level);
		break;

	case 3:
		*outCopyLen = r12->parent->level + len + 1;
		*outOffset = srcUsed - r12->start;
		if (!inCompressor->fFreeze)
			InsertANode(*inSrcPtr, r12, lr, inThisNode, srcUsed, len+1, level, inCompressor);
		break;

	case 4:
		*outCopyLen = 0;
		*outOffset = 0;
		AddFirstChild(*inSrcPtr, inThisNode, lr, srcUsed, level, inCompressor);
		break;

	case 5:
		*outCopyLen = r12->level;
		*outOffset = srcUsed - r12->start;
		if (!inCompressor->fFreeze
		&&  level > r12->level)
			ExtendAChild(r12, lr, srcUsed, level);
		break;

	case 6:
		*outCopyLen = r12->parent->level;
		*outOffset = srcUsed - r12->parent->start;
		if (!inCompressor->fFreeze)
			AddressANode(*inSrcPtr, inThisNode, r12, lr, srcUsed, level, inCompressor);
		break;

	case 7:
		*outCopyLen = r12->parent->level;
		*outOffset = srcUsed - r12->parent->start;
		break;

	case 8:
		*outCopyLen = r12->parent->level + len + 1;
		*outOffset = srcUsed - r12->start;
		break;

	default:
		printf("What is going on?? %ld\n", srcUsed);
	}
}

void
AddFirstChild(unsigned char index, TTNode * inNode, TTNode * inChild, long inStart, unsigned long inLength, CLZCompressor * inCompressor)
{
	// set up parent <-> child pointers
	inNode->child = inChild;
	inChild->parent = inNode;
	inChild->sibling = NULL;
	inChild->child = NULL;
	inChild->level = inLength;
	inChild->length = inLength;
	inChild->start = inStart;
	inCompressor->fNode[index] = inChild;
}

void
ExtendAChild(TTNode * inNode, TTNode * inNode2, long inArg3, long inArg4)
{ }

void
UpdateANode(TTNode * inNode)
{
	TTNode *	n;
	for (n = inNode->parent; n != NULL && n->start <= inNode->start; n = n->parent)
		n->start = inNode->start;
}

void
AddASibling(TTNode * inNode, TTNode * inSibling, long inStart, long inLevel)
{
	TTNode * parent = inNode->parent;
	if (inLevel > parent->level)
	{
		inNode->sibling = inSibling;
		inSibling->parent = parent;
		inSibling->sibling = NULL;
		inSibling->child = NULL;
		inSibling->level = inLevel;
		inSibling->length = inLevel - parent->level;
		inSibling->start = inStart;
		UpdateANode(inSibling);
	}
}

void
AddressANode(unsigned char index, TTNode * inParent, TTNode * inNode2, TTNode * inNode, long inStart, long inLevel, CLZCompressor * inCompressor)
{
	TTNode * parent = inNode2->parent;
	if (inLevel > parent->level)
	{
		inCompressor->fNode[index] = inNode;
		inNode->parent = inParent;
		inNode->sibling = NULL;
		inNode->child = NULL;
		inNode->level = inLevel;
		inNode->length = inLevel - inParent->level;
		inNode->start = inStart;
		UpdateANode(inNode);
	}
}

void
InsertANode(unsigned char index, TTNode * inNode1, TTNode * inNode2, TTNode * inNode3, long inStart, long inLength, long inLevel, CLZCompressor * inCompressor)
{
/*	r4 = inNode1;
	r5 = inNode2;
	r6 = inNode3;
	r0 = index;
	r8 = inLevel;
	r9 = inCompressor;
	r10 = inLength;
*/
	if (inLevel > inNode1->parent->level)
	{
		TTNode *	newNode = inCompressor->talloc();
		TTNode *	r0 = inNode1->parent;
		TTNode *	r1 = r0->child;
		if (r0 == inNode3)
		{
			if (r1 == inNode1)
			{
				r0->child = newNode;
				newNode->parent = r0;	// redundant, surely?
			}
			inCompressor->fNode[index] = newNode;
			newNode->parent = inNode3;
		}
		else if (r1 == inNode1)
		{
			r0->child = newNode;
			newNode->parent = r0;
			newNode->sibling = inNode1->sibling;
		}
		else
		{
			while (r1->sibling != inNode1)
				r1 = r1->sibling;
			r1->sibling = newNode;
			newNode->parent = r1->parent;
			newNode->sibling = inNode1->sibling;
		}

		newNode->child = inNode1;
		newNode->level = newNode->parent->level + inLength;
		newNode->length = inLength;
		newNode->start = inStart;

		inNode2->parent = newNode;
		inNode2->sibling = NULL;
		inNode2->child = NULL;
		inNode2->level = inLevel;
		inNode2->length = inLevel - newNode->level;
		inNode2->start = inStart;

		inNode1->parent = newNode;
		inNode1->sibling = inNode2;
		inNode1->length -= inLength;

		UpdateANode(inNode2);
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	C L Z C o m p r e s s o r
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CLZCompressor implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CLZCompressor::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN13CLZCompressor6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN13CLZCompressor4makeEv - 0b	\n"
"		.long		__ZN13CLZCompressor7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CLZCompressor\"	\n"
"2:	.asciz	\"CCompressor\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN13CLZCompressor9classInfoEv - 4b	\n"
"		.long		__ZN13CLZCompressor4makeEv - 4b	\n"
"		.long		__ZN13CLZCompressor7destroyEv - 4b	\n"
"		.long		__ZN13CLZCompressor4initEPv - 4b	\n"
"		.long		__ZN13CLZCompressor8compressEPmPvmS1_m - 4b	\n"
"		.long		__ZN13CLZCompressor23estimatedCompressedSizeEPvm - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CLZCompressor)

CLZCompressor *
CLZCompressor::make(void)
{
	fMemFlag = true;
	fMaxNode = 512;
	fAvailNode = (TTNode *)NewPtr(512*sizeof(TTNode));
	if (fAvailNode == NULL)
		fMemFlag = false;
	fBitStack = new CPushPopper;
	return this;
}


void
CLZCompressor::destroy(void)
{
	FreePtr((Ptr)fAvailNode);
	if (fBitStack)
		delete fBitStack;
}


NewtonErr
CLZCompressor::init(void * inContext)
{
	return noErr;
}


/*------------------------------------------------------------------------------
	Compress LZ encoded data into a buffer.
	Args:		outSize		returns actual size of decompressed data
				inDstBuf		LZ encoded data
				inDstLen		size of that buffer
				inSrcBuf		data to be encoded
				inSrcLen		size of that data
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CLZCompressor::compress(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen)
{
	NewtonErr	err;
	if ((err = init(NULL)) == noErr)
	{
		err = compressChunk(outSize, inDstBuf, inDstLen, inSrcBuf, inSrcLen);
		finish(NULL, 0);
	}
	return err;
}


size_t
CLZCompressor::estimatedCompressedSize(void * inSrcBuf, size_t inSrcLen)
{
	return inSrcLen + headerSize();
}


NewtonErr
CLZCompressor::setHeader(void * inBuf, size_t inBufLen)
{
	if (inBufLen < 8)
		return kOSErrBadParameters;
	*(unsigned long *)inBuf = CANONICAL_LONG(0x80000000);	// flags?
	*((unsigned long *)inBuf+1) = CANONICAL_LONG(0);		// size?
	return noErr;
}


size_t
CLZCompressor::headerSize(void)
{
	return 8;
}


NewtonErr
CLZCompressor::compressChunk(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen)
{
	unsigned char *	srcPtr = (unsigned char *)inSrcBuf;
	unsigned char *	dstPtr = (unsigned char *)inDstBuf;
	size_t	blockSize = kSubPageSize;
	size_t	totalSize = 0;
	size_t	doneSize;
	size_t	srcUsed = 0;
	size_t	srcRemaining;
	bool		isFirstBlock = true;
	bool		isLastBlock = false;

	if (!fMemFlag)
		return kOSErrNoMemory;

	while (!isLastBlock)
	{
		fNumNodes = 0;
		if ((srcRemaining = inSrcLen - srcUsed) <= blockSize)
		{
			isLastBlock = true;
			blockSize = srcRemaining;
		}
		if (isFirstBlock)
		{
			isFirstBlock = false;
			if (blockSize > 0)
				compressBlock(&doneSize, dstPtr + sizeof(size_t), inDstLen, srcPtr, blockSize);
			else
				doneSize = 0;
			doneSize += sizeof(size_t);
		}
		else 
			compressBlock(&doneSize, dstPtr, inDstLen, srcPtr, blockSize);
		srcPtr += blockSize;
		dstPtr += doneSize;
		totalSize += doneSize;
		srcUsed += blockSize;
	}
	*outSize = totalSize;
	*(size_t*)inDstBuf = CANONICAL_LONG(totalSize);
	return noErr;
}


NewtonErr
CLZCompressor::compressBlock(size_t * outSize, void * inDstBuf, size_t inDstLen, void * inSrcBuf, size_t inSrcLen)
{
	unsigned char *	srcBuf = (unsigned char *)inSrcBuf;	// r10
	unsigned char *	srcPtr = (unsigned char *)inSrcBuf;	// r7
	size_t	copyLen;				// sp04
	long		offset;				// sp00
	size_t	literalLen = 0;	// r5
	size_t	length = 0;			// r6

	fFreeze = false;
	for (ArrayIndex i = 0; i < 256; ++i)
		fNode[i] = NULL;

	fEncodeCase = 10;
	fNodeCount = 0;
	TTNode * node = talloc();
/*	original does:
	if (node == NULL)
		printf("Cannot allocate memory for root!!!");
	node->start = 0;
	node->level = 0;
	node->length = 0;
	node->parent = NULL;
	node->child = NULL;
	node->sibling = NULL;
*/
	fBitStack->setWriteBuffer((UChar *)inDstBuf, 2*kSubPageSize);	// ignores inDstLen!
	fBinaryFlag = true;
	fBitStack->pushBits(8, 0);
	fBitStack->pushBits(8, fBinaryFlag);
	fBitStack->pushBits(16, 0);

	while (length + literalLen < inSrcLen)
	{
		if (fBitStack->getByteCount() > inSrcLen)
			goto badKarma;

		TreeSearch(srcBuf, srcPtr, inSrcLen, &copyLen, &offset, node, fAvailNode, 1, this);
		if (copyLen + length + literalLen > inSrcLen)
			copyLen = inSrcLen - length - literalLen;
		if (copyLen >= 3)
		{
			codeword_gen_bin(copyLen, offset, literalLen, srcPtr, length);
			length += copyLen + literalLen;
			srcPtr += copyLen;
			literalLen = 0;
			copyLen = 0;
		}
		else
		{
			srcPtr++;
			literalLen++;
			copyLen = 0;
			if (literalLen >= 63)
			{
				codeword_gen_bin(0, offset, literalLen, srcPtr, length);
				length += 63;
				literalLen -= 63;
			}
		}
	}

	if (literalLen > 0)
		codeword_gen_bin(0, offset, literalLen, srcPtr, length);
	fBitStack->flushBits();
	*outSize = fBitStack->getByteCount();
	if (fBitStack->getByteCount() <= inSrcLen)
		return noErr;

badKarma:
	// compression was worse than using the original data!
	memmove((unsigned char*)inDstBuf + sizeof(size_t), srcBuf, inSrcLen);
	*(unsigned char*)inDstBuf = 1;
	*outSize = inSrcLen + sizeof(size_t);
	return -1;
}


void
CLZCompressor::finish(void * inBuf, size_t inBufLen)
{
	if (inBuf != NULL
	&&  inBufLen != 0)
		setHeader(inBuf, inBufLen);
}


void
CLZCompressor::codeword_gen_bin(size_t inCopyLen, long inOffset, size_t inLiteralLen, unsigned char * inBits, size_t inLen)
{
/*	original does:
	if (inCopyLen >= 2045)
		printf("copy length exceed 2045");
*/
	fLitFlag = (inLiteralLen == 0 || inLiteralLen >= 63);
	if (inLiteralLen > 0)
	{
		fBitStack->pushBits(2, 0);
		encode_lit_len_bin(inLiteralLen);
		ArrayIndex count = MIN(inLiteralLen, 63);
		inLen += count;
		inBits -= inLiteralLen;
		for (ArrayIndex i = 0; i < count; ++i)
			fBitStack->pushBits(8, inBits[i]);
	}
	if (inCopyLen >= 3)
	{
		if (!fLitFlag)
			inCopyLen--;
		fLitFlag = true;
		encode_copy_length_bin_huff4(inCopyLen - 2);
		encode_offset_bin(inOffset, inLen);
	}
}


void
CLZCompressor::encode_lit_len_bin(long inLen)
{
	if (inLen == 1)
		fBitStack->pushBits(1, 0);
	else
	{
		for (ArrayIndex i = 0; i < 5; ++i)
		{
			if (inLen <= LL[i])
			{
				fBitStack->pushBits(LLB[i], LLBase[i] + inLen);
				return;
			}
		}
		if (inLen >= 63)
			fBitStack->pushBits(10, 1023);
	}
}


void
CLZCompressor::encode_copy_length_bin_huff4(long inLen)
{
	if (inLen == 0)
		fBitStack->pushBits(2, 0);
	else if (inLen == 1)
		fBitStack->pushBits(2, 1);
	else if (inLen == 2)
		fBitStack->pushBits(3, 4);
	else if (inLen == 5)
		fBitStack->pushBits(4, 12);
	else
	{
		for (ArrayIndex i = 0; i < 6; ++i)
		{
			if (inLen <= CL[i])
			{
				fBitStack->pushBits(CLB[i], CLBase[i] + inLen);
				return;
			}
		}
	}
}


void
CLZCompressor::encode_offset_bin(long inParam, unsigned long b)
{
	switch (fEncodeCase)
	{
	case 10:
		if (inParam < 0x0015)
		{
			encode_offset_case10_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case10_bin(0x0015, b, fBitStack);
		fEncodeCase = 9;
	case 9:
		if (inParam < 0x002A)
		{
			encode_offset_case9_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case9_bin(0x002A, b, fBitStack);
		fEncodeCase = 8;
	case 8:
		if (inParam < 0x0054)
		{
			encode_offset_case8_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case8_bin(0x0054, b, fBitStack);
		fEncodeCase = 7;
	case 7:
		if (inParam < 0x00A8)
		{
			encode_offset_case7_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case7_bin(0x00A8, b, fBitStack);
		fEncodeCase = 6;
	case 6:
		if (inParam < 0x0150)
		{
			encode_offset_case6_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case6_bin(0x0150, b, fBitStack);
		fEncodeCase = 5;
	case 5:
		if (inParam < 0x02A0)
		{
			encode_offset_case5_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case5_bin(0x02A0, b, fBitStack);
		fEncodeCase = 4;
	case 4:
		if (inParam < 0x0540)
		{
			encode_offset_case4_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case4_bin(0x0540, b, fBitStack);
		fEncodeCase = 3;
	case 3:
		if (inParam < 0x0A80)
		{
			encode_offset_case3_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case3_bin(0x0A80, b, fBitStack);
		fEncodeCase = 2;
	case 2:
		if (inParam < 0x1500)
		{
			encode_offset_case2_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case2_bin(0x1500, b, fBitStack);
		fEncodeCase = 1;
	case 1:
		if (inParam < 0x2A00)
		{
			encode_offset_case1_bin(inParam, b, fBitStack); break;
		}
		encode_offset_case1_bin(0x2A00, b, fBitStack);
	}
}


TTNode *
CLZCompressor::talloc(void)
{
	TTNode * node;
	if (fNumNodes == fMaxNode - 1)
		fFreeze = true;
	node = &fAvailNode[fNumNodes++];	// no fNumNodes overflow chcking here!
	node->start = 0;
	node->level = 0;
	node->length = 0;
	node->parent = NULL;
	node->child = NULL;
	node->sibling = NULL;
	fNodeCount++;
	return node;
}
