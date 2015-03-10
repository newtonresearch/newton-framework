/*
	File:		Locales.h

	Contains:	Locale declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__LOCALES_H)
#define __LOCALES_H 1

#include "Lookup.h"

NewtonErr	InitInternationalUtils(void);
Ref			IntlResources(void);
Ref			GetCurrentLocale(void);
Ref			GetLocaleSlot(RefArg inLocale, RefArg inTag);
Ref			GetLocaleSlot(RefArg inTag);

NewtonErr	IntlNumberMunge(const char * inStr, UniChar * outStr, bool inIsNegative, ArrayIndex inNumOfDecimals, ArrayIndex inStrLen, ULong inFmt);
NewtonErr	IntegerString(int inNum, UniChar * outStr);

UniChar *	PositiveNumberProtoStr(void);
UniChar *	NegativeNumberProtoStr(void);
UniChar *	PositiveIntProtoStr(void);
UniChar *	NegativeIntProtoStr(void);


#define kFormatCurrency	0x0010
#define kFormatGroups	0x0020
#define kFormatMinus		0x0040
#define kFormatPercent	0x0100


struct LocaleStrCache
{
	RefStruct	longDofWeek;		// +00
	RefStruct	abbrDofWeek;		// +04
	RefStruct	terseDofWeek;		// +08
	RefStruct	shortDofWeek;		// +0C
	RefStruct	longMonth;			// +10
	RefStruct	decimalPoint;		// +14
	RefStruct	groupSepStr;		// +18
	RefStruct	minusPrefix;		// +1C
	RefStruct	minusSuffix;		// +20
	RefStruct	currencyPrefix;	// +24
	RefStruct	currencySuffix;	// +28
};

extern LocaleStrCache *	gLocaleStrCache;

extern const UniChar		gSpaceStr[];
extern const UniChar		gZeroStr[];


#endif	/* __LOCALES_H */
