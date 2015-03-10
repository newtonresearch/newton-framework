/*
	File:		Platform.cc

	Contains:	Platform-specific implementation.

	Written by:	Newton Research Group.
*/

#include "UserGlobals.h"
#include "Platform.h"


/* -------------------------------------------------------------------------------
	D a t a
------------------------------------------------------------------------------- */
extern bool			gWantDeferred;
extern ULong		gCPSR;

CPlatformDriver *	gPlatformDriver = NULL;	// 0C101764


/* -------------------------------------------------------------------------------
	Load the platform driver.
	Args:		--
	Return:	--
------------------------------------------------------------------------------- */

void
LoadPlatformDriver(void)
{
	// see if licensee has registered CMainPlatformDriver
	gPlatformDriver = (CPlatformDriver *)MakeByName("CPlatformDriver", "CMainPlatformDriver");
	if (gPlatformDriver == NULL)
		// no, fall back to default
		gPlatformDriver = (CPlatformDriver *)CMacPlatform::classInfo()->make();	// original would have made CVoyagerPlatform, obviously
	if (gPlatformDriver)
		gPlatformDriver->init();
}


/* -------------------------------------------------------------------------------
	Return the platform driver.
	Args:		--
	Return:	the platform driver implementation instance
------------------------------------------------------------------------------- */

CPlatformDriver *
GetPlatformDriver(void)
{
	return gPlatformDriver;
}


bool
UserWantsColdBoot(void)
{
	if (gPlatformDriver)
		return gPlatformDriver->resetZAPStoreCheck();
}



void
SpecialCPUIntDisable(void)
{
/*
		MSR	CPSR, #&D3
		NOP
		NOP
		MOV	PC, LK
*/
	gCPSR = kSuperMode+kFIQDisable+kIRQDisable;
}

void
SpecialCPUIntEnable(void)
{
/*
		MSR	CPSR, #&93
		NOP
		NOP
		MOV	PC, LK
*/
	gCPSR = kSuperMode+kFIQDisable;
}


NewtonErr
PowerOnSystem(void)
{
	return GetPlatformDriver()->powerOnSystem();
}


NewtonErr
PauseSystemKernelGlue(void)
{
	SpecialCPUIntDisable();
	if (!gWantDeferred)	// only pause if no deferred tasks waiting
		GetPlatformDriver()->pauseSystem();
	SpecialCPUIntEnable();
	return noErr;
}


NewtonErr
PowerOffSystemKernelGlue(void)
{
	return GetPlatformDriver()->powerOffSystem();
}


void
IOPowerOffAll(void)
{
	CPlatformDriver * driver = GetPlatformDriver();
	if (driver)
		driver->powerOffAllSubsystems();
}


#pragma mark - Mac platform driver
/* ----------------------------------------------------------------
	CMacPlatform implementation class info.
---------------------------------------------------------------- */

const CClassInfo *
CMacPlatform::classInfo(void)
{
__asm__ (
CLASSINFO_BEGIN
"		.long		0			\n"
"		.long		1f - .	\n"
"		.long		2f - .	\n"
"		.long		3f - .	\n"
"		.long		4f - .	\n"
"		.long		5f - .	\n"
"		.long		__ZN12CMacPlatform6sizeOfEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		__ZN12CMacPlatform4makeEv - 0b	\n"
"		.long		__ZN12CMacPlatform7destroyEv - 0b	\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		0			\n"
"		.long		6f - 0b	\n"
"1:	.asciz	\"CMacPlatform\"	\n"
"2:	.asciz	\"CPlatformDriver\"	\n"
"3:	.byte		0			\n"
"		.align	2			\n"
"4:	.long		0			\n"
"		.long		__ZN12CMacPlatform9classInfoEv - 4b	\n"
"		.long		__ZN12CMacPlatform4makeEv - 4b	\n"
"		.long		__ZN12CMacPlatform7destroyEv - 4b	\n"
"		.long		__ZN12CMacPlatform4initEv - 4b	\n"
"		.long		__ZN12CMacPlatform16backlightTriggerEv - 4b	\n"
"		.long		__ZN12CMacPlatform28registerPowerSwitchInterruptEv - 4b	\n"
"		.long		__ZN12CMacPlatform23enableSysPowerInterruptEv - 4b	\n"
"		.long		__ZN12CMacPlatform16interruptHandlerEv - 4b	\n"
"		.long		__ZN12CMacPlatform21timerInterruptHandlerEv - 4b	\n"
"		.long		__ZN12CMacPlatform18resetZAPStoreCheckEv - 4b	\n"
"		.long		__ZN12CMacPlatform13powerOnSystemEv - 4b	\n"
"		.long		__ZN12CMacPlatform11pauseSystemEv - 4b	\n"
"		.long		__ZN12CMacPlatform14powerOffSystemEv - 4b	\n"
"		.long		__ZN12CMacPlatform16powerOnSubsystemEj - 4b	\n"
"		.long		__ZN12CMacPlatform17powerOffSubsystemEj - 4b	\n"
"		.long		__ZN12CMacPlatform21powerOffAllSubsystemsEv - 4b	\n"
"		.long		__ZN12CMacPlatform19translatePowerEventEj - 4b	\n"
"		.long		__ZN12CMacPlatform18getPCMCIAPowerSpecEjPj - 4b	\n"
"		.long		__ZN12CMacPlatform18powerOnDeviceCheckEh - 4b	\n"
"		.long		__ZN12CMacPlatform17setSubsystemPowerEjj - 4b	\n"
"		.long		__ZN12CMacPlatform17getSubsystemPowerEjPj - 4b	\n"
CLASSINFO_END
);
}

PROTOCOL_IMPL_SOURCE_MACRO(CMacPlatform)


CMacPlatform *
CMacPlatform::make(void)
{
	return this;
}


void
CMacPlatform::destroy(void)
{ }


void
CMacPlatform::init(void)
{
	isPaused = false;
	interrupt = dispatch_semaphore_create(0);
}


void
CMacPlatform::backlightTrigger(void)
{}

void
CMacPlatform::registerPowerSwitchInterrupt(void)
{}

void
CMacPlatform::enableSysPowerInterrupt(void)
{}

void
CMacPlatform::interruptHandler(void)
{}


void
CMacPlatform::timerInterruptHandler(void)
{
	// not what Voyager uses this for, but…
	// signal semaphore
/*
	if (isPaused)
	{
		dispatch_semaphore_signal(interrupt);
	}
*/
}


bool
CMacPlatform::resetZAPStoreCheck(void)
{ return NO; }


NewtonErr
CMacPlatform::powerOnSystem(void)
{ return noErr; }


NewtonErr
CMacPlatform::pauseSystem(void)
{
printf("CMacPlatform::pauseSystem()\n");
	//
	// wait for semaphore
	isPaused = true;
	dispatch_semaphore_wait(interrupt, DISPATCH_TIME_FOREVER /*dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_MSEC*500)*/);	// unpause every 500ms?
	isPaused = false;
printf("CMacPlatform::pauseSystem() exiting\n");
	return noErr;
}


NewtonErr
CMacPlatform::powerOffSystem(void)
{ return noErr; }


void
CMacPlatform::powerOnSubsystem(ULong)
{}

void
CMacPlatform::powerOffSubsystem(ULong)
{}

void
CMacPlatform::powerOffAllSubsystems(void)
{}


void
CMacPlatform::translatePowerEvent(ULong)
{}

void
CMacPlatform::getPCMCIAPowerSpec(ULong, ULong *)
{}

void
CMacPlatform::powerOnDeviceCheck(unsigned char)
{}

void
CMacPlatform::setSubsystemPower(ULong, ULong)
{}

void
CMacPlatform::getSubsystemPower(ULong, ULong *)
{}
