/*
	File:		DataView.h

	Contains:	CDataView declarations.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__DATAVIEW_H)
#define __DATAVIEW_H 1

#include "View.h"

/*------------------------------------------------------------------------------
	C D a t a  V i e w
------------------------------------------------------------------------------*/
class CEditView;

class CDataView : public CView
{
public:
	VIEW_HEADER_MACRO

	CView *	getEnclosingEditView(void);

	void		diceHilited(RefArg, CEditView*, Point&, unsigned char);
	void		addHilited(RefArg, CEditView*);
	CView *	getHiliteView(void);
	CView *	getTextView(void);
	void		hiliteText(long, long, unsigned char);
	void		pointOverHilitedText(Point&);
	void		pointOverText(Point&, Point*);
//	virtual	void		drawHilitedData(void);

	void		handleWord(const unsigned short*, unsigned long, const Rect&, const Point&, unsigned long, unsigned long, RefArg, unsigned char, long*, CUnit*);
	void		handleCaret(unsigned long, long, Point&, Point&, Point&, Point&);
	void		handleLineGesture(long, Point&, Point&);
	void		handleInkWord(RefArg, unsigned char);
	void		handleInk(RefArg, unsigned char);

	void		saveAddedUnitBounds(const Rect&, const Point&, unsigned long);
	void		getContext(void);
	void		getProperties(RefArg);
	void		cleanupData(void);

	void		handleTap(Point&);

};

#endif	/* __DATAVIEW_H */
