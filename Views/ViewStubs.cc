/*
	File:		ViewStubs.cc

	Contains:	Implementation of views we have only stubbed so far.

	Written by:	Newton Research Group, 2010.
*/

#include "ViewStubs.h"


/*------------------------------------------------------------------------------
	C E d i t V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clEditView, CEditView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C M o n t h V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clMonthView, CMonthView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C P o l y g o n V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clPolygonView, CPolygonView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C M a t h E x p V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clMathExpView, CMathExpView, CView)	// actually CContainerView


#pragma mark -
/*------------------------------------------------------------------------------
	C M a t h O p V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clMathOpView, CMathOpView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C M a t h L i n e V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clMathLineView, CMathLineView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C R e m o t e V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clRemoteView, CRemoteView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C P r i n t V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clPrintView, CPrintView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C M e e t i n g V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clMeetingView, CMeetingView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C S l i d e r V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clSliderView, CSliderView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C L i s t V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clListView, CListView, CEditView)


#pragma mark -
/*------------------------------------------------------------------------------
	C C l i p b o a r d V i e w
------------------------------------------------------------------------------*/
#include "Clipboard.h"

VIEW_SOURCE_MACRO(clClipboard, CClipboardView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C O u t l i n e V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clOutline, COutlineView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C H e l p O u t l i n e V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clHelpOutline, CHelpOutlineView, CView)


#pragma mark -
/*------------------------------------------------------------------------------
	C T X V i e w
------------------------------------------------------------------------------*/

VIEW_SOURCE_MACRO(clTXView, CTXView, CView)

