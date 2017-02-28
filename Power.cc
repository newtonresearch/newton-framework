/*
	File:		Power.cc

	Contains:	Power management.

	Written by:	Newton Research Group, 2009.
*/

#include "Objects.h"
#include "Power.h"
#include "RSSymbols.h"
#include "EventHandler.h"


/*--------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
--------------------------------------------------------------------------------*/

extern "C" {
Ref	FPowerOff(RefArg rcvr);

Ref	FMinimumBatteryCheck(RefArg inRcvr);
Ref	FBatteryCount(RefArg rcvr);
Ref	FBatteryStatus(RefArg rcvr, RefArg inBatterySelector);
Ref	FBatteryRawStatus(RefArg rcvr, RefArg inBatterySelector);
Ref	FBatteryLevel(RefArg rcvr, RefArg inBatterySelector);
Ref	FSetBatteryType(RefArg rcvr, RefArg inBatterySelector, RefArg inType);
Ref	FCheckCardBattery(RefArg rcvr);
Ref	FEnablePowerStats(RefArg rcvr, RefArg inBatterySelector);
Ref	FGetPowerStats(RefArg rcvr);
Ref	FResetPowerStats(RefArg rcvr);
}


/*--------------------------------------------------------------------------------
	C P o w e r E v e n t H a n d l e r
--------------------------------------------------------------------------------*/
typedef int PowerPlantStatus;
int	SetBatteryType(ULong, ULong);
int	GetPowerPlantStatus(ULong, PowerPlantStatus *);
int	GetRawPowerPlantStatus(ULong, PowerPlantStatus *);
int	GetPowerPlantCount(void);

#if !defined(forFramework)

void
CPowerEventHandler::eventHandlerProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
#if defined(correct)
	switch (inEvent->x08)
	{
	case 1:
	case 2:
	case 3:
		inEvent->x08 = 100;
		setReply(16, inEvent);
		break;
	case 4:
		inEvent->x08 = GetPowerPlantStatus(inEvent->x0C, &inEvent->x10);
		setReply(68, inEvent);
		break;
	case 5:
		inEvent->x08 = GetRawPowerPlantStatus(inEvent->x0C, &inEvent->x10);
		setReply(68, inEvent);
		break;
	case 6:
		inEvent->x0C = GetPowerPlantCount();
		setReply(16, inEvent);
		break;
	case 7:
		inEvent->x0C = SetBatteryType(inEvent->x0C, inEvent->x10);
		break;
	default:
		gPowerManager->doCommand(inToken, inSize, inEvent);
		gPowerManager->eventSetReply(16);
	}
#endif
}


void
CPowerEventHandler::eventCompletionProc(CUMsgToken * inToken, size_t * inSize, CEvent * inEvent)
{
#if defined(correct)
	gPowerManager->doReply(inToken, inSize, inEvent);
#endif
}

#endif


/*--------------------------------------------------------------------------------
	P o w e r
--------------------------------------------------------------------------------*/
extern "C" {
Ref	FSetLCDContrast(RefArg inRcvr, RefArg inContrast);
}


int	SleepUntilNextWakeup(void);
ULong	CyclePower(void);

int
SleepUntilNextWakeup(void)
{
#if defined(correct)
	ULong pwrEvt;
	FBacklight(RA(NILREF), RA(NILREF));
	pwrEvt = CyclePower();
	if (pwrEvt != 0)
		ClearHardKeymap();
	FSetLCDContrast(RA(NILREF), GetPreference(SYMA(lcdContrast)));
	return TranslatePowerEvent(pwrEvt);
#else
	return 0;
#endif
}


ULong
CyclePower(void)
{
#if defined(correct)
	//CSysEventRequest sp+2C
	CSendSystemEvent sysEvent(kSysEvent_PowerOff);	//sp18
	CPowerEvent pwrEvent(kSystemEventId, kSysEvent_PowerOff);		//sp08
	StackLimits limits;			//sp00
	ULong pwrEvt;
	NewtonErr err;

	sysEvent.init();
	sysEvent.sendSystemEvent(&pwrEvent, 12);

	BatteryShutDown();
	LCDPowerOff(true);
	TabShutDown();

	while (IsInternalFlashEraseActive())
		/* wait for it to finish */ ;

	if (LockStack(&limits, 256) == noErr)
	{
		EnterFIQAtomicFast();
		HoldSchedule();
		r6 = gNewtPort->receive(NULL, kNoTimeout, 1);
		AllowSchedule();
		if (r6)
		{
			do
			{
				IOPowerOffAll();
				PowerOffSystem();

				//...zzz...

				err = PowerOnSystem();
			}
			while (err != noErr || (pwrEvt = g0F183000 & g0F184800, (pwrEvt & 0x04) == 0 && CRealTimeClock::sleepingCheckFire()));
			SCCPowerInit();
		}
		ExitFIQAtomicFast();
		UnlockStack(&limits);
	}

	LCDPowerInit(true);
	sysEvent->setEvent(kSysEvent_PowerOn);
	sysEvent->sendSystemEvent(&pwrEvent, 16);
	TabWakeup();
	gPowerInterruptAsyncMessage->abort();
	return pwrEvt;
#else
	return 0;
#endif
}


/*--------------------------------------------------------------------------------
	P l a i n   C   I n t e r f a c e
--------------------------------------------------------------------------------*/
extern CTime gLastWakeupTime;

Ref
FPowerOff(RefArg rcvr)
{
	Ref reason;
	int wakeReason = SleepUntilNextWakeup();
#if defined(correct)

	//...zzz...

	FMinimumBatteryCheck(RA(NILREF));
	if (EQ(GetPreference(SYMA(blessedApp)), SYMA(setup)))
		LoadInkerCalibration();
	gLastWakeupTime = GetGlobalTime();
#endif
	switch (wakeReason)
	{
	case 2:
		reason = SYMA(serialGPI);
		break;
	case 3:
		reason = SYMA(alarm);
		break;
	case 4:
		reason = SYMA(user);
		break;
	case 5:
		reason = SYMA(cardLock);
		break;
	case 7:
		reason = SYMA(interconnect);
		break;
	default:
		reason = SYMA(because);
		break;
	}
	return reason;
}


Ref
FMinimumBatteryCheck(RefArg inRcvr)
{
	bool  isDead = false;
#if defined(correct)
	for ( ; ; )
	{
		PowerPlantStatus  pwr;  // size +34
		if (GetBatteryStatus(0, &pwr, false) == 0)
		{
			if (pwr.f18 == 1 || pwr.f08 > pwr.f10)
				break;
			isDead = true;
			SleepUntilNextWakeup();
		}
	}
#endif
	return MAKEBOOLEAN(isDead);
}


Ref
FBatteryCount(RefArg rcvr)
{ return MAKEINT(1); }

Ref
FBatteryStatus(RefArg rcvr, RefArg inBatterySelector)
{
	RefVar status(AllocateFrame());
	SetFrameSlot(status, SYMA(acPower), SYMA(yes));
	SetFrameSlot(status, SYMA(batteryCapacity), MAKEINT(100));
	return status;
}

Ref
FBatteryRawStatus(RefArg rcvr, RefArg inBatterySelector)
{ return NILREF; }

Ref
FBatteryLevel(RefArg rcvr, RefArg inBatterySelector)
{ return NILREF; }

Ref
FSetBatteryType(RefArg rcvr, RefArg inBatterySelector, RefArg inType)
{ return NILREF; }

Ref
FCheckCardBattery(RefArg rcvr)
{ return NILREF; }

Ref
FEnablePowerStats(RefArg rcvr, RefArg inBatterySelector)
{ return NILREF; }

Ref
FGetPowerStats(RefArg rcvr)
{ return NILREF; }

Ref
FResetPowerStats(RefArg rcvr)
{ return NILREF; }

