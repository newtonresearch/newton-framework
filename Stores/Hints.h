/*
	File:		Hints.h

	Contains:	Text hints, for indexing PSS text.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__HINTS_H)
#define __HINTS_H 1

/* -----------------------------------------------------------------------------
	T e x t H i n t s
----------------------------------------------------------------------------- */

struct TextHint
{
	int		x00[2];
};

void	ClearHintBits(TextHint * inHint);


class CWordHintsHandler
{
public:
	virtual ArrayIndex	getNumHintChunks(ArrayIndex inNumChars, ArrayIndex * outHintChunkSize);
	virtual bool			findHintWord(UniChar *& inStr, ArrayIndex & outWordLen, ArrayIndex & ioStrLen);
	virtual void			setHints(TextHint * outHint, UniChar * inStr, ArrayIndex inLen);
};


class COldWordHintsHandler : public CWordHintsHandler
{
public:
	void	setHints(long * outHint, UniChar * inStr, ArrayIndex inLen);
};


extern ULong		gMaxHintsHandlerId;
extern ULong		gDefaultHintsHandlerId;

#define kNumOfHintsHandlers 4
extern CWordHintsHandler *	gHintsHandlers[kNumOfHintsHandlers];

extern char *	GetWordsHints(RefArg inWords);
extern int		TestObjHints(char * inHints, ArrayIndex inNumOfWords, CStoreWrapper * inStoreWrapper, PSSId inId);


/* -----------------------------------------------------------------------------
	WithPermObjectTextDo
----------------------------------------------------------------------------- */

typedef bool (*TextProcPtr)(UniChar * inStr, ArrayIndex inStrLen, void * inData);

extern bool		WithPermObjectTextDo(CStoreWrapper * inStoreWrapper, PSSId inObjId, TextProcPtr inTextProc, void * inTextProcData, void ** ioRefCon);
extern void		ReleasePermObjectTextCache(void * instance);


#endif	/* __HINTS_H */
