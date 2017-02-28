/*
	File:		EndpointEvents.cc

	Contains:	Communications code.

	Written by:	Newton Research Group, 2008.
*/

#include "Transport.h"
#include "EndpointEvents.h"
#include "Endpoint.h"
#include "OSErrors.h"

#include "AppWorld.h"
#define gCommWorld static_cast<CAppWorld*>(GetGlobals())


/*------------------------------------------------------------------------------
	C E n d p o i n t E v e n t
------------------------------------------------------------------------------*/

CEndpointEvent::CEndpointEvent()
{
	fEventId = kEndpointEventId;
	f08 = 0;
	f0C = 0;
	f10 = 0;
	f14 = 0;
	f18 = GetGlobalTime();
}


CEndpointEvent::CEndpointEvent(int inArg1, VAddr inArg2, int inArg3)
{
	fEventId = kEndpointEventId;
	f08 = inArg1;
	f0C = inArg2;
	f10 = inArg3;
	f14 = 0;
	f18 = GetGlobalTime();
}


/*------------------------------------------------------------------------------
	C E n d p o i n t E v e n t   S u b C l a s s e s
------------------------------------------------------------------------------*/

CDefaultEvent::CDefaultEvent()
{
	f20 = 0;
}


CBindCompleteEvent::CBindCompleteEvent()
{
	f20 = 0;
}

CBindCompleteEvent::CBindCompleteEvent(int inArg1, VAddr inArg2, int inArg3)
	:	CEndpointEvent(inArg1, inArg2, inArg3)
{
	f20 = 0;
}


CConnectCompleteEvent::CConnectCompleteEvent()
{
	f20 = 0;
	f24 = 0;
	f28 = 0;
	f2C = 0;
}

CConnectCompleteEvent::CConnectCompleteEvent(int inArg1, VAddr inArg2, int inArg3)
	:	CEndpointEvent(inArg1, inArg2, inArg3)
{
	f20 = 0;
	f24 = 0;
	f28 = 0;
	f2C = 0;
}


CGetProtAddrCompleteEvent::CGetProtAddrCompleteEvent(int inArg1, VAddr inArg2)
	:	CEndpointEvent(inArg1, inArg2, -3)
{
	f20 = 0;
	f24 = 0;
}


COptMgmtCompleteEvent::COptMgmtCompleteEvent()
{
	f20 = 0;
}

COptMgmtCompleteEvent::COptMgmtCompleteEvent(int inArg1, VAddr inArg2)
	:	CEndpointEvent(inArg1, inArg2, -4)
{
	f20 = 0;
}


CSndCompleteEvent::CSndCompleteEvent()
{
	f20 = 0;
	f24 = 0;
	f28 = 0;
	f2C = 0;
}


CRcvCompleteEvent::CRcvCompleteEvent()
{
	f20 = 0;
	f24 = 0;
	f28 = 0;
	f2C = 0;
	f30 = 0;
}


CDisconnectEvent::CDisconnectEvent()
{
	f20 = 0;
	f24 = 0;
	f28 = 0;
}

CDisconnectEvent::CDisconnectEvent(int inArg1, VAddr inArg2)
	:	CEndpointEvent(inArg1, inArg2, 2)
{
	f20 = 0;
	f24 = 0;
	f28 = 0;
}


#pragma mark -

/*------------------------------------------------------------------------------
	C E n d p o i n t T i m e r
------------------------------------------------------------------------------*/

CEndpointTimer::CEndpointTimer(CEndpointEventHandler * inEventHandler, CTimerQueue * inQueue, ULong inRefCon)
	:	CTimerElement(inQueue, inRefCon)
{
	fEventHandler = inEventHandler;
}


void
CEndpointTimer::timeout(void)
{
	fEventHandler->timeout(this);
}

#pragma mark -

/*------------------------------------------------------------------------------
	C E n d p o i n t E v e n t H a n d l e r
------------------------------------------------------------------------------*/

CEndpointEventHandler::CEndpointEventHandler(CEndpoint * inEndpoint, bool inArg2)
	:	CEventHandler()
{
	f14 = inEndpoint;
	f28 = false;
	f29 = false;
	f2A = inArg2;
}


CEndpointEventHandler::~CEndpointEventHandler()
{ }


NewtonErr
CEndpointEventHandler::init(ObjectId inPortId, EventId inEventId, EventClass inEventClass)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CEventHandler::init(inEventId, inEventClass))
		f18 = inPortId;
		err = f20.init();
	}
	XENDTRY;
	return err;
}


NewtonErr
CEndpointEventHandler::addToAppWorld(void)
{
	return -1;
}


NewtonErr
CEndpointEventHandler::removeFromAppWorld(void)
{
	return -1;
}


NewtonErr
CEndpointEventHandler::addTimer(Timeout inDelta, ULong inRefCon)
{
	NewtonErr err = noErr;

	XTRY
	{
		if (inDelta)
		{
			CEndpointTimer * timer = new CEndpointTimer(this, gCommWorld->fTimers, inRefCon);
			XFAILIF(timer == NULL, err = MemError();)
			XFAILNOT(timer->prime(inDelta), delete timer;)
		}
	}
	XENDTRY;

	return err;
}


void
CEndpointEventHandler::killTimer(ULong inRefCon)
{
	CTimerElement * timer;
	if ((timer = gCommWorld->fTimers->cancel(inRefCon)) != NULL)
		delete timer;
}


bool
CEndpointEventHandler::timeout(CEndpointTimer * inTimer)
{
	f14->timeout(inTimer->getRefCon());
	return inTimer != NULL;	// sic
}


bool
CEndpointEventHandler::useForks(bool inDoIt)
{
	// we always use forks
	return true;
}


NewtonErr
CEndpointEventHandler::block(ULong inArg)
{
	NewtonErr err;
	if (f28)
		return TALRDYSYNC;
	f28 = true;
	err = f20.block(inArg);
	f28 = false;
	if (err == noErr)
	{
		if (f29)
		{
			f29 = 0;
			err = kOSErrCallAborted;
		}
	}
	return err;
}


void
CEndpointEventHandler::unblock(void)
{
	if (f28)
		f20.unblock();
}


NewtonErr
CEndpointEventHandler::callService(ULong inMsgType, CUAsyncMessage * ioMessage, CEvent * inRequest, size_t inReqSize, CEvent * inReply, size_t inReplySize, Timeout inTimeout, ULong inTimerName, bool inSync)
{
	NewtonErr err, err2;
	size_t replySize;

	XTRY
	{
		if (inSync)
		{
			XFAIL(err = gCommWorld->fork(NULL))
			XFAIL(err = gCommWorld->releaseMutex())
			err = f18.sendRPC(&replySize, inRequest, inReqSize, inReply, inReplySize, inTimeout, inMsgType);
			err2 = gCommWorld->acquireMutex();
			if (err == noErr && err2 != noErr)
				err = err2;
		}
		else
		{
			XFAIL(err = ioMessage->setCollectorPort((ObjectId)*gCommWorld->getMyPort()))
			XFAIL(err = addTimer(inTimeout, inTimerName))
			XFAILIF(err = SendRPC(this, &f18, ioMessage, inRequest, inReqSize, inReply, inReplySize, 0, NULL, inMsgType, false), killTimer(inTimerName);)
		}
	}
	XENDTRY;

	return err;
}


NewtonErr
CEndpointEventHandler::callServiceNoForks(ULong inMsgType, CUAsyncMessage * ioMessage, CEvent * inRequest, size_t inReqSize, CEvent * inReply, size_t inReplySize, Timeout inTimeout, ULong inTimerName, bool inSync)
{
	NewtonErr err, err2;
	size_t replySize;

	XTRY
	{
		if (inSync)
		{
			XFAIL(err = gCommWorld->releaseMutex())
			err = f18.sendRPC(&replySize, inRequest, inReqSize, inReply, inReplySize, inTimeout, inMsgType);
			err2 = gCommWorld->acquireMutex();
			if (err == noErr && err2 != noErr)
				err = err2;
		}
		else
		{
			XFAIL(err = ioMessage->setCollectorPort((ObjectId)*gCommWorld->getMyPort()))
			XFAIL(err = addTimer(inTimeout, inTimerName))
			XFAILIF(err = SendRPC(this, &f18, ioMessage, inRequest, inReqSize, inReply, inReplySize, 0, NULL, inMsgType, false), killTimer(inTimerName);)
		}
	}
	XENDTRY;

	return err;
}


void
CEndpointEventHandler::callService(ULong inMsgType, CUAsyncMessage * ioMessage, CEvent * inRequest, size_t inReqSize, CEvent * inReply, size_t inReplySize, unsigned long, bool inSync)
{ }


void
CEndpointEventHandler::callServiceBlocking(ULong inMsgType, CEvent * inRequest, size_t inReqSize, CEvent * inReply, size_t inReplySize, unsigned long)
{ }


void
CEndpointEventHandler::doEventLoop(unsigned long)
{ }


void
CEndpointEventHandler::terminateEventLoop(void)
{ }


void
CEndpointEventHandler::handleAborts(bool)
{ }


void
CEndpointEventHandler::abort(void)
{
	f29 = true;
	f14->abort();
	if (f28)
	{
		f20.unblock();
		f28 = false;
	}
}


bool
CEndpointEventHandler::testEvent(CEvent * inEvent)
{
	return (VAddr)this == ((CEndpointEvent*)inEvent)->f0C;
}


void
CEndpointEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	if (f2A && ((CEndpointEvent*)inEvent)->f08 == 'abrt')
		abort();
	else
		f14->handleEvent(gCommWorld->eventGetMsgType(), inEvent, *inSize);
	if (inToken && inToken->getReplyId() != 0)
		gCommWorld->eventReplyImmed();
}


void
CEndpointEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	ULong size = *inSize;
	f14->handleComplete(inToken, &size, inEvent);
}


void
CEndpointEventHandler::idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{ }


ObjectId
CEndpointEventHandler::getPrivatePortId(void)
{
	return 0;
}


ObjectId
CEndpointEventHandler::getServicePortId(void)
{
	return f18;
}


ObjectId
CEndpointEventHandler::getSyncPortId(void)
{
	return 0;
}
