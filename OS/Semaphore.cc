/*
	File:		Semaphore.cc

	Contains:	Semaphore implementation.

	Written by:	Newton Research Group.
*/

#include "Semaphore.h"
#include "KernelSemaphore.h"
#include "OSErrors.h"

#include <stdarg.h>

#if defined(forFramework)
NewtonErr SemaphoreOpGlue(ObjectId id, ObjectId opListId, SemFlags flags) { return noErr; }

/*------------------------------------------------------------------------------
	C U O b j e c t
	Stub.
------------------------------------------------------------------------------*/

CUObject::~CUObject()
{ destroyObject(); }

NewtonErr
CUObject::makeObject(ObjectTypes inObjectType, struct ObjectMessage * ioMsg, size_t inMsgSize)
{ return noErr; }

void
CUObject::copyObject(const ObjectId inId)
{ }

void
CUObject::destroyObject(void)
{ }

#endif


/*------------------------------------------------------------------------------
	U t i l i t i e s
------------------------------------------------------------------------------*/

NewtonErr
SemGroupSetRefCon(ObjectId inSemGroupId, void * inRefCon)
{
#if !defined(forFramework)
	if (IsSuperMode())
	{
		CSemaphoreGroup *	semGroup = (CSemaphoreGroup *)IdToObj(kSemGroupType, inSemGroupId);
		if (semGroup == NULL)
			return kOSErrBadSemaphoreGroupId;

		semGroup->setRefCon(inRefCon);
		return noErr;
	}

	else
		return GenericSWI(kSemGroupSetRefCon, inSemGroupId, inRefCon);
#else
	return noErr;
#endif
}


NewtonErr
SemGroupGetRefCon(ObjectId inSemGroupId, void ** outRefCon)
{
#if !defined(forFramework)
	if (IsSuperMode())
	{
		CSemaphoreGroup *	semGroup = (CSemaphoreGroup *)IdToObj(kSemGroupType, inSemGroupId);
		if (semGroup == NULL)
			return kOSErrBadSemaphoreGroupId;

		*outRefCon = semGroup->getRefCon();
		return noErr;
	}

	else
		return GenericWithReturnSWI(kSemGroupGetRefCon, inSemGroupId, 0, 0, (OpaqueRef *)outRefCon, 0, 0);
#else
	return noErr;
#endif
}

#pragma mark -

/*----------------------------------------------------------------------
	C U S e m a p h o r e O p L i s t
----------------------------------------------------------------------*/

NewtonErr
CUSemaphoreOpList::init(ArrayIndex inNumOfOps, ...)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(inNumOfOps == 0, err = kOSErrBadParameters;)

		size_t				msgSize = MSG_SIZE(1 + inNumOfOps);
		ObjectMessage *	msg = (ObjectMessage *)NewPtr(msgSize);
		XFAILIF(msg == NULL, err = kOSErrCouldNotCreateObject;)

		va_list	args;
		va_start(args, inNumOfOps);
		for (ArrayIndex i = 0; i < inNumOfOps; ++i)
			msg->request.semList.ops[i] = va_arg(args, SemOp);
		va_end(args);

		msg->request.semList.opCount = inNumOfOps;

#if !defined(forFramework)
		err = makeObject(kObjectSemList, msg, msgSize);
#endif
		FreePtr((Ptr)msg);
	}
	XENDTRY;
	return err;
}

#pragma mark -

/*----------------------------------------------------------------------
	C U S e m a p h o r e G r o u p
----------------------------------------------------------------------*/

NewtonErr
CUSemaphoreGroup::init(ULong inSize)
{
	ObjectMessage	msg;

	msg.request.semGroup.size = inSize;
#if !defined(forFramework)
	return makeObject(kObjectSemGroup, &msg, MSG_SIZE(1));
#else
	return noErr;
#endif
}


#pragma mark -

/*----------------------------------------------------------------------
	C U R d W r S e m a p h o r e
----------------------------------------------------------------------*/

CUSemaphoreOpList	CURdWrSemaphore::fAcquireWrOp;
CUSemaphoreOpList	CURdWrSemaphore::fReleaseWrOp;
CUSemaphoreOpList	CURdWrSemaphore::fAcquireRdOp;
CUSemaphoreOpList	CURdWrSemaphore::fReleaseRdOp;


NewtonErr
CURdWrSemaphore::staticInit(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fAcquireWrOp.init(3, MAKESEMLISTITEM(0,0), MAKESEMLISTITEM(0,1), MAKESEMLISTITEM(1,0)))
		XFAIL(err = fReleaseWrOp.init(1, MAKESEMLISTITEM(0,-1)))
		XFAIL(err = fAcquireRdOp.init(2, MAKESEMLISTITEM(0,0), MAKESEMLISTITEM(1,1)))
		err = fReleaseRdOp.init(1, MAKESEMLISTITEM(1,-1));
	}
	XENDTRY;
	return err;
}


NewtonErr
CURdWrSemaphore::init(void)
{
	return CUSemaphoreGroup::init(2);
}

#pragma mark -

/*----------------------------------------------------------------------
	C U L o c k i n g S e m a p h o r e
----------------------------------------------------------------------*/

CUSemaphoreOpList	CULockingSemaphore::fAcquireOp;
CUSemaphoreOpList	CULockingSemaphore::fReleaseOp;


CULockingSemaphore::CULockingSemaphore()
{
	CUSemaphoreGroup::getRefCon((void**)&fRefCon);
}


CULockingSemaphore::~CULockingSemaphore()
{
	if (fObjectCreatedByUs && fRefCon != NULL)
		FreePtr((Ptr)fRefCon);
}


NewtonErr
CULockingSemaphore::staticInit(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fAcquireOp.init(2, MAKESEMLISTITEM(0,0), MAKESEMLISTITEM(0,1)))
		err = fReleaseOp.init(1, MAKESEMLISTITEM(0,-1));
	}
	XENDTRY;
	return err;
}


NewtonErr
CULockingSemaphore::init(void)
{
	NewtonErr err;
	XTRY
	{
		fRefCon = (ULong *)NewPtr(sizeof(ULong));
		XFAILIF(fRefCon == NULL, err = kOSErrNoMemory;)
		*fRefCon = 0;
		XFAILIF(err = CUSemaphoreGroup::init(1), FreePtr((Ptr)fRefCon); fRefCon = NULL;)
		setRefCon(fRefCon);
	}
	XENDTRY;
	return err;
}


NewtonErr
CULockingSemaphore::copyObject(ObjectId inId)
{
#if !defined(forFramework)
	CUObject::copyObject(inId);
	return getRefCon((void**)&fRefCon);
#else
	return noErr;
#endif
}


NewtonErr
CULockingSemaphore::acquire(SemFlags inBlocking)
{
	NewtonErr	err = noErr;
	bool	wasBlocked = false;
/*
	while (Swap(fRefCon, gCurrentTaskId) != 0)
	{
		wasBlocked = true;
		if ((err = semOp(fReleaseOp, inBlocking)) != 0)
			break;
	}
*/
	if (wasBlocked)
		semOp(fAcquireOp, kNoWaitOnBlock);
		
	return err;
}


NewtonErr
CULockingSemaphore::release(void)
{
//	if (Swap(fRefCon, 0) == gCurrentTaskId)
		return noErr;
	return semOp(fAcquireOp, kNoWaitOnBlock);
}

/*----------------------------------------------------------------------
	C U M u t e x
----------------------------------------------------------------------*/

CUMutex::CUMutex()
{
	f0C = 0;
	f10 = 0;
}

