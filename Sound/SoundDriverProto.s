/*
	File:		SoundDriverProto.s

	Contains:	PSoundDriver p-class protocol interfaces.

	Written by:	Newton Research Group, 2015.
*/

#include "../Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN12PSoundDriver4makeEPKc
		.globl	__ZN12PSoundDriver7destroyEv
		.globl	__ZN12PSoundDriver20setSoundHardwareInfoEPK15SoundDriverInfo
		.globl	__ZN12PSoundDriver20getSoundHardwareInfoEP15SoundDriverInfo
		.globl	__ZN12PSoundDriver16setOutputBuffersEmmmm
		.globl	__ZN12PSoundDriver15setInputBuffersEmmmm
		.globl	__ZN12PSoundDriver20scheduleOutputBufferEmm
		.globl	__ZN12PSoundDriver19scheduleInputBufferEmm
		.globl	__ZN12PSoundDriver13powerOutputOnEi
		.globl	__ZN12PSoundDriver14powerOutputOffEv
		.globl	__ZN12PSoundDriver12powerInputOnEi
		.globl	__ZN12PSoundDriver13powerInputOffEv
		.globl	__ZN12PSoundDriver11startOutputEv
		.globl	__ZN12PSoundDriver10startInputEv
		.globl	__ZN12PSoundDriver10stopOutputEv
		.globl	__ZN12PSoundDriver9stopInputEv
		.globl	__ZN12PSoundDriver15outputIsEnabledEv
		.globl	__ZN12PSoundDriver14inputIsEnabledEv
		.globl	__ZN12PSoundDriver15outputIsRunningEv
		.globl	__ZN12PSoundDriver14inputIsRunningEv
		.globl	__ZN12PSoundDriver16currentOutputPtrEv
		.globl	__ZN12PSoundDriver15currentInputPtrEv
		.globl	__ZN12PSoundDriver12outputVolumeEf
		.globl	__ZN12PSoundDriver12outputVolumeEv
		.globl	__ZN12PSoundDriver11inputVolumeEi
		.globl	__ZN12PSoundDriver11inputVolumeEv
		.globl	__ZN12PSoundDriver20enableExtSoundSourceEi
		.globl	__ZN12PSoundDriver21disableExtSoundSourceEi
		.globl	__ZN12PSoundDriver16outputIntHandlerEv
		.globl	__ZN12PSoundDriver15inputIntHandlerEv
		.globl	__ZN12PSoundDriver26outputIntHandlerDispatcherEv
		.globl	__ZN12PSoundDriver25inputIntHandlerDispatcherEv
		.globl	__ZN12PSoundDriver21setOutputCallbackProcEPFlPvES0_
		.globl	__ZN12PSoundDriver20setInputCallbackProcEPFlPvES0_

		.text
		.align	2

PSoundDriver_name:
		.asciz	"PSoundDriver"
		.align	2

__ZN12PSoundDriver4makeEPKc:
		New PSoundDriver_name
		Dispatch 2
__ZN12PSoundDriver7destroyEv:
		Delete 3

__ZN12PSoundDriver20setSoundHardwareInfoEPK15SoundDriverInfo:
		Dispatch 4
__ZN12PSoundDriver20getSoundHardwareInfoEP15SoundDriverInfo:
		Dispatch 5
__ZN12PSoundDriver16setOutputBuffersEmmmm:
		Dispatch 6
__ZN12PSoundDriver15setInputBuffersEmmmm:
		Dispatch 7
__ZN12PSoundDriver20scheduleOutputBufferEmm:
		Dispatch 8
__ZN12PSoundDriver19scheduleInputBufferEmm:
		Dispatch 9
__ZN12PSoundDriver13powerOutputOnEi:
		Dispatch 10
__ZN12PSoundDriver14powerOutputOffEv:
		Dispatch 11
__ZN12PSoundDriver12powerInputOnEi:
		Dispatch 12
__ZN12PSoundDriver13powerInputOffEv:
		Dispatch 13
__ZN12PSoundDriver11startOutputEv:
		Dispatch 14
__ZN12PSoundDriver10startInputEv:
		Dispatch 15
__ZN12PSoundDriver10stopOutputEv:
		Dispatch 16
__ZN12PSoundDriver9stopInputEv:
		Dispatch 17
__ZN12PSoundDriver15outputIsEnabledEv:
		Dispatch 18
__ZN12PSoundDriver14inputIsEnabledEv:
		Dispatch 19
__ZN12PSoundDriver15outputIsRunningEv:
		Dispatch 20
__ZN12PSoundDriver14inputIsRunningEv:
		Dispatch 21
__ZN12PSoundDriver16currentOutputPtrEv:
		Dispatch 22
__ZN12PSoundDriver15currentInputPtrEv:
		Dispatch 23
__ZN12PSoundDriver12outputVolumeEf:
		Dispatch 24
__ZN12PSoundDriver12outputVolumeEv:
		Dispatch 25
__ZN12PSoundDriver11inputVolumeEi:
		Dispatch 26
__ZN12PSoundDriver11inputVolumeEv:
		Dispatch 27
__ZN12PSoundDriver20enableExtSoundSourceEi:
		Dispatch 28
__ZN12PSoundDriver21disableExtSoundSourceEi:
		Dispatch 29
__ZN12PSoundDriver16outputIntHandlerEv:
		Dispatch 30
__ZN12PSoundDriver15inputIntHandlerEv:
		Dispatch 31
__ZN12PSoundDriver26outputIntHandlerDispatcherEv:
		Dispatch 32
__ZN12PSoundDriver25inputIntHandlerDispatcherEv:
		Dispatch 33
__ZN12PSoundDriver21setOutputCallbackProcEPFlPvES0_:
		Dispatch 34
__ZN12PSoundDriver20setInputCallbackProcEPFlPvES0_:
		Dispatch 35
