/*
	File:		ListLoop.h

	Contains:	Interface to the CListLoop class.

	Written by:	Newton Research Group.
*/

#if !defined(__LISTLOOP_H)
#define __LISTLOOP_H 1

#if !defined(__LIST_H)
#include "List.h"
#endif


/*------------------------------------------------------------------------------
	C L i s t L o o p
------------------------------------------------------------------------------*/

class CListLoop
{
public:
	CListLoop(CList * inList);

	void		reset(void);
	void *	current(void);
	void *	next(void);
	long		index(void);
	void		removeCurrent(void);

protected:
	CList *	fList;
	long		fIndex;
	long		fSize;
};

inline long CListLoop::index(void)
{ return fIndex; }


/*------------------------------------------------------------------------------
	C B a c k w a r d L o o p
------------------------------------------------------------------------------*/

class CBackwardLoop
{
public:
	CBackwardLoop(CList * inList);

	void *	current(void);
	void *	next(void);
	long		index(void);

protected:
	CList *	fList;
	long		fIndex;
};

inline long CBackwardLoop::index(void)
{ return fIndex; }


#endif	/*	__LISTLOOP_H	*/
