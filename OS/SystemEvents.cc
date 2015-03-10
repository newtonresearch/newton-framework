/*
	File:		SystemEvents.cc

	Contains:	COSEvent implementation.

	Written by:	Newton Research Group.
*/

#include "SystemEvents.h"


/*--------------------------------------------------------------------------------
	C O S E v e n t
--------------------------------------------------------------------------------*/

COSEvent::COSEvent(SystemEvent inEvent)
{
	fEvent = inEvent;
	fNameServerPort = GetPortSWI(kGetNameServerPort);
}


void
COSEvent::setEvent(SystemEvent inEvent)
{
	fEvent = inEvent;
}


NewtonErr
COSEvent::registerForSystemEvent(ObjectId inPortId, ULong inSendFilter, Timeout inTimeout)
{
	CNameServerReply	reply;
	CSysEventRequest	request;
	size_t				theSize;

	request.fCommand = kRegisterForSystemEvent;
	request.fTheEvent = fEvent;
	request.fSysEventObjId = inPortId;
	request.fSysEventTimeOut = inTimeout;
	request.fSysEventSendFilter = inSendFilter;

	return fNameServerPort.sendRPC(&theSize, &request, sizeof(request), &reply, sizeof(reply));
}


NewtonErr
COSEvent::unregisterForSystemEvent(ObjectId inPortId)
{
	CNameServerReply	reply;
	CSysEventRequest	request;
	size_t				theSize;

	request.fCommand = kUnregisterForSystemEvent;
	request.fTheEvent = fEvent;
	request.fSysEventObjId = inPortId;

	return fNameServerPort.sendRPC(&theSize, &request, sizeof(request), &reply, sizeof(reply));
}


/*--------------------------------------------------------------------------------
	C S e n d S y s t e m E v e n t
--------------------------------------------------------------------------------*/

NewtonErr
CSendSystemEvent::init(void)
{
	return fMsgToSend.init();
}


NewtonErr
CSendSystemEvent::sendSystemEvent(void * inMessage, size_t inMessageSize)
{
	NewtonErr			err;
	CSysEventRequest	request;
	if ((err = fMsgToSend.setBuffer(inMessage, inMessageSize)) == noErr)
	{
		request.fCommand = kSendSystemEvent;
		request.fTheEvent = fEvent;
		// INCOMPLETE
		//err = sendRPC(&theSize, &request, sizeof(request), &reply, sizeof(reply));
	}
	return err;
}


NewtonErr
CSendSystemEvent::sendSystemEvent(CUAsyncMessage * inAsyncMessage, void * inMessage, size_t inMessageSize, void * outReply, ULong outReplySize)
{
	// INCOMPLETE
	return noErr;
}
