/*
	File:		Lexer.h

	Contains:	Class to tokenise input stream.

	Written by:	The Newton Tools group.
*/

#ifndef __LEXER_H
#define __LEXER_H 1

#include "NewtonKit.h"
#include <iostream>
#include <vector>
using namespace std;


/* -----------------------------------------------------------------------------
	S o u r c e P o s
	Position of token in the source stream.
----------------------------------------------------------------------------- */

struct SourcePos
{
	SourcePos();
	SourcePos(SourcePos &);

	const char *	fFilename;
	int		fLineNo;
	int		fMark;
	int		fCharNo;
};

ostream & operator<<(ostream & __os, SourcePos & inPos);


/* -----------------------------------------------------------------------------
	T o k e n
	Token returned from lexical analysis.
	A token is a name|value pair at a position in the source stream.
----------------------------------------------------------------------------- */

enum Kind
{
	kIdentifierToken,
	kIntegerToken,
	kStringToken,
	kDataToken,
	kValueToken,
	kEOFToken,
	kOpenBracketToken,
	kCloseBracketToken,
	kAssignToken
};

struct Token
{
	Token();
	Token(Kind inType, SourcePos & inPos, string inData);
	Token(Kind inType, SourcePos & inPos, int inValue = 0);

	Kind			fType;
	SourcePos	fPosition;
	string		fStrValue;
	int			fValue;
};

#define kOptional true
#define kMandatory false


/* -----------------------------------------------------------------------------
	C L e x e r
	Lexical analysis of the source stream.
----------------------------------------------------------------------------- */

class CLexer
{
public:
					CLexer();
					CLexer(const char * inFilename);
					~CLexer();

	bool			get(Kind inType, Token & outToken, bool inOptional);
	bool			getToken(Token & outToken);
	void			consumeToken(void);
	void			eatWhitespace(void);
	SourcePos &	position(void);

private:
	void			consumeChar(void);

	SourcePos		fPosition;
	char				theChar;
	Token				theToken;
};

inline SourcePos & CLexer::position(void) { return fPosition; }


extern CLexer * gLex;


#endif	/* __LEXER_H */
