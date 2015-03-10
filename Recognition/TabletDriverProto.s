/*
	File:		TabletDriverProto.s

	Contains:	CTabletDriver p-class protocol interfaces.

	Written by:	Newton Research Group, 2010.
*/

#include "../Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN13CTabletDriver4makeEPKc
		.globl	__ZN13CTabletDriver7destroyEv
		.globl	__ZN13CTabletDriver4initERK4Rect
		.globl	__ZN13CTabletDriver6wakeUpEv
		.globl	__ZN13CTabletDriver8shutDownEv
		.globl	__ZN13CTabletDriver10tabletIdleEv
		.globl	__ZN13CTabletDriver13getSampleRateEv
		.globl	__ZN13CTabletDriver13setSampleRateEj
		.globl	__ZN13CTabletDriver20getTabletCalibrationEP11Calibration
		.globl	__ZN13CTabletDriver20setTabletCalibrationERK11Calibration
		.globl	__ZN13CTabletDriver19setDoingCalibrationEbPj
		.globl	__ZN13CTabletDriver19getTabletResolutionEPfS0_
		.globl	__ZN13CTabletDriver14setOrientationEi
		.globl	__ZN13CTabletDriver14getTabletStateEv
		.globl	__ZN13CTabletDriver19getFingerInputStateEPb
		.globl	__ZN13CTabletDriver19setFingerInputStateEb
		.globl	__ZN13CTabletDriver28recalibrateTabletAfterRotateEv
		.globl	__ZN13CTabletDriver24tabletNeedsRecalibrationEv
		.globl	__ZN13CTabletDriver17startBypassTabletEv
		.globl	__ZN13CTabletDriver16stopBypassTabletEv
		.globl	__ZN13CTabletDriver27returnTabletToConsciousnessEjjj

		.text
		.align	2

CTabletDriver_name:
		.asciz	"CTabletDriver"
		.align	2

__ZN13CTabletDriver4makeEPKc:
		New CTabletDriver_name
		Dispatch 2
__ZN13CTabletDriver7destroyEv:
		Delete 3
__ZN13CTabletDriver4initERK4Rect:
		Dispatch 4
__ZN13CTabletDriver6wakeUpEv:
		Dispatch 5
__ZN13CTabletDriver8shutDownEv:
		Dispatch 6
__ZN13CTabletDriver10tabletIdleEv:
		Dispatch 7
__ZN13CTabletDriver13getSampleRateEv:
		Dispatch 8
__ZN13CTabletDriver13setSampleRateEj:
		Dispatch 9
__ZN13CTabletDriver20getTabletCalibrationEP11Calibration:
		Dispatch 10
__ZN13CTabletDriver20setTabletCalibrationERK11Calibration:
		Dispatch 11
__ZN13CTabletDriver19setDoingCalibrationEbPj:
		Dispatch 12
__ZN13CTabletDriver19getTabletResolutionEPfS0_:
		Dispatch 13
__ZN13CTabletDriver14setOrientationEi:
		Dispatch 14
__ZN13CTabletDriver14getTabletStateEv:
		Dispatch 15
__ZN13CTabletDriver19getFingerInputStateEPb:
		Dispatch 16
__ZN13CTabletDriver19setFingerInputStateEb:
		Dispatch 17
__ZN13CTabletDriver28recalibrateTabletAfterRotateEv:
		Dispatch 18
__ZN13CTabletDriver24tabletNeedsRecalibrationEv:
		Dispatch 19
__ZN13CTabletDriver17startBypassTabletEv:
		Dispatch 20
__ZN13CTabletDriver16stopBypassTabletEv:
		Dispatch 21
__ZN13CTabletDriver27returnTabletToConsciousnessEjjj:
		Dispatch 22
