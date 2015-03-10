/*
	File:		GSMCodec.cc

	Contains:	GSM codec implementation.

	Written by:	Newton Research Group, 2007.
*/

#include "GSMCodec.h"


/*------------------------------------------------------------------------------
	G S M C o d e c
	Requires the open source gsmlib.
------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------
	CGSMCodec implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CGSMCodec::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN9CGSMCodec6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN9CGSMCodec4makeEv - 0b	\n"
"		.long		__ZN9CGSMCodec7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CGSMCodec\"	\n"
"2:	.asciz	\"CSoundCodec\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN9CGSMCodec9classInfoEv - 4b	\n"
"		.long		__ZN9CGSMCodec4makeEv - 4b	\n"
"		.long		__ZN9CGSMCodec7destroyEv - 4b	\n"
"		.long		__ZN9CGSMCodec4initEP10CodecBlock - 4b	\n"
"		.long		__ZN9CGSMCodec5resetEP10CodecBlock - 4b	\n"
"		.long		__ZN9CGSMCodec7produceEPvPmS1_P10CodecBlock - 4b	\n"
"		.long		__ZN9CGSMCodec7consumeEPKvPmS2_PK10CodecBlock - 4b	\n"
"		.long		__ZN9CGSMCodec5startEv - 4b	\n"
"		.long		__ZN9CGSMCodec4stopEi - 4b	\n"
"		.long		__ZN9CGSMCodec15bufferCompletedEv - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CGSMCodec)

CGSMCodec *
CGSMCodec::make(void)
{
	fComprBuffer = NULL;
	fComprBufOffset = 0;
	return this;
}


void
CGSMCodec::destroy(void)
{
	if (fRefState == 'aloc')
		gsm_destroy(fGSMRef);
}


NewtonErr
CGSMCodec::init(CodecBlock * inParms)
{
	fGSMRef = gsm_create();
	fRefState = (fGSMRef == NULL) ? 'dead' : 'aloc';
	return noErr;
}


NewtonErr
CGSMCodec::reset(CodecBlock * inParms)
{
	fComprBuffer = inParms->x04;
	f24 = inParms->x10;
	f28 = inParms->x14;
	f2C = inParms->x0C;
	fComprBufLength = inParms->x08;
	fComprBufOffset = 0;
	return noErr;
}


// decode blocks of 33 bytes to 160 16-bit samples
NewtonErr
CGSMCodec::produce(void * outBuf, size_t * ioBufSize, size_t * outComprSize, CodecBlock * ioParms)
{
	NewtonErr	err;

	if (fComprBuffer == NULL)
		err = -30008;	// no samples
	else
	{
		UChar * srcPtr = fComprBuffer + fComprBufOffset;
		short * dstPtr = (short *)outBuf;
		int numOfBlocks = *ioBufSize / (160 * sizeof(short));
		int numOfBlocksAvail = (fComprBufLength - fComprBufOffset) / 33;
		if (numOfBlocks > numOfBlocksAvail)
			numOfBlocks = numOfBlocksAvail;
		for (ArrayIndex i = 0; i < numOfBlocks; ++i)
		{
			gsm_decode(fGSMRef, srcPtr, dstPtr);
			srcPtr += 33;
			dstPtr += 160;
		}
		fComprBufOffset += numOfBlocks * 33;
		*ioBufSize = numOfBlocks * (160 * sizeof(short));
		*outComprSize = numOfBlocks * 33;
		ioParms->x0C = f2C;
		ioParms->x10 = 6;
		ioParms->x14 = f28;
		err = noErr;
	}

	return err;
}


// encode blocks of 160 16-bit samples to 33 bytes
NewtonErr
CGSMCodec::consume(const void * inBuf, size_t * ioBufSize, size_t * outComprSize, const CodecBlock * inParms)
{
	NewtonErr	err;

	if (fComprBuffer == NULL)
		err = -30008;	// no samples
	else
	{
		short * srcPtr = (short *)inBuf;
		UChar * dstPtr = fComprBuffer + fComprBufOffset;
		int numOfBlocks = *ioBufSize / (160 * sizeof(short));
		int numOfBlocksAvail = (fComprBufLength - fComprBufOffset) / 33;
		if (numOfBlocks > numOfBlocksAvail)
			numOfBlocks = numOfBlocksAvail;
		for (ArrayIndex i = 0; i < numOfBlocks; ++i)
		{
			gsm_encode(fGSMRef, srcPtr, dstPtr);
			srcPtr += 160;
			dstPtr += 33;
		}
		fComprBufOffset += numOfBlocks * 33;
		*ioBufSize = numOfBlocks * (160 * sizeof(short));
		*outComprSize = numOfBlocks * 33;
		err = noErr;
	}

	return err;
}


void
CGSMCodec::start(void)
{ }


void
CGSMCodec::stop(int)
{ }


bool
CGSMCodec::bufferCompleted(void)
{
	return fComprBufOffset == fComprBufLength;
}
