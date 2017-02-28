/*
	File:		IMACodec.cc

	Contains:	IMA codec implementation.

	Written by:	Newton Research Group, 2015.
	With help from: http://www.cs.columbia.edu/~hgs/audio/dvi/p31.jpg
*/

#include "IMACodec.h"
#include "SoundErrors.h"


/*------------------------------------------------------------------------------
	I M A C o d e c
------------------------------------------------------------------------------*/

int kIMAIndexTable[16] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

int kIMAStepTable[89] = {
	7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
	19, 21, 23, 25, 28, 31, 34, 37, 41, 45, 
	50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 
	130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
	337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
	876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 
	2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
	5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 
	15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767 
};


void
CheckState(char * inPtr, IMAState * ioState)
{
	short blockState = *(short *)inPtr;
	int index = blockState & 0x7F;
	int predictor = blockState & ~0x7F;
	if (index != ioState->index  ||  predictor != (ioState->predictor & ~0x7F))
	{
		ioState->predictor = predictor;
		ioState->index = index;
	}
}

void
CompressIMA(char * inSrc, char * inDst, ArrayIndex inLen, IMAState * ioState, ArrayIndex inNumOfChannels, ArrayIndex inNumOfBlocks)
{
	ArrayIndex numOfBlocks = inLen / 64;
	if (numOfBlocks > 0)
	{
		int blockStep = 0;
		if (inNumOfChannels == 2)
		{
			--inNumOfBlocks;
			inSrc += inNumOfBlocks * 2;
			inDst += inNumOfBlocks * 34;
			blockStep = 34;
		}

		int predictedSample = ioState->predictor;
		int index = ioState->index;

		int stepSize = kIMAStepTable[index];

		int oddSample = 0;

		// encode a number of blocks
		for (ArrayIndex count = numOfBlocks; count > 0; inSrc += blockStep, --count)
		{
			// store block encoding state
			*(short *)inDst = (predictedSample & ~0x7F) | index;
			inDst += 2;

			// encode a block of 64 4-bit IMA samples
			for (ArrayIndex i = 64; i > 0; --i)
			{
				// read 16-bit source sample
				int difference = *(short *)inSrc - predictedSample;
				inSrc += inNumOfChannels * sizeof(short);
				int newSample;	// r2
				if (difference >= 0) {
					newSample = 0;
				} else {
					newSample = 8;
					difference = -difference;
				}

				int mask = 4;
				int tempStepSize = stepSize;
				for (ArrayIndex i = 0; i < 3; ++i)
				{
					if (difference >= tempStepSize) {
						newSample |= mask;
						difference -= tempStepSize;
					}
					tempStepSize >>= 1;
					mask >>= 1;
				}
				
				// store 4-bit sample
				if (i & 1) {
					*inDst++ = (newSample << 4) | oddSample;
				} else {
					oddSample = newSample & 0x0F;
				}

				// calculate difference = (newSample + 0.5) * stepSize/4 === newSample * stepSize/4 + stepSize/8
				// by repetitive addition
				difference = 0;	// r12
				if (newSample & 4)
					difference = stepSize;	// addition would be redundant
				if (newSample & 2)
					difference += stepSize >> 1;
				if (newSample & 1)
					difference += stepSize >> 2;
				difference += stepSize >> 3;
				if (newSample & 8)
					difference = -difference;

				predictedSample += difference;

				if (predictedSample > 32767) predictedSample = 32767;
				else if (predictedSample < -32768) predictedSample = -32768;

				// compute new step size
				index += kIMAIndexTable[newSample];
				if (index < 0) index = 0;
				else if (index > 88) index = 88;

				stepSize = kIMAStepTable[index];
			}
		}
		// update state for next call
		ioState->predictor = predictedSample;
		ioState->index = index;
	}
}


void
ExpandIMA(char * inSrc, char * inDst, IMAState * ioState, ArrayIndex inLen, ArrayIndex inNumOfBlocks, ArrayIndex inSampleSize)
{
	if (inLen > 0)
	{
		char * dst8Ptr = inDst;					// 8-bit output sample pointer
		short * dst16Ptr = (short *)inDst;	// useful 16-bit output sample pointer

		int blockStep = 2;
		if (inSampleSize & 1)
		{
			--inNumOfBlocks;
			inSrc += 34;
			dst8Ptr += inNumOfBlocks;
			dst16Ptr += inNumOfBlocks;
			blockStep = 2 + 34;
		}

		// first 16-bit word of the source block is encoding state
		CheckState(inSrc, ioState);
		inSrc += 2;

		int newSample = ioState->predictor;	// r0
		int index = ioState->index;		// r1

		int stepSize = kIMAStepTable[index];

		int sourceByte = 0;

		// decode a number of blocks
		for (ArrayIndex count = inNumOfBlocks; count > 0; inSrc += blockStep, --count)
		{
			// decode a block of 64 4-bit IMA samples
			for (ArrayIndex i = 64; i > 0; --i)
			{
				// read 4-bit original sample
				int originalSample;
				if (i & 1) {
					originalSample = (sourceByte >> 4) & 0x0F;
				} else {
					sourceByte = *inSrc++;
					originalSample = sourceByte & 0x0F;
				}

				// calculate differrence = (originalSample + 0.5) * stepSize/4 === originalSample * stepSize/4 + stepSize/8
				// by repetitive addition
				int difference = 0;	// r12
				if (originalSample & 4)
					difference = stepSize;	// addition would be redundant
				if (originalSample & 2)
					difference += stepSize >> 1;
				if (originalSample & 1)
					difference += stepSize >> 2;
				difference += stepSize >> 3;
				if (originalSample & 8)
					difference = -difference;

				newSample += difference;

				if (newSample > 32767) newSample = 32767;
				else if (newSample < -32768) newSample = -32768;

				// store the output sample
				if (inSampleSize == 0) {
					*dst8Ptr++ = (newSample >> 8) ^ 0x80;
				} else if (inSampleSize == 1) {
					*dst8Ptr = (newSample >> 8) ^ 0x80;
					dst8Ptr += 2;
				} else if (inSampleSize == 2) {	// 16-bit						<-- ENDIANNESS
					*dst16Ptr++ = newSample;
				} else if (inSampleSize == 3) {	// 32-bit
					*dst16Ptr++ = newSample;				// store in hi-word
					dst16Ptr++;
				}

				index += kIMAIndexTable[originalSample];
				if (index < 0) index = 0;
				else if (index > 88) index = 88;

				stepSize = kIMAStepTable[index];
			}
		}
		// update state for next call
		ioState->predictor = newSample;
		ioState->index = index;
	}
}


/* ----------------------------------------------------------------
	CIMACodec implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CIMACodec::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN9CIMACodec6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN9CIMACodec4makeEv - 0b	\n"
"		.long		__ZN9CIMACodec7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CIMACodec\"	\n"
"2:	.asciz	\"CSoundCodec\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN9CIMACodec9classInfoEv - 4b	\n"
"		.long		__ZN9CIMACodec4makeEv - 4b	\n"
"		.long		__ZN9CIMACodec7destroyEv - 4b	\n"
"		.long		__ZN9CIMACodec4initEP10CodecBlock - 4b	\n"
"		.long		__ZN9CIMACodec5resetEP10CodecBlock - 4b	\n"
"		.long		__ZN9CIMACodec7produceEPvPmS1_P10CodecBlock - 4b	\n"
"		.long		__ZN9CIMACodec7consumeEPKvPmS2_PK10CodecBlock - 4b	\n"
"		.long		__ZN9CIMACodec5startEv - 4b	\n"
"		.long		__ZN9CIMACodec4stopEi - 4b	\n"
"		.long		__ZN9CIMACodec15bufferCompletedEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CIMACodec)

CIMACodec *
CIMACodec::make(void)
{
	fState.predictor = 0;
	fState.index = 0;
	fComprBuffer = NULL;
	fComprBufLength = 0;	// not original
	fComprBufOffset = 0;
	f30 = 0;
	f34 = 0x0A00;
	f38 = 3;
	return this;
}


void
CIMACodec::destroy(void)
{ }


NewtonErr
CIMACodec::init(CodecBlock * inParms)
{
	return noErr;
}


NewtonErr
CIMACodec::reset(CodecBlock * inParms)
{
	fState.predictor = 0;
	fState.index = 0;
	fComprBuffer = (char *)inParms->data;
	fComprType = inParms->comprType;
	fSampleRate = inParms->sampleRate;
	fDataType = inParms->dataType;
	fComprBufLength = inParms->dataSize;
	fComprBufOffset = 0;
	return noErr;
}


// decode 34-byte blocks of IMA 4-bit data to 64-byte blocks of 8/16-bit linear samples
NewtonErr
CIMACodec::produce(void * outBuf, size_t * ioBufSize, size_t * outComprSize, CodecBlock * ioParms)
{
	if (fComprBuffer == NULL)
		return kSndErrNoSamples;

	char * srcPtr = fComprBuffer + fComprBufOffset;
	char * dstPtr = (char *)outBuf;
	ArrayIndex sampleSizeInBytes = fDataType / 8;
	ArrayIndex numOfBlocks = *ioBufSize / (sampleSizeInBytes * 64);
	ArrayIndex available = (fComprBufLength - fComprBufOffset) / 34;

	if (numOfBlocks > available)
		numOfBlocks = available;
	if (numOfBlocks > 0)	// not original
	{
		ExpandIMA(srcPtr, dstPtr, &fState, numOfBlocks, 1, sampleSizeInBytes);
		fComprBufOffset += numOfBlocks * 34;
		*ioBufSize = numOfBlocks * 64 * sampleSizeInBytes;
		*outComprSize = numOfBlocks * 34;
	}
	ioParms->dataType = fDataType;
	ioParms->comprType = (fDataType == k8Bit) ? kSampleStandard : kSampleLinear;
	ioParms->sampleRate = fSampleRate;
	return noErr;
}


// encode 64-byte blocks of 8/16-bit linear samples to 34-byte blocks of 4-bit IMA data
NewtonErr
CIMACodec::consume(const void * inBuf, size_t * ioBufSize, size_t * outComprSize, const CodecBlock * inParms)
{
	if (fComprBuffer == NULL)
		return kSndErrNoSamples;

	char * srcPtr = (char *)inBuf;
	char * dstPtr = fComprBuffer + fComprBufOffset;
	ArrayIndex numOfBlocks = (*ioBufSize / 2) / 64;		// two samples per byte
	ArrayIndex available = (fComprBufLength - fComprBufOffset) / 34;
	if (numOfBlocks > available)
		numOfBlocks = available;
	if (numOfBlocks > 0)	// not original
	{
		CompressIMA(srcPtr, dstPtr, numOfBlocks*64, &fState, 1, 1);
		fComprBufOffset += (numOfBlocks * 34);
		*ioBufSize = numOfBlocks * 2 * 64;
		*outComprSize = numOfBlocks * 34;
	}
	return noErr;
}


void
CIMACodec::start(void)
{ }


void
CIMACodec::stop(int)
{ }


bool
CIMACodec::bufferCompleted(void)
{
	return fComprBufOffset == fComprBufLength;
}
