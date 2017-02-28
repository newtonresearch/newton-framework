/*
	File:		RichStrings.cc

	Contains:	Rich string functions.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "Unicode.h"
#include "UStringUtils.h"
#include "RichStrings.h"
#include "ROMResources.h"


Ref
MakeRichString(RefArg inText, RefArg inStyles, bool inArg3)
{ return NILREF; /*INCOMPLETE*/}


#pragma mark Constructors
/* ---------------------------------------------------------------------
	Constructors
--------------------------------------------------------------------- */

CRichString::CRichString()
{
	setNoStringData();
}


CRichString::CRichString(RefArg inObj)
{
	fUString = NULL;

	if (IsString(inObj))
		setStringData(inObj);
	else
		ThrowBadTypeWithFrameData(kNSErrNotAString, inObj);
}


CRichString::CRichString(UniChar * inStr)
{
	setUStringData(inStr);
}


CRichString::CRichString(UniChar * inStr, ArrayIndex inSize)
{
	setUStringData(inStr, inSize);
}


CRichString::~CRichString()
{ }


#pragma mark Lockers
/* ---------------------------------------------------------------------
	Lockers
--------------------------------------------------------------------- */

UniChar *
CRichString::grabPtr(void) const
{
	if (NOTNIL(fRefString))
	{
		LockRef(fRefString);
		return (UniChar*) BinaryData(fRefString);
	}
	return fUString;
}


void
CRichString::releasePtr(void) const
{
	if (NOTNIL(fRefString))
		UnlockRef(fRefString);
}


#pragma mark Setters
/* ---------------------------------------------------------------------
	Setters
--------------------------------------------------------------------- */

void
CRichString::setNoStringData(void)
{
	fRefString = NILREF;
	fUString = NULL;
	fIsInky = false;

	UniChar ch = kEndOfString;
	setFormatAndLength(&ch, sizeof(UniChar));	// original says 0 length !!!!
}


void
CRichString::setStringData(RefArg inStrRef)
{
	fRefString = inStrRef;
	fUString = NULL;
	fIsInky = false;

	setFormatAndLength(grabPtr(), Length(fRefString));	// Length() includes nul terminator
	releasePtr();
}


void
CRichString::setUStringData(UniChar * inStr, ArrayIndex inSize)
{
	fRefString = NILREF;
	fUString = inStr;
	fIsInky = false;

	setFormatAndLength(inStr, inSize);
}


void
CRichString::setUStringData(UniChar * inStr)
{
	setUStringData(inStr, (Ustrlen(inStr) + 1) * sizeof(UniChar));
}


void
CRichString::setChar(ArrayIndex index, UniChar inCh)
{
	UniChar * uchr = grabPtr() + index;

	if (*uchr == kInkChar)
	{
		// replacing an ink char is non-trivial
		UniChar str[2] = { inCh, kEndOfString };
		CRichString richStr(str);
		mungeRange(index, 1, &richStr, 0, 1);
	}
	else
		*uchr = inCh;

	releasePtr();
}


void
CRichString::setObjectSize(ArrayIndex inSize)
{
	SetLength(fRefString, inSize);
}


void
CRichString::setFormatAndLength(UniChar * inStr, ArrayIndex inSize)
{
	// inSize is size in bytes, includes nul terminator, any ink and format word
	// examine terminator unichar -- for an inked string it may not be 0!
	int index = inSize / sizeof(UniChar) - 1;
	UniChar fmt = inStr[index] & 0x03;
	if (fmt == 0x01)
	{
		// it’s an inked string -- final ULong is length-of-text + flags
		ULong flags = (inStr[index - 1] << 16) + fmt;
		// repurpose index to point to end of unicode
		index = flags >> 4;
	}
	fFormat = fmt;
	fSize = inSize;
	fNumOfChars = index;

	if (fmt == 0x00)
	{
		// it’s a plain unicode string
		fOffsetToInk = 0;
		x14 = inSize;
		fLengthOfInk = 0;
	}
	else
	{
		// it’s an inked string
		fOffsetToInk = LONGALIGN((index + 1) * sizeof(UniChar));
		x14 = fOffsetToInk;
		fLengthOfInk = inSize - fOffsetToInk - sizeof(ULong)/*format*/;
	}

	x20 = fLengthOfInk;
}


#pragma mark Getters
/* ---------------------------------------------------------------------
	Getters
--------------------------------------------------------------------- */

/* ---------------------------------------------------------------------
	Get a character from the string.
	Args:		index				offset to character
	Return:	UniChar character
--------------------------------------------------------------------- */

UniChar
CRichString::getChar(ArrayIndex index) const
{
	const UniChar * str = grabPtr();
	UniChar unic = str[index];
	releasePtr();

	return unic;
}


/* ---------------------------------------------------------------------
	Get ink data within range.
	Args:		inStart			offset to start of range
				inCount			length of range
				outInkStart		offset to start of ink data
				outInkLength	length of ink data
									0 => no ink found
	Return:	--
--------------------------------------------------------------------- */

void
CRichString::getInkData(ArrayIndex inStart, ArrayIndex inCount, ArrayIndex * outInkStart, ArrayIndex * outInkLength) const
{
	const UniChar * str = grabPtr();
	InkInfo * inkInfo = (InkInfo *)((Ptr) str + fOffsetToInk);	// point to first ink word info
	ArrayIndex inkInfoOffset = 0;
	ArrayIndex inkDataStart = 0;
	bool isInkDataFound = false;

	// search for ink from the start of the string
	for (ArrayIndex i = 0; i < inStart + inCount; ++i)
	{
		if (*str++ == kInkChar)
		{
			// we encountered an ink char -- calculate its info length
			ULong	inkLength = LONGALIGN(sizeof(inkInfo->length) + inkInfo->length);
			if (i >= inStart)
			{
				// this is the range we’re interested in
				if (!isInkDataFound)
				{
					isInkDataFound = true;
					inkDataStart = inkInfoOffset;
					inkInfoOffset = 0;	// reset ink offset -- now we’re measuring its length
				}
			}
			else
				// not yet; skip this ink info
				inkDataStart = inkInfoOffset + inkLength;
			inkInfoOffset += inkLength;
			inkInfo = (InkInfo *)((Ptr) inkInfo + inkLength);		// point to next ink word info
		}
	}
	releasePtr();

	*outInkStart = inkDataStart;
	if (!isInkDataFound)
		inkInfoOffset = 0;
	*outInkLength = inkInfoOffset;
}


/* ---------------------------------------------------------------------
	Get the offset to ink data for an ink word number.
	Args:		inWordNo			ink word number
	Return:	offset to InkInfo
--------------------------------------------------------------------- */

ArrayIndex
CRichString::getInkWordNoInfoOffset(ArrayIndex inWordNo) const
{
	ArrayIndex infoOffset = 0;

	const UniChar * str = grabPtr();
	ArrayIndex inkInfoOffset = fOffsetToInk;	// offset to first ink word info
	// scan the string
	for ( ; *str != kEndOfString; str++)
	{
		if (*str == kInkChar)
		{
			// encountered an ink char
			if (inWordNo-- == 0)
			{
				// we’ve reached the required number -- return the offset to its info
				infoOffset = inkInfoOffset;
				break;
			}
			// point to this ink word info
			InkInfo * inkInfo = (InkInfo *)((Ptr) str + inkInfoOffset);
			// update offset to next ink word info
			ArrayIndex	 inkLength = LONGALIGN(sizeof(inkInfo->length) + inkInfo->length);
			inkInfoOffset += inkLength;
		}
	}
	releasePtr();

	return infoOffset;
}


/* ---------------------------------------------------------------------
	Get runs of ink and text within a range pof the string.
	Args:		inStart			offset to start of range
				inCount			length of range
				outLength		pointer to array of run lengths
				outData			pointer to array of text/ink pointers
	Return:	--
--------------------------------------------------------------------- */

void
CRichString::getLengthsAndDataInRange(ArrayIndex inStart, ArrayIndex inCount, short * outLength, Ptr * outData) const
{
	const UniChar *	str = grabPtr();
	const UniChar *	s;
	short	runLength;

	if (inStart < length())
	{
		if (inStart + inCount > length())
			inCount = length() - inStart;

		// skip the ink words before the start of the required range
		ArrayIndex wordNo = 0;
		s = str;
		for (ArrayIndex i = 1; i < inStart; ++i)
			if (*s++ == kInkChar)
				wordNo++;

//		s = str + inStart;	// will be set up from above
		// scan the range
		while (inCount > 0)
		{
			if (*s++ == kInkChar)
			{
			//	add offset to ink to list; ink is always 1 item
				*outData++ = (Ptr) str + getInkWordNoInfoOffset(wordNo++);
				runLength = 1;
				--inCount;
			}
			else
			{
			//	add NULL to list (indicating text); scan to find length of text
				*outData++ = NULL;
				runLength = 1;
				while (--inCount > 0 && *s++ != kInkChar)
					runLength++;
			}
			*outLength++ = runLength;
		}
	}

	releasePtr();
}


/* ---------------------------------------------------------------------
	Count the number of ink words within the string.
	Args:		--
	Return:	the number of ink words
--------------------------------------------------------------------- */

ArrayIndex
CRichString::numInkWords(void) const
{
	ArrayIndex inkWordCount = 0;

	if (format() != 0)
	{
		// string is not plain text
		const UniChar * str = grabPtr();
		for ( ; *str != kEndOfString; str++)
			if (*str == kInkChar)
				inkWordCount++;
		releasePtr();
	}

	return inkWordCount;
}


/* ---------------------------------------------------------------------
	Count the number of ink words within a range of the string.
	Args:		inStart			offset to start of range
				inCount			length of range
	Return:	the number of ink words
--------------------------------------------------------------------- */

ArrayIndex
CRichString::numInkWordsInRange(ArrayIndex inStart, ArrayIndex inCount) const
{
	ArrayIndex inkWordCount = 0;

	if (format() != 0)
	{
		// original iterates over string from the beginning -- rather inefficient so we do like numInkAndTextRunsInRange()
		// constrain inCount to length of string
		if (inStart < length())
		{
			const UniChar * str = grabPtr() + inStart;
			if (inStart + inCount > length())
				inCount = length() - inStart;
			// scan string from start offset
			for ( ; inCount-- != 0 && *str != kEndOfString; str++)
			{
				if (*str == kInkChar)
					inkWordCount++;
			}
			releasePtr();
		}
	}
	return inkWordCount;
}


/* ---------------------------------------------------------------------
	Count the number of ink words and text runs within a range of the string.
	Args:		inStart			offset to start of range
				inCount			length of range
	Return:	the number of runs
--------------------------------------------------------------------- */

ArrayIndex
CRichString::numInkAndTextRunsInRange(ArrayIndex inStart, ArrayIndex inCount) const
{
	ArrayIndex numInkRuns = 0;

	// constrain inCount to length of string
	if (inStart < length())
	{
		const UniChar * str = grabPtr() + inStart;
		if (inStart + inCount > length())
			inCount = length() - inStart;
		// scan string from start offset
		for ( ; inCount-- != 0 && *str != kEndOfString; str++)
		{
			if (*str == kInkChar)
			{
				// found an ink char -- count it and scan for non-ink char
				numInkRuns++;
				for ( ; inCount-- != 0 && *str != kEndOfString; str++)
					if (*str != kInkChar)
						break;
			}
		}
		releasePtr();
	}

	return numInkRuns;
}


/* ---------------------------------------------------------------------
	Return the ink word number at an offset in the string.
	Args:		inOffset			offset in string
	Return:	the ink word number
				kIndexNotFound => there is no ink at that offset
--------------------------------------------------------------------- */

ArrayIndex
CRichString::inkWordNoAtOffset(ArrayIndex inOffset) const
{
	int number = -1;	// MUST also be kIndexNotFound !!
	const UniChar * str = grabPtr();

	if (str[inOffset] == kInkChar)
	{
		// okay, we really do have an ink word at the offset -- now scan the string from the beginning to find its index number
		for (inOffset++; inOffset-- != 0 && *str != kEndOfString; str++)
			if (*str == kInkChar)
				number++;
	}
	releasePtr();

	return number;
}


/* ---------------------------------------------------------------------
	Clone an ink word.
	Args:		inWordNo			the ink word number
	Return:	binary object, class 'inkWord, containing the ink data
--------------------------------------------------------------------- */

Ref
CRichString::cloneInkWordNo(ArrayIndex inWordNo) const
{
	const Ptr strPtr = (const Ptr) grabPtr();
	InkInfo * ink = (InkInfo *)(strPtr + getInkWordNoInfoOffset(inWordNo));
	RefVar clonedInk = AllocateBinary(SYMA(inkWord), ink->length);
	memmove(BinaryData(clonedInk), ink->data, ink->length);
	releasePtr();

	return clonedInk;
}


#pragma mark Comparison
/* ---------------------------------------------------------------------
	Comparison
--------------------------------------------------------------------- */

struct CompareInfo
{
	CRichString *  str;
	ArrayIndex		start;
	CRichString *  other;
};

int
CompareInk(ArrayIndex inOffset1, ArrayIndex inOffset2, void * inRefCon)
{
	CompareInfo * info = (CompareInfo *)inRefCon;
	return info->str->compareInk(*info->other, info->start + inOffset1, inOffset2);
}

int
CRichString::compareSubStringCommon(const CRichString & other, ArrayIndex inStart, ArrayIndex inCount, bool doDiacritical) const
{
	int result;

	if (inCount == kIndexNotFound)
		inCount = length() - inStart;

	const UniChar * thisStr = grabPtr() + inStart;
	const UniChar * otherStr = other.grabPtr();
	CompareInfo info;
	info.str = const_cast<CRichString*>(this);
	info.other = const_cast<CRichString*>(&other);
	info.start = inStart;
	result = CompareUnicodeText(thisStr, inCount, otherStr, other.length(), (const CSortingTable *)1, doDiacritical, CompareInk, &info);
	releasePtr();
	other.releasePtr();

	return result;
}


int
CRichString::compareInk(const CRichString & other, ArrayIndex this1, ArrayIndex other1) const
{
	int result;

	const char * thisStr = (const char *)grabPtr();
	const char * otherStr = (const char *)other.grabPtr();

	InkInfo * thisInk = (InkInfo *)(thisStr + getInkWordNoInfoOffset(inkWordNoAtOffset(this1)));
	InkInfo * otherInk = (InkInfo *)(otherStr + other.getInkWordNoInfoOffset(other.inkWordNoAtOffset(other1)));
	result = memcmp(thisInk->data, otherInk->data, MIN(thisInk->length, otherInk->length));

	releasePtr();
	other.releasePtr();

	return result;
}


#pragma mark Paragraph export
/* -----------------------------------------------------------------------------
	Paragraph stuff.
	The string in the 'text slot of a paragraph view uses the kParaInkChar
	character as a placeholder character instead of the kInkChar code.
	The 'text slot string is not a rich string but might contain ink word
	placeholders.
----------------------------------------------------------------------------- */

Ref
CRichString::makeParagraphTextSlot(void) const
{
	ArrayIndex	txLen = (length() + 1) * sizeof(UniChar);
	RefVar	obj(AllocateBinary(SYMA(string), txLen));
	UniChar *	str;

	LockRef(obj);
	str = (UniChar *)BinaryData(obj);
	memmove(str, grabPtr(), txLen);
	UnlockRef(obj);
	
	for (ArrayIndex i = 0; i < length(); ++i)
		if (str[i] == kInkChar)
			str[i] = kParaInkChar;

	releasePtr();

	return obj;
}


Ref
CRichString::makeParagraphStylesSlot(RefArg inViewFont) const
{
	ArrayIndex	numOfInkWords = numInkWords();
	RefVar		styles;

	if (numOfInkWords == 0)
	{
		// no ink, all text
		styles = AllocateArray(SYMA(styles), 2);
		SetArraySlot(styles, 0, MAKEINT(length()));
		SetArraySlot(styles, 1, inViewFont);
	}
	else
	{
		RefVar	ink;
		const UniChar * str = grabPtr();
		ArrayIndex		runLength, inkIndex, styleIndex = 0;
		// Calculate the length of the styles array
		for (ArrayIndex i = 0; str[i] != kEndOfString; ++i)
			if (str[i] == kInkChar)
				styleIndex++;
			else
				while (str[i] != kInkChar && str[i] != kEndOfString)
					++i;
		// Allocate an array of that length
		styles = AllocateArray(SYMA(styles), styleIndex * 2);

		// Add elements to that array
		runLength = 0;
		inkIndex = 0;
		styleIndex = 0;
		for (ArrayIndex i = 0; str[i] != kEndOfString; ++i)
		{
			if (str[i] == kInkChar)
			{
				if (runLength != 0)
				{
					// Add the run of text up to this ink
					SetArraySlot(styles, styleIndex * 2, MAKEINT(runLength));
					SetArraySlot(styles, styleIndex * 2 + 1, inViewFont);
					styleIndex++;
				}
				// Create an ink object
				InkInfo * inkInfo = (InkInfo *)((Ptr) str + getInkWordNoInfoOffset(inkIndex));
				runLength = inkInfo->length;
				ink = AllocateBinary(SYMA(inkWord), runLength);
				LockRef(ink);
				memmove(BinaryData(ink), inkInfo->data, runLength);
				UnlockRef(ink);
				inkIndex++;
				// Add the run of ink
				SetArraySlot(styles, styleIndex * 2, MAKEINT(1));
				SetArraySlot(styles, styleIndex * 2 + 1, ink);
				styleIndex++;
				// There's no run of text
				runLength = 0;
			}
			else
				while (str[i] != kInkChar && str[i] != kEndOfString)
					// Count the length of text
					++runLength, ++i;
		}
		// Add the last run of text, if any
		if (runLength != 0)
		{
			SetArraySlot(styles, styleIndex * 2, MAKEINT(runLength));
			SetArraySlot(styles, styleIndex * 2 + 1, inViewFont);
		}
		releasePtr();
	}
	
	return styles;
}


#pragma mark Stringer
/* ---------------------------------------------------------------------
	Stringer stuff
--------------------------------------------------------------------- */

void
CRichString::doStringerStuff(char * outText, int * outTextSize, char * outInk, int * outInkSize)
{
	*outTextSize = length() * sizeof(UniChar);
	*outInkSize = fLengthOfInk;

	if (outText || outInk)
	{
		const Ptr str = (const Ptr) grabPtr();	// sic -- we do need a char pointer
		if (outText)
			memmove(outText, str, *outTextSize);
		if (outInk && fLengthOfInk)
			memmove(outInk, str + fOffsetToInk, fLengthOfInk);
		releasePtr();
	}
}


#pragma mark Munging
/* ---------------------------------------------------------------------
	Insert a range of rich text.
	Args:		inSrc				rich text to insert
				inSrcStart		offset to start of range
				inSrcCount		number of characters in range
				index				index at which to insert
	Return:	--
--------------------------------------------------------------------- */

void
CRichString::insertRange(const CRichString * inSrc, ArrayIndex inSrcStart, ArrayIndex inSrcCount, ArrayIndex index)
{
	mungeRange(index, 0, inSrc, inSrcStart, inSrcCount);
}


/* ---------------------------------------------------------------------
	Delete a range of rich text.
	Args:		inStart		offset to start of range
				inCount		number of characters in range
	Return:	--
--------------------------------------------------------------------- */

void
CRichString::deleteRange(ArrayIndex inStart, ArrayIndex inCount)
{
	mungeRange(inStart, inCount, NULL, 0, 0);
}


/* ---------------------------------------------------------------------
	Replace a range of rich text with another.
	Args:		inStart			offset to start of range to replace
				inCount			number of characters in that range
				inSrc				rich text to replace that range
				inSrcStart		offset to start of range
				inSrcCount		number of characters in range
	Return:	--
--------------------------------------------------------------------- */

void
CRichString::mungeRange(ArrayIndex inStart, ArrayIndex inCount, const CRichString * inSrc, ArrayIndex inSrcStart, ArrayIndex inSrcCount)
{
// r4: r1 r2 r5
//sp-24
	ArrayIndex numOfChars = length();	//sp0C
	ArrayIndex originalSize = fSize;		//sp08

	// get replacement ink range
	ArrayIndex replInkStart = 0;			//sp04
	ArrayIndex replInkCount = 0;			//sp00
	if (inSrc != NULL)
		inSrc->getInkData(inSrcStart, inSrcCount, &replInkStart, &replInkCount);

//sp-08  -08
	// get range of ink to be replaced
	ArrayIndex inkStart = 0;	//sp04
	ArrayIndex inkCount = 0;	//sp00
	this->getInkData(inStart, inCount, &inkStart, &inkCount);
//sp-04  stacked arg not balanced

//sp-04  -10
	// calculate deltas in text/ink sizes
	int txtDelta = (inSrcCount - inCount) * sizeof(UniChar);	// r7 number of bytes to insert(delete) in this string
	int inkDelta = replInkCount - inkCount;	//sp04
	int fmtDelta = 0;		//sp00
	bool isInkinessChanged = false;	// r0
	ULong inkiness = format();	//r6
	if (inkiness == 0)
	{
		// we are plain text...
		if (inkDelta > 0)
		{
			//...but want to insert ink
			fmtDelta = 4;	// we will be adding a format word
			inkiness = 1;	// and our format has become inky
			isInkinessChanged = true;
		}
	}
	else
	{
		// we are inky text...
		if (replInkCount == 0 && originalSize - fOffsetToInk - 4 == inkCount)
		{
			//...but want to munge that ink away
			fmtDelta = -4;	// we will be removing the format word
			inkiness = 0;	// and our format has become plain
			isInkinessChanged = true;
		}
	}

	ArrayIndex newTxtSize = (numOfChars + 1) * sizeof(UniChar) + txtDelta;	// r1
	if (inkiness)
		LONGALIGN(newTxtSize);

//sp-08  -18
	int inkTxtDelta = newTxtSize - x14;	//sp04
	int totalDelta = (isInkinessChanged ? inkTxtDelta : txtDelta) + inkDelta + fmtDelta;	//sp00
	if (totalDelta > 0)
		// string will grow -- enlarge the string object ref
		setObjectSize(originalSize + totalDelta);

//sp-04  -1C
	// string has been resized -- now we can grab the string pointer
	const UniChar * srcPtr = inSrc ? inSrc->grabPtr() : NULL;	// sp00
	const UniChar * strPtr = this->grabPtr();	// r8
	if (txtDelta != 0)
	{
		// text length has changed
		const UniChar * txEndPtr = strPtr + (inStart + inCount);	// r9
		// move existing text+ink to make way for insertion
		if ((txtDelta & 0x03) == 0 || format() == 0)
		{
			// ink is long-aligned or there is no ink anyway
			memmove((Ptr)txEndPtr+txtDelta, (Ptr)txEndPtr, originalSize - ((Ptr)txEndPtr - (Ptr)strPtr));
		}
		else
		{
//sp-04  -20
			//r0 = fOffsetToInk;
			bool isShrinking = txtDelta < 0;	// r10
//			Ptr newEndPtr = (Ptr)txEndPtr + txtDelta;			// sp40
			ArrayIndex strEndIndex = numOfChars + 1;			// r1
			ArrayIndex rangeEndIndex = inStart + inCount;	// r2
			Ptr inkInfoPtr = (Ptr)strPtr + fOffsetToInk;		//sp38
//			ArrayIndex trailingSize = (strEndIndex - rangeEndIndex) * sizeof(UniChar);	//sp3C
//			Ptr sp34 = inkInfoPtr + inkTxtDelta;
//			ArrayIndex inkInfoSize = originalSize - fOffsetToInk;
//			int sp00 = 1;
//			do {
//				if (isShrinking)
//				{
					memmove((Ptr)txEndPtr + txtDelta, txEndPtr, (strEndIndex - rangeEndIndex) * sizeof(UniChar));	// move text only
//					isShrinking = false;
//				}
//				else
//				{
					if (inkTxtDelta != 0)
						memmove(inkInfoPtr + inkTxtDelta, inkInfoPtr, originalSize - fOffsetToInk);	// move ink only -- long aligned
//					isShrinking = true;
//				}
//			} while (sp00-- != 0);
//sp+04  -1C
		}
	}

	ArrayIndex newInkTxtSize = originalSize + inkTxtDelta;	// r9

	if (inSrc != NULL && inSrcCount != 0)
		// copy inSrc into our string
		memmove((void *)(strPtr + inStart), srcPtr + inSrcStart, inSrcCount * sizeof(UniChar));

	// update rich string ivars
	fFormat = inkiness;
	fNumOfChars = numOfChars + txtDelta / sizeof(UniChar);
	fSize += totalDelta;

	if (inkiness == 0)
	{
		// plain text, no ink
		fOffsetToInk = x14 = LONGALIGN((fNumOfChars+1)*sizeof(UniChar));
	}

//sp-04  -20
	Ptr inkInfoPtr = (Ptr)strPtr + fOffsetToInk;	// sp00

	if (replInkCount != inkCount)
	{
		// need to adjust space for ink info
		Ptr inkPtr = inkInfoPtr + inkStart;
		memmove(inkPtr + inkDelta, inkPtr, newInkTxtSize - (inkPtr - (Ptr)strPtr));
		newInkTxtSize += inkDelta;
	}
	newInkTxtSize += fmtDelta;

	// copy ink info
	if (inSrc != NULL && replInkCount != 0)
	{
		Ptr replInkInfoPtr = (Ptr)srcPtr + inSrc->fOffsetToInk;
		memmove(inkInfoPtr + inkStart, replInkInfoPtr + replInkStart, replInkCount);
	}

	if (inkiness != 1)
	{
		fOffsetToInk = 0;
		fLengthOfInk = 0;
		x14 = newInkTxtSize;
	}
	else
	{
		*((Ptr)strPtr + newInkTxtSize - 4) = (fNumOfChars << 4) | inkiness;	// unaligned long
		fLengthOfInk = newInkTxtSize - fOffsetToInk - 4;
	}

	x20 = fLengthOfInk;

	// clean up
	if (inSrc)
		inSrc->releasePtr();
	this->releasePtr();

	if (totalDelta < 0)
		// only now can we shrink the object
		setObjectSize(newInkTxtSize);

	format();	// sic -- return value? redundant?
}


#pragma mark Debug
/* ---------------------------------------------------------------------
	D E B U G
	Verify the rich string structures.
	Args:		--
	Return:	error code
					100				length() does not match actual number of characters
					110				numInkWords() does not match actual number of ink words
					120				numInkAndTextRunsInRange() does not match actual number of runs
					130
					140
					150
					155				format discrepancy
					160
					170
					180
					200
					210
					1000 + index	getChar() does not return character at index in string
					2000 + index	inkWordNoAtOffset() does not return ink word at index in string
					3000 + index	string longer than cached size
					4000 + index	getInkWordNoInfoOffset() does not match calculated info offset
					5000 + index
--------------------------------------------------------------------- */
extern int	GetCStringFormat(const UniChar * inStr, ArrayIndex index);
extern int	GetStringFormat(RefArg inStr);

NewtonErr
CRichString::verify(void)
{
	NewtonErr err;
	const UniChar * strPtr = grabPtr();
	const UniChar * strEnd = (const UniChar *)((Ptr)strPtr + (fOffsetToInk ? fOffsetToInk : fSize));

	XTRY
	{
		ArrayIndex inkWordNo = 0;	// r7
		ArrayIndex numOfRuns = 0;	// r8
		bool isInTextRun = false;		// r9

		ArrayIndex index = 0;	// index into UniChar string
		UniChar ch;
		// scan string checking text and ink chars
		for (const UniChar * chPtr = strPtr; (ch = *chPtr++) != 0; index++)
		{
			XFAILIF(getChar(index) != ch, err = 1000 + index;)
			if (ch == kInkChar)
			{
				XFAILIF(inkWordNoAtOffset(index) != inkWordNo, err = 2000 + index;)
				inkWordNo++;
				numOfRuns++;
				isInTextRun = false;
			}
			else
			{
				if (!isInTextRun)
				{
					numOfRuns++;
					isInTextRun = true;
				}
			}
			XFAILIF(chPtr >= strEnd, err = 3000 + index;)
		}
		XFAIL(err)

		XFAILIF(index != length(), err = 100;)
		XFAILIF(inkWordNo != numInkWords(), err = 110;)
		XFAILIF(numOfRuns != numInkAndTextRunsInRange(0, index), err = 120;)

		ArrayIndex eos = (index + 1) * sizeof(UniChar);		// length of string in bytes, incl nul terminator

		if (!fIsInky)
		{
			if (fFormat == 0)
			{
				XFAILIF(eos != fSize, err = 130;)
			}
			else
			{
				XFAILIF(LONGALIGN(eos) != fOffsetToInk, err = 140;)
			}
		}
		else
		{
			// there is ink -- check its offset
			XFAILIF(x14 < eos, err = 150;)
			REPprintf("text slop: %d\n", x14 - eos);	// original has no newline
			eos = x14;
		}

		if (fUString)
		{
			XFAILIF(GetCStringFormat(fUString, fSize) != fFormat, err = 155;)
		}
		else if (NOTNIL(fRefString))
		{
			XFAILIF(GetStringFormat(fRefString) != fFormat, err = 155;)
		}

		if (fFormat == 0)
		{
			// should be plain text, no ink data
			XFAILIF(eos != fSize, err = 160;)
			XFAILIF(inkWordNo != 0, err = 170;)
		}

		eos = LONGALIGN(eos);	// r10
		XFAILIF(eos != LONGALIGN((length() + 1) * sizeof(UniChar)), err = 180;)
		strEnd = strPtr + eos;	// r6

		// iterate over  ink words
		InkInfo * inkInfoPtr = (InkInfo *)strEnd;
		InkInfo * inkInfoEnd = (InkInfo *)((Ptr)strEnd + fSize);	// sp00 but that’s mad!?
		ArrayIndex inkInfoOffset = 0;	// r9
		ArrayIndex inkInfoLen;
		index = 0;		// r8
		while (inkWordNo-- != 0)
		{
			XFAILIF(getInkWordNoInfoOffset(index) != eos + inkInfoOffset, err = 4000 + index;)
			inkInfoLen = LONGALIGN(sizeof(UShort) + inkInfoPtr->length);		// sizeof(UShort) == sizeof(InkInfo)
			inkInfoOffset += inkInfoLen;
			inkInfoPtr = (InkInfo *)((Ptr)inkInfoPtr + inkInfoLen);
			index++;
			XFAILIF(inkInfoPtr >= inkInfoEnd, err = 5000 + index;)
		}
		XFAIL(err)

		if (fIsInky)
		{
#if 0
			eos = LONGALIGN((length() + 1) * sizeof(UniChar));
			r1 = strEnd - strPtr - eos;
			XFAILIF(r1 > eos, err = 200;)
			REPprintf("ink slop: %d\n", x14 - strEnd);	// original has no newline
			strEnd = LONGALIGN(x20 + (strEnd - fLengthOfInk));
#endif
		}

		XFAILIF(length() != *strEnd >> 4, err = 210;)
	}
	XENDTRY;
	
	releasePtr();
	return err;
}
