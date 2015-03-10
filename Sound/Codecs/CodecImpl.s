/*
	File:		CodecImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2009.
*/

#include "../../Protocols/ProtoMacro.s"

		.const
		.align	2

/* ----------------------------------------------------------------
	CDTMFCodec implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN10CDTMFCodec9classInfoEv
__ZN10CDTMFCodec9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN10CDTMFCodec6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN10CDTMFCodec4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN10CDTMFCodec7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CDTMFCodec"
2:		.asciz	"CSoundCodec"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN10CDTMFCodec9classInfoEv
		BRA		__ZN10CDTMFCodec4makeEv
		BRA		__ZN10CDTMFCodec7destroyEv
		BRA		__ZN10CDTMFCodec4initEP10CodecBlock
		BRA		__ZN10CDTMFCodec5resetEP10CodecBlock
		BRA		__ZN10CDTMFCodec7produceEPvPmS1_P10CodecBlock
		BRA		__ZN10CDTMFCodec7consumeEPKvPmS2_PK10CodecBlock
		BRA		__ZN10CDTMFCodec5startEv
		BRA		__ZN10CDTMFCodec4stopEi
		BRA		__ZN10CDTMFCodec15bufferCompletedEv

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CGSMCodec implementation class info.
---------------------------------------------------------------- */
#if 0
		.globl	__ZN9CGSMCodec9classInfoEv
__ZN9CGSMCodec9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN9CGSMCodec6sizeOfEv		// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN9CGSMCodec4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN9CGSMCodec7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CGSMCodec"
2:		.asciz	"CSoundCodec"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN9CGSMCodec9classInfoEv
		BRA		__ZN9CGSMCodec4makeEv
		BRA		__ZN9CGSMCodec7destroyEv
		BRA		__ZN9CGSMCodec4initEP10CodecBlock
		BRA		__ZN9CGSMCodec5resetEP10CodecBlock
		BRA		__ZN9CGSMCodec7produceEPvPmS1_P10CodecBlock
		BRA		__ZN9CGSMCodec7consumeEPKvPmS2_P10CodecBlock
		BRA		__ZN9CGSMCodec5startEv
		BRA		__ZN9CGSMCodec4stopEi
		BRA		__ZN9CGSMCodec15bufferCompletedEv

5:									// monitor code goes here

6:		RetNil
#endif
