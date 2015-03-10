/*
	File:		TabletDriverImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2010.
*/

#include "../Protocols/ProtoMacro.s"

		.const
		.align	2

/* ----------------------------------------------------------------
	CResistiveTablet implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN16CResistiveTablet9classInfoEv
__ZN16CResistiveTablet9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN16CResistiveTablet6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN16CResistiveTablet4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN16CResistiveTablet7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CResistiveTabletDriver"
2:		.asciz	"CTabletDriver"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN16CResistiveTablet9classInfoEv
		BRA		__ZN16CResistiveTablet4makeEv
		BRA		__ZN16CResistiveTablet7destroyEv
		BRA		__ZN16CResistiveTablet4initERK4Rect
		BRA		__ZN16CResistiveTablet6wakeUpEv
		BRA		__ZN16CResistiveTablet8shutDownEv
		BRA		__ZN16CResistiveTablet10tabletIdleEv
		BRA		__ZN16CResistiveTablet13getSampleRateEv
		BRA		__ZN16CResistiveTablet13setSampleRateEj
		BRA		__ZN16CResistiveTablet20getTabletCalibrationEP11Calibration
		BRA		__ZN16CResistiveTablet20setTabletCalibrationERK11Calibration
		BRA		__ZN16CResistiveTablet19setDoingCalibrationEaPj
		BRA		__ZN16CResistiveTablet19getTabletResolutionEPfS0_
		BRA		__ZN16CResistiveTablet14setOrientationEi
		BRA		__ZN16CResistiveTablet14getTabletStateEv
		BRA		__ZN16CResistiveTablet19getFingerInputStateEPa
		BRA		__ZN16CResistiveTablet19setFingerInputStateEa
		BRA		__ZN16CResistiveTablet28recalibrateTabletAfterRotateEv
		BRA		__ZN16CResistiveTablet24tabletNeedsRecalibrationEv
		BRA		__ZN16CResistiveTablet17startBypassTabletEv
		BRA		__ZN16CResistiveTablet16stopBypassTabletEv
		BRA		__ZN16CResistiveTablet27returnTabletToConsciousnessEjjj

5:									// monitor code goes here

6:		RetNil
