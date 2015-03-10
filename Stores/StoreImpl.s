/*
	File:		StoreImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2009.
*/

#include "../Protocols/ProtoMacro.s"

		.const
		.align	2

/* ----------------------------------------------------------------
	CMuxStore implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN9CMuxStore9classInfoEv
__ZN9CMuxStore9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN9CMuxStore6sizeOfEv		// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN9CMuxStore4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN9CMuxStore7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CMuxStore"
2:		.asciz	"CStore"
3:		.asciz	"LOBJ", ""
		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN9CMuxStore9classInfoEv
		BRA		__ZN9CMuxStore4makeEv
		BRA		__ZN9CMuxStore7destroyEv
		BRA		__ZN9CMuxStore4initEPvmjjjS0_
		BRA		__ZN9CMuxStore11needsFormatEPb
		BRA		__ZN9CMuxStore6formatEv
		BRA		__ZN9CMuxStore9getRootIdEPj
		BRA		__ZN9CMuxStore9newObjectEPjm
		BRA		__ZN9CMuxStore11eraseObjectEj
		BRA		__ZN9CMuxStore12deleteObjectEj
		BRA		__ZN9CMuxStore13setObjectSizeEjm
		BRA		__ZN9CMuxStore13getObjectSizeEjPm
		BRA		__ZN9CMuxStore5writeEjmPvm
		BRA		__ZN9CMuxStore4readEjmPvm
		BRA		__ZN9CMuxStore12getStoreSizeEPmS0_
		BRA		__ZN9CMuxStore10isReadOnlyEPb
		BRA		__ZN9CMuxStore9lockStoreEv
		BRA		__ZN9CMuxStore11unlockStoreEv
		BRA		__ZN9CMuxStore5abortEv
		BRA		__ZN9CMuxStore4idleEPbS0_
		BRA		__ZN9CMuxStore10nextObjectEjPj
		BRA		__ZN9CMuxStore14checkIntegrityEPj
		BRA		__ZN9CMuxStore8setBuddyEP6CStore
		BRA		__ZN9CMuxStore10ownsObjectEj
		BRA		__ZN9CMuxStore7addressEj
		BRA		__ZN9CMuxStore9storeKindEv
		BRA		__ZN9CMuxStore8setStoreEP6CStorej
		BRA		__ZN9CMuxStore11isSameStoreEPvm
		BRA		__ZN9CMuxStore8isLockedEv
		BRA		__ZN9CMuxStore5isROMEv
		BRA		__ZN9CMuxStore6vppOffEv
		BRA		__ZN9CMuxStore5sleepEv
		BRA		__ZN9CMuxStore20newWithinTransactionEPjm
		BRA		__ZN9CMuxStore23startTransactionAgainstEj
		BRA		__ZN9CMuxStore15separatelyAbortEj
		BRA		__ZN9CMuxStore23addToCurrentTransactionEj
		BRA		__ZN9CMuxStore21inSeparateTransactionEj
		BRA		__ZN9CMuxStore12lockReadOnlyEv
		BRA		__ZN9CMuxStore14unlockReadOnlyEb
		BRA		__ZN9CMuxStore13inTransactionEv
		BRA		__ZN9CMuxStore9newObjectEPjPvm
		BRA		__ZN9CMuxStore13replaceObjectEjPvm
		BRA		__ZN9CMuxStore17calcXIPObjectSizeEllPl
		BRA		__ZN9CMuxStore12newXIPObjectEPjm
		BRA		__ZN9CMuxStore16getXIPObjectInfoEjPmS0_S0_

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CFlashStore implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN11CFlashStore9classInfoEv
__ZN11CFlashStore9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN11CFlashStore6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN11CFlashStore4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN11CFlashStore7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CFlashStore"
2:		.asciz	"CStore"
3:		.asciz	"LOBJ", ""
		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN11CFlashStore9classInfoEv
		BRA		__ZN11CFlashStore4makeEv
		BRA		__ZN11CFlashStore7destroyEv
		BRA		__ZN11CFlashStore4initEPvmjjjS0_
		BRA		__ZN11CFlashStore11needsFormatEPb
		BRA		__ZN11CFlashStore6formatEv
		BRA		__ZN11CFlashStore9getRootIdEPj
		BRA		__ZN11CFlashStore9newObjectEPjm
		BRA		__ZN11CFlashStore11eraseObjectEj
		BRA		__ZN11CFlashStore12deleteObjectEj
		BRA		__ZN11CFlashStore13setObjectSizeEjm
		BRA		__ZN11CFlashStore13getObjectSizeEjPm
		BRA		__ZN11CFlashStore5writeEjmPvm
		BRA		__ZN11CFlashStore4readEjmPvm
		BRA		__ZN11CFlashStore12getStoreSizeEPmS0_
		BRA		__ZN11CFlashStore10isReadOnlyEPb
		BRA		__ZN11CFlashStore9lockStoreEv
		BRA		__ZN11CFlashStore11unlockStoreEv
		BRA		__ZN11CFlashStore5abortEv
		BRA		__ZN11CFlashStore4idleEPbS0_
		BRA		__ZN11CFlashStore10nextObjectEjPj
		BRA		__ZN11CFlashStore14checkIntegrityEPj
		BRA		__ZN11CFlashStore8setBuddyEP6CStore
		BRA		__ZN11CFlashStore10ownsObjectEj
		BRA		__ZN11CFlashStore7addressEj
		BRA		__ZN11CFlashStore9storeKindEv
		BRA		__ZN11CFlashStore8setStoreEP6CStorej
		BRA		__ZN11CFlashStore11isSameStoreEPvm
		BRA		__ZN11CFlashStore8isLockedEv
		BRA		__ZN11CFlashStore5isROMEv
		BRA		__ZN11CFlashStore6vppOffEv
		BRA		__ZN11CFlashStore5sleepEv
		BRA		__ZN11CFlashStore20newWithinTransactionEPjm
		BRA		__ZN11CFlashStore23startTransactionAgainstEj
		BRA		__ZN11CFlashStore15separatelyAbortEj
		BRA		__ZN11CFlashStore23addToCurrentTransactionEj
		BRA		__ZN11CFlashStore21inSeparateTransactionEj
		BRA		__ZN11CFlashStore12lockReadOnlyEv
		BRA		__ZN11CFlashStore14unlockReadOnlyEb
		BRA		__ZN11CFlashStore13inTransactionEv
		BRA		__ZN11CFlashStore9newObjectEPjPvm
		BRA		__ZN11CFlashStore13replaceObjectEjPvm
		BRA		__ZN11CFlashStore17calcXIPObjectSizeEllPl
		BRA		__ZN11CFlashStore12newXIPObjectEPjm
		BRA		__ZN11CFlashStore16getXIPObjectInfoEjPmS0_S0_

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CPackageStore implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN13CPackageStore9classInfoEv
__ZN13CPackageStore9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN13CPackageStore6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN13CPackageStore4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN13CPackageStore7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CPackageStore"
2:		.asciz	"CStore"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN13CPackageStore9classInfoEv
		BRA		__ZN13CPackageStore4makeEv
		BRA		__ZN13CPackageStore7destroyEv
		BRA		__ZN13CPackageStore4initEPvmjjjS0_
		BRA		__ZN13CPackageStore11needsFormatEPb
		BRA		__ZN13CPackageStore6formatEv
		BRA		__ZN13CPackageStore9getRootIdEPj
		BRA		__ZN13CPackageStore9newObjectEPjm
		BRA		__ZN13CPackageStore11eraseObjectEj
		BRA		__ZN13CPackageStore12deleteObjectEj
		BRA		__ZN13CPackageStore13setObjectSizeEjm
		BRA		__ZN13CPackageStore13getObjectSizeEjPm
		BRA		__ZN13CPackageStore5writeEjmPvm
		BRA		__ZN13CPackageStore4readEjmPvm
		BRA		__ZN13CPackageStore12getStoreSizeEPmS0_
		BRA		__ZN13CPackageStore10isReadOnlyEPb
		BRA		__ZN13CPackageStore9lockStoreEv
		BRA		__ZN13CPackageStore11unlockStoreEv
		BRA		__ZN13CPackageStore5abortEv
		BRA		__ZN13CPackageStore4idleEPbS0_
		BRA		__ZN13CPackageStore10nextObjectEjPj
		BRA		__ZN13CPackageStore14checkIntegrityEPj
		BRA		__ZN13CPackageStore8setBuddyEP6CStore
		BRA		__ZN13CPackageStore10ownsObjectEj
		BRA		__ZN13CPackageStore7addressEj
		BRA		__ZN13CPackageStore9storeKindEv
		BRA		__ZN13CPackageStore8setStoreEP6CStorej
		BRA		__ZN13CPackageStore11isSameStoreEPvm
		BRA		__ZN13CPackageStore8isLockedEv
		BRA		__ZN13CPackageStore5isROMEv
		BRA		__ZN13CPackageStore6vppOffEv
		BRA		__ZN13CPackageStore5sleepEv
		BRA		__ZN13CPackageStore20newWithinTransactionEPjm
		BRA		__ZN13CPackageStore23startTransactionAgainstEj
		BRA		__ZN13CPackageStore15separatelyAbortEj
		BRA		__ZN13CPackageStore23addToCurrentTransactionEj
		BRA		__ZN13CPackageStore21inSeparateTransactionEj
		BRA		__ZN13CPackageStore12lockReadOnlyEv
		BRA		__ZN13CPackageStore14unlockReadOnlyEb
		BRA		__ZN13CPackageStore13inTransactionEv
		BRA		__ZN13CPackageStore9newObjectEPjPvm
		BRA		__ZN13CPackageStore13replaceObjectEjPvm
		BRA		__ZN13CPackageStore17calcXIPObjectSizeEllPl
		BRA		__ZN13CPackageStore12newXIPObjectEPjm
		BRA		__ZN13CPackageStore16getXIPObjectInfoEjPmS0_S0_

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CLOPackageStore implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN15CLOPackageStore9classInfoEv
__ZN15CLOPackageStore9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN15CLOPackageStore6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN15CLOPackageStore4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN15CLOPackageStore7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CLOPackageStore"
2:		.asciz	"CLrgObjStore"
3:		.asciz	"CZippyRelocStoreDecompressor", ""
		.asciz	"CZippyStoreDecompressor", ""
		.asciz	"CLZRelocStoreDecompressor", ""
		.asciz	"CLZStoreDecompressor", ""
		.asciz	"CSimpleRelocStoreDecompressor", ""
		.asciz	"CSimpleStoreDecompressor", ""
		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN15CLOPackageStore9classInfoEv
		BRA		__ZN15CLOPackageStore4makeEv
		BRA		__ZN15CLOPackageStore7destroyEv
		BRA		__ZN15CLOPackageStore4initEv
		BRA		__ZN15CLOPackageStore6createEPjP6CStoreP5CPipemaPKcPvmP11CLOCallback
		BRA		__ZN15CLOPackageStore20createFromCompressedEPjP6CStoreP5CPipemaPKcPvmP11CLOCallback
		BRA		__ZN15CLOPackageStore12deleteObjectEP6CStorej
		BRA		__ZN15CLOPackageStore9duplicateEPjP6CStorejS2_
		BRA		__ZN15CLOPackageStore6resizeEP6CStorejm
		BRA		__ZN15CLOPackageStore11storageSizeEP6CStorej
		BRA		__ZN15CLOPackageStore12sizeOfStreamEP6CStoreja
		BRA		__ZN15CLOPackageStore6backupEP5CPipeP6CStorejaP11CLOCallback

5:									// monitor code goes here

6:		RetNil

#if 0
/* ----------------------------------------------------------------
	CHackStore implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN10CHackStore9classInfoEv
__ZN10CHackStore9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN10CHackStore6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN10CHackStore4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN10CHackStore7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CHackStore"
2:		.asciz	"CStore"
3:		.asciz	"LOBJ", ""
		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN10CHackStore9classInfoEv
		BRA		__ZN10CHackStore4makeEv
		BRA		__ZN10CHackStore7destroyEv
		BRA		__ZN10CHackStore4initEPvmjjjS0_
		BRA		__ZN10CHackStore11needsFormatEPb
		BRA		__ZN10CHackStore6formatEv
		BRA		__ZN10CHackStore9getRootIdEPj
		BRA		__ZN10CHackStore9newObjectEPjm
		BRA		__ZN10CHackStore11eraseObjectEj
		BRA		__ZN10CHackStore12deleteObjectEj
		BRA		__ZN10CHackStore13setObjectSizeEjm
		BRA		__ZN10CHackStore13getObjectSizeEjPm
		BRA		__ZN10CHackStore5writeEjmPvm
		BRA		__ZN10CHackStore4readEjmPvm
		BRA		__ZN10CHackStore12getStoreSizeEPmS0_
		BRA		__ZN10CHackStore10isReadOnlyEPb
		BRA		__ZN10CHackStore9lockStoreEv
		BRA		__ZN10CHackStore11unlockStoreEv
		BRA		__ZN10CHackStore5abortEv
		BRA		__ZN10CHackStore4idleEPbS0_
		BRA		__ZN10CHackStore10nextObjectEjPj
		BRA		__ZN10CHackStore14checkIntegrityEPj
		BRA		__ZN10CHackStore8setBuddyEP6CStore
		BRA		__ZN10CHackStore10ownsObjectEj
		BRA		__ZN10CHackStore7addressEj
		BRA		__ZN10CHackStore9storeKindEv
		BRA		__ZN10CHackStore8setStoreEP6CStorej
		BRA		__ZN10CHackStore11isSameStoreEPvm
		BRA		__ZN10CHackStore8isLockedEv
		BRA		__ZN10CHackStore5isROMEv
		BRA		__ZN10CHackStore6vppOffEv
		BRA		__ZN10CHackStore5sleepEv
		BRA		__ZN10CHackStore20newWithinTransactionEPjm
		BRA		__ZN10CHackStore23startTransactionAgainstEj
		BRA		__ZN10CHackStore15separatelyAbortEj
		BRA		__ZN10CHackStore23addToCurrentTransactionEj
		BRA		__ZN10CHackStore21inSeparateTransactionEj
		BRA		__ZN10CHackStore12lockReadOnlyEv
		BRA		__ZN10CHackStore14unlockReadOnlyEb
		BRA		__ZN10CHackStore13inTransactionEv
		BRA		__ZN10CHackStore9newObjectEPjPvm
		BRA		__ZN10CHackStore13replaceObjectEjPvm
		BRA		__ZN10CHackStore17calcXIPObjectSizeEllPl
		BRA		__ZN10CHackStore12newXIPObjectEPjm
		BRA		__ZN10CHackStore16getXIPObjectInfoEjPmS0_S0_

5:									// monitor code goes here

6:		RetNil

/* ----------------------------------------------------------------
	CLOHackStore implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN12CLOHackStore9classInfoEv
__ZN12CLOHackStore9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN12CLOHackStore6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN12CLOHackStore4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN12CLOHackStore7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CLOHackStore"
2:		.asciz	"CLrgObjStore"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN12CLOHackStore9classInfoEv
		BRA		__ZN12CLOHackStore4makeEv
		BRA		__ZN12CLOHackStore7destroyEv
		BRA		__ZN12CLOHackStore4initEv
		BRA		__ZN12CLOHackStore6createEPjP6CStoreP5CPipemaPKcPvmP11CLOCallback
		BRA		__ZN12CLOHackStore20createFromCompressedEPjP6CStoreP5CPipemaPKcPvmP11CLOCallback
		BRA		__ZN12CLOHackStore12deleteObjectEP6CStorej
		BRA		__ZN12CLOHackStore9duplicateEPjP6CStorejS2_
		BRA		__ZN12CLOHackStore6resizeEP6CStorejm
		BRA		__ZN12CLOHackStore11storageSizeEP6CStorej
		BRA		__ZN12CLOHackStore12sizeOfStreamEP6CStoreja
		BRA		__ZN12CLOHackStore6backupEP5CPipeP6CStorejaP11CLOCallback

5:									// monitor code goes here

6:		RetNil
#endif
