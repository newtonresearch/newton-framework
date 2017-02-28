/*
	File:		PickView.h

	Contains:	CPickView declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__PICKVIEW_H)
#define __PICKVIEW_H 1

#include "View.h"
#include "NewtonTime.h"

struct GridInfo
{
	int	outerFrameWd;
	int	cellFrameWd;
	int	numOfRows;
	int	numOfColumns;
	int	cellWd;
	int	cellHt;
};


struct PickStuff
{
	ArrayIndex index;
	bool	isGrid;
	int	gridX;
	int	gridY;
//size+10
};

struct ItemInfo
{
	int	length:12;
	int	huh:3;
	int	isPickable:1;
	int	markCharacter:16;
//size+04
};

/*------------------------------------------------------------------------------
	C P i c k V i e w
------------------------------------------------------------------------------*/

class CPickView : public CView
{
public:
	VIEW_HEADER_MACRO
								~CPickView();

	virtual  void			init(RefArg inContext, CView * inView);

	virtual  bool			realDoCommand(RefArg inCmd);

	virtual	void			realDraw(Rect& inRect);

// View manipulation
	virtual	void		hide(void);

	void		flashItem(PickStuff * inPick);
	int		getDisplayFixedHeight(RefArg);
	Ref		getDisplayIcon(RefArg);
	int		getDisplayIndent(RefArg);
	GridInfo *	getGridInfo(RefArg, Rect*);
	void		getGridItemRect(PickStuff * inPick, Rect*);
	void		setItemFlags(PickStuff * inPick, bool, UniChar);
	void		getItemFlags(PickStuff * inPick, bool*, UniChar*);
	void		setItemLength(PickStuff * inPick, int inLength);
	int		getItemLength(ArrayIndex index);
	void		getItemRect(PickStuff * inPick, Rect * outRect);
	void		invertItem(PickStuff * inPick);
	CView *	pickItem(PickStuff * inPick);
	void		subItem(Point&, PickStuff * inPick);
	void		trackStroke(CStroke*, PickStuff * inPick);
	void		item(Point&, PickStuff * inPick);
	void		pickableItem(Point&, PickStuff * inPick);
	Ref		getDisplayItem(ArrayIndex index, bool*, UniChar*);
	Ref		getItemNoText(ArrayIndex index);
	void		keyToNextItem(ArrayIndex index);
	void		keyToPrevItem(ArrayIndex index);

	bool		handleKeyDown(UniChar, ULong);
	void		getKeyCommandInfo(void);
	Ref		getKeyCommand(ArrayIndex index);
	int		getKeyCommandModifierWidth(ArrayIndex index);

	Ref		getOverflows(void);
	void		scroll(RefArg, bool);

	bool		isItemNoPickable(ArrayIndex index);

// Scripts
	virtual	void		setupForm(void);

private:
	bool			fAutoClose;	//+30
	bool			fHasMark;	//+31
	RefStruct	fItems;		//+34
	int *			fItemBottoms;	//+38 was short*
	ItemInfo *	fItemInfo;	//+3C
	GridInfo **	fItemGrid;	//+40
	FontInfo		f44;
	StyleRecord	fTextStyle;	//+54
	RefStruct	f70;	// ?
	PickStuff	fPicked;		//+74
	int			fPickTextItemHeight;	//+84
	ArrayIndex	fNumOfItems;	//+88
	int			fLeftMargin;	//+8C
	int			fMarkIndent;	//+90
	int			fRightMargin;	//+98
	int			fTopMargin;		//+9C
	int			fBottomMargin;	//+A0
	RefStruct	fCommandKeys;	//+A4
	int			fCommandKeySpace;	//+A8 was short
	RefStruct	fTypeSelectStr;		//+AC
	CTime			fLastKeyDownTime;		//+B0
	Timeout		fTypeSelectTimeout;	//+B4
	bool			fIsPicked;	//+B8
// size +BC
};

#endif	/* __PICKVIEW_H */
