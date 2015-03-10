/*
	File:		Power.cc

	Contains:	Power management.

	Written by:	Newton Research Group, 2009.
*/

#include "Objects.h"
#include "Power.h"
#include "ROMSymbols.h"


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

int	SleepUntilNextWakeup(void);
ULong	CyclePower(void);

#if 0
Ref
FPowerOff(RefArg rcvr)
{
	Ref reason;
	int wakeReason = SleepUntilNextWakeup();

	//...zzz...

	FMinimumBatteryCheck(RA(NILREF));
	if (EQ(GetPreference(SYMA(blessedApp)), SYMA(setup)))
		LoadInkerCalibration();
	gLastWakeupTime = GetGlobalTime();
	switch (wakeReason)
	{
	case 2:
		reason = RSYMserialGPI;
		break;
	case 3:
		reason = RSYMalarm;
		break;
	case 4:
		reason = RSYMuser;
		break;
	case 5:
		reason = RSYMcardLock;
		break;
	case 7:
		reason = RSYMinterconnect;
		break;
	default:
		reason = RSYMbecause;
		break;
	}
	return reason;
}


int
SleepUntilNextWakeup(void)
{
	ULong pwrEvt;
	FBacklight(RA(NILREF), RA(NILREF));
	pwrEvt = CyclePower();
	if (pwrEvt != 0)
		ClearHardKeymap();
	FSetLCDContrast(RA(NILREF), GetPreference(SYMA(lcdContrast)));
	return TranslatePowerEvent(pwrEvt);
}


ULong
CyclePower(void)
{
	//CSysEventRequest sp+2C
	CSendSystemEvent sysEvent('pwof');	//sp18
	CPowerEvent pwrEvent('sysm', 'pwof');		//sp08
	StackLimits limits;			//sp00
	ULong pwrEvt;
	NewtonErr err;

	sysEvent.init();
	sysEvent.sendSystemEvent(&pwrEvent, 12);

	BatteryShutDown();
	LCDPowerOff(YES);
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

	LCDPowerInit(YES);
	sysEvent->setEvent('pwon');
	sysEvent->sendSystemEvent(&pwrEvent, 16);
	TabWakeup();
	gPowerInterruptAsyncMessage->abort();
	return pwrEvt;
}
#endif


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

