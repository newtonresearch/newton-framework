/*
	File:		ListIterator.h

	Contains:	Interface to the CListIterator class.

	Written by:	Newton Research Group.
*/

#if !defined(__LISTITERATOR_H)
#define __LISTITERATOR_H 1

#if !defined(__ARRAYITERATOR_H)
#include "ArrayIterator.h"
#endif


/*--------------------------------------------------------------------------------
	C L i s t I t e r a t o r
--------------------------------------------------------------------------------*/

class CListIterator : public CArrayIterator
{
public:
				CListIterator();
				CListIterator(CDynamicArray * inList);
				CListIterator(CDynamicArray * inList, bool inForward);
				CListIterator(CDynamicArray * inList, ArrayIndex inLowBound, ArrayIndex inHighBound, bool inForward);

	void *	firstItem(void);
	void *	currentItem(void);
	void *	nextItem(void);
};


#endif	/*	__LISTITERATOR_H	*/
