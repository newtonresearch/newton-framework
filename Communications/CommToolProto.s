/*
	File:		CCommToolProto.s

	Contains:	CCommToolProtocol p-class protocol interface.

	Written by:	Newton Research Group, 2016.
*/

#include "../Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN17CCommToolProtocol4makeEPKc
		.globl	__ZN17CCommToolProtocol7destroyEv
		.globl	__ZN17CCommToolProtocol4initEPv

		.text
		.align	2

CCommToolProtocol_name:
		.asciz	"CCommToolProtocol"
		.align	2

__ZN17CCommToolProtocol4makeEPKc:
		New CCommToolProtocol_name
		Dispatch 2
__ZN17CCommToolProtocol7destroyEv:
		Delete 3
__ZN17CCommToolProtocol4initEPv:
		Dispatch 4

