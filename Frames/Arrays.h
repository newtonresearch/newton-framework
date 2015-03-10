/*
	File:		Arrays.h

	Contains:	Array declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__ARRAYS__)
#define __ARRAYS__ 1

#if defined(__cplusplus)
extern "C" {
#endif

Ref	ArrayPop(RefArg array);
bool	ArrayIsEmpty(RefArg array);

Ref	ArrayInsert(RefArg ioArray, RefArg inObj, ArrayIndex index);

Ref	GetArraySlotRef(Ref obj, ArrayIndex slot);
void	SetArraySlotRef(Ref obj, ArrayIndex slot, Ref value);

void			UnsafeSetArrayLength(RefArg obj, ArrayIndex length);
ArrayIndex	UnsafeArrayLength(Ptr obj);

#if defined(__cplusplus)
}
#endif

#endif	/* __ARRAYS__ */
