/*
	File:		DES.c

	Contains:	Data Encryption Standard (DES) functions.
					These are used when connecting to a Newton device to authenticate the user.

	Written by:	Newton Research Group, 2005.
*/


#include "DES.h"

/*------------------------------------------------------------------------------
	T a b l e s
--------------------------------------------------------------------------------
	Tables that operate on 64-bit numbers are split into two blocks each of which
	will fit into a long, delimited by 64 and 128 (both illegal values).
	Bit numbers are adjusted from FIP-46 which defines bit 1 as MSB.
------------------------------------------------------------------------------*/

/* Permuted Choice 1 */
const unsigned char DESPC1Tbl[] =
{
	 7, 15, 23, 31, 39, 47, 55,
	63,  6, 14, 22, 30, 38, 46,
	54, 62,  5, 13, 21, 29, 37,
	45, 53, 61,  4, 12, 20, 28,
	64,
	 1,  9, 17, 25, 33, 41, 49,
	57,  2, 10, 18, 26, 34, 42,
	50, 58,  3, 11, 19, 27, 35,
	43, 51, 59, 36, 44, 52, 60,
	128
};

/* Permuted Choice 2 */
const unsigned char DESPC2Tbl[] =
{
	50, 47, 53, 40, 63, 59, 61, 36,
	49, 58, 43, 54, 41, 45, 52, 60,
	64,
	38, 56, 48, 57, 37, 44, 51, 62,
	19,  8, 29, 23, 13,  5, 30, 20,
	 9, 15, 27, 12, 16, 11, 21,  4,
	26,  7, 14, 18, 10, 24, 31, 28,
	128
};

const unsigned char DESIPInvTbl[] =
{
	24, 56, 16, 48,  8, 40,  0, 32,
	25, 57, 17, 49,  9, 41,  1, 33,
	26, 58, 18, 50, 10, 42,  2, 34,
	27, 59, 19, 51, 11, 43,  3, 35,
	64,
	28, 60, 20, 52, 12, 44,  4, 36,
	29, 61, 21, 53, 13, 45,  5, 37,
	30, 62, 22, 54, 14, 46,  6, 38,
	31, 63, 23, 55, 15, 47,  7, 39,
	128
};

const unsigned char DESPTbl[] = {
	16, 25, 12, 11,
	 3, 20,  4, 15,
	31, 17,  9,  6,
	27, 14,  1, 22,
	30, 24,  8, 18,
	 0,  5, 29, 23,
	13, 19,  2, 26,
	10, 21, 28,  7,
	128
};

const unsigned char DESSBoxes[8][64] = {
	{	13,  1,  2, 15,  8, 13,  4,  8,  6, 10, 15,  3, 11,  7,  1,  4,
		10, 12,  9,  5,  3,  6, 14, 11,  5,  0,  0, 14, 12,  9,  7,  2,
		 7,  2, 11,  1,  4, 14,  1,  7,  9,  4, 12, 10, 14,  8,  2, 13,
		 0, 15,  6, 12, 10,  9, 13,  0, 15,  3,  3,  5,  5,  6,  8, 11 },
	{	 4, 13, 11,  0,  2, 11, 14,  7, 15,  4,  0,  9,  8,  1, 13, 10,
		 3, 14, 12,  3,  9,  5,  7, 12,  5,  2, 10, 15,  6,  8,  1,  6,
		 1,  6,  4, 11, 11, 13, 13,  8, 12,  1,  3,  4,  7, 10, 14,  7,
		10,  9, 15,  5,  6,  0,  8, 15,  0, 14,  5,  2,  9,  3,  2, 12 },
	{	12, 10,  1, 15, 10,  4, 15,  2,  9,  7,  2, 12,  6,  9,  8,  5,
		 0,  6, 13,  1,  3, 13,  4, 14, 14,  0,  7, 11,  5,  3, 11,  8,
		 9,  4, 14,  3, 15,  2,  5, 12,  2,  9,  8,  5, 12, 15,  3, 10,
		 7, 11,  0, 14,  4,  1, 10,  7,  1,  6, 13,  0, 11,  8,  6, 13 },
	{	 2, 14, 12, 11,  4,  2,  1, 12,  7,  4, 10,  7, 11, 13,  6,  1,
		 8,  5,  5,  0,  3, 15, 15, 10, 13,  3,  0,  9, 14,  8,  9,  6,
		 4, 11,  2,  8,  1, 12, 11,  7, 10,  1, 13, 14,  7,  2,  8, 13,
		15,  6,  9, 15, 12,  0,  5,  9,  6, 10,  3,  4,  0,  5, 14,  3 },
	{	 7, 13, 13,  8, 14, 11,  3,  5,  0,  6,  6, 15,  9,  0, 10,  3,
		 1,  4,  2,  7,  8,  2,  5, 12, 11,  1, 12, 10,  4, 14, 15,  9,
		10,  3,  6, 15,  9,  0,  0,  6, 12, 10, 11,  1,  7, 13, 13,  8,
		15,  9,  1,  4,  3,  5, 14, 11,  5, 12,  2,  7,  8,  2,  4, 14 },
	{	10, 13,  0,  7,  9,  0, 14,  9,  6,  3,  3,  4, 15,  6,  5, 10,
		 1,  2, 13,  8, 12,  5,  7, 14, 11, 12,  4, 11,  2, 15,  8,  1,
		13,  1,  6, 10,  4, 13,  9,  0,  8,  6, 15,  9,  3,  8,  0,  7,
		11,  4,  1, 15,  2, 14, 12,  3,  5, 11, 10,  5, 14,  2,  7, 12 },
	{	15,  3,  1, 13,  8,  4, 14,  7,  6, 15, 11,  2,  3,  8,  4, 14,
		 9, 12,  7,  0,  2,  1, 13, 10, 12,  6,  0,  9,  5, 11, 10,  5,
		 0, 13, 14,  8,  7, 10, 11,  1, 10,  3,  4, 15, 13,  4,  1,  2,
		 5, 11,  8,  6, 12,  7,  6, 12,  9,  0,  3,  5,  2, 14, 15,  9 },
	{	14,  0,  4, 15, 13,  7,  1,  4,  2, 14, 15,  2, 11, 13,  8,  1,
		 3, 10, 10,  6,  6, 12, 12, 11,  5,  9,  9,  5,  0,  3,  7,  8,
		 4, 15,  1, 12, 14,  8,  8,  2, 13,  4,  6,  9,  2,  1, 11,  7,
		15,  5, 12, 11,  9,  3,  7, 14,  3, 10, 10,  0,  5,  6,  0, 13 }
};

// --------- Odd Bit Number ---------
// Table to fix the parity of a byte.

const unsigned char kParitizedByte[256] =
{
	0x01, 0x01, 0x02, 0x03, 0x04, 0x05, 0x07, 0x07, 0x08, 0x09, 0x0B, 0x0B, 0x0D, 0x0D, 0x0E, 0x0F,
	0x10, 0x11, 0x13, 0x13, 0x15, 0x15, 0x16, 0x17, 0x19, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x1F,
	0x20, 0x21, 0x23, 0x23, 0x25, 0x25, 0x26, 0x27, 0x29, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2F, 0x2F,
	0x31, 0x31, 0x32, 0x33, 0x34, 0x35, 0x37, 0x37, 0x38, 0x39, 0x3B, 0x3B, 0x3D, 0x3D, 0x3E, 0x3F,
	0x40, 0x41, 0x43, 0x43, 0x45, 0x45, 0x46, 0x47, 0x49, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4F, 0x4F,
	0x51, 0x51, 0x52, 0x53, 0x54, 0x55, 0x57, 0x57, 0x58, 0x59, 0x5B, 0x5B, 0x5D, 0x5D, 0x5E, 0x5F,
	0x61, 0x61, 0x62, 0x63, 0x64, 0x65, 0x67, 0x67, 0x68, 0x69, 0x6B, 0x6B, 0x6D, 0x6D, 0x6E, 0x6F,
	0x70, 0x71, 0x73, 0x73, 0x75, 0x75, 0x76, 0x77, 0x79, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7F, 0x7F,
	0x80, 0x81, 0x83, 0x83, 0x85, 0x85, 0x86, 0x87, 0x89, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8F, 0x8F,
	0x91, 0x91, 0x92, 0x93, 0x94, 0x95, 0x97, 0x97, 0x98, 0x99, 0x9B, 0x9B, 0x9D, 0x9D, 0x9E, 0x9F,
	0xA1, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA7, 0xA7, 0xA8, 0xA9, 0xAB, 0xAB, 0xAD, 0xAD, 0xAE, 0xAF,
	0xB0, 0xB1, 0xB3, 0xB3, 0xB5, 0xB5, 0xB6, 0xB7, 0xB9, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBF, 0xBF,
	0xC1, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC7, 0xC7, 0xC8, 0xC9, 0xCB, 0xCB, 0xCD, 0xCD, 0xCE, 0xCF,
	0xD0, 0xD1, 0xD3, 0xD3, 0xD5, 0xD5, 0xD6, 0xD7, 0xD9, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDF, 0xDF,
	0xE0, 0xE1, 0xE3, 0xE3, 0xE5, 0xE5, 0xE6, 0xE7, 0xE9, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEF, 0xEF,
	0xF1, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF7, 0xF7, 0xF8, 0xF9, 0xFB, 0xFB, 0xFD, 0xFD, 0xFE, 0xFF
};


/*------------------------------------------------------------------------------
	P r o t o t y p e s
------------------------------------------------------------------------------*/

static void DESip(uint32_t inKeyHi, uint32_t inKeyLo, SNewtNonce * outKey);
static uint32_t DESfrk(uint32_t Khi, uint32_t Klo, uint32_t R);


/*------------------------------------------------------------------------------
	Permute a 64 bit number using the given permute choice table.
	The number is handled in two 32 bit longs.
------------------------------------------------------------------------------*/

void DESPermute(const unsigned char * inPermuteTable, uint32_t inKeyHi, uint32_t inKeyLo, SNewtNonce * outKey)
{
	uint32_t permutedHi, permutedLo;
	uint32_t srcBits;
	unsigned int bitPos;

	do {
		permutedHi = 0;
		// set chosen bit in output
		while ((bitPos = *inPermuteTable++) < 64)
		{
			permutedHi <<= 1;
			if (bitPos < 32)
				srcBits = inKeyLo;
			else
			{
				srcBits = inKeyHi;
				bitPos -= 32;
			}
			if (srcBits & (1 << bitPos))
				permutedHi |= 1;
		}
		// swap high <-> low	
		srcBits = permutedLo;
		permutedLo = permutedHi;
		permutedHi = srcBits;
	} while (bitPos < 128);

	outKey->hi = permutedHi;
	outKey->lo = permutedLo;
}


/*------------------------------------------------------------------------------
	Calculate the Key Schedule.
------------------------------------------------------------------------------*/

void DESKeySched(SNewtNonce * inKey, SNewtNonce * outKeys)
{
	uint32_t rotateSchedule;
	uint32_t permuteKeyHi, permuteKeyLo;
	SNewtNonce permutedKey;

	DESPermute(DESPC1Tbl, inKey->hi << 1, inKey->lo << 1, &permutedKey);
	permuteKeyHi = permutedKey.hi << 4;
	permuteKeyLo = permutedKey.lo << 4;
	for (rotateSchedule = 0xC0810000; rotateSchedule != 0; rotateSchedule <<= 1)
	{
		if (rotateSchedule & 0x80000000)
		{
			permuteKeyHi = (permuteKeyHi << 1) | ((permuteKeyHi >> 27) & 0x0010);
			permuteKeyLo = (permuteKeyLo << 1) | ((permuteKeyLo >> 27) & 0x0010);
		}
		else
		{
			permuteKeyHi = (permuteKeyHi << 2) | ((permuteKeyHi >> 26) & 0x0030);
			permuteKeyLo = (permuteKeyLo << 2) | ((permuteKeyLo >> 26) & 0x0030);
		}
		DESPermute(DESPC2Tbl, permuteKeyHi, permuteKeyLo, outKeys++);
	}
}


/*------------------------------------------------------------------------------
	Generate a key from a unicode string.
	NOTE	This function relies on big-endian byte order!
------------------------------------------------------------------------------*/

void DESCharToKey(UniChar * inString, SNewtNonce * outKey)
{
	SNewtNonce key0 = { 0x57406860, 0x626D7464 };
	SNewtNonce key1;
	SNewtNonce * pKey1;
	SNewtNonce keysArray[16];
	UniChar buf[4];
	unsigned char * src, * dst;
	int i;
	int moreChars;

	for (moreChars = 1; moreChars == 1; )
	{
		// set up keys array
		DESKeySched(&key0, keysArray);

		// initialize buf with 4 UniChars (64 bits) from string
		for (i = 0; i < 4; ++i)
		{
			if (moreChars)
			{
				if ((buf[i] = *inString) != 0)
					inString++;
				else
					moreChars = 0;
			}
			else
			{
				buf[i] = 0;
			}
		}

		// set up key1
		key1.hi = (buf[0] << 16) | buf[1];
		key1.lo = (buf[2] << 16) | buf[3];

		// encode it
		pKey1 = &key1;
		DESEncode(keysArray, sizeof(SNewtNonce), &pKey1);

		// use this key to set up next keysArray
		src = (unsigned char *)&key1;
		dst = (unsigned char *)&key0;
		for (i = 0; i < 8; ++i)
		{
			// ensure key has odd parity
			*dst++ = kParitizedByte[*src++];
		}
	}

	// return the final key
	*outKey = key0;
}


/*------------------------------------------------------------------------------
	DESip
------------------------------------------------------------------------------*/

static void DESip(uint32_t inKeyHi, uint32_t inKeyLo, SNewtNonce * outKey)
{
	uint32_t d6 = inKeyHi;
	uint32_t d7 = inKeyHi << 16;
	uint32_t a1 = inKeyLo;
	uint32_t a3 = inKeyLo << 16;
	uint32_t resultHi = 0;
	uint32_t resultLo = 0;
	uint32_t temp;
	int i, j;

	for (j = 0; j < 2; j++)
	{
		resultHi = (resultHi >> 1) | (resultHi << 31);
		resultLo = (resultLo >> 1) | (resultLo << 31);

		for (i = 0; i < 8; ++i)
		{
			resultLo = (a3 >> 31) | (resultLo << 1);
			a3 <<= 1;
			resultLo = (resultLo >> 31) | (resultLo << 1);

			resultLo = (a1 >> 31) | (resultLo << 1);
			a1 <<= 1;
			resultLo = (resultLo >> 31) | (resultLo << 1);

			resultLo = (d7 >> 31) | (resultLo << 1);
			d7 <<= 1;
			resultLo = (resultLo >> 31) | (resultLo << 1);

			resultLo = (d6 >> 31) | (resultLo << 1);
			d6 <<= 1;
			resultLo = (resultLo >> 31) | (resultLo << 1);

			temp = resultLo;
			resultLo = resultHi;
			resultHi = temp;
		}
	}
	outKey->hi = resultHi;
	outKey->lo = resultLo;
}


/*------------------------------------------------------------------------------
	The cipher function f(R,K)
	Define 8 blocks of 6 bits each from the 32 bit input R.
------------------------------------------------------------------------------*/

static uint32_t DESfrk(uint32_t Khi, uint32_t Klo, uint32_t R)
{
	uint32_t L;
	SNewtNonce permutedR;
	int i;

	L = 0;
	R = (R >> 31) + (R << 1);			// rotate left 1 bit initially
	for (i = 0; i < 8; ++i)
	{
		L |= DESSBoxes[i][(R ^ Klo) & 0x3F];
		L = (L << 28) | (L >> 4);		// rotate LR right 4 bits for next iter
		R = (R << 28) | (R >> 4);
		Klo = (Khi << 26) + (Klo >> 6);	// rotate K right 6 bits
		Khi = Khi >> 6;
	}
	// apply permutation function P(L)
	DESPermute(DESPTbl, 0, L, &permutedR);

	// return 32 bit result
	return permutedR.lo;
}


/*------------------------------------------------------------------------------
	Encode.
------------------------------------------------------------------------------*/

void DESEncode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr)
{
	int i;
	SNewtNonce permutedData;
	SNewtNonce * keyPtr = *memPtr;
	SNewtNonce * kPtr;
	uint32_t keyHi, keyLo;

	while ((byteCount -= sizeof(SNewtNonce)) >= 0)
	{
		kPtr = keys;
		keyHi = keyPtr->hi;
		keyLo = keyPtr->lo;
		DESip(keyHi, keyLo, &permutedData);
		keyHi = permutedData.hi;
		keyLo = permutedData.lo;
		for (i = 0; i < 8; ++i)
		{
			keyHi ^= DESfrk(kPtr->hi, kPtr->lo, keyLo);	kPtr++;
			keyLo ^= DESfrk(kPtr->hi, kPtr->lo, keyHi);	kPtr++;
		}
		DESPermute(DESIPInvTbl, keyLo, keyHi, &permutedData);
		*keyPtr++ = permutedData;
	}
	*memPtr = keyPtr;
}


void DESCBCEncode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr, SNewtNonce * a4)
{
	int i;
	SNewtNonce permutedData;
	SNewtNonce * keyPtr = *memPtr;
	SNewtNonce * kPtr;
	uint32_t keyHi, keyLo;

	while ((byteCount -= sizeof(SNewtNonce)) >= 0)
	{
		kPtr = keys;
		keyHi = keyPtr->hi;
		keyLo = keyPtr->lo;
		keyHi = (a4->hi ^= keyHi);
		keyLo = (a4->lo ^= keyLo);
		DESip(keyHi, keyLo, &permutedData);
		keyHi = permutedData.hi;
		keyLo = permutedData.lo;
		for (i = 0; i < 8; ++i)
		{
			keyHi ^= DESfrk(kPtr->hi, kPtr->lo, keyLo);	kPtr++;
			keyLo ^= DESfrk(kPtr->hi, kPtr->lo, keyHi);	kPtr++;
		}
		DESPermute(DESIPInvTbl, keyLo, keyHi, &permutedData);
		*keyPtr++ = permutedData;
	}
	*memPtr = keyPtr;
}


void DESEncodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce)
{
	SNewtNonce * pNonce;
	SNewtNonce   keysArray[16];

	DESKeySched(initVect, keysArray);
	pNonce = outNonce;
	DESEncode(keysArray, sizeof(SNewtNonce), &pNonce);
}


/*------------------------------------------------------------------------------
	Decode.
------------------------------------------------------------------------------*/

void DESDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr)
{
	int i;
	SNewtNonce permutedData;
	SNewtNonce * keyPtr = *memPtr;
	SNewtNonce * kPtr;
	uint32_t keyHi, keyLo;

	while ((byteCount -= sizeof(SNewtNonce)) >= 0)
	{
		kPtr = keys;
		DESip(keyPtr->hi, keyPtr->lo, &permutedData);
		keyHi = permutedData.hi;
		keyLo = permutedData.lo;
		for (i = 0; i < 8; ++i)
		{
			keyHi ^= DESfrk(kPtr->hi, kPtr->lo, keyLo);	kPtr++;
			keyLo ^= DESfrk(kPtr->hi, kPtr->lo, keyHi);	kPtr++;
		}
		DESPermute(DESIPInvTbl, keyLo, keyHi, keyPtr++);
	}
	*memPtr = keyPtr;
}


void DESCBCDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr, SNewtNonce * a4)
{
	int i;
	SNewtNonce permutedData, huh;
	SNewtNonce * keyPtr = *memPtr;
	SNewtNonce * kPtr;
	uint32_t dataHi, dataLo;

	while ((byteCount -= sizeof(SNewtNonce)) >= 0)
	{
		kPtr = keys;
		huh = **memPtr;
		DESip(huh.hi, huh.lo, &permutedData);
		dataHi = permutedData.hi;
		dataLo = permutedData.lo;
		for (i = 0; i < 8; ++i)
		{
			dataHi ^= DESfrk(kPtr->hi, kPtr->lo, dataLo);	kPtr++;
			dataLo ^= DESfrk(kPtr->hi, kPtr->lo, dataHi);	kPtr++;
		}
		DESPermute(DESIPInvTbl, dataLo, dataHi, &permutedData);
		dataHi = permutedData.hi ^ a4->hi;
		dataLo = permutedData.lo ^ a4->lo;
		*a4 = huh;
		keyPtr->hi = dataHi;
		keyPtr->lo = dataLo;
		keyPtr++;
	}
	*memPtr = keyPtr;
}


void DESDecodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce)
{
	SNewtNonce *	pNonce;
	SNewtNonce		keysArray[16];

	DESKeySched(initVect, keysArray);
	pNonce = outNonce;
	DESDecode(keysArray, sizeof(SNewtNonce), &pNonce);
}

