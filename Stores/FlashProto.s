/*
	File:		FlashProto.s

	Contains:	CFlash p-class protocol interface.

	Written by:	Newton Research Group, 2014.
*/

#include "../Protocols/ProtoMacro.s"

/* ---------------------------------------------------------------- */


		.globl	__ZN6CFlash4makeEPKc
		.globl	__ZN6CFlash7destroyEv
		.globl	__ZN6CFlash4readEjmPc
		.globl	__ZN6CFlash5writeEjmPc
		.globl	__ZN6CFlash5eraseEj
		.globl	__ZN6CFlash12suspendEraseEjjj
		.globl	__ZN6CFlash11resumeEraseEj
		.globl	__ZN6CFlash9deepSleepEj
		.globl	__ZN6CFlash6wakeupEj
		.globl	__ZN6CFlash6statusEj
		.globl	__ZN6CFlash9resetCardEv
		.globl	__ZN6CFlash16acknowledgeResetEv
		.globl	__ZN6CFlash15getPhysResourceEv
		.globl	__ZN6CFlash18registerClientInfoEj
		.globl	__ZN6CFlash17getWriteProtectedEPb
		.globl	__ZN6CFlash20getWriteErrorAddressEv
		.globl	__ZN6CFlash13getAttributesEv
		.globl	__ZN6CFlash13getDataOffsetEv
		.globl	__ZN6CFlash12getTotalSizeEv
		.globl	__ZN6CFlash12getGroupSizeEv
		.globl	__ZN6CFlash18getEraseRegionSizeEv
		.globl	__ZN6CFlash16getChipsPerGroupEv
		.globl	__ZN6CFlash21getBlocksPerPartitionEv
		.globl	__ZN6CFlash22getMaxConcurrentVppOpsEv
		.globl	__ZN6CFlash21getEraseRegionCurrentEv
		.globl	__ZN6CFlash21getWriteRegionCurrentEv
		.globl	__ZN6CFlash18getEraseRegionTimeEv
		.globl	__ZN6CFlash18getWriteAccessTimeEv
		.globl	__ZN6CFlash17getReadAccessTimeEv
		.globl	__ZN6CFlash13getVendorInfoEv
		.globl	__ZN6CFlash15getSocketNumberEv
		.globl	__ZN6CFlash9vppStatusEv
		.globl	__ZN6CFlash13vppRisingTimeEv
		.globl	__ZN6CFlash13flashSpecificEjPvj
		.globl	__ZN6CFlash10initializeEP11CCardSocketP11CCardPCMCIAjj
		.globl	__ZN6CFlash14suspendServiceEv
		.globl	__ZN6CFlash13resumeServiceEP11CCardSocketP11CCardPCMCIAj
		.globl	__ZN6CFlash4copyEjjm
		.globl	__ZN6CFlash8isVirginEjm

		.text
		.align	2

CFlash_name:
		.asciz	"CFlash"
		.align	2

__ZN6CFlash4makeEPKc:
		New CFlash_name
		Dispatch 2
__ZN6CFlash7destroyEv:
		Delete 3
__ZN6CFlash4readEjmPc:
		Dispatch 4
__ZN6CFlash5writeEjmPc:
		Dispatch 5
__ZN6CFlash5eraseEj:
		Dispatch 6
__ZN6CFlash12suspendEraseEjjj:
		Dispatch 7
__ZN6CFlash11resumeEraseEj:
		Dispatch 8
__ZN6CFlash9deepSleepEj:
		Dispatch 9
__ZN6CFlash6wakeupEj:
		Dispatch 10
__ZN6CFlash6statusEj:
		Dispatch 11
__ZN6CFlash9resetCardEv:
		Dispatch 12
__ZN6CFlash16acknowledgeResetEv:
		Dispatch 13
__ZN6CFlash15getPhysResourceEv:
		Dispatch 14
__ZN6CFlash18registerClientInfoEj:
		Dispatch 15
__ZN6CFlash17getWriteProtectedEPb:
		Dispatch 16
__ZN6CFlash20getWriteErrorAddressEv:
		Dispatch 17
__ZN6CFlash13getAttributesEv:
		Dispatch 18
__ZN6CFlash13getDataOffsetEv:
		Dispatch 19
__ZN6CFlash12getTotalSizeEv:
		Dispatch 20
__ZN6CFlash12getGroupSizeEv:
		Dispatch 21
__ZN6CFlash18getEraseRegionSizeEv:
		Dispatch 22
__ZN6CFlash16getChipsPerGroupEv:
		Dispatch 23
__ZN6CFlash21getBlocksPerPartitionEv:
		Dispatch 24
__ZN6CFlash22getMaxConcurrentVppOpsEv:
		Dispatch 25
__ZN6CFlash21getEraseRegionCurrentEv:
		Dispatch 26
__ZN6CFlash21getWriteRegionCurrentEv:
		Dispatch 27
__ZN6CFlash18getEraseRegionTimeEv:
		Dispatch 28
__ZN6CFlash18getWriteAccessTimeEv:
		Dispatch 29
__ZN6CFlash17getReadAccessTimeEv:
		Dispatch 30
__ZN6CFlash13getVendorInfoEv:
		Dispatch 31
__ZN6CFlash15getSocketNumberEv:
		Dispatch 32
__ZN6CFlash9vppStatusEv:
		Dispatch 33
__ZN6CFlash13vppRisingTimeEv:
		Dispatch 34
__ZN6CFlash13flashSpecificEjPvj:
		Dispatch 35
__ZN6CFlash10initializeEP11CCardSocketP11CCardPCMCIAjj:
		Dispatch 36
__ZN6CFlash14suspendServiceEv:
		Dispatch 37
__ZN6CFlash13resumeServiceEP11CCardSocketP11CCardPCMCIAj:
		Dispatch 38
__ZN6CFlash4copyEjjm:
		Dispatch 39
__ZN6CFlash8isVirginEjm:
		Dispatch 40
