/*
	File:		Recognizers.cc

	Contains:	CRecognizer implementation.

	Written by:	Newton Research Group, 2010.
*/

#include "Objects.h"
#include "Globals.h"
#include "Arrays.h"
#include "Lookup.h"
#include "Recognition.h"
#include "WordRecognizer.h"
#include "Dictionaries.h"
#include "UStringUtils.h"
#include "ViewFlags.h"
#include "Responder.h"

extern bool		DomainOn(CRecArea * inArea, ULong inId);


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

ULong		gRecMemErrCount;			// 0C104F78
Heap		gWRecHeap;					// 0C104F7C
Heap		gWRecTaskDefaultHeap;	// 0C104F80


/* -----------------------------------------------------------------------------
	Initialize the recognizer.
----------------------------------------------------------------------------- */

void
InstallWRecRecognizer(CRecognitionManager * inManager)
{
#if defined(correct)
	if (ISNIL(GetPreference(SYMA(inhibitBaseROMWRecRegistration))))
		RegisterWRec();
	if (ClassInfoByName("CWRecognizer", NULL) != NULL)
#endif
	{
		CWRecDomain *		domain = CWRecDomain::make(inManager->controller());
		CWRecRecognizer * recognizer = new CWRecRecognizer;
		recognizer->init(domain, domain->getType(), aeWord, 1, 1);
		recognizer->initServices(vWordsAllowed, 0x00000000);
		inManager->addRecognizer(recognizer);

		recognizer->sleep();
	}
}


#if defined(hasParaGraphRecognizer)
void
InstallWordRecognizer(CRecognitionManager * inManager)
{
	CStrXrDomain *		strDomain = CStrXrDomain::make(inManager->controller());
	CXrWordDomain *	wordDomain = CXrWordDomain::make(inManager->controller());
	CWordRecognizer * recognizer = new CWordRecognizer;
	recognizer->init(domain, domain->getType(), aeWord, 1, 1);
	recognizer->initServices(vWordsAllowed, 0x00000000);
	inManager->addRecognizer(recognizer);

	recognizer->wakeUp();
	recognizer->sleep();

	XTRY
	{
		CUGestalt	gestalt;
		ULong * hasParaGraphRecognizer = new ULong;
		XFAIL(hasParaGraphRecognizer == NULL)
		*(bool*)hasParaGraphRecognizer = true;
		gestalt.registerGestalt(kGestalt_Ext_ParaGraphRecognizer, hasParaGraphRecognizer, sizeof(hasParaGraphRecognizer));
	}
	XENDTRY;
}
#endif

#pragma mark -

ArrayIndex
GetFirstWordIndex(CStdWordUnit * inUnit)
{
	for (ArrayIndex i = 0, count = inUnit->interpretationCount(); i < count; ++i)
	{
		if (inUnit->getLabel(i) != 40)
			return i;
	}
	return kIndexNotFound;
}


ULong
WordRecognizerHandleUnit(CRecognizer * inRecognizer, CUnit * inUnit)
{return 0;}	// INCOMPLETE


#pragma mark CWRecDomain
/* -----------------------------------------------------------------------------
	C W R e c D o m a i n
	Recognition is implemented by a recognition domain;
	CWRecognizer protocol does yer actual word recognition.
----------------------------------------------------------------------------- */

CWRecDomain *
CWRecDomain::make(CController * inController)
{
	CWRecDomain * domain = new CWRecDomain;
	XTRY
	{
		XFAIL(domain == NULL)
		newton_try
		{
			domain->iWRecDomain(inController);
		}
		newton_catch_all
		{
			SetHeap(gWRecTaskDefaultHeap);
//			if (domain->f24)
//				domain->f24->destroy();
			domain->release(), domain = NULL;
		}
		end_try;
	}
	XENDTRY;
	return domain;
}


NewtonErr
CWRecDomain::iWRecDomain(CController * inController)
{
	NewtonErr err;

	iRecDomain(inController, 'WREC', "Protocol-based Word-rec");
	f24 = NULL;

	gWRecTaskDefaultHeap = GetHeap();
	if ((err = NewVMHeap(0, 221*KByte, &gWRecHeap, 0)))
		gWRecHeap = NULL;
/*
	CWRecognizer * wRec = MakeByName(NULL, "CWRecognizer");
	if (wRec == NULL)
		Throw(exAbort, NULL);
	wRec->f10 = this;
	f24 = wRec;

	SetHeap(gWRecHeap);
	wRec->initialize();
*/
	SetHeap(gWRecTaskDefaultHeap);
	setFlags(0x80000000);

	addPieceType('STRK');
	fDelay = 120;
	inController->registerDomain(this);
	return noErr;
}


bool
CWRecDomain::group(CRecUnit * inUnit, RecDomainInfo * info)
{return false;}


void
CWRecDomain::classify(CRecUnit * inUnit)
{}


void
CWRecDomain::reclassify(CRecUnit * inUnit)
{}


void
CWRecDomain::configureArea(RefArg, OpaqueRef)
{}


ULong
CWRecDomain::unitConfidence(CSIUnit *)
{return 0;}


void
CWRecDomain::unitInfoFreePtr(Ptr)
{}


void
CWRecDomain::verifyWordSymbols(UniChar *)
{}


void
CWRecDomain::sleep(void)
{}


void
CWRecDomain::wakeUp(void)
{}


void
CWRecDomain::signalMemoryError(void)
{}


void
CWRecDomain::domainParameter(ULong, OpaqueRef, OpaqueRef)
{}


bool
CWRecDomain::setParameters(char ** inParms)
{return 0;}


#pragma mark -
#pragma mark CStdWordUnit
/* -----------------------------------------------------------------------------
	C S t d W o r d U n i t
----------------------------------------------------------------------------- */

NewtonErr
CStdWordUnit::iStdWordUnit(CRecDomain * inDomain, ULong inSubtype, CArray * inAreas, ULong inInterpSize)
{
	return iSIUnit(inDomain, inDomain->getType(), inSubtype, inAreas, inInterpSize);
}


size_t
CStdWordUnit::sizeInBytes(void)
{
	size_t thisSize = 0;
	for (ArrayIndex i = 0, count = interpretationCount(); i < count; ++i)
	{
		Ptr str = getString(i);
		if (str)
			thisSize = GetPtrSize(str);
	}

	thisSize += (sizeof(CStdWordUnit) - sizeof(CSIUnit));	// original doesn’t need sizeof evaluation b/c it uses dynamic Ptr size
	return CSIUnit::sizeInBytes() + thisSize;
}


void
CStdWordUnit::dump(CMsg * outMsg)
{ }


void
CStdWordUnit::endUnit(void)
{
	endSubs();
	compactInterpretations();
}


void
CStdWordUnit::setCharWordString(ArrayIndex index, char * inStr)
{
	char * str = getString(index);
	if (str)
	{
		size_t strLen = (strlen(inStr) + 1) * sizeof(UniChar);
		if ((str = ReallocPtr(str, strLen)))	// need to assign this!
		{
			ConvertToUnicode(inStr, (UniChar *)str);
			str[strLen] = 0;
		}
	}
}


void
CStdWordUnit::setWordString(ArrayIndex index, UniChar * inStr)
{
	char * str = getString(index);
	if (str)
	{
		size_t strLen = (Ustrlen(inStr) + 1) * sizeof(UniChar);
		if ((str = ReallocPtr(str, strLen)))	// need to assign this!
		{
			Ustrcpy((UniChar *)str, inStr);
			str[strLen] = 0;
		}
	}
}


char *	// was Handle
CStdWordUnit::getString(ArrayIndex index)
{
	WordInterpretation * interp = (WordInterpretation *)getInterpretation(index);
	if (interp != NULL)
		return interp->word;
	return NULL;
}


void
CStdWordUnit::setParam(UnitInterpretation*, ArrayIndex index, Ptr)
{ /* we don’t use params */ }


CArray *
CStdWordUnit::getParam(ArrayIndex index)
{ return NULL; }


void
CStdWordUnit::addWordInterpretation(void)
{ insertWordInterpretation(kWordDelimiter); }


ArrayIndex
CStdWordUnit::insertWordInterpretation(ArrayIndex index)
{
	char * wordStr = NewPtr(10 * sizeof(UniChar));	// was Handle
	if (wordStr != NULL)
	{
		wordStr[0] = 0;
		SetPtrName(wordStr, 'istr');
		ArrayIndex interpIndex = insertInterpretation(index);
		WordInterpretation * interp = (WordInterpretation *)getInterpretation(interpIndex);
		if (interp != NULL)
		{
			InitInterpretation(interp, 0, 0);
			interp->word = wordStr;
			return interpIndex;
		}
	}
	return kIndexNotFound;
}


int
CStdWordUnit::deleteInterpretation(ArrayIndex index)
{
	WordInterpretation * interp = (WordInterpretation *)getInterpretation(index);
	if (interp != NULL)
	{
		FreePtr(interp->word), interp->word = NULL;
		interp->label = kNoLabel;
		CSIUnit::deleteInterpretation(index);
	}
	return 0;
}


void
CStdWordUnit::reinforceWordChoice(long)
{ }


void
CStdWordUnit::getWordBase(FPoint * outPt1, FPoint * outPt2, ULong inArg3)
{
	FRect box;
	getBBox(&box);
	outPt1->x = box.left;
	outPt1->y = box.bottom;
	outPt2->x = box.right;
	outPt2->y = box.bottom;
}


int
CStdWordUnit::getWordSlant(ULong)
{ return 0; }

int
CStdWordUnit::getWordSize(ULong)
{ return 0; }


Ptr
CStdWordUnit::getTrainingData(long inArg)
{ return NULL; }


void
CStdWordUnit::disposeTrainingData(Ptr inData)
{ }


#pragma mark -
#pragma mark CWRecUnit
/* -----------------------------------------------------------------------------
	C W R e c U n i t
	Original class hierarchy looks like
		TWRecUnit <- TRecUnit <- TStdWordUnit <- TSIUnit <- TUnit.
	We’ve eliminated the TRecUnit layer which would clash names anyway.
----------------------------------------------------------------------------- */

CWRecUnit *
CWRecUnit::make(CRecDomain * inDomain, ULong inType, CArray * inAreas)
{
	CWRecUnit * wRecUnit = new CWRecUnit;
	XTRY
	{
		XFAIL(wRecUnit == NULL)
		XFAILIF(wRecUnit->iWRecUnit(inDomain, inType, inAreas) != noErr, wRecUnit->release(); wRecUnit = NULL;)
	}
	XENDTRY;
	return wRecUnit;
}


NewtonErr
CWRecUnit::iWRecUnit(CRecDomain * inDomain, ULong inType, CArray * inAreas)
{
	f3C = NULL;
	return iStdWordUnit(inDomain, inType, inAreas, 16);
}


void
CWRecUnit::dealloc(void)
{
	if (f3C)
		((CWRecDomain *)fDomain)->unitInfoFreePtr(f3C);
	// super dealloc
	CStdWordUnit::dealloc();
}


#pragma mark -
#pragma mark CWRecRecognizer
/* -----------------------------------------------------------------------------
	C W R e c R e c o g n i z e r
----------------------------------------------------------------------------- */

ULong
CWRecRecognizer::unitConfidence(CUnit * inUnit)
{
	return ((CWRecDomain *)domain())->unitConfidence((CSIUnit *)inUnit->siUnit());
}


void
CWRecRecognizer::sleep(void)
{
	((CWRecDomain *)domain())->sleep();
}


void
CWRecRecognizer::wakeUp(void)
{
	((CWRecDomain *)domain())->wakeUp();
}


int
CWRecRecognizer::configureArea(CRecArea * inArea, RefArg inArg2)
{
	if (DomainOn(inArea, id()))
	{
		((CWRecDomain *)domain())->configureArea(inArg2, (OpaqueRef)inArea->getInfoFor('WREC', true));

		CDictChain * sp00[3];
		BuildChains(sp00, inArg2);
		for (ArrayIndex i = 0; i < 3; ++i)
			inArea->setChain(i, sp00[i]);
		inArea->paramsAllSet('WREC');
	}
	return 0;
}


ULong
CWRecRecognizer::handleUnit(CUnit * inUnit)
{
	return WordRecognizerHandleUnit(this, inUnit);
}


#pragma mark -
#pragma mark TryString
/* -----------------------------------------------------------------------------
	T r y S t r i n g
	The gTryString global UniChar string holds a short string for testing.
----------------------------------------------------------------------------- */

UniChar		gTryString[2+1];	// 0C104D64
ArrayIndex	gTryIndex;			// 0C104D6C


size_t
TryStringLength(void)
{
	return Ustrlen(gTryString);
}


void
ClearTryString(void)
{
	gTryString[0] = 0;
	gTryIndex = 0;
}


bool
InTryString(UniChar inCh)
{
	UniChar * tryStr;
	for (tryStr = gTryString; *tryStr != 0; tryStr++)
	{
		if (*tryStr == inCh)
			return true;
	}
	return false;
}


void
AddTryString(UniChar inCh)
{
	if (InTryString(inCh))
		ClearTryString();

	if (Ustrlen(gTryString) < 2)
	{
		gTryString[gTryIndex++] = inCh;
		gTryString[gTryIndex] = 0;
	}
	else
	{
		gTryString[gTryIndex] = inCh;
		gTryIndex = 0;
	}
}


#pragma mark -
#pragma mark CWordList
/* -----------------------------------------------------------------------------
	C W o r d L i s t
	A word list contains up to 16 UniChar words, separated by 0xFFFF and
	terminated by 0x0000. Each word can be assigned a label and a score.
	Has its own operators new & delete.
----------------------------------------------------------------------------- */
char gPreallocWordLists[sizeof(CWordList)*12];

void *
CWordList::operator new(size_t inSize)
{
	CWordList * wlist = (CWordList *)gPreallocWordLists;
	for (ArrayIndex i = 0; i < 12; ++i)
	{
		if (wlist->fWords == NULL)
			return wlist;
	}
	return ::new CWordList;
}

void
CWordList::operator delete(void * inPtr)
{
	*((CWordList *)inPtr)->fWords = NULL;
	if (inPtr >= gPreallocWordLists && inPtr < gPreallocWordLists + sizeof(CWordList)*12)
		return;	// don’t delete a preallocated word list!
	delete (CWordList *)inPtr;
}


CWordList::CWordList()
{
	fNumOfWords = 0;
	fWords = (UniChar *)NewPtr(sizeof(UniChar));	// was Handle
	if (fWords)
	{
		SetPtrName((Ptr)fWords, 'wlst');
		fWords[0] = kListDelimiter;
	}
}


CWordList::~CWordList()
{
	if (fWords)
		FreePtr((Ptr)fWords);
}


ArrayIndex
CWordList::count(void)
{
	return fNumOfWords;
}


ULong
CWordList::score(ArrayIndex index)
{
	return fScores[index];
}


ULong
CWordList::label(ArrayIndex index)
{
	return fLabels[index];
}


UniChar *
CWordList::scanTo(ArrayIndex index)
{
	UniChar ch;
	UniChar * wrd = fWords;
	for ( ; index > 0; index--)
	{
		while ((ch = *wrd++) != kWordDelimiter)	// end of word
		{
			if (ch == kListDelimiter)					// end of list
			{
				wrd = NULL;
				index = 0;
				break;
			}
		}
	}
	return wrd;
}


UniChar *	// was Handle
CWordList::word(ArrayIndex index)
{
	UniChar * wrd = scanTo(index);
	// should check non-NULL, surely?
	Size wrdLen = (Ustrlen(wrd) + 1) * sizeof(UniChar);
	UniChar * copiedWord = (UniChar *)NewPtr(wrdLen);	// was Handle
	SetPtrName((Ptr)copiedWord, 'wrdw');
	Ustrcpy(copiedWord, wrd);
	return copiedWord;
}


UniChar *
CWordList::ith(ArrayIndex index, ULong * outScore, ULong * outLabel)
{
	UniChar * wrd = word(index);
	SetPtrName((Ptr)wrd, 'wrdl');
	if (outScore)
		*outScore = fScores[index];
	if (outLabel)
		*outLabel = fLabels[index];
	return wrd;
}


void
CWordList::insertLast(UniChar * inWord, ULong inScore, ULong inLabel)	// inWord was UniChar **
{
	if (fNumOfWords < 16)
	{
		ArrayIndex wrdLen = Ustrlen(inWord) + 1;
		ArrayIndex lstLen = 0;
		if (fNumOfWords > 0)
		{
			lstLen = Ustrlen(fWords);
			fWords[lstLen++] = kWordDelimiter;
		}
		fWords = (UniChar *)ReallocPtr((Ptr)fWords, lstLen + wrdLen);
		memcpy(fWords + lstLen, inWord, wrdLen);
		fScores[fNumOfWords] = inScore;
		fLabels[fNumOfWords] = inLabel;
		fNumOfWords++;
	}
}


ArrayIndex
CWordList::find(UniChar * inWord)	// inWord was UniChar **
{
	UniChar ch;
	UniChar * wrd = fWords;
	for (ArrayIndex index = 0, ch = *wrd; ch != kListDelimiter; index++)
	{
		bool isFound = true;
		UniChar * wrdToFind = inWord;
		while ((ch = *wrd++) != kWordDelimiter && ch != kListDelimiter)
		{
			if (*wrdToFind++ != ch)
				isFound = false;
		}
		if (isFound && *wrdToFind == 0)
			return index;
	}
	return kIndexNotFound;
}


bool
IsSlash(UniChar inCh)
{
	return inCh == '/';
//		 || inCh == '\';	// why not backslash?
}

bool
IsCircular(UniChar inCh)
{
	return inCh == 'o'
		 || inCh == 'O'
		 || inCh == '0';
}

bool
IsLinear(UniChar inCh)
{
	return inCh == '1'
		 || inCh == 'I'
		 || inCh == 'l'
		 || inCh == 'i'
		 || inCh == '|';
}


void
CWordList::bubbleGuess(UniChar inCh, CharShapeProcPtr inTest, int inDirection)
{
	UniChar str[2];
	str[0] = inCh;
	str[1] = 0;
	ArrayIndex numOfWords = count();
	int incr, limit;
	int index = find(str);
	if (index != kIndexNotFound)
	{
		if (inDirection)
		{
			incr = -1;	limit = -1;
		}
		else
		{
			incr = 1;	limit = numOfWords;
		}
		index += incr;
		if (index >= 0 && index < numOfWords)
		{
			for ( ; index != limit; index += incr)
			{
				UniChar * wrd = scanTo(index);
				if (Ustrlen(wrd) == 1
				&&  inTest(wrd[0]))
					swapSingleCharacterGuesses(index - incr, index);
				else
					break;
			}
		}
	}
}


void
CWordList::reorder(void)
{
	if (count() > 1)
	{
		UniChar ch = gTryString[0];
		if (IsAlphabet(ch))
		{
			bubbleGuess('0', IsCircular, kBubbleUp);
			bubbleGuess('1', IsLinear, kBubbleUp);
			bubbleGuess('|', IsLinear, kBubbleUp);
		}
		else if (IsDigit(ch) || IsSlash(ch))
		{
			bubbleGuess('0', IsCircular, kBubbleDown);
			bubbleGuess('1', IsLinear, kBubbleDown);
		}
	}
}


void
CWordList::swapSingleCharacterGuesses(ArrayIndex index1, ArrayIndex index2)
{
	UniChar * c1 = scanTo(index1);
	UniChar * c2 = scanTo(index2);
	UniChar tmp = *c1;
	*c1 = *c2;
	*c2 = tmp;
}
