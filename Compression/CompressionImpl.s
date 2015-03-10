/*
	File:		CompressionImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2009.
*/

#include "../Protocols/ProtoMacro.s"

		.const
		.align	2

/* ----------------------------------------------------------------
	CZippyDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN18CZippyDecompressor9classInfoEv
__ZN18CZippyDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN18CZippyDecompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN18CZippyDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN18CZippyDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CZippyDecompressor"
2:		.asciz	"CDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN18CZippyDecompressor9classInfoEv
		BRA		__ZN18CZippyDecompressor4makeEv
		BRA		__ZN18CZippyDecompressor7destroyEv
		BRA		__ZN18CZippyDecompressor4initEPv
		BRA		__ZN18CZippyDecompressor10decompressEPmPvmS1_m

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CLZCompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN13CLZCompressor9classInfoEv
__ZN13CLZCompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN13CLZCompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN13CLZCompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN13CLZCompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CLZCompressor"
2:		.asciz	"CCompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN13CLZCompressor9classInfoEv
		BRA		__ZN13CLZCompressor4makeEv
		BRA		__ZN13CLZCompressor7destroyEv
		BRA		__ZN13CLZCompressor4initEPv
		BRA		__ZN13CLZCompressor8compressEPmPvmS1_m
		BRA		__ZN13CLZCompressor23estimatedCompressedSizeEPvm

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CLZDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN15CLZDecompressor9classInfoEv
__ZN15CLZDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN15CLZDecompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN15CLZDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN15CLZDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CLZDecompressor"
2:		.asciz	"CDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN15CLZDecompressor9classInfoEv
		BRA		__ZN15CLZDecompressor4makeEv
		BRA		__ZN15CLZDecompressor7destroyEv
		BRA		__ZN15CLZDecompressor4initEPv
		BRA		__ZN15CLZDecompressor10decompressEPmPvmS1_m

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CUnicodeCompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN18CUnicodeCompressor9classInfoEv
__ZN18CUnicodeCompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN18CUnicodeCompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN18CUnicodeCompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN18CUnicodeCompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CUnicodeCompressor"
2:		.asciz	"CCallbackCompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN18CUnicodeCompressor9classInfoEv
		BRA		__ZN18CUnicodeCompressor4makeEv
		BRA		__ZN18CUnicodeCompressor7destroyEv
		BRA		__ZN18CUnicodeCompressor4initEPv
		BRA		__ZN18CUnicodeCompressor5resetEv
		BRA		__ZN18CUnicodeCompressor10writeChunkEPvm
		BRA		__ZN18CUnicodeCompressor5flushEv

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CUnicodeDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN20CUnicodeDecompressor9classInfoEv
__ZN20CUnicodeDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN20CUnicodeDecompressor6sizeOfEv		// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN20CUnicodeDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN20CUnicodeDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CUnicodeDecompressor"
2:		.asciz	"CCallbackDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN20CUnicodeDecompressor9classInfoEv
		BRA		__ZN20CUnicodeDecompressor4makeEv
		BRA		__ZN20CUnicodeDecompressor7destroyEv
		BRA		__ZN20CUnicodeDecompressor4initEPv
		BRA		__ZN20CUnicodeDecompressor5resetEv
		BRA		__ZN20CUnicodeDecompressor9readChunkEPvPmPa

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CPixelMapCompander implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN18CPixelMapCompander9classInfoEv
__ZN18CPixelMapCompander9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN18CPixelMapCompander6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN18CPixelMapCompander4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN18CPixelMapCompander7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CPixelMapCompander"
2:		.asciz	"CStoreCompander"
3:		.asciz	"CLZDecompressor", ""
		.asciz	"CLZCompressor", ""
		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN18CPixelMapCompander9classInfoEv
		BRA		__ZN18CPixelMapCompander4makeEv
		BRA		__ZN18CPixelMapCompander7destroyEv
		BRA		__ZN18CPixelMapCompander4initEP6CStorejjbb
		BRA		__ZN18CPixelMapCompander9blockSizeEv
		BRA		__ZN18CPixelMapCompander4readEmPcmm
		BRA		__ZN18CPixelMapCompander5writeEmPcmm
		BRA		__ZN18CPixelMapCompander20doTransactionAgainstEij
		BRA		__ZN18CPixelMapCompander10isReadOnlyEv

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CStoreCompanderWrapper implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN22CStoreCompanderWrapper9classInfoEv
__ZN22CStoreCompanderWrapper9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN22CStoreCompanderWrapper6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN22CStoreCompanderWrapper4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN22CStoreCompanderWrapper7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CStoreCompanderWrapper"
2:		.asciz	"CStoreCompander"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN22CStoreCompanderWrapper9classInfoEv
		BRA		__ZN22CStoreCompanderWrapper4makeEv
		BRA		__ZN22CStoreCompanderWrapper7destroyEv
		BRA		__ZN22CStoreCompanderWrapper4initEP6CStorejjbb
		BRA		__ZN22CStoreCompanderWrapper9blockSizeEv
		BRA		__ZN22CStoreCompanderWrapper4readEmPcmm
		BRA		__ZN22CStoreCompanderWrapper5writeEmPcmm
		BRA		__ZN22CStoreCompanderWrapper20doTransactionAgainstEij
		BRA		__ZN22CStoreCompanderWrapper10isReadOnlyEv

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CSimpleStoreDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN24CSimpleStoreDecompressor9classInfoEv
__ZN24CSimpleStoreDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN24CSimpleStoreDecompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN24CSimpleStoreDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN24CSimpleStoreDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CSimpleStoreDecompressor"
2:		.asciz	"CStoreDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN24CSimpleStoreDecompressor9classInfoEv
		BRA		__ZN24CSimpleStoreDecompressor4makeEv
		BRA		__ZN24CSimpleStoreDecompressor7destroyEv
		BRA		__ZN24CSimpleStoreDecompressor4initEP6CStorejPc
		BRA		__ZN24CSimpleStoreDecompressor4readEjPcmm

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CSimpleRelocStoreDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN29CSimpleRelocStoreDecompressor9classInfoEv
__ZN29CSimpleRelocStoreDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN29CSimpleRelocStoreDecompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN29CSimpleRelocStoreDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN29CSimpleRelocStoreDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CSimpleRelocStoreDecompressor"
2:		.asciz	"CStoreDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN29CSimpleRelocStoreDecompressor9classInfoEv
		BRA		__ZN29CSimpleRelocStoreDecompressor4makeEv
		BRA		__ZN29CSimpleRelocStoreDecompressor7destroyEv
		BRA		__ZN29CSimpleRelocStoreDecompressor4initEP6CStorejPc
		BRA		__ZN29CSimpleRelocStoreDecompressor4readEjPcmm

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CSimpleStoreCompander implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN21CSimpleStoreCompander9classInfoEv
__ZN21CSimpleStoreCompander9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN21CSimpleStoreCompander6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN21CSimpleStoreCompander4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN21CSimpleStoreCompander7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CSimpleStoreCompander"
2:		.asciz	"CStoreCompander"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN21CSimpleStoreCompander9classInfoEv
		BRA		__ZN21CSimpleStoreCompander4makeEv
		BRA		__ZN21CSimpleStoreCompander7destroyEv
		BRA		__ZN21CSimpleStoreCompander4initEP6CStorejjb
		BRA		__ZN21CSimpleStoreCompander9blockSizeEv
		BRA		__ZN21CSimpleStoreCompander4readEmPcmm
		BRA		__ZN21CSimpleStoreCompander5writeEmPcmm
		BRA		__ZN21CSimpleStoreCompander20doTransactionAgainstEij
		BRA		__ZN21CSimpleStoreCompander10isReadOnlyEv

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CZippyStoreDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN23CZippyStoreDecompressor9classInfoEv
__ZN23CZippyStoreDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN23CZippyStoreDecompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN23CZippyStoreDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN23CZippyStoreDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CZippyStoreDecompressor"
2:		.asciz	"CStoreDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN23CZippyStoreDecompressor9classInfoEv
		BRA		__ZN23CZippyStoreDecompressor4makeEv
		BRA		__ZN23CZippyStoreDecompressor7destroyEv
		BRA		__ZN23CZippyStoreDecompressor4initEP6CStorejPc
		BRA		__ZN23CZippyStoreDecompressor4readEjPcmm

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CZippyRelocStoreDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN28CZippyRelocStoreDecompressor9classInfoEv
__ZN28CZippyRelocStoreDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN28CZippyRelocStoreDecompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN28CZippyRelocStoreDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN28CZippyRelocStoreDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CZippyRelocStoreDecompressor"
2:		.asciz	"CStoreDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN28CZippyRelocStoreDecompressor9classInfoEv
		BRA		__ZN28CZippyRelocStoreDecompressor4makeEv
		BRA		__ZN28CZippyRelocStoreDecompressor7destroyEv
		BRA		__ZN28CZippyRelocStoreDecompressor4initEP6CStorejPc
		BRA		__ZN28CZippyRelocStoreDecompressor4readEjPcmm

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CLZStoreDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN20CLZStoreDecompressor9classInfoEv
__ZN20CLZStoreDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN20CLZStoreDecompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN20CLZStoreDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN20CLZStoreDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CLZStoreDecompressor"
2:		.asciz	"CStoreDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN20CLZStoreDecompressor9classInfoEv
		BRA		__ZN20CLZStoreDecompressor4makeEv
		BRA		__ZN20CLZStoreDecompressor7destroyEv
		BRA		__ZN20CLZStoreDecompressor4initEP6CStorejPc
		BRA		__ZN20CLZStoreDecompressor4readEjPcmm

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CLZRelocStoreDecompressor implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN25CLZRelocStoreDecompressor9classInfoEv
__ZN25CLZRelocStoreDecompressor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN25CLZRelocStoreDecompressor6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN25CLZRelocStoreDecompressor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN25CLZRelocStoreDecompressor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CLZRelocStoreDecompressor"
2:		.asciz	"CStoreDecompressor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN25CLZRelocStoreDecompressor9classInfoEv
		BRA		__ZN25CLZRelocStoreDecompressor4makeEv
		BRA		__ZN25CLZRelocStoreDecompressor7destroyEv
		BRA		__ZN25CLZRelocStoreDecompressor4initEP6CStorejPc
		BRA		__ZN25CLZRelocStoreDecompressor4readEjPcmm

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CLZStoreCompander implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN17CLZStoreCompander9classInfoEv
__ZN17CLZStoreCompander9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN17CLZStoreCompander6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN17CLZStoreCompander4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN17CLZStoreCompander7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CLZStoreCompander"
2:		.asciz	"CStoreCompander"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN17CLZStoreCompander9classInfoEv
		BRA		__ZN17CLZStoreCompander4makeEv
		BRA		__ZN17CLZStoreCompander7destroyEv
		BRA		__ZN17CLZStoreCompander4initEP6CStorejjbb
		BRA		__ZN17CLZStoreCompander9blockSizeEv
		BRA		__ZN17CLZStoreCompander4readEmPcmm
		BRA		__ZN17CLZStoreCompander5writeEmPcmm
		BRA		__ZN17CLZStoreCompander20doTransactionAgainstEij
		BRA		__ZN17CLZStoreCompander10isReadOnlyEv

5:									// monitor code goes here

6:		RetNil

