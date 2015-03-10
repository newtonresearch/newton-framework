/*
	File:		PSSTypes.h

	Contains:	Persistent Store System interface.

	Written by:	Newton Research Group.
*/

#if !defined(__PSSTYPES_H)
#define __PSSTYPES_H 1

#include "Newton.h"

// identifier for objects in the PSS
typedef uint32_t PSSId;
#define kNoPSSId 0xFFFFFFFF

// zero-based address for objects on a store (that would be an offset, then)
typedef uint32_t ZAddr;
#define kIllegalZAddr 0xFFFFFFFF

#endif	/* __PSSTYPES_H */
