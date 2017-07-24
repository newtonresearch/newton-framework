/*
	File:		NewtonPackage.h

	Abstract:	An object that loads a Newton package file containing 32-bit big-endian Refs
					and presents part entries as 64-bit platform-endian Refs.

	Written by:	Newton Research Group, 2015.
*/

#if !defined(__NEWTONPACKAGE_H)
#define __NEWTONPACKAGE_H 1

#include <stdio.h>
#include "PackageParts.h"

/* -----------------------------------------------------------------------------
	N e w t o n P a c k a g e
----------------------------------------------------------------------------- */
struct MemAllocation
{
	char *	data;
	size_t	size;
};


class NewtonPackage
{
public:
							NewtonPackage(const char * inPkgPath);
							NewtonPackage(void * inPkgData);
							~NewtonPackage();

	PackageDirectory *	directory(void);
	const PartEntry *	partEntry(ArrayIndex inPartNo);
	Ref					partRef(ArrayIndex inPartNo);
	MemAllocation *	partPkgData(ArrayIndex inPartNo);

private:
	FILE * pkgFile;
	void * pkgMem;
	PackageDirectory * pkgDir;
	RelocationHeader pkgRelo;
	char * relocationData;
	MemAllocation * pkgPartData;
	MemAllocation part0Data;
};


#endif	/* __NEWTONPACKAGE_H */
