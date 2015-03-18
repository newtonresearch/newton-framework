/*
	File:		Dictionaries.h

	Contains:	Dictionary declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__DICTIONARIES_H)
#define __DICTIONARIES_H 1

#include "RecObject.h"


extern NewtonErr	airusResult;


/* -----------------------------------------------------------------------------
	T r i e
	Dictionary data format.
----------------------------------------------------------------------------- */

struct Trie
{
	UChar		signature;	// should be 'a'
#if defined(hasByteSwapping)
	UChar		type:4;
	UChar		attrLen:4;
#else
	UChar		attrLen:4;
	UChar		type:4;
#endif
	UChar		data[];
};


/* -----------------------------------------------------------------------------
	C D i c t i o n a r y
----------------------------------------------------------------------------- */
typedef bool (*DictWalker)(UniChar * inWord, ULong, UChar, ULong, void *);

typedef void (*AProcPtr)(ULong,ULong,ULong,ULong);


class CDictionary
{
public:
					CDictionary(ULong inType, ULong inArg2);
					CDictionary(void * inData, size_t inSize);
					CDictionary(RefArg inData);
					~CDictionary();

	int			type(void);
	void			setId(int inId);
	int			id(void);
	NewtonErr	expand(size_t inDelta);

	void			verifyStart(void);
	NewtonErr	verifyCharacter(UChar * inStr, UChar ** outArg3, ULong ** outArg4, ULong ** outArg5, bool inArg6, UChar ** outArg7);
	NewtonErr	verifyWord(UChar * inStr, UChar ** outArg3, ULong ** outArg4, ULong ** outArg5, bool inArg6, UChar ** outArg7);
	NewtonErr	verifyString(UniChar * inStr, UniChar ** outArg3, ULong ** outArg4, char ** outArg5);

	NewtonErr	addWord(ULong inSeq, UniChar * inWord, ULong inArg4);
	NewtonErr	deleteWord(UniChar * inWord);
	NewtonErr	deletePrefix(UniChar * inPrefix);

	bool			hasActualOrImpliedAttr(void);
	ArrayIndex	attributeLength(void);
	NewtonErr	changeAttribute(UniChar * inWord, ULong inAttribute);

	NewtonErr	walk(UniChar * inWord, DictWalker inWalker, void * inData);

// for Dates
	NewtonErr	findLongestWord(UChar * outWord, const UniChar * inStr, ArrayIndex inStrLen, ArrayIndex * outWordLen);
	NewtonErr	parseString(void * inContext, const UniChar * inStr, ArrayIndex * outParsedStrLen, ArrayIndex inStrLen);

private:
	void			init(void * inData, size_t inSize);
	CDictionary *	positionToHandle(ArrayIndex inSeq);

	ULong			readInt(ULong inOffset, int inLen);
	UShort		getSymbol(ULong inOffset);
	ULong			RPByteSize(ULong inOffset);
	ULong			skipNode(ULong inOffset);
	ULong			getAttr(ULong inOffset);
	ULong			followLeft(ULong inOffset);

	void			despatch(int inSelector);
	void			startA(void);
	void			exitA(void);
	void			verify(void);
	void			addWord(void);
	void			deleteWord(void);
	void			firstLast(void);
	void			nextPrevious(void);
	void			changeAttribute(void);
	void			nextSet(void);
	void			nextSet9(void);

	ULong		terminalCharsOffset(ULong inOffset);
	ULong		nextNodeOffset(ULong inOffset);
	void		AL_getAttribute(ULong inOffset);
	void		AL_getAttribute2(ULong inOffset);
	void		AL_verify(void);
	void		AL_nextSetCB(OpaqueRef ioArg1, OpaqueRef inArg2, ULong inArg3, ULong inArg4);
	int		AL_nextSet9(void);
	int		AL_nextSet(void);


	int			x00;
	int			fId;				// +04	dictionary id
	Trie *		fTrie;			// +0C	pointer to dict data
	UChar *		fTrieEnd;		// +10	pointer past end of dict data
	ArrayIndex	fTrieSize;		// +14	size of dict data
	ArrayIndex	fCharSize;		//			size of chars in the dictionary; 1|2 bytes
	ArrayIndex	fChunkSize;		// +18	expansion chunk size
	UniChar *	fStr;				// +1C	was void * since char strings were also handled
	ULong			fStrIndex;		// +20	index into fStr string
	ULong			fStrLen;			//			length of fStr string
	ULong			fAttr;			// +24
	ULong			fTrieOffset;	// +28	offset into trie
	int			x2C;				// error code
	int			x30;
	ArrayIndex	fAttrLen;		// attribute size
	int			x38;
	int			x3C;
	CDictionary *	fHead;		// +40
	CDictionary *	fNext;		// +44
	char *		fNodeName;		// +48
	int			x4C;
	UniChar **	x50;
	AProcPtr		x54;
// size +58
};

inline int		CDictionary::type(void)  { return fTrie->type; }	// could we & 0x07 ?
inline void		CDictionary::setId(int inId)  { fId = inId; }
inline int		CDictionary::id(void)  { return fId; }

/* -----------------------------------------------------------------------------
	L e x i c a l   D i c t i o n a r i e s
----------------------------------------------------------------------------- */

extern CDictionary *	gTimeLexDictionary;
extern CDictionary *	gDateLexDictionary;
extern CDictionary *	gPhoneLexDictionary;
extern CDictionary *	gNumberLexDictionary;


/* -----------------------------------------------------------------------------
	C D i c t C h a i n
----------------------------------------------------------------------------- */
struct DictListEntry;

class CDictChain : public CDArray
{
public:
						CDictChain();

	static CDictChain *	make(ArrayIndex inSize, ArrayIndex inArg2 = kIndexNotFound);

	void				addDictToChain(CDictionary * inDict);
	ArrayIndex		removeDictFromChain(CDictionary * inDict);

	CDictionary *	positionToHandle(ArrayIndex inSeq);
	ArrayIndex		handleToPosition(CDictionary * inDict);

//	void				lockChain(void);			we don’t use Handles any more
//	void				unlockChain(void);

protected:
	NewtonErr		iDictChain(ArrayIndex inSize, ArrayIndex inArg2);

	DictListEntry *	f1C;
	ArrayIndex			f20;
// size+24
};



/* -----------------------------------------------------------------------------
	F u n c t i o n   D e c l a r a t i o n s
----------------------------------------------------------------------------- */

extern void				InitDictionaries(void);
extern CDictionary *	GetROMDictionary(ULong inDictNumber);

extern void				VerifyStart(CDictionary * inDict);
extern void				VerifyWord(CDictionary * inDict, UChar * inArg2, UChar ** outArg3, ULong ** outArg4, ULong ** outArg5, bool inArg6, UChar ** inArg7);
extern void				VerifyString(CDictionary * inDict, UniChar * inStr, UniChar ** outArg3, ULong ** outArg4,  char ** outArg5);

extern NewtonErr		ParseString(CDictionary * inDict, void * inContext, const UniChar * inStr, ArrayIndex * outParsedStrLen, ArrayIndex inStrLen);

extern void				BuildChains(CDictChain ** outChains, RefArg inContext);

#endif	/* __DICTIONARIES_H */
