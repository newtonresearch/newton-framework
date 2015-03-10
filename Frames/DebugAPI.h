/*
	File:		DebugAPI.h

	Contains:	Interpreter debug function definitions.

	Written by:	Newton Research Group, 2013.
*/

#if !defined(__DEBUGAPI_H)
#define __DEBUGAPI_H 1

#include "Objects.h"
#include "Interpreter.h"


/* -----------------------------------------------------------------------------
	C N S D e b u g A P I
----------------------------------------------------------------------------- */

class CNSDebugAPI
{
public:
					CNSDebugAPI(CInterpreter * interpreter);
					~CNSDebugAPI();

	ArrayIndex	numStackFrames(void);
	VMState *	stackFrameAt(ArrayIndex index);
	bool			accurateStack(void);

	ArrayIndex	PC(ArrayIndex index);
	void			setPC(ArrayIndex index, ArrayIndex inPC);
	Ref			function(ArrayIndex index);
	void			setFunction(ArrayIndex index, RefArg inFunction);
	Ref			receiver(ArrayIndex index);
	void			setReceiver(ArrayIndex index, RefArg inRcvr);
	Ref			implementor(ArrayIndex index);
	void			setImplementor(ArrayIndex index, RefArg inImpl);
	Ref			locals(ArrayIndex index);

	Ref			getVar(ArrayIndex index, ArrayIndex inVarIndex);
	void			setVar(ArrayIndex index, ArrayIndex inVarIndex, RefArg inVar);
	Ref			findVar(ArrayIndex index, RefArg inSym);
	void			setFindVar(ArrayIndex index, RefArg inSym, RefArg inVar);

	ArrayIndex	numTemps(ArrayIndex index);
	Ref			tempValue(ArrayIndex index, ArrayIndex inTempIndex);
	void			setTempValue(ArrayIndex index, ArrayIndex inTempIndex, RefArg inTemp);

	void			Return(ArrayIndex index, RefArg);

private:
	ArrayIndex	stackStart(ArrayIndex index);

	CInterpreter * fInterpreter;
};


extern CNSDebugAPI *	NewNSDebugAPI(CInterpreter * interpreter);
extern void				DeleteNSDebugAPI(CNSDebugAPI * inDebugAPI);

#endif	/* __DEBUGAPI_H */
