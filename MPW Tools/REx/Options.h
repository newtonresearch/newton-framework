/*
	File:		Options.h

	Contains:	Command-line option handling.

	Written by:	The Newton Tools group.
*/

#ifndef __OPTIONS_H
#define __OPTIONS_H 1

#include "Lexer.h"


/* -----------------------------------------------------------------------------
	E l e m e n t
	An element in the linked list of options - name|value pair.
----------------------------------------------------------------------------- */

struct Element
{
	Element();

	Element *	fNext;
	string		fName;
	Token			fToken;
};


/* -----------------------------------------------------------------------------
	C O p t i o n L i s t
----------------------------------------------------------------------------- */

class COptionList
{
public:
				COptionList();
				~COptionList();

	void		readOptions(CLexer * inSource);
	bool		hasOptions(void) const;
	bool		getOption(const char * inName, int * outValue, int inArg3, int inMin, int inMax);
	bool		findOption(const char * inName, Token & outToken);
	bool		nextOption(string & outName, Token & outToken);
	SourcePos &	position(void);
	void		printLeftovers(void);

private:
	Element *	fList;
	SourcePos	fPosition;
};

inline bool COptionList::hasOptions(void) const { return fList != NULL; }
inline SourcePos & COptionList::position(void) { return fPosition; }


#endif	/* __OPTIONS_H */
