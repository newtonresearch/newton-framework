/*
	File:		Clausehandlers.h

	Contains:	Class to handle a REx config clause.

	Written by:	The Newton Tools group.
*/

#ifndef __CLAUSEHANDLERS_H
#define __CLAUSEHANDLERS_H 1


/* -----------------------------------------------------------------------------
	C C l a u s e H a n d l e r
	REx config file clause handler.
----------------------------------------------------------------------------- */
typedef void (*ClauseHandlerProc)(void);

class CClauseHandler
{
public:
				CClauseHandler();

	void		globalRegister(void);

	static void		printClauseHelp(void);
	static bool		handleClause(Token & inToken);

private:
	virtual bool	canHandle(Token & inToken) = 0;
	virtual void	handle(void) = 0;
	virtual void	printHelp(void) = 0;

	CClauseHandler *	fNext;
};


/* -----------------------------------------------------------------------------
	C N a m e d C l a u s e H a n d l e r
	Private implementation of REx config file clause handler.
----------------------------------------------------------------------------- */

class CNamedClauseHandler : public CClauseHandler
{
public:
	static void		registerHandler(const char * inToken, ClauseHandlerProc inHandler, const char * inHelp);

private:
				CNamedClauseHandler(const char * inToken, ClauseHandlerProc inHandler, const char * inHelp);
				~CNamedClauseHandler();

	bool		canHandle(Token & inToken);
	void		handle(void);
	void		printHelp(void);

	const char *		fToken;
	ClauseHandlerProc	fHandler;
	const char *		fHelpStr;
};


#endif	/* __CLAUSEHANDLERS_H */
