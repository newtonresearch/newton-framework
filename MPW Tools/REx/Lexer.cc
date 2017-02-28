/*
	File:		Lexer.cc

	Contains:	Lexical analysis of source stream - tokenisation.

	Written by:	The Newton Tools group.
*/

#include "Lexer.h"
#include "Reporting.h"
#include <sstream>
#include <ctype.h>


/* -----------------------------------------------------------------------------
	S o u r c e P o s
----------------------------------------------------------------------------- */

SourcePos::SourcePos()
	: fFilename(NULL), fLineNo(1), fMark(1), fCharNo(0)
{ }


SourcePos::SourcePos(SourcePos & inPos)
{
	fFilename = inPos.fFilename;
	fLineNo = inPos.fLineNo;
	fMark = inPos.fMark;
	fCharNo = inPos.fCharNo;
}


/* -----------------------------------------------------------------------------
	Reporting.
	Output source position to a stream.
----------------------------------------------------------------------------- */

ostream &
operator<<(ostream & __os, SourcePos & inPos)
{
	__os << "File “" << (inPos.fFilename ? inPos.fFilename : "<unknown>") << "” Line " << inPos.fLineNo << " " << inPos.fMark << ":" << inPos.fCharNo - inPos.fMark;
	return __os;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	T o k e n
----------------------------------------------------------------------------- */

Token::Token()
	: fType(kIdentifierToken), /*fStrValue(NULL),*/ fValue(0)
{ }

Token::Token(Kind inType, SourcePos & inPos, string inData)
{
	fType = inType;
	fPosition = inPos;
	fStrValue = inData;
}

Token::Token(Kind inType, SourcePos & inPos, int inValue)
{
	fType = inType;
	fPosition = inPos;
//	fStrValue = NULL;
	fValue = inValue;
}


const char * kTokenTypes[] = {
	"identifier",
	"integer",
	"string",
	"raw data block",
	"???",
	"EOF",
	"'{'",
	"'}'",
	"'='"
};


#pragma mark -
/* -----------------------------------------------------------------------------
	H e l p e r
----------------------------------------------------------------------------- */

int
HexDigit(char inChar)
{
	if (inChar >= '0' && inChar <= '9')
		return inChar - '0';
	if (inChar >= 'A' && inChar <= 'F')
		return inChar - 'A' + 0x0A;
	if (inChar >= 'a' && inChar <= 'f')
		return inChar - 'a' + 0x0A;
	return 0;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C L e x e r
----------------------------------------------------------------------------- */

CLexer::CLexer(const char * inFilename)
{
	fPosition.fFilename = inFilename;
	// prime the token pipe
	theChar = EOF;
	consumeChar();
	consumeToken();
}

CLexer::~CLexer()
{
}


bool
CLexer::get(Kind inType, Token & outToken, bool inOptional)
{
	if (getToken(outToken) && outToken.fType == inType) {
		consumeToken();
		return true;
	}
	if (!inOptional) {
		FatalError("expected %s", kTokenTypes[inType]);
	}
	return false;
}


bool
CLexer::getToken(Token & outToken)
{
	outToken = theToken;
	return (theToken.fType != kEOFToken);
}


void
CLexer::consumeToken(void)
{
	// skip whitespace
	eatWhitespace();

	// EOF => return an EOF token; there is no more
	if (theChar == EOF) {
		theToken = Token(kEOFToken, fPosition);
		return;
	}
	fPosition.fMark = fPosition.fCharNo;	// mark start of token

	if (isalpha(theChar)) {
		// identifier
		ostringstream idStr;
		while (isalnum(theChar)) {
			idStr << theChar;
			consumeChar();
		}
		string s(idStr.str());
		theToken = Token(kIdentifierToken, fPosition, s.data());

	} else if (isdigit(theChar) || theChar == '-') {
		// integer
		int theValue = 0;
		bool isNegative = false, isHex = false;
		if (theChar == '-') {
			isNegative = true;
			consumeChar();
		}
		if (theChar == '0') {
			// might be 0x hex number -- if not doesn’t matter, it won’t affect the value
			consumeChar();
			if (theChar == 'x') {
				isHex = true;
				consumeChar();
			}
		}
		// at this point theChar is the first digit of a number
		while (theChar != EOF) {
			if (isHex) {
				if (isxdigit(theChar)) {
					theValue = (theValue << 4) + HexDigit(theChar);
					consumeChar();
				} else {
					break;
				}
			} else {
				if (isdigit(theChar)) {
					theValue = (theValue * 10) + (theChar - '0');
					consumeChar();
				} else {
					break;
				}
			}
		}
		theToken = Token(kIntegerToken, fPosition, theValue);

	} else if (theChar == '"') {
		// string
		ostringstream str;
		bool isEscaped = false;
		while (1) {
			consumeChar();
			if (theChar == EOF) {
				FatalError(fPosition, "end of file in string");
			}
			if (isEscaped) {
				if (theChar == '"' || theChar == '\\') {
					// pass it through
				} else if (theChar == 'n') {
					theChar = 0x0A;	// newline
				} else if (theChar == 't') {
					theChar = 0x09;	// tab
				} else {
					FatalError(fPosition, "bad string escape '\\%c'", theChar);
				}
				isEscaped = false;
			} else if (theChar == '\\') {
				// escape character
				isEscaped = true;
			} else if (theChar == '"') {
				// end of string
				consumeChar();
				break;
			}
			if (!isEscaped) {
				str << theChar;
			}
		}
		theToken = Token(kStringToken, fPosition, str.str());

	} else if (theChar == '\'') {
		// four-char constant
		ArrayIndex chLen = 0;
		ULong theValue = 0;
		while (1) {
			consumeChar();
			if (theChar == EOF) {
				FatalError(fPosition, "end of file in char constant");
			}
			if (theChar == '\'') {
				// end of string
				consumeChar();
				break;
			}
			if (++chLen > 4) {
				FatalError(fPosition, "more than four characters in char constant");
			}
			theValue = (theValue << 8) + theChar;
		}
		theToken = Token(kIntegerToken, fPosition, theValue);

	} else if (theChar == '<') {
		// raw hex data
		ostringstream data;
		unsigned char theValue = 0;
		ArrayIndex dataLen = 0;	// counts nybbles
		while (1) {
			consumeChar();
			if (theChar == EOF) {
				FatalError(fPosition, "end of file in string");
			}
			if (isxdigit(theChar)) {
				theValue = (theValue << 4) + HexDigit(theChar);
				++dataLen;
				if (EVEN(dataLen)) {
					data << theValue;
				}
			} else if (theChar == '/' || theChar == '>') {
				// end of line/string
				if (ODD(dataLen)) {
					Warning(fPosition, "hex group doesn’t end at a byte boundary");
				}
				if (theChar == '/') {
					eatWhitespace();
				} else {
					// must be '>'
					consumeChar();
					break;
				}
			} else {
				FatalError(fPosition, "bad character '%c' in raw data", theChar);
			}
		}
		theToken = Token(kDataToken, fPosition, data.str());

	} else if (theChar == '{') {
		consumeChar();
		theToken = Token(kOpenBracketToken, fPosition);

	} else if (theChar == '}') {
		consumeChar();
		theToken = Token(kCloseBracketToken, fPosition);

	} else if (theChar == '=') {
		consumeChar();
		theToken = Token(kAssignToken, fPosition);

	} else {
		fPosition.fMark = fPosition.fCharNo - 1;
		FatalError(fPosition, "bad character '%c'", theChar);
	}
}


void
CLexer::consumeChar(void)
{
	if (theChar == '\n' || theChar == '\r') {
		++fPosition.fLineNo;
		fPosition.fCharNo = 0;
	}
	if (theChar != EOF) {
		++fPosition.fCharNo;
	}
	if (cin.eof()) {
		theChar = EOF;
	} else {
		cin >> noskipws >> theChar;
	}
}


void
CLexer::eatWhitespace(void)
{
	while (1) {
		if (isspace(theChar)) {
			consumeChar();
		} else if (theChar == '/') {
			// could be a comment
			consumeChar();
			if (theChar == '/') {
				// it’s a comment like this -- look for end of line
				do { consumeChar(); } while (theChar != 0x0A && theChar != 0x0D && theChar != EOF);
			} else if (theChar == '*') {
				/* it’s a comment like this -- look for closing */
				do {
					consumeChar();
					if (theChar == '*') {
						do { consumeChar(); } while (theChar == '*');
						if (theChar == '/') {
							// end of comment
							consumeChar();
							break;
						}
					}
				} while (theChar != EOF);
			} else {
				// nothing else is allowed
				fPosition.fMark = fPosition.fCharNo - 1;
				FatalError(fPosition, "bad character '/'");
			}
			if (theChar == EOF) {
				FatalError("end of file inside comment");
			}
		} else {
			break;
		}
	}
}

