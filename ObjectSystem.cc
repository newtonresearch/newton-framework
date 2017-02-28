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
#include "Interpreter.h"

#define forDebug 1
//#define forExperiment 1

#if defined(forFramework)
#define kBundleId CFSTR("org.newton.objects")
#else
#define kBundleId CFSTR("org.newton.messagepad")
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

		 void			MakeROMResources(void);
static void			InitBuiltInFunctions(void);
static void			LoadNSData(void);
static void			LoadDictionaries(void);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

void *			gConstNSDataStart, * gConstNSDataEnd;

ConstNSData *  gConstNSData;
char *			gDictData;
int				gFramesInitialized = false;
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

extern "C" Ref FAddDeferredSend(RefArg inRcvr, RefArg inTarget, RefArg inMsg, RefArg inArg) { return NILREF; }
extern "C" Ref FAddDeferredCall(RefArg inRcvr, RefArg inMsg, RefArg inArg) { return NILREF; }

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
		SetFrameSlot(gRootView->fContext, SYMA(_proto), RA(viewRoot));
		RefVar vwChildren(GetFrameSlot(RA(viewRoot), SYMA(viewChildren)));
		FOREACH(vwChildren, child)
			RefVar contextTag(GetFrameSlot(child, SYMA(preallocatedContext)));
			if (NOTNIL(contextTag))
			{
				RefVar mutableChild(Clone(child));
				SetFrameSlot(gRootView->fContext, contextTag, mutableChild);
				if (SymbolCompareLexRef(contextTag, SYMA(assistant)) == 0)
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
		gFramesInitialized = true;
	}
}


/*------------------------------------------------------------------------------
	Simulate NewtonScript ROM resources by loading NTK .pkg
	and creating Refs from the elements in its array.
------------------------------------------------------------------------------*/

void
MakeROMResources(void)
{
	LoadNSData();
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

	gConstNSDataStart = pkg.partPkgData(0)->data;
	gConstNSDataEnd = (char *)gConstNSDataStart + pkg.partPkgData(0)->size;

	CFRelease(pkgPath);
	CFRelease(url);
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

	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		dictSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		gDictData = (char *)malloc(dictSize);
		fread(gDictData, dictSize, 1, fp);
		fclose(fp);
	}
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
	CGImageRef png = CGImageCreateWithPNGDataProvider(dataProvider, NULL, false, kCGRenderingIntentDefault);
	CFRelease(dataProvider);
	return png;
}

