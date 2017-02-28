/*
	File:		ClauseHandlers.cc

	Contains:	ROM extension config file clause handlers.

	Written by:	The Newton Tools group.
*/

#include "Lexer.h"
#include "ClauseHandlers.h"

extern int symcmp(const char * inSym1, const char * inSym2);

/* -----------------------------------------------------------------------------
	D a t a
----------------------------------------------------------------------------- */

CClauseHandler * gClauses = NULL;	// linked list


#pragma mark -
/* -----------------------------------------------------------------------------
	C N a m e d C l a u s e H a n d l e r
----------------------------------------------------------------------------- */

CNamedClauseHandler::CNamedClauseHandler(const char * inToken, ClauseHandlerProc inHandler, const char * inHelp)
{
	fToken = inToken;
	fHandler = inHandler;
	fHelpStr = inHelp;
}


void
CNamedClauseHandler::registerHandler(const char * inToken, void(*inHandler)(void), const char * inHelp)
{
	CNamedClauseHandler * handler = new CNamedClauseHandler(inToken, inHandler, inHelp);
	// should fail if NULL, surely?
	handler->globalRegister();
}


bool
CNamedClauseHandler::canHandle(Token & inToken)
{
	return symcmp(inToken.fStrValue.c_str(), fToken) == 0;
}

void
CNamedClauseHandler::handle(void)
{
	fHandler();
}

void
CNamedClauseHandler::printHelp(void)
{
	cout << fToken;
	if (fHelpStr != NULL) {
		cout << " " << fHelpStr;
	}
	cout << endl;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C C l a u s e H a n d l e r
----------------------------------------------------------------------------- */

CClauseHandler::CClauseHandler()
{
	fNext = NULL;
}

void
CClauseHandler::globalRegister(void)
{
	fNext = gClauses;	// thread us onto head of list
	gClauses = this;
}

bool
CClauseHandler::handleClause(Token & inToken)
{
	for (CClauseHandler * handler = gClauses; handler != NULL; handler = handler->fNext) {
		if (handler->canHandle(inToken)) {
			handler->handle();
			return true;
		}
	}
	return false;
}

void
CClauseHandler::printClauseHelp(void)
{
	for (CClauseHandler * handler = gClauses; handler != NULL; handler = handler->fNext) {
		cerr << "### ";
		handler->printHelp();
	}
}

