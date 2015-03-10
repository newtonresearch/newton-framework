/*
	File:		TranslatorProto.s

	Contains:	PInTranslator and POutTranslator p-class protocol interfaces.

	Written by:	Newton Research Group, 2009.
*/

#include "Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN13PInTranslator4makeEPKc
		.globl	__ZN13PInTranslator7destroyEv
		.globl	__ZN13PInTranslator4initEPv
		.globl	__ZN13PInTranslator4idleEv
		.globl	__ZN13PInTranslator14frameAvailableEv
		.globl	__ZN13PInTranslator12produceFrameEi

		.text
		.align	2

PInTranslator_name:
		.asciz	"PInTranslator"
		.align	2

__ZN13PInTranslator4makeEPKc:
		New PInTranslator_name
		Dispatch 2
__ZN13PInTranslator7destroyEv:
		Delete 3
__ZN13PInTranslator4initEPv:
		Dispatch 4
__ZN13PInTranslator4idleEv:
		Dispatch 5
__ZN13PInTranslator14frameAvailableEv:
		Dispatch 6
__ZN13PInTranslator12produceFrameEi:
		Dispatch 7

/* ---------------------------------------------------------------- */

		.globl	__ZN14POutTranslator4makeEPKc
		.globl	__ZN14POutTranslator7destroyEv
		.globl	__ZN14POutTranslator4initEPv
		.globl	__ZN14POutTranslator4idleEv
		.globl	__ZN14POutTranslator12consumeFrameERK6RefVarii
		.globl	__ZN14POutTranslator6promptEi
		.globl	__ZN14POutTranslator5printEPKcz
		.globl	__ZN14POutTranslator6vprintEPKcP13__va_list_tag
		.globl	__ZN14POutTranslator4putcEi
		.globl	__ZN14POutTranslator5flushEv
		.globl	__ZN14POutTranslator14enterBreakLoopEi
		.globl	__ZN14POutTranslator13exitBreakLoopEv
		.globl	__ZN14POutTranslator10stackTraceEPv
		.globl	__ZN14POutTranslator15exceptionNotifyEP9Exception

		.text
		.align	2

POutTranslator_name:
		.asciz	"POutTranslator"
		.align	2

__ZN14POutTranslator4makeEPKc:
		New POutTranslator_name
		Dispatch 2
__ZN14POutTranslator7destroyEv:
		Delete 3
__ZN14POutTranslator4initEPv:
		Dispatch 4
__ZN14POutTranslator4idleEv:
		Dispatch 5
__ZN14POutTranslator12consumeFrameERK6RefVarii:
		Dispatch 6
__ZN14POutTranslator6promptEi:
		Dispatch 7
__ZN14POutTranslator5printEPKcz:
		Dispatch 8
__ZN14POutTranslator6vprintEPKcP13__va_list_tag:
		Dispatch 9
__ZN14POutTranslator4putcEi:
		Dispatch 10
__ZN14POutTranslator5flushEv:
		Dispatch 11
__ZN14POutTranslator14enterBreakLoopEi:
		Dispatch 12
__ZN14POutTranslator13exitBreakLoopEv:
		Dispatch 13
__ZN14POutTranslator10stackTraceEPv:
		Dispatch 14
__ZN14POutTranslator15exceptionNotifyEP9Exception:
		Dispatch 15

