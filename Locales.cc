/*
	File:		Locale.cc

	Contains:	Locale implementation.

	Written by:	Newton Research Group.
*/

#include <stdarg.h>

#include "Objects.h"
#include "Globals.h"
#include "NewtonScript.h"
#include "OSErrors.h"
#include "Preference.h"
#include "Maths.h"
#include "Strings.h"
#include "UStringUtils.h"
#include "Locales.h"
#include "ROMResources.h"


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

const UniChar	gSpaceStr[] = {' ',0};	// +00 0C100F84 ?
const UniChar	gZeroStr[] = {'0',0};	// +04 0C100F88 ?
int			gNumberGroupWidth;	// +18	0C100F9C
bool			gNumberLeadingZero;	// +1C	0C100FA0
bool			gIntlDayLeadingZero;	// +20	0C100FA4
UniChar *	gPositiveNumProto;	// +24	0C100FA8
UniChar *	gNegativeNumProto;	// +28	0C100FAC
UniChar *	gPositiveIntProto;	// +2C	0C100FB0
UniChar *	gNegativeIntProto;	// +30	0C100FB4

LocaleStrCache *	gLocaleStrCache;


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FGetLocale(RefArg inRcvr);
Ref	FSetLocale(RefArg inRcvr, RefArg inLocale);

Ref	FFormattedNumberStr(RefArg inRcvr, RefArg inNum, RefArg inFormatStr);
}


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

Ref FindLocaleBundleByName(RefArg inLocale);
Ref SetCurrentLocale(RefArg inLocale);

static int ROMCacheLocaleAttributes(void);
static int CacheLocaleAttributes(void);

#pragma mark -

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FGetLocale(RefArg inRcvr)
{
	return GetCurrentLocale();
}


Ref
FSetLocale(RefArg inRcvr, RefArg inLocale)
{
	return SetCurrentLocale(inLocale);
}

#pragma mark -

/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

NewtonErr
InitInternationalUtils(void)
{
	gLocaleStrCache = new LocaleStrCache;
	if (ISNIL(GetCurrentLocale()) || !CacheLocaleAttributes())
		return kOSErrNoMemory;
	return noErr;
}


Ref
IntlResources(void)
{
	return GetFrameSlot(gVarFrame, SYMA(international));
}


Ref
GetLocaleSlot(RefArg inLocale, RefArg inTag)
{
	RefVar locale;
	if (ISNIL(inLocale) || EQ(inLocale, SYMA(currentLocaleBundle)))
		locale = GetCurrentLocale();
	else if (EQ(inLocale, SYMA(systemLocaleBundle)))
		locale = GetProtoVariable(IntlResources(), SYMA(systemLocaleBundle));
	else
		locale = inLocale;
	return GetProtoVariable(locale, inTag);
}


Ref
GetLocaleSlot(RefArg inTag)
{
	return GetProtoVariable(GetCurrentLocale(), inTag);
}


Ref
GetCurrentLocale(void)
{
	return GetProtoVariable(IntlResources(), SYMA(currentLocaleBundle));
}


Ref
SetCurrentLocale(RefArg inLocale)
{
	RefVar localeBundle;	// sp00
	if (IsString(inLocale) || IsSymbol(inLocale))
		localeBundle = FindLocaleBundleByName(inLocale);
	else if (IsFrame(inLocale))
		localeBundle = inLocale;

	if (NOTNIL(localeBundle))
	{
		RefVar saveLocale(GetCurrentLocale());
		RefVar international(IntlResources());
		SetFrameSlot(international, SYMA(currentLocaleBundle), localeBundle);
		if (CacheLocaleAttributes() != 0)
			SetPreference(SYMA(locale), GetFrameSlot(localeBundle, SYMA(title)));
		else
		{
			SetFrameSlot(international, SYMA(currentLocaleBundle), saveLocale);
			localeBundle = NILREF;
		}
//		FReadCursiveOptions(NILREFARG);	// we donÕt do cursive
		NSCallGlobalFn(SYMA(UpdateLocaleFromUserConfig));
	}
	return localeBundle;
}


Ref
FindLocaleBundleByName(RefArg inLocale)
{
// INCOMPLETE
	return NILREF;
}


UniChar *
PositiveNumberProtoStr(void)
{
	if (gPositiveNumProto == NULL)
	{
		int			dpStrLen = Ustrlen(GetUString(gLocaleStrCache->decimalPoint));
		UniChar *	str = (UniChar *)calloc(dpStrLen + 5, sizeof(UniChar));
		if (str != NULL)
		{
			str[0] = '^';
			str[1] = '0';
			Ustrcpy(str, GetUString(gLocaleStrCache->decimalPoint));	// might have moved after calloc
			str += dpStrLen;
			str[2] = '^';
			str[3] = '1';
		}
		gPositiveNumProto = str;
	}
	return gPositiveNumProto;
}


UniChar *
NegativeNumberProtoStr(void)
{
	if (gNegativeNumProto == NULL)
	{
		UniChar *	protoStr = PositiveNumberProtoStr();
		UniChar *	prefixStr = GetUString(gLocaleStrCache->minusPrefix);
		UniChar *	suffixStr = GetUString(gLocaleStrCache->minusSuffix);
		UniChar *	str = (UniChar *)calloc(Ustrlen(protoStr) + Ustrlen(prefixStr) + Ustrlen(suffixStr), sizeof(UniChar));
		if (str != NULL)
		{
			Ustrcpy(str, prefixStr);
			Ustrcat(str, protoStr);
			Ustrcat(str, suffixStr);
		}
		gNegativeNumProto = str;
	}
	return gNegativeNumProto;
}


UniChar *
PositiveIntProtoStr(void)
{
	if (gPositiveIntProto == NULL)
	{
		UniChar *	str = (UniChar *)calloc(3, sizeof(UniChar));
		if (str != NULL)
			ConvertToUnicode("^0", str, 2);
		gPositiveIntProto = str;
	}
	return gPositiveIntProto;
}


UniChar *
NegativeIntProtoStr(void)
{
	if (gNegativeIntProto == NULL)
	{
		UniChar *	protoStr = PositiveIntProtoStr();
		UniChar *	prefixStr = GetUString(gLocaleStrCache->minusPrefix);
		UniChar *	suffixStr = GetUString(gLocaleStrCache->minusSuffix);
		UniChar *	str = (UniChar *)calloc(Ustrlen(protoStr) + Ustrlen(prefixStr) + Ustrlen(suffixStr), sizeof(UniChar));
		if (str != NULL)
		{
			Ustrcpy(str, prefixStr);
			Ustrcat(str, protoStr);
			Ustrcat(str, suffixStr);
		}
		gNegativeIntProto = str;
	}
	return gNegativeIntProto;
}


static int
ROMCacheLocaleAttributes(void)
{
	RefVar	theLocale(GetCurrentLocale());	// sp18
	RefVar	format(GetProtoVariable(theLocale, SYMA(longDateFormat)));	// sp14
//	op[](&sp00, &sp14, 4, new RefVar);
	RefVar	longDofWeek(GetProtoVariable(format, SYMA(longDofWeek)));	// sp00
	RefVar	abbrDofWeek(GetProtoVariable(format, SYMA(abbrDofWeek)));	// sp04
	RefVar	terseDofWeek(GetProtoVariable(format, SYMA(terseDofWeek)));	// sp08
	RefVar	shortDofWeek(GetProtoVariable(format, SYMA(shortDofWeek)));	// sp0C
	if (ISNIL(longDofWeek))
		return 0;
	if (ISNIL(abbrDofWeek))
		abbrDofWeek = longDofWeek;
	if (ISNIL(terseDofWeek))
		terseDofWeek = abbrDofWeek;
	if (ISNIL(shortDofWeek))
		shortDofWeek = terseDofWeek;

	RefVar	longMonth(GetProtoVariable(format, SYMA(longMonth)));			// sp10

	//sp-1C
	format = GetProtoVariable(theLocale, SYMA(numberFormat));
//	op[](&sp00, &sp18, 4, new RefVar);
	RefVar	decimalPoint(GetProtoVariable(format, SYMA(decimalPoint)));			// sp00
	RefVar	groupSepStr(GetProtoVariable(format, SYMA(groupSepStr)));			// sp04
	RefVar	minusPrefix(GetProtoVariable(format, SYMA(minusPrefix)));			// sp08
	RefVar	minusSuffix(GetProtoVariable(format, SYMA(minusSuffix)));			// sp0C
	RefVar	currencyPrefix(GetProtoVariable(format, SYMA(currencyPrefix)));	// sp10
	RefVar	currencySuffix(GetProtoVariable(format, SYMA(currencySuffix)));	// sp14
	long		groupWidth = RINT(GetProtoVariable(format, SYMA(groupWidth)));		// r9
	if (ISNIL(decimalPoint)
	||  ISNIL(groupSepStr)
	||  ISNIL(minusPrefix)
	||  ISNIL(minusSuffix)
	||  ISNIL(currencyPrefix)
	||  ISNIL(currencySuffix))
		return 0;
/*
	ReplaceDictionaryHandle(&gTimeLexDictionary, SYMA(timeDictionary));
	ReplaceDictionaryHandle(&gDateLexDictionary, SYMA(dateDictionary));
	ReplaceDictionaryHandle(&gPhoneLexDictionary, SYMA(phoneDictionary));
	ReplaceDictionaryHandle(&gNumberLexDictionary, SYMA(numberDictionary));
	if (gTimeLexDictionary == NULL
	||  gDateLexDictionary == NULL
	||  gPhoneLexDictionary == NULL
	||  gNumberLexDictionary = NULL)
		return 0;
*/
	gLocaleStrCache->longDofWeek = longDofWeek;
	gLocaleStrCache->abbrDofWeek = abbrDofWeek;
	gLocaleStrCache->terseDofWeek = terseDofWeek;
	gLocaleStrCache->shortDofWeek = shortDofWeek;
	gLocaleStrCache->longMonth = longMonth;
	gLocaleStrCache->decimalPoint = decimalPoint;
	gLocaleStrCache->groupSepStr = groupSepStr;
	gLocaleStrCache->minusPrefix = minusPrefix;
	gLocaleStrCache->minusSuffix = minusSuffix;
	gLocaleStrCache->currencyPrefix = currencyPrefix;
	gLocaleStrCache->currencySuffix = currencySuffix;

	gNumberGroupWidth = groupWidth;
	gNumberLeadingZero = (groupWidth == 0);
	if (gPositiveNumProto != NULL)
	{
		free(gPositiveNumProto);
		gPositiveNumProto = NULL;
	}
	if (gNegativeNumProto != NULL)
	{
		free(gNegativeNumProto);
		gNegativeNumProto = NULL;
	}
	return 1;
}


static int
CacheLocaleAttributes(void)
{
	int	result = ROMCacheLocaleAttributes();
	if (result)
	{
		// cache number protos for the new locale
		NegativeNumberProtoStr();
	//	PositiveNumberProtoStr();	done by NegativeNumberProtoStr
	}
	return result;
}

#pragma mark -

NewtonErr
IntlNumberMunge(const char * inStr, UniChar * outStr, bool inIsNegative, ArrayIndex inNumOfDecimals, ArrayIndex inStrLen, ULong inFmt)
{
	UniChar	intStr[32];
	UniChar	fracStr[32];
	UniChar	prefixStr[8];
	UniChar	suffixStr[8];

	size_t	srcStrLen = strlen(inStr);
	if (srcStrLen > 20)
		return -2;		// number too large

	size_t	intLen = inNumOfDecimals;
	size_t	fracLen = 0;
	size_t	prefixLen;
	size_t	suffixLen;
	intStr[0] = kEndOfString;
	fracStr[0] = kEndOfString;
	prefixStr[0] = kEndOfString;
	suffixStr[0] = kEndOfString;

	if (intLen == 0)
		intLen = srcStrLen;

	if ((inFmt & kFormatGroups) != 0)
	{
		// format integer part in groups
		UniChar *	groupSep = GetUString(gLocaleStrCache->groupSepStr);
		size_t		groupSepLen = Ustrlen(groupSep);
		UniChar *	dstStr = intStr;
		const char *srcStr = inStr;
		int			srcLen = intLen;
		size_t		groupWidth = srcLen % gNumberGroupWidth;
		if (groupWidth == 0)
			groupWidth = gNumberGroupWidth;
		while (srcLen > 0)
		{
			ConvertToUnicode(srcStr, dstStr, groupWidth);
			srcLen -= groupWidth;
			srcStr += groupWidth;
			dstStr += groupWidth;
			if (srcLen > 0)
			{
				Ustrcpy(dstStr, groupSep);
				dstStr += groupSepLen;
			}
			groupWidth = gNumberGroupWidth;
		}
	}
	else
	{
		ConvertToUnicode(inStr, intStr, intLen);
		intStr[intLen] = kEndOfString;
	}
	if (intLen != srcStrLen)
	{
		// there are digits remaining after the integer part - this must be the fraction
		fracLen = srcStrLen - intLen - 1;
		ConvertToUnicode(inStr + intLen + 1, fracStr, fracLen);
		fracStr[fracLen] = kEndOfString;
	}

	// ¥¥ build the prefix string
	UniChar *	str = prefixStr;
	UniChar *	affixStr;
	bool	isBracketed = ((inFmt & kFormatMinus) != 0);
	if (inIsNegative)
	{
		if (isBracketed)
		{
			ConvertToUnicode("(", str, 1);
			str++;
		}
		else
		{
			affixStr = GetUString(gLocaleStrCache->minusPrefix);
			Ustrcpy(str, affixStr);
			str += Ustrlen(affixStr);
		}
	}
	bool	isCurrency = ((inFmt & kFormatCurrency) != 0);
	if (isCurrency)
	{
		affixStr = GetUString(gLocaleStrCache->currencyPrefix);
		Ustrcpy(str, affixStr);
		str += Ustrlen(affixStr);
	}
	*str = kEndOfString;
	prefixLen = Ustrlen(prefixStr);

	// ¥¥ build the suffix string
	str = suffixStr;
	if (isCurrency)
	{
		affixStr = GetUString(gLocaleStrCache->currencySuffix);
		Ustrcpy(str, affixStr);
		str += Ustrlen(affixStr);
	}
	if ((inFmt & kFormatPercent) != 0)
	{
		ConvertToUnicode("%", str, 1);
		str++;
	}
	if (inIsNegative)
	{
		if (isBracketed)
		{
			ConvertToUnicode(")", str, 1);
			str++;
		}
		else
		{
			affixStr = GetUString(gLocaleStrCache->minusSuffix);
			Ustrcpy(str, affixStr);
			str += Ustrlen(affixStr);
		}
	}
	*str = kEndOfString;
	suffixLen = Ustrlen(suffixStr);

	// ¥¥ assemble the final string
	if (prefixLen + intLen + fracLen + suffixLen > inStrLen)
		return -10;		// final string too long for destination

	if (prefixLen != 0)
	{
		Ustrcpy(outStr, prefixStr);
		outStr += prefixLen;
	}
	if (intLen != 0)
	{
		Ustrcpy(outStr, intStr);
		outStr += intLen;
	}
	if (fracLen != 0)
	{
		str = GetUString(gLocaleStrCache->decimalPoint);
		Ustrcpy(outStr, str);
		outStr += Ustrlen(str);
		Ustrcpy(outStr, fracStr);
		outStr += fracLen;
	}
	if (suffixLen != 0)
	{
		Ustrcpy(outStr, suffixStr);
		outStr += suffixLen;
	}
	*outStr = kEndOfString;
	return noErr;
}


/*------------------------------------------------------------------------------
	Convert number to string.
------------------------------------------------------------------------------*/

void
ParamString(UniChar * ioStr, size_t inStrLen, const UniChar * inFmt, ...)
{
	UniChar *paramPtr[10];
	long		paramIndex[10];
	va_list	args;
	va_start(args, inFmt);

//	scan inFmt for params and build paramArrays
	int		numOfParams = 0;
	UniChar	ch;
	const UniChar *	p = inFmt;
	while ((ch = *p++) != kEndOfString)
	{
		if (ch == '^')
		{
			if ((ch = *p++) == kEndOfString)
				break;
			if ('0' <= ch && ch <= '9')
			{
				paramPtr[numOfParams] = va_arg(args, UniChar*);
				paramIndex[numOfParams] = (p - inFmt) - 2;
				numOfParams++;
			}
		}
	}
	va_end(args);

	int	fmtStrLen = (p - inFmt);	// sp00
	int	index = 0;	// r8
	int	indexOfParam = paramIndex[index];	// r0
	int	lenRemaining;	// r4
	p = inFmt;
	for (int i = 0, lenRemaining = inStrLen; i < fmtStrLen && lenRemaining > 0; ++i)
	{
		if ((ch = *p++) == kEndOfString)
			break;
		if (i == indexOfParam)
		{
			++i;
			int			pStrLen;
			UniChar *	pStr = paramPtr[*p++ - '0'];
			if (pStr != NULL)
			{
				pStrLen = Ustrlen(pStr);
				if (pStrLen <= lenRemaining)
				{
					Ustrcpy(ioStr, pStr);
					lenRemaining -= pStrLen;
					ioStr += pStrLen;
				}
				else
					Ustrncpy(ioStr, pStr, lenRemaining);
			}
			index++;
			indexOfParam = (index < numOfParams) ? paramIndex[index] : 0;
		}
		else
			*ioStr++ = ch;
	}
	*ioStr = kEndOfString;
}


NewtonErr
NumberString(double inNum, UniChar * outStr, ArrayIndex inStrLen, const char * inFmt)
{
	NewtonErr	err = noErr;

	*outStr = kEndOfString;
	if (inNum > DBL_MAX)
		err = -2;
	else if (inNum < DBL_MIN)
		err = -3;

	else
	{
		bool isNegative = (inNum < 0.0);	// r8
		if (isNegative)
			inNum = -inNum;

		// make the ANSI-C formatted string
		char *	s, str[64], ch;
		sprintf(str, inFmt, inNum);

		//sp-0C
		char *	decimalPt = NULL;	// r6
		char *	srcStr = NULL;		// r9
		char *	postamble = NULL;	// r10
		int		preambLen = 0;		// r7
		int		srcLen = 0;			// r5
		int		postambLen = 0;	// sp08
	//	int		sp04 = 0

		// scan input string to determine:
		//  location of first digit (stuff before this is preamble, output verbatim)
		//  location of decimal point (so we can format int & frac parts separately)
		//  location of last digit (stuff after this is postamble, output verbatim)
		//  whether number has exponent, in which case forget about fancy formatting
		for (s = str; (ch = *s) != kEndOfString; s++)
		{
			if (ch == '.')
				decimalPt = s + 1;
			else
			{
				if (ch == 'e')
				{
					if (isNegative)
					{
					//	inNum = -inNum;
					//	sprintf(str, inFmt, inNum);
						*outStr++ = '-';
					}
					ConvertToUnicode(str, outStr, strlen(str));	// look, no inStrLen size check!
					return err;
				}
				else if ('0' <= ch && ch <= '9')
				{
					if (srcLen == 0)
						srcStr = s;
					if (decimalPt == NULL)
						srcLen++;
				}
				else if (decimalPt != NULL && postamble == NULL)
				{
					postamble = s;
					postambLen = strlen(s);
				}
				if (srcLen == 0)
					preambLen++;
			}
		}
//L94
		//sp-14
		UniChar *	groupSep = GetUString(gLocaleStrCache->groupSepStr);	// sp10
		int			groupSepLen = Ustrlen(groupSep);								// sp0C
		//int			sp08 = gNumberGroupWidth;
		UniChar *	protoStr;															// sp04
		if (isNegative)
			protoStr = (decimalPt != NULL) ? NegativeNumberProtoStr() : NegativeIntProtoStr();
		else
			protoStr = (decimalPt != NULL) ? PositiveNumberProtoStr() : PositiveIntProtoStr();
//L127
		//sp-08
		int	intLen = srcLen + groupSepLen * (srcLen-1)/gNumberGroupWidth;	// sp04
		int	fracLen;																			// sp00
		if (decimalPt != NULL)
			fracLen = strlen(decimalPt) - postambLen;
//		else if (sp24 != NULL)
//			fracLen = strlen(sp24) - postambLen;
		else
			fracLen = 0;
//L146
		if (inStrLen > Ustrlen(protoStr) + preambLen + intLen + fracLen + postambLen)
		{
			if (intLen < 32 && fracLen < 32)
			{
				UniChar	intStr[32];		// sp40
				UniChar	fracStr[32];	// sp00

				if (srcStr)
				{
					UniChar *	dstStr = intStr;	// r8
					int	groupWidth = srcLen % gNumberGroupWidth;	// r9
					if (groupWidth == 0)
						groupWidth = gNumberGroupWidth;
					while (srcLen > 0)
					{
						ConvertToUnicode(srcStr, dstStr, groupWidth);
						srcLen -= groupWidth;
						srcStr += groupWidth;
						dstStr += groupWidth;
						if (srcLen > 0)
						{
							Ustrcpy(dstStr, groupSep);
							dstStr += groupSepLen;
						}
						groupWidth = gNumberGroupWidth;
					}
				}
				else
					intStr[0] = kEndOfString;
//L200
				if (decimalPt != NULL)
					ConvertToUnicode(decimalPt, fracStr, fracLen);
//				else if (sp24 != NULL)
//					ConvertToUnicode(sp24, fracStr, fracLen);
				else
					fracStr[0] = kEndOfString;
//L217
				// finally output string - any preamble first
				if (preambLen)
				{
					ConvertToUnicode(str, outStr, preambLen);
					outStr += preambLen;
				}
				// output number in prototype format
				ParamString(outStr, inStrLen, protoStr, intStr, fracStr);
				// output postamble
				if (postambLen)
				{
					outStr += Ustrlen(outStr);
					ConvertToUnicode(postamble, outStr, postambLen);
				}
			}
		}
		else
			err = isNegative ? -3 : -2;
	}
	return err;
}


NewtonErr
NumberStringSpec(double inNum, UniChar * outStr, ArrayIndex inStrLen, ULong inFmt)
{
	char	str[48]; // sp10
	ArrayIndex numOfDecimals;
	bool	isNegative;
/*
	int	r12 = 0x46;
	int	sp08 = 0x06;
	int	sp04 = -1;
	int	sp00 = -1;

	if ((inFmt & 0x0080) != 0)
	{
		sp08 = inFmt & 0x0F;
		if ((inFmt & 0x0200) == 0)
			r12 = 0x66;
	}
*/
	if ((inFmt & kFormatPercent) != 0)
		inNum *= 100.0;

//	numOfDecimals = _fp_display(r12, inNum, &sp04, 0x0110, &sp0C, &sp08, &sp04, &sp00);
	numOfDecimals = sprintf(str, "%#g", inNum);
//	str[numOfDecimals] = kEndOfString;

	isNegative = (str[0] == '-');
/*
	if (sp04 > 0 && sp00 > 0)
		for (ArrayIndex i = 0; i < numOfDecimals; ++i)
			if (str[i] == '<' ||  str[i] == '>')
				str[i] == '0';
*/
	for (ArrayIndex i = 0; i < numOfDecimals; ++i)
		if (str[i] == '.')
		{
			numOfDecimals = i;
			break;
		}

	return IntlNumberMunge(isNegative?str+1:str, outStr, isNegative, isNegative?numOfDecimals-1:numOfDecimals, inStrLen, inFmt);
}


NewtonErr
IntegerString(int inNum, UniChar * outStr)
{
	UniChar *	s, ch;
	div_t	number;
	int	first;
	int	last;

	// Determine sign and ensure inNum is positive
	bool isNegative = (inNum < 0);
	if (isNegative)
		inNum = -inNum;

	// Convert to chars - but backwards!
	s = outStr;
	number.quot = inNum;
	do {
		number = div(number.quot, 10);
		*s++ = '0' + number.rem;
	} while (number.quot != 0);
	if (isNegative)
		*s++ = '-';
	*s = kEndOfString;

	// Put it the right way around
	first = 0;
	last = Ustrlen(outStr) - 1;
	while (first < last)
	{
		ch = outStr[first];
		outStr[first++] = outStr[last];
		outStr[last--] = ch;
	}
	return noErr;
}


void
uitoa(ULong inNum, unsigned char * ioStr)
{
	unsigned char *	s = ioStr;
	do {
		*s++ = (inNum % 10) + '0';
	} while ((inNum /= 10) != 0);
	*s = kEndOfString;

	unsigned char	ch;
	int	first = 0;
	int	last = strlen((const char *)ioStr) - 1;
	while (first < last)
	{
		ch = ioStr[first];
		ioStr[first++] = ioStr[last];
		ioStr[last--] = ch;
	}
}


NewtonErr
IntegerStringSpec(long inNum, UniChar * outStr, ArrayIndex inStrLen, ULong inFmt)
{
	if ((inFmt & 0x0080) != 0)
		return NumberStringSpec(inNum, outStr, inStrLen, inFmt);

	unsigned char	str[32];
	bool	isNegative;

	if ((inFmt & kFormatPercent) != 0)
		inNum *= 100;
	isNegative = (inNum < 0);
	if (isNegative)
		inNum = -inNum;
	uitoa(inNum, str);
	return IntlNumberMunge((const char *)str, outStr, isNegative, 0, inStrLen, inFmt);
}


Ref
FFormattedNumberStr(RefArg inRcvr, RefArg inNum, RefArg inFmtStr)
{
	NewtonErr	err;
	RefVar		result;
	if (FIsFiniteNumber(inRcvr, inNum))
	{
		UniChar	str[64];
		if (IsString(inFmtStr))
		{
			char	fmtStr[16];
			ConvertFromUnicode(GetUString(inFmtStr), fmtStr, 15);
			err = NumberString(CoerceToDouble(inNum), str, 63, fmtStr);
		}
		else
		{
			if (ISINT(inNum))
				err = IntegerStringSpec(RINT(inNum), str, 63, RINT(inFmtStr));
			else
				err = NumberStringSpec(CoerceToDouble(inNum), str, 63, RINT(inFmtStr));
		}
		if (err == noErr)
			result = MakeString(str);
		else if (err == -2)
			result = Clone(RA(STRnTooLarge));
		else if (err == -3)
			result = Clone(RA(STRnTooSmall));
	}
	else
		result = Clone(RA(STRnan));
	return result;
}
