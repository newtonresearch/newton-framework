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

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

extern "C" {
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

extern "C" {
		ArrayIndex	GetFunctionArgCount(Ref fn);
}

// Forward declarations
		 void		PrintObjectAux(Ref obj, int indent, int depth);
static bool		IsAggregate(Ref obj);
static void		PrintInstructions(Ref obj);
		 Ref		Stringer(RefArg obj);
static bool		StringerStringObject(RefArg obj, Ptr outText, int * outTextSize, Ptr outInk, int * outInkSize);

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
		switch (RTAG(obj))
		{
			case kTagInteger:
			// can only be an integer
				REPprintf("%d", RINT(obj));
				break;

			case kTagImmed:
			// try boolean
				if (EQRef(obj, TRUEREF))
					REPprintf("true");
				else if (ISNIL(obj))
					REPprintf("nil");

			// try character
				else if (ISCHAR(obj))
				{
					UniChar	ch = RCHAR(obj);
					if (ch > 0xFF)
						REPprintf("$\\u%04X", ch);
					else if (ch >= 0x20 && ch <= 0x7F)
					{
						if (ch == 0x5C)
							REPprintf("$\\\\");
						else if (ch == 0x09)
							REPprintf("$\\t");
						else if (ch == 0x0D)
							REPprintf("$\\n");
						else
							REPprintf("$%c", ch);
					}
					else
						REPprintf("$\\%02X", ch);
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
					REPprintf("#%08X", obj);
				break;

			case kTagPointer:
			case kTagMagicPtr:
			// try precedents array first
				for (i = 0; i < depth && i < kPrecedentStackSize-1; ++i)
					if (EQRef(obj, GetArraySlotRef(gPrecedentStack, i)))
						break;
				if (i < depth && i < kPrecedentStackSize-1)
					REPprintf("<%d>", depth - i);

				else
				{
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
					if (flags & kObjSlotted)
					{
						if (flags & kObjFrame)
						{
							if (IsFunction(obj) && doPrettyPrint)
							{
								int argCount = GetFunctionArgCount(obj);
								const char * argPlural = (argCount == 1) ? "" : "s";
								int saveTrace = gInterpreter->tracing;		// | not original
								gInterpreter->tracing = kTraceNothing;		// | but required to prevent infinite loop in IsNativeFunction > ClassOf > GetProtoVariable > gInterpreter->TraceGet
								REPprintf(IsNativeFunction(obj) ? "<native" : "<");
								gInterpreter->tracing = saveTrace;			// |
								REPprintf("function, %ld arg%s, #%08X>", argCount, argPlural, obj);
							}
							else if (depth > printDepth)
							{
								if (ISMAGICPTR(obj))
									REPprintf("{@%ld}", RVALUE(obj));
								else
									REPprintf("{#%08X}", obj);
							}
							else
							{
							// print slots in frame
								numOfSlots = Length(obj);
								indent += REPprintf("{");
								if (doPrettyPrint && depth <= printDepth)
								{
									Ref	tag;
									bool	isAggr = NO;
									// determine whether any slot in frame is aggregate
									// and therefore this frame needs multi-line output
									CObjectIterator	iter(obj);
									for (i = 0; !iter.done() && !isAggr; iter.next(), ++i)
									{
										if (printLength >= 0 && i >= printLength)
											// don’t exceed number-of-slots-to-print preference
											break;
										isAggr = IsAggregate(iter.value());
									}
									// increment depth for slots in this frame
									depth++;
									for (iter.reset(), i = 0; !iter.done(); iter.next(), ++i)
									{
										if (printLength >= 0 && i >= printLength)
										{
											// don’t exceed number-of-slots-to-print preference
											REPprintf("...");
											break;
										}
										tag = iter.tag();
										if ((EQRef(tag, RSYMviewCObject) && ISINT(iter.value()))
										 || (EQRef(tag, RSYMviewClipper) && ISINT(iter.value()))
										 || EQRef(tag, RSYMfuncPtr))
											REPprintf("%s: #%08X", SymbolName(tag), RINT(iter.value()));
										else
										{
											count = REPprintf("%s: ", SymbolName(tag));
											PrintObjectAux(iter.value(), indent + count, depth);
										}
										if (--numOfSlots > 0)
										{
										// separate slots
											REPprintf(", ");
											if (isAggr)
												REPprintf("\n%*s", indent, " ");
										}
									}
								}
								REPprintf("}");
							}
						}

						// insanity check
						else if (flags & kObjForward)
							REPprintf("<forwarding pointer #%08X...WTF!!!>", obj);

						else	// must be an array
						{
							objClass = ClassOf(obj);
							if (EQRef(objClass, RSYMpathExpr))
							{
							// it’s a path expression
								bool	hasSymbol = NO;
								numOfSlots = Length(obj);
								for (i = 0; i < numOfSlots && !hasSymbol; ++i)
									hasSymbol = IsSymbol(GetArraySlotRef(obj, i));
								if (hasSymbol)
								{
								// it contains symbols so print in dotted form
									for (i = 0; i < numOfSlots; ++i)
									{
										PrintObjectAux(GetArraySlotRef(obj, i), indent, depth + 1);
										if (i < numOfSlots - 1)
											REPprintf(".");
									}
								}
								else
								{
								// path expression contains non-symbols so enumerate the elements
									REPprintf("[pathExpr: ");
									for (i = 0; i < numOfSlots; ++i)
									{
										PrintObjectAux(GetArraySlotRef(obj, i), indent, depth + 1);
										if (i < numOfSlots - 1)
											REPprintf(", ");
									}
									REPprintf("]");
								}
							}
							else if (depth > printDepth)
							{
								if (ISMAGICPTR(obj))
									REPprintf("[@%ld]", RVALUE(obj));
								else
									REPprintf("[#%08X]", obj);
							}
							else
							{
							// print elements in array
								numOfSlots = Length(obj);
								indent += REPprintf("[");
								if (doPrettyPrint && depth <= printDepth)
								{
									bool	isAggr = NO;
									CObjectIterator	iter(obj);
									for (i = 0; !iter.done() && !isAggr; iter.next(), ++i)
									{
										if (printLength >= 0 && i >= printLength)
											break;
										isAggr = IsAggregate(iter.value());
									}
									if (!EQRef(objClass, RSYMarray))
									{
										if (IsSymbol(objClass))
										{
											count = REPprintf("%s: ", SymbolName(objClass));
											if (isAggr)
												REPprintf("\n%*s", indent, " ");
											else
												indent += count;
										}
										else
										{
											PrintObjectAux(objClass, indent, depth + 1);
											if (isAggr)
												REPprintf(": \n%*s", indent, " ");
											else
												REPprintf(": ");
										}
									}
									
									for (iter.reset(), i = 0; !iter.done(); iter.next(), ++i)
									{
										if (printLength >= 0 && i >= printLength)
										{
											// exceeded number-of-slots-to-print preference
											REPprintf("...");
											break;
										}
										PrintObjectAux(iter.value(), indent, depth + 1);
										if (--numOfSlots > 0)
										{
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
					objClass = ClassOf(obj);

					if (IsSymbol(obj))
					{
						unsigned char		ch;
						unsigned char *	s = (unsigned char *)SymbolName(obj);
						bool		isWeird = YES;		// assume symbol contains character other than alpha|digit|underscore

						if ((ch = *s) != kEndOfString
						 && ((sType[ch] & (isUppercase|isLowercase)) != 0 || ch == '_'))
						{
							isWeird = NO;
							for (unsigned char * p = s; (ch = *p) != kEndOfString; p++)
							{
								if (((sType[ch] & (isUppercase|isLowercase|isDigit)) != 0) || ch == '_')
									/* it’s good */;
								else
								{
									isWeird = YES;
									break;
								}
							}
						}
						if (isWeird)
						{
							// symbol contains special characters so enclose it in |brackets|
							REPprintf("'|");
							isWeird = NO;	// re-purpose the flag to mean isCtrlOr8bitChar
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
										isWeird = NO;
									}
									else
										REPprintf("%02X", ch);
								}
								else if (ch >= 0x20 && ch <= 0x7F)
								{
									if (ch == 0x5C)
										REPprintf("\\\\");
									else if (ch == 0x7C)
										REPprintf("\\|");
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
									isWeird = YES;
								}
							}
							REPprintf("|");
						}
						else
							// it’s a simple symbol
							REPprintf("'%s", s);
					}

					else if (EQRef(objClass, RSYMreal))
						REPprintf("%#g", *(double*)BinaryData(obj));

					else if (EQRef(objClass, RSYMinstructions) && NOTNIL(GetFrameSlot(gVarFrame, SYMA(printInstructions))))
						PrintInstructions(obj);

					else if (IsSymbol(objClass))
					{
						if (IsSubclass(objClass, RSYMstring))
						{
							REPprintf("\"");
							SafelyPrintString((UniChar *)BinaryData(obj));
							REPprintf("\"");
						}
						else
							REPprintf("<%s, length %d>", SymbolName(objClass), Length(obj));
					}

					else
					{
						REPprintf("<binary, class ");
						PrintObjectAux(objClass, indent, depth + 1);
						REPprintf(", length %d>", Length(obj));
					}
				}

				if (depth < kPrecedentStackSize)
					SetArraySlotRef(gPrecedentStack, depth, NILREF);
				break;
			}
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
	Return:  YES|NO
------------------------------------------------------------------------------*/

static bool
IsAggregate(Ref obj)
{
	return ISREALPTR(obj) && (ObjectFlags(obj) & kObjSlotted);
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
		return YES;
	}

	else if (ISNIL(obj))
	{
		*outTextSize = *outInkSize = 0;
		return YES;
	}

	else if (ISCHAR(obj))
	{
		*outTextSize = sizeof(UniChar);
		*outInkSize = 0;
		if (outText != NULL)
			*(UniChar *)outText = RCHAR(obj);
		return YES;		
	}

	else if (ISINT(obj))
	{
		char  str[32];
		int  strLen = sprintf(str, "%ld", RINT(obj));
		*outTextSize = strLen * sizeof(UniChar);
		*outInkSize = 0;
		if (outText != NULL)
			ConvertToUnicode(str, (UniChar *)outText, strLen);
		return YES;		
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
		return YES;		
	}

	else if (IsSymbol(obj))
	{
		Ptr	sym = SymbolName(obj);
		long  symLen = strlen(sym);
		*outTextSize = symLen;
		*outInkSize = 0;
		if (outText != NULL)
			ConvertToUnicode(sym, (UniChar *)outText, symLen);
		return YES;
	}

	*outTextSize = *outInkSize = 0;
	return NO;
}


/*------------------------------------------------------------------------------
	More object printing stuff.
------------------------------------------------------------------------------*/

Ref
FindSlotName(RefArg context, RefArg obj)
{
	Ref	slotName = NILREF;
	CObjectIterator	iter(context, YES);
	for ( ; !iter.done(); iter.next())
	{
		if (EQRef(obj, iter.value()))
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
			int count = REPprintf("(#%08X) ", obj);
			PrintObject(obj, indent + count);
		}
	}
}


#pragma mark -
/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FEvalStringer(RefArg inRcvr, RefArg inFrame, RefArg inArray)
{
	RefVar	slot;
	RefVar	strArray = Clone(inArray);
	for (ArrayIndex i = 0, count = Length(inArray); i < count; ++i)
	{
		slot = GetArraySlot(inArray, i);
		if (EQRef(ClassOf(slot), RSYMsymbol))
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

