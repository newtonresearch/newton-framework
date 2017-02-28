/*
	File:		UserObjects.cc

	Contains:	User Object implementation.

	Written by:	Newton Research Group.
*/

#include "UserObjects.h"
#include "KernelObjects.h"
#include "UserMonitor.h"
#include "OSErrors.h"


CUMonitor *	gUObjectMgrMonitor;	// 0C104F04


/*------------------------------------------------------------------------------
	C U O b j e c t
------------------------------------------------------------------------------*/

/*------------------------------------------------------------------------------
	Destructor.
	Ask the object manager monitor to destroy the object.
------------------------------------------------------------------------------*/

CUObject::~CUObject()
{
	destroyObject();
}


/*------------------------------------------------------------------------------
	Make an object.
	Ask the object manager monitor to make an object of the given type.
	We become that object.
	Args:		inObjectType		its type
				ioMsg					a parameterized message struct
				inMsgSize			size of message struct actually used
	Return:	error code
------------------------------------------------------------------------------*/

NewtonErr
CUObject::makeObject(ObjectTypes inObjectType, struct ObjectMessage * ioMsg, size_t inMsgSize)
{
	NewtonErr err;

	XTRY
	{
		XFAILIF(fObjectCreatedByUs, err = kOSErrObjectAlreadyInitialized;)

		ioMsg->target = (ObjectId) inObjectType;	// overloading!
		ioMsg->size = inMsgSize;
		XFAILIF(err = gUObjectMgrMonitor->invokeRoutine(kCreate, ioMsg), fId = kNoId;)

		fId = *(ObjectId*)ioMsg;	// yes, result is really element 0
		fObjectCreatedByUs = true;
	}
	XENDTRY;

	return err;
}


/*------------------------------------------------------------------------------
	Become a copy of another object.
	Args:		inId					object to copy
	Return:	--
------------------------------------------------------------------------------*/

void
CUObject::copyObject(const ObjectId inId)
{
	if ((ObjectId)*this != inId)
	{
		destroyObject();
		fObjectCreatedByUs = false;
		fId = inId;
	}
}


/*------------------------------------------------------------------------------
	Ask the object manager monitor to destroy us.
	Args:		--
	Return:	--
------------------------------------------------------------------------------*/

void
CUObject::destroyObject(void)
{
	if ((ObjectId)*this != kNoId)
	{
		if (fObjectCreatedByUs)
		{
			ObjectMessage	msg;

			msg.target = fId;
			msg.size = MSG_SIZE(0);
			gUObjectMgrMonitor->invokeRoutine(kDestroy, &msg);
		}
		fObjectCreatedByUs = false;
		fId = kNoId;
	}
}
