/*
	File:		MemObject.cc

	Contains:	Interface to CMemObject.

	Written by:	Newton Research Group.
*/

#include "MemObject.h"


/*--------------------------------------------------------------------------------
	C M e m O b j e c t
--------------------------------------------------------------------------------*/

CMemObject::CMemObject()
{
	fFlags.internal = true;
	fFlags.createdMemory=
	fFlags.shared =
	fFlags.useToken = false;
	fSharedMemoryObject = 0;
	fSize = 0;
	fBuffer = NULL;
}


CMemObject::~CMemObject()
{
	destroy();
}

NewtonErr
CMemObject::init(size_t inSize, Ptr inBuffer, bool inMakeShared, ULong inPermissions)
{
	NewtonErr	err = noErr;

	fSize = inSize;
	fBuffer = inBuffer;
	fFlags.internal = true;
	if (inMakeShared)
		err = makeShared(inPermissions);
	return err;
}

/* this isnÕt in Hammer
NewtonErr
CMemObject::init(size_t inSize, bool inMakeShared, ULong inPermissions)
{
}
*/

long
CMemObject::make(ObjectId inSharedObjectId, CUMsgToken * inMsgToken)
{
	CUSharedMem	huh(inSharedObjectId);	// doesnÕt appear to be used
	
	fSharedMemoryObject = inSharedObjectId;
	fFlags.internal = false;
	fFlags.shared = true;
	if (inMsgToken)
	{
		fSharedMemoryToken = *inMsgToken;
		fFlags.useToken = true;
	}
	return fSharedMemoryObject.getSize(&fSize);
}


NewtonErr
CMemObject::makeShared(ULong inPermissions)
{
	NewtonErr	err = noErr;

	if (!fFlags.shared)
		err = fSharedMemoryObject.init();
	if (err == noErr)
		err = fSharedMemoryObject.setBuffer(fBuffer, fSize, inPermissions);
	if (err == noErr)
		fFlags.shared = true;
	return err;
}


void
CMemObject::destroy(void)
{
	if (fFlags.internal && fFlags.createdMemory)
		delete (Ptr) fBuffer;
	fBuffer = NULL;
}



ObjectId
CMemObject::getId(void)
{
	return (ObjectId) fSharedMemoryObject;
}


NewtonErr
CMemObject::copyTo(Ptr inBuffer, size_t inSize, ULong inOffset)
{
	if (fFlags.internal)
	{
		memmove((Ptr) fBuffer + inOffset, inBuffer, inSize);
		return noErr;
	}
	return fSharedMemoryObject.copyToShared(inBuffer, inSize, inOffset, fFlags.useToken ? &fSharedMemoryToken : NULL);
}


NewtonErr
CMemObject::copyFrom(size_t * outSize, Ptr outBuffer, size_t inSize, ULong inOffset)
{
	if (fFlags.internal)
	{
		ULong	availableSize = fSize - inOffset;
		if (inSize > availableSize)
			inSize = availableSize;
		*outSize = inSize;
		memmove(outBuffer, (Ptr) fBuffer + inOffset, inSize);
		return noErr;
	}
	return fSharedMemoryObject.copyFromShared(outSize, outBuffer, inSize, inOffset, fFlags.useToken ? &fSharedMemoryToken : NULL);
}


void *
CMemObject::getBase(void)
{
	return fBuffer;	// not in Hammer so just guessing
}
