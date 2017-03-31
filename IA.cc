/*
	File:		IA.cc

	Contains:	Intelligent Assistant C++ support functions.
					Code name Dark Star (DS).

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Globals.h"
#include "ROMResources.h"
#include "Arrays.h"
#include "Iterators.h"
#include "Unicode.h"
#include "UStringUtils.h"
#include "Strings.h"
#include "RSSymbols.h"
#include "Funcs.h"
#include "NewtonScript.h"
#include "Dictionaries.h"

#include "Soups.h"
#include "Cursors.h"
#include "Entries.h"
#include "RootView.h"

extern "C" void	PrintObject(Ref inObj, int indent);

extern bool	IsRichString(RefArg inObj);
extern Ref	GetUnionSoup(RefArg inName);

extern "C" {
Ref	FOpenX(RefArg inRcvr);
Ref	FSetValue(RefArg inRcvr, RefArg inView, RefArg inTag, RefArg inValue);
}


const UniChar talkto[] = {'t','a','l','k',' ','t','o',0};

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FRegTaskTemplate(RefArg inRcvr, RefArg inTemplate);
Ref	FUnRegTaskTemplate(RefArg inRcvr, RefArg inTemplate);
Ref	FParseUtter(RefArg inRcvr, RefArg inString);
Ref	FOrigPhrase(RefArg inRcvr);
Ref	FIAatWork(RefArg inRcvr, RefArg inProgressSlip);
Ref	FGenFullCommands(RefArg inRcvr);
Ref	FIsA(RefArg inRcvr, RefArg inFrame, RefArg inTemplate);

Ref	FCleanString(RefArg inRcvr, RefArg ioStr);
Ref	FSplitString(RefArg inRcvr, RefArg inStr);
Ref	FGlueStrings(RefArg inRcvr, RefArg inStrs);
Ref	FStringAssoc(RefArg inRcvr, RefArg inStr1, RefArg inArray);
Ref	FPhraseFilter(RefArg inRcvr, RefArg inStrs);

Ref	DSGetMatchedEntries(RefArg inRcvr, RefArg inWhich, RefArg inEntries);
/*
Ref	GuessAddressee
Ref	ScanForWackyName
*/

// imports
Ref	FSetRemove(RefArg inRcvr, RefArg ioArray, RefArg inMember);
Ref	FStrLen(RefArg inRcvr, RefArg inStr);
Ref	FStrEqual(RefArg inRcvr, RefArg inStr1, RefArg inStr2);
Ref	FStrConcat(RefArg inRcvr, RefArg inStr1, RefArg inStr2);
}


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

Ref	InitDSPhraseSupport(RefArg inRcvr, RefArg inOptions);
Ref	InitDSDictionary(RefArg inRcvr, RefArg inOptions);
Ref	InitDSHeuristics(RefArg inRcvr, RefArg inOptions);
Ref	InitDSTaskTemplates(RefArg inRcvr, RefArg inOptions);

Ref	DSResolveString(RefArg inRcvr, RefArg inStr, RefArg ioState);

CDictionary *	TrieInit(void);
Ref	DynaTrieLookup(UniChar * inStr);

// strings
char *		NewASCIIString(RefArg inStr);
Ref			PartialGlueString(RefArg inStrs, ULong inIndex, ULong inCount);
ArrayIndex	StringLeftTrim(RefArg inStr);
ArrayIndex	StringRightTrim(RefArg inStr);

// list creation
Ref	member_p(RefArg inList, RefArg inItem);
Ref	Append(RefArg ioList, RefArg inItem);
Ref	UniqueAppendItem(RefArg ioList, RefArg inItem);
Ref	UniqueAppendListGen(RefArg ioList1, RefArg inList2);

// phrase handling
void	IPhraseGenerator(RefArg inStr);
Ref	NextPhrase(void);
int	PeekValidPhrase(void);
void	SetPhraseElem(ArrayIndex in1, ArrayIndex in2, int in3);
int	GetPhraseElem(ArrayIndex in1, ArrayIndex in2);
void	PhraseHitExt(void);
void	PhraseHit(int in1, int in2);

bool	interval_intersection_p(int in1, int in2, int in3, int in4);
Ref	GeneratePhrases(RefArg inStr);
void	PrintGeneratorState(void);

Ref	IAInputErrors(RefArg inRcvr, RefArg inAssistant);
Ref	FillPreconditions(RefArg ioTaskTemplate);
Ref	CheezySubsumption(RefArg inArray1, RefArg inArray2);
Ref	CheezyIntersect(RefArg inArray1, RefArg inArray2);
Ref	GetClasses(RefArg inPhrases);
Ref	CompositeClass(RefArg inRcvr, RefArg inPhrase);

Ref	UnmatchedWords(RefArg inRcvr);
Ref	MapSymToFrame(RefArg inSym);	// RefArg inRcvr stripped
Ref	TagStringHelper(RefArg inNameFrame, RefArg inArg2);
Ref	FavorAction(RefArg inRcvr, RefArg inPhrase);
Ref	FavorObject(RefArg inRcvr, RefArg inPhrase);

Ref	AddEntry(RefArg inRcvr, RefArg inTag, RefArg inPath, RefArg inAlias, RefArg ioState);

Ref	GetCardFileSoup(void);
Ref	IASmartCFLookup(RefArg inRcvr, RefArg inStr);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

struct DSPhrase
{
	UChar			element[16][16];
	ULong			numOfWords;				// +100
	ULong			x104;
	ULong			x108;
	ULong			x10C;
	RefStruct	words;					// original phrase -- array of words
};


CDictionary *	gTrie;
CDictionary *	gDynaTrie;
/*
Ref			gDateFrame;
Ref			gTimeFrame;
Ref			gPhoneFrame;
Ref			gNumberFrame;
*/
Ref			gPossFrame;
Ref			gDictionaryFrame;			// [] !
Ref			gDynaDictionaryFrame;	// [] !
Ref			gDynaDeleteSym;
Ref			gDynaDeleteFrame;
Ref			gLastLookupString;

Ref			gWhoHistory;
Ref			gWhatHistory;
Ref			gWhenHistory;
Ref			gWhereHistory;
/*
Ref			gWhoObj;
Ref			gWhatObj;
Ref			gWhenObj;
Ref			gWhereObj;
*/
//				gReferenceList;	// 0C100BB8
DSPhrase *	gPhrasalGlobals;
Ref			gUtterGenFrame;
Ref			gActionClass;
Ref			gObjectClass;


#pragma mark -
/*------------------------------------------------------------------------------
	D a r k   S t a r   I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/

Ref
InitDarkStar(RefArg inRcvr, RefArg inOptions)
{
	InitDSPhraseSupport(inRcvr, inOptions);
	InitDSDictionary(inRcvr, inOptions);
	InitDSHeuristics(inRcvr, inOptions);
	InitDSTaskTemplates(inRcvr, inOptions);

	return TRUEREF;
}


Ref
InitDSDictionary(RefArg inRcvr, RefArg inOptions)
{
	gDynaTrie = TrieInit();
	gTrie = GetROMDictionary(2);
/*
	assistFrames is in ROM -- why add GC roots?
	gDateFrame = GetFrameSlot(RA(assistFrames), SYMA(Date));
	AddGCRoot(&gDateFrame);

	gTimeFrame = GetFrameSlot(RA(assistFrames), SYMA(Time));
	AddGCRoot(&gTimeFrame);

	gPhoneFrame = GetFrameSlot(RA(assistFrames), SYMA(parsed_phone));
	AddGCRoot(&gPhoneFrame);

	gNumberFrame = GetFrameSlot(RA(assistFrames), SYMA(parsed_number));
	AddGCRoot(&gNumberFrame);
*/
	gDictionaryFrame = MAKEMAGICPTR(248);

	gDynaDictionaryFrame = MakeArray(0);
	AddGCRoot(&gDynaDictionaryFrame);

	gLastLookupString = NILREF;
	AddGCRoot(&gLastLookupString);
	
	return TRUEREF;
}


Ref
InitDSHeuristics(RefArg inRcvr, RefArg inOptions)
{
	gWhoHistory = AllocateArray(SYMA(history), 3);
	AddGCRoot(&gWhoHistory);

	gWhatHistory = AllocateArray(SYMA(history), 3);
	AddGCRoot(&gWhatHistory);

	gWhenHistory = AllocateArray(SYMA(history), 3);
	AddGCRoot(&gWhenHistory);

	gWhereHistory = AllocateArray(SYMA(history), 3);
	AddGCRoot(&gWhereHistory);
/*
	assistFrames is in ROM -- why add GC roots?
	gWhoObj = GetFrameSlot(RA(assistFrames), SYMA(who_obj));
	AddGCRoot(&gWhoObj);

	gWhatObj = GetFrameSlot(RA(assistFrames), SYMA(what_obj));
	AddGCRoot(&gWhatObj);

	gWhenObj = GetFrameSlot(RA(assistFrames), SYMA(when_obj));
	AddGCRoot(&gWhenObj);

	gWhereObj = GetFrameSlot(RA(assistFrames), SYMA(where_obj));
	AddGCRoot(&gWhereObj);
*/
	return TRUEREF;
}


Ref
InitDSTaskTemplates(RefArg inRcvr, RefArg inOptions)
{
	gActionClass = GetFrameSlot(RA(assistFrames), SYMA(action));
//	assistFrames is in ROM -- why add GC roots?
//	AddGCRoot(&gActionClass);

	gObjectClass = GetFrameSlot(RA(assistFrames), SYMA(user_obj));
//	AddGCRoot(&gObjectClass);

	return TRUEREF;
}

#pragma mark -

/*------------------------------------------------------------------------------
	D a r k   S t a r   P h r a s e   S u p p o r t
------------------------------------------------------------------------------*/

Ref
InitDSPhraseSupport(RefArg inRcvr, RefArg inOptions)
{
	AddGCRoot(&gUtterGenFrame);
	gPhrasalGlobals = new DSPhrase;

	return TRUEREF;
}


// for debug
Ref
GeneratePhrases(RefArg inStr)	// RefArg inRcvr stripped
{
	RefVar aPhrase;
	IPhraseGenerator(inStr);
	PrintGeneratorState();
	PhraseHit(3,3);
	PrintGeneratorState();
	while (NOTNIL(aPhrase = NextPhrase()))
	{
//		printf("\n%U", GetUString(aPhrase));	// this is what we want to happen
		char buf[256];
		ConvertFromUnicode(GetUString(aPhrase), buf);
		printf("\n%s", buf);
	}
	printf("\n");
	return TRUEREF;
}


// for debug
void
PrintGeneratorState(void)
{
	ArrayIndex i, j, count = Length(gPhrasalGlobals->words);
	for (i = count; i > 0; i--)
		for (j = 1; j <= count - (i-1); j++)
		{
//			printf("\n[%d,%d] : %U = %d", j, i, GetUString(PartialGlueString(gPhrasalGlobals->words, j, i)), GetPhraseElem(i, j));	// this is what we want to happen
			char buf[256];
			ConvertFromUnicode(GetUString((PartialGlueString(gPhrasalGlobals->words, j, i))), buf);
			printf("\n[%d,%d] : %s = %d", j, i, buf, GetPhraseElem(i, j));
		}
	printf("\n");
}


void
IPhraseGenerator(RefArg inStr)
{
	// set up array of words in input string
	gPhrasalGlobals->words = FSplitString(RA(NILREF), inStr);

	ArrayIndex i, j, count = Length(gPhrasalGlobals->words);	// r7, r6, r5
	// we’re limited to 15 words
	if (count > 15)
		count = 15;

	// init phrase elements
	for (i = 1; i <= count; ++i)
		for (j = 1; j <= count - (i-1); ++j)
			SetPhraseElem(i, j, 4);		// i := 1 [1 2 3 4]  2 [1 2 3]  3 [1 2]  4 [1]

	gPhrasalGlobals->numOfWords = count;	// last word in array?
	gPhrasalGlobals->x104 = 1;					// first word in array?
	gPhrasalGlobals->x108 = 0;
	gPhrasalGlobals->x10C = 0;
}


int
PeekValidPhrase(void)
{
	ArrayIndex i, j, count = Length(gPhrasalGlobals->words);
	for (i = gPhrasalGlobals->numOfWords; i > 0; i--)
	{
		int r6 = gPhrasalGlobals->x104;
		for (j = 1; j <= count - (i-1); j++, r6++)
		{
			if (GetPhraseElem(i, r6) == 4)
			{
				gPhrasalGlobals->numOfWords = i;
				gPhrasalGlobals->x104 = r6;
				gPhrasalGlobals->x108 = i;
				gPhrasalGlobals->x10C = r6;
				return 4;
			}
		}
		gPhrasalGlobals->x104 = 1;
	}
	gPhrasalGlobals->numOfWords = 0;
	gPhrasalGlobals->x104 = 0;
	gPhrasalGlobals->x108 = 0;
	gPhrasalGlobals->x10C = 0;
	return 0;
}


Ref
NextPhrase(void)
{
	RefVar aPhrase;
	ArrayIndex count = Length(gPhrasalGlobals->words);
	for ( ; gPhrasalGlobals->numOfWords > 0 && PeekValidPhrase() == 4; gPhrasalGlobals->numOfWords--)
	{
		if (gPhrasalGlobals->x104 <= count - (gPhrasalGlobals->numOfWords - 1))
		{
			aPhrase = PartialGlueString(gPhrasalGlobals->words, gPhrasalGlobals->x104, gPhrasalGlobals->numOfWords);
			gPhrasalGlobals->x104++;
			return aPhrase;
		}
		gPhrasalGlobals->x104 = 1;
	}
	return NILREF;
}


void
SetPhraseElem(ArrayIndex in1, ArrayIndex in2, int in3)
{
	 gPhrasalGlobals->element[in1][in2] = in3;
}


int
GetPhraseElem(ArrayIndex in1, ArrayIndex in2)
{
	return gPhrasalGlobals->element[in1][in2];
}


/*------------------------------------------------------------------------------
	Return the original phrase.
	Args:		inRcvr
	Return:	array of words forming the original phrase
------------------------------------------------------------------------------*/

Ref
FOrigPhrase(RefArg inRcvr)
{
	return gPhrasalGlobals->words;
}


void
PhraseHitExt(void)
{
	PhraseHit(gPhrasalGlobals->x108, gPhrasalGlobals->x10C);
}


void
PhraseHit(int in1, int in2)
{
	ArrayIndex i, j, count = Length(gPhrasalGlobals->words);	// r6, r8, r10
	int r7 = in2 - 1;
	if (r7 == 0) r7 = 1;
	for (i = in1; i > 0; i--, r7 = 1)
	{
		for (j = 1; j <= count - (i-1); j++, r7++)
		{
			if (GetPhraseElem(i, r7) == 4 && interval_intersection_p(in1, in2, i, r7))
				SetPhraseElem(i, r7, 3);
		}
	}
	SetPhraseElem(in1, in2, 1);
}


bool
interval_intersection_p(int in1, int in2, int in3, int in4)
{
	int r0 = in1 + in2;
	int r2 = in3 + in4;
	return (in1 == in3 && in2 == in4)
		 || (in2 < r2 && in4 < r0)
		 || (in4 < r0 && in2 < r2);
}


Ref
UnmatchedWords(RefArg inRcvr)
{
	RefVar words(MakeArray(0));
	for (ArrayIndex	i = 1, count = Length(gPhrasalGlobals->words); i <= count; ++i)
	{
		unsigned char	elem = GetPhraseElem(1, i);
		if (elem != 1 && elem != 3)
			Append(words, PartialGlueString(gPhrasalGlobals->words, i, 1));
	}
	return words;
}


/*------------------------------------------------------------------------------
	Generate an array of canonical frames corresponding to the tags parsed from
	a phrase. The value slot of each frame is the phrase.
	Args:		inRcvr
				inTags			array of tag symbols
				ioStr				the phrase
	Return:	array of frames
------------------------------------------------------------------------------*/

Ref
TagPhraseFrame(RefArg inRcvr, RefArg inTags, RefArg inStr)
{
	RefVar item;
	RefVar theFrames(MakeArray(0));
	for (ArrayIndex i = 0, count = Length(inTags); i < count; ++i)
	{
		item = GetArraySlot(inTags, i);
		if (IsSymbol(item))
			item = MapSymToFrame(item);
		item = Clone(item);
		SetFrameSlot(item, SYMA(value), inStr);
		Append(theFrames, item);
	}
	return theFrames;
}


Ref
LexLookup(CDictionary * inTrie, UniChar * inStr)
{
	ULong * sp00;
	inTrie->verifyString(inStr, NULL, &sp00, NULL);
	return MAKEBOOLEAN(airusResult == 2 || airusResult == 3);
}


Ref
cLexDateLookup(RefArg inRcvr, UniChar * inStr)
{
	return LexLookup(gDateLexDictionary, inStr);
}


Ref
cLexTimeLookup(RefArg inRcvr, UniChar * inStr)
{
	return LexLookup(gTimeLexDictionary, inStr);
}


Ref
cLexPhoneLookup(RefArg inRcvr, UniChar * inStr)
{
	return LexLookup(gPhoneLexDictionary, inStr);
}


Ref
cLexNumberLookup(RefArg inRcvr, UniChar * inStr)
{
	return LexLookup(gNumberLexDictionary, inStr);
}


Ref
FastStringLookup(RefArg inRcvr, RefArg inStr)
{
	// make a lower-case copy of the string
	UniChar * str = GetUString(Clone(inStr));
	LowerCaseText(str, RSTRLEN(inStr));
	// look it up in the dyna trie
	RefVar dynaTag(DynaTrieLookup(str));
	// look it up in the built-in trie
	ULong * index;
	gTrie->verifyString(str, NULL, &index, NULL);
	// return whatever tags we recognised
	if (airusResult == 2 || airusResult == 3)
	{
		RefVar tag(GetArraySlot(gDictionaryFrame, *index));
		if (NOTNIL(dynaTag))
			return UniqueAppendListGen(dynaTag, tag);
		return tag;
	}
	return dynaTag;
}


/*------------------------------------------------------------------------------
	This is The Big One. 14 pages!
	Major change from the original: strings are now Unicode, not ASCII
	so we aren’t constantly converting from one to the other.
	Args:		inTrie
				inStr
				ioState
	Return:	array of canonical frames with value slot
------------------------------------------------------------------------------*/

Ref
MatchString(CDictionary * inTrie, UniChar * inStr, RefArg ioState)
{
	RefVar theStr(MakeString(inStr));			// also a new departure, so we only do it once
	// Caution! This modifies inStr!
	LowerCaseText(inStr, RSTRLEN(theStr));		// was inStr = DownCase(inStr);

	// look it up in the dyna trie
	RefVar dynaTag(DynaTrieLookup(inStr));	// sp14

	// look it up in the specified trie
	ArrayIndex * index;	// sp1C
	inTrie->verifyString(inStr, NULL, &index, NULL);

	// return whatever tags we recognised
	if (airusResult == 2 || airusResult == 3)
	{
		RefVar tag(GetArraySlot(gDictionaryFrame, *index));
		if (NOTNIL(dynaTag))
//sp-14
			return TagPhraseFrame(RA(NILREF), UniqueAppendListGen(dynaTag, tag), theStr);
//sp-0C
		// tag the phrase as what we recognised
		return TagPhraseFrame(RA(NILREF), tag, theStr);
	}

	if (NOTNIL(dynaTag))
		return dynaTag;

	// okay, at this point the passed trie and dyna trie didn’t recognise anything
	RefVar objFrame;		// sp0C
	RefVar lexDateFrame;	// r9
//sp-08
	if (NOTNIL(cLexDateLookup(RA(NILREF), inStr)))
	{
		objFrame = Clone(GetFrameSlot(RA(assistFrames), SYMA(lexical)));
		lexDateFrame = objFrame;
		SetFrameSlot(objFrame, SYMA(isa), GetFrameSlot(RA(assistFrames), SYMA(Date)));
		SetFrameSlot(objFrame, SYMA(Date), MAKEINT(0));
	}
//sp-04

	else if (NOTNIL(cLexTimeLookup(RA(NILREF), inStr)))
	{
		objFrame = Clone(GetFrameSlot(RA(assistFrames), SYMA(lexical)));
		SetFrameSlot(objFrame, SYMA(isa), GetFrameSlot(RA(assistFrames), SYMA(Time)));
		SetFrameSlot(objFrame, SYMA(Time), MAKEINT(0));
	}
//sp-04

	else if (NOTNIL(cLexPhoneLookup(RA(NILREF), inStr)))
	{
		objFrame = Clone(GetFrameSlot(RA(assistFrames), SYMA(lexical)));
		SetFrameSlot(objFrame, SYMA(isa), GetFrameSlot(RA(assistFrames), SYMA(parsed_phone)));
	}
//sp-04

	else if (NOTNIL(cLexNumberLookup(RA(NILREF), inStr)))
	{
		objFrame = Clone(GetFrameSlot(RA(assistFrames), SYMA(lexical)));
		SetFrameSlot(objFrame, SYMA(isa), GetFrameSlot(RA(assistFrames), SYMA(parsed_number)));
	}

	RefVar dateException;
	if (NOTNIL(lexDateFrame))
	{
		// we have a date...
//sp-0C
		dateException = FFindStringInArray(RA(NILREF), RA(DSExceptions), theStr);
		if (NOTNIL(dateException))
		{
			//... no, it’s a girl’s name...
			int exceptIndex = RVALUE(dateException);
			RefVar except(GetFrameSlot(ioState, SYMA(exception)));
//sp-04
			if (ISNIL(GetArraySlot(except, exceptIndex)))
				SetArraySlot(except, exceptIndex, MAKEINT(0));
			else
				// we’ve already seen this
				dateException = NILREF;
		}
	}

	if (NOTNIL(objFrame) && ISNIL(dateException))
	{
		// we got something unexceptional, return it in an array
//sp-04
		SetFrameSlot(objFrame, SYMA(value), theStr);
		return Append(RA(NILREF), objFrame);
	}

//sp-08
	if (ISNIL(dateException))
	{
//sp-08
		RefVar words(FSplitString(RA(NILREF), theStr));	// r7
		ArrayIndex count = Length(words);
		if (count > 4)
			return NILREF;

		// we want only objects, not actions
		for (ArrayIndex i = 0; i < count; ++i)
		{
//sp-08
			RefVar tags(FastStringLookup(RA(NILREF), GetArraySlot(words, i)));	// r6
			if (NOTNIL(tags))
			{
//sp-10
				for (ArrayIndex j = 0, jcount = Length(tags); j < jcount; ++j)
				{
					if (NOTNIL(FIsA(RA(NILREF), GetArraySlot(tags, j), SYMA(action))))
						return NILREF;
				}
			}
		}
	}

	// try names
	return DSResolveString(RA(NILREF), theStr, ioState);
}


/*------------------------------------------------------------------------------
	Test for string equality.
	Not quite the same as FStrEqual -- any reason why?
	Args:		inRcvr
				inStr1
				inStr2
	Return:	TRUEREF => strings equal
------------------------------------------------------------------------------*/

Ref
DSStringEQ(RefArg inRcvr, RefArg inStr1, RefArg inStr2)
{
	return MAKEBOOLEAN(CompareStringNoCase(GetUString(inStr1), GetUString(inStr2)) == 0);
}


/*------------------------------------------------------------------------------
	Test whether a list of words match another list of words.
	Args:		inRcvr
				inStrs1
				inStrs2
	Return:	TRUEREF => all words match
------------------------------------------------------------------------------*/

Ref
DSPartialStrMatch(RefArg inRcvr, RefArg inStrs1, RefArg inStrs2)
{
	RefVar isStrEQ;
	RefVar str1, str2;
	ArrayIndex strs1Count = Length(inStrs1);
	ArrayIndex strs2Count = Length(inStrs2);
	if (strs1Count > strs2Count)
		return NILREF;

	for (ArrayIndex i = 0; i < strs1Count; ++i)
	{
		str1 = GetArraySlot(inStrs1, i);
		for (ArrayIndex j = 0; j < strs2Count; ++j)
		{
			str2 = GetArraySlot(inStrs2, j);
			if (NOTNIL(str2))
			{
				isStrEQ = DSStringEQ(RA(NILREF), str1, str2);
				if (isStrEQ == TRUEREF)
					break;
			}
		}
		if (ISNIL(isStrEQ))
			return NILREF;
	}
	return TRUEREF;
}


/*------------------------------------------------------------------------------
	Test whether a list of words contains a stop word.
	Args:		inRcvr
				inStrArray
	Return:	TRUEREF => stop word was found
------------------------------------------------------------------------------*/

Ref
DSHasStopString(RefArg inRcvr, RefArg inStrArray)
{
	RefVar str;
	ArrayIndex strLen;
	for (ArrayIndex i = 0, count = Length(inStrArray); i < count; ++i)
	{
		str = GetArraySlot(inStrArray, i);
		strLen = RSTRLEN(str);
		if (strLen > 1 && strLen < 5)
		{
			if (NOTNIL(FFindStringInArray(RA(NILREF), RA(stopWordList), str)))
				return TRUEREF;
		}
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Tag...
	Args:		inRcvr
				inNames		array of names soup entries
				inStr
				ioState
	Return:	array of unique tag symbols
				will only ever be ['person] ??
------------------------------------------------------------------------------*/

Ref
DSTagString(RefArg inRcvr, RefArg inNames, RefArg inStr, RefArg ioState)
{
	if (NOTNIL(inNames))
	{
		RefVar theEntry;		// sp20
		RefVar entryAlias;	// sp1C
		RefVar slot;			// sp14
		RefVar sp10;
		RefVar strClass;		// r6
		RefVar words;
		RefVar personTag;

//sp-04
		RefVar tags(MakeArray(0));			// sp18
		RefVar strWords(FSplitString(RA(NILREF), inStr));	// sp04

		for (ArrayIndex i = 0, count = Length(inNames); i < count; ++i)
		{
//sp-08
			theEntry = GetArraySlot(inNames, i);
			entryAlias = MakeEntryAlias(theEntry);
			SetFrameSlot(ioState, SYMA(personAdded), RA(NILREF));
			SetFrameSlot(ioState, SYMA(placeAdded), RA(NILREF));
//sp-30
			// iterate over slots in the name frame
			CObjectIterator iter(theEntry);
			for ( ; !iter.done(); iter.next())
			{
				slot = iter.value();
				if (NOTNIL(slot))
				{
					int slotType = -1;
					if (IsString(slot))
					{
						strClass = ClassOf(slot);
						if ((iter.tag(), SYMA(group)))
							slotType = 4;
						else if (EQ(strClass, SYMA(company)))
							slotType = 1;
						else if (EQ(strClass, SYMA(string_2Ecustom)))
							slotType = 3;
						else if (EQ(iter.tag(), SYMA(title)))
							slotType = 5;
					}
					else
					{
						if (EQ(iter.tag(), SYMA(name)))
							slotType = 0;
						else if (EQ(iter.tag(), SYMA(names)))
							slotType = 2;
					}

					if (slotType != -1 /*&& !EQ(iter.tag(), SYMA(sortOn)*/)	// sic
					{
						switch (slotType)
						{
						case 0:	// name
							// slot is a name frame
							personTag = TagStringHelper(slot, strWords);
							if (NOTNIL(personTag))
							{
								UniqueAppendListGen(tags, personTag);
								AddEntry(RA(NILREF), SYMA(person), MAKEINT(0), entryAlias, ioState);
							}
							break;

						case 1:	// company
							// slot is a string
							words = FSplitString(RA(NILREF), slot);
							if (NOTNIL(DSPartialStrMatch(RA(NILREF), strWords, words)))
							{
								AddEntry(RA(NILREF), SYMA(personAdded), MAKEINT(0), entryAlias, ioState);
								UniqueAppendItem(tags, SYMA(person));
							}
							break;

						case 2:	// names
							{
								// slot is an array of names frames
								RefVar aName;
								for (ArrayIndex j = 0, limit = Length(slot); j < limit; j++)
								{
									aName = GetArraySlot(slot, j);
									personTag = TagStringHelper(aName, strWords);
									if (NOTNIL(personTag))
									{
										UniqueAppendListGen(tags, personTag);
										AddEntry(RA(NILREF), SYMA(affiliate), MAKEINT(i), entryAlias, ioState);
									}
								}
							}
							break;

						case 3:	// custom
							// slot is a string
							words = FSplitString(RA(NILREF), slot);
							if (NOTNIL(DSPartialStrMatch(RA(NILREF), strWords, words)))
							{
								AddEntry(RA(NILREF), SYMA(custom), iter.tag(), entryAlias, ioState);
								UniqueAppendItem(tags, SYMA(person));
							}
							break;

						case 4:	// group
							// slot is a string
							words = FSplitString(RA(NILREF), slot);
							if (NOTNIL(DSPartialStrMatch(RA(NILREF), strWords, words)))
							{
								AddEntry(RA(NILREF), SYMA(group), MAKEINT(0), entryAlias, ioState);
								UniqueAppendItem(tags, SYMA(person));
							}
							// fall thru

						case 5:	// title
							// slot is a string
							words = FSplitString(RA(NILREF), slot);
							if (NOTNIL(DSPartialStrMatch(RA(NILREF), strWords, words)))
							{
								AddEntry(RA(NILREF), SYMA(title), MAKEINT(0), entryAlias, ioState);
								UniqueAppendItem(tags, SYMA(person));
							}
							break;
						}
					}
				}
			}
		}
		if (Length(tags) > 0)
			return TagPhraseFrame(RA(NILREF), tags, inStr);
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Tag a name frame as a person.
	Args:		inNameFrame
				inArg2
	Return:	array of 'person tag
				NILREF => not a person’s name
------------------------------------------------------------------------------*/
extern Ref* RSSTRspace;

Ref
TagStringHelper(RefArg inNameFrame, RefArg inArg2)
{
	RefVar firstName(GetFrameSlot(inNameFrame, SYMA(first)));
	RefVar lastName(GetFrameSlot(inNameFrame, SYMA(last)));
	RefVar fullName;

	if (ISNIL(firstName) || IsRichString(firstName))
		fullName = lastName;
	else if (ISNIL(lastName) || IsRichString(lastName))
		fullName = firstName;
	else
		fullName = FStrConcat(RA(NILREF), FStrConcat(RA(NILREF), firstName, RA(space)), lastName);

	if (NOTNIL(fullName))
	{
		RefVar names(FSplitString(RA(NILREF), fullName));
		if (NOTNIL(DSPartialStrMatch(RA(NILREF), inArg2, names)))
			return Append(RA(NILREF), SYMA(person));
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Look for Names soup entries matching a string -- presumably a name.
	Args:		inRcvr stripped
				inStr
	Return:	array of Names soup entries
				NILREF => no matches
------------------------------------------------------------------------------*/

Ref
GetCardFileSoup(void)
{
	return GetUnionSoup(RA(cardfileSoupName));
}


Ref
StringToFrameMapper(RefArg inStr)
{
	RefVar names(MakeArray(0));
	RefVar words(FSplitString(RA(NILREF), inStr));
	ArrayIndex numOfWords = Length(words);

	// any more than 4 words and it’s unlikely to be a name
	if (numOfWords > 4)
		return NILREF;

	// any one-letter words, it’s unlikely to be a name
	for (ArrayIndex i = 0; i < numOfWords; ++i)
	{
		if (RINT(FStrLen(RA(NILREF), GetArraySlot(words, i))) == 1)
			return NILREF;
	}

	// set up Names soup query -- looking for entries that match inStr as a name
	RefVar theQuery = Clone(GetFrameSlot(RA(assistFrames), SYMA(DSQuery)));
	SetFrameSlot(theQuery, SYMA(words), words);
	RefVar theCursor = SoupQuery(GetCardFileSoup(), theQuery);
	RefVar theEntry;
	// collect up to 15 names soup entries
	for (ArrayIndex i = 0; NOTNIL(theEntry = CursorEntry(theCursor)) && i < 15; CursorNext(theCursor), ++i)
	{
		Append(names, theEntry);
	}
	if (Length(names) > 0)
		return names;

	// didn’t find anything
	return NILREF;
}


Ref
DSResolveString(RefArg inRcvr, RefArg inStr, RefArg ioState)
{
	RefVar words(FSplitString(RA(NILREF), inStr));
	if (ISNIL(DSHasStopString(RA(NILREF), words)))
	{
		RefVar namesEntries(StringToFrameMapper(inStr));
		RefVar nameTags(DSTagString(RA(NILREF), namesEntries, inStr, ioState));
		if (NOTNIL(nameTags))
			return nameTags;

		// and just how does this differ from the above?
		namesEntries = IASmartCFLookup(RA(NILREF), inStr);
		if (NOTNIL(namesEntries))
			return DSTagString(RA(NILREF), namesEntries, GetArraySlot(words, 0), ioState);
	}
	return NILREF;
}

#pragma mark -

Ref
IASmartCFLookup(RefArg inRcvr, RefArg inStr)
{
	gLastLookupString = inStr;
	return StringToFrameMapper(inStr);
}


#pragma mark -

/*------------------------------------------------------------------------------
	T r i e
------------------------------------------------------------------------------*/

CDictionary *
TrieInit(void)
{
	return new CDictionary(15, 4);
}


void
TrieAdd(UniChar * inStr, CDictionary * inDict, RefArg inTemplate)
{
	;
}


Ref
DynaTrieLookup(UniChar * inStr)
{
	ArrayIndex * index;
	gDynaTrie->verifyString(inStr, NULL, &index, NULL);
	if (airusResult == 2 || airusResult == 3)
		return GetArraySlot(GetArraySlot(gDynaDictionaryFrame, *index), 1);
	return NILREF;
}


void
DynaTrieDelete(RefArg inWord)
{
}

#pragma mark -

/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

Ref
member_p(RefArg inList, RefArg inItem)
{
	if (NOTNIL(inList) && NOTNIL(inItem))
	{
		RefVar item;
		for (ArrayIndex i = 0, count = Length(inList); i < count; ++i)
		{
			item = GetArraySlot(inList, i);
			if (EQ(item, inItem))
				return item;
		}
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Append an object to an array.
	Create the array if it doesn’t already exist.
	Args:		inRcvr stripped
				ioArray
				inValue
	Return:	the modified ioArray
------------------------------------------------------------------------------*/

Ref
Append(RefArg ioArray, RefArg inValue)
{
	if (ISNIL(ioArray))
	{
		RefVar a(MakeArray(1));
		SetArraySlot(a, 0, inValue);
		return a;
	}

	AddArraySlot(ioArray, inValue);
	return ioArray;
}


/*------------------------------------------------------------------------------
	Append a string to a list.
	The resulting list contains unique items.
	Args:		inRcvr stripped
				ioList		the master list
				inStr			string to be added
	Return:	the modified list
------------------------------------------------------------------------------*/

Ref
UniqueAppendString(RefArg ioList, RefArg inStr)
{
	if (ISNIL(FFindStringInArray(RA(NILREF), ioList, inStr)))
		Append(ioList, inStr);
	return ioList;
}


/*------------------------------------------------------------------------------
	Append an item to a list.
	The resulting list contains unique items.
	Args:		inRcvr stripped
				ioList		the master list
				inItem		item to be added
	Return:	the modified list
------------------------------------------------------------------------------*/

Ref
UniqueAppendItem(RefArg ioList, RefArg inItem)
{
	if (ISNIL(member_p(ioList, inItem)))
		Append(ioList, inItem);
	return ioList;
}


/*------------------------------------------------------------------------------
	Append items from one list to another.
	The resulting list contains unique items.
	Args:		inRcvr stripped
				ioList1		the master list
				inList2		list of items to be added
	Return:	the modified master list
------------------------------------------------------------------------------*/

Ref
UniqueAppendListGen(RefArg ioList1, RefArg inList2)
{
	if (ISNIL(ioList1))
		return inList2;

	else if (ISNIL(inList2))
		return ioList1;

	RefVar item;
	RefVar theList = IsReadOnly(ioList1) ? (Ref)Clone(ioList1) : (Ref)ioList1;
	for (ArrayIndex i = 0, count = Length(inList2); i < count; ++i)
	{
		item = GetArraySlot(inList2, i);
		if (ISNIL(member_p(theList, item)))	// could UniqueAppendItem
		{
			Append(theList, item);
		}
	}
	return theList;
}


/*------------------------------------------------------------------------------
	Determine whether a frame represents an action (as opposed to an object).
	Args:		inFrame
	Return:	NS boolean
------------------------------------------------------------------------------*/

Ref
IsAction(RefArg inFrame)
{
	return FIsA(RA(NILREF), inFrame, GetFrameSlot(RA(assistFrames), SYMA(action)));
}


/*------------------------------------------------------------------------------
	Determine whether an action frame represents the primary action.
	Args:		inFrame
				inTaskTemplate
	Return:	NS boolean
------------------------------------------------------------------------------*/

Ref
IsPrimaryAct(RefArg inFrame, RefArg inTaskTemplate)
{
	RefVar templ8(GetFrameSlot(inTaskTemplate, SYMA(primary_act)));
	if (NOTNIL(templ8))
		return FIsA(RA(NILREF), inFrame, templ8);
	return NILREF;
}

#pragma mark -

/*------------------------------------------------------------------------------
	H i s t o r y
------------------------------------------------------------------------------*/

Ref
SelectHistList(RefArg inItem)
{
	if (NOTNIL(FIsA(RA(NILREF), inItem, GetFrameSlot(RA(assistFrames), SYMA(who_obj)))))
		return gWhoHistory;
	if (NOTNIL(FIsA(RA(NILREF), inItem, GetFrameSlot(RA(assistFrames), SYMA(what_obj)))))
		return gWhatHistory;
	if (NOTNIL(FIsA(RA(NILREF), inItem, GetFrameSlot(RA(assistFrames), SYMA(when_obj)))))
		return gWhenHistory;
	if (NOTNIL(FIsA(RA(NILREF), inItem, GetFrameSlot(RA(assistFrames), SYMA(where_obj)))))
		return gWhereHistory;
	return NILREF;
}


Ref
ShuffleHistory(RefArg inHistoryList, RefArg inItem)
{
	if (NOTNIL(inHistoryList))
	{
		for (ArrayIndex i = 2; i > 0; i--)
			SetArraySlot(inHistoryList, i, GetArraySlot(inHistoryList, i-1));
		SetArraySlot(inHistoryList, 0, inItem);
		return TRUEREF;
	}
	return NILREF;
}


Ref
RecordHistory(RefArg inItem)
{
	RefVar historyList(SelectHistList(inItem));
	if (ISNIL(member_p(historyList, inItem)))
	{
		ShuffleHistory(historyList, inItem);
		return TRUEREF;
	}
	return NILREF;
}


#pragma mark -
/*------------------------------------------------------------------------------
	S t r i n g   M a n i p u l a t i o n
	We don’t use ASCII strings.
------------------------------------------------------------------------------*/
#if 0
/*------------------------------------------------------------------------------
	Make a new ASCII string from a string object.
	Args:		inStr		the unicode string
	Return:	char *	the ASCII string -- caller MUST FreePtr() this
------------------------------------------------------------------------------*/

char *
NewASCIIString(RefArg inStr)
{
	char * str = NewPtr(Length(inStr)/sizeof(UniChar));
	if (str != NULL)
		ConvertFromUnicode((UniChar *)BinaryData(inStr), str);
	return str;
}


char *
ASCIIString(RefArg inStr, char * outStr)
{
	ConvertFromUnicode((UniChar *)BinaryData(inStr), outStr);
	return outStr;
}


UChar *
DownCase(UChar * ioStr)
{
	for (ArrayIndex i = 0, count = strlen(ioStr); i < count; ++i)
		ioStr[i] = tolower(ioStr[i]);
	return ioStr;
}
#endif

#pragma mark -
/*------------------------------------------------------------------------------
	Clean up string -- translate newline and tab characters to spaces.
	Args:		inRcvr
				ioStr		the unicode string
	Return:	cleaned string
------------------------------------------------------------------------------*/

Ref
FCleanString(RefArg inRcvr, RefArg ioStr)
{
	ArrayIndex i, count = RSTRLEN(ioStr);
	CDataPtr p(ioStr);
	UniChar * s = (UniChar *)(char *)p;
	for (i = 0; i < count; i++, s++)
	{
		UniChar  ch = *s;
		if (ch == 0x0D
		||  ch == 0x0A
		||  ch == 0x09)
			*s = ' ';
	}
	return ioStr;
}


/*------------------------------------------------------------------------------
	Split string into words.
	The original implementation is just too horrible to reproduce, so we
	paraphrase more succinctly here.
	Words longer than 127 characters are truncated.
	Args:		inRcvr
				ioStr		the unicode string
	Return:	array of words
				NEVER nil
------------------------------------------------------------------------------*/

Ref
FSplitString(RefArg inRcvr, RefArg inStr)
{
#if 0
	size_t	strLen = Length(inStr);
	char *	str = NewASCIIString(inStr);
	char *	s;
	RefVar	strList(MakeArray(1));
	RefVar	substr = AllocateBinary(SYMA(string), 1);
	LockRef(substr);
	if (strLen > 0)
	{
		ArrayIndex	substrLen = 0;
		ArrayIndex	numOfStrs = 0;
		ArrayIndex	left = StringLeftTrim(inStr);
		ArrayIndex	right = StringRightTrim(inStr);
		for ( ; left < right; left++)
		{
			char  ch = str[left];
			if (ch == ' ')
			{
				if (substrLen > 0)
				{
					UnlockRef(substr);
					SetLength(substr, substrLen + 1);
					LockRef(substr);
					s = BinaryData(substr);
					s[substrLen] = 0;
					SetLength(strList, numOfStrs + 1);
					SetArraySlot(strList, numOfStrs, MakeStringFromCString(s));
					substrLen = 0;
					UnlockRef(substr);
					substr = AllocateBinary(SYMA(string), 1);
					LockRef(substr);
				}
			}
			else
			{
				UnlockRef(substr);
				SetLength(substr, substrLen + 1);
				LockRef(substr);
				BinaryData(substr)[substrLen++] = ch;
			}
		}
		if (substrLen > 0)
		{
			UnlockRef(substr);
			SetLength(substr, substrLen + 1);
			LockRef(substr);
			s = BinaryData(substr);
			s[substrLen] = 0;
			SetLength(strList, numOfStrs + 1);
			SetArraySlot(strList, numOfStrs, MakeStringFromCString(s));
		}
	}
	UnlockRef(substr);
	free(str);
	return strList;
#else

	RefVar	strList(MakeArray(0));
	if (RSTRLEN(inStr) > 0)
	{
		UniChar		ch;
		UniChar *	str = GetUString(inStr);
		UniChar		buf[128];
		UniChar *	word = buf;
		ArrayIndex	wordLen = 0;
		ArrayIndex	left = StringLeftTrim(inStr);
		ArrayIndex	right = StringRightTrim(inStr);
		for ( ; left < right; left++)
		{
			ch = str[left];
			if (ch == ' ')
			{
				if (wordLen > 0)
				{
					*word = 0;
					AddArraySlot(strList, MakeString(buf));
					word = buf;
					wordLen = 0;
				}
			}
			else if (wordLen < 127)
			{
				*word++ = ch;
				wordLen++;
			}
		}

		if (wordLen > 0)
		{
			*word = 0;
			AddArraySlot(strList, MakeString(buf));
		}
	}
	return strList;

#endif
}

// StringDicer very like FSplitString


/*------------------------------------------------------------------------------
	Glue array of words into a string.
	Args:		inRcvr
				inStrs		array of words
	Return:	unicode string
------------------------------------------------------------------------------*/
Ref	NStringCat(RefArg inRcvr, RefArg ioStr1, RefArg inStr2);

Ref
FGlueStrings(RefArg inRcvr, RefArg inStrs)
{
	RefVar str(MakeStringFromCString(""));
	ArrayIndex count = Length(inStrs);
	if (count > 0)
	{
		RefVar spaceStr(MakeStringFromCString(" "));
		for (ArrayIndex i = 0, limit = count - 1; i < count; ++i)
		{
			NStringCat(RA(NILREF), str, GetArraySlot(inStrs, i));
			if (i < limit)
				NStringCat(RA(NILREF), str, spaceStr);
		}
	}
	return str;
}


Ref
PartialGlueString(RefArg inStrs, ULong inIndex, ULong inCount)
{
	// inIndex is 1-based!
	RefVar str(MakeStringFromCString(""));
	if (inCount > 0)
	{
		RefVar spaceStr(MakeStringFromCString(" "));
		for (ArrayIndex i = inIndex - 1, limit = inIndex + inCount - 2; i <= limit; ++i)
		{
			NStringCat(RA(NILREF), str, GetArraySlot(inStrs, i));
			if (i < limit)
				NStringCat(RA(NILREF), str, spaceStr);
		}
	}
	return str;
}


Ref
NStringCat(RefArg inRcvr, RefArg ioStr1, RefArg inStr2)
{
	SetLength(ioStr1, Length(ioStr1) + Length(inStr2) - sizeof(UniChar));	// count only one nul terminator
	Ustrcat(GetUString(ioStr1), GetUString(inStr2));
	return ioStr1;
}


Ref
FStringAssoc(RefArg inRcvr, RefArg inStr, RefArg inArray)
{
	if (IsString(inStr) && IsArray(inArray))
	{
		RefVar item;
		CObjectIterator iter(inArray);
		for ( ; !iter.done(); iter.next())
		{
			item = iter.value();
			if (!IsArray(item))
				break;
			if (NOTNIL(FStrEqual(RA(NILREF), GetArraySlot(item, 0), inStr)))
				return item;
		}
	}
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Return the index of the leftmost non-space character in a string.
	Args:		inStr		the string object
	Return:	ArrayIndex
				if string is all spaces, length of string
----------------------------------------------------------------------------- */

ArrayIndex
StringLeftTrim(RefArg inStr)
{
	ArrayIndex i, count = RSTRLEN(inStr);
	CDataPtr p(inStr);
	UniChar * s = (UniChar *)(char *)p;
	for (i = 0; i < count; i++, s++)
	{
		if (*s != ' ')
			break;
	}
	return i;
}


/* -----------------------------------------------------------------------------
	Return the index BEYOND the rightmost non-space character in a string.
	Args:		inStr		the string object
	Return:	ArrayIndex
				if string is all spaces, zero
----------------------------------------------------------------------------- */

ArrayIndex
StringRightTrim(RefArg inStr)
{
	ArrayIndex	i = RSTRLEN(inStr);
	if (i > 0)
	{
		CDataPtr p(inStr);
		UniChar * s = (UniChar *)(char *)p + i - 1;
		for ( ; i > 0; i--, s--)
		{
			if (*s != ' ')
				break;
		}
	}
	return i;
}


Ref
RemoveTrailingPunct(RefArg inStr)	// RefArg inRcvr stripped
{
	RefVar str;
	size_t numOfChars;
	if (NOTNIL(inStr) && (numOfChars = RSTRLEN(inStr)) < 32 && numOfChars > 0)
	{
		RefVar charStr;
		UniChar s[32];
		UniChar c[2];

		Ustrcpy(s, GetUString(inStr));
		int index = numOfChars;
		int i, end = numOfChars - 1;
		for (i = end; i >= 0; i--)
		{
			c[0] = s[i];
			c[1] = 0;
			charStr = MakeString(c);
			if (ISNIL(FFindStringInArray(RA(NILREF), RA(salutationSuffix), charStr)))	// @249
			{
				// this char isn’t trailing punct: string is valid up to and including this point
				index = i;
				break;
			}
		}
		if (index == end)
			// string has no trailing punct
			str = inStr;
		else if (index < end)
		{
			// terminate string early
			s[index+1] = 0;
			str = MakeString(s);
		}
	//	else string is ALL punct so return nil
	}
	return str;
}

#pragma mark -

Ref
MapSymToFrame(RefArg inSym)	// RefArg inRcvr stripped
{
	RefVar frame = GetGlobalVar(inSym);
	if (ISNIL(frame))
		frame = GetFrameSlot(RA(assistFrames), inSym);
	return frame;
}


/*------------------------------------------------------------------------------
	Add all the words in a template’s lexicon to the dynaTrie.
	Args:		inRcvr stripped
				inTemplate			frame
	Return:	TRUEREF => success
------------------------------------------------------------------------------*/

Ref
MakePhrasalLexEntry(RefArg inTemplate)
{
	RefVar templ8(inTemplate);

	if (!IsFrame(templ8))
		templ8 = MapSymToFrame(templ8);

	if (IsFrame(templ8))
	{
		RefVar	word;
		RefVar	lexicon;
		if (NOTNIL(GetFrameSlot(templ8, SYMA(isa)))
		&&  NOTNIL(lexicon = GetFrameSlot(templ8, SYMA(lexicon))))
		{
			for (ArrayIndex i, count = Length(lexicon); i < count; ++i)
			{
				word = GetArraySlot(lexicon, i);
				TrieAdd(GetUString(word), gDynaTrie, templ8);
			}
		}
		return TRUEREF;
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Remove all the words in a template’s lexicon from the dynaTrie.
	Args:		inRcvr stripped
				inTemplate			frame
	Return:	TRUEREF => success
------------------------------------------------------------------------------*/

Ref
RemovePhrasalLexEntry(RefArg inTemplate)
{
	RefVar templ8(inTemplate);

	if (!IsFrame(templ8))
	{
		gDynaDeleteSym = templ8;
		templ8 = MapSymToFrame(templ8);
	}
	else
		gDynaDeleteSym = NILREF;
	gDynaDeleteFrame = templ8;

	if (IsFrame(templ8)
	&& NOTNIL(GetFrameSlot(templ8, SYMA(isa))))
	{
		RefVar	word;
		RefVar	lexicon;
		if (NOTNIL(lexicon = GetFrameSlot(templ8, SYMA(lexicon))))
		{
			for (ArrayIndex i, count = Length(lexicon); i < count; ++i)
			{
				word = GetArraySlot(lexicon, i);
				if (IsArray(word))
				{
					int j, len = Length(word);
					for (j = 0; j < len; j++)
						DynaTrieDelete(GetArraySlot(word, j));
				}
				else
					DynaTrieDelete(word);
			}
		}
		gDynaDeleteSym = NILREF;
		gDynaDeleteFrame = NILREF;
		return TRUEREF;
		
	}
	gDynaDeleteSym = NILREF;
	gDynaDeleteFrame = NILREF;
	return NILREF;
}


#pragma mark -
/*------------------------------------------------------------------------------
	I n t e l l i g e n t   A s s i s t a n t
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Register a task template with the Assistant.
	Args:		inRcvr
				inTemplate
	Return:	non-nil (actually cloned template) => success
------------------------------------------------------------------------------*/

Ref
FRegTaskTemplate(RefArg inRcvr, RefArg inTemplate)
{
	RefVar theSignature;
	if (FrameHasSlot(inTemplate, SYMA(value))	// original tests NOTNIL !!
	&&  FrameHasSlot(inTemplate, SYMA(isa))
	&&  FrameHasSlot(inTemplate, SYMA(primary_act))
	&&  FrameHasSlot(inTemplate, SYMA(signature))
	&&  FrameHasSlot(inTemplate, SYMA(preconditions))
	&&  FrameHasSlot(inTemplate, SYMA(taskSlip))
	&&  FrameHasSlot(inTemplate, SYMA(PostParse))
	&&  FrameHasSlot(inTemplate, SYMA(score))
	&&  NOTNIL(theSignature = GetFrameSlot(inTemplate, SYMA(signature))))
	{
		RefVar theTemplate(TotalClone(inTemplate));
		for (ArrayIndex i = 0, count = Length(theSignature); i < count; ++i)
			MakePhrasalLexEntry(GetArraySlot(theSignature, i));
		RefVar dynaTemplates(GetGlobalVar(SYMA(dynaTemplates)));
		if (!IsArray(dynaTemplates))
			dynaTemplates = MakeArray(0);
		dynaTemplates = Append(dynaTemplates, theTemplate);
		DefGlobalVar(SYMA(dynaTemplates), dynaTemplates);
		return theTemplate;
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Unregister a task template with the Assistant.
	Args:		inRcvr
				inTemplate
	Return:	non-nil (actually dynaTemplates array) => success
------------------------------------------------------------------------------*/

Ref
FUnRegTaskTemplate(RefArg inRcvr, RefArg inTemplate)
{
	RefVar theSignature(GetFrameSlot(inTemplate, SYMA(signature)));
	if (NOTNIL(theSignature))
	{
		for (ArrayIndex i = 0, count = Length(theSignature); i < count; ++i)
			RemovePhrasalLexEntry(GetArraySlot(theSignature, i));
		RefVar dynaTemplates(GetGlobalVar(SYMA(dynaTemplates)));
		if (IsArray(dynaTemplates))
			return FSetRemove(RA(NILREF), dynaTemplates, inTemplate);
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Filter useless words out of a phrase.
	Args:		inRcvr
				inStrs		array of words constituting the phrase
	Return:	filtered array of words
------------------------------------------------------------------------------*/

Ref
FPhraseFilter(RefArg inRcvr, RefArg inStrs)
{
	if (NOTNIL(inStrs))
	{
		RefVar	badWords = RA(stopWordList);
		RefVar	filteredStrs(MakeArray(0));
		RefVar	str;
		ArrayIndex	strLen;
		for (ArrayIndex i = 0, count= Length(inStrs); i < count; ++i)
		{
			str = GetArraySlot(inStrs, i);
			strLen = RSTRLEN(str);
			if (strLen < 2 || strLen > 4
			|| ISNIL(FFindStringInArray(RA(NILREF), badWords, str)))
			{
				Append(filteredStrs, str);
			}
		}
		if (Length(filteredStrs) > 0)
			return filteredStrs;
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Parse an utterance.  This is the interesting bit!
	Args:		inRcvr
				inString
	Return:	task frame, or nil if parse failed
------------------------------------------------------------------------------*/

Ref
FParseUtter(RefArg inRcvr, RefArg inString)
{
	// open the assistant slip and show the phrase in its entry line
	gRootView->setParsingUtterance(true);
	RefVar assistant(GetFrameSlot(gRootView->fContext, SYMA(assistant)));	//sp18
	FOpenX(assistant);

	RefVar entryLine(GetFrameSlot(assistant, SYMA(assistLine)));
	entryLine = GetFrameSlot(entryLine, SYMA(entryLine));
	FSetValue(assistant, entryLine, SYMA(text), inString);
	gRootView->update();

	// can’t do anything until ink is recognised
	if (IsRichString(inString))
	{
		DoMessage(assistant, SYMA(DeferredRec), RA(NILREF));
		return TRUEREF;
	}

	// keep user informed of progress
	DoMessage(assistant, SYMA(Thinking), RA(NILREF));

	// ignore empty string
	if (IsString(inString) && RSTRLEN(inString) == 0)
	{
		DoMessage(assistant, SYMA(Duh), MakeArray(1));
		return NILREF;
	}

	// cache the request string in the assistant
	SetFrameSlot(assistant, SYMA(matchString), inString);

	RefVar interpretation;	// sp1C
	RefVar taskFrame;			// sp0C
	RefVar sp08;
	int theScore = 0;			// r7

//sp-04 [-4]
	if (NOTNIL(IAInputErrors(RA(NILREF), assistant)))
#if defined(forFramework)
		interpretation = FIAatWork(inRcvr, AllocateFrame());	// since DoProgress requires the view system
#else
		interpretation = NSCall(RA(StartIAProgress));	// -> @296 -> DoProgress -> FIAatWork
#endif

	if (NOTNIL(interpretation))
	{
		if (Length(interpretation) == 0)
		{
#if !defined(forFramework)
			NSCall(RA(IACancelAlert));		// user cancelled -- or actually, any old exception occurred
#endif
			return NILREF;
		}
		// interpretation is an array: [taggedPhrases, rawPhrases, taggedClasses, matchedEntries]
//sp-08 [-0C]
		// create array of all task frames -- start with the built-in ones
		RefVar taskList(Clone(GetFrameSlot(RA(assistFrames), SYMA(task_list))));
//sp-0C [-18]
		// add any user-registered ones
		RefVar dynaTemplates(GetGlobalVar(SYMA(dynaTemplates)));
		if (IsArray(dynaTemplates))
			taskList = UniqueAppendListGen(dynaTemplates, taskList);

		// find the task frame
		RefVar sp10, sp14;
		RefVar sp04(GetArraySlot(interpretation, 2));	// taggedClasses
		ArrayIndex count = Length(sp04);
//sp-30 [-48]
		CObjectIterator iter(sp04);
		for (ArrayIndex i = 0; ISNIL(taskFrame) && !iter.done(); iter.next(), ++i)
		{
			sp10 = iter.value();
			if (NOTNIL(IsAction(sp10)))
			{
				for (ArrayIndex j = i; j < count; ++j)
				{
					sp14 = GetArraySlot(sp04, j);
					if (IsSymbol(sp14))
						sp14 = MapSymToFrame(sp14);
					if (NOTNIL(GetFrameSlot(sp14, SYMA(meta_level))))
						sp10 = sp14;
				}
//sp-30 [-78]
				CObjectIterator iter2(taskList);
				for ( ; !iter2.done(); iter2.next())
				{
					if (NOTNIL(IsPrimaryAct(sp10, iter.value())))
					{
						taskFrame = iter2.value();
						sp08 = GetFrameSlot(taskFrame, SYMA(signature));
						sp08 = CheezySubsumption(sp04, sp08);
					}
				}
//sp+30 [-48]
			}
		}

		if (ISNIL(taskFrame))
		{
			// no exact match -- find the closest
			int score, highestScoreSoFar = -10000;
//sp-30 [-78]
			CObjectIterator iter2(taskList);
			for ( ; !iter2.done(); iter2.next())
			{
				sp14 = GetFrameSlot(iter2.value(), SYMA(signature));
				ArrayIndex r10 = Length(sp14);
				sp14 = CheezySubsumption(sp04, sp14);
				ArrayIndex r0 = NOTNIL(sp14) ? Length(sp14) : 0;
				if (r10 > 0 && r0 > 0)
				{
					score = (r0*2 - count)*1000 / r10;
					if (score > highestScoreSoFar)
					{
						taskFrame = iter2.value();
						sp08 = sp14;
						highestScoreSoFar = score;
					}
				}
			}
			theScore = highestScoreSoFar;
		}
	}
//sp+30 [-48]
//sp+3C [-0C]
//sp+08 [-04]

	if (NOTNIL(taskFrame))
	{
		taskFrame = Clone(taskFrame);
		SetFrameSlot(taskFrame, SYMA(Parse), sp08);
		SetFrameSlot(taskFrame, SYMA(input), GetArraySlot(interpretation, 2));
		SetFrameSlot(taskFrame, SYMA(raw), GetArraySlot(interpretation, 0));
		SetFrameSlot(taskFrame, SYMA(score), MAKEINT(theScore));
		SetFrameSlot(taskFrame, SYMA(phrases), GetArraySlot(interpretation, 1));
		SetFrameSlot(taskFrame, SYMA(noiseWords), UnmatchedWords(RA(NILREF)));
		SetFrameSlot(taskFrame, SYMA(origPhrase), FOrigPhrase(RA(NILREF)));
		SetFrameSlot(taskFrame, SYMA(entries), GetArraySlot(interpretation, 3));
		FillPreconditions(taskFrame);
// can’t PostParse yet b/c this needs apps and the view system
//		DoMessage(taskFrame, SYMA(PostParse), RA(NILREF));
		return taskFrame;
	}

	// it didn’t work out
	DoMessage(assistant, SYMA(Duh), MakeArray(1));
	return NILREF;
}


/*------------------------------------------------------------------------------
	Check a string for IA errors.
	String must be shorter than 16 words.
	Args:		inRcvr
				inAssistant
	Return:	TRUEREF => string is okay
------------------------------------------------------------------------------*/

Ref
IAInputErrors(RefArg inRcvr, RefArg inAssistant)
{
	// the assistant must have the string -- we’ll read it from there later
	RefVar str(GetFrameSlot(inAssistant, SYMA(matchString)));
	if (ISNIL(str))
		return NILREF;

	// the string must have something in it
	ArrayIndex strLen = RSTRLEN(str);
	if (strLen == 1)
		return NILREF;

	// clean up the string
	str = FCleanString(RA(NILREF), str);
	SetFrameSlot(inAssistant, SYMA(matchString), str);

	// if the string contains only whitespace, reject it
	UniChar spaceChar = U_CONST_CHAR(0x20);
	CDataPtr strData(str);
	UniChar * s = (UniChar *)(char *)strData;
	bool isWS = true;
	for (ArrayIndex i = 0; i < strLen; ++i)
		if (s[i] != spaceChar)
			{ isWS = false; break; }
	if (isWS)
		return NILREF;

	// we can’t cope with more than 15 words
	if (Length(FSplitString(RA(NILREF), str)) > 15)
		return NILREF;

	return TRUEREF;
}


/*------------------------------------------------------------------------------
	Perform intellignt assistance.
	Args:		inRcvr
				inProgressSlip		progress gauge to be updated during phrase interpretation
	Return:	NILREF => phrase could not be interpreted
				array[0] => error/cancellation during interpretation
				array[4] => interpretation result
								[taggedPhrases, rawPhrases, taggedClasses, matchedEntries]
------------------------------------------------------------------------------*/

Ref
FIAatWork(RefArg inRcvr, RefArg inProgressSlip)
{
	RefVar interpretation;

	newton_try
	{
		RefVar taggedPhrases(MakeArray(0));
		RefVar rawPhrases(MakeArray(0));

		RefVar state(Clone(GetFrameSlot(RA(assistFrames), SYMA(entries))));
		SetFrameSlot(state, SYMA(exception), MakeArray(Length(RA(DSExceptions))));

		RefVar assistant(GetFrameSlot(gRootView->fContext, SYMA(assistant)));
		IPhraseGenerator(GetFrameSlot(assistant, SYMA(matchString)));

		RefVar aPhrase;
		for (aPhrase = NextPhrase(); NOTNIL(aPhrase); aPhrase = NextPhrase())
		{
#if !defined(forFramework)
			NSCall(RA(TickleIAProgress), inProgressSlip);
#endif
			RefVar cleanStr(RemoveTrailingPunct(aPhrase));
			if (NOTNIL(cleanStr) && RSTRLEN(cleanStr) > 0)
			{
				RefVar taggedPhrase(MatchString(gTrie, GetUString(cleanStr), state));
				if (NOTNIL(taggedPhrase))
				{
					PhraseHitExt();
					Append(taggedPhrases, taggedPhrase);
					Append(rawPhrases, aPhrase);
					for (ArrayIndex i, count = Length(taggedPhrase); i < count; ++i)
						RecordHistory(GetArraySlot(taggedPhrase, i));
				}
			}
		}

#if defined(forDebug)
		PrintGeneratorState();
#endif
		if (Length(taggedPhrases) > 0)
		{
			RefVar matchedEntries(Clone(GetFrameSlot(RA(assistFrames), SYMA(matched))));
			SetFrameSlot(matchedEntries, SYMA(person), GetFrameSlot(state, SYMA(person)));
			SetFrameSlot(matchedEntries, SYMA(places), GetFrameSlot(state, SYMA(places)));

			interpretation = MakeArray(4);
			SetArraySlot(interpretation, 0, taggedPhrases);
			SetArraySlot(interpretation, 1, rawPhrases);
			SetArraySlot(interpretation, 2, GetClasses(taggedPhrases));
			SetArraySlot(interpretation, 3, matchedEntries);

			// clear the request
			SetFrameSlot(assistant, SYMA(matchString), RA(NILREF));
		}
	}
	newton_catch(exRootException)
	{
		interpretation = MakeArray(0);
	}
	end_try;

	return interpretation;
}


Ref
FillPreconditions(RefArg ioTaskTemplate)	// RefArg inRcvr stripped
{
	RefVar templates(GetFrameSlot(ioTaskTemplate, SYMA(signature)));			// array of templates
	ArrayIndex numOfPreconditions = Length(templates);
	RefVar templateTags(GetFrameSlot(ioTaskTemplate, SYMA(preconditions)));	// array of symbols
//	ArrayIndex numOfPreconditions = Length(preconditions);								// *must* be the same
	RefVar wordsInPhrase(GetFrameSlot(ioTaskTemplate, SYMA(input)));			// array of words
	ArrayIndex numOfWordsInPhrase = Length(wordsInPhrase);
	RefVar sp10(GetFrameSlot(ioTaskTemplate, SYMA(raw)));	// unused
//	ArrayIndex r0 = Length(sp10);										// unused
	RefVar matchedWords;												// unused!
	RefVar theWord;
	RefVar theTemplate;

	size_t numOfWordsToMatch = numOfWordsInPhrase;
	size_t matchedWordsArraySize = (numOfWordsInPhrase > 16) ? 16 : numOfWordsInPhrase;
	for (ArrayIndex i = 0; i < numOfPreconditions && numOfWordsToMatch > 0; ++i)
	{
		matchedWords = MakeArray(matchedWordsArraySize);
		size_t numOfMatchedWords = 0;
		theTemplate = GetArraySlot(templates, i);
		for (ULong j = 0; j < numOfWordsInPhrase; j++)
		{
			theWord = GetArraySlot(wordsInPhrase, j);
			if (NOTNIL(FIsA(RA(NILREF), theWord, theTemplate)))
			{
				SetArraySlot(matchedWords, numOfMatchedWords++, theWord);
				numOfWordsToMatch--;
			}
		}
		if (numOfMatchedWords > 0)
		{
			SetLength(matchedWords, numOfMatchedWords);
			SetFrameSlot(ioTaskTemplate, GetArraySlot(templateTags, i), theTemplate);
		}
	}
	return TRUEREF;
}


Ref
CheezySubsumption(RefArg inArray1, RefArg inArray2)	// RefArg inRcvr stripped
{
	if (NOTNIL(inArray1) && NOTNIL(inArray2))
	{
		RefVar array1(Clone(inArray1));
		ArrayIndex array1Len = Length(inArray1);
		ArrayIndex array2Len = Length(inArray2);
		if (array1Len > 0 && array2Len > 0)
		{
			RefVar subsumption(MakeArray(0));
			RefVar elem1, elem2;
			RefVar a;
			for (ArrayIndex j = 0; j < array2Len; ++j)
			{
				elem2 = GetArraySlot(inArray2, j);
				a = MakeArray(0);
				for (ArrayIndex i = 0; i < array1Len; ++i)
				{
					elem1 = GetArraySlot(array1, i);
					if (NOTNIL(FIsA(RA(NILREF), elem1, elem2)))
					{
						AddArraySlot(a, elem1);
						SetArraySlot(array1, i, RA(NILREF));
					}
				}
				if (Length(a) > 0)
					AddArraySlot(subsumption, a);
			}
			return subsumption;
		}
	}
	return NILREF;
}


Ref
CheezyIntersect(RefArg inArray1, RefArg inArray2)	// RefArg inRcvr stripped
{
	if (NOTNIL(inArray1) && NOTNIL(inArray2))
	{
		ArrayIndex array1Len = Length(inArray1);
		ArrayIndex array2Len = Length(inArray2);
		if (array1Len > 0 && array2Len > 0)
		{
			RefVar intersection(MakeArray(0));
			RefVar elem1, elem2;
			RefVar a1, a2;
			ArrayIndex a1Count, a2Count;
			if (array1Len > array2Len)
			{
				a1 = inArray1;
				a1Count = array1Len;
				a2 = inArray2;
				a2Count = array2Len;
			}
			else
			{
				a1 = inArray2;
				a1Count = array2Len;
				a2 = inArray1;
				a2Count = array1Len;
			}

			for (ArrayIndex i = 0; i < a1Count; ++i)
			{
				elem1 = GetArraySlot(a1, i);
				for (ArrayIndex j = 0; j < a2Count; ++j)
				{
					elem2 = GetArraySlot(a2, j);
					if (EQ(elem1, elem2))
						AddArraySlot(intersection, elem1);
						// should we break here? any point in duplicates?
				}
			}
			return intersection;
		}
	}
	return NILREF;
}


Ref
GetClasses(RefArg inPhrases)
{
	RefVar aPhrase;		// sp08
	RefVar phraseClass;	// sp0C
	RefVar sp00;
	ArrayIndex count = Length(inPhrases);
	RefVar theClasses(MakeArray(count));	//sp04
	for (ArrayIndex i = 0; i < count; ++i)
	{
		aPhrase = GetArraySlot(inPhrases, i);
		phraseClass = CompositeClass(RA(NILREF), aPhrase);
		sp00 = NILREF;
		if (NOTNIL(phraseClass) && NOTNIL(FIsA(RA(NILREF), phraseClass, gActionClass)))
			sp00 = phraseClass;
		if (ISNIL(phraseClass) && ISNIL(sp00))
		{
			phraseClass = FavorAction(RA(NILREF), aPhrase);
			sp00 = phraseClass;
		}
		if (ISNIL(phraseClass) && NOTNIL(sp00))
		{
			if (IsSymbol(sp00))
				sp00 = MapSymToFrame(sp00);
			phraseClass = NOTNIL(GetFrameSlot(sp00, SYMA(meta_level))) ? FavorAction(RA(NILREF), aPhrase)
																						  : FavorObject(RA(NILREF), aPhrase);
		}
		SetArraySlot(theClasses, i, phraseClass);
	}
	return theClasses;
}


Ref
PathToRoot(RefArg inRcvr, RefArg inFrame)
{
	RefVar element(inFrame);
	if (IsSymbol(element))
		element = MapSymToFrame(element);
	if (NOTNIL(element))
	{
		RefVar path(MakeArray(1));
		SetArraySlot(path, 0, element);
		while (NOTNIL(element = GetFrameSlot(element, SYMA(isa))))
		{
			if (IsSymbol(element))
				element = MapSymToFrame(element);
			AddArraySlot(path, element);
		}
		return path;
	}
	return NILREF;
}


Ref
CommonAncestors(RefArg inRcvr, RefArg inFrame1, RefArg inFrame2)
{
	RefVar ancestors(CheezyIntersect(PathToRoot(RA(NILREF), inFrame1), PathToRoot(RA(NILREF), inFrame2)));
	if (NOTNIL(ancestors) && Length(ancestors) > 0)
		return ancestors;
	return NILREF;
}


Ref
CompositeClass(RefArg inRcvr, RefArg inPhrase)
{
	ArrayIndex count = Length(inPhrase);
	if (count == 1)
		return GetArraySlot(inPhrase, 0);

	RefVar ancestors(CommonAncestors(RA(NILREF), GetArraySlot(inPhrase, 0), GetArraySlot(inPhrase, 1)));
	if (ISNIL(ancestors))
		return NILREF;

	for (ArrayIndex i = 2; i < count; ++i)
	{
		ancestors = CommonAncestors(RA(NILREF), GetArraySlot(ancestors, 0), GetArraySlot(inPhrase, i));
		if (ISNIL(ancestors))
			return NILREF;
	}

	return GetArraySlot(ancestors, 0);
}


Ref
FavorAction(RefArg inRcvr, RefArg inPhrase)
{
	RefVar ancestors;
	for (ArrayIndex i = 0, count = Length(inPhrase); i < count; ++i)
	{
		ancestors = CommonAncestors(RA(NILREF), gActionClass, GetArraySlot(inPhrase, i));
		if (NOTNIL(ancestors) && EQ(gActionClass, GetArraySlot(ancestors, 0)))
			return GetArraySlot(inPhrase, i);
	}
	return NILREF;
}


Ref
FavorObject(RefArg inRcvr, RefArg inPhrase)
{
	RefVar ancestors;
	for (ArrayIndex i = 0, count = Length(inPhrase); i < count; ++i)
	{
		ancestors = CommonAncestors(RA(NILREF), gObjectClass, GetArraySlot(inPhrase, i));
		if (NOTNIL(ancestors) && EQ(gObjectClass, GetArraySlot(ancestors, 0)))
			return GetArraySlot(inPhrase, i);
	}
	return NILREF;
}


/*------------------------------------------------------------------------------
	Generate full list of recognised IA actions.
	Args:		inRcvr
	Return:	array of action strings
------------------------------------------------------------------------------*/

Ref
FGenFullCommands(RefArg inRcvr)
{
	RefVar taskList(Clone(GetFrameSlot(RA(assistFrames), SYMA(task_list))));
	RefVar dynaTemplates(GetGlobalVar(SYMA(dynaTemplates)));
	if (IsArray(dynaTemplates))
		taskList = UniqueAppendListGen(dynaTemplates, taskList);

	RefVar aTask;
	RefVar action;
	RefVar cmdList(MakeArray(0));
	for (ArrayIndex i = 0, count = Length(taskList); i < count; ++i)
	{
		aTask = GetArraySlot(taskList, i);
		if (!EQ(aTask, GetFrameSlot(RA(assistFrames), SYMA(default_task)))
		&&  !EQ(aTask, GetFrameSlot(RA(assistFrames), SYMA(about_task))))
		{
			action = GetFrameSlot(aTask, SYMA(primary_act));
			if (IsSymbol(action))
				action = MapSymToFrame(action);
			if (NOTNIL(FrameHasSlot(action, SYMA(lexicon))))
			{
				action = GetFrameSlot(action, SYMA(lexicon));
				if (IsArray(action))
					action = GetArraySlot(action, 0);
				if (IsArray(action))
					action = GetArraySlot(action, 0);
				if (NOTNIL(action))
					UniqueAppendString(cmdList, action);
			}
		}
	}
	if (Length(cmdList) > 0)
		return cmdList;
	return NILREF;
}


/*------------------------------------------------------------------------------
	Test whether a task frame conforms to a template.
	Originally ISATest
	Args:		inRcvr
				inFrame
				inTemplate
	Return:	TRUEREF => frame isa tamplate
------------------------------------------------------------------------------*/

Ref
FIsA(RefArg inRcvr, RefArg inFrame, RefArg inTemplate)
{
	RefVar super;
	RefVar frame(inFrame);
	RefVar templ8(inTemplate);

	if (IsSymbol(frame))
		frame = MapSymToFrame(frame);
	if (IsSymbol(templ8))
		templ8 = MapSymToFrame(templ8);
	if (NOTNIL(frame) && NOTNIL(templ8))
	{
		if (EQ(frame, templ8))
			return TRUEREF;
		if (EQ(GetFrameSlot(frame, SYMA(isa)), GetFrameSlot(templ8, SYMA(isa)))
		&&  EQ(GetFrameSlot(frame, SYMA(lexicon)), GetFrameSlot(templ8, SYMA(lexicon))))
			return TRUEREF;

		for (super = GetFrameSlot(frame, SYMA(isa)); NOTNIL(super); super = GetFrameSlot(super, SYMA(isa)))
		{
			if (IsSymbol(super))
				super = MapSymToFrame(super);
			if (EQ(super, templ8))
				return TRUEREF;
		}
	}
	return NILREF;
}


Ref
AddEntry(RefArg inRcvr, RefArg inTag, RefArg inPath, RefArg inAlias, RefArg ioState)
{
	RefVar arrayTag;
	RefVar flagTag;
	if (NOTNIL(FIsA(RA(NILREF), inTag, SYMA(person))))
	{
		if (NOTNIL(GetFrameSlot(ioState, SYMA(personAdded))))
			return NILREF;
		arrayTag = SYMA(person);
		flagTag = SYMA(personAdded);
	}
	else
	{
		if (NOTNIL(GetFrameSlot(ioState, SYMA(placeAdded))))
			return NILREF;
		arrayTag = SYMA(places);
		flagTag = SYMA(placeAdded);
	}

	// create array to hold entry if necessary
	if (ISNIL(GetFrameSlot(ioState, arrayTag)))
		SetFrameSlot(ioState, arrayTag, MakeArray(0));

	// create entry frame
	RefVar theEntry(Clone(GetFrameSlot(RA(assistFrames), SYMA(IARef))));
	SetFrameSlot(theEntry, SYMA(alias), inAlias);
	SetFrameSlot(theEntry, SYMA(class), inTag);
	if (NOTNIL(inPath))
		SetFrameSlot(theEntry, SYMA(path), inPath);

	// add the entry and flag it
	AddArraySlot(GetFrameSlot(ioState, arrayTag), theEntry);
	SetFrameSlot(ioState, flagTag, MAKEINT(1));

	return NILREF;
}


Ref
DSGetMatchedEntries(RefArg inRcvr, RefArg inWhich, RefArg inEntries)
{
	RefVar matchedEntries;
	RefVar whichEntries;

	if (EQ(inWhich, SYMA(person))
	||  EQ(inWhich, SYMA(allEntries)))
	{
		whichEntries = GetFrameSlot(inEntries, SYMA(person));
		if (NOTNIL(whichEntries))
			matchedEntries = FSetUnion(RA(NILREF), matchedEntries, whichEntries, RA(TRUEREF));
	}

	if (EQ(inWhich, SYMA(places))
	||  EQ(inWhich, SYMA(allEntries)))
	{
		whichEntries = GetFrameSlot(inEntries, SYMA(places));
		if (NOTNIL(whichEntries))
			matchedEntries = FSetUnion(RA(NILREF), matchedEntries, whichEntries, RA(TRUEREF));
	}
	return matchedEntries;
}

