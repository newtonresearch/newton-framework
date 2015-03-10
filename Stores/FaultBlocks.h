/*
	File:		FaultBlocks.h

	Contains:	Fault block declarations.
	
	Written by:	Newton Research Group.
*/

#if !defined(__FAULTBLOCKS_H)
#define __FAULTBLOCKS_H 1

#include "ObjHeader.h"
#include "DynamicArray.h"
#include "StoreWrapper.h"

/*------------------------------------------------------------------------------
	F a u l t   B l o c k s
------------------------------------------------------------------------------*/

struct FaultObject
{
	OBJHEADER

	Ref	objClass;
	Ref	handler;
	Ref	store;			// actually (CStoreWrapper *)
	Ref	id;
	Ref	object;
}__attribute__((packed));


ObjHeader * ObjectPtr1(Ref inObj, long inTag, bool inDoFaultCheck);

Ref		ForwardEntryMessage(RefArg inRcvr, RefArg inSymbol);
Ref		ForwardEntryMessage(RefArg inRcvr, RefArg inSymbol, RefArg inArg);

Ref		MakeFaultBlock(RefArg inHandler, CStoreWrapper * inStoreWrapper, PSSId inId);
Ref		MakeFaultBlock(RefArg inHandler, CStoreWrapper * inStoreWrapper, PSSId inId, RefArg inObject);
bool		IsFaultBlock(Ref r);
void		WriteFaultBlock(RefArg inRcvr);
void		InvalFaultBlock(RefArg inRcvr);
void		UncacheIfFaultBlock(RefArg inEntry);

Ref		LoadPermObject(CStoreWrapper * inStoreWrapper, PSSId inId, CDynamicArray ** ioArray);
void		StorePermObject(RefArg inObj, CStoreWrapper * inStoreWrapper, PSSId & ioId, CDynamicArray * inArray, bool * outArg5);
void		DeletePermObject(CStoreWrapper * inStoreWrapper, PSSId inId);


/*------------------------------------------------------------------------------
	C a c h e
------------------------------------------------------------------------------*/

Ref	GetEntry(RefArg inSoup, PSSId inId);

Ref	MakeEntryCache(void);
void	PutEntryIntoCache(RefArg inCache, RefArg inEntry);
Ref	FindEntryInCache(RefArg inCache, PSSId inId);
void	InvalidateCacheEntries(RefArg inCache);
void	DeleteEntryFromCache(RefArg inCache, RefArg inEntry);

#endif	/* __FAULTBLOCKS_H */
