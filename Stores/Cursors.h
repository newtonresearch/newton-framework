/*
	File:		Cursors.h

	Contains:	Soup query cursor interface.

	Written by:	Newton Research Group, 2007.
*/

#if !defined(__CURSORS_H)
#define __CURSORS_H 1

#include "Indexes.h"

/*------------------------------------------------------------------------------
	C u r s o r S o u p I n f o
------------------------------------------------------------------------------*/

class CursorSoupInfo
{
public:
			CursorSoupInfo();

	Ref	fSoup;		// +00	soup this cursor belongs to
	Ref	fTags;		// +04	tags encoded for query
};


/*------------------------------------------------------------------------------
	C u r s o r S t a t e
------------------------------------------------------------------------------*/

struct CursorState
{
	int		seqInUnion;			// +00
	RefVar	entry;				// +04
	SKey		entryKey;			// +08
	PSSId		entryId;				// +58
	bool		isCursorAtEnd;		// +5C
	bool		isEntryDeleted;	// +5D
};


/*------------------------------------------------------------------------------
	C C u r s o r
	Must also do CCollectCursor based on CCursor (hence the virtuals).
------------------------------------------------------------------------------*/

class CCursor
{
public:
							CCursor();
	virtual				~CCursor();

	static	Ref		createNewCursor(void);

				void		init(RefArg, RefArg, RefArg);
				void		init(RefArg, const CCursor * inCursor);
	virtual	Ref		clone(void);
	virtual	ArrayIndex	countEntries(void);

	virtual	void		rebuildInfo(bool, long);
	virtual	void		invalidate(void);
				Ref		cloneFrameSlot(RefArg inFrame, RefArg inTag) const;
				void		buildSoupsInfo(void);
				ArrayIndex	getSoupInfoIndex(RefArg inSoup);
				void		createIndexes(void);
				void		park(bool inEnd);
				Ref		isParked(void);
				int		exitParking(bool inForward);

				bool		keyBoundsValidTest(const SKey &, bool);
				bool		wordsValidTest(PSSId);
				bool		textValidTest(PSSId);
				bool		validTest(const SKey &, PSSId, bool, bool*, bool*);

				void		getState(CursorState * outState);
				void		setState(CursorState & inState);
				void		makeEntryFaultBlock(PSSId);
				void		soupRemoved(RefArg inSoup);
				void		soupAdded(RefArg inSoup);
				Ref		status(void);
				void		setSoup(RefArg inSoup);
				void		indexRemoved(RefArg inArg1, RefArg inQuerySpec);
				void		indexObjectsChanged(void);
				void		soupTagsChanged(RefArg inSoup);
				bool		pinCurrentKey(void);

				Ref		reset(void);
				Ref		resetToEnd(void);
	virtual	Ref		move(int);
	virtual	Ref		gotoEntry(RefArg inEntry);
	virtual	Ref		gotoKey(RefArg inKey);
				Ref		entryKey(void);
				Ref		entry(void);

				void		entryChanged(RefArg inEntry, bool inArg2, bool inArg3);
				void		entryReadded(RefArg inEntry, RefArg inNewEntry);
	virtual	void		entryRemoved(RefArg inEntry);
	virtual	void		entrySoupChanged(RefArg inEntry, RefArg inNewEntry);

				void		registerInSoup(RefArg) const;
				void		unregisterFromSoup(RefArg) const;

	virtual	void		gcMark(void);
	virtual	void		gcUpdate(void);

				Ref		soup(void)			const;
				Ref		indexPath(void)	const;

private:
	Ref		fSoup;					// +04
	Ref		fCursor;					// +08
	unsigned	fQuerySpecBits;		// +0C
	ArrayIndex	fNumOfSoupsInUnion;	// +10
	CursorSoupInfo *	fSoupInfo;			// +14
	CUnionSoupIndex *	fUnionSoupIndex;	// +18
	Ref		fTagSpec;				// +1C
	CSoupIndex **	fTagsIndexes;	// +20
	Ref		fIndexPath;				// +24
	Ref		fIndexType;				// +28
	bool		fIsSecSortOrder;		// +2C
	Ref		fQryWords;				// +30
	char *	fHints;					// +34
	Ref		fQryText;				// +38
	Ref		fIndexValidTestFn;	// +3C
	Ref		fValidTestFn;			// +40
	Ref		fEndTestFn;				// +44
	Ref		fTestFnArgs;			// +48
	Ref		fStartKey;				// +4C
	Ref		fBeginKey;				// +50
	Ref		fEndKey;					// +54
	char *	fBeginKeyData;			// +58
	char *	fEndKeyData;			// +5C
	bool		fIsIndexInvalid;		// +60
	PSSId		fEntryId;				// +64	indexData -- id of entry on store
	Ref		fCurrentEntry;			// +68
	SKey		fIndexKey;				// +6C
	bool		fIsCursorAtEnd;		// +BC	0 => at beginning, 1 => at end
	bool		fIsEntryDeleted;		// +BD	current entry is deleted
//size +C0
};

inline	Ref	CCursor::soup(void)	const
{ return fSoup; }

inline	Ref	CCursor::indexPath(void)	const
{ return fIndexPath; }


enum QueryFlags
{
	kQueryTags				= 1,
	kQueryWords				= 1 <<  1,
	kQueryEntireWords		= 1 << 10,
	kQueryText				= 1 <<  2,
	kQueryBeginKey			= 1 <<  3,
	kQueryBeginExclKey	= 1 <<  4,
	kQueryEndKey			= 1 <<  5,
	kQueryEndExclKey		= 1 <<  6,
	kQueryIndexValidTest	= 1 <<  7,
	kQueryValidTest		= 1 <<  8,
	kQueryEndTest			= 1 <<  9,

	kQueryByBeginKey		= kQueryBeginKey + kQueryBeginExclKey,
	kQueryByEndKey			= kQueryEndKey + kQueryEndExclKey,
	kQueryByKey				= kQueryByBeginKey + kQueryByEndKey,
	kQueryByTest			= kQueryIndexValidTest + kQueryValidTest + kQueryEndTest,

	kQueryByAnything		= BITS(0,9)
};


/*------------------------------------------------------------------------------
	Soup update functions;
------------------------------------------------------------------------------*/

enum ESoupCursorOp
{
	kSoupAdded,
	kSoupRemoved,
	kEntryRemoved,
	kSoupSet,
	kSoupTagsChanged,
	kSoupIndexesChanged,
	kEntrySoupChanged,
	kSoupEntryReadded,
	kSoupIndexRemoved
};

void EachSoupCursorDo(RefArg inSoup, ESoupCursorOp inOp);
void EachSoupCursorDo(RefArg inSoup, ESoupCursorOp inOp, RefArg inArg);
void EachSoupCursorDo(RefArg inSoup, ESoupCursorOp inOp, RefArg inArg1, RefArg inArg2);


CCursor * CursorObj(RefArg cursor);
void	DefineCursor(RefArg, RefArg, RefArg);

Ref	QueryEntriesWithTags(RefArg soup, RefArg tags);


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	CommonSoupQuery(RefArg inRcvr, RefArg inQuerySpec);
Ref	SoupCollect(RefArg inRcvr);

Ref	CursorCountEntries(RefArg inRcvr);
Ref	CursorClone(RefArg inRcvr);
Ref	CursorReset(RefArg inRcvr);
Ref	CursorResetToEnd(RefArg inRcvr);
Ref	CursorWhichEnd(RefArg inRcvr);
Ref	CursorEntry(RefArg inRcvr);
Ref	CursorEntryKey(RefArg inRcvr);
Ref	CursorMove(RefArg inRcvr, RefArg inOffset);
Ref	CursorNext(RefArg inRcvr);
Ref	CursorPrev(RefArg inRcvr);
Ref	CursorGoto(RefArg inRcvr, RefArg inEntry);
Ref	CursorGotoKey(RefArg inRcvr, RefArg inKey);
Ref	CursorSoup(RefArg inRcvr);
Ref	CursorIndexPath(RefArg inRcvr);
Ref	CursorStatus(RefArg inRcvr);
}


#endif /* __CURSORS_H */
