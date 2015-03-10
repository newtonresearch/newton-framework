/*
	File:		Compiler.cc

	Contains:	The NewtonScript compiler.
					The compiler uses yacc to generate the parser,
					but we need to tweak its output (y.tab.c) so…
					if NewtonScript.y is changed
						enable it and rebuild
						move y.tab.c from Derived Sources to Frames
						cut switch from yyparse() and paste into CCompiler::parser()
						disable NewtonScript.y for next build

	Written by:	The Newton Tools group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "Iterators.h"
#include "Arrays.h"
#include "Frames.h"
#include "Lookup.h"
#include "Symbols.h"
#include "Funcs.h"

#include "Compiler.h"
#include "REPTranslators.h"
#include "Unicode.h"
#include "UStringUtils.h"

DeclareException(exBadType, exRootException);
DeclareException(exCompilerData, exRootException);


/*------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------*/

ULong			gNewtonScriptVersion = 1;

int			gCompilerCompatibility = 0;		// 0C1051EC
Ref			gFreqFuncNames = NILREF;			// 0C1051F0 frame of offsets into gFreqFuncInfo
FreqFuncDef	gFreqFuncInfo[25] = {				// 0C1051F4
				{ "+", 2 },
				{ "-", 2 },
				{ "aref", 2 },
				{ "setAref", 3 },
				{ "=", 2 },
				{ "not", 1 },
				{ "<>", 2 },
				{ "*", 2 },
				{ "/", 2 },
				{ "div", 2 },
				{ "<", 2 },
				{ ">", 2 },
				{ ">=", 2 },
				{ "<=", 2 },
				{ "band", 2 },
				{ "bor", 2 },
				{ "bnot", 1 },							// original says 2!
				{ "newIterator", 2 },
				{ "length", 1 },
				{ "clone", 1 },
				{ "setClass", 2 },
				{ "addArraySlot", 2 },
				{ "stringer", 1 },
				{ "hasPath", 2 },
				{ "classOf", 1 } };
ArrayIndex	gNumFreqFuncs = 25;					// 0C1052BC
Ref			gConstantsFrame = NILREF;			// 0C1052C0 global constants
Ref *			RSgConstantsFrame = &gConstantsFrame;
Ref			gConstFuncFrame = NILREF;			// 0C1052C4 global constant functions
#if defined(hasPureFunctionSupport)
Ref *			RSgConstFuncFrame = &gConstFuncFrame;
#endif
int			gCompilerIsInitialized = NO;		// 0C1052C8


//	yacc/bison stuff

#include "y.tab.h"

#define kInitYaccStackSize 64
#define kTokenInvalid	911

//#define YYDEBUG 1
#define YYSTYPE Ref
#define YYERRCODE 256

extern short yylhs[];
extern short yylen[];
extern short yydefred[];
extern short yydgoto[];
extern short yysindex[];
extern short yyrindex[];
extern short yygindex[];
extern short yytable[];
extern short yycheck[];
extern char *yyname[];	// should be only for YYDEBUG, but we use ’em for reporting syntax errors
#if YYDEBUG
extern char *yyrule[];
#endif

#define YYTABLESIZE 4392
#define YYFINAL 31
#define YYMAXTOKEN 312

int		yydebug = 0;	// +00	debug level		0C105574
int		yynerrs;			// +04	number of errors
int		yyerrflag;		// +08	error flag
int		yychar;			// +0C	token type
int		yystate;			// +10	state
short *	yyssp;			// +14	state stack ptr
YYSTYPE *yyvsp;			// +18	value stack ptr
YYSTYPE	yyval;			// +1C	$$
YYSTYPE	yylval;			// +20	token value
short *	yyss;				// +24	state stack base

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab


/*------------------------------------------------------------------------------
	R e s e r v e d   W o r d s
------------------------------------------------------------------------------*/

struct ReservedWord
{
	const char *	word;
	int				token;
};

static const ReservedWord gReservedWords[] =
{
	{ "and",				kTokenAnd },
	{ "begin",			kTokenBegin },
	{ "break",			kTokenBreak },
	{ "by",				kTokenBy },
	{ "call",			kTokenCall },
	{ "constant",		kTokenConstant },
	{ "deeply",			kTokenDeeply },
	{ "div",				kTokenDiv },
	{ "do",				kTokenDo },
	{ "else",			kTokenElse },
	{ "end",				kTokenEnd },
	{ "exists",			kTokenExists },
	{ "for",				kTokenFor },
	{ "foreach",		kTokenForeach },
	{ "func",			kTokenFunc },
	{ "global",			kTokenGlobal },
	{ "if",				kTokenIf },
	{ "in",				kTokenIn },
	{ "inherited",		kTokenInherited },
	{ "local",			kTokenLocal },
	{ "loop",			kTokenLoop },
	{ "mod",				kTokenMod },
	{ "native",			kTokenNative },
	{ "not",				kTokenNot },
	{ "onexception",	kTokenOnexception },
	{ "or",				kTokenOr },
	{ "repeat",			kTokenRepeat },
	{ "return",			kTokenReturn },
	{ "self",			kTokenSelf },
	{ "then",			kTokenThen },
	{ "to",				kTokenTo },
	{ "try",				kTokenTry },
	{ "until",			kTokenUntil },
	{ "while",			kTokenWhile },
	{ "with", 			kTokenWith }
};

static int		ReservedWordToken(const char * word);


/*------------------------------------------------------------------------------
	P l a i n   C   F u n c t i o n   I n t e r f a c e
------------------------------------------------------------------------------*/
Ref	ParseString(RefArg inStr);
Ref	ParseFile(const char * inFilename);

extern "C" Ref
FCompile(RefArg inRcvr, RefArg inStr)
{
	return ParseString(inStr);
}


extern "C" Ref
FLoad(RefArg inRcvr, RefArg inFilenameStr)
{
	// convert Unicode string to ASCII
	RefVar asciiStr(ASCIIString(inFilenameStr));
	CDataPtr filename(asciiStr);
	return ParseFile((const char *)filename);
}


/*------------------------------------------------------------------------------
	P u b l i c   I n t e r f a c e
------------------------------------------------------------------------------*/

Ref
ParseString(RefArg inStr)
{
	CStringInputStream	stream(inStr);
	CCompiler				compiler(&stream);
	Ref						result;

	newton_try
	{
		result = compiler.compile();
	}
	cleanup
	{
		compiler.~CCompiler();
		stream.~CStringInputStream();
	}
	end_try;

	return result;
}


Ref
ParseFile(const char * inFilename)
{
	RefVar result;
	RefVar codeBlock;

	FILE * fd = fopen(inFilename, "r");
	if (fd == NULL)
		ThrowMsg("couldn't open file");

	CStdioInputStream stream(fd, inFilename);
	// original does some mysterious copying of stream
	CCompiler compiler(&stream, YES);

	newton_try
	{
		while (!feof(fd))
		{
			codeBlock = compiler.compile();

			if (NOTNIL(GetFrameSlot(RA(gVarFrame), MakeSymbol("showCodeBlocks"))))
			{
				gREPout->print("(#%X) ", (Ref)codeBlock);
				PrintObject(codeBlock, 0);
				gREPout->print("\n");
			}

			result = NOTNIL(codeBlock) ? InterpretBlock(codeBlock, RA(NILREF)) : NILREF;

			if (NOTNIL(GetFrameSlot(RA(gVarFrame), MakeSymbol("showLoadResults"))))
			{
				gREPout->print("(#%-6X) ", (Ref)result);
				PrintObject(result, 0);
				gREPout->print("\n");
			}
		}
	}
	newton_catch(exRefException)
	{
		RefVar data(*(RefStruct *)CurrentException()->data);
		if (IsFrame(data)
		&&  ISNIL(GetFrameSlot(data, SYMA(filename))))
		{
			SetFrameSlot(data, SYMA(filename), MakeStringFromCString(stream.fileName()));
			SetFrameSlot(data, SYMA(lineNumber), MAKEINT(stream.lineNumber()));
		}
		gREPout->exceptionNotify(CurrentException());
	}
	newton_catch_all
	{
		gREPout->exceptionNotify(CurrentException());
	}
	end_try;

	fclose(fd);
	return result;
}


#pragma mark -

/*------------------------------------------------------------------------------
	Node construction.
	The output of the parser is a valid stream of nodes.
	Each node is a Parameterised Token.
------------------------------------------------------------------------------*/

Ref
AllocatePT1(int inToken, RefArg inP1)
{
	RefVar pt(MakeArray(2));
	SetArraySlot(pt, 0, MAKEINT(inToken));
	SetArraySlot(pt, 1, inP1);
	return pt;
}

Ref
AllocatePT2(int inToken, RefArg inP1, RefArg inP2)
{
	RefVar pt(MakeArray(3));
	SetArraySlot(pt, 0, MAKEINT(inToken));
	SetArraySlot(pt, 1, inP1);
	SetArraySlot(pt, 2, inP2);
 	return pt;
}

Ref
AllocatePT3(int inToken, RefArg inP1, RefArg inP2, RefArg inP3)
{
	RefVar pt(MakeArray(4));
	SetArraySlot(pt, 0, MAKEINT(inToken));
	SetArraySlot(pt, 1, inP1);
	SetArraySlot(pt, 2, inP2);
	SetArraySlot(pt, 3, inP3);
	return pt;
}

Ref
AllocatePT5(int inToken, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5)
{
	RefVar pt(MakeArray(6));
	SetArraySlot(pt, 0, MAKEINT(inToken));
	SetArraySlot(pt, 1, inP1);
	SetArraySlot(pt, 2, inP2);
	SetArraySlot(pt, 3, inP3);
	SetArraySlot(pt, 4, inP4);
	SetArraySlot(pt, 5, inP5);
	return pt;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C C o m p i l e r

	Construct a compiler with an input stream.
	There can be only one instance of the compiler since it uses yacc
	globals.
	Args:		inStream				the input stream (string or file)
				inDoFirstCmdOnly	YES => stop compiling when first ';'
												 encountered. (Why?)
------------------------------------------------------------------------------*/

CCompiler::CCompiler(CInputStream * inStream, BOOL inDoFirstCmdOnly)
{
	func = nil;
	stackSize = kInitYaccStackSize;
	stream = inStream;
	yaccStack = AllocateArray(SYMA(yaccStack), kInitYaccStackSize);
	sStack = (short *) malloc(kInitYaccStackSize * sizeof(short));
	do1CmdOnly = inDoFirstCmdOnly;
	funcDepthPtr = NULL;
	isStackedToken = NO;
	stackedTokenValue = NILREF;
	LockRef(yaccStack);
	vStack = Slots(yaccStack);
	if (!gCompilerIsInitialized)
	{
		gCompilerIsInitialized = YES;
		AddGCRoot(&yyval);
		AddGCRoot(&yylval);

		AddGCRoot(&gConstFuncFrame);
		gConstFuncFrame = GetFrameSlot(gVarFrame, SYMA(constantFunctions));
		if (ISNIL(gConstFuncFrame))
		{
			gConstFuncFrame = AllocateFrame();
			SetFrameSlot(gVarFrame, SYMA(constantFunctions), gConstFuncFrame);
		}

		AddGCRoot(&gFreqFuncNames);
		gFreqFuncNames = AllocateFrame();
		for (ArrayIndex i = 0; i < gNumFreqFuncs; i++)
			SetFrameSlot(gFreqFuncNames, MakeSymbol(gFreqFuncInfo[i].name), MAKEINT(i));
	}
}


/*------------------------------------------------------------------------------
	Destroy the compiler.
	Unlock the yacc parser stack.
------------------------------------------------------------------------------*/
CCompiler::~CCompiler()
{
	UnlockRef(yaccStack);
}


/*------------------------------------------------------------------------------
	Compile the input stream.
	Args:		--
	Return:	a function object
------------------------------------------------------------------------------*/

Ref
CCompiler::compile(void)
{
	func = nil;
	funcDepthPtr = NULL;

	NewtonErr	err = parser();
	RefVar		graph(AllocatePT1(kTokenBegin, yyval));
	yyval = NILREF;
	yylval = NILREF;
	if (err)
		return NILREF;

	Ref cc = GetFrameSlot(gVarFrame, SYMA(compilerCompatibility));
	gCompilerCompatibility = ISINT(cc) ? RINT(cc) : 1;

	newFunctionState(MakeArray(0), func, funcDepthPtr);
	RefVar codeblock;

	unwind_protect
	{
		walkForDeclarations(graph);
		simplify(graph);
		walkForClosures(graph);
		ArrayIndex graphLen = Length(GetArraySlot(graph, 1));
		if (graphLen > 0)
		{
			RefVar finalPT(GetArraySlot(GetArraySlot(graph, 1), graphLen - 1));
			if (RINT(GetArraySlot(finalPT, 0)) == kTokenEQL)
				warning("= at top level... did you mean := ?");
		}
		func->copyClosedArgs();
		walkForCode(graph, NO);
		codeblock = endFunction();
	}
	on_unwind
	{
		if (func != nil)	// ?? appears to be +04 rather than +00
			func->~CFunctionState();
	}
	end_unwind;

	return codeblock;
}

#pragma mark -

/*------------------------------------------------------------------------------
	L e x i c a l  A n a l y s i s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Parse the input stream.
	This function is copied from the standard yacc output function
	yyparse() in file y.tab.c. It has been tweaked a bit to cope with
	our own parser stack (of Refs).
	Args:		--
	Return:	yyparse status code
				Global yyval contains a valid array of tokens.
------------------------------------------------------------------------------*/

int
CCompiler::parser(void)
{
	int		yym, yyn;
#if YYDEBUG
	char *	yys;
#endif

	yynerrs = 0;
	yyerrflag = 0;
	yychar = -1;
	yyssp = yyss = sStack;
	yystate = 0;
	yyvsp = vStack;
	*sStack = 0;

yyloop:
	if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
	if (yychar < 0)
	{
		if ((yychar = getToken()) < 0) yychar = 0;
#if YYDEBUG
		if (yydebug)
		{
			yys = 0;
			if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
			if (!yys) yys = "illegal-symbol";
			printf("yydebug: state %d, reading %d (%s)\n",
					  yystate, yychar, yys);
		}
#endif
	}

	if ((yyn = yysindex[yystate])
	&&  (yyn += yychar) >= 0
	&&   yyn <= YYTABLESIZE
	&&   yycheck[yyn] == yychar)
	{
#if YYDEBUG
		if (yydebug)
			printf("yydebug: state %d, shifting to state %d\n",
					  yystate, yytable[yyn]);
#endif
		if (yyssp >= &yyss[stackSize-1] && parserStackOverflow()) goto yyoverflow;
		*++yyssp = yystate = yytable[yyn];
		*++yyvsp = yylval;
		yychar = -1;
		if (yyerrflag > 0) --yyerrflag;
		goto yyloop;
	}

	if ((yyn = yyrindex[yystate])
	&&  (yyn += yychar) >= 0
	&&   yyn <= YYTABLESIZE
	&&   yycheck[yyn] == yychar)
	{
		yyn = yytable[yyn];
		goto yyreduce;
	}

	if (yyerrflag == 0)
	{
		syntaxError("syntax error");
		++yynerrs;
	}

	if (yyerrflag < 3)
	{
		yyerrflag = 3;
		for ( ; ; )
		{
			if ((yyn = yysindex[*yyssp])
			&&  (yyn += YYERRCODE) >= 0
			&&   yyn <= YYTABLESIZE
			&&   yycheck[yyn] == YYERRCODE)
			{
#if YYDEBUG
				if (yydebug)
					printf("yydebug: state %d, error recovery shifting to state %d\n",
							 (int)*yyssp, yytable[yyn]);
#endif
				if (yyssp >= &yyss[stackSize-1] && parserStackOverflow()) goto yyoverflow;
				*++yyssp = yystate = yytable[yyn];
				*++yyvsp = yylval;
				goto yyloop;
			}
			else
			{
#if YYDEBUG
				if (yydebug)
					printf("yydebug: error recovery discarding state %d\n",
							 (int)*yyssp);
#endif
				if (yyssp <= yyss) goto yyabort;
				--yyssp;
				--yyvsp;
			}
		}
	}
	else
	{
		if (yychar == 0) goto yyabort;
#if YYDEBUG
		if (yydebug)
		{
			yys = 0;
			if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
			if (!yys) yys = "illegal-symbol";
			printf("yydebug: state %d, error recovery discards token %d (%s)\n",
					  yystate, yychar, yys);
		}
#endif
		yychar = -1;
		goto yyloop;
	}

yyreduce:
#if YYDEBUG
	if (yydebug)
		printf("yydebug: state %d, reducing by rule %d (%s)\n",
				  yystate, yyn, yyrule[yyn]);
#endif
	yym = yylen[yyn];
	yyval = yyvsp[1-yym];

//•••••• PASTE SWITCH FROM y.tab.c : yyparse HERE
    switch (yyn)
    {
case 1:
					{	yyval = MakeArray(0); }
break;
case 3:
					{	yyval = MakeArray(1);
						SetArraySlot(yyval, 0, yyvsp[0]); }
break;
case 4:
					{	yyval = yyvsp[-1];  if (do1CmdOnly) YYACCEPT; }
break;
case 5:
					{	yyval = yyvsp[-3];  AddArraySlot(yyval, yyvsp[0]); }
break;
case 11:
					{	yyval = AllocatePT1(kTokenSelf, RA(NILREF)); }
break;
case 12:
					{	yyval = AllocatePT1(kTokenBegin, yyvsp[-1]); }
break;
case 13:
					{	yyval = yyvsp[-1]; }
break;
case 14:
					{	yyval = AllocatePT2('+', yyvsp[-2], yyvsp[0]); }
break;
case 15:
					{	yyval = AllocatePT2('-', yyvsp[-2], yyvsp[0]); }
break;
case 16:
					{	yyval = AllocatePT2('*', yyvsp[-2], yyvsp[0]); }
break;
case 17:
					{	yyval = AllocatePT2('/', yyvsp[-2], yyvsp[0]); }
break;
case 18:
					{	yyval = AllocatePT2(kTokenDiv, yyvsp[-2], yyvsp[0]); }
break;
case 19:
					{	yyval = AllocatePT2('&', yyvsp[-2], yyvsp[0]); }
break;
case 20:
					{	yyval = AllocatePT2(kTokenAmperAmper, yyvsp[-2], yyvsp[0]); }
break;
case 21:
					{	yyval = AllocatePT2(kTokenMod, yyvsp[-2], yyvsp[0]); }
break;
case 22:
					{	yyval = AllocatePT1(kTokenUMinus, yyvsp[0]); }
break;
case 23:
					{	yyval = AllocatePT2(kTokenLShift, yyvsp[-2], yyvsp[0]); }
break;
case 24:
					{	yyval = AllocatePT2(kTokenRShift, yyvsp[-2], yyvsp[0]); }
break;
case 25:
					{	yyval = AllocatePT2('<', yyvsp[-2], yyvsp[0]); }
break;
case 26:
					{	yyval = AllocatePT2('>', yyvsp[-2], yyvsp[0]); }
break;
case 27:
					{	yyval = AllocatePT2(kTokenLEQ, yyvsp[-2], yyvsp[0]); }
break;
case 28:
					{	yyval = AllocatePT2(kTokenGEQ, yyvsp[-2], yyvsp[0]); }
break;
case 29:
					{	yyval = AllocatePT2(kTokenEQL, yyvsp[-2], yyvsp[0]); }
break;
case 30:
					{	yyval = AllocatePT2(kTokenNEQ, yyvsp[-2], yyvsp[0]); }
break;
case 31:
					{	yyval = AllocatePT2(kTokenAnd, yyvsp[-2], yyvsp[0]); }
break;
case 32:
					{	yyval = AllocatePT2(kTokenOr, yyvsp[-2], yyvsp[0]); }
break;
case 33:
					{	yyval = AllocatePT1(kTokenNot, yyvsp[0]); }
break;
case 34:
					{	yyval = AllocatePT1(kTokenExists, yyvsp[-1]); }
break;
case 35:
					{	yyval = AllocatePT1(kTokenExists, yyvsp[-1]); }
break;
case 47:
					{	/* 1024 */
						yyval = AllocatePT1(kTokenConst, yyvsp[0]); }
break;
case 48:
					{	yyval = AllocatePT1(kTokenConst, yyvsp[0]); }
break;
case 49:
					{	yyval = AllocatePT1(kTokenConst, yyvsp[0]); }
break;
case 50:
					{	yyval = AllocatePT1(kTokenConst, yyvsp[0]); }
break;
case 51:
					{	yyval = AllocatePT1(kTokenConst, yyvsp[0]); }
break;
case 52:
					{	/* 1079 */
						yyval = AllocatePT1(kTokenSymbol, yyvsp[0]); }
break;
case 53:
					{	yyval = AllocatePT2('.', yyvsp[-4], yyvsp[-1]); }
break;
case 54:
					{	yyval = AllocatePT2('.', yyvsp[-2], AllocatePT1(kTokenConst, yyvsp[0])); }
break;
case 55:
					{	yyval = AllocatePT2('[', yyvsp[-3], yyvsp[-1]); }
break;
case 56:
					{	/* 1152 */
						yyval = AllocatePT2(kTokenAssign, yyvsp[-2], yyvsp[0]); }
break;
case 57:
					{	/* 1171 */
						yyval = AllocatePT2(kTokenLocal, GetArraySlot(yyvsp[0], 0), GetArraySlot(yyvsp[0], 1)); }
break;
case 58:
					{	/* 1194 */
						yyval = AllocatePT1(kTokenConstant, yyvsp[0]); }
break;
case 59:
					{	/* 1205 */
						yyval = AllocatePT2(kTokenGlobal, yyvsp[0], AllocatePT1(kTokenConst, RA(NILREF))); }
break;
case 60:
					{	yyval = AllocatePT2(kTokenGlobal, yyvsp[-2], yyvsp[0]); }
break;
case 61:
					{	/* 1251 - CHECK THIS… */
						RefVar	fn(MakeArray(2));
						SetArraySlot(fn, 0, AllocatePT1(kTokenConst, yyvsp[-4]));
						SetArraySlot(fn, 1, AllocatePT5(kTokenFunc, GetArraySlot(yyvsp[-2], 0), yyvsp[0], RA(NILREF), GetArraySlot(yyvsp[-2], 1), RA(NILREF)));
						yyval = AllocatePT2(kTokenCall, SYMA(DefGlobalFn), fn); }
break;
case 62:
					{	/* …AND THIS */
						RefVar	fn(MakeArray(2));
						SetArraySlot(fn, 0, AllocatePT1(kTokenConst, yyvsp[-4]));
						SetArraySlot(fn, 1, AllocatePT5(kTokenFunc, GetArraySlot(yyvsp[-2], 0), yyvsp[0], RA(NILREF), GetArraySlot(yyvsp[-2], 1), RA(NILREF)));
						yyval = AllocatePT2(kTokenCall, SYMA(DefGlobalFn), fn); }
break;
case 63:
					{	AllocatePT1(kTokenBreak, yyvsp[0]); }
break;
case 64:
					{	AllocatePT1(kTokenBreak, AllocatePT1(kTokenConst, RA(NILREF))); }
break;
case 65:
					{	AllocatePT1(kTokenReturn, yyvsp[0]); }
break;
case 66:
					{	AllocatePT1(kTokenReturn, AllocatePT1(kTokenConst, RA(NILREF))); }
break;
case 67:
					{	/* 1483 */
						yyval = AllocatePT2(kTokenCall, yyvsp[-3], yyvsp[-1]); }
break;
case 68:
					{	yyval = AllocatePT2(kTokenInvoke, yyvsp[-4], yyvsp[-1]); }
break;
case 69:
					{	/* 1520 */
						yyval = AllocatePT2(':', yyvsp[0], yyvsp[-2]); }
break;
case 70:
					{	yyval = AllocatePT2(':', yyvsp[0], AllocatePT1(kTokenSelf, RA(NILREF))); }
break;
case 71:
					{	/* 1563 */
						yyval = AllocatePT3(':', yyvsp[-3], yyvsp[-5], yyvsp[-1]); }
break;
case 72:
					{	yyval = AllocatePT3(':', yyvsp[-3], RA(NILREF), yyvsp[-1]); }
break;
case 73:
					{	yyval = AllocatePT3(':', yyvsp[-3], AllocatePT1(kTokenSelf, RA(NILREF)), yyvsp[-1]); }
break;
case 74:
					{	yyval = AllocatePT3(kTokenSendIfDefined, yyvsp[-3], yyvsp[-5], yyvsp[-1]); }
break;
case 75:
					{	yyval = AllocatePT3(kTokenSendIfDefined, yyvsp[-3], RA(NILREF), yyvsp[-1]); }
break;
case 76:
					{	yyval = AllocatePT3(kTokenSendIfDefined, yyvsp[-3], AllocatePT1(kTokenSelf, RA(NILREF)), yyvsp[-1]); }
break;
case 77:
					{	/* 1735 */
						yyval = AllocatePT3(kTokenIf, yyvsp[-4], yyvsp[-2], yyvsp[0]); }
break;
case 78:
					{	yyval = AllocatePT3(kTokenIf, yyvsp[-2], yyvsp[0], RA(NILREF)); }
break;
case 79:
					{	/* nothing to do here */ }
break;
case 80:
					{	/* nothing to do here */ }
break;
case 81:
					{	/* nothing to do here */ }
break;
case 82:
					{	/* nothing to do here */ }
break;
case 83:
					{	/* nothing to do here */ }
break;
case 84:
					{	/* 1791 */
						yyval = AllocatePT1(kTokenLoop, yyvsp[0]); }
break;
case 85:
					{	/* 1802 */
						yyval = AllocatePT5(kTokenFor, yyvsp[-6], yyvsp[-4], yyvsp[-2], AllocatePT1(kTokenConst, MAKEINT(1)), yyvsp[0]); }
break;
case 86:
					{	yyval = AllocatePT5(kTokenFor, yyvsp[-8], yyvsp[-6], yyvsp[-4], yyvsp[-2], yyvsp[0]); }
break;
case 87:
					{	/* 1899 */
						RefVar	req(MakeArray(2));
						SetArraySlot(req, 0, yyvsp[-7]);
						SetArraySlot(req, 1, yyvsp[-5]);
						yyval = AllocatePT5(kTokenForeach, yyvsp[-1], yyvsp[-2], yyvsp[0], req, yyvsp[-4]); }
break;
case 88:
					{	RefVar	req(MakeArray(1));
						SetArraySlot(req, 0, yyvsp[-5]);
						yyval = AllocatePT5(kTokenForeach, yyvsp[-1], yyvsp[-2], yyvsp[0], req, yyvsp[-4]); }
break;
case 89:
					{	yyval = NILREF; }
break;
case 90:
					{	yyval = TRUEREF; }
break;
case 91:
					{	yyval = RSYMmap; }
break;
case 92:
					{	if (!EQRef(yyvsp[0], RSYMcollect))
							syntaxError("FOREACH requires DO or COLLECT");
						yyval = RSYMcollect; }
break;
case 93:
					{	/* 2034 */
						yyval = AllocatePT2(kTokenWhile, yyvsp[-2], yyvsp[0]); }
break;
case 94:
					{	/* 2053 */
						yyval = AllocatePT2(kTokenRepeat, yyvsp[-2], yyvsp[0]); }
break;
case 95:
					{	/* 2072 */
						yyval = AllocatePT5(kTokenFunc, GetArraySlot(yyvsp[-2], 0), yyvsp[0], RA(NILREF), GetArraySlot(yyvsp[-2], 1), RA(NILREF)); }
break;
case 96:
					{	yyval = AllocatePT5(kTokenFunc, GetArraySlot(yyvsp[-2], 0), yyvsp[0], RA(TRUEREF), GetArraySlot(yyvsp[-2], 1), RA(NILREF)); }
break;
case 97:
					{	yyval = AllocatePT5(kTokenFunc, GetArraySlot(yyvsp[-2], 0), yyvsp[0], RA(TRUEREF), GetArraySlot(yyvsp[-2], 1), RA(NILREF)); }
break;
case 98:
					{	/* 2216 */
						yyval = AllocatePT2(kTokenTry, AllocatePT1(kTokenBegin, yyvsp[-1]), yyvsp[0]); }
break;
case 99:
					{	/* 594 */
						yyval = MakeArray(1);
						SetArraySlot(yyval, 0, yyvsp[0]); }
break;
case 100:
					{	yyval = yyvsp[-1];
						AddArraySlot(yyval, yyvsp[0]); }
break;
case 101:
					{	yyval = AllocatePT2(kTokenOnexception, yyvsp[-2], yyvsp[0]); }
break;
case 102:
					{	yyval = AllocatePT1(kTokenBuildArray, yyvsp[-1]); }
break;
case 103:
					{	yyval = AllocatePT1(kTokenBuildArray, yyvsp[-1]);
						SetClass(yyvsp[-1], yyvsp[-3]); }
break;
case 104:
					{	yyval = AllocatePT1(kTokenBuildFrame, yyvsp[-1]); }
break;
case 105:
					{	/* 590 */
						yyval = MakeArray(0); }
break;
case 107:
					{	/* 594 */
						yyval = MakeArray(1);
						SetArraySlot(yyval, 0, yyvsp[0]); }
break;
case 108:
					{	yyval = yyvsp[-2];
						AddArraySlot(yyval, yyvsp[0]); }
break;
case 109:
					{	/* 590 */
						yyval = MakeArray(0); }
break;
case 110:
					{	/* 594 */
						yyval = MakeArray(1);
						SetArraySlot(yyval, 0, yyvsp[0]); }
break;
case 111:
					{	AddArraySlot(yyvsp[-2], yyvsp[0]);
						yyval = yyvsp[-2]; }
break;
case 112:
					{	/* 2364 */
						yyval = MakeArray(2);
						SetArraySlot(yyval, 0, MakeArray(0));
						SetArraySlot(yyval, 1, MakeArray(0)); }
break;
case 114:
					{	/* 2384 */
						yyval = MakeArray(2);
						RefVar	argNames(MakeArray(1));
						RefVar	argTypes(MakeArray(1));
						SetArraySlot(yyval, 0, argNames);
						SetArraySlot(yyval, 1, argTypes);
						SetArraySlot(argNames, 0, yyvsp[0]); }
break;
case 115:
					{	yyval = MakeArray(2);
						RefVar	argNames(MakeArray(1));
						RefVar	argTypes(MakeArray(1));
						SetArraySlot(yyval, 0, argNames);
						SetArraySlot(yyval, 1, argTypes);
						SetArraySlot(argNames, 0, yyvsp[0]);
						SetArraySlot(argTypes, 0, yyvsp[-1]); }
break;
case 116:
					{	yyval = yyvsp[-2];
						RefVar	argNames(GetArraySlot(yyvsp[-2], 0));
						RefVar	argTypes(GetArraySlot(yyvsp[-2], 1));
						CObjectIterator	iter(argNames);
						for ( ; !iter.done(); iter.next())
						{
							if (EQRef(iter.value(), yyvsp[0]))
							{
								Str255	str;
								sprintf(str, "Duplicate argument name: %s\n", SymbolName(yyvsp[0]));
								warning(str);
								break;
							}
						}
						AddArraySlot(argNames, yyvsp[0]);
						AddArraySlot(argTypes, RA(NILREF)); }
break;
case 117:
					{	yyval = yyvsp[-3];
						RefVar	argNames(GetArraySlot(yyvsp[-3], 0));
						RefVar	argTypes(GetArraySlot(yyvsp[-3], 1));
						CObjectIterator	iter(argNames);
						for ( ; !iter.done(); iter.next())
						{
							if (EQRef(iter.value(), yyvsp[-1]))
							{
								Str255	str;
								sprintf(str, "Duplicate argument name: %s\n", SymbolName(yyvsp[-1]));
								warning(str);
								break;
							}
						}
						AddArraySlot(argNames, yyvsp[0]);
						AddArraySlot(argTypes, yyvsp[-1]); }
break;
case 118:
					{	/* 2607 */
						if (NOTNIL(yyvsp[0]))
						{
							yyval = yyvsp[0];
							SetArraySlot(yyval, 1, yyvsp[-1]);
						}
						else
						{
							yyval = MakeArray(2);
							RefVar	locals(MakeArray(2));
							SetArraySlot(locals, 0, yyvsp[-1]);
							SetArraySlot(yyval, 0, locals);
						} }
break;
case 119:
					{	yyval = MakeArray(2);
						RefVar	locals(MakeArray(2));
						SetArraySlot(locals, 0, yyvsp[-2]);
						SetArraySlot(locals, 1, yyvsp[0]);
						SetArraySlot(yyval, 0, locals); }
break;
case 120:
					{	yyval = yyvsp[-2];
						RefVar	locals(GetArraySlot(yyvsp[-2], 0));
						CObjectIterator	iter(locals);
						for ( ; !iter.done(); iter.next())
						{
							if (EQRef(iter.value(), yyvsp[0]))
							{
								Str255	str;
								sprintf(str, "Duplicate variable name: %s\n", SymbolName(yyvsp[0]));
								warning(str);
								break;
							}
						}
						AddArraySlot(locals, yyvsp[0]);
						AddArraySlot(locals, RA(NILREF)); }
break;
case 121:
					{	yyval = yyvsp[-4];
						RefVar	locals(GetArraySlot(yyvsp[-4], 0));
						CObjectIterator	iter(locals);
						for ( ; !iter.done(); iter.next())
						{
							if (EQRef(iter.value(), yyvsp[-2]))
							{
								Str255	str;
								sprintf(str, "Duplicate variable name: %s\n", SymbolName(yyvsp[-2]));
								warning(str);
								break;
							}
						}
						AddArraySlot(locals, yyvsp[-2]);
						AddArraySlot(locals, yyvsp[0]); }
break;
case 122:
					{	yyval = NILREF; }
break;
case 123:
					{	yyval = MakeArray(2);
						RefVar	locals(MakeArray(2));
						SetArraySlot(locals, 0, yyvsp[0]);
						SetArraySlot(yyval, 0, locals); }
break;
case 124:
					{	yyval = MakeArray(2);
						RefVar	locals(MakeArray(2));
						SetArraySlot(locals, 0, yyvsp[-2]);
						SetArraySlot(locals, 1, yyvsp[0]);
						SetArraySlot(yyval, 0, locals); }
break;
case 125:
					{	/* 2812 */
						yyval = MakeArray(2);
						SetArraySlot(yyval, 0, yyvsp[-2]);
						SetArraySlot(yyval, 1, yyvsp[0]); }
break;
case 126:
					{	yyval = yyvsp[-4];
						AddArraySlot(yyval, yyvsp[-2]);
						AddArraySlot(yyval, yyvsp[0]); }
break;
case 127:
					{	yyval = AllocateFrame(); }
break;
case 129:
					{	/* 2862 */
						yyval = AllocateFrame();
						SetFrameSlot(yyval, yyvsp[-2], yyvsp[0]); }
break;
case 130:
					{	yyval = yyvsp[-4];
						if (FrameHasSlot(yyval, yyvsp[-2]))
						{
							Str255	str;
							sprintf(str, "duplicate slot name: %s", SymbolName(yyvsp[-2]));
							warning(str);
						}
						SetFrameSlot(yyval, yyvsp[-2], yyvsp[0]); }
break;
case 133:
					{	yyval = MAKEINT(-RINT(yyvsp[0])); }
break;
case 135:
					{	yyval = MakeReal(-CDouble(yyvsp[0])); }
break;
case 138:
					{	yyval = yyvsp[-1]; }
break;
case 139:
					{	yyval = yyvsp[-1];
						SetClass(yyvsp[-1], yyvsp[-3]); }
break;
case 140:
					{	yyval = yyvsp[-1]; }
break;
case 142:
					{	/* 2974 */
						if (EQRef(ClassOf(yyvsp[-2]), RSYMpathExpr))
						{
							AddArraySlot(yyvsp[-2], yyvsp[0]);
							yyval = yyvsp[-2];
						}
						else
						{
							yyval = AllocateArray(SYMA(pathExpr), 2);
							SetArraySlot(yyval, 0, yyvsp[-2]);
							SetArraySlot(yyval, 1, yyvsp[0]); }
						}
break;
case 143:
					{	/* 590 */
						yyval = MakeArray(0); }
break;
case 145:
					{	/* 594 */
						yyval = MakeArray(1);
						SetArraySlot(yyval, 0, yyvsp[0]); }
break;
case 146:
					{	/* 3028 */
						yyval = yyvsp[-2];
						AddArraySlot(yyvsp[-2], yyvsp[0]); }
break;
case 147:
					{	yyval = AllocateFrame(); }
break;
case 149:
					{	yyval = AllocateFrame();
						SetFrameSlot(yyval, yyvsp[-2], yyvsp[0]); }
break;
case 150:
					{	yyval = yyvsp[-4];
						if (FrameHasSlot(yyval, yyvsp[-2]))
						{
							Str255	str;
							sprintf(str, "duplicate slot name: %s", SymbolName(yyvsp[-2]));
							warning(str);
						}
						SetFrameSlot(yyval, yyvsp[-2], yyvsp[0]); }
break;
    }
//•••••• END OF SWITCH FROM yyparse

	yyssp -= yym;
	yystate = *yyssp;
	yyvsp -= yym;
	yym = yylhs[yyn];
	if (yystate == 0 && yym == 0)
	{
#if YYDEBUG
		if (yydebug)
			printf("yydebug: after reduction, shifting from state 0 to state %d\n",
					  YYFINAL);
#endif
		yystate = YYFINAL;
		*++yyssp = YYFINAL;
		*++yyvsp = yyval;
		if (yychar < 0)
		{
			if ((yychar = getToken()) < 0) yychar = 0;
#if YYDEBUG
			if (yydebug)
			{
				yys = 0;
				if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
				if (!yys) yys = "illegal-symbol";
				printf("yydebug: state %d, reading %d (%s)\n",
						  YYFINAL, yychar, yys);
			}
#endif
		}
		if (yychar == 0) goto yyaccept;
		goto yyloop;
	}

	if ((yyn = yygindex[yym])
	&&  (yyn += yystate) >= 0
	&&   yyn <= YYTABLESIZE
	&&   yycheck[yyn] == yystate)
		yystate = yytable[yyn];
	else
		yystate = yydgoto[yym];
#if YYDEBUG
	if (yydebug)
		printf("yydebug: after reduction, shifting to state %d\n",
				 yystate);
#endif
	if (yyssp >= &yyss[stackSize-1] && parserStackOverflow()) goto yyoverflow;
	*++yyssp = yystate;
	*++yyvsp = yyval;
	goto yyloop;

yyoverflow:
    syntaxError("yacc stack overflow");

yyabort:
    return 1;

yyaccept:
    return 0;
}


/*------------------------------------------------------------------------------
	Grow the parser stack.
	This function replaces the standard yacc output function
	yygrowstack() in file y.tab.c. It has been tweaked a bit to cope with
	our own parser stack (of Refs).
	Args:		--
	Return:	YES => stack overflowed - can’t allocate any more
------------------------------------------------------------------------------*/

BOOL
CCompiler::parserStackOverflow(void)
{
	BOOL ovflw = YES;

	newton_try
	{
		int	i = yyssp - yyss;
		int	newSize = stackSize * 2;	// or * 5 / 4 ?
		short *	newss;
		if ((newss = (short *) realloc(yyss, newSize * sizeof(short))) != NULL)
		{
			yyss = sStack = newss;
			yyssp = yyss + i;

			UnlockRef(yaccStack);
			SetLength(yaccStack, newSize);
			LockRef(yaccStack);
			vStack = Slots(yaccStack);
			yyvsp = vStack + i;

			stackSize = newSize;
			ovflw = NO;
		}
	}
	newton_catch(exRootException)
	{
		rethrow;
	}
	end_try;

	return ovflw;
}


/*------------------------------------------------------------------------------
	Get the next token from the input stream.
	This just calls yylex() but allows trailing ';' and ',' in lists.
	Args:		--
	Return:	token number
				Global yylval contains the token value.
------------------------------------------------------------------------------*/

int
CCompiler::getToken(void)
{
	int	token;

	if (isStackedToken)
	{
		isStackedToken = NO;
		yylval = stackedTokenValue;
		stackedTokenValue = NILREF;
		return stackedToken;
	}
	else
	{
		token = yylex();
		if (token == ';')
		{
			token = yylex();
			if (token == -1			// ignore trailing ';'
			 || token == kTokenEnd
			 || token == kTokenElse
			 || token == ']'
			 || token == ')'
			 || token == '}'
			 || token == ','
			 || token == kTokenUntil
			 || token == kTokenConst)
				return token;
			isStackedToken = YES;
			stackedTokenValue = yylval;
			stackedToken = token;
			return ';';
		}
		else if (token == ',')
		{
			token = yylex();
			if (token == '}'			// similarly trailing ','
			 || token == ']')
				return token;
			isStackedToken = YES;
			stackedTokenValue = yylval;
			stackedToken = token;
			return ',';
		}
		return token;
	}
}


/*------------------------------------------------------------------------------
	Get the next token from the input stream.
	Called yylex for historical reasons, but not actually generated by
	lex/flex from NewtonScript.l.
	Args:		--
	Return:	token number
				Global yylval contains the token value.
------------------------------------------------------------------------------*/

int
CCompiler::yylex(void)
{
	UniChar	ch, chNext;
	Str255	str;
	size_t	size;

	for ( ; ; )
	{
		ch = stream->getch();
		if (ch == '/')
		{
			ch = stream->getch();
			if (ch == '/')			// got a comment like this
			{
				do { ch = stream->getch(); } while (!IsBreaker(ch) && ch != (UniChar)EOF);
			}
			else if (ch == '*')	/* got a comment like this */
			{
				do
				{
					ch = stream->getch();
					if (ch == '*')
					{
						do { ch = stream->getch(); } while (ch == '*');
						if (ch == '/' || ch == (UniChar)EOF)
							break;
					}
				} while (ch != (UniChar)EOF);
			}
			else
			{
				stream->ungetch(ch);
				return '/';
			}
		}

		if (IsWhiteSpace(ch))
		{
			continue;
		}
		else if (ch == (UniChar)EOF)
		{
			return -1;
		}
		else if (ch == '|')		// |symbol|
		{
			UniChar * tokenStr = getCharsUntil('|', NO, &size);
			if (tokenStr == NULL)
				return -1;
			ConvertFromUnicode(tokenStr, str, sizeof(str), kASCIIEncoding);
			yylval = MakeSymbol(str);
			free(tokenStr);
			return kTokenSymbol;
		}
		else if (IsAlphabet(ch) || ch == '_')	// symbol
		{
			int		token;
			char *	s = str;
			do
			{
				CHECK(s <= &str[254], "Symbol too big");
				*s++ = A_CONST_CHAR(ch);
				ch = stream->getch();
			} while (ch != (UniChar)EOF && (IsAlphaNumeric(ch) || ch == '_'));
			stream->ungetch(ch);
			*s = 0;
			if (symcmp(str, "NIL") == 0)
			{
				yylval = NILREF;
				return kTokenConst;
			}
			else if (symcmp(str, "TRUE") == 0)
			{
				yylval = TRUEREF;
				return kTokenConst;
			}
			else if ((token = ReservedWordToken(str)) != kTokenInvalid)
				return token;
			else
			{
				yylval = MakeSymbol(str);
				return kTokenSymbol;
			}
		}
		else if (IsDigit(ch))	// number
		{
			return getNumber(ch);
		}
		else if (ch == '@')		// magic pointer
		{
			int mptr = 0;
			ch = stream->getch();
			if (!IsDigit(ch))
				error(kNSErrDigitRequired);
			do
			{
				mptr = mptr * 10 + ch - '0';
				ch = stream->getch();
			} while (ch != (UniChar)EOF && IsDigit(ch));
			stream->ungetch(ch);
			yylval = MAKEMAGICPTR(mptr);
			return kTokenRefConst;
		}
		else if (ch == '"')		// string
		{
			UniChar * tokenStr = getCharsUntil('"', YES, &size);
			if (tokenStr == NULL)
				return -1;
			yylval = AllocateBinary(SYMA(string), size);
			memmove(BinaryData(yylval), tokenStr, size);
			free(tokenStr);
			return kTokenConst;
		}
		else if (ch == '$')		// character
		{
			ch = stream->getch();
			if (ch == '\\')
			{
				ArrayIndex numOfHexDigits;
				ch = stream->getch();
				if (ch == '\\')
				{
					yylval = MAKECHAR('\\');
					return kTokenConst;
				}
				if (ch == 'n' || ch == 'N')
				{
					yylval = MAKECHAR(0x0D);
					return kTokenConst;
				}
				if (ch == 't' || ch == 'T')
				{
					yylval = MAKECHAR(0x09);
					return kTokenConst;
				}
				if (ch == 'u' || ch == 'U')
				{
					numOfHexDigits = 4;
					ch = stream->getch();
				}
				else
					numOfHexDigits = 2;
				int chValue = 0;
				for (ArrayIndex i = numOfHexDigits; i > 0; i--)
				{
					if (!IsHexDigit(ch))
						error(numOfHexDigits == 2 ? kNSErr2HexDigitsRequired : kNSErr4HexDigitsRequired);
					if (ch >= 'a')
						ch = ch - 'a' + 0x0A;
					else if (ch >= 'A')
						ch = ch - 'A' + 0x0A;
					else
						ch = ch - '0';
					chValue = (chValue << 4) + ch;
					ch = stream->getch();
				}
			}
			yylval = MAKECHAR(ch);
			return kTokenConst;
		}
		else if (ch == '<')
		{
			chNext = stream->getch();
			if (chNext == '=')
				return kTokenLEQ;
			else if (chNext == '<')
				return kTokenLShift;
			else if (chNext == '>')
				return kTokenNEQ;
			else
			{
				stream->ungetch(chNext);
				return '<';
			}
		}
		else if (ch == '>')
		{
			chNext = stream->getch();
			if (chNext == '=')
				return kTokenGEQ;
			else if (chNext == '>')
				return kTokenRShift;
			else
			{
				stream->ungetch(chNext);
				return '>';
			}
		}
		else if (ch == '=')
		{
			return kTokenEQL;
		}
		else if (ch == ':')
		{
			chNext = stream->getch();
			if (chNext == '=')
				return kTokenAssign;
			else if (chNext == '?')
				return kTokenSendIfDefined;
			else
			{
				stream->ungetch(chNext);
				return ':';
			}
		}
		else if (ch == '&')
		{
			chNext = stream->getch();
			if (chNext == '&')
				return kTokenAmperAmper;
			else
			{
				stream->ungetch(chNext);
				return '&';
			}
		}
		else if (strchr("+-*/()'.[],;{}", A_CONST_CHAR(ch)) != NULL)
		{
			return ch;
		}
		errorWithValue(kNSErrUnrecognized, MAKECHAR(ch));
	}
}


/*------------------------------------------------------------------------------
	Scan a symbol (delimited by |) or string (delimited by ")
	from the input stream.
	Args:		inCh			delimiter character
				isString		for calculating size: (char)symbol | (UniChar)string
				outSize		size to allocate for scanned token
	Return:	pointer to malloc'd buffer containing token string
				This must be free'd by the caller.
------------------------------------------------------------------------------*/

UniChar *
CCompiler::getCharsUntil(UniChar inCh, BOOL isString, size_t * outSize)
{
	UniChar	chNext;
	UniChar * buf = NULL;		// r5
	ArrayIndex	bufLen = 0;			// r8
	BOOL		isEscape = NO;		// r9
	int		hexIndex = -1;		// r6
	int		numOfHexDigits = isString ? 4 : 2;	// sp04
	int		chValue = 0;		// r7
	UniChar	delimiter = inCh;	// sp08
	ArrayIndex	index = 0;			// sp00

	for ( ; ; )
	{
		if (buf == NULL || index >= bufLen - 2 * sizeof(UniChar))
		{
			size_t newBufLen = bufLen + 64 * sizeof(UniChar);
			UniChar * newBuf = (UniChar *) malloc(newBufLen);
			CHECK(newBuf != NULL, "Compiler can't get buffer space");
			if (buf != NULL)
			{
				memmove(newBuf, buf, bufLen);
				free(buf);
			}
			buf = newBuf;
			bufLen = newBufLen;
		}

		chNext = stream->getch();
		if (chNext == (UniChar)EOF)
		{
			free(buf);
			error(kNSErrEOFInAString);
			return NULL;
		}

		if (isEscape)
		{
			if (chNext == 'u' || chNext == 'U')
			{
				if (hexIndex < 0)
					hexIndex = 0;
				else if (hexIndex == 0)
					hexIndex = -1;
				else
				{
					free(buf);
					error(kNSErrIllegalEscape);
					return NULL;
				}
			}
			else if (hexIndex >= 0)
			{
				free(buf);
				error(kNSErrBadEscape);
				return NULL;
			}
			else if (chNext == 'n' || chNext == 'N')
				buf[index++] = 0x0D;
			else if (chNext == 't' || chNext == 'T')
				buf[index++] = 0x09;
			else
				buf[index++] = chNext;
			isEscape = NO;
		}

		else if (chNext == '\\')
			isEscape = YES;

		else if (chNext == delimiter)
		{
			if (hexIndex > 0)
			{
				free(buf);
				error(kNSErrBadEscape);
				return NULL;
			}
			break;
		}

		else if (hexIndex >= 0)
		{
			int	hex;
			if (chNext >= '0' && chNext <= '9')			hex = chNext - '0';
			else if (chNext >= 'a' && chNext <= 'f')	hex = chNext - 'a' + 0x0A;
			else if (chNext >= 'A' && chNext <= 'F')	hex = chNext - 'A' + 0x0A;
			else
			{
				free(buf);
				errorWithValue(kNSErrBadHex, MAKECHAR(chNext));
				return NULL;
			}
			if (hexIndex < numOfHexDigits)
			{
				chValue = (chValue << 4) + hex;
				hexIndex++;
			}
			if (hexIndex == numOfHexDigits)
			{
				buf[index++] = chValue;
				chValue = 0;
				hexIndex = 0;
			}
		}

		else
			buf[index++] = chNext;
	}

	do { chNext = stream->getch(); } while (IsWhiteSpace(chNext) || chNext == '`');
	if (chNext != (UniChar)EOF)
		stream->ungetch(chNext);
	buf[index++] = kEndOfString;
	*outSize = index * sizeof(UniChar);
	return buf;
}


/*------------------------------------------------------------------------------
	Scan a number from the input stream.
	Args:		inCh		first character of number
	Return:	token number (integer or real)
				Global yylval contains the token value.
------------------------------------------------------------------------------*/

int
CCompiler::getNumber(UniChar inCh)
{
	UniChar	ch;
	Str255	str;
	ArrayIndex	i;
	int		value;

	if (inCh == '0')
	{
	//	might be hex
		ch = stream->getch();
		if (ch == 'x' || ch == 'X')
		{
		// it’s a hex integer
			str[0] = '0';
			str[1] = 'x';
			i = 2;
			while ((ch = stream->getch()) != (UniChar)EOF)
			{
				if (IsDigit(ch)	|| (ch >= 'a' && ch <= 'f')
										|| (ch >= 'A' && ch <= 'F'))
				{
					if (i < 255)
						str[i++] = ch;
					else
						error(kNSErrNumberTooLong);
				}
				else
					break;
			}
			stream->ungetch(ch);
			str[i] = 0;
			value = strtol(str, NULL, 10);
			if (value >= 0x40000000)
				errorWithValue(kNSErrInvalidHex, MakeStringFromCString(str));
			yylval = MAKEINT(value);
			return kTokenInteger;
		}
		else
		{
		// OK, it’s not hex
			stream->ungetch(ch);
		}
	}

	// it’s a decimal integer
	str[0] = inCh;
	i = 1;
	while ((ch = stream->getch()) != (UniChar)EOF)
	{
		if (IsDigit(ch))
		{
			if (i < 255)
				str[i++] = ch;
			else
				error(kNSErrNumberTooLong);
		}
		else if (ch == '.')
		{
		// actually it’s real!
			str[i++] = '.';
			while ((ch = stream->getch()) != (UniChar)EOF)
			{
				if (IsDigit(ch))
				{
					if (i < 255)
						str[i++] = ch;
					else
						error(kNSErrNumberTooLong);
				}
				else if (ch == 'e' || ch == 'E')
				{
					if (i < 255)
						str[i++] = ch;
					else
						error(kNSErrNumberTooLong);
					ch = stream->getch();
					if (ch == '+' || ch == '-')
					{
						if (i < 255)
							str[i++] = ch;
						else
							error(kNSErrNumberTooLong);
					}
					else
					{
						stream->ungetch(ch);
						while ((ch = stream->getch()) != (UniChar)EOF)
						{
							if (IsDigit(ch))
							{
								if (i < 255)
									str[i++] = ch;
								else
									error(kNSErrNumberTooLong);
							}
							else
								break;
						}
					}
				}
				else
					break;
			}
			stream->ungetch(ch);
			str[i] = 0;
			yylval = MakeReal(strtod(str, NULL));
			return kTokenReal;
		}
		else
		//	that’s the end of the number
			break;
	}
	stream->ungetch(ch);
	str[i] = 0;
	value = strtol(str, NULL, 10);
	if (value <= -0x20000000 || value >= 0x20000000)
		errorWithValue(kNSErrInvalidDecimal, MakeStringFromCString(str));
	yylval = MAKEINT(value);
	return kTokenInteger;
}


/*------------------------------------------------------------------------------
	Reserved word lookup.
	Args:		inWord		word scanned from input stream
								ASCII because it’s a symbol.
	Return:	token number
------------------------------------------------------------------------------*/

static int
ReservedWordToken(const char * inWord)
{
	int	first = 0;
	int	last = sizeof(gReservedWords) / sizeof(ReservedWord) - 1;
	while (first <= last)
	{
		int	i = (first + last) / 2;
		int	cmp = symcmp(const_cast<char *>(inWord), const_cast<char *>(gReservedWords[i].word));
		if (cmp == 0)
			return gReservedWords[i].token;
		else if (cmp < 0)
			last = i - 1;
		else
			first = i + 1;
	}
	return kTokenInvalid;
}


#pragma mark -

/*------------------------------------------------------------------------------
	S y n t a c t i c   A n a l y s i s
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Create a new function state.
	Args:		inArgs
				inFS
				ip
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::newFunctionState(RefArg inArgs, CFunctionState * inFS, int *ip)
{
	CFunctionState * theFunc = new CFunctionState(this, inArgs, inFS, ip);
	if (theFunc == nil)
		OutOfMemory();
	func = theFunc;
}


/*------------------------------------------------------------------------------
	Finish code generation and create a function object.
	Args:		--
	Return:	a function object
------------------------------------------------------------------------------*/

Ref
CCompiler::endFunction(void)
{
	emitReturn();

	CFunctionState * theFunc = func;
	RefVar	codeblock(theFunc->makeCodeBlock());
	func = theFunc->context();
	delete theFunc;
	return codeblock;
}


/*------------------------------------------------------------------------------
	Simplify the graph.
	This really does nothing.
	Args:		inGraph
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::simplify(RefArg inGraph)
{ }


/*------------------------------------------------------------------------------
	Walk the graph.
------------------------------------------------------------------------------*/

typedef int (*Trampoline)(void * inContext, RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5);
int	ClosureWalkerTrampoline(void * inContext, RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5);
int	DeclarationWalkerTrampoline(void * inContext, RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5);


/*------------------------------------------------------------------------------
	Walk the nodes…
	for declarations, closures, assignments, code, etc.
	Args:		inGraph
				inContext	compiler context
				inWalker		function that does the actual business
				inPostProcessing
	Return:	--
------------------------------------------------------------------------------*/

void	WalkNodes(RefArg inGraph, CCompiler * inContext, Trampoline inWalker, BOOL inPostProcessing)
{
	int		tokenType = RINT(GetArraySlot(inGraph, 0));
	int		numOfParams = Length(inGraph) - 1;
	RefVar	p1(numOfParams >= 1 ? GetArraySlot(inGraph, 1) : NILREF);
	RefVar	p2(numOfParams >= 2 ? GetArraySlot(inGraph, 2) : NILREF);
	RefVar	p3(numOfParams >= 3 ? GetArraySlot(inGraph, 3) : NILREF);
	RefVar	p4(numOfParams >= 4 ? GetArraySlot(inGraph, 4) : NILREF);
	RefVar	p5(numOfParams >= 5 ? GetArraySlot(inGraph, 5) : NILREF);
	ArrayIndex	i, count;

	if (inPostProcessing
	||  inWalker(inContext, inGraph, tokenType, p1, p2, p3, p4, p5) != 0)
	{
//88
		switch(tokenType)
		{
		case '+':
		case '-':
		case '*':
		case '/':
		case kTokenDiv:
		case kTokenMod:
		case kTokenLShift:
		case kTokenRShift:
		case kTokenAnd:
		case kTokenOr:
		case '<':
		case kTokenLEQ:
		case '>':
		case kTokenGEQ:
		case kTokenEQL:
		case kTokenNEQ:
		case '&':
		case kTokenAmperAmper:
		case '.':
		case '[':
		case kTokenWhile:
//474
			WalkNodes(p1, inContext, inWalker, inPostProcessing);
			WalkNodes(p2, inContext, inWalker, inPostProcessing);
			break;

		case kTokenBegin:
//299
			count = Length(p1);
			for (i = 0; i < count; i++)
				WalkNodes(GetArraySlot(p1, i), inContext, inWalker, inPostProcessing);
			break;

		case kTokenGlobal:
//258
			WalkNodes(p2, inContext, inWalker, inPostProcessing);
			break;

		case kTokenLocal:
//324
			count = Length(p1);
			for (i = 1; i < count; i += 2)
			{
				RefVar	value(GetArraySlot(p1, i));
				if (NOTNIL(value))
					WalkNodes(value, inContext, inWalker, inPostProcessing);
			}
			break;

		case kTokenIf:
//428
			WalkNodes(p1, inContext, inWalker, inPostProcessing);
			WalkNodes(p2, inContext, inWalker, inPostProcessing);
			if (NOTNIL(p3))
				WalkNodes(p3, inContext, inWalker, inPostProcessing);
			break;

		case kTokenTry:
//582
			WalkNodes(p1, inContext, inWalker, inPostProcessing);
			count = Length(p2);
			for (i = 0; i < count; i++)
			{
				RefVar	p2x(GetArraySlot(p2, i));
				WalkNodes(GetArraySlot(p2x, 2), inContext, inWalker, inPostProcessing);
			}
			break;

		case kTokenBuildArray:
//502
			count = Length(p1);
			for (i = 1; i < count; i += 2)
				WalkNodes(GetArraySlot(p1, i), inContext, inWalker, inPostProcessing);
			break;

		case kTokenBuildFrame:
//528
			{
				CObjectIterator	iter(p1);
				for ( ; !iter.done(); iter.next())
					WalkNodes(iter.value(), inContext, inWalker, inPostProcessing);
			}
			break;

		case kTokenRepeat:
//119+
			count = Length(p1);
			for (i = 1; i < count; i += 2)
				WalkNodes(GetArraySlot(p1, i), inContext, inWalker, inPostProcessing);
			WalkNodes(p1, inContext, inWalker, inPostProcessing);
			break;

		case kTokenCall:
//353
			count = Length(p2);
			for (i = 0; i < count; i++)
				WalkNodes(GetArraySlot(p2, i), inContext, inWalker, inPostProcessing);
			break;

		case kTokenInvoke:
//378
			count = Length(p2);
			for (i = 0; i < count; i++)
				WalkNodes(GetArraySlot(p2, i), inContext, inWalker, inPostProcessing);
			WalkNodes(p1, inContext, inWalker, inPostProcessing);
			break;

		case kTokenFor:
//447
			WalkNodes(p2, inContext, inWalker, inPostProcessing);
			WalkNodes(p3, inContext, inWalker, inPostProcessing);
			WalkNodes(p4, inContext, inWalker, inPostProcessing);
			WalkNodes(p5, inContext, inWalker, inPostProcessing);
			break;

		case kTokenForeach:
//468
			WalkNodes(p2, inContext, inWalker, inPostProcessing);
			WalkNodes(p3, inContext, inWalker, inPostProcessing);
			break;

		case kTokenLoop:
		case kTokenReturn:
		case kTokenBreak:
		case kTokenNot:
		case kTokenUMinus:
//186
			WalkNodes(p1, inContext, inWalker, inPostProcessing);
			break;

		case kTokenAssign:
//247
			{
				int	subType = RINT(GetArraySlot(p1, 0));
				if (subType == kTokenSymbol)
					WalkNodes(p2, inContext, inWalker, inPostProcessing);
				else if (subType == '.'
						|| subType == ':')
				{
					WalkNodes(GetArraySlot(p1, 1), inContext, inWalker, inPostProcessing);
					WalkNodes(GetArraySlot(p1, 2), inContext, inWalker, inPostProcessing);
					WalkNodes(p2, inContext, inWalker, inPostProcessing);
				}
			}
			break;

		case kTokenExists:
//191
			{
				int	subType = RINT(GetArraySlot(p1, 0));
				if (subType == '.')
				{
					WalkNodes(GetArraySlot(p1, 1), inContext, inWalker, inPostProcessing);
					WalkNodes(GetArraySlot(p1, 2), inContext, inWalker, inPostProcessing);
				}
				else if (subType == ':')
				{
					WalkNodes(GetArraySlot(p1, 2), inContext, inWalker, inPostProcessing);
				}
			}
			break;

		case ':':
		case kTokenSendIfDefined:
//177
			count = Length(p3);
			for (i = 0; i < count; i++)
				WalkNodes(GetArraySlot(p3, i), inContext, inWalker, inPostProcessing);
			break;
		}
	}
//557
	if (inPostProcessing)
		inWalker(inContext, inGraph, tokenType, p1, p2, p3, p4, p5);
}


/*------------------------------------------------------------------------------
	Walk nodes for declarations.
	Args:		inGraph
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::walkForDeclarations(RefArg inGraph)
{
	WalkNodes(inGraph, this, DeclarationWalkerTrampoline, NO);
	func->declarationsFinished();
}

int
DeclarationWalkerTrampoline(void * inContext, RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5)
{ return static_cast<CCompiler*>(inContext)->declarationWalker(inGraph, inNodeType, inP1, inP2, inP3, inP4, inP5); }

int
CCompiler::declarationWalker(RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5)
{
	ArrayIndex	i, count;

	switch (inNodeType)
	{
	case kTokenFunc:
//349
		//	p1 = formal args [0]
		//	p2 = func expr
		//	p3 = isNative
		//	p4 = formal args [1]
		//	p5 = used to save func context
		newFunctionState(inP1, func, funcDepthPtr);
		walkForDeclarations(inP2);
		SetArraySlot(inGraph, 5, (Ref) func);
		func = func->context();
		break;

	case kTokenConstant:
//19
		count = Length(inP1);
		for (i = 0; i < count; i += 2)
		{
			RefVar	varName(GetArraySlot(inP1, i));		// sp+04
			RefVar	varValue(GetArraySlot(inP1, i+1));	// sp+00
			if (func->isLocalConstant(varName))
				errorWithValue(kNSErrConstRedefined, varName);
			if (func->atTopLevel() && FrameHasSlot(RA(gVarFrame), varName))
				errorWithValue(kNSErrGlobalAndConstCollision, varName);
			simplify(varValue);
			if (isConstantExpr(varValue))
				func->addConstant(varName, evaluateConstantExpr(varValue));
			else
				error(kNSErrNonLiteralConst);
		}
		break;

	case kTokenLocal:
//66
		{
			count = Length(inP1);
			RefVar	varNames(MakeArray(count/2));
			for (i = 0; i < count; i += 2)
				SetArraySlot(varNames, i/2, GetArraySlot(inP1, i));
			func->addLocals(varNames);
		}
		break;

	case kTokenFor:
//100
		{
			RefVar	varNames(MakeArray(3));
			SetArraySlot(varNames, 0, inP1);
			const char *	name = SymbolName(inP1);
			ArrayIndex	nameLen = strlen(name);
			char *	iterVarName = (char *) malloc(nameLen + 7);
			strcpy(iterVarName, name);
			memmove(iterVarName + nameLen, "|limit", 7);
			SetArraySlot(varNames, 1, MakeSymbol(iterVarName));
			memmove(iterVarName + nameLen, "|incr", 6);
			SetArraySlot(varNames, 2, MakeSymbol(iterVarName));
			free(iterVarName);
			func->addLocals(varNames);
		}
		break;

	case kTokenForeach:
//39
		{
			func->addLocals(Clone(inP3));
			const char *	name = SymbolName(GetArraySlot(inP3, 0));
			const char *	deep = Length(inP3) > 1 ? SymbolName(GetArraySlot(inP3, 1)) : "";
			ArrayIndex	nameLen = strlen(name);
			ArrayIndex	deepLen = strlen(deep);
			char *	iterVarName = (char *) malloc(nameLen + deepLen + 8);
			strcpy(iterVarName, name);
			strcat(iterVarName, deep);
			strcat(iterVarName, "|iter");
			BOOL		doCollect = EQ(inP1, SYMA(collect));
			RefVar	varNames(MakeArray(doCollect ? 3 : 1));
			SetArraySlot(varNames, 0, MakeSymbol(iterVarName));
			if (doCollect)
			{
				memmove(iterVarName + nameLen + deepLen, "|index", 7);
				SetArraySlot(varNames, 1, MakeSymbol(iterVarName));
				memmove(iterVarName + nameLen + deepLen, "|result", 8);
				SetArraySlot(varNames, 2, MakeSymbol(iterVarName));
			}
			free(iterVarName);
			func->addLocals(varNames);
		}
		break;
	}

	return 1;
}


/*------------------------------------------------------------------------------
	Walk nodes for closures.
	Args:		inGraph
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::walkForClosures(RefArg inGraph)
{
	func->computeInitialVarLocs();
	WalkNodes(inGraph, this, ClosureWalkerTrampoline, NO);
	func->computeArgFrame();
}

int
ClosureWalkerTrampoline(void * inContext, RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5)
{ return static_cast<CCompiler*>(inContext)->closureWalker(inGraph, inNodeType, inP1, inP2, inP3, inP4, inP5); }

int
CCompiler::closureWalker(RefArg inGraph, int inNodeType, RefArg inP1, RefArg inP2, RefArg inP3, RefArg inP4, RefArg inP5)
{
	switch (inNodeType)
	{
	case kTokenSymbol:
//107
		if (func->isLocalVariable(inP1))
		{
			RefVar	varLocs(func->varLocs());
			Ref		refCount = GetFrameSlot(varLocs, inP1);
			if (ISNIL(refCount))
				SetFrameSlot(varLocs, inP1, MAKEINT(1));
			else if (ISINT(refCount))
				SetFrameSlot(varLocs, inP1, MAKEINT(RINT(refCount) + 1));
		}
		else if (func->noteVarReference(inP1) == 0)
			func->noteMsgEnvReference(0);
		break;

	case kTokenFunc:
//19
		//	p1 = formal args [0]
		//	p2 = func expr
		//	p3 = isNative
		//	p4 = formal args [1]
		//	p5 = used to save func context
		func = (CFunctionState *) ((Ref) inP5);
		walkForClosures(inP2);
		func = func->context();
		break;

	case kTokenSelf:
//158
		func->noteMsgEnvReference(0);
		break;

	case kTokenAssign:
//42
		if (RINT(GetArraySlot(inP1, 0)) == kTokenSymbol)
			closureWalker(inP1, RINT(GetArraySlot(inP1, 0)), GetArraySlot(inP1, 1), RA(NILREF), RA(NILREF), RA(NILREF), RA(NILREF));
		break;

	case ':':
	case kTokenSendIfDefined:
//35
		if (ISNIL(inP2))
			func->noteMsgEnvReference(1);
		break;
	}
	
	return 1;
}


/*------------------------------------------------------------------------------
	Walk the graph and generate code!
	Args:		inGraph			
				inFinalNode		
	Return:	BOOL?
------------------------------------------------------------------------------*/

int
CCompiler::walkForCode(RefArg inGraph, BOOL inFinalNode)
{
//	sp-24
	int		tokenType = RINT(GetArraySlot(inGraph, 0));
	int		subType;
	int		numOfParams = Length(inGraph) - 1;
	RefVar	p1(numOfParams >= 1 ? GetArraySlot(inGraph, 1) : NILREF);
	RefVar	p2(numOfParams >= 2 ? GetArraySlot(inGraph, 2) : NILREF);
	RefVar	p3(numOfParams >= 3 ? GetArraySlot(inGraph, 3) : NILREF);
	RefVar	p4(numOfParams >= 4 ? GetArraySlot(inGraph, 4) : NILREF);
	RefVar	p5(numOfParams >= 5 ? GetArraySlot(inGraph, 5) : NILREF);

	BOOL		isArithmetic = NO;
	ArrayIndex	i, numOfElements, numOfArgs;
	int		isFinalNode = 1;

	switch(tokenType)
	{
	//	for all arithmetic operators
	//	p1 = operand1
	//	p2 = operand2
	case '+':
//380
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_2B), 2);		// '+
		break;

	case '-':
//392
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_2D), 2);		// '-
		break;

	case '*':
//404
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_2A), 2);		// '*
		break;

	case '/':
//69+
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_2F), 2);		// '/
		break;

	case kTokenDiv:
//416
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(div), 2);
		break;

	case kTokenMod:
//428
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(mod), 2);
		break;

	case kTokenLShift:
//440
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_3C_3C), 2);		// <<
		break;

	case kTokenRShift:
//452
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_3E_3E), 2);		// >>
		break;

	case kTokenAnd:
//524
		{
		//	generate
		//	expr1 expr1offset:branchIfFalse->noAction expr2 expr2offset:branch->noAction nil noAction:
			ArrayIndex	expr1PC, expr2PC;
			isArithmetic = YES;
			walkForCode(p1, NO);
			expr1PC = emitPlaceholder();
			if (inFinalNode)
			{
				if (walkForCode(p2, inFinalNode) != 0)
					emitPop();
			}
			else
			{
				walkForCode(p2, NO);
				expr2PC = emitPlaceholder();
			}
			backpatch(expr1PC, kOpcodeBranchIfFalse, curPC());
			if (inFinalNode)
				isFinalNode = 0;
			else
			{
				emitPush(RA(NILREF));
				backpatch(expr2PC, kOpcodeBranch, curPC());
			}
		}
		break;

	case kTokenOr:
//567
		{
		//	generate
		//	expr1 expr1offset:branchIfTrue->noAction expr2 expr2offset:branch->noAction true noAction:
			ArrayIndex	expr1PC, expr2PC;
			isArithmetic = YES;
			walkForCode(p1, NO);
			expr1PC = emitPlaceholder();
			if (inFinalNode)
			{
				if (walkForCode(p2, inFinalNode) != 0)
					emitPop();
			}
			else
			{
				walkForCode(p2, NO);
				expr2PC = emitPlaceholder();
			}
			backpatch(expr1PC, kOpcodeBranchIfTrue, curPC());
			if (inFinalNode)
				isFinalNode = 0;
			else
			{
				emitPush(RA(TRUEREF));
				backpatch(expr2PC, kOpcodeBranch, curPC());
			}
		}
		break;

	case kTokenNot:
//615
		isArithmetic = YES;
		walkForCode(p1, NO);
		emitFuncall(SYMA(not), 1);
		break;

	case '<':
//464
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_3C), 2);		// '<
		break;

	case kTokenLEQ:
//476
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_3C_3D), 2);	// <=
		break;

	case '>':
//122
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_3E), 2);		// '>
		break;

	case kTokenGEQ:
//488
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_3E_3D), 2);	// >=
		break;

	case kTokenEQL:
//500
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_3D), 2);		// '=
		break;

	case kTokenNEQ:
//512
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(_3C_3E), 2);	// '<>
		break;

	case '&':
	case kTokenAmperAmper:
//1873
//1847
		isArithmetic = YES;
		numOfElements = walkForStringer(inGraph);
		emitPush(SYMA(array));
		emit(kOpcodeMakeArray, numOfElements);
		emitFuncall(SYMA(stringer), 1);
		break;

	case '.':
//349
		{
			int		throwIfNilObject = YES;
			RefVar	pathExpr(walkForPath(inGraph, &throwIfNilObject));
			if (NOTNIL(pathExpr))
				emitPush(pathExpr);
			emit(kOpcodeGetPath, throwIfNilObject);
		}
		break;

	case '[':
//369
		isArithmetic = YES;
		walkForCode(p1, NO);
		walkForCode(p2, NO);
		emitFuncall(SYMA(aref), 2);
		break;

	case kTokenUMinus:
//623
		isArithmetic = YES;
		if (RINT(GetArraySlot(p1, 0)) == kTokenConst)
		{
			newton_try
			{
				Ref	value = GetArraySlot(p1, 1);
				if (ISINT(value))
					emitPush(MAKEINT(-RVALUE(value)));
				else
					emitPush(MakeReal(-CDouble(value)));
			}
			newton_catch(exBadType)
			{
				ThrowBadTypeWithFrameData(kNSErrNotANumber, GetArraySlot(p1, 1));
			}
			end_try;
		}
		else
		{
			walkForCode(p1, NO);
			emitFuncall(SYMA(negate), 1);
		}
		break;

	case kTokenConst:
//334
		if (inFinalNode)
			isFinalNode = 0;
		else
		{
			emitPush(p1);
			return isFinalNode;
		}
		break;

	case kTokenSymbol:
//140
		if (func->isConstant(p1))
			emitPush(func->getConstantValue(p1, NULL));
		else
			emitVarGet(p1);
		break;

	case kTokenBegin:
//820
		//	p1 = expr seq
		numOfElements = Length(p1);
		if (numOfElements > 0)
		{
			ArrayIndex	finalNode = numOfElements - 1;
			for (i = 0; i < numOfElements; i++)
			{
				if (i <= finalNode)
					isFinalNode = walkForCode(GetArraySlot(p1, i), inFinalNode);
				else
				{
				//	surely this is unreachable?
					if (walkForCode(GetArraySlot(p1, i), YES) != 0)
						emitPop();
				}
			}
		}
		else
		{
			if (inFinalNode)
				isFinalNode = 0;
			else
				emitPush(RA(NILREF));
		}
		break;

	case kTokenFunc:
//1724
		//	p1 = formal args [0]
		//	p2 = func expr
		//	p3 = isNative
		//	p4 = formal args [1]
		//	p5 = used to save func context
		isArithmetic = YES;
		if (inFinalNode)
			isFinalNode = 0;
		else
		{
			func = (CFunctionState *) ((Ref) p5);
			func->copyClosedArgs();
			walkForCode(p2, NO);
			BOOL		hasScope = NOTNIL(func->argFrame());
			RefVar	fnBlock(endFunction());
			emitPush(fnBlock);
			if (hasScope)
				emit(kOpcodeSimple, kSimpleSetLexScope);
			return isFinalNode;
		}
		break;

	case kTokenGlobal:
//938
		if (FrameHasSlot(gConstantsFrame, p1))
			errorWithValue(kNSErrGlobalAndConstCollision, p1);
		walkForCode(p2, NO);
		emitPush(p1);
		emitFuncall(SYMA(setGlobal), 2);
		break;

	case kTokenConstant:
//964
		if (inFinalNode)
			isFinalNode = 0;
		else
			emitPush(RA(NILREF));
		break;

	case kTokenLocal:
//885
		numOfElements = Length(p1);
		for (i = 0; i < numOfElements; i += 2)
		{
			ArrayIndex	j = i + 1;
			if (NOTNIL(GetArraySlot(p1, j)))
			{
				walkForCode(GetArraySlot(p1, j), NO);
				emitVarSet(GetArraySlot(p1, i));
			}
		}
		if (inFinalNode)
			isFinalNode = 0;
		else
			emitPush(RA(NILREF));
		break;

	case kTokenIf:
//1090
		//	p1 = if expr
		//	p2 = then expr
		//	p3 = else expr
		{
			if (RINT(GetArraySlot(p1, 0)) == kTokenAmperAmper)
				warning("&& used in IF statement... did you mean AND?");
			walkForCode(p1, NO);
			ArrayIndex	elseBranch = emitPlaceholder();
			ArrayIndex	continueBranch;
			if (inFinalNode)
			{
				if (walkForCode(p2, inFinalNode) != 0)
					emitPop();
			}
			else
				walkForCode(p2, NO);
			if (ISNIL(p3))
			{
			//	there’s no ELSE clause
				if (inFinalNode)
				{
					backpatch(elseBranch, kOpcodeBranchIfFalse, curPC());
					isFinalNode = 0;
				}
				else
				{
					continueBranch = emitPlaceholder();
					backpatch(elseBranch, kOpcodeBranchIfFalse, curPC());
					emitPush(RA(NILREF));
					backpatch(continueBranch, kOpcodeBranch, curPC());
				}
			}
			else
			{
			//	this is the end of the IF clause
				continueBranch = emitPlaceholder();
			//	and the start of the ELSE clause
				backpatch(elseBranch, kOpcodeBranchIfFalse, curPC());
				if (inFinalNode)
				{
					if (walkForCode(p2, inFinalNode) != 0)
						emitPop();
				}
				else
					walkForCode(p2, NO);
				backpatch(continueBranch, kOpcodeBranch, curPC());
				if (inFinalNode)
					isFinalNode = 0;
			}
		}
		break;

	case kTokenTry:
//1898
		//	p1 = pt1(kTokenBegin, expr seq)
		//	p2 = handlers array
		//sp-08
		{
			numOfElements = Length(p2);
			RefVar	entryBranches(MakeArray(numOfElements));
			ArrayIndex	numOfExits = numOfElements - 1;
			RefVar	exitBranches(MakeArray(numOfExits));
			RefVar	handler;
			// emit sym|PC pairs
			for (i = 0; i < numOfElements; i++)
			{
				handler = GetArraySlot(p2, i);
				emitPush(GetArraySlot(handler, 1));
				SetArraySlot(entryBranches, i, MAKEINT(emitPlaceholder()));
			}
			emit(kOpcodeNewHandlers, numOfElements);	// emit opcode

			if (inFinalNode)
			{
				if (walkForCode(p1, YES) != 0)
					emitPop();
			}
			else
				walkForCode(p1, NO);
			emit(kOpcodeSimple, kSimplePopHandlers);

			ArrayIndex	continueBranch = emitPlaceholder();
			// iterate over handlers
			for (i = 0; i < numOfElements; i++)
			{
				handler = GetArraySlot(p2, i);
				Ref	handlerOffset = MAKEINT(curPC());
				if (handlerOffset == (handlerOffset & 0xFFFF))
				//	short branch
					backpatch(RINT(GetArraySlot(entryBranches, i)), kOpcodePushConstant, handlerOffset);
				else
				//	long branch
					backpatch(RINT(GetArraySlot(entryBranches, i)), kOpcodePush, func->literalOffset(handlerOffset));

				if (inFinalNode)
				{
					if (walkForCode(GetArraySlot(handler, 2), YES) != 0)
						emitPop();
				}
				else
					walkForCode(GetArraySlot(handler, 2), NO);

				// set up handler exit branch
				if (i < numOfExits)
					SetArraySlot(exitBranches, i, MAKEINT(emitPlaceholder()));
			}

			// backpatch handler exit branches
			for (i = 0; i < numOfExits; i++)
				backpatch(RINT(GetArraySlot(exitBranches, i)), kOpcodeBranch, curPC());
			emit(kOpcodeSimple, kSimplePopHandlers);

			// backpatch try end branch
			backpatch(continueBranch, kOpcodeBranch, curPC());
			if (inFinalNode)
				isFinalNode = 0;
		}
		break;

	case kTokenBuildArray:
//1763
		isArithmetic = YES;
		numOfElements = Length(p1);
		for (i = 0; i < numOfElements; i++)
			walkForCode(GetArraySlot(p1, i), NO);
		emitPush(ClassOf(p1));
		emit(kOpcodeMakeArray, numOfElements);
		break;

	case kTokenBuildFrame:
//1801
		{
			isArithmetic = YES;
			numOfElements = Length(p1);
			CObjectIterator	iter(p1);
			for ( ; !iter.done(); iter.next())
				walkForCode(iter.value(), NO);
			emitPush(SharedFrameMap(p1));
			emit(kOpcodeMakeFrame, numOfElements);
		}
		break;

	case kTokenLoop:
//1209
		{
			int	loopStart = curPC();
			func->beginLoop();
			walkForCode(p1, NO);
			emitPop();
			emitBranch(loopStart);
			func->endLoop();
		}
		break;

	case kTokenWhile:
//1627
		{
			ArrayIndex	entryBranch = emitPlaceholder();
			ArrayIndex	loopStart = curPC();
			func->beginLoop();
			walkForCode(p2, NO);
			emitPop();
			backpatch(entryBranch, kOpcodeBranch, curPC());
			walkForCode(p1, NO);
			emit(kOpcodeBranchIfTrue, loopStart);
			emitPush(RA(NILREF));
			func->endLoop();
		}
		break;

	case kTokenRepeat:
//1668
		{
			ArrayIndex	loopStart = curPC();
			func->beginLoop();
			numOfElements = Length(p1);
			for (i = 0; i < numOfElements; i++)
			{
				if (walkForCode(GetArraySlot(p1, i), YES) != 0)
					emitPop();
			}
			walkForCode(p2, NO);
			emit(kOpcodeBranchIfFalse, loopStart);
			emitPush(RA(NILREF));
			func->endLoop();
		}
		break;

	case kTokenCall:
//989
		numOfArgs = Length(p2);
		for (i = 0; i < numOfArgs; i++)
			walkForCode(GetArraySlot(p2, i), NO);
		emitFuncall(p1, numOfArgs);
		break;

	case kTokenInvoke:
//1017
		numOfArgs = Length(p2);
		for (i = 0; i < numOfArgs; i++)
			walkForCode(GetArraySlot(p2, i), NO);
		walkForCode(p1, NO);
		emit(kOpcodeInvoke, numOfArgs);
		break;

	case kTokenFor:
//157+
		{
		//	uses iterName, iterName|limit, iterName|incr
		//	p1 = iterator name
		//	p2 = initial value
		//	p3 = limit
		//	p4 = incr
		//	p5 = code
			const char *	name = SymbolName(p1);
			int		nameLen = strlen(name);
			char *	iterVarName = (char *) malloc(nameLen + 7);
			strcpy(iterVarName, name);
			memmove(iterVarName + nameLen, "|limit", 7);
			RefVar	iterLimit(MakeSymbol(iterVarName));
			memmove(iterVarName + nameLen, "|incr", 6);
			RefVar	iterIncr(MakeSymbol(iterVarName));
			free(iterVarName);

			walkForCode(p2, NO);
			emitVarSet(p1);
			walkForCode(p3, NO);
			emitVarSet(iterLimit);
			walkForCode(p4, NO);
			emitVarSet(iterIncr);
			emitVarGet(iterIncr);
			emitVarGet(p1);

			ArrayIndex	entryBranch = emitPlaceholder();
			ArrayIndex	loopStart = curPC();
			func->beginLoop();
			if (walkForCode(p5, YES) != 0)
				emitPop();
			emitVarGet(iterIncr);
			emitVarIncr(p1);
			backpatch(entryBranch, kOpcodeBranch, curPC());
			emitVarGet(iterLimit);
			emit(kOpcodeBranchIfLoopNotDone, loopStart);
			emitPush(RA(NILREF));
			func->endLoop();
		}
		break;

	case kTokenForeach:
//1226
		{
		//	uses iterName[deeply]|iter; if collect then iterName|index, iterName|result
		//	p1 = do | collect
		//	p2 = frame|array object
		//	p3 = expr to iterate over
		//	p4 = [optional tag|index, iterName]
		//	p5 = optional deeply
		//	sp-18
			BOOL		doCollect = EQ(p1, SYMA(collect));	// r10
			BOOL		isTagged = (Length(p4) > 1);					// sp+14
			BOOL		isDeeply = NOTNIL(p5);									// sp+10
			RefVar	tagSym(isTagged ? GetArraySlot(p4, 0) : NILREF);	// sp+0C
			RefVar	valueSym(GetArraySlot(p4, isTagged ? 1 : 0));				// sp+08
			const char *	namePart1 = SymbolName(GetArraySlot(p4, 0));					// sp+04
			const char *	namePart2 = isTagged ? SymbolName(GetArraySlot(p4, 1)) : "";	// sp+00
			int		namePart1Len = strlen(namePart1);
			int		namePart2Len = strlen(namePart2);
			char *	iterVarName = (char *) malloc(namePart1Len + namePart2Len + 8);
			strcpy(iterVarName, namePart1);
			strcat(iterVarName, namePart2);
			strcat(iterVarName, "|iter");
		//	sp-0C
			RefVar	iterVarSym(MakeSymbol(iterVarName));	// sp+08
			RefVar	iterIndexSym;		// sp+04
			RefVar	iterResultSym;		// sp+00
			walkForCode(p2, NO);
		//	sp-04
			if (doCollect)
			{
				memmove(iterVarName + namePart1Len + namePart2Len, "|index", 7);
				iterIndexSym = MakeSymbol(iterVarName);
				memmove(iterVarName + namePart1Len + namePart2Len, "|result", 8);
				iterResultSym = MakeSymbol(iterVarName);
			}
//1351
			free(iterVarName);

			emitPush(isDeeply ? RA(TRUEREF) : RA(NILREF));
			emitFuncall(SYMA(newIterator), 2);
			emitVarSet(iterVarSym);

			if (doCollect)
			{
				if (isDeeply)
				{
					emitVarGet(iterVarSym);
					emitPush(MAKEINT(kIterDeepNumOfSlotsIndex));
				}
				else
				{
					emitVarGet(iterVarSym);
					emitPush(MAKEINT(kIterNumOfSlotsIndex));
				}
				emitFuncall(SYMA(aref), 2);
				emitPush(SYMA(array));
				emit(kOpcodeMakeArray, 0xFFFF);
				emitVarSet(iterResultSym);
				emitPush(MAKEINT(0));
				emitVarSet(iterIndexSym);
			}
		//	sp-08
			ArrayIndex	entryBranch = emitPlaceholder();	// r8
			ArrayIndex	exitBranch;
			ArrayIndex	loopStart = curPC();					// sp+04
			func->beginLoop();
			emitVarGet(iterVarSym);
			emitPush(MAKEINT(kIterValueIndex));
			emitFuncall(SYMA(aref), 2);
			emitVarSet(valueSym);
			if (isDeeply)
			{
				emitVarGet(iterVarSym);
				emitPush(MAKEINT(kIterTagIndex));
				emitFuncall(SYMA(aref), 2);
				emitVarSet(tagSym);
			}
//1484
			if (doCollect)
			{
				emitVarGet(iterResultSym);
				emitVarGet(iterIndexSym);
				walkForCode(p3, NO);
				emitFuncall(SYMA(setAref), 3);
				emitPop();
				emitPush(MAKEINT(1));
				emitVarIncr(iterIndexSym);
				emitPop();
				emitPop();
			}
			else
			{
				if (walkForCode(p3, YES))
					emitPop();
			}
//1528
			emitVarGet(iterVarSym);
			emit(kOpcodeSimple, kSimpleIterNext);
			backpatch(entryBranch, kOpcodeBranch, curPC());
			emitVarGet(iterVarSym);
			emit(kOpcodeSimple, kSimpleIterDone);
			emit(kOpcodeBranchIfFalse, loopStart);
		//	sp-04
			if (doCollect)
				exitBranch = emitPlaceholder();
			else
				emitPush(RA(NILREF));
//1568
			func->endLoop();
		//	sp-04
			if (doCollect)
			{
				emitVarSet(iterResultSym);
				emitPop();
				emitPop();
				backpatch(exitBranch, kOpcodeBranch, curPC());
				emitVarGet(iterResultSym);
				emitPush(RA(NILREF));
				emitVarSet(iterResultSym);
			}
			emitPush(RA(NILREF));		// nil out the iterator object
			emitVarSet(iterVarSym);
		}
		break;

	case kTokenSelf:
//344
		emit(kOpcodeSimple, kSimplePushSelf);
		break;

	case kTokenReturn:
//982
		walkForCode(p1, NO);
		emitReturn();
		break;

	case kTokenBreak:
//975
		walkForCode(p1, NO);
		func->addLoopExit();
		break;

	case kTokenAssign:
//813
		isFinalNode = walkAssignment(p1, p2, inFinalNode);
		break;

	case kTokenExists:
//724
		isArithmetic = YES;
		subType = RINT(GetArraySlot(p1, 0));
		if (subType == kTokenSymbol)
		{
			emitPush(GetArraySlot(p1, 1));
			emitFuncall(SYMA(hasVar), 1);
		}
		else if (subType == '.')
		{
			int		throwIfNilObject = NO;
			RefVar	pathExpr(walkForPath(p1, &throwIfNilObject));
			if (NOTNIL(pathExpr))
				emitPush(pathExpr);
			emitFuncall(SYMA(hasPath), throwIfNilObject);
		}
		else if (subType == ':')
		{
			walkForCode(GetArraySlot(p1, 2), NO);
			emitPush(GetArraySlot(p1, 1));
			emitFuncall(SYMA(hasVariable), 2);
		}
		else
			error(kNSErrBadSubscriptTest);
		break;

	case ':':
	case kTokenSendIfDefined:
//325
		//	p1 = message
		//	p2 = receiver
		//	p3	= args
		numOfArgs = Length(p3);
		for (i = 0; i < numOfArgs; i++)
			walkForCode(GetArraySlot(p3, i), NO);
		if (NOTNIL(p2))
		{
			walkForCode(p2, NO);
			emitPush(p1);
			emit(tokenType == ':' ? kOpcodeSend : kOpcodeSendIfDefined, numOfArgs);
		}
		else
		{
			emitPush(p1);
			emit(tokenType == ':' ? kOpcodeResend : kOpcodeResendIfDefined, numOfArgs);
		}
		break;
	}

//2131
	if (inFinalNode)
	{
		if (tokenType == kTokenEQL)
			warning("= with no effect... did you mean := ?");
		else if (isArithmetic)
			warning("Statement has no effect");
	}

	return isFinalNode;
}


/*------------------------------------------------------------------------------
	Walk the graph for an assignment.
	Args:		inLHS			object to assign to
				inRHS			value to assign
				inArg3		is top level? expression will not be required?
	Return:	some kind of boolean
------------------------------------------------------------------------------*/

int
CCompiler::walkAssignment(RefArg inLHS, RefArg inRHS, BOOL inArg3)
{
	int	isFinalNode = 1;
	int	tokenType = RINT(GetArraySlot(inLHS, 0));
	if (tokenType == kTokenSymbol)
	{
	//	it’s a simple variable name
		RefVar	varName(GetArraySlot(inLHS, 1));
		if (func->isConstant(varName))
			errorWithValue(kNSErrBadAssign, varName);
		walkForCode(inRHS, NO);
		emitVarSet(varName);
		if (inArg3)
			isFinalNode = 0;
		else
			emitVarGet(varName);
	}

	else if (tokenType == '.')
	{
	//	it’s a slot accessor
//57
		int		throwIfNilObject = YES;
		RefVar	pathExpr(walkForPath(inLHS, &throwIfNilObject));
		if (NOTNIL(pathExpr))
			emitPush(pathExpr);
		walkForCode(inRHS, NO);
		emit(kOpcodeSetPath, inArg3 ? 0 : 1);
		if (inArg3)
			isFinalNode = 0;
	}

	else if (tokenType == '[')
	{
	//	it’s an array accessor
//92
		walkForCode(GetArraySlot(inLHS, 1), NO);	// array object
		walkForCode(GetArraySlot(inLHS, 2), NO);	// index
		walkForCode(inRHS, NO);							// element
		emitFuncall(SYMA(setAref), 3);
	}

	return isFinalNode;
}


/*------------------------------------------------------------------------------
	Walk the graph for a path expression.
	Args:		inGraph					
				ioThrowIfNilObject	
	Return:	path expression
------------------------------------------------------------------------------*/

Ref
CCompiler::walkForPath(RefArg inGraph, int * ioThrowIfNilObject)
{
	int	tokenType = RINT(GetArraySlot(inGraph, 0));
	if (tokenType == '.')
	{
		RefVar	left(GetArraySlot(inGraph, 1));
		RefVar	right(GetArraySlot(inGraph, 2));
		if (RINT(GetArraySlot(right, 0)) == kTokenConst
		&&  IsSymbol(GetArraySlot(right, 1)))
		{
			if (RINT(GetArraySlot(left, 0) == '.'))
			{
				RefVar	object(walkForPath(left, ioThrowIfNilObject));
				if (ISNIL(object))
				{
					emit(kOpcodeGetPath, *ioThrowIfNilObject);
					*ioThrowIfNilObject = NO;
					return GetArraySlot(right, 1);
				}
				else if (IsSymbol(object))
				{
					RefVar	pathExpr(AllocateArray(SYMA(pathExpr), 2));
					SetArraySlot(pathExpr, 0, object);
					SetArraySlot(pathExpr, 1, GetArraySlot(right, 1));
					return pathExpr;
				}
				else	// must be a pathExpr array
				{
					AddArraySlot(object, GetArraySlot(right, 1));
					return object;
				}
			}
			else
			{
				walkForCode(left, NO);
				return GetArraySlot(right, 1);
			}
		}
		else if (RINT(GetArraySlot(left, 0) == '.'))
		{
			RefVar	pathExpr(walkForPath(left, ioThrowIfNilObject));
			if (NOTNIL(pathExpr))
				emitPush(pathExpr);
			emit(kOpcodeGetPath, *ioThrowIfNilObject);
			*ioThrowIfNilObject = NO;
			walkForCode(right, NO);
		}
		else
		{
			walkForCode(left, NO);
			walkForCode(right, NO);
		}
	}
	else
	// MUST be a path node
		errorWithValue(kNSErrBadPath, inGraph);

	return NILREF;
}


/*------------------------------------------------------------------------------
	Walk the graph for stringer items.
	Args:		inGraph		
	Return:	number of items walked over
------------------------------------------------------------------------------*/

int
CCompiler::walkForStringer(RefArg inGraph)
{
	int	tokenType = RINT(GetArraySlot(inGraph, 0));
	if (tokenType == '&'
	||  tokenType == kTokenAmperAmper)
	{
		int	count1 = walkForStringer(GetArraySlot(inGraph, 1));
		int	spacer = (tokenType == kTokenAmperAmper) ? 1 : 0;
		if (spacer)
			emitPush(MakeStringFromCString(" "));
		int	count2 = walkForStringer(GetArraySlot(inGraph, 2));
		return count1 + spacer + count2;
	}
	walkForCode(inGraph, NO);
	return 1;
}


/*------------------------------------------------------------------------------
	Determine whether a PT represents a constant expression.
	Args:		inGraph		a parameterised token.
	Return:	YES => PT is a constant expression
------------------------------------------------------------------------------*/

BOOL
CCompiler::isConstantExpr(RefArg inGraph)
{
	int		tokenType = RINT(GetArraySlot(inGraph, 0));
	BOOL		result;

	if (tokenType == kTokenConst)
		return YES;

	RefVar	tokenValue(Length(inGraph) >= 2 ? GetArraySlot(inGraph, 1) : NILREF);
	if (tokenType == kTokenSymbol)
		result = func->isConstant(tokenValue);
	else if (tokenType == kTokenUMinus)
		result = isConstantExpr(tokenValue);
	else
		result = NO;

	return result;
}


/*------------------------------------------------------------------------------
	Evaluate a constant expression.
	Args:		inGraph		a parameterised token.
	Return:	a constant expression
------------------------------------------------------------------------------*/

Ref
CCompiler::evaluateConstantExpr(RefArg inGraph)
{
	int		tokenType = RINT(GetArraySlot(inGraph, 0));
	RefVar	tokenValue(Length(inGraph) >= 2 ? GetArraySlot(inGraph, 1) : NILREF);
	Ref		result;

	if (tokenType == kTokenConst)
		result = tokenValue;
	else if (tokenType == kTokenSymbol)
		result = func->getConstantValue(tokenValue, NULL);
	else if (tokenType == kTokenUMinus)
		result = MAKEINT(0) - evaluateConstantExpr(tokenValue);
	else
		result = NILREF;

	return result;
}

#pragma mark -

/*------------------------------------------------------------------------------
	C o d e   G e n e r a t i o n
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Find the index of a primitive function.
	Args:		inName		function name (symbol)
				inNumArgs	number of arguments passed
	Return:	freqFunc index
				-1 =>	function is not a freqFunc
						or expects a different number of arguments
------------------------------------------------------------------------------*/

ArrayIndex
CCompiler::freqFuncIndex(RefArg inName, ArrayIndex inNumArgs)
{
	Ref index = GetFrameSlot(gFreqFuncNames, inName);

	if (ISNIL(index)
	|| gFreqFuncInfo[RINT(index)].numOfArgs != inNumArgs)
		return kIndexNotFound;
	return RINT(index);
}


/*------------------------------------------------------------------------------
	Emit function call instruction.
	Args:		inName		function name (symbol)
				inNumArgs	number of arguments passed
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::emitFuncall(RefArg inName, ArrayIndex inNumArgs)
{
	Opcode		a;
	ArrayIndex	b;

	if ((b = freqFuncIndex(inName, inNumArgs)) != kIndexNotFound)
	{
		a = kOpcodeFreqFunc;
	}
	else
	{
		emitPush(inName);
		a = kOpcodeCall;
		b = inNumArgs;
	}
	emit(a, b);
}


/*------------------------------------------------------------------------------
	Emit return instruction.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::emitReturn(void)
{
	emit(kOpcodeSimple, kSimpleReturn);
}


/*------------------------------------------------------------------------------
	Emit branch instruction.
	Args:		inPC		destination of branch
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::emitBranch(ULong inPC)
{
	emit(kOpcodeBranch, inPC);
}


/*------------------------------------------------------------------------------
	Emit pop instruction.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::emitPop(void)
{
	emit(kOpcodeSimple, kSimplePop);
}


/*------------------------------------------------------------------------------
	Emit push instruction.
	If value is a magic pointer 0..4095
	or an immediate that will fit b (short) then push a constant.
	Args:		inValue		value to push
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::emitPush(RefArg inValue)
{
	Opcode	a;
	int		b;
	Ref		ref = inValue;

	if ((ISMAGICPTR(ref) && RVALUE(ref) < 4096)
	 || (!ISPTR(ref) && ref == (ref & 0xFFFF)))
	{
		b = ref;
		a = kOpcodePushConstant;
	}
	else
	{
		b = func->literalOffset(inValue);
		a = kOpcodePush;
	}
	emit(a, b);
}


/*------------------------------------------------------------------------------
	Emit variable operator: get value.
	Args:		inName		variable name (symbol)
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::emitVarGet(RefArg inName)
{
	Opcode	a;
	int		b;

	if (EQ(inName, SYMA(_parent)))
		warning("References to the variable “_parent” have undefined behavior");

	if (func->isLocalVariable(inName) && (b = func->variableIndex(inName)) != -1)
		a = kOpcodeGetVar;
	else
	{
		b = func->literalOffset(inName);
		a = kOpcodeFindVar;
	}
	emit(a, b);
}


/*------------------------------------------------------------------------------
	Emit variable operator: set value.
	Args:		inName		variable name (symbol)
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::emitVarSet(RefArg inName)
{
	Opcode	a;
	int		b;

	if (func->isLocalVariable(inName) && (b = func->variableIndex(inName)) != -1)
		a = kOpcodeSetVar;
	else
	{
		b = func->literalOffset(inName);
		a = kOpcodeFindAndSetVar;
	}
	emit(a, b);
}


/*------------------------------------------------------------------------------
	Emit variable operator: increment value.
	Args:		inName		variable name (symbol)
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::emitVarIncr(RefArg inName)
{
	int	b;

	if (func->isLocalVariable(inName) && (b = func->variableIndex(inName)) != -1)
		emit(kOpcodeIncrVar, b);
	else
		syntaxError("can't close over a for-loop index variable");
}

#pragma mark -

/*------------------------------------------------------------------------------
	E r r o r   R e p o r t i n g
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Issue a warning.
	Args:		msg		the warning
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::warning(const char * msg)
{
	REPprintf("File “%s”; Line %d ### Warning: %s\n",
				 stream->fileName(), stream->lineNumber(), msg);
}


/*------------------------------------------------------------------------------
	Issue a syntax error.
	Args:		msg		the error message
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::syntaxError(const char * msg)
{
	if (yychar != -1)
	{
		char		offender[64];
		char		str[512];
		char *	s = str;
		const char *	what;

		if (yychar > YYMAXTOKEN || (what = yyname[yychar]) == NULL)
			what = "illegal symbol";

		if (yychar == kTokenSymbol)
			sprintf(offender, "%s", SymbolName(yylval));
		else if (yychar == kTokenInteger)
			sprintf(offender, "%d", RINT(yylval));
		else if (yychar == kTokenReal)
			sprintf(offender, "%#g", CDouble(yylval));
		else
			offender[0] = 0;
		s += sprintf(s, "%s -- read %s %s, but wanted ", msg, what, offender);

		int r0, expectedCount = 0;
		for (ArrayIndex i = 0; i <= YYMAXTOKEN; i++)
		{
			if (((r0 = yyrindex[yystate] + i) >= 0
			   && r0 < YYTABLESIZE
			   && i == yycheck[r0])
			 || ((r0 = yysindex[yystate] + i) >= 0
			   && r0 < YYTABLESIZE
			   && i == yycheck[r0]))
			{
				if (expectedCount++ > 0)
					s += sprintf(s, ", ");
				s += sprintf(s, "%s", yyname[i]);
			}
		}
		*s = 0;
		errorWithValue(kNSErrSyntaxError, MakeStringFromCString(str));
	}
	errorWithValue(kNSErrSyntaxError, MakeStringFromCString(msg));
}


/*------------------------------------------------------------------------------
	Throw a syntax error exception.
	Args:		inErr		the error code
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::error(NewtonErr inErr)
{
	RefVar	fr(AllocateFrame());

	SetFrameSlot(fr, SYMA(errorCode), MAKEINT(inErr));
	SetFrameSlot(fr, SYMA(filename), MakeStringFromCString(stream->fileName()));
	SetFrameSlot(fr, SYMA(lineNumber), MAKEINT(stream->lineNumber()));

	ThrowRefException(exCompilerData, fr);
}


/*------------------------------------------------------------------------------
	Throw a syntax error exception.
	Args:		inErr		the error code
				inVal		a value in error
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::errorWithValue(NewtonErr inErr, RefArg inVal)
{
	RefVar	fr(AllocateFrame());

	SetFrameSlot(fr, SYMA(errorCode), MAKEINT(inErr));
	SetFrameSlot(fr, SYMA(value), inVal);
	SetFrameSlot(fr, SYMA(filename), MakeStringFromCString(stream->fileName()));
	SetFrameSlot(fr, SYMA(lineNumber), MAKEINT(stream->lineNumber()));

	ThrowRefException(exCompilerData, fr);
}
