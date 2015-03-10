/*
	File:		FlashTracker.h

	Contains:	Flash store declarations.
	
	Written by:	Newton Research Group, 2010.
*/

#if !defined(__FLASHTRACKER_H)
#define __FLASHTRACKER_H 1

#include "PSSTypes.h"

/*------------------------------------------------------------------------------
	C F l a s h T r a c k e r
	Okay, so it tracks the addition and removal of PSSIds -- but to what end?
------------------------------------------------------------------------------*/

class CFlashTracker
{
public:
					CFlashTracker(ArrayIndex inSize);
					~CFlashTracker();

	void			add(PSSId inId);
	void			remove(PSSId inId);
	bool			isFull(void) const;

	void			lock(void);
	void			unlock(void);
	bool			isLocked(void) const;

	void			reset(void);

private:
	friend class CFlashIterator;

	ArrayIndex	fSize;
	ArrayIndex	fIndex;
	PSSId *		fList;
	bool			fIsFull;
	ArrayIndex	fLockCount;
};

inline bool			CFlashTracker::isFull(void) const	{ return fIsFull; }
inline bool			CFlashTracker::isLocked(void) const	{ return fLockCount != 0; }


#endif	/* __FLASHTRACKER_H */
