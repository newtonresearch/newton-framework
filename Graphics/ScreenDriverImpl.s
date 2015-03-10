/*
	File:		ScreenDriverImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2010.
*/

#include "../Protocols/ProtoMacro.s"

		.const
		.align	2

/* ----------------------------------------------------------------
	CMainScreenDriver implementation class info.  00796564..007977FC
---------------------------------------------------------------- */

		.globl	__ZN18CMainDisplayDriver9classInfoEv
__ZN18CMainDisplayDriver9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN18CMainDisplayDriver6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN18CMainDisplayDriver4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN18CMainDisplayDriver7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CMainDisplayDriver"
2:		.asciz	"CScreenDriver"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN18CMainDisplayDriver9classInfoEv
		BRA		__ZN18CMainDisplayDriver4makeEv
		BRA		__ZN18CMainDisplayDriver7destroyEv
		BRA		__ZN18CMainDisplayDriver11screenSetupEv
		BRA		__ZN18CMainDisplayDriver13getScreenInfoEP10ScreenInfo
		BRA		__ZN18CMainDisplayDriver9powerInitEv
		BRA		__ZN18CMainDisplayDriver7powerOnEv
		BRA		__ZN18CMainDisplayDriver8powerOffEv
		BRA		__ZN18CMainDisplayDriver4blitEP8PixelMapP4RectS3_i
		BRA		__ZN18CMainDisplayDriver10doubleBlitEP8PixelMapS1_P4RectS3_i
		BRA		__ZN18CMainDisplayDriver10getFeatureEi
		BRA		__ZN18CMainDisplayDriver10setFeatureEii
		BRA		__ZN18CMainDisplayDriver18autoAdjustFeaturesEv
		BRA		__ZN18CMainDisplayDriver13enterIdleModeEv
		BRA		__ZN18CMainDisplayDriver12exitIdleModeEv

5:									// monitor code goes here

6:		RetNil
