/*
	File:		StoreProto.s

	Contains:	CStore p-class protocol interface.

	Written by:	Newton Research Group, 2009.
*/

#include "../Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN6CStore4makeEPKc
		.globl	__ZN6CStore7destroyEv
		.globl	__ZN6CStore4initEPvmjjjS0_
		.globl	__ZN6CStore11needsFormatEPb
		.globl	__ZN6CStore6formatEv
		.globl	__ZN6CStore9getRootIdEPj
		.globl	__ZN6CStore9newObjectEPjm
		.globl	__ZN6CStore11eraseObjectEj
		.globl	__ZN6CStore12deleteObjectEj
		.globl	__ZN6CStore13setObjectSizeEjm
		.globl	__ZN6CStore13getObjectSizeEjPm
		.globl	__ZN6CStore5writeEjmPvm
		.globl	__ZN6CStore4readEjmPvm
		.globl	__ZN6CStore12getStoreSizeEPmS0_
		.globl	__ZN6CStore10isReadOnlyEPb
		.globl	__ZN6CStore9lockStoreEv
		.globl	__ZN6CStore11unlockStoreEv
		.globl	__ZN6CStore5abortEv
		.globl	__ZN6CStore4idleEPbS0_
		.globl	__ZN6CStore10nextObjectEjPj
		.globl	__ZN6CStore14checkIntegrityEPj
		.globl	__ZN6CStore8setBuddyEPS_
		.globl	__ZN6CStore10ownsObjectEj
		.globl	__ZN6CStore7addressEj
		.globl	__ZN6CStore9storeKindEv
		.globl	__ZN6CStore8setStoreEPS_j
		.globl	__ZN6CStore11isSameStoreEPvm
		.globl	__ZN6CStore8isLockedEv
		.globl	__ZN6CStore5isROMEv
		.globl	__ZN6CStore6vppOffEv
		.globl	__ZN6CStore5sleepEv
		.globl	__ZN6CStore20newWithinTransactionEPjm
		.globl	__ZN6CStore23startTransactionAgainstEj
		.globl	__ZN6CStore15separatelyAbortEj
		.globl	__ZN6CStore23addToCurrentTransactionEj
		.globl	__ZN6CStore21inSeparateTransactionEj
		.globl	__ZN6CStore12lockReadOnlyEv
		.globl	__ZN6CStore14unlockReadOnlyEb
		.globl	__ZN6CStore13inTransactionEv
		.globl	__ZN6CStore9newObjectEPjPvm
		.globl	__ZN6CStore13replaceObjectEjPvm
		.globl	__ZN6CStore17calcXIPObjectSizeEllPl
		.globl	__ZN6CStore12newXIPObjectEPjm
		.globl	__ZN6CStore16getXIPObjectInfoEjPmS0_S0_

		.text
		.align	2

CStore_name:
		.asciz	"CStore"
		.align	2

__ZN6CStore4makeEPKc:
		New CStore_name
		Dispatch 2
__ZN6CStore7destroyEv:
		Delete 3
__ZN6CStore4initEPvmjjjS0_:
		Dispatch 4
__ZN6CStore11needsFormatEPb:
		Dispatch 5
__ZN6CStore6formatEv:
		Dispatch 6
__ZN6CStore9getRootIdEPj:
		Dispatch 7
__ZN6CStore9newObjectEPjm:
		Dispatch 8
__ZN6CStore11eraseObjectEj:
		Dispatch 9
__ZN6CStore12deleteObjectEj:
		Dispatch 10
__ZN6CStore13setObjectSizeEjm:
		Dispatch 11
__ZN6CStore13getObjectSizeEjPm:
		Dispatch 12
__ZN6CStore5writeEjmPvm:
		Dispatch 13
__ZN6CStore4readEjmPvm:
		Dispatch 14
__ZN6CStore12getStoreSizeEPmS0_:
		Dispatch 15
__ZN6CStore10isReadOnlyEPb:
		Dispatch 16
__ZN6CStore9lockStoreEv:
		Dispatch 17
__ZN6CStore11unlockStoreEv:
		Dispatch 18
__ZN6CStore5abortEv:
		Dispatch 19
__ZN6CStore4idleEPbS0_:
		Dispatch 20
__ZN6CStore10nextObjectEjPj:
		Dispatch 21
__ZN6CStore14checkIntegrityEPj:
		Dispatch 22
__ZN6CStore8setBuddyEPS_:
		Dispatch 23
__ZN6CStore10ownsObjectEj:
		Dispatch 24
__ZN6CStore7addressEj:
		Dispatch 25
__ZN6CStore9storeKindEv:
		Dispatch 26
__ZN6CStore8setStoreEPS_j:
		Dispatch 27
__ZN6CStore11isSameStoreEPvm:
		Dispatch 28
__ZN6CStore8isLockedEv:
		Dispatch 29
__ZN6CStore5isROMEv:
		Dispatch 30
__ZN6CStore6vppOffEv:
		Dispatch 31
__ZN6CStore5sleepEv:
		Dispatch 32
__ZN6CStore20newWithinTransactionEPjm:
		Dispatch 33
__ZN6CStore23startTransactionAgainstEj:
		Dispatch 34
__ZN6CStore15separatelyAbortEj:
		Dispatch 35
__ZN6CStore23addToCurrentTransactionEj:
		Dispatch 36
__ZN6CStore21inSeparateTransactionEj:
		Dispatch 37
__ZN6CStore12lockReadOnlyEv:
		Dispatch 38
__ZN6CStore14unlockReadOnlyEb:
		Dispatch 39
__ZN6CStore13inTransactionEv:
		Dispatch 40
__ZN6CStore9newObjectEPjPvm:
		Dispatch 41
__ZN6CStore13replaceObjectEjPvm:
		Dispatch 42
__ZN6CStore17calcXIPObjectSizeEllPl:
		Dispatch 43
__ZN6CStore12newXIPObjectEPjm:
		Dispatch 44
__ZN6CStore16getXIPObjectInfoEjPmS0_S0_:
		Dispatch 45

/* ---------------------------------------------------------------- */

		.globl	__ZN12CLrgObjStore4makeEPKc
		.globl	__ZN12CLrgObjStore7destroyEv
		.globl	__ZN12CLrgObjStore4initEv
		.globl	__ZN12CLrgObjStore6createEPjP6CStoreP5CPipembPKcPvmP11CLOCallback
		.globl	__ZN12CLrgObjStore20createFromCompressedEPjP6CStoreP5CPipembPKcPvmP11CLOCallback
		.globl	__ZN12CLrgObjStore12deleteObjectEP6CStorej
		.globl	__ZN12CLrgObjStore9duplicateEPjP6CStorejS2_
		.globl	__ZN12CLrgObjStore6resizeEP6CStorejm
		.globl	__ZN12CLrgObjStore11storageSizeEP6CStorej
		.globl	__ZN12CLrgObjStore12sizeOfStreamEP6CStorejb
		.globl	__ZN12CLrgObjStore6backupEP5CPipeP6CStorejbP11CLOCallback

		.text
		.align	2

CLrgObjStore_name:
		.asciz	"CLrgObjStore"
		.align	2

__ZN12CLrgObjStore4makeEPKc:
		New CLrgObjStore_name
		Dispatch 2
__ZN12CLrgObjStore7destroyEv:
		Delete 3
__ZN12CLrgObjStore4initEv:
		Dispatch 4
__ZN12CLrgObjStore6createEPjP6CStoreP5CPipembPKcPvmP11CLOCallback:
		Dispatch 5
__ZN12CLrgObjStore20createFromCompressedEPjP6CStoreP5CPipembPKcPvmP11CLOCallback:
		Dispatch 6
__ZN12CLrgObjStore12deleteObjectEP6CStorej:
		Dispatch 7
__ZN12CLrgObjStore9duplicateEPjP6CStorejS2_:
		Dispatch 8
__ZN12CLrgObjStore6resizeEP6CStorejm:
		Dispatch 9
__ZN12CLrgObjStore11storageSizeEP6CStorej:
		Dispatch 10
__ZN12CLrgObjStore12sizeOfStreamEP6CStorejb:
		Dispatch 11
__ZN12CLrgObjStore6backupEP5CPipeP6CStorejbP11CLOCallback:
		Dispatch 12
