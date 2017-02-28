/*
	File:		PackageAccessor.cc

	Contains:	Functions to access parts of a package.

	Written by:	The Newton Tools group.
*/

#include "PackageAccessor.h"
#include "Reporting.h"
#include "RExArray.h"
#include "Frames/Objects.h"


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

//extern CChunk *	gGelatoPageTable;
//extern CChunk *	gPatchTablePadding;
//extern CChunk *	gPatchTablePageTable;

//extern CChunk *	gPackages;
PendingImport *	g12FA;	// ?
extern RExArray *	g12BE;	// gExportTable? array of Ref32
//extern CChunk *	g12BA;	// gExportTablePadding?
extern RExArray *	g12B6;


/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */
extern int hcistrcmp(const char * inStr, const char * inOptionStr);

void RenumberImports(CFramesPartAccessor & inPart, Ref32 * inArg2, PartRef inArg3, PendingImport ** inArg4, int inArg5);
void ProcessImports(CFramesPartAccessor & inPart, PartRef inRef);	// 7 pages
void ProcessExports(PartRef inRef, long inOffset);
void RedirectExports(CFramesPartAccessor & inPart, PartRef inRef);


/* -----------------------------------------------------------------------------
	C R e l o c a t e S c a n n e r
	Used by CFramesPartAccessor::relocate() to relocate refs.
----------------------------------------------------------------------------- */
class CRelocateScanner : public RefScanner
{
public:
			CRelocateScanner(long inOffset);

	void	scanRef(Ref32 * ioRefPtr);
private:
	long	f04;
};

inline CRelocateScanner::CRelocateScanner(long inOffset) { f04 = inOffset; }


void
CRelocateScanner::scanRef(Ref32 * ioRefPtr)
{
	if (ISPTR(*ioRefPtr)) {
		*ioRefPtr += f04;
	}
}


/* -----------------------------------------------------------------------------
	C R e n u m b e r I m p o r t s S c a n n e r
	Used by RenumberImports to, erm, renumber imports.
----------------------------------------------------------------------------- */
int g12F6;	// don’t really know
int g0CD8;	// don’t really know

// see also MagicPointers.cc
#define kMPTableShift	12
#define kMPTableMask		0x3FFFF000
#define kMPIndexMask		0x00000FFF

class CRenumberImportsScanner : public RefScanner
{
public:
			CRenumberImportsScanner(Ref32 * inArg2, PendingImport ** inArg4, bool inArg5);

	void	scanRef(Ref32 * ioRefPtr);
private:
	int	f00;
	Ref32	f04;	// not really a Ref (no tag bits) but will make one
	int *	f08;
	PendingImport **	f0C;
	bool	f10;
};

inline CRenumberImportsScanner::CRenumberImportsScanner(Ref32 * inArg2, PendingImport ** inArg4, bool inArg5)
{
	f00 = g12F6;
	f04 = (g0CD8 * 2 + 2) << kMPTableShift;
	f08 = inArg2;
	f0C = inArg4;
	f10 = inArg5;
}


void
CRenumberImportsScanner::scanRef(Ref32 * ioRefPtr)
{
//a4: a3
	if (ISMAGICPTR(*ioRefPtr)) {
		ULong rValue = *ioRefPtr >> kRefTagBits;		// d7
		ArrayIndex table = rValue >> kMPTableShift;	// d6
		if (table >= 2) {
			// not built-in
			ArrayIndex index = rValue & kMPIndexMask;	//d7
			int d5 = f08[table-2];
			if (f10) {
				if (d5 == -1) {
					int spm04 = f0C[table-2];
					*ioRefPtr = MAKEMAGICPTR(index + *spm04 + (1 << kMPTableShift) + f04);
				}
			} else {
				if (d5 == -1) {
					int spm04 = f0C[table-2];
					if (*spm04 < index) {
						*spm04 = index + 1;
					}
				} else {
					*ioRefPtr = MAKEMAGICPTR(index + d5 + f04);
				}
			}
		}
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	P r o c e s s P a r t
	If a part has UnitReferences we need to process imported/exported units.
----------------------------------------------------------------------------- */

void
ProcessPart(int inSelector, CFramesPartAccessor & inPart, long inPkgOffset, long inOffset)
{
//	C588(0x20);
	bool exists;
	PartRef partRef;
	PartRef partRoot;
	Ref32 tableRef;
	Ref32 rootRef = inPart.getArraySlot(MAKEPTR(inOffset), 0);
	partRoot.fAccessor = &inPart;
	partRoot.fRef = rootRef;

	switch (inSelector) {
	case 0:	// process exports
//+0088
		tableRef = partRoot.fAccessor->getFrameSlot(partRoot.fRef, "_ExportTable", &exists);
		partRef.fAccessor = partRoot.fAccessor;
		partRef.fRef = tableRef;
		if (NOTNIL(partRef.fRef)) {
			ProcessExports(partRef, inPkgOffset);
		}
		break;

	case 1:	// process imports
//+0104
		tableRef = partRoot.fAccessor->getFrameSlot(partRoot.fRef, "_ImportTable", &exists);
		partRef.fAccessor = partRoot.fAccessor;
		partRef.fRef = tableRef;
		if (NOTNIL(partRef.fRef)) {
			ProcessImports(inPart, partRef);
		}
		break;

	case 2:	// redirect exports
//+017E
		tableRef = partRoot.fAccessor->getFrameSlot(partRoot.fRef, "_ExportTable", &exists);
		partRef.fAccessor = partRoot.fAccessor;
		partRef.fRef = tableRef;
		if (NOTNIL(partRef.fRef)) {
			RedirectExports(inPart, partRef);
		}
		break;
	}
}


/* -----------------------------------------------------------------------------
	Process exported UnitReferences.
	Args:		inTable		the _ExportTable array of unit declaration frames
				inPkgOffset	base offset
	Return:	--
----------------------------------------------------------------------------- */

void
ProcessExports(PartRef inTable, long inPkgOffset)
{
	if (g12BE == NULL) {
		g12BE = new RExArray(sizeof(Ref32));
	}
//+0056
//	PartRef spm0C;
//	spm0C.fAccessor = NULL;
//	spm0C.fRef = NILREF;
//	PartRef * a0 = &spm0C;

	for (ArrayIndex unitNo = 0, numOfUnits = inTable.fAccessor->arrayLength(inTable.fRef); unitNo < numOfUnits; ++unitNo) {	//spm04 spm10
//+0084
		PartRef unitDecl;	//spm20
//		PartRef * spm18 = &spm20;
		unitDecl.fAccessor = inTable.fAccessor;
		unitDecl.fRef = inTable.fAccessor->getArraySlot(inTable.fRef, unitNo);
//+00DC
//		spm0C = unitDecl;

		// we want to add an ExportUnit to the g12B6 array
		ExportUnit unit;	//spm8C..74
//+00E8
//		unit.fObjects.fAccessor = NULL;
//		unit.fObjects.fRef = NILREF;
//		PartRef * spm2C = &spm0C;

		PartRef nameRef;	//spm70
//		PartRef * spm28 = &nameRef;
		nameRef.fAccessor = inTable.fAccessor;
		nameRef.fRef = unitDecl.fAccessor->getFrameSlot(unitDecl.fRef, "name");	//138A
//+0162
//		PartRef * spm24 = &nameRef;
		const char * name = nameRef.fAccessor->symbolName(nameRef.fRef);	//spm74
		unit.fName = (char *)malloc(strlen(name) + 1);	//spm78
		strcpy(unit.fName, name);
//+01A0
		PartRef versionRef;
//		PartRef * spm64 = &spm0C;
//		PartRef * spm60 = &versionRef;
		versionRef.fAccessor = unitDecl.fAccessor;
		versionRef.fRef = unitDecl.fAccessor->getFrameSlot(unitDecl.fRef, "major");	//-1384
//+020A
//		PartRef * spm58 = &versionRef;
		unit.fMajorVersion = RVALUE(versionRef.fRef);

//		PartRef spm9C;
//		PartRef * spm54 = &spm0C;
//		PartRef * spm50 = &spm9C;
		versionRef.fAccessor = unitDecl.fAccessor;
		versionRef.fRef = unitDecl.fAccessor->getFrameSlot(unitDecl.fRef, "minor");	//-137E
//+026C
//		PartRef * spm4C = &versionRef;
		unit.fMinorVersion = RVALUE(versionRef.fRef);

		PartRef objRef;	//spmA4
//		PartRef * spm48 = &spm0C;
//		PartRef * spm44 = &objRef;
		objRef.fAccessor = unitDecl.fAccessor;
		objRef.fRef = unitDecl.fAccessor->getFrameSlot(unitDecl.fRef, "objects");	//-1378
//+0302
		unit.fObjects = objRef;

		// we want to add the members of the unit declaration to the g12BE array
		ArrayIndex numOfObjects = objRef.fAccessor->arrayLength(objRef.fRef);
//		RExArray * spm3C = g12BE;
		unit.fCount = g12BE->count();
		g12BE->fChunk.append(NULL, numOfObjects * sizeof(Ref32));
//+035A
		Ref32 * exportedObject = (Ref32 *)g12BE->at(unit.fCount);	//spm68
//+0390
		for (ArrayIndex i = 0; i < numOfObjects; ++i) {
			PartRef obj;
//			PartRef * spm34 = &obj;
			obj.fAccessor = unit.fObjects.fAccessor;
			obj.fRef = unit.fObjects.fAccessor->getArraySlot(unit.fObjects.fRef, i);

//			PartRef * spm30 = &obj;
			exportedObject[i] = obj.fRef + inPkgOffset;
		}
//+0408
		g12B6->fChunk.append(&unit, sizeof(unit));
	}
}


void
RenumberImports(CFramesPartAccessor & inPart, Ref32 * inArg2, PartRef inArg3, PendingImport ** inArg4, int inArg5)
{
	if (inArg4 == NULL) {
		inArg4 = new PendingImport*;	// linked list?
		*inArg4 = g12FA;
	}
	CRenumberImportsScanner scanner(inArg2, inArg4, inArg5);
	inPart.scanRefs(scanner, inArg3.fRef);
}


void
ProcessImports(CFramesPartAccessor & inPart, PartRef inTable)
{
	ArrayIndex count = inTable.fAccessor->arrayLength(inTable.fRef);	// spm10
	Ref32 * spm0C = new Ref32[count];
	PendingImport ** spm08 = new PendingImport*[count];
	bool spm04 = false;
	for (ArrayIndex d5 = 0; d5 < count; ++d5) {
//+004C
		spm3C = &spm70;
		spm38 = inTable.fAccessor->getArraySlot(inTable.fRef, d5);
		if (spm3C == NULL) {
			//new(8);
		}
		spm3C->f00 = spm40->f00;
		spm3C->f04 = spm38;
		spm34 = &spm70;
//+00B4
		spm30 = &spm64;
		spm2C = spm34.fAccessor->getFrameSlot(spm34.fRef, "name");	//-1370
		if (spm30 == NULL) {
			//new(8);
		}
		spm30->f00 = spm34->f00;
		spm30->f04 = spm2C;
//+0116
		spm28 = &spm64;
		const char * name = spm28.fAccessor->symbolName(spm28.fRef);	//spm68
//+012E
		a3 = &spm70;
		a0 = &spm5C;
		spm24 = a0;
		spm2C = a3.fAccessor->getFrameSlot(a3.fRef, "major");		//-136A
		if (spm24 == NULL) {
			//new(8);
		}
		spm24->f00 = a3->f00;
		spm24->f04 = spm2C;
//+018C
		spm20 = &spm5C;
		d4 = spm20->f04 >> 2;
		a4 = &spm70;
		spm1C = &spm50;
		spm2C = a4.fAccessor->getFrameSlot(a4.fRef, "minor");		//-1364
		if (spm1C == NULL) {
			//new(8);
		}
		spm1C->f00 = a4->f00;
		spm1C->f04 = spm2C;
//+01F8
		spm18 = &spm50;
		spm54 = spm18->f04 >> 2;
		spm48 = 0;
		d6 = -1;
		for (ArrayIndex d7 = 0, spm44 = g12B6->count(); d7 < spm44; ++d7) {
//+023A
			spm78 = g12B6;
			spm7C = g12B6->at(d7);
			if (hcistrcmp(spm7C->14, name) == 0
			&&  d4 == spm7C->f08
			&&  d6 < spm7C->f0C) {
				d6 = spm7C->f0C;
				spm48 = spm7C;
			}
		}
		if (d6 >= spm54) {
			spm0C[d5] = spm48->f10;
		} else {
			spm8E = new[0x0C];
			spm8E->f00 = 0;
			if (g12C2 == NULL) {
				g12C2 = new CChunk;
				g12C2->append(NULL, 4);
			}
			spm8A = d4;
			spm86 = spm54;
			spm78 = g12C2;
			spm8E->f08 = spm78->f08;
			g12C2->append(&spm8A, 0x10);
			g12C2->append(&name, strlen(name)+1);
			g12C2->roundUp(4);
			spm0C[d5] = -1;
			spm08[d5] = spm8E;
			spm04 = true;
		}
	}
//+0398
	RenumberImports(inPart, spm0C, inTable, spm08, false);
	int d6 = 0;
	if (spm04) {
		for (ArrayIndex i = 0; i < count; ++i) {
			if (spm0C[i] == -1) {
				PendingImport * a4 = spm08[i];
				spm9A = g12C2;
				a3 = a4->f08 + **spm9A;
				a3->f08 = d6;
				a3->f0C = a4->f00;
				a4->f00 = d6;
				d6 += a3->f0C;
			}
		}
		spm74 = g12C2;
		a0 = **spm74;
		a0->f00 = d6;
		RenumberImports(inPart, spm0C, inTable, spm08, true);
		AddBlock(kFrameImportTableTag, g12C2);
	}
//+0476
	delete[] spm0C;
	for (ArrayIndex i = 0; i < count; ++i) {
		delete spm08[i];
	}
	delete[] spm08;
}


/* -----------------------------------------------------------------------------
	Redirect exported UnitReferences.
	See “Q&A Newton 2.x”

	A part’s _ExportTable slot contains an array of UnitReference frames, eg:
	 _ExportTable: [{name: '|Inet Protos:NIE|, 
                 major: 1, 
                 minor: 0, 
                 objects: [{DoEvent: <nativefunction, 2 args, #0287BC4D>, ...},...] }
					  //possibly more exported frames
					  ]

	DeclareUnit(unitName, majorVersion, minorVersion, memberIndexes)
		unitName - symbol - name of the unit
		majorVersion - integer - major version number of the unit
		minorVersion - integer - minor version number of the unit
		memberIndexes - frame - unit member name/index pairs (slot/value) return value - unspecified

	Args:		inPart		accessor for part data
				inTable		the _ExportTable array of unit declaration frames
	Return:	--
----------------------------------------------------------------------------- */

void
RedirectExports(CFramesPartAccessor & inPart, PartRef inTable)
{
	ArrayIndex count = g12B6->count();	//spm04
	if (count != 0) {
		bool isRedirectionReqd = false;
		Ref32 table = (g0CD8 * 2 + 2) << kMPTableShift;	//d4 not really a Ref32 but will make one
		for (ArrayIndex unitIndex = 0, unitCount = inTable.fAccessor->arrayLength(inTable.fRef); unitIndex < unitCount; ++unitIndex) {	//spm08 spm0C
			PartRef unitDecl;	//spm30
			unitDecl.fAccessor = inTable.fAccessor;
			unitDecl.fRef = inTable.fAccessor->getArraySlot(inTable.fRef, unitIndex);

			PartRef unitMembers;	//spm38
			unitMembers.fAccessor = unitDecl.fAccessor;
			unitMembers.fRef = unitDecl.fAccessor->getFrameSlot(unitDecl.fRef, "objects");

			// iterate over existing exports
			for (ArrayIndex i = 0; i < count; ++i) {
				spm3C = g12B6;
				spm54 = spm3C->f00;
				PartRef * spm40 = spm54->f00 + spm3C->f10 / i;
				if (spm40 == unitMembers) {
					if (!isRedirectionReqd) {
						inPart.beginRedirectionSetup();
						isRedirectionReqd = true;
					}
					for (ArrayIndex memberIndex = 0, memberCount = unitMembers.fAccessor->arrayLength(unitMembers.fRef); memberIndex < memberCount; ++memberIndex) {	//d7
						Ref32 destRef = MAKEMAGICPTR(spm40->f10 + table + memberIndex);

						PartRef member;	//spm50
						member.fAccessor = unitMembers.fAccessor;
						member.fRef = unitMembers.fAccessor.getArraySlot(unitMembers.fRef, memberIndex);
						inPart.setRedirection(member.fRef, destRef);
					}
				}
			}
#if 0
			spm20 = &spm30;
			spm1C = inTable.fAccessor->getArraySlot(inTable.fRef, unitIndex);
			//if (spm20 == NULL) { new(8); }
			spm20->f00 = inTable.fAccessor;
			spm20->f04 = spm1C;
			a3 = &spm30;
			spm18 = &spm38;
			spm14 = spm30.fAccessor->getFrameSlot(spm30.fRef, "objects");	//-135E
			//if (spm18 == NULL) { new(8); }
			spm18->f00 = *a3;
			spm18->f04 = spm14;
			for (ArrayIndex spm28 = 0; spm28 < count; ++spm28) {
//+012A
				spm3C = g12B6;
				spm54 = spm3C->f00;
				spm40 = spm54->f00 + spm3C->f10 / spm28;
				if (spm40 == spm38) {
					if (!isRedirectionReqd) {
						inPart.beginRedirectionSetup();
						isRedirectionReqd = true;
					}
					for (ArrayIndex index = 0; index < spm38.fAccessor.arrayLength(spm38.fRef); ++index) {	//d7
//+01A0
						Ref32 toRef = MAKEMAGICPTR(spm40->f10 + table + index);
						a4 = &spm38;
						d6 = spm38.fAccessor.getArraySlot(spm38.fRef, index);
						//if (spm48 == NULL) { new(8); }
						spm48->f00 = *a4;
						spm48->f04 = d6;
//+0206
						spm44 = &spm50;
						inPart.setRedirection(spm50.fRef, toRef);
					}
				}
			}
#endif
		}
		if (isRedirectionReqd) {
			inPart.doRedirection();
		}
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C P a c k a g e A c c e s s o r
	Access to the package diretory and part directory entries.
----------------------------------------------------------------------------- */

CPackageAccessor::CPackageAccessor(CChunk & inData, long inOffset)
	: fChunk(inData), fOffset(inOffset)
{ }

PackageDirectory *
CPackageAccessor::directory(void) const
{
	return (PackageDirectory *)(fChunk.data() + fOffset + 8);
}

bool
CPackageAccessor::isPackage() const
{
	PackageDirectory * pkgDir = directory();
	return strncmp(pkgDir->signature, "package0", 8) == 0
		 || strncmp(pkgDir->signature, "package1", 8) == 0;
}

size_t
CPackageAccessor::size() const
{
	return directory()->size;
}

ArrayIndex
CPackageAccessor::numParts() const
{
	return directory()->numParts;
}

PartEntry *
CPackageAccessor::part(ArrayIndex index) const
{
	PackageDirectory * pkgDir = directory();
	return pkgDir->parts + index;
}

Ptr
CPackageAccessor::unsafePartDataPtr(ArrayIndex index)
{
	return fChunk.data() + fOffset + partOffset(index);
}

long
CPackageAccessor::partOffset(ArrayIndex index) const
{
	PartEntry * pe = (PartEntry *)((Ptr)directory() + part(index)->offset);
	return pe->offset;
}

size_t
CPackageAccessor::partSize(ArrayIndex index) const
{
	return part(index)->size;
}

ULong
CPackageAccessor::partFlags(ArrayIndex index) const
{
	return part(index)->flags;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C F r a m e s P a r t A c c e s s o r
	Access to 32-bit refs in a NewtonScript frame part.
----------------------------------------------------------------------------- */

CFramesPartAccessor::CFramesPartAccessor(Ptr inPart, long inArg2, long inArg3)
{
	fRootObj = (ArrayObject32 *)inPart;
	f04 = inArg2;
	f08 = inArg3;
	fIsRedirectionSetup = false;
	// first part first object has alignment info
	fAlignment = (fRootObj->gc.stuff & k4ByteAlignmentFlag) != 0 ? 4 : 8;
	fAlignMask = ~(fAlignment - 1);
}


ObjHeader32 *
CFramesPartAccessor::objectPtr(Ref32 inRef)
{
	if (!ISPTR(inRef)) {
		FatalError("Internal error: #%X not a pointer", inRef);
	}
	return (ObjHeader32 *)PTR(inRef);
}


SymbolObject32 *
CFramesPartAccessor::symHeader(Ref32 inRef)
{
	SymbolObject32 * obj = (SymbolObject32 *)objectPtr(inRef);
	if ((obj->flags & kObjSlotted) != 0 || obj->hash != kSymbolClass) {
		FatalError("Internal error: #%X not a symbol", inRef);
	}
	return obj;
}


const char *
CFramesPartAccessor::symbolName(Ref32 inRef)
{
	return symHeader(inRef)->name;
}


bool
CFramesPartAccessor::symTest(Ref32 inRef, const char * inName)
{
	return hcistrcmp(symHeader(inRef)->name, inName) == 0;
}


ArrayIndex
CFramesPartAccessor::arrayLength(Ref32 inRef)
{
	ArrayObject32 * obj = (ArrayObject32 *)objectPtr(inRef);
	return (obj->size - sizeof(ArrayObject32)) / sizeof(Ref32);
}


ArrayIndex
CFramesPartAccessor::findOffset1(Ref32 inRef, const char * inSlot, bool * outExists)
{
	FrameMapObject32 * map = (FrameMapObject32 *)objectPtr(inRef);
	ArrayIndex i = 0;
	if (NOTNIL(map->supermap)) {
		i = findOffset1(map->supermap, inSlot, outExists);
		if (*outExists) {
			return i;
		}
	}
	Ref32 * tag = map->slot;
	for (ArrayIndex count = (map->size - sizeof(FrameMapObject32)) / sizeof(Ref32); i < count; ++i) {
		if (symTest(*tag, inSlot)) {
			*outExists = true;
			return i;
		}
	}
	*outExists = false;
	return kIndexNotFound;
}


ArrayIndex
CFramesPartAccessor::findOffset(FrameObject32 * inObj, const char * inSlot)
{
	bool exists;
	ArrayIndex index = findOffset1(inObj->map, inSlot, &exists);
	if (!exists) {
		return kIndexNotFound;
	}
	return index;
}


Ref32
CFramesPartAccessor::getFrameSlot(Ref32 inRef, const char * inSlot, bool * outExists)
{
	FrameObject32 * obj = (FrameObject32 *)objectPtr(inRef);
	if (!ISFRAME(obj)) {
		FatalError("Internal error: #%X not a frame", inRef);
	}
	ArrayIndex index = findOffset(obj, inSlot);
	if (index == kIndexNotFound) {
		*outExists = false;
		return NILREF;
	}
	*outExists = true;
	return obj->slot[index];
}

Ref32
CFramesPartAccessor::getArraySlot(Ref32 inRef, ArrayIndex index)
{
	ArrayObject32 * obj = (ArrayObject32 *)objectPtr(inRef);
	if (!ISARRAY(obj)) {
		FatalError("Internal error: #%X not an array", inRef);
	}
	if (/*index < 0 ||*/ index >= arrayLength(inRef)) {
		FatalError("Internal error: index %d out of bounds", index);
	}
	return obj->slot[index];
}


void
CFramesPartAccessor::relocate(long inOffset)
{
	CRelocateScanner scanner(inOffset);
	scanRefs(scanner, NILREF);
}


void
CFramesPartAccessor::scanRefs(RefScanner & inScanner, Ref32 inRef)
{
//a4: a3 d5
	Ref32 * refPtr;
	ObjHeader32 * targetObj = NOTNIL(inRef) ? objectPtr(inRef) : NULL;	//spm04
	ArrayObject32 * obj = (ArrayObject32 *)fRootObj;	//spm0C
	ArrayObject32 * objLimit = (ArrayObject32 *)((Ptr)fRootObj + f04);	//spm08
	for ( ; obj < objLimit; obj = (ArrayObject32 *)((Ptr)obj + ALIGN(obj->size, fAlignment))) {
		if ((ObjHeader32 *)obj != targetObj) {
			// scan the object class
			inScanner.scanRef(&obj->objClass);
			if (ISARRAY(obj)) {
				// scan the frame/array elements
				ArrayIndex count = ARRAY32LENGTH(obj);
				if ((Ptr)&obj->slot[count] > (Ptr)objLimit) {
					FatalError("package file corrupted");	// object too big
				}
				refPtr = &obj->slot[0];
				for (ArrayIndex i = 0; i < count; ++i, ++refPtr) {
					inScanner.scanRef(refPtr);
				}
			}
		}
//+00DA
		if (ALIGN(obj->size, fAlignment) == 0) {
			FatalError("package file corrupted");	// can’t have zero-size objects
		}
	}
}


void
CFramesPartAccessor::beginRedirectionSetup(void)
{
	if (fIsRedirectionSetup) {
		FatalError("Internal error: beginRedirectionSetup twice");
	}
	fIsRedirectionSetup = true;
}


void
CFramesPartAccessor::setRedirection(Ref32 inFromRef, Ref32 inToRef)
{
	if (!fIsRedirectionSetup) {
		FatalError("Internal error: beginRedirectionSetup not called");
	}
	ObjHeader32 * obj = objectPtr(inFromRef);
	if (obj == (ObjHeader32 *)fRootObj) {
		FatalError("Internal error: redirecting part data array");
	}
	if (obj->gc.stuff == 0xFFFFFFFF) {
		FatalError("Internal error: redirecting a safe object");
	}
	obj->gc.destRef = inToRef;
}


void
CFramesPartAccessor::doRedirection(void)
{
	if (!fIsRedirectionSetup) {
		FatalError("Internal error: beginRedirectionSetup not called");
	}
	fIsRedirectionSetup = false;

	Ref32 * refPtr;
	ObjHeader32 * targetObj;
	ArrayObject32 * obj = (ArrayObject32 *)fRootObj;	//spm08
	ArrayObject32 * objLimit = (ArrayObject32 *)((Ptr)fRootObj + f04);	//spm04
	for ( ; obj < objLimit; obj = (ArrayObject32 *)((Ptr)obj + ALIGN(obj->size, fAlignment))) {
		if (obj->gc.stuff != 0xFFFFFFFF) {
			// not a special object
			// redirect the class
			refPtr = &obj->objClass;
			if (ISPTR(*refPtr)) {
				targetObj = objectPtr(*refPtr);
				if (targetObj->gc.stuff != 0 && targetObj->gc.stuff != 0xFFFFFFFF) {
					*refPtr = targetObj->gc.destRef;
				}
			}
//+009A
			if (ISARRAY(obj)) {
				// redirect all frame/array elements
				ArrayIndex count = ARRAY32LENGTH(obj);
				if ((Ptr)&obj->slot[count] > (Ptr)objLimit) {
					FatalError("package file corrupted");	// object too big
				}
				refPtr = &obj->slot[0];
				for (ArrayIndex i = 0; i < count; ++i, ++refPtr) {
					if (ISPTR(*refPtr)) {
						targetObj = objectPtr(*refPtr);
						if (targetObj->gc.stuff != 0 && targetObj->gc.stuff != 0xFFFFFFFF) {
							*refPtr = targetObj->gc.destRef;
						}
					}
				}
			}
		}
//+013C
		if (ALIGN(obj->size, fAlignment) == 0) {
			FatalError("package file corrupted");	// can’t have zero-size objects
		}
	}
//+017C
	// clean up -- zero out gc.stuff for all objects after the root array
	obj = (ArrayObject32 *)((Ptr)fRootObj + ALIGN(obj->size, fAlignment));		// originally (obj->size + (fAlignment-1)) & fAlignMask
	while (obj < objLimit) {
		if (obj->gc.stuff != 0) {
			obj->gc.stuff = 0;
		}
		obj = (ArrayObject32 *)((Ptr)obj + ALIGN(obj->size, fAlignment));		// originally (obj->size + (fAlignment-1)) & fAlignMask
	}
}



#pragma mark -
/* -----------------------------------------------------------------------------
	P a r t R e f
----------------------------------------------------------------------------- */

bool
PartRef::operator==(PartRef & _p)
{
	if (ISPTR(fRef) && ISPTR(_p.fRef)) {
		return fAccessor->objectPtr(fRef) == _p.fAccessor->objectPtr(_p.fRef);
	}
	return fRef == _p.fRef;
}

