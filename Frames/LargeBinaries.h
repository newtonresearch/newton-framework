/*
	File:		LargeBinaries.h

	Contains:	Large (virtual) binary object declarations.

	Written by:	Newton Research Group.
*/

#if !defined(__LARGEBINARIES_H)
#define __LARGEBINARIES_H 1

#include "LargeObjects.h"


/*------------------------------------------------------------------------------
	L B D a t a
------------------------------------------------------------------------------*/

struct LBData
{
	operator	PSSId() const;

	void					setStore(CStoreWrapper * inStore);
	CStoreWrapper *	getStore(void)	const;

	bool					isSameEntry(Ref r);

	size_t		fSize;
	PSSId			fId;
	Ref			fEntry;
	StoreRef		fClass;
	VAddr			fAddr;
	ArrayIndex	fStoreIndex;
};

inline	LBData::operator PSSId() const
{ return fId; }


/*------------------------------------------------------------------------------
	L B S t o r e I n f o
	Info about a store on which one might find large binary objects.
------------------------------------------------------------------------------*/

struct LBStoreInfo
{
	CStoreWrapper *	store;
	int					refCount;
};


/*------------------------------------------------------------------------------
	L a r g e   B i n a r y   O b j e c t   P r o c s
------------------------------------------------------------------------------*/

struct IndirectBinaryProcs
{
	size_t	(*GetLength)	(void * inData);
	char *	(*GetData)		(void * inData);
	void		(*SetLength)	(void * inData, size_t inLength);
	Ref		(*Clone)			(void * inData, Ref inClass);
	void		(*Delete)		(void * inData);
	void		(*SetClass)		(void * inData, RefArg inClass);
	void		(*Mark)			(void * inData);
	void		(*UpdateRef)	(void * inData);
};

extern const IndirectBinaryProcs  gLBProcs;


/*------------------------------------------------------------------------------
	F u n c t i o n   P r o t o t y p e s
------------------------------------------------------------------------------*/

extern	Ref		AllocateLargeBinary(RefArg inClass, StoreRef inClassRef, size_t inSize, CStoreWrapper * inStoreWrapper);
extern	void		AbortLargeBinaries(RefArg inEntry);
extern	void		CommitLargeBinary(RefArg inBinary);
extern	void		MungeLargeBinary(RefArg inBinary, long inOffset, long inLength);
extern	Ref		LoadLargeBinary(CStoreWrapper * inStoreWrapper, PSSId inId, StoreRef inClassRef);
extern	Ref		DuplicateLargeBinary(RefArg inBinary, CStoreWrapper * inStoreWrapper);
extern	void		DeleteLargeBinary(CStoreWrapper * inStoreWrapper, PSSId inId);
extern	void		FinalizeLargeObjectWrites(CStoreWrapper * inStoreWrapper, CDynamicArray * inArg2, CDynamicArray * inLargeBinaries);

extern "C" Ref		FLBAllocCompressed(RefArg inStoreWrapper, RefArg inClass, RefArg inLength, RefArg inCompanderName, RefArg inCompanderData);
extern "C" Ref		FLBAlloc(RefArg inStoreWrapper, RefArg inClass, RefArg inLength);
extern "C" Ref		FLBRollback(RefArg rcvr, RefArg inBinary);

extern "C" Ref		FGetBinaryStore(RefArg rcvr, RefArg obj);
extern "C" Ref		FGetBinaryCompander(RefArg rcvr, RefArg obj);
extern "C" Ref		FGetBinaryCompanderData(RefArg rcvr, RefArg obj);
extern "C" Ref		FGetBinaryStoredSize(RefArg rcvr, RefArg obj);

#endif	/* __LARGEBINARIES_H */
