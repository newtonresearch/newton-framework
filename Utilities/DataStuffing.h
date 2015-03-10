/*
	File:		DataStuffing.c

	Contains:	Binary object stuffing functions.

	Written by:	Newton Research Group, 2013.
*/

#if !defined(__DATASTUFFING_H)
#define __DATASTUFFING_H 1

#include "Objects.h"

extern "C" {
Ref	FExtractChar(RefArg rcvr, RefArg inObj, RefArg inOffset);
Ref	FExtractUniChar(RefArg rcvr, RefArg inObj, RefArg inOffset);
Ref	FExtractByte(RefArg rcvr, RefArg inObj, RefArg inOffset);
Ref	FExtractWord(RefArg rcvr, RefArg inObj, RefArg inOffset);
Ref	FExtractLong(RefArg rcvr, RefArg inObj, RefArg inOffset);
Ref	FExtractXLong(RefArg rcvr, RefArg inObj, RefArg inOffset);
Ref	FExtractCString(RefArg rcvr, RefArg inObj, RefArg inOffset);
Ref	FExtractPString(RefArg rcvr, RefArg inObj, RefArg inOffset);
Ref	FExtractBytes(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inLength, RefArg inClass);

Ref	FStuffChar(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData);
Ref	FStuffUniChar(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inChar);
Ref	FStuffByte(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData);
Ref	FStuffWord(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData);
Ref	FStuffLong(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData);
Ref	FStuffCString(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inStr);
Ref	FStuffPString(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inStr);

#if defined(forNTK)
Ref	FStuffHex(RefArg rcvr, RefArg inHexStr, RefArg inClass);
#endif
}

#endif	/* __DATASTUFFING_H */
