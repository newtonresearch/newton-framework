/*
	File:		CompilerSupport.cc

	Contains:	NewtonScript compiler support functions.

	Written by:	The Newton Tools group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "Arrays.h"
#include "Frames.h"
#include "Lookup.h"
#include "Symbols.h"
#include "Iterators.h"

#include "Compiler.h"

/*----------------------------------------------------------------------
	C o n s t a n t s
----------------------------------------------------------------------*/

#define kLiteralsChunkSize			16
#define kInstructionsChunkSize  128


/*----------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------*/

extern int			gCompilerCompatibility;
extern Ref			gConstantsFrame;
extern BOOL			gPrintLiterals;


/*----------------------------------------------------------------------
	F u n c t i o n S t a t e
----------------------------------------------------------------------*/

static Ref
MakeCodeBlockFrame(void)
{
	return Clone(gCodeBlockPrototype);
}

static Ref
MakeDebugCodeBlockFrame(void)
{
	return Clone(gDebugCodeBlockPrototype);
}

#pragma mark -

/*----------------------------------------------------------------------
	Constructor.
	Args:		inCompiler	compiler context
				inArgs		function arg frame
				inState		enclosing function state
				ioDepthPtr	function depth
----------------------------------------------------------------------*/

CFunctionState::CFunctionState(CCompiler * inCompiler, RefArg inArgs, CFunctionState * inFunc, int * ioDepthPtr)
{
	int	depth;

	fCompiler = inCompiler;
	fArgs = inArgs;
	fLocals = NILREF;
	fConstants = NILREF;
	fInstructions = AllocateBinary(SYMA(instructions), kInstructionsChunkSize);
	fLiterals = AllocateArray(SYMA(literals), kLiteralsChunkSize);
	x1C = AllocateFrame();
	fPC = 0;
	fEncFunc = inFunc;
	fNumOfLiterals = 0;
	fLoop = nil;
	if (ioDepthPtr != NULL)
	{
		depth = *ioDepthPtr;
		*ioDepthPtr = depth + 1;
	}
	else
		depth = -1;
	fFuncDepth = depth;
	x40 = 0;
	x44 = NO;
	x48 = NO;
	x4C = NO;

	if (inFunc != nil)
	{
		fConstants = AllocateFrame();		// local constants
		SetFrameSlot(fConstants, SYMA(_proto), inFunc->fConstants);
	}
	else
		fConstants = gConstantsFrame;		// global constants

	if (FrameHasSlot(gVarFrame, MakeSymbol("dbgNoVarNames")))
		fKeepVarNames = ISNIL(GetFrameSlot(gVarFrame, MakeSymbol("dbgNoVarNames")));
	else if (FrameHasSlot(gVarFrame, MakeSymbol("dbgKeepVarNames")))
		fKeepVarNames = NOTNIL(GetFrameSlot(gVarFrame, MakeSymbol("dbgKeepVarNames")));
	else
		fKeepVarNames = NO;

	// set up lexical scope?
	x58 = inFunc;
}


/*----------------------------------------------------------------------
	Destructor.
----------------------------------------------------------------------*/

CFunctionState::~CFunctionState()
{
	if (x58)
		delete x58;
	if (fLoop)
		delete fLoop;
}


/*----------------------------------------------------------------------
	Determine whether this function is at the top level
	(or conversely is enclosed by further function state).
	Args:		--
	Return:	YES => is at top level
----------------------------------------------------------------------*/

BOOL
CFunctionState::atTopLevel(void)
{
	return (fEncFunc == nil);
}


/*----------------------------------------------------------------------
	Set up a frame that maps var name to index.
	In OS 2, a function’s args and local vars are indexed off the stack.
	Initially all indices are zero.
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

void
CFunctionState::computeInitialVarLocs(void)
{
	RefVar	tag;
	Ref		zero = MAKEINT(0);
	Ref *		RSzero = &zero;

	fVarLocs = AllocateFrame();
	for (ArrayIndex i = 0; i < fNumOfArgs; i++)
	{
		tag = GetArraySlot(fArgs, i);
		SetFrameSlot(fVarLocs, tag, RA(zero));
	}

	for (ArrayIndex i = 0; i < fNumOfLocals; i++)
	{
		tag = GetArraySlot(fLocals, i);
		SetFrameSlot(fVarLocs, tag, RA(zero));
	}
}


/*----------------------------------------------------------------------
	Set up the arg frame for a function call.
	OS 1	-	_nextArgFrame, _parent, _implementor, args, locals
				frame as documented
	OS 2	-	slightly different
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

void
CFunctionState::computeArgFrame(void)
{
	if (gCompilerCompatibility == 0)
	{
	//	OS 1
		RefVar argFrTags(MakeArray(3 + fNumOfArgs + fNumOfLocals));
		SetArraySlot(argFrTags, 0, SYMA(_nextArgFrame));
		SetArraySlot(argFrTags, 1, SYMA(_parent));
		SetArraySlot(argFrTags, 2, SYMA(_implementor));
		if (fNumOfArgs != 0)
			ArrayMunger(argFrTags, 3, fNumOfArgs, fArgs, 0, fNumOfArgs);
		if (fNumOfLocals != 0)
			ArrayMunger(argFrTags, 3 + fNumOfArgs, fNumOfLocals, fLocals, 0, fNumOfLocals);
		fArgFrame = AllocateFrameWithMap(AllocateMapWithTags(NILREF, argFrTags));
	}

	else
	{
	//	OS 2
		// where do closed vars fit in here?
		RefVar	argFrTags(MakeArray(3 + fNumOfArgs + fNumOfLocals));
		RefVar	tag;

		// iterate over the args frame -
		// nil out arg names in fVarLocs
		for (ArrayIndex i = 0; i < fNumOfArgs; i++)
		{
			tag = GetArraySlot(fArgs, i);
			if (ISINT(GetFrameSlot(fVarLocs, tag)))
				SetFrameSlot(fVarLocs, tag, RA(NILREF));
		}

		// iterate over the fVarLocs frame -
		// any unflagged names must be locals, so set their index correctly
		// any flagged names are arg names so set them up in the arg frame map
		int	argIndex = 3;
		int	firstLocalIndex = 3 + fNumOfArgs;
		int	localIndex = firstLocalIndex;
		Ref	iterVal;
		CObjectIterator	iter(fVarLocs);
		for ( ; !iter.done(); iter.next())
		{
			iterVal = iter.value();
			if (ISINT(iterVal))
				SetFrameSlot(fVarLocs, iter.tag(), MAKEINT(localIndex++));
			else if (NOTNIL(iterVal))
				SetArraySlot(argFrTags, argIndex++, iter.tag());
		}

		// iterate over the args frame -
		// restore arg names to their correct index
		for (ArrayIndex i = 0; i < fNumOfArgs; i++)
		{
			tag = GetArraySlot(fArgs, i);
			if (ISNIL(GetFrameSlot(fVarLocs, tag)))
				SetFrameSlot(fVarLocs, tag, MAKEINT(i));
		}

		// if there are any args, or we have some context, create the arg frame
		if (argIndex > kArgFrameArgIndex
		|| (!atTopLevel() && (x44 || x48 || x4C)))
		{
			SetLength(argFrTags, argIndex);
			SetArraySlot(argFrTags, kArgFrameNextArgFrameIndex, SYMA(_nextArgFrame));
			SetArraySlot(argFrTags, kArgFrameParentIndex, SYMA(_parent));
			SetArraySlot(argFrTags, kArgFrameImplementorIndex, SYMA(_implementor));
			fArgFrame = AllocateFrameWithMap(AllocateMapWithTags(NILREF, argFrTags));
			if (!x44 && !atTopLevel())
				SetArraySlot(fArgFrame, kArgFrameParentIndex, fVarLocs);
			if (!x48 && !atTopLevel())
				SetArraySlot(fArgFrame, kArgFrameImplementorIndex, fVarLocs);
		}

		fNumOfVarLocs = localIndex - firstLocalIndex;
	}
}


/*----------------------------------------------------------------------
	Add a constant value to the constants frame.
	Args:		inTag		the constant’s name
				inVal		the constant value
	Return:	--
----------------------------------------------------------------------*/

void
CFunctionState::addConstant(RefArg inTag, RefArg inVal)
{
	if (isLocalVariable(inTag))
		fCompiler->errorWithValue(kNSErrVarAndConstCollision, inTag);

	SetFrameSlot(fConstants, inTag, inVal);
}


/*----------------------------------------------------------------------
	Add an array of local variables to the locals frame.
	Args:		inLocals		an array of local variable names
	Return:	--
----------------------------------------------------------------------*/

void
CFunctionState::addLocals(RefArg inLocals)
{
	if (ISNIL(fLocals))
		fLocals = inLocals;
	else
	{
		RefVar	tag;
		ArrayIndex	i, count = Length(inLocals);
		for (i = 0; i < count; i++)
		{
			tag = GetArraySlot(inLocals, i);
			if (isConstant(tag))
				fCompiler->errorWithValue(kNSErrVarAndConstCollision, tag);
			if (ArrayPosition(fLocals, tag, 0, NILREF) == -1)
				AddArraySlot(fLocals, tag);
		}
	}
}


/*----------------------------------------------------------------------
	If any of the args is closed, emit code to set its initial value.
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

void
CFunctionState::copyClosedArgs(void)
{
	RefVar	tag;
	ArrayIndex	i, count = Length(fArgs);
	for (i = 0; i < count; i++)
	{
		tag = GetArraySlot(fArgs, i);
		if (variableIndex(tag) == -1)
		{
			ArrayIndex index = ArrayPosition(fArgs, tag, 0, NILREF);
			emit(kOpcodeGetVar, 3 + index);
			emit(kOpcodeFindAndSetVar, literalOffset(tag));
		}
	}
}


/*----------------------------------------------------------------------
	Set up relevant info once all declarations have been read.
	Args:		--
	Return:	--
----------------------------------------------------------------------*/

void
CFunctionState::declarationsFinished(void)
{
	fNumOfArgs = NOTNIL(fArgs) ? Length(fArgs) : 0;
	fNumOfLocals = NOTNIL(fLocals) ? Length(fLocals) : 0;
}


/*----------------------------------------------------------------------
	Return a value from the constants frame.
	Args:		inTag			name of the constant
				outExists	YES => constant actually exists
	Return:	the value
----------------------------------------------------------------------*/

Ref
CFunctionState::getConstantValue(RefArg inTag, BOOL * outExists)
{
	return GetProtoVariable(fConstants, inTag, outExists);
}


/*----------------------------------------------------------------------
	Determine whether a name refers to a constant
	(as opposed to a variable).
	Args:		inTag			name of the item
	Return:	YES => is a constant
----------------------------------------------------------------------*/

BOOL
CFunctionState::isConstant(RefArg inTag)
{
	CFunctionState * funcState;

	if (!EQ(inTag, SYMA(_proto)))
	{
		for (funcState = this; funcState != nil; funcState = funcState->x58)
		{
			if (funcState->isLocalConstant(inTag))
				return YES;
			if (funcState->isLocalVariable(inTag))
				return NO;
		}
	}

	return NO;
}


/*----------------------------------------------------------------------
	Determine whether a name refers to a constant local to this function.
	Args:		inTag			name of the item
	Return:	YES => is a local constant
----------------------------------------------------------------------*/

BOOL
CFunctionState::isLocalConstant(RefArg inTag)
{
	return FrameHasSlot(fConstants, inTag);
}


/*----------------------------------------------------------------------
	Determine whether a name refers to a variable local to this function.
	Args:		inTag			name of the item
	Return:	YES => is a local variable
----------------------------------------------------------------------*/

BOOL
CFunctionState::isLocalVariable(RefArg inTag)
{
	if (NOTNIL(fLocals) && ArrayPosition(fLocals, inTag, 0, NILREF) != kIndexNotFound)
		return YES;
	if (NOTNIL(fArgs) && ArrayPosition(fArgs, inTag, 0, NILREF) != kIndexNotFound)
		return YES;
	return NO;
}


/*----------------------------------------------------------------------
	Return the index of a local variable.
	OS1	-	index into the arg frame
	OS2	-	index into the stack
	Args:		tag		the variable name
	Return:	index of named variable
----------------------------------------------------------------------*/

ArrayIndex
CFunctionState::variableIndex(RefArg inTag)
{
	if (gCompilerCompatibility == 0)
	//	OS 1
		return FrameSlotPosition(fArgFrame, inTag);
	else
	//	OS 2
		return RINT(GetFrameSlot(fVarLocs, inTag));
}


ArrayIndex
CFunctionState::literalOffset(RefArg inLiteral)
{
	for (ArrayIndex i = 0; i < fNumOfLiterals; i++)
	{
		Ref	lit = GetArraySlot(fLiterals, i);
		if (((ISMAGICPTR(lit) || ISMAGICPTR(inLiteral)) && lit == inLiteral)
		||  EQ(lit, inLiteral))
			return i;
	}
	// literal wasn’t found so add it to our list
	if (fNumOfLiterals >= Length(fLiterals))
		SetLength(fLiterals, Length(fLiterals) + kLiteralsChunkSize);
	SetArraySlot(fLiterals, fNumOfLiterals, inLiteral);
	return fNumOfLiterals++;
}


void
CFunctionState::noteMsgEnvReference(MsgEnvComponent msg)
{
	CFunctionState * state;
	for (state = this; state != nil; state = state->fEncFunc)
	{
		if (msg == 0)			// slot access
			state->x44 = YES;
		else if (msg == 1)	// message send
			state->x48 = YES;
	}
}


BOOL
CFunctionState::noteVarReference(RefArg inVarName)
{
	BOOL	isNoted;

	if (isLocalVariable(inVarName))
	{
		SetFrameSlot(x1C, inVarName, RA(TRUEREF));
		SetFrameSlot(fVarLocs, inVarName, SYMA(closed));
		isNoted = YES;
	}
	 else if (fEncFunc != nil)
	{
		isNoted = fEncFunc->noteVarReference(inVarName);
		if (isNoted)
			x4C = YES;	// var is inherited
	}
	else
		isNoted = NO;

	return isNoted;
}


/*----------------------------------------------------------------------
	L o o p s
----------------------------------------------------------------------*/

void
CFunctionState::beginLoop(void)
{
	CLoopState * theLoop = new CLoopState(this, fLoop);
	if (theLoop == nil)
		OutOfMemory();
	fLoop = theLoop;
}


void
CFunctionState::addLoopExit(void)
{
	CHECK(fLoop != nil, "BREAK statement outside a loop");
	fLoop->addExit(curPC());
	emitPlaceholder();
}


void
CFunctionState::endLoop(void)
{
	CLoopState * theLoop = fLoop;
	fLoop = theLoop->endLoop(curPC());
	delete theLoop;
}


/*----------------------------------------------------------------------
	C o d e   G e n e r a t i o n
----------------------------------------------------------------------*/

Ref
CFunctionState::makeCodeBlock(void)
{
	RefVar cbf;

	SetLength(fLiterals, fNumOfLiterals);
	SetLength(fInstructions, curPC());
	cbf = (fFuncDepth >= 0 || fKeepVarNames)	? MakeDebugCodeBlockFrame()
															: MakeCodeBlockFrame();
	if (fFuncDepth >= 0)
		SetFrameSlot(cbf, SYMA(debuggerInfo), MAKEINT(fFuncDepth));

	SetArraySlot(cbf, kFunctionInstructionsIndex, fInstructions);
	if (fNumOfLiterals > 0)
		SetArraySlot(cbf, kFunctionLiteralsIndex, fLiterals);
	else
		SetArraySlot(cbf, kFunctionLiteralsIndex, RA(NILREF));

	if (gCompilerCompatibility == 0)
	//	OS 1
		SetArraySlot(cbf, kFunctionNumArgsIndex, MAKEINT(fNumOfArgs));
	else
	{
	//	OS 2
		SetArraySlot(cbf, kFunctionClassIndex, kPlainFuncClass);		// special Ref instead of symbol
		SetArraySlot(cbf, kFunctionNumArgsIndex, MAKEINT(fNumOfArgs + (fNumOfVarLocs << 16)));
		if (fFuncDepth < 0 && fKeepVarNames)	// no depth info but want var names anyway
		{
			RefVar	dbg(AllocateArray(MakeSymbol("dbg1"), 1));
			CFunctionState *	func;
			for (func = this; func != nil; func = func->context())
			{
				if (NOTNIL(func->fArgFrame))
				{
					CObjectIterator	iter(func->fArgFrame);
					iter.next();	// skip _nextArgFrame
					iter.next();	// skip _parent
					iter.next();	// skip _implementor
					for ( ; !iter.done(); iter.next())
					{
						AddArraySlot(dbg, iter.value());	// add inherited var names
					}
				}
			}

			ArrayIndex	dbgLen = Length(dbg);
			SetArraySlot(dbg, 0, MAKEINT(dbgLen - 1));	// number of inherited names | index to arg names
			SetLength(dbg, dbgLen + fNumOfArgs + fNumOfVarLocs);
			CObjectIterator iter(fVarLocs);
			for ( ; !iter.done(); iter.next())
			{
				if (ISINT(iter.value()))
					SetArraySlot(dbg, RINT(iter.value()) + dbgLen - 3, iter.tag());
				else
				{
					ArrayIndex posn = ArrayPosition(fArgs, iter.tag(), 0, NILREF);
					if (posn != kIndexNotFound)
						SetArraySlot(dbg, dbgLen + posn, MAKEINT(ArrayPosition(dbg, iter.tag(), 0, NILREF)));
				}
			}
			if (Length(dbg) > 1)
				SetArraySlot(cbf, kFunctionDebugIndex, dbg);
		}
	}
	SetArraySlot(cbf, kFunctionArgFrameIndex, fArgFrame);

	if (gPrintLiterals)
	{
		PrintObject(fLiterals, 0);
		REPprintf("\n");
	}

	return cbf;
}


/*----------------------------------------------------------------------
	Emit an instruction.
	Args:		a			opcode
				b			operand
	Return:	PC of next instruction
				fPC is also updated.
----------------------------------------------------------------------*/

ArrayIndex
CFunctionState::emit(Opcode a, int b)
{
	if (b >= 0 && b < 7)
		return emitOne((a << 3) | b);
	else
		return emitThree((a << 3) | 0x07, b);
}


/*----------------------------------------------------------------------
	Emit a 1-byte instruction.
	Args:		bytecode	instruction
	Return:	PC of next instruction
				fPC is also updated.
----------------------------------------------------------------------*/

ArrayIndex
CFunctionState::emitOne(unsigned char bytecode)
{
	ArrayIndex	pc = curPC();
	ArrayIndex	instrLen = Length(fInstructions);

	if (instrLen < pc + 1)
		SetLength(fInstructions, instrLen + kInstructionsChunkSize);

	unsigned char * p = (unsigned char *) BinaryData(fInstructions) + pc;
	*p = bytecode;
	fPC = pc + 1;
	return fPC;
}


/*----------------------------------------------------------------------
	Emit a 3-byte instruction.
	Args:		bytecode	instruction
				b			operand
	Return:	PC of next instruction
				fPC is also updated.
----------------------------------------------------------------------*/

ArrayIndex
CFunctionState::emitThree(unsigned char bytecode, int b)
{
	ArrayIndex	pc = curPC();
	ArrayIndex	instrLen = Length(fInstructions);

	if (instrLen < pc + 1)
		SetLength(fInstructions, instrLen + kInstructionsChunkSize);

	unsigned char * p = (unsigned char *) BinaryData(fInstructions) + pc;
	*p++ = bytecode;
	*p++ = b >> 8;
	*p = b;
	fPC = pc + 3;
	return fPC;
}


/*----------------------------------------------------------------------
	Emit a 3-byte placeholder instruction.
	Return:	PC of this instruction (!)
----------------------------------------------------------------------*/

ArrayIndex
CFunctionState::emitPlaceholder(void)
{
	ArrayIndex	pc = curPC();
	emitThree(kOpcodeBranch, 0);
	return pc;
}


/*----------------------------------------------------------------------
	Patch a placeholder 3-byte instruction.
	Args:		offset	offset in instructions binary object
				a			opcode to patch in
				b			operand
	Return:	--
----------------------------------------------------------------------*/

void
CFunctionState::backpatch(ArrayIndex inPC, Opcode a, int b)
{
	unsigned char *	p = (unsigned char *) BinaryData(fInstructions) + inPC;
	*p++ = (a << 3) | 0x07;
	*p++ = b >> 8;
	*p = b;
}

#pragma mark -

/*----------------------------------------------------------------------
	C L o o p S t a t e
----------------------------------------------------------------------*/

/*----------------------------------------------------------------------
	Construct a CLoopState.
	Args:		inFunc		function state in which this loop exists
				inLoop		outer loop (if any) enclosing this loop
----------------------------------------------------------------------*/

CLoopState::CLoopState(CFunctionState * inFunc, CLoopState * inLoop)
{
	fEncFunc = inFunc;
	fEncLoop = inLoop;
	fExits = MakeArray(0);
}


/*----------------------------------------------------------------------
	Destroy a CLoopState.
	Delete any enclosing loop state.
----------------------------------------------------------------------*/

CLoopState::~CLoopState()
{
	if (fEncLoop)
		delete fEncLoop;
}


/*----------------------------------------------------------------------
	Remember a loop exit.
	Args:		inPC		the PC of exit instruction
	Return:	--
----------------------------------------------------------------------*/

void
CLoopState::addExit(ArrayIndex inPC)
{
	AddArraySlot(fExits, MAKEINT(inPC));
}


/*----------------------------------------------------------------------
	Backpatch remembered loop exit branch offsets.
	Args:		inPC		the PC of the loop exit (to go to)
	Return:	the outer loop enclosing this (ended) one
----------------------------------------------------------------------*/

CLoopState *
CLoopState::endLoop(ArrayIndex inPC)
{
	ArrayIndex	i, count = Length(fExits);
	for (i = 0; i < count; i++)
		fEncFunc->backpatch(RINT(GetArraySlot(fExits, i)), kOpcodeBranch, inPC);

	CLoopState *	encLoop = fEncLoop;
	fEncLoop = nil;
	return encLoop;
}
