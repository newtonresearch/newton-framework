/*
	File:		Funcs.h

	Contains:	NewtonScript function calling functions(!)

	Written by:	Newton Research Group.
*/

#if !defined(__FUNCS__)
#define __FUNCS__ 1

#if defined(__cplusplus)
extern "C" {
#endif

Ref	InterpretBlock(RefArg codeBlock, RefArg args);
Ref	DoBlock(RefArg codeBlock, RefArg args);
Ref	DoScript(RefArg rcvr, RefArg script, RefArg args);
Ref	DoMessage(RefArg rcvr, RefArg msg, RefArg args);
Ref	DoMessageIfDefined(RefArg rcvr, RefArg msg, RefArg args, bool * isDefined);

ArrayIndex	GetFunctionArgCount(Ref fn);

#if defined(__cplusplus)
}
#endif

#endif	/* __FUNCS__ */
