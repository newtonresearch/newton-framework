/*
	File:		DataView.cc

	Contains:	CDataView implementation.

	Written by:	Newton Research Group, 2014.
*/

#include "DataView.h"

/*------------------------------------------------------------------------------
	C D a t a  V i e w
------------------------------------------------------------------------------*/
VIEW_SOURCE_MACRO(clDataView, CDataView, CView)


CView *
CDataView::getEnclosingEditView(void)
{ return NULL; }

void
CDataView::diceHilited(RefArg, CEditView*, Point&, unsigned char)
{}

void
CDataView::addHilited(RefArg, CEditView*)
{}

CView *
CDataView::getHiliteView(void)
{ return NULL; }

CView *
CDataView::getTextView(void)
{ return NULL; }

void
CDataView::hiliteText(long, long, unsigned char)
{}

void
CDataView::pointOverHilitedText(Point&)
{}

void
CDataView::pointOverText(Point&, Point*)
{}

void
CDataView::handleWord(const unsigned short*, unsigned long, const Rect&, const Point&, unsigned long, unsigned long, RefArg, unsigned char, long*, CUnit*)
{}

void
CDataView::handleCaret(unsigned long, long, Point&, Point&, Point&, Point&)
{}

void
CDataView::handleLineGesture(long, Point&, Point&)
{}

void
CDataView::handleInkWord(RefArg, unsigned char)
{}

void
CDataView::handleInk(RefArg, unsigned char)
{}

void
CDataView::saveAddedUnitBounds(const Rect&, const Point&, unsigned long)
{}

void
CDataView::getContext(void)
{}

void
CDataView::getProperties(RefArg)
{}

void
CDataView::cleanupData(void)
{}

void
CDataView::handleTap(Point&)
{}

