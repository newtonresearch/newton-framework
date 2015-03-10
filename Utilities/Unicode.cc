/*
	File:		Unicode.cc

	Contains:	Unicode <-> 8-bit encoding functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Globals.h"
#include "Locales.h"
#include "Sorting.h"
#include "UnicodePrivate.h"
#include "UStringUtils.h"
#include "RichStrings.h"
#include "ItemTester.h"


extern "C" {
Ref	FGetSortId(RefArg inRcvr, RefArg inArg2);
Ref	FSetSortId(RefArg inRcvr, RefArg inArg2);
}


/*------------------------------------------------------------------------------
	Generic text encoding data.
------------------------------------------------------------------------------*/

typedef 	struct
{
	EncodingMap *					map;
	ConvertFromUnicodeProcPtr	func;
} FromUnicodeConverter;

typedef 	struct
{
	EncodingMap *					map;
	ConvertToUnicodeProcPtr		func;
} ToUnicodeConverter;

enum _ctype
{
	k0,
	kWS,
	kDl,
	kNo,
	kUC,
	kLC,
};

/*------------------------------------------------------------------------------
	Platform character encoding data.
------------------------------------------------------------------------------*/

bool					gHasUnicode;				// 0C104EDC
bool					gUnicodeInited;			// 0C104EE0
const UniChar *	gASCIItoUnicodeTable;	// 0C104EE4
const char *		gASCIIBreakTable;			// 0C104EE8

struct CharacterEncodings
{
	FromUnicodeConverter	fromUnicode[kNumOfEncodings];
	ToUnicodeConverter	  toUnicode[kNumOfEncodings];
	ArrayIndex		count;
	const char *	charClass;
	void *			typeList;
	const char *	upperDeltas;
	const char *	lowerDeltas;
	const char *	upperNoMarkDeltas;
	const char *	noMarkDeltas;
} gUnicode;						// 0C107790

CSortTables gSortTables;	// 0C107800


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

Ref	InstallBuiltInEncodings(void);

/*------------------------------------------------------------------------------
	ASCII <-> Unicode character conversion function prototypes.
------------------------------------------------------------------------------*/

static void	subConvertToUnicode(const void * str, UniChar * ustr, int encoding, ArrayIndex numChars);
static void	ConvertToUnicodeFunc_Contiguous8(const void * inStr, UniChar * outStr, void * map, ArrayIndex numChars);

static void	subConvertFromUnicode(const UniChar * ustr, void * str, int encoding, ArrayIndex numChars);
static void	ConvertFromUnicodeFunc_Segmented16(const UniChar * inStr, void * outStr, void * map, ArrayIndex numOfChars);


/*------------------------------------------------------------------------------
	Initialization.
------------------------------------------------------------------------------*/

Ref
InitUnicode(void)
{
	RefVar		tables(gConstNSData->sortTables);
	ArrayIndex	count;
	if (NOTNIL(tables) && (count = Length(tables)) > 0)
	{
		for (ArrayIndex i = 0; i < count; ++i)
			gSortTables.addSortTable((const CSortingTable *)BinaryData(GetArraySlot(tables, i)), NO);
	}

	Ref	mp6 = MAKEMAGICPTR(6);
	Ref *	RSmp6 = &mp6;
	gASCIIBreakTable = BinaryData(RA(mp6));
	InstallBuiltInEncodings();
	gASCIItoUnicodeTable = (const UniChar *)gUnicode.toUnicode[kMacRomanEncoding].map->charCodes;

	gUnicode.charClass = BinaryData(GetFrameSlot(MAKEMAGICPTR(283), SYMA(charClass)));
	gUnicode.typeList = BinaryData(GetFrameSlot(MAKEMAGICPTR(283), SYMA(typeList)));
	gUnicode.upperDeltas = BinaryData(GetFrameSlot(MAKEMAGICPTR(283), SYMA(upperList)));
	gUnicode.lowerDeltas = BinaryData(GetFrameSlot(MAKEMAGICPTR(283), SYMA(lowerList)));
	gUnicode.upperNoMarkDeltas = BinaryData(GetFrameSlot(MAKEMAGICPTR(283), SYMA(upperNoMarkList)));
	gUnicode.noMarkDeltas = BinaryData(GetFrameSlot(MAKEMAGICPTR(283), SYMA(noMarkList)));

	gUnicodeInited = YES;
	gHasUnicode = YES;

	return tables;
}


void
AllocateEarlyStuff(void)
{
	RefVar	locale(GetCurrentLocale());
	Ref		sorting = GetProtoVariable(locale, SYMA(sortId));
	if (NOTNIL(sorting))
	{
		int sortId = RINT(sorting);
		if (sortId != 0)
			gSortTables.setDefaultTableId(sortId);
	}
}


/*------------------------------------------------------------------------------
	Install the built-in unicode encodings (defined in some magic pointers).
	Args:		--
	Return:	the built-in unicode encodings
------------------------------------------------------------------------------*/

Ref
InstallBuiltInEncodings(void)
{
	RefVar		encodings(GetFrameSlot(MAKEMAGICPTR(283), SYMA(charEncodings)));
	ArrayIndex	count = Length(encodings);
	for (ArrayIndex i = 0; i < count; ++i)
	{
		RefVar	encodingDef(GetArraySlot(encodings, i));
		UShort	encodingId = RINT(GetFrameSlot(encodingDef, SYMA(encodingId)));
		EncodingMap *	fromUnicodeMapData = (EncodingMap *)NewPtrClear(sizeof(EncodingMap));
		EncodingMap *	toUnicodeMapData = (EncodingMap *)NewPtrClear(sizeof(EncodingMap));
		ConvertFromUnicodeProcPtr fromUnicodeProc;
		ConvertToUnicodeProcPtr toUnicodeProc;
		GetMappingInfo(BinaryData(GetFrameSlot(encodingDef, SYMA(mapFromUnicode))), fromUnicodeMapData, (ProcPtr *)&fromUnicodeProc);
		GetMappingInfo(BinaryData(GetFrameSlot(encodingDef, SYMA(mapToUnicode))), toUnicodeMapData, (ProcPtr *)&toUnicodeProc);
		InstallCharEncoding(encodingId, fromUnicodeMapData, toUnicodeMapData, fromUnicodeProc, toUnicodeProc);
	}
	return encodings;
}


/*------------------------------------------------------------------------------
	Complete an EncodingMap record from a mapping table resource.
	Args:		data
					mapping table data
				map
					encoding map record to be completed
				mapFunction
					address of ProcPtr to be set
	Return:	--
------------------------------------------------------------------------------*/

void
GetMappingInfo(void * data, EncodingMap * outMap, ProcPtr * outMapFunction)
{
	UShort * table = (UShort *)data;

	outMap->formatID = *table++;

	if (outMap->formatID == 0)
	{
		outMap->unicodeTableSize = *table++;
		outMap->revision = *table++;
		outMap->tableInfo = *table++;

		outMap->charCodes = table;
		*outMapFunction = (ProcPtr) ConvertToUnicodeFunc_Contiguous8;
	}
	else if (outMap->formatID == 4)
	{
		outMap->revision = *table++;
		outMap->tableInfo = *table++;
		outMap->unicodeTableSize = *table++;

		outMap->endCodes = (UniChar *)table;
		table += outMap->unicodeTableSize;
		outMap->startCodes = (UniChar *)table;
		table += outMap->unicodeTableSize;
		outMap->codeDeltas = (SShort *)table;
		table += outMap->unicodeTableSize;
		outMap->charCodes = table;
		*outMapFunction = (ProcPtr) ConvertFromUnicodeFunc_Segmented16;
	}
	else
	{
		*outMapFunction = (ProcPtr) NULL;	// original uses -1 to flag unknown format
	}
}


/*------------------------------------------------------------------------------
	Install a character encoding scheme.
	Args:		encoding
					index of encoding scheme
				fromUnicodeMapData
					encoding map, unicode -> ascii - optional, mapping function may not need it
				toUnicodeMapData
					encoding map, ascii -> unicode - optional, mapping function may not need it
				fromUnicodeProc
					mapping function, unicode -> ascii
				toUnicodeProc
					mapping function, ascii -> unicode
	Return:	--
------------------------------------------------------------------------------*/

void
InstallCharEncoding(UShort encoding,
		EncodingMap * fromUnicodeMapData, EncodingMap * toUnicodeMapData, 
		ConvertFromUnicodeProcPtr fromUnicodeProc, ConvertToUnicodeProcPtr toUnicodeProc)
{
	if (encoding < kNumOfEncodings
	 && fromUnicodeProc != NULL
	 && toUnicodeProc != NULL)
	{
		gUnicode.fromUnicode[encoding].map = fromUnicodeMapData;
		gUnicode.fromUnicode[encoding].func = fromUnicodeProc;
		gUnicode.toUnicode[encoding].map = toUnicodeMapData;
		gUnicode.toUnicode[encoding].func = toUnicodeProc;
		gUnicode.count++;
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	Case conversion.
------------------------------------------------------------------------------*/

void
ConvertTextCase(UniChar * str, ArrayIndex len, const char * caseDeltas)
{
	UChar ch;
	UniChar unic;

	for (ArrayIndex i = 0; i < len; ++i)
	{
		if (*str == kEndOfString)
			return;
		unic = *str;
		ch = (unic < 0x80) ? unic : A_CONST_CHAR(unic);
		if (ch != kNoTranslationChar)
		{
			ch += caseDeltas[gUnicode.charClass[ch]];
			unic = (ch >= 0x80) ? U_CONST_CHAR(ch) : ch;
			*str++ = unic;
		}
	}
}

void
UpperCaseText(UniChar * str, ArrayIndex len)
{
	ConvertTextCase(str, len, gUnicode.upperDeltas);
}

void
UpperCaseNoDiacriticsText(UniChar * str, ArrayIndex len)
{
	ConvertTextCase(str, len, gUnicode.upperNoMarkDeltas);
}

void
LowerCaseText(UniChar * str, ArrayIndex len)
{
	ConvertTextCase(str, len, gUnicode.lowerDeltas);
}

#pragma mark -

/*------------------------------------------------------------------------------
	To Unicode conversion.
------------------------------------------------------------------------------*/

void
ConvertToUnicode(const void * str, UniChar * ustr, ArrayIndex numChars, CharEncoding encoding)
{
	if (gUnicodeInited)
	{
		if (gUnicode.toUnicode[encoding].func)
			gUnicode.toUnicode[encoding].func(str, ustr, gUnicode.toUnicode[encoding].map, numChars);
	}
	else
		subConvertToUnicode(str, ustr, encoding, numChars);
}


static void
subConvertToUnicode(const void * str, UniChar * ustr, int encoding, ArrayIndex numChars)
{
	char ch;
	const char * s = (const char *)str;
	for (ArrayIndex i = 0; i < numChars; ++i)
	{
		ch = *s++;
		if (ch == kEndOfString)
			break;
		*ustr++ = ch;
	}
	*ustr = kEndOfString;
}


static void
ConvertToUnicodeFunc_Contiguous8(const void * inStr, UniChar * outStr, void * map, ArrayIndex numChars)
{
	unsigned char ch;
	const unsigned char * p = (const unsigned char *)inStr;
	UniChar * s = outStr;
	for ( ; numChars > 0 && (ch = *p++) != kEndOfString; numChars--)
		*s++ = ((UniChar *)((EncodingMap *)map)->charCodes)[ch];
	*s = kEndOfString;
}


UniChar
U_CONST_CHAR(char ch)
{
	return gASCIItoUnicodeTable[(UChar)ch];
}

#pragma mark -

/*------------------------------------------------------------------------------
	From Unicode conversion.
------------------------------------------------------------------------------*/

void
ConvertFromUnicode(const UniChar * ustr, void * str, ArrayIndex numChars, CharEncoding encoding)
{
	if (gUnicodeInited)
	{
		if (gUnicode.fromUnicode[encoding].func)
			gUnicode.fromUnicode[encoding].func(ustr, str, gUnicode.fromUnicode[encoding].map, numChars);
	}
	else
		subConvertFromUnicode(ustr, str, encoding, numChars);
}


static void
subConvertFromUnicode(const UniChar * ustr, void * str, int encoding, ArrayIndex numChars)
{
	UniChar ch;
	char * s = (char *)str;
	for (ArrayIndex i = 0; i < numChars; ++i)
	{
		ch = *ustr++;
		if (ch == kEndOfString)
			break;
		if (ch >= 0x80)
			ch = kNoTranslationChar;
		*s++ = ch;
		
	}
	*s = kEndOfString;
}


static void
ConvertFromUnicodeFunc_Segmented16(const UniChar * inStr, void * outStr, void * map, ArrayIndex numOfChars)
{
	UniChar unic;
	unsigned char ch;
	unsigned char * p = (unsigned char *)outStr;
	for ( ; numOfChars > 0 && (unic = *inStr++) != kEndOfString; numOfChars--)
	{
		ch = kNoTranslationChar;
		for (ArrayIndex i = 0; i < ((EncodingMap *)map)->unicodeTableSize; ++i)
		{
			if ((unic >= ((EncodingMap *)map)->startCodes[i])
			 && (unic <= ((EncodingMap *)map)->endCodes[i]))
			{
				ch = ((unsigned char *)((EncodingMap *)map)->charCodes)[unic + ((EncodingMap *)map)->codeDeltas[i]];
				break;
			}
		}
		*p++ = ch;
	}
	*p = kEndOfString;
}


char
A_CONST_CHAR(UniChar unic)
{
	char ch;
	ConvertFromUnicode(&unic, &ch, 1);
	return ch;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S o r t i n g T a b l e
------------------------------------------------------------------------------*/

ArrayIndex
CSortingTable::calcSize() const
{
	ArrayIndex  r2 = f20*6 + f26*8 + f2A*2 + 0x44;
	for (ArrayIndex i = 0; i < f06; ++i)
		/*r2 += (f08[i*4].f02 - f08[i*4].f00 + 1)*4*/;
	return r2;
}


void
CSortingTable::convertTextToLowestSort(UniChar * inText, ArrayIndex inCount) const
{
	UniChar				sortedCh;
	ProjectionEntry * projection;

	while (inCount-- > 0)
	{
		if ((projection = getProjectionEntry(*inText)) != NULL)
		{
/*
			if (projection->f00 < f2A)
				*inText = this + f28 + 0x44;
			else
				sortedCh = ;
			*inText = sortedCh;
*/
		}
		inText++;
	}
}


ProjectionEntry *
CSortingTable::getProjectionEntry(UniChar inChar) const
{
// INCOMPLETE
	return NULL;
}


LigatureEntry *
CSortingTable::getLigatureEntry(UniChar inChar) const
{
/*
	LigatureEntry *	entry;
	for (entry = this + f24 + 0x44 ; entry->fChar != inChar; entry++)
		;
	return entry;
*/
	return NULL;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C S o r t T a b l e s
------------------------------------------------------------------------------*/

int
DefaultSortTableId(void)
{
	return gSortTables.fDefaultTableId;
}

const CSortingTable *
GetIndexSortTable(RefArg indexDesc)
{
	RefVar sortId(GetFrameSlot(indexDesc, SYMA(sortId)));
	if (NOTNIL(sortId))
		return gSortTables.getSortTable(RINT(sortId), NULL);
	return NULL;
}


Ref
FGetSortId(RefArg inRcvr, RefArg inArg2)
{
	int	sortId = DefaultSortTableId();
	if (sortId != 0
	&&  ISNIL(inArg2))
		return MAKEINT(sortId);
	return NILREF;
}

Ref
FSetSortId(RefArg inRcvr, RefArg inId)
{
	int	sortId;
	if (ISNIL(inId))
		sortId = 0;
	else if (ISINT(inId))
		sortId = RINT(inId);
	else
		return NILREF;

	return MAKEBOOLEAN(sortId == DefaultSortTableId()
						 || gSortTables.setDefaultTableId(sortId));
}


bool
CSortTables::setDefaultTableId(int inTableId)
{
	const CSortingTable *	sortTable = getSortTable(inTableId, NULL);
	bool  isValid = (sortTable != NULL || inTableId == 0);
	if (isValid)
	{
		fDefaultTableId = inTableId;
		fDefaultTable = sortTable;
	}
	return isValid;
}


bool
CSortTables::addSortTable(const CSortingTable * inTable, bool inOwned)
{
	const CSortingTable *	sortTable = getSortTable(inTable->fId, NULL);
	if (sortTable != NULL)
		return NO;

	TableEntry *	entry = fEntry;
	for (ArrayIndex i = 0; i < kNumOfEncodings; ++i, ++entry)
	{
		if (entry->fSortingTable == NULL)
		{
			entry->fSortingTable = inTable;
			entry->fOwnedByUs = inOwned;
			entry->fRefCount = 1;
			return YES;
		}
	}

	OutOfMemory();
	return NO;
}


const CSortingTable *
CSortTables::getSortTable(int inTableId, size_t * outSize) const
{
	if (inTableId == fDefaultTableId)
	{
		if (outSize)
			*outSize = fDefaultTable->calcSize();
		return fDefaultTable;
	}

	TableEntry *	entry;
	if ((entry = getTableEntry(inTableId)) != NULL)
	{
		const CSortingTable *	sortTable = entry->fSortingTable;
		if (outSize)
			*outSize = sortTable->calcSize();
		return sortTable;
	}

	return NULL;
}


CSortTables::TableEntry *
CSortTables::getTableEntry(int inTableId) const
{
	const CSortingTable *	sortTable;
	TableEntry *	entry = (TableEntry *)fEntry;
	for (ArrayIndex i = 0; i < kNumOfEncodings; ++i, ++entry)
		if ((sortTable = entry->fSortingTable) != NULL && sortTable->fId == inTableId)
			return entry;
	return NULL;
}


void
CSortTables::subscribe(int inTableId)
{
	TableEntry *	entry;
	if ((entry = getTableEntry(inTableId)) != NULL)
		entry->fRefCount++;
}


void
CSortTables::unsubscribe(int inTableId)
{
	TableEntry *	entry;
	if ((entry = getTableEntry(inTableId)) != NULL)
	{
		entry->fRefCount--;
		if (entry->fRefCount == 0)
		{
			if (entry->fOwnedByUs)
				free(entry);
			entry->fSortingTable = NULL;
		}
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	Comparison functions.
------------------------------------------------------------------------------*/

class CStringToSort
{
public:
				CStringToSort(const UniChar * inStr, ArrayIndex inLen, const CSortingTable * inSorter);

	bool		fetch(void);
	UniChar  project(char);
	UniChar  secondOrderProject(void) const;

	friend	int CalcSecondOrderResult(const CStringToSort & inStr1, const CStringToSort & inStr2);

//private:
	const CSortingTable *	f00;
	const UniChar *	fString;		// +04
	ArrayIndex			fLength;		// +08
	int					f0C;
	UniChar				f10;
	UniChar				f12;
	UniChar				f14;
	char					f16;
	char					f17;
};

CStringToSort::CStringToSort(const UniChar * inStr, ArrayIndex inLen, const CSortingTable * inSorter)
{
	f00 = inSorter;
	fString = inStr;
	fLength = inLen;
	f0C = 0;
	f12 = 0;
	f16 = 0;
	f17 = 0;
}

bool
CStringToSort::fetch(void)
{
	f17 = 0;

	UniChar  r1 = f12;
	if (r1 != 0)
	{
		f10 = r1;
		f12 = 0;
		return YES;
	}

	if (fLength == 0)
		return NO;

	f10 = *fString++;
	fLength--;
	return YES;
}

UniChar
CStringToSort::project(char inArg1)
{
	ProjectionEntry * projection = f00->getProjectionEntry(f10);
	if (projection == NULL)
		return f10;

	if (projection->f00 == 0xFFFF)
	{
		LigatureEntry * ligature = f00->getLigatureEntry(f10);
		f10 = ligature->f02;
		f12 = ligature->f04;
		if (inArg1 == 0)
			f16 = 0;
		return project(inArg1);
	}

	f17 = (projection->f00 == 0) ? 1 : 0;
	if (inArg1 == 0)
		f14 = f10;
	return projection->f00;
}

UniChar
CStringToSort::secondOrderProject(void) const
{
	ProjectionEntry * projection = f00->getProjectionEntry(f14);
	if (projection == NULL)
		return f14;
	return projection->f02;
}


int
CalcSecondOrderResult(const CStringToSort & inStr1, const CStringToSort & inStr2)
{
	int	delta = inStr1.secondOrderProject() - inStr2.secondOrderProject();
	if (delta == 0)
		return inStr1.f16 - inStr2.f16;
	return (delta <= 0) ? kItemLessThanCriteria : kItemGreaterThanCriteria;
}

#pragma mark -

int
OldCompareText(const UniChar * s1, ArrayIndex len1, const UniChar * s2, ArrayIndex len2,
					bool doDiacritical, CompareProcPtr cmp, void * refCon)
{
	UniChar	unich1, unich2;
	UChar		ch1, ch2;
	int		cmpResult = kItemEqualCriteria;
	ArrayIndex	i, count = (len1 > len2) ? len2 : len1;
	for (i = 0; i < count && cmpResult == kItemEqualCriteria; ++i)
	{
		unich1 = *s1++;
		unich2 = *s2++;
		if (cmp != NULL)
		{
			if (unich1 == kInkChar)
			{
				if (unich2 == kInkChar)
					cmpResult = cmp(i, i, refCon);
				else
					cmpResult = kItemGreaterThanCriteria;
				continue;
			}
			else if (unich2 == kInkChar)
			{
				cmpResult = kItemLessThanCriteria;
				continue;
			}
		}
		if (doDiacritical)
		{
			cmpResult = unich1 - unich2;
		}
		else
		{
			ch1 = (unich1 < 0x80) ? unich1 : A_CONST_CHAR(unich1);
			if (ch1 != kNoTranslationChar)
				ch1 += gUnicode.upperNoMarkDeltas[gUnicode.charClass[ch1]];
			ch2 = (unich2 < 0x80) ? unich2 : A_CONST_CHAR(unich2);
			if (ch2 != kNoTranslationChar)
				ch2 += gUnicode.upperNoMarkDeltas[gUnicode.charClass[ch2]];
			cmpResult = ch1 - ch2;
		}
	}
	if (cmpResult == kItemEqualCriteria)
		cmpResult = len1 - len2;
	return cmpResult;
}


int
CompareUnicodeText(const UniChar * s1, ArrayIndex len1, const UniChar * s2, ArrayIndex len2,
						 const CSortingTable * sorter, bool doDiacritical, CompareProcPtr cmp, void * refCon)
{
/*	r0 = s1;
	r12 = len1;
	r5 = s2;
	r4 = len2;
	r7 = doDiacritical;
	r9 = cmp;
	r10 = refCon; 
*/
	UniChar	unich1, unich2;
	int		cmpResult;

	if (sorter == (const CSortingTable *)1)
		sorter = gSortTables.fDefaultTable;
	if (sorter == NULL)
		return OldCompareText(s1, len1, s2, len2, doDiacritical, cmp, refCon);

	if (len1 == 0 || len2 == 0)
		return (int)len1 - (int)len2;

	CStringToSort  str1(s1, len1, sorter);  // sp18
	CStringToSort  str2(s2, len2, sorter);  // sp00
	bool  r4 = 0;
	bool  r8 = 0, r5;
	bool  r6 = 0, r0;

//L43
	for ( ; ; )
	{
		if (r8)
		{
			r5 = 1;
			r8 = 0;
		}
		else
			r5 = str1.fetch();

		if (r6)
		{
			r0 = 1;
			r6 = 0;
		}
		else
			r0 = str2.fetch();

//L56
		if (r5)
		{
			if (r0)
			{
//L97
				if (cmp != NULL)
				{
					unich1 = str1.f10;
					unich2 = str2.f10;
					if (unich1 == kInkChar)
					{
						if (unich2 != kInkChar)
							return kItemGreaterThanCriteria;
						cmpResult = cmp(str1.f0C - (int)str1.fLength - 1, str2.f0C - (int)str2.fLength - 1, refCon);
						if (cmpResult != kItemEqualCriteria)
							return cmpResult;
						continue;
					}
					else if (unich2 == kInkChar)
						return kItemLessThanCriteria;
				}
				unich1 = str1.f10;
				unich2 = str2.f10;
				if (unich1 != unich2)
				{
					int	projch1 = str1.project(r4);
					int	projch2 = str2.project(r4);
					if (projch1 == projch2)
						r4 = 1;
					else
					{
						if (str1.f17 == 0 && str2.f17 == 0)
							return (projch1 <= projch2) ? kItemLessThanCriteria : kItemGreaterThanCriteria;
						r8 = !str1.f17;
						r6 = !str2.f17;
					}
				}
				continue;
			}
		}
//L62
		else if (r0 == 0)
		{
			if (r4 && doDiacritical)
				return CalcSecondOrderResult(str1, str2);
			return kItemEqualCriteria;
		}

//L73
		if (r4 == 0)
			return (r5 == 0) ? kItemLessThanCriteria : kItemGreaterThanCriteria;

		CStringToSort *	str = (r5 == 0) ? &str2 : &str1;
		do
		{
			str->project(r4);
			if (!str->f17)
				return (r5 == 0) ? kItemLessThanCriteria : kItemGreaterThanCriteria;
		} while (str->fetch());
		if (doDiacritical)
			return CalcSecondOrderResult(str1, str2);
		return kItemEqualCriteria;
	}
	return kItemEqualCriteria;
}


int
CompareStringNoCase(const UniChar * s1, const UniChar * s2)
{
	return CompareUnicodeText(s1, Ustrlen(s1), s2, Ustrlen(s2));
}


int
CompareTextNoCase(const UniChar * s1, ArrayIndex len1, const UniChar * s2, ArrayIndex len2)
{
	return CompareUnicodeText(s1, len1, s2, len2);
}


UniChar *
FindString(UniChar * inStr, ArrayIndex inLen, UniChar * inStrToFind)
{
	ArrayIndex strToFindLen = Ustrlen(inStrToFind);
	for (UniChar * s = inStr, * end = inStr + ((int)inLen - (int)strToFindLen); s < end; ++s)
		if (CompareTextNoCase(s, strToFindLen, inStrToFind, strToFindLen) == 0)
			return s;
	return NULL;	// not found
}


UniChar *
FindWord(UniChar * inStr, ArrayIndex inLen, UniChar * inWord, bool inWordStart)
{
	if (IsDelimiter(*inWord))
		return FindString(inStr, inLen, inWord);

	ArrayIndex wordLen = Ustrlen(inWord);
	for (UniChar * s = inStr, * end = inStr + ((int)inLen - (int)wordLen); s < end; ++s)
	{
		if (IsDelimiter(*s))
			inWordStart = YES;
		else if (inWordStart)
		{
			if (CompareTextNoCase(s, wordLen, inWord, wordLen) == 0)
				return s;
			inWordStart = NO;
		}
	}
	return NULL;
}
