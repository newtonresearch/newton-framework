/*
	File:		PowerManager.cc

	Contains:	Power management task world.

	Written by:	Newton Research Group.
*/

#include "PowerManager.h"

CEvent				gPowerInterruptEvent;			// 0C101770
CUAsyncMessage		gPowerInterruptAsyncMessage;	// 0C10177C
CUPort *				gPowerPort = NULL;					// 0C101794
CPowerManager *	gPowerMgr = NULL;					// 0C101798


void
InitPowerManager(void)
{
	XTRY
	{
#if defined(correct)
		gPowerMgr = new CPowerManager;
		XFAIL(gPowerMgr == NULL)
		XFAIL(gPowerMgr->init('pg&e', true, kSpawnedTaskStackSize))
		InitializePowerInterrupt();
#endif
	}
	XENDTRY;
}

#if defined(correct)
NewtonErr
InitializePowerInterrupt(void)
{
	new (&gPowerInterruptEvent) CPowerEvent('pg&e');
	gPowerInterruptEvent.fSysEventType = 'powr';
	gPowerInterruptAsyncMessage.init(false);
	GetPlatformDriver()->registerPowerSwitchInterrupt();
	GetPlatformDriver()->enableSysPowerInterrupt();
	return noErr;
}


CUPort *
GetPowerPort(void)
{
	return gPowerPort;
}


#pragma mark -
/*----------------------------------------------------------------------
	C P o w e r M a n a g e r
----------------------------------------------------------------------*/

CPowerManager::CPowerManager()
{}


CPowerManager::~CPowerManager()
{}


size_t
CPowerManager::getSizeOf(void) const
{
	return sizeof(CPowerManager);
}


NewtonErr
CPowerManager::mainConstructor(void)
{}


void
CPowerManager::mainDestructor(void)
{
	CAppWorld::mainDestructor();
}


void
CPowerManager::doCommand(CUMsgToken * inToken, size_t * inSize, CPowerManagerEvent * inEvent)
{
	if (inEvent->fEventType == 'powr')
		powerOffMessage();
	else if (inEvent->fEventType == 'bklt')
		backlightMessage();
}


void
CPowerManager::doReply(CUMsgToken * inToken, size_t * inSize, CPowerManagerEvent * inEvent)
{
	if (inToken->getMsgId() == x90.fMsg.fId)
		powerOffTimeout();
}


void
CPowerManager::backlightMessage(void)
{
	bool	isBacklightOn;
	GetGrafInfo(kGrafBacklight, &isBacklightOn);
	SetGrafInfo(kGrafBacklight, !isBacklightOn);

	CUPort *	newt = GetNewtTaskPort();
	if (newt)
	{
		// INCOMPLETE
		sendRPC();
	}
}


void
CPowerManager::powerOffMessage(void)
{}


void
CPowerManager::powerOffTimeout(void)
{
	if (x90->getResult == kOSErrMessageTimedOut)
		PowerOffAndReboot(noErr)
}
#endif
