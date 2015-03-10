/*
	File:		StoreRootObjects.h

	Contains:	Newton object store interface.

	Written by:	Newton Research Group.
*/

#if !defined(__STOREROOTOBJECTS_H)
#define __STOREROOTOBJECTS_H 1

#include "PSSTypes.h"

/* -----------------------------------------------------------------------------
	P a c k a g e R o o t
	A store has a root object (accessed via CStore::getRootId()) that defines
	the store’s capabilities.
----------------------------------------------------------------------------- */

class PackageRoot
{
public:
				PackageRoot();

	PSSId		fDataId;					// +00	id of table of ids of actual data on store
	PSSId		fCompanderNameId;		// +04
	PSSId		fCompanderParmsId;	// +08
	ULong		flags;					// +0C
	ULong		fSignature;				// +10	'paok'
};


/* -----------------------------------------------------------------------------
	L a r g e O b j e c t R o o t
----------------------------------------------------------------------------- */

class LargeObjectRoot : public PackageRoot
{
public:
				LargeObjectRoot();

	size_t	fActualSize;		// +14
	ULong		f18;
	ULong		f1C;
};

#endif	/* __STOREROOTOBJECTS_H */
