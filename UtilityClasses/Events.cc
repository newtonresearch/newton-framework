/*
	File:		Events.cc

	Contains:	Implementation of main interface calls.

	Written by:	Newton Research Group.
*/

#include "Events.h"
#include "EventHandler.h"
#include "AppWorld.h"


/*--------------------------------------------------------------------------------
	C E v e n t C o m p a r e r
	fItem points to a CEvent.
--------------------------------------------------------------------------------*/

CEventComparer::CEventComparer()
{ }


CompareResult
CEventComparer::testItem(const void * inItem) const
{
	CEventHandler *	handler = (CEventHandler *)inItem;
	// compare eventClass|eventId
	if (*(int64_t *)&handler->fEventClass > *(int64_t *)&((CEvent *)fItem)->fEventClass)
		return kItemGreaterThanCriteria;
	if (*(int64_t *)&handler->fEventClass < *(int64_t *)&((CEvent *)fItem)->fEventClass)
		return kItemLessThanCriteria;
	return kItemEqualCriteria;
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C E v e n t I d l e T i m e r
--------------------------------------------------------------------------------*/

CEventIdleTimer::CEventIdleTimer(CTimerQueue * inQ, ULong inRefCon, CEventHandler * inHandler, Timeout inIdle)
	:	CTimerElement(inQ, inRefCon), fHandler(inHandler), fTimeout(inIdle)
{ }


void
CEventIdleTimer::timeout(void)
{
	CTimerEvent evt;
	size_t size = sizeof(CTimerEvent);
	evt.fTimer = this;
	evt.fTimerRefCon = getRefCon();
	
	fHandler->idleProc(NULL, &size, &evt);
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C E v e n t H a n d l e r
--------------------------------------------------------------------------------*/

CEventHandler::CEventHandler()
	:	fNext(NULL), fEventClass(0), fEventId(0), fIdler(NULL)
{ }


CEventHandler::~CEventHandler()
{
	if (fIdler)
		delete fIdler;
	if (fEventClass && fEventId)
		gAppWorld->eventRemoveHandler(this);
}


NewtonErr
CEventHandler::init(EventId inEventId, EventClass inEventClass)
{
	fEventId = inEventId;
	fEventClass = inEventClass;
	gAppWorld->eventInstallHandler(this);
	return noErr;
}


void
CEventHandler::setReply(ULong inReply)
{
	gAppWorld->eventSetReply(inReply);
}


void
CEventHandler::setReply(size_t inSize, CEvent * inEvent)
{
	gAppWorld->eventSetReply(inSize, inEvent);
}


void
CEventHandler::setReply(CUMsgToken * inToken)
{
	gAppWorld->eventSetReply(inToken);
}


void
CEventHandler::setReply(CUMsgToken * inToken, size_t inSize, CEvent * inEvent)
{
	gAppWorld->eventSetReply(inToken, inSize, inEvent);
}


void
CEventHandler::deferReply(void)
{
	gAppWorld->eventDeferReply();
}


NewtonErr
CEventHandler::replyImmed(void)
{
	return gAppWorld->eventReplyImmed();
}


bool
CEventHandler::testEvent(CEvent * inEvent)
{
	return true;
}


void
CEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{	/* this really does nothing */	}


void
CEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{	/* this really does nothing */	}


void
CEventHandler::idleProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{	/* this really does nothing */	}


NewtonErr
CEventHandler::initIdler(Timeout inIdle, ULong inRefCon, bool inStart)
{
	NewtonErr err = noErr;
	XTRY
	{
		fIdler = new CEventIdleTimer(gAppWorld->fTimers, inRefCon, this, inIdle);
		XFAILIF(fIdler == NULL, err = MemError();)
		XFAILIF(inStart && !fIdler->start(), err = -1;)
	}
	XENDTRY;
	return err;
}


NewtonErr
CEventHandler::initIdler(ULong inIdleAmount, TimeUnits inIdleUnits, ULong inRefCon, bool inStart)
{
	return initIdler(inIdleAmount * inIdleUnits, inRefCon, inStart);
}


NewtonErr
CEventHandler::startIdle(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILNOT(fIdler && fIdler->start(), err = -1;)
	}
	XENDTRY;
	return err;
}


NewtonErr
CEventHandler::stopIdle(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILNOT(fIdler && fIdler->stop(), err = -1;)
	}
	XENDTRY;
	return err;
}


NewtonErr
CEventHandler::resetIdle(void)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILNOT(fIdler && fIdler->primeme(), err = -1;)
	}
	XENDTRY;
	return err;
}


NewtonErr
CEventHandler::resetIdle(Timeout inIdle)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILNOT(fIdler && fIdler->prime(inIdle), err = -1;)
	}
	XENDTRY;
	return err;
}


NewtonErr
CEventHandler::resetIdle(ULong inIdleAmount, TimeUnits inIdleUnits)
{
	NewtonErr err = resetIdle(inIdleAmount * inIdleUnits);
	gAppWorld->getMyPort()->reset(0, kPortFlags_Timeout);
	return err;
}


CEventHandler *
CEventHandler::addHandler(CEventHandler * inHeadOfChain)
{
	fNext = inHeadOfChain;
	return this;
}


CEventHandler *
CEventHandler::removeHandler(CEventHandler * inHeadOfChain)
{
	CEventHandlerIterator	iter(inHeadOfChain);
	CEventHandler *			theHandler;
	if ((theHandler = iter.firstHandler()) == this)
		return getNextHandler();
	if (iter.currentHandler())
		do
		{
			if (theHandler->fNext == this)
			{
				theHandler->fNext = theHandler->fNext->fNext;
				break;
			}
		} while ((theHandler = iter.nextHandler()) != NULL);
	return inHeadOfChain;
}


NewtonErr
CEventHandler::doEvent(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	NewtonErr			err = noErr;
	CEventHandler *	handler = this;
	if (handler->testEvent(inEvent))
		handler->eventHandlerProc(inToken, inSize, inEvent);
	else if ((handler = handler->getNextHandler()))		// intentional assignment
		handler->doEvent(inToken, inSize, inEvent);
	else
		err = -14100;
	return err;
}


void
CEventHandler::doComplete(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	CEventHandler *	handler = this;
	do
	{
		if (handler->testEvent(inEvent))										// vt04
			handler->eventCompletionProc(inToken, inSize, inEvent);	// vt0C
	} while ((handler = handler->getNextHandler()));		// intentional assignment
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C S y s t e m E v e n t H a n d l e r
--------------------------------------------------------------------------------*/
#include "SystemEvents.h"

CSystemEventHandler::CSystemEventHandler()
{
	fInited = false;
}


NewtonErr
CSystemEventHandler::init(SystemEvent inSystemEvent, ULong inSendFilter)
{
	NewtonErr err;
	CSystemEvent sysEvt(inSystemEvent);
	err = sysEvt.registerForSystemEvent(*gAppWorld->getMyPort(), inSendFilter);
	if (err == noErr && !fInited)
	{
		CEventHandler::init(kSystemEventId, kNewtEventClass);
		fInited = true;
	}
	return noErr;
}


void
CSystemEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
	anySystemEvents(inEvent);
	switch (inEvent->fEventId)
	{
	case kSysEvent_AppAlive:
		appAlive(inEvent);
		break;
	case kSysEvent_NewICCard:
		newCard(inEvent);
		break;
	case kSysEvent_DeviceNotification:
		deviceNotification(inEvent);
		break;
	case kSysEvent_PowerOn:
		powerOn(inEvent);
		break;
	case kSysEvent_PowerOff:
		powerOff(inEvent);
		break;
	case kSysEvent_PowerOffPending:
		powerOffPending(inEvent);
		break;
	}
}


void
CSystemEventHandler::anySystemEvents(CEvent * inEvent)
{ }


void
CSystemEventHandler::appAlive(CEvent * inEvent)
{
	printf("CSystemEventHandler::appAlive\n");
}


void
CSystemEventHandler::newCard(CEvent * inEvent)
{ }


void
CSystemEventHandler::deviceNotification(CEvent * inEvent)
{ }


void
CSystemEventHandler::powerOn(CEvent * inEvent)
{ }


void
CSystemEventHandler::powerOff(CEvent * inEvent)
{ }


void
CSystemEventHandler::powerOffPending(CEvent * inEvent)
{ }


#pragma mark -
/*--------------------------------------------------------------------------------
	C E v e n t H a n d l e r C o m p a r e r
	fItem points to a CEventHandler.
--------------------------------------------------------------------------------*/

CEventHandlerComparer::CEventHandlerComparer()
{ }


CompareResult
CEventHandlerComparer::testItem(const void * inItem) const
{
	CEventHandler *	thisHandler = (CEventHandler *)fItem;
	CEventHandler *	otherHandler = (CEventHandler *)inItem;
	// compare eventClass|eventId
	if (*(int64_t *)&otherHandler->fEventClass > *(int64_t *)&thisHandler->fEventClass)
		return kItemGreaterThanCriteria;
	if (*(int64_t *)&otherHandler->fEventClass < *(int64_t *)&thisHandler->fEventClass)
		return kItemLessThanCriteria;
	return kItemEqualCriteria;
}


#pragma mark -
/*--------------------------------------------------------------------------------
	C E v e n t H a n d l e r I t e r a t o r
--------------------------------------------------------------------------------*/

CEventHandlerIterator::CEventHandlerIterator(CEventHandler* inHead)
{
	fCurrentHandler = fFirstHandler = inHead;
	fNextHandler = (fCurrentHandler) ? fCurrentHandler->getNextHandler() : NULL;
}


void
CEventHandlerIterator::reset(void)
{
	fCurrentHandler = fFirstHandler;
	fNextHandler = (fCurrentHandler) ? fCurrentHandler->getNextHandler() : NULL;
}


void
CEventHandlerIterator::advance(void)
{
	fCurrentHandler = fNextHandler;
	if (fCurrentHandler)
		fNextHandler = fCurrentHandler->getNextHandler();
}
