/*
	File:		Options.cc

	Contains:	Command-line option handling.

	Written by:	The Newton Tools group.
*/

#include "Options.h"
#include "Reporting.h"

extern int symcmp(const char * inSym1, const char * inSym2);


/* -----------------------------------------------------------------------------
	E l e m e n t
----------------------------------------------------------------------------- */

Element::Element()
{ }


#pragma mark -
/* -----------------------------------------------------------------------------
	C O p t i o n L i s t
----------------------------------------------------------------------------- */

COptionList::COptionList()
	: fList(NULL)
{ }


COptionList::~COptionList()
{
	for (Element * opt = fList; opt != NULL; opt = opt->fNext) {
		delete opt;
	}
}


/* -----------------------------------------------------------------------------
	Read options from the lexer, populating our list.
	Args:		inSource		the lexical source
	Return:	--
----------------------------------------------------------------------------- */

void
COptionList::readOptions(CLexer * inSource)
{
	// read opening {
	Token tokn;
	inSource->get(kOpenBracketToken, tokn, kMandatory);
	fPosition = tokn.fPosition;

	// read name|value pairs until closing }
	while (!inSource->get(kCloseBracketToken, tokn, kOptional)) {
		Token optName;
		inSource->get(kIdentifierToken, optName, kMandatory);
		Token optValue;
		if (!inSource->getToken(optValue) || optValue.fType > kValueToken) {
			FatalError(optValue.fPosition, "expected a value");
		}
		inSource->consumeToken();
		// create a new element and thread it onto head of list
		Element * item = new Element;
		item->fNext = fList;
		fList = item;
		item->fName = optName.fStrValue;
		item->fToken = optValue;
	}
}


/* -----------------------------------------------------------------------------
	Get a named option in our list, removing it from the list, and returning
	its value.
	Args:		inName		the name of the option -- MUST be lowercase
				outValue		its value
				inArg3
				inMin			minimum permitted value
				inMax			maximum permitted value
	Return:	true => success
----------------------------------------------------------------------------- */

bool
COptionList::getOption(const char * inName, int * outValue, int inArg3, int inMin, int inMax)
{
	Token tokn;
	if (findOption(inName, tokn)) {
		if (tokn.fType != kIntegerToken) {
			FatalError("option “%s” must be an integer", inName);
		}
		if (tokn.fValue < inMin || tokn.fValue > inMax) {
			FatalError("option “%s” must be between %ld and %ld", inName, inMin, inMax);
		}
		*outValue = tokn.fValue;
		return true;
	}

	if (tokn.fPosition.fLineNo == 0) {
		FatalError("missing required option “%s”", inName);
	}
	return false;
}


/* -----------------------------------------------------------------------------
	Find a named option in our list, removing it from the list.
	Args:		inName		the name of the option -- MUST be lowercase
				outToken		its value
	Return:	true => success
----------------------------------------------------------------------------- */

bool
COptionList::findOption(const char * inName, Token & outToken)
{
	for (Element * prevItem = NULL, * item = fList; item != NULL; prevItem = item, item = item->fNext) {
		if (symcmp(item->fName.c_str(), inName) == 0) {
			outToken = item->fToken;
			// unthread the element from the list
			Element * nextItem = item->fNext;
			if (prevItem != NULL) {
				prevItem->fNext = nextItem;
			} else {
				fList = nextItem;
			}
			delete item;
			return true;
		}
	}
	return false;
}


/* -----------------------------------------------------------------------------
	Return the next option in our list, removing it from the list.
	Args:		outName		the name of the option
				outToken		its value
	Return:	true => success
----------------------------------------------------------------------------- */

bool
COptionList::nextOption(string & outName, Token & outToken)
{
	Element * item = fList;
	if (item != NULL) {
		outName = item->fName;
		outToken = item->fToken;
		fList = item->fNext;
		delete item;
		return true;
	}
	return false;
}


/* -----------------------------------------------------------------------------
	Print the unused options read from the lex stream.
	Args:		--
	Return:	--
----------------------------------------------------------------------------- */

void
COptionList::printLeftovers(void)
{
	string name;
	Token tokn;
	while (nextOption(name, tokn)) {
		cout << name;
		if (fList != NULL) {	// there are more
			cout << ", ";
		}
	}
	cout << endl;
}

