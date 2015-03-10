/*
	File:		CInterpreter.cc

	Contains:	The NewtonScript CInterpreter.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "RefMemory.h"
#include "TaskGlobals.h"
#include "VirtualMemory.h"
#include "Iterators.h"
#include "Funcs.h"
#include "RichStrings.h"
#include "Unicode.h"
#include "Maths.h"
#include "Interpreter.h"
#include "Lookup.h"
#include "ObjectHeap.h"
#include "NewtGlobals.h"
#include "DeveloperNotification.h"
#include "REPTranslators.h"

DeclareException(exMessage, exRootException);
DeclareException(exInterpreter, exRootException);
DeclareException(exInterpreterData, exRootException);

/* -----------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
----------------------------------------------------------------------------- */

extern "C" {
Ref	FNewIterator(RefArg inRcvr, RefArg inObj, RefArg inDeeply);
Ref	FGetFunctionArgCount(RefArg inRcvr, RefArg inFn);
Ref	FApply(RefArg inRcvr, RefArg inFn, RefArg inArgs);
Ref	FPerform(RefArg inRcvr, RefArg inRx, RefArg inMsg, RefArg inArgs);
Ref	FPerformIfDefined(RefArg inRcvr, RefArg inRx, RefArg inMsg, RefArg inArgs);
Ref	FProtoPerform(RefArg inRcvr, RefArg inRx, RefArg inMsg, RefArg inArgs);
Ref	FProtoPerformIfDefined(RefArg inRcvr, RefArg inRx, RefArg inMsg, RefArg inArgs);
Ref	FStackTrace(RefArg inRcvr);
}


/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

Ref				gCodeBlockPrototype;			// 0C10517C
Ref				gDebugCodeBlockPrototype;	// 0C105180
Ref				gCFunctionPrototype;			// 0C105470

extern Ref		gConstantsFrame;
Ref				gFunctionFrame;				// +00 0C10544C - frame, GCRoot  -  this is actually THE global
Ref *				RSgFunctionFrame = &gFunctionFrame;
Ref				gFreqFuncs;						// +04 0C105450 - array, GCRoot
bool				gUseCFunctionDocStrings;	// +08 0C105454 - byte
CInterpreter *	gInterpreter;					// +0C 0C105458
CInterpreter *	gInterpreterList;				// +10 0C10545C link to next interpreter instance
bool				gFramesBreakPointsEnabled;	// +14 0C105460 - byte
Ref				gFramesBreakPoints;			// +18 0C105464 - frame, GCRoot


/* -----------------------------------------------------------------------------
	F r a m e s F u n c t i o n P r o f i l i n g
----------------------------------------------------------------------------- */

#define	kProfilingEnabled 'ffpe'

struct FramesFunctionProfilerInfo
{
	TraceFunction func;
	void * data;
};


ULong				gFramesFunctionProfilingEnabled;	// 0C10535C
TraceFunction	gFramesFunctionProfiler;			// 0C105360
void *			gFramesFunctionProfilerData;		// 0C105364


/*------------------------------------------------------------------------------
	I t e r a t o r
------------------------------------------------------------------------------*/

static void		ForEachLoopReset(RefArg iter, Ref obj);
static bool		ForEachLoopNext(RefArg iter);
static bool		ForEachLoopDone(RefArg iter);

Ref
FNewIterator(RefArg inRcvr, RefArg inObj, RefArg inDeeply)
{
	if (!(ISPTR(inObj && (ObjectFlags(inObj) & kObjSlotted))))
		ThrowBadTypeWithFrameData(inObj, kNSErrNotAFrameOrArray);

	ArrayIndex	slotCount = Length(inObj);
	RefVar	iter(AllocateArray(SYMA(forEachState), kIterSize));
	SetArraySlot(iter, kIterObjectIndex, inObj);
	SetArraySlot(iter, kIterNumOfSlotsIndex, MAKEINT(slotCount));

	if (ObjectFlags(inObj) & kObjFrame)
	{
		SetArraySlot(iter, kIterMapIndex, ((FrameObject*)ObjectPtr(inObj))->map);
		if (NOTNIL(inDeeply))
		{
			ArrayIndex  deepCount = slotCount;
			RefVar	current;
			for (current = GetFrameSlot(inObj, SYMA(_proto)); NOTNIL(current); current = GetFrameSlot(current, SYMA(_proto)))
				deepCount = deepCount + Length(current) - 1;
			SetArraySlot(iter, kIterDeepNumOfSlotsIndex, MAKEINT(deepCount));
		}
		else
			SetArraySlot(iter, kIterDeepNumOfSlotsIndex, NILREF);
	}
	else
	{
		SetArraySlot(iter, kIterMapIndex, RA(NILREF));
		SetArraySlot(iter, kIterDeepNumOfSlotsIndex, NOTNIL(inDeeply) ? MAKEINT(slotCount) : NILREF);
	}
	SetArraySlot(iter, kIterSlotIndex, MAKEINT(-1));
	ForEachLoopNext(iter);
	return iter;
}


/*------------------------------------------------------------------------------
	iter-new
------------------------------------------------------------------------------*/

static void
ForEachLoopReset(RefArg iter, Ref obj)
{
	SetArraySlotRef(iter, kIterObjectIndex, obj);
	SetArraySlotRef(iter, kIterMapIndex, (ObjectFlags(obj) & kObjFrame) ? ((FrameObject *)ObjectPtr(obj))->map : NILREF);
	SetArraySlotRef(iter, kIterNumOfSlotsIndex, MAKEINT(Length(obj)));
	SetArraySlotRef(iter, kIterSlotIndex, MAKEINT(-1));
	ForEachLoopNext(iter);
}


/*------------------------------------------------------------------------------
	iter-next
	iterator --
------------------------------------------------------------------------------*/

static bool
ForEachLoopNext(RefArg iter)
{
	RefVar obj(GetArraySlot(iter, kIterObjectIndex));
	RefVar iterMap;
	Ref	 proto;
	bool	 isDeep, isMore;
	ArrayIndex	 numOfSlots = RINT(GetArraySlot(iter, kIterNumOfSlotsIndex));
	ArrayIndex	 numOfObjSlots = Length(obj);
	ArrayIndex	 slotIndex = RINT(GetArraySlot(iter, kIterSlotIndex));

	if (numOfObjSlots >= numOfSlots)
		SetArraySlotRef(iter, kIterSlotIndex, MAKEINT(++slotIndex));
	if (numOfObjSlots != numOfSlots)
		SetArraySlotRef(iter, kIterNumOfSlotsIndex, MAKEINT(numOfObjSlots));
	isDeep = NOTNIL(GetArraySlot(iter, kIterDeepNumOfSlotsIndex));
	iterMap = GetArraySlot(iter, kIterMapIndex);
	if (slotIndex < numOfObjSlots)
	{
		Ref	tag = NOTNIL(iterMap) ? GetTag(iterMap, slotIndex) : MAKEINT(slotIndex);
		SetArraySlotRef(iter, kIterTagIndex, tag);
		if (isDeep && EQRef(tag, RSYM_proto))
			isMore = ForEachLoopNext(iter);
		else
		{
			SetArraySlotRef(iter, kIterValueIndex, GetArraySlot(obj, slotIndex));
			isMore = YES;
		}
	}
	else if (!isDeep || ISNIL(iterMap) || ISNIL(proto = GetFrameSlot(obj, SYMA(_proto))))
	{
		SetArraySlotRef(iter, kIterTagIndex, NILREF);
		SetArraySlotRef(iter, kIterValueIndex, NILREF);
		isMore = NO;
	}
	else
	{
		ForEachLoopReset(iter, proto);
		isMore = !ForEachLoopDone(iter);
	}
	return isMore;
}

/*------------------------------------------------------------------------------
	iter-done
	iterator -- done?
------------------------------------------------------------------------------*/

static bool
ForEachLoopDone(RefArg iter)
{
	ArrayIndex	currentIndex = RINT(GetArraySlot(iter, kIterSlotIndex));
	ArrayIndex	lastIndex = RINT(GetArraySlot(iter, kIterNumOfSlotsIndex));
	if (NOTNIL(GetArraySlot(iter, kIterDeepNumOfSlotsIndex))
	 && NOTNIL(GetArraySlot(iter, kIterMapIndex)))
	{
		if (currentIndex < lastIndex)
			return NO;
		return ISNIL(GetFrameSlot(GetArraySlot(iter, kIterObjectIndex), SYMA(_proto)));
	}
	
	return currentIndex >= lastIndex;
}

#pragma mark -

/*------------------------------------------------------------------------------
	R e f S t a c k
------------------------------------------------------------------------------*/

#define	kRefStackMax	300

void
MarkRefStack(RefStack * inStack)
{
	for (Ref * p = inStack->base; p < inStack->top; p++)
		DIYGCMark(*p);
}


void
UpdateRefStack(RefStack * inStack)
{
	for (Ref * p = inStack->base; p < inStack->top; p++)
		*p = DIYGCUpdate(*p);
}


bool
ReleaseRefStackProc(void * p, OpaqueRef * bp, OpaqueRef * lp, bool c)
{
	*bp = (OpaqueRef) ((RefStack *)p)->base;
	*lp = (OpaqueRef) ((RefStack *)p)->limit;
	return c;
}


RefStack::RefStack()
{
	VAddr			topOfStack;
	VAddr			bottomOfStack;

	size = kRefStackMax;
	NewtonErr	error = NewStack(TaskSwitchedGlobals()->fDefaultHeapDomainId,
											64 * KByte,
											TaskSwitchedGlobals()->fTaskId,
											&topOfStack, &bottomOfStack);
	if (error)
		ThrowErr(exRootException, error);
	base = (Ref *)bottomOfStack;
	top = (Ref *)bottomOfStack;
	limit = base + kRefStackMax - sizeof(VMState) / sizeof(Ref);

	DIYGCRegister(this, (GCProcPtr) MarkRefStack, (GCProcPtr) UpdateRefStack);
}


RefStack::~RefStack()
{
	DIYGCUnregister(this);
	FreePagedMem((VAddr) base);
}


void
RefStack::reset(ArrayIndex newSize)
{
	ArrayIndex	currentSize = (top - base) - 1;  // assumes pointer math
	int	delta = currentSize - newSize;
	if (delta > 0)
		top -= delta;
}


void
RefStack::pushNILs(ArrayIndex count)
{
	Ref * s = top;
	top = top + count;
	for ( ; count >= 8; count -=8, s += 8)
	{
		s[0] = s[1] = s[2] = s[3] = s[4] = s[5] = s[6] = s[7] = NILREF;
	}
	switch (count)
	{
	case 7:
		s[6] = NILREF;
	case 6:
		s[5] = NILREF;
	case 5:
		s[4] = NILREF;
	case 4:
		s[3] = NILREF;
	case 3:
		s[2] = NILREF;
	case 2:
		s[1] = NILREF;
	case 1:
		s[0] = NILREF;
	case 0:
		break;
	}
}

#pragma mark -

/*------------------------------------------------------------------------------
	R e f S t r u c t S t a c k
------------------------------------------------------------------------------*/

bool
ReleaseRefStructStackProc(void * p, OpaqueRef * bp, OpaqueRef * lp, bool c)
{
	*bp = (OpaqueRef) ((RefStructStack *)p)->xBase;
	*lp = (OpaqueRef) ((RefStructStack *)p)->xBase + (((RefStructStack *)p)->limit - ((RefStructStack *)p)->base);
	return c;
}


RefStructStack::RefStructStack()
{
	VAddr	topOfStack;
	VAddr	bottomOfStack;

	size = kRefStackMax;
	NewtonErr	error = NewStack(TaskSwitchedGlobals()->fDefaultHeapDomainId,
											64 * KByte,
											TaskSwitchedGlobals()->fTaskId,
											&topOfStack, &bottomOfStack);
	if (error)
		ThrowErr(exRootException, error);

	xBase = xTop = (RefStruct *)bottomOfStack;
}


RefStructStack::~RefStructStack()
{
	FreePagedMem((VAddr) xBase);
}


void
RefStructStack::fill(void)
{
	RefStruct * dst = xTop;
	RefStruct * end = xBase + (top - base);	// assumes pointer math
	Ref *  src = base + (xTop - xBase);
	for ( ; dst < end; dst++)
		dst->h = (RefHandle *)src++;
	xTop = dst;
}

#pragma mark -

/*------------------------------------------------------------------------------
	I n t e r p r e t e r   S t a c k
------------------------------------------------------------------------------*/

VMState *
VMStack::push(void)
{
	VMState * p = (VMState *)top;

	p->pc =
	p->func =
	p->locals =
	p->impl =
	p->rcvr =
	p->stackFrame = NILREF;
	top = (Ref *)(p + 1);

	return p;
}


VMState *
VMStack::dup(void)
{
	VMState * p = (VMState *)top;
	VMState * q = (VMState *)top - 1;

	p->pc		= q->pc;
	p->func	= q->func;
	p->locals	= q->locals;
	p->impl		= q->impl;
	p->rcvr			= q->rcvr;
	p->stackFrame	= q->stackFrame;
	top = (Ref *)(p + 1);

	return p;
}


VMState *
VMStack::pop(void)
{
	VMState * p = (VMState *)top - 1;
	top = (Ref *)p;
	return p - 1;
}


VMState *
VMStack::at(ArrayIndex index)
{
	return (VMState *)base + index;
}


#pragma mark -

/*------------------------------------------------------------------------------
	I n t e r p r e t e r
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialize fork globals.
	Args:		inGlobals		pointer to fork’s globals
	Return:  error code
------------------------------------------------------------------------------*/

NewtonErr
InitForkGlobalsForFrames(NewtGlobals * inGlobals)
{
	NewtGlobals *	newt = GetNewtGlobals();

	// create a new interpreter instance for the fork
	CInterpreter *	intrp = new CInterpreter;
	newt->interpreter = intrp;
	if (intrp == NULL)
		return MemError();
// if new -> NewPtr() as in the original you could:
//	SetPtrName(intrp, 'intp');

	// give it a unique id
	ArrayIndex	index = inGlobals->lastId;	// short
	for (intrp = gInterpreterList; intrp != NULL; )
	{
		index++;
		for (intrp = gInterpreterList; intrp != NULL; intrp = intrp->next)
			 if (intrp->id == index)	// ensure next unique id hasn’t already been taken
				break;
	}
	newt->interpreter->id = index;
	newt->lastId = index;

#if defined(correct)
	// set up fork’s stack
	VAddr		stackBase;
	CUTask	aTask;
	return GetTaskStackInfo(&aTask, &newt->stackTop, &stackBase);
#else
	return noErr;
#endif
}


/*------------------------------------------------------------------------------
	Switch fork globals in/out.
	Args:		inDoFork		YES => switching into fork
	Return:  --
------------------------------------------------------------------------------*/

void
SwitchFramesForkGlobals(bool inDoFork)
{
	NewtGlobals *	newt = GetNewtGlobals();
	if (inDoFork)
	{
		gCurrentStackPos = newt->lastId;
		gInterpreter = newt->interpreter;
	}
	else
		newt->lastId = gCurrentStackPos;
}


/*------------------------------------------------------------------------------
	Destroy fork globals - fork is dead.
	Args:		inGlobals	pointer to fork’s globals
	Return:  --
------------------------------------------------------------------------------*/

void
DestroyForkGlobalsForFrames(NewtGlobals * inGlobals)
{
	if (inGlobals->interpreter)
		delete inGlobals->interpreter;
}


/*------------------------------------------------------------------------------
	Initialize the global functions frame.
	Also function frames - for the compiler?
	This is a bit suspect - we don’t use the old CodeBlock function frames
	any more.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

static void
InitFunctions(void)
{
	gFunctionFrame = AllocateFrame();

	gDebugCodeBlockPrototype = RA(debugCodeBlockPrototype);
	gCodeBlockPrototype = RA(codeBlockPrototype);

	if (gUseCFunctionDocStrings)
		gCFunctionPrototype = RA(debugCFunctionPrototype);
	else
		gCFunctionPrototype = RA(cFunctionPrototype);
}


/*------------------------------------------------------------------------------
	Initialize the interpreter environment.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

void
InitInterpreter(void)
{
	InitICache();
	InitFunctions();

	// set up an array of freq-func functions
	AddGCRoot(&gFreqFuncs);
	gFreqFuncs = MakeArray(gNumFreqFuncs);
	for (ArrayIndex i = 0; i < gNumFreqFuncs; ++i)
	{
		Ref	opTag = MakeSymbol(gFreqFuncInfo[i].name);
		Ref	opFunc = GetFrameSlot(RA(gFunctionFrame), opTag);
		SetArraySlotRef(gFreqFuncs, i, opFunc);
	}

	AddGCRoot(&gConstantsFrame);
	gConstantsFrame = AllocateFrame();

	// create a global interpreter
	CInterpreter * intrp = new CInterpreter;
	intrp->id = 0;
	gInterpreter = intrp;
	gNewtGlobals->interpreter = intrp;

	// initialize breakpoints
	gFramesBreakPointsEnabled = NO;
	gFramesBreakPoints = NILREF;
	AddGCRoot(&gFramesBreakPoints);
}


/*------------------------------------------------------------------------------
	Get the threaded interpreter with the given id.
	Args:		inId
	Return:  the threaded interpreter instance
------------------------------------------------------------------------------*/

CInterpreter *
GetCInterpreter(ULong inId)
{
	CInterpreter * iPtr;

	while ((iPtr = gInterpreterList) != NULL)
		if (iPtr->id == inId)
			return iPtr;

	return NULL;
}


/*------------------------------------------------------------------------------
	Get the global interpreter.
	Args:		--
	Return:  the global interpreter instance
------------------------------------------------------------------------------*/

CInterpreter *
GetGInterpreter(void)
{
	return gInterpreter;
}


/*------------------------------------------------------------------------------
	Get the global function frame.
	Not referenced anywhere - presumably meant to be inline.
	Args:		--
	Return:  functions
------------------------------------------------------------------------------*/

Ref
GetGFunctionFrame(void)
{
	return gFunctionFrame;
}


#pragma mark -


/*------------------------------------------------------------------------------
	Return the number of function arguments.
------------------------------------------------------------------------------*/

ArrayIndex
GetFunctionArgCount(Ref fn)
{
	if (!IsFunction(fn))
		ThrowBadTypeWithFrameData(kNSErrNotAFunction, fn);

	ArrayIndex	argCount = 2;
	Ref fnClass = GetArraySlotRef(fn, kFunctionClassIndex);
	if (fnClass == kPlainFuncClass)
		argCount = RVALUE(((ArrayObject *)ObjectPtr(fn))->slot[kFunctionNumArgsIndex]) & 0xFFFF;
	else if (fnClass == kPlainCFunctionClass)
		argCount = RVALUE(((ArrayObject *)ObjectPtr(fn))->slot[kPlainCFunctionNumArgsIndex]);
	else if (fnClass == kBinCFunctionClass)
		argCount = RINT(GetArraySlotRef(fn, kCFunctionNumArgsIndex));
	else if (EQRef(fnClass, RSYMCodeBlock))
		argCount = RINT(GetArraySlotRef(fn, kFunctionNumArgsIndex));
	else if (EQRef(fnClass, RSYMBinCFunction))
		argCount = RINT(GetFrameSlot(fn, SYMA(numArgs)));

	return argCount;
}


/*------------------------------------------------------------------------------
	Push function arguments.
------------------------------------------------------------------------------*/

ArrayIndex
PushArgArray(Ref args)
{
	ArrayIndex numArgs = 0;
	if (NOTNIL(args))
	{
		Ref * slot = Slots(args);
		numArgs = Length(args);
		for (ArrayIndex i = 0; i < numArgs; ++i)
			gInterpreter->pushValue(slot[i]);
	}
	return numArgs;
}


/*------------------------------------------------------------------------------
	Execute a Newton Script.
------------------------------------------------------------------------------*/

Ref
DoScript(RefArg rcvr, RefArg script, RefArg args)
{
	VMState * originalState;
	ArrayIndex numArgs = 0;

	if (!IsFunction(script))
		ThrowBadTypeWithFrameData(kNSErrNotAFunction, script);

	// push args onto stack
	if (NOTNIL(args))
	{
		numArgs = Length(args);
		for (ArrayIndex i = 0; i < numArgs; ++i)
			gInterpreter->pushValue(GetArraySlot(args, i));
	}

	// trace script entry
	if (gInterpreter->tracing)
		gInterpreter->traceSend(script, RA(NILREF), numArgs, 0);

	originalState = gInterpreter->vm;
	newton_try
	{
		if (gInterpreter->send(rcvr, rcvr, script, numArgs) == kNSFunction)
			gInterpreter->run();
	}
	cleanup
	{
		while (gInterpreter->vm != originalState)
			gInterpreter->vm = gInterpreter->ctrlStack.pop();
	}
	end_try;

	// trace return -- not original
	if (gInterpreter->tracing)
		gInterpreter->traceReturn(YES);

	return gInterpreter->popValue();
}


/*------------------------------------------------------------------------------
	Execute a code block.
------------------------------------------------------------------------------*/

Ref
DoBlock(RefArg codeBlock, RefArg args)
{
	if (!IsFunction(codeBlock))
		ThrowBadTypeWithFrameData(kNSErrNotAFunction, codeBlock);

	return DoCall(codeBlock, PushArgArray(args));
}


/*------------------------------------------------------------------------------
	Interpret a code block.
	Called by REPAcceptLine() when it has compiled some input to interpret.
------------------------------------------------------------------------------*/

Ref
InterpretBlock(RefArg codeBlock, RefArg args)
{
	gInterpreter->topLevelCall(codeBlock, args);
	gInterpreter->run();
	return gInterpreter->popValue();
}


/*------------------------------------------------------------------------------
	Send a message.
	Find implementor first: throw exception if not defined.
------------------------------------------------------------------------------*/

Ref
DoMessage(RefArg rcvr, RefArg msg, RefArg args)
{
	Ref result;

	if (!IsSymbol(msg))
		ThrowBadTypeWithFrameData(kNSErrNotASymbol, msg);

	RefVar	impl(FindImplementor(rcvr, msg));
	result = DoSend(rcvr, impl, msg, PushArgArray(args));

	return result;
}


Ref
DoProtoMessage(RefArg rcvr, RefArg msg, RefArg args)
{
	Ref result;

	if (!IsSymbol(msg))
		ThrowBadTypeWithFrameData(kNSErrNotASymbol, msg);

	RefVar	impl(FindProtoImplementor(rcvr, msg));
	result = DoSend(rcvr, impl, msg, PushArgArray(args));

	return result;
}


/*------------------------------------------------------------------------------
	Send a message.
	Find implementor first: ignore it if not defined.
------------------------------------------------------------------------------*/

Ref
DoMessageIfDefined(RefArg rcvr, RefArg msg, RefArg args, bool * isDefined)
{
	Ref result;

	if (!IsSymbol(msg))
		ThrowBadTypeWithFrameData(kNSErrNotASymbol, msg);

	RefVar	impl(FindImplementor(rcvr, msg));
	if (ISNIL(impl))
	{
		if (isDefined)
			*isDefined = NO;
		result = NILREF;
	}
	else
	{
		if (isDefined)
			*isDefined = YES;
		result = DoSend(rcvr, impl, msg, PushArgArray(args));
	}

	return result;
}


Ref
DoProtoMessageIfDefined(RefArg rcvr, RefArg msg, RefArg args, bool * isDefined)
{
	Ref result;

	if (!IsSymbol(msg))
		ThrowBadTypeWithFrameData(kNSErrNotASymbol, msg);

	RefVar	impl(FindProtoImplementor(rcvr, msg));
	if (ISNIL(impl))
	{
		if (isDefined)
			*isDefined = NO;
		result = NILREF;
	}
	else
	{
		if (isDefined)
			*isDefined = YES;
		result = DoSend(rcvr, impl, msg, PushArgArray(args));
	}

	return result;
}


/*------------------------------------------------------------------------------
	Send a message to a known implementor: throw exception if not defined.
------------------------------------------------------------------------------*/

Ref
DoSend(RefArg rcvr, RefArg impl, RefArg msg, ArrayIndex numArgs)
{
	VMState * originalState;

	if (gInterpreter->tracing)
		gInterpreter->traceSend(rcvr, msg, numArgs, 0);

	if (ISNIL(impl))
		ThrowExInterpreterWithSymbol(kNSErrUndefinedMethod, msg);

	originalState = gInterpreter->vm;
	newton_try
	{
		RefVar method(GetFrameSlot(impl, msg));
		if (gInterpreter->send(rcvr, impl, method, numArgs) == kNSFunction)
			gInterpreter->run();
	}
	cleanup
	{
		while (gInterpreter->vm != originalState)
			gInterpreter->vm = gInterpreter->ctrlStack.pop();
	}
	end_try;

	// trace return -- not original
	if (gInterpreter->tracing)
		gInterpreter->traceReturn();

	return gInterpreter->popValue();
}


/*------------------------------------------------------------------------------
	Call a NS function.
------------------------------------------------------------------------------*/

Ref
DoCall(RefArg codeBlock, ArrayIndex numArgs)
{
	VMState * originalState;

	if (!IsFunction(codeBlock))
		ThrowBadTypeWithFrameData(kNSErrNotAFunction, codeBlock);

	if (gInterpreter->tracing)
		gInterpreter->traceApply(codeBlock, numArgs);

	originalState = gInterpreter->vm;
	newton_try
	{
		if (gInterpreter->call(codeBlock, numArgs) == kNSFunction)
			gInterpreter->run();
	}
	cleanup
	{
		while (gInterpreter->vm != originalState)
			gInterpreter->vm = gInterpreter->ctrlStack.pop();
	}
	end_try;

	// trace return -- not original
	if (gInterpreter->tracing)
		gInterpreter->traceReturn();

	return gInterpreter->popValue();
}

#pragma mark -

/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
FGetFunctionArgCount(RefArg inRcvr, RefArg inFn)
{
	return MAKEINT(GetFunctionArgCount(inFn));
}


Ref
FApply(RefArg inRcvr, RefArg inFn, RefArg inArgs)
{
	if (!ISPTR(inFn))
		ThrowBadTypeWithFrameData(kNSErrNotAFunction, inFn);
	return DoBlock(inFn, inArgs);
}


Ref
FPerform(RefArg inRcvr, RefArg inRx, RefArg inMsg, RefArg inArgs)
{
	return DoMessage(inRx, inMsg, inArgs);
}


Ref
FPerformIfDefined(RefArg inRcvr, RefArg inRx, RefArg inMsg, RefArg inArgs)
{
	return DoMessageIfDefined(inRx, inMsg, inArgs, NULL);
}


Ref
FProtoPerform(RefArg inRcvr, RefArg inRx, RefArg inMsg, RefArg inArgs)
{
	return DoProtoMessage(inRx, inMsg, inArgs);
}


Ref
FProtoPerformIfDefined(RefArg inRcvr, RefArg inRx, RefArg inMsg, RefArg inArgs)
{
	return DoProtoMessageIfDefined(inRx, inMsg, inArgs, NULL);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C I n t e r p r e t e r
--------------------------------------------------------------------------------
	Constructor.
------------------------------------------------------------------------------*/

CInterpreter::CInterpreter()
{
// turn off tracing
	fTraceMethodDepth = 0;
	traceIndent = 0;
	tracing = kTraceNothing;
	isTraceGetEnabled = NO;
	isTraceFuncEnabled = NO;
	exceptionStackIndex = 0;

// thread this intepreter instance into the VM environment
	next = gInterpreterList;
	gInterpreterList = this;

// set up a new VM state
	vm = ctrlStack.push();
	vm->pc = MAKEINT(0);
}


/*------------------------------------------------------------------------------
	Destructor.
------------------------------------------------------------------------------*/

CInterpreter::~CInterpreter()
{ }


/*------------------------------------------------------------------------------
	Set interpreter flags.
------------------------------------------------------------------------------*/

void
CInterpreter::setFlags()
{
	int				stackInfo;
	ArrayObject *	fnFrame = ((ArrayObject *)ObjectPtr(vm->func));

	instructions = fnFrame->slot[kFunctionInstructionsIndex];
	literals = fnFrame->slot[kFunctionLiteralsIndex];
	isReadOnly = (ObjectFlags(instructions) & (kObjReadOnly | kObjLocked));
	stackInfo = RVALUE(vm->stackFrame);
	if ((stackInfo & kStackFrameFlagMask) & kStackFrameLocals)
	{
		localsIndex = stackInfo >> kStackFrameFlagBits;
		is2x = YES;
	}
	else
		is2x = NO;
}


static void
GetStackState(StackState * state)
{
	CInterpreter * self = gInterpreter;

	ArrayIndex	ctrlStackIndex = STACKINDEX(self->ctrlStack);
	ArrayIndex	dataStackIndex = STACKINDEX(self->dataStack);
	RefStruct	exceptionContext(self->exceptionContext);
	ArrayIndex	traceIndent = self->traceIndent;

	if (state == NULL)
		state = (StackState *)malloc(sizeof(StackState));

	if (state != NULL)
	{
		state->ctrlStackIndex = ctrlStackIndex;
		state->dataStackIndex = dataStackIndex;
		state->exceptionContext = exceptionContext;
		state->traceIndent = traceIndent;
	}
}


/*------------------------------------------------------------------------------
	Reset the interpreter stack.
	Args:		ioState
	Return:  --
------------------------------------------------------------------------------*/

static void
ResetStack(const StackState & ioState)
{
	CInterpreter * self = gInterpreter;

	self->ctrlStack.reset(ioState.ctrlStackIndex);
	self->dataStack.reset(ioState.dataStackIndex);
	self->exceptionContext = ioState.exceptionContext;
	self->traceIndent = ioState.traceIndent;
}


/*------------------------------------------------------------------------------
	Interpret.
------------------------------------------------------------------------------*/

void
CInterpreter::run()
{
	traceSetOptions();

	StackState stackState;
	GetStackState(&stackState);

	ArrayIndex initialStackDepth = STACKINDEX(ctrlStack);

	do {
		newton_try
		{
			IncrementCurrentStackPos();
			run1(initialStackDepth);
			DecrementCurrentStackPos();
		}
		newton_catch(exRootException)
		{
			DecrementCurrentStackPos();
			ClearRefHandles();
			if (!handleException(CurrentException(), initialStackDepth, stackState))
				rethrow;
		}
		end_try;
	} while (STACKINDEX(ctrlStack) >= initialStackDepth);
}

/*------------------------------------------------------------------------------
	And this is where our story really starts…
------------------------------------------------------------------------------*/

void
CInterpreter::run1(ArrayIndex initialStackDepth)
{
	RefVar	var1;
	RefVar	var2;
	RefVar	var3;
	RefVar	var4;
	Ref *		localSlot;
	Ref *		literalSlot;
	unsigned char *	instrBase, * instrPtr;
	unsigned char	a;
	int		b;
	int		fnType;
	bool		exists;

	for ( ; ; )
	{
		instrBase = (unsigned char *)BinaryData(instructions);
		instrPtr = instrBase + instructionOffset;
		literalSlot = NOTNIL(literals) ? ((FrameObject *)ObjectPtr(literals))->slot : NULL;
		if (is2x)
			localSlot = dataStack.base + localsIndex;
		else
			localSlot = ((FrameObject *)ObjectPtr(vm->locals))->slot;

		for ( ; ; )
		{
			a = *instrPtr++;
			b = a & 0x07;

			switch (a)
			{
		/*------------------------------
			pop
			x --
		------------------------------*/
			case 000:
				dataStack.top--;
				break;

		/*------------------------------
			dup
			x -- x x
		------------------------------*/
			case 001:
				*dataStack.top++ = *(dataStack.top-1);
				break;

		/*------------------------------
			return
			--
		------------------------------*/
			case 002:
				// unwind stack and leave return value on top
				var1 = *(dataStack.top-1);
				dataStack.top = dataStack.base + (RVALUE(vm->stackFrame) >> kStackFrameFlagBits) + 3 + 1;	// ••
				*(dataStack.top-1) = var1;
				// restore VM state of caller
				vm = ctrlStack.pop();
				instructionOffset = RVALUE(vm->pc);
				// if we had any exception handlers, pop them
				if (STACKINDEX(ctrlStack) < exceptionStackIndex)
					popHandlers();
				if (NOTNIL(vm->func) && instructionOffset != -1)
					setFlags();
				fnType = kCFunction; // not really, but we want the stack check to be made
				goto bailCheck;
				break;

		/*------------------------------
			push-self
			-- RCVR
		------------------------------*/
			case 003:
				*dataStack.top++ = vm->rcvr;
				break;

		/*------------------------------
			set-lex-scope
			func -- closure
		------------------------------*/
			case 004:
				{
					Ref	argFrame, * argSlot;

					var1 = *(dataStack.top-1);	// func
					var1 = Clone(var1);

					var2 = ((ArrayObject *)ObjectPtr(var1))->slot[kFunctionArgFrameIndex];
					argFrame = Clone(var2);

					argSlot = ((ArrayObject *)ObjectPtr(argFrame))->slot;
					argSlot[kArgFrameNextArgFrameIndex] = vm->locals;

					if (argSlot[kArgFrameParentIndex] == INVALIDPTRREF)
						argSlot[kArgFrameParentIndex] = NILREF;
					else
						argSlot[kArgFrameParentIndex] = vm->rcvr;
					if (argSlot[kArgFrameImplementorIndex] == INVALIDPTRREF)
						argSlot[kArgFrameImplementorIndex] = NILREF;
					else
						argSlot[kArgFrameImplementorIndex] = vm->impl;
					if (Length(argFrame) == 3	// then no args/locals
					&& ISNIL(argSlot[kArgFrameNextArgFrameIndex])
					&& ISNIL(argSlot[kArgFrameParentIndex])
					&& ISNIL(argSlot[kArgFrameImplementorIndex]))
						argFrame = NILREF;

					((ArrayObject *)ObjectPtr(var1))->slot[kFunctionArgFrameIndex] = argFrame;

					*(dataStack.top-1) = var1;	// closure
				}
				break;

		/*------------------------------
			iter-next
			iterator --
		------------------------------*/
			case 005:
				ForEachLoopNext(*(dataStack.top-1));
				dataStack.top--;
				break;

		/*------------------------------
			iter-done
			iterator -- done?
		------------------------------*/
			case 006:
				*(dataStack.top-1) = MAKEBOOLEAN(ForEachLoopDone(*(dataStack.top-1)));
				break;

		/*------------------------------
			pop-handlers
			--
		------------------------------*/
			case 007:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
				if (b == 0x07)
					exceptionContext = GetArraySlot(exceptionContext, 0);
				else
					ThrowErr(exInterpreter, kNSErrUndefinedBytecode);
				break;

		/*------------------------------
			push
			-- literal
		------------------------------*/
			case 037:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 030:
			case 031:
			case 032:
			case 033:
			case 034:
			case 035:
			case 036:
				*dataStack.top++ = literalSlot[b];
				break;

		/*------------------------------
			push-constant
			-- value
			This is the ONLY time b is signed.
		------------------------------*/
			case 047:
				b = *(char *)instrPtr++ << 8;
				b += *instrPtr++;
			case 040:
			case 041:
			case 042:
			case 043:
			case 044:
			case 045:
			case 046:
				*dataStack.top++ = b;
				break;

		/*------------------------------
			call
			arg1 arg2 ... argn name -- result
		------------------------------*/
			case 057:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 050:
			case 051:
			case 052:
			case 053:
			case 054:
			case 055:
			case 056:
				var1 = *--(dataStack.top); // name
				var2 = UnsafeGetFrameSlot(gFunctionFrame, var1, &exists);  // function object
				if (exists)
				{
					instructionOffset = instrPtr - instrBase;
					fnType = unsafeDoCall(var2, b);
					goto bailCheck;
				}
				else
					ThrowExInterpreterWithSymbol(kNSErrUndefinedGlobalFunction, var1);
				break;

		/*------------------------------
			invoke
			arg1 arg2 ... argn func -- result
		------------------------------*/
			case 067:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 060:
			case 061:
			case 062:
			case 063:
			case 064:
			case 065:
			case 066:
				var1 =  *--(dataStack.top);	// func
				instructionOffset = instrPtr - instrBase;
				fnType = unsafeDoCall(var1, b);
				goto bailCheck;
				break;

		/*------------------------------
			send
			arg1 arg2 ... argn receiver name -- result
		------------------------------*/
			case 077:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 070:
			case 071:
			case 072:
			case 073:
			case 074:
			case 075:
			case 076:
				var2 = *--(dataStack.top);	// name
				var1 = *--(dataStack.top);	// receiver
				if (XFindImplementor(var1, var2, &var3, &var4))
				{
					instructionOffset = instrPtr - instrBase;
					fnType = unsafeDoSend(var1, var3, var4, b);
					goto bailCheck;
				}
				else
					ThrowExInterpreterWithSymbol(kNSErrUndefinedMethod, var1);
				break;

		/*------------------------------
			send-if-defined
			arg1 arg2 ... argn receiver name -- result
		------------------------------*/
			case 0107:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0100:
			case 0101:
			case 0102:
			case 0103:
			case 0104:
			case 0105:
			case 0106:
				var2 = *--(dataStack.top);	// name
				var1 = *--(dataStack.top);	// receiver
				
				if (XFindImplementor(var1, var2, &var3, &var4))
				{
					instructionOffset = instrPtr - instrBase;
					fnType = unsafeDoSend(var1, var3, var4, b);
					goto bailCheck;
				}
				else
				{
					dataStack.top -= b;
					*(dataStack.top - 1) = NILREF;
				}
				break;

		/*------------------------------
			resend
			arg1 arg2 ... argn name -- result
		------------------------------*/
			case 0117:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0110:
			case 0111:
			case 0112:
			case 0113:
			case 0114:
			case 0115:
			case 0116:
				var1 = *--(dataStack.top);	// name
				
				if (XFindProtoImplementor(vm->impl, var1, &var2, &var3))
				{
					instructionOffset = instrPtr - instrBase;
					fnType = unsafeDoSend(vm->rcvr, var2, var3, b);
					goto bailCheck;
				}
				else
					ThrowExInterpreterWithSymbol(kNSErrUndefinedMethod, var1);
				break;

		/*------------------------------
			resend-if-defined
			arg1 arg2 ... argn name -- result
		------------------------------*/
			case 0127:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0120:
			case 0121:
			case 0122:
			case 0123:
			case 0124:
			case 0125:
			case 0126:
				var1 = *--(dataStack.top);	// name
				
				if (XFindProtoImplementor(vm->impl, var1, &var2, &var3))
				{
					instructionOffset = instrPtr - instrBase;
					fnType = unsafeDoSend(vm->rcvr, var2, var3, b);
					goto bailCheck;
				}
				else
				{
					dataStack.top -= b;
					*(dataStack.top - 1) = NILREF;
				}
				break;

		/*------------------------------
			branch
			--
		------------------------------*/
			case 0137:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0130:
			case 0131:
			case 0132:
			case 0133:
			case 0134:
			case 0135:
			case 0136:
				instrPtr = instrBase + b;
				break;

		/*------------------------------
			branch-if-true
			value --
		------------------------------*/
			case 0147:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0140:
			case 0141:
			case 0142:
			case 0143:
			case 0144:
			case 0145:
			case 0146:
				if (ISTRUE(*--(dataStack.top)))
					instrPtr = instrBase + b;
				break;

		/*------------------------------
			branch-if-false
			value --
		------------------------------*/
			case 0157:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0150:
			case 0151:
			case 0152:
			case 0153:
			case 0154:
			case 0155:
			case 0156:
				if (ISFALSE(*--(dataStack.top)))
					instrPtr = instrBase + b;
				break;

		/*------------------------------
			find-var
			-- result
			get name from literals then look up variable in
			1	lexical environment
			2	receiver
			3	globals
		------------------------------*/
			case 0167:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0160:
			case 0161:
			case 0162:
			case 0163:
			case 0164:
			case 0165:
			case 0166:
				{
					Ref	context, result;
					LookupType	lookup;

					var1 = literalSlot[b];
					context = vm->locals;
					lookup = kLexicalLookup;

					if (ISNIL(context))
					{
						context = vm->rcvr;
						lookup = kNoLookup;
					}
	
					result = XGetVariable(context, var1, &exists, lookup);
					if (!exists)
						result = UnsafeGetFrameSlot(gVarFrame, var1, &exists);
					if (exists)
						*dataStack.top++ = result;
					else
						ThrowExInterpreterWithSymbol(kNSErrUndefinedVariable, var1);
				}
				break;

		/*------------------------------
			get-var
			-- result
		------------------------------*/
			case 0177:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0170:
			case 0171:
			case 0172:
			case 0173:
			case 0174:
			case 0175:
			case 0176:
				*dataStack.top++ = localSlot[b];
				break;

		/*------------------------------
			make-frame
			val1 val2 ... valn map -- frame
		------------------------------*/
			case 0207:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0200:
			case 0201:
			case 0202:
			case 0203:
			case 0204:
			case 0205:
			case 0206:
				var1 = *--(dataStack.top);	// map
				{
					Ref	frm = AllocateFrameWithMap(var1);
					Ref *	sp = dataStack.top - b;
					Ref *	fp = ((FrameObject *)ObjectPtr(frm))->slot;
					for (ArrayIndex i = 0; i < b; ++i)
						*fp++ = *sp++;
					dataStack.top -= b;
					*dataStack.top++ = frm;
				}
				break;

		/*------------------------------
			make-array
			b == 0xFFFF		size class -- array
			b <  0xFFFF		val1 val2 ... valn class -- array
		------------------------------*/
			case 0217:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0210:
			case 0211:
			case 0212:
			case 0213:
			case 0214:
			case 0215:
			case 0216:
				var1 = *--(dataStack.top);	// class
				if (b == 0xFFFF)
				{
					b = RINT(*(dataStack.top-1));	// size
					*(dataStack.top-1) = AllocateArray(var1, b);
				}
				else
				{
					Ref	ary = AllocateArray(var1, b);
					Ref *	sp = dataStack.top - b;
					Ref *	fp = ((ArrayObject *)ObjectPtr(ary))->slot;
					for (ArrayIndex i = 0; i < b; ++i)
						*fp++ = *sp++;
					dataStack.top -= b;
					*dataStack.top++ = ary;
				}
				break;

		/*------------------------------
			get-path (nil object allowed)
			object pathExpr -- value
		------------------------------*/
			case 0220:
				var2 = *--(dataStack.top);	// pathExpr
				var1 = *--(dataStack.top);	// object
				if (ISNIL(var1))
					*dataStack.top++ = NILREF;
				else
					*dataStack.top++ = GetFramePath(var1, var2);
				break;

		/*------------------------------
			get-path (nil object throws exception)
			object pathExpr -- value
		------------------------------*/
			case 0221:
				var2 = *--(dataStack.top);	// pathExpr
				var1 = *--(dataStack.top);	// object
				if (ISNIL(var1))
					ThrowExFramesWithBadValue(kNSErrPathFailed, var2);
				*dataStack.top++ = GetFramePath(var1, var2);
				break;

		/*------------------------------
			set-path (don’t push value)
			object pathExpr value --
		------------------------------*/
			case 0230:
				var3 = *--(dataStack.top);
				var2 = *--(dataStack.top);
				var1 = *--(dataStack.top);
				SetFramePath(var1, var2, var3);
				break;

		/*------------------------------
			set-path (and push value)
			object pathExpr value -- value
		------------------------------*/
			case 0231:
				var3 = *--(dataStack.top);
				var2 = *--(dataStack.top);
				var1 = *--(dataStack.top);
				SetFramePath(var1, var2, var3);
				*dataStack.top++ = var3;
				break;

		/*------------------------------
			set-var
			value --
		------------------------------*/
			case 0247:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0240:
			case 0241:
			case 0242:
			case 0243:
			case 0244:
			case 0245:
			case 0246:
				localSlot[b] = *--(dataStack.top);
				break;

		/*------------------------------
			find-and-set-var
			value --
		------------------------------*/
			case 0257:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0250:
			case 0251:
			case 0252:
			case 0253:
			case 0254:
			case 0255:
			case 0256:
				var1 = literalSlot[b];		// name
				var2 = *--(dataStack.top);	// value
				if (NOTNIL(vm->locals))
				{
					SetVariableOrGlobal(vm->locals, var1, var2, ((RVALUE(vm->stackFrame) & 0x3F) & 0x02) ? kLexicalLookup | kGlobalLookup | kSetGlobalLookup : kLexicalLookup | kGlobalLookup | kSetSelfLookup);
				}
				else
				{
					if (!SetVariableOrGlobal(vm->rcvr, var1, var2, ((RVALUE(vm->stackFrame) & 0x3F) & 0x02) ? kSetGlobalLookup | kGlobalLookup : kGlobalLookup))
					{
						var3 = Clone(RA(canonicalFakeContext));
						((FrameObject *)ObjectPtr(var3))->slot[1] = vm->rcvr;	// _parent
						SetFrameSlot(var3, var1, var2);
						vm->locals = var3;
					}
				}
				break;

		/*------------------------------
			incr-var
			addend -- addend value
		------------------------------*/
			case 0267:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0260:
			case 0261:
			case 0262:
			case 0263:
			case 0264:
			case 0265:
			case 0266:
				localSlot[b] = MAKEINT(RINT(localSlot[b]) + RINT(*(dataStack.top-1)));
				*dataStack.top++ = localSlot[b];
				break;

		/*------------------------------
			branch-if-loop-not-done
			incr index limit --
		------------------------------*/
			case 0277:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0270:
			case 0271:
			case 0272:
			case 0273:
			case 0274:
			case 0275:
			case 0276:
				{
					int	limit = RINT(*--(dataStack.top));
					int	index = RINT(*--(dataStack.top));
					int	incr = RINT(*--(dataStack.top));
					if ((incr > 0 && index <= limit)
					|| (incr < 0 && index >= limit))
						instrPtr = instrBase + b;
					else if (incr == 0)
						ThrowErr(exInterpreter, kNSErrZeroForLoopIncr);
				}
				break;

		/*------------------------------
			freq-func add
			num1 num2 -- result
		------------------------------*/
			case 0300:
				{
					Ref	r2 = *--(dataStack.top);
					Ref	r1 = *--(dataStack.top);
					if (ISINT(r1) && ISINT(r2))
						*dataStack.top++ = r1 + r2;
					else
					{
						var2 = r2;
						var1 = r1;
						*dataStack.top++ = NumberAdd(var1, var2);
					}
				}
				break;

		/*------------------------------
			freq-func subtract
			num1 num2 -- result
		------------------------------*/
			case 0301:
				{
					Ref r2 = *--(dataStack.top);
					Ref r1 = *--(dataStack.top);
					if (ISINT(r1) && ISINT(r2))
						*dataStack.top++ = r1 - r2;
					else
					{
						var2 = r2;
						var1 = r1;
						*dataStack.top++ = NumberSubtract(var1, var2);
					}
				}
				break;

		/*------------------------------
			freq-func aref
			object index -- element
		------------------------------*/
			case 0302:
				{
					Ref	r2 = *--(dataStack.top);	// index
					Ref	r1 = *(dataStack.top-1);	// object
					int	index = RINT(r2);
					ArrayObject *	obj = (ArrayObject *)ObjectPtr(r1);
					var1 = r1;
					if (ISARRAY(obj))
					{
						if (index < 0 || index >= ARRAYLENGTH(obj))
							ThrowOutOfBoundsException(var1, index);
						*(dataStack.top-1) = obj->slot[index];
					}
					else
					{
						Ref	rClass = ((StringObject *)ObjectPtr(r1))->objClass;
						if ((obj->flags & kObjSlotted) == 0 && (EQRef(rClass, RSYMstring) || IsSubclass(rClass, RSYMstring)))
						{
							CRichString	rstr(var1);
							if (index < 0 || index >= rstr.length())
								ThrowOutOfBoundsException(var1, index);
							*(dataStack.top-1) = MAKECHAR(rstr.getChar(index));
						}
						else
							ThrowBadTypeWithFrameData(kNSErrNotAnArrayOrString, var1);
					}
				}
				break;

		/*------------------------------
			freq-func set-aref
			object index element -- element
		------------------------------*/
			case 0303:
				{
					int	index;
					var2 = *--(dataStack.top);				// element
					index = RINT(*--(dataStack.top));	// index
					var1 = *(dataStack.top-1);				// object
					ArrayObject *	obj = (ArrayObject *)ObjectPtr(var1);
					if ((obj->flags & (kObjMask + kObjReadOnly)) == kArrayObject)
					{
						if (index < 0 || index >= ARRAYLENGTH(obj))
							ThrowOutOfBoundsException(var1, index);
						*(dataStack.top-1) = obj->slot[index] = var2;
					}
					else
					{
						Ref	rClass = ((StringObject *)ObjectPtr(var1))->objClass;
						if ((obj->flags & kObjSlotted) == 0
						 && (EQRef(rClass, RSYMstring) || IsSubclass(rClass, RSYMstring)))
						{
							UniChar		ch;
							CRichString	rstr(var1);
							if (index < 0 || index >= rstr.length())
								ThrowOutOfBoundsException(var1, index);
							ch = RCHAR(var2);
							if (ch == kEndOfString
							 || ch == kInkChar)
								ThrowExInterpreterWithSymbol(kNSErrBadCharForString, var2);

							rstr.setChar(index, ch);
							*(dataStack.top - 1) = MAKECHAR(ch);
						}
						else
							ThrowBadTypeWithFrameData(kNSErrNotAnArrayOrString, var1);
					}
				}
				break;

		/*------------------------------
			freq-func equals
			obj1 obj2 -- result
		------------------------------*/
			case 0304:
				{
					Ref r2 = *--(dataStack.top);
					Ref r1 = *--(dataStack.top);
					bool	result;
					if (!((r1 | r2) & kTagPointer))
						result = (r1 == r2);
					else
					{
						bool	isVar1Real = IsReal(r1);
						bool	isVar2Real = IsReal(r2);
						if (isVar1Real || isVar2Real)
						{
							if (isVar1Real && isVar2Real)
								result = (CDouble(r1) == CDouble(r2));
							else
								result = (CoerceToDouble(r1) == CoerceToDouble(r2));
						}
						else
							result = EQRef(r1, r2);
					}
					*dataStack.top++ = MAKEBOOLEAN(result);
				}
				break;

		/*------------------------------
			freq-func not
			value -- result
		------------------------------*/
			case 0305:
				*(dataStack.top-1) = MAKEBOOLEAN(ISNIL(*(dataStack.top-1)));
				break;

		/*------------------------------
			freq-func not-equals
			obj1 obj2 -- result
		------------------------------*/
			case 0306:
				{
					Ref	r2 = *--(dataStack.top);
					Ref	r1 = *--(dataStack.top);
					bool	result;
					if (!((r1 | r2) & kTagPointer))
						result = (r1 != r2);
					else
					{
						bool	isVar1Real = IsReal(r1);
						bool	isVar2Real = IsReal(r2);
						if (isVar1Real || isVar2Real)
						{
							if (isVar1Real && isVar2Real)
								result = (CDouble(r1) != CDouble(r2));
							else
								result = (CoerceToDouble(r1) != CoerceToDouble(r2));
						}
						else
							result = !EQRef(r1, r2);
					}
					*dataStack.top++ = MAKEBOOLEAN(result);
				}
				break;

		/*------------------------------
			freq-func
			arg1 arg2 ... argn -- result
		------------------------------*/
			case 0307:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
				switch (b)
				{
				case 7:	// mul
					if (ISINT(dataStack.top[-1]) && ISINT(dataStack.top[-2]))
					{
						*(dataStack.top-2) = MAKEINT(RVALUE(dataStack.top[-2]) * RVALUE(dataStack.top[-1]));
						dataStack.top--;
					}
					break;

				case 9:	// div
					if (ISINT(dataStack.top[-1]) && ISINT(dataStack.top[-2]))
					{
						*(dataStack.top-2) = MAKEINT(RVALUE(dataStack.top[-2]) / RVALUE(dataStack.top[-1]));
						dataStack.top--;
					}
					break;

				case 10:	// less-than
					if (ISINT(dataStack.top[-1]) && ISINT(dataStack.top[-2]))
					{
						*(dataStack.top-2) = MAKEBOOLEAN(RVALUE(dataStack.top[-2]) < RVALUE(dataStack.top[-1]));
						dataStack.top--;
					}
					break;

				case 11:	// greater-than
					if (ISINT(dataStack.top[-1]) && ISINT(dataStack.top[-2]))
					{
						*(dataStack.top-2) = MAKEBOOLEAN(RVALUE(dataStack.top[-2]) > RVALUE(dataStack.top[-1]));
						dataStack.top--;
					}
					break;

				case 12:	// greater-or-equal
					if (ISINT(dataStack.top[-1]) && ISINT(dataStack.top[-2]))
					{
						*(dataStack.top-2) = MAKEBOOLEAN(RVALUE(dataStack.top[-2]) >= RVALUE(dataStack.top[-1]));
						dataStack.top--;
					}
					break;

				case 13:	// less-or-equal
					if (ISINT(dataStack.top[-1]) && ISINT(dataStack.top[-2]))
					{
						*(dataStack.top-2) = MAKEBOOLEAN(RVALUE(dataStack.top[-2]) <= RVALUE(dataStack.top[-1]));
						dataStack.top--;
					}
					break;

				case 14:	// bit-and
					if (ISINT(dataStack.top[-1]) && ISINT(dataStack.top[-2]))
					{
						*(dataStack.top-2) = MAKEINT(RVALUE(*(dataStack.top-2)) & RVALUE(*(dataStack.top-1)));
						dataStack.top--;
					}
					break;

				case 15:	// bit-or
					if (ISINT(dataStack.top[-1]) && ISINT(dataStack.top[-2]))
					{
						*(dataStack.top-2) = MAKEINT(RVALUE(*(dataStack.top-2)) | RVALUE(*(dataStack.top-1)));
						dataStack.top--;
					}
					break;

				case 16:	// bit-not
					*(dataStack.top-1) = MAKEINT(~RVALUE(*(dataStack.top-1)));
					break;

				default:
					instructionOffset = instrPtr - instrBase;
					fnType = unsafeDoCall(((ArrayObject *)ObjectPtr(gFreqFuncs))->slot[b], gFreqFuncInfo[b].numOfArgs);
					goto bailCheck;
				}
				break;

		/*------------------------------
			new-handlers
			sym1 PC1 sym2 PC2 ... symn PCn --
		------------------------------*/
			case 0317:
				b = *instrPtr++ << 8;
				b += *instrPtr++;
			case 0310:
			case 0311:
			case 0312:
			case 0313:
			case 0314:
			case 0315:
			case 0316:
				{
					Ref *	slot;
					var1 = MakeArray(kExcDataSize);
					var2 = MakeArray(b*2);

					// copy handler sym/PC pairs off the stack
					slot = ((ArrayObject *)ObjectPtr(var2))->slot;
					for (int i = b*2 - 1; i >= 0; i--)
						*slot++ = dataStack.top[-1 - i];

					*dataStack.top = MAKEINT(b*2);	// sic

					// init exception context array
					slot = ((ArrayObject *)ObjectPtr(var1))->slot;
					slot[kExcDataNext] = exceptionContext;
					slot[kExcDataFnStackIndex] = MAKEINT(STACKINDEX(dataStack));
					slot[kExcDataCtrlStackIndex] = MAKEINT(STACKINDEX(ctrlStack));
					slot[kExcDataFn] = vm->func;
					slot[kExcDataRcvr] = vm->rcvr;
					slot[kExcDataImpl] = vm->impl;
					slot[kExcDataLocals] = vm->locals;
					slot[kExcDataHandlers] = var2;	// array of handler sym/PC pairs

					exceptionContext = var1;
					exceptionStackIndex = STACKINDEX(ctrlStack);
				}
				break;

			default:
				ThrowErr(exInterpreter, kNSErrUndefinedBytecode);
				break;
			}
		}
		// The for-loop above never breaks so we get here ONLY by goto
		// after a function call/return. NS functions should keep running
		// through the interpreter with their new VM state.
bailCheck:
		if (fnType == kCFunction
		&& STACKINDEX(ctrlStack) < initialStackDepth)
			break;
	}
}


/*------------------------------------------------------------------------------
	F r e q u e n t   F u n c t i o n s
------------------------------------------------------------------------------*/

extern "C"
Ref
FAref(RefArg inRcvr, RefArg inObj, RefArg index)
{
	ArrayIndex i = RINT(index);

	// object we are indexing into must be an array…
	if ((ObjectFlags(inObj) & kObjMask) == kArrayObject)
		return GetArraySlot(inObj, i);

	// …or a string
	else if (IsString(inObj))
	{
		CRichString str(inObj);
		if (/*i < 0 ||*/ i > str.length())
		{
			RefVar	fr(AllocateFrame());
			SetFrameSlot(fr, SYMA(errorCode), MAKEINT(kNSErrOutOfBounds));
			SetFrameSlot(fr, SYMA(value), inObj);
			SetFrameSlot(fr, SYMA(index), index);
			ThrowRefException(exFramesData, fr);
		}
		return MAKECHAR(str.getChar(i));
	}

	ThrowBadTypeWithFrameData(kNSErrNotAnArrayOrString, inObj);
	return NILREF;	// keep the compiler happy
}


extern "C"
Ref
FSetAref(RefArg inRcvr, RefArg inObj, RefArg index, RefArg inValue)
{
	ArrayIndex i = RINT(index);

	if ((ObjectFlags(inObj) & kObjMask) == kArrayObject)
		SetArraySlot(inObj, i, inValue);

	else if (!IsString(inObj))
		ThrowBadTypeWithFrameData(kNSErrNotAnArrayOrString, inObj);

	CRichString str(inObj);
	if (/*i < 0 ||*/ i > (int)str.length())
	{
		RefVar	fr(AllocateFrame());
		SetFrameSlot(fr, SYMA(errorCode), MAKEINT(kNSErrOutOfBounds));
		SetFrameSlot(fr, SYMA(value), inObj);
		SetFrameSlot(fr, SYMA(index), index);
		ThrowRefException(exFramesData, fr);
	}
	UniChar  ch = RCHAR(inValue);
	if (ch == kEndOfString || ch == kInkChar)
	{
		RefVar	fr(AllocateFrame());
		SetFrameSlot(fr, SYMA(errorCode), MAKEINT(kNSErrBadCharForString));
		SetFrameSlot(fr, SYMA(value), inValue);
		ThrowRefException(exInterpreterData, fr);
	}
	str.setChar(i, ch);
	return inValue;
}


#pragma mark -

/*------------------------------------------------------------------------------
	S t a c k
------------------------------------------------------------------------------*/

void
CInterpreter::pushValue(Ref value)
{
	*(dataStack.top)++ = value;
}


Ref
CInterpreter::popValue(void)
{
	return *(--dataStack.top);
}


Ref
CInterpreter::peekValue(ArrayIndex at)
{
	return dataStack.top[at-1];
}


void
CInterpreter::setValue(ArrayIndex at, Ref value)
{
	dataStack.top[at-1] = value;
}


ArrayIndex
CInterpreter::valuePosition(void)
{
	return STACKINDEX(dataStack);
}


Ref
CInterpreter::peekControl(ArrayIndex at)
{
	return ctrlStack.top[at-1];
}


void
CInterpreter::setControl(ArrayIndex at, Ref value)
{
	ctrlStack.top[at-1] = value;
}


ArrayIndex
CInterpreter::controlPosition(void)
{
	return STACKINDEX(ctrlStack);
}


/*------------------------------------------------------------------------------
	Set a local value.
	Args:		index		VM stack frame index
				tag		name of local or index into function’s literals array
				value		the value to set
	Return:	--
------------------------------------------------------------------------------*/

void
CInterpreter::setLocalOnStack(RefArg index, RefArg tag, RefArg value)
{
	VMState * vms = stackFrameAt(RINT(index));

	if (ISINT(tag))	// SetVar
	{
		ArrayIndex	localIndex = RINT(tag);
		Ref	fn = vms->func;
		Ref	fnClass = ClassOf(fn);
		ArrayIndex	stackIndex;
		if (EQRef(fnClass, RSYM_function))
		{
			if (/*localIndex < 0 ||*/ localIndex >= RINT(GetArraySlotRef(fn, kFunctionNumArgsIndex)))
				ThrowExFramesWithBadValue(kNSErrOutOfRange, tag);
			stackIndex = RVALUE(vms->stackFrame) >> 6;
			dataStack.base[stackIndex + 3] = value;	// ••
		}
		else if (IsNativeFunction(fn))
		{
			if (/*localIndex < 0 ||*/ localIndex >= GetFunctionArgCount(fn))
				ThrowExFramesWithBadValue(kNSErrOutOfRange, tag);
			stackIndex = RVALUE(vms->stackFrame) >> 6;
			dataStack.base[stackIndex] = value;	// ••
		}
		else if (EQRef(fnClass, RSYMCodeBlock))
		{
			Ref locals = vms->locals;
			if (/*localIndex < 0 ||*/ localIndex + 3 >= Length(locals))
				ThrowExFramesWithBadValue(kNSErrOutOfRange, tag);
			SetArraySlotRef(locals, localIndex + 3, value);	// ••
		}
	}
	else	// SetFindVar
	{
		RefVar	locals(vms->locals);
		if (ISNIL(locals) || !SetVariableOrGlobal(locals, tag, value, kLexicalLookup))
			ThrowExInterpreterWithSymbol(kNSErrUndefinedVariable, tag);
	}
}


/*------------------------------------------------------------------------------
	Get a local value.
	Args:		index		VM stack frame index
				tag		name of local or index into function’s literals array
	Return:	Ref		the value
------------------------------------------------------------------------------*/

Ref
CInterpreter::getLocalFromStack(RefArg index, RefArg tag)
{
	Ref		 var = NILREF;
	VMState * vms = stackFrameAt(RINT(index));

	if (ISINT(tag))	// GetVar
	{
		ArrayIndex	localIndex = RINT(tag);
		Ref	fn = vms->func;
		Ref	fnClass = ClassOf(fn);
		ArrayIndex	stackIndex;
		if (EQRef(fnClass, RSYM_function))
		{
			if (/*localIndex < 0 ||*/ localIndex >= RINT(GetArraySlotRef(fn, kFunctionNumArgsIndex)))
				ThrowExFramesWithBadValue(kNSErrOutOfRange, tag);
			stackIndex = RVALUE(vms->stackFrame) >> 6;
			var = dataStack.base[stackIndex + 3];	// ••
		}
		else if (IsNativeFunction(fn))
		{
			if (/*localIndex < 0 ||*/ localIndex >= GetFunctionArgCount(fn))
				ThrowExFramesWithBadValue(kNSErrOutOfRange, tag);
			stackIndex = RVALUE(vms->stackFrame) >> 6;
			var = dataStack.base[stackIndex];	// ••
		}
		else if (EQRef(fnClass, RSYMCodeBlock))
		{
			Ref	locals = vms->locals;
			if (/*localIndex < 0 ||*/ localIndex + 3 >= Length(locals))
				ThrowExFramesWithBadValue(kNSErrOutOfRange, tag);
			var = GetArraySlotRef(locals, localIndex + 3);	// ••
		}
	}
	else	// FindVar
	{
		bool		exists;
		if (ISNIL(vms->locals))
			ThrowExInterpreterWithSymbol(kNSErrUndefinedVariable, tag);
		var = XGetVariable(vms->locals, tag, &exists, kLexicalLookup);
		if (!exists)
			ThrowExInterpreterWithSymbol(kNSErrUndefinedVariable, tag);
	}

	return var;
}


/*------------------------------------------------------------------------------
	Get self.
	Args:		index		VM stack frame index
	Return:	Ref		the receiver
------------------------------------------------------------------------------*/

Ref
CInterpreter::getSelfFromStack(Ref index)
{
	return stackFrameAt(RINT(index))->rcvr;
}


/*------------------------------------------------------------------------------
	Return the size of the VM stack.
	Args:
	Return:	ArrayIndex		the number of VM stack frames
------------------------------------------------------------------------------*/

ArrayIndex
CInterpreter::numStackFrames(void)
{
	return ((ctrlStack.top - ctrlStack.base) / sizeof(VMState)) - 1;
}


/*------------------------------------------------------------------------------
	Get a VM stack frame.
	Args:		index			VM stack frame index
	Return:	VMState *	the VM stack frame
------------------------------------------------------------------------------*/

VMState *
CInterpreter::stackFrameAt(ArrayIndex index)
{
	if (/*index < 0 ||*/ index >= numStackFrames())
		ThrowExFramesWithBadValue(kNSErrOutOfRange, MAKEINT(index));

	return ctrlStack.at(index + 1);	// ••
}

#pragma mark -

/*------------------------------------------------------------------------------
	F u n c t i o n   A c t i v a t i o n
--------------------------------------------------------------------------------
	FastDoSend
------------------------------------------------------------------------------*/

int
CInterpreter::unsafeDoSend(RefArg rcvr, RefArg impl, RefArg fn, ArrayIndex numArgs)
{
	if (ISREALPTR(fn) && ISRO(fn))
	{
		Ref *	funSlot = ((FrameObject *)PTR(fn))->slot;
		Ref	funClass = funSlot[kFunctionClassIndex];
		if (funClass == kPlainFuncClass)
		{
			// save current PC in VM
			vm->pc = MAKEINT(instructionOffset);
			// set up VM
			vm = ctrlStack.push();
			vm->rcvr = rcvr;
			vm->impl = impl;
			vm->func = fn;
			instructionOffset = 0;
			// check number of arguments
			ArrayIndex numArgsExpected = RVALUE(funSlot[kFunctionNumArgsIndex]);
			ArrayIndex numLocals = numArgsExpected >> 16;
			numArgsExpected = numArgsExpected & 0xFFFF;
			if (numArgs != numArgsExpected)
				ThrowErr(exInterpreter, kNSErrWrongNumberOfArgs);

			// set up stack frame for OS2.x function
			int stackFrameIndex = STACKINDEX(dataStack) - numArgs - 2;	// ••
			vm->stackFrame = MAKEINT((stackFrameIndex << kStackFrameFlagBits) | kStackFrameLocals);
			// set up lexical environment
			RefVar	argFrame(funSlot[kFunctionArgFrameIndex]);
			if (NOTNIL(argFrame))
			{
				Ref * argSlot;
				argFrame = Clone(argFrame);
				vm->locals = argFrame;
				argSlot = ((FrameObject *)ObjectPtr(argFrame))->slot;
				argSlot[kArgFrameParentIndex] = (argSlot[kArgFrameParentIndex] != INVALIDPTRREF) ? vm->rcvr : NILREF;
				argSlot[kArgFrameImplementorIndex] = (argSlot[kArgFrameImplementorIndex] != INVALIDPTRREF) ? vm->impl : NILREF;
			}
			// push locals
			if (numLocals != 0)
				dataStack.pushNILs(numLocals);
			// reset PC and return, indicating run-loop continuation
			setFlags();
			return kNSFunction;
		}

		else if (funClass == kPlainCFunctionClass)
		{
			// check number of arguments
			if (numArgs != RVALUE(funSlot[kPlainCFunctionNumArgsIndex]))
				ThrowErr(exInterpreter, kNSErrWrongNumberOfArgs);

			// set up VM
			vm = ctrlStack.push();
			setSendEnv(rcvr, impl);

			// call the function: pop args off the stack and replace with the result
			Ref result = callCFuncPtr((CFunction) funSlot[kCFunctionPtrIndex], numArgs);
			dataStack.top -= ((int)numArgs - 1);
			*(dataStack.top - 1) = result;

			// restore VM and return, indicating new run-loop required
			ctrlStack.pop();
			setFlags();
			return kCFunction;
		}
	}

	// function is not in heap - must be in packages or ROM
	// so call it directly
	return send(rcvr, impl, fn, numArgs);
}


/*------------------------------------------------------------------------------
	FastDoCall
------------------------------------------------------------------------------*/

int
CInterpreter::unsafeDoCall(Ref func, ArrayIndex numArgs)		// yes, really a Ref!
{
	if (ISREALPTR(func) && ISRO(func))
	{
		Ref * funSlot = ((FrameObject *)PTR(func))->slot;
		Ref	funClass = funSlot[kFunctionClassIndex];
		if (funClass == kPlainFuncClass)
		{
			// save current PC in VM
			vm->pc = MAKEINT(instructionOffset);
			// set up VM
			vm = ctrlStack.push();
			vm->func = func;
			instructionOffset = 0;
			// check number of arguments
			ArrayIndex numArgsExpected = RVALUE(funSlot[kFunctionNumArgsIndex]);
			ArrayIndex numLocals = numArgsExpected >> 16;
			numArgsExpected = numArgsExpected & 0xFFFF;
			if (numArgs != numArgsExpected)
				ThrowErr(exInterpreter, kNSErrWrongNumberOfArgs);

			// set up stack frame for OS2.x function
			int stackFrameIndex = STACKINDEX(dataStack) - numArgs - 2;	// ••
			vm->stackFrame = MAKEINT((stackFrameIndex << kStackFrameFlagBits) | kStackFrameLocals);
			// set up lexical environment
			RefVar	argFrame(funSlot[kFunctionArgFrameIndex]);
			if (NOTNIL(argFrame))
			{
				Ref * argSlot;
				argFrame = Clone(argFrame);
				vm->locals = argFrame;
				argSlot = ((FrameObject *)ObjectPtr(argFrame))->slot;
				vm->rcvr = argSlot[kArgFrameNextArgFrameIndex];
				vm->impl = argSlot[kArgFrameImplementorIndex];
			}
			// push locals
			if (numLocals != 0)
				dataStack.pushNILs(numLocals);
			// reset PC and return, indicating run-loop continuation
			setFlags();
			return kNSFunction;
		}

		else if (funClass == kPlainCFunctionClass)
		{
			// check number of arguments
			if (numArgs != RVALUE(funSlot[kPlainCFunctionNumArgsIndex]))
				ThrowErr(exInterpreter, kNSErrWrongNumberOfArgs);

			setCallEnv();
			// call the function: pop args off the stack and replace with the result
			Ref result = callCFuncPtr((CFunction) funSlot[kCFunctionPtrIndex], numArgs);
			dataStack.top -= ((int)numArgs - 1);
			*(dataStack.top - 1) = result;

			// reset PC and return, indicating new run-loop required
			setFlags();
			return kCFunction;
		}
	}

	// function is not in heap - must be in packages or ROM
	// so call it directly
	return call(func, numArgs);
}


/*------------------------------------------------------------------------------
	Send a message to a known implementor.
	Despatch the message depending on the handling function’s class:
		'CodeBlock				1.x
		kPlainFuncClass      2.x
		kPlainCFunctionClass 2.x native
		kBinCFunctionClass   2.x NCT
	or	'BinCFunction

	Args:		rcvr		receiver frame
				impl		implementor frame
				func		function frame
				numArgs	number of arguments on the stack
	Return:	int		type of function executed
					0 =>	NS
					1 =>	C
------------------------------------------------------------------------------*/

int
CInterpreter::send(RefArg rcvr, RefArg impl, RefArg func, ArrayIndex numArgs)
{
// trace message send
	TraceFunction trace;
	if (gFramesFunctionProfilingEnabled == kProfilingEnabled && (trace = gFramesFunctionProfiler) != NULL)
		trace(&vm->func, func, STACKINDEX(ctrlStack), gCurrentStackPos >> 16, 1, gFramesFunctionProfilerData);

	vm->pc = MAKEINT(instructionOffset);

	// set up VM
	vm = ctrlStack.push();
	setSendEnv(rcvr, impl);

	// despatch message depending on function class
	Ref funClass = GetArraySlot(func, 0);
	if (ISFUNCCLASS(funClass))
	{
		switch (FUNCKIND(funClass))
		{
		case kNSFunction:
			callPlainCodeBlock(func, numArgs, 0x01);
			return kNSFunction;
		case kCFunction:
			callPlainCFunction(func, numArgs);
			return kCFunction;
		}
	}

	else if (funClass == kBinCFunctionClass
	|| EQRef(funClass, RSYMBinCFunction))
	{
		// set up environment
		vm->locals = vm->rcvr;
		callCFunction(func, numArgs, funClass != kBinCFunctionClass);
		// restore VM
		vm = ctrlStack.pop();
		instructionOffset = RINT(vm->pc);
		if (NOTNIL(vm->func) && instructionOffset != -1)
			setFlags();
		return kCFunction;
	}

	else if (EQRef(funClass, RSYMCodeBlock))
		callCodeBlock(func, numArgs, 0x01);

	else
		ThrowBadTypeWithFrameData(kNSErrNotAFunction, func);

	return kNSFunction;
}


/*------------------------------------------------------------------------------
	Call a function.
	Similar to Send, but uses existing receiver.

	Args:		func		function frame
				numArgs	number of arguments on the stack
	Return:	int		type of function executed
					0 =>	NS
					1 =>	C
------------------------------------------------------------------------------*/

int
CInterpreter::call(RefArg func, ArrayIndex numArgs)
{
// trace function entry
	TraceFunction trace;
	if (gFramesFunctionProfilingEnabled == kProfilingEnabled && (trace = gFramesFunctionProfiler) != NULL)
		trace(&vm->func, func, STACKINDEX(ctrlStack), gCurrentStackPos >> 16, 0, gFramesFunctionProfilerData);

	vm->pc = MAKEINT(instructionOffset);

	Ref funClass = GetArraySlot(func, kFunctionClassIndex);
	if (ISFUNCCLASS(funClass))
	{
		switch (FUNCKIND(funClass))
		{
		case kNSFunction:
			vm = ctrlStack.push();
			callPlainCodeBlock(func, numArgs, 0x00);
			return kNSFunction;
		case kCFunction:
			VMState * prev = vm;
			vm = ctrlStack.push();
			vm->rcvr = prev->rcvr;
			int stackFrameIndex = STACKINDEX(dataStack) - numArgs + 1;
			vm->stackFrame = MAKEINT((stackFrameIndex << kStackFrameFlagBits) | kStackFrameNoLocals);
			setCallEnv();
			callPlainCFunction(func, numArgs);
			return kCFunction;
		}
	}

	else if (funClass == kBinCFunctionClass
	|| EQRef(funClass, RSYMBinCFunction))
	{
		// set up environment
		setCallEnv();
		vm = ctrlStack.push();
		int stackFrameIndex = STACKINDEX(dataStack) - numArgs + 1;
		vm->stackFrame = MAKEINT((stackFrameIndex << kStackFrameFlagBits) | kStackFrameNoLocals);
		callCFunction(func, numArgs, funClass != kBinCFunctionClass);

		// restore VM
		vm = ctrlStack.pop();
		instructionOffset = RINT(vm->pc);
		if (NOTNIL(vm->func) && instructionOffset != -1)
			setFlags();
		return kCFunction;
	}

	else if (EQRef(funClass, RSYMCodeBlock))
	{
		vm = ctrlStack.push();
		callCodeBlock(func, numArgs, 0x01);
	}

	else
		ThrowBadTypeWithFrameData(kNSErrNotAFunction, func);

	return kNSFunction;
}


/*------------------------------------------------------------------------------
	Call NS function, 1.3 or 2.0 style.

	Args:		func		function frame
				args		arguments array
	Return:	--
------------------------------------------------------------------------------*/

void
CInterpreter::topLevelCall(RefArg func, RefArg args)
{
// trace function entry
	TraceFunction trace;
	if (gFramesFunctionProfilingEnabled == kProfilingEnabled && (trace = gFramesFunctionProfiler) != NULL)
		trace(&vm->func, func, STACKINDEX(ctrlStack), gCurrentStackPos >> 16, 0, gFramesFunctionProfilerData);

	// despatch to appropriate function flavour
	Ref	funClass = GetArraySlot(func, kFunctionClassIndex);
	if (funClass == kPlainFuncClass)
	{
		vm = ctrlStack.push();
		callPlainCodeBlock(func, 0, 0x02);
	}
	else if (EQRef(funClass, RSYMCodeBlock))
	{
		vm = ctrlStack.push();
		callCodeBlock(func, 0, 0x02);
	}
}


/*------------------------------------------------------------------------------
	Call function, class = 'CodeBlock (function in Newton 1.x).

	DEPRECATED -- but used by NTK installer:Install()

	Args:		func		function frame
				numArgs	number of arguments on the stack
				huh		some kind of flags
	Return:	--
------------------------------------------------------------------------------*/

void
CInterpreter::callCodeBlock(RefArg func, ArrayIndex numArgs, unsigned huh)
{
	vm->func = func;
	instructionOffset = 0;

	// check number of args
	ArrayIndex numArgsExpected = RVALUE(GetArraySlot(func, kFunctionNumArgsIndex));
	if (numArgsExpected != numArgs)
		ThrowErr(exInterpreter, kNSErrWrongNumberOfArgs);

	// clone the arg frame
	RefVar	argFrame(GetArraySlot(func, kFunctionArgFrameIndex));
	argFrame = Clone(argFrame);
	Ref * argSlot = ((FrameObject *)ObjectPtr(argFrame))->slot;

	// copy args from stack into the arg frame
	Ref * sp = dataStack.top;
	for (int i = numArgs - 1; i >= 0; i--)
		argSlot[kArgFrameArgIndex + i] = *--sp;

	// set up stack info
	int stackFrameIndex = STACKINDEX(dataStack);
	vm->stackFrame = MAKEINT((stackFrameIndex << kStackFrameFlagBits) | ((huh & 0x02) ? 0x02 : kStackFrameNoLocals));

	// set up vm
	if (huh & 0x01)
	{
		argSlot[kArgFrameParentIndex] = vm->rcvr;
		argSlot[kArgFrameImplementorIndex] = vm->impl;
	}
	else
	{
		vm->rcvr = argSlot[kArgFrameParentIndex];
		vm->impl = argSlot[kArgFrameImplementorIndex];
	}
	vm->locals = argFrame;
	setFlags();
}


/*------------------------------------------------------------------------------
	Call function, class = '_function (newFunc in Newton 2.x).
	Args:		func		function frame
				numArgs	number of arguments on the stack
				huh		some kind of flags
	Return:	--
------------------------------------------------------------------------------*/

void
CInterpreter::callPlainCodeBlock(RefArg func, ArrayIndex numArgs, unsigned huh)
{
	FrameObject *	funcObj = (FrameObject *)NoFaultObjectPtr(func);
	vm->func = func;
	instructionOffset = 0;
	vm->pc = MAKEINT(instructionOffset);

	// check number of args
	ArrayIndex numArgsExpected = RVALUE(funcObj->slot[kFunctionNumArgsIndex]);
	ArrayIndex numLocals = numArgsExpected >> 16;
	numArgsExpected = numArgsExpected & 0xFFFF;
	if (numArgs != numArgsExpected)
		ThrowErr(exInterpreter, kNSErrWrongNumberOfArgs);

	// set up stack info
	int stackFrameIndex = STACKINDEX(dataStack) - numArgs - 2;	// ••
	vm->stackFrame = MAKEINT((stackFrameIndex << kStackFrameFlagBits) | ((huh & 0x02) ? 0x03 : kStackFrameLocals));

	// set up vm
	RefVar	argFrame(funcObj->slot[kFunctionArgFrameIndex]);
	if (NOTNIL(argFrame))
	{
		Ref *		argSlot;
		argFrame = Clone(argFrame);
		argSlot = ((FrameObject *)ObjectPtr(argFrame))->slot;
		if (huh & 0x01)
		{
			argSlot[kArgFrameParentIndex] = (argSlot[kArgFrameParentIndex] != INVALIDPTRREF) ? vm->rcvr : NILREF;
			argSlot[kArgFrameImplementorIndex] = (argSlot[kArgFrameImplementorIndex] != INVALIDPTRREF) ? vm->impl : NILREF;
		}
		else
		{
			vm->rcvr = argSlot[kArgFrameParentIndex];
			vm->impl = argSlot[kArgFrameImplementorIndex];
		}
		vm->locals = argFrame;
	}
	if (numLocals != 0)
		dataStack.pushNILs(numLocals);
	setFlags();
}


/*------------------------------------------------------------------------------
	Call C function, class = kBinCFunctionClass or 'BinCFunction.
	Args:		func			function frame with slots in the following order
					class
					code
					numArgs
					closure
					offset
				numArgs		number of arguments on the stack
				isUnordered	set if func slots may be unordered:
								YES when class = 'BinCFunction
	Return:	--
------------------------------------------------------------------------------*/

void
CInterpreter::callCFunction(RefArg func, ArrayIndex numArgs, bool isUnordered)
{
	CFunction	code;
	ArrayIndex	numArgsExpected, offset;
	Ref			closure;

	// set up VM
	vm->func = func;
	instructionOffset = -1;
	vm->pc = MAKEINT(instructionOffset);

	if (isUnordered)
	{
		numArgsExpected = RINT(GetFrameSlot(func, SYMA(numArgs)));
		closure = GetFrameSlot(func, SYMA(closure));
		offset = RINT(GetFrameSlot(func, SYMA(offset)));
		code = (CFunction) (BinaryData(GetFrameSlot(func, SYMA(code))) + offset);
	}
	else
	{
		numArgsExpected = MAKEINT(((FrameObject *)ObjectPtr(func))->slot[kNativeFunctionNumArgsIndex]);
		closure = ((FrameObject *)ObjectPtr(func))->slot[kNativeFunctionClosureIndex];
		offset = RVALUE(((FrameObject *)ObjectPtr(func))->slot[kNativeFunctionOffsetIndex]);
		code = (CFunction) (BinaryData(((FrameObject *)ObjectPtr(func))->slot[kNativeFunctionCodeIndex]) + offset);
	}

	// check number of args
	if (numArgs != numArgsExpected)
		ThrowErr(exInterpreter, kNSErrWrongNumberOfArgs);

	if (NOTNIL(closure))
		*dataStack.top++ = closure;

	// call the function: pop args off the stack and replace with the result
	Ref result = callCFuncPtr(code, numArgs);
	dataStack.top -= ((int)numArgs - 1);
	*(dataStack.top - 1) = result;

	// trace return -- in original but moved
//	if (tracing)
//		traceReturn();
}


/*------------------------------------------------------------------------------
	Call plain C function, class = kPlainCFunctionClass.
	Args:		func		function frame with slots in the following order
					class
					funcPtr
					numArgs
				numArgs	number of arguments on the stack
	Return:	--
------------------------------------------------------------------------------*/

void
CInterpreter::callPlainCFunction(RefArg func, ArrayIndex numArgs)
{
	// point to function frame object
	FrameObject * funcObj = (FrameObject *)NoFaultObjectPtr(func);

	// set up VM
	vm->func = func;
	instructionOffset = -1;
	vm->pc = MAKEINT(instructionOffset);

	// check number of args
	ArrayIndex numArgsExpected = RVALUE(funcObj->slot[kPlainCFunctionNumArgsIndex]);
	if (numArgs != numArgsExpected)
		ThrowErr(exInterpreter, kNSErrWrongNumberOfArgs);
	
	// call the function: pop args off the stack and replace with the result
	Ref result = callCFuncPtr((CFunction) funcObj->slot[kPlainCFunctionPtrIndex], numArgs);
	dataStack.top -= ((int)numArgs - 1);
	*(dataStack.top - 1) = result;

	// trace return -- in original but moved
//	if (tracing)
//		traceReturn();

	// restore VM state
	vm = ctrlStack.pop();
	instructionOffset = RINT(vm->pc);
	if (NOTNIL(vm->func) && instructionOffset != -1)
		setFlags();

// trace C function return
	TraceFunction trace;
	if (gFramesFunctionProfilingEnabled == kProfilingEnabled && (trace = gFramesFunctionProfiler) != NULL)
		trace(&vm->func, func, STACKINDEX(ctrlStack), gCurrentStackPos >> 16, 2, gFramesFunctionProfilerData);
}


/*------------------------------------------------------------------------------
	Call a C function.  Both NCT and native NS functions end up here.
	Args:		cfunc		C function pointer
				numArgs	number of arguments on the stack
	Return:	--
------------------------------------------------------------------------------*/

Ref
CInterpreter::callCFuncPtr(CFunction cfunc, ArrayIndex numArgs)
{
	Ref			result;
	RefVar		rcvr(vm->rcvr);
	ArrayIndex	stackIndex = dataStack.top - dataStack.base;

	// ensure the stack’s RefStruct pointers are valid
	if ((dataStack.xTop - dataStack.xBase) < stackIndex)
		dataStack.fill();

	// point to RefStruct stack data
	RefStruct * arg = dataStack.xBase + stackIndex - numArgs;

	// despatch to function with right number of args
	switch (numArgs)
	{
	case 0:
		result = cfunc(rcvr);
		break;
	case 1:
		result = cfunc(rcvr, arg);
		break;
	case 2:
		result = cfunc(rcvr, arg, arg+1);
		break;
	case 3:
		result = cfunc(rcvr, arg, arg+1, arg+2);
		break;
	case 4:
		result = cfunc(rcvr, arg, arg+1, arg+2, arg+3);
		break;
	case 5:
		result = cfunc(rcvr, arg, arg+1, arg+2, arg+3, arg+4);
		break;
	case 6:
		result = cfunc(rcvr, arg, arg+1, arg+2, arg+3, arg+4, arg+5);
		break;
	default:
		result = NILREF;	// placate the compiler
		ThrowErr(exInterpreter, kNSErrTooManyArgs);
	}

	return result;
}

#pragma mark -

/*------------------------------------------------------------------------------
	T r a c i n g
	Tracing is enabled by the glopbal var 'trace.
	nil => no tracing
	'functions => trace function calls
	'full => trace var access and functions
	{
		functions:		if defined, trace vars (sic)
		name:				if defined, trace only named function
		contextFrame:	if defined, trace view context frame
		slot:				if defined, trace only named var
	}
------------------------------------------------------------------------------*/

void
CInterpreter::rreturn(FramesProfilingKind profile)
{
	RefVar sp00(vm->func);
	if (tracing)
		traceReturn();
	// INCOMPLETE
	// there’s little point since this is never called -- was used in the slow interpreter loop
}

/*------------------------------------------------------------------------------
	Print stack trace for NewtonScript.
	Args:		inRcvr		ignored
	Return:	nil
------------------------------------------------------------------------------*/

Ref
FStackTrace(RefArg inRcvr)
{
	gInterpreter->stackTrace();
	return NILREF;
}

void
CInterpreter::stackTrace(void)
{
	gREPout->stackTrace(this);
}

void
CInterpreter::taciturnPrintObject(Ref obj, int indent)
{
	if (ISPTR(obj) && !IsSymbol(obj) && !IsReal(obj) && tracing != kTraceEverything)
		REPprintf("#%08X", obj);
	else
		PrintObject(obj, indent);
}

void
CInterpreter::traceSetOptions(void)
{
	RefVar trace(GetFrameSlot(RA(gVarFrame), SYMA(trace)));
	if (ISNIL(trace))
		tracing = kTraceNothing;

	else
	{
		tracing = kTraceVars;
		isTraceGetEnabled = YES;
		isTraceFuncEnabled = YES;
		fTraceMethodName = NILREF;
		fTraceViewContextFrame = NILREF;
		fTraceSlotName = NILREF;
		if (IsSymbol(trace))
		{
			if (EQ(trace, SYMA(functions)))
			{
				tracing = kTraceFunctions;
				isTraceGetEnabled = NO;
			}
			else if (EQ(trace, SYMA(full)))
			{
				tracing = kTraceEverything;
			}
			else
				fTraceMethodName = trace;
		}
		else if (IsFrame(trace))
		{
			if (FrameHasSlot(trace, SYMA(viewCObject)))
			{
				fTraceViewContextFrame = trace;
			}
			else
			{
				if (FrameHasSlot(trace, SYMA(functions)))
					isTraceGetEnabled = NOTNIL(GetFrameSlot(trace, SYMA(functions)));
				if (FrameHasSlot(trace, SYMA(name)))
					fTraceMethodName = GetFrameSlot(trace, SYMA(name));
				if (FrameHasSlot(trace, SYMA(contextFrame)))
					fTraceViewContextFrame = GetFrameSlot(trace, SYMA(contextFrame));
				if (FrameHasSlot(trace, SYMA(slot)))
				{
					RefVar slot(GetFrameSlot(trace, SYMA(slot)));
					if (!EQ(slot, RA(TRUEREF)))
					{
						isTraceGetEnabled = NOTNIL(slot);
						fTraceSlotName = slot;
						isTraceFuncEnabled = NO;
					}
				}
			}
		}
	}
}

void
CInterpreter::traceFreqCall(ArrayIndex index)
{
	FreqFuncDef * def = &gFreqFuncInfo[index];
	traceMethod(RA(NILREF), RA(NILREF), def->name, def->numOfArgs, 0);
}

void
CInterpreter::traceApply(RefArg func, ArrayIndex numArgs)
{
	traceMethod(RA(NILREF), RA(NILREF), "[call with]", numArgs, 0);
}

void
CInterpreter::traceCall(RefArg func, ArrayIndex numArgs)
{
	traceMethod(RA(NILREF), func, SymbolName(func), numArgs, 0);
}

void
CInterpreter::traceSend(RefArg rcvr, RefArg msg, ArrayIndex numArgs, ArrayIndex stackFrameIndex)
{
	traceMethod(rcvr, msg, NOTNIL(msg) ? SymbolName(msg) : "---", numArgs, stackFrameIndex);
}

void
CInterpreter::traceMethod(RefArg rcvr, RefArg msg, const char * name, ArrayIndex numArgs, ArrayIndex stackFrameIndex)
{
	if (NOTNIL(fTraceMethodName))
	{
		if (fTraceMethodDepth != 0 || EQ(fTraceMethodName, msg))
			fTraceMethodDepth++;
		if (fTraceMethodDepth == 0)
			return;
	}
	if (NOTNIL(fTraceViewContextFrame) && (!IsParent(fTraceViewContextFrame, vm->locals) || !IsParent(rcvr, vm->locals)))
		return;

	if (isTraceFuncEnabled)
	{
		int indent = REPprintf("%*s", traceIndent, " ");
		if (NOTNIL(rcvr))
		{
			REPprintf("(");
			taciturnPrintObject(rcvr, indent);
			REPprintf("):");
		}
		REPprintf("%s(", name);
		traceArgs(numArgs, stackFrameIndex, indent);
		REPprintf(")\n");
	}
	traceIndent += 4;
}

void
CInterpreter::traceGet(RefArg rcvr, RefArg impl, RefArg path)
{
	if (!isTraceGetEnabled)
		return;
	if (ISNIL(impl))	// can happen when tracing hasVar and var doesn’t exist
		return;
	if (NOTNIL(fTraceSlotName) && !EQ(path, fTraceSlotName))
		return;
	if (NOTNIL(fTraceMethodName) && fTraceMethodDepth == 0)
		return;
	if (NOTNIL(fTraceViewContextFrame) && !IsParent(fTraceViewContextFrame, vm->locals))
		return;

	int indent = REPprintf("%*s", traceIndent, " ");
	if (IsSymbol(path))
		taciturnPrintObject(GetFrameSlot(impl, path), indent);
	else
		taciturnPrintObject(GetFramePath(impl, path), indent);
	REPprintf(" <= (");
	taciturnPrintObject(rcvr, indent);
	if (!EQ(impl, rcvr))
	{
		REPprintf("/");
		taciturnPrintObject(impl, indent);
	}
	REPprintf(").");
	taciturnPrintObject(path, indent);
	REPprintf("\n");
}

void
CInterpreter::traceSet(RefArg rcvr, RefArg impl, RefArg path, RefArg value)
{
	if (!isTraceGetEnabled)
		return;
	if (NOTNIL(fTraceSlotName) && !EQ(path, fTraceSlotName))
		return;
	if (NOTNIL(fTraceMethodName) && fTraceMethodDepth == 0)
		return;
	if (NOTNIL(fTraceViewContextFrame) && !IsParent(fTraceViewContextFrame, vm->locals))
		return;

	int indent = REPprintf("%*s(", traceIndent, " ");
	taciturnPrintObject(rcvr, indent);
	if (!EQ(impl, rcvr))
	{
		REPprintf("/");
		taciturnPrintObject(impl, indent);
	}
	REPprintf(").");
	taciturnPrintObject(path, indent);
	REPprintf(" := ");
	taciturnPrintObject(value, indent);
	REPprintf("\n");
}

void
CInterpreter::traceReturn(bool hasValue)
{
	if (NOTNIL(fTraceMethodName))
	{
		if (fTraceMethodDepth == 0)
			return;
		fTraceMethodDepth--;
	}
	if (traceIndent != 0)
		traceIndent -= 4;

	if (isTraceFuncEnabled)
	{
		int indent = REPprintf("%*s=> ", traceIndent, " ");
		if (hasValue)
			taciturnPrintObject(*(dataStack.top - 1), indent);
		REPprintf("\n");
	}
}

void
CInterpreter::traceArgs(ArrayIndex numArgs, ArrayIndex stackFrameIndex, int indent)
{
	for (int i = (int)stackFrameIndex + (int)numArgs - 1; i >= (int)stackFrameIndex; i--)
	{
		taciturnPrintObject(*(dataStack.top - 1 - i), indent);
		if (i > stackFrameIndex)
			REPprintf(", ");
	}
}


#pragma mark -
/*------------------------------------------------------------------------------
	E x c e p t i o n s

	Exception data is held in an array object (see new-handlers bytecode):
------------------------------------------------------------------------------*/

Ref
CInterpreter::translateException(Exception * x)
{
	RefVar	fr(AllocateFrame());
	RefVar	arg;

	RefVar sym(MakeSymbol(x->name));
	SetFrameSlot(fr, SYMA(name), sym);

	if (Subexception(x->name, exMessage))
	{
		arg = MakeStringFromCString((const char *)x->data);
		SetFrameSlot(fr, SYMA(message), arg);
	}
	else if (Subexception(x->name, "type.ref"))
	{
		arg = (Ref) x->data;
		SetFrameSlot(fr, SYMA(data), arg);
	}
	else
	{
		arg = MAKEINT((long) x->data);
		SetFrameSlot(fr, SYMA(error), arg);
	}

	return fr;
}


Ref
CInterpreter::exceptionBeingHandled(void)
{
	Ref	context;
	Ref	exception;

	for (context = exceptionContext; NOTNIL(context); context = GetArraySlotRef(context, 0))
	{
		exception = GetArraySlotRef(context, kExcDataFrame);
		if (NOTNIL(exception))
			return exception;
	}
	return NILREF;
}



bool
CInterpreter::handleException(Exception * inException, int inDepth, StackState & inState)
{
	const char * xName = inException->name;

	if (!DeveloperNotified(inException) && NOTNIL(GetFrameSlot(RA(gVarFrame), SYMA(breakOnThrows))))
	{
		gREPout->exceptionNotify(inException);
		DoBlock(GetFrameSlot(RA(gFunctionFrame), SYMA(BreakLoop)), RA(NILREF));
		RememberDeveloperNotified(inException);
	}

	TraceFunction trace;
	int xStackDepth;

	RefVar xContext;
	for (xContext = exceptionContext; NOTNIL(xContext) && (xStackDepth = RINT(GetArraySlot(xContext, kExcDataCtrlStackIndex))) >= inDepth; xContext = GetArraySlot(xContext, kExcDataNext))
	{
		if (ISNIL(GetArraySlot(xContext, kExcDataFrame)))
		{
			RefVar handlers(GetArraySlot(xContext, kExcDataHandlers));
			for (ArrayIndex i = 0, count = Length(handlers); i < count; i += 2)
			{
				if (Subexception(xName, SymbolName(GetArraySlot(handlers, i))))
				{
					int stackDelta = STACKINDEX(dataStack) - RINT(GetArraySlot(xContext, kExcDataFnStackIndex));
					dataStack.top = dataStack.top - MAKEINT(stackDelta);
					if (gFramesFunctionProfilingEnabled == kProfilingEnabled && (trace = gFramesFunctionProfiler) != NULL)
					{
						int stackPos = gCurrentStackPos >> 16;
						// trace the stack
						for (VMState * xVM = ctrlStack.at(xStackDepth/kNumOfItemsInStackFrame); vm != xVM; )
						{
							VMState * prevVM = vm;
							vm = ctrlStack.pop();
							
							trace(&vm->func, &prevVM->func, STACKINDEX(ctrlStack), stackPos, 3, gFramesFunctionProfilerData);
						}
					}

					stackDelta = STACKINDEX(ctrlStack) - xStackDepth;
					ctrlStack.top = ctrlStack.top - MAKEINT(stackDelta);
					vm = ctrlStack.at(STACKINDEX(ctrlStack)/kNumOfItemsInStackFrame);
					
					vm->func = GetArraySlot(xContext, kExcDataFn);
					instructionOffset = RINT(GetArraySlot(handlers, i+1));
					vm->rcvr = GetArraySlot(xContext, kExcDataRcvr);
					vm->impl = GetArraySlot(xContext, kExcDataImpl);
					vm->locals = GetArraySlot(xContext, kExcDataLocals);
					setFlags();
					SetArraySlot(xContext, kExcDataFrame, translateException(inException));
					exceptionContext = xContext;
					return YES;
				}
			}
		}
	}

	if (gFramesFunctionProfilingEnabled == kProfilingEnabled && (trace = gFramesFunctionProfiler) != NULL)
	{
		int stackPos = gCurrentStackPos >> 16;
		int stackDepth = STACKINDEX(ctrlStack);
		// trace the stack
		for (VMState * xVM = ctrlStack.at(inState.ctrlStackIndex/kNumOfItemsInStackFrame); vm != xVM; )
		{
			VMState * prevVM = vm;
			vm = ctrlStack.pop();
			stackDepth -= kNumOfItemsInStackFrame;

			trace(&vm->func, &prevVM->func, stackDepth, stackPos, 3, gFramesFunctionProfilerData);
		}
	}

	ResetStack(inState);
	DisposeRefHandle(inState.exceptionContext.h);

	// restore VM state of caller
	vm = ctrlStack.pop();
	instructionOffset = RVALUE(vm->pc);

	// if we had any exception handlers, pop them
	if (STACKINDEX(ctrlStack) < exceptionStackIndex)
		popHandlers();
	if (NOTNIL(vm->func) && instructionOffset != -1)
		setFlags();

	return NO;
}


/*------------------------------------------------------------------------------
	Pop exception handlers off the stack until they’re back within
	the current stack.
------------------------------------------------------------------------------*/

void
CInterpreter::popHandlers(void)
{
	for ( ; NOTNIL(exceptionContext) && RINT(GetArraySlot(exceptionContext, 2)) > STACKINDEX(ctrlStack); exceptionContext = GetArraySlot(exceptionContext, 0))
	{
		; // just loop
	}
}


#pragma mark -
/*----------------------------------------------------------------------
	Developer notification.
----------------------------------------------------------------------*/

struct NotifyItem
{
	struct NotifyItem	*	next;
	char *					name;
} * gDeveloperNotified = NULL;	// 0C10546C


bool
DeveloperNotified(Exception * inException)
{
	NotifyItem * p;

	for (p = gDeveloperNotified; p != NULL; p = p->next)
		if (p->name == inException->name)
			return YES;
	return NO;
}


void
RememberDeveloperNotified(Exception * inException)
{
	XTRY
	{
		// dup exception name
		char * xName = (char *)malloc(strlen(inException->name) + 1);
		XFAIL(xName == NULL)

		// create new NotifyItem with that name
		NotifyItem * xItem = (NotifyItem *)malloc(sizeof(NotifyItem));
		XFAILIF(xItem == NULL, free(xName);)
		strcpy(xName, inException->name);
		inException->name = xName;		// sic
		xItem->name = xName;

		// insert at head of list
		xItem->next = gDeveloperNotified;
		gDeveloperNotified = xItem;
	}
	XENDTRY;
}


void
ForgetDeveloperNotified(ExceptionName inName)
{
	NotifyItem * p, * prevp;

	for (prevp = p = gDeveloperNotified; p != NULL; prevp = p, p = p->next)
	{
		if (p->name == inName)
		{
			prevp->next = p->next;
			free(p->name);
			free(p);
			break;
		}
	}
}


#pragma mark -
/*------------------------------------------------------------------------------
	B r e a k p o i n t s
------------------------------------------------------------------------------*/

Ref
SetBreakPoints(Ref bps)
{
	Ref	prevBPs = gFramesBreakPoints;
	gFramesBreakPoints = bps;
	return prevBPs;
}


bool
EnableBreakPoints(bool doEnable)
{
	bool	prevEnable = gFramesBreakPointsEnabled;
	gFramesBreakPointsEnabled = doEnable;
	return prevEnable;
}


void
CInterpreter::handleBreakPoints(void)
{
	if (ISNIL(gFramesBreakPoints))
		return;

	RefVar	bps(GetFrameSlot(gFramesBreakPoints, SYMA(programCounter)));
	if (NOTNIL(bps))
	{
		bool					isBP = NO;
		CObjectIterator	iter(bps);
		for ( ; !iter.done(); iter.next())
		{
			Ref pc = GetFrameSlot(iter.value(), SYMA(programCounter));
			if (RINT(pc) == instructionOffset)
			{
				if (EQRef(instructions, GetFrameSlot(iter.value(), SYMA(instructions)))
				 && ISNIL(GetFrameSlot(iter.value(), SYMA(disabled))))
				{
					isBP = YES;
					if (NOTNIL(GetFrameSlot(iter.value(), SYMA(temporary))))
					{
						ArrayRemoveCount(bps, RINT(iter.tag()), 1);
					}
				}
			}
		}
		if (Length(bps) == 0)
			RemoveSlot(gFramesBreakPoints, SYMA(programCounter));
		if (Length(gFramesBreakPoints) == 0)
			gFramesBreakPoints = NILREF;
		if (isBP)
		{
			RefVar	breakLoop(GetFrameSlot(gFunctionFrame, SYMA(BreakLoop)));
			DoBlock(breakLoop, RA(NILREF));
		}
	}
}


#pragma mark -
/*------------------------------------------------------------------------------
	F r a m e s F u n c t i o n P r o f i l i n g
	Never called.
------------------------------------------------------------------------------*/

bool
EnableFramesFunctionProfiling(bool inEnable)
{
	bool	prevEnable = (gFramesFunctionProfilingEnabled == kProfilingEnabled);
	gFramesFunctionProfilingEnabled = inEnable ? kProfilingEnabled : 0;
	return prevEnable;
}


FramesFunctionProfilerInfo
InstallFramesFunctionProfiler(TraceFunction inFunc, void * inData)
{
	FramesFunctionProfilerInfo prev;
	prev.func = gFramesFunctionProfiler;
	prev.data = gFramesFunctionProfilerData;
	gFramesFunctionProfiler = inFunc;
	gFramesFunctionProfilerData = inData;
	return prev;
}

