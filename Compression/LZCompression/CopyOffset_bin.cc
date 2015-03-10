/*
	File:		CopyOffset_bin.cc

	Contains:	LZ compression support.
	
	Written by:	Newton Research Group, 2006.
					With a little help from Philz, and a disassembly of the DILs.
*/

#include "Newton.h"
#include "CopyOffset_bin.h"
#include "LZCompressionData-j.h"

#define C10A	0x0001
#define C10B	0x0005
#define C10C	0x0015
#define C10Bits	0

#define C9A	0x0002
#define C9B	0x000a
#define C9C	0x002a
#define C9Bits	1

#define C8A	0x0004
#define C8B	0x0014
#define C8C	0x0054
#define C8Bits	2

#define C7A	0x0008
#define C7B	0x0028
#define C7C	0x00a8
#define C7Bits	3

#define C6A	0x0010
#define C6B	0x0050
#define C6C	0x0150
#define C6Bits	4

#define C5A	0x0020
#define C5B	0x00a0
#define C5C	0x02a0
#define C5Bits	5

#define C4A	0x0040
#define C4B	0x0140
#define C4C	0x0540
#define C4Bits	6

#define C3A	0x0080
#define C3B	0x0280
#define C3C	0x0a80
#define C3Bits	7

#define C2A	0x0100
#define C2B	0x0500
#define C2C	0x1500
#define C2Bits	8

#define C1A	0x0200
#define C1B	0x0a00
#define C1C	0x2a00
#define C1Bits	9


#pragma mark Decoding

uint32_t
decode_offset_case10_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return 1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C10Bits+2)+C10A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O10)/sizeof(uint32_t); ++i)
		if (param <= O10[i-1])
			break;
	return inBitStack->popFewBits(i)+C10B+1;
}


uint32_t
decode_offset_case9_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C9Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C9Bits+2)+C9A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O9)/sizeof(uint32_t); ++i)
		if (param <= O9[i-1])
			break;
	return inBitStack->popFewBits(i)+C9B+1;
}

uint32_t
decode_offset_case8_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C8Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C8Bits+2)+C8A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O8)/sizeof(uint32_t); ++i)
		if (param <= O8[i-1])
			break;
	return inBitStack->popFewBits(i)+C8B+1;
}

uint32_t
decode_offset_case7_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C7Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C7Bits+2)+C7A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O7)/sizeof(uint32_t); ++i)
		if (param <= O7[i-1])
			break;
	return inBitStack->popFewBits(i)+C7B+1;
}

uint32_t
decode_offset_case6_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C6Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C6Bits+2)+C6A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O6)/sizeof(uint32_t); ++i)
		if (param <= O6[i-1])
			break;
	return inBitStack->popFewBits(i)+C6B+1;
}

uint32_t
decode_offset_case5_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C5Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C5Bits+2)+C5A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O5)/sizeof(uint32_t); ++i)
		if (param <= O5[i-1])
			break;	// original optimises return inBitStack->popFewBits(i)+C5B+1;
	return inBitStack->popBits(i)+C5B+1;
}

uint32_t
decode_offset_case4_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C4Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C4Bits+2)+C4A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O4)/sizeof(uint32_t); ++i)
		if (param <= O4[i-1])
			break;
	return inBitStack->popBits(i)+C4B+1;
}

uint32_t
decode_offset_case3_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C3Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C3Bits+2)+C3A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O3)/sizeof(uint32_t); ++i)
		if (param <= O3[i-1])
			break;
	return inBitStack->popBits(i)+C3B+1;
}

uint32_t
decode_offset_case2_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C2Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C2Bits+2)+C2A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O2)/sizeof(uint32_t); ++i)
		if (param <= O2[i-1])
			break;
	return inBitStack->popBits(i)+C2B+1;
}

uint32_t
decode_offset_case1_bin(int param, CPushPopper * inBitStack)
{
	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C1Bits)+1;

	if (inBitStack->popFewBits(1) == 0)
		return inBitStack->popFewBits(C1Bits+2)+C1A+1;

	ArrayIndex	i;
	for (i = 1; i <= sizeof(O1)/sizeof(uint32_t); ++i)
		if (param <= O1[i-1])
			break;
	return inBitStack->popBits(i)+C1B+1;
}

#pragma mark Encoding

void
encode_offset_case10_bin(int val, int param, CPushPopper * inBitStack)
{
	if (val == C10A)
		inBitStack->pushBits(C10Bits+1, 0);

	else if (val <= C10B)
		inBitStack->pushBits(C10Bits+4, val+C10B+0x001);

	else if (val <= C10C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2, 3);
		for (i = 1; i < C10Bits+4; ++i)
		{
			if (param <= O10[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C10B-1);
	}
}

void
encode_offset_case9_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C9A)
		inBitStack->pushBits(C9Bits+1, val-1);

	else if (val <= C9B)
		inBitStack->pushBits(C9Bits+4, val+C9B+0x003);

	else if (val <= C9C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C9Bits+4; ++i)
		{
			if (param <= O9[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C9B-1);
	}
}

void
encode_offset_case8_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C8A)
		inBitStack->pushBits(C8Bits+1, val-1);

	else if (val <= C8B)
		inBitStack->pushBits(C8Bits+4, val+C8B+0x007);

	else if (val <= C8C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C8Bits+4; ++i)
		{
			if (param <= O8[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C8B-1);
	}
}

void
encode_offset_case7_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C7A)
		inBitStack->pushBits(C7Bits+1, val-1);

	else if (val <= C7B)
		inBitStack->pushBits(C7Bits+4, val+C7B+0x00F);

	else if (val <= C7C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C7Bits+4; ++i)
		{
			if (param <= O7[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C7B-1);
	}
}

void
encode_offset_case6_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C6A)
		inBitStack->pushBits(C6Bits+1, val-1);

	else if (val <= C6B)
		inBitStack->pushBits(C6Bits+4, val+C6B+0x01F);

	else if (val <= C6C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C6Bits+4; ++i)
		{
			if (param <= O6[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C6B-1);
	}
}


void
encode_offset_case5_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C5A)
		inBitStack->pushBits(C5Bits+1, val-1);

	else if (val <= C5B)
		inBitStack->pushBits(C5Bits+4, val+C5B+0x03F);

	else if (val <= C5C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C5Bits+4; ++i)
		{
			if (param <= O5[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C5B-1);
	}
}

void
encode_offset_case4_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C4A)
		inBitStack->pushBits(C4Bits+1, val-1);

	else if (val <= C4B)
		inBitStack->pushBits(C4Bits+4, val+C4B+0x07F);

	else if (val <= C4C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C4Bits+4; ++i)
		{
			if (param <= O4[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C4B-1);
	}
}

void
encode_offset_case3_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C3A)
		inBitStack->pushBits(C3Bits+1, val-1);

	else if (val <= C3B)
		inBitStack->pushBits(C3Bits+4, val+C3B+0x0FF);

	else if (val <= C3C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C3Bits+4; ++i)
		{
			if (param <= O3[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C3B-1);
	}
}

void
encode_offset_case2_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C2A)
		inBitStack->pushBits(C2Bits+1, val-1);

	else if (val <= C2B)
		inBitStack->pushBits(C2Bits+4, val+C2B+0x1FF);

	else if (val <= C2C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C2Bits+4; ++i)
		{
			if (param <= O2[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C2B-1);
	}
}

void
encode_offset_case1_bin(int val,int param, CPushPopper * inBitStack)
{
	if (val <= C1A)
		inBitStack->pushBits(C1Bits+1, val-1);

	else if (val <= C4B)
		inBitStack->pushBits(C1Bits+4, val+C1B+0x3FF);

	else if (val <= C1C)
	{
		ArrayIndex	i;
		inBitStack->pushBits(2,3);
		for (i = 1; i < C1Bits+4; ++i)
		{
			if (param <= O1[i-1])
				break;
		}
		inBitStack->pushBits(i, val-C1B-1);
	}
}

