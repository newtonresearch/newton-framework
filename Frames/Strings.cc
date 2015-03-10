/*
	File:		Strings.cc

	Contains:	Unicode string functions.

	Written by:	Newton Research Group.
*/

#include <limits.h>
#include <float.h>
#include <stdarg.h>

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "Arrays.h"
#include "Unicode.h"
#include "UStringUtils.h"
#include "Strings.h"
#include "RichStrings.h"
#include "Iterators.h"
#include "Locales.h"
#include "Maths.h"


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FIntern(RefArg inRcvr, RefArg inStr);
Ref	FIsValidString(RefArg inRcvr, RefArg inObj);
Ref	FIsRichString(RefArg inRcvr, RefArg inObj);
Ref	FIsAlphaNumeric(RefArg inRcvr, RefArg inObj);
Ref	FIsWhiteSpace(RefArg inRcvr, RefArg inObj);
Ref	FBeginsWith(RefArg inRcvr, RefArg inStr, RefArg inPrefix);
Ref	FCapitalize(RefArg inRcvr, RefArg ioStr);
Ref	FCapitalizeWords(RefArg inRcvr, RefArg ioStr);
Ref	FCharPos(RefArg inRcvr, RefArg inStr, RefArg inChar, RefArg inStart);
Ref	FDowncase(RefArg inRcvr, RefArg ioStr);
Ref	FEndsWith(RefArg inRcvr, RefArg inStr, RefArg inSuffix);
Ref	FFindStringInArray(RefArg inRcvr, RefArg inArray, RefArg inStr);
Ref	FFindStringInFrame(RefArg inRcvr, RefArg inFrame, RefArg inStrArray, RefArg inDoPaths);
Ref	FGetRichString(RefArg inRcvr);
Ref	FNumberStr(RefArg inRcvr, RefArg inNum);
Ref	FParamStr(RefArg inRcvr, RefArg inStr, RefArg inArgs);
Ref	FSPrintObject(RefArg inRcvr, RefArg inObj);
Ref	FStrEqual(RefArg inRcvr, RefArg inStr1, RefArg inStr2);
Ref	FStrFilled(RefArg inRcvr, RefArg inStr);
Ref	FStringer(RefArg inRcvr, RefArg inArray);
Ref	FStrLen(RefArg inRcvr, RefArg inStr);
Ref	FStrConcat(RefArg inRcvr, RefArg inStr1, RefArg inStr2);
Ref	FStrMunger(RefArg inRcvr, RefArg s1, RefArg s1start, RefArg s1count,
										  RefArg s2, RefArg s2start, RefArg s2count);
Ref	FStrPos(RefArg inRcvr, RefArg inStr, RefArg inSubstr, RefArg inStart);
Ref	FStrReplace(RefArg inRcvr, RefArg inStr, RefArg inSubstr, RefArg inReplacement, RefArg inCount);
Ref	FSubStr(RefArg inRcvr, RefArg inStr, RefArg inStart, RefArg inCount);
Ref	FTrimString(RefArg inRcvr, RefArg ioStr);
Ref	FUpcase(RefArg inRcvr, RefArg ioStr);

Ref	FStrHexDump(RefArg inRcvr, RefArg inData, RefArg inWidth);
}

bool	IsRichString(RefArg inObj);


/*------------------------------------------------------------------------------
	Make a new string object from a C string.
	Args:		str		the C string
	Return:	Ref		the NS string
------------------------------------------------------------------------------*/

Ref
MakeStringFromCString(const char * str)
{
	RefVar	obj(AllocateBinary(SYMA(string), (strlen(str) + 1) * sizeof(UniChar)));
	ConvertToUnicode(str, (UniChar *)BinaryData(obj));
	return obj;
}


/*------------------------------------------------------------------------------
	Make a new string object from a unicode string.
	Args:		str		the unicode string
	Return:	Ref		the NS string
------------------------------------------------------------------------------*/

Ref
MakeString(const UniChar * str)
{
	RefVar	obj(AllocateBinary(SYMA(string), (Ustrlen(str) + 1) * sizeof(UniChar)));
	Ustrcpy((UniChar *)BinaryData(obj), str);
	return obj;
}


Ref
MakeStringOfLength(const UniChar * str, size_t numChars)
{
	UniChar *	data;
	RefVar	obj(AllocateBinary(SYMA(string), (numChars + 1) * sizeof(UniChar)));

	LockRef(obj);
	data = (UniChar *)BinaryData(obj);
	memmove(data, str, numChars * sizeof(UniChar));
	data[numChars] = kEndOfString;
	UnlockRef(obj);

	return obj;
}

/*
Handle
MakeStringHandle(const UniChar * str, size_t numChars)	// unreferenced
{
	Handle h = NewHandle((numChars + 1) * sizeof(UniChar));
	if (h)
	{
		memmove(*h, str, numChars * sizeof(UniChar));
		*(UniChar*)*h = kEndOfString;
	}
	return h;
}

Ptr
MakeStringPtr(const UniChar * str, size_t numChars)	// unreferenced
{
	Ptr p = (Ptr) NewPtr(numChars * sizeof(UniChar));
	if (p)
	{
		memmove(p, str, numChars * sizeof(UniChar));
		// NUL terminator included in numChars
	}
	return p;
}
*/


/*------------------------------------------------------------------------------
	Make a symbol object from a unicode string.
	Args:		inRcvr	NS context
				inStr		the NS string
	Return:	Ref		a symbol
------------------------------------------------------------------------------*/

Ref
FIntern(RefArg inRcvr, RefArg inStr)
{
	char * str = NewPtr(Ustrlen(GetUString(inStr)) + 1);
	if (str == NULL)
		OutOfMemory();
	ConvertFromUnicode(GetUString(inStr), str);
	Ref	sym = MakeSymbol(str);
	FreePtr(str);
	return sym;
}


#pragma mark -

/*------------------------------------------------------------------------------
	Convert NS object to string.
------------------------------------------------------------------------------*/

Ref
SPrintObject(RefArg obj)
{
	ArrayIndex strLength;

	// calculate length of string required
	MakeStringObject(obj, NULL, &strLength, INT_MAX);

	// allocate string object and print into it
	CDataPtr	text(AllocateBinary(SYMA(string), (strLength + 1) * sizeof(UniChar)));
	MakeStringObject(obj, (UniChar *)(char *)text, &strLength, strLength);

	return text;
}


// was StringObject, but that's a typedef
bool
MakeStringObject(RefArg obj, UniChar * str, ArrayIndex * outLength, ArrayIndex inMaxLength)
{
	UniChar		buf[64];
	UniChar *	src;
	ArrayIndex	length;
	bool			result = YES;
	RefVar		op;

	if (IsString(obj))
	{
		length = RSTRLEN(obj);
		op = obj;
		LockRef(op);
		src = (UniChar *)BinaryData(op);
	}
	else if (ISNIL(obj))
		length = 0;
	else
	{
		src = &buf[0];
		if (ISCHAR(obj))
		{
			*src = RCHAR(obj);
			length = 1;
		}
		else if (ISINT(obj))
		{
			IntegerString(RINT(obj), src);
			length = Ustrlen(src);
		}
		else if (IsReal(obj))
		{
			NumberString(CDouble(obj), src, 63, "%.15g");
			length = Ustrlen(src);
		}
		else if (EQRef(ClassOf(obj), MakeSymbol("symbol")))
		{
			ConvertToUnicode(SymbolName(obj), src, 63);
			length = Ustrlen(src);
		}
		else
		{
			length = 0;
			result = NO;
		}
	}

	if (length > inMaxLength)
		length = inMaxLength;
	if (str && length)
	{
		memmove(str, src, length * sizeof(UniChar));
		str[length] = kEndOfString;
	}
	*outLength = length;

	if (NOTNIL(op))
		UnlockRef(op);

	return result;
}


#pragma mark -


Ref
FGetRichString(RefArg inRcvr)
{
	return MakeRichString(GetProtoVariable(inRcvr, SYMA(text)), GetProtoVariable(inRcvr, SYMA(styles)), NO);
}


Ref
FNumberStr(RefArg inRcvr, RefArg inNum)
{
	if (ISINT(inNum)
	||  IsReal(inNum))
	{
		UniChar		str[32];
		ArrayIndex	strLen;
		MakeStringObject(inNum, str, &strLen, 31);
		return MakeString(str);
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Parameterize a string.
	Args:		ioStr
				ioOffset
				inIgnore		YES => ignore placeholders (used in conditional phrases)
				inSuppress	YES => don’t parse ^ placeholders, delete them
				inArgs
	Return:	offset to last char parsed successfully
------------------------------------------------------------------------------*/

ArrayIndex
ParamStrParse(CRichString * ioStr, ArrayIndex ioOffset, bool inIgnore, bool inSuppress, RefArg inArgs)
{
	int		sp04 = 0;	// seems to be redundant
	RefVar	arg;
	ArrayIndex	nxOffset;
	ArrayIndex	c1Offset, c2Offset, length;
	UniChar	ch, ch2;

	while (ioOffset < ioStr->length())
	{
		ch = ioStr->getChar(ioOffset);
		if (ch == '^')
		{
			// next char should be a placeholder 0..9, conditional ?, or escaped ^|
//L37
			nxOffset = ioOffset + 1;	// r7
			if (nxOffset < ioStr->length())
			{
				ch2 = ioStr->getChar(nxOffset);
				switch (ch2)
				{
				case '?':
//L130
					ch = ioStr->getChar(ioOffset + 2);	// r7
					ioStr->deleteRange(ioOffset, 3);
					if ('0' <= ch && ch <= '9')
					{
						arg = GetArraySlot(inArgs, ch - '0');
						bool	doPhrase = NOTNIL(arg) && !(IsString(arg) && StrEmpty(arg));	// r10
						// parse phrase up to opening | delimiter
						nxOffset = ParamStrParse(ioStr, ioOffset, !doPhrase, inSuppress, inArgs);
						c1Offset = nxOffset;
						if (c1Offset < ioStr->length())
							c1Offset++;
						// parse phrase up to closing | delimiter
						c2Offset = ParamStrParse(ioStr, c1Offset, doPhrase, inSuppress, inArgs);
						if (doPhrase)
						{
							// delete second phrase
							length = c2Offset - nxOffset;
							if (c2Offset < ioStr->length())		// including closing | delimiter
								length++;
							ioStr->deleteRange(nxOffset, length);
							ioOffset = nxOffset;
						}
						else
						{
							// delete first phrase
							length = c1Offset - ioOffset;
							ioStr->deleteRange(ioOffset, length);
							ioOffset += (c2Offset - c1Offset);
							// and the closing delimiter
							if (ioOffset < ioStr->length())
								ioStr->deleteRange(ioOffset, 1);
						}
					}
					else
					{
//L219
						// funny placeholder - skip past | | section
						c1Offset = ParamStrParse(ioStr, ioOffset, YES, inSuppress, inArgs);
						ioOffset = ParamStrParse(ioStr, c1Offset + 1, YES, inSuppress, inArgs);
					}
					break;

				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
				case '8':
				case '9':
//L64
					if (inIgnore)
						ioOffset += 2;
					else if (inSuppress)
						ioStr->deleteRange(ioOffset, 2);
					else
					{
//L81
						ch = ioStr->getChar(nxOffset);
						if ('0' <= ch && ch <= '9')	// could have fallen thru switch?
						{
							arg = GetArraySlot(inArgs, ch - '0');
							if (!IsString(arg))
								arg = SPrintObject(arg);
							CRichString	argStr(arg);
							ArrayIndex	argLen = argStr.length();
							ioStr->mungeRange(ioOffset, 2, &argStr, 0, argLen);	// substitute placeholder
							ioOffset += argLen;
						}
					}
					break;

				case '^':
				case '|':
//L122
					// treat as escaped characters
					if (inSuppress)
					{
						ioStr->deleteRange(ioOffset, 1);
						ioOffset = nxOffset;
					}
					else
						ioOffset += 2;
					break;

				default:
//L237
					// ignore funny placeholder
					ioOffset += 2;
					break;
				}
			}
//L128
		}

		else if (ch == '|')
			// we’ve reached a conditional phrase - stop now and return this offset to caller
			break;

		else
			// it’s not an interesting character, just move along
			ioOffset++;
//L27
		if (sp04)
			break;
	}
	return ioOffset;
}


/*------------------------------------------------------------------------------
	NS	ParamStr
	Return a parameterized string.
	Args:		inRcvr
				inStr
				inArgs
	Return:	string object
------------------------------------------------------------------------------*/

Ref
FParamStr(RefArg inRcvr, RefArg inStr, RefArg inArgs)
{
	RefVar		str(Clone(inStr));
	CRichString	richStr(str);
	for (ArrayIndex i = 0; i < 4; ++i)
	{
		for (ArrayIndex offset = 0; offset < richStr.length(); )
		{
			offset = ParamStrParse(&richStr, offset, NO, i == 3, inArgs);
			if (offset < richStr.length())
				offset++;
		}
	}
	return str;
}


Ref
FSPrintObject(RefArg inRcvr, RefArg inObj)
{
	RefVar	strObj;
	if (IsRichString(inObj))
	{
		strObj = Clone(inObj);
		SetClass(strObj, SYMA(string));
	}
	else
	{
		ArrayIndex	actualLen;
		// calculate length of string required
		MakeStringObject(inObj, NULL, &actualLen, 1000);
		// allocate string object and print into it
		strObj = AllocateBinary(SYMA(string), (actualLen + 1) * sizeof(UniChar));
		CDataPtr		strPtr(strObj);
		MakeStringObject(inObj, (UniChar *)(char *)strPtr, &actualLen, actualLen);
	}
	return strObj;
}


Ref
FStringer(RefArg inRcvr, RefArg inArray)
{
	ArrayIndex	itemLength;
	ArrayIndex	stringerLength = 1;
	CObjectIterator	iter(inArray);
	for ( ; !iter.done(); iter.next())
	{
		// calculate length of string required
		MakeStringObject(iter.value(), NULL, &itemLength, INT_MAX);
		stringerLength += itemLength;
	}
	// allocate string object and print into it
	RefVar	strObj(AllocateBinary(SYMA(string), stringerLength * sizeof(UniChar)));
	if (Length(inArray) > 0)
	{
		CDataPtr		lockedObj(strObj);
		UniChar *	strPtr = (UniChar *)(char *)lockedObj;
		for (iter.reset(); !iter.done(); iter.next())
		{
			MakeStringObject(iter.value(), strPtr, &itemLength, INT_MAX);
			strPtr += itemLength;
		}
	}
	return strObj;
}

#pragma mark -

/*------------------------------------------------------------------------------
	Return a pointer to the unicode string data of a string object.
	Args:		inStr			the string object
	Return:	UniChar *	the unicode string
------------------------------------------------------------------------------*/

UniChar *
GetUString(RefArg inStr)
{
	if (!IsString(inStr))
		ThrowBadTypeWithFrameData(kNSErrNotAString, inStr);
	return (UniChar *)BinaryData(inStr);
}


/*------------------------------------------------------------------------------
	Make a new ASCII string binary object from a string object.
	Args:		inStr		the string object
	Return:	Ref		the ASCII string
------------------------------------------------------------------------------*/

Ref
ASCIIString(RefArg inStr)
{
	RefVar ascii(AllocateBinary(SYMA(asciiString), Length(inStr)/sizeof(UniChar)));
	ConvertFromUnicode((UniChar *)BinaryData(inStr), BinaryData(ascii));
	return ascii;
}


/*------------------------------------------------------------------------------
	Find the position of a character within a string.
	Args:		str		the unicode string
				ch			the character to find
				startPos	the index at which to start searching
	Return:	ArrayIndex	the character position
								kIndexNotFound => wasn’t found
------------------------------------------------------------------------------*/

ArrayIndex
CharacterPosition(RefArg str, RefArg ch, int startPos)
{
	CRichString	richStr(str);
	ArrayIndex	strLen = richStr.length();
	ArrayIndex	pos = kIndexNotFound;
	
	if (startPos < 0)
		startPos = 0;
	
	for ( ; startPos < strLen; startPos++)
	{
		if (richStr.getChar(startPos) == ch)
		{
			pos = startPos;
			break;
		}
	}
	return pos;
}


/*------------------------------------------------------------------------------
	Determine whether a string begins with a prefix.
	Args:		str		the string
				prefix	the string to match
	Return:	bool		0 => no match
							1 => match
------------------------------------------------------------------------------*/

bool
StrBeginsWith(RefArg str, RefArg prefix)
{
	CRichString	richStr(str);
	CRichString	richPrefix(prefix);
	bool			doesBegin = NO;

	if (richStr.length() >= richPrefix.length())
	{
		doesBegin = (richStr.compareSubStringCommon(richPrefix, 0, richPrefix.length()) == 0);
	}
	return doesBegin;
}


/*------------------------------------------------------------------------------
	Capitalize the first character in a string.
	Args:		str		the unicode string
	Return:	void
------------------------------------------------------------------------------*/

void
StrCapitalize(RefArg str)
{
	UniChar *	text;

	if (!IsString(str))
		ThrowBadTypeWithFrameData(kNSErrNotAString, str);

	CRichString	richStr(str);
	if (richStr.length() >= 1)
	{
		LockRef(str);
		text = (UniChar *)BinaryData(str);
		UpperCaseText(text, 1);
		UnlockRef(str);
	}
}


/*------------------------------------------------------------------------------
	Capitalize every word in a string.
	Args:		str		the unicode string
	Return:	void
------------------------------------------------------------------------------*/

void
StrCapitalizeWords(RefArg str)
{
	UniChar * text;
	bool isDelimiter;
	bool isInWord = NO;

	if (!IsString(str))
		ThrowBadTypeWithFrameData(kNSErrNotAString, str);

	LockRef(str);
	for (text = (UniChar *)BinaryData(str); *text; text++)
	{
		isDelimiter = IsDelimiter(*text);
		if (isInWord)
		{
			if (isDelimiter)
				isInWord = NO;
		}
		else
		{
			if (!isDelimiter)
			{
				UpperCaseText(text, 1);
				isInWord = YES;
			}
		}
	}
	UnlockRef(str);
}


/*------------------------------------------------------------------------------
	Make a string all lower-case.
	Args:		str		the unicode string
	Return:	void
------------------------------------------------------------------------------*/

void
StrDowncase(RefArg str)
{
	UniChar * text;

	if (!IsString(str))
		ThrowBadTypeWithFrameData(kNSErrNotAString, str);

	LockRef(str);
	text = (UniChar *)BinaryData(str);
	LowerCaseText(text, Ustrlen(text));
	UnlockRef(str);
}


/*------------------------------------------------------------------------------
	Determine whether a string is empty.
	Args:		str		the string
	Return:	bool
------------------------------------------------------------------------------*/

bool
StrEmpty(RefArg str)
{
	if (ISNIL(str))
		return YES;

	CRichString	richStr(str);
	return richStr.length() == 0;
}


/*------------------------------------------------------------------------------
	Determine whether a string ends with a suffix.
	Args:		str		the string
				prefix	the string to match
	Return:	bool		0 => no match
							1 => match
------------------------------------------------------------------------------*/

bool
StrEndsWith(RefArg str, RefArg suffix)
{
	CRichString	richStr(str);
	CRichString	richSuffix(suffix);
	bool			doesEnd = NO;

	if (richStr.length() >= richSuffix.length())
	{
		doesEnd = (richStr.compareSubStringCommon(richSuffix, richStr.length() - richSuffix.length(), richSuffix.length()) == 0);
	}
	return doesEnd;
}


/*------------------------------------------------------------------------------
	Munge two strings.
	Args:		s1			the destination string
				s1start	the first character to be munged
				s1count	the number of characters to be munged
				s2			the source string
				s2start	the first character to be munged
				s2count	the number of characters to be munged
	Return:	void		(the destination string object is modified)
------------------------------------------------------------------------------*/

void
StrMunger(RefArg s1, ArrayIndex s1start, ArrayIndex s1count,
			 RefArg s2, ArrayIndex s2start, ArrayIndex s2count)
{
	if (!IsString(s1))
		ThrowBadTypeWithFrameData(kNSErrNotAString, s1);
	if (NOTNIL(s2) && !IsString(s2))
		ThrowBadTypeWithFrameData(kNSErrNotAStringOrNil, s2);
	if (EQ(s1, s2))
		ThrowExFramesWithBadValue(kNSErrObjectsNotDistinct, s1);
	if (ObjectFlags(s1) & kObjReadOnly)
		ThrowExFramesWithBadValue(kNSErrObjectReadOnly, s1);

	CRichString	richStr1(s1);
	if (s1count == kIndexNotFound)
		s1count = richStr1.length() - s1start;
	s1start = MINMAX(0, s1start, richStr1.length());
	s1count = MINMAX(0, s1count, richStr1.length());

	if (ISNIL(s2))
		richStr1.deleteRange(s1start, s1count);
	else
	{
		CRichString	richStr2(s2);
		if (s2count == kIndexNotFound)
			s2count = richStr2.length() - s2start;
		s2start = MINMAX(0, s2start, richStr2.length());
		s2count = MINMAX(0, s2count, richStr2.length());
		richStr1.mungeRange(s1start, s1count, &richStr2, s2start, s2count);
	}
}


/*------------------------------------------------------------------------------
	Find the position of a string within a string.
	Args:		str		the unicode string
				substr	the string to find
				startPos	the index at which to start searching
	Return:	the position
				-1 => wasn’t found
------------------------------------------------------------------------------*/

ArrayIndex
StrPosition(RefArg str, RefArg substr, ArrayIndex startPos)
{
	CRichString	richStr(str);
	CRichString	richSubstr(substr);
	ArrayIndex	strLen = richStr.length();
	ArrayIndex	substrLen = richSubstr.length();
	ArrayIndex	pos = kIndexNotFound;
	
	for ( ; startPos + substrLen < strLen; startPos++)
	{
		if (richStr.compareSubStringCommon(richSubstr, startPos, substrLen) == 0)
		{
			pos = startPos;
			break;
		}
	}
	return pos;
}


/*------------------------------------------------------------------------------
	Replace occurrences of one string with another within a string.
	Args:		str		the unicode string
				substr	the string to find
				replacement	its replacement
				count		the number of occurrences to replace
							-1 => every occurrence
	Return:	number of replacements actually made
------------------------------------------------------------------------------*/

ArrayIndex
StrReplace(RefArg str, RefArg substr, RefArg replacement, ArrayIndex count)
{
	ArrayIndex strLen, substrLen, replLen;
	ArrayIndex pos, reps;
	int strDelta;

	if (!IsString(str))
		ThrowBadTypeWithFrameData(kNSErrNotAString, str);
	if (!IsString(substr))
		ThrowBadTypeWithFrameData(kNSErrNotAString, substr);
	if (!IsString(replacement))
		ThrowBadTypeWithFrameData(kNSErrNotAString, replacement);

	strLen = RSTRLEN(str);
	substrLen = RSTRLEN(substr);
	if (substrLen > strLen)
		return 0;

	if (count == kIndexNotFound)
		count = strLen;
	replLen = RSTRLEN(replacement);
	strDelta = replLen - substrLen;
	pos = 0;
	for (reps = 0; reps <= count; reps++)
	{
		pos = StrPosition(str, substr, pos);
		if (pos == kIndexNotFound)
			break;
		StrMunger(str, pos, substrLen, replacement, 0, kIndexNotFound);
		pos += replLen;
		strLen += strDelta;
		if (pos > strLen)
			break;
	}
	return reps;
}


/*------------------------------------------------------------------------------
	Make a string all upper-case.
	Args:		str		the unicode string
	Return:	void
------------------------------------------------------------------------------*/

void
StrUpcase(RefArg str)
{
	UniChar *	text;

	if (!IsString(str))
		ThrowBadTypeWithFrameData(kNSErrNotAString, str);

	LockRef(str);
	text = (UniChar *)BinaryData(str);
	UpperCaseText(text, Ustrlen(text));
	UnlockRef(str);
}


/*------------------------------------------------------------------------------
	Make a new string from part of a string.
	Args:		str		the unicode string
				start		the first character to extract
				count		the number of characters to be extracted
	Return:	void
------------------------------------------------------------------------------*/

Ref
Substring(RefArg str, ArrayIndex start, ArrayIndex count)
{
	if (count == kIndexNotFound)
	{
		CRichString	richStr(str);
		count = richStr.length();
	}
	else if (!IsString(str))
		ThrowBadTypeWithFrameData(kNSErrNotAString, str);

	RefVar	strClass(ClassOf(str));
	RefVar	substr(AllocateBinary(strClass, sizeof(UniChar)));
	*(UniChar *)BinaryData(substr) = 0;
	StrMunger(substr, 0, -1, str, start, count);
	return substr;
}


/*------------------------------------------------------------------------------
	Trim leading and trailing whitespace from a string.
	Args:		str		the unicode string
	Return:	void
------------------------------------------------------------------------------*/

void
TrimString(RefArg str)
{
	CRichString	richStr(str);
	ArrayIndex	count, richStrLen, richStrEnd;

	richStrLen = richStr.length();
	for (count = 0; count < richStrLen && IsWhiteSpace(richStr.getChar(count)); count++)
		;
	richStr.deleteRange(0, count);

	richStrEnd = richStr.length() - 1;
	for (count = 0; count < richStrLen && IsWhiteSpace(richStr.getChar(richStrEnd - count)); count++)
		;
	richStr.deleteRange(richStrLen - count, count);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Determine whether string is rich.
	A rich string has its format flags in the last byte instead of a
	nul terminator.
	Args:		obj		the object to test
	Return:	bool
------------------------------------------------------------------------------*/

int
GetCStringFormat(const UniChar * inStr, ArrayIndex index)
{
	const UniChar *	eos = inStr + index;
	return *(eos-1) & 0x03;
}

int
GetStringFormat(RefArg inStr)
{
	UniChar *	eos = (UniChar *)BinaryData(inStr) + Length(inStr)/sizeof(UniChar);
	return *(eos-1) & 0x03;
}


bool
IsRichString(RefArg inObj)
{
	return IsString(inObj)
		 && GetStringFormat(inObj) != 0;
}


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FIsValidString(RefArg inRcvr, RefArg inObj)
{
	if (IsString(inObj))
	{
		CRichString str(inObj);
		if (str.verify())
			return TRUEREF;
	}
	return NILREF;
}

Ref
FIsRichString(RefArg inRcvr, RefArg inObj)
{
	return MAKEBOOLEAN(IsRichString(inObj));
}

Ref
FIsAlphaNumeric(RefArg inRcvr, RefArg inObj)
{
	return MAKEBOOLEAN(IsAlphaNumeric(RCHAR(inObj)));
}

Ref
FIsWhiteSpace(RefArg inRcvr, RefArg inObj)
{
	return MAKEBOOLEAN(IsWhiteSpace(RCHAR(inObj)));
}

Ref
FBeginsWith(RefArg inRcvr, RefArg inStr, RefArg inPrefix)
{
	return MAKEBOOLEAN(StrBeginsWith(inStr, inPrefix));
}

Ref
FCapitalize(RefArg inRcvr, RefArg ioStr)
{
	StrCapitalize(ioStr);
	return ioStr;
}

Ref
FCapitalizeWords(RefArg inRcvr, RefArg ioStr)
{
	StrCapitalizeWords(ioStr);
	return ioStr;
}

Ref
FCharPos(RefArg inRcvr, RefArg inStr, RefArg inChar, RefArg inStart)
{
	ArrayIndex  pos = CharacterPosition(inStr, RCHAR(inChar), RINT(inStart));
	return (pos == kIndexNotFound) ? NILREF : MAKEINT(pos);
	
}

Ref
FDowncase(RefArg inRcvr, RefArg ioStr)
{
	if (ISCHAR(ioStr))
	{
		UniChar  ch = RCHAR(ioStr);
		LowerCaseText(&ch, 1);
		return MAKECHAR(ch);
	}

	StrDowncase(ioStr);
	return ioStr;
}

Ref
FEndsWith(RefArg inRcvr, RefArg inStr, RefArg inSuffix)
{
	return MAKEBOOLEAN(StrEndsWith(inStr, inSuffix));
}

Ref
FFindStringInArray(RefArg inRcvr, RefArg inArray, RefArg inStr)
{
	if (IsString(inStr) && IsArray(inArray))
	{
		CRichString	richStr1(inStr);
		for (ArrayIndex i = 0, count = Length(inArray); i < count; ++i)
		{
			CRichString	richStr2(GetArraySlot(inArray, i));
			if (richStr1.compareSubStringCommon(richStr2, 0, -1, YES) == 0)
				return MAKEINT(i);
		}
	}
	return NILREF;
}

bool	gFindFirstStringinFrame;	// 0C104C20

Ref
RecurseFindStringInFrame(RefArg inSlot, RefArg ioPath, RefArg outResult, const CRichString & inStr, long inDepth)
{
	Ref	found1 = NILREF;
	bool	doFirstOnly = gFindFirstStringinFrame;

	if (inDepth == 0 && IsString(inSlot))
	{
		CRichString	richStr(inSlot);
		bool	sp28 = IsDelimiter(inStr.getChar(0));
		bool	r8 = YES;
		for (int i = 0, count = richStr.length() - inStr.length(); i <= count; ++i)
		{
			bool	isDelim = IsDelimiter(richStr.getChar(i));
			if (isDelim)
			{
				r8 = YES;
				if (!sp28 && isDelim)
					continue;
			}
			if (r8)
			{
				r8 = NO;
				if (richStr.compareSubStringCommon(inStr, i, inStr.length()) == 0)
				{
					found1 = TRUEREF;
					if (NOTNIL(outResult))
					{
						AddArraySlot(outResult, inSlot);
						AddArraySlot(outResult, RA(NILREF));
						AddArraySlot(outResult, MAKEINT(i));
					}
					break;
				}
			}
		}
	}
	else
	{
//L108
		int					pathDepth = inDepth + 1;
		CObjectIterator	iter(inSlot);
		for ( ; !iter.done(); iter.next())
		{
//L120
			if (IsString(iter.value()))
			{
				CRichString	richStr(iter.value());
				bool	sp2C = IsDelimiter(inStr.getChar(0));
				bool	r10 = YES;
				for (int i = 0, count = richStr.length() - inStr.length(); i <= count; ++i)
				{
					bool	isDelim = IsDelimiter(richStr.getChar(i));
					if (isDelim)
					{
						r10 = YES;
						if (!sp2C && isDelim)
							continue;
					}
					if (r10)
					{
						r10 = NO;
						if (richStr.compareSubStringCommon(inStr, i, inStr.length()) == 0)
						{
							RefVar	path;
							found1 = TRUEREF;
							if (NOTNIL(outResult))
							{
							//	sp60 = sp34;
								if (inDepth > 0)
								{
									path = Clone(ioPath);
									SetArraySlot(path, inDepth, iter.tag());
									SetLength(path, pathDepth);
								}
								AddArraySlot(outResult, inSlot);
								AddArraySlot(outResult, path);
								AddArraySlot(outResult, MAKEINT(i));
							}
							i = i + inStr.length() - 1;
							if (doFirstOnly)
								break;
						}
					}
				}
			}
//L222
			else if (ISPTR(iter.value())
				  &&  (ObjectFlags(iter.value()) & kObjSlotted) != 0
				  &&  pathDepth < 10)
			{
				Ref	wasFound;
				// it’s a frame or array, so recurse
				if (NOTNIL(outResult))
					SetArraySlot(ioPath, inDepth, iter.tag());
				wasFound = RecurseFindStringInFrame(iter.value(), ioPath, outResult, inStr, pathDepth);
				if (NOTNIL(wasFound) && ISNIL(outResult))
				{
					found1 = TRUEREF;
					break;
				}
			}
		}
//L262 iterate
	}
//L268
//L273
	return found1;
}


Ref
FFindStringInFrame(RefArg inRcvr, RefArg inFrame, RefArg inStrArray, RefArg inDoPaths)
{
	RefVar	path;
	RefVar	result;

	gFindFirstStringinFrame = EQ(inDoPaths, SYMA(first));

	if (NOTNIL(inDoPaths))
	{
		path = MakeArray(10);
		SetClass(path, SYMA(pathExpr));
		result = MakeArray(0);
	}
	else
		result = TRUEREF;

	CObjectIterator	iter(inStrArray);
	for ( ; !iter.done(); iter.next())
	{
		CRichString	richStr(iter.value());
		if (NOTNIL(inDoPaths))
		{
			ArrayIndex	lengthBefore = Length(result);
			RecurseFindStringInFrame(inFrame, path, result, richStr, 0);
			if (Length(result) == lengthBefore)
			{
				result = inStrArray;
				break;
			}
		}
		else
		{
			Ref	r6 = RecurseFindStringInFrame(inFrame, path, result, richStr, 0);
			if (ISNIL(r6))
			{
				result = inStrArray;
				break;
			}
		}
	}
	return result;
}


Ref
FStrEqual(RefArg inRcvr, RefArg inStr1, RefArg inStr2)
{
	if (EQ(inStr1, inStr2) && IsString(inStr1))
		return TRUEREF;

	if (!(Length(inStr1) == Length(inStr2) && IsString(inStr1) && IsString(inStr2)))
		return NILREF;

	CRichString	richStr1(inStr1);
	CRichString	richStr2(inStr2);
	return MAKEBOOLEAN(richStr1.compareSubStringCommon(richStr2, 0, -1) == 0);
}

Ref
FStrFilled(RefArg inRcvr, RefArg inStr)
{
	return MAKEBOOLEAN(!StrEmpty(inStr));
}

Ref
FStrLen(RefArg inRcvr, RefArg inStr)
{
	CRichString richStr(inStr);
	return MAKEINT(richStr.length());
}

Ref
FStrConcat(RefArg inRcvr, RefArg inStr1, RefArg inStr2)
{
	RefVar str(Clone(inStr1));
	CRichString	richStr1(str);
	CRichString	richStr2(inStr2);
	richStr1.insertRange(&richStr2, 0, richStr2.length(), richStr1.length());
	return str;
}

Ref
FStrMunger(RefArg inRcvr, RefArg s1, RefArg s1start, RefArg s1count,
								  RefArg s2, RefArg s2start, RefArg s2count)
{
	StrMunger(s1, RINT(s1start), NOTNIL(s1count) ? RINT(s1count) : kIndexNotFound,
				 s2, RINT(s2start), NOTNIL(s2count) ? RINT(s2count) : kIndexNotFound);
	return s1;
}

Ref
FStrPos(RefArg inRcvr, RefArg inStr, RefArg inSubstr, RefArg inStart)
{
	int startPos = RINT(inStart);
	if (startPos < 0)
		startPos = 0;
	ArrayIndex  pos = StrPosition(inStr, inSubstr, startPos);
	return (pos == kIndexNotFound) ? NILREF : MAKEINT(pos);
}

Ref
FStrReplace(RefArg inRcvr, RefArg inStr, RefArg inSubstr, RefArg inReplacement, RefArg inCount)
{
	return MAKEINT(StrReplace(inStr, inSubstr, inReplacement, NOTNIL(inCount) ? RINT(inCount) : kIndexNotFound));
}

Ref
FSubStr(RefArg inRcvr, RefArg inStr, RefArg inStart, RefArg inCount)
{
	return Substring(inStr, RINT(inStart), NOTNIL(inCount) ? RINT(inCount) : kIndexNotFound);
}

Ref
FTrimString(RefArg inRcvr, RefArg ioStr)
{
	TrimString(ioStr);
	return ioStr;
}

Ref
FUpcase(RefArg inRcvr, RefArg ioStr)
{
	if (ISCHAR(ioStr))
	{
		UniChar  ch = RCHAR(ioStr);
		UpperCaseText(&ch, 1);
		return MAKECHAR(ch);
	}

	StrUpcase(ioStr);
	return ioStr;
}

#pragma mark -

Ref
StrHexDump(RefArg inData, RefArg inWidth)
{
	CDataPtr data(inData);
	UChar * srcPtr = (UChar *)(char *)data;
	ArrayIndex width = NOTNIL(inWidth) ? RINT(inWidth) : 0;
	ArrayIndex dataLen = Length(inData);
	ArrayIndex strLen = (dataLen * 2 + 1) * sizeof(UniChar);
	if (width != 0)
		strLen += (dataLen / width) * sizeof(UniChar);

	RefVar hexStr(AllocateBinary(SYMA(string), strLen));
	CDataPtr str(hexStr);
	UniChar * dstPtr = (UniChar *)(char *)str;
	for (ArrayIndex i = 0; i < dataLen; ++i)
	{
		UChar ch = *srcPtr++;
		UniChar uc = ch >> 4;
		if (uc < 10) uc += '0'; else uc += 'A' - 10;
		*dstPtr++ = uc;
		uc = ch & 0x0F;
		if (uc < 10) uc += '0'; else uc += 'A' - 10;
		*dstPtr++ = uc;
		if (width != 0 && (i+1) % width == 0)
			*dstPtr = ' ';
	}
	*dstPtr = 0;
	return hexStr;
}


Ref
FStrHexDump(RefArg inRcvr, RefArg inData, RefArg inWidth)
{
	return StrHexDump(inData, inWidth);
}

