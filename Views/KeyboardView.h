/*
	File:		KeyboardView.h

	Contains:	CKeyboardView declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__KEYBOARDVIEW_H)
#define __KEYBOARDVIEW_H 1

#include "View.h"


/*------------------------------------------------------------------------------
	K e y   F o r m a t   B i t s
------------------------------------------------------------------------------*/

#define keyVEighth		 1
#define keyVQuarter		(1 <<  1)
#define keyVHalf			(1 <<  2)
#define keyVUnit			(1 <<  3)
#define keyVSize(x)		((x) & 0xFF)

#define keyHEighth		(1 <<  8)
#define keyHQuarter		(1 <<  9)
#define keyHHalf			(1 << 10)
#define keyHUnit			(1 << 11)
#define keyHSize(x)		(((x) >> 8) & 0xFF)

#define keyRightOpen		(1 << 16)
#define keyTopOpen		(1 << 17)
#define keyBottomOpen	(1 << 18)
#define keyLeftOpen		(1 << 19)
#define keyOpenings(x)	((x) & (0x0F << 16))

#define keyRounding(x)	(((x) >> 20) & 0x07)

#define keyFrameWd(x)	(((x) >> 23) & 0x03)

#define keyInset(x)		(((x) >> 25) & 0x07)

#define keyAutoHilite	(1 << 28)
#define keySpacer			(1 << 29)


/*------------------------------------------------------------------------------
	C K e y I t e r a t o r s
------------------------------------------------------------------------------*/
class CKeyboardView;

class CRawKeyIterator
{
public:
				CRawKeyIterator(RefArg inKeyDef);

	void		copyInto(CRawKeyIterator * ioOther);
	void		loadRow(void);
	bool		loadKey(void);
	bool		next(void);
	void		reset(void);

	Ref		keyLegend(void) const;
	Ref		keyResult(void) const;
	ULong		keyFormat(void) const;

protected:
	friend class CKeyboardView;

	RefStruct	fCurKeyLegend;
	RefStruct	fCurKeyResult;
	ULong			fCurKeyFormat;
	RefStruct	fKeyDefs;
	RefStruct	fRowDef;
	int			fColumn;
	int			fRow;
	int			fNumOfColumns;
	int			fNumOfRows;
	int			fNumOfKeys;
};

inline 	Ref		CRawKeyIterator::keyLegend(void) const
{ return fCurKeyLegend; }

inline 	Ref		CRawKeyIterator::keyResult(void) const
{ return fCurKeyResult; }

inline 	ULong		CRawKeyIterator::keyFormat(void) const
{ return fCurKeyFormat; }


class CVisKeyIterator : public CRawKeyIterator
{
public:
				CVisKeyIterator(RefArg inKeyDef, Rect * inRect, Point inPt);

	void		copyInto(CVisKeyIterator * ioOther);
	bool		findEnclosingKey(Point inPt);
	void		loadRow(void);
	bool		loadKey(void);
	bool		next(void);
	void		reset(void);
	bool		skipToStartOfNextRow(void);

	const Rect *	rowFrame(void) const;
	const Rect *	keyFrame(void) const;

protected:
	friend class CKeyboardView;

	Rect			fKeyBox;					// +2C	current key rect
	Rect			fKeyFrame;				// +34	current key frame (exterior)
	Rect			fKeyCap;					// +3C	current key cap (interior)
	Rect			fRowBounds;				// +44
	bool			fHasVisKeys;			// +4C
	int			fFirstVisKeyIndex;	// +50
	int			fLastVisKeyIndex;		// +54
	Rect			fUnitBounds;			// +58	full key size
	Point			fRowOrigin;				// +60
	Point			fKeyOrigin;				// +64	current key origin
	int			fRowHeight;				// +6C
	int			fMaxRowHeight;			// +70
};

inline 	const Rect *	CVisKeyIterator::rowFrame(void) const
{ return &fRowBounds; }

inline 	const Rect *	CVisKeyIterator::keyFrame(void) const
{ return &fKeyFrame; }

/*------------------------------------------------------------------------------
	C K e y b o a r d V i e w
------------------------------------------------------------------------------*/

class CKeyboardView : public CView
{
public:
	VIEW_HEADER_MACRO

							~CKeyboardView();

//	Construction
	virtual  void		init(RefArg inContext, CView * inView);

	virtual  bool		realDoCommand(RefArg inCmd);

// View manipulation
	virtual	bool		insideView(Point inPt);

// Keyboard
	bool					doKey(CVisKeyIterator & inIter);
	void					handleKeyPress(CVisKeyIterator & inIter, RefArg inKeycode);
	void					postKeypressCommands(RefArg inCmd);
	Ref					getLegendRef(CRawKeyIterator & inIter);
	Ref					getResultRef(CRawKeyIterator & inIter);

	bool					trackStroke(CStroke * inStroke, CVisKeyIterator * inIter);

// Drawing
	virtual	void		realDraw(Rect * inBounds);
	void					drawKey(CVisKeyIterator & inIter, bool inIsHighlighted, bool);
	void					drawKeyFrame(CVisKeyIterator & inIter, bool inIsHighlighted, bool);

// first				f28;
	long				fKeyArrayIndex;			// +30	if key has several caps, the current index
	bool				fKeyResultsAreKeycodes;	// +34
	RefStruct		fKeyDefinitions;			// +38
	RefStruct		fKeyReceiverView;			// +3C
	StyleRecord		fStyle;						// +40
//	StyleRecord *	fStylePtr;					// +60
	TextOptions		fOptions;					// +64
	FontInfo			fFontInfo;					// +80
	Rect				fUnitBounds;				// +88
	bool				fHasSound;					// +90
// size +94
};

#endif	/* __KEYBOARDVIEW_H */
