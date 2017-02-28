/*
	File:		RichStrings.h

	Contains:	Rich string class declaration.

	Written by:	Newton Research Group
*/

#if !defined(__RICHSTRINGS_H)
#define __RICHSTRINGS_H 1

// name clash in AssertMacros.h
#if defined(verify)
#undef verify
#endif

#define kInkChar		((UniChar)0xF700)
#define kParaInkChar	((UniChar)0xF701)
#define kKeyInkChar	((UniChar)0xF702)

struct InkInfo
{
	UShort	length;
	char		data[];
};

/*----------------------------------------------------------------------
	R i c h S t r i n g
	From NPR 7-12:
	The rich string format lets you embed ink data in a text string.
	The location of each ink word in the string is indicated by a placeholder character (0xF700 or 0xF701),
	and the data for each ink word is stored after the string terminator character at the end of the string.
	The final 32 bits in a rich string also have special meaning.
	eg
	"Try <kInkChar> this." (UniChar)kEndOfString (InkInfo[])inkData (ULong)format
	InkInfo is long-aligned - there may be two bytes of slop after the kEndOfString
	=> the final format word is long-aligned
	format word:
		28 bits	= number of unicode chars in string (reverse-Pascal style)
		 4 bits	= flags
----------------------------------------------------------------------*/

class CRichString
{
public:
					CRichString();
					CRichString(RefArg obj);
					CRichString(UniChar * str);
					CRichString(UniChar * str, ArrayIndex size);		// watch this -- size MUST include nul terminator
					~CRichString();

	UniChar *	grabPtr(void) const;
	void			releasePtr(void) const;

	void			setNoStringData(void);
	void			setStringData(RefArg obj);
	void			setUStringData(UniChar * str, ArrayIndex size);	// was setCStringData
	void			setUStringData(UniChar * str);						// was setCPlainStringData
	void			setChar(ArrayIndex index, UniChar ch);
	void			setObjectSize(ArrayIndex size);
	void			setFormatAndLength(UniChar * str, ArrayIndex size);

	ULong			format(void) const;
	ArrayIndex	length(void) const;
	UniChar		getChar(ArrayIndex index) const;
	void			getInkData(ArrayIndex inStart, ArrayIndex inCount, ArrayIndex * outInkStart, ArrayIndex * outInkLength) const;
	ArrayIndex	getInkWordNoInfoOffset(ArrayIndex number) const;
	void			getLengthsAndDataInRange(ArrayIndex start, ArrayIndex count, short *, Ptr *)const;
	ArrayIndex	numInkWords(void) const;
	ArrayIndex	numInkWordsInRange(ArrayIndex start, ArrayIndex count) const;
	ArrayIndex	numInkAndTextRunsInRange(ArrayIndex start, ArrayIndex count) const;
	ArrayIndex	inkWordNoAtOffset(ArrayIndex inOffset) const;
	Ref			cloneInkWordNo(ArrayIndex inWordNo) const;

	int			compareSubStringCommon(const CRichString &, ArrayIndex offset, ArrayIndex length, bool doDiacritical = false) const;
	int			compareInk(const CRichString &, ArrayIndex, ArrayIndex) const;

	Ref			makeParagraphStylesSlot(RefArg) const;
	Ref			makeParagraphTextSlot(void) const;

	void			doStringerStuff(char *, int *, char *, int *);

	void			insertRange(const CRichString * src, ArrayIndex start, ArrayIndex count, ArrayIndex index);
	void			deleteRange(ArrayIndex start, ArrayIndex count);
	void			mungeRange(ArrayIndex start, ArrayIndex count, const CRichString * src, ArrayIndex srcStart, ArrayIndex srcCount);

	NewtonErr	verify(void);

private:
	RefStruct	fRefString;		// +00
	UniChar *	fUString;		// +04
	ArrayIndex	fSize;			// +08 in bytes
	ArrayIndex	fNumOfChars;	// +0C in UniChars, excluding nul terminator
	ULong			fFormat;			// +10
	int			x14;
	ArrayIndex	fOffsetToInk;	// +18
	ArrayIndex	fLengthOfInk;	// +1C
	int			x20;
	bool			fIsInky;			// +24
};

inline ArrayIndex CRichString::length(void) const
{	return fNumOfChars; }

inline ULong CRichString::format(void) const
{	return fFormat; }


extern Ref MakeRichString(RefArg inText, RefArg inStyles, bool inArg3);


#endif	/* __RICHSTRINGS_H */
