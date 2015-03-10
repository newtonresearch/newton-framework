/*
	File:		Clipboard.h

	Contains:	Declaration of clipboard view.

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__CLIPBOARD_H)
#define __CLIPBOARD_H 1

#include "View.h"

/*------------------------------------------------------------------------------
	C C l i p b o a r d V i e w
------------------------------------------------------------------------------*/

class CClipboardView : public CView
{
public:
	VIEW_HEADER_MACRO

private:
	RefStruct	f30;
	RefStruct	f34;
	RefStruct	f40;
// size +44
};

#endif	/* __CLIPBOARD_H */
