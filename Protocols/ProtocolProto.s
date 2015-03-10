/*
	File:		ProtocolProto.s

	Contains:	P-class protocol interface structures.

	Written by:	Newton Research Group, 2009.
*/

		.text
		.align	2

#if __LP64__

/* ----------------------------------------------------------------
	size_t
	PrivateClassInfoSize(const CClassInfo * inClass)
	{
		return *(inClass + inClass->fSizeofBranch)();
	}
---------------------------------------------------------------- */

		.globl	__Z20PrivateClassInfoSizePK10CClassInfo
__Z20PrivateClassInfoSizePK10CClassInfo:
		movl		24(%rdi), %eax		# rax = offset to sizeOf() branch
		addq		%rdi, %rax			# + inClass
		jmp		*%rax


/* ----------------------------------------------------------------
	void
	PrivateClassInfoMakeAt(const CClassInfo * inClass, const void * instance)
	{
		CProtocol * p = (CProtocol *)instance;
		p->fRuntime = NULL;
		p->fRealThis = p;
		p->fBTable = (void **)((char *)inClass + inClass->fBTableDelta);
		p->fMonitorId = 0;	// kNoId
	}
---------------------------------------------------------------- */

		.globl	__Z22PrivateClassInfoMakeAtPK10CClassInfoPKv
__Z22PrivateClassInfoMakeAtPK10CClassInfoPKv:
		movq		$0, 0(%rsi)			# rsi = instance
		movq		%rsi, 8(%rsi)
		movl		16(%rdi), %eax		# inClass->fBTableDelta
		addl		$16, %eax			# + offset
		addq		%rdi, %rax			# + inClass
		movq		%rax, 16(%rsi)
		movl		$0, 24(%rsi)
		ret


/* ----------------------------------------------------------------
	const char *
	PrivateClassInfoImplementationName(const CClassInfo * inClass)
	{
		return (const char *)inClass + offsetof(CClassInfo, fNameDelta) + inClass->fNameDelta;
	}
---------------------------------------------------------------- */

		.globl	__Z34PrivateClassInfoImplementationNamePK10CClassInfo
__Z34PrivateClassInfoImplementationNamePK10CClassInfo:
		movl		4(%rdi), %eax		# inClass->fNameDelta
		addl		$4, %eax				# + offset
		addq		%rdi, %rax			# + inClass
		ret

/* ----------------------------------------------------------------
	const char *
	PrivateClassInfoInterfaceName(const CClassInfo * inClass)
	{
		return (const char *)inClass + offsetof(CClassInfo, fInterfaceDelta) + inClass->fInterfaceDelta;
	}
---------------------------------------------------------------- */

		.globl	__Z29PrivateClassInfoInterfaceNamePK10CClassInfo
__Z29PrivateClassInfoInterfaceNamePK10CClassInfo:
		movl		8(%rdi), %eax		# inClass->fInterfaceDelta
		addl		$8, %eax				# + offset
		addq		%rdi, %rax			# + inClass
		ret

/* ----------------------------------------------------------------
	const char *
	PrivateClassInfoSignature(const CClassInfo * inClass)
	{
		return (const char *)inClass + offsetof(CClassInfo, fSignatureDelta) + inClass->fSignatureDelta;
	}
---------------------------------------------------------------- */

		.globl	__Z25PrivateClassInfoSignaturePK10CClassInfo
__Z25PrivateClassInfoSignaturePK10CClassInfo:
		movl		12(%rdi), %eax		# inClass->fSignatureDelta
		addl		$12, %eax			# + offset
		addq		%rdi, %rax			# + inClass
		ret

#else

/* ----------------------------------------------------------------
	size_t
	PrivateClassInfoSize(const CClassInfo * inClass)
	{
		return *(inClass + inClass->fSizeofBranch)();
	}
---------------------------------------------------------------- */

		.globl	__Z20PrivateClassInfoSizePK10CClassInfo
__Z20PrivateClassInfoSizePK10CClassInfo:
		movl		4(%esp), %eax
		addl		24(%eax), %eax
		jmp		*%eax


/* ----------------------------------------------------------------
	void
	PrivateClassInfoMakeAt(const CClassInfo * inClass, const void * instance)
	{
		CProtocol * p = (CProtocol *)instance;
		p->fRuntime = NULL;
		p->fRealThis = p;
		p->fBTable = (void **)((char *)inClass + inClass->fBTableDelta);
		p->fMonitorId = 0;	// kNoId
	}
---------------------------------------------------------------- */

		.globl	__Z22PrivateClassInfoMakeAtPK10CClassInfoPKv
__Z22PrivateClassInfoMakeAtPK10CClassInfoPKv:
		movl		8(%esp), %eax			# eax = instance
		movl		$0, 0(%eax)
		movl		%eax, 4(%eax)
		movl		4(%esp), %edx			# edx = inClass
		addl		16(%edx), %edx
		addl		$16, %edx
		movl		%edx, 8(%eax)
		movl		$0, 12(%eax)
		ret


/* ----------------------------------------------------------------
	const char *
	PrivateClassInfoInterfaceName(const CClassInfo * inClass)
	{
		return (const char *)inClass + offsetof(CClassInfo, fInterfaceDelta) + inClass->fInterfaceDelta;
	}
---------------------------------------------------------------- */

		.globl	__Z29PrivateClassInfoInterfaceNamePK10CClassInfo
__Z29PrivateClassInfoInterfaceNamePK10CClassInfo:
		movl		4(%esp), %eax			# eax = inClass
		addl		8(%eax), %eax			# += inClass->fInterfaceDelta
		addl		$8, %eax
		ret

/* ----------------------------------------------------------------
	const char *
	PrivateClassInfoImplementationName(const CClassInfo * inClass)
	{
		return (const char *)inClass + offsetof(CClassInfo, fNameDelta) + inClass->fNameDelta;
	}
---------------------------------------------------------------- */

		.globl	__Z34PrivateClassInfoImplementationNamePK10CClassInfo
__Z34PrivateClassInfoImplementationNamePK10CClassInfo:
		movl		4(%esp), %eax			# eax = inClass
		addl		4(%eax), %eax			# += inClass->fNameDelta
		addl		$4, %eax
		ret

/* ----------------------------------------------------------------
	const char *
	PrivateClassInfoSignature(const CClassInfo * inClass)
	{
		return (const char *)inClass + offsetof(CClassInfo, fSignatureDelta) + inClass->fSignatureDelta;
	}
---------------------------------------------------------------- */

		.globl	__Z25PrivateClassInfoSignaturePK10CClassInfo
__Z25PrivateClassInfoSignaturePK10CClassInfo:
		movl		4(%esp), %eax			# eax = inClass
		addl		12(%eax), %eax			# += inClass->fSignatureDelta
		addl		$12, %eax
		ret

#endif


/* ---------------------------------------------------------------- */

#include "ProtoMacro.s"

/* ---------------------------------------------------------------- */

		.globl	__ZN18CClassInfoRegistry4makeEPKc
		.globl	__ZN18CClassInfoRegistry7destroyEv
		.globl	__ZN18CClassInfoRegistry16registerProtocolEPK10CClassInfoj
		.globl	__ZN18CClassInfoRegistry18deregisterProtocolEPK10CClassInfob
		.globl	__ZNK18CClassInfoRegistry20isProtocolRegisteredEPK10CClassInfob
		.globl	__ZNK18CClassInfoRegistry7satisfyEPKcS1_j
		.globl	__ZNK18CClassInfoRegistry7satisfyEPKcS1_S1_
		.globl	__ZNK18CClassInfoRegistry7satisfyEPKcS1_S1_S1_
		.globl	__ZNK18CClassInfoRegistry7satisfyEPKcS1_ii
		.globl	__ZN18CClassInfoRegistry19updateInstanceCountEPK10CClassInfoi
		.globl	__ZN18CClassInfoRegistry16getInstanceCountEPK10CClassInfo

		.text
		.align	2

MonitorGlue

CClassInfoRegistry_name:
		.asciz	"CClassInfoRegistry"
		.align	2

__ZN18CClassInfoRegistry4makeEPKc:
		New CClassInfoRegistry_name
		ret
__ZN18CClassInfoRegistry7destroyEv:
		MonDelete 1
__ZN18CClassInfoRegistry16registerProtocolEPK10CClassInfoj:
		MonDispatch 2
__ZN18CClassInfoRegistry18deregisterProtocolEPK10CClassInfob:
		MonDispatch 3
__ZNK18CClassInfoRegistry20isProtocolRegisteredEPK10CClassInfob:
		MonDispatch 4
__ZNK18CClassInfoRegistry7satisfyEPKcS1_j:
		MonDispatch 5
__ZNK18CClassInfoRegistry7satisfyEPKcS1_S1_:
		MonDispatch 6
__ZNK18CClassInfoRegistry7satisfyEPKcS1_S1_S1_:
		MonDispatch 7
__ZNK18CClassInfoRegistry7satisfyEPKcS1_ii:
		MonDispatch 8
__ZN18CClassInfoRegistry19updateInstanceCountEPK10CClassInfoi:
		MonDispatch 9
__ZN18CClassInfoRegistry16getInstanceCountEPK10CClassInfo:
		MonDispatch 10
