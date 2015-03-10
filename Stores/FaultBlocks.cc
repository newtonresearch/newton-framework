/*
	File:		FaultBlocks.cc

	Contains:	Newton fault block interface.

	Written by:	Newton Research Group.
*/

#include "Objects.h"
#include "ObjHeader.h"
#include "Globals.h"
#include "Funcs.h"
#include "RefMemory.h"
#include "LargeBinaries.h"
#include "FaultBlocks.h"
#include "StoreStreamObjects.h"
#include "OSErrors.h"


/*------------------------------------------------------------------------------
	Create a fault block.
	A fault block is an array of class kFaultBlockClass with 4 elements:
	0	handler
	1	store wrapper
	2	id in store
	3	object
	Args:		inHandler
				inStoreWrapper
				inId
				inObj
	Return:	fault block object
------------------------------------------------------------------------------*/

Ref
MakeFaultBlock(RefArg inHandler, CStoreWrapper * inStoreWrapper, PSSId inId)
{
	RefVar	faultBlock(AllocateArray(RA(NILREF), 4));
	SetArraySlot(faultBlock, 0, inHandler);
	SetArraySlot(faultBlock, 1, (Ref) inStoreWrapper);
	SetArraySlot(faultBlock, 2, MAKEINT(inId));
	((FaultObject *)ObjectPtr(faultBlock))->objClass = kFaultBlockClass;
	gCached.ref = INVALIDPTRREF;
	return faultBlock;
}

Ref
MakeFaultBlock(RefArg inHandler, CStoreWrapper * inStoreWrapper, PSSId inId, RefArg inObj)
{
	RefVar	faultBlock(AllocateArray(RA(NILREF), 4));
	SetArraySlot(faultBlock, 0, inHandler);
	SetArraySlot(faultBlock, 1, (Ref) inStoreWrapper);
	SetArraySlot(faultBlock, 2, MAKEINT(inId));
	SetArraySlot(faultBlock, 3, inObj);
	((FaultObject *)ObjectPtr(faultBlock))->objClass = kFaultBlockClass;
	gCached.ref = INVALIDPTRREF;
	return faultBlock;
}


/*------------------------------------------------------------------------------
	Determine whether an object is a fault block.
	Args:		inObj			Ref of the object
	Return:	YES => is a fault block
------------------------------------------------------------------------------*/

bool
IsFaultBlock(Ref inObj)
{
	return ISPTR(inObj)
	&& ((FaultObject *)NoFaultObjectPtr(inObj))->objClass == kFaultBlockClass;
}


/*------------------------------------------------------------------------------
	Return a pointer to an object for a Ref.
	Load object from store if necessary.
	Args:		inObj				Ref whose pointer is required
				inTag				tag bits, already extracted
				inDoFaultCheck
	Return:	ObjHeader *		the object pointer
	Throw:	exFrames			if Ref is BadPackageRef
				exFrames			if Ref is not pointer object
------------------------------------------------------------------------------*/

static Ref FollowFaultBlock(RefArg inRcvr);

ObjHeader *
ObjectPtr1(Ref inObj, long inTag, bool inDoFaultCheck)
{
	ObjHeader * oPtr;

	if (inTag == kTagPointer)
	{
		gCached.ref = inObj;
		oPtr = PTR(ForwardReference(inObj));
		if (oPtr->flags & kObjSlotted
		&& ((FaultObject *)oPtr)->objClass == kFaultBlockClass)
		{
			gCached.ref = INVALIDPTRREF;
			if (NOTNIL(inObj = ((FaultObject *)oPtr)->object))
				return ObjectPtr(inObj);
			else if (inDoFaultCheck)
				return NULL;
			else
				return ObjectPtr(FollowFaultBlock(MAKEPTR(oPtr)));
		}
		gCached.ptr = oPtr;
		return oPtr;
	}

	else if (inObj == kBadPackageRef)
		ThrowErr(exFrames, kNSErrBadPackageRef);

	else
		ThrowExFramesWithBadValue(kNSErrObjectPointerOfNonPtr, inObj);

	return NULL;	// unreachable but keeps the compiler quiet
}


/*------------------------------------------------------------------------------
	Handle fault block object.
	Args:		inRcvr		fault block object
	Return:	Ref			of faulted object
	Throw:	exStore		if invalid fault block
------------------------------------------------------------------------------*/

Ref
FollowFaultBlock(RefArg inRcvr)
{
	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inRcvr);
	if (obj->store == INVALIDPTRREF)
		ThrowErr(exStore, kNSErrInvalidFaultBlock);
	if (obj->store == NILREF)
		return ForwardEntryMessage(inRcvr, SYMA(EntryAccess));

	Ref permObj = LoadPermObject((CStoreWrapper *)obj->store, RINT(obj->id), NULL);
	obj->object = permObj;
	return permObj;
}


/*------------------------------------------------------------------------------
	Send access message to fault object.
	Args:		inRcvr		fault block object
				inSymbol		message to send
	Return:	Ref			result of message
------------------------------------------------------------------------------*/

Ref
ForwardEntryMessage(RefArg inRcvr, RefArg inSymbol)
{
	RefVar args(MakeArray(1));
	ArrayObject * pArgs = (ArrayObject *)ObjectPtr(args);
	pArgs->slot[0] = inRcvr;

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inRcvr);
	return DoMessage(obj->handler, inSymbol, args);
}


/*------------------------------------------------------------------------------
	Send access message to fault object.
	Args:		inRcvr		fault block object
				inSymbol		message to send
				inArg			argument to send with message
	Return:	Ref			result of message
------------------------------------------------------------------------------*/

Ref
ForwardEntryMessage(RefArg inRcvr, RefArg inSymbol, RefArg inArg)
{
	RefVar args(MakeArray(2));
	ArrayObject * pArgs = (ArrayObject *)ObjectPtr(args);
	pArgs->slot[0] = inRcvr;
	pArgs->slot[1] = inArg;

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inRcvr);
	return DoMessage(obj->handler, inSymbol, args);
}

#pragma mark -


/*------------------------------------------------------------------------------
	Write fault block to store.
	Args:		inRcvr		fault block
	Return:	--
	Throw:	exStore		if invalid fault block
				OSErr			if store lock|unlock|abort fails
------------------------------------------------------------------------------*/

void
WriteFaultBlock(RefArg inRcvr)
{
	if (!IsFaultBlock(inRcvr))
		ThrowErr(exStore, kNSErrNotAFaultBlock);

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inRcvr);
	RefVar object(obj->object);
	if (NOTNIL(object))
	{
		CStoreWrapper * storeWrapper = (CStoreWrapper *)obj->store;
		PSSId id = RINT(obj->id);

		OSERRIF(storeWrapper->lockStore());
		newton_try
		{
			StorePermObject(object, storeWrapper, id, NULL, NULL);
		}
		newton_catch_all
		{
			OSERRIF(storeWrapper->abort());
		}
		end_try;
		OSERRIF(storeWrapper->unlockStore());
	}
}


/*------------------------------------------------------------------------------
	Invalidate a fault block.
	Args:		inRcvr		fault block
	Return:	--
	Throw:	exStore		if invalid fault block
				OSErr			if store lock|unlock|abort fails
------------------------------------------------------------------------------*/

void
InvalFaultBlock(RefArg inRcvr)
{
	if (!IsFaultBlock(inRcvr))
		ThrowErr(exStore, kNSErrNotAFaultBlock);

	FaultObject * obj = (FaultObject *)NoFaultObjectPtr(inRcvr);
	obj->store = INVALIDPTRREF;
	obj->object = NILREF;
}


/*------------------------------------------------------------------------------
	Uncache a fault block.
	Args:		inRcvr		fault block
	Return:	--
------------------------------------------------------------------------------*/

void
UncacheIfFaultBlock(RefArg inRcvr)
{
	if (IsFaultBlock(inRcvr))
		((FaultObject *)NoFaultObjectPtr(inRcvr))->object = NILREF;
}


/*------------------------------------------------------------------------------
	Load faulted object from store.
	Args:		inStoreWrapper		store object wrapper
				inId
				ioArray
	Return:	Ref			of faulted object
------------------------------------------------------------------------------*/

Ref
LoadPermObject(CStoreWrapper * inStoreWrapper, PSSId inId, CDynamicArray ** ioArray)
{
	Ref	obj;
	CStoreObjectReader reader(inStoreWrapper, inId, ioArray);

	newton_try
	{
		obj = reader.read();
	}
	cleanup
	{
		if (ioArray && *ioArray)
			(*ioArray)->~CDynamicArray();
		reader.~CStoreObjectReader();
	}
	end_try;

	return obj;
}


void
StorePermObject(RefArg inObj, CStoreWrapper * inStoreWrapper, PSSId & ioId, CDynamicArray * inLargeBinaries, bool * outArg5)
{
	NewtonErr	err;
	bool			isRO;

	if (inStoreWrapper == NULL)
		ThrowErr(exStore, kNSErrInvalidFaultBlock);
	if ((err = inStoreWrapper->store()->isReadOnly(&isRO)) != noErr)
		ThrowOSErr(err);
	if (isRO)
		ThrowErr(exStore, kStoreErrWriteProtected);

	CStoreObjectWriter writer(inObj, inStoreWrapper, ioId);

	newton_try
	{
		ioId = writer.write();
	}
	cleanup
	{
		writer.~CStoreObjectWriter();
	}
	end_try;

	if (outArg5 != NULL)
		*outArg5 = writer.hasDuplicateLargeBinary();
	if (inStoreWrapper->hasEphemerals())
		FinalizeLargeObjectWrites(inStoreWrapper, inLargeBinaries, writer.largeBinaries());
}


void
CopyPermObject(PSSId inId, CStoreWrapper * inFromStore, CStoreWrapper * inToStore)
{}


int
ZapLargeObject(CStoreWrapper * inStoreWrapper, PSSId inId, StoreRef, void*)
{
	DeleteLargeBinary(inStoreWrapper, inId);
	return 0;
}


void
ZapLargeBinaries(CStoreWrapper * inStoreWrapper, PSSId inId)
{
	CStoreObjectReader reader(inStoreWrapper, inId, NULL);

	newton_try
	{
		reader.eachLargeObjectDo(ZapLargeObject, NULL);
	}
	cleanup
	{
		reader.~CStoreObjectReader();
	}
	end_try;
}


void
DeletePermObject(CStoreWrapper * inStoreWrapper, PSSId inId)
{
	StoreObjectHeader root;
	OSERRIF(inStoreWrapper->store()->read(inId, 0, &root, sizeof(root)));
	// delete any large binaries within the object
	if (root.flags & 0x01)
		ZapLargeBinaries(inStoreWrapper, inId);
	// delete the object
	OSERRIF(inStoreWrapper->store()->deleteObject(inId));
	// delete any text associated with the object
	if (root.textBlockId)
		OSERRIF(inStoreWrapper->store()->deleteObject(root.textBlockId));
}

