/*
	File:		Platform.h

	Contains:	Platform-specific implementation.

	Written by:	Newton Research Group.
*/

#include "Protocols.h"


/* -----------------------------------------------------------------------------
	C P l a t f o r m D r i v e r
	P-class protocol interface.
----------------------------------------------------------------------------- */

PROTOCOL CPlatformDriver : public CProtocol
{
public:
	static CPlatformDriver *	make(const char * inName);
	void			destroy(void);

	void			init(void);

	void			backlightTrigger(void);
	void			registerPowerSwitchInterrupt(void);
	void			enableSysPowerInterrupt(void);
	void			interruptHandler(void);
	void			timerInterruptHandler(void);

	bool			resetZAPStoreCheck(void);

	NewtonErr	powerOnSystem(void);
	NewtonErr	pauseSystem(void);
	NewtonErr	powerOffSystem(void);

	void			powerOnSubsystem(ULong);
	void			powerOffSubsystem(ULong);
	void			powerOffAllSubsystems(void);

	void			translatePowerEvent(ULong);
	void			getPCMCIAPowerSpec(ULong, ULong *);
	void			powerOnDeviceCheck(unsigned char);
	void			setSubsystemPower(ULong, ULong);
	void			getSubsystemPower(ULong, ULong *);
};


/* -----------------------------------------------------------------------------
	C V o y a g e r P l a t f o r m
	Protocol implementation, just for interest.
	We actually need a Mac-specific platform driver.
----------------------------------------------------------------------------- */
struct PowerMapEntry;

PROTOCOL CVoyagerPlatform : public CPlatformDriver
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CVoyagerPlatform)

	CVoyagerPlatform *	make(void);
	void			destroy(void);

	void			init(void);

	void			backlightTrigger(void);
	void			registerPowerSwitchInterrupt(void);
	void			enableSysPowerInterrupt(void);
	void			interruptHandler(void);
	void			timerInterruptHandler(void);

	bool			resetZAPStoreCheck(void);

	NewtonErr	powerOnSystem(void);
	NewtonErr	pauseSystem(void);
	NewtonErr	powerOffSystem(void);

	void			powerOnSubsystem(ULong);
	void			powerOffSubsystem(ULong);
	void			powerOffAllSubsystems(void);

	void			translatePowerEvent(ULong);
	void			getPCMCIAPowerSpec(ULong, ULong *);
	void			powerOnDeviceCheck(unsigned char);
	void			setSubsystemPower(ULong, ULong);
	void			getSubsystemPower(ULong, ULong *);

private:
	void		samplePowerSwitchStateMachine(void);

	void		sleep(ULong);

	void		getMutex(void);
	void		relMutex(void);

	void		powerOnIC5v(unsigned char);
	void		powerOffIC5v(unsigned char);
	void		powerOnSrc5v(unsigned char);
	void		powerOffSrc5v(unsigned char);
	void		powerOnSrc12v(unsigned char);
	void		powerOffSrc12v(unsigned char);

	void		turnOnMiltonPwrRegBit(PowerMapEntry *);
	void		turnOffMiltonPwrRegBit(PowerMapEntry *);

	void		getPowerMapEntry(unsigned char, PowerMapEntry *);

	void		powerOnDMA(unsigned char);
	void		powerOffDMA(unsigned char);

	void		checkForKeyboard(unsigned char);
	void		startKeyboardDriver(unsigned char);

	void		serialPort0LineDriverConfig(unsigned char, unsigned char);
	void		serialPort3LineDriverConfig(unsigned char, unsigned char);

	char		f34;
	char		f36;
// size+0104
};


/* -----------------------------------------------------------------------------
	C M a c P l a t f o r m
	Protocol implementation.
----------------------------------------------------------------------------- */
#import <dispatch/dispatch.h>

PROTOCOL CMacPlatform : public CPlatformDriver
{
public:
	PROTOCOL_IMPL_HEADER_MACRO(CMacPlatform)

	CMacPlatform *	make(void);
	void			destroy(void);

	void			init(void);

	void			backlightTrigger(void);
	void			registerPowerSwitchInterrupt(void);
	void			enableSysPowerInterrupt(void);
	void			interruptHandler(void);
	void			timerInterruptHandler(void);

	bool			resetZAPStoreCheck(void);

	NewtonErr	powerOnSystem(void);
	NewtonErr	pauseSystem(void);
	NewtonErr	powerOffSystem(void);

	void			powerOnSubsystem(ULong);
	void			powerOffSubsystem(ULong);
	void			powerOffAllSubsystems(void);

	void			translatePowerEvent(ULong);
	void			getPCMCIAPowerSpec(ULong, ULong *);
	void			powerOnDeviceCheck(unsigned char);
	void			setSubsystemPower(ULong, ULong);
	void			getSubsystemPower(ULong, ULong *);

private:
	dispatch_semaphore_t interrupt;
	bool						isPaused;
};


/* -----------------------------------------------------------------------------
	Public interface.
----------------------------------------------------------------------------- */

extern void LoadPlatformDriver(void);
extern CPlatformDriver * GetPlatformDriver(void);
