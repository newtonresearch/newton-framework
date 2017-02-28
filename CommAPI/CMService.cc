/*
	File:		CMService.cc

	Contains:	Communications Manager service protocol.

	Written by:	Newton Research Group, 2015.
*/

#include "CMWorld.h"
#include "NameServer.h"


NewtonErr
ServiceToPort(ULong serviceId, CUPort* port, ObjectId taskId)
{
	NewtonErr err;
	XTRY
	{
		MAKE_ID_STR(serviceId,type);
		char name[16];		// is actually the task id
		sprintf(name, "%d", taskId);

		CUNameServer ns;
		OpaqueRef thing, spec;
		XFAIL(err = ns.lookup(name, type, &thing, &spec))

		*port = (ObjectId)thing;
	}
	XENDTRY;
	return err;
}


/* -------------------------------------------------------------------------------
	C A s y n c S e r v i c e M e s s a g e
------------------------------------------------------------------------------- */

CAsyncServiceMessage::CAsyncServiceMessage()
	:	fService(NULL), fMessage(NULL), fReply(NULL)
{ }


CAsyncServiceMessage::~CAsyncServiceMessage()
{
	if (fReply)
		FreePtr((Ptr)fReply);	// was delete
	gCommWorld->serviceMessages()->remove(this);
}


NewtonErr
CAsyncServiceMessage::init(CCMService * inService)
{
	NewtonErr err;
	fAsyncMessage.init();
	fAsyncMessage.setCollectorPort(*gCommWorld->getMyPort());
	err = fAsyncMessage.setUserRefCon((OpaqueRef)gCommWorld->eventHandler());
	fService = inService;
	gCommWorld->serviceMessages()->insert(this);
	return err;
}


NewtonErr
CAsyncServiceMessage::send(CUPort * inDestination, void * inMessage, size_t inMessageSize, void * inReply, size_t inReplySize, ULong inMessageType)
{
	fMessage = inMessage;
	fReply = inReply;
	return inDestination->sendRPC(&fAsyncMessage, inMessage, inMessageSize, inReply, inReplySize, kNoTimeout, NULL, inMessageType);
}


bool
CAsyncServiceMessage::match(CUMsgToken * inToken)
{
	return inToken->getMsgId() == fAsyncMessage.getMsgId();
}
