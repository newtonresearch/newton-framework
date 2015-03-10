/*
	File:		FlashImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2014.
*/

#include "../Protocols/ProtoMacro.s"

		.const
		.align	2

/* ----------------------------------------------------------------
	CNewInternalFlash implementation class info.
---------------------------------------------------------------- */

		.globl	__ZN17CNewInternalFlash9classInfoEv
__ZN17CNewInternalFlash9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN17CNewInternalFlash6sizeOfEv		// branch to sizeof-code
		NOBRA						// branch to operatorNew, or zero
		NOBRA						// branch to operatorDelete code, or zero
		BRAZ		__ZN17CNewInternalFlash4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN17CNewInternalFlash7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		.long		6f - 0b		// branch to bail-out function (that returns nil now)

1:		.asciz	"CNewInternalFlash"
2:		.asciz	"CFlash"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN17CNewInternalFlash9classInfoEv
		BRA		__ZN17CNewInternalFlash4makeEv
		BRA		__ZN17CNewInternalFlash7destroyEv
		BRA		__ZN17CNewInternalFlash4readEjmPc
		BRA		__ZN17CNewInternalFlash5writeEjmPc
		BRA		__ZN17CNewInternalFlash5eraseEm
		BRA		__ZN17CNewInternalFlash12suspendEraseEjjj
		BRA		__ZN17CNewInternalFlash11resumeEraseEj
		BRA		__ZN17CNewInternalFlash9deepSleepEj
		BRA		__ZN17CNewInternalFlash6wakeupEj
		BRA		__ZN17CNewInternalFlash6statusEj
		BRA		__ZN17CNewInternalFlash9resetCardEv
		BRA		__ZN17CNewInternalFlash16acknowledgeResetEv
		BRA		__ZN17CNewInternalFlash15getPhysResourceEv
		BRA		__ZN17CNewInternalFlash18registerClientInfoEj
		BRA		__ZN17CNewInternalFlash17getWriteProtectedEPa
		BRA		__ZN17CNewInternalFlash20getWriteErrorAddressEv
		BRA		__ZN17CNewInternalFlash13getAttributesEv
		BRA		__ZN17CNewInternalFlash13getDataOffsetEv
		BRA		__ZN17CNewInternalFlash12getTotalSizeEv
		BRA		__ZN17CNewInternalFlash12getGroupSizeEv
		BRA		__ZN17CNewInternalFlash18getEraseRegionSizeEv
		BRA		__ZN17CNewInternalFlash16getChipsPerGroupEv
		BRA		__ZN17CNewInternalFlash21getBlocksPerPartitionEv
		BRA		__ZN17CNewInternalFlash22getMaxConcurrentVppOpsEv
		BRA		__ZN17CNewInternalFlash21getEraseRegionCurrentEv
		BRA		__ZN17CNewInternalFlash21getWriteRegionCurrentEv
		BRA		__ZN17CNewInternalFlash18getEraseRegionTimeEv
		BRA		__ZN17CNewInternalFlash18getWriteAccessTimeEv
		BRA		__ZN17CNewInternalFlash17getReadAccessTimeEv
		BRA		__ZN17CNewInternalFlash13getVendorInfoEv
		BRA		__ZN17CNewInternalFlash15getSocketNumberEv
		BRA		__ZN17CNewInternalFlash9vppStatusEv
		BRA		__ZN17CNewInternalFlash13vppRisingTimeEv
		BRA		__ZN17CNewInternalFlash13flashSpecificEjPvj
		BRA		__ZN17CNewInternalFlash10initializeEP11CCardSocketP11CCardPCMCIAjj
		BRA		__ZN17CNewInternalFlash14suspendServiceEv
		BRA		__ZN17CNewInternalFlash13resumeServiceEP11CCardSocketP11CCardPCMCIAj
		BRA		__ZN17CNewInternalFlash4copyEjjm
		BRA		__ZN17CNewInternalFlash8isVirginEjm


5:									// monitor code goes here

6:		RetNil

