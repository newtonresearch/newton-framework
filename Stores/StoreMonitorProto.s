/*
	File:		StoreMonitorProto.s

	Contains:	CStoreMonitor p-class protocol interface.

	Written by:	Newton Research Group, 20014.
*/

#include "../Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN13CStoreMonitor4makeEPKc
		.globl	__ZN13CStoreMonitor7destroyEv
		.globl	__ZN13CStoreMonitor4initEP6CStore
		.globl	__ZN13CStoreMonitor11needsFormatEPb
		.globl	__ZN13CStoreMonitor6formatEv
		.globl	__ZN13CStoreMonitor9getRootIdEPj
		.globl	__ZN13CStoreMonitor9newObjectEPjm
		.globl	__ZN13CStoreMonitor11eraseObjectEj
		.globl	__ZN13CStoreMonitor12deleteObjectEj
		.globl	__ZN13CStoreMonitor13setObjectSizeEjm
		.globl	__ZN13CStoreMonitor13getObjectSizeEjPm
		.globl	__ZN13CStoreMonitor5writeEjmPvm
		.globl	__ZN13CStoreMonitor4readEjmPvm
		.globl	__ZN13CStoreMonitor12getStoreSizeEPmS0_
		.globl	__ZN13CStoreMonitor10isReadOnlyEPb
		.globl	__ZN13CStoreMonitor9lockStoreEv
		.globl	__ZN13CStoreMonitor11unlockStoreEv
		.globl	__ZN13CStoreMonitor5abortEv
		.globl	__ZN13CStoreMonitor4idleEPbS0_
		.globl	__ZN13CStoreMonitor10nextObjectEjPj
		.globl	__ZN13CStoreMonitor14checkIntegrityEPj
		.globl	__ZN13CStoreMonitor20newWithinTransactionEPjm
		.globl	__ZN13CStoreMonitor23startTransactionAgainstEj
		.globl	__ZN13CStoreMonitor15separatelyAbortEj
		.globl	__ZN13CStoreMonitor23addToCurrentTransactionEj
		.globl	__ZN13CStoreMonitor12lockReadOnlyEv
		.globl	__ZN13CStoreMonitor14unlockReadOnlyEb
		.globl	__ZN13CStoreMonitor9newObjectEPjPvm
		.globl	__ZN13CStoreMonitor13replaceObjectEjPvm
		.globl	__ZN13CStoreMonitor12newXIPObjectEPjm

		.text
		.align	2

MonitorGlue

CStoreMonitor_name:
		.asciz	"CStoreMonitor"
		.align	2

__ZN13CStoreMonitor4makeEPKc:
		New CStoreMonitor_name
		ret
__ZN13CStoreMonitor7destroyEv:
		MonDelete 1
__ZN13CStoreMonitor4initEP6CStore:
		MonDispatch 2
__ZN13CStoreMonitor11needsFormatEPb:
		MonDispatch 3
__ZN13CStoreMonitor6formatEv:
		MonDispatch 4
__ZN13CStoreMonitor9getRootIdEPj:
		MonDispatch 5
__ZN13CStoreMonitor9newObjectEPjm:
		MonDispatch 6
__ZN13CStoreMonitor11eraseObjectEj:
		MonDispatch 7
__ZN13CStoreMonitor12deleteObjectEj:
		MonDispatch 8
__ZN13CStoreMonitor13setObjectSizeEjm:
		MonDispatch 9
__ZN13CStoreMonitor13getObjectSizeEjPm:
		MonDispatch 10
__ZN13CStoreMonitor5writeEjmPvm:
		MonDispatch 11
__ZN13CStoreMonitor4readEjmPvm:
		MonDispatch 12
__ZN13CStoreMonitor12getStoreSizeEPmS0_:
		MonDispatch 13
__ZN13CStoreMonitor10isReadOnlyEPb:
		MonDispatch 14
__ZN13CStoreMonitor9lockStoreEv:
		MonDispatch 15
__ZN13CStoreMonitor11unlockStoreEv:
		MonDispatch 16
__ZN13CStoreMonitor5abortEv:
		MonDispatch 17
__ZN13CStoreMonitor4idleEPbS0_:
		MonDispatch 18
__ZN13CStoreMonitor10nextObjectEjPj:
		MonDispatch 19
__ZN13CStoreMonitor14checkIntegrityEPj:
		MonDispatch 20
__ZN13CStoreMonitor20newWithinTransactionEPjm:
		MonDispatch 21
__ZN13CStoreMonitor23startTransactionAgainstEj:
		MonDispatch 22
__ZN13CStoreMonitor15separatelyAbortEj:
		MonDispatch 23
__ZN13CStoreMonitor23addToCurrentTransactionEj:
		MonDispatch 24
__ZN13CStoreMonitor12lockReadOnlyEv:
		MonDispatch 25
__ZN13CStoreMonitor14unlockReadOnlyEb:
		MonDispatch 26
__ZN13CStoreMonitor9newObjectEPjPvm:
		MonDispatch 27
__ZN13CStoreMonitor13replaceObjectEjPvm:
		MonDispatch 28
__ZN13CStoreMonitor12newXIPObjectEPjm:
		MonDispatch 29

