/*
	File:		StoreDriver.h

	Contains:	Store driver declarations..

	Written by:	Newton Research Group, 2010.
*/

#if !defined(__STOREDRIVER_H)
#define __STOREDRIVER_H 1

#include "Newton.h"

/*------------------------------------------------------------------------------
	C S t o r e D r i v e r
------------------------------------------------------------------------------*/

class CStoreDriver
{
public:
	void		init(char*, size_t, char*, ULong);
	char *	addressOf(ULong, bool *);
	char *	addressOfAligned(ULong, bool *);
	void		copy(ULong, ULong, size_t);
	void		persistentCopy(ULong, ULong, ULong);
	void		doPersistentCopy(ULong, ULong);
	void		continuePersistentCopy(void);
	void		set(VAddr, size_t, ULong);
	void		read(char*, VAddr, size_t);
	void		write(char*, VAddr, size_t);

	char *	fStoreBase;		//	start of contiguous store data
	size_t	fStoreSize;		// size of same
	char *	fMemBase;		// start of non-contiguous extra data
	ULong		f0C;		// size of same
	ULong		f10;
	ULong		f14;
	ULong		f18;
	bool		f24;
};

#endif	/* __STOREDRIVER_H */
