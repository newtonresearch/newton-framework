/*
	File:		Lookup.h

	Contains:	Slot lookup functions.

	Written by:	Newton Research Group.
*/

#if !defined(__LOOKUP__)
#define __LOOKUP__ 1

#include "Objects.h"

#if defined(__cplusplus)
extern "C" {
#endif

typedef enum
{
	kNoLookup,
	kLexicalLookup		=	(1 << 0),
	kGlobalLookup		=	(1 << 1),
	kSetSelfLookup		=	(1 << 2),
	kSetGlobalLookup	=	(1 << 3)
} LookupType;

// interpreter cache
void	InitICache(void);
void	ICacheClear(void);
void	ICacheClearSymbol(Ref sym, ULong hash);
void	ICacheClearFrame(Ref fr);

Ref	FindImplementor(RefArg rcvr, RefArg msg);
Ref	FindProtoImplementor(RefArg rcvr, RefArg msg);
bool	XFindImplementor(RefArg rcvr, RefArg msg, RefVar * impl, RefVar *);
bool	XFindProtoImplementor(RefArg rcvr, RefArg msg, RefVar * impl, RefVar *);

Ref	GetVariable(RefArg context, RefArg name, bool * exists = NULL, int lookup = kNoLookup);
Ref	XGetVariable(RefArg context, RefArg name, bool * exists, int lookup);
Ref	GetProtoVariable(RefArg context, RefArg name, bool * exists = NULL);
bool	SetVariable(RefArg context, RefArg name, RefArg value);
bool	SetVariableOrGlobal(RefArg context, RefArg name, RefArg value, int lookup);

bool	IsParent(RefArg fr, RefArg context);

#if defined(__cplusplus)
}
#endif

#endif	/* __LOOKUP__ */
