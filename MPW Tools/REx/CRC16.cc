/*
	File:		CRC16.cc

	Contains:	Cyclic redundancy check for RExHeader.
					CRC16 is given by:  x^16 + x^15 + x^2 + 1

	Written by:	The Newton Tools group.
*/

#include "CRC16.h"

/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

static const uint16_t kCRC16HiTable[16] =
{
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

static const uint16_t kCRC16LoTable[16] =
{
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440
};


/* -----------------------------------------------------------------------------
	C R C 1 6
----------------------------------------------------------------------------- */

void
CRC16::reset(void)
{
	workingCRC = 0;
}

void
CRC16::computeCRC(unsigned char * inData, size_t inSize)
{
	for ( ; inSize > 0; --inSize, ++inData) {
		unsigned char n = workingCRC ^ *inData;
		workingCRC = kCRC16HiTable[n >> 4] ^ kCRC16LoTable[n & 0x0F] ^ (workingCRC >> 8);
	}
}

uint16_t
CRC16::get(void)
{
	finalCRC = workingCRC;
	return finalCRC;
}
