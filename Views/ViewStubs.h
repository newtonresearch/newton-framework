/*
	File:		ViewStubs.h

	Contains:	Declarations of views we have only stubbed so far.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__VIEWSTUBS_H)
#define __VIEWSTUBS_H 1

#include "View.h"

/*------------------------------------------------------------------------------
	C E d i t V i e w
------------------------------------------------------------------------------*/

class CEditView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
// size +50
};


/*------------------------------------------------------------------------------
	C M o n t h V i e w
------------------------------------------------------------------------------*/
#include "Dates.h"

class CMonthView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
	CDate			f60;
	RefStruct	f88;
	RefStruct	f8C;
// size +94
};


/*------------------------------------------------------------------------------
	C P o l y g o n V i e w
------------------------------------------------------------------------------*/

class CPolygonView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
// size +30
};


/*------------------------------------------------------------------------------
	C M a t h E x p V i e w
------------------------------------------------------------------------------*/

class CMathExpView : public CView	// actually CContainerView
{
public:
	VIEW_HEADER_MACRO

private:
	RefStruct	f38;
// size +44
};


/*------------------------------------------------------------------------------
	C M a t h O p V i e w
------------------------------------------------------------------------------*/

class CMathOpView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
	long		fValue;	// +30
// size +34
};


/*------------------------------------------------------------------------------
	C M a t h L i n e V i e w
------------------------------------------------------------------------------*/

class CMathLineView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
// size +38
};


/*------------------------------------------------------------------------------
	C R e m o t e V i e w
------------------------------------------------------------------------------*/

class CRemoteView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
// size +58
};


/*------------------------------------------------------------------------------
	C P i c k V i e w
------------------------------------------------------------------------------*/

class CPickView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
	RefStruct	f34;
	RefStruct	f54;
	RefStruct	f70;
	RefStruct	fAC;
// size +BC
};


/*------------------------------------------------------------------------------
	C P r i n t V i e w
------------------------------------------------------------------------------*/

class CPrintView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
// size +48
};


/*------------------------------------------------------------------------------
	C M e e t i n g V i e w
------------------------------------------------------------------------------*/

class CMeetingView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
// size +3C
};


/*------------------------------------------------------------------------------
	C S l i d e r V i e w
------------------------------------------------------------------------------*/

class CSliderView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
// size +4C
};


/*------------------------------------------------------------------------------
	C L i s t V i e w
------------------------------------------------------------------------------*/

class CListView : public CEditView
{
public:
	VIEW_HEADER_MACRO

private:
// size +5C
};


/*------------------------------------------------------------------------------
	C O u t l i n e V i e w
------------------------------------------------------------------------------*/

class COutlineView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
	RefStruct	f30;
	RefStruct	f60;
	RefStruct	f64;
// size +68
};


/*------------------------------------------------------------------------------
	C H e l p O u t l i n e V i e w
------------------------------------------------------------------------------*/

class CHelpOutlineView : public COutlineView
{
public:
	VIEW_HEADER_MACRO

private:
// size +68
};


/*------------------------------------------------------------------------------
	C T X V i e w
------------------------------------------------------------------------------*/

class CTXView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
	RefStruct	f54;
	RefStruct	f58;
// size +60
};


#endif	/* __VIEWSTUBS_H */
