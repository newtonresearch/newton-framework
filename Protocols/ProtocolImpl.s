/*
	File:		ProtocolImpl.s

	Contains:	P-class protocol implementation structures.

	Written by:	Newton Research Group, 2009.
*/

#include "../Protocols/ProtoMacro.s"

/* ----------------------------------------------------------------
	CClassInfoRegistryImpl implementation class info.
---------------------------------------------------------------- */

		.const
		.align	2

		.globl	__ZN22CClassInfoRegistryImpl9classInfoEv
__ZN22CClassInfoRegistryImpl9classInfoEv:
		ItIsHere

0:		.long		0				// reserved
		.long		1f - .		// SRO (Self-Relative-Offset) to asciz implementation name
		.long		2f - .		// SRO to asciz protocol name
		.long		3f - .		// SRO to asciz signature
		.long		4f - .		// SRO to dispatch table
		.long		5f - .		// SRO to monitor entry (valid only for monitors)

		BRAZ		__ZN22CClassInfoRegistryImpl6sizeOfEv	// branch to sizeof-code
		NOBRA						// branch to alloc-code, or zero
		NOBRA						// branch to OperatorDelete code, or zero
		BRAZ		__ZN22CClassInfoRegistryImpl4makeEv		// branch to New(void), or MOV PC,LK
		BRAZ		__ZN22CClassInfoRegistryImpl7destroyEv	// branch to Delete(void), or MOV PC,LK

		.long		0				// this implementation’s version
		.long		0				// flags
		.long		0				// reserved
		BRAZ		6f				// branch to bail-out function (that returns nil now)

1:		.asciz	"CClassInfoRegistryImpl"
2:		.asciz	"CClassInfoRegistry"
3:		.byte		0

		.align	2
4:		NOBRA
		BRA		__ZN22CClassInfoRegistryImpl9classInfoEv
		BRA		__ZN22CClassInfoRegistryImpl4makeEv
		BRA		__ZN22CClassInfoRegistryImpl7destroyEv
		BRA		__ZN22CClassInfoRegistryImpl16registerProtocolEPK10CClassInfoj
		BRA		__ZN22CClassInfoRegistryImpl18deregisterProtocolEPK10CClassInfoa
		BRA		__ZNK22CClassInfoRegistryImpl20isProtocolRegisteredEPK10CClassInfoa
		BRA		__ZNK22CClassInfoRegistryImpl7satisfyEPKcS1_j
		BRA		__ZNK22CClassInfoRegistryImpl7satisfyEPKcS1_S1_
		BRA		__ZNK22CClassInfoRegistryImpl7satisfyEPKcS1_S1_S1_
		BRA		__ZNK22CClassInfoRegistryImpl7satisfyEPKcS1_ii
		BRA		__ZN22CClassInfoRegistryImpl19updateInstanceCountEPK10CClassInfoi
		BRA		__ZN22CClassInfoRegistryImpl16getInstanceCountEPK10CClassInfo

5:									// monitor code goes here

6:		RetNil

