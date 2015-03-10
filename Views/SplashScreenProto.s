/*
	File:		ScreenDriverProto.s

	Contains:	CScreenDriver p-class protocol interfaces.

	Written by:	Newton Research Group, 2010.
*/

#include "ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN13CScreenDriver4makeEPKc
		.globl	__ZN13CScreenDriver7destroyEv
		.globl	__ZN13CScreenDriver11screenSetupEv
		.globl	__ZN13CScreenDriver13getScreenInfoEP10ScreenInfo
		.globl	__ZN13CScreenDriver9powerInitEv
		.globl	__ZN13CScreenDriver7powerOnEv
		.globl	__ZN13CScreenDriver8powerOffEv
		.globl	__ZN13CScreenDriver4blitEP8PixelMapP4RectP4Rectl
		.globl	__ZN13CScreenDriver10doubleBlitEP8PixelMapP8PixelMapP4RectP4Rectl
		.globl	__ZN13CScreenDriver10getFeatureEi
		.globl	__ZN13CScreenDriver10setFeatureEil
		.globl	__ZN13CScreenDriver18autoAdjustFeaturesEv
		.globl	__ZN13CScreenDriver13enterIdleModeEv
		.globl	__ZN13CScreenDriver12exitIdleModeEv

		.text
		.align	2

CScreenDriver_name:
		Name		"CScreenDriver"

__ZN13CScreenDriver4makeEPKc:
		New CScreenDriver_name
		Dispatch 2
__ZN13CScreenDriver7destroyEv:
		Delete 3
__ZN13CScreenDriver11screenSetupEv:
		Dispatch 4
__ZN13CScreenDriver13getScreenInfoEP10ScreenInfo:
		Dispatch 5
__ZN13CScreenDriver9powerInitEv:
		Dispatch 6
__ZN13CScreenDriver7powerOnEv:
		Dispatch 7
__ZN13CScreenDriver8powerOffEv:
		Dispatch 8
__ZN13CScreenDriver4blitEP8PixelMapP4RectP4Rectl:
		Dispatch 9
__ZN13CScreenDriver10doubleBlitEP8PixelMapP8PixelMapP4RectP4Rectl:
		Dispatch 10
__ZN13CScreenDriver10getFeatureEi:
		Dispatch 11
__ZN13CScreenDriver10setFeatureEil:
		Dispatch 12
__ZN13CScreenDriver18autoAdjustFeaturesEv:
		Dispatch 13
__ZN13CScreenDriver13enterIdleModeEv:
		Dispatch 14
__ZN13CScreenDriver12exitIdleModeEv:
		Dispatch 15
