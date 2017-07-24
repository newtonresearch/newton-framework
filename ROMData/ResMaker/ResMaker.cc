/*
	File:		ResMaker.cc

	Contains:	Newton object system ROM Ref* builder.
					We use the RSxxxx definitions from the ROM symbol file to read objects from the ROM image
					and recreate their source in assembler. These NewtonScript objects are then available
					in the C world.
					Presumably originally there would have been a tool to do the same thing from a .pkg file
					created by NTK.

					Useful grep expressions for creating a C function pointer file:
					^([^:\n]*)::.*$\n
					^([^(\n]*)(.*$\n

	Written by:	Newton Research Group, 2015.


The ROM has:
003af000	gROMMagicPointerTable	(C array of Refs to MPxxxx, preceded by count)
003afda8	gROMSoupData
-- symbol objects (lots of ’em)
003afdc4	SYMcfunction
003afde0	SYM_2B
003afdf4	SYM_2D
003afe08	SYMaref
003afe20	SYMsetaref
..
0063ab38	SYMnoevilliveon
0063ace8	SYMstringeq
-- magic pointer objects (values are pointer Refs not addresses, but these are actual objects)
0063ad05	MP0000
0063ad31	MP0001
..
0064a971	MP0871
0064a9a9	MP0872
0067fa40	gROMSoupDataSize
-- RefStars
0067fa44	Rupbitmap
0067fa48	RSupbitmap
..
00681c94	Rroutemailicon
00681c98	RSroutemailicon
-- symbol RefStars (in alphabetical order)
00681ca0	RSSYM_2A
00681ca8	RSSYM_2B

*/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <ctype.h>	// tolower

#include "SymbolTable.h"
#include "Reporting.h"

#define forNTK 1

#include "Objects.h"
#include "ObjHeader.h"
#include "Ref32.h"
#include "UStringUtils.h"
#include "ROMResources.h"

#include "ROMExtension.h"
#include "PackageParts.h"
#include "NewtonPackage.h"
#include "Iterators.h"


/* -----------------------------------------------------------------------------
	B u i l d   O p t i o n s
----------------------------------------------------------------------------- */
#define forROMExtensions 0
#define forMagicPointers 1
#define forRefStarData 1

/* -----------------------------------------------------------------------------
	F u n c t i o n   f r a m e   s l o t   i n d i c e s
----------------------------------------------------------------------------- */

enum
{
	kPlainCFunctionClassIndex,
	kPlainCFunctionPtrIndex,
	kPlainCFunctionNumArgsIndex
};

/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */
extern unsigned B32ObjectFlags(Ref r);
extern Ptr B32BinaryData(Ref r);
extern ArrayIndex B32Length(Ref r);
extern bool B32EQRef(Ref a, Ref b);
extern Ref B32ClassOf(Ref r);
extern bool B32IsSubclass(Ref obj, const char * superName);
extern bool B32IsSymbol(Ref r);
extern const char * B32SymbolName(Ref r);
extern Ref B32GetTag(Ref inMap, ArrayIndex index, ArrayIndex * mapIndex);

extern void	UnRelocatePkg(void * inPkg, void * inBase);

const char * PrintROMObject(FILE* inFP, const char * inName, Ref inTag, Ref inObj, bool inDefineObjects, bool inDefineSymbols);
const char * PrintPkgObject(FILE* inFP, const char * inName, Ref inObj, bool inDefineObjects, bool inDefineSymbols);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/
#define kMaxExportFileCount 3

const char *	gCmdName;
bool				gDoProgress = false;
bool				gDoDump = false;
bool				gDoHelp = false;
bool				gHasViaOption = false;
const char *	gExportFilename[kMaxExportFileCount];
int				gExportFileCount = 0;
const char *	gSourceFilename = NULL;
const char *	gSymbolFilename = NULL;
const char *	gOutputFilename = NULL;
const char *	gImageFilename = NULL;

char * ROMdata;
CSymbolTable * gSymbolTable;
CSymbol * gFunctionNames = NULL;
CSymbol * gEditorFunctionNames = NULL;
CPointerTable * gCSymbolTable;


/* -----------------------------------------------------------------------------
	S y m b o l   D a t a
----------------------------------------------------------------------------- */
extern Ref* RSSYMframe;
extern Ref* RSSYMsymbol;
extern Ref* RSSYMReal;
extern Ref* RSSYM_weakarray;


/* -----------------------------------------------------------------------------
	R e f   o b j e c t   t e m p l a t e s
----------------------------------------------------------------------------- */

const char * kHeaderPreamble =
"/*\n"
"	File:		RefStar%s.h\n\n"
"	Contains:	Newton object system ROM Ref* declarations.\n"
"					This file is built automatically by ResMaker from the ROM image and RS symbols.\n"
"	Written by:	Newton Research Group, 2016.\n"
"*/\n\n";

const char * kRSPreamble =
"/*\n"
"	File:		RefStar%s.s\n\n"
"	Contains:	Newton object system ROM Ref* data.\n"
"					This file is built automatically by ResMaker from the ROM image and RS symbols.\n"
"	Written by:	Newton Research Group, 2016.\n"
"*/\n"
"		.data\n"
"		.align	2\n\n"
"#if __LP64__\n"
"/* Refs are 64-bit */\n"
"#define Ref .quad\n"
"#else\n"
"/* Refs are 32-bit */\n"
"#define Ref .long\n"
"#endif\n\n"
"#define MAKEMAGICPTR(n) (n<<2)+3\n"
"#define MAKEPTR(p) p+1\n\n";

const char * kSymbolDataPreamble =
"/*\n"
"	File:		RefSymbols.s\n\n"
"	Contains:	Newton object system ROM Ref* data.\n"
"					This file is built automatically by ResMaker from the ROM image and RS symbols.\n"
"	Written by:	Newton Research Group, 2016.\n"
"*/\n"
"		.data\n"
"		.align	2\n\n"
"#define kFlagsBinary (0x40<<24)\n\n"
"#if __LP64__\n"
"/* Refs are 64-bit, object header is long size|flags, quad gcStuff, quad class */\n"
"#define Ref .quad\n"
"#define kHeaderSize 20\n"
"#else\n"
"/* Refs are 32-bit, object header is long size|flags, long gcStuff, long class */\n"
"#define Ref .long\n"
"#define kHeaderSize 12\n"
"#endif\n\n";

const char * kDataPreamble =
"/*\n"
"	File:		%s.s\n\n"
"	Contains:	Newton object system ROM Ref* data.\n"
"					This file is built automatically by ResMaker from the ROM image and RS symbols.\n"
"	Written by:	Newton Research Group, 2016.\n"
"*/\n"
"		.data\n"
"		.align	2\n\n"
"#define kFlagsBinary (0x40<<24)\n"
"#define kFlagsArray  (0x41<<24)\n"
"#define kFlagsFrame  (0x43<<24)\n\n"
"#if __LP64__\n"
"/* Refs are 64-bit, object header is long size|flags, quad gcStuff, quad class */\n"
"#define Ref .quad\n"
"#define kHeaderSize 20\n"
"#define ArrayObj(_len,_class) \\\n"
"		.long		kHeaderSize + (_len<<3) + kFlagsArray ;\\\n"
"		Ref		0, _class\n"
"#define FrameMapObj(_len) \\\n"
"		.long		kHeaderSize + ((_len+1)<<3) + kFlagsArray ;\\\n"
"		Ref		0\n"
"#define FrameObj(_len, _map) \\\n"
"		.long		kHeaderSize + (_len<<3) + kFlagsFrame ;\\\n"
"		Ref		0, MAKEPTR(_map)\n\n"
"#else\n"
"/* Refs are 32-bit, object header is long size|flags, long gcStuff, long class */\n"
"#define Ref .long\n"
"#define kHeaderSize 12\n"
"#define ArrayObj(_len,_class) \\\n"
"		.long		kHeaderSize + (_len<<2) + kFlagsArray ;\\\n"
"		Ref		0, _class\n"
"#define FrameMapObj(_len) \\\n"
"		.long		kHeaderSize + ((_len+1)<<2) + kFlagsArray ;\\\n"
"		Ref		0\n"
"#define FrameObj(_len, _map) \\\n"
"		.long		kHeaderSize + (_len<<2) + kFlagsFrame ;\\\n"
"		Ref		0, MAKEPTR(_map)\n"
"#endif\n\n"
"#define NILREF 2\n"
"#define MAKEINT(n) (n<<2)\n"
"#define MAKEMAGICPTR(n) (n<<2)+3\n"
"#define MAKEPTR(p) p+1\n\n";

const char * kROMData =
"		.globl	_gROMData%s\n"
"_gROMData%s:\n\n";


/* -----------------------------------------------------------------------------
	String/symbol utilities.
----------------------------------------------------------------------------- */

size_t
Ustrlen(const UniChar * str)
{
	const UniChar * s = str;
	while (*s)
		s++;
	return (s - str);
}


int
symcmp(const char * s1, const char * s2)
{
	char c1, c2;
	for ( ; ; )
	{
		c1 = *s1++;
		c2 = *s2++;
		if (c1 == '\0')
			return (c2 == '\0') ? 0 : -1;
		if (c2 == '\0')
			break;
		if ((c1 = tolower(c1)) != (c2 = tolower(c2)))
			return (c1 > c2) ? 1 : -1;
	}
	return 1;
}


void
MakeFourCharStr(ULong inVal, char * outStr)
{
	sprintf(outStr, "%c%c%c%c", inVal>>24, inVal>>16, inVal>>8, inVal);
}


#pragma mark -

const char *
MungeSymbol(const char * inSymbol)
{
	static char symName[128];
	char * symp = symName;
	for (const char * s = inSymbol; *s != 0; ++s) {
		UChar ch = *s;
		if (isalpha(ch) || isdigit(ch) || ch == '_') {
			*symp++ = ch;
		} else {
			sprintf(symp, "_%02X", ch);
			symp += 3;
		}
	}
	*symp = 0;
	return symName;
}


/* -----------------------------------------------------------------------------
	Print a plain C function name.
	Look up its name from its funcPtr.
----------------------------------------------------------------------------- */

const char *
PlainCFuncName(Ref32 inPtr, Ref inTag)
{
	CPointer * symbol = gCSymbolTable->find(inPtr);
	if (symbol != NULL) {
		static char fname[32];
		sprintf(fname, "_%s", symbol->fName);
//		if (gSymbolTable->find(fname+1)!= NULL) {
//			printf("%s #%08x -> %s\n", NOTNIL(inTag)? B32SymbolName(inTag) : "nil", inPtr, fname);
			return fname;
//		}
//		if (NOTNIL(inTag) ) {
//			printf("%s #%08x not known\n", B32SymbolName(inTag), inPtr);
//		}
		return "_FNullCFunc";
	}
	printf("#%08x not found\n", inPtr);
	return "_FNullCFunc";
}


/* -----------------------------------------------------------------------------
	Print source of a symbol object.
----------------------------------------------------------------------------- */

ULong
SymbolHashFunction(const char * inName)
{
	char ch;
	ULong sum = 0;

	while ((ch = *inName++) != 0)
		sum += toupper(ch);

	return sum * 0x9E3779B9;	// 2654435769
}


void
PrintSymbol(FILE* inFP, const char * inName, const char * inSymbol)
{
	fprintf(inFP, "		.globl	SYM%s\n", inName);
	fprintf(inFP, "SYM%s:	.long		kHeaderSize + 4 + %ld + kFlagsBinary\n", inName, strlen(inSymbol)+1);
	fprintf(inFP, "		Ref		0, 0x55552\n");
	fprintf(inFP, "		.long		0x%08X\n", SymbolHashFunction(inSymbol));
	fprintf(inFP, "		.asciz	\"%s\"\n", inSymbol);
	fprintf(inFP, "		.align	2\n");
}


/* -----------------------------------------------------------------------------
	Print source of a string object.
	Assume UniChars are in host byte order.
----------------------------------------------------------------------------- */

void
PrintString(FILE* inFP, const char * inName, const char * inClass, const UniChar * inString)
{
	ArrayIndex len = Ustrlen(inString);
	if (strncmp(inName, "obj_", 4) != 0) {
		fprintf(inFP, "		.globl	%s\n", inName);
	}
	fprintf(inFP, "%s:	.long		kHeaderSize + %d + kFlagsBinary\n", inName, (len+1)<<1);
	fprintf(inFP, "		Ref		0, %s\n", inClass);
	fprintf(inFP, "		.short	");
	for (ArrayIndex i = 0; i < len; ++i) {
		UniChar ch = inString[i];
		if (ch == '\\') {
			fprintf(inFP, "'\\\\',");
		} else if (ch >= 0x20 && ch <= 0x7E) {
			fprintf(inFP, "'%c',", ch);
		} else {
			fprintf(inFP, "0x%04X,", ch);
		}
	}
	fprintf(inFP, "0\n");
	fprintf(inFP, "		.align	2\n");
}


#if forNTK
/* -----------------------------------------------------------------------------
	Print protoProtoEditor frame.
----------------------------------------------------------------------------- */

void
PrintProtoProtoEditor(FILE * inFP)
{
	const char * name = "protoProtoEditor";
	ArrayIndex numOfFns = 0;
	for (CSymbol * fn = gEditorFunctionNames; fn != NULL; fn = fn->fNext) {
		++numOfFns;
	}
	fprintf(inFP, "%s_map:	FrameMapObj(%d)\n", name, numOfFns);
	fprintf(inFP, "		Ref		0,NILREF\n");
	for (CSymbol * fn = gEditorFunctionNames; fn != NULL; ) {
		fprintf(inFP, "		Ref		");
		for (ArrayIndex i = 0; i < 16 && fn != NULL; ++i, fn = fn->fNext) {
			fprintf(inFP, "MAKEPTR(SYM%s)%c", fn->fName, (i < 15 && fn->fNext != NULL) ? ',' : '\n');
		}
	}

	fprintf(inFP, "%s:	FrameObj(%d, %s_map)\n", name, numOfFns, name);
	for (CSymbol * fn = gEditorFunctionNames; fn != NULL; ) {
		fprintf(inFP, "		Ref		");
		for (ArrayIndex i = 0; i < 16 && fn != NULL; ++i, fn = fn->fNext) {
			fprintf(inFP, "MAKEPTR(obj_%s)%c", fn->fName, (i < 15 && fn->fNext != NULL) ? ',' : '\n');
		}
	}
}
#endif /* forNTK */


/* -----------------------------------------------------------------------------
	Map a pointer Ref from offset to address within ROMdata block.
----------------------------------------------------------------------------- */

Ref
FixPointerRef(Ref r)
{
	if (ISREALPTR(r)) {
		return (Ref)ROMdata + r;
	} else if (ISINT(r)) {
		// handle negative Refs
		long n = RVALUE(r);
		if (n >= INT32_MAX>>2) {
			r = MAKEINT(n - (UINT32_MAX>>2) - 1);
		}
	}
	return r;
}


/* -----------------------------------------------------------------------------
	Print a frame map.
----------------------------------------------------------------------------- */

ArrayIndex
PrintFrameMap(FILE* inFP, const char * inName, Ref inMap)
{
	char nameStr[64];
	FrameMapObject32 * frMap = (FrameMapObject32 *)(inMap-1);
	ArrayIndex numOfSlots = ARRAY32LENGTH(frMap);	// includes supermap
	Ref supermapRef = CANONICAL_LONG(frMap->supermap);

	if NOTNIL(supermapRef) {
		Ref map = FixPointerRef(supermapRef);
		if (ISREALPTR(map)) {
			ObjHeader32 * elem = (ObjHeader32 *)(map-1);
			if (!FLAGTEST(elem->flags, kObjMarked)) {
				FLAGSET(elem->flags, kObjMarked);
				sprintf(nameStr, "obj_%08X", supermapRef);
				PrintFrameMap(inFP, nameStr, map);
			}
		}
	}
	ArrayIndex fnCount = 0;
#if forNTK
	if (strcmp(inName, "gFunky_map") == 0) {
		// count external funtions
		for (CSymbol * fn = gFunctionNames; fn != NULL; fn = fn->fNext) {		// gExternalFunctionNames
			++fnCount;
		}
	} else if (strcmp(inName, "MP0546_map") == 0) {
		// count protoProtoEditor
		++fnCount;
		// and generate its frame
		PrintProtoProtoEditor(inFP);
	}
#endif /* forNTK */
	fprintf(inFP, "%s:	FrameMapObj(%d)\n", inName, numOfSlots+fnCount-1);	// exclude supermap
	Ref32 * p = &frMap->objClass;
	for (ArrayIndex slotsRemaining = numOfSlots+1; slotsRemaining > 0; ) {
		fprintf(inFP, "		Ref		");
		for (ArrayIndex i = 0; i < 16 && slotsRemaining > 0; ++i, --slotsRemaining, ++p) {
			Ref tag = FixPointerRef(CANONICAL_LONG(*p));
			sprintf(nameStr, "obj_%08X", CANONICAL_LONG(*p));
			if (fnCount > 0 && ISINT(tag)) {
				FLAGCLEAR(tag, kMapSorted);
			}
			fprintf(inFP, "%s%c", PrintROMObject(inFP, nameStr, NILREF, tag, false, false), (i < 15 && slotsRemaining > 1) ? ',' : '\n');
		}
	}
#if forNTK
	// append external functions
	if (fnCount > 0) {
		if (strcmp(inName, "gFunky_map") == 0) {
			for (CSymbol * fn = gFunctionNames; fn != NULL; ) {	// gExternalFunctionNames
				fprintf(inFP, "		Ref		");
				for (ArrayIndex i = 0; i < 16 && fn != NULL; ++i, fn = fn->fNext) {
					fprintf(inFP, "MAKEPTR(SYM%s)%c", fn->fName, (i < 15 && fn->fNext != NULL) ? ',' : '\n');
				}
			}
		} else /*if (strcmp(inName, "MP0546_map") == 0)*/ {
			// append protoProtoEditor
			fprintf(inFP, "		Ref		MAKEPTR(SYMprotoProtoEditor)\n");
		}
	}
#endif /* forNTK */
	return fnCount;
}

void
PrintPkgFrameMap(FILE* inFP, const char * inName, Ref inMap)
{
	char nameStr[64];
	FrameMapObject * frMap = (FrameMapObject *)(inMap-1);
	ArrayIndex numOfSlots = ARRAYLENGTH(frMap);	// includes supermap
	Ref supermapRef = frMap->supermap;

	if NOTNIL(supermapRef) {
		Ref map = supermapRef;
		if (ISREALPTR(map)) {
			ObjHeader * elem = (ObjHeader *)(map-1);
			if (!FLAGTEST(elem->flags, kObjMarked)) {
				FLAGSET(elem->flags, kObjMarked);
				sprintf(nameStr, "obj_%08X", supermapRef);
				PrintPkgFrameMap(inFP, nameStr, map);
			}
		}
	}
	fprintf(inFP, "%s:	FrameMapObj(%d)\n", inName, numOfSlots-1);	// exclude supermap
	Ref * p = &frMap->objClass;
	for (ArrayIndex slotsRemaining = numOfSlots+1; slotsRemaining > 0; ) {
		fprintf(inFP, "		Ref		");
		for (ArrayIndex i = 0; i < 16 && slotsRemaining > 0; ++i, --slotsRemaining, ++p) {
			Ref tag = *p;
			sprintf(nameStr, "obj_%08X", *p);
			fprintf(inFP, "%s%c", PrintPkgObject(inFP, nameStr, tag, false, false), (i < 15 && slotsRemaining > 1) ? ',' : '\n');
		}
	}
}


/* -----------------------------------------------------------------------------
	Print a C function object.
----------------------------------------------------------------------------- */

void
PrintCFunction(FILE * inFP, const char * inName, const char * inPointer, int inNumArgs)
{
	fprintf(inFP,	"		.globl	%s\n"
						"%s_map:	FrameMapObj(3)\n"
						"		Ref		0,NILREF,MAKEPTR(SYMclass),MAKEPTR(SYMfuncPtr),MAKEPTR(SYMnumArgs)\n"
						"%s:	FrameObj(3, %s_map)\n"
						"		Ref		0x00000132,%s,MAKEINT(%d)\n"
						"		.align	2\n", inName,inName,inName,inName,inPointer,inNumArgs);
}

void
PrintDebugCFunction(FILE * inFP, const char * inName, const char * inPointer, int inNumArgs)
{
	fprintf(inFP,	"		.globl	%s\n"
						"%s_map:	FrameMapObj(4)\n"
						"		Ref		0,NILREF,MAKEPTR(SYMclass),MAKEPTR(SYMfuncPtr),MAKEPTR(SYMnumArgs),MAKEPTR(SYMdocString)\n"
						"%s:	FrameObj(4, %s_map)\n"
						"		Ref		0x00000132,%s,MAKEINT(%d),NILREF\n"
						"		.align	2\n", inName,inName,inName,inName,inPointer,inNumArgs);
}


/* -----------------------------------------------------------------------------
	Print an RS (RefStar) reference.
----------------------------------------------------------------------------- */

void
PrintRefStar(FILE* inFP, const char * inName, const char * inValue)
{
	fprintf(inFP, "		.globl	_RS%s\n", inName);
	fprintf(inFP, "_R%s:		Ref	%s\n", inName, inValue);
	fprintf(inFP, "_RS%s:	Ref	_R%s\n", inName, inName);
}


/* -----------------------------------------------------------------------------
	Extract const Ref* (RS) objects from ROM image and print their definitions.

	for top-level object
		switch object tag
			integer			N/A
			immediate		N/A
			magic pointer	print RS with "MAKEMAGICPTR(%ld)", RVALUE(obj)
			pointer
				frame			print map, frame
				array			print array
					maybe recurse
				binary
								print RS with obj name


	for frame/array element
		switch object tag
			integer			print Ref, but no RS
			immediate		print Ref, but no RS
			magic pointer	print Ref

				maybe recurse

	Args:		inFP			data file ref
				inName		object name
				inObj			object
				inDefineObjects
				inDefineSymbols
	Return:	--
----------------------------------------------------------------------------- */

const char *
PrintROMObject(FILE* inFP, const char * inName, Ref inTag, Ref inObj, bool inDefineObjects, bool inDefineSymbols)
{
	Ref objClass;
	ArrayIndex i, numOfSlots;
	static char valueStr[256];
	const char * rsValue = NULL;

	switch (RTAG(inObj))
	{
	case kTagInteger:
/*-- integer --*/
		// cannot have integer Ref at top level

	case kTagImmed:
/*-- immediate --*/
		// cannot have immediate Ref at top level
		if (ISNIL(inObj))
			sprintf(valueStr, "NILREF");
		else if (ISCHAR(inObj))
			sprintf(valueStr, "0x%04lX", inObj);
		else
			sprintf(valueStr, "0x%08lX", inObj);
		rsValue = valueStr;
		break;

	case kTagPointer: {
/*-- pointer --*/
			Ref val;
			char nameStr[64];
			// switch on type of object: slotted (frame/array) or binary
			unsigned flags = B32ObjectFlags(inObj);
			if (FLAGTEST(flags, kObjSlotted)) {
/*-- SLOTTED --*/
				if (FLAGTEST(flags, kObjFrame)) {
/*-- frame --*/
/*
foreach slot in frame
if pointer object not previously printed, print it using name derived from address
	might recurse
print frameMap
print frameHeader
foreach slot in frame
print object -- if pointer object use name above
*/
					if (inDefineObjects) {
						numOfSlots = B32Length(inObj);
						Ref32 map32 = ((FrameObject32 *)(inObj-1))->map;
						Ref map = FixPointerRef(CANONICAL_LONG(map32));
						Ref32 * tagPtr = ((FrameMapObject32 *)(map-1))->slot;
						Ref32 * p, * slotPtr = ((FrameObject32 *)(inObj-1))->slot;
		// PASS 1: define pointer objects contained in the frame
						for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p, ++tagPtr) {
							val = FixPointerRef(CANONICAL_LONG(*p));
							if (ISREALPTR(val)) {
								ObjHeader32 * elem = (ObjHeader32 *)(val-1);
								if (!FLAGTEST(elem->flags, kObjMarked)) {
									FLAGSET(elem->flags, kObjMarked);
									sprintf(nameStr, "obj_%08X", CANONICAL_LONG(*p));
//						Ref tag = B32GetTag(inObj, i, NULL);
									PrintROMObject(inFP, nameStr, B32GetTag(map, i, NULL), val, true, false);
								}
							}
						}

		// PASS 2: define frame map
						sprintf(nameStr, "%s_map", inName);
						ArrayIndex fnCount = PrintFrameMap(inFP, nameStr, FixPointerRef(CANONICAL_LONG(((FrameObject32 *)(inObj-1))->map)));

		// PASS 3: define frame
						if (strncmp(inName, "obj_", 4) != 0) {
							fprintf(inFP, "		.globl	%s\n", inName);
						}
						fprintf(inFP, "%s:	FrameObj(%d, %s_map)\n", inName, numOfSlots+fnCount, inName);
						bool isPlainCFunc = false;
						p = slotPtr;
						for (ArrayIndex slotsRemaining = numOfSlots; slotsRemaining > 0; ) {
							fprintf(inFP, "		Ref		");
							for (i = 0; i < 16 && slotsRemaining > 0; ++i, --slotsRemaining, ++p) {
								val = FixPointerRef(CANONICAL_LONG(*p));
								if (i == kPlainCFunctionClassIndex && val == kPlainCFunctionClass) {
									isPlainCFunc = true;
								}
								if (i == kPlainCFunctionPtrIndex && isPlainCFunc) {
									fprintf(inFP, "%s,", PlainCFuncName(CANONICAL_LONG(*p), inTag));
								} else {
									sprintf(nameStr, "obj_%08X", CANONICAL_LONG(*p));
									fprintf(inFP, "%s%c", PrintROMObject(inFP, nameStr, NILREF, val, false, false), (i < 15 && slotsRemaining > 1) ? ',' : '\n');
								}
							}
						}
#if forNTK
						if (strcmp(inName, "gFunky") == 0) {
						// append external functions
							for (CSymbol * fn = gFunctionNames; fn != NULL; ) {
								fprintf(inFP, "		Ref		");
								for (ArrayIndex i = 0; i < 16 && fn != NULL; ++i, fn = fn->fNext) {
									fprintf(inFP, "MAKEPTR(obj_%s)%c", fn->fName, (i < 15 && fn->fNext != NULL) ? ',' : '\n');
								}
							}
						} else if (strcmp(inName, "MP0546") == 0) {
						// append protoProtoEditor
							fprintf(inFP, "		Ref		MAKEPTR(protoProtoEditor)\n");
						}
#endif
//							fprintf(inFP, "		.align	2\n");
					}
					sprintf(valueStr, "MAKEPTR(%s)", inName);
					rsValue = valueStr;
				}

				// insanity check
				else if (FLAGTEST(flags, kObjForward)) {
					printf("<forwarding pointer #%08lX...WTF!!!>", inObj);
				}

				else {
/*-- array --*/
					if (inDefineObjects) {
						objClass = B32ClassOf(inObj);
					// print elements in array
						numOfSlots = B32Length(inObj);
/*
foreach slot in array
if pointer object not previously printed, print it using name inName & "-" & index
	might recurse
print arrayHeader
foreach element in array
print object -- if pointer object use name above
*/
						Ref32 * p, * slotPtr = ((FrameObject32 *)(inObj-1))->slot;
		// PASS 1: define pointer objects contained in the array
						for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p) {
							val = FixPointerRef(CANONICAL_LONG(*p));
							if (ISREALPTR(val)) {
								ObjHeader32 * elem = (ObjHeader32 *)(val-1);
								if (!FLAGTEST(elem->flags, kObjMarked)) {
									FLAGSET(elem->flags, kObjMarked);
									sprintf(nameStr, "obj_%08X", CANONICAL_LONG(*p));
									PrintROMObject(inFP, nameStr, NILREF, val, true, false);
								}
							}
						}

		// PASS 2: define the array
						if (strncmp(inName, "obj_", 4) != 0) {
							fprintf(inFP, "		.globl	%s\n", inName);
						}
						fprintf(inFP, "%s:	ArrayObj(%d, %s)\n", inName, numOfSlots, PrintROMObject(inFP, inName, NILREF, objClass, false, false));
						p = slotPtr;
						for (ArrayIndex slotsRemaining = numOfSlots; slotsRemaining > 0; ) {
							fprintf(inFP, "		Ref		");
							for (i = 0; i < 16 && slotsRemaining > 0; ++i, --slotsRemaining, ++p) {
								val = FixPointerRef(CANONICAL_LONG(*p));
								sprintf(nameStr, "obj_%08X", CANONICAL_LONG(*p));
								fprintf(inFP, "%s%c", PrintROMObject(inFP, nameStr, NILREF, val, false, false), (i < 15 && slotsRemaining > 1) ? ',' : '\n');
							}
						}
//							fprintf(inFP, "		.align	2\n");
					}
					sprintf(valueStr, "MAKEPTR(%s)", inName);
					rsValue = valueStr;
				}
			}

			else {
/*-- NOT SLOTTED --*/
				objClass = B32ClassOf(inObj);

				if (B32IsSymbol(inObj)) {
/*-- symbol --*/
					const char * sym = B32SymbolName(inObj);
					const char * symName = MungeSymbol(sym);
					if (inDefineSymbols) {
						PrintSymbol(inFP, symName, sym);
					}
					sprintf(valueStr, "MAKEPTR(SYM%s)", symName);
					rsValue = valueStr;
				}

				else if (B32EQRef(objClass, SYMA(Real))) {
/*-- real number --*/
					if (inDefineObjects) {
						// byte-swap double == 64 bits
						uint32_t * p = (uint32_t *)B32BinaryData(inObj);
						uint32_t p1 = p[1];
						p[1] = CANONICAL_LONG(*p);
						p[0] = CANONICAL_LONG(p1);

						fprintf(inFP, "REAL%s:	.long		kHeaderSize + 8 + kFlagsBinary\n", inName);
						fprintf(inFP, "		Ref		0, MAKEPTR(SYMreal)\n");
						fprintf(inFP, "		.long		0x%08X,0x%08X\n", p[0],p[1]);
						fprintf(inFP, "		.align	2\n");
					}
					sprintf(valueStr, "MAKEPTR(REAL%s)", inName);
					rsValue = valueStr;
				}

//				else if (B32EQRef(objClass, SYMA(instructions)) && NOTNIL(GetFrameSlot(gVarFrame, SYMA(printInstructions)))) {
/*-- codeblock --*/
//					PrintInstructions(inObj);
//				}

				else if (B32IsSymbol(objClass)) {
					if (B32IsSubclass(objClass, "string")) {
/*-- string --*/
						if (inDefineObjects) {
							// byte-swap UniChar
							UniChar * str = (UniChar *)B32BinaryData(inObj);
							UniChar * s = str;
							for (ArrayIndex i = 0, len = B32Length(inObj)/sizeof(UniChar); i < len; ++i, ++s) {
								*s = CANONICAL_SHORT(*s);
							}
							PrintString(inFP, inName, PrintROMObject(inFP, inName, NILREF, objClass, false, false), str);
						}
						sprintf(valueStr, "MAKEPTR(%s)", inName);
						rsValue = valueStr;
					} else {
/*-- specific binary object --*/
						if (inDefineObjects) {
							UChar * data = (UChar *)B32BinaryData(inObj);
							ArrayIndex len = B32Length(inObj);
							if (strncmp(inName, "obj_", 4) != 0) {
								fprintf(inFP, "		.globl	%s\n", inName);
							}
							fprintf(inFP, "%s:	.long		kHeaderSize + %d + kFlagsBinary\n", inName, len);
							fprintf(inFP, "		Ref		0, %s\n", PrintROMObject(inFP, inName, NILREF, objClass, false, false));
							while (len > 0) {
								fprintf(inFP, "		.byte		");
								for (ArrayIndex i = 0; i < 16 && len > 0; ++i, --len) {
									fprintf(inFP, "0x%02X%c", *data++, (i < 15 && len > 1) ? ',' : '\n');
								}
							}
							fprintf(inFP, "		.align	2\n");
						}
						sprintf(valueStr, "MAKEPTR(%s)", inName);
						rsValue = valueStr;
					}
				}

				else {
/*-- generic binary object --*/
					printf("<binary, class ");
					PrintROMObject(inFP, inName, NILREF, objClass, false, false);
					printf(", length %d>", B32Length(inObj));
				}
			}
		}
		break;

	case kTagMagicPtr:
/*-- magic pointer --*/
		sprintf(valueStr, "MAKEMAGICPTR(%ld)", RVALUE(inObj));
		rsValue = valueStr;
		break;
	}

	fflush(stdout);
	return rsValue;
}


const char *
PkgSymbolName(Ref r)
{
	const char * target[] =	{
		"debuggerInfo","InstallScript",
		0 };

	SymbolObject * obj = (SymbolObject *)(r-1);
	const char * sym = obj->name;
	// apply case correction
	for (const char ** s = target; *s != 0; ++s) {
		if (symcmp(sym, *s) == 0) {
			sym = *s;
			break;
		}
	}
	return sym;
}


/* -----------------------------------------------------------------------------
	Extract const Ref* (RS) objects from package and print their definitions.
	Args:		inFP			data file ref
				inName			object name
				inObj			object
	Return:	--
----------------------------------------------------------------------------- */

const char *
PrintPkgObject(FILE* inFP, const char * inName, Ref inObj, bool inDefineObjects, bool inDefineSymbols)
{
	Ref objClass;
	ArrayIndex i, numOfSlots;
	static char valueStr[256];
	const char * rsValue = NULL;

	switch (RTAG(inObj))
	{
	case kTagInteger:
/*-- integer --*/
		// cannot have integer Ref at top level

	case kTagImmed:
/*-- immediate --*/
		// cannot have immediate Ref at top level
		if (ISNIL(inObj))
			sprintf(valueStr, "NILREF");
		else if (ISCHAR(inObj))
			sprintf(valueStr, "0x%04lX", inObj);
		else
			sprintf(valueStr, "0x%08lX", inObj);
		rsValue = valueStr;
		break;

	case kTagPointer: {
/*-- pointer --*/
			Ref val;
			char nameStr[64];
			// switch on type of object: slotted (frame/array) or binary
			unsigned flags = ObjectFlags(inObj);
			if (FLAGTEST(flags, kObjSlotted)) {
/*-- SLOTTED --*/
				if (FLAGTEST(flags, kObjFrame)) {
/*-- frame --*/
					if (inDefineObjects) {
						numOfSlots = Length(inObj);
						Ref map = ((FrameObject *)(inObj-1))->map;
						Ref * tagPtr = ((FrameMapObject *)(map-1))->slot;
						Ref * p, * slotPtr = ((FrameObject *)(inObj-1))->slot;
		// PASS 1: define pointer objects contained in the frame
						for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p, ++tagPtr) {
							val = *p;
							if (ISREALPTR(val)) {
								ObjHeader * elem = (ObjHeader *)(val-1);
								if (!FLAGTEST(elem->flags, kObjMarked)) {
									FLAGSET(elem->flags, kObjMarked);
									sprintf(nameStr, "obj_%08X", *p);
									PrintPkgObject(inFP, nameStr, val, true, false);
								}
							}
						}

		// PASS 2: define frame map
						sprintf(nameStr, "%s_map", inName);
						PrintPkgFrameMap(inFP, nameStr, ((FrameObject *)(inObj-1))->map);

		// PASS 3: define frame
						if (strncmp(inName, "obj_", 4) != 0) {
							fprintf(inFP, "		.globl	%s\n", inName);
						}
						fprintf(inFP, "%s:	FrameObj(%d, %s_map)\n", inName, numOfSlots, inName);
						p = slotPtr;
						for (ArrayIndex slotsRemaining = numOfSlots; slotsRemaining > 0; ) {
							fprintf(inFP, "		Ref		");
							for (i = 0; i < 16 && slotsRemaining > 0; ++i, --slotsRemaining, ++p) {
								val = *p;
								sprintf(nameStr, "obj_%08X", *p);
								fprintf(inFP, "%s%c", PrintPkgObject(inFP, nameStr, val, false, false), (i < 15 && slotsRemaining > 1) ? ',' : '\n');
							}
						}
					}
					sprintf(valueStr, "MAKEPTR(%s)", inName);
					rsValue = valueStr;
				}

				// insanity check
				else if (FLAGTEST(flags, kObjForward)) {
					printf("<forwarding pointer #%08lX...WTF!!!>", inObj);
				}

				else {
/*-- array --*/
					if (inDefineObjects) {
						objClass = ClassOf(inObj);
					// print elements in array
						numOfSlots = Length(inObj);
						Ref * p, * slotPtr = ((FrameObject *)(inObj-1))->slot;
		// PASS 1: define pointer objects contained in the array
						for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p) {
							val = *p;
							if (ISREALPTR(val)) {
								ObjHeader * elem = (ObjHeader *)(val-1);
								if (!FLAGTEST(elem->flags, kObjMarked)) {
									FLAGSET(elem->flags, kObjMarked);
									sprintf(nameStr, "obj_%08X", *p);
									PrintPkgObject(inFP, nameStr, val, true, false);
								}
							}
						}

		// PASS 2: define the array
						if (strncmp(inName, "obj_", 4) != 0) {
							fprintf(inFP, "		.globl	%s\n", inName);
						}
						fprintf(inFP, "%s:	ArrayObj(%d, %s)\n", inName, numOfSlots, PrintPkgObject(inFP, inName, objClass, false, false));
						p = slotPtr;
						for (ArrayIndex slotsRemaining = numOfSlots; slotsRemaining > 0; ) {
							fprintf(inFP, "		Ref		");
							for (i = 0; i < 16 && slotsRemaining > 0; ++i, --slotsRemaining, ++p) {
								val = *p;
								sprintf(nameStr, "obj_%08X", *p);
								fprintf(inFP, "%s%c", PrintPkgObject(inFP, nameStr, val, false, false), (i < 15 && slotsRemaining > 1) ? ',' : '\n');
							}
						}
					}
					sprintf(valueStr, "MAKEPTR(%s)", inName);
					rsValue = valueStr;
				}
			}

			else {
/*-- NOT SLOTTED --*/
				objClass = ClassOf(inObj);

				if (IsSymbol(inObj)) {
/*-- symbol --*/
					const char * sym = PkgSymbolName(inObj);
					const char * symName = MungeSymbol(sym);
					if (inDefineSymbols) {
						PrintSymbol(inFP, symName, sym);
					}
					sprintf(valueStr, "MAKEPTR(SYM%s)", symName);
					rsValue = valueStr;
				}

				else if (EQRef(objClass, SYMA(Real))) {
/*-- real number --*/
					if (inDefineObjects) {
						// byte-swap double == 64 bits
						uint32_t * p = (uint32_t *)BinaryData(inObj);

						fprintf(inFP, "REAL%s:	.long		kHeaderSize + 8 + kFlagsBinary\n", inName);
						fprintf(inFP, "		Ref		0, MAKEPTR(SYMreal)\n");
						fprintf(inFP, "		.long		0x%08X,0x%08X\n", p[0],p[1]);
						fprintf(inFP, "		.align	2\n");
					}
					sprintf(valueStr, "MAKEPTR(REAL%s)", inName);
					rsValue = valueStr;
				}

//				else if (EQRef(objClass, SYMA(instructions)) && NOTNIL(GetFrameSlot(gVarFrame, SYMA(printInstructions)))) {
/*-- codeblock --*/
//					PrintInstructions(inObj);
//				}

				else if (IsSymbol(objClass)) {
					if (IsSubclass(objClass, SYMA(string))) {
/*-- string --*/
						if (inDefineObjects) {
							// the package loader has already helpfully byte-swapped UniChar
							PrintString(inFP, inName, PrintPkgObject(inFP, inName, objClass, false, false), (UniChar *)BinaryData(inObj));
						}
						sprintf(valueStr, "MAKEPTR(%s)", inName);
						rsValue = valueStr;
					} else {
/*-- specific binary object --*/
						if (inDefineObjects) {
							UChar * data = (UChar *)BinaryData(inObj);
							ArrayIndex len = Length(inObj);
							if (strncmp(inName, "obj_", 4) != 0) {
								fprintf(inFP, "		.globl	%s\n", inName);
							}
							fprintf(inFP, "%s:	.long		kHeaderSize + %d + kFlagsBinary\n", inName, len);
							fprintf(inFP, "		Ref		0, %s\n", PrintPkgObject(inFP, inName, objClass, false, false));
							while (len > 0) {
								fprintf(inFP, "		.byte		");
								for (ArrayIndex i = 0; i < 16 && len > 0; ++i, --len) {
									fprintf(inFP, "0x%02X%c", *data++, (i < 15 && len > 1) ? ',' : '\n');
								}
							}
							fprintf(inFP, "		.align	2\n");
						}
						sprintf(valueStr, "MAKEPTR(%s)", inName);
						rsValue = valueStr;
					}
				}

				else {
/*-- generic binary object --*/
					printf("<binary, class ");
					PrintPkgObject(inFP, inName, objClass, false, false);
					printf(", length %d>", Length(inObj));
				}
			}
		}
		break;

	case kTagMagicPtr:
/*-- magic pointer --*/
		sprintf(valueStr, "MAKEMAGICPTR(%ld)", RVALUE(inObj));
		rsValue = valueStr;
		break;
	}

	fflush(stdout);
	return rsValue;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Command line options.
----------------------------------------------------------------------------- */

void
AssureNextArg(const char ** inArg, const char ** inArgLimit, const char * inMessage)
{
	if (inArg + 1 >= inArgLimit)
		UsageExitWithMessage("no %s follows “%s”", inMessage, *inArg);
}


int
hcistrcmp(const char * inStr, const char * inOptionStr)
{
	char  ch;
	int	delta;
	do {
		ch = *inStr++;
		if ((delta = *inOptionStr++ - tolower(ch)) != 0)
			return delta;
	} while (ch != 0);
	return 0;
}


const char **
ParseOptions(const char ** inArg, const char ** inArgLimit)
{
	const char **	inputFilenameLimit = inArg;
	const char *	option;

	for ( ; inArg < inArgLimit; inArg++) {
		option = *inArg;
		if (hcistrcmp(option, "-help") == 0) {
			gDoHelp = true;
		} else if (hcistrcmp(option, "-p") == 0) {
			gDoProgress = true;
		} else if (hcistrcmp(option, "-dump") == 0) {
			gDoDump = true;
		} else if (hcistrcmp(option, "-s") == 0) {
			AssureNextArg(inArg, inArgLimit, "Ref source filename");
			gSourceFilename = *++inArg;
		} else if (hcistrcmp(option, "-sym") == 0) {
			AssureNextArg(inArg, inArgLimit, "plainC function name file");
			gSymbolFilename = *++inArg;
		} else if (hcistrcmp(option, "-via") == 0) {
			AssureNextArg(inArg, inArgLimit, "function name file");
			if (gExportFileCount >= kMaxExportFileCount) {
				ExitWithMessage("only %d function name files allowed", kMaxExportFileCount);
			}
			gHasViaOption = true;
			gExportFilename[gExportFileCount++] = *++inArg;
		} else if (hcistrcmp(option, "-o") == 0) {
			AssureNextArg(inArg, inArgLimit, "output folder");
			gOutputFilename = *++inArg;
		} else if (*option == '-') {
			UsageExitWithMessage("%s is not an option", option);
		} else {
			*inputFilenameLimit++ = option;
		}
	}

	return inputFilenameLimit;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	And this is where our story REALLY starts...
	Extract const Ref* (RS) objects from ROM image and print their definitions.
	foreach line in RefStarSymbols decalaration file
		scan address(=offset into ROM image), symbol name
		declare symbol name in header file
		define symbol data in data file
			maybe recurse
		define RS reference in RS file
	Args:		argc			number of command line args
				argv			those args in full
	Return:	exit code
----------------------------------------------------------------------------- */

int
main(int argc, const char * argv[])
{
	gCmdName = argv[0];
	/*const char **  inputFilenameLimit =*/ ParseOptions(&argv[1], &argv[argc]);

	gImageFilename = argv[1];
	FILE * fp = fopen(gImageFilename, "r");
	if (fp == NULL)
		ExitWithMessage("can’t open ROM image file “%s”", gImageFilename);

/*== read ROM image into memory ==*/
	size_t ROMsize;
	fseek(fp, 0, SEEK_END);
	ROMsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	ROMdata = (char *)malloc(ROMsize);	// yup, all 8MB of it
	fread(ROMdata, ROMsize, 1, fp);
	fclose(fp);

/*== export ROM extensions ==*/
#if forROMExtensions
	RExHeader * rex = (RExHeader *)(ROMdata + 0x0071FC4C);	// it’s at a well-known address in the ROM image

#if defined(hasByteSwapping)
	// byte-swap the directory
	ULong * ulp = (ULong *)rex;
	for (ArrayIndex i = 0; i < sizeof(RExHeader)/sizeof(ULong); ++i, ++ulp) {
		*ulp = BYTE_SWAP_LONG(*ulp);
	}
	// including config directory entries
	ConfigEntry * entry = rex->table;
	for (ArrayIndex i = 0; i < rex->count; ++i, ++entry) {
		// byte swap directory entry
		entry->tag = BYTE_SWAP_LONG(entry->tag);
		entry->offset = BYTE_SWAP_LONG(entry->offset);
		entry->length = BYTE_SWAP_LONG(entry->length);
	}
#endif

	char str[9];
	MakeFourCharStr(rex->signatureA, str);
	MakeFourCharStr(rex->signatureB, str+4);
	printf("ROM Extension\n"
			 "signature: %s\n"
			 "header version: %d\n"
			 "manufacturer / version: %08X / %08X\n"
			 "REx id: %d\n\n"
			 "%d table entries:\n", str, rex->headerVersion, rex->manufacturer, rex->version, rex->id, rex->count);
	for (ArrayIndex i = 0; i < rex->count; ++i) {
		ConfigEntry * entry = &rex->table[i];
		MakeFourCharStr(entry->tag, str);
		printf("  %s[%d]\n", str, entry->length);
		if (entry->tag == kPackageListTag) {
			PackageDirectory * pkg = (PackageDirectory *)((Ptr)rex + entry->offset);
			size_t pkgSize;
			ArrayIndex pkgi = 1;
			for (size_t sizeRemaining = entry->length; sizeRemaining > 0; sizeRemaining -= pkgSize, ++pkgi) {
				UnRelocatePkg(pkg, ROMdata);
				pkgSize = CANONICAL_LONG(pkg->size);
				char pkgName[16];
				sprintf(pkgName, "Package-%d.newtonpkg", pkgi);
				FILE * pkgFile = fopen(pkgName, "w");
				fwrite(pkg, pkgSize, 1, pkgFile);
				fclose(pkgFile);
				pkg = (PackageDirectory *)((Ptr)pkg + ALIGN(pkgSize,4));
			}
		}
	}
#endif /*forROMExtensions*/


/*== read implemented plain C function names into symbol table ==*/
	gSymbolTable = new CSymbolTable(4*KByte);
#if forNTK
	if (FillSymbolTable(gSymbolTable, &gFunctionNames, &gEditorFunctionNames))		// Toolkit.exp and Editor.exp
		ExitWithMessage("unable to build a valid function name table");
#endif
	gCSymbolTable = new CPointerTable(16*KByte);
	if (FillSymbolTable(gCSymbolTable))							// 717006.cfunctions
		ExitWithMessage("unable to build a valid function pointer table");

	unsigned int rAddr;
	Ref rRef;
	Ref32 * rsRef;
	char rsName[256];
	const char * rsValue;

/*-- set up path for Newton / NTK data --*/
	char fullPath[256], * pathStart;
	if (gOutputFilename) {
		strcpy(fullPath, gOutputFilename);
		strcat(fullPath, "/");
		pathStart = fullPath + strlen(fullPath);
	}
	const char * path;

/*== first handle magic pointers ==*/
#if forMagicPointers
	path = "MagicPointers.s";
	if (gOutputFilename) {
		strcpy(pathStart, path); path = fullPath;
	}
	FILE * fp_mp = fopen(path, "w");
	struct MagicPointerTable
	{
		ArrayIndex	numOfPointers;
		Ref32			magicPointer[];
	};
	MagicPointerTable * mp = (MagicPointerTable *)(ROMdata + 0x003AF000);	// it’s at a well-known address in the ROM image
	ArrayIndex count = CANONICAL_LONG(mp->numOfPointers);

/*-- define the table --*/
	fprintf(fp_mp, kDataPreamble, "MagicPointers");
	fprintf(fp_mp, "		.globl	_gROMMagicPointerTable\n");
	fprintf(fp_mp, "_gROMMagicPointerTable:\n		.long	%d\n", count);
	for (ArrayIndex slotIndex = 0, slotsRemaining = count; slotsRemaining > 0;) {
		fprintf(fp_mp, "		Ref		");
		for (ArrayIndex i = 0; i < 10 && slotsRemaining > 0; ++i, ++slotIndex, --slotsRemaining) {
			fprintf(fp_mp, "MAKEPTR(MP%04d)%c", slotIndex, (i < 9 && slotsRemaining > 1) ? ',' : '\n');
		}
	}
/*-- define the contents of the table --*/
	size_t filePos, prevPos = 0;
	Ref32 * mpp = mp->magicPointer;
	for (ArrayIndex i = 0; i < count; ++i, ++mpp) {
		sprintf(rsName, "MP%04d", i);
		rRef = FixPointerRef(CANONICAL_LONG(*mpp));
		rsValue = PrintROMObject(fp_mp, rsName, NILREF, rRef, true, false);
		// there are a couple of weird immediates in the magic pointer table that don’t get printed by PrintROMObject
		filePos = ftell(fp_mp);
		if (filePos == prevPos) {
			fprintf(fp_mp, "%s:	Ref		%s\n", rsName, rsValue);
			filePos = ftell(fp_mp);
		}
		prevPos = filePos;
	}

	fclose(fp_mp);
#endif /*forMagicPointers*/

#if forRefStarData
/*== read object pointers and names ==*/
	fp = fopen(gSourceFilename, "r");
	if (fp == NULL)
		ExitWithMessage("can’t open source file “%s”", gSourceFilename);

/*-- PASS 1: build header file, locate symbol table array object --*/
	FILE * fp_h = fopen("../Frames/RSData.h", "w");
	fprintf(fp_h, kHeaderPreamble, "");
	// scan address, name of object
	rRef = NILREF;
	int lineLen;
	char line[300];
	while (!feof(fp)) {
		lineLen = 0;
		line[0] = 0;
		if (fgets(line, 300, fp) != NULL) {
			if (line[0] != '\n') {
				int	numOfItems = sscanf(line, "%8x%255s%n", &rAddr, rsName, &lineLen);
				if (lineLen == 255) {
					ExitWithMessage("input line too long in file “%s”", gSourceFilename);
				}
				if (numOfItems > 1 && line[0] != ';') {
					if (strlen(rsName) > 254) {
						ExitWithMessage("encountered function name longer than 254 characters in file “%s”", gSourceFilename);
					}
					fprintf(fp_h, "extern Ref* RS%s;\n", rsName);
					if (strcmp(rsName, "symbolTable") == 0) {
						rsRef = (Ref32 *)(ROMdata + rAddr);
						rRef = FixPointerRef(CANONICAL_LONG(*rsRef));
					}
				}
			}
		}
	}

/*-- declare objects that the original created at runtime --*/
	fprintf(fp_h, "extern Ref* RS%s;\n", "space");
	fprintf(fp_h, "extern Ref* RS%s;\n", "cFunctionPrototype");
	fprintf(fp_h, "extern Ref* RS%s;\n", "debugCFunctionPrototype");

	fprintf(fp_h, "\n/*-- for NTK --*/\n");
	fprintf(fp_h, "extern Ref* RS%s;\n", "constantFunctions");
	fprintf(fp_h, "extern Ref* RS%s;\n", "formInstallScript");
	fprintf(fp_h, "extern Ref* RS%s;\n", "formRemoveScript");
	fprintf(fp_h, "extern Ref* RS%s;\n", "autoInstallScript");

	fclose(fp_h);

/*-- build symbol data --*/
	fp_h = fopen("../Frames/RSSymbols.h", "w");
	fprintf(fp_h, kHeaderPreamble, "Symbols");
	path = "RSSymbols.s";
	if (gOutputFilename) {
		strcpy(pathStart, path); path = fullPath;
	}
	FILE * fp_rs = fopen(path, "w");
	fprintf(fp_rs, kRSPreamble, "Symbols");
	path = "RefSymbols.s";
	if (gOutputFilename) {
		strcpy(pathStart, path); path = fullPath;
	}
	FILE * fp_rd = fopen(path, "w");
	fprintf(fp_rd, kSymbolDataPreamble);

	ArrayIndex i, numOfSlots = B32Length(rRef);
	Ref sym;
	Ref32 * p, * slotPtr = ((FrameObject32 *)(rRef-1))->slot;
	for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p) {
		sym = FixPointerRef(CANONICAL_LONG(*p));
		if (B32IsSymbol(sym)) {
			const char * symName = MungeSymbol(B32SymbolName(sym));
			const char * rsValue = PrintROMObject(fp_rd, symName, NILREF, sym, true, true);
			sprintf(rsName, "SYM%s", symName);
			PrintRefStar(fp_rs, rsName, rsValue);
			fprintf(fp_h, "extern Ref* RSSYM%s;\n", symName);
		}
	}

/*-- symbols that don’t appear in the original symbol table but that we’d rather not MakeSymbol() --*/
	const char * extraSym[] = { "compilerCompatibility", "closed", "DebugHashToName", "dbg1", 0 };
	for (const char ** symp = extraSym; *symp != 0; ++symp ) {
		const char * sym = *symp;
		const char * symName = MungeSymbol(sym);
		char rsValue[128];
		PrintSymbol(fp_rd, symName, sym);
		sprintf(rsName, "SYM%s", symName);
		sprintf(rsValue, "MAKEPTR(%s)", rsName);
		PrintRefStar(fp_rs, rsName, rsValue);
		fprintf(fp_h, "extern Ref* RSSYM%s;\n", symName);
	}

/*-- symbols used by NTK --*/
	const char * ntkSym[] = { "GetKeyHandler","RemoveScript","partData","control",
									  "__origFunctions","__origVars","__platform",
									  "platformFunctions","platformWeakFunctions","platformVariables","platformConstants",
									  "SetPartFrameSlot","GetPartFrameSlot","thisView","beforeScript","afterScript",
									  "protoEditor","protoProtoEditor",
									   0 };
	fprintf(fp_h, "\n/*-- for NTK --*/\n");
	for (const char ** symp = ntkSym; *symp != 0; ++symp ) {
		const char * sym = *symp;
		const char * symName = MungeSymbol(sym);
		fprintf(fp_h, "extern Ref* RSSYM%s;\n", symName);
	}
#if forNTK
	fprintf(fp_rs, "\n/*-- for NTK --*/\n");
	fprintf(fp_rd, "\n/*-- for NTK --*/\n");
	for (const char ** symp = ntkSym; *symp != 0; ++symp ) {
		const char * sym = *symp;
		const char * symName = MungeSymbol(sym);
		char rsValue[128];
		if (strcmp(symName, "GetKeyHandler") != 0) {	// GetKeyHandler has already been defined
			PrintSymbol(fp_rd, symName, sym);
		}
		sprintf(rsName, "SYM%s", symName);
		sprintf(rsValue, "MAKEPTR(%s)", rsName);
		PrintRefStar(fp_rs, rsName, rsValue);
	}
	for (CSymbol * fn = gFunctionNames; fn != NULL; fn = fn->fNext) {	// should be NTKFunctions.newtonpkg > externalFunctions?
		const char * sym = fn->fName;
		const char * symName = MungeSymbol(sym);
		if (strcmp(symName, "ReplaceSelection") != 0) {	// ReplaceSelection has already been defined
			PrintSymbol(fp_rd, symName, sym);
		}
	}
#endif /* forNTK */

	fclose(fp_rd);
	fclose(fp_rs);
	fclose(fp_h);

/*-- PASS 2: build object data --*/
	path = "RSData.s";
	if (gOutputFilename) {
		strcpy(pathStart, path); path = fullPath;
	}
	fp_rs = fopen(path, "w");
	fprintf(fp_rs, kRSPreamble, "Data");
	path = "RefData.s";
	if (gOutputFilename) {
		strcpy(pathStart, path); path = fullPath;
	}
	fp_rd = fopen(path, "w");
	fprintf(fp_rd, kDataPreamble, "RefData");
	fprintf(fp_rd, kROMData, "Start","Start");
#if forNTK
	// define external functions
	for (CSymbol * fn = gFunctionNames; fn != NULL; fn = fn->fNext) {
		char fnName[64];
		sprintf(fnName, "obj_%s", fn->fName);
		PrintCFunction(fp_rd, fnName, fn->fCName, fn->fNumArgs);
	}
#endif /* forNTK */
	// scan address, name of object
	fseek(fp, 0, SEEK_SET);
	while (!feof(fp)) {
		lineLen = 0;
		line[0] = 0;
		if (fgets(line, 300, fp) != NULL) {
			if (line[0] != '\n') {
				int numOfItems = sscanf(line, "%8x%255s%n", &rAddr, rsName, &lineLen);
				if (numOfItems > 1 && line[0] != ';') {
					rsRef = (Ref32 *)(ROMdata + rAddr);
					rRef = FixPointerRef(CANONICAL_LONG(*rsRef));

					rsValue = PrintROMObject(fp_rd, rsName, NILREF, rRef, true, false);
					if (ISPTR(rRef) && rsValue) {
						PrintRefStar(fp_rs, rsName, rsValue);
					}
				}
			}
		}
	}

#if forNTK
	NewtonPackage scriptPkg("NTKFunctions.newtonpkg");
	// load constantFunctions
	RefVar fns(GetArraySlot(scriptPkg.partRef(0), 1));
	PrintPkgObject(fp_rd, "constantFunctions", fns, true, false);
	PrintRefStar(fp_rs, "constantFunctions", "MAKEPTR(constantFunctions)");
	// load install/remove scripts
	RefVar scripts(GetArraySlot(scriptPkg.partRef(0), 2));
	FOREACH_WITH_TAG(scripts, tag, fn)
		// print the script
		const char * fname = SymbolName(tag);
		PrintPkgObject(fp_rd, fname, fn, true, false);

		char rsValue[128];
		sprintf(rsValue, "MAKEPTR(%s)", fname);
		PrintRefStar(fp_rs, fname, rsValue);
	END_FOREACH;
#endif /* forNTK */

/*-- objects that the original created at runtime --*/
	const UniChar STRspace[] = { CANONICAL_SHORT(' '), 0 };
	PrintString(fp_rd, "space", "MAKEPTR(SYMstring)", STRspace);
	PrintRefStar(fp_rs, "space", "MAKEPTR(space)");

	PrintCFunction(fp_rd, "cFunctionPrototype", "0x00000000", 0);
	PrintRefStar(fp_rs, "cFunctionPrototype", "MAKEPTR(cFunctionPrototype)");
	PrintDebugCFunction(fp_rd, "debugCFunctionPrototype", "0x00000000", 0);
	PrintRefStar(fp_rs, "debugCFunctionPrototype", "MAKEPTR(debugCFunctionPrototype)");

	fprintf(fp_rd,	"#include \"MagicPointers.s\"\n");
	fprintf(fp_rd, kROMData, "End","End");

	fclose(fp_rd);
	fclose(fp_rs);

	fclose(fp);
#endif /*forRefStarData*/
}

