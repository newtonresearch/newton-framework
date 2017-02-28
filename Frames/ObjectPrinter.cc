/*
	File:		ObjectPrinter.cc

	Contains:	Functions to print a NewtonScript object.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "RefMemory.h"
#include "Iterators.h"
#include "Lookup.h"
#include "Interpreter.h"
#include "RichStrings.h"
#include "UStringUtils.h"
#include "ROMResources.h"

extern "C" const char * GetMagicPointerString(int inMP);

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
Ref	FFramesStringer(RefArg rcvr, RefArg inArray);
Ref	FEvalStringer(RefArg inRcvr, RefArg inFrame, RefArg inArray);
Ref	FPrint(RefArg inRcvr, RefArg inObj);
Ref	FWrite(RefArg inRcvr, RefArg inObj);
}


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

static Ref gPrecedentStack;	// 0C1056EC


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern void PrintCode(RefArg obj);
extern "C" {
		ArrayIndex	GetFunctionArgCount(Ref fn);
}

// Forward declarations
		 void		PrintObjectAux(Ref obj, int indent, int depth);
static bool		IsAggregate(Ref obj);
static void		PrintInstructions(Ref obj);
		 Ref		Stringer(RefArg obj);
static bool		StringerStringObject(RefArg obj, Ptr outText, int * outTextSize, Ptr outInk, int * outInkSize);
		 void		PrintViewClass(int inViewClass);
		 void		PrintViewFlags(int inViewFlags);
		 void		PrintViewFormat(int inViewFormat);
		 void		PrintViewFont(int inViewFont);
		 void		PrintViewJustify(int inViewJustify);


extern "C" {
		void		SafelyPrintString(UniChar * str);
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
	Print a NewtonScript object.
	Args:		obj		the object
				indent	number of spaces to indent if new line required
				depth		depth of frame hierarchy to follow
	Return:	void
------------------------------------------------------------------------------*/

void
PrintObjectAux(Ref obj, int indent, int depth)
{
	Ref	objClass;
	ArrayIndex	i, count, numOfSlots;

	obj = ForwardReference(obj);
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
			if (EQRef(obj, TRUEREF)) {
				REPprintf("true");
			} else if (ISNIL(obj)) {
				REPprintf("nil");
		// try character
			} else if (ISCHAR(obj)) {
/*-- character --*/
				UniChar	ch = RCHAR(obj);
				if (ch > 0xFF) {
					// it’s a Unicode char
					REPprintf("$\\u%04X", ch);
				} else if (ch >= 0x20 && ch <= 0x7F) {
					// it’s a printable ASCII char
					if (ch == 0x5C)
						REPprintf("$\\\\");
					else
						REPprintf("$%c", ch);
				// else it’s a control char
				} else if (ch == 0x09) {
					REPprintf("$\\t");
				} else if (ch == 0x0D) {
					REPprintf("$\\n");
				} else {
					REPprintf("$\\%02X", ch);
				}
			}

		// try weird immediate = special class
			else if (obj == kSymbolClass)
				REPprintf("<symbol class>");
			else if (obj == kWeakArrayClass)
				REPprintf("<weak array class>");
			else if (obj == kBadPackageRef)
				REPprintf("<bad pkg ref>");
		// can't categorize it
			else
				REPprintf("#%p", obj);
			break;

		case kTagPointer:
/*-- pointer --*/
		// try precedents array first
			for (i = 0; i < depth && i < kPrecedentStackSize-1; ++i) {
				if (EQ(obj, GetArraySlotRef(gPrecedentStack, i))) {
					break;
				}
			}
			if (i < depth && i < kPrecedentStackSize-1) {
				REPprintf("<%d>", depth - i);

			} else {
				Ref	prLenRef;
				int	printDepth, printLength;
				bool	doPrettyPrint;
				unsigned	flags;
				
				// add this object to the precedent stack
				if (depth < kPrecedentStackSize)
					SetArraySlotRef(gPrecedentStack, depth, obj);

				// get print parameters
				printDepth = RINT(GetFrameSlot(gVarFrame, SYMA(printDepth)));
				if (printDepth > kPrecedentStackSize-1)
					printDepth = kPrecedentStackSize-1;
				prLenRef = GetFrameSlot(gVarFrame, SYMA(printLength));
				printLength = NOTNIL(prLenRef) ? RINT(prLenRef) : -1;
				doPrettyPrint = NOTNIL(GetFrameSlot(gVarFrame, SYMA(prettyPrint)));
				// switch on type of object: slotted (frame/array) or binary
				flags = ObjectFlags(obj);
				if (FLAGTEST(flags, kObjSlotted)) {
					if (FLAGTEST(flags, kObjFrame)) {
						if (IsFunction(obj) && doPrettyPrint) {
							RefVar fnClass = ClassOf(obj);
							if (EQ(fnClass, SYMA(CodeBlock)) || EQ(fnClass, SYMA(_function))) {
								PrintCode(obj);
							} else {
								ArrayIndex argCount = GetFunctionArgCount(obj);
								const char * argPlural = (argCount == 1) ? "" : "s";
								int saveTrace = gInterpreter->tracing;		// | not original
								gInterpreter->tracing = kTraceNothing;		// | but required to prevent infinite loop in IsNativeFunction > ClassOf > GetProtoVariable > gInterpreter->TraceGet
								REPprintf(IsNativeFunction(obj) ? "<native" : "<");
								gInterpreter->tracing = saveTrace;			// |
								REPprintf("function, %ld arg%s, #%p>", argCount, argPlural, obj);
							}
						} else if (depth > printDepth) {
							REPprintf("{#%p}", obj);
						} else {
						// print slots in frame
							numOfSlots = Length(obj);
							indent += REPprintf("{");
							if (doPrettyPrint && depth <= printDepth) {
								Ref	tag;
								bool	isAggr = false;
								// determine whether any slot in frame is aggregate
								// and therefore this frame needs multi-line output
								CObjectIterator	iter(obj);
								for (i = 0; !iter.done() && !isAggr; iter.next(), ++i) {
									if (printLength >= 0 && i >= printLength)
										// don’t exceed number-of-slots-to-print preference
										break;
									isAggr = IsAggregate(iter.value());
								}
								// increment depth for slots in this frame
								depth++;
								for (iter.reset(), i = 0; !iter.done(); iter.next(), ++i) {
									if (printLength >= 0 && i >= printLength) {
										// don’t exceed number-of-slots-to-print preference
										REPprintf("...");
										break;
									}
									tag = iter.tag();
									count = REPprintf("%s: ", SymbolName(tag));
									if ((EQ(tag, SYMA(viewCObject)) && ISINT(iter.value()))
									 || (EQ(tag, SYMA(viewClipper)) && ISINT(iter.value()))
									 || EQ(tag, SYMA(funcPtr))) {
										REPprintf("#%p", RINT(iter.value()));
									// Newton Research additions
									} else if (EQ(tag, SYMA(viewClass)) && ISINT(iter.value())) {
										PrintViewClass(RVALUE(iter.value()));
									} else if (EQ(tag, SYMA(viewFlags)) && ISINT(iter.value())) {
										PrintViewFlags(RVALUE(iter.value()));
									} else if (EQ(tag, SYMA(viewFormat)) && ISINT(iter.value())) {
										PrintViewFormat(RVALUE(iter.value()));
									} else if (EQ(tag, SYMA(viewFont)) && ISINT(iter.value())) {
										PrintViewFont(RVALUE(iter.value()));
									} else if (EQ(tag, SYMA(viewJustify)) && ISINT(iter.value())) {
										PrintViewJustify(RVALUE(iter.value()));
									} else {
										PrintObjectAux(iter.value(), indent + count, depth);
									}
									if (--numOfSlots > 0) {
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
						numOfSlots = Length(obj);
						objClass = ClassOf(obj);
						if (EQ(objClass, SYMA(pathExpr))) {
						// it’s a path expression
							bool	hasSymbol = true;
							for (i = 0; i < numOfSlots && hasSymbol; ++i) {
								hasSymbol = IsSymbol(GetArraySlotRef(obj, i));
							}
							if (hasSymbol) {
							// it’s all symbols so print in dotted form
								for (i = 0; i < numOfSlots; ++i) {
									REPprintf("%s", SymbolName(GetArraySlotRef(obj, i)));
									if (i < numOfSlots - 1)
										REPprintf(".");
								}
							} else {
							// path expression contains non-symbols so enumerate the elements
								REPprintf("[pathExpr: ");
								for (i = 0; i < numOfSlots; ++i) {
									PrintObjectAux(GetArraySlotRef(obj, i), indent, depth + 1);
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
							if (doPrettyPrint && depth <= printDepth) {
								bool	isAggr = false;
								CObjectIterator	iter(obj);
								for (i = 0; !iter.done() && !isAggr; iter.next(), ++i) {
									if (printLength >= 0 && i >= printLength)
										break;
									isAggr = IsAggregate(iter.value());
								}
								if (!EQ(objClass, SYMA(array))) {
									if (IsSymbol(objClass)) {
										count = REPprintf("%s: ", SymbolName(objClass));
										if (isAggr)
											REPprintf("\n%*s", indent, " ");
										else
											indent += count;
									} else {
										PrintObjectAux(objClass, indent, depth + 1);
										if (isAggr)
											REPprintf(": \n%*s", indent, " ");
										else
											REPprintf(": ");
									}
								}
								
								for (iter.reset(), i = 0; !iter.done(); iter.next(), ++i) {
									if (printLength >= 0 && i >= printLength) {
										// exceeded number-of-slots-to-print preference
										REPprintf("...");
										break;
									}
									PrintObjectAux(iter.value(), indent, depth + 1);
									if (--numOfSlots > 0) {
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

				} else {	// not slotted
					objClass = ClassOf(obj);
					if (IsSymbol(obj)) {
						unsigned char		ch;
						unsigned char *	s = (unsigned char *)SymbolName(obj);
						bool		isWeird = true;		// assume symbol contains character other than alpha|digit|underscore

						if ((ch = *s) != kEndOfString
						 && ((sType[ch] & (isUppercase|isLowercase)) != 0 || ch == '_')) {
							isWeird = false;
							for (unsigned char * p = s; (ch = *p) != kEndOfString; p++) {
								if (((sType[ch] & (isUppercase|isLowercase|isDigit)) != 0) || ch == '_') {
									/* it’s good */;
								} else {
									isWeird = true;
									break;
								}
							}
						}
						if (isWeird) {
							// symbol contains special characters so enclose it in |brackets|
							REPprintf("'|");
							isWeird = false;	// re-purpose the flag to mean isCtrlOr8bitChar
							while ((ch = *s++) != kEndOfString) {
								if (isWeird) {
									if (ch >= 0x20 && ch <= 0x7F) {
										// it’s no longer weird
										REPprintf("\\u");
										isWeird = false;
										if (ch == 0x5C)
											REPprintf("\\\\");
										else if (ch == 0x7C)
											REPprintf("\\|");
										else
											REPprintf("%c", ch);
									} else {
										REPprintf("%02X", ch);
									}
								} else if (ch >= 0x20 && ch <= 0x7F) {
									if (ch == 0x5C)
										REPprintf("\\\\");
									else if (ch == 0x7C)
										REPprintf("\\|");
									else
										REPprintf("%c", ch);
								} else {
									REPprintf("\\u%02X", ch);
									isWeird = true;
								}
							}
							REPprintf("|");
						} else {
							// it’s a simple symbol
							REPprintf("'%s", s);
						}

					} else if (EQ(objClass, SYMA(real))) {
						REPprintf("%#g", *(double*)BinaryData(obj));

					} else if (EQ(objClass, SYMA(instructions)) && NOTNIL(GetFrameSlot(gVarFrame, SYMA(printInstructions)))) {
						PrintInstructions(obj);

					} else if (IsSymbol(objClass)) {
						if (IsSubclass(objClass, SYMA(string))) {
							REPprintf("\"");
							SafelyPrintString((UniChar *)BinaryData(obj));
							REPprintf("\"");
						} else {
							REPprintf("MakeBinaryFromHex(\"");
							const unsigned char * p = (const unsigned char *)BinaryData(obj);
							for (ArrayIndex i = 0, count = Length(obj); i < count; ++i, ++p) {
								REPprintf("%02X", *p);
							}
							REPprintf("\", '%s)", SymbolName(objClass));
						}

					} else {
						REPprintf("<binary, class ");
						PrintObjectAux(objClass, indent, depth + 1);
						REPprintf(", length %d>", Length(obj));
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
	return ISREALPTR(obj) && FLAGTEST(ObjectFlags(obj), kObjSlotted);
//		&& !IsSymbol(obj);	// how can it be a symbol if it’s slotted? But that’s what the original does.
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
	char	buf[256];
	int	bufLen, len = Ustrlen(str);

	while (len > 0)
	{
		bufLen = MIN(len, 255);
		ConvertFromUnicode(str, buf, bufLen);
		str += bufLen;
		len -= bufLen;
		REPprintf("%s", buf);
	}
}


/*------------------------------------------------------------------------------
	Print the instructions (bytecodes) from a function frame.
	Args:		obj		a binary object containing the instructions
	Return:  --
------------------------------------------------------------------------------*/

static void
PrintInstructions(Ref obj)
{
	int		count = Length(obj);
	CDataPtr lockedObj(obj);
	unsigned char *	instrPtr = (unsigned char *)(Ptr) lockedObj;
	int		instrLen;

	REPprintf("<instrs: ");
	while (count > 0)
	{
		unsigned int	opCode = *instrPtr >> 3;
		unsigned int	b = *instrPtr & 0x07;
		if (opCode == 0)
		{
			REPprintf("%s", simpleInstrs[b]);
		}
		else
		{
			REPprintf("%s", paramInstrs[opCode]);
			if (opCode >= 3)
				REPprintf(" %ld", (b == 0x07) ? ((*(instrPtr + 1) << 8) + *(instrPtr + 2)) : b);
		}
		instrLen = ((b == 0x07) ? 3 : 1);
		instrPtr += instrLen;
		count -= instrLen;
		if (count > 0)
			REPprintf(", ");
	}
	REPprintf(">");
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
		*outTextSize = symLen * sizeof(UniChar);
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
			REPprintf("(%s)", BinaryData(ASCIIString(name)));
		else
		{
			int count = REPprintf("(#%p) ", obj);
			PrintObject(obj, indent + count);
		}
	}
}


#pragma mark -
#pragma mark Newton Research additions
#include <string.h>
#include "ViewFlags.h"
#include "Responder.h"

/*------------------------------------------------------------------------------
	Print viewClass value.
	Args:		inViewClass
	Return:  --
------------------------------------------------------------------------------*/

void
PrintViewClass(int inViewClass)
{
	const char * s;
	switch (inViewClass) {
	case clObject: s = "clObject"; break;
	case clResponder: s = "clResponder"; break;
	case clModel: s = "clModel"; break;
	case clApplication: s = "clApplication"; break;
	case clNotebook: s = "clNotebook"; break;
	case clMacNotebook: s = "clMacNotebook"; break;
	case clARMNotebook: s = "clARMNotebook"; break;
	case clWindowsNotebook: s = "clWindowsNotebook"; break;
	case clView: s = "clView"; break;
	case clRootView: s = "clRootView"; break;
	case clPictureView: s = "clPictureView"; break;
	case clEditView: s = "clEditView"; break;
	case clContainerView: s = "clContainerView"; break;
	case clKeyboardView: s = "clKeyboardView"; break;
	case clMonthView: s = "clMonthView"; break;
	case clParagraphView: s = "clParagraphView"; break;
	case clPolygonView: s = "clPolygonView"; break;
	case clDataView: s = "clDataView"; break;
	case clMathExpView: s = "clMathExpView"; break;
	case clMathOpView: s = "clMathOpView"; break;
	case clMathLineView: s = "clMathLineView"; break;
	case clScheduleView: s = "clScheduleView"; break;
	case clRemoteView: s = "clRemoteView"; break;
	case clTrainView: s = "clTrainView"; break;
	case clDayView: s = "clDayView"; break;
	case clPickView: s = "clPickView"; break;
	case clGaugeView: s = "clGaugeView"; break;
	case clRepeatingView: s = "clRepeatingView"; break;
	case clPrintView: s = "clPrintView"; break;
	case clMeetingView: s = "clMeetingView"; break;
	case clSliderView: s = "clSliderView"; break;
	case clCharBoxView: s = "clCharBoxView"; break;
	case clTextView: s = "clTextView"; break;
	case clListView: s = "clListView"; break;
	case clClipboard: s = "clClipboard"; break;
	case clLibrarian: s = "clLibrarian"; break;
	case clOutline: s = "clOutline"; break;
	case clHelpOutline: s = "clHelpOutline"; break;
	case clTXView: s = "clTXView"; break;
	default: { REPprintf("cl%d", inViewClass); return; }
	}
	REPprintf(s);
}


/*------------------------------------------------------------------------------
	Print viewFont value.
	Args:		inViewFont
	Return:  --
------------------------------------------------------------------------------*/

void
PrintViewFont(int inViewFont)
{
	char str[16];
	const char * s;
	// font family
	int vf = inViewFont & tsFamilyMask;
	switch (vf) {
	case tsSystem: s = "tsSystem"; break;
	case tsFancy: s = "tsFancy"; break;
	case tsSimple: s = "tsSimple"; break;
	case tsHandwriting: s = "tsHandwriting"; break;
	default: sprintf(str, "tsFamily(%d)", vf); s = str; break;
	}
	REPprintf(s);
	// font size
	vf = inViewFont & tsSizeMask;
	REPprintf("+tsSize(%d)", vf >> tsSizeShift);
	// font face
	vf = inViewFont & tsFaceMask;
	if (vf == 0) {
		REPprintf("+tsPlain");
	} else {
		if (FLAGTEST(vf, tsBold)) { REPprintf("+tsBold"); }
		if (FLAGTEST(vf, tsItalic)) { REPprintf("+tsItalic"); }
		if (FLAGTEST(vf, tsUnderline)) { REPprintf("+tsUnderline"); }
		if (FLAGTEST(vf, tsOutline)) { REPprintf("+tsOutline"); }
		if (FLAGTEST(vf, tsSuperScript)) { REPprintf("+tsSuperScript"); }
		if (FLAGTEST(vf, tsSubScript)) { REPprintf("+tsSubScript"); }
		if (FLAGTEST(vf, tsUndefinedFace)) { REPprintf("+tsUndefinedFace"); }
	}
}


/*------------------------------------------------------------------------------
	Print viewFormat value.
	Args:		inViewFormat
	Return:  --
------------------------------------------------------------------------------*/

void
PrintViewFormat(int inViewFormat)
{
	if (inViewFormat == vfNone) {
		REPprintf("vfNone");
		return;
	}
	char str[256];
	char * s = str;
	int n;
	// view fill
	int vf = inViewFormat & vfFillMask;
	if (vf != 0) {
		switch (vf) {
		case vfFillWhite: strcpy(s, "+vfFillWhite"); s += 12;  break;
		case vfFillLtGray: strcpy(s, "+vfFillLtGray"); s += 13;  break;
		case vfFillGray: strcpy(s, "+vfFillGray"); s += 11;  break;
		case vfFillDkGray: strcpy(s, "+vfFillDkGray"); s += 13;  break;
		case vfFillBlack: strcpy(s, "+vfFillBlack"); s += 12;  break;
		case vfFillCustom: strcpy(s, "+vfFillCustom"); s += 13;  break;
		case vfMatte: strcpy(s, "+vfMatte"); s += 8;  break;
		default: n = sprintf(s, "+/*fill*/0x%08X", vf); s += n;  break;
		}
	}
	// view frame
	vf = inViewFormat & vfFrameMask;
	if (vf != 0) {
		switch (vf) {
		case vfFrameWhite: strcpy(s, "+vfFrameWhite"); s += 13;  break;
		case vfFrameLtGray: strcpy(s, "+vfFrameLtGray"); s += 14;  break;
		case vfFrameGray: strcpy(s, "+vfFrameGray"); s += 12;  break;
		case vfFrameDkGray: strcpy(s, "+vfFrameDkGray"); s += 14;  break;
		case vfFrameBlack: strcpy(s, "+vfFrameBlack"); s += 13;  break;
		case vfFrameCustom: strcpy(s, "+vfFrameCustom"); s += 14;  break;
		case vfFrameMatte: strcpy(s, "+vfFrameMatte"); s += 13;  break;
		default: n = sprintf(s, "+/*frame*/0x%08X", vf); s += n;  break;
		}
	}
	// view lines
	vf = inViewFormat & vfLinesMask;
	if (vf != 0) {
		switch (vf) {
		case vfLinesWhite: strcpy(s, "+vfLinesWhite"); s += 13;  break;
		case vfLinesLtGray: strcpy(s, "+vfLinesLtGray"); s += 14;  break;
		case vfLinesGray: strcpy(s, "+vfLinesGray"); s += 12;  break;
		case vfLinesDkGray: strcpy(s, "+vfLinesDkGray"); s += 14;  break;
		case vfLinesBlack: strcpy(s, "+vfLinesBlack"); s += 13;  break;
		default: n = sprintf(s, "+/*lines*/0x%08X", vf); s += n;  break;
		}
	}
	// view pen
	vf = (inViewFormat & vfPenMask) >> vfPenShift;
	if (vf != 0) {
		n = sprintf(s, "+vfPen(%d)", vf); s += n;
	}
	// view inset
	vf = (inViewFormat & vfInsetMask) >> vfInsetShift;
	if (vf != 0) {
		n = sprintf(s, "+vfInset(%d)", vf); s += n;
	}
	// view shadow
	vf = (inViewFormat & vfShadowMask) >> vfShadowShift;
	if (vf != 0) {
		n = sprintf(s, "+vfShadow(%d)", vf); s += n;
	}
	// view hiliting
	vf = inViewFormat & vfHiliteMask;
	if (vf != 0) {
		switch (vf) {
		case vfHiliteInvert: strcpy(s, "+vfHiliteInvert"); s += 15;  break;
		case vfHiliteBullet: strcpy(s, "+vfHiliteBullet"); s += 15;  break;
		case vfHiliteTriangle: strcpy(s, "+vfHiliteTriangle"); s += 17;  break;
		}
	}
	// view rounding
	vf = (inViewFormat & vfRoundMask) >> vfRoundShift;
	if (vf != 0) {
		n = sprintf(s, "+vfRound(%d)", vf); s += n;
	}
	REPprintf(str+1);
}


/*------------------------------------------------------------------------------
	Print viewFlags value.
	Args:		inViewFlags
	Return:  --
------------------------------------------------------------------------------*/

void
PrintViewFlags(int inViewFlags)
{
	if (inViewFlags == vNoFlags) {
		REPprintf("vNoFlags");
		return;
	}
	static char str[256];
	char * s = str;
	if (FLAGTEST(inViewFlags, vVisible)) { strcpy(s, "+vVisible"); s += 9; }
	if (FLAGTEST(inViewFlags, vReadOnly)) { strcpy(s, "+vReadOnly"); s += 10; }
	if (FLAGTEST(inViewFlags, vApplication)) { strcpy(s, "+vApplication"); s += 13; }
	if (FLAGTEST(inViewFlags, vCalculateBounds)) { strcpy(s, "+vCalculateBounds"); s += 17; }
	if (FLAGTEST(inViewFlags, vNoKeys)) { strcpy(s, "+vNoKeys"); s += 8; }
	if (FLAGTEST(inViewFlags, vClipping)) { strcpy(s, "+vClipping"); s += 10; }
	if (FLAGTEST(inViewFlags, vFloating)) { strcpy(s, "+vFloating"); s += 10; }
	if (FLAGTEST(inViewFlags, vWriteProtected)) { strcpy(s, "+vWriteProtected"); s += 16; }
	if (FLAGTEST(inViewFlags, vRecognitionAllowed)) {
		if (FLAGTEST(inViewFlags, vSingleUnit)) { strcpy(s, "+vSingleUnit"); s += 12; }
		if (FLAGTEST(inViewFlags, vClickable)) { strcpy(s, "+vClickable"); s += 11; }
		if (FLAGTEST(inViewFlags, vStrokesAllowed)) { strcpy(s, "+vStrokesAllowed"); s += 16; }
		if (FLAGTEST(inViewFlags, vGesturesAllowed)) { strcpy(s, "+vGesturesAllowed"); s += 17; }
		if (FLAGTEST(inViewFlags, vCharsAllowed)) { strcpy(s, "+vCharsAllowed"); s += 14; }
		if (FLAGTEST(inViewFlags, vNumbersAllowed)) { strcpy(s, "+vNumbersAllowed"); s += 16; }
		if (FLAGTEST(inViewFlags, vLettersAllowed)) { strcpy(s, "+vLettersAllowed"); s += 16; }
		if (FLAGTEST(inViewFlags, vPunctuationAllowed)) { strcpy(s, "+vPunctuationAllowed"); s += 20; }
		if (FLAGTEST(inViewFlags, vShapesAllowed)) { strcpy(s, "+vShapesAllowed"); s += 15; }
		if (FLAGTEST(inViewFlags, vMathAllowed)) { strcpy(s, "+vMathAllowed"); s += 13; }
		if (FLAGTEST(inViewFlags, vPhoneField)) { strcpy(s, "+vPhoneField"); s += 12; }
		if (FLAGTEST(inViewFlags, vDateField)) { strcpy(s, "+vDateField"); s += 11; }
		if (FLAGTEST(inViewFlags, vTimeField)) { strcpy(s, "+vTimeField"); s += 11; }
		if (FLAGTEST(inViewFlags, vAddressField)) { strcpy(s, "+vAddressField"); s += 14; }
		if (FLAGTEST(inViewFlags, vNameField)) { strcpy(s, "+vNameField"); s += 11; }
		if (FLAGTEST(inViewFlags, vCapsRequired)) { strcpy(s, "+vCapsRequired"); s += 14; }
		if (FLAGTEST(inViewFlags, vCustomDictionaries)) { strcpy(s, "+vCustomDictionaries"); s += 20; }
	}
	if (FLAGTEST(inViewFlags, vSelected)) { strcpy(s, "+vSelected"); s += 10; }
	if (FLAGTEST(inViewFlags, vClipboard)) { strcpy(s, "+vClipboard"); s += 11; }
	if (FLAGTEST(inViewFlags, vNoScripts)) { strcpy(s, "+vNoScripts"); s += 11; }
	REPprintf(str+1);
}


/*------------------------------------------------------------------------------
	Print viewJustify value.
	Args:		inViewJustify
	Return:  --
------------------------------------------------------------------------------*/

void
PrintViewJustify(int inViewJustify)
{
	switch (inViewJustify & vjHMask) {
	case vjLeftH: REPprintf("vjLeftH"); break;
	case vjRightH: REPprintf("vjRightH"); break;
	case vjCenterH: REPprintf("vjCenterH"); break;
	case vjFullH: REPprintf("vjFullH"); break;
	}
	switch (inViewJustify & vjVMask) {
	case vjTopV: REPprintf("+vjTopV"); break;
	case vjCenterV: REPprintf("+vjCenterV"); break;
	case vjBottomV: REPprintf("+vjBottomV"); break;
	case vjFullV: REPprintf("+vjFullV"); break;
	}
	if ((inViewJustify & vjParentMask) != 0) {
		switch (inViewJustify & vjParentHMask) {
		case vjParentLeftH: REPprintf("+vjParentLeftH"); break;
		case vjParentCenterH: REPprintf("+vjParentCenterH"); break;
		case vjParentRightH: REPprintf("+vjParentRightH"); break;
		case vjParentFullH: REPprintf("+vjParentFullH"); break;
		}
		switch (inViewJustify & vjParentVMask) {
		case vjParentTopV: REPprintf("+vjParentTopV"); break;
		case vjParentCenterV: REPprintf("+vjParentCenterV"); break;
		case vjParentBottomV: REPprintf("+vjParentBottomV"); break;
		case vjParentFullV: REPprintf("+vjParentFullV"); break;
		}
	}
	if ((inViewJustify & vjSiblingMask) != 0) {
		switch (inViewJustify & vjSiblingHMask) {
		case vjSiblingNoH: REPprintf("+vjSiblingNoH"); break;
		case vjSiblingCenterH: REPprintf("+vjSiblingCenterH"); break;
		case vjSiblingRightH: REPprintf("+vjSiblingRightH"); break;
		case vjSiblingFullH: REPprintf("+vjSiblingFullH"); break;
		case vjSiblingLeftH: REPprintf("+vjSiblingLeftH"); break;
		}
		switch (inViewJustify & vjSiblingVMask) {
		case vjSiblingNoV: REPprintf("+vjSiblingNoV"); break;
		case vjSiblingCenterV: REPprintf("+vjSiblingCenterV"); break;
		case vjSiblingBottomV: REPprintf("+vjSiblingBottomV"); break;
		case vjSiblingFullV: REPprintf("+vjSiblingFullV"); break;
		case vjSiblingTopV: REPprintf("+vjSiblingTopV"); break;
		}
	}
	if (FLAGTEST(inViewJustify, vjParentClip)) { REPprintf("+vjParentClip"); }
	if (FLAGTEST(inViewJustify, vjChildrenLasso)) { REPprintf("+vjChildrenLasso"); }
	if (FLAGTEST(inViewJustify, vjReflow)) { REPprintf("+vjReflow"); }

	switch (inViewJustify & vjLineLimitMask) {
	case vjOneLineOnly: REPprintf("+vjOneLineOnly"); break;
	case vjOneWordOnly: REPprintf("+vjOneWordOnly"); break;
	case vjOneCharOnly: REPprintf("+vjOneCharOnly"); break;
	}
	if (FLAGTEST(inViewJustify, vjLeftRatio)) { REPprintf("+vjLeftRatio"); }
	if (FLAGTEST(inViewJustify, vjRightRatio)) { REPprintf("+vjRightRatio"); }
	if (FLAGTEST(inViewJustify, vjTopRatio)) { REPprintf("+vjTopRatio"); }
	if (FLAGTEST(inViewJustify, vjBottomRatio)) { REPprintf("+vjBottomRatio"); }
}


/*------------------------------------------------------------------------------
	Print name of magic pointer into string.
	Args:		inObj			magic pointer
	Return:  string
------------------------------------------------------------------------------*/
extern "C" const char * GetMagicPointerString(int inMP);

void
PrintMagicPtr(RefArg inObj)
{
	REPprintf(GetMagicPointerString(RVALUE(inObj)));
}


#pragma mark -
/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FFramesStringer(RefArg rcvr, RefArg inArray)
{
	return Stringer(inArray);
}


Ref
FEvalStringer(RefArg inRcvr, RefArg inFrame, RefArg inArray)
{
	RefVar	slot;
	RefVar	strArray = Clone(inArray);
	for (ArrayIndex i = 0, count = Length(inArray); i < count; ++i)
	{
		slot = GetArraySlot(inArray, i);
		if (EQ(ClassOf(slot), SYMA(symbol)))
			SetArraySlot(strArray, i, GetVariable(inFrame, slot));
	}
	return Stringer(strArray);
}


/*------------------------------------------------------------------------------
	Print a Ref object.
	Follow it with a newline.
	Args:		inRcvr		the receiver
				inObj			the object to print
	Return:	nil
------------------------------------------------------------------------------*/

Ref
FPrint(RefArg inRcvr, RefArg inObj)
{
	PrintObject(inObj, 0);
	REPprintf("\n");
	return NILREF;
}


/*------------------------------------------------------------------------------
	Print a Ref object.
	DON’T follow it with a newline.
	Args:		inRcvr		the receiver
				inObj			the object to print
	Return:	nil
------------------------------------------------------------------------------*/

Ref
FWrite(RefArg inRcvr, RefArg inObj)
{
	if (ISCHAR(inObj))
	{
		UniChar  str[2];
		str[0] = RCHAR(inObj);
		str[1] = kEndOfString;
		SafelyPrintString(str);
	}
	else if (IsString(inObj))
		SafelyPrintString(GetUString(inObj));
	else
		PrintObject(inObj, 0);
	return NILREF;
}

