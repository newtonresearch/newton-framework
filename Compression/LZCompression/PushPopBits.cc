/*
	File:		PushPopBits.cc

	Contains:	Bit twiddling class for LZ compression classes.
	
	Written by:	Newton Research Group, 2006.
					With a little help from Philz, and a disassembly of the DILs.
*/

#include "Newton.h"
#include "PushPopBits.h"


CPushPopper::CPushPopper(void)
{ }


CPushPopper::~CPushPopper(void)
{ }


void
CPushPopper::setReadBuffer(unsigned char * inBuf, size_t inLen)
{
	fBytePtr = inBuf;
	fMaxBytes = inLen;
	fSource = 0;
	fByteCount = 0;
	fBitIndex = 0;
}


void
CPushPopper::setWriteBuffer(unsigned char * inBuf, size_t inLen)
{
	fBytePtr = inBuf;
	fMaxBytes = inLen;
	fSource = 0;
	fByteCount = 0;
	fBitIndex = 32;
}


void
CPushPopper::restoreBits(int inCount)
{
	fBitIndex += inCount;
}


uint32_t
CPushPopper::popBits(int inCount)
{
	for ( ; fBitIndex < 23; fByteCount++, fBitIndex += 8)
	{
		fSource = (fSource << 8) | *fBytePtr++;
	}
	fBitIndex -= inCount;
	return (fSource >> fBitIndex) & (0xFFFFFFFF >> (32 - inCount));
}


uint32_t
CPushPopper::popFewBits(int inCount)
{
	fBitIndex -= inCount;
	if (fBitIndex < 0)
	{
		fSource = (fSource << 8) | *fBytePtr++;
		fByteCount++;
		fBitIndex += 8;
	}
	return (fSource >> fBitIndex) & (0xFFFFFFFF >> (32 - inCount));
}


void
CPushPopper::popString(unsigned char * outBuf, int inCount)
{
	uint32_t bits = fSource;
	int bitIndex = fBitIndex;
	while (bitIndex >= 8 && inCount-- > 0)
	{
		bitIndex -= 8;
		*outBuf++ = bits >> bitIndex;
	}
	if (inCount > 0)
	{
		fByteCount += inCount;
		while (inCount-- > 0)
		{
			bits = (bits << 8) | *fBytePtr++;
			*outBuf++ = bits >> bitIndex;
		}
		fSource = bits;
	}
	fBitIndex = bitIndex;
}


void
CPushPopper::pushBits(int inCount, uint32_t inValue)
{
	if (inCount < 0
	||  inCount > 20
	||  inValue >= (1 << inCount))
		printf("NOTE: CPushPopper::pushBits(%d, %d) argument out of range\n", inCount, inValue);

	while (fBitIndex < 23)
	{
		*fBytePtr++ = fSource >> 24;
		fSource <<= 8;
		fByteCount++;
		fBitIndex += 8;
		if (fByteCount > fMaxBytes)
			printf("NOTE: bytecount = %ld overflow in CPushPopper::pushBits()\n", fByteCount);
	}
	fBitIndex -= inCount;
	fSource |= (inValue << fBitIndex);
}


void
CPushPopper::flushBits(void)
{
	while (fBitIndex < 32)
	{
		*fBytePtr++ = fSource >> 24;
		fSource <<= 8;
		fByteCount++;
		fBitIndex += 8;
		if (fByteCount > fMaxBytes)
			printf("NOTE: bytecount = %ld overflow in CPushPopper::flushBits()\n", fByteCount);
	}
}
