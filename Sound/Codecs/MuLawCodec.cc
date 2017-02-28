/*
	File:		MuLawCodec.cc

	Contains:	MuLaw codec implementation.

	Written by:	Newton Research Group, 2015.
	With help from: http://csweb.cs.wfu.edu/~burg/CCLI/Tutorials/C/Chapter5/Mu_Law_Encoding_in_C++.pdf
*/

#include "MuLawCodec.h"
#include "SoundErrors.h"


/*------------------------------------------------------------------------------
	M u L a w C o d e c
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CMuLawCodec implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CMuLawCodec::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN11CMuLawCodec6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN11CMuLawCodec4makeEv - 0b	\n"
"		.long		__ZN11CMuLawCodec7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CMuLawCodec\"	\n"
"2:	.asciz	\"CSoundCodec\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN11CMuLawCodec9classInfoEv - 4b	\n"
"		.long		__ZN11CMuLawCodec4makeEv - 4b	\n"
"		.long		__ZN11CMuLawCodec7destroyEv - 4b	\n"
"		.long		__ZN11CMuLawCodec4initEP10CodecBlock - 4b	\n"
"		.long		__ZN11CMuLawCodec5resetEP10CodecBlock - 4b	\n"
"		.long		__ZN11CMuLawCodec7produceEPvPmS1_P10CodecBlock - 4b	\n"
"		.long		__ZN11CMuLawCodec7consumeEPKvPmS2_PK10CodecBlock - 4b	\n"
"		.long		__ZN11CMuLawCodec5startEv - 4b	\n"
"		.long		__ZN11CMuLawCodec4stopEi - 4b	\n"
"		.long		__ZN11CMuLawCodec15bufferCompletedEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CMuLawCodec)

CMuLawCodec *
CMuLawCodec::make(void)
{
	fComprBuffer = NULL;
	fComprBufLength = 0;
	fComprBufOffset = 0;
	return this;
}


void
CMuLawCodec::destroy(void)
{ }


NewtonErr
CMuLawCodec::init(CodecBlock * inParms)
{
	return noErr;
}


NewtonErr
CMuLawCodec::reset(CodecBlock * inParms)
{
	fComprBuffer = (UChar *)inParms->data;
	fComprType = inParms->comprType;
	fSampleRate = inParms->sampleRate;
	fDataType = inParms->dataType;
	fComprBufLength = inParms->dataSize;
	fComprBufOffset = 0;
	return noErr;
}


// decode block of muLaw data to 16-bit linear samples
NewtonErr
CMuLawCodec::produce(void * outBuf, size_t * ioBufSize, size_t * outComprSize, CodecBlock * ioParms)
{
	ArrayIndex sampleSizeInBytes = fDataType / 8;
	ArrayIndex numOfSamples = *ioBufSize / sampleSizeInBytes;

	*ioBufSize = 0;
	*outComprSize = 0;
		
	if (fComprBuffer == NULL)
		return kSndErrNoSamples;

	UChar * srcPtr = fComprBuffer + fComprBufOffset;
	short * dstPtr = (short *)outBuf;
	ArrayIndex available = fComprBufLength - fComprBufOffset;
	if (numOfSamples > available)
		numOfSamples = available;
	if (numOfSamples > 0)
	{
		blockConvertMuLawToLin16(dstPtr, srcPtr, numOfSamples);
		fComprBufOffset += numOfSamples;
		*ioBufSize = numOfSamples * sampleSizeInBytes;
		*outComprSize = numOfSamples;
	}
	ioParms->dataType = k16Bit;
	ioParms->comprType = kSampleLinear;
	ioParms->sampleRate = fSampleRate;
	return noErr;
}

void
CMuLawCodec::blockConvertMuLawToLin16(void * inDst, const void * inSrc, size_t inLen)
{
	const char * srcPtr = (const char *)inSrc;
	short * dstPtr = (short *)inDst;
	for (ArrayIndex i = 0; i < inLen; ++i)
	{
		// convert 8-bit pseudo-float to 16-bit linear sample
		short ulawValue = ~*srcPtr++;
		// Save the exponent region (the three bits to the right of the sign bit) as a 3- bit field with value e.
		short e = (ulawValue >> 4) & 0x07;
		// Save the mantissa region (the remaining four bits) as a four-bit field called m, which we view as a binary field.
		short m = ulawValue & 0x0F;
		//		a = 3 + e
		//		b = (1000 0100 left shifted by e bits) – 1000 0100 decoded sample = (m left shifted by a bits) + b
		short x = (m << 1) | 0x21;
		short sample = (x << e) - 0x21;
		// Affix the sign bit as the leftmost bit.
		if (ulawValue & 0x80)
			sample = -sample;
		*dstPtr++ = sample << 2;
	}

}


// encode block of 16-bit linear samples to muLaw data
NewtonErr
CMuLawCodec::consume(const void * inBuf, size_t * ioBufSize, size_t * outComprSize, const CodecBlock * inParms)
{
	ArrayIndex sampleSizeInBytes = fDataType / 8;
	ArrayIndex numOfSamples = *ioBufSize / sampleSizeInBytes;

	*ioBufSize = 0;
	*outComprSize = 0;
		
	if (fComprBuffer == NULL)
		return kSndErrNoSamples;

	short * srcPtr = (short *)inBuf;
	UChar * dstPtr = fComprBuffer + fComprBufOffset;
	ArrayIndex available = fComprBufLength - fComprBufOffset;
	if (numOfSamples > available)
		numOfSamples = available;
	if (numOfSamples > 0)
	{
		blockConvertMuLawToLin16(dstPtr, srcPtr, numOfSamples);
		fComprBufOffset += numOfSamples;
		*ioBufSize = numOfSamples * sampleSizeInBytes;
		*outComprSize = numOfSamples;
	}
	return noErr;
}

void
CMuLawCodec::blockConvertLin16ToMuLaw(void * inDst, const void * inSrc, size_t inLen)
{
	const short * srcPtr = (const short *)inSrc;
	char * dstPtr = (char *)inDst;
	for (ArrayIndex i = 0; i < inLen; ++i)
	{
		// convert 16-bit sample to 8-bit pseudo-float
		int sample = *srcPtr++ >> 2;
		// extract the sign bit
		char signBit = 0;
		if (sample < 0)
		{
			sample = -sample;
			signBit = 0x80;
		}
		// bias the sample
		sample += 0x21;
		// extract the exponent region: top 8 bits after the sign
		int exponent = sample >> 5;
		// find leftmost bit p in exponent region
		int p;
		for (p = 7; p >= 0; --p)
			if ((exponent & (1 << p)) != 0)
				break;
		// mantissa is 4 bits to the right of p
		int mantissa = ((sample >> 1) >> p) & 0x0F;
		// assemble the muLaw value: sign[1 bit] exponent[3 bits] mantissa[4 bits]
		*dstPtr++ = ~(signBit | (p << 4) | mantissa);
	}
}


void
CMuLawCodec::start(void)
{ }


void
CMuLawCodec::stop(int)
{ }


bool
CMuLawCodec::bufferCompleted(void)
{
	return fComprBufOffset == fComprBufLength;
}
