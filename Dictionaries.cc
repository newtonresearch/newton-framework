/*
	File:		Dictionaries.cc

	Contains:	Dictionary implementation.
					In a major departure from the original, we do not use Handles.
					Dictionaries are always referenced by pointer.
					Also, although dictionaries can contain ASCII characters,
					we always pass in Unicode strings for lookup.
					Plus, we now use a CDictionary class to encapsulate dictionary functions.
					You get the idea: the original was very badly written.

	Written by:	Newton Research, 2010.

	Trie format:
				all tries have a header:
					UChar		'a' signature
					UChar		flags:4, type:4
				followed by the data:
					UChar		data[]
				which varies by trie type.
				For type 7, each data item contains:
					UChar			character; always lower case; could be unicode?
					UChar[1..3]	offset to alt character
									top 2 bits of first byte indicate size of offset
									00 => no alt character
*/

#include "Objects.h"
#include "Globals.h"
#include "Funcs.h"
#include "Dictionaries.h"
#include "ViewFlags.h"
#include "Locales.h"
#include "Unicode.h"
#include "UStringUtils.h"
#include "ROMResources.h"
#include "OSErrors.h"

#include "View.h"


/*------------------------------------------------------------------------------
	T y p e s
------------------------------------------------------------------------------*/

// dictionary data in ROM
struct ROMDictData
{
	ULong size;
	UByte data[];
};


// dictionary item in global dictionary list
struct DictListEntry
{
	CDictionary * dict;
	UByte seq;
	UByte status;
	bool isCustom;
};


// dictionary walking
struct DictWalkBlock
{
	CDictionary *	x00;
	int				x04;
	DictWalker		x08;
	void *			x0C;
	UniChar			x10[32];
};


/*------------------------------------------------------------------------------
	D a t a

constant kCommonDictionary := 0;
constant kSharedPropersDictionary := 1;
constant kLocalPropersDictionary := 2;
constant kFirstNamesDictionary := 48;
constant kLastNamesDictionary := 7;
constant kCountriesDictionary := 8;
constant kUserDictionary := 31;
constant kDaysMonthsDictionary := 34;
constant kLocalCitiesDictionary := 41;
constant kLocalCompaniesDictionary := 42;
constant kLocalStatesDictionary := 43;
constant kLocalStatesAbbrevsDictionary := 44;
constant kLocalDateDictionary := 110;
constant kLocalTimeDictionary := 111;
constant kLocalMoneyDictionary := 112;
constant kLocalNumberDictionary := 113;

constant kLocalPhoneDictionary := 112;
constant kWorldPhoneDictionary := 114;
constant kSimplePhoneDictionary := 115;
constant kPostalCodeDictionary := 116;
constant kNumbersOnlyDictionary := 117;
constant kMoneyOnlyDictionary := 118;
constant kSimpleDateDictionary := 119;
constant kWorldPostalCodeDictionary := 120;
constant kAddressAbbrevsDictionary := 121;

------------------------------------------------------------------------------*/
#define kEnum80DaysMonths (0x006853dc - 0x006853DC)
#define kEnum80Empty (0x00685500 - 0x006853DC)
#define kEnum80Letters (0x00685508 - 0x006853DC)
#define kEnum80loc_words (0x00685544 - 0x006853DC)
#define kEnum80prefixes (0x0068c948 - 0x006853DC)
#define kEnum80sh_city (0x0068cc50 - 0x006853DC)
#define kEnum80sh_country (0x00691e40 - 0x006853DC)
#define kEnum80sh_first (0x00692a4c - 0x006853DC)
#define kEnum80sh_honorific (0x00697388 - 0x006853DC)
#define kEnum80sh_state (0x00697ef4 - 0x006853DC)
#define kEnum80sh_words (0x0069943c - 0x006853DC)
#define kEnum80spellcheck_ignore (0x006f7d60 - 0x006853DC)
#define kEnum80suffixes (0x0070881c - 0x006853DC)
#define kEnum80us_words (0x00708fc4 - 0x006853DC)
#define kEnum81IAWordList (0x0070a788 - 0x006853DC)
#define kEnum81sh_address (0x0070ab14 - 0x006853DC)
#define kEnum81sh_common_freq_fwd (0x0070ac74 - 0x006853DC)
#define kLex8FunnyPhone (0x0070c220 - 0x006853DC)
#define kLex8IDNumbers (0x0070c3c4 - 0x006853DC)
#define kLex8WorldPhone (0x0070c538 - 0x006853DC)
#define kLex8calcsheet (0x0070c63c - 0x006853DC)
#define kLex8closequote (0x0070d8e8 - 0x006853DC)
#define kLex8date (0x0070d914 - 0x006853DC)
#define kLex8daymonth (0x00711c9c - 0x006853DC)
#define kLex8endpunct (0x00711f90 - 0x006853DC)
#define kLex8hyphen (0x00711fd0 - 0x006853DC)
#define kLex8money (0x00711ff4 - 0x006853DC)
#define kLex8moneyCFr (0x00714d00 - 0x006853DC)
#define kLex8numbers (0x007173f0 - 0x006853DC)
#define kLex8numbersCFr (0x007180b8 - 0x006853DC)
#define kLex8openquote (0x00718d7c - 0x006853DC)
#define kLex8phone (0x00718dac - 0x006853DC)
#define kLex8postcode (0x00719b4c - 0x006853DC)
#define kLex8ssn (0x00719c4c - 0x006853DC)
#define kLex8symbolsPart1 (0x00719c9c - 0x006853DC)
#define kLex8symbolsPart2 (0x00719d0c - 0x006853DC)
#define kLex8time (0x00719d64 - 0x006853DC)
#define kLex8wordlike (0x0071a7a4 - 0x006853DC)
#define kSymb80SwedishSymbols (0x0071a864 - 0x006853DC)
#define kSymb80Symbols (0x0071a8e4 - 0x006853DC)

extern char *	gDictData;					// dictionary data read from resource file

extern CDictionary *	gTrie;

char				huh[256];

NewtonErr		airusResult;				// 0C100810
// result codes:
//	5 => empty string

int				g0C100814;
char *			g0C100818 = huh;
UniChar *		g0C10081C = (UniChar *)huh;
ULong				g0C100824;
char				gNoName[4] = {0,0,0,0};	// 0C100828
ULong				g0C100834[4] = {0x3F,0x3F,0x7F,0x7F};	// don’t know how this gets initialised
ULong				g0C100844[4] = {2,2,3,4};	// don’t know how this gets initialised

CDictionary *	gTimeLexDictionary;		// +08	0C100F8C
CDictionary *	gDateLexDictionary;		// +0C	0C100F90
CDictionary *	gPhoneLexDictionary;		// +10	0C100F94
CDictionary *	gNumberLexDictionary;	// +14	0C100F98

CDArray *		gDictList;					// 0C10162C
ULong				gNextCustomDictionaryId;// 0C101650

//char			g0C105AF4[256];
UniChar			gStrBuf[256];				// 0C105BF4

ROMDictData *	gROMDictionaryData[129];	// 0C106840 -> 0C106A44


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
// for @171 protoDictionary
Ref	FAirusResult(RefArg inRcvr);
Ref	FAirusNew(RefArg inRcvr, RefArg inWordSize, RefArg inAttrSize);
Ref	FAirusDispose(RefArg inRcvr);
Ref	FAirusRegisterDictionary(RefArg inRcvr);
Ref	FAirusUnregisterDictionary(RefArg inRcvr);
Ref	FAirusDictionaryType(RefArg inRcvr);
Ref	FAirusAddWord(RefArg inRcvr, RefArg inWord, RefArg inAttribute);
Ref	FAirusDeleteWord(RefArg inRcvr, RefArg inWord);
Ref	FAirusDeletePrefix(RefArg inRcvr, RefArg inPrefix);
Ref	FAirusLookupWord(RefArg inRcvr, RefArg inWord, RefArg inAttribute);
Ref	FAirusWalkDictionary(RefArg inRcvr, RefArg inWord, RefArg inArg2);
Ref	FAirusAttributeSize(RefArg inRcvr);
Ref	FAirusChangeAttribute(RefArg inRcvr, RefArg inWord, RefArg inAttribute);

// for @345 protoDictionaryCursor
Ref	FAirusIteratorMake(RefArg inRcvr);
Ref	FAirusIteratorClone(RefArg inRcvr);
Ref	FAirusIteratorDispose(RefArg inRcvr);
Ref	FAirusIteratorReset(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3);
Ref	FAirusIteratorThisWord(RefArg inRcvr, RefArg inEntry);
Ref	FAirusIteratorNextWord(RefArg inRcvr);
Ref	FAirusIteratorPreviousWord(RefArg inRcvr);

Ref	FGetRandomWord(RefArg inRcvr, RefArg inMinLength, RefArg inMaxLength);
}


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" Ref	FSetRemove(RefArg inRcvr, RefArg ioArray, RefArg inMember);

void			InitROMDictionaryData(void);
void *		GetROMDictionaryData(ULong inDictNumber, size_t * outSize);

ULong			CountCustomDictionaries(RefArg inContext);
ULong			GetCustomDictionary(RefArg inContext, ArrayIndex index);


#pragma mark -
/*------------------------------------------------------------------------------
	I n i t i a l i s a t i o n
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create all the dictionaries.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
InitDictionaries(void)
{
	InitROMDictionaryData();

	// create global list of dictionaries
	CDictionary * dict;
	RefVar aDictionary;
	RefVar dictionaries(Clone(RA(dictionaryList)));
	SetFrameSlot(gVarFrame, SYMA(dictionaries), dictionaries);

	// populate that list with data
	gDictList = CDArray::make(sizeof(DictListEntry), 0);
	for (ArrayIndex i = 0, count = Length(dictionaries); i < count; ++i)
	{
		// create a mutable dictionary frame using the original list item as the _proto
		aDictionary = GetArraySlot(dictionaries, i);
		int dictId = RINT(GetProtoVariable(aDictionary, SYMA(dictId)));
		Ref romDictId = GetProtoVariable(aDictionary, SYMA(ROMdictId));
		RefVar dictFrame(Clone(RA(canonicalDictRAMFrame)));
		SetFrameSlot(dictFrame, SYMA(_proto), aDictionary);
		SetFrameSlot(dictFrame, SYMA(ROMdictId), romDictId);
		SetArraySlot(dictionaries, i, dictFrame);

		// create CDictionary instance encapsulating the dictionary data
		dict = NULL;
		if (NOTNIL(romDictId))
		{
			size_t	dictSize;
			void *	dictData = GetROMDictionaryData(RINT(romDictId), &dictSize);
			int		dictType = RINT(GetProtoVariable(dictFrame, SYMA(dictType)));
			if (dictType == 0 || dictType == 1 || dictType == 4)
			{
				dict = new CDictionary(dictData, dictSize);
//				dict->x4C = 1;
			}
		}
		else if (dictId == 31 || dictId == 35 || dictId == 36)	// user, expand, auto-add dictionaries
		{
			dict = new CDictionary(15, 1);
		}
		else if (dictId == 32)	// Dark Star
			dict = gTrie;

		// put CDictionary into the frame
		if (dict)
			dict->setId(dictId & 0xFFFF);
		SetFrameSlot(dictFrame, SYMA(dict), dict ? AddressToRef(dict) : NILREF);

		// also build chain of CDictionaries
		DictListEntry d;
		d.dict = dict;
		d.seq = i;
		d.status = RINT(GetProtoVariable(dictFrame, SYMA(status)));
		d.isCustom = false;
		memcpy(gDictList->addEntry(), &d, sizeof(DictListEntry));
	}
	gDictList->compact();

	// create localized dictionaries
	aDictionary = GetLocaleSlot(RA(NILREF), SYMA(timeDictionary));
	if (NOTNIL(aDictionary) && (dict = new CDictionary(aDictionary)) != NULL)
	{
//		dict->x4C = 1;
		gTimeLexDictionary = dict;
	}

	aDictionary = GetLocaleSlot(RA(NILREF), SYMA(dateDictionary));
	if (NOTNIL(aDictionary) && (dict = new CDictionary(aDictionary)) != NULL)
	{
//		dict->x4C = 1;
		gDateLexDictionary = dict;
	}

	aDictionary = GetLocaleSlot(RA(NILREF), SYMA(phoneDictionary));
	if (NOTNIL(aDictionary) && (dict = new CDictionary(aDictionary)) != NULL)
	{
//		dict->x4C = 1;
		gPhoneLexDictionary = dict;
	}

	aDictionary = GetLocaleSlot(RA(NILREF), SYMA(numberDictionary));
	if (NOTNIL(aDictionary) && (dict = new CDictionary(aDictionary)) != NULL)
	{
//		dict->x4C = 1;
		gNumberLexDictionary = dict;
	}
}


/*------------------------------------------------------------------------------
	Create the dictionaries based on ROM data.
	Recreating the source for the Newton dictionaries would be an interesting
	exercise, but very time consuming.
	So we use the dictionary data from the ROM image:
		start: 0x006863DC (6841308), length: 0x00095580 (611712)
	It can be extracted with the command line:
		tail -c +6841309 "ROM Image" | head -c 611712 > DictData
	This data file is read at init time into a malloc’d block
	and we point into that.
	The offsets are generated from the ROM symbols file using grep:
		(.*)g(.*)$
		#define k\2 (0x\1 - 0x006853DC)
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
InitROMDictionaryData(void)
{
	gROMDictionaryData[4] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[5] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[6] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[9] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[10] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[11] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[12] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[13] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[14] = (ROMDictData *)(gDictData + kEnum80Empty);

	gROMDictionaryData[15] = (ROMDictData *)(gDictData + kLex8phone);
	gROMDictionaryData[16] = (ROMDictData *)(gDictData + kLex8time);
	gROMDictionaryData[105] = (ROMDictData *)(gDictData + kLex8FunnyPhone);
	gROMDictionaryData[106] = (ROMDictData *)(gDictData + kLex8WorldPhone);
	gROMDictionaryData[107] = (ROMDictData *)(gDictData + kLex8postcode);
	gROMDictionaryData[109] = (ROMDictData *)(gDictData + kLex8calcsheet);
	gROMDictionaryData[110] = (ROMDictData *)(gDictData + kLex8closequote);
	gROMDictionaryData[111] = (ROMDictData *)(gDictData + kLex8daymonth);
	gROMDictionaryData[112] = (ROMDictData *)(gDictData + kLex8endpunct);
	gROMDictionaryData[113] = (ROMDictData *)(gDictData + kLex8hyphen);
	gROMDictionaryData[114] = (ROMDictData *)(gDictData + kLex8IDNumbers);
	gROMDictionaryData[115] = (ROMDictData *)(gDictData + kLex8money);
	gROMDictionaryData[120] = (ROMDictData *)(gDictData + kLex8moneyCFr);
	gROMDictionaryData[116] = (ROMDictData *)(gDictData + kLex8numbers);
	gROMDictionaryData[121] = (ROMDictData *)(gDictData + kLex8numbersCFr);
	gROMDictionaryData[117] = (ROMDictData *)(gDictData + kLex8openquote);
	gROMDictionaryData[118] = (ROMDictData *)(gDictData + kLex8ssn);
	gROMDictionaryData[119] = (ROMDictData *)(gDictData + kLex8symbolsPart1);
	gROMDictionaryData[128] = (ROMDictData *)(gDictData + kLex8symbolsPart2);
	gROMDictionaryData[108] = (ROMDictData *)(gDictData + kLex8wordlike);
	gROMDictionaryData[7] = (ROMDictData *)(gDictData + kLex8date);
	gROMDictionaryData[8] = (ROMDictData *)(gDictData + kLex8date);

	gROMDictionaryData[0] = (ROMDictData *)(gDictData + kEnum80Letters);
	gROMDictionaryData[1] = (ROMDictData *)(gDictData + kSymb80Symbols);

	gROMDictionaryData[126] = (ROMDictData *)(gDictData + kSymb80SwedishSymbols);
	gROMDictionaryData[124] = (ROMDictData *)(gDictData + kLex8date);
	gROMDictionaryData[2] = (ROMDictData *)(gDictData + kEnum81IAWordList);
	gROMDictionaryData[125] = (ROMDictData *)(gDictData + kLex8date);
	gROMDictionaryData[122] = (ROMDictData *)(gDictData + kEnum80suffixes);
	gROMDictionaryData[123] = (ROMDictData *)(gDictData + kEnum80prefixes);
	gROMDictionaryData[3] = (ROMDictData *)(gDictData + kEnum80DaysMonths);
	gROMDictionaryData[127] = (ROMDictData *)(gDictData + kEnum80spellcheck_ignore);
	gROMDictionaryData[21] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[19] = (ROMDictData *)(gDictData + kEnum81sh_common_freq_fwd);
	gROMDictionaryData[97] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[18] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[20] = (ROMDictData *)(gDictData + kEnum80Empty);
	gROMDictionaryData[28] = (ROMDictData *)(gDictData + kEnum81sh_address);
	gROMDictionaryData[22] = (ROMDictData *)(gDictData + kEnum80sh_first);
	gROMDictionaryData[29] = (ROMDictData *)(gDictData + kEnum80sh_city);
	gROMDictionaryData[24] = (ROMDictData *)(gDictData + kEnum80sh_honorific);
	gROMDictionaryData[98] = (ROMDictData *)(gDictData + kEnum80sh_honorific);
	gROMDictionaryData[30] = (ROMDictData *)(gDictData + kEnum80sh_country);
	gROMDictionaryData[31] = (ROMDictData *)(gDictData + kEnum80sh_state);
	gROMDictionaryData[99] = (ROMDictData *)(gDictData + kEnum80sh_words);
	gROMDictionaryData[100] = (ROMDictData *)(gDictData + kEnum80sh_words);
	gROMDictionaryData[80] = (ROMDictData *)(gDictData + kEnum80us_words);

	gROMDictionaryData[48] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[102] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[32] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[101] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[64] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[103] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[23] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[25] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[26] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[27] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[96] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[17] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[33] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[104] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[34] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[35] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[36] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[37] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[38] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[39] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[40] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[41] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[43] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[44] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[47] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[42] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[45] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[49] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[46] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[50] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[51] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[52] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[53] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[54] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[55] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[56] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[57] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[59] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[60] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[63] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[58] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[61] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[65] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[62] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[66] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[67] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[68] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[69] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[70] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[71] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[72] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[73] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[75] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[76] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[79] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[74] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[77] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[81] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[78] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[82] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[83] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[84] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[85] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[86] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[87] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[88] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[89] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[91] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[92] = (ROMDictData *)(gDictData + kEnum80loc_words);
	gROMDictionaryData[95] = (ROMDictData *)(gDictData + kEnum80loc_words);

	gROMDictionaryData[93] = (ROMDictData *)(gDictData + kEnum80sh_state);
	gROMDictionaryData[90] = (ROMDictData *)(gDictData + kEnum80sh_city);
	gROMDictionaryData[94] = (ROMDictData *)(gDictData + kEnum80sh_state);
}


Ref
Dictionaries(void)
{
	return GetFrameSlot(gVarFrame, SYMA(dictionaries));
}


DictListEntry *
FindDictionaryEntry(ULong inId)
{
	RefVar dicts(Dictionaries());
	RefVar aDict;

	switch (inId)
	{
	case 1:
	case 7:
	case 9:
	case 20:
	case 42:
		inId = 6;
	case 2:
		inId = 3;
	case 10:
		inId = 34;
	case 13:
	case 41:
		inId = 24;
		break;
	}

	for (ArrayIndex i = 0, count = gDictList->count(); i < count; ++i)
	{
		DictListEntry * entry = (DictListEntry *)gDictList->getEntry(i);
		aDict = GetArraySlot(dicts, entry->seq);
		if (RINT(GetProtoVariable(aDict, SYMA(dictId))) == inId)
			return entry;
	}
	return NULL;
}


void
AddToChain(CDictChain ** outChains, DictListEntry * inEntry)
{
	RefVar aDict;
	for (ArrayIndex i = 0; i < 100; ++i)
	{
		aDict = GetArraySlot(Dictionaries(), inEntry->seq);
		int dictIndex;
		int dictType = RINT(GetProtoVariable(aDict, SYMA(dictType)));
		if (dictType == 0)
			dictIndex = 0;
		else if (dictType == 1)
			dictIndex = 1;
		else if (dictType == 4)
			dictIndex = 2;
		else
			XFAIL(true)

		if (inEntry->dict)
		{
			CDictChain * dictChain;
			bool exists = false;
			if ((dictChain = outChains[dictIndex]) == NULL)
			{
				XFAIL((dictChain = CDictChain::make(0)) == NULL)
				outChains[dictIndex] = dictChain;
			}
			else
			{
				for (ArrayIndex j = 0; j < dictChain->count(); j++)
				{
					DictListEntry * entry = (DictListEntry *)dictChain->getEntry(j);
					if (entry->dict == inEntry->dict)
					{
						exists = true;
						break;
					}
				}
			}
			if (!exists)
			{
				DictListEntry * entry = (DictListEntry *)dictChain->addEntry();
				entry->dict = inEntry->dict;
			}
		}

		RefVar linkedDictId(GetProtoVariable(aDict, SYMA(linkedDictId)));
		if (NOTNIL(linkedDictId))
		{
			DictListEntry * entry = FindDictionaryEntry(RINT(linkedDictId));
			XFAIL(entry == NULL || entry->seq == inEntry->seq)
		}
	}
}


void
BuildChains(CDictChain ** outChains, RefArg inContext)
{
	ULong inputMask = RINT(GetVariable(inContext, SYMA(inputMask)));
	ArrayIndex count;
	for (ArrayIndex i = 0; i < 3; ++i)
		outChains[i] = NULL;
	count = CountCustomDictionaries(inContext);
	for (ArrayIndex i = 0; i < count; ++i)
	{
		DictListEntry * entry = FindDictionaryEntry(GetCustomDictionary(inContext, i));
		if (entry)
			AddToChain(outChains, entry);
	}
	RefVar sp00;
	count = gDictList->count();
	for (ArrayIndex i = 0; i < count; ++i)
	{
		DictListEntry * entry = (DictListEntry *)gDictList->getEntry(i);
		if (entry && entry->dict != NULL && entry->status != 0 && !entry->isCustom)
		{
			sp00 = GetArraySlot(Dictionaries(), entry->seq);
			ULong flags = RINT(GetProtoVariable(sp00, SYMA(domainType))) & vDictionariesAllowed;
			if ((flags & inputMask) != 0)
				AddToChain(outChains, entry);
		}
	}
}


void
CompactChains(CDictChain ** inChains)
{
	for (ArrayIndex i = 0; i < 3; ++i)
	{
		CDictChain * dChain = inChains[i];
		if (dChain != NULL)
			dChain->compact();
	}
}


void
DoneChains(CDictChain ** inChains)
{
	for (ArrayIndex i = 0; i < 3; ++i)
	{
		CDictChain * dChain = inChains[i];
		if (dChain != NULL)
		{
			delete dChain;
			inChains[i] = 0;
		}
	}
}


ULong
CountCustomDictionaries(CView * inView)
{
	ULong recFlags = inView->viewFlags & vRecognitionAllowed;
	if ((recFlags & vAnythingAllowed) == vAnythingAllowed
	||  (recFlags & vCustomDictionaries) == 0)
		return 0;

#if defined(forFramework)
	return 0;
#else
	RefVar dicts(inView->getVar(SYMA(dictionaries)));
	return NOTNIL(dicts) ? Length(dicts) : 0;
#endif
}


ULong
CountCustomDictionaries(RefArg inContext)
{
	RefVar dicts(GetProtoVariable(inContext, SYMA(dictionaries)));
	return NOTNIL(dicts) ? Length(dicts) : 0;
}


ULong
GetCustomDictionary(RefArg inContext, ArrayIndex index)
{
	RefVar dicts(GetProtoVariable(inContext, SYMA(dictionaries)));
	if (ISINT(dicts))
		return RVALUE(dicts);
	return RINT(GetArraySlot(dicts, index));
}


void
DictionariesChanged(void)
{
#if 0
	RefVar assistant(GetFrameSlot(gRootView->fContext, SYMA(assistant)));
	if (NOTNIL(GetFrameSlot(assistant, SYMA(viewCObject))))
		DoMessage(GetFrameSlot(assistant, SYMA(assistLine)), SYMA(FindCustomDicts), RA(NILREF));
	PurgeAreaCache();
#endif
}


CDictionary *
GetScriptDictRef(RefArg inRcvr)
{
	Ref dictRef = GetFrameSlot(inRcvr, SYMA(dict));
	if (ISNIL(dictRef))
		ThrowMsg("dict not initialized");

	if (ISINT(dictRef))
		// dictionary already exists and this is its address
		return (CDictionary *)RefToAddress(dictRef);

	// create a new dictionary from a binary object
	return new CDictionary(dictRef);
}

#pragma mark -

/*------------------------------------------------------------------------------
	Dictionary construction.
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Construct an empty dictionary.
	Args:		inType
				inArg2
------------------------------------------------------------------------------*/

CDictionary::CDictionary(ULong inType, ULong inAttrLen)
{
	int n = inType & 0x07;

	fChunkSize = 100;
	fId = -1;
	x3C = 1;
	x38 = 0;
	fHead = this;
	fNext = NULL;
	x4C = 1;

	fTrieSize = 2;
	fTrie = (Trie *)NewPtr(2);
	if (fTrie)
	{
		fTrie->signature = 'a';
		fTrie->attrLen = inAttrLen;
		fTrie->type = inType;
		fTrieEnd = (UChar *)fTrie + fTrieSize;
		fAttrLen = inAttrLen;
		fAttr = 0;
		fNodeName = NULL;

		fCharSize = (n == 5 || n == 2) ? 2 : 1;
	}
	// else report faiure somehow
}


/*------------------------------------------------------------------------------
	Construct a  dictionary from existing trie data.
	Args:		inData
				inSize
------------------------------------------------------------------------------*/

CDictionary::CDictionary(void * inData, size_t inSize)
{
	init(inData, inSize);
}


/*------------------------------------------------------------------------------
	Construct a  dictionary from existing trie data in a binary object.
	Args:		inData
				inSize
------------------------------------------------------------------------------*/

CDictionary::CDictionary(RefArg inDict)
{
	CDataPtr dictData(inDict);
	init((Ptr)dictData, Length(inDict));
}


void
CDictionary::init(void * inData, size_t inSize)
{
	fChunkSize = 100;
	fId = 0;
	x3C = 1;
	x38 = 0;
	fHead = this;
	fNext = NULL;
	x4C = 1;

	fTrieSize = inSize;
	fTrie = (Trie *)inData;
	if (fTrieSize >= 2
	&&  fTrie->signature == 'a')
	{
		int n = fTrie->type & 0x07;
		if (n == 7 || n == 6 || n == 5 || n == 2 || n == 3 || n == 1)
		{
			fTrieEnd = (UChar *)fTrie + fTrieSize;
			fAttrLen = fTrie->attrLen;
			fAttr = 0;
			fNodeName = NULL;

			fCharSize = (n == 5 || n == 2) ? 2 : 1;
		}
	}
}


/*------------------------------------------------------------------------------
	Destroy a  dictionary.
	Problem:	fTrie may be ROM data!
------------------------------------------------------------------------------*/

CDictionary::~CDictionary()
{
	FreePtr((Ptr)fTrie);
	airusResult = noErr;
}


/*------------------------------------------------------------------------------
	Create a dictionary from ROM data referenced by number.
	Args:		inDictnumber
	Return:	dictionary
------------------------------------------------------------------------------*/

CDictionary *
GetROMDictionary(ULong inId)
{
	size_t		dictSize;
	void *		dictData = GetROMDictionaryData(inId, &dictSize);
	CDictionary *	dict = new CDictionary(dictData, dictSize);
	if (dict)
		dict->setId(inId);
	return dict;
}


/*------------------------------------------------------------------------------
	Return ROM dictionary data address, size given its number.
	Args:		inDictnumber
				outSize
	Return:	dictionary data
------------------------------------------------------------------------------*/

void *
GetROMDictionaryData(ULong inDictNumber, size_t * outSize)
{
	ROMDictData * data = gROMDictionaryData[inDictNumber];
	*outSize = data ? CANONICAL_LONG(data->size) : 0;
	return &data->data;
}

#pragma mark -

/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

void
CommonWord(UniChar * outWord, ArrayIndex inMinLen, ArrayIndex inMaxLen)
{
//	DictListEntry * dictEntry = FindDictionaryEntry(6);
//	GetRandomWord(dictEntry->dict, outWord, inMinLen, inMaxLen);
}


Ref
FGetRandomWord(RefArg inRcvr, RefArg inMinLength, RefArg inMaxLength)
{
	UniChar word[20+1];
	ArrayIndex minLen = RINT(inMinLength);
	ArrayIndex maxLen = RINT(inMaxLength);
	if (maxLen > 20)
		maxLen = 20;
	CommonWord(word, minLen, maxLen);
	return MakeString(word);
}

/*
void
CDictionary::getRandomWord(UniChar * outStr, ArrayIndex inMinLen, ArrayIndex inMaxLen)
{}
*/



CDictionary *
CDictionary::positionToHandle(ArrayIndex inSeq)
{
	CDictionary * theHandle = this;
	airusResult = noErr;
	for (ArrayIndex i = 0; i < inSeq; ++i)
	{
		theHandle = theHandle->fNext;
		if (theHandle == NULL)
		{
			airusResult = -12;
			break;
		}
	}
	return theHandle;
}


bool
CDictionary::hasActualOrImpliedAttr(void)
{
	return fTrie->attrLen != 0 || (fTrie->type & 0x07) == 3;		// was fAttrLen != 0
}


void
CDictionary::verifyStart(void)
{
	CDictionary * firstDict = positionToHandle(0);
	if (airusResult == noErr)
	{
		CDictionary * dict;
		for (dict = this; dict != NULL; dict = dict->fNext)
		{
			dict->x2C = 0;
			dict->fStr = gStrBuf;
			dict->fStrIndex = 0;
			dict->fHead = firstDict;
			if (dict == firstDict)
			{
				dict->fTrieOffset = 0;
				// original allows setting a string
			}
			else
				dict->fTrieOffset = 0;
		}
	}
}


NewtonErr
CDictionary::verifyCharacter(UChar * inStr, UChar ** outArg3, ULong ** outArg4, ULong ** outArg5, bool inArg6, UChar ** outArg7)
{ return 0; }


NewtonErr
CDictionary::verifyWord(UChar * inStr, UChar ** outArg3, ULong ** outArg4, ULong ** outArg5, bool inArg6, UChar ** outArg7)
{
	return verifyCharacter(inStr, outArg3, outArg4, outArg5, inArg6, outArg7);
}


NewtonErr
CDictionary::verifyString(UniChar * inStr, UniChar ** outArg3, ULong ** outArg4, char ** outArg5)
{
	ULong * attrPtr = &g0C100824;		// r6
	char * nodeNamePtr = gNoName;		// r7
	UniChar * r9 = g0C10081C;

	Ustrcpy(gStrBuf, inStr);		// don’t know why we need to copy the string
	fStr = gStrBuf;
	fStrLen = Ustrlen(gStrBuf);
	fStrIndex = 0;

	fTrieOffset = 0;			// lookup from the start of the trie
	despatch(2);

	long r0 = x30;
	if (r0 == -1)
		r9 = NULL;
	else
		*r9 = r0;

	long status = x2C;		// r0
	if (status == 0)
	{
		airusResult = 1;
		attrPtr = NULL;
		nodeNamePtr = NULL;
	}
	else if (status == 1)
	{
		airusResult = 2;
		if (hasActualOrImpliedAttr())
		{
			*attrPtr = fAttr;
			nodeNamePtr = fNodeName;
		}
		else
			attrPtr = NULL;
	}
	else if (status == 2)
	{
		airusResult = 3;
		if (hasActualOrImpliedAttr())
		{
			*attrPtr = fAttr;
			nodeNamePtr = fNodeName;
		}
		else
			attrPtr = NULL;
	}
	else if (status == 3)
	{
		airusResult = -6;
		attrPtr = NULL;
	}

	if (outArg3)	// terminalClass
		*outArg3 = r9;

	if (outArg4)	// attribute
		*outArg4 = attrPtr;

	if (outArg5)
		*outArg5 = nodeNamePtr;
}


#pragma mark -

NewtonErr
CDictionary::expand(size_t inDelta)
{
	NewtonErr err = noErr;
	size_t newSize;
	for (newSize = fTrieSize + inDelta; newSize > fTrieSize; fTrieSize += fChunkSize)
	{
		fTrie = (Trie *)realloc((Ptr)fTrie, fTrieSize + fChunkSize);
		if (MemError())
		{
			err = 2;
			break;
		}
	}
	x2C = err;
	return MemError();
}


#pragma mark -

ULong
CDictionary::readInt(ULong inOffset, int inLen)	// was GetDictBytes
{
	ULong value = 0;
	UChar * dictData = (UChar *)fTrie;
	switch (inLen)
	{
	case 4:
		value += (dictData[inOffset++] << 24);
	case 3:
		value += (dictData[inOffset++] << 16);
	case 2:
		value += (dictData[inOffset++] << 8);
	case 1:
		value += dictData[inOffset];
	}
	return value;
}


UShort
CDictionary::getSymbol(ULong inOffset)
{
	return readInt(inOffset, fCharSize);
}


ULong
CDictionary::RPByteSize(ULong inOffset)
{
	UChar * dictData = (UChar *)fTrie;
	int flags = (dictData[inOffset + fCharSize] & 0xC0) >> 6;
	if (flags == 2)
		return 1;
	if (flags == 3)
		return 3;
	return 0;
}


ULong
CDictionary::skipNode(ULong inOffset)
{
	return inOffset + fCharSize + RPByteSize(inOffset) + 1;
}


ULong
CDictionary::getAttr(ULong inOffset)
{
	if (fAttrLen > 0)
		return readInt(inOffset, fAttrLen);

	return ((type() & 0x07) == 3) ? 0x80 : 0;
}


ULong
CDictionary::followLeft(ULong inOffset)
{
	UChar * dictData = (UChar *)fTrie;
	if ((dictData[inOffset + fCharSize] & 0x10) != 0 && fAttrLen > 0)
		return skipNode(inOffset) + fAttrLen;
	return skipNode(inOffset);
}

#pragma mark -

void
CDictionary::despatch(int inSelector)
{
	switch (type() & 0x07)
	{
	case 1:	// 8-bit
	case 2:	// 16-bit
		switch (inSelector)
		{
		case 2:
			AL_verify();
			break;
		case 8:
			AL_nextSet();
			break;
		case 9:
			AL_nextSet9();
			break;
		}
		break;
	case 3:
	case 5:
	case 7:
		switch (inSelector)	//fAttr->type == 5|2 => 16-bit
		{
		case 0:
			startA();
			break;
		case 1:
			exitA();
			break;
		case 2:
			verify();
			break;
		case 3:
			addWord();
			break;
		case 4:
			deleteWord();
			break;
		case 5:
			firstLast();
			break;
		case 6:
			nextPrevious();
			break;
		case 7:
			changeAttribute();
			break;
		case 8:
			nextSet();
			break;
		case 9:
			nextSet9();
			break;
		}
		break;
	}
}


void
CDictionary::startA(void)
{
	x2C = 0;
}


void
CDictionary::exitA(void)
{
	x2C = 0;
}


void
CDictionary::verify(void)
{
	if (fTrieOffset == 0)
	{
		fStrIndex = 0;		// redundant?
		if (fTrieSize == 2)
		{
			// dictionary is empty
			x2C = 3;
			return;
		}
	}

	ULong offset = fTrieOffset;
	// zero offset => start of dictionary data which is actually after the header
	if (offset == 0)
		offset = 2;

	x30 = -1;

	UChar * dictData = (UChar *)fTrie + offset;	// r2
	for ( ; fStrIndex < fStrLen; fStrIndex++)
	{
		UChar flags = *(dictData + 1);					// r1
		UniChar ch = fStr[fStrIndex];
		ULong r7;
		while (ch > *dictData
			&&  (r7 = (flags & 0xC0) >> 6) != 0)
		{
			// character in string doesn’t match dictinoary character -- try alternative
			ULong r1;
			UChar * r8 = dictData + g0C100844[r7];
			if ((flags & 0x10) != 0)
				r8 += fAttrLen;
			if (r7 == 3)
			{
				r1 = ((flags & 0x0F) << 8) | dictData[2];
				r1 = (r1 << 8) | dictData[3];
				r1 = (r1 << 8) | dictData[4];
			}
			else
			{
				r1 = (flags << 8) | dictData[r7];
				r1 &= g0C100834[r7];
			}
			dictData = r8 + r1;
			flags = dictData[1];
		}

		if (ch != *dictData)
		{
			// no match
			x2C = 3;
			return;
		}

		offset = dictData - (UChar *)fTrie;
		fTrieOffset = offset;

		if ((flags & 0x20) != 0 && fStrIndex < (fStrLen-1))
		{
			// dictionary word has ended before input string
			x2C = 3;
			return;
		}

		// on to next letter in dictionary word
		dictData += g0C100844[(flags & 0xC0) >> 6];
		if ((flags & 0x10) != 0)
			dictData += fAttrLen;
	}

	long status = 0;
	dictData = (UChar *)fTrie;
	if ((dictData[offset + fCharSize] & 0x10) != 0)
	{
		fAttr = getAttr(skipNode(offset));
		status = 1;
	}

	if ((dictData[offset + fCharSize] & 0x20) != 0)
		status = 2;
	else
	{
		offset = followLeft(offset);
		if ((dictData[offset + fCharSize] & 0xC0) == 0)
			x30 = getSymbol(offset);
	}
	x2C = status;
}


void
CDictionary::addWord(void)
{
#if 0
//sp-208
	ULong sp00;
	UniChar sp04[256];
	UniChar * sp204;
	int r6 = 0;
	CheckDictPtrs();
	CopyBufferHack(fStr, sp04, 0);
	sp204 = sp04;
	ULong r0 = Ustrlen(sp204);
//	Ptr dictData = (UChar *)fTrie;
//	int n = dictData[1] & 0x07;
//	if (n == 5 || n == 2)
	NewtonErr err = expand(100 + fAttrLen + Ustrlen(sp204));
	if (err)
	{
		x2C = err;
		return;
	}
	x04 = 63;
	long r5 = FindInsertionPoint(&sp204, &sp00);
	if (*sp204 == kEndOfString)
	{
		;
	}
	else
	{
		r5 += PutWord(sp00, sp204);
		if (x04 != 63)
			FixupPointers(r5, 0);
		x2C = noErr;
		return;
	}
#endif
}

void
CDictionary::deleteWord(void)
{}

void
CDictionary::firstLast(void)
{}

void
CDictionary::nextPrevious(void)
{}

void
CDictionary::changeAttribute(void)
{}

void
CDictionary::nextSet(void)
{
	UniChar * sp00 = fStr;
	x50 = &sp00;
//	Ptr dictData = (UChar *)fTrie;
//	int n = dictData[1] & 0x07;
//	if (n == 5 || n == 2) treat inWord as unicode else ASCII
//	x54 = AE16_NextSetCB(unsigned long, unsigned long, unsigned long, unsigned long);
//	AE16_NextSet9();
	*sp00 = kEndOfString;
}

void
CDictionary::nextSet9(void)
{}

#pragma mark -

ULong
CDictionary::terminalCharsOffset(ULong inOffset)
{
	UChar * p = (UChar *)fTrie + inOffset;
	return (p[0] << 8) + p[1];
}

ULong
CDictionary::nextNodeOffset(ULong inOffset)
{
	UChar * p = (UChar *)fTrie + inOffset;
	return (p[3] << 8) + p[4];
}

void
CDictionary::AL_getAttribute(ULong inOffset)
{
	UChar * p = (UChar *)fTrie + inOffset;
	p += (((p[2] & 0x02) != 0) ? 3 : 5);

	switch (fAttrLen)
	{
	case 1:
		fAttr = p[0];
		break;
	case 2:
		fAttr = (p[0] << 8) + p[1];
		break;
	case 4:
		fAttr = (p[0] << 24) + (p[1] << 16) + (p[2] << 8) + p[3];
		break;
	default:
		fAttr = 0;
		break;
	}
}


void
CDictionary::AL_getAttribute2(ULong inOffset)
{
	char * s = (char *)fTrie + terminalCharsOffset(inOffset);
	fNodeName = s + strlen(s) + 1;
}


void
CDictionary::AL_verify(void)
{
	if (fTrieOffset == 0)
	{
		fStrIndex = 0;
		if (fTrieSize == 2)
		{
			// dictionary is empty
			x2C = 3;
			return;
		}
	}

	ULong offset = fTrieOffset;	// r5
	if (offset == 0)
		offset = 2;

	x30 = -1;
	UChar flags;

	for ( ; fStrIndex < fStrLen; fStrIndex++)
	{
		flags = *((UChar *)fTrie + offset+2);
		if ((flags & 0x02) != 0)
		{
			x2C = 3;
			return;
		}

		UniChar ch = fStr[fStrIndex];
		for (offset = nextNodeOffset(offset); ; offset = offset + (((flags & 0x02) != 0) ? 3 : 5) + fAttrLen)
		{
			UChar r1, * p = (UChar *)fTrie + terminalCharsOffset(offset);
			for (r1 = *p; r1 != 0; r1 = *++p)
			{
				if (r1 == ch)
					goto next1;
			}
			flags = *((UChar *)fTrie + offset+2);
			if ((flags & 0x04) != 0)
			{
				x2C = 3;
				return;
			}
		}
next1:
		fTrieOffset = offset;
		AL_getAttribute(offset);
		AL_getAttribute2(offset);
	}

	flags = *((UChar *)fTrie + offset+2);
	ULong r7 = (flags & 0x01) != 0 ? 1 : 0;
	if ((flags & 0x02) != 0)
	{
		x2C = 2;
		return;
	}

	ULong r6 = 0;

	for (offset = nextNodeOffset(offset); ; offset = offset + (((flags & 0x02) != 0) ? 3 : 5) + fAttrLen)
	{
		char * terminals = (char *)fTrie + terminalCharsOffset(offset);
		if (*(terminals + 1) != 0)
		{
			x2C = r7;
			return;
		}
		char r0 = *terminals;
		if (r6 == 0)
			r6 = r0;
		else if (r6 != r0)
		{
			x2C = r7;
			return;
		}
		flags = *((UChar *)fTrie + offset+2);
		if ((flags & 0x04) != 0)
		{
			x30 = r6;
			x2C = r7;
			return;
		}
	}
}


void
AL_FilterString(UniChar * ioStr)
{
	UniChar filteredStr[256], *fs;
	UniChar * s;
	int i, count = Ustrlen(ioStr);
	for (i = 0, s = ioStr, fs = filteredStr; i < count; i++, s++)
	{
		if (Ustrchr(filteredStr, *s) == NULL)
		{
			*fs++ = *s;
		}
	}
	*fs = kEndOfString;
	Ustrcpy(ioStr, filteredStr);
}


void
CDictionary::AL_nextSetCB(OpaqueRef ioArg1, OpaqueRef inArg2, ULong inArg3, ULong inArg4)
{
	UniChar * dst = *((UniChar **) ioArg1);
	UniChar * src = (UniChar *)inArg2;
	while (*src)
		*dst++ = *src++;
	*((UniChar **) ioArg1) = dst;
}


int
CDictionary::AL_nextSet9(void)
{
	;
}


int
CDictionary::AL_nextSet(void)
{
	UniChar * s = fStr;		// original is char *
	x50 = &s;
//	x54 = AL_nextSetCB;	need to take pointer to member function
	int status = AL_nextSet9();
	*s = 0;
	AL_FilterString(fStr);
	return status;
}

#pragma mark -

bool
A8_PrefixCompletions(DictWalkBlock * inParms)
{}

bool
A8_WalkNextChars(DictWalkBlock * inParms, ULong inOffset)
{
// r5 r4
#if 0
	CDictionary * dict = inParms->x00;	// r6
	UniChar * str = inParms->x10;			// r7
	if (dict->x30 == -1)
	{
		UniChar sp00[256];					// original uses chars -- hance A8_ presumably
		Ustrncpy(sp00, str, inOffset);
		long r10 = dict->fTrieOffset;
		dict->x20 = inOffset - 1;	// need to change this to fStrIndex/fStrLen
		dict->fStr = sp00;
		dict->despatch(8);
		if (dict->x2C == noErr)
		{
			int i;
			for (i = 0; sp00[i] != 0; ++i)
			{
				str[inOffset] = sp00[i] << 8;
				dict->fTrieOffset = r10;
				dict->x20 = inOffset;
				if (!A8_PrefixCompletions(inParms))
					return false;
			}
		}
		return true;
	}
	else
	{
		str[inOffset] = dict->x30 << 8;
		dict->x20 = inOffset;
		return A8_PrefixCompletions(inParms);
	}
#endif
}

#pragma mark -

NewtonErr
CDictionary::addWord(ULong inSeq, UniChar * inWord, ULong inAttribute)
{
	CDictionary * dict = positionToHandle(inSeq);
	if (airusResult == noErr)
	{
		dict->fStr = inWord;
//		UChar * dictData = (UChar *)fTrie;
//		int n = dictData[1] & 0x07;
//		if (n == 5 || n == 2) treat inWord as unicode else ASCII
		if (inWord[0] == kEndOfString)
			airusResult = 2;
		else
		{
			if (fAttrLen == 1)
				fAttr = inAttribute & 0xFF;
			else if (fAttrLen == 2 || fAttrLen == 4)
				fAttr = inAttribute;
			fNodeName = NULL;
			despatch(3);
			if (x2C == noErr)
				airusResult = 0;
			else if (x2C == 1)
				airusResult = 4;
			else
				airusResult = -2;
		}
	}
	return airusResult;
}


NewtonErr
CDictionary::deleteWord(UniChar * inWord)
{
	x2C = noErr;
	fStr = inWord;
//	UChar * dictData = (UChar *)fTrie;
//	int n = dictData[1] & 0x07;
	if (inWord[0] == kEndOfString)
		airusResult = 5;
	else
	{
		despatch(4);
		if (x2C == noErr)
			airusResult = 0;
		else if (x2C == 1)
			airusResult = 4;
		else
			airusResult = -2;
	}
	return airusResult;
}


NewtonErr
CDictionary::deletePrefix(UniChar * inPrefix)
{
	x2C = 1;
	fStr = inPrefix;
//	UChar * dictData = (UChar *)fTrie;
//	int n = dictData[1] & 0x07;
	if (inPrefix[0] == kEndOfString)
		airusResult = 5;
	else
	{
		despatch(4);
		if (x2C == noErr)
			airusResult = 0;
		else if (x2C == 1)
			airusResult = 4;
		else
			airusResult = -2;
	}
	return airusResult;
}


ArrayIndex
CDictionary::attributeLength(void)
{
	airusResult = noErr;

	if (fAttrLen != 0)
		return fAttrLen;

	if ((type() & 0x07) == 3)	// not sure if the & 0x07 is required
		return 1;

	return 0;
}


NewtonErr
CDictionary::changeAttribute(UniChar * inWord, ULong inAttribute)
{
	fStr = inWord;
	fAttr = inAttribute;
	despatch(7);
	if (x2C == 0)
		airusResult = 0;
	else if (x2C == 1)
		airusResult = -7;
	else if (x2C == 2)
		airusResult = -6;
	return airusResult;
}


NewtonErr
CDictionary::walk(UniChar * inWord, DictWalker inWalker, void * inData)
{
	ULong wordLen;
	DictWalkBlock parms;
	parms.x00 = this;
	parms.x04 = 0;
	parms.x08 = inWalker;
	parms.x0C = inData;
	if (inWord == NULL)
	{
		wordLen = 0;
		parms.x10[0] = 0;
	}
	else
	{
		wordLen = Ustrlen(inWord);
		Ustrcpy(parms.x10, inWord);
	}

	if (wordLen == 0)
	{
		fTrieOffset = 0;
		x30 = -1;
		A8_WalkNextChars(&parms, 0);
	}
	else
		A8_PrefixCompletions(&parms);
	return parms.x04;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C D i c t C h a i n
----------------------------------------------------------------------------- */

CDictChain::CDictChain()
{ }


CDictChain *
CDictChain::make(ArrayIndex inSize, ArrayIndex inArg2)
{
	CDictChain * chain;
	XTRY
	{
		XFAIL((chain = new CDictChain) == NULL)
		XFAILIF(chain->iDictChain(inSize, inArg2) != noErr, chain->release(); chain = NULL;)
	}
	XENDTRY;
	return chain;
}


NewtonErr
CDictChain::iDictChain(ArrayIndex inSize, ArrayIndex inArg2)
{
	NewtonErr err;
	f1C = NULL;
	if ((err = iDArray(4, inSize)) == noErr)
		f20 = inArg2;
	return err;
}


void
CDictChain::addDictToChain(CDictionary * inDict)
{
	insertEntry(count(), (Ptr)inDict);
}


ArrayIndex
CDictChain::removeDictFromChain(CDictionary * inDict)
{
	ArrayIndex i, seq = handleToPosition(inDict);
	if ((i = deleteEntry(seq)) != 0)
		return i;
	if (f20 == seq)
		f20 = kIndexNotFound;
	return 0;
}


CDictionary *
CDictChain::positionToHandle(ArrayIndex inSeq)
{
	DictListEntry * entry = (DictListEntry *)getEntry(inSeq);
	return entry ? entry->dict : NULL;
}


ArrayIndex
CDictChain::handleToPosition(CDictionary * inDict)
{
	ArrayIndex i;
	DictListEntry * entry = f1C;
	for (i = 0; i < count(); ++i)
		if (entry[i].dict == inDict)
			break;
	if (i == count())
		i = kIndexNotFound;
	return i;
}


#pragma mark -
/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
	FAirus prefix functions are used only by @171 dictionary proto.
------------------------------------------------------------------------------*/

Ref
FAirusResult(RefArg inRcvr)
{
	return MAKEINT(airusResult);
}


Ref
FAirusNew(RefArg inRcvr, RefArg inType, RefArg inArg2)
{
	CDictionary * dict = new CDictionary(RINT(inType), RINT(inArg2));
	if (dict == NULL)
		airusResult = -1;
	if (airusResult >= 0)
		SetFrameSlot(inRcvr, SYMA(dict), AddressToRef(dict));
	return MAKEINT(airusResult);
	
}


Ref
FAirusDispose(RefArg inRcvr)
{
	CDictionary * dict = GetScriptDictRef(inRcvr);
	delete dict;
	RemoveSlot(inRcvr, SYMA(dict));
	return TRUEREF;
}


Ref
FAirusRegisterDictionary(RefArg inRcvr)
{
	Ref dictId = MAKEINT(gNextCustomDictionaryId++);
	SetFrameSlot(inRcvr, SYMA(_proto), RA(protoDictionary));
	SetFrameSlot(inRcvr, SYMA(dictId), dictId);
	AddArraySlot(Dictionaries(), inRcvr);

	DictListEntry d;
	d.dict = GetScriptDictRef(inRcvr);
	d.seq = gDictList->count();
	d.status = RINT(GetProtoVariable(inRcvr, SYMA(status)));
	d.isCustom = EQ(GetProtoVariable(inRcvr, SYMA(custom)), SYMA(custom));
	memcpy(gDictList->addEntry(), &d, sizeof(DictListEntry));

	DictionariesChanged();
	return dictId;
}


Ref
FAirusUnregisterDictionary(RefArg inRcvr)
{
	int dictId = RINT(GetProtoVariable(inRcvr, SYMA(dictId)));
	bool isUnregistered = false;
	for (ArrayIndex i = 0, count = gDictList->count(); i < count; ++i)
	{
		DictListEntry * d = (DictListEntry *)gDictList->getEntry(i);
		if (isUnregistered)
			d->seq--;
		else if (d->dict != NULL && d->dict->id() == dictId)
		{
			gDictList->deleteEntry(i);
			i--;
			count--;
			isUnregistered = true;
		}
	}
	FSetRemove(RA(NILREF), Dictionaries(), inRcvr);
	RemoveSlot(inRcvr, SYMA(dictId));
	return inRcvr;
}


Ref
FAirusDictionaryType(RefArg inRcvr)
{
	CDictionary * dict = GetScriptDictRef(inRcvr);
	return MAKEINT(dict->type());
}


Ref
FAirusAddWord(RefArg inRcvr, RefArg inWord, RefArg inAttribute)
{
	CDictionary * dict = GetScriptDictRef(inRcvr);
	return MAKEINT(dict->addWord(0, GetUString(inWord), RINT(inAttribute)));	// err
}


Ref
FAirusDeleteWord(RefArg inRcvr, RefArg inWord)
{
	CDictionary * dict = GetScriptDictRef(inRcvr);
	return MAKEINT(dict->deleteWord(GetUString(inWord)));	// err
}


Ref
FAirusDeletePrefix(RefArg inRcvr, RefArg inPrefix)
{
	CDictionary * dict = GetScriptDictRef(inRcvr);
	return MAKEINT(dict->deletePrefix(GetUString(inPrefix)));	// err
}


Ref
FAirusLookupWord(RefArg inRcvr, RefArg inWord, RefArg ioArg2)
{
	NewtonErr err;
	CDictionary * dict = GetScriptDictRef(inRcvr);
	UniChar * sp04;
	ULong * sp00;
	dict->verifyStart();
	err = dict->verifyString(GetUString(inWord), &sp04, &sp00, NULL);
	if (err == noErr && NOTNIL(ioArg2))
	{
		SetFrameSlot(ioArg2, SYMA(attribute), sp00 != NULL ? MAKEINT(*sp00) : NILREF);
		SetFrameSlot(ioArg2, SYMA(terminalClass), sp04 != NULL ? MAKEINT(*sp04) : NILREF);
	}
	return MAKEINT(err);
}


bool
Walker(UniChar * inWord, ULong inArg2, UChar inArg3, ULong inArg4, void * inArg5)
{
#if 0
	RefVar args(inArg5->x04);
	SetArraySlot(args, 0, MakeString(inWord));
	SetArraySlot(args, 1, (Ref)inArg2);	// sic
	SetArraySlot(args, 2, inArg3);
	SetArraySlot(args, 3, inArg4);
	return NOTNIL(DoBlock(inArg5->x00, args));
#else
	return false;
#endif
}

Ref
FAirusWalkDictionary(RefArg inRcvr, RefArg inWord, RefArg ioArg2)
{
	CDictionary * dict = GetScriptDictRef(inRcvr);
	UniChar * word = GetUString(inWord);
//	RefVar sp04(MakeArray(4));
//	Ref sp00 = ioArg2;
	if (ISNIL(ioArg2))
		return MAKEINT(dict->walk(word, NULL, NULL));
	return MAKEINT(dict->walk(word, Walker, (void *)(Ref)ioArg2));
}


Ref
FAirusAttributeSize(RefArg inRcvr)
{
	CDictionary * dict = GetScriptDictRef(inRcvr);
	return MAKEINT(dict->attributeLength());	// size
}


Ref
FAirusChangeAttribute(RefArg inRcvr, RefArg inWord, RefArg inAttribute)
{
	CDictionary * dict = GetScriptDictRef(inRcvr);
	return MAKEINT(dict->changeAttribute(GetUString(inWord), RINT(inAttribute)));	// err
}

#pragma mark -

Ref
FAirusIteratorMake(RefArg inRcvr)
{ return NILREF; }

Ref
FAirusIteratorClone(RefArg inRcvr)
{ return NILREF; }

Ref
FAirusIteratorDispose(RefArg inRcvr)
{ return NILREF; }

Ref
FAirusIteratorReset(RefArg inRcvr, RefArg inArg1, RefArg inArg2, RefArg inArg3)
{ return NILREF; }

Ref
FAirusIteratorThisWord(RefArg inRcvr, RefArg inEntry)
{ return NILREF; }

Ref
FAirusIteratorNextWord(RefArg inRcvr)
{ return NILREF; }

Ref
FAirusIteratorPreviousWord(RefArg inRcvr)
{ return NILREF; }

