/*
	File:		Funcs.cc

	Contains:	Plain C functions.

	Written by:	The Newton Tools group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "Strings.h"
#include "Exceptions.h"
#include "Iterators.h"
#include "Unicode.h"
#include "Screen.h"


/*------------------------------------------------------------------------------
	Initialize built-in functions.
	Plain C functions have to be added to the functions global procedurally
	since we can’t get a reference to them in the NS world.
	Args:		--
	Return:  --
------------------------------------------------------------------------------*/

extern "C"
void
InitBuiltInFunctions(void)
{
	FrameObject *	fr = (FrameObject *) PTR(gConstNSData->internalFunctions);
	fr->flags &= ~kObjReadOnly;

	AddPlainCFunction(FAdd, FAdd);
	AddPlainCFunction(FSubtract, FSubtract);
	AddPlainCFunction(aref, FAref);
	AddPlainCFunction(setAref, FSetAref);
	AddPlainCFunction(FEqual, FEqual);
	AddPlainCFunction(FNot, FNot);
	AddPlainCFunction(FUnorderedLessOrGreater, FUnorderedLessOrGreater);
	AddPlainCFunction(FMultiply, FMultiply);
	AddPlainCFunction(FDivide, FDivide);
	AddPlainCFunction(FDiv, FDiv);
	AddPlainCFunction(FLessThan, FLessThan);
	AddPlainCFunction(FLessOrEqual, FLessOrEqual);
	AddPlainCFunction(FGreaterThan, FGreaterThan);
	AddPlainCFunction(FGreaterOrEqual, FGreaterOrEqual);
	AddPlainCFunction(band, FBitAnd);
	AddPlainCFunction(bor, FBitOr);
	AddPlainCFunction(bnot, FBitNot);
	AddPlainCFunction(newIterator, FNewIterator);
	AddPlainCFunction(length, FLength);
	AddPlainCFunction(clone, FClone);
	AddPlainCFunction(setClass, FSetClass);
	AddPlainCFunction(addArraySlot, FAddArraySlot);
	AddPlainCFunction(stringer, FStringer);
	AddPlainCFunction(hasPath, FHasPath);
	AddPlainCFunction(classOf, FClassOf);

	AddPlainCFunction(_Open, FOpenX);
	AddPlainCFunction(GetOrientation, FGetOrientation);
	AddPlainCFunction(EnsureInternal, FEnsureInternal);
	AddPlainCFunction(HasSlot, FHasSlot);
	AddPlainCFunction(DefGlobalFn, FDefGlobalFn);
	AddPlainCFunction(IsFunction, FIsFunction);
	AddPlainCFunction(IsNativeFunction, FIsNativeFunction);

	AddPlainCFunction(AddDeferredAction, FAddDeferredAction);
	AddPlainCFunction(AddDeferredSend, FAddDeferredSend);
	AddPlainCFunction(AddDeferredCall, FAddDeferredCall);
	AddPlainCFunction(AddDelayedAction, FAddDelayedAction);
	AddPlainCFunction(AddDelayedSend, FAddDelayedSend);
	AddPlainCFunction(AddDelayedCall, FAddDelayedCall);

	fr->flags |= kObjReadOnly;
}

