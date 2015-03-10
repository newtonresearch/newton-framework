/*
	File:		SWIGlue.s

	Contains:	SWI function call LP64 assembler glue.
					All the functions in here have extern "C" linkage.
					See http://www.x86-64.org/documentation/abi.pdf
					for the ABI.
					 https://developer.apple.com/library/mac/documentation/darwin/conceptual/64bitporting/64bitporting.pdf
					 http://www.classes.cs.uchicago.edu/archive/2009/spring/22620-1/docs/handout-03.pdf
					 http://www.cs.cmu.edu/~fp/courses/15213-s07/misc/asm64-handout.pdf
					for 64-bit assembler info.

					In LP64 ABI
						args are in				%rdi %rsi %rdx %rcx %r8 %r9
						return value is in	%rax

	Written by:	Newton Research Group, 2009.
*/

//	register offsets in CTask
		.set	taskr1,	 0xC0		// == kReturnParm1; sizeof(CObject) + 21*sizeof(long) == 0x18 + 0xA8 == 0xC0
		.set	taskr2,	 0xC8
		.set	taskr3,	 0xD0
		.set	taskr4,	 0xD8

// PSR bits
		.set	usr_32,		0x10
		.set	fiq_32,		0x11
		.set	irq_32,		0x12
		.set	svc_32,		0x13
		.set	abt_32,		0x17
		.set	und_32,		0x1B
		.set	mode_mask,	0x1F

		.set	FIQdisable,	0x80
		.set	IRQdisable,	0x40

		.text
		.align	2

/* ----------------------------------------------------------------
	M i s c e l l a n e o u s
---------------------------------------------------------------- */
		.globl	_SetAndClearBitsAtomic
		.globl	_LowLevelGetCPUType
		.globl	_LowLevelProcRevLevel
		.globl	_LowLevelProcSpeed
		.globl	_GetCPUVersion
		.globl	_GetCPUMode
		.globl	_IsSuperMode
		.globl	_EnterAtomicSWI
		.globl	_ExitAtomicSWI
		.globl	_EnterFIQAtomicSWI
		.globl	_ExitFIQAtomicSWI
		.globl	_ClearFIQMask
		.globl	_GenericSWI
		.globl	_GenericWithReturnSWI

/* ----------------------------------------------------------------
	void
	SetAndClearBitsAtomic(ULong * ioTarget, ULong inSetBits, ULong inClearBits)

	Perform atomic bit twiddling.
	Args:		rdi	ioTarget			bits to be twiddled
				rsi	inSetBits		bits to be set in target
				rdx	inClearBits		bits to be cleared in target
	Return:	--
---------------------------------------------------------------- */

_SetAndClearBitsAtomic:
/*		assume super mode, no interrupts
		were we in user mode we’d need to:

		pushq		%rbp						# prolog
		movq		%rsp, %rbp

		callq		_EnterFIQAtomic
*/
		movl		0(%rdi), %eax
		orl		%esi, %eax
		andl		%edx, %eax
		movl		%eax, 0(%rdi)
/*
		callq		_ExitFIQAtomic

		addq		$8, %rsp
		popq		%rbp						# epilog
*/
		ret


/* ----------------------------------------------------------------
	ULong
	LowLevelGetCPUType(void)

	Return the processor type.
	Args:		--
	Return:	1 -> ARM610 = MP130
				2 -> ARM710 = eMate
				3 -> StrongARM = MP2x00
				4 -> PPC
				5 -> i386
				6 -> x86_64
---------------------------------------------------------------- */

_LowLevelGetCPUType:
		movl		$6, %eax
		ret


/* ----------------------------------------------------------------
	ULong
	LowLevelProcRevLevel(void)

	Return the processor revision level.
	Args:		--
	Return:	0 | 1
---------------------------------------------------------------- */

_LowLevelProcRevLevel:
		movl		$0, %eax
		ret


/* ----------------------------------------------------------------
	void
	LowLevelProcSpeed(ULong inNumOfIterations)

	Perform some calculations we can time to work out the processor speed.
	Args:		rdi	inNumOfIterations
	Return:	--
---------------------------------------------------------------- */

_LowLevelProcSpeed:
//		MOV	R1, #0
//1:	MOV	R2, #-256
//		MOV	R3, #-16
//		MUL	R2, R3, R2
//		ADD	R1, R1, #1
//		TEQ	R1, R0
//		BNE	L1
		movl	$0, %eax
		ret


/* ----------------------------------------------------------------
	int
	GetCPUVersion(void)

	Return the CPU version.
	Args:		--
	Return:	CPU version
---------------------------------------------------------------- */

_GetCPUVersion:
		movl		$10, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	int
	GetCPUMode(void)

	Return the CPU mode.
	Args:		--
	Return:	CPU mode
---------------------------------------------------------------- */

_GetCPUMode:
		movl		_gCPSR(%rip), %eax
		andl		$mode_mask, %eax
		ret


/* ----------------------------------------------------------------
	bool
	IsSuperMode(void)

	Determine whether we are in supervisor mode.
	(Actually any non-user mode.)
	Args:		--
	Return:	YES if in supervisor mode
---------------------------------------------------------------- */

_IsSuperMode:
		movl		_gCPSR(%rip), %eax
		andl		$mode_mask, %eax
		cmpl		$usr_32, %eax
		je			1f
		movl		$1, %eax
		ret
1:		movl		$0, %eax
		ret


/* ----------------------------------------------------------------
	Enter an atomic action.
	Args:		--
	Return:	--
---------------------------------------------------------------- */

_EnterAtomicSWI:
		movl		$3, %eax
		jmp		SWI

/* ----------------------------------------------------------------
	Exit an atomic action.
	Args:		--
	Return:	--
---------------------------------------------------------------- */

_ExitAtomicSWI:
		movl		$4, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	Enter an atomic action.
	Args:		--
	Return:	--
---------------------------------------------------------------- */

_EnterFIQAtomicSWI:
		movl		$30, %eax
		jmp		SWI

/* ----------------------------------------------------------------
	Exit an atomic action.
	Args:		--
	Return:	--
---------------------------------------------------------------- */

_ExitFIQAtomicSWI:
		movl		$31, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	void
	ClearFIQMask(void)

	Args:		--
	Return:	--
---------------------------------------------------------------- */

_ClearFIQMask:
		movl		_gCPSR(%rip), %eax
		andl		$!FIQdisable, %eax
		movl		%eax, _gCPSR(%rip)
		ret


/* ----------------------------------------------------------------
	DebugStr Pascal strings
---------------------------------------------------------------- */
		.cstring
		.globl	nonUserModeSWI
		.globl	undefinedSWI
		.globl	zot
		.globl	badMonExit
		.globl	badMonThrow
		.globl	tooManyAtomicExits
		.globl	tooManyFIQExits

nonUserModeSWI:
		.byte		35
		.asciz	"SWI from non-user mode (rebooting)."
undefinedSWI:
		.byte		13
		.asciz	"Undefined SWI"
zot:
		.byte		42
		.asciz	"Zot! GenericSWI called from non-user mode."
zotMon:
		.byte		40
		.asciz	"Zot! Check SVC mode in MonitorEntryGlue."
badMonExit:
		.byte		50
		.asciz	"MonitorExitSWI failed! This should never happen..."
badMonThrow:
		.byte		40
		.asciz	"MonitorThrowSWI failed; check your head."
tooManyAtomicExits:
		.byte		36
		.asciz	"Exit Atomic called too many times!!!"
tooManyFIQExits:
		.byte		40
		.asciz	"Exit FIQ Atomic called too many times!!!"

/* ------------------------------------------------------------- */

		.text
		.align	2

/* ----------------------------------------------------------------
	NewtonErr
	GenericSWI(int inSelector, ...)

	Handle a generic SWI.
	Args:		 rdi	inSelector	generic SWI sub-selector
				 rsi...
				 16(%rsp)..				args
	Return:	error code
---------------------------------------------------------------- */

_GenericSWI:
		movl		_gCPSR(%rip), %eax
		andl		$mode_mask, %eax
		cmp		$usr_32, %eax
		je			1f
		leaq		zot(%rip), %rdi
		callq		_DebugStr$stub

1:		movl		$5, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	GenericWithReturnSWI(int inSelector, ULong inp1, ULong inp2, ULong inp3,
													 OpaqueRef * outp1, OpaqueRef * outp2, OpaqueRef * outp3)

	Handle a generic SWI that returns information.
	Args:		rdi	inSelector
				rsi	inp1
				rdx	inp2
				rcx	inp3
				r8		outp1
				r9		outp2

				+16	outp3
				+8		return address
				0		rbp <-- rbp
				-8		outp2 cache
				-16	outp1 cache
						16-byte alignment padding

	Return:	error code
---------------------------------------------------------------- */

_GenericWithReturnSWI:
		pushq		%rbp						# prolog
		movq		%rsp, %rbp
		pushq		%r9
		pushq		%r8

		movl		$5, %eax
		call		SWI
// results are returned in %rax and current task registers

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			9f
1:
		movq		-16(%rbp), %r9
		cmp		$0, %r9
		je			2f							# if outp1 != NULL
		movq		taskr1(%r11), %r8
		movq		%r8, 0(%r9)					# *outp1 = r1
2:
		movq		-8(%rbp), %r9
		cmp		$0, %r9
		je			3f							# if outp2 != NULL
		movq		taskr2(%r11), %r8
		movq		%r8, 0(%r9)				#   *outp2 = r2
3:
		movq		16(%rbp), %r9
		cmp		$0, %r9
		je			9f							# if outp3 != NULL
		movq		taskr3(%r11), %r8
		movq		%r8, 0(%r9)				#   *outp3 = r3
9:
		leave									# epilog
		ret


RegisterDelayedFunctionGlue:
//	5 args
		movl		_gCPSR(%rip), %eax
		andl		$mode_mask, %eax
		cmp		$usr_32, %eax
		je			1f
		leaq		zot(%rip), %rdi
		callq		_DebugStr$stub

1:		movl		$5, %eax
		jmp		SWI


RememberMappingUsingPAddrGlue:
//	5 args
		movl		_gCPSR(%rip), %eax
		andl		$mode_mask, %eax
		cmp		$usr_32, %eax
		je			1f
		leaq		zot(%rip), %rdi
		callq		_DebugStr$stub

1:		movl		$5, %eax
		jmp		SWI


_GenerateMessageIRQ:
		movl		$6, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	M M U
---------------------------------------------------------------- */
		.globl	PurgePageFromTLB
		.globl	FlushEntireTLB
		.globl	_PurgeMMUTLBEntry
		.globl	_FlushMMU
		.globl	FlushCache
		.globl	_SetDomainRegister
		.globl	TurnOffCache
		.globl	TurnOnCache

PurgePageFromTLB:
		ret

FlushEntireTLB:
		ret

_PurgeMMUTLBEntry:
		movl		$7, %eax
		jmp		SWI

_FlushMMU:
		movl		$8, %eax
		jmp		SWI

FlushCache:
_FlushIDC:
		movl		$9, %eax
		jmp		SWI

_SetDomainRegister:
		movl		$12, %eax
		jmp		SWI

TurnOffCache:
		movl		$24, %eax
		jmp		SWI

TurnOnCache:
		movl		$25, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	P o r t s
---------------------------------------------------------------- */
		.globl	_GetPortSWI
		.globl	_PortSendSWI
		.globl	_PortReceiveSWI
		.globl	_PortResetFilterSWI

/* ----------------------------------------------------------------
	ObjectId
	GetPortSWI(int inWhat)

	Return the ObjectId of a well-known port.
	Args:		 rdi	inWhat	0 => object manager
									1 => null
									2 => name server
	Return:	ObjectId of port
---------------------------------------------------------------- */

_GetPortSWI:
		movl		$0, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	PortSendSWI(ObjectId inId, ULong inMsgId, ULong inReplyId, ULong inMsgType, ULong inFlags)

	Send a message from a port.
	Args:		rdi	inId				port id
				rsi	inMsgId			message id
				rdx	inReplyId		reply id
				rcx	inMsgType		type of message
				r8		inFlags			flags
	Return:	error code
---------------------------------------------------------------- */

_PortSendSWI:
		movl		$1, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	PortReceiveSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter, ULong inFlags,
						ULong * outSenderMsgId, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)

	Receive a message at a port.
	Args:		rdi	inId
				rsi	inMsgId
				rdx	inMsgFilter
				rcx	inFlags
				r8		outSenderMsgId
				r9		outReplyMemId

				+24	outSignature
				+16	outMsgType
				+8		return address
				0		rbp <-- rbp
				-8		outReplyMemId cache
				-16	outSenderMsgId cache
						16-byte alignment padding

	Return:	error code
---------------------------------------------------------------- */

_PortReceiveSWI:
		pushq		%rbp						# prolog
		movq		%rsp, %rbp
		pushq		%r9
		pushq		%r8

		movl		$2, %eax
		call		SWI
// results are returned in %rax and current task registers

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			9f
1:
		movq		-16(%rbp), %r9
		cmp		$0, %r9
		je			2f							# if outMsgId != NULL
		movq		taskr1(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outMsgId = r1
2:
		movq		-8(%rbp), %r9
		cmp		$0, %r9
		je			3f							# if outReplyMemId != NULL
		movq		taskr2(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outReplyMemId = r2
3:
		movq		16(%rbp), %r9
		cmp		$0, %r9
		je			4f							# if outMsgType != NULL
		movq		taskr3(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outMsgType = r3
4:
		movq		24(%rbp), %r9
		cmp		$0, %r9
		je			9f							# if outSignature != NULL
		movq		taskr4(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outSignature = r4
9:
		leave									# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	PortResetFilterSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter)

	Reset the message filter on a port.
	Args:		rdi	inId					port id
				rsi	inMsgId				message id
				rdx	inMsgFilter			types of message we’re interested in
	Return:	error code
---------------------------------------------------------------- */

_PortResetFilterSWI:
		movl		$33, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	S e m a p h o r e s
---------------------------------------------------------------- */
		.globl	_SemaphoreOpGlue
		.globl	_Swap

/* ----------------------------------------------------------------
	NewtonErr
	SemaphoreOpGlue(ObjectId inGroupId, ObjectId inListId, SemFlags inBlocking)

	Atomically swap a memory location with a new value.
	Args:		rdi	inGroupId	semaphore group
				rsi	inListId		semaphore op list
				rdx	inBlocking	should wait if blocked?
	Return:	error code
---------------------------------------------------------------- */

_SemaphoreOpGlue:
		movl		$11, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	ULong
	Swap(ULong * ioAddr, ULong inValue)

	Atomically swap a memory location with a new value.
	Args:		rdi	ioAddr		the memory location
				rsi	inValue		its new value

	Return:	its former value
---------------------------------------------------------------- */

_Swap:
		xchg		%rsi, 0(%rdi)
		ret


/* ----------------------------------------------------------------
	S h a r e d   M e m o r y
---------------------------------------------------------------- */
		.globl	_SMemSetBufferSWI
		.globl	_SMemGetSizeSWI
		.globl	_SMemCopyToSharedSWI
		.globl	_SMemCopyFromSharedSWI
		.globl	_SMemMsgSetTimerParmsSWI
		.globl	_SMemMsgSetMsgAvailPortSWI
		.globl	_SMemMsgGetSenderTaskIdSWI
		.globl	_SMemMsgSetUserRefConSWI
		.globl	_SMemMsgGetUserRefConSWI
		.globl	_SMemMsgCheckForDoneSWI
		.globl	_SMemMsgMsgDoneSWI

/* ----------------------------------------------------------------
	NewtonErr
	SMemSetBufferSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inPermissions)

	Set shared memory buffer.
	Args:		rdi	inId
				rsi	inBuffer
				rdx	inSize
				rcx	inPermissions
	Return:	error code
---------------------------------------------------------------- */

_SMemSetBufferSWI:
		movl		$13, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemGetSizeSWI(ObjectId inId, size_t * outSize, void ** outBuffer, OpaqueRef * outRefCon)

	Get shared memory buffer size (and other info).
	Args:		rdi	inId
				rsi	outSize
				rdx	outBuffer
				rcx	outRefCon

				+8		return address
				0		rbp <-- rbp
				-8		outRefCon cache
				-16	outBuffer cache
				-24	outSize cache

	Return:	error code
---------------------------------------------------------------- */

_SMemGetSizeSWI:
		pushq		%rbp						# prolog
		movq		%rsp, %rbp
		pushq		%rcx
		pushq		%rdx
		pushq		%rsi
		subq		$8, %rsp					# 16-byte alignment padding

		movl		$14, %eax
		call		SWI
// results are returned in %rax and current task registers

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			9f
1:
		movq		-24(%rbp), %r9
		cmp		$0, %r9
		je			2f							# if outSize != NULL
		movq		taskr1(%r11), %r8
		movq		%r8, 0(%r9)				#   *outSize = r1
2:
		movq		-16(%rbp), %r9
		cmp		$0, %r9
		je			3f							# if outBuffer != NULL
		movq		taskr2(%r11), %r8
		movq		%r8, 0(%r9)			#   *outBuffer = r2
3:
		movq		-8(%rbp), %r9
		cmp		$0, %r9
		je			9f							# if outRefCon != NULL
		movq		taskr3(%r11), %r8
		movq		%r8, 0(%r9)				#   *outRefCon = r3
9:
		leave									# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemCopyToSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)

	Copy buffer to shared memory.
	Args:		rdi	inId
				rsi	inBuffer
				rdx	inSize
				rcx	inOffset
				r8		inSendersMsgId
				r9		inSendersSignature
	Return:	error code
---------------------------------------------------------------- */

_SMemCopyToSharedSWI:
		movl		$15, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemCopyFromSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature, size_t * outSize)

	Copy from shared memory to buffer.
	Args:		rdi	inId
				rsi	inBuffer
				rdx	inSize
				rcx	inOffset
				r8		inSendersMsgId
				r9		inSendersSignature

				+16	outSize
				+8		return address
				0		rbp <-- rbp
						16-byte alignment padding

	Return:	error code
---------------------------------------------------------------- */

_SMemCopyFromSharedSWI:
		pushq		%rbp						# prolog
		movq		%rsp, %rbp

		movl		$16, %eax
		call		SWI
// results are returned in %rax and current task registers

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			9f
1:
		movq		16(%rbp), %r9
		cmp		$0, %r9
		je			9f							# if outSize != NULL
		movq		taskr1(%r11), %r8
		movq		%r8, 0(%r9)				#   *outSize = r1
9:
		leave									# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgSetTimerParmsSWI(ObjectId inId, ULong inTimeout, int64_t inDelay)

	Set message timer.
	Args:		rdi	inId
				rsi	inTimeout
				rdx	inDelay
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgSetTimerParmsSWI:
		movl		$17, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgSetMsgAvailPortSWI(ObjectId inId, ObjectId inPortId)

	Set message available on port.
	Args:		rdi	inId
				rsi	inPortId
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgSetMsgAvailPortSWI:
		movl		$18, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgGetSenderTaskIdSWI(ObjectId inId, ObjectId * outSenderTaskId)

	Get message sender’s task id.
	Args:		rdi	inId
				rsi	outSenderTaskId

				+8		return address
				0		rbp <-- rbp
				-8		outSenderTaskId cache

	Return:	error code
---------------------------------------------------------------- */

_SMemMsgGetSenderTaskIdSWI:
		pushq		%rbp						# prolog
		movq		%rsp, %rbp
		pushq		%rsi
		subq		$8, %rsp					# 16-byte alignment padding

		movl		$19, %eax
		call		SWI
// results are returned in %rax and current task registers

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			9f
1:
		movq		-8(%rbp), %r9
		cmp		$0, %r9
		je			9f							# if outSenderTaskId != NULL
		movq		taskr1(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outSenderTaskId = r1
9:
		leave									# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgSetUserRefConSWI(ObjectId inId, ULong inRefCon)

	Set message refcon.
	Args:		rdi	inId
				rsi	inRefCon
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgSetUserRefConSWI:
		movl		$20, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgGetUserRefConSWI(ObjectId inId, OpaqueRef * outRefCon)

	Get message refcon.
	Args:		rdi	inId
				rsi	outRefCon

				+8		return address
				0		rbp <-- rbp
				-8		outRefCon cache

	Return:	error code
---------------------------------------------------------------- */

_SMemMsgGetUserRefConSWI:
		pushq		%rbp						# prolog
		movq		%rsp, %rbp
		pushq		%rsi
		subq		$8, %rsp					# 16-byte alignment padding

		movl		$21, %eax
		call		SWI
// results are returned in %eax and current task registers

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			9f
1:
		movq		-8(%rbp), %r9
		cmp		$0, %r9
		je			9f							# if outRefCon != NULL
		movq		taskr1(%r11), %r8
		movq		%r8, 0(%r9)				#   *outRefCon = r1
9:
		leave									# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgCheckForDoneSWI(ObjectId inId, ULong inFlags, ULong * outSentById, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)

	Check whether message is done.

	Args:		rdi	inId
				rsi	inFlags
				rdx	outSentById
				rcx	outReplyMemId
				r8		outMsgType
				r9		outSignature

				+8		return address
				0		rbp <-- rbp
				-8		outSignature cache
				-16	outMsgType cache
				-24	outReplyMemId cache
				-32	outSentById cache
						16-byte alignment padding

	Return:	error code
---------------------------------------------------------------- */

_SMemMsgCheckForDoneSWI:
		pushq		%rbp						# prolog
		movq		%rsp, %rbp
		pushq		%r9
		pushq		%r8
		pushq		%rcx
		pushq		%rdx

		movl		$22, %eax
		call		SWI
// results are returned in %eax and current task registers

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			9f
1:
		movq		-32(%rbp), %r9
		cmp		$0, %r9
		je			2f							# if outSentById != NULL
		movq		taskr1(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outSentById = r1
2:
		movq		-24(%rbp), %r9
		cmp		$0, %r9
		je			3f							# if outReplyMemId != NULL
		movq		taskr2(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outReplyMemId = r2
3:
		movq		-16(%rbp), %r9
		cmp		$0, %r9
		je			4f							# if outMsgType != NULL
		movq		taskr3(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outMsgType = r3
4:
		movq		-8(%rbp), %r9
		cmp		$0, %r9
		je			9f							# if outSignature != NULL
		movq		taskr4(%r11), %r8
		movl		%r8d, 0(%r9)			#   *outSignature = r4
9:
		leave									# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgMsgDoneSWI(ObjectId inId, NewtonErr inResult, ULong inSignature)

	Notify message done.
	Args:		rdi	inId
				rsi	inResult
				rdx	inSignature
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgMsgDoneSWI:
		movl		$23, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	M o n i t o r s
---------------------------------------------------------------- */
		.globl	_MonitorEntryGlue
		.globl	_MonitorDispatchSWI
		.globl	_MonitorExitSWI
		.globl	_MonitorThrowSWI
		.globl	_MonitorFlushSWI


/* ----------------------------------------------------------------
	NewtonErr
	MonitorEntryGlue(CTask * inContext, int inSelector, void * inData, ProcPtr inProc)

	Run a task member function as a monitor.
	Args:		%rdi	inContext		instance of task in which to run
				%rsi	inSelector
				%rdx	inData
				%rcx	inProc
	Return:	error code
---------------------------------------------------------------- */

_MonitorEntryGlue:
		movl		_gCPSR(%rip), %eax
		andl		$mode_mask, %eax
		cmp		$usr_32, %eax
		je			1f
		leaq		zotMon(%rip), %rdi
		callq		_DebugStr$stub
1:
		pushq		%rbp					# prolog
		movq		%rsp, %rbp

		call		*%rcx

// exit
		movq		%rax, %rdi
		movq		$0, %rsi				# ignored
		call		_MonitorExitSWI #(monitorResult, continuationPC)

		popq		%rbp					# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	MonitorDispatchSWI(ObjectId inMonitorId, int inSelector, OpaqueRef inData)

	Dispatch a message to a monitor.
	Args:		%rdi	inMonitorId
				%rsi	inSelector
				%rdx	inData
	Return:	error code
---------------------------------------------------------------- */

_MonitorDispatchSWI:
		movl		$27, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	MonitorExitSWI(int inMonitorResult, void * inContinuationPC)

	Exit a monitor.
	Args:		rdi	inMonitorResult
				rsi	inContinuationPC		ignored
	Return:	error code
---------------------------------------------------------------- */

_MonitorExitSWI:
		movl		$28, %eax
		jmp		SWI
// we should never return
		leaq		badMonExit(%rip), %rdi
		callq		_DebugStr$stub
1:		nop
		jmp		1b


/* ----------------------------------------------------------------
	void
	MonitorThrowSWI(ExceptionName inName, void * inData, ExceptionDestructor inDestructor)

	Throw an exception from a monitor.
	Args:		rdi	inName
				rsi	inData
				rdx	inDestructor
	Return:	--
---------------------------------------------------------------- */

_MonitorThrowSWI:
		movl		$29, %eax
		jmp		SWI
// we should never return
		leaq		badMonThrow(%rip), %rdi
		callq		_DebugStr$stub
1:		nop
		jmp		1b


/* ----------------------------------------------------------------
	NewtonErr
	MonitorFlushSWI(ObjectId inMonitorId)

	Flush messages on a monitor.
	Args:		rdi	inMonitorId
	Return:	error code
---------------------------------------------------------------- */

_MonitorFlushSWI:
		movl		$32, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	S c h e d u l e r
---------------------------------------------------------------- */
		.globl	_DoSchedulerSWI

/* ----------------------------------------------------------------
	NewtonErr
	DoSchedulerSWI(void)

	Actually, the scheduler gets updated on every SWI;
	we just need to provide a legitimate SWI.
	Args:		--
	Return:	error code
---------------------------------------------------------------- */

_DoSchedulerSWI:
		movl		$34, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	S T U B S
---------------------------------------------------------------- */

		.section __IMPORT,__jump_table,symbol_stubs,self_modifying_code+pure_instructions,5

_DebugStr$stub:
		.indirect_symbol _DebugStr
		hlt ; hlt ; hlt ; hlt ; hlt

