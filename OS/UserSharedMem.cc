/*
	File:		UserSharedMem.cc

	Contains:	Shared memory objects.

	Written by:	Newton Research Group.
*/

#include "UserSharedMem.h"
#include "KernelObjects.h"
#include "UserPorts.h"


/*------------------------------------------------------------------------------
	U S h a r e d M e m
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialize a new shared memory object.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUSharedMem::init()
{
	ObjectMessage	msg;
	return makeObject(kObjectSharedMem, &msg, MSG_SIZE(0));
}


NewtonErr
CUSharedMem::setBuffer(void * inBuffer, size_t inSize, ULong inPermissions)
{
	return SMemSetBufferSWI(fId, inBuffer, inSize, inPermissions);
}


NewtonErr
CUSharedMem::getSize(size_t * outSize, void ** outBuffer)
{
	return SMemGetSizeSWI(fId, outSize, outBuffer, NULL);
}


NewtonErr
CUSharedMem::copyToShared(void * inBuffer, size_t inSize, ULong inOffset, CUMsgToken * inToken)
{
	return SMemCopyToSharedSWI(fId, inBuffer, inSize, inOffset, inToken ? inToken->getMsgId() : kNoId, inToken ? inToken->getSignature() : 0);
}


NewtonErr
CUSharedMem::copyFromShared(size_t * outSize, void * outBuffer, size_t inSize, ULong inOffset, CUMsgToken * inToken)
{
	return SMemCopyFromSharedSWI(fId, outBuffer, inSize, inOffset, inToken ? inToken->getMsgId() : kNoId, inToken ? inToken->getSignature() : 0, outSize);
}

#pragma mark -
/*------------------------------------------------------------------------------
	U S h a r e d M e m M s g
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Initialize a new shared memory message.
	Args:		--
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUSharedMemMsg::init()
{
	ObjectMessage	msg;
	return makeObject(kObjectSharedMemMsg, &msg, MSG_SIZE(0));
}


NewtonErr
CUSharedMemMsg::setTimerParms(Timeout inTimeout, CTime * inDelay)
{
#if defined(correct)
	return SMemMsgSetTimerParmsSWI(fId, inTimeout, inDelay->fTime.part.lo, inDelay->fTime.part.hi);
#else
	return SMemMsgSetTimerParmsSWI(fId, inTimeout, *inDelay);
#endif
}


NewtonErr
CUSharedMemMsg::setMsgAvailPort(ObjectId inAvailPortId)
{
	return SMemMsgSetMsgAvailPortSWI(fId, inAvailPortId);
}


NewtonErr
CUSharedMemMsg::getSenderTaskId(ObjectId * outSenderTaskId)
{
	return SMemMsgGetSenderTaskIdSWI(fId, outSenderTaskId);
}


NewtonErr
CUSharedMemMsg::getSize(size_t * outSize, void ** outBuffer, OpaqueRef * outRefCon)
{
	return SMemGetSizeSWI(fId, outSize, outBuffer, outRefCon);
}


NewtonErr
CUSharedMemMsg::setUserRefCon(OpaqueRef inRefCon)
{
	return SMemMsgSetUserRefConSWI(fId, inRefCon);
}


NewtonErr
CUSharedMemMsg::getUserRefCon(OpaqueRef * outRefCon)
{
	return SMemMsgGetUserRefConSWI(fId, outRefCon);
}
