/*
	File:		StoreMonitorImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2014.
*/

#include "../Protocols/ProtoMacro.s"

/* ----------------------------------------------------------------
	CMuxStoreMonitor implementation class info.
---------------------------------------------------------------- */

		.const
		.align	2

		.globl	__ZN16CMuxStoreMonitor9classInfoEv
__ZN16CMuxStoreMonitor9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN16CMuxStoreMonitor6sizeOfEv		// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN16CMuxStoreMonitor4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN16CMuxStoreMonitor7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		BRAZ		6f				// branch to bail-out function (that returns nil now)

1:		.asciz	"CMuxStoreMonitor"
2:		.asciz	"CStoreMonitor"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN16CMuxStoreMonitor9classInfoEv
		BRA		__ZN16CMuxStoreMonitor4makeEv
		BRA		__ZN16CMuxStoreMonitor7destroyEv
		BRA		__ZN16CMuxStoreMonitor4initEP6CStore
		BRA		__ZN16CMuxStoreMonitor11needsFormatEPb
		BRA		__ZN16CMuxStoreMonitor6formatEv
		BRA		__ZN16CMuxStoreMonitor9getRootIdEPj
		BRA		__ZN16CMuxStoreMonitor9newObjectEPjm
		BRA		__ZN16CMuxStoreMonitor11eraseObjectEj
		BRA		__ZN16CMuxStoreMonitor12deleteObjectEj
		BRA		__ZN16CMuxStoreMonitor13setObjectSizeEjm
		BRA		__ZN16CMuxStoreMonitor13getObjectSizeEjPm
		BRA		__ZN16CMuxStoreMonitor5writeEjmPvm
		BRA		__ZN16CMuxStoreMonitor4readEjmPvm
		BRA		__ZN16CMuxStoreMonitor12getStoreSizeEPmS0_
		BRA		__ZN16CMuxStoreMonitor10isReadOnlyEPb
		BRA		__ZN16CMuxStoreMonitor9lockStoreEv
		BRA		__ZN16CMuxStoreMonitor11unlockStoreEv
		BRA		__ZN16CMuxStoreMonitor5abortEv
		BRA		__ZN16CMuxStoreMonitor4idleEPbS0_
		BRA		__ZN16CMuxStoreMonitor10nextObjectEjPj
		BRA		__ZN16CMuxStoreMonitor14checkIntegrityEPj
		BRA		__ZN16CMuxStoreMonitor20newWithinTransactionEPjm
		BRA		__ZN16CMuxStoreMonitor23startTransactionAgainstEj
		BRA		__ZN16CMuxStoreMonitor15separatelyAbortEj
		BRA		__ZN16CMuxStoreMonitor23addToCurrentTransactionEj
		BRA		__ZN16CMuxStoreMonitor12lockReadOnlyEv
		BRA		__ZN16CMuxStoreMonitor14unlockReadOnlyEb
		BRA		__ZN16CMuxStoreMonitor9newObjectEPjPvm
		BRA		__ZN16CMuxStoreMonitor13replaceObjectEjPvm
		BRA		__ZN16CMuxStoreMonitor12newXIPObjectEPjm

5:									// monitor code goes here

6:		RetNil
