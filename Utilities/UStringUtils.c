/*
	File:		UStringUtils.c

	Contains:	Unicode-friendly version of some common string
					utility functions.

	Written by:	Newton Research Group.
*/
typedef char bool;

#include "Unicode.h"
#include "UnicodePrivate.h"
#include "UStringUtils.h"

#undef YES
#define YES 1
#undef NO
#define NO 0


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

const UniChar * gEmptyString = (const UniChar *)L"";


/*----------------------------------------------------------------------
	Unicode string utilities.
----------------------------------------------------------------------*/

UniChar *
Ustrcpy(UniChar * dst, const UniChar * src)
{
	UniChar * s = dst;
	while ((*dst++ = *src++))
		;
	return s;
}


UniChar *
Ustrncpy(UniChar * dst, const UniChar * src, size_t len)
{
	UniChar * s = dst;
	while (len-- > 0)
	{
		if ((*dst++ = *src++) == kEndOfString)
			return s;
	}
	*dst = kEndOfString;
	return s;
}


UniChar *
Ustrcat(UniChar * dst, const UniChar * src)
{
	UniChar * s = dst;
	while (*dst)
		dst++;
	while ((*dst++ = *src++))
		;
	return s;
}


UniChar *
Ustrncat(UniChar * dst, const UniChar * src, size_t len)
{
	UniChar * s = dst;
	while (*dst)
		dst++;
	while (len-- > 0)
	{
		if ((*dst++ = *src++) == kEndOfString)
			return s;
	}
	*dst = kEndOfString;
	return s;
}


size_t
Ustrlen(const UniChar * str)
{
	const UniChar * s = str;
	while (*s)
		s++;
	return (s - str);
}


UniChar *
Ustrchr(const UniChar * s, UniChar ch)
{
	UniChar c;
	do {
		if ((c = *s) == ch)
			return (UniChar *)s;
		s++;
	} while (c);
	return NULL;
}


int
Ustrcmp(const UniChar * s1, const UniChar * s2)
{
	int delta;
	while (*s1)
		if ((delta = (*s1++ - *s2++)) != 0)
			return delta;
	return 0;
}


void *
Umemset(void * str, UniChar ch, size_t numChars)
{
	UniChar * s = (UniChar *)str;
	while (numChars-- > 0)
		*s++ = ch;
	return str;
}


size_t
Umbstrlen(const UniChar * str, CharEncoding destEncoding)
{
	const UniChar * s = str;
	while (*s)
		s++;
	return (s - str);
}


size_t
Umbstrnlen(const UniChar * str, CharEncoding destEncoding, size_t n)
{
	const UniChar * s = str;
	while (*s && n--)
		s++;
	return (s - str);
}

#pragma mark -

bool
HasChars(UniChar * s)
{
	UniChar	ch;
	while ((ch = *s++) != kEndOfString)
		if ((ch >= 'A' && ch <= 'Z')
		 || (ch >= 'a' && ch <= 'z'))
			return YES;
	return NO;
}

bool
HasDigits(UniChar * s)
{
	UniChar	ch;
	while ((ch = *s++) != kEndOfString)
		if (ch >= '0' && ch <= '9')
			return YES;
	return NO;
}

bool
HasSpaces(UniChar * s)
{
	UniChar	ch;
	while ((ch = *s++) != kEndOfString)
		if (ch == ' ')
			return YES;
	return NO;
}

#pragma mark -

/*----------------------------------------------------------------------
	Character predicates.
----------------------------------------------------------------------*/

bool
IsWhiteSpace(UniChar unic)
{
	return IsSpace(unic)
		 || IsTab(unic)
		 || IsBreaker(unic);
}

bool
IsSpace(UniChar unic)
{
	return unic == ' ';
}

bool
IsTab(UniChar unic)
{
	return unic == 0x0009;
}

bool
IsReturn(UniChar unic)
{
	return unic == 0x000D;
}

bool
IsBreaker(UniChar unic)
{
	return unic == 0x000A
		 || unic == 0x000D;
}

bool
IsDelimiter(UniChar unic)
{
	UChar ch = (unic < 0x80)? unic : A_CONST_CHAR(unic);
	return gASCIIBreakTable[ch];
}

bool
IsAlphabet(UniChar unic)
{
	UniChar	c = unic;
	UpperCaseNoDiacriticsText(&c, 1);
	return (c >= 'A' && c <= 'Z');
}

bool
IsDigit(UniChar unic)
{
	return (unic >= '0' && unic <= '9');
}

bool
IsHexDigit(UniChar unic)
{
	return (unic >= '0' && unic <= '9')
		 || (unic >= 'A' && unic <= 'F')
		 || (unic >= 'a' && unic <= 'f');
}

bool
IsAlphaNumeric(UniChar unic)
{
	return IsDigit(unic)
		 || IsAlphabet(unic);
}

bool
IsFirstByteOf2Byte(UChar ch, CharEncoding encoding)
{
	switch (encoding)
	{
	case kMacRomanEncoding:
	case kASCIIEncoding:
	case kPCRomanEncoding:
		return NO;

	case kMacKanjiEncoding:
		return (ch >= 0x81 && ch <= 0x9F)
			 || (ch >= 0xE0 && ch <= 0xEF)
			 || (ch >= 0xF0 && ch <= 0xFB);

	default:
		ThrowMsg("Unknown character encoding");
	}
	return NO;
}

bool
IsPunctSymbol(UniChar * inStr, int inOffset)	// 00256524
{
	static const UniChar	punctSymbols[] = { '!', '\"', '\'', '(', ')', ',', '.', ':', ';', '?',
														0x2018, 0x2019, 0x201C, 0x201D };

	if (inOffset > 0
	 && (inStr[inOffset] == '\'' || inStr[inOffset] == 0x2019)	// ’s don’t count as punctuation
	 && (inStr[inOffset-1] == 'S' || inStr[inOffset-1] == 's'))
		return NO;

	UniChar ch = inStr[inOffset];
	for (ArrayIndex i = 0; i < sizeof(punctSymbols)/sizeof(UniChar); ++i)
	{
		UniChar punct = punctSymbols[i];
		if (punct == ch)
			return YES;
		if (punct > ch)
			break;
	}
	return NO;
}

void
StripPunctSymbols(UniChar * inStr)
{
	int i = 0, limit = Ustrlen(inStr);

	// strip from the beginning
	for ( ; i < limit && IsPunctSymbol(inStr, i); )
		++i;
	if (i > 0)
		memmove(inStr, inStr + i, (limit - (i - 1)) * sizeof(UniChar));

	// strip from the end
	i = Ustrlen(inStr) - 1;
	while (i >= 0 && IsPunctSymbol(inStr, i))
		inStr[i--] = kEndOfString;
}
