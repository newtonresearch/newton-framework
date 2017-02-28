/*
	File:		SystemEvents.cc

	Contains:	CSystemEvent implementation.

	Written by:	Newton Research Group.
*/

#include "SystemEvents.h"


/*--------------------------------------------------------------------------------
	C S y s t e m E v e n t
--------------------------------------------------------------------------------*/

CSystemEvent::CSystemEvent(SystemEvent inEvent)
{
	fEvent = inEvent;
	fSystemPort = GetPortSWI(kGetNameServerPort);
}


void
CSystemEvent::setEvent(SystemEvent inEvent)
{
	fEvent = inEvent;
}


NewtonErr
CSystemEvent::registerForSystemEvent(ObjectId inPortId, ULong inSendFilter, Timeout inTimeout)
{
	CNameServerReply	reply;
	CSysEventRequest	request;
	size_t				replySize;

	request.fCommand = kRegisterForSystemEvent;
	request.fTheEvent = fEvent;
	request.fSysEventObjId = inPortId;
	request.fSysEventTimeOut = inTimeout;
	request.fSysEventSendFilter = inSendFilter;

	return fSystemPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
}


NewtonErr
CSystemEvent::unregisterForSystemEvent(ObjectId inPortId)
{
	CNameServerReply	reply;
	CSysEventRequest	request;
	size_t				replySize;

	request.fCommand = kUnregisterForSystemEvent;
	request.fTheEvent = fEvent;
	request.fSysEventObjId = inPortId;

	return fSystemPort.sendRPC(&replySize, &request, sizeof(request), &reply, sizeof(reply));
}


#pragma mark -
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
	NewtonErr err;
	XTRY
	{
		XFAIL(err = fMsgToSend.setBuffer(inMessage, inMessageSize))

		size_t replySize;
		CSysEventRequest request;
		request.fCommand = kSendSystemEvent;
		request.fTheEvent = fEvent;
		request.fSysEventObjId = fMsgToSend;
		err = fSystemPort.sendRPC(&replySize, &request, sizeof(request), NULL, 0, kNoTimeout, 1);
	}
	XENDTRY;
	return err;
}


NewtonErr
CSendSystemEvent::sendSystemEvent(CUAsyncMessage * inAsyncMessage, void * inMessage, size_t inMessageSize, void * outReply, ULong outReplySize)
{
	NewtonErr err;
	XTRY
	{
		XFAILIF(inMessage == (void *)-1, err = kOSErrBadParameters;)	// well, obviously
		if (outReply == (void *)-1)
			;			 // we donÕt want a reply
		else
		{
			// get the shared reply mem
			CUSharedMem replyMem(inAsyncMessage->getReplyMemId());
			size_t replySize = MIN(inMessageSize, outReplySize);
			// point it to the reply buffer
			XFAIL(err = replyMem.setBuffer(outReply, replySize, kSMemReadWrite))
			if (replySize > 0)
				// the reply is the message
				memcpy(outReply, inMessage, replySize);
		}
		XFAIL(err = fMsgToSend.setBuffer(inMessage, inMessageSize))

		fMsgToNameServer.fCommand = kSendSystemEvent;
		fMsgToNameServer.fTheEvent = fEvent;
		fMsgToNameServer.fSysEventObjId = fMsgToSend;
		err = fSystemPort.sendRPC(inAsyncMessage, &fMsgToNameServer, sizeof(fMsgToNameServer), NULL, 0, kNoTimeout, NULL, 1);
	}
	XENDTRY;
	return err;
}
