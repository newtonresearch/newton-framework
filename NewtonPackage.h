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

class NewtonPackage
{
public:
					NewtonPackage(const char * inPkgPath);
					~NewtonPackage();

	PackageDirectory *	directory(void);
	const PartEntry *	partEntry(ArrayIndex inPartNo);
	Ref			partRef(ArrayIndex inPartNo);
	char *		partPkgData(ArrayIndex inPartNo);

private:
	FILE * pkgFile;
	PackageDirectory * pkgDir;
	RelocationHeader pkgRelo;
	char * relocationData;
	char ** pkgPartData;
	char * part0Data;
};


#endif	/* __NEWTONPACKAGE_H */
