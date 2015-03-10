/*
	File:		RAMInfo.h

	Contains:	Physical memory info declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__RAMINFO_H)
#define __RAMINFO_H 1

// for RAMAllocSelector
#include "ROMExtension.h"

extern size_t	InternalStoreInfo(int inSelector);
extern size_t	InternalRAMInfo(int inSelector, size_t inAlign = 0);


#endif	/* __RAMINFO_H */
