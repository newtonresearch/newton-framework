/*
	File:		RefMemory.h

	Contains:	Ref-to-address translation functions.

	Written by:	Newton Research Group.
*/

#if !defined(__REFMEMORY_H)
#define __REFMEMORY_H 1

#include "ObjHeader.h"

ObjHeader *	FaultCheckObjectPtr(Ref r);
ObjHeader *	NoFaultObjectPtr(Ref r);
ObjHeader *	ResolveMagicPtr(Ref r);
Ref			ForwardReference(Ref r);

void			DirtyObject(Ref r);
void			UndirtyObject(Ref r);

#endif	/* __REFMEMORY_H */
