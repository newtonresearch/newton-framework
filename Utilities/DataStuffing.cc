/*
	File:		DataStuffing.cc

	Contains:	Binary object stuffing functions.

	Written by:	Newton Research Group, 2013.
*/

#include "DataStuffing.h"
#include "ObjHeader.h"
#include "Unicode.h"
#include "RSSymbols.h"

#pragma mark Bounds checking

/* -----------------------------------------------------------------------------
	Check read bounds request against object.
	Args:		inObj			the object -- must be a pointer object
				inOffset		must be > 0
				inLength		offset + length must not exceed size of object
	Return:	--
	Throws:	kNSErrBadArgs if necessary
----------------------------------------------------------------------------- */

void
BoundsCheck(RefArg inObj, ArrayIndex inOffset, ArrayIndex inLength)
{
	Ref obj = inObj;
	if (!ISPTR(obj) || (ObjectFlags(obj) & kObjSlotted) != 0 || /*inOffset < 0 ||*/ (inOffset + inLength) > Length(obj))
		ThrowErr(exFrames, kNSErrBadArgs);
}


/* -----------------------------------------------------------------------------
	Check write bounds request against object.
	Args:		inObj			the object -- must be a writable pointer object
				inOffset		must be > 0
				inLength		offset + length must not exceed size of object
	Return:	--
	Throws:	kNSErrBadArgs
				kNSErrObjectReadOnly
----------------------------------------------------------------------------- */

void
BoundsWriteCheck(RefArg inObj, ArrayIndex inOffset, ArrayIndex inLength)
{
	Ref obj = inObj;
	unsigned flags;
	if (!ISPTR(obj) || (flags = (ObjectFlags(obj)) & kObjSlotted) != 0 || /*inOffset < 0 ||*/ (inOffset + inLength) > Length(obj))
		ThrowErr(exFrames, kNSErrBadArgs);
	if ((flags & kObjReadOnly) != 0)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, inObj);
}


#pragma mark Extracting

Ref
FExtractChar(RefArg rcvr, RefArg inObj, RefArg inOffset)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsCheck(inObj, offset, sizeof(char));

	char * p = BinaryData(inObj);
	UniChar ustr[2];
	char str[2];
	str[0] = *(p + offset);
	str[1] = 0;
	ConvertToUnicode(str, ustr);
	return MAKECHAR(ustr[0]);
}


Ref
FExtractUniChar(RefArg rcvr, RefArg inObj, RefArg inOffset)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsCheck(inObj, offset, sizeof(UniChar));

	char * p = BinaryData(inObj);
	UniChar uch;
	memcpy(&uch, p+offset, sizeof(UniChar));	// yes, the offset is a char offset, not UniChar
	return MAKECHAR(uch);
}


Ref
FExtractByte(RefArg rcvr, RefArg inObj, RefArg inOffset)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsCheck(inObj, offset, sizeof(int8_t));

	char * p = BinaryData(inObj);
	int8_t v = *(p + offset);
	return MAKEINT(v);
}


Ref
FExtractWord(RefArg rcvr, RefArg inObj, RefArg inOffset)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsCheck(inObj, offset, sizeof(int16_t));

	char * p = BinaryData(inObj);
	int16_t v = *(int16_t *)(p + offset);
	return MAKEINT(v);
}


Ref
FExtractLong(RefArg rcvr, RefArg inObj, RefArg inOffset)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsCheck(inObj, offset, sizeof(int32_t));

	char * p = BinaryData(inObj);
	int32_t v = *(int32_t *)(p + offset);
	uint32_t vMask = (uint32_t)kRefValueMask >> kRefTagBits;
	if ((v & ~vMask) != 0)
		ThrowErr(exFrames, kNSErrLongOutOfRange);
	return MAKEINT(v);
}


Ref
FExtractXLong(RefArg rcvr, RefArg inObj, RefArg inOffset)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsCheck(inObj, offset, sizeof(uint32_t));

	char * p = BinaryData(inObj);
	uint32_t v = *(uint32_t *)(p + offset);
	return MAKEINT(v >> 3);
}


Ref
FExtractCString(RefArg rcvr, RefArg inObj, RefArg inOffset)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsCheck(inObj, offset, sizeof(char));

	// check that C string is nul-terminated
	LockRef(inObj);
	char * p = BinaryData(inObj);
	char * s = p + offset;
	char * limit = s + Length(inObj);
	for ( ; s < limit; s++)
		if (*s == 0)
			break;
	if (s >= limit)
	{
		UnlockRef(inObj);
		ThrowErr(exFrames, kNSErrBadArgs);
	}

	RefVar str(MakeStringFromCString(p));
	UnlockRef(inObj);
	return str;
}

Ref
FExtractPString(RefArg rcvr, RefArg inObj, RefArg inOffset)
{
	ArrayIndex offset = RINT(inOffset);
	unsigned char * p = (unsigned char *)BinaryData(inObj);
	ArrayIndex pStrLen = *p + 1;
	BoundsCheck(inObj, offset, pStrLen);

	RefVar str(AllocateBinary(SYMA(string), pStrLen * sizeof(UniChar)));
	ConvertToUnicode(p+1+offset, (UniChar *)BinaryData(str), pStrLen);
	return str;
}


Ref
FExtractBytes(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inLength, RefArg inClass)
{
	ArrayIndex offset = RINT(inOffset);
	ArrayIndex length = NOTNIL(inLength) ? RINT(inLength) : Length(inObj) - offset;	// if not specified, assume all
	BoundsCheck(inObj, offset, length);		// original does its own checks
	if (!IsSymbol(inClass))
		ThrowErr(exFrames, kNSErrBadArgs);

	RefVar obj(AllocateBinary(inClass, length));
	memcpy(BinaryData(obj), BinaryData(inObj)+offset, length);
	return obj;
}


#pragma mark Stuffing

Ref
FStuffChar(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsWriteCheck(inObj, offset, sizeof(char));

	UniChar ustr[2];
	char str[2];
	ustr[0] = ISINT(inData) ? RINT(inData) : RCHAR(inData);
	ustr[1] = 0;
	ConvertFromUnicode(ustr, str);
	*(BinaryData(inObj) + offset) = str[0];
	return NILREF;
}


Ref
FStuffUniChar(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsWriteCheck(inObj, offset, sizeof(UniChar));

	UniChar uc = ISINT(inData) ? RINT(inData) : RCHAR(inData);
	*(UniChar *)(BinaryData(inObj) + offset) = uc;
	return NILREF;
}


Ref
FStuffByte(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsWriteCheck(inObj, offset, sizeof(int8_t));

	int8_t v = RINT(inData);
	*(BinaryData(inObj) + offset) = v;
	return NILREF;
}


Ref
FStuffWord(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsWriteCheck(inObj, offset, sizeof(int16_t));

	int16_t v = RINT(inData);
	*(int16_t *)(BinaryData(inObj) + offset) = v;
	return NILREF;
}

Ref
FStuffLong(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inData)
{
	ArrayIndex offset = RINT(inOffset);
	BoundsWriteCheck(inObj, offset, sizeof(int32_t));

	int32_t v = RINT(inData);
	*(int32_t *)(BinaryData(inObj) + offset) = v;
	return NILREF;
}


Ref
FStuffCString(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inStr)
{
	ArrayIndex offset = RINT(inOffset);
	if (!IsString(inStr))
		ThrowBadTypeWithFrameData(kNSErrNotAString, inStr);
	ArrayIndex length = Length(inStr) / sizeof(UniChar);	// string includes nul-terminator
	BoundsWriteCheck(inObj, offset, length);

	ConvertFromUnicode((UniChar *)BinaryData(inStr), BinaryData(inObj) + offset, length);
	return NILREF;
}


Ref
FStuffPString(RefArg rcvr, RefArg inObj, RefArg inOffset, RefArg inStr)
{
	ArrayIndex offset = RINT(inOffset);
	if (!IsString(inStr))
		ThrowBadTypeWithFrameData(kNSErrNotAString, inStr);
	ArrayIndex length = Length(inStr) / sizeof(UniChar);	// string includes nul-terminator
	if (length > 255)
		ThrowErr(exFrames, kNSErrStringTooBig);
	BoundsWriteCheck(inObj, offset, length+1);

	char * p = BinaryData(inObj) + offset;
	*p = length;
	ConvertFromUnicode((UniChar *)BinaryData(inStr), p+1, length);
	return NILREF;
}


#if defined(forNTK)
#pragma mark -
// The function more properly known as MakeBinaryFromHex
Ref
FStuffHex(RefArg rcvr, RefArg inHexStr, RefArg inClass)
{
	if (!ISPTR(inHexStr) || (ObjectFlags(inHexStr) & kObjSlotted) != 0 || !IsSymbol(inClass))
		ThrowErr(exFrames, kNSErrBadArgs);

	ArrayIndex i, numOfChars = Length(inHexStr);
	// we’re expecting a sequence of hex chars -- should be paired to make bytes
	RefVar obj(AllocateBinary(inClass, numOfChars/2));
	LockRef(inHexStr); LockRef(obj);
	UniChar * s = (UniChar *)BinaryData(inHexStr);
	UniChar uc1, uc2;
	unsigned char * p = (unsigned char *)BinaryData(obj);
	for (i = 0; i < numOfChars; i += 2)
	{
		uc1 = *s++ - '0'; if (uc1 > 9) uc1 -= 7;
		uc2 = *s++ - '0'; if (uc2 > 9) uc2 -= 7;
		*p++ = (uc1 << 8) + (uc2 & 0x0F);
	}
	UnlockRef(obj); UnlockRef(inHexStr);
	return obj;
}
#endif

