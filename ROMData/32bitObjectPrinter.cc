/*
	File:		32bitObjectPrinter.cc

	Contains:	Functions to print a 32-bit big-endian NewtonScript object.

	Written by:	Newton Research Group.
*/

#define hasByteSwapping 1
#define forNTK 1

#include "Objects.h"
#include "Ref32.h"
#include "RefMemory.h"
#include "Iterators.h"
#include "Lookup.h"
#include "NewtGlobals.h"
#include "Interpreter.h"
#include "RichStrings.h"
#include "UStringUtils.h"
#include "ROMResources.h"


extern void		PrintCode(RefArg obj);
extern void		PrintViewClass(int inViewClass);
extern void		PrintViewFlags(int inViewFlags);
extern void		PrintViewFormat(int inViewFormat);
extern void		PrintViewFont(int inViewFont);
extern void		PrintViewJustify(int inViewJustify);
extern "C" const char * GetMagicPointerString(int inMP);

extern Ref FixPointerRef(Ref r);
extern bool EQROMRef(Ref a, Ref b);
extern unsigned ROMObjectFlags(Ref r);
extern Ptr ROMBinaryData(Ref r);
extern ArrayIndex ROMLength(Ref r);
extern Ref ROMClassOf(Ref r);
extern bool IsROMSymbol(Ref r);
extern const char * ROMSymbolName(Ref r);
extern bool IsROMSubclass(Ref obj, const char * superName);
extern Ref GetArraySlotRef32(Ref r, ArrayIndex index);



DeclareException(exMessage, exRootException);


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

/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/
extern Ref		gConstantsFrame;

extern const char * simpleInstrs[];
extern const char * paramInstrs[];

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

#define kPrecedentStackSize	16
static Ref gPrecedentStack;	// 0C1056EC


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

// Forward declarations
static bool		IsAggregate(Ref obj);
static void		PrintInstructions(Ref obj);
		 Ref		Stringer(RefArg obj);
static bool		StringerStringObject(RefArg obj, Ptr outText, int * outTextSize, Ptr outInk, int * outInkSize);
void	SafelyPrintString(UniChar * str);


Ref32
GetROMTag(Ref32 inMap, ArrayIndex index, ArrayIndex * mapIndex)
{
	Ref mapRef = FixPointerRef(CANONICAL_LONG(inMap));
	if (ISNIL(mapRef)) {
		if (mapIndex)
			*mapIndex = 0;
		return NILREF;
	}

	ArrayIndex x = 0;
	FrameMapObject32 * map = (FrameMapObject32 *)(mapRef-1);
	Ref32 tag = GetROMTag(map->supermap, index, &x);	// recurse thru supermaps
	if (x == kIndexNotFound) {
		if (mapIndex)
			*mapIndex = kIndexNotFound;
		return tag;
	}

	ArrayIndex mapLength = ROMLength(mapRef) - 1 + x;
	if (mapLength > index) {
		if (mapIndex)
			*mapIndex = kIndexNotFound;
		ArrayObject32 * a = (ArrayObject32 *)(mapRef-1);
		return a->slot[index - x + 1];
	} else {
		if (mapIndex)
			*mapIndex = mapLength;
		return NILREF;
	}
}


bool
IsROMFunction(Ref r)
{
	bool isFunction = false;

	if (ISPTR(r)
	&& FLAGTEST(ROMObjectFlags(r), kObjSlotted)
	&& ROMLength(r) != 0)
	{
		Ref theClass = GetArraySlotRef32(r, 0);
		if (theClass == kPlainFuncClass
		 || theClass == kPlainCFunctionClass
		 || theClass == kBinCFunctionClass
		 || EQROMRef(theClass, SYMA(CodeBlock))
		 || EQROMRef(theClass, SYMA(binCFunction)))
			isFunction = true;
	}
	return isFunction;	
}

bool
IsNativeROMFunction(Ref r)
{
	Ref fnClass = ROMClassOf(r);
	return EQROMRef(fnClass, SYMA(_function_2Enative))
		 || EQROMRef(fnClass, SYMA(binCFunction));
}

#include "Interpreter.h"
ArrayIndex
GetROMFunctionArgCount(Ref fn)
{
	ArrayIndex	argCount = 2;
	Ref fnClass = GetArraySlotRef32(fn, kFunctionClassIndex);
	if (fnClass == kPlainFuncClass) {
		argCount = (((ArrayObject32 *)(fn-1))->slot[kFunctionNumArgsIndex]);
		argCount = RVALUE(CANONICAL_LONG(argCount) & 0xFFFF);
	} else if (fnClass == kPlainCFunctionClass) {
		argCount = ((ArrayObject32 *)(fn-1))->slot[kPlainCFunctionNumArgsIndex];
		argCount = RVALUE(CANONICAL_LONG(argCount));
	} else if (fnClass == kBinCFunctionClass) {
		argCount = GetArraySlotRef32(fn, kCFunctionNumArgsIndex);
		argCount = RINT(CANONICAL_LONG(argCount));
	} else if (EQROMRef(fnClass, SYMA(CodeBlock))) {
		argCount = GetArraySlotRef32(fn, kFunctionNumArgsIndex);
		argCount = RINT(CANONICAL_LONG(argCount));
	} else if (EQROMRef(fnClass, SYMA(binCFunction))) {
		argCount = GetFrameSlot(fn, SYMA(numArgs));
		argCount = RINT(CANONICAL_LONG(argCount));
	}

	return argCount;
}


/*------------------------------------------------------------------------------
	Initialize the precedents array for object printing.
------------------------------------------------------------------------------*/

void
InitPrinter(void)
{
	gPrecedentStack = MakeArray(kPrecedentStackSize);
	AddGCRoot(&gPrecedentStack);
}


/*------------------------------------------------------------------------------
	Print a 32-bit big-endian NewtonScript object.
	Args:		obj		the object
				indent	number of spaces to indent if new line required
				depth		depth of frame hierarchy to follow
	Return:	void
------------------------------------------------------------------------------*/
int	printDepth = 10;
int	printLength = -1;
bool	printInstructions = true;

void
PrintROMObject(Ref obj, int indent, int depth)
{
	Ref	objClass;
	ArrayIndex	i, count, numOfSlots;

	newton_try
	{
		switch (RTAG(obj)) {
		case kTagInteger:
/*-- integer --*/
		// can only be an integer
			REPprintf("%ld", RINT(obj));
			break;

		case kTagImmed:
/*-- immediate --*/
		// try boolean
			if (EQROMRef(obj, TRUEREF)) {
				REPprintf("true");
			} else if (ISNIL(obj)) {
				REPprintf("nil");
			}
		// try character
			else if (ISCHAR(obj)) {
/*-- character --*/
				UniChar	ch = RCHAR(obj);
				if (ch > 0xFF) {
					REPprintf("$\\u%04X", ch);
				} else if (ch >= 0x20 && ch <= 0x7F) {
					if (ch == 0x5C)
						REPprintf("$\\\\");
					else if (ch == 0x09)
						REPprintf("$\\t");
					else if (ch == 0x0D)
						REPprintf("$\\n");
					else
						REPprintf("$%c", ch);
				} else {
					REPprintf("$\\%02X", ch);
				}
			}

		// try weird immediate = special class
			else if (obj == kSymbolClass) {
				REPprintf("<symbol class>");
			} else if (obj == kWeakArrayClass) {
				REPprintf("<weak array class>");
			} else if (obj == kBadPackageRef) {
				REPprintf("<bad pkg ref>");

		// can't categorize it
			} else {
				REPprintf("#%p", obj);
			}
			break;

		case kTagPointer:
/*-- pointer --*/
		// try precedents array first
			for (i = 0; i < depth && i < kPrecedentStackSize-1; ++i) {
				if (EQROMRef(obj, GetArraySlotRef(gPrecedentStack, i)))
					break;
			}
			if (i < depth && i < kPrecedentStackSize-1) {
				REPprintf("<%d>", depth - i);
			} else {
				// add this object to the precedent stack
				if (depth < kPrecedentStackSize)
					SetArraySlotRef(gPrecedentStack, depth, obj);

				// switch on type of object: slotted (frame/array) or binary
				unsigned flags = ROMObjectFlags(obj);
				if (FLAGTEST(flags, kObjSlotted)) {
					if (FLAGTEST(flags, kObjFrame)) {
						if (IsROMFunction(obj)) {
							Ref fnClass = ROMClassOf(obj);
							/*if (EQROMRef(fnClass, SYMA(CodeBlock)) || EQROMRef(fnClass, SYMA(_function))) {
								PrintCode(obj);
							} else*/ {
								ArrayIndex argCount = GetROMFunctionArgCount(obj);
								const char * argPlural = (argCount == 1) ? "" : "s";
								REPprintf(IsNativeROMFunction(obj) ? "<native" : "<");
								REPprintf("function, %ld arg%s, #%p>", argCount, argPlural, obj);
							}
						} else if (depth > printDepth) {
							REPprintf("{#%p}", obj);
						} else {
						// print slots in frame
							numOfSlots = ROMLength(obj);
							indent += REPprintf("{");
							if (depth <= printDepth) {
								Ref32 map = ((FrameObject32 *)(obj-1))->map;
								Ref32 * slotPtr = ((FrameObject32 *)(obj-1))->slot;
								Ref32 * p;
								bool	isAggr = false;
								// determine whether any slot in frame is aggregate
								// and therefore this frame needs multi-line output
								for (i = 0, p = slotPtr; i < numOfSlots && !isAggr; ++i, ++p) {
									if (printLength >= 0 && i >= printLength)
										// don’t exceed number-of-slots-to-print preference
										break;
									isAggr = IsAggregate(FixPointerRef(CANONICAL_LONG(*p)));
								}
								// increment depth for slots in this frame
								depth++;
								for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p) {
									if (printLength >= 0 && i >= printLength) {
										// don’t exceed number-of-slots-to-print preference
										REPprintf("...");
										break;
									}
									Ref tag = FixPointerRef(CANONICAL_LONG(GetROMTag(map, i, NULL)));
									Ref slot = FixPointerRef(CANONICAL_LONG(*p));
									count = REPprintf("%s: ", ROMSymbolName(tag));
									if ((EQ(tag, SYMA(viewCObject)) && ISINT(slot))
									 || (EQ(tag, SYMA(viewClipper)) && ISINT(slot))
									 || EQ(tag, SYMA(funcPtr))) {
										REPprintf("#%p", RINT(slot));
									// Newton Research additions
									} else if (EQROMRef(tag, SYMA(viewClass)) && ISINT(slot)) {
										PrintViewClass(RVALUE(slot));
									} else if (EQROMRef(tag, SYMA(viewFlags)) && ISINT(slot)) {
										PrintViewFlags(RVALUE(slot));
									} else if (EQROMRef(tag, SYMA(viewFormat)) && ISINT(slot)) {
										PrintViewFormat(RVALUE(slot));
									} else if (EQROMRef(tag, SYMA(viewFont)) && ISINT(slot)) {
										PrintViewFont(RVALUE(slot));
									} else if (EQROMRef(tag, SYMA(viewJustify)) && ISINT(slot)) {
										PrintViewJustify(RVALUE(slot));
									} else {
										PrintROMObject(slot, indent + count, depth);
									}
									if (i < numOfSlots-1) {
									// separate slots
										REPprintf(", ");
										if (isAggr)
											REPprintf("\n%*s", indent, " ");
									}
								}
							}
							REPprintf("}");
						}

					// insanity check
					} else if (FLAGTEST(flags, kObjForward)) {
						REPprintf("<forwarding pointer #%p...WTF!!!>", obj);

					} else {	// must be an array
						objClass = ROMClassOf(obj);
						numOfSlots = ROMLength(obj);
						Ref32 * p, * slotPtr = ((FrameObject32 *)(obj-1))->slot;
						if (EQROMRef(objClass, SYMA(pathExpr))) {
						// it’s a path expression
							bool	hasSymbol = true;
							for (i = 0, p = slotPtr; i < numOfSlots && hasSymbol; ++i, ++p) {
								hasSymbol = IsROMSymbol(FixPointerRef(CANONICAL_LONG(*p)));
							}
							if (hasSymbol) {
							// it’s all symbols so print in dotted form
								for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p) {
									PrintROMObject(FixPointerRef(CANONICAL_LONG(*p)), indent, depth + 1);
									if (i < numOfSlots - 1)
										REPprintf(".");
								}
							} else {
							// path expression contains non-symbols so enumerate the elements
								REPprintf("[pathExpr: ");
								for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p) {
									PrintROMObject(FixPointerRef(CANONICAL_LONG(*p)), indent, depth + 1);
									if (i < numOfSlots - 1)
										REPprintf(", ");
								}
								REPprintf("]");
							}
						} else if (depth > printDepth) {
							REPprintf("[#%p]", obj);
						} else {
						// print elements in array
							indent += REPprintf("[");
							if (depth <= printDepth) {
								bool	isAggr = false;
								for (i = 0, p = slotPtr; i < numOfSlots && !isAggr; ++i, ++p) {
									if (printLength >= 0 && i >= printLength)
										break;
									isAggr = IsAggregate(FixPointerRef(CANONICAL_LONG(*p)));
								}
								if (!EQROMRef(objClass, SYMA(array))) {
									if (IsROMSymbol(objClass)) {
										count = REPprintf("%s: ", ROMSymbolName(objClass));
										if (isAggr)
											REPprintf("\n%*s", indent, " ");
										else
											indent += count;
									} else {
										PrintROMObject(objClass, indent, depth + 1);
										if (isAggr)
											REPprintf(": \n%*s", indent, " ");
										else
											REPprintf(": ");
									}
								}
								for (i = 0, p = slotPtr; i < numOfSlots; ++i, ++p) {
									if (printLength >= 0 && i >= printLength) {
										// exceeded number-of-slots-to-print preference
										REPprintf("...");
										break;
									}
									PrintROMObject(FixPointerRef(CANONICAL_LONG(*p)), indent, depth + 1);
									if (i < numOfSlots-1) {
									// separate slots
										REPprintf(", ");
										if (isAggr)
											REPprintf("\n%*s", indent, " ");
									}
								}
							}
							REPprintf("]");
						}
					}
				}

			else	// not slotted
			{
				objClass = ROMClassOf(obj);

				if (IsROMSymbol(obj))
				{
					unsigned char		ch;
					unsigned char *	s = (unsigned char *)ROMSymbolName(obj);
					bool		isWeird = true;		// assume symbol contains character other than alpha|digit|underscore

					if ((ch = *s) != kEndOfString
					 && ((sType[ch] & (isUppercase|isLowercase)) != 0 || ch == '_'))
					{
						isWeird = false;
						for (unsigned char * p = s; (ch = *p) != kEndOfString; p++)
						{
							if (((sType[ch] & (isUppercase|isLowercase|isDigit)) != 0) || ch == '_')
								/* it’s good */;
							else
							{
								isWeird = true;
								break;
							}
						}
					}
					if (isWeird)
					{
						// symbol contains special characters so enclose it in |brackets|
						REPprintf("'|");
						isWeird = false;	// re-purpose the flag to mean isCtrlOr8bitChar
						while ((ch = *s++) != kEndOfString)
						{
							if (isWeird)
							{
								if (ch >= 0x20 && ch <= 0x7F)
								{
									if (ch == 0x5C)
										REPprintf("\\u\\");
									else if (ch == 0x7C)
										REPprintf("\\u|");
									else
										REPprintf("\\u%c", ch);
									isWeird = false;
								}
								else
									REPprintf("%02X", ch);
							}
							else if (ch >= 0x20 && ch <= 0x7F)
							{
								if (ch == 0x5C)
									REPprintf("\\\\");
//									else if (ch == 0x7C)
//										REPprintf("\\|");
								else
									REPprintf("%c", ch);
							}
							else if (ch == 0x0D)
								REPprintf("\\n");
							else if (ch == 0x09)
								REPprintf("\\t");
							else
							{
								REPprintf("\\u%02X", ch);
								isWeird = true;
							}
						}
						REPprintf("|");
					}
					else
						// it’s a simple symbol
						REPprintf("'%s", s);
				}

				else if (EQROMRef(objClass, SYMA(real)))
					REPprintf("%#g", *(double*)ROMBinaryData(obj));

//				else if (EQROMRef(objClass, SYMA(instructions)) && printInstructions)
//					PrintInstructions(obj);

				else if (IsROMSymbol(objClass))
				{
					if (IsROMSubclass(objClass, "string"))
					{
						REPprintf("\"");
						SafelyPrintString((UniChar *)ROMBinaryData(obj));
						REPprintf("\"");
					}
					else
						REPprintf("<%s, length %d>", ROMSymbolName(objClass), ROMLength(obj));
				}

				else
				{
					REPprintf("<binary, class ");
					PrintROMObject(objClass, indent, depth + 1);
					REPprintf(", length %d>", ROMLength(obj));
				}
			}

			if (depth < kPrecedentStackSize)
				SetArraySlotRef(gPrecedentStack, depth, NILREF);
			}
			break;

		case kTagMagicPtr:
/*-- magic pointer --*/
			REPprintf(GetMagicPointerString(RVALUE(obj)));
			break;
		}
	}

	newton_catch(exMessage)
	{
		REPprintf("*** %s ***", CurrentException()->data);
	}
	newton_catch(exRootException)
	{
		REPprintf("*** %s ***", CurrentException()->name);
	}
	end_try;

	fflush(stdout);
}


/*------------------------------------------------------------------------------
	Determine whether an object is an aggregate of other objects
	(ie is an array or frame).
	Args:		obj		an object
	Return:  true|false
------------------------------------------------------------------------------*/

static bool
IsAggregate(Ref obj)
{
	return ISREALPTR(obj) && FLAGTEST(ROMObjectFlags(obj), kObjSlotted);
}


/*------------------------------------------------------------------------------
	Print a unicode string.
	Break it up into 256-char chunks and print as ASCII.
	Args:		str		a unicode string
	Return:  --
------------------------------------------------------------------------------*/

void
SafelyPrintString(UniChar * str)
{
	UniChar ubuf[256];
	char	buf[256];
	int	bufLen, len = Ustrlen(str);

	while (len > 0)
	{
		bufLen = MIN(len, 255);
		for (ArrayIndex i = 0; i < bufLen; ++i) {
			ubuf[i] = CANONICAL_SHORT(str[i]);
		}
		ConvertFromUnicode(ubuf, buf, bufLen);
		str += bufLen;
		len -= bufLen;
		REPprintf("%s", buf);
	}
}


/*------------------------------------------------------------------------------
	Concatenate the elements of an array into a string.
	Args:		obj		an array object
	Return:  a string object
------------------------------------------------------------------------------*/

Ref
Stringer(RefArg obj)
{
	int  textSize, inkSize;
	int  textLength = 0;
	int  inkLength = 0;
	int  strLength, inkOffset;
	ArrayIndex  i, count = Length(obj);

	// pass 1: calculate length of string to allocate
	for (i = 0; i < count; ++i)
	{
		StringerStringObject(GetArraySlotRef(obj, i), NULL, &textSize, NULL, &inkSize);
		textLength += textSize;
		inkLength += inkSize;
	}
	if (inkLength == 0)
	{
		inkOffset = 0;
		strLength = textLength + sizeof(UniChar);
	}
	else
	{
		inkOffset = LONGALIGN(textLength + sizeof(UniChar));
		strLength = inkOffset + inkLength + 4;
	}

	RefVar	strObj(AllocateBinary(SYMA(string), strLength));
	CDataPtr lockedObj(strObj);
	Ptr		strPtr = (Ptr) lockedObj;
	Ptr		inkPtr = strPtr + inkOffset;

	// pass 2: populate the string
	for (i = 0; i < count; ++i)
	{
		StringerStringObject(GetArraySlotRef(obj, i), strPtr, &textSize, inkPtr, &inkSize);
		strPtr += textSize;
		inkPtr += inkSize;		
	}
	if (inkLength > 0)
	{
		uint32_t  inkInfo = ((textLength/sizeof(UniChar)) << 4) | 0x01;
		inkPtr = (Ptr) lockedObj + strLength;
		*(uint32_t *)inkPtr = inkInfo;
	}
	return strObj;
}


static bool
StringerStringObject(RefArg obj, Ptr outText, int * outTextSize, Ptr outInk, int * outInkSize)
{
	if (IsString(obj))
	{
		CRichString richStr(obj);
		richStr.doStringerStuff(outText, outTextSize, outInk, outInkSize);
		return true;
	}

	else if (ISNIL(obj))
	{
		*outTextSize = *outInkSize = 0;
		return true;
	}

	else if (ISCHAR(obj))
	{
		*outTextSize = sizeof(UniChar);
		*outInkSize = 0;
		if (outText != NULL)
			*(UniChar *)outText = RCHAR(obj);
		return true;		
	}

	else if (ISINT(obj))
	{
		char  str[32];
		int  strLen = sprintf(str, "%ld", RINT(obj));
		*outTextSize = strLen * sizeof(UniChar);
		*outInkSize = 0;
		if (outText != NULL)
			ConvertToUnicode(str, (UniChar *)outText, strLen);
		return true;		
	}

	else if (IsReal(obj))
	{
		char  str[32];
		const char * strPtr;
		int  strLen;
		double	value = CDouble(obj);
		if (value == 0.0)
		{
			strPtr = "0.0";
			strLen = strlen(strPtr);
		}
		else
		{
			strPtr = str;
			strLen = sprintf(str, "%g", value);
		}
		*outTextSize = strLen * sizeof(UniChar);
		*outInkSize = 0;
		if (outText != NULL)
			ConvertToUnicode(strPtr, (UniChar *)outText, strLen);
		return true;		
	}

	else if (IsSymbol(obj))
	{
		const char * sym = SymbolName(obj);
		int symLen = strlen(sym);
		*outTextSize = symLen;
		*outInkSize = 0;
		if (outText != NULL)
			ConvertToUnicode(sym, (UniChar *)outText, symLen);
		return true;
	}

	*outTextSize = *outInkSize = 0;
	return false;
}


/*------------------------------------------------------------------------------
	More object printing stuff.
------------------------------------------------------------------------------*/

Ref
FindSlotName(RefArg context, RefArg obj)
{
	Ref	slotName = NILREF;
	CObjectIterator	iter(context, true);
	for ( ; !iter.done(); iter.next())
	{
		if (EQ(obj, iter.value()))
		{
			slotName = iter.tag();
			break;
		}
	}
	return slotName;
}


static Ref
CheckForObjectName(RefArg context, const char * contextName, RefArg obj)
{
	if (EQ(obj, context))
		return MakeStringFromCString(contextName);

	RefVar	slotName(FindSlotName(context, obj));
	if (NOTNIL(slotName))
	{
		RefVar	strArray(MakeArray(3));
		SetArraySlotRef(strArray, 0, MakeStringFromCString(contextName));
		SetArraySlotRef(strArray, 1, MAKECHAR('.'));
		SetArraySlotRef(strArray, 2, slotName);
		return Stringer(strArray);
	}

	return NILREF;
}


static Ref
SearchForObjectName(RefArg obj)
{
	Ref	objName;

	if (NOTNIL(objName = CheckForObjectName(gVarFrame, "vars", obj)))
		return objName;

	if (NOTNIL(objName = CheckForObjectName(gFunctionFrame, "functions", obj)))
		return objName;

#if defined(hasGlobalConstantFunctions)
	if (NOTNIL(objName = CheckForObjectName(gConstantsFrame, "constants", obj)))
		return objName;
#endif

#if defined(hasPureFunctionSupport)
	if (NOTNIL(objName = CheckForObjectName(gConstFuncFrame, "constant functions", obj)))
		return objName;
#endif

	return NILREF;
}


void
PrintWellKnownObject(Ref obj, int indent)
{
	if (IsAggregate(obj))
		PrintObject(obj, indent);
	else
	{
		RefVar name(SearchForObjectName(obj));
		if (NOTNIL(name))
			REPprintf("(%s)", ROMBinaryData(ASCIIString(name)));
		else
		{
			int count = REPprintf("(#%p) ", obj);
			PrintObject(obj, indent + count);
		}
	}
}

