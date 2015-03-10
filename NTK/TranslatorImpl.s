/*
	File:		TranslatorImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2009.
*/

#include "Protocols/ProtoMacro.s"

		.const
		.align	2

/* ----------------------------------------------------------------
	PHammerInTranslator implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN19PHammerInTranslator9classInfoEv
__ZN19PHammerInTranslator9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN19PHammerInTranslator6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN19PHammerInTranslator4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN19PHammerInTranslator7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"PHammerInTranslator"
2:		.asciz	"PInTranslator"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN19PHammerInTranslator9classInfoEv
		BRA		__ZN19PHammerInTranslator4makeEv
		BRA		__ZN19PHammerInTranslator7destroyEv
		BRA		__ZN19PHammerInTranslator4initEPv
		BRA		__ZN19PHammerInTranslator4idleEv
		BRA		__ZN19PHammerInTranslator14frameAvailableEv
		BRA		__ZN19PHammerInTranslator12produceFrameEi

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	PHammerOutTranslator implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN20PHammerOutTranslator9classInfoEv
__ZN20PHammerOutTranslator9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN20PHammerOutTranslator6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN20PHammerOutTranslator4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN20PHammerOutTranslator7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"PHammerOutTranslator"
2:		.asciz	"POutTranslator"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN20PHammerOutTranslator9classInfoEv
		BRA		__ZN20PHammerOutTranslator4makeEv
		BRA		__ZN20PHammerOutTranslator7destroyEv
		BRA		__ZN20PHammerOutTranslator4initEPv
		BRA		__ZN20PHammerOutTranslator4idleEv
		BRA		__ZN20PHammerOutTranslator12consumeFrameERK6RefVarii
		BRA		__ZN20PHammerOutTranslator6promptEi
		BRA		__ZN20PHammerOutTranslator5printEPKcz
		BRA		__ZN20PHammerOutTranslator6vprintEPKcP13__va_list_tag
		BRA		__ZN20PHammerOutTranslator4putcEi
		BRA		__ZN20PHammerOutTranslator5flushEv
		BRA		__ZN20PHammerOutTranslator14enterBreakLoopEi
		BRA		__ZN20PHammerOutTranslator13exitBreakLoopEv
		BRA		__ZN20PHammerOutTranslator10stackTraceEPv
		BRA		__ZN20PHammerOutTranslator15exceptionNotifyEP9Exception

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	PNullInTranslator implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN17PNullInTranslator9classInfoEv
__ZN17PNullInTranslator9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN17PNullInTranslator6sizeOfEv		// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN17PNullInTranslator4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN17PNullInTranslator7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"PNullInTranslator"
2:		.asciz	"PInTranslator"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN17PNullInTranslator9classInfoEv
		BRA		__ZN17PNullInTranslator4makeEv
		BRA		__ZN17PNullInTranslator7destroyEv
		BRA		__ZN17PNullInTranslator4initEPv
		BRA		__ZN17PNullInTranslator4idleEv
		BRA		__ZN17PNullInTranslator14frameAvailableEv
		BRA		__ZN17PNullInTranslator12produceFrameEi

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	PNullOutTranslator implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN18PNullOutTranslator9classInfoEv
__ZN18PNullOutTranslator9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN18PNullOutTranslator6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN18PNullOutTranslator4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN18PNullOutTranslator7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"PNullOutTranslator"
2:		.asciz	"POutTranslator"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN18PNullOutTranslator9classInfoEv
		BRA		__ZN18PNullOutTranslator4makeEv
		BRA		__ZN18PNullOutTranslator7destroyEv
		BRA		__ZN18PNullOutTranslator4initEPv
		BRA		__ZN18PNullOutTranslator4idleEv
		BRA		__ZN18PNullOutTranslator12consumeFrameERK6RefVarii
		BRA		__ZN18PNullOutTranslator6promptEi
		BRA		__ZN18PNullOutTranslator5printEPKcz
		BRA		__ZN18PNullOutTranslator6vprintEPKcP13__va_list_tag
		BRA		__ZN18PNullOutTranslator4putcEi
		BRA		__ZN18PNullOutTranslator5flushEv
		BRA		__ZN18PNullOutTranslator14enterBreakLoopEi
		BRA		__ZN18PNullOutTranslator13exitBreakLoopEv
		BRA		__ZN18PNullOutTranslator10stackTraceEPv
		BRA		__ZN18PNullOutTranslator15exceptionNotifyEP9Exception

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	PStdioInTranslator implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN18PStdioInTranslator9classInfoEv
__ZN18PStdioInTranslator9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN18PStdioInTranslator6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN18PStdioInTranslator4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN18PStdioInTranslator7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"PStdioInTranslator"
2:		.asciz	"PInTranslator"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN18PStdioInTranslator9classInfoEv
		BRA		__ZN18PStdioInTranslator4makeEv
		BRA		__ZN18PStdioInTranslator7destroyEv
		BRA		__ZN18PStdioInTranslator4initEPv
		BRA		__ZN18PStdioInTranslator4idleEv
		BRA		__ZN18PStdioInTranslator14frameAvailableEv
		BRA		__ZN18PStdioInTranslator12produceFrameEi

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	PStdioOutTranslator implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN19PStdioOutTranslator9classInfoEv
__ZN19PStdioOutTranslator9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN19PStdioOutTranslator6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN19PStdioOutTranslator4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN19PStdioOutTranslator7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"PStdioOutTranslator"
2:		.asciz	"POutTranslator"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN19PStdioOutTranslator9classInfoEv
		BRA		__ZN19PStdioOutTranslator4makeEv
		BRA		__ZN19PStdioOutTranslator7destroyEv
		BRA		__ZN19PStdioOutTranslator4initEPv
		BRA		__ZN19PStdioOutTranslator4idleEv
		BRA		__ZN19PStdioOutTranslator12consumeFrameERK6RefVarii
		BRA		__ZN19PStdioOutTranslator6promptEi
		BRA		__ZN19PStdioOutTranslator5printEPKcz
		BRA		__ZN19PStdioOutTranslator6vprintEPKcP13__va_list_tag
		BRA		__ZN19PStdioOutTranslator4putcEi
		BRA		__ZN19PStdioOutTranslator5flushEv
		BRA		__ZN19PStdioOutTranslator14enterBreakLoopEi
		BRA		__ZN19PStdioOutTranslator13exitBreakLoopEv
		BRA		__ZN19PStdioOutTranslator10stackTraceEPv
		BRA		__ZN19PStdioOutTranslator15exceptionNotifyEP9Exception

5:									// monitor code goes here

6:		RetNil

#if 0
/* ----------------------------------------------------------------
	PSerialInTranslator implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN19PSerialInTranslator9classInfoEv
__ZN19PSerialInTranslator9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN19PSerialInTranslator6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN19PSerialInTranslator4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN19PSerialInTranslator7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"PSerialInTranslator"
2:		.asciz	"PInTranslator"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN19PSerialInTranslator9classInfoEv
		BRA		__ZN19PSerialInTranslator4makeEv
		BRA		__ZN19PSerialInTranslator7destroyEv
		BRA		__ZN19PSerialInTranslator4initEPv
		BRA		__ZN19PSerialInTranslator4idleEv
		BRA		__ZN19PSerialInTranslator14frameAvailableEv
		BRA		__ZN19PSerialInTranslator12produceFrameEi

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	PSerialOutTranslator implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN20PSerialOutTranslator9classInfoEv
__ZN20PSerialOutTranslator9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN20PSerialOutTranslator6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN20PSerialOutTranslator4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN20PSerialOutTranslator7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"PSerialOutTranslator"
2:		.asciz	"POutTranslator"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN20PSerialOutTranslator9classInfoEv
		BRA		__ZN20PSerialOutTranslator4makeEv
		BRA		__ZN20PSerialOutTranslator7destroyEv
		BRA		__ZN20PSerialOutTranslator4initEPv
		BRA		__ZN20PSerialOutTranslator4idleEv
		BRA		__ZN20PSerialOutTranslator12consumeFrameERK6RefVarii
		BRA		__ZN20PSerialOutTranslator6promptEi
		BRA		__ZN20PSerialOutTranslator5printEPKcz
		BRA		__ZN20PSerialOutTranslator6vprintEPKcPP13__va_list_tag
		BRA		__ZN20PSerialOutTranslator4putcEi
		BRA		__ZN20PSerialOutTranslator5flushEv
		BRA		__ZN20PSerialOutTranslator14enterBreakLoopEi
		BRA		__ZN20PSerialOutTranslator13exitBreakLoopEv
		BRA		__ZN20PSerialOutTranslator10stackTraceEPv
		BRA		__ZN20PSerialOutTranslator15exceptionNotifyEP9Exception

5:									// monitor code goes here

6:		RetNil
#endif
