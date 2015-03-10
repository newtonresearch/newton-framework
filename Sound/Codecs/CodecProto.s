/*
	File:		CodecProto.s

	Contains:	CSoundCodec p-class protocol interface.

	Written by:	Newton Research Group, 2009.
*/

#include "../../Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN11CSoundCodec4makeEPKc
		.globl	__ZN11CSoundCodec7destroyEv
		.globl	__ZN11CSoundCodec4initEP10CodecBlock
		.globl	__ZN11CSoundCodec5resetEP10CodecBlock
		.globl	__ZN11CSoundCodec7produceEPvPmS1_P10CodecBlock
		.globl	__ZN11CSoundCodec7consumeEPKvPmS2_PK10CodecBlock
		.globl	__ZN11CSoundCodec5startEv
		.globl	__ZN11CSoundCodec4stopEi
		.globl	__ZN11CSoundCodec15bufferCompletedEv

		.text
		.align	2

CSoundCodec_name:
		.asciz	"CSoundCodec"
		.align	2

__ZN11CSoundCodec4makeEPKc:
		New CSoundCodec_name
		Dispatch 2
__ZN11CSoundCodec7destroyEv:
		Delete 3
__ZN11CSoundCodec4initEP10CodecBlock:
		Dispatch 4
__ZN11CSoundCodec5resetEP10CodecBlock:
		Dispatch 5
__ZN11CSoundCodec7produceEPvPmS1_P10CodecBlock:
		Dispatch 6
__ZN11CSoundCodec7consumeEPKvPmS2_PK10CodecBlock:
		Dispatch 7
__ZN11CSoundCodec5startEv:
		Dispatch 8
__ZN11CSoundCodec4stopEi:
		Dispatch 9
__ZN11CSoundCodec15bufferCompletedEv:
		Dispatch 10
