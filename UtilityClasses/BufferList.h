/*
	File:		BufferList.h

	Contains:	Interface to the CBufferList class.

	Copyright:	© 1992-1995 by Apple Computer, Inc.  All rights reserved.
*/

#ifndef __BUFFERLIST_H
#define __BUFFERLIST_H

#ifndef __NEWTON_H
#include "Newton.h"
#endif

#ifndef __BUFFER_H
#include "Buffer.h"
#endif

#include "List.h"
#include "ListIterator.h"

/*----------------------------------------------------------------------
	C B u f f e r L i s t
----------------------------------------------------------------------*/

class CBufferList : public CBuffer
{
public:
// in the original:
// the constructor and destructor methods are intentionally declared private
// to prevent external code from knowing the size of the object
					CBufferList();
	virtual		~CBufferList();

	NewtonErr	init(bool deleteSegments = true);
	NewtonErr	init(CList * bufList, bool deleteSegments = true);

	// Buffer methods

	virtual void		reset(void);
	void					resetMark(void);

	virtual int			peek(void);
	virtual int			next(void);
	virtual int			skip(void);
	virtual int			get(void);
	virtual size_t		getn(UByte * p, size_t n);
	virtual int			copyOut(UByte * p, size_t& n);

	virtual int			put(int dataByte);
	virtual size_t		putn(const UByte * p, size_t n);
	virtual int			copyIn(const UByte * p, size_t& n);

	virtual size_t		getSize(void);
	virtual bool		atEOF(void);

	virtual long		hide(long inCount, int inDirection);
	virtual long		seek(long inOffset, int inDirection);
	virtual long		position(void);

	// List methods

	CBuffer *	at(ArrayIndex index);
	CBuffer *	first(void);
	CBuffer *	last(void);

	NewtonErr	insert(CBuffer * item);
	NewtonErr	insertBefore(ArrayIndex index, CBuffer * item);
	NewtonErr	insertAt(ArrayIndex index, CBuffer * item);
	NewtonErr	insertFirst(CBuffer * item);
	NewtonErr	insertLast(CBuffer * item);

	NewtonErr	remove(CBuffer * item);
	NewtonErr	removeAt(ArrayIndex index);
	NewtonErr	removeFirst(void);
	NewtonErr	removeLast(void);
	NewtonErr	removeAll(void);

	NewtonErr	deleteItem(CBuffer * item);	// was Delete
	NewtonErr	deleteAt(ArrayIndex index);
	NewtonErr	deleteFirst(void);
	NewtonErr	deleteLast(void);
	NewtonErr	deleteAll(void);

	ArrayIndex	getIndex(CBuffer * item);

private:
	void			selectSegment(ArrayIndex index);
	bool			nextSegment(void);

	CBuffer *	fBuffer;				// +04
	CList *		fList;				// +08
	CListIterator *	fIter;		// +0C
	ArrayIndex	fLoBound;			// +10
	ArrayIndex	fCurrentIndex;		// +14
	ArrayIndex	fHiBound;			// +18
	bool			fBuffersAreOurs;	// +1C
	bool			fListIsOurs;		// +1D
// size +20
};


#endif //__BUFFERLIST_H
