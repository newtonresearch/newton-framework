/*
	File:		NewtonPackage.cc

	Abstract:	An object that loads a Newton package file containing 32-bit big-endian Refs
					and presents part entries as 64-bit platform-endian Refs.

	Written by:	Newton Research Group, 2015.
*/

#include "Objects.h"
#include "NewtonPackage.h"
#include "ObjHeader.h"
#include "Ref32.h"

#if __LP64__
/* -----------------------------------------------------------------------------
	T Y P E S
----------------------------------------------------------------------------- */
#include <unordered_set>
typedef std::unordered_set<Ref32> ScanRefMap;

#include <map>
typedef std::map<Ref32, Ref> RefMap;

/* -----------------------------------------------------------------------------
	F O R W A R D   D E C L A R A T I O N S
----------------------------------------------------------------------------- */
bool		IsObjClass(Ref obj, const char * inClassName);

size_t	ScanRef(Ref32 inRef, ScanRefMap & ioMap, const char * inBaseAddr);
Ref		CopyRef(Ref32 inRef, ArrayObject * &ioDstPtr, RefMap & ioMap, const char * inBaseAddr);
void		UpdateRef(Ref * inRefPtr, RefMap & inMap);
#else
void		FixUpRef(Ref * inRefPtr, char * inBaseAddr);
#endif


#pragma mark -
/* -----------------------------------------------------------------------------
	Constructor.
----------------------------------------------------------------------------- */

NewtonPackage::NewtonPackage(const char * inPkgPath)
{
	pkgFile = fopen(inPkgPath, "r");
	pkgDir = NULL;
	relocationData = NULL;
	part0Data = NULL;
	pkgPartData = NULL;
}


/* -----------------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------------- */

NewtonPackage::~NewtonPackage()
{
	// free data
	if (pkgDir)
		free(pkgDir);
	if (relocationData)
		free(relocationData);
	if (pkgPartData != NULL && pkgPartData != &part0Data)
		free(pkgPartData);
	// don’t free individual part data -- it is persistent
	// if client wants to free it they must keep a reference to it before the NewtonPackage is destroyed
	// close the file
	if (pkgFile)
		fclose(pkgFile);
}


/* -----------------------------------------------------------------------------
	Return the package directory.
	Args:		--
	Return:	package directory, platform byte-aligned; treat as read-only
----------------------------------------------------------------------------- */

PackageDirectory *
NewtonPackage::directory(void)
{
	XTRY
	{
		XFAIL(pkgFile == NULL)

		PackageDirectory dir;
		// read the directory
		fread(&dir, sizeof(PackageDirectory), 1, pkgFile);
		// verify signature
		XFAIL(strncmp(dir.signature, "package", 7) != 0)
		// allocate for the directory + part entries
		int pkgDirSize = CANONICAL_LONG(dir.directorySize);
		pkgDir = (PackageDirectory *)malloc(pkgDirSize);
		XFAIL(pkgDir == NULL)
		// read all that
		fseek(pkgFile, 0, SEEK_SET);
		fread(pkgDir, pkgDirSize, 1, pkgFile);

#if defined(hasByteSwapping)
		pkgDir->id = BYTE_SWAP_LONG(pkgDir->id);
		pkgDir->flags = BYTE_SWAP_LONG(pkgDir->flags);
		pkgDir->version = BYTE_SWAP_LONG(pkgDir->version);
		pkgDir->copyright.offset = BYTE_SWAP_SHORT(pkgDir->copyright.offset);
		pkgDir->copyright.length = BYTE_SWAP_SHORT(pkgDir->copyright.length);
		pkgDir->name.offset = BYTE_SWAP_SHORT(pkgDir->name.offset);
		pkgDir->name.length = BYTE_SWAP_SHORT(pkgDir->name.length);
		pkgDir->size = BYTE_SWAP_LONG(pkgDir->size);
		pkgDir->creationDate = BYTE_SWAP_LONG(pkgDir->creationDate);
		pkgDir->modifyDate = BYTE_SWAP_LONG(pkgDir->modifyDate);
		pkgDir->directorySize = BYTE_SWAP_LONG(pkgDir->directorySize);
		pkgDir->numParts = BYTE_SWAP_LONG(pkgDir->numParts);

		PartEntry * pe = pkgDir->parts;
		for (int i = 0; i < pkgDir->numParts; ++i, ++pe)
		{
			pe->offset = BYTE_SWAP_LONG(pe->offset);
			pe->size = BYTE_SWAP_LONG(pe->size);
			pe->size2 = pe->size;
			pe->type = BYTE_SWAP_LONG(pe->type);
			pe->flags = BYTE_SWAP_LONG(pe->flags);
			pe->info.offset = BYTE_SWAP_SHORT(pe->info.offset);
			pe->info.length = BYTE_SWAP_SHORT(pe->info.length);
			pe->compressor.offset = BYTE_SWAP_SHORT(pe->compressor.offset);
			pe->compressor.length = BYTE_SWAP_SHORT(pe->compressor.length);
		}
#endif

		//	if it’s a "package1" with relocation info then read that relocation info
		if ((pkgDir->signature[7] == '1') && (pkgDir->flags & kRelocationFlag) != 0)
		{
			// read relocation header
			fseek(pkgFile, pkgDir->directorySize, SEEK_SET);
			fread(&pkgRelo, sizeof(RelocationHeader), 1, pkgFile);

#if defined(hasByteSwapping)
			pkgRelo.relocationSize = BYTE_SWAP_LONG(pkgRelo.relocationSize);
			pkgRelo.pageSize = BYTE_SWAP_LONG(pkgRelo.pageSize);
			pkgRelo.numEntries = BYTE_SWAP_LONG(pkgRelo.numEntries);
			pkgRelo.baseAddress = BYTE_SWAP_LONG(pkgRelo.baseAddress);
#endif

			// read relocation data into memory
			size_t relocationDataSize = pkgRelo.relocationSize - sizeof(RelocationHeader);
			relocationData = (char *)malloc(relocationDataSize);
			XFAILIF(relocationData == NULL, free(pkgDir); pkgDir = NULL;)
			fread(relocationData, relocationDataSize, 1, pkgFile);
		}
		else
			pkgRelo.relocationSize = 0;
	}
	XENDTRY;
	return pkgDir;
}


/* -----------------------------------------------------------------------------
	Return part entry info.
	Args:		inPartNo		the part number, typically 0
	Return:	part info entry, platform byte-aligned; treat as read-only
----------------------------------------------------------------------------- */

const PartEntry *
NewtonPackage::partEntry(ArrayIndex inPartNo)
{
	// load the directory if necessary
	if (pkgDir == NULL)
		directory();

	// sanity check
	if (inPartNo >= pkgDir->numParts)
		return NULL;

	return &pkgDir->parts[inPartNo];
}


/* -----------------------------------------------------------------------------
	Return part data loaded from the .pkg file containing the ref.
	We assume the NewtonPackage is persistent (it certainly is for the Newton
	framework) but in the case of eg the Package Inspector we need to dispose
	the package data when the package inspector window is closed. To accomplish
	this we expose the underlying allocated part Ref data so it can be free()d.
	Args:		inPartNo		the part number, typically 0
	Return:	pointer to malloc()d Ref part data
----------------------------------------------------------------------------- */

char *
NewtonPackage::partPkgData(ArrayIndex inPartNo)
{
	// load the directory if necessary
	if (pkgDir == NULL)
		directory();

	// sanity check
	if (inPartNo == 0 || inPartNo >= pkgDir->numParts)
		return NULL;

	return pkgPartData[inPartNo];
}


/* -----------------------------------------------------------------------------
	Return the top part Ref for a part in the package.
	Args:		inPartNo		the part number, typically 0
	Return:	top part Ref
----------------------------------------------------------------------------- */

Ref
NewtonPackage::partRef(ArrayIndex inPartNo)
{
	// load the directory if necessary
	if (pkgDir == NULL)
		directory();

	// sanity check
	if (inPartNo >= pkgDir->numParts)
		return NILREF;

	// create the array of per-part partData pointers if necessary
	if (pkgPartData == NULL) {
		if (pkgDir->numParts == 1) {
			pkgPartData = &part0Data;
		} else {
			pkgPartData = (char **)malloc(pkgDir->numParts*sizeof(char *));
		}
	}

	const PartEntry * thePart = partEntry(inPartNo);

	// we can only handle NOS parts
	if ((thePart->flags & kPartTypeMask) != kNOSPart) {
		pkgPartData[inPartNo] = NULL;
		return NILREF;
	}

	// read part into memory
	char * partData;
	size_t partSize = thePart->size;
	partData = (char *)malloc(partSize);	// will leak if partRef called more than once
	if (partData == NULL)
		return NILREF;
	pkgPartData[inPartNo] = partData;

	long baseOffset = pkgDir->directorySize + pkgRelo.relocationSize;
	baseOffset = LONGALIGN(baseOffset);
	fseek(pkgFile, baseOffset + thePart->offset, SEEK_SET);
	fread(partData, partSize, 1, pkgFile);

	// adjust any relocation info
	if (pkgRelo.relocationSize != 0)
	{
// need to do this for THIS PART ONLY
#if 0
		long  delta = (long)partData - pkgRelo.baseAddress;
		RelocationEntry *	set = (RelocationEntry *)relocationData;
		for (ArrayIndex setNum = 0; setNum < pkgRelo.numEntries; ++setNum)
		{
			int32_t * p, * pageBase = (int32_t *)(partData + CANONICAL_SHORT(set->pageNumber) * pkgRelo.pageSize);
			ArrayIndex offsetCount = CANONICAL_SHORT(set->offsetCount);
			for (ArrayIndex offsetNum = 0; offsetNum < offsetCount; ++offsetNum)
			{
				p = pageBase + set->offsets[offsetNum];
				*p = CANONICAL_LONG(*p) + delta;	// was -; but should be + surely?
			}
			set = (RelocationEntry *)((char *)set
					+ sizeof(UShort) * 2					// RelocationEntry less offsets
					+ LONGALIGN(offsetCount));
		}
#endif
	}

	// locate the part frame
	ArrayObject32 * pkgRoot = (ArrayObject32 *)partData;
#if __LP64__
/*
	So, now:
		partData = address of part file read into memory
		baseOffset = offset into part of Ref data
	Refs in the .pkg file are offsets into Ref data and need to be fixed up to run-time addresses
*/
	ScanRefMap scanRefMap;
	size_t part64Size = ScanRef(CANONICAL_LONG(REF(baseOffset)), scanRefMap, partData - baseOffset);
	char * part64Data = (char *)malloc(part64Size);
	if (part64Data == NULL) {
		free(partData), partData = NULL;
		return NILREF;
	}

	ArrayObject * newRoot = (ArrayObject *)part64Data;
	ArrayObject * dstPtr = newRoot;
	RefMap map;
	CopyRef(CANONICAL_LONG(REF(baseOffset)), dstPtr, map, partData - baseOffset);
	UpdateRef(newRoot->slot, map);	// Ref offsets -> addresses

	// don’t need the 32-bit part data any more
	free(partData);
	// but we will need to free the 64-bit part data at some point
	partData = part64Data;
	// point to the 64-bit refs we want
	return newRoot->slot[0];
#else
	// fix up those refs from offsets to addresses, and do byte-swapping as appropriate
	FixUpRef(pkgRoot->slot, partData - baseOffset);

	// point to the refs we want
	return pkgRoot->slot[0];
#endif
}


#pragma mark -

#define BYTE_SWAP_SIZE(n) (((n << 16) & 0x00FF0000) | (n & 0x0000FF00) | ((n >> 16) & 0x000000FF))
#if defined(hasByteSwapping)
#define CANONICAL_SIZE BYTE_SWAP_SIZE
#else
#define CANONICAL_SIZE(n) (n)
#endif

#if __LP64__
/* -----------------------------------------------------------------------------
	Pass 1: calculate space needed for 64-bit refs
			  no modifications to Ref data
	Refs are offsets from inBaseAddr and in big-endian byte-order
	Args:		inRef				32-bit Ref -- offset from inBaseAddr
				inBaseAddr		address of part data
	Return:	memory requirement of 64-bit Ref
----------------------------------------------------------------------------- */

size_t
ScanRef(Ref32 inRef, ScanRefMap & ioMap, const char * inBaseAddr)
{
	Ref32 ref = CANONICAL_LONG(inRef);
	if (ISREALPTR(ref)) {
		if (ioMap.count(ref) > 0) {
			// ignore this object if it has already been scanned
			return 0;
		}
		ioMap.insert(ref);

		ArrayObject32 * obj = (ArrayObject32 *)PTR(ref + inBaseAddr);
		size_t refSize = sizeof(ArrayObject);
		// for frames, class is actually the map which needs fixing too; non-pointer refs may need byte-swapping anyway so we always need to do this
		refSize += ScanRef(obj->objClass, ioMap, inBaseAddr);

		ArrayIndex objSize = CANONICAL_SIZE(obj->size);
		//	if it’s a frame / array, step through each slot / element adding those
		if ((obj->flags & kObjSlotted) != 0) {
			Ref32 * refPtr = obj->slot;
			for (ArrayIndex count = (objSize - sizeof(ArrayObject32)) / sizeof(Ref32); count > 0; --count, ++refPtr) {
				refSize += sizeof(Ref) + ScanRef(*refPtr, ioMap, inBaseAddr);
			}
		} else {
			refSize += (objSize - sizeof(BinaryObject32));
		}
		return LONGALIGN(refSize);
	}
	// else it’s an immediate
	return 0;
}


/* -----------------------------------------------------------------------------
	Pass 2: copy 32-bit package source -> 64-bit platform Refs
			  adjust byte-order while we’re at it
	Args:		ioSrcPtr			Refs are offsets from inBaseAddr and in big-endian byte-order
				ioDstPtr			platform-oriented Refs
				ioMap
				inBaseOffset
				inBaseAddr
	Return:	64-bit platform-endian Ref
----------------------------------------------------------------------------- */

#if defined(hasByteSwapping)
bool
IsSymClass(SymbolObject32 * inSym, const char * inClassName)
{
	char * subName = inSym->name;
	for ( ; *subName && *inClassName; subName++, inClassName++) {
		if (tolower(*subName) != tolower(*inClassName)) {
			return NO;
		}
	}
	return (*inClassName == 0 && (*subName == 0 || *subName == '.'));
}
#endif



Ref
CopyRef(Ref32 inRef, ArrayObject * &ioDstPtr, RefMap & ioMap, const char * inBaseAddr)
{
	Ref32 srcRef = CANONICAL_LONG(inRef);
	if (ISREALPTR(srcRef)) {
		RefMap::iterator findMapping = ioMap.find(srcRef);
		if (findMapping != ioMap.end()) {
			// we have already fixed up this ref -- return its pointer ref
			return findMapping->second;
		}
		// map 32-bit big-endian package-relative offset ref -> 64-bit pointer ref
		ArrayObject * dstPtr = ioDstPtr;
		Ref ref = REF(dstPtr);
		std::pair<Ref32, Ref> mapping = std::make_pair(srcRef, ref);
		ioMap.insert(mapping);

		ArrayObject32 * srcPtr = (ArrayObject32 *)PTR(inBaseAddr + srcRef);	// might not actually be an ArrayObject, but header/class are common to all pointer objects
		size_t srcSize = CANONICAL_SIZE(srcPtr->size);
		ArrayIndex count;
		size_t dstSize;
		if ((srcPtr->flags & kObjSlotted)) {
			//	work out size for 64-bit Ref slots
			count = (srcSize - sizeof(ArrayObject32)) / sizeof(Ref32);
			dstSize = sizeof(ArrayObject) + count * sizeof(Ref);
		} else {
			// adjust for change in header size
			dstSize = srcSize - sizeof(BinaryObject32) + sizeof(BinaryObject);
		}
		dstPtr->size = dstSize;
		dstPtr->flags = kObjReadOnly | (srcPtr->flags & kObjMask);
		dstPtr->gc.stuff = 0;

		//	update/align ioDstPtr to next object
		ioDstPtr = (ArrayObject *)((char *)ioDstPtr + LONGALIGN(dstSize));
		// for frames, class is actually the map which needs fixing too; non-slotted refs may need byte-swapping anyway so we always need to do this
		dstPtr->objClass = CopyRef(srcPtr->objClass, ioDstPtr, ioMap, inBaseAddr);

		if ((srcPtr->flags & kObjSlotted)) {
			//	iterate over src slots; fix them up
			Ref32 * srcRefPtr = srcPtr->slot;
			Ref * dstRefPtr = dstPtr->slot;
			for ( ; count > 0; --count, ++srcRefPtr, ++dstRefPtr) {
				*dstRefPtr = CopyRef(*srcRefPtr, ioDstPtr, ioMap, inBaseAddr);
			}
		} else {
			memcpy(dstPtr->slot, srcPtr->slot, dstSize - sizeof(ArrayObject));
#if defined(hasByteSwapping)
			SymbolObject32 * sym;
			Ref srcClass = CANONICAL_LONG(srcPtr->objClass);
			if (srcClass == kSymbolClass) {
				// symbol -- byte-swap hash
				SymbolObject * sym = (SymbolObject *)dstPtr;
				sym->hash = BYTE_SWAP_LONG(sym->hash);
//NSLog(@"'%s", sym->name);
			} else if (ISPTR(srcClass) && (sym = ((SymbolObject32 *)PTR(inBaseAddr + srcClass))), sym->objClass == CANONICAL_LONG(kSymbolClass)) {
				if (IsSymClass(sym, "string")) {
					// string -- byte-swap UniChar characters
					UniChar * s = (UniChar *)dstPtr->slot;
					for (count = (dstSize - sizeof(StringObject)) / sizeof(UniChar); count > 0; --count, ++s)
						*s = BYTE_SWAP_SHORT(*s);
//NSLog(@"\"%@\"", [NSString stringWithCharacters:(const UniChar *)srcPtr->slot length:(dstSize - sizeof(StringObject32)) / sizeof(UniChar)]);
				} else if (IsSymClass(sym, "real")) {
					// real number -- byte-swap 64-bit double
					uint32_t tmp;
					uint32_t * dbp = (uint32_t *)dstPtr->slot;
					tmp = BYTE_SWAP_LONG(dbp[1]);
					dbp[1] = BYTE_SWAP_LONG(dbp[0]);
					dbp[0] = tmp;
				} else if (IsSymClass(sym, "UniC")) {
					// EncodingMap -- byte-swap UniChar characters
					UShort * table = (UShort *)dstPtr->slot;
					UShort formatId, unicodeTableSize;
					*table = formatId = BYTE_SWAP_SHORT(*table), ++table;
					if (formatId == 0) {
						// it’s 8-bit to UniCode
						*table = unicodeTableSize = BYTE_SWAP_SHORT(*table), ++table;
						*table = BYTE_SWAP_SHORT(*table), ++table;		// revision
						*table = BYTE_SWAP_SHORT(*table), ++table;		// tableInfo
						for (ArrayIndex i = 0; i < unicodeTableSize; ++i, ++table) {
							*table = BYTE_SWAP_SHORT(*table);
						}
					} else if (formatId == 4) {
						// it’s UniCode to 8-bit
						*table = BYTE_SWAP_SHORT(*table), ++table;		// revision
						*table = BYTE_SWAP_SHORT(*table), ++table;		// tableInfo
						*table = unicodeTableSize = BYTE_SWAP_SHORT(*table), ++table;
						for (ArrayIndex i = 0; i < unicodeTableSize*3; ++i, ++table) {
							*table = BYTE_SWAP_SHORT(*table);
						}
					}
				}
			}
#endif
		}
		return ref;
	}
	return srcRef;
}


/* -----------------------------------------------------------------------------
	Pass 3: fix up 64-bit refs at new addresses
	For pointer refs, fix up the class.
	For slotted refs, fix up all slots recursively.

	Args:		inRefPtr		pointer to ref to be fixed up = offset from inBaseAddr
				ioMap			mapping of pointer Ref, pkg relative offset -> address
	Return:	--
----------------------------------------------------------------------------- */

void
UpdateRef(Ref * inRefPtr, RefMap & inMap)
{
	Ref ref = *inRefPtr;
	if (ISREALPTR(ref)) {
		RefMap::iterator mapping = inMap.find(ref);
		if (mapping != inMap.end()) {
			*inRefPtr = mapping->second;
			ArrayObject *  obj = (ArrayObject *)PTR(*inRefPtr);
			// for frames, class is actually the map which needs fixing too; non-pointer refs may need byte-swapping anyway so we always need to do this
			UpdateRef(&obj->objClass, inMap);

			//	if it’s a frame / array, step through each slot / element fixing those
			if ((obj->flags & kObjSlotted)) {
				Ref * refPtr = obj->slot;
				for (ArrayIndex count = ARRAYLENGTH(obj); count > 0; --count, ++refPtr) {
					UpdateRef(refPtr, inMap);
				}
			}
		}
	}
}

#else
#pragma mark -
/* -----------------------------------------------------------------------------
	FOR 32-BIT PLATFORM
	Fix up a ref: change offset -> address, adjust for platform byte order.
	For pointer refs, fix up the class.
	For slotted refs, fix up all slots recursively.

	Args:		inRefPtr		pointer to ref to be fixed up = offset from inBaseAddr
				inBaseAddr	base address of package from which refs are offsets
	Return:	--
----------------------------------------------------------------------------- */

#if defined(hasByteSwapping)
bool
IsObjClass(Ref obj, const char * inClassName)
{
	if (ISPTR(obj) && ((SymbolObject *)PTR(obj))->objClass == kSymbolClass) {
		const char * subName = SymbolName(obj);
		for ( ; *subName && *inClassName; subName++, inClassName++) {
			if (tolower(*subName) != tolower(*inClassName)) {
				return NO;
			}
		}
		return (*inClassName == 0 && (*subName == 0 || *subName == '.'));
	}
	return NO;
}
#endif


void
FixUpRef(Ref * inRefPtr, char * inBaseAddr)
{
#if defined(hasByteSwapping)
	// byte-swap ref in memory
	*inRefPtr = BYTE_SWAP_LONG(*inRefPtr);
#endif

	if (ISREALPTR(*inRefPtr) && *inRefPtr < (Ref)inBaseAddr)	// assume offset MUST be less than address
	{
		// ref has not been fixed from offset to address, so do it
		*inRefPtr += (Ref) inBaseAddr;

		ArrayObject * obj = (ArrayObject *)PTR(*inRefPtr);
		// if gc set, this object has already been fixed up
		if (obj->gc.stuff < 4) {
			obj->gc.stuff = 4;
			obj->size = CANONICAL_SIZE(obj->size);
			// for frames, class is actually the map which needs fixing too; non-pointer refs may need byte-swapping anyway so we always need to do this
			FixUpRef(&obj->objClass, inBaseAddr);

			//	if it’s a frame / array, step through each slot / element fixing those
			if ((obj->flags & kObjSlotted)) {
				Ref * refPtr = obj->slot;
				for (ArrayIndex count = (objSize - sizeof(ArrayObject)) / sizeof(Ref); count > 0; --count, ++refPtr) {
					FixUpRef(refPtr, inBaseAddr);
				}
			}
#if defined(hasByteSwapping)
			// else object is some form of binary; symbol, string, real or encoding table (for example) which needs byte-swapping
			else {
				Ref objClass = obj->objClass;
				if (objClass == kSymbolClass) {
					// symbol -- byte-swap hash
					SymbolObject * sym = (SymbolObject *)obj;
					sym->hash = BYTE_SWAP_LONG(sym->hash);
				} else if (IsObjClass(objClass, "string")) {
					// string -- byte-swap UniChar characters
					StringObject * str = (StringObject *)obj;
					UniChar * s = str->str;
					for (ArrayIndex count = (str->size - sizeof(StringObject32)) / sizeof(UniChar); count > 0; --count, ++s)
						*s = BYTE_SWAP_SHORT(*s);
				} else if (IsObjClass(objClass, "real")) {
					// real number -- byte-swap 64-bit double
					uint32_t tmp;
					uint32_t * dbp = (uint32_t *)((BinaryObject *)obj)->data;
					tmp = BYTE_SWAP_LONG(dbp[1]);
					dbp[1] = BYTE_SWAP_LONG(dbp[0]);
					dbp[0] = tmp;
				} else if (IsObjClass(objClass, "UniC")) {
					// EncodingMap -- byte-swap UniChar characters
					UShort * table = (UShort *)&obj->slot[0];
					UShort formatId, unicodeTableSize;

					*table = formatId = BYTE_SWAP_SHORT(*table), ++table;
					if (formatId == 0) {
						// it’s 8-bit to UniCode
						*table = unicodeTableSize = BYTE_SWAP_SHORT(*table), ++table;
						*table = BYTE_SWAP_SHORT(*table), ++table;		// revision
						*table = BYTE_SWAP_SHORT(*table), ++table;		// tableInfo
						for (ArrayIndex i = 0; i < unicodeTableSize; ++i, ++table) {
							*table = BYTE_SWAP_SHORT(*table);
						}
					} else if (formatId == 4) {
						// it’s UniCode to 8-bit
						*table = BYTE_SWAP_SHORT(*table), ++table;		// revision
						*table = BYTE_SWAP_SHORT(*table), ++table;		// tableInfo
						*table = unicodeTableSize = BYTE_SWAP_SHORT(*table), ++table;
						for (ArrayIndex i = 0; i < unicodeTableSize*3; ++i, ++table) {
							*table = BYTE_SWAP_SHORT(*table);
						}
					}
				}
			}
#endif
		}
	}
}

#endif

