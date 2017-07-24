/*
	File:		32bitObjects.cc

	Contains:	Functions to handle 32-bit big-endian non-forwarding NewtonScript objects.

	Written by:	Newton Research Group.
*/

#include <ctype.h>	// tolower

#include "Objects.h"
#include "ObjHeader.h"
#include "Ref32.h"
#include "ROMResources.h"

extern Ref FixPointerRef(Ref r);


/* -----------------------------------------------------------------------------
	Return name of 32-bit symbol.
	Correct capitalisation while we’re about it.
	Args:		r			pointer Ref within ROM data block
	Return:	C string, not necessarily within symbol object
----------------------------------------------------------------------------- */

const char *
B32SymbolName(Ref r)
{
	const char * target[] =	{
		"real", "array", "pathExpr", "trace", "tagSpec", "blessedApp", "frameParameter", "punctuationCursiveOption", "modalState",
		"viewFrontMost","viewFrontMostApp","viewFrontKey", "viewGestureScript", "copyProtection", "viewGrid","lineGrid","squareGrid",
		"viewLinePattern","viewDragFeedbackScript","viewDrawDragBackgroundScript","viewDrawDragDataScript",
		"lastEx", "lastExMessage", "lastExError", "lastExData", "secOrder", "indexValidTest", "Move","Prev",
		"TCursor", "TStore", "entireWords", "diskSoup", "Flush", "don\'tActivate", "SuckPackageFromEndpoint",
		"sortId", "dirSortId", "encodingId", "_uniqueId", "lastUId", "indexNextUId", "EntryUniqueId", "AddWithUniqueId", "GetNextUId", "dictId", "ROMdictId", "linkedDictId","PSSidToPid","PSSidToPkgRef",
		"inputMask", "currentLocaleBundle", "numberFormat", "decimalPoint", "printLength", "viewClipper",
		"pickSeparator","pickSolidSeparator","strokeList","cellFrame","outerFrame",
		"colorData", "colorTable", "cBits", "bitDepth", "boundsRect", "rowBytes", "companderName", "companderData", "polygonShape","polygonData",
		"textData","textBox", "roundRectangle", "selection", "penSize",
		"serialGPI", "cardLock", "interconnect", "highROM", "vbo", "typeList", "debuggerInfo","rebootReason",
		"ROMVersionString","CPUSpeed","CPUType","strongARM","ARM610A","ARM710A","screenWidth","screenHeight","screenResolutionX","screenResolutionY","screenDepth","tabletResolutionX","tabletResolutionY","manufactureDate",
		"_curClick", "_clickSong", "Clicker", "activePackageList","_importTable","_exportTable",
		"shortMonth", "terseMonth", "timeFormat", "midnightForm", "daylightSavings","sleepTime","typeSelectTimeout","keyboard","mapping",
		"personAdded", "placeAdded", "DSQuery", "lexicon", "Parse", "PostParse", "IARef", "dynaTemplates", "matchString", "noiseWords", "origPhrase",
		"callbackSpec", "callbackFreq", "callbacks", "callbackContext", "callbackParams", "supportsCallback", "callbackFn", "GetSoupChangeCallbacks",
		"constantFunctions",
		"Aref","SetAref","Length","Clone","Stringer","Map","Send","GetIndexChar","GetLetterIndex","GC",
		"SetClass","AddArraySlot","GetSlot","SetSlot","GetPath","SetPath","HasVar","HasPath",
		"NewIterator","ModalState","Translate","GetCardInfo","SetSortId",
		"stepAllocateContext","preAllocatedContext",
		0 };

	SymbolObject32 * obj = (SymbolObject32 *)(r-1);
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
	Return 32-bit object flags.
	Args:		a			32-bit Ref
	Return:	flags
----------------------------------------------------------------------------- */

unsigned
B32ObjectFlags(Ref r)
{
	ObjHeader32 * obj = (ObjHeader32 *)(r-1);
	return obj->flags;
}


/* -----------------------------------------------------------------------------
	Return 32-bit object data.
	Args:		a			32-bit Ref
	Return:	pointer to data
----------------------------------------------------------------------------- */

Ptr
B32BinaryData(Ref r)
{
	BinaryObject32 * obj = (BinaryObject32 *)(r-1);
	return obj->data;
}


/* -----------------------------------------------------------------------------
	Return 32-bit object length.
	Args:		a			32-bit Ref
	Return:	length
----------------------------------------------------------------------------- */

ArrayIndex
B32Length(Ref r)
{
	ObjHeader32 * obj = (ObjHeader32 *)(r-1);
	if (FLAGTEST(obj->flags, kObjSlotted))
		return ARRAY32LENGTH(obj);
	return BINARY32LENGTH(obj);
}


/* -----------------------------------------------------------------------------
	Compare pointer Ref within ROM data block with 64-bit Ref.
	Args:		a			pointer Ref within ROM data block
				b			64-bit Ref
	Return:	true => objects are equal (not necessarily identical)
----------------------------------------------------------------------------- */

static bool
B32EQ1(Ref a, Ref b)
{
	ObjHeader32 * obj1;
	ObjHeader * obj2;

	if (ISREALPTR(a))
		obj1 = (ObjHeader32 *)(a-1);
	else
		obj1 = NULL;

	if (ISREALPTR(b))
		obj2 = (ObjHeader *)(b-1);
	else
		obj2 = NULL;

	if ((long)obj1 == (long)obj2)
		return true;
	if ((obj1 == NULL) || (obj2 == NULL))
		return false;
//	There are some loose symbols in the ROM - they aren’t in the symbol table
//	so we have to make a manual check for them.
	return (((SymbolObject32 *)obj1)->objClass == CANONICAL_LONG(kSymbolClass) && ((SymbolObject *)obj2)->objClass == kSymbolClass
	    &&  CANONICAL_LONG(((SymbolObject32 *)obj1)->hash) == ((SymbolObject *)obj2)->hash
	    &&  symcmp(((SymbolObject32 *)obj1)->name, ((SymbolObject *)obj2)->name) == 0);
}


bool
B32EQRef(Ref a, Ref b)
{
	return (a == b) ? true : ((a & b & kTagPointer) ? B32EQ1(a,b) : false);
}


bool
B32IsSymbol(Ref r)
{
	return ISPTR(r)
	&& ((SymbolObject32 *)(r-1))->objClass == CANONICAL_LONG(kSymbolClass);
}



bool
B32IsSubclass(Ref obj, const char * superName)
{
	const char * subName = B32SymbolName(obj);
	for ( ; *subName && *superName; subName++, superName++) {
		if (tolower(*subName) != tolower(*superName)) {
			return false;
		}
	}
	return (*superName == 0 && (*subName == 0 || *subName == '.'));
}


/* -----------------------------------------------------------------------------
	Return class of 32-bit Ref.
	Args:		r			32-bit Ref
	Return:	64-bit class
----------------------------------------------------------------------------- */
static Ref B32GetProtoVariable(Ref inRcvr, Ref inTag, bool * outExists);

Ref
B32ClassOf(Ref r)
{
	long tag = RTAG(r);

	if (tag == kTagInteger)
		return NILREF;

	if (tag == kTagImmed)
		return NILREF;

	if (ISPTR(tag))
	{
		unsigned flags = B32ObjectFlags(r);
		if (FLAGTEST(flags, kObjSlotted))
		{
			if (FLAGTEST(flags, kObjFrame))
			{
				Ref theClass = B32GetProtoVariable(r, SYMA(class), NULL);
				if (NOTNIL(theClass))
				{
					if (theClass == kPlainFuncClass)
						return SYMA(_function);
					else if (theClass == kPlainCFunctionClass
							|| theClass == kBinCFunctionClass)
						return SYMA(_function_2Enative);
					return theClass;
				}
				return SYMA(frame);
			}
			return FixPointerRef(CANONICAL_LONG(((ArrayObject32 *)(r-1))->objClass));
		}
		else if (((BinaryObject32 *)(r-1))->objClass == CANONICAL_LONG(kWeakArrayClass))
			return SYMA(_weakarray);
		else if (((SymbolObject32 *)(r-1))->objClass == CANONICAL_LONG(kSymbolClass))
			return SYMA(symbol);
		return FixPointerRef(CANONICAL_LONG(((BinaryObject32 *)(r-1))->objClass));
	}

	return NILREF;	//	should never happen
}


/* -----------------------------------------------------------------------------
	Find proto variable within 32-bit frame.
	Args:		inRcvr		32-bit frame
				inTag			64-bit slot tag
				outExists	true => we found it (but may be nil)
	Return:	slot value
----------------------------------------------------------------------------- */

bool
UnsafeSymbolEqual(Ref sym1, Ref sym2, ULong hash)
{
	if (sym1 == sym2)
		return true;
	if (((SymbolObject32 *)(sym1-1))->hash != CANONICAL_LONG(hash))
		return false;
	return symcmp(((SymbolObject32 *)(sym1-1))->name,
					  ((SymbolObject *)(sym2-1))->name) == 0;
}


#define kMaxDepth 16
static ArrayIndex
B32FindOffset1(Ref inMap, Ref tag, Ref * outMap)
{
	FrameMapObject32 *	stack[kMaxDepth];
	FrameMapObject32 *	map;
	int		depth = 0;
	long		offset = 0;
	ULong		hash = ((SymbolObject *)PTR(tag))->hash;

	for (map = (FrameMapObject32 *)(inMap-1);
		  NOTNIL(CANONICAL_LONG(map->supermap));
		  inMap = FixPointerRef(CANONICAL_LONG(map->supermap)), map = (FrameMapObject32 *)(inMap-1))
	{
		stack[depth++] = map;
		if (depth == kMaxDepth)
		{
			offset = B32FindOffset1(map->supermap, tag, outMap);
			if (NOTNIL(*outMap))
				return offset;
			depth = kMaxDepth - 1;
			break;
		}
	}

	while (depth >= 0)
	{
		Ref32 *	slotPtr = &map->slot[0];
		for (ArrayIndex len = ARRAY32LENGTH(map) - 1; len != 0; --len, ++slotPtr)	// length excluding supermap
		{
			Ref slotSym = FixPointerRef(CANONICAL_LONG(*slotPtr));
			if (slotSym == tag || UnsafeSymbolEqual(slotSym, tag, hash))
			{
				*outMap = MAKEPTR(map);
				return offset;
			}
			offset++;
		}
		if (depth-- > 0)
			map = stack[depth];
	}

	*outMap = NILREF;
	return offset;
}


#define k_protoHash  0x6622439B
static ArrayIndex
B32FindOffset(Ref map, Ref tag)
{
	ArrayIndex offset = kIndexNotFound;

	if ((tag == SYMA(_proto) || UnsafeSymbolEqual(tag, SYMA(_proto), k_protoHash))
	&&  (((FrameMapObject32 *)(map-1))->objClass & kMapProto) == 0)
		;	// we’re looking for a _proto slot but there’s no proto chain
	else
	{
		Ref	implMap;
		offset = B32FindOffset1(map, tag, &implMap);
		if (ISNIL(implMap))
			offset = kIndexNotFound;
	}

	return offset;
}


static Ref
B32GetProtoVariable(Ref inRcvr, Ref inTag, bool * outExists)
{
	// throw exception if no context
	if (ISNIL(inRcvr))
		ThrowExInterpreterWithSymbol(kNSErrNilContext, inRcvr);

	// cache lookups expect this pointer
	bool exists;
	if (outExists == NULL)
		outExists = &exists;

	Ref value;
	Ref impl = inRcvr;
	while (NOTNIL(impl))
	{
		FrameObject32 * frPtr = (FrameObject32 *)(impl-1);
		// must have a frame
		if (!ISFRAME(frPtr))
#if forNTK
			break;
#else
			ThrowBadTypeWithFrameData(kNSErrNotAFrame, impl);
#endif

		// look in the current frame map for the name of the variable
		Ref frMap = FixPointerRef(CANONICAL_LONG(frPtr->map));
		ArrayIndex slotIndex = B32FindOffset(frMap, inTag);
		if (slotIndex != kIndexNotFound)
		{
			// found it!
			*outExists = true;
			value = ((FrameObject32 *)(impl-1))->slot[slotIndex];
			return FixPointerRef(CANONICAL_LONG(value));
		}

		// follow the _proto chain
		slotIndex = B32FindOffset(frMap, SYMA(_proto));
		if (slotIndex == kIndexNotFound)
			break;
		impl = ((FrameObject32 *)(impl-1))->slot[slotIndex];
		impl = FixPointerRef(CANONICAL_LONG(impl));
	}

	// variable wasn’t found
	*outExists = false;
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Get slot from index within 32-bit frame.
	Args:		r				32-bit frame
				index			index within frame
	Return:	32-bit slot value
----------------------------------------------------------------------------- */

static Ref
B32GetArraySlotRef(Ref r, ArrayIndex index)
{
	ArrayObject32 * obj = (ArrayObject32 *)(r-1);
	if (FLAGTEST(obj->flags, kObjSlotted)
	&& index < ARRAY32LENGTH(obj)) {
		return FixPointerRef(CANONICAL_LONG(obj->slot[index]));
	}
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Get 32-bit slot tag from index within 32-bit frame map.
	Args:		inMap			32-bit frame map
				index			index within frame
				mapIndex		actual index within (super)map frame
	Return:	32-bit slot tag
----------------------------------------------------------------------------- */

Ref
B32GetTag(Ref inMap, ArrayIndex index, ArrayIndex * mapIndex)
{
	if (ISNIL(inMap))
	{
		if (mapIndex)
			*mapIndex = 0;
		return NILREF;
	}

	ArrayIndex x = 0;
	FrameMapObject32 * map = (FrameMapObject32 *)(inMap-1);
	Ref tag = B32GetTag(FixPointerRef(CANONICAL_LONG(map->supermap)), index, &x);	// recurse thru supermaps
	if (x == kIndexNotFound)
	{
		if (mapIndex)
			*mapIndex = kIndexNotFound;
		return tag;
	}

	ArrayIndex mapLength = B32Length(inMap) - 1 + x;
	if (mapLength > index)
	{
		if (mapIndex)
			*mapIndex = kIndexNotFound;
		return B32GetArraySlotRef(inMap, index - x + 1);
	}
	else
	{
		if (mapIndex)
			*mapIndex = mapLength;
		return NILREF;
	}
}

