/*
	File:		ParagraphView.h

	Contains:	CParagraphView declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__PARAGRAPHVIEW_H)
#define __PARAGRAPHVIEW_H 1

#include "DataView.h"

/*------------------------------------------------------------------------------
	C P a r a g r a p h V i e w
------------------------------------------------------------------------------*/

class CParagraphView : public CDataView
{
public:
	VIEW_HEADER_MACRO

							~CParagraphView();

	virtual	void		init(RefArg inContext, CView * inView);
	virtual	void		realDraw(Rect& inRect);

	Ref	extractRangeAsRichString(ArrayIndex inRangeStart, ArrayIndex inRangeEnd);
	Ref	extractTextRange(ArrayIndex inRangeStart, ArrayIndex inRangeEnd);
	Ref	getStyleAtOffset(ArrayIndex inOffset, long*, long*);

	void	flushWordAtCaret(void);

#if 0
	void	AddHilited(RefArg, CEditView*);
	void	AddTabStop(Rect&);
	void	AddWord(Finder*, const unsigned short*, unsigned long, RefArg, long*);
	void	AdjustHilites(long, long);
	void	AdjustInsertAreasAfterDeletion(InsertRunList*, unsigned long, unsigned long);
	void	AdjustInsertAreasAfterInsertion(InsertRunList*, unsigned long, unsigned long, unsigned char);
	void	AdjustStyles(long, long, long, RefArg, long);
	void	Area(long, long);
	void	BoundsOfLastLine(Rect*);
	void	ChangeStyleOfSelection(RefArg);
	void	CleanupData(void);
	void	ClearAllCaches(void);
	void	ClickOptions(void);
	void	CountTabStops(long, long);
	void	CreateAllCaches(void);
	void	CreateStyleRecordCache(short**);
	void	DeleteHilited(RefArg);
	void	DeleteHilitedTextOnly(RefArg);
	void	DestroyStyleRecordCache(void);
	void	DrawHilites(unsigned char);
	void	DrawHiliting(void);
	void	DrawScaledData(const Rect&, const Rect&, Rect*);
	void	FillAllCaches(short*);
	void	FindLineContainingCharOffset(long);
	void	FindLineContainingPoint(Point*, MarginSize);
	void	FindTab(Finder*, long);
	void	FindTextRunContainingCharOffset(LineInfo*, long, long*);
	void	FindTextRunContainingCoordinate(LineInfo*, short, long*);
	void	FindWordInParagraph(Finder*);
	void	FindWordInRun(Finder*);
	void	FindWordOffset(Point, long*, Point*);
	void	FixupBBox(void);
	void	GetProperties(RefArg);
	void	GetStyles(void);
	void	GetValue(RefArg, RefArg);
	void	GetWriteableTextStylesArray(void);
	void	HandleCaret(unsigned long, long, Point&, Point&, Point&, Point&);
	void	HandleHilite(CUnit*, long, unsigned char);
	void	HandleLineGesture(long, Point&, Point&);
	int	HandleScrub(const Rect&, int, CUnit*, bool);
	void	HandleTap(Point&);
	void	HandleWord(const unsigned short*, unsigned long, const Rect&, const Point&, unsigned long, unsigned long, RefArg, unsigned char, long*, CUnit*);
	void	HiliteAll(void);
	void	HiliteClick(CStroke*);
	void	HiliteLines(CUnit*, unsigned char);
	void	HiliteParagraph(CUnit*, unsigned char);
	void	HiliteRange(CUnit*, unsigned char);
	void	HiliteText(long, long, unsigned char);
	void	HiliteWords(CUnit*, unsigned char);
	void	IconClick(TStrokePublic*);
	void	Idle(long);
	void	InsertVerticalSpace(Point&, long);
	void	IsCompletelyHilited(RefArg);
	void	LineFitsInBounds(long, long, long, StyleRecord*);
	void	NearTabStop(long);
	void	OffsetCachedBounds(Point&);
	void	OffsetInRunToBounds(long, LineInfo*, long, long, Rect*);
	void	OffsetToBounds(long, Rect*);
	void	OffsetToCaret(long, Rect*);
	void	PointOverHilitedText(Point&);
	void	PointOverText(Point&, Point*);
	void	PointToCaret(Point&, Rect*, Rect*);
	void	PointToOffset(const Point&, MarginSize, unsigned char, Rect*, LineInfo**, long*, unsigned char*);
	void	PointToWord(const Point&, long*, long*, MarginSize, LineInfo**, long*, unsigned char*);
	void	PointToWordBoundary(Point, MarginSize, long, LineInfo**, long*, unsigned char*);
	void	PostDraw(Rect&);
	void	PreviousLineNeedsCR(unsigned short*, unsigned short*);
	void	RealDoCommand(RefArg);
	void	RefillAllCaches(void);
	void	RemoveExcessWhiteSpace(InsertRun*);
	void	RemoveHilite(RefArg);
	void	RemoveText(unsigned long, unsigned long);
	void	ReplaceCharacter(const LineInfo*, const long, Finder*);
	void	SaveAddedUnitBounds(const Rect&, const Point&, unsigned long);
	void	ScrubCharacter(LineInfo*, long, const Rect&, long*);
	void	ScrubHilite(const Rect&);
	void	ScrubLines(const Rect&, CUnit*, unsigned char);
	void	ScrubWords(const Rect&, CUnit*, unsigned char);
	void	SetBounds(const Rect&);
	void	SetFinderBelowParagraph(Finder*);
	void	SetupArea(TParagraphHilite*);
	void	SimpleOffset(Point, long);
	void	Styles(void);
	void	Tabs(void);
	void	Text(void);
	void	UpdateCachedBounds(void);
	void	UpdateHiliteArea(void);
	void	WordFitsAtEndOfLine(const unsigned short*, unsigned long, unsigned long, Rect&);
	void	WordOnLastLine(const Rect&);
	void	WordOnLineBelowParagraph(const Rect&, const Point&);
	void	AddDragInfo(TDragInfo*);
	void	DragFeedback(const TDragInfo&, const Point&, unsigned char);
	void	DropMove(RefArg, const Point&, const Point&, unsigned char);
	void	DropRemove(RefArg);
	void	Drop(RefArg, RefArg, Point*);
	void	GetDropData(RefArg, RefArg);
	void	GetSupportedDropTypes(const Point&);
	void	HandleInkWord(RefArg, unsigned char);
	void	AddKeyToCurrUndo(unsigned short, long);
	void	GetRangeProperties(long, long);
	void	AddSpaceToEnd(long);
	void	AdjustBoundsForFirstBaseline(long);
	void	CaretRelativeToVisibleRect(const Rect&);
	void	ChangeStylesOfRange(long, long, RefArg, unsigned char);
	void	CheckAndDoJoin(Point&, Point&, Point&);
	void	CheckAndDoSplitInk(Point&, long);
	void	CheckStyles(void);
	void	DrawHilitedData(void);
	void	DropDone(void);
	void	FindClosestBaseline(short);
	void	FindLineContainingRect(const Rect&, const long, unsigned char, unsigned char);
	void	FindLineForWord(const Rect&, long);
	void	GetCachedRange(long*, long*);
	void	GetDefaultViewStyle(void);
	void	GetFirstBaseline(void);
	void	GetInkRefAndBounds(long, Rect*);
	void	GetLastBaseline(void);
	void	GetNextBaseline(TParagraphView*);
	void	GetStyleForInsertion(long, unsigned char, unsigned char);
	void	GetStylesOfRange(long, long, unsigned char);
	void	HandleInsertItems(RefArg);
	void	HandleReplaceText(RefArg);
	void	InsertHorizontalSpace(Point&, long, long, unsigned char);
	void	InsertInk(unsigned long, RefArg, unsigned long);
	void	InsertStyledText(unsigned long, const unsigned short*, unsigned long, RefArg, RefArg, unsigned long, unsigned long, unsigned char);
	void	MakeAndDoReplaceCommand(unsigned long, const unsigned short*, unsigned long, RefArg, RefArg, unsigned long, unsigned long, unsigned char);
	void	PlaceAndAddWord(const UniChar*, unsigned long, const Rect&, const Point&, unsigned long, unsigned long, RefArg, long*, CUnit*, long);
	void	ProcessStyles(unsigned char);
	void	ROMDeleteHilited(RefArg);
	void	RangeChanged(long, long, long, RefArg);
	void	SetSlot(RefArg, RefArg);
	void	SetupDone(void);
	void	SimpleOffset(Point);
	void	GetRequestedLineSpacing(void);
	void	RegisterIdler(void);
	void	FindFirstWordHitByHilite(Point*, long, Point, unsigned char);
	void	MakeHilite(long, long, unsigned char);
	void	OffsetPastVisible(void);
	void	GetSelection(void);
	void	SetSelection(RefArg, long*, long*);
	void	ActivateSelection(unsigned char);
	void	GetRangeText(long, long);
#endif
protected:
	long			f30;
	RefStruct	f64;
	RefStruct	f68;
// size +D0
};

#endif	/* __PARAGRAPHVIEW_H */
