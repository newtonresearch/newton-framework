/*
	File:		Decompiler.cc

	Contains:	Functions to print a NewtonScript function object.

	Written by:	Newton Research Group, 2016.
					Based on US patent 5860008, "Method and apparatus for decompiling a compiled interpretive code" filed Feb 2 1996
*/

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
using namespace std;

#include "Objects.h"
#include "RefMemory.h"
#include "Iterators.h"
#include "Lookup.h"
#include "Interpreter.h"
#include "RichStrings.h"
#include "UStringUtils.h"
#include "ROMResources.h"
#include "Opcodes.h"

#include "REPTranslators.h"


#define useTabs 1

enum {
	kFragmentPop,
	kFragmentReturn,
	kFragmentIterNext,
	kFragmentIterDone,
	kFragmentPopHandlers,
	kFragmentBranch,
	kFragmentBranchIfTrue,
	kFragmentBranchIfFalse,
	kFragmentSetPath,
	kFragmentSetPathAndPush,
	kFragmentSetVar,
	kFragmentFindAndSetVar,
	kFragmentIncrVar,
	kFragmentForLoop,
	kFragmentNewHandlers,
	kFragmentFor,
	kFragmentLoop,
	kFragmentForeachEnd,
	kFragmentIf,
	kFragmentHandler
};

/* -----------------------------------------------------------------------------
	D e b u g
----------------------------------------------------------------------------- */
enum {
	kDumpRaw					=  1,
	kDumpFragments			= (1 <<  1),
	kDumpHandlers			= (1 <<  2),
	kDumpLoop				= (1 <<  3),
	kDumpForeachCollect	= (1 <<  4),
	kDumpForeach			= (1 <<  5),
	kDumpRepeat				= (1 <<  6),
	kDumpWhile				= (1 <<  7),
	kDumpFor					= (1 <<  8),
	kDumpBreak				= (1 <<  9),
	kDumpIf					= (1 << 10),
	kDumpElse				= (1 << 11),
	kDumpFinal				= (1 << 12),
	kDumpEverything		= 0x1FFE
};
int gDebugBits = 0;	//kDumpRaw+kDumpFragments;


#define XASSERTMSG(expr,msg) do { if (!(expr)) REPprintf("*** "#msg" ***\n"); } while (0)

const char * kFragmentNames[] = {
	"Pop",
	"Return",
	"IterNext",
	"IterDone",
	"PopHandlers",
	"Branch",
	"BranchIfTrue",
	"BranchIfFalse",
	"SetPath",
	"SetPath",
	"SetVar",
	"FindAndSetVar",
	"IncrVar",
	"ForLoop",
	"NewHandlers",
	"For",
	"Loop",
	"ForeachEnd",
	"IfThenElse",
	"Handler"
};


/* -----------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
----------------------------------------------------------------------------- */
extern "C" {
Ref FPrimClassOf(RefArg inRcvr, RefArg inObj);
Ref FSetContains(RefArg inRcvr, RefArg inArray, RefArg inMember);

ArrayIndex	GetFunctionArgCount(Ref fn);
const char * GetMagicPointerString(int inMP);
}

extern void Disassemble(RefArg inFunc);

string DumpObject(RefArg obj);
string DumpCode(RefArg inObj);


/*------------------------------------------------------------------------------
	P S t r i n g O u t T r a n s l a t o r
	Output to string.
------------------------------------------------------------------------------*/
extern void PrintObjectAux(Ref inObj, int indent, int inDepth);

PROTOCOL PStringOutTranslator : public POutTranslator
	PROTOCOLVERSION(1.0)
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(PStringOutTranslator)

	PStringOutTranslator *	make(void);
	void			destroy(void);

	NewtonErr	init(void * inContext);
	Timeout		idle(void);
	void			consumeFrame(RefArg inObj, int inDepth, int indent);
	void			prompt(int inLevel);
	int			print(const char * inFormat, ...);
	int			vprint(const char * inFormat, va_list args);
	int			putc(int inCh);
	void			flush(void);

	void			enterBreakLoop(int);
	void			exitBreakLoop(void);

	void			stackTrace(void * interpreter);
	void			exceptionNotify(Exception * inException);

	const char * string(void);

private:
	void			realloc(void);

	char *		fString;
	ArrayIndex	fAllocated;
	ArrayIndex	fLength;
};


/* -----------------------------------------------------------------------------
	T y p e s
----------------------------------------------------------------------------- */

/* -----------------------------------------------------------------------------
	E x p r
	Text representation of an expression.
----------------------------------------------------------------------------- */
struct Expr {
	Expr(string const& inText, int inPrecedence = 150);
	string exprText(int inPrecedence);

	string text;
	int precedence;
};

Expr::Expr(string const& inText, int inPrecedence)
{
	text = inText;
	precedence = inPrecedence;
}

string
Expr::exprText(int inPrecedence)
{
	if (inPrecedence > precedence) {
		return '(' + text + ')';
	}
	return text;
}


/* -----------------------------------------------------------------------------
	F r a g m e n t
	Text representation of a code fragment.
----------------------------------------------------------------------------- */
struct Fragment {
	Fragment(ArrayIndex inAddr, string const& inText, ArrayIndex inOpcode, ArrayIndex inBranch = 0);
	void	dump(void);
	void	dump(vector<Fragment>& inFragments);

	ArrayIndex addr;
	ArrayIndex opcode;
	ArrayIndex branch;
	string text;
};

Fragment::Fragment(ArrayIndex inAddr, string const& inText, ArrayIndex inOpcode, ArrayIndex inBranch)
{
	addr = inAddr;
	text = inText;
	opcode = inOpcode;
	branch = inBranch;
}

void
Fragment::dump(void)
{
	REPprintf("[%4d \"%s\" %s %d]\n", addr, text.c_str(), kFragmentNames[opcode], branch);
}

void
Fragment::dump(vector<Fragment>& inFragments)
{
	dump();
	REPprintf("-- in all fragments --\n");
	for (vector<Fragment>::iterator it = inFragments.begin(); it != inFragments.end(); ++it) {
		it->dump();
	}
	REPprintf("\n");
}


/* -----------------------------------------------------------------------------
	H a n d l e r
	Text representation of a try-onexception handler.
----------------------------------------------------------------------------- */
struct Handler {
	Handler(string inText, ArrayIndex inAddr = 0);

	string text;
	ArrayIndex addr;
};

Handler::Handler(string inText, ArrayIndex inAddr)
{
	text = inText;
	addr = inAddr;
}



#pragma mark -
/* -----------------------------------------------------------------------------
	S t r i n g   U t i l i t i e s
----------------------------------------------------------------------------- */

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


void
TrimString(string& str)
{
	int i, count;
	for (i = 0, count = str.size(); i < count; ++i) {
		if ((sType[str[i]] & isWhitespace) == 0) {
			break;
		}
	}
	if (i > 0) {
		str.erase(str.begin(), str.begin()+i);
	}
	for (i = str.size(); i > 0; --i) {
		if ((sType[str[i-1]] & isWhitespace) == 0) {
			break;
		}
	}
	if (i < str.size()) {
		str.erase(str.begin()+i);
	}
}


static bool
BeginsWith(string const& str, string const& find)
{
	return str.find(find) == 0;
}

static bool
EndsWith(string const& str, string const& find)
{
	return (str.size() >= find.size()) && str.rfind(find) == str.size() - find.size();
}

static void
StrReplace(string& str, string const& find, string const& replace)
{
	for (string::size_type i = 0; (i = str.find(find, i)) != string::npos;) {
		str.replace(i, find.length(), replace);
		i += replace.length();
	}
}

static vector<string>
SplitString(string const& str)	// str is a string representation of an array
{
	vector<string> a;
	return a;
}


string
GetFirstWord(string& str)
{
	string::size_type i = str.find(' ');
	if (i != string::npos) {
		return str.substr(0,i);
	}
	return str;
}

string
GetLastWord(string& str)
{
	string::size_type i = str.find(":= ");
	if (i != string::npos) {
		return str.substr(i+3);
	}
	i = str.rfind(' ', 0);
	if (i != string::npos) {
		return str.substr(i+1);
	}
	return str;
}

string
GetIterator(string& str, bool& isDeeply)
{
	isDeeply = false;
	// find xxxx var name in "NewIterator(xxxx, nil)"
	string::size_type j,i = str.find("NewIterator(");
	if (i != string::npos) {
		i += 12;
		j = str.find(',', i);
		if (j != string::npos) {
			isDeeply = (str.find("true", j) != string::npos);
			return str.substr(i,j-i);
		}
	}
	return "";
}

/* -----------------------------------------------------------------------------
	Create hex string representation of a number.
----------------------------------------------------------------------------- */

const char *
HexStr(int num, int size)
{
	static char str[16+1], * s = str+16;
	str[16] = 0;
	if (size > 16) {
		size = 16;
	}
	bool isNegative = (num < 0);
	if (isNegative) {
		num = -num;
		--size;		// really?
	}
	do {
		*--s = "0123456789ABCDEF"[num & 0x0F];
		num = num >> 4;
		--size;
	} while (num > 0 || size > 0);
	if (isNegative) {
		*--s = '-';
	}
	return s;
}

bool
Contains(vector<string>& inArray, string& inString)
{
	for (vector<string>::iterator s = inArray.begin(); s != inArray.end(); ++s) {
		if (*s == inString) {
			return true;
		}
	}
	return false;
}

void
Remove(vector<string>& inArray, string inString)
{
	for (vector<string>::iterator s = inArray.begin(); s != inArray.end(); ++s) {
		if (*s == inString) {
			*s = "";
			break;
		}
	}
}

vector<string>
MakeArrayFromString(string const& inString)
{
	vector<string> a;
	string::size_type i, j;
	for (i = 1; (j = inString.find(", ", i)) != string::npos; i = j+2) {
		a.push_back(inString.substr(i,j-i));
	}
	a.push_back(inString.substr(i,inString.length()-1-i));
	return a;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	Create array of text strings representing lines(fragments) of code.
	At this stage we have a bona-fide function object, var names and literals
	already extracted.
	Args:		inFunc			a function object
				inVarNames
				inLiteralStrs
	Return:	array of code fragments
----------------------------------------------------------------------------- */

vector<Fragment>
MakeFragments(RefArg inFunc, vector<string> const& varNames, vector<string> const& literalStrs)
{
	bool is_function = EQ(ClassOf(inFunc), SYMA(_function));
	RefVar instructions(GetArraySlot(inFunc, kFunctionInstructionsIndex));
	RefVar literals(GetArraySlot(inFunc, kFunctionLiteralsIndex));

	vector<Handler> handlers;
	vector<Expr> stack;
	ArrayIndex lineStart = 0;
	vector<Fragment> lines;
	Expr expr("");
	string str;

	// set up instruction pointers
	CDataPtr instrData(instructions);
	unsigned char * instrPtr = (unsigned char *)instrData;
	ArrayIndex instrCount = Length(instructions);
	ArrayIndex instrLen;

	for (ArrayIndex pc = 0; pc < instrCount; pc += instrLen, instrPtr += instrLen) {
		unsigned int opCode = *instrPtr >> 3;
		unsigned int b = *instrPtr & 0x07;
		if (b == 7) {
			b = (*(instrPtr + 1) << 8) + *(instrPtr + 2);
			instrLen = 3;
		} else {
			instrLen = 1;
		}

		switch (opCode) {
		case kOpcodeSimple:
			switch (b) {
			case kSimplePop:
				// stack:	x --
				if (stack.empty()) {
					REPprintf("\n-- tried to pop an empty stack! --\n");
				} else {
					str = stack.back().text;
					stack.pop_back();
				}
				if (str.size() > 0) {
					lines.push_back(Fragment(lineStart, str, kFragmentPop));
				}
				lineStart = pc + instrLen;
				break;
			case kSimpleDup:
				// stack:	x -- x x
					REPprintf("\n-- tried to dup item on stack! --\n");
				// not handled
				break;
			case kSimpleReturn:
				// stack:	--
				if (!stack.empty()) {
					// there’s a value to return
					if (pc < instrCount-1) {
						str = "return " + stack.back().text;
						// ignore following pop (this is a compiler idiom to keep the stack aligned)
						if (*(instrPtr + 1) == kOpcodeSimple+kSimplePop) {
							++instrPtr; ++pc;
						}
					} else {
						// at the end of the function we don’t need explicitly to return a value;
						str = stack.back().text;
					}
					stack.pop_back();
					lines.push_back(Fragment(lineStart, str, kFragmentReturn));
					lineStart = pc + instrLen;
				} else {
					if (pc < instrCount-1) {
						lines.push_back(Fragment(lineStart, "", kFragmentReturn));
					}
				}
				break;
			case kSimplePushSelf:
				// stack:	-- rcvr
				stack.push_back(Expr("self"));
				break;
			case kSimpleSetLexScope:
				// stack:	func -- closure
				//stack.back() = DumpCode(stack.back());
				break;
			case kSimpleIterNext:
				// stack:	iterator --
				str = "IteratorNext(" + stack.back().text + ')';
				stack.pop_back();
				lines.push_back(Fragment(lineStart, str, kFragmentIterNext));	//'iterdone
				lineStart = pc + instrLen;
				break;
			case kSimpleIterDone:
				// stack:	iterator -- done?
				str = "IteratorDone(" + stack.back().text + ')';
				lines.push_back(Fragment(lineStart, str, kFragmentIterDone));	//'pophandlers
				lineStart = pc + instrLen;
				break;
			case kSimplePopHandlers:
				// stack:	--
				if (!stack.empty()) {
					// why would you do this?
					str = stack.back().text;
					stack.pop_back();
					lines.push_back(Fragment(lineStart, str, kFragmentPop));	//'pop
						lineStart = pc;
				}
				str = handlers.back().text;
				lines.push_back(Fragment(lineStart, str, kFragmentPopHandlers, handlers.back().addr));	//'callhandlers
				lineStart = pc + instrLen;
				break;
			}
			break;

		case kUnary2:
			// unused
			stack.push_back(Expr("unary2 " + to_string(b)));
			break;
		case kUnary3:
			stack.push_back(Expr("unary3 " + to_string(b)));
			// unused
			break;

		case kOpcodePush: {
			// stack:	-- literal
				RefVar obj(GetArraySlot(literals, b));
				if (EQ(ClassOf(obj), SYMA(symbol))) { // || (EQ(FPrimClassOf(RA(NILREF), obj), SYMA(frame)) && !(EQ(ClassOf(obj), SYMA(CodeBlock)) || EQ(ClassOf(obj), SYMA(_function))))) {
					stack.push_back(Expr("'" + literalStrs[b]));
				} else {
					stack.push_back(Expr(literalStrs[b]));
				}
			}
			break;

		case kOpcodePushConstant:
			// stack:	-- value
			if (ISINT(b)) {
				stack.push_back(Expr(to_string(RVALUE(b))));
			} else  if (ISMAGICPTR(b)) {
				stack.push_back(Expr(GetMagicPointerString(RVALUE(b))));
			} else if (ISPTR(b)) {
				stack.push_back(Expr(string("#") + HexStr(b, 8)));
			} else if (ISIMMED(b)) {
				// try boolean
				if (EQRef(b, TRUEREF)) {
					str = "true";
				} else if (ISNIL(b)) {
					str = "nil";
				}
			// try character
				else if (ISCHAR(b)) {
					UniChar	ch = RCHAR(b);
					if (ch > 0xFF) {
						// it’s a Unicode char
						str = string("$\\u") + HexStr(ch, 4);
					} else if (ch >= 0x20 && ch <= 0x7F) {
						// it’s a printable ASCII char
						if (ch == 0x5C)
							str = "$\\\\";
						else
							str = string("$") + (char)ch;
					// else it’s a control char
					} else if (ch == 0x09) {
						str = "$\\t";
					} else if (ch == 0x0D) {
						str = "$\\n";
					} else {
						str = string("$\\") + HexStr(ch, 2);
					}
				} else {
					str = DumpObject(b);	// should never happen
				}
				stack.push_back(Expr(str));
			}
			break;

		case kOpcodeCall: {
			// stack:	arg1 arg2 ... argN name -- result
			// b = num of args
				str = stack.back().text.substr(1);
				stack.pop_back();

				int precedence = 0;
				if (str == "|<<|" || str == "|>>|") {
					str = str.substr(1,2);
					precedence = 90;
				} else if (str == "div" || str == "mod") {
					precedence = 80;
				}
				if (precedence > 0) {
					// format as infix operator
					str = ' ' + str + ' ';
					if (b != 2) {
						REPprintf("\n-- infix operator with more than 2 args! --\n");
					}
					for (ArrayIndex i = b; i > 0; --i) {
						expr = stack.back();
						stack.pop_back();
						if (i > 1) {
							str = str + expr.exprText(precedence);
						} else {
							str = expr.exprText(precedence) + str;
						}
					}
					stack.push_back(Expr(str, precedence));
				} else {
					string args;
					for (ArrayIndex i = b; i > 0; --i) {
						args = stack.back().text + args;
						stack.pop_back();
						if (i > 1) {
							args = ", " + args;
						}
					}
					stack.push_back(Expr(str + '(' + args + ')'));
				}
			}
			break;

		case kOpcodeInvoke: {
			// stack:	arg1 arg2 ... argN func -- result
			// b = num of args
				str = stack.back().text;
				stack.pop_back();
				if (str[0] == '\'') {
					str.erase(0);
				}
				string args;
				for (ArrayIndex i = b; i > 0; --i) {
					args = stack.back().text + args;
					if (i > 1) {
						args = ", " + args;
					}
					stack.pop_back();
				}
				stack.push_back(Expr("call " + str + " with (" + args + ')'));
			}
			break;

		case kOpcodeSend:
		case kOpcodeSendIfDefined: {
			// stack:	arg1 arg2 ... argN rcvr name -- result
			// b = num of args
				str = stack.back().text.substr(1);
				stack.pop_back();
				str = stack.back().text + ((opCode == kOpcodeSend)? ":" : ":?") + str;
				stack.pop_back();
				string args;
				for (ArrayIndex i = b; i > 0; --i) {
					args = stack.back().text + args;
					if (i > 1) {
						args = ", " + args;
					}
					stack.pop_back();
				}
				stack.push_back(Expr(str + '(' + args + ')', 120));
			}
			break;

		case kOpcodeResend:
		case kOpcodeResendIfDefined: {
			// stack:	arg1 arg2 ... argN name -- result
			// b = num of args
				str = stack.back().text.substr(1);
				stack.pop_back();
				str = ((opCode == kOpcodeResend)? "inherited:" : "inherited:?") + str;
				string args;
				for (ArrayIndex i = b; i > 0; --i) {
					args = stack.back().text + args;
					if (i > 1) {
						args = ", " + args;
					}
					stack.pop_back();
				}
				stack.push_back(Expr(str + '(' + args + ')', 120));
			}
			break;

		case kOpcodeBranch:
			// stack:	--
			if (b > pc) {
				// forward branch
				if ((*(instrPtr+instrLen) >> 3) == kOpcodeSetVar) {
					ArrayIndex setVarInstrLen = ((*(instrPtr+instrLen) & 0x07) == 7)? 3 : 1;
					if (*(instrPtr+instrLen+setVarInstrLen) == kSimplePop && *(instrPtr+instrLen+setVarInstrLen+1) == kSimplePop) {
						// end of foreach-collect -- ignore the pops
						instrLen += (setVarInstrLen + 2);
					}
				} else if (*(instrPtr+instrLen) == kSimplePop) {
					// loop exit -- ignore the pop
					++instrLen;
				}
			}
			if (stack.empty()) {
				str = "";
			} else {
				// we might need the stack value for if-then-else; but don’t pop it
				str = stack.back().text;
			}
			lines.push_back(Fragment(lineStart, str, kFragmentBranch, b));
			lineStart = pc + instrLen;
			break;

		case kOpcodeBranchIfTrue:
			// stack:	value --
			str = stack.back().text;
			stack.pop_back();
			lines.push_back(Fragment(lineStart, str, kFragmentBranchIfTrue, b));
			lineStart = pc + instrLen;
			break;

		case kOpcodeBranchIfFalse:
			// stack:	value --
			str = stack.back().text;
			stack.pop_back();
			lines.push_back(Fragment(lineStart, str, kFragmentBranchIfFalse, b));
			lineStart = pc + instrLen;
			break;

		case kOpcodeFindVar:
			// stack:	-- value
			stack.push_back(Expr(literalStrs[b]));
			break;

		case kOpcodeGetVar:
			// stack:	-- value
			stack.push_back(Expr(varNames[b]));
			break;

		case kOpcodeMakeFrame: {
			// stack:	val1 val2 ... valN map -- frame
			// b = num of slots
				string map = stack.back().text;
				stack.pop_back();
				string tag;
				string::size_type tagStart, tagEnd = map.length()-1;
				str.clear();
				for (ArrayIndex i = b; i > 0; --i) {
					tagStart = map.rfind(',', tagEnd-1)+3;
					tag = map.substr(tagStart,tagEnd-tagStart);
					tagEnd = tagStart-3;
					str = tag +  ": " + stack.back().text + str;
					stack.pop_back();
					if (i > 1) {
						str = ", " + str;
					}
				}
				stack.push_back(Expr('{' + str + '}'));
			}
			break;

		case kOpcodeMakeArray: {
			// stack:	B = 0xFFFF: size class -- array
			//				B < 0xFFFF: val1 val2 ... valN class -- array
				string aClass = stack.back().text.substr(1);
				stack.pop_back();
				if (aClass == "array") {
					aClass = "";
				} else {
					aClass += ": ";
				}
				str.clear();
				if (b == 0xFFFF) {
					b = 1;	// we’re not interested in the value but we need to pop it off the stack
				}
				for (ArrayIndex i = b; i > 0; --i) {
					str = stack.back().text + str;
					stack.pop_back();
					if (i > 1) {
						str = ", " + str;
					}
				}
				stack.push_back(Expr('[' + aClass + str + ']'));
			}
			break;

		case kOpcodeGetPath:
			// stack:	object pathExpr -- value
			// NOS1:		B > 0: pathExpr is B elements on the stack
			if (is_function) {
				str = stack.back().text;
				stack.pop_back();
				if (str[0] == '\'') {
					str[0] = '.';
				} else if (str.find('.') != string::npos) {
					/* it’s a path expression */
					str.insert(0, ".");
				} else {
					str = ".(" + str + ')';
				}
				str = stack.back().text + str;
				stack.pop_back();
			} else {
				string path;
				for (ArrayIndex i = 0; i < b; ++i) {
					str = stack.back().text;
					stack.pop_back();
					if (str[0] == '\'') {
						path = '.' + str.substr(1) + path;
					} else if (str.find('.') != string::npos) {
						/* it’s a path expression */
						path = '.' + str + path;
					} else {
						path = ".(" + str + ')' + path;
					}
				}
				str = stack.back().text + path;
				stack.pop_back();
			}
			stack.push_back(Expr(str, 130));
			break;

		case kOpcodeSetPath: {
			// stack:	B = 0: object pathExpr value --
			//				B = 1: object pathExpr value -- value
			//	NOS1:		B > 0: pathExpr is B elements on the stack
				str = stack.back().text;
				stack.pop_back();

				if (is_function) {
					string path = stack.back().text;
					stack.pop_back();
					if (path[0] == '\'') {
						path[0] = '.';
					} else if (path.find('.') != string::npos) {
						/* it’s a path expression */
						str.insert(0, ".");
					} else {
						path = ".(" + path + ')';
					}
					str = stack.back().text + path + " := " + str;
					stack.pop_back();
					if (b > 0) {
						stack.push_back(Expr(str, 10));
					}
				} else {
					string path;
					string pathElement;
					if (b == 0) {
						b = 1;
					}
					for (ArrayIndex i = 0; i < b; ++i) {
						pathElement = stack.back().text;
						stack.pop_back();
						if (pathElement[0] == '\'') {
							path = '.' + pathElement.substr(1) + path;
						} else if (pathElement.find('.') != string::npos) {
							/* it’s a path expression */
							path = '.' + pathElement + path;
						} else {
							path = ".(" + pathElement + ')' + path;
						}
					}
					b = 0;
					str = stack.back().text + path + " := " + str;
					stack.pop_back();
				}

				lines.push_back(Fragment(lineStart, str, b > 0? kFragmentSetPathAndPush : kFragmentSetPath));
				lineStart = pc + instrLen;
			}
			break;

		case kOpcodeSetVar:
			// stack:	value --
			if (stack.empty()) {
				// global assignment
				lines.back().text = varNames[b] + " := " + lines.back().text;
			} else {
				str = varNames[b] + " := " + stack.back().text;
				stack.pop_back();
				lines.push_back(Fragment(lineStart, str, kFragmentSetVar));
				lineStart = pc + instrLen;
			}
			break;

		case kOpcodeFindAndSetVar:
			// stack:	value --
			str = literalStrs[b] + " := " + stack.back().text;
			stack.pop_back();
			lines.push_back(Fragment(lineStart, str, kFragmentFindAndSetVar));
			lineStart = pc + instrLen;
			break;

		case kOpcodeIncrVar:
			// stack:	addend -- addend value
			str = "IncrVar(" + stack.back().text + ")";
			stack.push_back(Expr(varNames[b]));
			lines.push_back(Fragment(lineStart, str, kFragmentIncrVar));
			lineStart = pc + instrLen;
			break;

		case kOpcodeBranchIfLoopNotDone:
			// stack:	incr index limit --
			str = stack.back().text;					// limit
			stack.pop_back();
			str = stack.back().text + ':' + str;	// index
			stack.pop_back();
			str = stack.back().text + ':' + str;	// incr
			stack.pop_back();
			stack.pop_back();						// index |
			stack.pop_back();						// incr  | that compiler pushed at start of loop
			lines.push_back(Fragment(lineStart, str, kFragmentForLoop, b));
			lineStart = pc + instrLen;
			break;

		case kOpcodeFreqFunc:
		// stack:	val1 val2 ... valN -- result
			expr = stack.back();
			stack.pop_back();
			switch (b) {
			case 0:	// +
				expr.text = stack.back().exprText(70) + " + " + expr.exprText(70);
				expr.precedence = 70;
				stack.pop_back();
				break;
			case 1:	// -
				expr.text = stack.back().exprText(70) + " - " + expr.exprText(70);
				expr.precedence = 70;
				stack.pop_back();
				break;
			case 2:	// aref
				expr.text = stack.back().exprText(110) + '[' + expr.text + ']';
				expr.precedence = 110;
				stack.pop_back();
				break;
			case 3:	// setAref
				expr.text = '[' + stack.back().text + "] := " + expr.exprText(10);
				expr.precedence = 10;
				stack.pop_back();
				expr.text = stack.back().text + expr.text;
				stack.pop_back();
				break;
			case 4:	// =
				expr.text = stack.back().exprText(40) + " = " + expr.exprText(40);
				expr.precedence = 40;
				stack.pop_back();
				break;
			case 5:	// not
				expr.text = "not " + expr.exprText(30);
				expr.precedence = 30;
				break;
			case 6:	// <>
				expr.text = stack.back().exprText(40) + " <> " + expr.exprText(40);
				expr.precedence = 40;
				stack.pop_back();
				break;
			case 7:	// *
				expr.text = stack.back().exprText(80) + " * " + expr.exprText(80);
				expr.precedence = 80;
				stack.pop_back();
				break;
			case 8:	// /
				expr.text = stack.back().exprText(80) + " / " + expr.exprText(80);
				expr.precedence = 80;
				stack.pop_back();
				break;
			case 9:	// div
				expr.text = stack.back().exprText(80) + " div " + expr.exprText(80);
				expr.precedence = 80;
				stack.pop_back();
				break;
			case 10:	// <
				expr.text = stack.back().exprText(40) + " < " + expr.exprText(40);
				expr.precedence = 40;
				stack.pop_back();
				break;
			case 11:	// >
				expr.text = stack.back().exprText(40) + " > " + expr.exprText(40);
				expr.precedence = 40;
				stack.pop_back();
				break;
			case 12:	// >=
				expr.text = stack.back().exprText(40) + " >= " + expr.exprText(40);
				expr.precedence = 40;
				stack.pop_back();
				break;
			case 13:	// <=
				expr.text = stack.back().exprText(40) + " <= " + expr.exprText(40);
				expr.precedence = 40;
				stack.pop_back();
				break;
			case 14:	// band
				expr.text = "band(" + stack.back().text + ", " + expr.text + ')';
				expr.precedence = 150;
				stack.pop_back();
				break;
			case 15:	// bor
				expr.text = "bor(" + stack.back().text + ", " + expr.text + ')';
				expr.precedence = 150;
				stack.pop_back();
				break;
			case 16:	// bnot
				expr.text = "bnot(" + stack.back().text + ')';		// original says 2 args!
				expr.precedence = 150;
				stack.pop_back();
				break;
			case 17:	// newIterator
				expr.text = "NewIterator(" + stack.back().text + ", " + expr.text + ')';
				expr.precedence = 150;
				stack.pop_back();
				break;
			case 18:	// Length
				expr.text = "Length(" + expr.text + ')';
				expr.precedence = 150;
				break;
			case 19:	// Clone
				expr.text = "Clone(" + expr.text + ')';
				expr.precedence = 150;
				break;
			case 20:	// SetClass
				expr.text = "SetClass(" + stack.back().text + ", " + expr.text + ')';
				expr.precedence = 150;
				stack.pop_back();
				break;
			case 21:	// AddArraySlot
				expr.text = "AddArraySlot(" + stack.back().text + ", " + expr.text + ')';
				expr.precedence = 150;
				stack.pop_back();
				break;
			case 22: {	// Stringer
					// expr.text is text representation of an array: unpack it to an array
					vector<string> array = MakeArrayFromString(expr.text);
					// map to string w/ & &&
					bool isSpaced = false;
					vector<string>::iterator s = array.begin();
					expr.text = *s++;
					for ( ; s != array.end(); ++s) {
						if (*s == "\" \"") {
							expr.text += " && ";
							isSpaced = true;
						} else {
							if (!isSpaced) {
								expr.text += " & ";
							}
							expr.text += *s;
							isSpaced = false;
						}
					}
					expr.precedence = 60;
				}
				break;
			case 23:	// exists
				if (expr.text[0] == '\'') {
					expr.text.erase(0);
				} else {
					expr.text = '(' + expr.text + ')';
				}
				expr.text = stack.back().text + '.' + expr.text + " exists";
				expr.precedence = 50;
				stack.pop_back();
				break;
			case 24:	// ClassOf
				expr.text = "ClassOf(" + expr.text + ')';
				expr.precedence = 150;
				break;
			default:
				REPprintf("Unknown instruction: %4d %02X\n", pc, *instrPtr);
			}
			stack.push_back(expr);
			break;

		case kOpcodeNewHandlers:
			// stack:	sym1 pc1 sym2 pc2 ... symN pcN --
			str = "try";
			lines.push_back(Fragment(lineStart, str, kFragmentNewHandlers));
			lineStart = pc + instrLen;
			if (b == 0) {
				handlers.push_back(Handler("onexception do", 0));
			} else for (ArrayIndex i = 0; i < b; ++i) {
				str = stack.back().text;
				stack.pop_back();
				handlers.push_back(Handler("onexception " + stack.back().text.substr(1) + " do", stoul(str)));
				stack.pop_back();
			}
			break;

		default:
			REPprintf("Unknown instruction: %4d %02X", pc, *instrPtr);
			if (instrLen > 1) {
				REPprintf(" %02X%02X", *(instrPtr+1), *(instrPtr+2));
			}
			REPprintf("\n");
			break;
		}
	}
	return lines;
}


/* -----------------------------------------------------------------------------
	Dump the decompilation of a function object into a std::string.
	Args:		inFunc		a function object
								must be of class 'CodeBlock (OS1)
								or '_function (=>kPlainFuncClass) (OS2)
	Return	string
----------------------------------------------------------------------------- */

string
DumpCode(RefArg inFunc)
{
	static bool isTranslatorRegistered = false;
	if (!isTranslatorRegistered) {
		isTranslatorRegistered = true;
		PStringOutTranslator::classInfo()->registerProtocol();
	}

	if (FLAGTEST(gDebugBits, kDumpRaw)) {
		Disassemble(inFunc);
	}

	RefVar funcClass = ClassOf(inFunc);
	if (!(EQ(funcClass, SYMA(CodeBlock)) || EQ(funcClass, SYMA(_function)))) {
		ThrowMsg("not a codeblock");
	}
	bool is_function = EQ(funcClass, SYMA(_function));

// extract arg/local names from function frame
// if 'CodeBlock (OS1): argFrame contains arg and local names
// if _function (OS2): argFrame contains only _nextArgFrame,_parent,_implementor scope
//		if the function was compiled with global dbgKeepVarNames=true or dbgNoVarNames=false then names are contained in the debug slot of the function frame
//		if we have no debugInfo then we make up arg and local names
	RefVar literals(GetArraySlot(inFunc, kFunctionLiteralsIndex));
	RefVar argFrame(GetArraySlot(inFunc, kFunctionArgFrameIndex));
	RefVar debugInfo;
	if (Length(inFunc) > kFunctionDebugIndex) {
		debugInfo = GetArraySlot(inFunc, kFunctionDebugIndex);
	}
	if (!EQ(ClassOf(debugInfo), SYMA(dbg1))) {
		debugInfo = NILREF;
	}
	ArrayIndex argCount = RINT(GetArraySlot(inFunc, kFunctionNumArgsIndex));
	ArrayIndex localCount = argCount >> 16;
	argCount &= 0xFF;

	vector<string> varNames;
	if (is_function) {
		// _function (OS2): arg and local names are contained in the debug slot of the function frame
		varNames.push_back("_nextArgFrame");
		varNames.push_back("_parent");
		varNames.push_back("_implementor");
		if (NOTNIL(debugInfo)) {
			ArrayIndex debugNamesBaseIndex = 1 + RINT(GetArraySlot(debugInfo, 0));
			for (ArrayIndex i = debugNamesBaseIndex, count = Length(debugInfo); i < count; ++i) {
				string litrl = DumpObject(GetArraySlot(debugInfo, i));
				if (litrl[0] == '\'') {
					litrl.erase(0,1);
				}
				varNames.push_back(litrl);
			}
		}
	} else {
		// CodeBlock (OS1): argFrame contains arg and local names
		FOREACH_WITH_TAG(argFrame, tag, slot)
			string litrl = DumpObject(tag);
			if (litrl[0] == '\'') {
				litrl.erase(0,1);
			}
			varNames.push_back(litrl);
		END_FOREACH;
	}
	// if we have no debug info then make up arg and local names
	if (varNames.size() < 3 + argCount + localCount) {
		char varName[8];
		for (ArrayIndex i = 0; i < argCount; ++i) {
			sprintf(varName, "arg%d", i+1);
			varNames.push_back(varName);
		}
		for (ArrayIndex i = 0; i < localCount; ++i) {
			sprintf(varName, "local%d", i+1);
			varNames.push_back(varName);
		}
	}

	// build literal strings
	vector<string> literalStrs;
	if (NOTNIL(literals)) {
		FOREACH(literals, slot)
			RefVar litClass = ClassOf(slot);
			if (EQ(litClass, SYMA(CodeBlock))) {
				string str = DumpCode(slot) + ';';
				literalStrs.push_back(str);
			} else if (EQ(litClass, SYMA(string))) {
				string str;
				UniChar c, * s = GetUString(slot);
				for (ArrayIndex i = 0, count = Length(slot)/sizeof(UniChar) - 1; i < count; ++i, ++s) {
					c = *s;
					if (c == '\n') {
						str.append("\\n");
					} else if (c == '\t') {
						str.append("\\t");
					} else if (c == '"') {
						str.append("\\\"");
					} else {
						str.push_back(c);
					}
				}
				literalStrs.push_back("\"" + str + "\"");
			} else if (EQ(litClass, SYMA(char))) {
				string str;
				UniChar	ch = RCHAR(slot);
				if (ch > 0xFF) {
					str = string("$\\u") + HexStr(ch, 4);
				} else if (ch >= 0x20 && ch <= 0x7F) {
					if (ch == 0x5C)
						str = "$\\\\";
					else
						str = ch;
				} else if (ch == 0x09) {
					str = "$\\t";
				} else if (ch == 0x0D) {
					str = "$\\n";
				} else {
					str = string("$\\") + HexStr(ch, 2);
				}
				literalStrs.push_back(str);
			} else {
				if (IsFrame(slot)) {	//(ISPTR(slot) && FLAGTEST(ObjectFlags(slot), kObjSlotted)) {
					// create an external object
					static ArrayIndex index = 1;
					string objName = "kObject" + to_string(index);
					++index;
					string str = "DefConst('" + objName + ", " + DumpObject(slot) + ");\n";
					literalStrs.push_back(objName);
					REPprintf(str.c_str());
				} else {
					string litrl = DumpObject(slot);
					if (litrl[0] == '\'') {
						litrl.erase(0,1);
					}
					literalStrs.push_back(litrl);
				}
			}
		END_FOREACH;
	}


	// build code fragment array
	vector<Fragment> frags = MakeFragments(inFunc, varNames, literalStrs);


	if (FLAGTEST(gDebugBits, kDumpFragments)) {
		REPprintf("-- code fragments --\n");
		for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
			frag->dump();
		}
	}

/*-- detect code constructs in descending order of complexity and modify the code fragment array --*/

/*-- try-on-exception construct --
	Code:				push exception-symbol
						push <handler>
						new-handlers
						[code that might throw an exception]
						pop-handlers			<--
						branch <loop-end>
					handler:
						[exception handler]
						pop-handlers
					loop-end:
	Condition:	pop-handlers
	Action:		 */

	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		if (frag->opcode == kFragmentPopHandlers) {
			if (FLAGTEST(gDebugBits, kDumpHandlers)) {
				REPprintf("-- try-on-exception construct --\n");
				frag->dump(frags);
			}
			// remove branch following
			frags.erase(frag+1);
			// find end of handler block
			vector<Fragment>::iterator nextFrag;
			for (nextFrag = frag+1; nextFrag != frags.end(); ++nextFrag) {
				if (nextFrag->opcode == kFragmentPopHandlers) {
					break;
				}
			}
			if (nextFrag == frag+2) {
				// one line only
				frag->text += "\n" + (frag+1)->text;
				frag->opcode = kFragmentHandler;
				// remove the line
				frags.erase(frag+1);
			} else {
				// more than one line => wrap handler block in begin-end
				frag->text += "\nbegin\n" + (frag+1)->text;
				frag->opcode = kFragmentHandler;
				nextFrag->text = "end";
				nextFrag->opcode = kFragmentHandler;
			}
			// remove next-handler delimiter
			frags.erase(frag+1);
			// now wrap try block in begin-end
			for (nextFrag = frag-1; nextFrag != frags.begin(); --nextFrag) {
				if (nextFrag->text == "try") {
					break;
				}
			}
			if (nextFrag+2 == frag) {
				// one line only
				nextFrag->opcode = kFragmentSetVar;
				frag = nextFrag+1;
			} else {
				// more than one line => wrap try block in begin-end
				nextFrag->text += "\nbegin";
				frag->opcode = kFragmentHandler;
				nextFrag->opcode = kFragmentSetVar;
				frag->text = "end\n" + frag->text;
				--frag;
			}
		}
	}

/*-- loop construct --
	Code:			loop:
						[code that must exit with a break]
						branch <loop-end>
						[code]
						branch <loop>		<--
					loop-end:
	Condition:	branch before current location
	Action:		line text -> lineText + "end"
					symbol -> loop
					branch address -> 0
					line-at-branch text -> "loop\nbegin\n" + lineText */

	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		if (frag->opcode == kFragmentBranch && frag->addr > frag->branch) {
			if (FLAGTEST(gDebugBits, kDumpLoop)) {
				REPprintf("-- loop construct --\n");
				frag->dump(frags);
			}
			frag->text = "end";
			frag->opcode = kFragmentLoop;
			for (vector<Fragment>::iterator it = frag; it >= frags.begin(); --it) {
				if (it->addr == frag->branch) {
					++(it->addr);
					frags.insert(it, Fragment(frag->branch, "loop\nbegin", kFragmentLoop));
					break;
				}
			}
		}
	}

/*-- foreach-collect construct --
	Code:				push object to iterate over
						new-iterator
						set-var xx|iter
						push 'array
						make-array 0xFFFF to collect result
						set-var xx|result
						push-constant initial-index
						set-var xx|index
						branch <loop-test>
					loop:
						[code]
						get-var xx|iter
						iter-next
					loop-test:
						get-var xx|iter
						iter-done
						branch-f <loop>		<--
						branch <loop-end>
						set-var xx|result
						pop
						pop
					loop-end:
						get-var xx|result
	Condition:	branch-if-false before current location, frag-1 is iter-done, frag+1 is branch
	Action:		 */
	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		if (frag->opcode == kFragmentBranchIfFalse && frag->addr > frag->branch && (frag-1)->opcode == kFragmentIterDone && (frag+1)->opcode == kFragmentBranch) {
			if (FLAGTEST(gDebugBits, kDumpForeachCollect)) {
				REPprintf("-- foreach-collect construct --\n");
				frag->dump(frags);
			}
			// remove this branch
			frags.erase(frag);
			// look back for NewIterator
			for (vector<Fragment>::iterator it = frag; it >= frags.begin(); --it) {
				if (it->text.find("NewIterator(") != string::npos) {
					string result;
					bool isDeeply;
					string v;
					string s = " in " + GetIterator(it->text, isDeeply) + " collect ";
					if (isDeeply) {
						s = " deeply" + s;
					}
					ArrayIndex startAddr = it->addr;
					while (it->opcode != kFragmentBranch) {
						if (it->text.find("|result|") != string::npos) {
							result = GetFirstWord(it->text);
						}
						// remove auto vars
						Remove(varNames, GetFirstWord(it->text));
						frags.erase(it); --frag;
					}
					// remove the branch
					frags.erase(it);
					v = GetFirstWord(it->text);
					s = v + s;
					Remove(varNames, v);
					// remove result expr
					frags.erase(it);
					frag -= 2;
					if (it->text.find("|iter|") != string::npos) {
						v = GetFirstWord(it->text);
						s = v + ", " + s;
						Remove(varNames, v);
						frags.erase(it); --frag;
					}
					string::size_type i;
					if ((i = it->text.find(":= ")) != string::npos) {
						s += it->text.substr(i+3);
						frags.erase(it); --frag;
					}
					// remove fragments up to next branch
					while (it->opcode != kFragmentBranch) {
						frags.erase(it); --frag;
					}
					// remove fragments up to branch destination
					ArrayIndex dest = it->branch;
					while (it->addr != dest) {
						frags.erase(it);
					}
					// if followed by iterator clean-up then remove that
					if (it->text.find("|result| := nil") != string::npos) {
						frags.erase(it);
						frags.erase(it);
					}
					// replace |x|result| with the foreach-collect expression
					if ((i = it->text.find(result)) != string::npos) {
						it->text.replace(i, result.length(), "foreach " + s);
					}
					it->addr = startAddr;
					break;
				}
			}
		}
	}

/*-- foreach construct --
	Code:				push object to iterate over
						new-iterator
						set-var xx|iter
						branch <loop-test>
					loop:
						[code]
						get-var xx|iter
						iter-next
					loop-test:
						get-var xx|iter
						iter-done
						branch-f <loop>		<--
	Condition:	branch-if-false before current location, frag-1 is iter-done
	Action:		 */
	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		if (frag->opcode == kFragmentBranchIfFalse && frag->addr > frag->branch && frag != frags.begin() && (frag-1)->opcode == kFragmentIterDone) {
			if (FLAGTEST(gDebugBits, kDumpForeach)) {
				REPprintf("-- foreach construct --\n");
				frag->dump(frags);
			}
			// remember where we’re going then remove this branch
			ArrayIndex dest = frag->branch;
			frags.erase(frag);
			// if followed by iterator clean-up then remove that
			if (frag->text.find("|iter| := nil") != string::npos) {
				frags.erase(frag);
			}
			// if followed by branch then remove that too
			if (frag->text == "nil" && frag->opcode == kFragmentBranch) {
				frags.erase(frag);
			}
			// look back for start of loop
			for (vector<Fragment>::iterator it = frag; it >= frags.begin(); --it) {
				if (it->addr == dest) {
					// look back for NewIterator
					for ( ; it >= frags.begin(); --it) {
						if (it->text.find("NewIterator(") != string::npos) {
							bool isDeeply;
							string s = " in " + GetIterator(it->text, isDeeply) + " do\nbegin\n";
							if (isDeeply) {
								s = " deeply" + s;
							}
							ArrayIndex startAddr = it->addr;
							// remove auto vars from argNames -- look for branch at start of loop
							while (it->opcode != kFragmentBranch) {
								Remove(varNames, GetFirstWord(it->text));
								frags.erase(it); --frag;
							}
							// remember where we’re going then remove this branch
							dest = it->branch;
							frags.erase(it); --frag;
							// add auto vars to fragment text
							s = GetFirstWord(it->text) + s;
							Remove(varNames, GetFirstWord(it->text));
							if ((it+1)->text.find("|iter") != string::npos) {
								s = GetFirstWord((it+1)->text) + ", " + s;
								Remove(varNames, GetFirstWord((it+1)->text));
								frags.erase(it+1); --frag;
							}
							it->text = "foreach " + s;
							it->addr = startAddr;
							for ( ; it != frags.end(); ++it) {
								if (it->addr == dest) {
									frags.erase(it); --frag;
									--it;
									it->text = "end";
									it->opcode = kFragmentForeachEnd;
									break;
								}
							}
							break;
						}
					}
					break;
				}
			}
		}
	}

/*-- repeat construct --
	Code:			loop:
						[code]
						branch-f <loop>		<--
	Condition:	branch-if-false before current location
	Action:		 */
	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		if (frag->opcode == kFragmentBranchIfFalse && frag->addr > frag->branch) {
			if (FLAGTEST(gDebugBits, kDumpRepeat)) {
				REPprintf("-- repeat construct --\n");
				frag->dump(frags);
			}
			frag->text = "until " + frag->text;
			frag->opcode = kFragmentLoop;
			// find start of loop
			for (vector<Fragment>::iterator it = frag; it >= frags.begin(); --it) {
				if (it->addr == frag->branch) {
					if (it->text.empty()) {
						it->text = "repeat";
					} else {
						it->text = "repeat\n" + it->text;
					}
					break;
				}
			}
		}
	}

/*-- while construct --
	Code:				branch <loop-test>
					loop:
						[code]
					loop-test:
						[test]
						branch-t <loop>		<--
	Condition:	branch-if-true before current location
	Action:		 */
	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		if (frag->opcode == kFragmentBranchIfTrue && frag->addr > frag->branch) {
			if (FLAGTEST(gDebugBits, kDumpWhile)) {
				REPprintf("-- while construct --\n");
				frag->dump(frags);
			}
			string loopStr = "while " + frag->text + " do\nbegin\n";
			frag->text = "end";
			frag->opcode = kFragmentLoop;
			// find start of loop
			for (vector<Fragment>::iterator it = frag; it >= frags.begin(); --it) {
				if (it->addr == frag->branch) {
					it->text = loopStr + it->text;
					vector<Fragment>::iterator prevFrag = it-1;
					// if prev frag branches to this frag, remove it
					if (prevFrag->opcode == kFragmentBranch && prevFrag->branch == it->addr) {
						it->addr = prevFrag->addr;
						frags.erase(prevFrag);
					}
					break;
				}
			}
		}
	}

/*-- for construct --
	Code:				push-constant
						set-var xx
						branch <loop-test>
						push-constant
						set-var xx|limit
						push-constant
						set-var xx|incr
						get-var xx|incr
						get-var xx
						branch loop-test
					loop:
						[code]
						get-var xx|incr
						incr-var xx
					loop-test:
						get-var xx|limit
						branch-if-loop-not-done <loop>		<--
	Condition:	for-loop
	Action:		 */
	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		if (frag->opcode == kFragmentForLoop) {
			if (FLAGTEST(gDebugBits, kDumpFor)) {
				REPprintf("-- for construct --\n");
				frag->dump(frags);
			}
			frag->text = "end";
			frag->opcode = kFragmentLoop;
			// find start of loop
			for (vector<Fragment>::iterator it = frag; it >= frags.begin(); --it) {
				if (it->addr == frag->branch) {
					string loopVar = (it-4)->text;
					string loopLimitVar = GetFirstWord((it-3)->text);
					string loopLimit = GetLastWord((it-3)->text);
					string loopIncrVar = GetFirstWord((it-2)->text);
					string loopIncr = GetLastWord((it-2)->text);
					string loopStr = " do\nbegin\n";
					if (loopIncr != "1") {
						loopStr = " by " + loopIncr + loopStr;	// incr
					}
					loopStr = "for " + loopVar + " to " + loopLimit + loopStr;
					// remove start-of-loop code
					it -= 4;
					frags.erase(it+1, it+4); frag -= 3;
					it->text = loopStr;
					it->opcode = kFragmentFor;
					if ((frag-1)->opcode == kFragmentIncrVar) {
						frags.erase(frag-1);
					}
					if ((frag-2)->text == loopIncrVar) {	// when would this happen?
						frags.erase(frag-2);
					}
					Remove(varNames, loopIncrVar);
					Remove(varNames, loopLimitVar);
					break;
				}
			}
		}
	}

/*-- break construct --
	Code:			loop:
						[test]
						branch-f <continue>
						push-constant result
						branch <loop-end>
					continue:
						[code]
						branch <loop>			<--
					loop-end:
	Condition:	branch, fragment preceding destination of branch is loop-start
	Action:		 */
	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		if (frag->opcode == kFragmentBranch) {
			if (FLAGTEST(gDebugBits, kDumpBreak)) {
				REPprintf("-- break construct --\n");
				frag->dump(frags);
			}
			ArrayIndex endAddr = frag->branch;
			for (vector<Fragment>::iterator it = frag; it != frags.end(); ++it) {
				if ((it->addr == endAddr || it->addr == endAddr+1) && (it-1)->opcode == kFragmentLoop) {
					string breakStr = "break";
					if (!frag->text.empty() && frag->text != "nil") {
						breakStr += " " + frag->text;
					}
					frag->text = breakStr;
					frag->opcode = kFragmentSetVar;
					if ((frag+1)->opcode == kFragmentBranch) {
						for (it = frag; it != frags.end(); ++it) {
							if (it->addr == (frag+1)->branch) {
								break;
							}
						}
						if (it == frags.end()) {
							// couldn’t find destination of branch so remove it
							frags.erase(frag+1);
						}
					}
					break;
				}
			}
		}
	}

/*-- if construct --
	Code:				[test]
						branch-f <if-fail>		<--
						[test]
						branch test-2
					if-fail:
						push-constant nil
					test-2:
						branch-f <if-else>		<--
						[then-code]
						branch-pop
						branch <if-end>			<--
					if-else:
						[else-code]					is nil if no else clause
					if-end:
	Condition:
	Action:		 */
	RefVar instructions(GetArraySlot(inFunc, kFunctionInstructionsIndex));
	ArrayIndex exitAddr = Length(instructions) - 1;
	ArrayIndex ifAddr, elseAddr, endAddr;
	// scan backwards because we need to know if-end address
	for (vector<Fragment>::iterator frag = frags.begin() + frags.size()-1; frag >= frags.begin(); --frag) {
		if (frag > frags.begin() && (frag-1)->opcode == kFragmentBranch) {
			endAddr = (frag-1)->branch;
			if (frag->text == "nil") {
				endAddr -= 1;
			} else {
				endAddr -= 3;
			}
		} else {
			endAddr = kIndexNotFound;
		}
		if ((frag->opcode == kFragmentBranchIfFalse || frag->opcode == kFragmentBranchIfTrue) && frag->addr != endAddr) {
			if (FLAGTEST(gDebugBits, kDumpIf)) {
				REPprintf("-- if construct --\n");
				frag->dump(frags);
			}
			ifAddr = frag->addr;
			string str;
			if ((frag+1)->opcode == kFragmentBranch && (frag+1)->branch != exitAddr
			&& ((frag+2)->opcode == kFragmentBranchIfTrue || (frag+2)->opcode == kFragmentBranchIfFalse)
			&& ((frag+1)->branch == (frag+2)->addr+1 || (frag+1)->branch == (frag+2)->addr+3)) {
				str = "if " + frag->text + (frag->opcode == kFragmentBranchIfFalse? " and " : " or ");
				frags.erase(frag);
				while ((frag+1)->opcode == kFragmentBranchIfFalse || (frag+1)->opcode == kFragmentBranchIfTrue) {
					if ((frag+3)->opcode == kFragmentBranch && (frag+3)->branch != endAddr
					&& ((frag+3)->branch == (frag+4)->addr+1 || (frag+3)->branch == (frag+4)->addr+3)) {
						str += frag->text + ((frag+1)->opcode == kFragmentBranchIfFalse? " and " : " or ");
						frags.erase(frag, frag+2);
					} else {
						str += frag->text + " then\n";
						frags.erase(frag, frag+1);
					}
				}
			} else {
				str = "if " + frag->text + " then\n";
			}
			elseAddr = frag->branch;
			vector<Fragment>::iterator elseFrag;
			for (elseFrag = frag; elseFrag != frags.end(); ++elseFrag) {
				if (elseFrag->addr == elseAddr) {
					break;
				}
			}
			if (elseFrag == frags.end() && elseAddr == exitAddr) {
				// no else at end of code
				elseFrag = frags.begin() + frags.size()-1;
			}
			if (FLAGTEST(gDebugBits, kDumpElse)) {
				REPprintf("-- else construct --\n");
				REPprintf("“%s”  elseAddr=%d  exitAddr=%d\n", str.c_str(), elseAddr, exitAddr);
				elseFrag->dump(frags);
			}
			if (elseFrag != frags.end()) {
				if ((elseFrag-1)->opcode == kFragmentBranch) {
					endAddr = (elseFrag-1)->branch;
					vector<Fragment>::iterator endFrag;
					for (endFrag = elseFrag; endFrag != frags.end(); ++endFrag) {
						if (endFrag->addr == endAddr) {
							break;
						}
					}
//					if (endFrag == frags.end() && endAddr == exitAddr) {
//						// at end of code
//						endFrag = frags.begin() + frags.size()-1;
//					}
					if (endFrag != frags.end()) {
						if (elseFrag == (endFrag-1)) {
							elseFrag->text = "else\n" + elseFrag->text;
//							frags.erase(elseFrag-1);
						} else if (elseFrag == (endFrag-2)) {
							elseFrag->text = "else\n" + elseFrag->text;
//							elseFrag->opcode = elseFrag->opcode;
							frags.erase(endFrag-1);
						} else {
							elseFrag->text = "else\nbegin\n" + elseFrag->text;
							if (endAddr != exitAddr) {
								if ((endFrag-1)->opcode == kFragmentBranch) {
									(endFrag-1)->text = "end";
								} else {
									(endFrag-1)->text = (endFrag-1)->text + "\nend";
								}
							} else {
								if (endFrag->opcode == kFragmentBranch) {
									endFrag->text = "end";
								} else {
									endFrag->text = endFrag->text + "\nend";
								}
							}
							frags.erase(elseFrag-1);
						}
					}

					if (elseFrag == frag+2) {
						str += (elseFrag-1)->text + "\n";
						frags.erase(elseFrag-1);
					} else {
						if ((elseFrag-1)->opcode == kFragmentBranch) {
							(elseFrag-1)->text = "end";
						} else {
							(elseFrag-1)->text = (elseFrag-1)->text + "\nend";
						}
						str += "begin\n";
					}
//				} else if ((elseFrag+1)->opcode == kFragmentIf) {
//					(elseFrag+1)->text = " else " + (elseFrag+1)->text;
//					(elseFrag+1)->opcode = kFragmentElseIf;
				} else if (elseFrag == frag+1) {
					if (elseFrag->opcode == kFragmentBranch) {
						elseFrag->text = "end";
					} else {
						elseFrag->text += "\nend";
					}
					str += "begin\n";
				}
			}
			frags.erase(frag);
			frag->addr = ifAddr;
			frag->text = str + frag->text;
			frag->opcode = kFragmentIf;
		}
	}

	/*-- clean up --*/
	// remove auto vars that the user did not also declare
	for (vector<Fragment>::iterator frag = frags.begin() + frags.size()-2; frag >= frags.begin(); --frag) {
		if (frag->opcode == kFragmentPop && (frag+1)->opcode == kFragmentLoop) {
			if (frag->text.find(" ") == string::npos && Contains(varNames, frag->text)) {
				frags.erase(frag);
			}
		} else if (frag->opcode == kFragmentSetPathAndPush && (frag+1)->opcode == kFragmentReturn) {
// if instruction is set-path 1 (leaving value on stack) followed by return then remove it (otherwise there’s a duplicate line) 
			frags.erase(frag);
		}
	}
	// remove nil fragments
	for (vector<Fragment>::iterator frag = frags.begin() + frags.size()-1; frag >= frags.begin(); --frag) {
		if (frag->text.empty() || frag->text == "nil") {
			frags.erase(frag);
		}
	}

/*-- build final function text from fragments --*/
	if (FLAGTEST(gDebugBits, kDumpFinal)) {
		REPprintf("-- final --\n");
		for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
			frag->dump();
		}
	}
	ostringstream fnstr;
	ArrayIndex n;
	// function and arguments
	fnstr << "func(";
	if (argCount > 0) {
		// there are args
		n = 0;
		for (vector<string>::iterator var = varNames.begin()+3; var != varNames.begin()+3+argCount; ++var) {
			if (n > 0) {
				fnstr << ", ";
			}
			++n;
			fnstr << *var;
		}
	}
	// function body
	fnstr << ")\nbegin";
	// declare locals
	n = 0;
	for (vector<string>::iterator var = varNames.begin()+3+argCount; var != varNames.end(); ++var) {
		if (!var->empty()) {
			if (n == 0) {
				fnstr << "\nlocal ";
			} else {
				fnstr << ", ";
			}
			++n;
			fnstr << *var;
		}
	}
	if (n > 0) {
		fnstr << ";";
	}
	// append code fragments
	for (vector<Fragment>::iterator frag = frags.begin(); frag != frags.end(); ++frag) {
		TrimString(frag->text);
		if (frag->text[0] == ';') {
			frag->text.erase(frag->text.begin());
		}
		if (frag->text.size() > 0) {
			if (!EndsWith(frag->text, "begin")) {
				frag->text += ";";
			}
			fnstr << "\n" << frag->text;
		}
	}
	// all done
	fnstr << "\nend";

	string funcStr = fnstr.str();
	// remove nil-only lines
	StrReplace(funcStr, "begin\nnil;\n", "begin\n");

	string::size_type i, j;
	// convert text to array of lines
	vector<string> lines;
	for (i = 0, j = funcStr.find("\n", i); j != string::npos; i = j+1, j = funcStr.find("\n", i)) {
		if (i == j) {
			lines.push_back("");
		} else {
			lines.push_back(funcStr.substr(i,j-i));
		}
	}
	if (i < funcStr.size()) {
		lines.push_back(funcStr.substr(i));
	}

	// remove useless begin-end pairs
	for (vector<string>::iterator line = lines.begin(); line < lines.end()-3; ++line) {
		if (*line == "begin" && BeginsWith(*(line+2), "end")) {
			lines.erase(line+2);
			lines.erase(line);
		}
	}

	// remove useless else-nil pairs
	for (vector<string>::iterator line = lines.begin(); line < lines.end()-2; ++line) {
		if (*line == "else" && BeginsWith(*(line+1), "nil")) {
			lines.erase(line,line+2);
		}
	}

	// convert array back to string
	string codeStr;
	int indent = -1, singleLineIndent = 0;
	const char * tabs = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";
	const char * spaces = "                                                ";
	for (vector<string>::iterator line = lines.begin(); line != lines.end(); ++line) {
		if (*line == "begin") {
			++indent;
		}
		if (indent > 0) {
#if useTabs
			codeStr += tabs + (16-indent);
#else
			codeStr += spaces + (16-indent)*3;
#endif
		}
		codeStr += *line + '\n';
		indent -= singleLineIndent, singleLineIndent = 0;
		if (*line == "repeat") {
			++indent;
		} else if ((EndsWith(*line, "do") || EndsWith(*line, "then") || EndsWith(*line, "else")) && *(line+1) != "begin") {
			if (BeginsWith(*(line+1), "if")) {
				codeStr.back() = ' ';
				singleLineIndent = -indent;
				indent = 0;
			} else {
				++singleLineIndent;
				++indent;
			}
		} else if (BeginsWith(*line, "end") || BeginsWith(*(line+1), "until")) {
			indent = MAX(0, indent-1);
		}
	}

	return codeStr;
}


void
PrintCode(RefArg obj)
{
	string code = DumpCode(obj);
	code.pop_back();	// remove trailing newline
	REPprintf("%s", code.c_str());
}


#pragma mark -
/*------------------------------------------------------------------------------
	P S t r i n g O u t T r a n s l a t o r
	Output to string.
------------------------------------------------------------------------------*/
PStringOutTranslator * CreateStringOutTranslator(void);

string
DumpObject(RefArg obj)
{
	// set up output translator so we can send REPprintf() to string
	PStringOutTranslator * stringREPout = CreateStringOutTranslator();
	POutTranslator * savedREPOut = gREPout;
	gREPout = (POutTranslator *)stringREPout;
	PrintObject(obj, 0);
	string str = stringREPout->string();
	stringREPout->destroy();
	gREPout = savedREPOut;
	return str;
}


const CClassInfo *
PStringOutTranslator::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN20PStringOutTranslator6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN20PStringOutTranslator4makeEv - 0b	\n"
"		.long		__ZN20PStringOutTranslator7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"PStringOutTranslator\"	\n"
"2:	.asciz	\"POutTranslator\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN20PStringOutTranslator9classInfoEv - 4b	\n"
"		.long		__ZN20PStringOutTranslator4makeEv - 4b	\n"
"		.long		__ZN20PStringOutTranslator7destroyEv - 4b	\n"
"		.long		__ZN20PStringOutTranslator4initEPv - 4b	\n"
"		.long		__ZN20PStringOutTranslator4idleEv - 4b	\n"
"		.long		__ZN20PStringOutTranslator12consumeFrameERK6RefVarii - 4b	\n"
"		.long		__ZN20PStringOutTranslator6promptEi - 4b	\n"
"		.long		__ZN20PStringOutTranslator5printEPKcz - 4b	\n"
"		.long		__ZN20PStringOutTranslator6vprintEPKcP13__va_list_tag - 4b	\n"
"		.long		__ZN20PStringOutTranslator4putcEi - 4b	\n"
"		.long		__ZN20PStringOutTranslator5flushEv - 4b	\n"
"		.long		__ZN20PStringOutTranslator14enterBreakLoopEi - 4b	\n"
"		.long		__ZN20PStringOutTranslator13exitBreakLoopEv - 4b	\n"
"		.long		__ZN20PStringOutTranslator10stackTraceEPv - 4b	\n"
"		.long		__ZN20PStringOutTranslator15exceptionNotifyEP9Exception - 4b	\n"
"		.long		__ZN20PStringOutTranslator6stringEv - 4b	\n"
CLASSINFO_END
);
}

PStringOutTranslator *
CreateStringOutTranslator(void)
{
	NewtonErr err = noErr;
	PStringOutTranslator * translator;
	XTRY
	{
		translator = (PStringOutTranslator *)MakeByName("POutTranslator", "PStringOutTranslator");
		XFAILIF(translator == NULL, err = MemError();)
		XFAILIF(err = translator->init(NULL), (translator)->destroy(); translator = NULL;)
	}
	XENDTRY;
	return translator;
}

PROTOCOL_IMPL_SOURCE_MACRO(PStringOutTranslator)

PStringOutTranslator *
PStringOutTranslator::make(void)
{
	fString = NULL;
	return this;
}

void
PStringOutTranslator::destroy(void)
{
	if (fString) {
		free(fString), fString = NULL;
	}
}

NewtonErr
PStringOutTranslator::init(void * inArgs)
{
	fAllocated = 4*KByte;
	fString = (char *)malloc(fAllocated);
	fString[0] = 0;
	fLength = 0;
	return noErr;
}

Timeout
PStringOutTranslator::idle(void)
{ return kNoTimeout; }

void
PStringOutTranslator::consumeFrame(RefArg inObj, int inDepth, int indent)
{
	PrintObjectAux(inObj, indent, inDepth);
}

void
PStringOutTranslator::prompt(int inLevel)
{ }

int
PStringOutTranslator::vprint(const char * inFormat, va_list args)
{
	int len = vsprintf(fString+fLength, inFormat, args);
	fLength += len;
	realloc();
	return len;
}

int
PStringOutTranslator::print(const char * inFormat, ...)
{
	int result = 0;
	va_list	args;
	va_start(args, inFormat);
	result = vprint(inFormat, args);
	va_end(args);
	return result;
}

int
PStringOutTranslator::putc(int inCh)
{
	fString[fLength++] = (char)inCh;
	fString[fLength] = 0;
	realloc();
	return inCh;
}

void
PStringOutTranslator::flush(void)
{ }

void
PStringOutTranslator::enterBreakLoop(int inLevel)
{ }

void
PStringOutTranslator::exitBreakLoop(void)
{ }


void
PStringOutTranslator::stackTrace(void * interpreter)
{ }

void
PStringOutTranslator::exceptionNotify(Exception * inException)
{ }

void
PStringOutTranslator::realloc(void)
{
	if (fLength > fAllocated*3/4) {
		fAllocated *= 2;
		fString = (char *)::realloc(fString, fAllocated);
	}
}

const char *
PStringOutTranslator::string(void)
{ return fString; }
