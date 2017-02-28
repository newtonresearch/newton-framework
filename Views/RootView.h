/*
	File:		RootView.h

	Contains:	CRootView declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__ROOTVIEW_H)
#define __ROOTVIEW_H 1

#include "View.h"
#include "ErrorNotify.h"
#include "NewtonTime.h"

/*------------------------------------------------------------------------------
	C V i e w S t u f f
------------------------------------------------------------------------------*/
#define kNumOfLayers 3

class CViewStuff
{
public:
			CViewStuff() : fView(NULL) { }

	CView *	fView;		// +00
	CRegionStruct	fInvalid;		// +04
};


/*------------------------------------------------------------------------------
	I d l i n g I t e m
------------------------------------------------------------------------------*/

struct IdlingView
{
	IdlingView *	fNext;		// +00
	CView *			fView;		// +04
};


struct IdleViewInfo
{
	CView *	fView;	// +00
	ULong		fId;		// +04
	CTime		fTime;	// +08
};


#define kFarFuture 0x7FFFFFFF00000000


/*------------------------------------------------------------------------------
	C R o o t V i e w
------------------------------------------------------------------------------*/
class CClipboardView;

class CRootView : public CView
{
public:
	VIEW_HEADER_MACRO

						CRootView();
	virtual			~CRootView();

//	Construction
	virtual	void	init(RefArg inContext, CView * inView);

	virtual  bool	realDoCommand(RefArg inCmd);

//	Idling
	IdlingView *	getIdlingView(CView * inView);
	void			addIdler(CView * inView, Timeout inTimeout, ULong inId);
	ULong			removeIdler(CView * inView, ULong inId);
	void			removeAllIdlers(CView * inView);
	void			unlinkIdleView(CView * inView);
	CTime			idleViews(void);

//	View manipulation
	virtual void	dirty(const Rect * inRect = NULL);
	void			invalidate(const CBaseRegion& inRgn, CView * inView);
	void			validate(const CBaseRegion& inRgn);
	void			smartInvalidate(const Rect * inRect);
	void			smartScreenDirty(const Rect * inRect);
	bool			needsUpdate(void);
	void			update(Rect * inRect = NULL);

	virtual void	removeAllViews(void);
	void			forgetAboutView(CView * inView);
	CView *		getCommonParent(CView * inView1, CView * inView2);
	CView *		getFrontmostModalView(void);
	CView *		getPopup(void) const;
	void			setPopup(CView * inView, bool inShow);
	bool			getRemoteWriting(void);

//	Hiliting
	void			hiliter(CUnit * inUnit, CView * inView);
	CView *		hilitedView(void) const;
	void			setHilitedView(CView * inView);
	bool			setPreserveHilites(bool inDoIt);

//	Input
	void			showCaret(void);
	void			hideCaret(void);
	void			dirtyCaret(void);
	void			drawCaret(Point inPt);
	bool			caretEnabled(void);
	bool			caretValid(Point * outPt);
	void			checkForCaretRemoval(void);
	bool			doCaretClick(CUnit * inUnit);
	Point			getCaretPoint(void);		// originally void getCaretPoint(Point * outPt)
	Rect			getCaretRect(void);		// originally void getCaretRect(Rect * outRect)
	void			restoreBitsUnderCaret(void);
	CView *		caretView(void) const;
	void			caretViewGone(void);

//	Keyboard	
	void			registerKeyboard(RefArg, ULong);
	bool			unregisterKeyboard(RefArg);
	ArrayIndex	getKeyboardIndex(RefArg);
	void			connectPassthruKeyboard(bool inDoIt);
	bool			keyboardActive(void);
	bool			keyboardConnected(void);
	bool			commandKeyboardConnected(void);
	CView *		defaultButtonView(void) const;
	CView *		caretSlipView(void) const;
	void			findDefaultButtonAndCaretSlip(CView * inView, CView ** outDefaultButton, CView ** outCaretSlip);
	void			updateDefaultButtonAndCaretSlip(void);
	Ref			handleKeyIn(ULong inKeycode, bool inisDown, CView * inView);
	void			setKeyView(CView * inView, int inX, int inY, bool);
	void			setKeyViewSelection(CView * inView, RefArg inInfo, bool);
	void			commonSetKeyView(CView * inView, int inX, int inY);
	bool			restoreKeyView(CView * inView);
	CView *		findRestorableKeyView(CView * inView, ULong *);
	void			holdPendingKeyView(RefArg, RefArg);
	void			activatePendingKeyView(void);

//	Selection
	Ref			getSelectionStack(void);
	void			cleanSelectionStack(CView * inView, bool inPrune);
	void			pushSelection(CView * inView, RefArg inInfo);
	Ref			popSelection(void);

//	Drawing
	void			realDraw(const Rect * inRect);
	void			postDraw(Rect& inRect);

//	Clipboard
	void			addClipboard(RefArg, RefArg);
	void			removeClipboard(void);
	CView *		getClipboard(void);
	CView *		getClipboard(CView * inIconView);
	CView *		getClipboardIcon(void);
	CView *		getClipboardIcon(CClipboardView * inClip);
	Ref			getClipboardIcons(void);

	void			setParsingUtterance(bool inDoingIt);

private:
	CView *			fHilitedView;		// +30	view containing the hilite
	CViewStuff *	fLayer;				// +34	array of 3
	Rect				fInkyRect;			// +38	rect dirtied by ink
	CDynamicArray *	fIdlingViews;	// +40
	ArrayIndex		x44;
	ArrayIndex		x48;
	IdlingView *	fIdlingList;		// +4C
	CView *			fPopup;				// +50	popup view
	RefStruct		fClipboardIcons;	// +54	array of clipboard icons
	RefStruct		fClipboardViews;	// +58	array of clipboards
	bool				fIsParsingUtterance;	// +5C
	RefStruct		fKeyboards;			// +60	array of registered keyboards
	bool				fIsPassthruKeyboardConnected;	// +64
	CView *			fCaretView;			// +68	view containing the caret
	int				x6C;					// +6C
	CView *			x70;					// just guessing it’s a CView
	CView *			fDefaultButton;	// +74
	CView *			fCaretSlip;			// +78
	RefStruct		fSelectionStack;	// +7C
	bool				fPreserveHilites;	// +80
	CGImageRef		fCaret;				// +84	caret pixmap -- was CBits*
	bool				fIsCaretUp;			// +88
	Point				fCaretPt;			// +8C
	CView *			x90;
	int				fHideCaretCount;	// +94
	RefStruct		fPendingKeyView;	// +98
	RefStruct		fPendingKeyViewSelection;	// 9C
//	size +A0
};

extern CRootView * gRootView;

inline CView * CRootView::hilitedView(void) const  { return fHilitedView; }
inline CView * CRootView::caretView(void) const  { return fCaretView; }
inline CView * CRootView::defaultButtonView(void) const  { return fDefaultButton; }
inline CView * CRootView::caretSlipView(void) const  { return fCaretSlip; }
inline CView * CRootView::getPopup(void) const  { return fPopup; }

inline void CRootView::setParsingUtterance(bool inDoingIt)  { fIsParsingUtterance = inDoingIt; }


// Defined in the ROM, although not inlined and apparently never used.
inline CRootView * GetNewtRootView(void)	{ return gRootView; }


#endif	/* __ROOTVIEW_H */
