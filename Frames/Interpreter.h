/*
	File:		Interpreter.h

	Contains:	Internal interpreter function definitions.

	Written by:	Newton Research Group.
*/

#if !defined(__INTERPRETER_H)
#define __INTERPRETER_H 1

#include "Globals.h"
#include "Symbols.h"
#include "Arrays.h"
#include "Frames.h"
#include "RefStack.h"


/* -----------------------------------------------------------------------------
	Exception data is held in an array object (see new-handlers bytecode):
----------------------------------------------------------------------------- */

enum
{
	kExcDataNext,				// next exception context
	kExcDataFnStackIndex,	// function stack depth
	kExcDataCtrlStackIndex,	// control stack depth
	kExcDataFn,					// function
	kExcDataRcvr,				// receiver
	kExcDataImpl,				// implementor
	kExcDataHandlers,			// array of handler sym/PC pairs
	kExcDataFrame,				// exception frame, {name:, message:|data:|error:}
	kExcDataLocals,			// locals

	kExcDataSize				// length of this array
};


/*------------------------------------------------------------------------------
	T y p e s
------------------------------------------------------------------------------*/

// Virtual machine state
struct VMState
{
	Ref		pc;			// +00 - int
	Ref		func;			// +04 - frame
	Ref		locals;		// +08 - array
	Ref		impl;			// +0C - frame
	Ref		rcvr;			// +10 - frame
	Ref		stackFrame;	// +14 - int stack index + flags
};

#define kNumOfItemsInStackFrame 6


// Stack that holds pseudo-RefStructs used by the VM
// - convenient for passing pseudo-RefArgs
class RefStructStack : public RefStack
{
public:
				RefStructStack();
				~RefStructStack();

	void		fill(void);

	RefStruct *	xBase;	// +10
	RefStruct *	xTop;		// +14

friend bool ReleaseRefStructStackProc(void *, OpaqueRef *, OpaqueRef *, bool);
};


// Stack that holds the VM state
class VMStack : public RefStack  // should really be a RefStructStack
{
public:
	VMState *	push(void);	// pushes a nil VMState
	VMState *	dup(void);
	VMState *	pop(void);
	VMState *	at(ArrayIndex);
};


struct StackState
{
	ArrayIndex	ctrlStackIndex;
	ArrayIndex	dataStackIndex;
	RefStruct	exceptionContext;
	ArrayIndex	traceIndent;
};


typedef Ref (*CFunction)(RefArg,...);

typedef int FramesProfilingKind;
typedef void (*TraceFunction)(RefArg, RefArg, int, int, FramesProfilingKind, void*);


/*------------------------------------------------------------------------------
	S t a c k   F r a m e   B i t s
------------------------------------------------------------------------------*/

#define kStackFrameFlagBits	6
#define kStackFrameFlagMask	((1 << kStackFrameFlagBits) - 1)

#define kStackFrameNoLocals	0
#define kStackFrameLocals		1

#define STACKINDEX(s) (ArrayIndex)((s.top - s.base) - 1)


/*------------------------------------------------------------------------------
	C I n t e r p r e t e r
------------------------------------------------------------------------------*/

class CInterpreter
{
public:
				CInterpreter();
				~CInterpreter();

	void		setFlags(void);

	void		run(void);
	void		run1(ArrayIndex initialStackDepth);

	void		pushValue(Ref);
	Ref		popValue(void);
	Ref		peekValue(ArrayIndex);
	void		setValue(ArrayIndex, Ref);
	ArrayIndex	valuePosition(void);
	Ref		peekControl(ArrayIndex);
	void		setControl(ArrayIndex, Ref);
	ArrayIndex	controlPosition(void);

	void		setLocalOnStack(RefArg, RefArg, RefArg);
	Ref		getLocalFromStack(RefArg, RefArg);
	Ref		getSelfFromStack(Ref);
	ArrayIndex	numStackFrames(void);
	VMState * stackFrameAt(ArrayIndex index);

	void		setCallEnv(void);
	void		setSendEnv(RefArg, RefArg);
	bool		getIsSend(void);
	Ref		getReceiver(void);
	Ref		getImplementor(void);

	void		topLevelCall(RefArg func, RefArg args);

	int		call(RefArg codeBlock, ArrayIndex numArgs);
	int		send(RefArg rcvr, RefArg impl, RefArg msg, ArrayIndex numArgs);

	ArrayIndex	pushArgArray(Ref args);

	int		unsafeDoSend(RefArg rcvr, RefArg impl, RefArg fn, ArrayIndex numArgs);
	int		unsafeDoCall(Ref func, ArrayIndex numArgs);

	void		callCodeBlock(RefArg, ArrayIndex, unsigned);
	void		callPlainCodeBlock(RefArg, ArrayIndex, unsigned);
	void		callCFunction(RefArg, ArrayIndex, bool);
	void		callPlainCFunction(RefArg, ArrayIndex);
	Ref		callCFuncPtr(Ref (*)(RefArg, ...), ArrayIndex);

	void		rreturn(FramesProfilingKind);
	void		stackTrace(void);
	void		taciturnPrintObject(Ref, int);

	void		traceSetOptions(void);
	void		traceApply(RefArg func, ArrayIndex numArgs);
	void		traceArgs(ArrayIndex numArgs, ArrayIndex stackFrameIndex, int indent);
	void		traceFreqCall(ArrayIndex index);
	void		traceCall(RefArg func, ArrayIndex numArgs);
	void		traceGet(RefArg, RefArg, RefArg);
	void		traceSet(RefArg, RefArg, RefArg, RefArg);
	void		traceSend(RefArg rcvr, RefArg msg, ArrayIndex numArgs, ArrayIndex stackFrameIndex);
	void		traceMethod(RefArg, RefArg, const char * name, ArrayIndex numArgs, ArrayIndex stackFrameIndex);
	void		traceReturn(bool hasValue = YES);

	Ref		translateException(Exception * inException);
	bool		handleException(Exception * inException, int inDepth, StackState & inState);
	Ref		exceptionBeingHandled(void);
	void		popHandlers(void);

	void		handleBreakPoints(void);

	CInterpreter *	next;					// x00
	ULong			id;						// x04
	VMStack		ctrlStack;				// x08
	RefStructStack dataStack;			// x20
	RefStruct	exceptionContext;		// x38
	ArrayIndex	exceptionStackIndex;	// x3C
	RefStruct	literals;				// x40
	RefStruct	instructions;			// x44
	bool			isReadOnly;				// x48
	VMState *	vm;						// x4C
	int			instructionOffset;	// x50
	bool			is2x;						// x54	locals for 2.x
	int			localsIndex;			// x58	index into dataStack for 2.x locals
	bool			isSend;					// x5C
//	bool			isFast;					// x60
	int			traceIndent;			// x64
	bool			isTraceGetEnabled;	// x68
	bool			isTraceFuncEnabled;	// x69
	RefStruct	fTraceSlotName;		// x6C
	RefStruct	fTraceMethodName;		// x70
	RefStruct	fTraceViewContextFrame;	// x74
	int			fTraceMethodDepth;	// x78
	int			tracing;					// x7C
};


/*------------------------------------------------------------------------------
	i n l i n e s
	Message Send Environment
------------------------------------------------------------------------------*/

inline void CInterpreter::setCallEnv(void)
{ isSend = NO;}

inline void CInterpreter::setSendEnv(RefArg receiver, RefArg implementor)
{ isSend = YES; vm->rcvr = receiver; vm->impl = implementor; }

inline bool CInterpreter::getIsSend(void)
{ return isSend; }

inline Ref  CInterpreter::getReceiver(void)
{ return vm->rcvr; }

inline Ref  CInterpreter::getImplementor(void)
{ return vm->impl; }


/*----------------------------------------------------------------------
	Primitive function definition.
----------------------------------------------------------------------*/

struct FreqFuncDef
{
	const char *	name;
	ArrayIndex		numOfArgs;
};

extern FreqFuncDef	gFreqFuncInfo[25];		// 0C1051F4
extern ArrayIndex		gNumFreqFuncs;				// 0C1052BC

extern Ref				gCodeBlockPrototype;
extern Ref				gDebugCodeBlockPrototype;


/*----------------------------------------------------------------------
	Function type.
----------------------------------------------------------------------*/

typedef enum
{
	kNSFunction,
	kCFunction
} FunctionType;


/*----------------------------------------------------------------------
	Function frame slot indices.
----------------------------------------------------------------------*/

// CodeBlock, _function
enum
{
	kFunctionClassIndex,
	kFunctionInstructionsIndex,
	kFunctionLiteralsIndex,
	kFunctionArgFrameIndex,
	kFunctionNumArgsIndex,
	kFunctionDebugIndex
};

enum
{
	kPlainCFunctionClassIndex,
	kPlainCFunctionPtrIndex,
	kPlainCFunctionNumArgsIndex
};

enum
{
	kCFunctionClassIndex,
	kCFunctionCodeIndex,
	kCFunctionNumArgsIndex,
	kCFunctionPtrIndex
};

enum
{
	kNativeFunctionClassIndex,
	kNativeFunctionCodeIndex,
	kNativeFunctionNumArgsIndex,
	kNativeFunctionClosureIndex,
	kNativeFunctionOffsetIndex
};

enum
{
	kArgFrameNextArgFrameIndex,
	kArgFrameParentIndex,
	kArgFrameImplementorIndex,
	kArgFrameArgIndex
};


/*------------------------------------------------------------------------------
	T r a c i n g
------------------------------------------------------------------------------*/

enum
{
	kTraceNothing,
	kTraceFunctions,
	kTraceVars,
	kTraceEverything
};


/*------------------------------------------------------------------------------
	I t e r a t o r   A r r a y   I n d i c e s
------------------------------------------------------------------------------*/

enum
{
	kIterTagIndex,					//	tag of current slot
	kIterValueIndex,				//	value of current slot
	kIterObjectIndex,				//	object
	kIterDeepNumOfSlotsIndex,  //	total number of slots if deeply
	kIterSlotIndex,				//	index of current slot
	kIterNumOfSlotsIndex,		//	number of slots in object
	kIterMapIndex,					//	map (if object is frame)
	kIterSize
};


/*------------------------------------------------------------------------------
	F u n c t i o n  P r o t o t y p e s
------------------------------------------------------------------------------*/

#if defined(__cplusplus)
extern "C" {
#endif

ArrayIndex	PushArgArray(Ref args);
Ref		DoCall(RefArg codeBlock, ArrayIndex numArgs);
Ref		DoSend(RefArg rcvr, RefArg impl, RefArg msg, ArrayIndex numArgs);

Ref		SetBreakPoints(Ref);
bool		EnableBreakPoints(bool);

#if defined(__cplusplus)
}
#endif



#endif	/* __INTERPRETER_H */
