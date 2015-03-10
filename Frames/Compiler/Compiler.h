/*
	File:		Compiler.h

	Contains:	NewtonScript compiler classes.

	Written by:	Newton Research Group.
*/

#if !defined(__COMPILER_H)
#define __COMPILER_H 1

#if !defined(__INTERPRETER_H)
#include "Interpreter.h"
#endif

#if !defined(__OPCODES_H)
#include "Opcodes.h"
#endif

#if !defined(__INPUTSTREAMS_H)
#include "InputStreams.h"
#endif

#include "Compiler-AST.h"


typedef char Str255[256];

enum MsgEnvComponent
{
	kMsgSlotAccess,
	kMsgMessageSend
};

class	CFunctionState;
class	CLoopState;

/*------------------------------------------------------------------------------
	C C o m p i l e r
------------------------------------------------------------------------------*/

class	CCompiler
{
public:
					CCompiler(CInputStream * inStream, bool inDoConstituents);
					~CCompiler();

	Ref			compile(void);
	void			parse(void);	// FOR DEBUG

	ArrayIndex	lineNo(void) const;
	void			error(NewtonErr inErr);
	void			errorWithValue(NewtonErr inErr, RefArg inValue);

private:
	// the lexer
	int			getToken(void);
	int			consumeToken(void);
	void			PrintToken(int inToken);	// FOR DEBUG

	UniChar		consumeChar(void);
	UniChar *	getCharsUntil(UniChar inCh, bool isString, ArrayIndex * outSize);
	int			getNumber(void);
	int			getHexNumber(void);	// new!

	int			makeToken(int inToken);
	int			makeBooleanToken(bool inValue);
	int			makeCharacterToken(UniChar inValue);
	int			makeIntegerToken(long inValue);
	int			makeRealToken(double inValue);
	int			makeRefToken(long inValue);
	int			makeSymbolToken(const char * inValue);
	int			makeStringToken(const UniChar * inValue, ArrayIndex inLength);


	// the parser
	ExprAST *	parseExpression(void);
	ExprAST *	parseExpression1(void);
	ExprAST *	parseUnary(void);
	ExprAST *	parseOperRHS(int inPrecedence, ExprAST * inLHS);

	ExprAST *	parseGlobalDecl(void);
	ExprAST *	parseLocalDecl(void);
	ExprAST *	parseConstantDecl(void);
	bool			parseInitClause(ExprAST * &value);

	ExprAST *	parseParenExpression(void);
	ExprAST *	parseCompoundExpression(void);
	ExprAST *	parseExpressionSequence(int inUntilToken);
	ExprAST *	parseLiteralExpr(void);
	ExprAST *	parseObjectExpr(void);
	ExprAST *	parseArrayConstructor(void);
	ExprAST *	parseFrameConstructor(void);
	ExprAST *	parseFunctionConstructor(void);
	bool			parseFormalArgumentList(std::vector<std::pair<int,std::string> > &args);
	int			parseType(void);

	ExprAST *	parseSymbolExpr(void);
	ExprAST *	parseCallExpr(void);
	bool			parseActualArgumentList(std::vector<ExprAST*> &args);
	ExprAST *	parseIfExpr(void);
	ExprAST *	parseLoopExpr(void);
	ExprAST *	parseForExpr(void);
	ExprAST *	parseForeachExpr(void);
	ExprAST *	parseWhileExpr(void);
	ExprAST *	parseRepeatExpr(void);
	ExprAST *	parseBreakExpr(void);
	ExprAST *	parseReturnExpr(void);
	ExprAST *	parseTryExpr(void);

	int			parser(void);
	bool			parserStackOverflow(void);

	void			newFunctionState(RefArg inArgs, CFunctionState * inFS, int * ioDepthPtr);
	Ref			endFunction(void);

	void			simplify(RefArg inGraph);
	void			walkForDeclarations(RefArg inGraph);
	friend int	DeclarationWalkerTrampoline(void * inContext, RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5);
	int			declarationWalker(RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5);
	void			walkForClosures(RefArg inGraph);
	friend int	ClosureWalkerTrampoline(void * inContext, RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5);
	int			closureWalker(RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5);
	int			walkForCode(RefArg inGraph, bool);
	int			walkAssignment(RefArg, RefArg, bool);
	Ref			walkForPath(RefArg inGraph, int * ioNumOfPathElements);
	int			walkForStringer(RefArg inGraph);
	bool			isConstantExpr(RefArg inGraph);
	Ref			evaluateConstantExpr(RefArg inGraph);

	ArrayIndex	freqFuncIndex(RefArg inName, ArrayIndex inNumArgs);
	void			emitFuncall(RefArg inName, ArrayIndex inNumArgs);
	void			emitReturn(void);
	void			emitBranch(ArrayIndex inPC);
	void			emitPop(void);
	void			emitPush(RefArg inValue);
	void			emitVarGet(RefArg inName);
	void			emitVarSet(RefArg inName);
	void			emitVarIncr(RefArg inName);
	void			emit(Opcode a, int b);
	ArrayIndex	emitPlaceholder(void);
	ArrayIndex	curPC(void) const;
	void			backpatch(ArrayIndex inOffset, Opcode a, int b);

	void			warning(const char * inMsg);
	void			error(const char * inMsg);
	void			syntaxError(const char * inMsg);

	CFunctionState *	func;				// +00
	ArrayIndex	lineNumber;				// +04
	ArrayIndex	colmNumber;
	CInputStream *		stream;			// +08
	UniChar		theChar;					//			lexer input -- char last read from the stream
	Token			theToken;				//			lexer output -- transitioning from the yylval global
	int			stackSize;				//	+0C	yystacksize, yacc stack size
	RefStruct	yaccStack;				// +10
	Ref *			vStack;					// +14	parser value stack
	short *		sStack;					// +18	parser state stack
	bool			doConstituents;		// +1C	interpret constituent-at-a-time
//	int32_t		x20;						// +20	never referenced
	int *			funcDepthPtr;			// +24
//	bool			isStackedToken;		// +28
	Token			stackedToken;			// +2C
//	RefStruct	stackedTokenValue;	// +30
};

inline ArrayIndex CCompiler::lineNo(void) const { return lineNumber; }


/*------------------------------------------------------------------------------
	C F u n c t i o n S t a t e
------------------------------------------------------------------------------*/

class	CFunctionState
{
public:
				CFunctionState(CCompiler * inCompiler, RefArg inArgs, CFunctionState * inFunc, int * ioDepthPtr);
				~CFunctionState();

	Ref		makeCodeBlock(void);
	void		addConstant(RefArg, RefArg);
	void		addLocals(RefArg);
	bool		atTopLevel(void);
	void		setNext(CFunctionState * inLink);
	CFunctionState *	context(void);
	void		beginLoop(void);
	void		addLoopExit(void);
	void		endLoop(void);
	void		computeArgFrame(void);
	void		computeInitialVarLocs(void);
	Ref		argFrame(void);
	Ref		varLocs(void);
	void		copyClosedArgs(void);
	void		declarationsFinished(void);
	Ref		getConstantValue(RefArg, bool *);
	bool		isConstant(RefArg);
	bool		isLocalConstant(RefArg);
	bool		isLocalVariable(RefArg);
	ArrayIndex	variableIndex(RefArg inTag);
	ArrayIndex	literalOffset(RefArg);
	void		noteMsgEnvReference(MsgEnvComponent);
	bool		noteVarReference(RefArg);

	ArrayIndex	emit(Opcode a, int b);
	ArrayIndex	emitOne(unsigned char bytecode);
	ArrayIndex	emitThree(unsigned char bytecode, int value);
	ArrayIndex	emitPlaceholder(void);
	ArrayIndex	curPC(void) const;
	void			backpatch(ArrayIndex inPC, Opcode a, int b);

private:
	CCompiler *		fCompiler;			// +00
	RefStruct		fArgs;				// +04
	RefStruct		fLocals;				// +08
	RefStruct		fConstants;			// +0C
	RefStruct		fInstructions;		// +10
	RefStruct		fLiterals;			// +14
	RefStruct		fArgFrame;			// +18
	RefStruct		fNotedLocals;		// +1C	doesnÕt do anything useful
	RefStruct		fVarLocs;			// +20
	ArrayIndex		fNumOfVarLocs;		// +24
	ArrayIndex		fPC;					// +28
	ArrayIndex		fNumOfLiterals;	// +2C
	ArrayIndex		fNumOfArgs;			// +30
	ArrayIndex		fNumOfLocals;		// +34
	CLoopState *	fLoop;				// +38
	int				fFuncDepth;			// +3C
	bool				fIsParent;			// +40
	bool				fIsSlotReferenced;	// +44
	bool				fIsMethodReferenced;	// +48
	bool				fHasClosedVars;	// +4C
	bool				fKeepVarNames;		// +50
	CFunctionState *	fScope;			// +54
	CFunctionState *	fNext;			// +58
};


/*------------------------------------------------------------------------------
	C F u n c t i o n S t a t e   I n l i n e s
------------------------------------------------------------------------------*/

inline	ArrayIndex		CFunctionState::curPC(void) const
{ return fPC; }

inline	void				CFunctionState::setNext(CFunctionState * inLink)
{ fNext = inLink; }

inline	CFunctionState *	CFunctionState::context(void)
{ return fScope; }

inline		Ref		CFunctionState::argFrame(void)
{ return fArgFrame; }

inline		Ref		CFunctionState::varLocs(void)
{ return fVarLocs; }


/*------------------------------------------------------------------------------
	C L o o p S t a t e
------------------------------------------------------------------------------*/

class CLoopState
{
public:
						CLoopState(CFunctionState * inFunc, CLoopState * inLoop);
						~CLoopState();

	void				addExit(ArrayIndex pc);
	CLoopState *	endLoop(ArrayIndex pc);

private:
	CFunctionState *	fEncFunc;
	CLoopState *		fEncLoop;
	RefStruct			fExits;
};


/*------------------------------------------------------------------------------
	C C o m p i l e r   I n l i n e s
------------------------------------------------------------------------------*/

inline	ArrayIndex	CCompiler::curPC(void) const
{ return func->curPC(); }

inline	ArrayIndex	CCompiler::emitPlaceholder(void)
{ return func->emitPlaceholder(); }

inline	void			CCompiler::emit(Opcode a, int b)
{ func->emit(a,b); }

inline	void			CCompiler::backpatch(ArrayIndex inPC, Opcode a, int b)
{ func->backpatch(inPC, a, b); }


#endif	/* __COMPILER_H */
