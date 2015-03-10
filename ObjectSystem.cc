/*
	File:		ObjectSystem.cc

	Contains:	Newton object system framework initialization.

	Written by:	Newton Research Group.
*/

#include "RootView.h"

#include <CoreFoundation/CoreFoundation.h>

#include "Objects.h"
#include "ObjHeader.h"
#include "Ref32.h"
#include "PackageParts.h"
#include "Globals.h"
#include "NewtWorld.h"
#include "Iterators.h"
#include "REP.h"

#if defined(forFramework)
#define kBundleId CFSTR("com.newton.objects")
#else
#define kBundleId CFSTR("com.newton.messagepad")
#endif

#define kNSDictData CFSTR("DictData")

#if defined(forNTK)
#define kNSDataPkg CFSTR("NTKConstData.pkg")
#else
#define kNSDataPkg CFSTR("NSConstData.pkg")
#endif


extern PInTranslator *	gREPin;
extern POutTranslator *	gREPout;

extern void			ResetFromResetSwitch(void);
extern void			UserInit(void);
extern void			StartupProtocolRegistry(void);
extern NewtonErr	InitROMDomainManager(void);
extern NewtonErr	RegisterROMDomainManager(void);
extern NewtonErr	InitPSSManager(ObjectId inEnvId, ObjectId inDomId);
extern void			InitializeCompression(void);
extern void			InitObjects(void);
extern void			InitTranslators(void);
extern void			NTKInit(void);
extern Ref			InitUnicode(void);
extern void			InitExternal(void);
extern void			InitGraf(void);
extern void			InitScriptGlobals(void);
extern NewtonErr	InitInternationalUtils(void);
extern void			InitDictionaries(void);
extern void			RunInitScripts(void);
extern Ref			InitDarkStar(RefArg inRcvr, RefArg inOptions);

/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern "C" {
void	NSLog(CFStringRef format, ...);
void  InitObjectSystem(void);
}
		 void			InitStartupHeap(void);
		 void			FixStartupHeap(void);

		 void			MakeROMResources(void);
static void			InitBuiltInFunctions(void);
static void			LoadNSData(void);
static void			LoadDictionaries(void);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

ConstNSData *  gConstNSData;
char *			gDictData;
int				gFramesInitialized = NO;
TaskGlobals		gMyTaskGlobals;
NewtGlobals		gMyNewtGlobals;

#if defined(forFramework)
// Stubs
const Ref *		gTagCache;
NewtGlobals *	gNewtGlobals;
void *			gCurrentGlobals;
void *			GetGlobals(void)  { return gCurrentGlobals; }


int		GetCPUMode(void)	{ return kUserMode; }
void		Wait(ULong inMilliseconds) { }
ULong		GetTicks(void) {
	CTime now(GetGlobalTime());
	return now.convertTo(kMilliseconds)/20;
}

// RefStack allocates a new stack, but we don’t want the whole VM system
extern "C" NewtonErr	NewStack(ObjectId inDomainId, size_t inMaxSize, ObjectId inOwnerId, VAddr * outTopOfStack, VAddr * outBottomOfStack)
{
	if (inMaxSize == 64*KByte)
		inMaxSize = 4*KByte;
	*outBottomOfStack = (VAddr)malloc(inMaxSize);
	*outTopOfStack = *outBottomOfStack + inMaxSize - 1;
	return noErr;
}

// RefStack frees its stack, but we don’t want the whole VM system
extern "C" NewtonErr	FreePagedMem(VAddr inAddressInArea)
{
	free((void *)inAddressInArea);
	return noErr;
}
#if 1
/*------------------------------------------------------------------------------
	S t u b s
	The framework has no view system -- provide stubs.
	Needed only for IA.CC : ParseUtter
------------------------------------------------------------------------------*/

CRootView * gRootView;

void		CRootView::update(Rect * inRect) { }


extern "C" {
Ref	FOpenX(RefArg inRcvr);
Ref	FSetValue(RefArg inRcvr, RefArg inView, RefArg inTag, RefArg inValue);
}

Ref
FOpenX(RefArg inRcvr)
{ return NILREF; }

Ref
FSetValue(RefArg inRcvr, RefArg inView, RefArg inTag, RefArg inValue)
{ return NILREF; }

#endif
#endif	// forFramework


/*------------------------------------------------------------------------------
	O b j e c t   S y s t e m   I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/

void
InitObjectSystem(void)
{
	if (!gFramesInitialized)
	{
#if defined(forFramework)
	// Simulate OsBoot() sequence

	// Set up task globals
		gMyTaskGlobals.fExceptionGlobals.firstCatch = NULL;
		gMyTaskGlobals.fTaskId = 0;
		gMyTaskGlobals.fErrNo = noErr;
		gMyTaskGlobals.fMemErr = noErr;
		gMyTaskGlobals.fTaskName = 'newt';
		gMyTaskGlobals.fStackTop = 0;
		gMyTaskGlobals.fStackBottom = 0;
		gMyTaskGlobals.fCurrentHeap = 0;
		gMyTaskGlobals.fDefaultHeapDomainId = 0;
		gCurrentGlobals = &gMyTaskGlobals + 1;	// globals are AFTER task globals

	//	UserBoot task
		StartupProtocolRegistry();
		InitROMDomainManager();

	//	InitialKSRVTask task
		RegisterROMDomainManager();

	// CLoader::theMain -> UserMain -> CNewtWorld::mainConstructor()
		SetNewtGlobals(&gMyNewtGlobals);

		InitPSSManager(0, 0);

		InitObjects();
		InitTranslators();			// register flatten/unflatten etc

		NTKInit();
		gREPin = InitREPIn();
		gREPout = InitREPOut();
		ResetREPIdler();
		REPInit();

		InitUnicode();
		InitExternal();
		InitGraf();

	// CNotebook::initToolbox()
		InitScriptGlobals();
		InitInternationalUtils();
#if defined(forDarkStar)
		InitDictionaries();		//	should be gRecognitionManager.init(2);
#endif
		RunInitScripts();
#if defined(forDarkStar)
		InitDarkStar(RA(gFunctionFrame), RA(NILREF));

	// CNotebook::init()
		gRootView = (CRootView *)calloc(0xA0, 1);
		gRootView->fContext.h = AllocateRefHandle(AllocateFrame());
		gRootView->fContext.h->stackPos = 0;
	// stub up the root view -- can’t gRootView->init(SYS_rootProto, NULL) w/o bringing in a world of stuff we don’t really want
		RefVar assistant(AllocateFrame());
		SetFrameSlot(assistant, SYMA(matchString), RA(NILREF));
		SetFrameSlot(assistant, SYMA(assistLine), AllocateFrame());
		gRootView->fContext = AllocateFrame();
		SetFrameSlot(gRootView->fContext, SYMA(assistant), assistant);
#endif
//		Override defaults set in REPInit()
		SetFrameSlot(RA(gVarFrame), SYMA(printLength), MAKEINT(15));	// nil => no limit; for fuller debug
//		SetFrameSlot(RA(gVarFrame), SYMA(printDepth), RA(NILREF));	// nil => no limit; for fuller debug
#else
		ResetFromResetSwitch();
#endif
		gFramesInitialized = YES;
	}
}


/*------------------------------------------------------------------------------
	Simulate NewtonScript ROM resources by loading NTK .pkg
	and creating Refs from the elements in its array.
------------------------------------------------------------------------------*/
extern Ref * RSstorePrototype;
extern Ref * RScursorPrototype;
extern Ref * RS_clicks;

void
MakeROMResources(void)
{
	LoadNSData();
	InitStartupHeap();
	FixStartupHeap();
// Can only use FOREACH after globals have been set up
	InitBuiltInFunctions();
// Hack in parent function frames
	((FrameObject *)PTR(*RSstorePrototype))->slot[0] = gConstNSData->storeParent;
	((FrameObject *)PTR(*RSplainSoupPrototype))->slot[0] = gConstNSData->soupParent;
	((FrameObject *)PTR(*RScursorPrototype))->slot[0] = gConstNSData->cursorParent;
// and click song
	*RS_clicks = gConstNSData->clicks;
#if defined(forDarkStar)
	LoadDictionaries();
#endif
}


/*------------------------------------------------------------------------------
	Mimic package installation by following object refs from each part base and
	fixing up absolute addresses.  Create a Ref for each root frame/array in
	each part.

	NOTE!
	NTK generates 32-bit Refs; we need to rebuild all Refs for a 64-bit world.

	ALSO NOTE!
	NCX is going to import packages with 32-bit Refs so we can’t have a 64-bit
	world for that.

	Args:		inPkgName
				outRef
	Return:	--
------------------------------------------------------------------------------*/

static void
LoadNSData(void)
{
	PackageDirectory dir;
	RelocationHeader hdr;
	ULong baseOffset;
	size_t relocationDataSize, relocationBlockSize;
	char * relocationData = NULL;

	CFBundleRef newtBundle = CFBundleGetBundleWithIdentifier(kBundleId);
	CFURLRef url = CFBundleCopyResourceURL(newtBundle, kNSDataPkg, NULL, NULL);
	CFStringRef pkgPath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
//	const char * pathStr = CFStringGetCStringPtr(pkgPath, kCFStringEncodingMacRoman);	// can return NULL if space in the path
	char pathStr[256];
	CFStringGetCString(pkgPath, pathStr, 256, kCFStringEncodingUTF8);							// so use this instead

	FILE * pkgFile = fopen(pathStr, "r");
	CFRelease(pkgPath);
	CFRelease(url);

	if (pkgFile == NULL)
		return;

	// read the directory
	fread(&dir, sizeof(PackageDirectory), 1, pkgFile);
	baseOffset = CANONICAL_LONG(dir.directorySize);

	//	if it’s a "package1" with relocation info then read that relocation info
	if ((dir.signature[7] == '1') && (CANONICAL_LONG(dir.flags) & kRelocationFlag))
	{
		// read relocation header
		fseek(pkgFile, offsetof(PackageDirectory, directorySize), SEEK_SET);
		fread(&hdr, sizeof(RelocationHeader), 1, pkgFile);

		// read relocation data into memory
		relocationBlockSize = CANONICAL_LONG(hdr.relocationSize);
		relocationDataSize = relocationBlockSize - sizeof(RelocationHeader);
		relocationData = (char *)malloc(relocationDataSize);
		if (relocationData == NULL)
			return;
		fread(relocationData, relocationDataSize, 1, pkgFile);
    
		// update the base offset past the relocation info to the part data
		baseOffset += relocationBlockSize;
	}
	else
		hdr.relocationSize = 0;

	// there’d better be only one part
	for (ArrayIndex partNum = 0, partCount = CANONICAL_LONG(dir.numParts); partNum < partCount; partNum++)
	{
		PartEntry thePart;

		// read PartEntry for the part
		fseek(pkgFile, offsetof(PackageDirectory, parts) + partNum * sizeof(PartEntry), SEEK_SET);
		fread(&thePart, sizeof(PartEntry), 1, pkgFile);

		// read part into memory
		size_t partSize = CANONICAL_LONG(thePart.size);
		char * partData = (char *)malloc(partSize);
		if (partData == NULL)
			return;
		fseek(pkgFile, baseOffset + CANONICAL_LONG(thePart.offset), SEEK_SET);
		fread(partData, partSize, 1, pkgFile);

		// locate the part frame
		ArrayObject32 * pkgRoot = (ArrayObject32 *)partData;
		ULong alignment = (pkgRoot->gc.stuff & k4ByteAlignmentFlag) != 0 ? 4 : 8;		// 4-byte | 8-byte alignment defined by root object
/*
	So, now:
		partData = address of part file read into memory
		baseOffset = offset into part of Ref data
	Refs in the .pkg file are offsets into Ref data and need to be fixed up to run-time addresses
*/
#if __LP64__
		size_t newPartSize = partSize + 8 + ScanRef(&pkgRoot->slot[0], partData - baseOffset);	// fix up byte order but leave Refs as offsets
		char * newPartData = (char *)malloc(newPartSize);
		if (newPartData == NULL)
		{
			free(partData);
			return;
		}

		ArrayObject * newRoot = (ArrayObject *)newPartData;
		ArrayObject32 * srcPtr = pkgRoot;
		ArrayObject * dstPtr = newRoot;
		RefMap map;
		while ((char *)srcPtr < partData + partSize)
		{
			std::pair<Ref32, Ref> mapping = std::make_pair(REF(((char *)srcPtr - (partData - baseOffset))), REF(dstPtr));
			map.insert(mapping);
			CopyRef(srcPtr, dstPtr, map, partData - baseOffset, alignment);
		}

		UpdateRef(&newRoot->slot[0], map);	// Ref offsets -> addresses

		// point to the 64-bit refs we want
		Ref partFrame = newRoot->slot[0];
		gConstNSData = (ConstNSData *)&((ArrayObject *)PTR(partFrame))->slot[0];

#else
		// fix up those refs from offsets to addresses, and do byte-swapping as appropriate
		FixUpRef(&pkgRoot->slot[0], partData - baseOffset);

		// point to the refs we want
		Ref partFrame = pkgRoot->slot[0];
		gConstNSData = (ConstNSData *)&((ArrayObject *)PTR(partFrame))->slot[0];
#endif

		// adjust any relocation info
		if (hdr.relocationSize != 0)
		{
			long  delta = (long) partData - CANONICAL_LONG(hdr.baseAddress);
			RelocationEntry *	set = (RelocationEntry *)relocationData;
			for (ArrayIndex setNum = 0, setCount = CANONICAL_LONG(hdr.numEntries); setNum < setCount; ++setNum)
			{
				int32_t * p, * pageBase = (int32_t *)(partData + CANONICAL_SHORT(set->pageNumber) * CANONICAL_LONG(hdr.pageSize));
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
			free(relocationData);
		}
#if __LP64__
		// don’t need the 32-bit part data any more
		free(partData);
#endif
	}

	fclose(pkgFile);
}


/*------------------------------------------------------------------------------
	Fix up a ref.
	For pointer refs, fix up the class.
	For slotted refs, fix up all slots recursively.

	Args:		inRefPtr		pointer to ref to be fixed up = offset from inBaseAddr
				inBaseAddr	base address of package from which refs are offsets
	Return:	--
------------------------------------------------------------------------------*/

#if defined(hasByteSwapping)
#define BYTE_SWAP_SIZE(n) (((n << 16) & 0x00FF0000) | (n & 0x0000FF00) | ((n >> 16) & 0x000000FF))

bool
IsObjClass(Ref obj, const char * inClassName)
{
	if (ISPTR(obj) && ((SymbolObject *)PTR(obj))->objClass == kSymbolClass)
	{
		char * subName = SymbolName(obj);
		for ( ; *subName && *inClassName; subName++, inClassName++)
		{
			if (tolower(*subName) != tolower(*inClassName))
				return NO;
		}
		return (*inClassName == 0 && (*subName == 0 || *subName == '.'));
	}
	return NO;
}
#endif


void
FixUpRef(Ref32 * inRefPtr, char * inBaseAddr)
{
#if defined(hasByteSwapping)
	// byte-swap ref in memory
	*inRefPtr = BYTE_SWAP_LONG(*inRefPtr);
#endif

	if (ISREALPTR(*inRefPtr) && *inRefPtr < (Ref)inBaseAddr)	// assume offset MUST be less than address
	{
		// ref has not been fixed from offset to address, so do it
		*inRefPtr += (Ref) inBaseAddr;

		ArrayObject32 *  obj = (ArrayObject32 *)PTR(*inRefPtr);
		// if gc set, this object has already been fixed up
		if (obj->gc.stuff < 4)
		{
			obj->gc.stuff = 4;
#if defined(hasByteSwapping)
			obj->size = BYTE_SWAP_SIZE(obj->size);
#endif
			// for frames, class is actually the map which needs fixing too; non-pointer refs may need byte-swapping anyway so we always need to do this
			FixUpRef(&obj->objClass, inBaseAddr);

			//	if it’s a frame / array, step through each slot / element fixing those
			if ((obj->flags & kObjSlotted))
			{
				Ref32 * refPtr = &obj->slot[0];
				for (ArrayIndex count = ARRAYLENGTH(obj); count > 0; --count, ++refPtr)
				{
					FixUpRef(refPtr, inBaseAddr);
				}
			}
#if defined(hasByteSwapping)
			// else object is some form of binary; symbol, string, real or encoding table (for example) which needs byte-swapping
			else
			{
				Ref objClass = obj->objClass;
				if (objClass == kSymbolClass)
				{
					// symbol -- byte-swap hash
					SymbolObject * sym = (SymbolObject *)obj;
					sym->hash = CANONICAL_LONG(sym->hash);
				}
				else if (IsObjClass(objClass, "string"))
				{
					// string -- byte-swap UniChar characters
					StringObject * str = (StringObject *)obj;
					UniChar * s = (UniChar *)str->str;
					UniChar * eos = s + (str->size - SIZEOF_BINARYOBJECT)/sizeof(UniChar);
					while (s < eos)
						*s = BYTE_SWAP_SHORT(*s), s++;
				}
				else if (IsObjClass(objClass, "real"))
				{
					// real number -- byte-swap 64-bit double
					uint32_t tmp;
					uint32_t * dbp = (uint32_t *)((BinaryObject *)obj)->data;
					tmp = BYTE_SWAP_LONG(dbp[1]);
					dbp[1] = BYTE_SWAP_LONG(dbp[0]);
					dbp[0] = tmp;
				}
				else if (IsObjClass(objClass, "UniC"))
				{
					// EncodingMap -- byte-swap UniChar characters
					UShort * table = (UShort *)&obj->slot[0];
					UShort formatId, unicodeTableSize;

					formatId = BYTE_SWAP_SHORT(*table);
					*table = formatId, table++;

					if (formatId == 0)
					{
						// it’s 8-bit to UniCode
						unicodeTableSize = BYTE_SWAP_SHORT(*table);
						*table = unicodeTableSize, table++;
						*table = BYTE_SWAP_SHORT(*table), table++;		// revision
						*table = BYTE_SWAP_SHORT(*table), table++;		// tableInfo
						for (ArrayIndex i = 0; i < unicodeTableSize; ++i)
							*table = BYTE_SWAP_SHORT(*table), table++;
					}
					else if (formatId == 4)
					{
						// it’s UniCode to 8-bit
						*table = BYTE_SWAP_SHORT(*table), table++;		// revision
						*table = BYTE_SWAP_SHORT(*table), table++;		// tableInfo
						unicodeTableSize = BYTE_SWAP_SHORT(*table);
						*table++ = unicodeTableSize;
						for (ArrayIndex i = 0; i < unicodeTableSize*3; ++i)
							*table = BYTE_SWAP_SHORT(*table), table++;
					}
				}
			}
#endif
		}
	}
}


#if __LP64__

#if defined(hasByteSwapping)
bool
IsSymClass(SymbolObject32 * inSym, const char * inClassName)
{
	char * subName = inSym->name;
	for ( ; *subName && *inClassName; subName++, inClassName++)
	{
		if (tolower(*subName) != tolower(*inClassName))
			return NO;
	}
	return (*inClassName == 0 && (*subName == 0 || *subName == '.'));
}
#endif

// Refs are offsets and in big-endian byte-order
// pass 1: calculate extra space needed for 64-bit refs
//			  no modifications to Ref data
size_t
ScanRef(Ref32 * inRefPtr, char * inBaseAddr)
{
	size_t extra = 4;		// we start with a 32-bit ref -> 64-bit
	Ref32 ref = *inRefPtr;
#if defined(hasByteSwapping)
	ref = BYTE_SWAP_LONG(ref);
#endif

	if (ISREALPTR(ref))
	{
		ArrayObject32 *  obj = (ArrayObject32 *)PTR(ref + inBaseAddr);
		// if gc set, this object has already been scanned
		if (obj->gc.stuff == 0)
		{
			obj->gc.stuff = 1;
			extra += 4;		// for 64-bit gc.destRef

			// for frames, class is actually the map which needs fixing too; non-pointer refs may need byte-swapping anyway so we always need to do this
			extra += ScanRef(&obj->objClass, inBaseAddr);

			//	if it’s a frame / array, step through each slot / element fixing those
			if ((obj->flags & kObjSlotted))
			{
				ArrayIndex objSize = obj->size;
#if defined(hasByteSwapping)
				objSize = BYTE_SWAP_SIZE(objSize);
#endif
				Ref32 * refPtr = &obj->slot[0];
				for (ArrayIndex count = (objSize - 12) / sizeof(Ref32); count > 0; --count, ++refPtr)
				{
					extra += ScanRef(refPtr, inBaseAddr);
				}
			}
		}
	}
	return extra;
}


// pass 2: write out 64-bit refs
//			  adjust byte-order while we’re at it
void
CopyRef(ArrayObject32 * &inSrcPtr, ArrayObject * &inDstPtr, RefMap & inMap, char * inBaseAddr, ULong inAlignment)
{
/*
	assume data is sequence of long-aligned pointer refs
	until end-of-data
		map original Ref32 to REF(64bitRefSpace)
		write uint32_t size, Ref gc, Ref class; if slotted update size, write slot Refs else write data
		align src, dst pointers
*/
	size_t objSize = inSrcPtr->size;
#if defined(hasByteSwapping)
	objSize = BYTE_SWAP_SIZE(objSize);
#endif
	// remember where we started
	char * srcp = (char *)inSrcPtr;
	size_t srcsize = objSize;

	inDstPtr->size = objSize + 8;		// adjust for gc,class -> 64-bit
	inDstPtr->flags = inSrcPtr->flags;
	inDstPtr->gc.stuff = 0;
	inDstPtr->objClass = CANONICAL_LONG(inSrcPtr->objClass);
	Ref32 * srcPtr = &inSrcPtr->slot[0];
	Ref * dstPtr = &inDstPtr->slot[0];
	if ((inSrcPtr->flags & kObjSlotted))
	{
		ArrayIndex count = (objSize - 12) / sizeof(Ref32);
		inDstPtr->size += count*sizeof(Ref32);	// adjust for slots -> 64-bit
		for ( ; count > 0; --count, ++srcPtr, ++dstPtr)
		{
			*dstPtr = CANONICAL_LONG(*srcPtr);
		}
	}
	else
	{
		// else object is some form of binary; symbol, string, real or encoding table (for example) which needs byte-swapping
		objSize -= 12;
		memcpy(dstPtr, srcPtr, objSize);
		srcPtr = (Ref32 *)((char *)srcPtr + objSize);
		dstPtr = (Ref *)((char *)dstPtr + objSize);

#if defined(hasByteSwapping)
		SymbolObject32 * sym;
		Ref objClass = inDstPtr->objClass;
		if (objClass == kSymbolClass)
		{
			// symbol -- byte-swap hash
			SymbolObject * sym = (SymbolObject *)inDstPtr;		// ooh, tricky, name overloading!
			sym->hash = BYTE_SWAP_LONG(sym->hash);
		}
		else if (ISPTR(objClass) && (sym = ((SymbolObject32 *)PTR(objClass + inBaseAddr))), sym->objClass == CANONICAL_LONG(kSymbolClass))
		{
			if (IsSymClass(sym, "string"))
			{
				// string -- byte-swap UniChar characters
				StringObject * str = (StringObject *)inDstPtr;
				UniChar * s = (UniChar *)str->str;
				UniChar * eos = s + (str->size - SIZEOF_BINARYOBJECT)/sizeof(UniChar);
				while (s < eos)
					*s = BYTE_SWAP_SHORT(*s), s++;
			}
			else if (IsSymClass(sym, "real"))
			{
				// real number -- byte-swap 64-bit double
				uint32_t tmp;
				uint32_t * dbp = (uint32_t *)((BinaryObject *)inDstPtr)->data;
				tmp = BYTE_SWAP_LONG(dbp[1]);
				dbp[1] = BYTE_SWAP_LONG(dbp[0]);
				dbp[0] = tmp;
			}
			else if (IsSymClass(sym, "UniC"))
			{
				// EncodingMap -- byte-swap UniChar characters
				UShort * table = (UShort *)&inDstPtr->slot[0];
				UShort formatId, unicodeTableSize;

				formatId = BYTE_SWAP_SHORT(*table);
				*table = formatId, table++;

				if (formatId == 0)
				{
					// it’s 8-bit to UniCode
					unicodeTableSize = BYTE_SWAP_SHORT(*table);
					*table = unicodeTableSize, table++;
					*table = BYTE_SWAP_SHORT(*table), table++;		// revision
					*table = BYTE_SWAP_SHORT(*table), table++;		// tableInfo
					for (ArrayIndex i = 0; i < unicodeTableSize; ++i)
						*table = BYTE_SWAP_SHORT(*table), table++;
				}
				else if (formatId == 4)
				{
					// it’s UniCode to 8-bit
					*table = BYTE_SWAP_SHORT(*table), table++;		// revision
					*table = BYTE_SWAP_SHORT(*table), table++;		// tableInfo
					unicodeTableSize = BYTE_SWAP_SHORT(*table);
					*table++ = unicodeTableSize;
					for (ArrayIndex i = 0; i < unicodeTableSize*3; ++i)
						*table = BYTE_SWAP_SHORT(*table), table++;
				}
			}
		}
#endif

	}
	inSrcPtr = (ArrayObject32 *)(srcp + ALIGN(srcsize, inAlignment));	// source objects might be 8-byte aligned
	inDstPtr = (ArrayObject *)LONGALIGN(dstPtr);		// destination objects are always 4-byte aligned
}


// pass 3: fix up 64-bit refs at new addresses
void
UpdateRef(Ref * inRefPtr, RefMap & inMap)
{
	Ref ref = *inRefPtr;
	if (ISREALPTR(ref))
	{
		RefMap::iterator mapping = inMap.find(ref);
		if (mapping != inMap.end())
		{
			*inRefPtr = mapping->second;
			ArrayObject *  obj = (ArrayObject *)PTR(*inRefPtr);
			// for frames, class is actually the map which needs fixing too; non-pointer refs may need byte-swapping anyway so we always need to do this
			UpdateRef(&obj->objClass, inMap);

			//	if it’s a frame / array, step through each slot / element fixing those
			if ((obj->flags & kObjSlotted))
			{
				Ref * refPtr = &obj->slot[0];
				for (ArrayIndex count = ARRAYLENGTH(obj); count > 0; --count, ++refPtr)
				{
					UpdateRef(refPtr, inMap);
				}
			}
		}
	}
}

#endif


/*------------------------------------------------------------------------------
	Fix up the funcPtrs in plain C functions to their addresses in this
	invocation.
	NOTE  that because function addresses are long-aligned, they appear as
			NS integers (albeit shifted down). Neat, huh?
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
InitBuiltInFunctions(void)
{
	CFBundleRef newtBundle = CFBundleGetBundleWithIdentifier(kBundleId);
	CFStringRef functionName;
	Ref functionPtr;

	FOREACH_WITH_TAG(gConstNSData->plainCFunctions, fnTag, fnFrame)
		SymbolObject * fnTagRec = (SymbolObject *)PTR(fnTag);
		FrameObject * fnFrameRec = (FrameObject *)PTR(fnFrame);
		functionName = CFStringCreateWithCString(NULL, fnTagRec->name, kCFStringEncodingMacRoman);
		functionPtr = (Ref) CFBundleGetFunctionPointerForName(newtBundle, functionName);
#if defined(forNTK)
		if (functionPtr == 0)
		{
			char fname[32];
			strcpy(fname, fnTagRec->name);
			fname[0] &= ~0x20;	// Capitalize the name -- may share a lowercase symbol name
			CFRelease(functionName);
			functionName = CFStringCreateWithCString(NULL, fname, kCFStringEncodingMacRoman);
			functionPtr = (Ref) CFBundleGetFunctionPointerForName(newtBundle, functionName);
		}
#endif
#if defined(forDebug)
if (functionPtr == 0) printf("%s: nil function pointer, frame 0x%08X\n", fnTagRec->name, fnFrameRec);
printf("%s: #%08X\n", fnTagRec->name, functionPtr);
#endif
		fnFrameRec->slot[1] = functionPtr;
		CFRelease(functionName);
	END_FOREACH	
}


/*------------------------------------------------------------------------------
	Load recognition/IA dictionaries.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/
#if defined(forDarkStar)
static void
LoadDictionaries(void)
{
	CFBundleRef newtBundle = CFBundleGetBundleWithIdentifier(kBundleId);
	CFURLRef		url = CFBundleCopyResourceURL(newtBundle, kNSDictData, NULL, NULL);
	CFStringRef dictPath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
	const char * dictStr = CFStringGetCStringPtr(dictPath, kCFStringEncodingMacRoman);
	size_t dictSize;

	FILE * fp = fopen(dictStr, "r");

	CFRelease(dictPath);
	CFRelease(url);

	if (fp == NULL)
		return;

	fseek(fp, 0, SEEK_END);
	dictSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	gDictData = (char *)malloc(dictSize);
	fread(gDictData, dictSize, 1, fp);

	fclose(fp);
}
#endif


#include <CoreGraphics/CoreGraphics.h>

CGImageRef
LoadPNG(const char * inName)
{
	CFBundleRef newtBundle = CFBundleGetBundleWithIdentifier(kBundleId);
	CFStringRef name = CFStringCreateWithCString(NULL, inName, kCFStringEncodingASCII);
	CFURLRef url = CFBundleCopyResourceURL(newtBundle, name, CFSTR("png"), NULL);
	CFRelease(name);
	CGDataProviderRef dataProvider = CGDataProviderCreateWithURL(url);
	CFRelease(url);
	CGImageRef png = CGImageCreateWithPNGDataProvider(dataProvider, NULL, NO, kCGRenderingIntentDefault);
	CFRelease(dataProvider);
	return png;
}

