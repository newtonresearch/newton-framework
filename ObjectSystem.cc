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
#include "ROMResources.h"
#include "ROMData.h"

#if defined(forFramework)
#define kBundleId CFSTR("com.newton.objects")
#else
#define kBundleId CFSTR("com.newton.messagepad")
#endif

#define kNSDictData CFSTR("DictData")

#if defined(forNTK)
#define kNSDataPkg CFSTR("NTKConstData.newtonpkg")
#else
#define kNSDataPkg CFSTR("NSConstData.newtonpkg")
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
Ref	FRefreshViews(RefArg inRcvr);
}

Ref
FOpenX(RefArg inRcvr)
{ return NILREF; }

Ref
FSetValue(RefArg inRcvr, RefArg inView, RefArg inTag, RefArg inValue)
{ printf("SetValue(view, "); PrintObject(inTag, 0); printf(", "); PrintObject(inValue, 0); printf(")\n"); return NILREF; }

Ref
FRefreshViews(RefArg inRcvr)
{ return TRUEREF; }

#endif
#endif	// forFramework


/*------------------------------------------------------------------------------
	O b j e c t   S y s t e m   I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/
extern "C" Ref	GetProtoVariable(RefArg context, RefArg name, bool * exists = NULL);

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

//		Override debug print defaults set in init script
		SetFrameSlot(RA(gVarFrame), SYMA(printDepth), MAKEINT(7));

#if defined(forDarkStar)
		InitDarkStar(RA(gFunctionFrame), RA(NILREF));

	// CNotebook::init()
	// stub up the root view -- can’t gRootView->init(SYS_rootProto, NULL) w/o bringing in a world of stuff we don’t really want
		gRootView = (CRootView *)calloc(0xA0, 1);
		gRootView->fContext.h = AllocateRefHandle(AllocateFrame());
		gRootView->fContext.h->stackPos = 0;

		gRootView->fContext = Clone(gConstNSData->rootContext);	// must make it mutable
		SetFrameSlot(gRootView->fContext, SYMA(_proto), RA(rootProto));
		RefVar vwChildren(GetFrameSlot(RA(rootProto), SYMA(viewChildren)));
		FOREACH(vwChildren, child)
			RefVar contextTag(GetFrameSlot(child, SYMA(preallocatedContext)));
			if (NOTNIL(contextTag))
			{
				RefVar mutableChild(Clone(child));
				SetFrameSlot(gRootView->fContext, contextTag, mutableChild);
				if (SymbolCompareLexRef(contextTag, RSYMassistant) == 0)
				{
					RefVar assistLine(AllocateFrame());
					SetFrameSlot(assistLine, SYMA(entryLine), AllocateFrame());
					SetFrameSlot(mutableChild, SYMA(assistLine), assistLine);
				}
			}
		END_FOREACH;
#endif
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

------------------------------------------------------------------------------*/
#include "NewtonPackage.h"

static void
LoadNSData(void)
{
	CFBundleRef newtBundle = CFBundleGetBundleWithIdentifier(kBundleId);
	CFURLRef url = CFBundleCopyResourceURL(newtBundle, kNSDataPkg, NULL, NULL);
	CFStringRef pkgPath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
//	const char * pathStr = CFStringGetCStringPtr(pkgPath, kCFStringEncodingMacRoman);	// can return NULL if space in the path
	char pathStr[256];
	CFStringGetCString(pkgPath, pathStr, 256, kCFStringEncodingUTF8);							// so use this instead

	NewtonPackage pkg(pathStr);
	gConstNSData = (ConstNSData *)&((ArrayObject *)PTR(pkg.partRef(0)))->slot[0];

	CFRelease(pkgPath);
	CFRelease(url);
}


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

