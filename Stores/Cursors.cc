/*
	File:		Cursors.cc

	Contains:	Cursor functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "Globals.h"
#include "Funcs.h"
#include "Stores.h"
#include "Soups.h"
#include "Cursors.h"
#include "Hints.h"
#include "RefMemory.h"
#include "FaultBlocks.h"
#include "CObjectBinaries.h"
#include "UStringUtils.h"
#include "ROMSymbols.h"

extern Ref EntrySoup(RefArg);

extern UniChar *	FindWord(UniChar * inStr, ArrayIndex inLen, UniChar * inWord, bool inWordStart);
extern UniChar *	FindString(UniChar * inStr, ArrayIndex inLen, UniChar * inStrToFind);	// Unicode.cc

void * gPermObjectTextCache = NULL;	// 0C105358

void	GCDeleteCursor(void * inData);
void	GCMarkCursor(void * inData);
void	GCUpdateCursor(void * inData);


char *
PtrToPtr(char * inPtr)
{
	Size ptrSize = GetPtrSize(inPtr);
	char * p = NewPtr(ptrSize);
	if (p == NULL)
		OutOfMemory();
	memcpy(p, inPtr, ptrSize);
	return p;
}


/*------------------------------------------------------------------------------
	C C u r s o r
	Allocated in the frames heap. It needs to be garbage collected.

0001FB5C EA689A05 .h.. | B        TCursor::Move(long)
0001FB60 EA689E17 .h.. | B        TCursor::GotoEntry(RefArg)
0001FB64 EA689E15 .h.. | B        TCursor::GotoKey(RefArg)
0001FB68 EA689E1E .h.. | B        TCursor::Clone()
0001FB6C EA689E08 .h.. | B        TCursor::CountEntries()
0001FB70 EA689E18 .h.. | B        TCursor::EntryRemoved(RefArg)
0001FB74 EA689E18 .h.. | B        TCursor::EntrySoupChanged(RefArg, RefArg)
0001FB78 EA689E1C .h.. | B        TCursor::GCMark()
0001FB7C EA689E1C .h.. | B        TCursor::GCUpdate()
0001FB80 EA68A223 .h.# | B        TCursor::~TCursor()
0001FB84 EA689E03 .h.. | B        TCursor::RebuildInfo(unsigned char, long)
0001FB88 EA6899E5 .h.. | B        TCursor::Invalidate()

------------------------------------------------------------------------------*/
inline void * operator new(size_t, void * p) { return p; }

Ref
CCursor::createNewCursor(void)
{
	RefVar	cursorObj(AllocateFramesCObject(sizeof(CCursor), GCDeleteCursor, GCMarkCursor, GCUpdateCursor));
	void *	cursorData = BinaryData(cursorObj);
	if (cursorData)
		new (cursorData) CCursor;

	RefVar	cursor(Clone(RA(cursorPrototype)));
	SetFrameSlot(cursor, SYMA(TCursor), cursorObj);
	return cursor;
}

CCursor *
CursorObj(RefArg inCursor)
{
	return (CCursor *)BinaryData(GetFrameSlot(inCursor, SYMA(TCursor)));
}


void
GCDeleteCursor(void * inData)
{
	CCursor *	cursor = reinterpret_cast<CCursor*>(inData);
	cursor->~CCursor();
}

void
GCMarkCursor(void * inData)
{
	CCursor *	cursor = reinterpret_cast<CCursor*>(inData);
	cursor->gcMark();
}

void
GCUpdateCursor(void * inData)
{
	CCursor *	cursor = reinterpret_cast<CCursor*>(inData);
	cursor->gcUpdate();
}

#pragma mark -

CursorSoupInfo::CursorSoupInfo()
{
	fSoup = NILREF;
	fTags = NILREF;
}

#pragma mark -

CCursor::CCursor()
{
	fUnionSoupIndex = NULL;
	fTagsIndexes = NULL;
	fSoupInfo = NULL;
	fNumOfSoupsInUnion = 0;
	fHints = NULL;
	fBeginKeyData = NULL;
	fEndKeyData = NULL;
	fIsIndexInvalid = false;
	fSoup = NILREF;
	fCursor = NILREF;
	fCurrentEntry = NILREF;
	fTagSpec = NILREF;
	fIndexPath = NILREF;
	fIndexType = NILREF;
	fQryWords = NILREF;
	fQryText = NILREF;
	fStartKey = NILREF;
	fBeginKey = NILREF;
	fEndKey = NILREF;
	fIndexValidTestFn = NILREF;
	fValidTestFn = NILREF;
	fEndTestFn = NILREF;
	fTestFnArgs = NILREF;
}


CCursor::~CCursor()
{
	invalidate();
	if (fHints)
		delete fHints;
	if (fBeginKeyData)
		FreePtr(fBeginKeyData);
	if (fEndKeyData)
		FreePtr(fEndKeyData);
}


void
CCursor::init(RefArg inCursor, RefArg inSoup, RefArg inQuerySpec)
{
	fQuerySpecBits = 0;
	fCurrentEntry = NILREF;
	fIsCursorAtEnd = false;
	fIsEntryDeleted = false;
	fSoup = inSoup;
	fCursor = inCursor;
	if (ISNIL(inQuerySpec))
	{
		fIndexPath = RSYM_uniqueId;
	}
	else
	{
		if (ISNIL(fIndexPath = EnsureInternal(GetFrameSlot(inQuerySpec, SYMA(indexPath)))))
			fIndexPath = RSYM_uniqueId;

		if (NOTNIL(fQryWords = GetFrameSlot(inQuerySpec, SYMA(words))))
		{
			fQuerySpecBits |= kQueryWords;
			if (NOTNIL(GetFrameSlot(inQuerySpec, SYMA(entireWords))))
				fQuerySpecBits |= kQueryEntireWords;
			if (!IsArray(fQryWords))
			{
				RefVar theWord(fQryWords);
				fQryWords = MakeArray(1);
				SetArraySlot(fQryWords, 1, theWord);
			}
			fHints = GetWordsHints(fQryWords);
		}

		if (NOTNIL(fQryText = GetFrameSlot(inQuerySpec, SYMA(text))))
			fQuerySpecBits |= kQueryText;

		if (NOTNIL(fTagSpec = cloneFrameSlot(inQuerySpec, SYMA(tagSpec))))
			fQuerySpecBits |= kQueryTags;

		fIsSecSortOrder = NOTNIL(GetFrameSlot(inQuerySpec, SYMA(secOrder)));

		if (NOTNIL(fBeginKey = cloneFrameSlot(inQuerySpec, SYMA(beginKey))))
			fQuerySpecBits |= kQueryBeginKey;
		else if (NOTNIL(fBeginKey = cloneFrameSlot(inQuerySpec, SYMA(beginExclKey))))
			fQuerySpecBits |= kQueryBeginExclKey;

		if (NOTNIL(fEndKey = cloneFrameSlot(inQuerySpec, SYMA(endKey))))
			fQuerySpecBits |= kQueryEndKey;
		else if (NOTNIL(fEndKey = cloneFrameSlot(inQuerySpec, SYMA(endExclKey))))
			fQuerySpecBits |= kQueryEndExclKey;

		fStartKey = cloneFrameSlot(inQuerySpec, SYMA(startKey));

		if (NOTNIL(fIndexValidTestFn = GetFrameSlot(inQuerySpec, SYMA(indexValidTest))))
			fQuerySpecBits |= kQueryIndexValidTest;

		if (NOTNIL(fValidTestFn = GetFrameSlot(inQuerySpec, SYMA(validTest))))
			fQuerySpecBits |= kQueryValidTest;

		if (NOTNIL(fEndTestFn = GetFrameSlot(inQuerySpec, SYMA(endTest))))
			fQuerySpecBits |= kQueryEndTest;

		if ((fQuerySpecBits & kQueryByTest) != 0)
			fTestFnArgs = MakeArray(1);
	}

	buildSoupsInfo();
	createIndexes();
	registerInSoup(inSoup);
}


void
CCursor::init(RefArg inCursor, const CCursor * inCursorData)
{
	newton_try
	{
		memmove((void*)this, (void*)inCursorData, offsetof(CCursor, fIndexKey));	// sic -- why not just copy it all?
		fIndexKey = inCursorData->fIndexKey;
		fIsCursorAtEnd = inCursorData->fIsCursorAtEnd;
		fIsEntryDeleted = inCursorData->fIsEntryDeleted;
		fCursor = inCursor;
		if (fHints)	// need to regenerate hints for this instance
			fHints = GetWordsHints(fQryWords);
		if (fBeginKeyData != NULL)
			fBeginKeyData = PtrToPtr(fBeginKeyData);
		if (fEndKeyData != NULL)
			fEndKeyData = PtrToPtr(fEndKeyData);
		buildSoupsInfo();
		createIndexes();
		if (fUnionSoupIndex)
			fUnionSoupIndex->setCurrentSoup(inCursorData->fUnionSoupIndex ? inCursorData->fUnionSoupIndex->i() : 0);
		registerInSoup(fSoup);
	}
	cleanup
	{
		invalidate();
	}
	end_try;
}


Ref
CCursor::clone(void)
{
	RefVar	cursor(createNewCursor());
	CursorObj(cursor)->init(cursor, this);
	return cursor;
}


void
CCursor::invalidate(void)
{
	if (fUnionSoupIndex)
	{
		delete fUnionSoupIndex;
		fUnionSoupIndex = NULL;
	}
	if (fTagsIndexes)
	{
		delete[] fTagsIndexes;
		fTagsIndexes = NULL;
	}
	if (fSoupInfo)
	{
		delete[] fSoupInfo;
		fSoupInfo = NULL;
	}
	fCurrentEntry = NILREF;
	fIsCursorAtEnd = false;
	fNumOfSoupsInUnion = 0;
}


Ref
CCursor::cloneFrameSlot(RefArg inFrame, RefArg inTag) const
{
	Ref theSlot = GetFrameSlot(inFrame, inTag);
	if (NOTNIL(theSlot))
		theSlot = Clone(theSlot);
	return theSlot;
}


void
CCursor::buildSoupsInfo(void)
{
	RefVar theList(GetFrameSlot(fSoup, SYMA(soupList)));
	if (NOTNIL(theList))
	{
		fNumOfSoupsInUnion = Length(theList);
		if (fNumOfSoupsInUnion == 0)
			return;
	}
	else
		fNumOfSoupsInUnion = 1;

	newton_try
	{
		fSoupInfo = new CursorSoupInfo[fNumOfSoupsInUnion];
		if (fSoupInfo == NULL)
			OutOfMemory();

		RefVar soupProto;	// sp04
		RefVar soupIndex;	// r6
		RefVar tagsIndex;	// sp00
		CursorSoupInfo * infoPtr = fSoupInfo;	// r5
		for (ArrayIndex i = 0; i < fNumOfSoupsInUnion; i++, infoPtr++)
		{
			infoPtr->fSoup = NOTNIL(theList) ? GetArraySlot(theList, i) : fSoup;
			soupProto = GetFrameSlot(infoPtr->fSoup, SYMA(_proto));
			if (ISNIL(soupProto))
				ThrowErr(exStore, kNSErrInvalidSoup);
			soupIndex = IndexPathToIndexDesc(soupProto, fIndexPath, NULL);
			if (ISNIL(soupIndex))
				ThrowErr(exStore, kNSErrNoSuchIndex);
			if (i == 0)
			{
				fIndexType = GetFrameSlot(soupIndex, SYMA(type));
				if (EQRef(fIndexType, RSYMtags))
					ThrowErr(exStore, kNSErrBadIndexDesc);
				if ((fQuerySpecBits & kQueryByKey) != 0)
				{
					//sp-54
					SKey keyData;
					short keySize;
					bool isMultiSlot = EQRef(GetFrameSlot(soupIndex, SYMA(structure)), RSYMmultislot);	// r10
					if ((fQuerySpecBits & kQueryByBeginKey) != 0
					&&  fBeginKeyData == NULL)
					{
						keySize = 0;
						KeyToSKey(fBeginKey, fIndexType, &keyData, &keySize, NULL);
						if (isMultiSlot && (fQuerySpecBits & kQueryBeginExclKey) != 0)
							keyData.setFlags(keyData.flags() | 0x80);
						fBeginKeyData = NewPtr(keyData.size());
						if (fBeginKeyData == NULL)
							OutOfMemory();
						memmove(fBeginKeyData, keyData.data(), keyData.size());
						fBeginKey = NILREF;
					}
					if ((fQuerySpecBits & kQueryByEndKey) != 0
					&&  fEndKeyData == NULL)
					{
						keySize = 0;
						KeyToSKey(fEndKey, fIndexType, &keyData, &keySize, NULL);
						if (isMultiSlot && (fQuerySpecBits & kQueryEndKey) != 0)	// sic
							keyData.setFlags(keyData.flags() | 0x80);
						fEndKeyData = NewPtr(keyData.size());
						if (fEndKeyData == NULL)
							OutOfMemory();
						memmove(fEndKeyData, keyData.data(), keyData.size());
						fEndKey = NILREF;
					}
				}
			}
			if (NOTNIL(fTagSpec))
			{
				tagsIndex = GetTagsIndexDesc(soupProto);
				if (ISNIL(tagsIndex))
					ThrowErr(exStore, kNSErrNoTags);
				infoPtr->fTags = EncodeQueryTags(tagsIndex, fTagSpec);
			}
		}
		
	}
	cleanup
	{
		invalidate();
	}
	end_try;
}


ArrayIndex
CCursor::getSoupInfoIndex(RefArg inSoup)
{
	for (ArrayIndex i = 0; i < fNumOfSoupsInUnion; ++i)
		if (EQRef(fSoupInfo[i].fSoup, inSoup))
			return i;
	return kIndexNotFound;
}


void
CCursor::createIndexes(void)
{
	if (fNumOfSoupsInUnion == 0)
		return;

	newton_try
	{
		UnionIndexData * indexData = new UnionIndexData[fNumOfSoupsInUnion];	// r10
		if (indexData == NULL)
			OutOfMemory();
		if (NOTNIL(fTagSpec))
		{
			fTagsIndexes = new CSoupIndex*[fNumOfSoupsInUnion];
			if (fTagsIndexes == NULL)
				OutOfMemory();
		}
		else
			fTagsIndexes = NULL;
		//sp-04
		RefVar soupProto;	// sp00
		RefVar soupIndex;	// r8
		CursorSoupInfo * infoPtr = fSoupInfo;	// r6
		for (ArrayIndex i = 0; i < fNumOfSoupsInUnion; i++, infoPtr++)
		{
			//sp-08
			soupProto = GetFrameSlot(infoPtr->fSoup, SYMA(_proto));
			soupIndex = IndexPathToIndexDesc(soupProto, fIndexPath, NULL);
			indexData[i].index = GetSoupIndexObject(infoPtr->fSoup, RINT(GetFrameSlot(soupIndex, SYMA(index))));
			if (fTagsIndexes)
			{
				//sp-04
				soupIndex = GetTagsIndexDesc(soupProto);	// now the tags index
				fTagsIndexes[i] = GetSoupIndexObject(infoPtr->fSoup, RINT(GetFrameSlot(soupIndex, SYMA(index))));
			}
		}
		fUnionSoupIndex = new CUnionSoupIndex(fNumOfSoupsInUnion, indexData);
		if (fUnionSoupIndex == NULL)
			OutOfMemory();
	}
	cleanup
	{
		invalidate();
	}
	end_try;
}


void
CCursor::park(bool inAtEnd)
{
	fCurrentEntry = NILREF;
	fIsCursorAtEnd = inAtEnd;
	if (fUnionSoupIndex)
		fUnionSoupIndex->setCurrentSoup(0);
}


Ref
CCursor::isParked(void)
{
	if (ISNIL(fCurrentEntry))
		return fIsCursorAtEnd ? RSYMend : RSYMbegin;
	return NILREF;
}

int
CCursor::exitParking(bool inForward)
{
	int result;

	if (fIsCursorAtEnd == inForward)
		return 3;	// already at end -- can’t go any further

	if (inForward)
	{
		if ((fQuerySpecBits & kQueryByBeginKey) != 0)
		{
			result = fUnionSoupIndex->find((SKey*)fBeginKeyData, &fIndexKey, (SKey*)&fEntryId, fIsSecSortOrder);
			if (result == 2)
				return 0;
			if (result == 0)
			{
				if ((fQuerySpecBits & kQueryBeginExclKey) != 0)
					return fUnionSoupIndex->next(&fIndexKey, (SKey*)&fEntryId, 1, &fIndexKey, (SKey*)&fEntryId);
			}
			return result;
		}
		return fUnionSoupIndex->first(&fIndexKey, (SKey*)&fEntryId);
	}
	else
	{
		if ((fQuerySpecBits & kQueryByEndKey) != 0)
		{
			result = fUnionSoupIndex->findPrior((SKey*)fEndTestFn, &fIndexKey, (SKey*)&fEntryId, fIsSecSortOrder, (fQuerySpecBits & kQueryEndExclKey) != 0);
			if (result == 2)
				return 0;
			return result;
		}
		return fUnionSoupIndex->last(&fIndexKey, (SKey*)&fEntryId);
	}
}


bool
CCursor::keyBoundsValidTest(const SKey & inKey, bool inIsEndBound)
{
	int result;

	if (inIsEndBound)
	{
		if ((fQuerySpecBits & kQueryByEndKey) != 0)
		{
			result = fUnionSoupIndex->index()->compareKeys(inKey, *(SKey*)fEndKeyData);	// CSoupIndex
			if (result > 0)
				return false;
			else if (result == 0)
				return (fQuerySpecBits & kQueryEndExclKey) == 0;
		}
	}
	else
	{
		if ((fQuerySpecBits & kQueryByBeginKey) != 0)
		{
			result = fUnionSoupIndex->index()->compareKeys(inKey, *(SKey*)fBeginKeyData);
			if (result < 0)
				return false;
			else if (result == 0)
				return (fQuerySpecBits & kQueryBeginExclKey) == 0;
		}
	}

	return true;
}


/*------------------------------------------------------------------------------
	Perform words-valid-test; determine whether all of the words in a given array
	exist within a string.
	Args:		inStr			the string to test
				inLen			length of that string
				inParms		array of words we must find in that string
	Return:	YES => all words have been found in the string
------------------------------------------------------------------------------*/

struct SWordsTestParms
{
	SWordsTestParms();

	RefVar	fWords;
	bool		fIsWholeWords;
};
inline SWordsTestParms::SWordsTestParms() : fIsWholeWords(false) { }


bool
WordsValidTestTextProc(UniChar * inStr, ArrayIndex inLen, void * inParms)
{
	RefVar testWord;
	int i = Length(((SWordsTestParms*)inParms)->fWords) - 1;
	for ( ; i >= 0; i--)
	{
		testWord = GetArraySlot(((SWordsTestParms*)inParms)->fWords, i);
		UniChar * wordStr = GetUString(testWord);
		UniChar * theStr = FindWord(inStr, inLen, wordStr, true);
		if (theStr == NULL)
			break;	// we didn’t find the word
		if (((SWordsTestParms*)inParms)->fIsWholeWords)
		{
			// we want to find whole words, so next character must be a delimiter
			while (theStr != NULL)
			{
				theStr += Ustrlen(wordStr);
				if (IsDelimiter(*theStr))
					break;	// yup, found a whole word
				// there’s more to this word -- look again starting from this point
				theStr = FindWord(theStr, inLen - (theStr - inStr), wordStr, false);
			}
		}
	}
	return i < 0;	// ie we found them all
}


bool
CCursor::wordsValidTest(PSSId inId)
{
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(fSoupInfo[fUnionSoupIndex->i()].fSoup, SYMA(TStore));
	if (fHints && TestObjHints(fHints, Length(fQryWords), storeWrapper, inId) == 0)
		return false;

	SWordsTestParms parms;
	parms.fWords = fQryWords;
	parms.fIsWholeWords = fQuerySpecBits & kQueryEntireWords;
	return WithPermObjectTextDo(storeWrapper, inId, WordsValidTestTextProc, &parms, &gPermObjectTextCache);
}


bool
TextValidTestTextProc(UniChar * inStr, ArrayIndex inLen, void * inParms)
{
	return (FindString(inStr, inLen, (UniChar *)inParms) != NULL);
}

bool
CCursor::textValidTest(PSSId inId)
{
	CStoreWrapper * storeWrapper = (CStoreWrapper *)GetFrameSlot(fSoupInfo[fUnionSoupIndex->i()].fSoup, SYMA(TStore));
	CDataPtr qryText(fQryText);
	return WithPermObjectTextDo(storeWrapper, inId, TextValidTestTextProc, (Ptr)qryText, &gPermObjectTextCache);
}

bool
CCursor::validTest(const SKey & inKey, PSSId inId, bool inForward, bool * outIsEntrySet, bool * outIsDone)
{
	*outIsEntrySet = false;
	*outIsDone = false;
	if ((fQuerySpecBits & kQueryByAnything) != 0)
	{
		if ((fQuerySpecBits & kQueryByKey) != 0
		&&   !keyBoundsValidTest(inKey, inForward))
		{
			*outIsDone = true;
			return false;
		}

		if (fTagsIndexes != NULL
		&&  !TagsValidTest(*fTagsIndexes[fUnionSoupIndex->i()], fSoupInfo[fUnionSoupIndex->i()].fTags, inId))
			return false;

		if ((fQuerySpecBits & kQueryWords) != 0
		&&  !wordsValidTest(inId))
			return false;

		if ((fQuerySpecBits & kQueryText) != 0
		&&  !textValidTest(inId))
			return false;

		if ((fQuerySpecBits & kQueryByTest) != 0)
		{
			if ((fQuerySpecBits & kQueryIndexValidTest) != 0)
			{
				SetArraySlot(fTestFnArgs, 0, SKeyToKey(inKey, fIndexType, NULL));
				if (ISNIL(DoBlock(fIndexValidTestFn, fTestFnArgs)))
					return false;
			}
			else
			{
				makeEntryFaultBlock(inId);
				*outIsEntrySet = true;
				SetArraySlot(fTestFnArgs, 0, fCurrentEntry);
				if ((fQuerySpecBits & kQueryEndTest) != 0)
				{
					if (NOTNIL(DoBlock(fEndTestFn, fTestFnArgs)))
					{
						*outIsDone = true;
						return false;
					}
				}
				if ((fQuerySpecBits & kQueryValidTest) != 0)
				{
					if (NOTNIL(DoBlock(fValidTestFn, fTestFnArgs)))
						return false;
				}
			}
		}
	}
	return true;
}


void
CCursor::getState(CursorState * outState)
{
	outState->seqInUnion = fUnionSoupIndex->i();
	outState->entry = fCurrentEntry;
	if (NOTNIL(outState->entry))
	{
		memmove(&outState->entryKey, &fIndexKey, fUnionSoupIndex->index()->kfSizeOfKey(&fIndexKey));
		outState->entryId = fEntryId;
	}
	outState->isCursorAtEnd = fIsCursorAtEnd;
	outState->isEntryDeleted = fIsEntryDeleted;
}

void
CCursor::setState(CursorState & inState)
{
	fUnionSoupIndex->setCurrentSoup(inState.seqInUnion);
	fCurrentEntry = inState.entry;
	if (NOTNIL(inState.entry))
	{
		memmove(&fIndexKey, &inState.entryKey, fUnionSoupIndex->index()->kfSizeOfKey(&inState.entryKey));
	}
	fIsCursorAtEnd = inState.isCursorAtEnd;
	fIsEntryDeleted = inState.isEntryDeleted;
}


void
CCursor::makeEntryFaultBlock(PSSId inId)
{
	RefVar	theSoup(fSoupInfo[fUnionSoupIndex->i()].fSoup);
	fCurrentEntry = GetEntry(theSoup, inId);
}


struct SCountParms
{
	CCursor *	cursor;
	size_t		count;
};

int
CountEntriesStopFn(SKey * inKey, SKey * inData, void * ioParms)
{
	SCountParms * parms = (SCountParms *)ioParms;
	bool isEntrySet, isDone;
	if (parms->cursor->validTest(*inKey, (PSSId)(int)*inData, true, &isEntrySet, &isDone))
		parms->count++;
	return isDone;
}


ArrayIndex
CCursor::countEntries(void)
{
	ArrayIndex numOfEntries = 0;
	if (fSoupInfo)
	{
		//sp-60
		CursorState saveState;
		getState(&saveState);
		//sp-08
		newton_try
		{
			if (reset() != 2)
			{
				SCountParms parms;
				parms.cursor = this;
				parms.count = 1;
				fUnionSoupIndex->search(true, &fIndexKey, (SKey*)&fEntryId, CountEntriesStopFn, &parms, NULL, NULL, 0);
				numOfEntries = parms.count;
				if (gPermObjectTextCache)
				{
					ReleasePermObjectTextCache(gPermObjectTextCache);
					gPermObjectTextCache = NULL;
				}
			}
			setState(saveState);
		}
		cleanup
		{
			setState(saveState);
		}
		end_try;
	}
	return numOfEntries;
}


void
CCursor::rebuildInfo(bool inArg1, long inArg2)
{
	int r6;

	if (!inArg1)
	{
		if (fSoupInfo)
		{
			delete[] fSoupInfo;
			fSoupInfo = NULL;
		}
		buildSoupsInfo();
	}

	if (fUnionSoupIndex)
	{
		r6 = fUnionSoupIndex->i();
		delete fUnionSoupIndex;
		fUnionSoupIndex = NULL;
	}
	else
		r6 = 0;

	bool r8 = false;
	if (inArg2 >= 0)
	{
		if (ISNIL(fCurrentEntry))
			r6 = 0;
		else if (r6 == inArg2)
			r8 = true;
		else if (r6 > inArg2)
			r6--;
	}

	if (fTagsIndexes)
	{
		delete[] fTagsIndexes;
		fTagsIndexes = NULL;
	}
	createIndexes();
	if (fUnionSoupIndex)
		fUnionSoupIndex->setCurrentSoup(r6);
	if (r8)
	{
		if (fUnionSoupIndex->currentSoupGone(&fIndexKey, &fIndexKey, (SKey*)&fEntryId))
			park(false);
		else
			move(0);
	}
}


void
CCursor::soupRemoved(RefArg inSoup)
{
	ArrayIndex index;
	if (EQRef(inSoup, fSoup))
		invalidate();
	else if ((index = getSoupInfoIndex(inSoup)) != kIndexNotFound)
	{
		if (fNumOfSoupsInUnion == 1)
			invalidate();
		else
			rebuildInfo(false, index);
	}
	unregisterFromSoup(inSoup);
}


void
CCursor::soupAdded(RefArg inSoup)
{
	if (!fIsIndexInvalid)
	{
		RefVar soupProto(GetFrameSlot(inSoup, SYMA(_proto)));
		if (ISNIL(IndexPathToIndexDesc(soupProto, fIndexPath, NULL))
		|| (fTagsIndexes && ISNIL(GetTagsIndexDesc(soupProto))))
			invalidate();
		else
		{
			rebuildInfo(false, -1);
			registerInSoup(inSoup);
		}
	}
}


Ref
CCursor::status(void)
{
	return fIsIndexInvalid ? RSYMmissingIndex : RSYMvalid;
}


void
CCursor::setSoup(RefArg inSoup)
{
	unregisterFromSoup(fSoup);
	fSoup = inSoup;
	rebuildInfo(false, -1);
	DeleteEntryFromCache(GetFrameSlot(inSoup, SYMA(cursors)), fCursor);
	registerInSoup(inSoup);
	reset();
}


void
CCursor::indexRemoved(RefArg inArg1, RefArg inQuerySpec)
{
	if ((EQRef(GetFrameSlot(inQuerySpec, SYMA(type)), RSYMtags) && fTagsIndexes)
	||  IndexPathsEqual(fIndexPath, GetFrameSlot(inQuerySpec, SYMA(path))))
	{
		invalidate();
		fIsIndexInvalid = true;
	}
}


void
CCursor::indexObjectsChanged(void)
{
	if (fSoupInfo)
		rebuildInfo(true, -1);
}


void
CCursor::soupTagsChanged(RefArg inSoup)
{
	ArrayIndex index;
	if (NOTNIL(fTagSpec)
	&& (index = getSoupInfoIndex(inSoup)) != kIndexNotFound)
	{
		RefVar soupProto(GetFrameSlot(inSoup, SYMA(_proto)));
		fSoupInfo[index].fTags = EncodeQueryTags(GetTagsIndexDesc(soupProto), fTagSpec);
	}
}


bool
CCursor::pinCurrentKey(void)
{
	if (keyBoundsValidTest(fIndexKey, false) == 0)
	{
		reset();
		return true;
	}
	if (keyBoundsValidTest(fIndexKey, true) == 0)
	{
		resetToEnd();
		return true;
	}
	return false;
}


Ref
CCursor::reset(void)
{
	if (NOTNIL(fStartKey))
		return gotoKey(fStartKey);

	park(false);
	return move(1);
}


Ref
CCursor::resetToEnd(void)
{
	park(true);
	return move(-1);
}


struct SCursorParms
{
	CCursor *	cursor;
	bool		isForward;
	bool		isEntrySet;
	int		offset;
	int		decr;
};

int
CursorStopFn(SKey * inKey, SKey * inData, void * ioParms)
{
	SCursorParms * parms = (SCursorParms *)ioParms;
	bool isDone;
	if (parms->cursor->validTest(*inKey, (PSSId)(int)*inData, parms->isForward, &parms->isEntrySet, &isDone))
		return (parms->offset += parms->decr) == 0;
	return isDone;
}


Ref
CCursor::move(int inOffset)
{
	if (fSoupInfo)
	{
		int entryStatus;
		bool isForward = (inOffset >= 0);			// r7
		bool wasNoEntry = ISNIL(fCurrentEntry);	// r1
		bool wasDeleted = fIsEntryDeleted;			// r0
		if (inOffset != 0)
			fIsEntryDeleted = false;
		if (wasNoEntry)
			entryStatus = exitParking(isForward);
		else
		{
			if (inOffset > 0 && wasDeleted)
				inOffset--;	// so offset = 0, ie re-read entry
			if (inOffset != 0)
			{
				if (isForward)
					entryStatus = fUnionSoupIndex->next(&fIndexKey, (SKey*)&fEntryId, 0, &fIndexKey, (SKey*)&fEntryId);
				else
					entryStatus = fUnionSoupIndex->prior(&fIndexKey, (SKey*)&fEntryId, 0, &fIndexKey, (SKey*)&fEntryId);
			}
			else
				entryStatus = 0;
		}

		if (entryStatus == 0)
		{
			bool entryIsFound = false;
			SCursorParms parms;
			parms.decr = isForward ? -1 : 1;
			if (inOffset == 0)
				inOffset = -parms.decr;
			//sp-04
			bool isQueryDone;
			if (validTest(fIndexKey, fEntryId, isForward, &parms.isEntrySet, &isQueryDone)
			&&  (inOffset += parms.decr) == 0)
				entryIsFound = true;
			else if (!isQueryDone)
			{
				parms.cursor = this;
				parms.isForward = isForward;
				parms.offset = inOffset;
				entryIsFound = fUnionSoupIndex->search(isForward, &fIndexKey, (SKey*)&fEntryId, CursorStopFn, &parms, &fIndexKey, (SKey*)&fEntryId, 0) == 0
								&& parms.offset == 0;
			}

			if (entryIsFound)
			{
				if (!parms.isEntrySet)
					makeEntryFaultBlock(fEntryId);
			}
			else
				park(isForward);
		}
		else
			park(isForward);

		if (gPermObjectTextCache != NULL)
		{
			ReleasePermObjectTextCache(gPermObjectTextCache);
			gPermObjectTextCache = NULL;
		}

		return entry();
	}
	return NILREF;
}


Ref
CCursor::gotoKey(RefArg inKey)
{
	if (fSoupInfo)
	{
		fIsEntryDeleted = false;
		KeyToSKey(inKey, fIndexType, &fIndexKey, NULL, NULL);
		int result = fUnionSoupIndex->find(&fIndexKey, &fIndexKey, (SKey*)&fEntryId, fIsSecSortOrder);
		if (result != 0 && result != 2)
		{
			park(true);
			return NILREF;
		}
		if (!pinCurrentKey())
		{
			fCurrentEntry = TRUEREF;
			move(0);
		}
		return fCurrentEntry;
	}
	return NILREF;
}


Ref
CCursor::gotoEntry(RefArg inEntry)
{
	if (!IsFaultBlock(inEntry))
		ThrowErr(exStore, kNSErrNotASoupEntry);
	if (fSoupInfo == NULL)
		return NILREF;

	RefVar theKey(GetEntryKey(inEntry, fIndexPath));
	if (ISNIL(theKey))
		return NILREF;

	fIsEntryDeleted = false;

	bool r8 = false;
	ArrayIndex soupIndex;	// r6
	if ((soupIndex = getSoupInfoIndex(EntrySoup(inEntry))) != kIndexNotFound)
	{
		//sp-04
		KeyToSKey(theKey, fIndexType, &fIndexKey, NULL, NULL);
		fEntryId = RINT(((FaultObject *)NoFaultObjectPtr(inEntry))->id);
		int entryStatus = fUnionSoupIndex->index(soupIndex)->next(&fIndexKey, (SKey*)&fEntryId, 0, NULL, NULL);
		if (entryStatus == 0 || entryStatus == 3)
		{
			fCurrentEntry = inEntry;
			fUnionSoupIndex->setCurrentSoup(soupIndex);
			if (pinCurrentKey())
				return NILREF;
			move(0);
			r8 = true;
		}
	}
	if (!r8)
		gotoKey(theKey);
	return MAKEBOOLEAN(EQRef(inEntry, fCurrentEntry));
}


Ref
CCursor::entry(void)
{
	return fIsEntryDeleted ? RSYMdeleted : fCurrentEntry;
}


Ref
CCursor::entryKey(void)
{
	if (NOTNIL(fCurrentEntry)
	&&  !fIsEntryDeleted)
	{
		return SKeyToKey(fIndexKey, fIndexType, NULL);
	}
	return NILREF;
}


void
CCursor::entryChanged(RefArg inEntry, bool inArg2, bool inArg3)
{
	if (EQRef(fCurrentEntry, inEntry))
	{
		if (inArg2)
			gotoEntry(inEntry);
		else if (inArg3 && fTagsIndexes)
			move(0);
	}
}


void
CCursor::entryReadded(RefArg inEntry, RefArg inNewEntry)
{
	if (EQRef(fCurrentEntry, inEntry))
		fCurrentEntry = inNewEntry;
}


void
CCursor::entryRemoved(RefArg inEntry)
{
	if (EQRef(fCurrentEntry, inEntry))
	{
		fIsEntryDeleted = false;
		move(1);
		fIsEntryDeleted = NOTNIL(fCurrentEntry);
	}
}


void
CCursor::entrySoupChanged(RefArg inEntry, RefArg inNewEntry)
{
	if (EQRef(fCurrentEntry, inEntry))
		gotoEntry(inNewEntry);
}


void
CCursor::registerInSoup(RefArg inSoup) const
{
	PutEntryIntoCache(GetFrameSlot(inSoup, SYMA(cursors)), fCursor);

	RefVar soupList(GetFrameSlot(inSoup, SYMA(soupList)));
	if (NOTNIL(soupList))
	{
		for (int i = Length(soupList) - 1; i >= 0; i--)
			PutEntryIntoCache(GetFrameSlot(GetArraySlot(soupList, i), SYMA(cursors)), fCursor);
	}
}


void
CCursor::unregisterFromSoup(RefArg inSoup) const
{
	RefVar soupList(GetFrameSlot(inSoup, SYMA(soupList)));
	if (NOTNIL(soupList))
	{
		for (int i = Length(soupList) - 1; i >= 0; i--)
			DeleteEntryFromCache(GetFrameSlot(GetArraySlot(soupList, i), SYMA(cursors)), fCursor);
	}
//	else		can’t be right, but that’s what the original does
	DeleteEntryFromCache(GetFrameSlot(inSoup, SYMA(cursors)), fCursor);
}


void
CCursor::gcMark(void)
{
	DIYGCMark(fSoup);
	DIYGCMark(fCursor);
	DIYGCMark(fCurrentEntry);
	DIYGCMark(fTagSpec);
	DIYGCMark(fIndexPath);
	DIYGCMark(fIndexType);
	DIYGCMark(fQryWords);
	DIYGCMark(fQryText);
	DIYGCMark(fStartKey);
	if ((fQuerySpecBits & kQueryByKey) != 0)
	{
		DIYGCMark(fBeginKey);
		DIYGCMark(fEndKey);
	}
	if ((fQuerySpecBits & kQueryByTest) != 0)
	{
		DIYGCMark(fIndexValidTestFn);
		DIYGCMark(fValidTestFn);
		DIYGCMark(fEndTestFn);
		DIYGCMark(fTestFnArgs);
	}
	CursorSoupInfo * info = fSoupInfo;
	if (info)
	{
		CursorSoupInfo * limit = info + fNumOfSoupsInUnion;
		for ( ; info < limit; info++)
		{
			DIYGCMark(info->fSoup);
			DIYGCMark(info->fTags);
		}
	}
}


void
CCursor::gcUpdate(void)
{
	fSoup = DIYGCUpdate(fSoup);
	fCursor = DIYGCUpdate(fCursor);
	fCurrentEntry = DIYGCUpdate(fCurrentEntry);
	fTagSpec = DIYGCUpdate(fTagSpec);
	fIndexPath = DIYGCUpdate(fIndexPath);
	fIndexType = DIYGCUpdate(fIndexType);
	fQryWords = DIYGCUpdate(fQryWords);
	fQryText = DIYGCUpdate(fQryText);
	fStartKey = DIYGCUpdate(fStartKey);
	if ((fQuerySpecBits & kQueryByKey) != 0)
	{
		fBeginKey = DIYGCUpdate(fBeginKey);
		fEndKey = DIYGCUpdate(fEndKey);
	}
	if ((fQuerySpecBits & kQueryByTest) != 0)
	{
		fIndexValidTestFn = DIYGCUpdate(fIndexValidTestFn);
		fValidTestFn = DIYGCUpdate(fValidTestFn);
		fEndTestFn = DIYGCUpdate(fEndTestFn);
		fTestFnArgs = DIYGCUpdate(fTestFnArgs);
	}
	CursorSoupInfo * info = fSoupInfo;
	if (info)
	{
		CursorSoupInfo * limit = info + fNumOfSoupsInUnion;
		for ( ; info < limit; info++)
		{
			info->fSoup = DIYGCUpdate(info->fSoup);
			info->fTags = DIYGCUpdate(info->fTags);
		}
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	Create cursor for query.
------------------------------------------------------------------------------*/

void
DefineCursor(RefArg inSoup, RefArg inQuerySpec, RefArg inCursor)
{
	RefVar	theSoup(inSoup);
	bool	isBadSoup = NOTNIL(GetFrameSlot(theSoup, SYMA(errorCode)));
	if (isBadSoup)
	{
		RefVar	soupList(GetFrameSlot(theSoup, SYMA(soupList)));
		theSoup = GetArraySlot(soupList, Length(soupList) - 1);
	}
	CursorObj(inCursor)->init(inCursor, theSoup, inQuerySpec);
	if (isBadSoup)
	{
		PutEntryIntoCache(GetFrameSlot(theSoup, SYMA(cursors)), inCursor);
	}
}


/*------------------------------------------------------------------------------
	The soup:Query(spec) handler.
------------------------------------------------------------------------------*/

Ref
CommonSoupQuery(RefArg inRcvr, RefArg inQuerySpec)
{
	RefVar	cursor(CCursor::createNewCursor());
	DefineCursor(inRcvr, inQuerySpec, cursor);
	CursorObj(cursor)->reset();
	return cursor;
}


/*------------------------------------------------------------------------------
	Collect... what, exactly?
------------------------------------------------------------------------------*/

Ref
SoupCollect(RefArg inRcvr, RefArg inQuerySpec)
{
#if 0
	RefVar	cursor(CCollectCursor::CreateNewCollectCursor());
	DefineCursor(inRcvr, inQuerySpec, cursor);
	CCursor * curs = CursorObj(cursor);
	newton_try
	{
		curs->collect();
	}
	newton_catch(exOutOfMemory)
	{
		cursor = CommonSoupQuery(inRcvr, inQuerySpec);
	}
	end_try;
	return cursor;
#else
	return NILREF;
#endif
}


/*------------------------------------------------------------------------------
	Perform a tag query.
------------------------------------------------------------------------------*/

Ref
QueryEntriesWithTags(RefArg inRcvr, RefArg inTags)
{
	RefArg	tagSpec(AllocateFrame());
	SetFrameSlot(tagSpec, SYMA(any), inTags);

	RefArg	querySpec(AllocateFrame());
	SetFrameSlot(querySpec, SYMA(type), SYMA(index));
	SetFrameSlot(querySpec, SYMA(tagSpec), tagSpec);

	return SoupQuery(inRcvr, querySpec);
}

#pragma mark -

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref	CursorMove(RefArg inRcvr, RefArg inOffset) { return CursorObj(inRcvr)->move(RINT(inOffset)); }
Ref	CursorNext(RefArg inRcvr) { return CursorObj(inRcvr)->move(1); }
Ref	CursorPrev(RefArg inRcvr) { return CursorObj(inRcvr)->move(-1); }
Ref	CursorGoto(RefArg inRcvr, RefArg inEntry) { return CursorObj(inRcvr)->gotoEntry(inEntry); }
Ref	CursorGotoKey(RefArg inRcvr, RefArg inKey) { return CursorObj(inRcvr)->gotoKey(inKey); }
Ref	CursorEntry(RefArg inRcvr) { return CursorObj(inRcvr)->entry(); }
Ref	CursorEntryKey(RefArg inRcvr) { return CursorObj(inRcvr)->entryKey(); }
Ref	CursorReset(RefArg inRcvr) { return CursorObj(inRcvr)->reset(); }
Ref	CursorResetToEnd(RefArg inRcvr) { return CursorObj(inRcvr)->resetToEnd(); }
Ref	CursorWhichEnd(RefArg inRcvr) { return CursorObj(inRcvr)->isParked(); }
Ref	CursorClone(RefArg inRcvr) { return CursorObj(inRcvr)->clone(); }
Ref	CursorCountEntries(RefArg inRcvr) { return MAKEINT(CursorObj(inRcvr)->countEntries()); }
Ref	CursorSoup(RefArg inRcvr) { return CursorObj(inRcvr)->soup(); }
Ref	CursorIndexPath(RefArg inRcvr) { return CursorObj(inRcvr)->indexPath(); }
Ref	CursorStatus(RefArg inRcvr) { return CursorObj(inRcvr)->status(); }

#pragma mark -

/*----------------------------------------------------------------------
	Update a soup’s cursors after some change to the soup.
----------------------------------------------------------------------*/

void
EachSoupCursorDo(RefArg inSoup, ESoupCursorOp inOp)
{
	EachSoupCursorDo(inSoup, inOp, RA(NILREF), RA(NILREF));
}


void
EachSoupCursorDo(RefArg inSoup, ESoupCursorOp inOp, RefArg inArg)
{
	EachSoupCursorDo(inSoup, inOp, inArg, RA(NILREF));
}


void
EachSoupCursorDo(RefArg inSoup, ESoupCursorOp inOp, RefArg inArg1, RefArg inArg2)
{
	RefVar cursors(GetFrameSlot(inSoup, SYMA(cursors)));

	for (int i = Length(cursors) - 1; i >= 0; i--)
	{
		Ref cursor = GetArraySlot(cursors, i);
		if (NOTNIL(cursor))
		{
			CCursor * co = CursorObj(cursor);
			switch (inOp)
			{
			case 0:
				co->soupAdded(inArg1);
				break;
			case 1:
				co->soupRemoved(inArg1);
				break;
			case 2:
				co->entryRemoved(inArg1);
				break;
			case 3:
				co->setSoup(inArg1);
				break;
			case 4:
				co->soupTagsChanged(inSoup);
				break;
			case 5:
				co->indexObjectsChanged();
				break;
			case 6:
				co->entrySoupChanged(inArg1, inArg2);
				break;
			case 7:
				co->entryReadded(inArg1, inArg2);
				break;
			case 8:
				co->indexRemoved(inSoup, inArg1);
				break;
			}
		}
	}
}
