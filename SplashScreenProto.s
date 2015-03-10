/*
	File:		SplashScreenProto.s

	Contains:	CSplashScreenInfo and CVersionString p-class protocol interfaces.

	Written by:	Newton Research Group, 2010.
*/

#include "Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN17CSplashScreenInfo4makeEPKc
		.globl	__ZN17CSplashScreenInfo7destroyEv
		.globl	__ZN17CSplashScreenInfo7getBitsEPP12PictureShape
		.globl	__ZN17CSplashScreenInfo7getTextEPt

		.text
		.align	2

CSplashScreenInfo_name:
		.asciz	"CSplashScreenInfo"
		.align	2

__ZN17CSplashScreenInfo4makeEPKc:
		New CSplashScreenInfo_name
		Dispatch 2
__ZN17CSplashScreenInfo7destroyEv:
		Delete 3
__ZN17CSplashScreenInfo7getBitsEPP12PictureShape:
		Dispatch 4
__ZN17CSplashScreenInfo7getTextEPt:
		Dispatch 5

/* ---------------------------------------------------------------- */

		.globl	__ZN14CVersionString4makeEPKc
		.globl	__ZN14CVersionString7destroyEv
		.globl	__ZN14CVersionString13versionStringEPt

		.text
		.align	2

CVersionString_name:
		.asciz	"CVersionString"
		.align	2

__ZN14CVersionString4makeEPKc:
		New CSplashScreenInfo_name
		Dispatch 2
__ZN14CVersionString7destroyEv:
		Delete 3
__ZN14CVersionString13versionStringEPt:
		Dispatch 4
