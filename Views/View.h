/*
	File:		View.h

	Contains:	CView declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__VIEW_H)
#define __VIEW_H 1

#include "Quartz.h"
#include "Regions.h"

#include "Responder.h"
#include "NewtGlobals.h"
#include "List.h"
#include "DragAndDrop.h"

#include "TextStyles.h"
#include "ViewFlags.h"

class CUnit;
class CStroke;


/*------------------------------------------------------------------------------
	C H i l i t e
------------------------------------------------------------------------------*/

class CHilite
{
public:
								CHilite();
	virtual					~CHilite();

	virtual	CHilite *	clone(void);
				void			copyFrom(CHilite * inHilite);

	virtual	void			updateBounds(void);
	virtual	bool			overlaps(const Rect * inRect);
	virtual	bool			encloses(const Point inPt);
//	virtual	CRegion *	area(void);

	Rect	fBounds;		// +04
};

class CHiliteLoop
{
public:
};


/*------------------------------------------------------------------------------
	C V i e w L i s t
------------------------------------------------------------------------------*/

class CViewList : public CList
{
};


/*------------------------------------------------------------------------------
	C V i e w
------------------------------------------------------------------------------*/

class CView : public CResponder
{
public:
	VIEW_HEADER_MACRO

							CView();
	virtual				~CView();

// Responder protocol
	virtual  bool		doCommand(RefArg inCmd);

//	Construction
	virtual	void		init(RefArg inContext, CView * inView);	// was Constructor
	virtual	void		dispose(void);										// was Delete
				Ref		buildContext(RefArg inProto, bool inArg2);

	virtual  bool		realDoCommand(RefArg inCmd);

				CView *	findId(int inId);
	virtual	int		idle(int inArg);		// return ms until next idle
				bool		printing(void);

// Accessors
				bool		protoedFrom(RefArg inContext);
				Ref		getProto(RefArg inTag) const;
				Ref		getCacheProto(ArrayIndex index);
				Ref		getCacheVariable(ArrayIndex index);
				bool		invalidateSlotCache(ArrayIndex index);
				Ref		getVar(RefArg inTag);
	virtual	Ref		getValue(RefArg inTag, RefArg inType);
	virtual	void		setSlot(RefArg inTag, RefArg inValue);	// was setValue
				void		setContextSlot(RefArg inTag, RefArg inValue);
				void		setDataSlot(RefArg inTag, RefArg inValue);
				Ref		dataFrame(void);
				CView *	getWindowView(void);
				Ref		getWriteableVariable(RefArg inTag);
				Ref		getWriteableProtoVariable(RefArg inTag);
				ULong		copyProtection(void);
				void		transferCopyProtection(RefArg inFrom);

// View manipulation
				void		setFlags(ULong inBits);
				void		clearFlags(ULong inBits);
				void		setOrigin(Point inOrigin);
				bool		setCustomPattern(RefArg inName);
				Point		localOrigin(void);
				Point		contentsOrigin(void);
				Point		childOrigin(void);
				CRegion	setupVisRgn(void);
				bool		hasVisRgn(void);
				bool		visibleDeep(void);
				CRegion	getFrontMask(void);
				CClipper * clipper(void);
	virtual	void		dirty(const Rect * inBounds = NULL);
				void		show(void);
	virtual	void		hide(void);

	virtual	void		setBounds(const Rect * inBounds);
				void		justifyBounds(Rect * ioBounds);
				void		dejustifyBounds(Rect * ioBounds);
	virtual	void		outerBounds(Rect * outBounds);
				void		writeBounds(const Rect * inBounds);
				void		recalcBounds(void);
	virtual	void		scale(const Rect * inFromRect, const Rect * inToRect);
				void		move(const Point * inDelta);
				void		doMoveCommand(Point inDelta);
				void		offset(Point inDelta);
	virtual	void		simpleOffset(Point inDelta, bool	inAlways);
	virtual	bool		insideView(Point inPt);
				void		sync(void);
				Ref		syncScroll(RefArg inWhat, RefArg index, RefArg inupDown);
				Ref		syncScrollSoup(RefArg inWhat, RefArg inupDown);
				CView *	addToSoup(RefArg inSoup);
				CView *	addChild(RefArg inChild);
				CView *  addView(RefArg inView);
				void		addCView(CView * inView);
				void		addViews(bool inArg1);
				void		removeView(void);
	virtual	void		removeAllViews(void);
				void		removeChildView(CView * inView);
				void		removeFromSoup(CView * inView);
				void		removeUnmarked(void);
				void		reorderView(CView * inView, int index);
				void		bringToFront(void);
				CView *	frontMost(void);
				CView *	frontMostApp(void);

				Ref		children(void);
				int		childrenHeight(ArrayIndex * outNumOfChildren);
				Ref		childViewFrames(void);
				void		childViewMoved(CView * inView, Point newOrigin);
	virtual	void		childBoundsChanged(CView * inView, Rect * inBounds);
				void		moveChildBehind(CView * inView, CView * inBehindView);
				int		setChildrenVertical(int inArg1, int inArg2);

				CView *  findView(RefArg inView);
				CView *  findView(Point inPt, ULong inRecognitionReqd, Point * inSlop);
				CView *  findClosestView(Point inPt, ULong inRecognitionReqd, int * outDistance, Point * inSlop, bool * outIsClickable);
				int		distance(Point inPt1, Point * ioPt2);
	virtual	void		narrowVisByIntersectingObscuringSiblingsAndUncles(CView * inView, Rect * inBounds);
				void		viewVisibleChanged(CView * inView, bool inVisible);

	virtual	Ref		getRangeText(long inStart, long inEnd);
				bool		isGridded(RefArg inGrid, Point * outSpacing);
	virtual	void		changed(RefArg inArg1);
	virtual	void		changed(RefArg inArg1, RefArg inContext);

// Hiliting
				bool		addHiliter(CUnit * unit);
	virtual	void		handleHilite(CUnit * unit, long inArg2, bool inArg3);
	virtual	void		globalHiliteBounds(Rect * inBounds);
	virtual	void		globalHilitePinnedBounds(Rect * inBounds);
	virtual	void		globalHiliteResizeBounds(Rect * inBounds);
	virtual	void		hilite(bool isOn);
	virtual	void		hiliteAll(void);
				Ref		hilites(void);
				Ref		firstHilite(void);
	virtual	bool		hilited(void);
	virtual	void		deleteHilited(RefArg);
	virtual	void		removeHilite(RefArg inArg1);
	virtual	void		removeAllHilites(void);
	virtual	bool		isCompletelyHilited(RefArg inArg1);
	virtual	bool		pointInHilite(Point inPt);
	virtual	void		drawHiliting(void);
	virtual	void		drawHilites(bool inArg1);
	virtual	void		drawHilitedData(void);

// Text
				Ref		getTextStyle(void);
				void		getTextStyleRecord(StyleRecord * outRec);

// Gestures
	virtual	ULong		clickOptions(void);
	virtual	int		handleScrub(const Rect * inScrubbedRect, int inArg2, CUnit * unit, bool inArg4);

// Input
	virtual	void		setCaretOffset(int * ioX, int * ioY);
	virtual	void		offsetToCaret(int inArg1, Rect * outArg2);
	virtual	void		pointToCaret(Point inPt, Rect * inArg2, Rect * inArg3);
	virtual	ULong		textFlags(void) const;

// Keyboard
	virtual	void		buildKeyChildList(CViewList * inArg1, long inArg2, long inArg3);
				CView *  nextKeyView(CView * inView, long inArg2, long inArg3);
				void		handleKeyEvent(RefArg inArg1, ULong inArg2, bool * inArg3);
	virtual	void		doEditCommand(long inArg);

// View selection
				void		select(bool isOn, bool isUnique);
				void		selectNone(void);

// Content selection
	virtual	void		setSelection(RefArg inInfo, int * ioX, int * ioY);
	virtual	Ref		getSelection(void);
	virtual	void		activateSelection(bool isOn);

// Effects
				void		soundEffect(RefArg inArg);

// Drawing
				void		update(CBaseRegion& inRgn, CView * inView);
				void		draw(Rect& inRect, bool inForPrint=false);
				void		draw(CBaseRegion& inRgn, bool inForPrint=false);
				void		drawChildren(Rect& inRect, CView * inView);
				void		drawChildren(CBaseRegion& inRgn, CView * inView);
	virtual	void		preDraw(Rect& inRect);
	virtual	void		realDraw(Rect& inRect);
	virtual	void		postDraw(Rect& inRect);
	virtual	void		drawScaledData(const Rect * inSrcBounds, const Rect * inDstBounds, Rect * ioBounds);

// Drag and drop
	virtual	bool		addDragInfo(CDragInfo * info);
	virtual	int		dragAndDrop(CStroke * stroke, const Rect * inBounds, const Rect * inArg3, const Rect * inArg4, bool inArg5, const CDragInfo * info, const Rect * inArg7);
				void		drag(CStroke * stroke, const Rect * inBounds);
				int		drag(const CDragInfo * info, CStroke * stroke, const Rect * inArg3, const Rect * inArg4, const Rect* inArg5, bool inArg6, Point * inArg7, Point * inArg8, bool * inArg9, bool * inArg10);
				void		alignDragPtToGrid(const CDragInfo * inArg1, Point * inArg2);
	virtual	bool		dragFeedback(const CDragInfo * inDragInfo, const Point & inPt, bool inShow);
	virtual	void		endDrag(const CDragInfo * inArg1, CView * inArg2, const Point * inArg3, const Point * inArg4, const Point * inArg5, bool inArg6);
	virtual	bool		drawDragBackground(const Rect * inBounds, bool inCopy);
	virtual	void		drawDragData(const Rect * inBounds);
				bool		dropApprove(CView * inTarget);
	virtual	Ref		getSupportedDropTypes(Point inPt);
	virtual	void		getClipboardDataBits(Rect * inArea);
	virtual	CView *	findDropView(const CDragInfo & inDragInfo, Point inPt);
	virtual	bool		acceptDrop(const CDragInfo & inDragInfo, Point inPt);
				CView *	targetDrop(const CDragInfo & inDragInfo, Point inPt);
	virtual	Ref		getDropData(RefArg inDragType, RefArg inDragRef);
	virtual	bool		drop(RefArg inDropType, RefArg inDropData, Point inDropPt);
	virtual	bool		dropMove(RefArg inDragRef, Point inOffset, Point inLastDragPt, bool inCopy);
	virtual	void		dropRemove(RefArg inDragRef);
	virtual	bool		dropDone(void);

// Scripts
	virtual	void		setupForm(void);
	virtual	void		setupDone(void);
				Ref		runScript(RefArg tag, RefArg inArgs, bool inSelf = false, bool * outDone = NULL);
				Ref		runCacheScript(ArrayIndex index, RefArg inArgs, bool inSelf = false, bool * outDone = NULL);

// Debug
				void		dump(ArrayIndex inLevel);

//protected:
	int				fId;					// +04
	Rect				viewBounds;			// +10
	ULong				viewFlags;			// +08
	ULong				viewFormat;			// +0C
	ULong				viewJustify;		// +28
	CViewList *		viewChildren;		// +20
	CView *			fParent;				// +1C
	ULong				fTagCacheMaskHi;	// +2C
	ULong				fTagCacheMaskLo;	// +18
	RefStruct		fContext;			// +24 NS object corresponding to this C view
};


/*------------------------------------------------------------------------------
	T a g   C a c h e   I n d i c e s
------------------------------------------------------------------------------*/

enum
{
	kIndexViewQuitScript,
	kIndexStyles,
	kIndexTabs,
	kIndexRealData,
	kIndexText,
	kIndexViewTransferMode,
	kIndexViewFont,
	kIndexViewOriginY,
	kIndexViewOriginX,
	kIndexViewJustify,
	kIndexViewFlags,
	kIndexViewDrawScript,
	kIndexViewIdleScript,
	kIndexViewSetupChildrenScript,
	kIndexViewChildren,
	kIndexButtonClickScript,
	kIndexViewClickScript,
	kIndexViewFormat,
	kIndexViewBounds,
	kIndexViewShowScript,
	kIndexViewStrokeScript,
	kIndexViewGestureScript,
	kIndexViewHiliteScript,
	kIndexAllocateContext,
	kIndexKeyPressScript,
	kIndexViewKeyUpScript,
	kIndexViewKeyDownScript,
	kIndexViewKeyRepeatScript,
	kIndexStepAllocateContext,
	kIndexViewChangedScript,
	kIndexViewRawInkScript,
	kIndexViewInkWordScript,
	kIndexViewKeyStringScript,
	kIndexViewCaretActivateScript,
	kIndexView
};


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

CView *	GetView(RefArg inContext);
CView *	GetView(RefArg inContext, RefArg inView);
CView *	GetKeyReceiver(RefArg inContext, RefArg inView);

CView *	FailGetView(RefArg inContext);


/*------------------------------------------------------------------------------
	G l o b a l   D a t a
------------------------------------------------------------------------------*/

// when finding the distance between views/strokes
#define kDistanceOutOfRange 0x00010000

extern int		gSlowMotion;		// delay after drawing in milliseconds


#endif	/* __VIEW_H */
