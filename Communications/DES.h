/*
	File:		DES.h

	Contains:	Interface to Data Encryption Standard (DES) functions.
					These are used when connecting to a Newton device to authenticate the user.

	Written by:	Newton Research Group, 2005.
*/

#if !defined(__DES_H)
#define __DES_H 1

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef unsigned short UniChar;

typedef struct
{
	uint32_t hi;
	uint32_t lo;
} SNewtNonce;


void DESCharToKey(UniChar * inString, SNewtNonce * outKey);
void DESKeySched(SNewtNonce * inKey, SNewtNonce * outKeys);
void DESPermute(const unsigned char * inPermuteTable, uint32_t inHi, uint32_t inLo, SNewtNonce * outKey);

void DESEncode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr);
void DESEncodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce);

void DESDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr);
void DESCBCDecode(SNewtNonce * keys, int byteCount, SNewtNonce ** memPtr, SNewtNonce * a4);
void DESDecodeNonce(SNewtNonce * initVect, SNewtNonce * outNonce);

#if defined(__cplusplus)
}
#endif

#endif	/* __DES_H */
