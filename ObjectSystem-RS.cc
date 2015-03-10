/*
	File:		ObjectSystem.cc

	Contains:	Newton object system framework initialization.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "PackageParts.h"
#include "Globals.h"
#include "NewtWorld.h"
#include "Iterators.h"
#include "REP.h"

#undef __MACTYPES__
#include "PrivateMacTypes.h"
#include <CoreFoundation/CoreFoundation.h>

#if defined(forFramework)
#define kBundleId CFSTR("com.newton.objects")
#else
#define kBundleId CFSTR("com.newton.messagepad")
#endif

#if defined(forNTK)
#define kNSDataPkg CFSTR("NTKConstData.pkg")
#else
#define kNSDataPkg CFSTR("NSConstData.pkg")
#endif

#define kNSDictData CFSTR("DictData")

extern PInTranslator *	gREPin;
extern POutTranslator *	gREPout;

extern void			UserInit(void);
extern void			StartupProtocolRegistry(void);
extern NewtonErr	InitROMDomainManager(void);
extern NewtonErr	RegisterROMDomainManager(void);
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
void  InitStartupHeap(void);
void  FixStartupHeap(void);
}
		 void			MakeROMResources(void);
static void			InitBuiltInFunctions(void);
static void			LoadNSData(void);
static void			FixUpRef(Ref * inRefPtr, Ptr inBaseAddr);
static void			LoadDictionaries(void);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

ConstNSData *  gConstNSData;
char *			gDictData;

#if defined(forFramework) //|| defined(forOSFramework)
TaskGlobals		gMyTaskGlobals;
NewtGlobals		gMyNewtGlobals;
int				gFramesInitialized = NO;

// Stubs
//long		gScreenWidth;
//long		gScreenHeight;

const Ref *		gTagCache;
NewtGlobals *	gNewtGlobals;
void *			gCurrentGlobals;
void *			GetGlobals(void)  { return gCurrentGlobals; }

//extern "C" Ref	FGetLCDContrast(RefArg inRcvr)  { return MAKEINT(2); }
//extern "C" Ref FSetLCDContrast(RefArg inRcvr, RefArg inContrast)  { return NILREF; }
//void		InitScreen(void) { }

int		GetCPUMode(void)	{ return kUserMode; }
void		Wait(ULong inMilliseconds) { }
ULong		GetTicks(void) {
	CTime now(GetGlobalTime());
	return now.fTime.full / 20*kMilliseconds;
}


/*------------------------------------------------------------------------------
	O b j e c t   S y s t e m   I n i t i a l i z a t i o n
------------------------------------------------------------------------------*/

DeclareException(exMessage, exRootException);

#define kPrecedentStackSize	16

enum sTypeFlags
{
	isWhitespace	= 1,
	isPunctuation	= 1 << 1,
	isSpace			= 1 << 2,
	isLowercase		= 1 << 3,
	isUppercase		= 1 << 4,
	isDigit			= 1 << 5,
	isControl		= 1 << 6,
	isHex				= 1 << 7,
};

static const unsigned char	sType[256] = {
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x41, 0x41, 0x41, 0x41, 0x41, 0x40, 0x40,	// 0x00..0x0F
	0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,	// 0x10..0x1F
	0x05, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,	// 0x20..0x2F
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,	// 0x30..0x3F
	0x02, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,	// 0x40..0x4F
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x02, 0x02, 0x02, 0x02, 0x02,	// 0x50..0x5F
	0x02, 0x88, 0x88, 0x88, 0x88, 0x88, 0x88, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,	// 0x60..0x6F
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x02, 0x02, 0x02, 0x02, 0x40,	// 0x70..0x7F
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	// 0x80..
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const char * simpleInstrs[] =
{
	"pop",
	"dup",
	"return",
	"push-self",
	"set-lex-scope",
	"iter-next",
	"iter-done",
	"pop-handlers"
};

static const char * paramInstrs[] =
{
	"unary1",
	"unary2",
	"unary3",
	"push",
	"push-constant",
	"call",
	"invoke",
	"send",
	"send-if-defined",
	"resend",
	"resend-if-defined",
	"branch",
	"branch-t",
	"branch-f",
	"find-var",
	"get-var",
	"make-frame",
	"make-array",
	"get-path",
	"set-path",
	"set-var",
	"set-find-var",
	"incr-var",
	"branch-if-loop-not-done",
	"freq-func",
	"new-handlers"
};

static Ref myPrecedentStack;


extern int		REPprintf(const char * inFormat, ...);
extern "C" {
	ArrayIndex	GetFunctionArgCount(Ref fn);
	Ref	GetArraySlotRef(Ref obj, ArrayIndex index);
	void	SetArraySlotRef(Ref obj, ArrayIndex index, Ref value);
	void	SafelyPrintString(UniChar * str);
}


// Forward declarations
void PrintObjectBE(Ref obj, int indent, int depth);

char * ROMdata;


void
InitObjectSystem(void)
{
	if (!gFramesInitialized)
	{
	// Simulate task globals.
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
		//UserInit();

		StartupProtocolRegistry();
		InitROMDomainManager();
		RegisterROMDomainManager();

	// Do init from CNewtWorld::mainConstructor()
		SetNewtGlobals(&gMyNewtGlobals);

		InitializeCompression();
		InitObjects();
		InitTranslators();

		NTKInit();
		gREPin = InitREPIn();
		gREPout = InitREPOut();
		ResetREPIdler();
		REPInit();

		InitUnicode();
		InitExternal();
		InitGraf();

	// Do init from CNotebook::initToolbox()
		InitScriptGlobals();
		InitInternationalUtils();
#if defined(forNTK)
		InitDictionaries();		//	should be gRecognitionManager.init(2);
#endif
		RunInitScripts();
#if defined(forNTK)
		InitDarkStar(RA(gFunctionFrame), RA(NILREF));
#endif

		SetFrameSlot(RA(gVarFrame), SYMA(printLength), RA(NILREF));	// nil => no limit; for fuller debug
//		SetFrameSlot(RA(gVarFrame), SYMA(printDepth), RA(NILREF));	// nil => no limit; for fuller debug

		gFramesInitialized = YES;

/* ---- Print RSxxxx frames ---- */

		myPrecedentStack = MakeArray(kPrecedentStackSize);
		AddGCRoot(&myPrecedentStack);

		FILE * fp = fopen("/Users/simon/Projects/Newton/DisARM/build/Development/rom.image", "r");
		if (fp)
		{
			// read ROM image into memory
			size_t ROMsize;
			fseek(fp, 0, SEEK_END);
			ROMsize = ftell(fp);
			fseek(fp, 0, SEEK_SET);
			ROMdata = (char *) malloc(ROMsize);	// yup, all 8MB of it
			fread(ROMdata, ROMsize, 1, fp);
			fclose(fp);
/*
			// make ROM image little-endian
			ULong * p = (ULong *)ROMdata;
			for (int i = 0; i < ROMsize/4; i++)
			{
				ULong ul = *p;
				*p++ = BYTE_SWAP_LONG(ul);
			}
*/
			fp = fopen("/Users/simon/Projects/Newton/DisARM/build/Development/RSsymbols.txt", "r");
			if (fp)
			{
				Ref rRef;
				Ref * rsRef;
				char rsName[128];
				// scan address, name of frame
				while (fscanf(fp, "%8x %s\n", &rsRef, rsName) != EOF)
				{
					if (strncmp(rsName, "RS", 2) != 0)
					{
						rsRef = (Ref *)((long)rsRef + (long)ROMdata);
						rRef = *rsRef;
						printf("\n\n%s: ", rsName+1);
						PrintObjectBE(rRef, 0, 0);
					}
				}
			}
			fclose(fp);
		}

	}
}


#define BYTE_SWAP_24(n) (((n << 16) & 0x00FF0000) | ((n) & 0x0000FF00) | ((n >> 16) & 0x000000FF))

Ref
PatchPtrRef(Ref inRef)
{
// inRef MUST be little-endian
// if a pointer ref, convert object header’s flags, and if a slotted ref, convert object’s class

//	Ref obj = BYTE_SWAP_LONG(inRef);
	if (ISREALPTR(inRef))
	{
		// point into ROM image
		inRef += (Ref)ROMdata;
		// byte-swap obj header, and flag it so we don’t un-byte-swap it later
		ObjHeader * oh = PTR(inRef);
		if (oh->gc.stuff == 0)
		{
			ULong osize = oh->size;
			oh->size = BYTE_SWAP_24(osize);

			ULong oclass = ((BinaryObject *)oh)->objClass;
			oclass = BYTE_SWAP_LONG(oclass);
			if (ISREALPTR(oclass))
				oclass += (Ref)ROMdata;
			((BinaryObject *)oh)->objClass = oclass;

			oh->gc.stuff = 0xFFFFFFFF;
		}
	}
	return inRef;
}


Ref
PatchClassPtrRef(Ref inRef)
{
// inRef MUST be little-endian
// if a pointer ref, convert object header’s flags and class

	if (ISREALPTR(inRef))
	{
		// byte-swap obj header, and flag it so we don’t un-byte-swap it later
		ObjHeader * oh = PTR(inRef);
		if (oh->gc.stuff == 0)
		{
			ULong osize = oh->size;
			oh->size = BYTE_SWAP_24(osize);

			ULong oclass = ((BinaryObject *)oh)->objClass;
			oclass = BYTE_SWAP_LONG(oclass);
			if (ISREALPTR(oclass))
				oclass += (Ref)ROMdata;
			((BinaryObject *)oh)->objClass = oclass;

			if (oclass == kSymbolClass)
			{
				ULong ohash = ((SymbolObject *) oh)->hash;
				((SymbolObject *) oh)->hash = BYTE_SWAP_LONG(ohash);
			}

			oh->gc.stuff = 0xFFFFFFFF;
		}
	}
	return inRef;
}

Ref
PatchFrameMap(Ref inObj)
{
	FrameObject * oh = (FrameObject *) PTR(inObj);
	if (oh->flags & kObjFrame)
	{
		// map pointer ref is already patched
		// recurse over map and supermaps patching object headers
		FrameMapObject * map = (FrameMapObject *) PTR(oh->map);
		while (map)
		{
			if (map->gc.stuff == 0)
			{
				ULong osize = map->size;
				map->size = BYTE_SWAP_24(osize);
				map->gc.stuff = 0xFFFFFFFF;

				ULong oclass = map->objClass;
				oclass = BYTE_SWAP_LONG(oclass);
				if (ISREALPTR(oclass))
					oclass += (Ref)ROMdata;
				map->objClass = oclass;

				ULong supermap = map->supermap;
				supermap = BYTE_SWAP_LONG(supermap);
				if (ISREALPTR(supermap))
					supermap += (Ref)ROMdata;
				map->supermap = supermap;
				map = NOTNIL(supermap) ? (FrameMapObject *) PTR(supermap) : NULL;
			}
			else
				break;
		}
	}
}


Ref
MyClassOf(Ref r)
{
	long tag = RTAG(r);

	if (tag == kTagInteger)
		return RSYMint;

	if (tag == kTagImmed)
	{
		if (RIMMEDTAG(r) == kImmedChar)
			return RSYMchar;
		else if (RIMMEDTAG(r) == kImmedBoolean)
			return RSYMboolean;
		return RSYMweird_immediate;
	}

	if (tag & kTagPointer)
	{
		unsigned flags = ObjectFlags(r);
		if (flags & kObjSlotted)
		{
			if (flags & kObjFrame)
			{
/*				Ref theClass = GetProtoVariable(r, SYMA(class));
				if (NOTNIL(theClass))
				{
					if (theClass == kPlainFuncClass)
						return RSYM_function;
					else if (theClass == kPlainCFunctionClass
							|| theClass == kBinCFunctionClass)
						return RSYM_function_2Enative;
					return theClass;
				}
*/				return RSYMframe;
			}
			return ((ArrayObject *) ObjectPtr(r))->objClass;
		}
		else if (((BinaryObject *) ObjectPtr(r))->objClass == kWeakArrayClass)
			return RSYM_weakArray;
		else if (((SymbolObject *) ObjectPtr(r))->objClass == kSymbolClass)
			return RSYMsymbol;
		return PatchClassPtrRef(((BinaryObject *) ObjectPtr(r))->objClass);
	}

	return NILREF;	//	should never happen
}

#include "Unicode.h"
#include "UStringUtils.h"

void
SafelyPrintBEString(UniChar * str)
{
	UniChar ubuf[257];
	char	buf[256];
	int	bufLen, len = Ustrlen(str);

	memset(ubuf, 0, 257*sizeof(UniChar));
	while (len > 0)
	{
		bufLen = MIN(len, 255);
		memcpy(ubuf, (char *)str+1, bufLen*sizeof(UniChar));
		ConvertFromUnicode(ubuf, buf, kDefaultEncoding, bufLen);
		str += bufLen;
		len -= bufLen;
		REPprintf("%s", buf);
	}
}




void
PrintObjectBE(Ref obj, int indent, int depth)
{
// on entry, obj is big-endian read from ROM image

	Ref	objClass;
	ArrayIndex	i, count, numOfSlots;

//	obj = ForwardReference(obj);	ROM can’t forward reference
	obj = PatchPtrRef(BYTE_SWAP_LONG(obj));
//	at this point obj MUST be native: little-endian
	newton_try
	{
		switch (RTAG(obj))
		{
			case kTagInteger:
			// can only be an integer
				printf("%d", RINT(obj));
				break;

			case kTagImmed:
			// try boolean
				if (EQRef(obj, TRUEREF))
					printf("true");
				else if (ISNIL(obj))
					printf("nil");

			// try character
				else if (ISCHAR(obj))
				{
					UniChar	ch = RCHAR(obj);
					if (ch > 0xFF)
						printf("$\\u%04X", ch);
					else if (ch >= 0x20 && ch <= 0x7F)
					{
						if (ch == 0x5C)
							printf("$\\\\");
						else if (ch == 0x09)
							printf("$\\t");
						else if (ch == 0x0D)
							printf("$\\n");
						else
							printf("$%c", ch);
					}
					else
						printf("$\\%02X", ch);
				}

			// try weird immediate = special class
				else if (obj == kSymbolClass)
					printf("<symbol class>");
				else if (obj == kWeakArrayClass)
					printf("<weak array class>");
				else if (obj == kBadPackageRef)
					printf("<bad pkg ref>");

			// can't categorize it
				else
					printf("#%08X", obj);
				break;

			case kTagMagicPtr:
					printf("@%d", RVALUE(obj));
				break;

			case kTagPointer:
			// try precedents array first
				for (i = 0; i < depth && i < kPrecedentStackSize-1; i++)
					if (EQRef(obj, GetArraySlotRef(myPrecedentStack, i)))
						break;
				if (i < depth && i < kPrecedentStackSize-1)
					printf("<%d>", depth - i);

				else
				{
					Ref	prLenRef;
					int	printDepth, printLength;
					BOOL	doPrettyPrint;
					unsigned	flags;
					
					// add this object to the precedent stack
					if (depth < kPrecedentStackSize)
						SetArraySlotRef(myPrecedentStack, depth, obj);

					// get print parameters
					printDepth = RINT(GetFrameSlot(gVarFrame, SYMA(printDepth)));
					if (printDepth > kPrecedentStackSize-1)
						printDepth = kPrecedentStackSize-1;
					prLenRef = GetFrameSlot(gVarFrame, SYMA(printLength));
					printLength = NOTNIL(prLenRef) ? RINT(prLenRef) : -1;
					doPrettyPrint = NOTNIL(GetFrameSlot(gVarFrame, SYMA(prettyPrint)));
					// switch on type of object: slotted (frame/array) or binary
					flags = ObjectFlags(obj);
					if (flags & kObjSlotted)
					{
						if (flags & kObjFrame)
						{
//							FrameObject * fo = (FrameObject *)PTR(obj);
//							Ref fc = fo->slot[0];
//							fo->slot[0] = BYTE_SWAP_LONG(fo->slot[0]);
							if (NO /*IsFunction(obj) && doPrettyPrint*/)
							{
								int argCount = GetFunctionArgCount(obj);
								const char * argPlural = (argCount == 1) ? "" : "s";
								printf(IsNativeFunction(obj) ? "<native" : "<");
								printf("function, %ld arg%s, #%08X>", argCount, argPlural, obj);
/* DEBUG						if (NOTNIL(GetFrameSlot(gVarFrame, SYMA(printInstructions))))
									PrintInstructions(GetArraySlotRef(obj,1)); */	// we could probably do better with the formatting here
							}
							else if (depth > printDepth)
							{
								if (ISMAGICPTR(obj))
									printf("{@%ld}", RVALUE(obj));
								else
									printf("{#%08X}", obj);
							}
							else
							{
							// print slots in frame
								numOfSlots = Length(obj);
								indent += printf("{");
								if (doPrettyPrint && depth <= printDepth)
								{
									Ref	tag;
									BOOL	isAggr = YES;//NO;
									// determine whether any slot in frame is aggregate
									// and therefore this frame needs multi-line output
									PatchFrameMap(obj);
									CObjectIterator	iter(obj);
/*									for (i = 0; !iter.done() && !isAggr; iter.next(), i++)
									{
										if (printLength >= 0 && i >= printLength)
											// don’t exceed number-of-slots-to-print preference
											break;
										isAggr = IsAggregate(PatchPtrRef(iter.value()));
									}
*/									// increment depth for slots in this frame
									depth++;
									for (/*iter.reset(),*/ i = 0; !iter.done(); iter.next(), i++)
									{
										if (printLength >= 0 && i >= printLength)
										{
											// don’t exceed number-of-slots-to-print preference
											printf("...");
											break;
										}
										tag = iter.tag();
/* ---> */							tag = PatchPtrRef(BYTE_SWAP_LONG(tag));
										if ((EQRef(tag, RSYMviewCObject) && ISINT(BYTE_SWAP_LONG(iter.value())))
										 || (EQRef(tag, RSYMviewClipper) && ISINT(BYTE_SWAP_LONG(iter.value())))
										 || EQRef(tag, RSYMfuncPtr))
											printf("%s: #%08X", SymbolName(tag), RINT(iter.value()));
										else
										{
											count = printf("%s: ", SymbolName(tag));
											PrintObjectBE(iter.value(), indent + count, depth);
										}
										if (--numOfSlots > 0)
										{
										// separate slots
											printf(", ");
											if (isAggr)
												printf("\n%*s", indent, " ");
										}
									}
								}
								printf("}");
							}
						}

						// insanity check
						else if (flags & kObjForward)
							printf("<forwarding pointer #%08X...WTF!!!>", obj);

						else	// must be an array
						{
							objClass = MyClassOf(obj);
							if (EQRef(objClass, RSYMpathExpr))
							{
							// it’s a path expression
								BOOL	hasSymbol = NO;
								numOfSlots = Length(obj);
								for (i = 0; i < numOfSlots && !hasSymbol; i++)
									hasSymbol = IsSymbol(GetArraySlotRef(obj, i));
								if (hasSymbol)
								{
								// it contains symbols so print in dotted form
									for (i = 0; i < numOfSlots; i++)
									{
										PrintObjectBE(GetArraySlotRef(obj, i), indent, depth + 1);
										if (i < numOfSlots - 1)
											printf(".");
									}
								}
								else
								{
								// path expression contains non-symbols so enumerate the elements
									printf("[pathExpr: ");
									for (i = 0; i < numOfSlots; i++)
									{
										PrintObjectBE(GetArraySlotRef(obj, i), indent, depth + 1);
										if (i < numOfSlots - 1)
											printf(", ");
									}
									printf("]");
								}
							}
							else if (depth > printDepth)
							{
								if (ISMAGICPTR(obj))
									printf("[@%ld]", RVALUE(obj));
								else
									printf("[#%08X]", obj);
							}
							else
							{
							// print elements in array
								numOfSlots = Length(obj);
								indent += printf("[");
								if (doPrettyPrint && depth <= printDepth)
								{
									BOOL	isAggr = YES;//NO;
									CObjectIterator	iter(obj);
/*									for (i = 0; !iter.done() && !isAggr; iter.next(), i++)
									{
										if (printLength >= 0 && i >= printLength)
											break;
										isAggr = IsAggregate(PatchPtrRef(iter.value()));
									}
*/									if (!EQRef(objClass, RSYMarray))
									{
										if (IsSymbol(objClass))
										{
											count = printf("%s: ", SymbolName(objClass));
											if (isAggr)
												printf("\n%*s", indent, " ");
											else
												indent += count;
										}
										else
										{
											PrintObjectBE(objClass, indent, depth + 1);
											if (isAggr)
												printf(": \n%*s", indent, " ");
											else
												printf(": ");
										}
									}
									
									for (/*iter.reset(),*/ i = 0; !iter.done(); iter.next(), i++)
									{
										if (printLength >= 0 && i >= printLength)
										{
											// exceeded number-of-slots-to-print preference
											printf("...");
											break;
										}
										PrintObjectBE(iter.value(), indent, depth + 1);
										if (--numOfSlots > 0)
										{
										// separate slots
											printf(", ");
											if (isAggr)
												printf("\n%*s", indent, " ");
										}
									}
								}
								printf("]");
							}
						}
					}

				else	// not slotted
				{
					objClass = MyClassOf(obj);

					if (IsSymbol(obj))
					{
						unsigned char		ch;
						unsigned char *	s = (unsigned char *) SymbolName(obj);
						BOOL		isSpecial = NO;

						if ((ch = *s) != 0
						 && ((sType[ch] & 0x18) != 0 || ch == '_'))
						{
							unsigned char *	p;
							for (p = s; (ch = *p) != 0; p++)
							{
								if (((sType[ch] & 0x38) == 0) && ch != '_')
								{
									isSpecial = YES;
									break;
								}
							}
						}
						if (!isSpecial)
							printf("'%s", s);
						else
						{
							printf("'|");
							isSpecial = NO;
							while ((ch = *s++) != 0)
							{
								if (isSpecial)
								{
									if (ch >= 0x20 && ch <= 0x7F)
									{
										if (ch == 0x5C)
											printf("\\u\\");
										else if (ch == 0x7C)
											printf("\\u|");
										else
											printf("\\u%c", ch);
										isSpecial = NO;
									}
									else
										printf("%02X", ch);
								}
								else if (ch >= 0x20 && ch <= 0x7F)
								{
									if (ch == 0x5C)
										printf("\\\\");
									else if (ch == 0x7C)
										printf("\\|");
									else
										printf("%c", ch);
								}
								else if (ch == 0x0D)
									printf("\\n");
								else if (ch == 0x09)
									printf("\\t");
								else
								{
									printf("\\u%02X", ch);
									isSpecial = YES;
								}
							}
							printf("|");
						}
					}

					else if (EQRef(objClass, RSYMreal))
						printf("%#g", *(double*)BinaryData(obj));

//					else if (EQRef(objClass, RSYMinstructions) && NOTNIL(GetFrameSlot(gVarFrame, SYMA(printInstructions))))
//						PrintInstructions(obj);

					else if (IsSymbol(objClass))
					{
						if (IsSubclass(objClass, RSYMstring))
						{
							printf("\"");
							SafelyPrintBEString((UniChar *) BinaryData(obj));
							printf("\"");
						}
						else
							printf("<%s, length %d>", SymbolName(objClass), Length(obj));
					}

					else
					{
						printf("<binary, class ");
						PrintObjectBE(objClass, indent, depth + 1);
						printf(", length %d>", Length(obj));
					}
				}

				if (depth < kPrecedentStackSize)
					SetArraySlotRef(myPrecedentStack, depth, NILREF);
				break;
			}
		}
	}

	newton_catch(exMessage)
	{
		printf("*** %s ***", CurrentException()->data);
	}
	newton_catch(exRootException)
	{
		printf("*** %s ***", CurrentException()->name);
	}
	end_try;

	fflush(stdout);
}


#endif	/* forFramework */


/*------------------------------------------------------------------------------
	Simulate NewtonScript ROM resources by loading NTK .pkg
	and creating Refs from the elements in its array.
------------------------------------------------------------------------------*/
extern Ref * RSstorePrototype;
extern Ref * RScursorPrototype;

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
#if !defined(forFramework)
	LoadDictionaries();
#endif
}


/*------------------------------------------------------------------------------
	Mimic package installation by following object refs from each part base and
	fixing up absolute addresses.  Create a Ref for each root frame/array in
	each part.

	Args:		inPkgName
				outRef
	Return:	--
------------------------------------------------------------------------------*/

static void
LoadNSData(void)
{
	PackageDirectory  dir;
	RelocationHeader  hdr;
	ULong			baseOffset;
	size_t		relocationDataSize, relocationBlockSize;
	Ptr			relocationData;

	CFBundleRef newtBundle = CFBundleGetBundleWithIdentifier(kBundleId);
	CFURLRef		url = CFBundleCopyResourceURL(newtBundle, kNSDataPkg, NULL, NULL);
	CFStringRef pkgPath = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
//	const char * pathStr = CFStringGetCStringPtr(pkgPath, kCFStringEncodingMacRoman);	// can return NULL if space in the path
	char			pathStr[256];
	CFStringGetCString(pkgPath, pathStr, 256, kCFStringEncodingUTF8);							// so use this instead

	FILE *		pkgFile = fopen(pathStr, "r");
//NSLog(CFSTR("path to NSConstData.pkg = %@"),pkgPath);
//NSLog(CFSTR("char * path to NSConstData.pkg = %s"),pathStr);
//NSLog(CFSTR("file = %016lX"),pkgFile);
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
		relocationData = (Ptr) malloc(relocationDataSize);
		if (relocationData == NULL)
			return;
		fread(relocationData, relocationDataSize, 1, pkgFile);
    
		// update the base offset past the relocation info to the part data
		baseOffset += relocationBlockSize;
	}
	else
		hdr.relocationSize = 0;

	// there’d better be only one part
	ULong partNum, partCount = CANONICAL_LONG(dir.numParts);
	for (partNum = 0; partNum < partCount; partNum++)
	{
		PartEntry	thePart;

		// read PartEntry for the part
		fseek(pkgFile, offsetof(PackageDirectory, parts) + partNum * sizeof(PartEntry), SEEK_SET);
		fread(&thePart, sizeof(PartEntry), 1, pkgFile);

		// read part into memory
		size_t	partSize = CANONICAL_LONG(thePart.size);
		Ptr	partData = (Ptr) malloc(partSize);
		if (partData == NULL)
			return;
		fseek(pkgFile, baseOffset + CANONICAL_LONG(thePart.offset), SEEK_SET);
		fread(partData, partSize, 1, pkgFile);

		// locate the part frame
		ArrayObject *	pkgRoot = (ArrayObject *) partData;
		Ref				partFrame = CANONICAL_LONG(pkgRoot->slot[0]);

		// point to the refs we want
		gConstNSData = (ConstNSData *) &((ArrayObject *) PTR(partFrame - baseOffset + partData))->slot[0];

		// fix up those refs from offsets to addresses
		FixUpRef(&pkgRoot->slot[0], partData - baseOffset);

		// adjust any relocation info
		if (hdr.relocationSize != 0)
		{
			long  delta = (long) partData - CANONICAL_LONG(hdr.baseAddress);
			RelocationEntry *	set = (RelocationEntry *) relocationData;
			ULong setNum, setCount = CANONICAL_LONG(hdr.numEntries);
			for (setNum = 0; setNum < setCount; setNum++)
			{
				long * p, * pageBase = (long *) (partData + CANONICAL_SHORT(set->pageNumber) * CANONICAL_LONG(hdr.pageSize));
				UShort offsetNum, offsetCount = CANONICAL_SHORT(set->offsetCount);
				for (offsetNum = 0; offsetNum < offsetCount; offsetNum++)
				{
					p = pageBase + set->offsets[offsetNum];
					*p = CANONICAL_LONG(*p) + delta;	// was -; but should be + surely?
				}
				set = (RelocationEntry *) ((Ptr) set
						+ sizeof(UShort) * 2					// RelocationEntry less offsets
						+ LONGALIGN(offsetCount));
			}
			free(relocationData);
		}
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

BOOL
IsObjClass(Ref obj, const char * inClassName)
{
	if (IsSymbol(obj))
	{
		char * subName = SymbolName(obj);
		for ( ; *subName && *inClassName; subName++, inClassName++)
		{
			if (toupper(*subName) != toupper(*inClassName))
				return NO;
		}
		return (*inClassName == 0 && (*subName == 0 || *subName == '.'));
	}
	return NO;
}
#endif


static void
FixUpRef(Ref * inRefPtr, Ptr inBaseAddr)
{
#if defined(hasByteSwapping)
	// byte-swap ref in memory
	*inRefPtr = BYTE_SWAP_LONG(*inRefPtr);
#endif

	if (ISREALPTR(*inRefPtr) && *inRefPtr < (Ref)inBaseAddr)
	{
		// ref has not been fixed from offset to address, so do it
		*inRefPtr += (Ref) inBaseAddr;

		ArrayObject *  obj = (ArrayObject *) PTR(*inRefPtr);
		// if gc set, this object has already been fixed up
		if (obj->gc.stuff == 0)
		{
			obj->gc.stuff = 1;
#if defined(hasByteSwapping)
			obj->size = BYTE_SWAP_SIZE(obj->size);
#endif
			// for frames, class is actually the map which needs fixing too; non-pointer refs may need byte-swapping anyway so we always need to do this
			FixUpRef(&obj->objClass, inBaseAddr);

			//	if it’s a frame / array, step through each slot / element fixing those
			if ((obj->flags & kObjSlotted))
			{
				Ref * refPtr = &obj->slot[0];
				for (int count = ARRAYLENGTH(obj);  count > 0; count--, refPtr++)
				{
					FixUpRef(refPtr, inBaseAddr);
				}
			}
#if defined(hasByteSwapping)
			// else object is some form of binary; symbol, string, or encoding table (for example) which needs byte-swapping
			else
			{
				Ref objClass = obj->objClass;
				if (objClass == kSymbolClass)
				{
					// symbol -- byte-swap hash
					SymbolObject * sym = (SymbolObject *) obj;
					sym->hash = CANONICAL_LONG(sym->hash);
				}
				else if (IsObjClass(objClass, "string"))
				{
					// string -- byte-swap UniChar characters
					StringObject * str = (StringObject *) obj;
					UniChar * s = (UniChar *) str->str;
					UniChar * eos = s + (str->size - SIZEOF_BINARYOBJECT)/sizeof(UniChar);
					while (s < eos)
						*s++ = BYTE_SWAP_SHORT(*s);
				}
				else if (IsObjClass(objClass, "UniC"))
				{
					// EncodingMap -- byte-swap UniChar characters
					int i;
					UShort * table = (UShort *) &obj->slot[0];
					UShort formatId, unicodeTableSize;

					formatId = BYTE_SWAP_SHORT(*table);
					*table++ = formatId;

					if (formatId == 0)
					{
						// it’s 8-bit to UniCode
						unicodeTableSize = BYTE_SWAP_SHORT(*table);
						*table++ = unicodeTableSize;
						*table++ = BYTE_SWAP_SHORT(*table);		// revision
						*table++ = BYTE_SWAP_SHORT(*table);		// tableInfo
						for (i = 0; i < unicodeTableSize; i++)
							*table++ = BYTE_SWAP_SHORT(*table);
					}
					else if (formatId == 4)
					{
						// it’s UniCode to 8-bit
						*table++ = BYTE_SWAP_SHORT(*table);		// revision
						*table++ = BYTE_SWAP_SHORT(*table);		// tableInfo
						unicodeTableSize = BYTE_SWAP_SHORT(*table);
						*table++ = unicodeTableSize;
						for (i = 0; i < unicodeTableSize*3; i++)
							*table++ = BYTE_SWAP_SHORT(*table);
					}
				}
			}
#endif
		}
	}
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
/*
	CFArrayRef allBundles = CFBundleGetAllBundles();
	CFIndex i, count = CFArrayGetCount(allBundles);
	for (i = 0; i < count; i++)
	{
		CFBundleRef bndl = (CFBundleRef) CFArrayGetValueAtIndex(allBundles, i);
		CFStringRef idStr = CFBundleGetIdentifier(bndl);
		void * fnPtr = (void *) CFBundleGetFunctionPointerForName(bndl, CFSTR("FSetUnion"));
		char cBuf[256];
		CFStringGetCString(idStr, cBuf, 256, kCFStringEncodingMacRoman);
		printf("id = %s, FSetUnion = 0x%08X\n", cBuf, fnPtr);
	}
*/

#if defined(forNTK)
	CFBundleRef appBundle = CFBundleGetMainBundle();
#endif
#if defined(forFramework)
	CFBundleRef newtBundle = CFBundleGetBundleWithIdentifier(kBundleId);
#else
	CFBundleRef newtBundle = CFBundleGetMainBundle();
#endif
/*
	CFStringRef idStr = CFBundleGetIdentifier(newtBundle);
	char cBuf[256];
	CFStringGetCString(idStr, cBuf, 256, kCFStringEncodingMacRoman);
	printf("bundle id = %s\n", cBuf);
*/
	CFStringRef functionName;
	Ref functionPtr;

	FOREACH_WITH_TAG(gConstNSData->plainCFunctions, fnTag, fnFrame)
		SymbolObject * fnTagRec = (SymbolObject *) PTR(fnTag);
		FrameObject * fnFrameRec = (FrameObject *) PTR(fnFrame);
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
			functionPtr = (Ref) CFBundleGetFunctionPointerForName(appBundle, functionName);
		}
#endif
//printf("%s: 0x%08X / 0x%08X %s\n", fnTagRec->name, functionPtr, fnFrameRec, functionPtr == 0 ? "****" : "");
		fnFrameRec->slot[1] = functionPtr;
		CFRelease(functionName);
	END_FOREACH	
}


/*------------------------------------------------------------------------------
	Load recognition/IA dictionaries.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/
#if !defined(forFramework)
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
	gDictData = (char *) malloc(dictSize);
	fread(gDictData, dictSize, 1, fp);

	fclose(fp);
}
#endif
