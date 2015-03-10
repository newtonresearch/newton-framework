/*
	File:		Compiler-Parser.cc

	Contains:	The NewtonScript compiler.
					Parsing (AST generation) based on ideas from LLVM
					and not using yacc/flex tools as the original did.

	Written by:	The Newton Tools group.


	The formal grammar: from http://manuals.info.apple.com/en_US/NewtonScriptProgramLanguage.PDF

	input:
		[constituent [ ; constituent ]* [;] ]
	constituent:
		{ expression | global-declaration }

	expression:
		{ simple-expression | compound-expression | literal | constructor | lvalue | assignment | exists-expression | function-call | message-send | if-expression | iteration | break-expression | try-expression | local-declaration | constant-declaration | return-expression }
	simple-expression:
		{ expression binary-operator expression | unary-operator expression | ( expression ) | self }

	binary-operator:
		{ arithmetic-operator | relational-operator | boolean-operator | string-operator }
	arithmetic-operator:
		{ + | - | * | / | div | mod | << | >> }
	relational-operator:
		{ = | <> | < | > | <= | >= }
	boolean-operator:
		{ and | or }
	string-operator:
		{ & | && }
	unary-operator:
		{ - | not }

	compound-expression:
		begin expression-sequence end
	expression-sequence:
		[ expression [ ; expression ]* [ ; ] ]

	literal:
		{ simple-literal | ' object }
	simple-literal:
		{ string | integer | real | character | true | nil }
	object:
		{ simple-literal | path-expression | array | frame }
	path-expression:
		symbol [ . symbol ]+
	array:
		‘[’ [ symbol : ] [ object [ , object ]* [ , ] ] ‘]’
	frame:
		‘{’ [ frame-slot [ , frame-slot ]* [ , ] ] ‘}’
	frame-slot:
		symbol : object

	constructor:
		{ array-constructor | frame-constructor | function-constructor }
	array-constructor:
		‘[’ [ symbol : ] [ expression [ , expression ]* [ , ] ] ‘]’
	frame-constructor:
		‘{’ [ frame-constructor-slot [ , frame-constructor-slot ]* [ , ] ] ‘}’
	frame-constructor-slot:
		symbol : expression
	function-constructor:
		func [ native ] ( [formal-argument-list ] ) expression
	formal-argument-list:
		{formal-argument [ , formal-argument ]*
	formal-argument:
		[ [type ] symbol
	type:
		{ int | array }

	lvalue:
		{ symbol | frame-accessor | array-accessor }
	frame-accessor:
		expression . { symbol | ( expression ) }
	array-accessor:
		expression ‘[’ expression ‘]’

	assignment:
		lvalue := expression

	exists-expression:
		{ symbol | frame-accessor | [ expression ] : symbol } exists

	function-call:
		{ symbol ( [ actual-argument-list ] ) | call expression with ( [ actual-argument-list ] ) }
	actual-argument-list:
		expression [ , expression ]*

	message-send:
		[ { expression | inherited } ] { : | :? } symbol ( [ actual-argument-list ] )

	if-expression:
		if expression then expression [ ; ] [ else expression ]

	iteration:
		{ infinite-loop | for-loop | foreach-loop | while-loop | repeat-loop }
	infinite-loop:
		loop expression
	for-loop:
		for symbol := expression to expression [ by expression ] do expression
	foreach-loop:
		foreach symbol [ , symbol ] [ deeply ] in expression { do | collect } expression
	while-loop:
		while expression do expression
	repeat-loop:
		repeat expression-sequence until expression
	break-expression:
		break [ expression ]

	try-expression:
		try expression-sequence [ onexception symbol do expression [ ; ] ]+

	local-declaration:
		local [ type-specifier ] initalization-clause [ , initalization-clause ]*
	type-specifier:
		{ int | array }
	initialization-clause:
		symbol [ := expression ]

	constant-declaration:
		constant constant-init-clause [ , constant-init-clause ]*
	constant-init-clause:
		symbol := expression

	return-expression:
		return [ expression ]

	global-declaration:
		{ global initialization-clause | global-function-decl }
	global-function-decl:
		{ global | func } symbol ( [ formal-argument-list ] ) expression


	Operator Precedence

	Operator				Associativity
	.						->
	: :?					->
	[]						->
	-						->
	<< >>					->
	* / div mod			->
	+ -					->
	& &&					->
	exists				--
	< <= > >= = <>		->
	not					->
	and or				->
	:=						<-

*/

#include "Objects.h"
#include "Compiler.h"
#include "Tokens.h"
#include "Compiler-AST.h"
#include "Unicode.h"
#include "UStringUtils.h"


#define TRY
#define ENDTRY  						epicfail:

#define FAIL(expr)					if ((expr) != 0) goto epicfail;
#define FAILIF(expr, action)		if ((expr) != 0) { { action } goto epicfail; }
#define FAILNOT(expr, action)		if ((expr) == 0) { { action } goto epicfail; }


#pragma mark parsers
/* -----------------------------------------------------------------------------
	input:
		[constituent [ ; constituent ]* [;] ]
	constituent:
		{ expression | global-declaration }
----------------------------------------------------------------------------- */

void
CCompiler::parse(void)
{
	// prime the input stream
	lineNumber = 1; colmNumber = 0;
	consumeChar();
	consumeToken();

	ExprAST * ast;
	for (bool isEOF = NO; !isEOF; )
	{
		switch (theToken.id)
		{
		case EOF:
			isEOF = true; break;
		case ';':
			consumeToken(); break;

		case TOKENglobal:
		case TOKENfunc:
			parseGlobalDecl(); break;

		default:
			ast = parseExpression(); break;
		}
	}
	if (ast)
		ast->dump();	// FOR DEBUG
	// walk the AST
	// delete the AST to free all its mem
	delete ast;
}


/* -----------------------------------------------------------------------------
	expression:
		{ simple-expression | compound-expression | literal | constructor | lvalue | assignment | exists-expression | function-call | message-send | if-expression | iteration | break-expression | try-expression | local-declaration | constant-declaration | return-expression }
	simple-expression:
		{ expression binary-operator expression | unary-operator expression | ( expression ) | self }
----------------------------------------------------------------------------- */

ExprAST *
CCompiler::parseExpression(void)
{
	ExprAST * lhs;
	if ((lhs = parseUnary()))
		return parseOperRHS(0, lhs);
	return NULL;
}


ExprAST *
CCompiler::parseUnary(void)
{
	TRY
	{
		ExprAST * rcvr = NULL;
		std::string msg;
		switch (theToken.id)
		{
		default:
			return parseExpression1();

		// unary-operator
		case '-':
		case TOKENnot:
			{
			int oper = theToken.id;
			ExprAST * operand;
			consumeToken();
			FAILNOT(operand = parseExpression(), error("expected an expression");)
			return new UnaryExprAST(oper, operand);
			}

		case TOKENinherited:			// -> { : | :? } symbol ( [args] ) => message-send to inherited
			consumeToken();	// inherited
			rcvr = new ReceiverExprAST(kReceiverInherited);
			if (theToken.id == ':' || theToken.id == TOKENsendIfDefined)
				goto MessageSend;
			break;

		case ':':						// -> symbol => func-accessor implicitly in self
											// -> symbol ( [args] ) => message-send implicitly to self
			consumeToken();	// :
			rcvr = new ReceiverExprAST(kReceiverSelf);

			FAILNOT(theToken.id == TOKENsymbol, error("expected an identifier");)
			msg = SymbolName(theToken.value.ref);
			consumeToken();	// symbol

			if (theToken.id == '(')
				goto MessageArgs;
			return new FuncAccessExprAST(rcvr, msg);

		case TOKENsendIfDefined:	// -> symbol ( [args] ) => message-send implicitly to self
			rcvr = new ReceiverExprAST(kReceiverSelf);
MessageSend:
			consumeToken();	// : | :?

			FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
			msg = SymbolName(theToken.value.ref);
			consumeToken();	// symbol

MessageArgs:
			FAILNOT(theToken.id == '(', error("expected (");)
			consumeToken();	// (
			ExprList args;
			if (theToken.id != ')')
			{
				while (1)
				{
					ExprAST * arg = parseExpression();
					FAIL(arg == NULL)
					args.push_back(arg);

					if (theToken.id == ',')
						consumeToken();	// ,
					else
						break;	//	no more args
				}
			}
			FAILNOT(theToken.id == ')', error("expected )");)
			consumeToken();	// )

			return new MessageSendExprAST(rcvr, msg, args);
		}
	}
	ENDTRY;
	return NULL;
}

/*
	& #					will only work if expression values are refs

	Extras on LHS:
	inherited			-> { : | :? } symbol ( [args] ) => message-send
<	self					=> receiver self|inherited >
	:						-> symbol => func-accessor implicitly in self
	: | :?				-> symbol ( [args] ) => message-send implicitly to self

	Operators that can occur post-LHS:
	binary-operator	->	expression
	.						-> { symbol | ( expression ) } => frame-accessor
	[						-> { expression ] } => array-accessor
	:=						-> expression <LHS must be lvalue = { symbol | frame-accessor | array-accessor }>
	:						-> symbol => func-accessor
	: | :?				-> symbol ( [args] ) => message-send
	exists				=> exists-expression <LHS must be { symbol | frame-accessor | func-accessor }>


	Operator				Associativity
	.						->
	: :?					->
	[]						->
	-						->		<unary>
	<< >>					->
	* / div mod			->
	+ -					->
	& &&					->
	exists
	< <= > >= = <>		->
	not					->		<unary>
	and or				->
	:=						<-
	
*/
#define PRECEDENCEFrameAccessor	130
#define PRECEDENCEMessageSend		120
#define PRECEDENCEArrayAccessor	110
#define PRECEDENCEUnaryMinus		100
#define PRECEDENCEShiftOperator	 90
#define PRECEDENCEMulOperator		 80
#define PRECEDENCEAddOperator		 70
#define PRECEDENCEStrOperator		 60
#define PRECEDENCEExists			 50
#define PRECEDENCERelOperator		 40
#define PRECEDENCEUnaryNot			 30
#define PRECEDENCELogOperator		 20
#define PRECEDENCEAssign			 10

int
Token::precedence(void)
{
	switch (id)
	{
	case '.':						return PRECEDENCEFrameAccessor;
	case ':':
	case TOKENsendIfDefined:	return PRECEDENCEMessageSend;
	case '[':						return PRECEDENCEArrayAccessor;
	case TOKENLShift:
	case TOKENRShift:				return PRECEDENCEShiftOperator;
	case '*':
	case '/':
	case TOKENdiv:
	case TOKENmod:					return PRECEDENCEMulOperator;
	case '+':
	case '-':						return PRECEDENCEAddOperator;
	case '&':
	case TOKENAmperAmper:		return PRECEDENCEStrOperator;
	case TOKENexists:				return PRECEDENCEExists;
	case '<': case TOKENLEQ:
	case '>': case TOKENGEQ:
	case TOKENEQL:
	case TOKENNEQ:					return PRECEDENCERelOperator;
	case TOKENnot:					return PRECEDENCEUnaryNot;
	case TOKENand:
	case TOKENor:					return PRECEDENCELogOperator;
	case TOKENassign:				return PRECEDENCEAssign;
	}
	return -1;
}


ExprAST *
CCompiler::parseOperRHS(int inPrecedence, ExprAST * ioLHS)
{
	TRY
	{
		while (1)
		{
			int operPrecedence;
			int oper = theToken.id;
			switch (oper)
			{
			case '.':	// -> { symbol | ( expression ) } => frame-accessor
				{
					if (inPrecedence > PRECEDENCEFrameAccessor)
						return ioLHS;	// frame-accessor does not bind as tightly as previous operator

					consumeToken();	// .

					ExprAST * axsr = NULL;
					if (theToken.id == TOKENsymbol)
					{
						axsr = new SymbolAST(SymbolName(theToken.value.ref));
						consumeToken();	// symbol
					}
					else if (theToken.id == '(')
					{
						axsr = parseExpression();
						FAILNOT(axsr, error("expected an expression");)

						FAILNOT(theToken.id == ')', error("expected )");)
						consumeToken();	// )
					}
					FAILNOT(axsr, error("expected frame accessor");)

					ExprAST * rhs = new FrameAccessExprAST(ioLHS, axsr);

					if (theToken.precedence() > PRECEDENCEFrameAccessor)
					{
						// next operator binds more tightly
						rhs = parseOperRHS(PRECEDENCEFrameAccessor+1, rhs);
						FAIL(rhs == NULL)
					}
					ioLHS = rhs;
				}
				break;

			case '[':	//-> { expression ] } => array-accessor
				{
					if (inPrecedence > PRECEDENCEArrayAccessor)
						return ioLHS;	// array-accessor does not bind as tightly as previous operator

					consumeToken();	// [

					ExprAST * axsr = parseExpression();
					FAILNOT(axsr, error("expected array index");)

					FAILNOT(theToken.id == ']', error("expected ]");)
					consumeToken();	// ]

					ExprAST * rhs = new ArrayAccessExprAST(ioLHS, axsr);

					if (theToken.precedence() > PRECEDENCEArrayAccessor)
					{
						// next operator binds more tightly
						rhs = parseOperRHS(PRECEDENCEArrayAccessor+1, rhs);
						FAIL(rhs == NULL)
					}
					ioLHS = rhs;
				}
				break;

			case ':':						//	-> symbol => func-accessor  |  symbol ( [args] ) => message-send
			case TOKENsendIfDefined:	//	-> symbol ( [args] ) => message-send
				{
					if (inPrecedence > PRECEDENCEMessageSend)
						return ioLHS;	// message-send does not bind as tightly as previous operator

					consumeToken();	// : | :?

					FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
					std::string msg = SymbolName(theToken.value.ref);
					consumeToken();	// symbol

					if (theToken.id != '(' && oper == ':')
						return parseOperRHS(operPrecedence+1, new FuncAccessExprAST(ioLHS, msg));

					FAILNOT(theToken.id == '(', error("expected (");)
					consumeToken();	// (
					ExprList args;
					if (theToken.id != ')')
					{
						while (1)
						{
							ExprAST * arg = parseExpression();
							FAIL(arg == NULL)
							args.push_back(arg);

							if (theToken.id == ',')
								consumeToken();	// ,
							else
								break;	//	no more args
						}
					}
					FAILNOT(theToken.id == ')', error("expected )");)
					consumeToken();	// )

					ExprAST * rhs = new MessageSendExprAST(ioLHS, msg, args);

					if (theToken.precedence() > PRECEDENCEMessageSend)
					{
						// next operator binds more tightly
						rhs = parseOperRHS(PRECEDENCEMessageSend+1, rhs);
						FAIL(rhs == NULL)
					}
					ioLHS = rhs;
				}
				break;

			case TOKENexists:		// => exists-expression <LHS must be { symbol | frame-accessor | func-accessor }>
				{
					if (inPrecedence > PRECEDENCEExists)
						return ioLHS;	// array-accessor does not bind as tightly as previous operator

					consumeToken();	// exists

					FAILNOT(dynamic_cast<SymbolAST*>(ioLHS) || dynamic_cast<FrameAccessExprAST*>(ioLHS) || dynamic_cast<FuncAccessExprAST*>(ioLHS), error("destination of 'exists' must be a slot");)

					ExprAST * rhs = new ExistsExprAST(ioLHS);

					if (theToken.precedence() > PRECEDENCEExists)
					{
						// next operator binds more tightly
						rhs = parseOperRHS(PRECEDENCEExists+1, rhs);
						FAIL(rhs == NULL)
					}
					ioLHS = rhs;
				}
				break;

			case '<':
			case '>':
			case TOKENLEQ:
			case TOKENGEQ:
			case TOKENEQL:
			case TOKENNEQ:
				operPrecedence = PRECEDENCERelOperator;
				goto DoBinaryOp;

			case TOKENLShift:
			case TOKENRShift:
				operPrecedence = PRECEDENCEShiftOperator;
				goto DoBinaryOp;

			case '*':
			case '/':
			case TOKENdiv:
			case TOKENmod:
				operPrecedence = PRECEDENCEMulOperator;
				goto DoBinaryOp;

			case '+':
			case '-':
				operPrecedence = PRECEDENCEAddOperator;
				goto DoBinaryOp;

			case '&':
			case TOKENAmperAmper:
				operPrecedence = PRECEDENCEStrOperator;
				goto DoBinaryOp;

			case TOKENand:
			case TOKENor:
				operPrecedence = PRECEDENCELogOperator;
DoBinaryOp:
				{
					if (inPrecedence > operPrecedence)
						return ioLHS;	// does not bind as tightly as previous operator

					consumeToken();	// oper

					ExprAST * rhs = parseUnary();
					FAIL(rhs == NULL)

					if (theToken.precedence() > operPrecedence)
					{
						// next operator binds more tightly
						rhs = parseOperRHS(operPrecedence+1, rhs);
						FAIL(rhs == NULL)
					}
					ioLHS = new BinaryExprAST(oper, ioLHS, rhs);
				}
				break;

			case TOKENassign:		// -> expression <LHS must be lvalue = { symbol | frame-accessor | array-accessor }>
				{
					if (inPrecedence > PRECEDENCEAssign)
						return ioLHS;	// array-accessor does not bind as tightly as previous operator

					consumeToken();	// :=

					FAILNOT(dynamic_cast<SymbolAST*>(ioLHS) || dynamic_cast<FrameAccessExprAST*>(ioLHS) || dynamic_cast<ArrayAccessExprAST*>(ioLHS), error("destination of := must be a variable");)

					ExprAST * rhs = parseUnary();
					FAIL(rhs == NULL)

					if (theToken.precedence() > PRECEDENCEAssign)
					{
						// next operator binds more tightly
						rhs = parseOperRHS(PRECEDENCEAssign+1, rhs);
						FAIL(rhs == NULL)
					}
					ioLHS = new AssignExprAST(ioLHS, rhs);
				}
				break;

			default:
				// is not an operator
				return ioLHS;
			}
		}
	}
	ENDTRY;
	
	return NULL;
}


ExprAST *
CCompiler::parseExpression1(void)
{
	switch (theToken.id)
	{
//	{ simple-expression | compound-expression | literal | constructor | lvalue | assignment | exists-expression | function-call | message-send | if-expression | iteration | break-expression | try-expression | local-declaration | constant-declaration | return-expression }

//	simple-expression
	case '(':
		return parseParenExpression();
	case TOKENself:
		return new ReceiverExprAST(kReceiverSelf);

//	compound-expression
	case TOKENbegin:
		return parseCompoundExpression();

//	literal
	case '\'':
		return parseLiteralExpr();
	case TOKENconst:
	case TOKENinteger:
	case TOKENreal:
		return parseObjectExpr();
//	constructor
	case '[':
		return parseArrayConstructor();
	case '{':
		return parseFrameConstructor();
	case TOKENfunc:
		return parseFunctionConstructor();

//	lvalue | function-call
	case TOKENsymbol:
		return parseSymbolExpr();
	case TOKENcall:
		return parseCallExpr();

//	if-expression
	case TOKENif:
		return parseIfExpr();
//	iteration
	case TOKENloop:
		return parseLoopExpr();
	case TOKENfor:
		return parseForExpr();
	case TOKENforeach:
		return parseForeachExpr();
	case TOKENwhile:
		return parseWhileExpr();
	case TOKENrepeat:
		return parseRepeatExpr();
//	break-expression
	case TOKENbreak:
		return parseBreakExpr();
//	try-expression
	case TOKENtry:
		return parseTryExpr();
//	local-declaration
	case TOKENlocal:
		return parseLocalDecl();
//	constant-declaration
	case TOKENconstant:
		return parseConstantDecl();
//	return-expression
	case TOKENreturn:
		return parseReturnExpr();
	}
	return NULL;
}


/* -----------------------------------------------------------------------------
	simple-expression:
		{ <snipped> | ( expression ) | <snipped> }
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseParenExpression(void)
{
	TRY
	{
		consumeToken();	//	(

		ExprAST * body = parseExpression();
		FAIL(body == NULL)

		FAILNOT(theToken.id == ')', error("expected )");)
		consumeToken();	//	)

		return body;
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	compound-expression:
		begin expression-sequence end
	expression-sequence:
		[ expression [ ; expression ]* [ ; ] ]
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseCompoundExpression(void)
{
	TRY
	{
		consumeToken();	//	begin

		ExprAST * body = parseExpressionSequence(TOKENend);
		FAIL(body == NULL)

		FAILNOT(theToken.id == TOKENend, error("expected end");)
		consumeToken();	// end

		return body;
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


ExprAST *
CCompiler::parseExpressionSequence(int inUntilToken)
{
	TRY
	{
		std::vector<ExprAST*> seq;
		if (theToken.id != inUntilToken)
		{
			while (1)
			{
				ExprAST * expr = parseExpression();
				FAIL(expr == NULL)
				seq.push_back(expr);

				if (theToken.id == ';')
					consumeToken();	// ;
				if (theToken.id == inUntilToken)
					break;	//	no more expressions
			}
		}
		// caller must consume until-token

		return new ExprSeqAST(seq);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


#pragma mark literal
/* -----------------------------------------------------------------------------
	literal:
		{ simple-literal | ' object }
	simple-literal:
		{ string | integer | real | character | true | nil }
	object:
		{ simple-literal | path-expression | array | frame }
	path-expression:
		symbol [ . symbol ]+
	array:
		‘[’ [ symbol : ] [ object [ , object ]* [ , ] ] ‘]’
	frame:
		‘{’ [ frame-slot [ , frame-slot ]* [ , ] ] ‘}’
	frame-slot:
		symbol : object

	simple-literal is trivially reduced to "return new ConstExprAST(theToken);"
	in the look-ahead function so we don’t see it here
	we only see '
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseLiteralExpr(void)
{
	TRY
	{
		consumeToken();	//	'

		ExprAST * literal = parseObjectExpr();
		FAIL(literal == NULL)
		return new LiteralExprAST(literal);
	}
	ENDTRY;
	return NULL;
}

ExprAST *
CCompiler::parseObjectExpr(void)
{
	TRY
	{
		switch (theToken.id)
		{
		// simple-literal
		case TOKENconst:
		case TOKENinteger:
		case TOKENreal:
			{
			ExprAST * constant = new ConstExprAST(theToken);
			consumeToken();
			return constant;
			}

		// path-expression
		case TOKENsymbol:
			{
			std::vector<std::string> elements;
			elements.push_back(SymbolName(theToken.value.ref));
			consumeToken();	//	symbol

			while (theToken.id == '.')
			{
				consumeToken();	// .

				FAILNOT(theToken.id == TOKENsymbol, error("expected symbol in path expression");)
				elements.push_back(SymbolName(theToken.value.ref));
				consumeToken();	// symbol
			}

			return new PathExprAST(elements);
			}

		// array
		case '[':
			{
			consumeToken();	//	[

			std::string className;
			std::vector<ExprAST*> elements;
			if (theToken.id == TOKENsymbol)
			{
				className = SymbolName(theToken.value.ref);
				consumeToken();	// symbol

				if (theToken.id == ':')
				{
					consumeToken();	// :
				}
				else
				{
					elements.push_back(new SymbolAST(className));		// it wasn’t actually a class name
					className.clear();
					if (theToken.id == ',')
						consumeToken();	// ,
				}
			}

			if (theToken.id != ']')
			{
				while (1)
				{
					ExprAST * value = parseObjectExpr();
					FAILNOT(value, error("expected object");)
					elements.push_back(value);

					if (theToken.id == ',')
						consumeToken();	// ,
					else
						break;	//	no more elements
				}
			}

			FAILNOT(theToken.id == ']', error("expected ]");)
			consumeToken();	// ]

			return new ArrayAST(className, elements);
			}

		// frame
		case '{':
			{
			consumeToken();	//	{

			TaggedExprList elements;
			if (theToken.id != '}')
			{
				while (1)
				{
					FAILNOT(theToken.id == TOKENsymbol, error("expected slot name");)
					std::string tag = SymbolName(theToken.value.ref);
					consumeToken();	//	symbol

					FAILNOT(theToken.id == ':', error("expected : after slot name");)
					consumeToken();	//	:

					ExprAST * value = parseObjectExpr();
					FAILNOT(value, error("expected object");)

					elements.push_back(std::make_pair(tag,value));

					if (theToken.id == ',')
						consumeToken();	// ,
					else
						break;	//	no more elements
				}
			}

			FAILNOT(theToken.id == '}', error("expected }");)
			consumeToken();	// }

			return new FrameAST(elements);
			}
		}
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


#pragma mark constructors
/* -----------------------------------------------------------------------------
	array-constructor:
		‘[’ [ symbol : ] [ expression [ , expression ]* [ , ] ] ‘]’
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseArrayConstructor(void)
{
	TRY
	{
		consumeToken();	//	[

		std::string className;
		std::vector<ExprAST*> elements;
		if (theToken.id == TOKENsymbol)
		{
			className = SymbolName(theToken.value.ref);
			consumeToken();	// symbol

			if (theToken.id == ':')
			{
				consumeToken();	// :
			}
			else
			{
				elements.push_back(new SymbolAST(className));		// it wasn’t actually a class name
				className.clear();
				if (theToken.id == ',')
					consumeToken();	// ,
			}
		}

		if (theToken.id != ']')
		{
			while (1)
			{
				ExprAST * value = parseExpression();
				if (value == NULL) break;
				elements.push_back(value);

				if (theToken.id == ',')
					consumeToken();	// ,
				else
					break;	//	no more elements
			}
		}

		FAILNOT(theToken.id == ']', error("expected ]");)
		consumeToken();	// ]

		return new ArrayAST(className, elements);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	frame-constructor:
		‘{’ [ frame-constructor-slot [ , frame-constructor-slot ]* [ , ] ] ‘}’
	frame-constructor-slot:
		symbol : expression
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseFrameConstructor(void)
{
	TRY
	{
		consumeToken();	//	{

		TaggedExprList elements;
		if (theToken.id != '}')
		{
			while (1)
			{
				FAILNOT(theToken.id == TOKENsymbol, error("expected slot name");)
				std::string tag = SymbolName(theToken.value.ref);
				consumeToken();	//	symbol

				FAILNOT(theToken.id == ':', error("expected : after slot name");)
				consumeToken();	//	:

				ExprAST * value = parseExpression();
				FAILNOT(value, error("expected expression");)

				elements.push_back(std::make_pair(tag,value));

				if (theToken.id == ',')
					consumeToken();	// ,
				else
					break;	//	no more elements
			}
		}

		FAILNOT(theToken.id == '}', error("expected }");)
		consumeToken();	// }

		return new FrameAST(elements);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	function-constructor:
		func [ native ] ( [formal-argument-list ] ) expression
	formal-argument-list:
		{formal-argument [ , formal-argument ]*
	formal-argument:
		[ [type ] symbol
	type:
		{ int | array }
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseFunctionConstructor(void)
{
	TRY
	{
		consumeToken();	//	func

		bool isNative = false;
		if (theToken.id == TOKENnative)
		{
			consumeToken();	//	native
			isNative = true;
		}

		std::vector<std::pair<int,std::string> > args;
		FAIL(!parseFormalArgumentList(args))

		ExprAST * body = parseExpression();
		FAIL(body == NULL)

		return new FuncExprAST(isNative, args, body);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


bool
CCompiler::parseFormalArgumentList(std::vector<std::pair<int,std::string> > & args)
{
	TRY
	{
		FAILNOT(theToken.id == '(', error("expected (");)
		consumeToken();	// (

		if (theToken.id != ')')
		{
			while (1)
			{
				int typeSpec = parseType();	// consumes type if specified

				FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
				args.push_back(std::make_pair(typeSpec,SymbolName(theToken.value.ref)));
				consumeToken();	//	symbol

				if (theToken.id == ',')
					consumeToken();	// ,
				else
					break;	//	no more args
			}
		}

		FAILNOT(theToken.id == ')', error("expected )");)
		consumeToken();	// )

		return true;
	}
	ENDTRY;
	// if we get here we failed
	return false;
}


int
CCompiler::parseType(void)
{
	int typeSpec = 0;		// should enum this somewhere
	if (theToken.id == TOKENsymbol)
	{
		char * typeSym = SymbolName(theToken.value.ref);
		if (strcmp(typeSym, "int") == 0)				// int is not a reserved word
		{
			consumeToken();	//	int
			typeSpec = 1;
		}
		else if (strcmp(typeSym, "array") == 0)	// array is not a reserved word
		{
			consumeToken();	//	array
			typeSpec = 2;
		}
	}
	return typeSpec;
}


#pragma mark function-call
/* -----------------------------------------------------------------------------
	symbol -> variable identifier
	symbol ( [ actual-argument-list ] ) -> function call
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseSymbolExpr(void)
{
	TRY
	{
		ExprAST * sym = new SymbolAST(SymbolName(theToken.value.ref));
		consumeToken();	//	symbol
		FAIL(sym == NULL)

		if (theToken.id == '(')
		{
			std::vector<ExprAST*> args;
			FAIL(!parseActualArgumentList(args))

			return new CallExprAST(sym, args);
		}
		return sym;
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	function-call:
		{ symbol ( [ actual-argument-list ] ) | call expression with ( [ actual-argument-list ] ) }
	actual-argument-list:
		expression [ , expression ]*
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseCallExpr(void)
{
	TRY
	{
		consumeToken();	//	call

		ExprAST * func = parseExpression();
		FAIL(func == NULL)

		FAILNOT(theToken.id == TOKENwith, error("expected with after call");)
		consumeToken();	//	with

		std::vector<ExprAST*> args;
		FAIL(!parseActualArgumentList(args))

		return new CallExprAST(func, args);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


bool
CCompiler::parseActualArgumentList(std::vector<ExprAST*> & args)
{
	TRY
	{
		FAILNOT(theToken.id == '(', error("expected (");)
		consumeToken();	// (

		if (theToken.id != ')')
		{
			while (1)
			{
				ExprAST * arg = parseExpression();
				FAIL(arg == NULL)
				args.push_back(arg);

				if (theToken.id == ',')
					consumeToken();	// ,
				else
					break;	//	no more args
			}
		}

		FAILNOT(theToken.id == ')', error("expected )");)
		consumeToken();	// )

		return true;
	}
	ENDTRY;
	// if we get here we failed
	return false;
}


#pragma mark if-expression
/* -----------------------------------------------------------------------------
	if-expression:
		if expression then expression [ ; ] [ else expression ]
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseIfExpr(void)
{
	TRY
	{
		consumeToken();	//	if

		ExprAST * condition = parseExpression();
		FAIL(condition == NULL)

		FAILNOT(theToken.id == TOKENthen, error("expected then after if");)
		consumeToken();	//	then

		ExprAST * ifBody = parseExpression();
		FAIL(ifBody == NULL)

		if (theToken.id == ';')
			consumeToken();	//	;	ignore trailing ;

		ExprAST * elseBody = NULL;
		if (theToken.id == TOKENelse)
		{
			consumeToken();	//	else
			FAIL((elseBody = parseExpression()) == NULL)
		}

		return new IfExprAST(condition, ifBody, elseBody);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


#pragma mark iteration
/* -----------------------------------------------------------------------------
	iteration:
		{ infinite-loop | for-loop | foreach-loop | while-loop | repeat-loop }
--------------------------------------------------------------------------------
	infinite-loop:
		loop expression
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseLoopExpr(void)
{
	TRY
	{
		consumeToken();	//	loop

		ExprAST * body = parseExpression();
		FAIL(body == NULL)

		return new LoopExprAST(body);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	for-loop:
		for symbol := expression to expression [ by expression ] do expression
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseForExpr(void)
{
	TRY
	{
		consumeToken();	//	for

		FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
		std::string forVar = SymbolName(theToken.value.ref);
		consumeToken();	//	symbol

		FAILNOT(theToken.id == TOKENassign, error("expected := after for");)
		consumeToken();	//	:=

		ExprAST * start = parseExpression();
		FAILNOT(start, error("expected for start value");)

		FAILNOT(theToken.id == TOKENto, error("expected to after for");)
		consumeToken();	//	to

		ExprAST * stop = parseExpression();
		FAILNOT(stop, error("expected for stop value");)

		ExprAST * step = NULL;
		if (theToken.id == TOKENby)
		{
			consumeToken();	//	by
			FAILNOT(step = parseExpression(), error("expected for increment");)
		}

		FAILNOT(theToken.id == TOKENdo, error("expected do after for");)
		consumeToken();	//	to

		ExprAST * body = parseExpression();
		FAIL(body == NULL)

		return new ForExprAST(forVar, start, stop, step, body);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	foreach-loop:
		foreach symbol [ , symbol ] [ deeply ] in expression { do | collect } expression
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseForeachExpr(void)
{
	TRY
	{
		consumeToken();	//	foreach

		FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
		std::string forVar = SymbolName(theToken.value.ref);
		consumeToken();	//	symbol

		std::string indexVar = NULL;
		if (theToken.id == ',')
		{
			consumeToken();	//	,

			FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
			indexVar = SymbolName(theToken.value.ref);
			consumeToken();	//	symbol
		}

		bool isDeeply = false;
		if (theToken.id == TOKENdeeply)
		{
			consumeToken();	//	deeply
			isDeeply = true;
		}

		FAILNOT(theToken.id == TOKENin, error("expected in after foreach");)
		consumeToken();	//	in

		ExprAST * iter = parseExpression();
		FAILNOT(iter, error("expected an object to iterate over");)

		bool doCollect = false;
		if (theToken.id == TOKENdo)
		{
			consumeToken();	//	do
		}
		else if (theToken.id == TOKENsymbol && strcmp(SymbolName(theToken.value.ref), "collect") == 0)	// collect is not a reserved word
		{
			consumeToken();	//	collect
			doCollect = true;
		}

		ExprAST * body = parseExpression();
		FAIL(body == NULL)

		return new ForeachExprAST(forVar, indexVar, isDeeply, iter, doCollect, body);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	while-loop:
		while expression do expression
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseWhileExpr(void)
{
	TRY
	{
		consumeToken();	//	while

		ExprAST * condition = parseExpression();
		FAIL(condition == NULL)

		FAILNOT(theToken.id == TOKENdo, error("expected do after while");)
		consumeToken();	//	do

		ExprAST * body = parseExpression();
		FAIL(body == NULL)

		return new WhileExprAST(condition, body);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	repeat-loop:
		repeat expression-sequence until expression
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseRepeatExpr(void)
{
	TRY
	{
		consumeToken();	//	repeat

		ExprAST * body = parseExpressionSequence(TOKENuntil);
		FAIL(body == NULL)

		FAILNOT(theToken.id == TOKENuntil, error("expected until after repeat");)
		consumeToken();	//	until

		ExprAST * condition = parseExpression();
		FAIL(condition == NULL)

		return new RepeatExprAST(condition, body);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


#pragma mark break-expression
/* -----------------------------------------------------------------------------
	break-expression:
		break [ expression ]
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseBreakExpr(void)
{
	TRY
	{
		consumeToken();	//	break

		ExprAST * value = parseExpression();	// may be NULL

		return new BreakExprAST(value);
	}
	ENDTRY;
	// TRY/ENDTRY redundant in this case
	return NULL;
}


#pragma mark return-expression
/* -----------------------------------------------------------------------------
	return-expression:
		return [ expression ]
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseReturnExpr(void)
{
	TRY
	{
		consumeToken();	//	return

		ExprAST * value = parseExpression();	// may be NULL

		return new ReturnExprAST(value);
	}
	ENDTRY;
	// TRY/ENDTRY redundant in this case
	return NULL;
}


#pragma mark try-expression
/* -----------------------------------------------------------------------------
	try-expression:
		try expression-sequence [ onexception symbol do expression [ ; ] ]+
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseTryExpr(void)
{
	TRY
	{
		consumeToken();	//	try

		ExprAST * body = parseExpressionSequence(TOKENonexception);
		FAIL(body == NULL)

		TaggedExprList handlers;
		while (theToken.id == TOKENonexception)
		{
			consumeToken();	//	onexception

			FAILNOT(theToken.id == TOKENsymbol, error("expected exception symbol");)
			std::string symbol = SymbolName(theToken.value.ref);
			consumeToken();	//	symbol

			FAILNOT(theToken.id == TOKENdo, error("expected do after onexception");)
			consumeToken();	//	do

			ExprAST * handler = parseExpression();
			FAIL(handler == NULL)

			handlers.push_back(std::make_pair(symbol,handler));

			if (theToken.id == ';')
				consumeToken();	//	;	ignore trailing ;		-- is this right? can we ditch the processing in getToken?
		}
		FAILIF(handlers.empty(), error("expected exception handler");)

		return new TryExprAST(body, handlers);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


#pragma mark declarations
/* -----------------------------------------------------------------------------
	global-declaration:
		{ global initialization-clause | global-function-decl }
	global-function-decl:
		{ global | func } symbol ( [ formal-argument-list ] ) expression
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseGlobalDecl(void)
{
	TRY
	{
		consumeToken();	//	global | func

		FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
		std::string symbol = SymbolName(theToken.value.ref);
		consumeToken();	//	symbol

		if (theToken.id == '(')
		{
			// we have a global-function-decl
			std::vector<std::pair<int,std::string> > args;
			FAIL(!parseFormalArgumentList(args))
			ExprAST * body = parseExpression();
			FAILNOT(body, error("expected function body");)
			return new GlobalFuncAST(symbol, args, body);
		}

		ExprAST * decl;
		FAIL(!parseInitClause(decl))
		return new GlobalDeclAST(symbol, decl);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


/* -----------------------------------------------------------------------------
	local-declaration:
		local [ type-specifier ] initalization-clause [ , initalization-clause ]*
	type-specifier:
		{ int | array }
	initialization-clause:
		symbol [ := expression ]
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseLocalDecl(void)
{
	TRY
	{
		consumeToken();	//	local

		int typeSpec = parseType();

		std::vector<std::pair<std::string, ExprAST*> > declarations;
		while (1)
		{
			FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
			std::string symbol = SymbolName(theToken.value.ref);
			consumeToken();	//	symbol

			ExprAST * decl;
			FAIL(!parseInitClause(decl))
			declarations.push_back(std::make_pair(symbol, decl));

			if (theToken.id != ',')
				break;	//	no more initialization-clauses
		}

		return new LocalDeclAST(typeSpec, declarations);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}


bool
CCompiler::parseInitClause(ExprAST * &value)
{
	TRY
	{
		value = NULL;
		if (theToken.id == TOKENassign)
		{
			consumeToken();	//	:=

			value = parseExpression();
			FAILNOT(value, error("expected expression");)
		}

		return true;
	}
	ENDTRY;
	// if we get here we failed
	return false;
}


/* -----------------------------------------------------------------------------
	constant-declaration:
		constant constant-init-clause [ , constant-init-clause ]*
	constant-init-clause:
		symbol := expression
----------------------------------------------------------------------------- */
ExprAST *
CCompiler::parseConstantDecl(void)
{
	TRY
	{
		consumeToken();	//	constant

		TaggedExprList declarations;
		while (1)
		{
			FAILNOT(theToken.id == TOKENsymbol, error("expected identifier");)
			std::string tag = SymbolName(theToken.value.ref);
			consumeToken();	//	symbol

			FAILNOT(theToken.id == TOKENassign, error("expected := after constant");)
			consumeToken();	//	:=

			ExprAST * value = parseExpression();
			FAILNOT(value, error("expected expression");)

			declarations.push_back(std::make_pair(tag,value));

			if (theToken.id != ',')
				break;	//	no more constant-init-clauses
		}

		return new ConstantDeclAST(declarations);
	}
	ENDTRY;
	// if we get here we failed
	return NULL;
}
