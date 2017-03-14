/*
	File:		UnRelocate.cc

	Contains:	Functions to unrelocate a Newton package: pointer Refs from addresses to package-relative offsets.

	Written by:	The Newton Tools group.
*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#define hasByteSwapping 1

#include "Objects.h"
#include "ObjHeader.h"
#include "Ref32.h"
#include "PackageParts.h"


/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */

inline bool IsPtr(Ref32 ref);
static void FixRef(Ref32 ref, Ptr inROMBase, int inPkgBase);

void
PrintName(PackageDirectory * inDirectory)
{
	char name[64];
	int numParts = CANONICAL_LONG(inDirectory->numParts);
	int offsetToName = CANONICAL_SHORT(inDirectory->name.offset) + numParts*sizeof(PartEntry) + sizeof(PackageDirectory);
	int lengthOfName = CANONICAL_SHORT(inDirectory->name.length) / sizeof(UniChar);
	UniChar * p = (UniChar *)((char *)inDirectory + offsetToName);
	for (ArrayIndex i = 0; i < lengthOfName; ++i, ++p) {
		name[i] = CANONICAL_SHORT(*p);
	}
	printf("\nPackage “%s”\n", name);
}


/* -----------------------------------------------------------------------------
	Reset the relocation offsets in a package to be relative to the package
	and not absolute addresses.
	We do this by following object refs from each part base and fixing up
	the addresses.

	What we have:
		address of the package in the ROM image
		package image

	Args:		pkg		package binary object
	Return:	--			the package is modified
----------------------------------------------------------------------------- */

void
UnRelocatePkg(void * inPkg, void * inROMBase)
{
	PackageDirectory * dir = (PackageDirectory *) inPkg;
	long offsetToPartData = CANONICAL_LONG(dir->directorySize);

//	if it’s a "package1" with relocation info then adjust that relocation info
	if (dir->signature[7] == '1') {
		ULong flags = CANONICAL_LONG(dir->flags);
		if (FLAGTEST(flags, kRelocationFlag)) {
			RelocationHeader * hdr = (RelocationHeader *)((char *)inPkg + offsetToPartData);
			long delta = (long) inPkg - CANONICAL_LONG(hdr->baseAddress);
			RelocationEntry * set = (RelocationEntry *)(hdr + 1);
			for (ArrayIndex i = 0; i < hdr->numEntries; ++i)
			{
//			long * pageBase = (long *) (pp + set->pageNumber * hdr->pageSize);
//			for (int offsetNumber = 0; offsetNumber < set->offsetCount; offsetNumber++)
//				*(pageBase + set->offsets[offsetNumber]) -= delta;
//			set = (RelocationEntry *) ((char *) set
//					+ sizeof(UShort) * 2					// RelocationEntry less offsets
//					+ LONGALIGN(set->offsetCount));
			}
			offsetToPartData += hdr->relocationSize;
		}
	}

	PrintName(dir);
//	fix up refs for all parts
	for (ArrayIndex partNumber = 0; partNumber < CANONICAL_LONG(dir->numParts); ++partNumber) {
		ArrayObject32 * rObj = (ArrayObject32 *)((char *)inPkg + offsetToPartData + CANONICAL_LONG(dir->parts[partNumber].offset));
printf("Part %d at %p\n", partNumber, rObj);
		Ref32 ref = CANONICAL_LONG(rObj->slot[0]);
		if (IsPtr(ref) ) {
			int pkgBase = (int)((long)inPkg - (long)inROMBase);
			Ref32 pkgRef = ref - pkgBase;
			rObj->slot[0] = CANONICAL_LONG(pkgRef);
			FixRef(ref, (Ptr)inROMBase, pkgBase);
		}
	}
}


//	Determine whether a Newton ref is a pointer (could be an immediate)

inline bool
IsPtr(Ref32 ref)
{
	return ISREALPTR(ref) && ((ref & 0xFFF00000) != 0);
}


/* -----------------------------------------------------------------------------
	Fix up a pointer ref from offset in ROM image to package-relative offset.

	Args:		ref			MUST be a pointer ref: pointer is really offset in ROM image
				inROMBase	address of ROM image
				inPkgBase	offset to package in ROM image
	Return:	--				the package is modified
----------------------------------------------------------------------------- */
#define	RPTR(_r)	(_r & ~3)

static void
FixRef(Ref32 ref, Ptr inROMBase, int inPkgBase)
{
//printf("FixRef(ref=%08X)\n", ref);
//	point to the object in the ROM image - could  be frame or array as well as binary
	ArrayObject32 * rObj = (ArrayObject32 *)(inROMBase + RPTR(ref));

//	point to its class
	Ref32 pkgRef;
	Ref32 * refPtr = &(rObj->objClass);
	ref = CANONICAL_LONG(*refPtr);
	if (IsPtr(ref)) {
		pkgRef = ref - inPkgBase;
		*refPtr = CANONICAL_LONG(pkgRef);	//	update the class ref
		if (ISFRAME(rObj))
			FixRef(ref, inROMBase, inPkgBase);	//	for frames, class is actually map
	}

//	if it’s a frame / array, step through each slot / element fixing those
	if (ISSLOTTED(rObj)) {
		refPtr = rObj->slot;
		for (ArrayIndex i = 0, count = ARRAY32LENGTH(rObj); i < count; ++i, ++refPtr) {
			ref = CANONICAL_LONG(*refPtr);
			if (IsPtr(ref)) {
				pkgRef = ref - inPkgBase;
				*refPtr = CANONICAL_LONG(pkgRef);
				FixRef(ref, inROMBase, inPkgBase);		//	follow it to update sub-refs
			}
		}
	}
}

