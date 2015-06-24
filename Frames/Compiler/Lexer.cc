/*
	File:		Compiler-Lexer.cc

	Contains:	The NewtonScript compiler.
					Lexical processing (token generation) based on ideas from LLVM
					and not using yacc/flex tools as the original did.

	Written by:	The Newton Tools group.
*/

#include "Objects.h"
#include "Compiler.h"
#include "Tokens.h"
#include "Unicode.h"
#include "UStringUtils.h"
#include "ROMResources.h"

// integer Ref limits
#if __LP64__
#define kMaxUnsignedIntRef 0x4000000000000000
#define kMaxSignedIntRef 0x2000000000000000
#define kMinSignedIntRef -0x2000000000000000
#else
#define kMaxUnsignedIntRef 0x40000000
#define kMaxSignedIntRef 0x20000000
#define kMinSignedIntRef -0x20000000
#endif

#pragma mark Reserved words
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
	{ "and",				TOKENand },
	{ "begin",			TOKENbegin },
	{ "break",			TOKENbreak },
	{ "by",				TOKENby },
	{ "call",			TOKENcall },
	{ "constant",		TOKENconstant },
	{ "deeply",			TOKENdeeply },
	{ "div",				TOKENdiv },
	{ "do",				TOKENdo },
	{ "else",			TOKENelse },
	{ "end",				TOKENend },
	{ "exists",			TOKENexists },
	{ "for",				TOKENfor },
	{ "foreach",		TOKENforeach },
	{ "func",			TOKENfunc },
	{ "global",			TOKENglobal },
	{ "if",				TOKENif },
	{ "in",				TOKENin },
	{ "inherited",		TOKENinherited },
	{ "local",			TOKENlocal },
	{ "loop",			TOKENloop },
	{ "mod",				TOKENmod },
	{ "native",			TOKENnative },
	{ "not",				TOKENnot },
	{ "onexception",	TOKENonexception },
	{ "or",				TOKENor },
	{ "repeat",			TOKENrepeat },
	{ "return",			TOKENreturn },
	{ "self",			TOKENself },
	{ "then",			TOKENthen },
	{ "to",				TOKENto },
	{ "try",				TOKENtry },
	{ "until",			TOKENuntil },
	{ "while",			TOKENwhile },
	{ "with", 			TOKENwith }
};
static const ReservedWord gOtherTokens[] =
{
	{ "const",			TOKENconst },
	{ "symbol",			TOKENsymbol },
	{ "integer",		TOKENinteger },
	{ "real",			TOKENreal },
	{ "ref",				TOKENref },
	{ "ref-const",		TOKENrefConst },
//assignment
	{ ":=",				TOKENassign },
// string operator
	{ "&&",				TOKENAmperAmper },
// arithmetic operators
	{ "<<",				TOKENLShift },
	{ ">>",				TOKENRShift },
// relational operators },
	{ "=",				TOKENEQL },
	{ "<>",				TOKENNEQ },
	{ "<=",				TOKENLEQ },
	{ ">=",				TOKENGEQ },
// unary operators },
	{ "-",				TOKENuMinus },
// message send
	{ ":?",				TOKENsendIfDefined },
// not really lexical as such
	{ "global-function",	TOKENgFunction  },
	{ "optional-expr",	TOKENOptionalExpr },
	{ "array",			TOKENBuildArray },
	{ "frame",			TOKENBuildFrame },
	{ "...",				TOKENKeepGoing }
};


/*----------------------------------------------------------------------
	Compare a symbol against a reserved word.
	This is very like symcmp() except we can omit one tolower() call:
	the symbol can be mixed case but the reserved word MUST be lowercase.
	We can assume this is the case when testing words in gReservedWords[].
	Args:		s1			a symbol name
				s2			reserved word in lowercase
	Return:	int		0		if symbol names are equal
							>0		if sym1 is greater than sym2
							<0		if sym1 is less than sym2
----------------------------------------------------------------------*/
#include <ctype.h>

static int
wordcmp(const char * s1, const char * s2)
{
	char c1, c2;
	for ( ; ; )
	{
		c1 = *s1++;
		c2 = *s2++;
		if (c1 == '\0')
			return (c2 == '\0') ? 0 : -1;
		if (c2 == '\0')
			break;
		if ((c1 = tolower(c1)) != c2)
			return (c1 > c2) ? 1 : -1;
	}
	return 1;
}

/*------------------------------------------------------------------------------
	Reserved word lookup, using binary search.
	Args:		inWord		word scanned from input stream
								ASCII because it’s a symbol.
	Return:	token number
				TOKENbad => word is not reserved
------------------------------------------------------------------------------*/

static int
ReservedWordToken(const char * inWord)
{
	int	first = 0;
	int	last = sizeof(gReservedWords) / sizeof(ReservedWord) - 1;
	while (first <= last)
	{
		int	i = (first + last) / 2;
		int	cmp = wordcmp(inWord, gReservedWords[i].word);
		if (cmp == 0)
			return gReservedWords[i].token;
		else if (cmp < 0)
			last = i - 1;
		else
			first = i + 1;
	}
	return TOKENbad;
}

/*------------------------------------------------------------------------------
	FOR DEBUG
	Pretty-print a token.
	Args:		inToken		token id
	Return:	--
------------------------------------------------------------------------------*/

void
CCompiler::PrintToken(int inToken)
{
	if (inToken < 128)
	{
		if (inToken == EOF)
			printf("<EOF>\n");
		else
			printf("%c%c", inToken,  inToken == ';' ? '\n' : ' ');
	}
	else
	{
		for (ArrayIndex i = 0; i < sizeof(gReservedWords)/sizeof(ReservedWord); ++i)
		{
			if (gReservedWords[i].token == inToken)
			{
				printf("%s ", gReservedWords[i].word); return;
			}
		}
		for (ArrayIndex i = 0; i < sizeof(gOtherTokens)/sizeof(ReservedWord); ++i)
		{
			if (gOtherTokens[i].token == inToken)
			{
				if (theToken.value.type == TYPEnone)
					printf("%s ", gOtherTokens[i].word);
				else
				{
					printf("<%s:", gOtherTokens[i].word);
					PrintObject(theToken.value.ref, 0);
					printf("> ");
				}
				return;
			}
		}
	}
}


#pragma mark Token construction
/*------------------------------------------------------------------------------
	Token construction.
	The output of the lexer is a stream of tokens. Some tokens represent a value.
	This abstraction should allow easier transition to LLVM.
------------------------------------------------------------------------------*/
#define forDebug 0
int
CCompiler::makeToken(int inToken)
{
	theToken.value.type = TYPEnone;
	theToken.value.ref = NILREF;
#if forDebug
	PrintToken(inToken);
#endif
	return theToken.id = inToken;
}

int
CCompiler::makeBooleanToken(bool inValue)
{
	theToken.value.type = TYPEboolean;
	theToken.value.ref = inValue ? TRUEREF : NILREF;
#if forDebug
	PrintToken(TOKENconst);
#endif
	return theToken.id = TOKENconst;
}

int
CCompiler::makeCharacterToken(UniChar inValue)
{
	theToken.value.type = TYPEcharacter;
	theToken.value.ref = MAKECHAR(inValue);
#if forDebug
	PrintToken(TOKENconst);
#endif
	return theToken.id = TOKENconst;
}

int
CCompiler::makeStringToken(const UniChar * inValue, ArrayIndex inLength)
{
	theToken.value.type = TYPEstring;
	theToken.value.ref = AllocateBinary(SYMA(string), inLength);
	memmove(BinaryData(theToken.value.ref), inValue, inLength);
#if forDebug
	PrintToken(TOKENconst);
#endif
	return theToken.id = TOKENconst;
}

int
CCompiler::makeRefToken(long inValue)
{
	theToken.value.type = TYPEref;
	theToken.value.ref = MAKEMAGICPTR(inValue);
#if forDebug
	PrintToken(TOKENrefConst);
#endif
	return theToken.id = TOKENrefConst;
}

int
CCompiler::makeIntegerToken(long inValue)
{
	theToken.value.type = TYPEinteger;
	theToken.value.ref = MAKEINT(inValue);
#if forDebug
	PrintToken(TOKENinteger);
#endif
	return theToken.id = TOKENinteger;
}

int
CCompiler::makeRealToken(double inValue)
{
	theToken.value.type = TYPEreal;
	theToken.value.ref = MakeReal(inValue);
#if forDebug
	PrintToken(TOKENreal);
#endif
	return theToken.id = TOKENreal;
}

int
CCompiler::makeSymbolToken(const char * inValue)
{
	theToken.value.type = TYPEsymbol;
	theToken.value.ref = MakeSymbol(inValue);
#if forDebug
	PrintToken(TOKENsymbol);
#endif
	return theToken.id = TOKENsymbol;
}


/*------------------------------------------------------------------------------

Lexical grammar, from http://manuals.info.apple.com/en_US/NewtonScriptProgramLanguage.PDF

	string:
		" character-sequence "
	character-sequence:
		[ { string-character | escape-sequence } ]* [ truncated-escape ]
	string-character:
		<tab or any ASCII character with code 32–127 except ‘"’ or ‘\’>
	escape-sequence:
		{ \ {"|\|n|t} | \ u [ hex-digit hex-digit hex-digit hex-digit ]* \ u }
	￼truncated-escape:
		\ u [ hex-digit hex-digit hex-digit hex-digit ]*
	symbol:
		{ { alpha | _ } [ { alpha | digit | _ } ]* |
		  ‘|’ [ { symbol-character | \ { ‘|’ | \ } ]* ‘|’ }
	symbol-character:
		<any ASCII character with code 32–127 except '|' or '\'>
	integer:
		[-] { [ digit ]+ | 0x [ hex-digit ]+ }
	real:
		[-] [ digit ]+ . [ digit ]* [ {e|E} [-] [ digit ]+ ]
	character:
		$ { non-escape-character | \ { \ | n | t | hex-digit hex-digit | u hex-digit hex-digit hex-digit hex-digit } }
	non-escape-character:
		<any ASCII character with code 32–127 except '\'>
	alpha:
		<A–Z and a–z>
	digit:
		{0|1|2|3|4|5|6|7|8|9}
	hex-digit:
		{ digit |a|b|c|d|e|f |A|B|C|D|E|F }
	reserved-word:
		{and|begin|break|by|call|constant|deeply|div|do|else|end|exists|for|foreach
		|func|global|if|in|inherited|local|loop|mod|native|not|onexception|or
		|repeat|return|self|then|to|try|until|while|with}


But note, from http://www.scribd.com/doc/156408379/NewtonScript-Syntax-Definition-Version-1-0

Real life
There are differences between this language definition and the language accepted by the compiler in NewtonToolkit 1.0 for Macintosh.
These differences are considered either bugs or extensions, as follows:
Extensions
	• The compiler accepts the 8-bit extended Macintosh character set in strings, and translates characters with codes >= 128 into the equivalent Unicode characters.
	  Such characters may have different translations, or may not be accepted at all, by other NewtonScript implementations.
	• The compiler accepts return characters in strings.
	  This is arguably a bug, since it makes it hard to figure out that you've accidentally omitted the final quote of a string;
	  however, in honor of existing code, we call it an extension.
	• The compiler accepts the syntax "#[ hex-digit ]+" for translating internal reference numbers into object references.
	  This is an extension for debugging purposes that is inherently unsafe, and not part of the formal NewtonScript definition.
	• The compiler does not allow the use of global variable declarations; they are anti-social when used in the Newton environment.
Bugs
x	• The compiler allows the use of \u escape sequences in symbols when surrounded by vertical bars (|).
	  This is in poor taste and can result in indeterminate behavior.
	• The compiler accepts any character after a backslash (\) in a string, xsymbol,x or character literal.
	  This behavior should not be relied on, in case future special escape characters are introduced.

------------------------------------------------------------------------------*/

UniChar
CCompiler::consumeChar(void)
{
	theChar = stream->getch();

	// update our location in the source -- we can probably do better than this
	if (theChar == '\r')		// someone, somewhere, translates \n -> \r
	{
		++lineNumber;
		colmNumber = 0;
	}
	else
		++colmNumber;

	return theChar;
}


/*------------------------------------------------------------------------------
	Get the next token from the input stream.
	Args:		--
	Return:	token number
				this.tokenValue contains the token value.
------------------------------------------------------------------------------*/

int
CCompiler::consumeToken(void)
{
	Str255 str;
	ArrayIndex length;

	while (1)
	{
		// ignore whitespace between tokens
		while (IsWhiteSpace(theChar))
			consumeChar();

		switch (theChar)
		{
		case (UniChar)EOF: case 0:
			// end-of-file
			return makeToken(EOF);

		case '/':
		{
			// could be start of comment
			consumeChar();
			if (theChar == '/')			// got a comment like this
			{
				do { consumeChar(); } while (!IsBreaker(theChar) && theChar != (UniChar)EOF);
				continue;
			}
			if (theChar == '*')	/* got a comment like this */
			{
				do
				{
					consumeChar();
					if (theChar == '*')
					{
						do { consumeChar(); } while (theChar == '*');
						if (theChar == '/')
						{
							// end of comment
							consumeChar();
							break;
						}
					}
				} while (theChar != (UniChar)EOF);
				continue;
			}
			// not a comment
			return makeToken('/');
		}

//	symbol:
//		{ { alpha | _ } [ { alpha | digit | _ } ]* | <as below> }
		case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
		case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
		case '_':
		{
			char * s = str;
			do
			{
				ASSERTMSG(s <= &str[254], "Symbol too big");
				*s++ = A_CONST_CHAR(theChar);
				consumeChar();
			} while (IsAlphaNumeric(theChar) || theChar == '_');
			*s = 0;
			if (wordcmp(str, "nil") == 0)
				return makeBooleanToken(false);
			if (wordcmp(str, "true") == 0)
				return makeBooleanToken(true);
			int token;
			if ((token = ReservedWordToken(str)) != TOKENbad)
				return makeToken(token);
			return makeSymbolToken(str);
		}

//	symbol:
//		{ <as above> | ‘|’ [ { symbol-character | \ { ‘|’ | \ } ]* ‘|’ }
//	symbol-character:
//		<any ASCII character with code 32–127 except '|' or '\'>
		case '|':
		{
			UniChar * tokenStr = getCharsUntil('|', NO, &length);
			if (tokenStr == NULL)
				return makeToken(EOF);
			ConvertFromUnicode(tokenStr, str, sizeof(str), kASCIIEncoding);
			// MUST free the string alloc’d by getCharsUntil()
			free(tokenStr);
			return makeSymbolToken(str);
		}

//	integer:
//		[-] { [ digit ]+ | 0x [ hex-digit ]+ }
//	real:
//		[-] [ digit ]+ . [ digit ]* [ {e|E} [-] [ digit ]+ ]
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			return getNumber();	// builds theToken

		case '@':		// magic pointer
		{
			consumeChar();
			if (!IsDigit(theChar))
				error(kNSErrDigitRequired);
			int mp = 0;
			while (IsDigit(theChar))
			{
				mp = (mp * 10) + (theChar - '0');
				consumeChar();
			}
			return makeRefToken(mp);
		}

//	string:
//		" character-sequence "
		case '"':
		{
			UniChar * tokenStr = getCharsUntil('"', YES, &length);
			if (tokenStr == NULL)
				return makeToken(EOF);
			int tokenId = makeStringToken(tokenStr, length);
			// MUST free the string alloc’d by getCharsUntil()
			free(tokenStr);
			return tokenId;
		}

//	character:
//		$ { non-escape-character | \ { \ | n | t | hex-digit hex-digit | u hex-digit hex-digit hex-digit hex-digit } }
		case '$':		// character
			consumeChar();
			if (theChar == '\\')
			{
				// escaped character
				consumeChar();
				if (theChar == '\\')
				{
					consumeChar();
					return makeCharacterToken('\\');
				}
				if (theChar == 'n' || theChar == 'N')
				{
					consumeChar();
					return makeCharacterToken(0x0D);
				}
				if (theChar == 't' || theChar == 'T')
				{
					consumeChar();
					return makeCharacterToken(0x09);
				}
				// must be hex equivalent
				ArrayIndex numOfHexDigits;
				if (theChar == 'u' || theChar == 'U')
				{
					// 16-bit unicode
					consumeChar();
					numOfHexDigits = 4;
				}
				else
					// 8-bit MacRoman
					numOfHexDigits = 2;
				UniChar chValue = 0;
				for (ArrayIndex i = numOfHexDigits; i > 0; i--)
				{
					if (!IsHexDigit(theChar))
						error(numOfHexDigits == 2 ? kNSErr2HexDigitsRequired : kNSErr4HexDigitsRequired);
					if (theChar >= 'a')
						theChar = theChar - 'a' + 0x0A;
					else if (theChar >= 'A')
						theChar = theChar - 'A' + 0x0A;
					else
						theChar = theChar - '0';
					chValue = (chValue << 4) + theChar;
					consumeChar();
				}
				return makeCharacterToken(chValue);
			}
			{
				UniChar uChar = theChar;
				consumeChar();
				return makeCharacterToken(uChar);
			}

		case '<':
			consumeChar();
			if (theChar == '=')
			{
				consumeChar();
				return makeToken(TOKENLEQ);
			}
			if (theChar == '<')
			{
				consumeChar();
				return makeToken(TOKENLShift);
			}
			if (theChar == '>')
			{
				consumeChar();
				return makeToken(TOKENNEQ);
			}
			return makeToken('<');

		case '>':
			consumeChar();
			if (theChar == '=')
			{
				consumeChar();
				return makeToken(TOKENGEQ);
			}
			if (theChar == '>')
			{
				consumeChar();
				return makeToken(TOKENRShift);
			}
			return makeToken('>');
		
		case '=':
			consumeChar();
			return makeToken(TOKENEQL);

		case ':':
			consumeChar();
			if (theChar == '=')
			{
				consumeChar();
				return makeToken(TOKENassign);
			}
			if (theChar == '?')
			{
				consumeChar();
				return makeToken(TOKENsendIfDefined);
			}
			return makeToken(':');

		case '&':
			consumeChar();
			if (theChar == '&')
			{
				consumeChar();
				return makeToken(TOKENAmperAmper);
			}
			return makeToken('&');

		case '\'':
 		case '.':
 		case '+': case '-': case '*': /*case '/':*/
		case '(': case ')':
		case '[': case ']':
		case '{': case '}':
		case ',': case ';':
		{
		// valid single-char tokens
			int tokn = theChar;
			consumeChar();
			return makeToken(tokn);
		}

		case '#':
		// debug extension
			getHexNumber();						// builds integer token
#if 1
			return (theToken.id = TOKENref);
#else
			theToken.value.type = TYPEref;	// munge it to ref token
			return theToken.id;
#endif

		default:
			errorWithValue(kNSErrUnrecognized, MAKECHAR(theChar));
		}
	}
}


/*------------------------------------------------------------------------------
	Scan a symbol (delimited by |) or string (delimited by ")
	from the input stream.

	symbol:
		{ { alpha | _ } [ { alpha | digit | _ } ]* | ‘|’ [ { symbol-character | \ { ‘|’ | \ } ]* ‘|’ }
	symbol-character:
		<any ASCII character with code 32–127 except '|' or '\'>

	character-sequence:
		[ { string-character | escape-sequence } ]* [ truncated-escape ]
	string-character:
		<tab or any ASCII character with code 32–127 except ‘"’ or ‘\’>
	escape-sequence:
		{ \ {"|\|n|t} | \ u [ hex-digit hex-digit hex-digit hex-digit ]* \ u }
	￼truncated-escape:
		\ u [ hex-digit hex-digit hex-digit hex-digit ]*

	Args:		inDelimiter	delimiter character
				isString		for calculating size: (char)symbol | (UniChar)string
				outLength	size to allocate for scanned token
	Return:	pointer to mallocd buffer containing token string
				This must be freed by the caller.
------------------------------------------------------------------------------*/

UniChar *
CCompiler::getCharsUntil(UniChar inDelimiter, bool isString, ArrayIndex * outLength)
{
	UniChar * buf = NULL;		// mallocd string buffer
	ArrayIndex	bufLen = 0;		// mallocd size
	ArrayIndex	index = 0;		// index into string buffer
	bool		isEscape = NO;
	int		hexIndex = -1;
	int		numOfHexDigits = isString ? 4 : 2;
	int		chValue = 0;

	consumeChar();	// consume leading delimiter
	while (1)
	{
		if (buf == NULL || index >= bufLen - 2 * sizeof(UniChar))
		{
			// grow the UniChar text buffer in 64-char chunks
			bufLen += 64 * sizeof(UniChar);
			buf = (UniChar *)realloc(buf, bufLen);
			ASSERTMSG(buf != NULL, "Compiler can’t get buffer space");
		}

		if (theChar == (UniChar)EOF)
		{
			free(buf);
			error(kNSErrEOFInAString);
			return NULL;
		}

		if (isEscape)
		{
			if (isString)
			{
//	escape-sequence:
//		{ \ {"|\|n|t} | \ u [ hex-digit hex-digit hex-digit hex-digit ]* \ u }
				if (theChar == 'u' || theChar == 'U')
				{
					if (hexIndex < 0)
						// it’s the \u prefix
						hexIndex = 0;
					else if (hexIndex == 0)
						// it’s the \u suffix
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
				else if (theChar == 'n' || theChar == 'N')
					buf[index++] = 0x0D;
				else if (theChar == 't' || theChar == 'T')
					buf[index++] = 0x09;
				else
					buf[index++] = theChar;
			}
			else
			{
//	symbol:
//		{  ‘|’ [ { symbol-character | \ { ‘|’ | \ } ]* ‘|’ }
				if (theChar == '|' || theChar == '\\')
					buf[index++] = theChar;
				else
				{
					free(buf);
					error(kNSErrBadEscape);
					return NULL;
				}
			}
			isEscape = NO;
		}

		else if (theChar == '\\')
			isEscape = YES;

		else if (theChar == inDelimiter)
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
			if (theChar >= '0' && theChar <= '9')			hex = theChar - '0';
			else if (theChar >= 'a' && theChar <= 'f')	hex = theChar - 'a' + 0x0A;
			else if (theChar >= 'A' && theChar <= 'F')	hex = theChar - 'A' + 0x0A;
			else
			{
				free(buf);
				errorWithValue(kNSErrBadHex, MAKECHAR(theChar));
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
			buf[index++] = theChar;

		consumeChar();	// consume character
	}
	consumeChar();	// consume trailing delimiter

	// nul-terminate the string
	buf[index++] = kEndOfString;
	*outLength = index * sizeof(UniChar);
	return buf;
}


/*------------------------------------------------------------------------------
	Scan a number from the input stream.

	integer:
		[-] { [ digit ]+ | 0x [ hex-digit ]+ }
	real:
		[-] [ digit ]+ . [ digit ]* [ {e|E} [-] [ digit ]+ ]

	Args:		inCh		first character of number
	Return:	token number (integer or real)
				this.tokenValue contains the token value.
------------------------------------------------------------------------------*/

int
CCompiler::getHexNumber(void)
{
	Str255 str;
	ArrayIndex i = 0;
	while (theChar != (UniChar)EOF)
	{
		if (IsHexDigit(theChar))
		{
			if (i < 255)
				str[i++] = theChar;
			else
				error(kNSErrNumberTooLong);
			consumeChar();
		}
		else
			break;
	}
	str[i] = 0;
	long value = strtol(str, NULL, 16);
	if (value >= kMaxUnsignedIntRef)
		errorWithValue(kNSErrInvalidHex, MakeStringFromCString(str));
	return makeIntegerToken(value);
}


int
CCompiler::getNumber(void)
{
	Str255 str;
	ArrayIndex i = 0;

	if (theChar == '0')
	{
	//	might be hex
		consumeChar();
		str[0] = '0'; i = 1;
		if (theChar == 'x' || theChar == 'X')
		{
		// it’s a hex integer
			consumeChar();
			return getHexNumber();
		}
		// OK, it’s not hex
	}

	// it’s a decimal integer
	while (theChar != (UniChar)EOF)
	{
		if (IsDigit(theChar))
		{
			if (i < 255)
				str[i++] = theChar;
			else
				error(kNSErrNumberTooLong);
			consumeChar();
		}
		else if (theChar == '.')
		{
		// actually it’s real!
			str[i++] = '.';
			consumeChar();
			while (theChar != (UniChar)EOF)
			{
				if (IsDigit(theChar))
				{
					if (i < 255)
						str[i++] = theChar;
					else
						error(kNSErrNumberTooLong);
					consumeChar();
				}
				else if (theChar == 'e' || theChar == 'E')
				{
					if (i < 255)
						str[i++] = 'e';
					else
						error(kNSErrNumberTooLong);
					consumeChar();
					if (theChar == '+' || theChar == '-')
					{
						if (i < 255)
							str[i++] = theChar;
						else
							error(kNSErrNumberTooLong);
						consumeChar();
					}
					else
					{
						while (theChar != (UniChar)EOF)
						{
							if (IsDigit(theChar))
							{
								if (i < 255)
									str[i++] = theChar;
								else
									error(kNSErrNumberTooLong);
								consumeChar();
							}
							else
								break;
						}
					}
				}
				else
				//	that’s the end of the real
					break;
			}
			str[i] = 0;
			return makeRealToken(strtod(str, NULL));
		}
		else
		//	that’s the end of the integer
			break;
	}
	str[i] = 0;
	long value = strtol(str, NULL, 10);
	if (value <= kMinSignedIntRef || value >= kMaxSignedIntRef)
		errorWithValue(kNSErrInvalidDecimal, MakeStringFromCString(str));
	return makeIntegerToken(value);
}

