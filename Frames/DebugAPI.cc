/*
	File:		DebugAPI.cc

	Contains:	Interpreter debug interface functions.

	Written by:	Newton Research Group, 2013.
*/
#define forDebug 1

#include "DebugAPI.h"
#include "Funcs.h"
#include "Lookup.h"
#include "NewtonErrors.h"
#include "Opcodes.h"
#include "ROMResources.h"


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */
/* Newton Research additions */
char	gFnIndentStr[kFnIndentStrLen+1] = "                                                                                ";
int	gFnDepth = 0;

bool	gAccurateStackTrace = false;			//		 0C105468


/* -----------------------------------------------------------------------------
	N e w t o n   R e s e a r c h   a d d i t i o n s
----------------------------------------------------------------------------- */

void
DumpHex(void * inBuf, size_t inLen)
{
	char str[32*3 + 1];
	for (int y = 0; y < inLen; y += 32)
	{
		int x;
		for (x = 0; x < 32 && y + x < inLen; x++)
			sprintf(str+x*3, "%02X ", ((unsigned char *)inBuf)[y+x]);
		if (x > 0)
		{
			str[x*3] = 0;
			PRINTF(("%s\n", str));
		}
	}
}


/* -----------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
----------------------------------------------------------------------------- */
extern "C" Ref FSetDebugMode(RefArg rcvr, RefArg inDebugOn);

Ref
FSetDebugMode(RefArg rcvr, RefArg inDebugOn)
{
	bool ast = gAccurateStackTrace;
	gAccurateStackTrace = NOTNIL(inDebugOn);
	return MAKEBOOLEAN(ast);
}


CNSDebugAPI *
NewNSDebugAPI(CInterpreter * interpreter)
{
	return new CNSDebugAPI(interpreter);
}


void
DeleteNSDebugAPI(CNSDebugAPI * inDebugAPI)
{
	if (inDebugAPI)
		delete inDebugAPI;
}

#pragma mark -
/* -----------------------------------------------------------------------------
	C N S D e b u g A P I
----------------------------------------------------------------------------- */

CNSDebugAPI::CNSDebugAPI(CInterpreter * interpreter)
	: fInterpreter(interpreter)
{ }


CNSDebugAPI::~CNSDebugAPI()
{ }


ArrayIndex
FunctionStackSize(RefArg inFunc)
{
	RefVar fnClass = ClassOf(inFunc);

	if (EQ(fnClass, SYMA(_function)))
	{
		ArrayIndex argCount = RINT(GetArraySlot(inFunc, kFunctionNumArgsIndex));
		return (argCount >> 16) /* number of locals */ + (argCount & 0xFFFF) /* number of args */;
	}

	else if (IsNativeFunction(inFunc))
		return GetFunctionArgCount(inFunc);

	else if (EQ(fnClass, SYMA(CodeBlock)))		// Newton 1.x does not stack args
		return 0;

	return kIndexNotFound;
}


ArrayIndex
CNSDebugAPI::numStackFrames(void)
{ return STACKINDEX(fInterpreter->ctrlStack) / kNumOfItemsInStackFrame; }


ArrayIndex
CNSDebugAPI::stackStart(ArrayIndex index)
{
	if (index == numStackFrames())
		return (fInterpreter->dataStack.top - fInterpreter->dataStack.base) - 1;
	return 3 + (RVALUE(stackFrameAt(index)->stackFrame) >> 6);
}


VMState *
CNSDebugAPI::stackFrameAt(ArrayIndex index)
{
	if (index >= numStackFrames())
		ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(index));
	return fInterpreter->ctrlStack.at(index+1);
}


bool
CNSDebugAPI::accurateStack(void)
{ return gAccurateStackTrace != 0; }


ArrayIndex
CNSDebugAPI::PC(ArrayIndex index)
{ Ref pcRef = stackFrameAt(index)->pc; return ISINT(pcRef) ? RVALUE(pcRef) : kIndexNotFound; }


void
CNSDebugAPI::setPC(ArrayIndex index, ArrayIndex inPC)
{ stackFrameAt(index)->pc = MAKEINT(inPC); }


Ref
CNSDebugAPI::function(ArrayIndex index)
{ return stackFrameAt(index)->func; }


void
CNSDebugAPI::setFunction(ArrayIndex index, RefArg inFunc)
{ stackFrameAt(index)->func = inFunc; }


Ref
CNSDebugAPI::receiver(ArrayIndex index)
{ return stackFrameAt(index)->rcvr; }


void
CNSDebugAPI::setReceiver(ArrayIndex index, RefArg inRcvr)
{ stackFrameAt(index)->rcvr = inRcvr; }


Ref
CNSDebugAPI::implementor(ArrayIndex index)
{ return stackFrameAt(index)->impl; }


void
CNSDebugAPI::setImplementor(ArrayIndex index, RefArg inImpl)
{ stackFrameAt(index)->impl = inImpl; }


Ref
CNSDebugAPI::locals(ArrayIndex index)
{
	VMState * state = stackFrameAt(index);
	Ref funcObj = state->func;
	Ref funcClass = ClassOf(funcObj);

	if (EQ(funcClass, SYMA(_function)))
	{
		ArrayIndex numArgs = RINT(GetArraySlotRef(funcObj, kFunctionNumArgsIndex));
		ArrayIndex numLocals = (numArgs >> 16) + (numArgs & 0xFFFF);
		RefVar localsArray(MakeArray(numLocals));
		for (ArrayIndex i = 0; i < numLocals; ++i)
			SetArraySlot(localsArray, i, *(fInterpreter->dataStack.base + 3 + (RVALUE(state->stackFrame) >> 6) + i));
		return localsArray;
	}

	else if (IsNativeFunction(funcObj))
	{
		ArrayIndex numLocals = GetFunctionArgCount(funcObj);
		RefVar localsArray(MakeArray(numLocals));
		for (ArrayIndex i = 0; i < numLocals; ++i)
			SetArraySlot(localsArray, i, *(fInterpreter->dataStack.base + (RVALUE(state->stackFrame) >> 6) + i));
		return localsArray;
	}

	else if (EQ(funcClass, SYMA(CodeBlock)))
	{
		ArrayIndex numLocals = Length(state->locals) - kArgFrameArgIndex;
		RefVar localsArray(MakeArray(numLocals));
		for (ArrayIndex i = 0; i < numLocals; ++i)
			SetArraySlot(localsArray, i, GetArraySlot(state->locals, kArgFrameArgIndex + i));
		return localsArray;
	}

	return NILREF;
}


Ref
CNSDebugAPI::getVar(ArrayIndex index, ArrayIndex inVarIndex)
{
	VMState * state = stackFrameAt(index);
	Ref funcObj = state->func;
	Ref funcClass = ClassOf(funcObj);

	if (EQ(funcClass, SYMA(_function)))
	{
		ArrayIndex numArgs = RINT(GetArraySlotRef(funcObj, kFunctionNumArgsIndex));
		ArrayIndex numVars = (numArgs >> 16) + (numArgs & 0xFFFF);
		if (inVarIndex >= numVars)
			ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inVarIndex));
		return *(fInterpreter->dataStack.base + 3 + (RVALUE(state->stackFrame) >> 6) + inVarIndex);
	}

	else if (IsNativeFunction(funcObj))
	{
		if (inVarIndex >= GetFunctionArgCount(funcObj))
			ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inVarIndex));
		return *(fInterpreter->dataStack.base + (RVALUE(state->stackFrame) >> 6) + inVarIndex);
	}

	else if (EQ(funcClass, SYMA(CodeBlock)))
	{
		if (inVarIndex >= Length(state->locals) - kArgFrameArgIndex)
			ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inVarIndex));
		return GetArraySlot(state->locals, kArgFrameArgIndex + inVarIndex);
	}

	return NILREF;
}


void
CNSDebugAPI::setVar(ArrayIndex index, ArrayIndex inVarIndex, RefArg inVar)
{
	VMState * state = stackFrameAt(index);
	Ref funcObj = state->func;
	Ref funcClass = ClassOf(funcObj);

	if (EQ(funcClass, SYMA(_function)))
	{
		ArrayIndex numArgs = RINT(GetArraySlotRef(funcObj, kFunctionNumArgsIndex));
		ArrayIndex numVars = (numArgs >> 16) + (numArgs & 0xFFFF);
		if (inVarIndex >= numVars)
			ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inVarIndex));
		*(fInterpreter->dataStack.base + 3 + (RVALUE(state->stackFrame) >> 6) + inVarIndex) = inVar;
	}

	else if (IsNativeFunction(funcObj))
	{
		if (inVarIndex >= GetFunctionArgCount(funcObj))
			ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inVarIndex));
		*(fInterpreter->dataStack.base + (RVALUE(state->stackFrame) >> 6) + inVarIndex) = inVar;
	}

	else if (EQ(funcClass, SYMA(CodeBlock)))
	{
		if (inVarIndex >= Length(state->locals) - kArgFrameArgIndex)
			ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inVarIndex));
		SetArraySlot(state->locals, kArgFrameArgIndex + inVarIndex, inVar);
	}
}


Ref
CNSDebugAPI::findVar(ArrayIndex index, RefArg inSym)
{
	bool exists;
	RefVar locals = stackFrameAt(index)->locals;
	if (ISNIL(locals))
		ThrowExInterpreterWithSymbol(kNSErrUndefinedVariable, inSym);

	RefVar var = XGetVariable(locals, inSym, &exists, kLexicalLookup);
	if (!exists)
		ThrowExInterpreterWithSymbol(kNSErrUndefinedVariable, inSym);
	return var;
}


void
CNSDebugAPI::setFindVar(ArrayIndex index, RefArg inSym, RefArg inVar)
{
	RefVar locals = stackFrameAt(index)->locals;
	if (ISNIL(locals))
		ThrowExInterpreterWithSymbol(kNSErrUndefinedVariable, inSym);

	SetVariableOrGlobal(locals, inSym, inVar, kLexicalLookup);
}


ArrayIndex
CNSDebugAPI::numTemps(ArrayIndex index)
{ return stackStart(index+1) - (stackStart(index) + FunctionStackSize(function(index))); }


Ref
CNSDebugAPI::tempValue(ArrayIndex index, ArrayIndex inTempIndex)
{
	ArrayIndex stkIndex = stackStart(index) + FunctionStackSize(function(index)) + inTempIndex;
	if (stkIndex >= stackStart(index+1))
		ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inTempIndex));
	return *(fInterpreter->dataStack.base + stkIndex);
	
}


void
CNSDebugAPI::setTempValue(ArrayIndex index, ArrayIndex inTempIndex, RefArg inTemp)
{
	ArrayIndex stkIndex = stackStart(index) + FunctionStackSize(function(index)) + inTempIndex;
	if (stkIndex >= stackStart(index+1))
		ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(inTempIndex));
	*(fInterpreter->dataStack.base + stkIndex) = inTemp;
}


void
CNSDebugAPI::Return(ArrayIndex index, RefArg)
{ ThrowErr(exFrames, kNSErrReturn); }


#pragma mark -
/*------------------------------------------------------------------------------
	Trace Read-Evaluate-Print stack.
	Args:		interpreter
	Return:	--
------------------------------------------------------------------------------*/
extern void	PrintWellKnownObject(Ref obj, int indent);
extern Ref	FindSlotName(RefArg context, RefArg obj);

void
REPStackTrace(void * interpreter)
{
	CNSDebugAPI debugAPI((CInterpreter *)interpreter);
	ArrayIndex numOfStackFrames = debugAPI.numStackFrames();
	if (numOfStackFrames > 0)
	{
		//sp14 = gVarFrame
		Ref savePrintDepth = GetGlobalVar(SYMA(printDepth));
		//sp-10
		Ref stackTracePrintDepth = GetGlobalVar(MakeSymbol("stackTracePrintDepth"));
		DefGlobalVar(SYMA(printDepth), ISINT(stackTracePrintDepth) ? stackTracePrintDepth : MAKEINT(0));

		if (!debugAPI.accurateStack())
			REPprintf("WARNING:  Inaccurate stack trace.  Use SetDebugMode(true) for accurate stack traces.\n");

		REPprintf("Stack trace:\n");
		//sp-10
		RefVar theFunc;	//sp0C
		RefVar theRcvr;	//sp08
		RefVar theSlot;	//sp04
		RefVar theImpl;	//sp00
		//sp-08
		//sp30 = exRoot
		//sp34 = &sp-64
		for (int stackIndex = numOfStackFrames - 1; stackIndex >= 0; stackIndex--)
		{
			newton_try
			{
				theFunc = debugAPI.function(stackIndex);		//sp80
				theRcvr = debugAPI.receiver(stackIndex);		//sp7C
				theImpl = debugAPI.implementor(stackIndex);	//sp74
				int indent = REPprintf("%4d : ", stackIndex);
				theSlot = NILREF;										//sp78
				if (ISNIL(theFunc))
					REPprintf("[incomplete stack frame]\n");
				else
				{
					theSlot = FindSlotName(theImpl, theFunc);

					if (NOTNIL(theSlot))
					{
						REPprintf("#%lX.", theImpl.h->ref);
						PrintObject(theSlot, 0);
					}
					else
						PrintWellKnownObject(theFunc, indent);

					if (IsNativeFunction(theFunc))
						REPprintf("[native]\n");
					else
						REPprintf(" : %ld\n", debugAPI.PC(stackIndex));

					if (NOTNIL(theRcvr))
					{
						int indent = REPprintf("       Receiver: ");
						PrintWellKnownObject(theRcvr, indent);
						REPprintf("\n");
					}

					if (EQ(ClassOf(theFunc), SYMA(CodeBlock)))
					{
						// it’s a Newton1 CodeBlock
						//sp-08
						RefVar argFrame(GetArraySlot(theFunc, kFunctionArgFrameIndex));
						RefVar argMap(((FrameObject *)ObjectPtr(argFrame))->map);
						ArrayIndex numOfLocals = Length(argFrame) - kArgFrameArgIndex;
						ArrayIndex numOfArgs = RINT(GetArraySlot(theFunc, kFunctionNumArgsIndex));
						for (ArrayIndex i = 0; i < numOfLocals; ++i)
						{
							indent = REPprintf("       %ld %s", i, SymbolName(GetTag(argMap, i+kArgFrameArgIndex)));
							if (i < numOfArgs)
								indent += REPprintf(" [arg %ld]", i);
							indent += REPprintf(": ");
							PrintWellKnownObject(debugAPI.findVar(stackIndex, GetTag(argMap, i+kArgFrameArgIndex)), indent);
							REPprintf("\n");
						}
					}
					else
					{
						// it’s a Newton2 _function
						RefVar locals(debugAPI.locals(stackIndex));
						ArrayIndex numOfLocals = Length(locals);
						ArrayIndex numOfArgs = GetFunctionArgCount(theFunc);
						for (ArrayIndex i = 0; i < numOfLocals; ++i)
						{
							indent = REPprintf("       %ld", i);
							if (i < numOfArgs)
								indent += REPprintf(" [arg %ld]", i);
							indent += REPprintf(": ");
							PrintWellKnownObject(GetArraySlot(locals, i), indent);
							REPprintf("\n");
						}
					}
				}
			}
			newton_catch(exRootException)
			{
				REPprintf("  *** Skipping bad stack frame\n");
			}
			end_try;
		}
		DefGlobalVar(SYMA(printDepth), savePrintDepth);
	}
}


#pragma mark -
/* -----------------------------------------------------------------------------
	D i s a s s e m b l y
----------------------------------------------------------------------------- */
#if 1		// #ifdef hasDisasm		I think it’s fun always to be able to Disassemble, no?

const char * simpleInstrs[] =
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

const char * paramInstrs[] =
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


/* -----------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
	Originally part of the NS Debug Tools.pkg which includes breakpoint and
	other extended debugging functions.
	We don’t respect the global NSDParamFrame (Newton Toolkit User’s Guide 7-23)
		verbose
		disasmInstWidth
		disasmArgWidth
----------------------------------------------------------------------------- */
extern "C" {
Ref FDisasm(RefArg rcvr, RefArg inFunc);
Ref FDisasmRange(RefArg rcvr, RefArg inFunc, RefArg inStart, RefArg inEnd);
}

void Disassemble(RefArg inFunc);
void Disassemble(RefArg inFunc, ArrayIndex inStart, ArrayIndex inEnd);
ArrayIndex PrintInstruction(bool in_function, unsigned char * instruction, RefArg inLiterals, RefArg inArgs, RefArg inDebugInfo);


Ref
FDisasm(RefArg rcvr, RefArg inFunc)
{
	Disassemble(inFunc, 0, kIndexNotFound);
	return NILREF;
}

Ref
FDisasmRange(RefArg rcvr, RefArg inFunc, RefArg inStart, RefArg inEnd)
{
	ArrayIndex start = 0;
	ArrayIndex end = kIndexNotFound;
	if (ISINT(inStart)) {
		start = RVALUE(inStart);
	}
	if (ISINT(inEnd)) {
		end = RVALUE(inEnd);
	}
	Disassemble(inFunc, start, end);
	return NILREF;
}


/* -----------------------------------------------------------------------------
	Disassemble function object.
	Args:		inFunc
	Return:	--
----------------------------------------------------------------------------- */

void
Disassemble(RefArg inFunc)
{
	Disassemble(inFunc, 0, kIndexNotFound);
}

void
Disassemble(RefArg inFunc, ArrayIndex inStart, ArrayIndex inEnd)
{
	Ref funcClass = ClassOf(inFunc);

	if (!(EQ(funcClass, SYMA(CodeBlock)) || EQ(funcClass, SYMA(_function)))) {
		ThrowMsg("not a codeblock");
	}

	RefVar instructions(GetArraySlot(inFunc, kFunctionInstructionsIndex));
	RefVar literals(GetArraySlot(inFunc, kFunctionLiteralsIndex));
	RefVar args(GetArraySlot(inFunc, kFunctionArgFrameIndex));
	RefVar debugInfo;
	if (Length(inFunc) > kFunctionDebugIndex) {
		debugInfo = GetArraySlot(inFunc, kFunctionDebugIndex);
	}
	if (!EQ(ClassOf(debugInfo), SYMA(dbg1))) {
		debugInfo = NILREF;
	}

	if (inEnd == kIndexNotFound || inEnd > Length(instructions)) {
		inEnd = Length(instructions);
	}
	if (inStart > inEnd) {
		inStart = inEnd;
	}

	CDataPtr instrData(instructions);
	unsigned char * instrPtr = (unsigned char *)instrData + inStart;
	ArrayIndex instrLen = 0;
	bool is_function = EQ(funcClass, SYMA(_function));
	for (ArrayIndex i = inStart; i < inEnd; i += instrLen, instrPtr += instrLen) {
		REPprintf("%4d: ", i);
		instrLen = PrintInstruction(is_function, instrPtr, literals, args, debugInfo);
		REPprintf("\n");
	}
}


/*------------------------------------------------------------------------------
	Print one instruction (bytecode) from a function frame.
	Called from Disassemble().
	Args:		in_function		is a '_function object (as opposed to 'CodeBlock)
				instruction
				inLiterals
				inArgs
				inDebugInfo
	Return:  instruction length, 1 | 3 bytes
------------------------------------------------------------------------------*/

ArrayIndex
PrintInstruction(bool in_function, unsigned char * instruction, RefArg inLiterals, RefArg inArgs, RefArg inDebugInfo)
{
	ArrayIndex		instrLen = 1;
	unsigned int	opCode = *instruction >> 3;
	unsigned int	b = *instruction & 0x07;
	if (b == 7) {
		b = (*(instruction + 1) << 8) + *(instruction + 2);
		instrLen = 3;
		REPprintf("%02X %04X  ", *instruction, b);
	} else {
		REPprintf("%02X       ", *instruction);
	}

	if (opCode == kOpcodeSimple) {
		REPprintf("%s", simpleInstrs[b]);
	} else {
		REPprintf("%s ", paramInstrs[opCode]);

		if (opCode == kOpcodePush || opCode == kOpcodeFindVar || opCode == kOpcodeFindAndSetVar) {
			PrintObject(GetArraySlot(inLiterals, b), 0);

		} else if (opCode == kOpcodePushConstant) {
			PrintObject(b, 0);

		} else if (opCode == kOpcodeGetVar || opCode == kOpcodeSetVar || opCode == kOpcodeIncrVar) {
			if (in_function) {
				const char * literalName = NULL;
				if (NOTNIL(inDebugInfo)) {
					ArrayIndex debugNamesBaseIndex = RINT(GetArraySlot(inDebugInfo, 0)) + 1;
					ArrayIndex literalNameIndex = debugNamesBaseIndex + b - 3;
					RefVar literal(GetArraySlot(inDebugInfo, literalNameIndex));
					if (NOTNIL(literal)) {
						literalName = SymbolName(literal);
					}
				}
				if (literalName) {
					REPprintf("%ld [%s]", b, literalName);
				} else {
					REPprintf("%ld", b);
				}
			} else {
				REPprintf("%ld [%s]", b, SymbolName(GetTag(((FrameObject *)ObjectPtr(inArgs))->map, b)));
			}

		} else if (opCode == kOpcodeFreqFunc) {
			REPprintf("%ld [%s/%ld]", b, gFreqFuncInfo[b].name, gFreqFuncInfo[b].numOfArgs);

		} else {
			REPprintf("%ld", b);
		}
	}

	return instrLen;
}
#endif

