/*
	File:		CMWorld.cc

	Contains:	Communications Manager task world.

	Written by:	Newton Research Group, 2010.
*/

#include "CMWorld.h"

bool		gSCPDevicePackageBusy = false;		// 0C100B64


/* -----------------------------------------------------------------------------
	C C M W o r l d
	The Communications Manager World listens for the insertion of a device
	on the interconnect port (that’ll be serial/AppleTalk or keyboard) or
	on a PC card, and starts the comm service associated with that device.
----------------------------------------------------------------------------- */
#define gCommWorld static_cast<CCMWorld*>(GetGlobals())

CCMWorld::CCMWorld()
{
	fD8 = NULL;
	fDC = 0;
	fE0 = 0;
}


size_t
CCMWorld::getSizeOf(void) const
{
	return sizeof(CCMWorld);
}


NewtonErr
CCMWorld::mainConstructor(void)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CAppWorld::mainConstructor())

		XFAIL(err = f70.init(kCommManagerId))

		CCMSystemEventHandler * evtHandler = new CCMSystemEventHandler;
		XFAILIF(evtHandler == NULL, err = kOSErrNoMemory;)	// original doesn’t FAIL
		XFAIL(err = evtHandler->init(kSysEvent_PowerOn))
		XFAIL(err = evtHandler->init(kSysEvent_PowerOff))
		err = evtHandler->init(kSysEvent_AppAlive);
	}
	XENDTRY;
	return err;
}


void
CCMWorld::mainDestructor(void)
{
	CAppWorld::mainDestructor();
}


CAsyncServiceMessage *
CCMWorld::matchPendingServiceMessage(CUMsgToken * inMsg)
{
	CListIterator iter(&f90);
	CAsyncServiceMessage * item;
	for (item = (CAsyncServiceMessage *)iter.firstItem(); iter.more(); item = (CAsyncServiceMessage *)iter.nextItem())
	{
		if (item->match(inMsg))
			return item;
	}
	return NULL;
}


void *
CCMWorld::matchPendingStartInfo(CCMService * inService)
{
	CListIterator iter(&fA8);
	void * item;
	for (item = (void *)iter.firstItem(); iter.more(); item = (void *)iter.nextItem())
	{
#if 0
		if (item->f18 == inService)
			return item;
#endif
	}
	return NULL;
}


void
CCMWorld::setDevice(CConnectedDevice * inDevice)
{
	fDevice = *inDevice;
	fDevice.fLastConnectTime = GetGlobalTime();
}


void
CCMWorld::setLastPackage(ULong inArg1, ULong inArg2)
{
	fDC = inArg2;
	fE0 = inArg1;
}


NewtonErr
CCMWorld::SCPCheck(ULong inArg1)
{
	if (gSCPDevicePackageBusy)
	{
		gSCPDevicePackageBusy = false;
		return 1;
	}
	return SCPLoad(500*kMilliseconds, 2, kSCPLoadAnyDevice, NULL, inArg1);
}


NewtonErr
CCMWorld::SCPLoad(ULong inWaitPeriod, ULong inNumOfTries, ULong inFilter, CUMsgToken * inToken, ULong inArg5)
{
	NewtonErr err = noErr;
	XTRY
	{
		XFAILIF(fD8 != NULL, err = kCMErrSCPLoadInProgress;)
#if defined(correct)
		// we’re not interested in the Serial Comms Protocol right now

		CSCPLoader sp00;	// CAppWorld, size+9C
		XFAIL(err = sp00.init('scpl', true, kSpawnedTaskStackSize))

		CUPort loaderPort;
		XFAIL(err = GetSCPLoaderPort(&loaderPort))
		fD8 = new CCMSCPAsyncMessage;
		XFAILIF(fD8 == NULL, err = kOSErrNoMemory;)
		XFAIL(err = fD8->init(*gCommWorld->getMyPort(), &f70))
		if (inToken)
			fD8->setToken(inToken);
		fD8->f10 = 'newt';
		fD8->f14 = 'scpl';
		fD8->f18 = 6;
		fD8->f20 = inNumOfTries;
		fD8->f24 = inWaitPeriod;
		fD8->f28 = inFilter;
		fD8->f2C = inArg5;
		err = fD8->sendRPC(&loaderPort);
#endif
	}
	XENDTRY;
	return err;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C C M S C P A s y n c M e s s a g e
----------------------------------------------------------------------------- */

CCMSCPAsyncMessage::CCMSCPAsyncMessage()
{
	f50 = false;
}


NewtonErr
CCMSCPAsyncMessage::init(ObjectId inPortId, CEventHandler * inHandler)
{
	NewtonErr err;
	XTRY
	{
		XFAIL(err = CUAsyncMessage::init(true))
		XFAIL(err = setCollectorPort(inPortId))
		err = setUserRefCon((OpaqueRef)inHandler);
	}
	XENDTRY;
	return err;
}


void
CCMSCPAsyncMessage::setToken(CUMsgToken * inToken)
{
	if (inToken)
	{
		f54 = *inToken;
		f50 = true;
	}
}


NewtonErr
CCMSCPAsyncMessage::sendRPC(CUPort * inPort)
{
	return inPort->sendRPC(this, &f10, 0x20, &f30, 0x20);
}


NewtonErr
CCMSCPAsyncMessage::replyRPC(void)
{
	if (f50)
		return f54.replyRPC(&f30, 0x20, noErr);
	return noErr;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C I C H a n d l e r
	Interconnect handler.
----------------------------------------------------------------------------- */

NewtonErr
CICHandler::init(ObjectId inPortId)
{
	NewtonErr err;
	XTRY
	{
//		LockHeapRange((VAddr)this, (VAddr)(this+1), false);

		f14 = inPortId;

		f1C.fEventClass = kNewtEventClass;
		f1C.fEventId = 'comg';
//		f1C.f08 = 9;

		f04.fEventClass = kNewtEventClass;
		f04.fEventId = kIdleEventId;
//		f04.f08 = 'ic  ';

		f00 = new CUAsyncMessage;
		XFAILIF(f00 == NULL, err = kOSErrNoMemory;)
		XFAIL(err = f00->init(false))

		f30 = new CUAsyncMessage;
		XFAILIF(f30 == NULL, err = kOSErrNoMemory;)
		XFAIL(err = f30->init(false))

#if defined(correct)
		f18 = 0;
		InitIRQTimerObject();
		f34 = GetIRQTimerObject()->acquireIRQTimer(ic_TimerInterruptHandler, this);
		ULong pins;
		GetBIOInterfaceObject()->readDIOPins(0x20, &pins);
		f18 = (pins == 0) ? 5 : 6;
		f38 = GetBIOInterfaceObject()->registerInterrupt(0x20, this, ic_InterruptHandler, 11);
		GetBIOInterfaceObject()->enableInterrupt(f38);
#endif
	}
	XENDTRY;
	return err;
}


void
CICHandler::send(ULong)
{}


void
CICHandler::sendICMessage(ULong)
{}


void
CICHandler::setTimer(Timeout)
{}


void
CICHandler::resetTimer(Timeout)
{}


void
CICHandler::sampleInterconnectStateMachine(void)
{}


void
CICHandler::ic_InterruptHandler(void)
{}


void
CICHandler::ic_TimerInterruptHandler(void)
{}


#pragma mark -
/* -----------------------------------------------------------------------------
	C C M E v e n t
----------------------------------------------------------------------------- */

//CCMEvent::CCMEvent()
//{}


#pragma mark -
/* -----------------------------------------------------------------------------
	C C M E v e n t H a n d l e r
----------------------------------------------------------------------------- */

CCMEventHandler::CCMEventHandler()
{ }


NewtonErr
CCMEventHandler::init(EventId inEventId, EventClass inEventClass)
{
	NewtonErr err;
	XTRY
	{
		fInterconnectHandler = new CICHandler;
		XFAILIF(fInterconnectHandler == NULL, err = kOSErrNoMemory;)
		XFAIL(err = fInterconnectHandler->init(*gCommWorld->getMyPort()))
		err = CEventHandler::init(inEventId, inEventClass);
	}
	XENDTRY;
	return err;
}


void
CCMEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{}


void
CCMEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{}


NewtonErr
CCMEventHandler::startService(CUMsgToken * inToken, CCMEvent * inEvent)
{
	NewtonErr err = noErr;
	XTRY
	{
	// INCOMPLETE
	}
	XENDTRY;
	return err;
}


NewtonErr
CCMEventHandler::getLastDevice(CCMEvent * inEvent)
{
	NewtonErr err = noErr;
	XTRY
	{
		gCommWorld->eventSetReply(0x28);
		XFAILIF(gCommWorld->fDevice.fDeviceType == 0, err = kCMErrNoKnownLastDevice;)
		inEvent->f10 = gCommWorld->fDevice;
	}
	XENDTRY;
	return err;
}


NewtonErr
CCMEventHandler::setLastDevice(CCMEvent * inEvent)
{
	gCommWorld->setDevice(&inEvent->f10);
	inEvent->f0C = noErr;
	gCommWorld->eventSetReply(0x10);
	return noErr;
}


NewtonErr
CCMEventHandler::getLastPackage(CCMEvent * inEvent)
{
	NewtonErr err = noErr;
	XTRY
	{
		gCommWorld->eventSetReply(0x18);
		XFAILIF(gCommWorld->fE0 == 0, err = kCMErrNoKnownLastPackageLoaded;)
#if defined(correct)
		inEvent->f14 = gCommWorld->fE0;
		inEvent->f10 = gCommWorld->fDC;
#endif
	}
	XENDTRY;
	return err;
}


NewtonErr
CCMEventHandler::setLastPackage(CCMEvent * inEvent)
{
#if defined(correct)
	gCommWorld->setLastPackage(inEvent->f14, inEvent->f10);	// inEvent->fConnectedDevice.fLastConnectTime ?
#endif
	inEvent->f0C = 0;
	gCommWorld->eventSetReply(0x10);
	return noErr;
}


#pragma mark -
/* -----------------------------------------------------------------------------
	C C M S y t e m E v e n t H a n d l e r
----------------------------------------------------------------------------- */

void
CCMSystemEventHandler::powerOn(CEvent * inEvent)
{
	setReply(0x0C, inEvent);
	replyImmed();
	gCommWorld->SCPCheck(0x30);
}


void
CCMSystemEventHandler::powerOff(CEvent * inEvent)
{
	setReply(0x0C, inEvent);
	replyImmed();
}


void
CCMSystemEventHandler::appAlive(CEvent * inEvent)
{
	setReply(0x0C, inEvent);
	replyImmed();
	gCommWorld->SCPCheck(0x31);
}

