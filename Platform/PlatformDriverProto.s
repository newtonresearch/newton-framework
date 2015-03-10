/*
	File:		PlatformDriverProto.s

	Contains:	CPlatformDriver p-class protocol interfaces.

	Written by:	Newton Research Group, 2015.
*/

#include "Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN15CPlatformDriver4makeEPKc
		.globl	__ZN15CPlatformDriver7destroyEv
		.globl	__ZN15CPlatformDriver4initEv
		.globl	__ZN15CPlatformDriver16backlightTriggerEv
		.globl	__ZN15CPlatformDriver28registerPowerSwitchInterruptEv
		.globl	__ZN15CPlatformDriver23enableSysPowerInterruptEv
		.globl	__ZN15CPlatformDriver16interruptHandlerEv
		.globl	__ZN15CPlatformDriver21timerInterruptHandlerEv
		.globl	__ZN15CPlatformDriver18resetZAPStoreCheckEv
		.globl	__ZN15CPlatformDriver13powerOnSystemEv
		.globl	__ZN15CPlatformDriver11pauseSystemEv
		.globl	__ZN15CPlatformDriver14powerOffSystemEv
		.globl	__ZN15CPlatformDriver16powerOnSubsystemEj
		.globl	__ZN15CPlatformDriver17powerOffSubsystemEj
		.globl	__ZN15CPlatformDriver21powerOffAllSubsystemsEv
		.globl	__ZN15CPlatformDriver19translatePowerEventEj
		.globl	__ZN15CPlatformDriver18getPCMCIAPowerSpecEjPj
		.globl	__ZN15CPlatformDriver18powerOnDeviceCheckEh
		.globl	__ZN15CPlatformDriver17setSubsystemPowerEjj
		.globl	__ZN15CPlatformDriver17getSubsystemPowerEjPj

		.text
		.align	2

CPlatformDriver_name:
		.asciz	"CPlatformDriver"
		.align	2

__ZN15CPlatformDriver4makeEPKc:
		New CPlatformDriver_name
		Dispatch 2
__ZN15CPlatformDriver7destroyEv:
		Delete 3
__ZN15CPlatformDriver4initEv:
		Dispatch 4
__ZN15CPlatformDriver16backlightTriggerEv:
		Dispatch 5
__ZN15CPlatformDriver28registerPowerSwitchInterruptEv:
		Dispatch 6
__ZN15CPlatformDriver23enableSysPowerInterruptEv:
		Dispatch 7
__ZN15CPlatformDriver16interruptHandlerEv:
		Dispatch 8
__ZN15CPlatformDriver21timerInterruptHandlerEv:
		Dispatch 9
__ZN15CPlatformDriver18resetZAPStoreCheckEv:
		Dispatch 10
__ZN15CPlatformDriver13powerOnSystemEv:
		Dispatch 11
__ZN15CPlatformDriver11pauseSystemEv:
		Dispatch 12
__ZN15CPlatformDriver14powerOffSystemEv:
		Dispatch 13
__ZN15CPlatformDriver16powerOnSubsystemEj:
		Dispatch 14
__ZN15CPlatformDriver17powerOffSubsystemEj:
		Dispatch 15
__ZN15CPlatformDriver21powerOffAllSubsystemsEv:
		Dispatch 16
__ZN15CPlatformDriver19translatePowerEventEj:
		Dispatch 17
__ZN15CPlatformDriver18getPCMCIAPowerSpecEjPj:
		Dispatch 18
__ZN15CPlatformDriver18powerOnDeviceCheckEh:
		Dispatch 19
__ZN15CPlatformDriver17setSubsystemPowerEjj:
		Dispatch 20
__ZN15CPlatformDriver17getSubsystemPowerEjPj:
		Dispatch 21
