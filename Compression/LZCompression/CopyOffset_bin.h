/*
	File:		CopyOffset_bin.h

	Contains:	LZ compression support.
	
	Written by:	Newton Research Group, 2006.
					With a little help from Philz, and a disassembly of the DILs.
*/

#if !defined(__COPYOFFSETBIN_H)
#define __COPYOFFSETBIN_H

#include "PushPopBits.h"

uint32_t decode_offset_case10_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case9_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case8_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case7_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case6_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case5_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case4_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case3_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case2_bin(int, CPushPopper * inBitStack);
uint32_t decode_offset_case1_bin(int, CPushPopper * inBitStack);

void encode_offset_case10_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case9_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case8_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case7_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case6_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case5_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case4_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case3_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case2_bin(int, int, CPushPopper * inBitStack);
void encode_offset_case1_bin(int, int, CPushPopper * inBitStack);

#endif	/* __COPYOFFSETBIN_H */
