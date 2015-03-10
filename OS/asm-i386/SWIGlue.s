/*
	File:		SWIGlue.s

	Contains:	SWI function call i386 assembler glue.
					All the functions in here have extern "C" linkage.
					See http://developer.apple.com/mac/library/documentation/DeveloperTools/Reference/Assembler/000-Introduction/introduction.html
					 or http://developer.apple.com/mac/library/documentation/DeveloperTools/Conceptual/LowLevelABI/130-IA-32_Function_Calling_Conventions/IA32.html
					for the ABI.

	Written by:	Newton Research Group, 2009.
*/

//	register offsets in CTask
		.set	taskr1,	 0x44		// kReturnParm1
		.set	taskr2,	 0x48
		.set	taskr3,	 0x4C
		.set	taskr4,	 0x50


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
	Args:		 4(%esp)	ioTarget			bits to be twiddled
				 8(%esp)	inSetBits		bits to be set in target
				12(%esp)	inClearBits		bits to be cleared in target
	Return:	--
---------------------------------------------------------------- */

_SetAndClearBitsAtomic:
		// assume super mode, no interrupts
		jmp		1f

		// were we in user mode we’d need to:
		pushl		%ebp						# prolog
		movl		%esp, %ebp
		subl		$(8+16), %esp			# keep stack 16-byte aligned

		call		_EnterFIQAtomic
		movl		8(%ebp), %edx
		movl		0(%edx), %eax
		orl		12(%ebp), %eax
		andl		16(%ebp), %eax
		movl		%eax, 0(%edx)
		call		_ExitFIQAtomic

		addl		$(8+16), %esp
		popl		%ebp						# epilog
		ret

1:
		movl		4(%esp), %edx
		movl		0(%edx), %eax
		orl		8(%esp), %eax
		andl		12(%esp), %eax
		movl		%eax, 0(%edx)
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
---------------------------------------------------------------- */

_LowLevelGetCPUType:
		movl		$5, %eax
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
	Args:		 4(%esp)	inNumOfIterations
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
		movl		_gCPUmode, %eax
		ret


/* ----------------------------------------------------------------
	bool
	IsSuperMode(void)

	Determine whether we are in supervisor mode.
	Args:		--
	Return:	YES if in supervisor mode
---------------------------------------------------------------- */

_IsSuperMode:
		movl		_gCPUmode, %eax
		cmpl		$0, %eax
		je			1f
		movl		$1, %eax
1:		ret


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
		ret


/* ----------------------------------------------------------------
	DebugStr strings
---------------------------------------------------------------- */
		.cstring
		.globl	undefinedSWI
		.globl	zot
		.globl	badMonExit
		.globl	badMonThrow
		.globl	tooManyAtomicExits
		.globl	tooManyFIQExits

undefinedSWI:
		.asciz	"Undefined SWI"
zot:
		.asciz	"Zot! GenericSWI called from non-user mode."
zotMon:
		.asciz	"Zot! Check SVC mode in MonitorEntryGlue."
badMonExit:
		.asciz	"MonitorExitSWI failed! This should never happen..."
badMonThrow:
		.asciz	"MonitorThrowSWI failed; check your head."
tooManyAtomicExits:
		.asciz	"Exit Atomic called too many times!!!"
tooManyFIQExits:
		.asciz	"Exit FIQ Atomic called too many times!!!"

/* ------------------------------------------------------------- */

		.text
		.align	2

/* ----------------------------------------------------------------
	NewtonErr
	GenericSWI(int inSelector, ...)

	Handle a generic SWI.
	Args:		 4(%esp)	inSelector	generic SWI sub-selector
				 8(%esp)..				args
	Return:	error code
---------------------------------------------------------------- */

_GenericSWI:
		movl		_gCPUmode, %eax
		cmp		$0, %eax
		je			1f
		subl		$12, %esp
		leal		zot, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp

1:		movl		$5, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	SWI functions that return values can not do that in registers
	in the i386 architecture. So we need to read from the current
	task’s registers when writing back return values.
---------------------------------------------------------------- */

/* ----------------------------------------------------------------
	NewtonErr
	GenericWithReturnSWI(int inSelector, ULong inp1, ULong inp2, ULong inp3,
													 ULong * outp1, ULong * outp2, ULong * outp3)

	Handle a generic SWI that returns information.
	Args:		outp3				+32
				outp2				+28
				outp1				+24
				inp3				+20
				inp2				+16
				inp1				+12
				inSelector		 +8

				 return address +4
				 ebp			<-- ebp
				 eax cache		 -4
				 ecx cache		 -8
	Return:	error code
---------------------------------------------------------------- */

_GenericWithReturnSWI:
		pushl		%ebp						# prolog
		movl		%esp, %ebp
		subl		$8, %esp

		movl		$5, %eax
		call		SWIandReturn
// results are returned in %eax and current task registers
		movl		%eax, -4(%ebp)
		movl		%ecx, -8(%ebp)
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		jne		9f
		jmp		3f
9:		movl		24(%ebp), %ecx
		cmp		$0, %ecx
		je			1f							# if outp1 != NULL
		movl		taskr1(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outp1 = r1
1:
		movl		28(%ebp), %ecx
		cmp		$0, %ecx
		je			2f							# if outp2 != NULL
		movl		taskr2(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outp2 = r2
2:
		movl		32(%ebp), %ecx
		cmp		$0, %ecx
		je			3f							# if outp3 != NULL
		movl		taskr3(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outp3 = r3
3:
		movl		-8(%ebp), %ecx
		movl		-4(%ebp), %eax
		addl		$8, %esp
		popl		%ebp						# epilog
		ret


RegisterDelayedFunctionGlue:
//	5 args
		movl		_gCPUmode, %eax
		cmp		$0, %eax
		je			1f
		subl		$12, %esp
		leal		zot, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp

1:		movl		$5, %eax
		jmp		SWI


RememberMappingUsingPAddrGlue:
//	5 args
		movl		_gCPUmode, %eax
		cmp		$0, %eax
		je			1f
		subl		$12, %esp
		leal		zot, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp

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
	Args:		 4(%esp)	inWhat	0 => object manager
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
	Args:		20(%esp)	inFlags			flags
				16(%esp)	inMsgType		type of message
				12(%esp)	inReplyId		reply id
				 8(%esp)	inMsgId			message id
				 4(%esp)	inId				port id
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
	Args:		outSignature		+36	- sent signature
				outMsgType			+32	- sent message type
				outReplyMemId		+28	- sent reply id
				outSenderMsgId		+24	sender info - sent message id
				inFlags				+20	flags
				inMsgFilter			+16	types of message we’re interested in
				inMsgId				+12	message id
				inId					 +8	port id

				 return address +4
				 ebp			<-- ebp
				 eax cache		 -4
				 ecx cache		 -8
	Return:	error code
---------------------------------------------------------------- */

_PortReceiveSWI:
		pushl		%ebp						# prolog
		movl		%esp, %ebp
		subl		$8, %esp

		movl		$2, %eax
		call		SWIandReturn
// results are returned in %eax and current task registers
		movl		%eax, -4(%ebp)
		movl		%ecx, -8(%ebp)
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		jne		9f
		jmp		4f
9:		movl		24(%ebp), %ecx
		cmp		$0, %ecx
		je			1f							# if outMsgId != NULL
		movl		taskr1(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outMsgId = r1
1:
		movl		28(%ebp), %ecx
		cmp		$0, %ecx
		je			2f							# if outReplyMemId != NULL
		movl		taskr2(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outReplyMemId = r2
2:
		movl		32(%ebp), %ecx
		cmp		$0, %ecx
		je			3f							# if outMsgType != NULL
		movl		taskr3(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outMsgType = r3
3:
		movl		36(%ebp), %ecx
		cmp		$0, %ecx
		je			4f							# if outSignature != NULL
		movl		taskr4(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outSignature = r4
4:
		movl		-8(%ebp), %ecx
		movl		-4(%ebp), %eax
		addl		$8, %esp
		popl		%ebp						# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	PortResetFilterSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter)

	Reset the message filter on a port.
	Args:		12(%esp)	inMsgFilter			types of message we’re interested in
				 8(%esp)	inMsgId				message id
				 4(%esp)	inId					port id
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
	Args:		12(%esp)	inBlocking	should wait if blocked?
				 8(%esp)	inListId		semaphore op list
				 4(%esp)	inGroupId	semaphore group
	Return:	error code
---------------------------------------------------------------- */

_SemaphoreOpGlue:
		movl		$11, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	ULong
	Swap(ULong * ioAddr, ULong inValue)

	Atomically swap a memory location with a new value.
	Args:		 8(%esp)	inValue		its new value
				 4(%esp)	ioAddr		the memory location
		
	Return:	its former value
---------------------------------------------------------------- */

_Swap:
		movl		4(%esp), %edx
		movl		8(%esp), %eax
		xchg		%eax, 0(%edx)
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
	Args:		16(%esp)	inPermissions
				12(%esp)	inSize
				 8(%esp)	inBuffer
				 4(%esp)	inId
	Return:	error code
---------------------------------------------------------------- */

_SMemSetBufferSWI:
		movl		$13, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemGetSizeSWI(ObjectId inId, size_t * outSize, void ** outBuffer, ULong * outRefCon)

	Get shared memory buffer size (and other info).
	Args:		outRefCon		+20
				outBuffer		+16
				outSize			+12
				inId				 +8

				 return address +4
				 ebp			<-- ebp
				 eax cache		 -4
				 ecx cache		 -8
	Return:	error code
---------------------------------------------------------------- */

_SMemGetSizeSWI:
		pushl		%ebp						# prolog
		movl		%esp, %ebp
		subl		$8, %esp

		movl		$14, %eax
		call		SWIandReturn
// results are returned in %eax and current task registers
		movl		%eax, -4(%ebp)
		movl		%ecx, -8(%ebp)
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		jne		9f
		jmp		3f
9:		movl		12(%ebp), %ecx
		cmp		$0, %ecx
		je			1f							# if outSize != NULL
		movl		taskr1(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outSize = r1
1:
		movl		16(%ebp), %ecx
		cmp		$0, %ecx
		je			2f							# if outBuffer != NULL
		movl		taskr2(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outBuffer = r2
2:
		movl		20(%ebp), %ecx
		cmp		$0, %ecx
		je			3f							# if outRefCon != NULL
		movl		taskr3(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outRefCon = r3
3:
		movl		-8(%ebp), %ecx
		movl		-4(%ebp), %eax
		addl		$8, %esp
		popl		%ebp						# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemCopyToSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)

	Copy buffer to shared memory.
	Args:		24(%esp)	inSendersSignature
				20(%esp)	inSendersMsgId
				16(%esp)	inOffset
				12(%esp)	inSize
				 8(%esp)	inBuffer
				 4(%esp)	inId
	Return:	error code
---------------------------------------------------------------- */

_SMemCopyToSharedSWI:
		movl		$15, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemCopyFromSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature, size_t * outSize)

	Copy from shared memory to buffer.
	Args:		outSize			+32
				inSendersSignature	+28
				inSendersMsgId	+24
				inOffset			+20
				inSize			+16
				inBuffer			+12
				inId				 +8

				 return address +4
				 ebp			<-- ebp
				 eax cache		 -4
				 ecx cache		 -8
	Return:	error code
---------------------------------------------------------------- */

_SMemCopyFromSharedSWI:
		pushl		%ebp						# prolog
		movl		%esp, %ebp
		subl		$8, %esp

		movl		$16, %eax
		call		SWIandReturn
// results are returned in %eax and current task registers
		movl		%eax, -4(%ebp)
		movl		%ecx, -8(%ebp)
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		jne		9f
		jmp		1f
9:		movl		32(%ebp), %ecx
		cmp		$0, %ecx
		je			1f							# if outSize != NULL
		movl		taskr1(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outSize = r1
1:
		movl		-8(%ebp), %ecx
		movl		-4(%ebp), %eax
		addl		$8, %esp
		popl		%ebp						# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgSetTimerParmsSWI(ObjectId inId, ULong inTimeout, ULong inTimeLo, ULong inTimeHi)

	Set message timer.
	Args:		16(%esp)	inTimeHi
				12(%esp)	inTimeLo
				 8(%esp)	inTimeout
				 4(%esp)	inId
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgSetTimerParmsSWI:
		movl		$17, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgSetMsgAvailPortSWI(ObjectId inId, ObjectId inPortId)

	Set message available on port.
	Args:		 8(%esp)	inPortId
				 4(%esp)	inId
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgSetMsgAvailPortSWI:
		movl		$18, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgGetSenderTaskIdSWI(ObjectId inId, ObjectId * outSenderTaskId)

	Get message sender’s task id.
	Args:		outSenderTaskId+12
				inId				 +8

				 return address +4
				 ebp			<-- ebp
				 eax cache		 -4
				 ecx cache		 -8
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgGetSenderTaskIdSWI:
		pushl		%ebp						# prolog
		movl		%esp, %ebp
		subl		$8, %esp

		movl		$19, %eax
		call		SWIandReturn
// results are returned in %eax and current task registers
		movl		%eax, -4(%ebp)
		movl		%ecx, -8(%ebp)
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		jne		9f
		jmp		1f
9:		movl		12(%ebp), %ecx
		cmpl		$0, %ecx
		je			1f							# if outSenderTaskId != NULL
		movl		taskr1(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outSenderTaskId = r1
1:
		movl		-8(%ebp), %ecx
		movl		-4(%ebp), %eax
		addl		$8, %esp
		popl		%ebp						# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgSetUserRefConSWI(ObjectId inId, ULong inRefCon)

	Set message refcon.
	Args:		 4(%esp)	inId
				 8(%esp)	inRefCon
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgSetUserRefConSWI:
		movl		$20, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgGetUserRefConSWI(ObjectId inId, ULong * outRefCon)

	Get message refcon.
	Args:		outRefCon		+12
				inId				 +8

				 return address +4
				 ebp			<-- ebp
				 eax cache		 -4
				 ecx cache		 -8
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgGetUserRefConSWI:
		pushl		%ebp						# prolog
		movl		%esp, %ebp
		subl		$8, %esp

		movl		$21, %eax
		call		SWIandReturn
// results are returned in %eax and current task registers
		movl		%eax, -4(%ebp)
		movl		%ecx, -8(%ebp)
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		jne		9f
		jmp		1f
9:		movl		12(%ebp), %ecx
		cmp		$0, %ecx
		je			1f							# if outRefCon != NULL
		movl		taskr1(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outRefCon = r1
1:
		movl		-8(%ebp), %ecx
		movl		-4(%ebp), %eax
		addl		$8, %esp
		popl		%ebp						# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgCheckForDoneSWI(ObjectId inId, ULong inFlags, ULong * outSentById, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)

	Check whether message is done.
	Args:		outSignature	+28
				outMsgType		+24
				outReplyMemId	+20
				outSentById		+16
				inFlags			+12
				inId				 +8

				 return address +4
				 ebp			<-- ebp
				 eax cache		 -4
				 ecx cache		 -8
	Return:	error code
---------------------------------------------------------------- */

_SMemMsgCheckForDoneSWI:
		pushl		%ebp						# prolog
		movl		%esp, %ebp
		subl		$8, %esp

		movl		$22, %eax
		call		SWIandReturn
// results are returned in %eax and current task registers
		movl		%eax, -4(%ebp)
		movl		%ecx, -8(%ebp)
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		jne		9f
		jmp		4f
9:		movl		16(%ebp), %ecx
		cmp		$0, %ecx
		je			1f							# if outSentById != NULL
		movl		taskr1(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outSentById = r1
1:
		movl		20(%ebp), %ecx
		cmp		$0, %ecx
		je			2f							# if outReplyMemId != NULL
		movl		taskr2(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outReplyMemId = r2
2:
		movl		24(%ebp), %ecx
		cmp		$0, %ecx
		je			3f							# if outMsgType != NULL
		movl		taskr3(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outMsgType = r3
3:
		movl		28(%ebp), %ecx
		cmp		$0, %ecx
		je			4f							# if outSignature != NULL
		movl		taskr4(%edx), %eax
		movl		%eax, 0(%ecx)			#   *outSignature = r4
4:
		movl		-8(%ebp), %ecx
		movl		-4(%ebp), %eax
		addl		$8, %esp
		popl		%ebp						# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	SMemMsgMsgDoneSWI(ObjectId inId, NewtonErr inResult, ULong inSignature)

	Notify message done.
	Args:		 8(%esp)	inSignature
				 4(%esp)	inResult
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
	Args:		inProc			+16
				inData			+12
				inSelector		 +8
				inContext		 +4	instance of task in which to run

				 ebp			<-- ebp
	Return:	error code
---------------------------------------------------------------- */

_MonitorEntryGlue:
		movl		_gCPUmode, %eax
		cmp		$0, %eax
		je			1f
		subl		$12, %esp
		leal		zotMon, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp
1:
		pushl		%ebp					# prolog
		movl		%esp, %ebp
		subl		$12, %esp			# keep stack 16-byte aligned…
		pushl		16(%ebp)
		pushl		12(%ebp)
		pushl		 8(%ebp)
		call		*20(%ebp)			#…at this point

		addl		$24, %esp

// exit
		subl		$4, %esp
		pushl		%eax
		call		_MonitorExitSWI
		addl		$8, %esp
		popl		%ebp					# epilog
		ret


/* ----------------------------------------------------------------
	NewtonErr
	MonitorDispatchSWI(ObjectId inMonitorId, int inSelector, void * inData)

	Dispatch a message to a monitor.
	Args:		12(%esp)	inData
				 8(%esp)	inSelector
				 4(%esp)	inMonitorId
	Return:	error code
---------------------------------------------------------------- */

_MonitorDispatchSWI:
		movl		$27, %eax
		jmp		SWI


/* ----------------------------------------------------------------
	NewtonErr
	MonitorExitSWI(int inMonitorResult, void * inContinuationPC)

	Exit a monitor.
	Args:		 8(%esp)	inContinuationPC
				 4(%esp)	inMonitorResult
	Return:	error code
---------------------------------------------------------------- */

_MonitorExitSWI:
		movl		$28, %eax
		jmp		SWI
// we should never return
		subl		$12, %esp
		leal		badMonExit, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp
1:		nop
		jmp		1b


/* ----------------------------------------------------------------
	void
	MonitorThrowSWI(ExceptionName inName, void * inData, ExceptionDestructor inDestructor)

	Throw an exception from a monitor.
	Args:		12(%esp)	inDestructor
				 8(%esp)	inData
		 		 4(%esp)	inName
	Return:	--
---------------------------------------------------------------- */

_MonitorThrowSWI:
		movl		$29, %eax
		jmp		SWI
// we should never return
		subl		$12, %esp
		leal		badMonThrow, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp
1:		nop
		jmp		1b


/* ----------------------------------------------------------------
	NewtonErr
	MonitorFlushSWI(ObjectId inMonitorId)

	Flush messages on a monitor.
	Args:		 4(%esp)	inMonitorId
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

