#if defined(__i386__)
#include "../asm-i386/SWIGlue.s"

#else
;	File:		SWIGlue.s
;
;	Contains:	SWI function call PPC assembler glue.
;					See http://developer.apple.com/documentation/DeveloperTools/Conceptual/PowerPCRuntime/index.html
;					 or http://the.wall.riscom.net/books/proc/ppc/cwg/a_abi.html
;					for the ABI.
;
;	Written by:	Newton Research Group

		.text
		.align	2

;-------------------------------------------------------------------------------
;	M i s c e l l a n e o u s
;-------------------------------------------------------------------------------
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

;	void SetAndClearBitsAtomic(ULong * ioTarget, ULong inSetBits, ULong inClearBits)
;
;	Perform atomic bit twiddling.
;	Args:		r3	ioTarget			bits to be twiddled
;				r4	inSetBits		bits to be set in target
;				r5	inClearBits		bits to be cleared in target
;	Return:	--

_SetAndClearBitsAtomic:
		li		r0, 0xC0				; assume super mode, no interrupts!
		andi.	r0, r0, 0xC0
		cmpwi	cr7, r0, 0xC0
		beq+	cr7, 1f

; prolog
		mflr	r0						; get LR
		stmw	r29, -12(r1)		; save working registers
		stw	r0, 8(r1)			; save LR
		stwu	r1, -80(r1)			; create activation record

		mr		r29, r3				; save args across function call
		mr		r30, r4
		mr		r31, r5
		bl		_EnterFIQAtomic
		lwz	r0, 0(r29)
		or		r0, r0, r30
		andc	r0, r0, r31
		stw	r0, 0(r29)
		bl		_ExitFIQAtomic

; epilog
		lwz	r1, 0(r1)			; delete activation record
		lwz	r0, 8(r1)			; restore LR
		lmw	r29, -12(r1)		; restore registers
		mtlr	r0
		blr
1:
		lwz	r0, 0(r3)
		or		r0, r0, r4
		andc	r0, r0, r5
		stw	r0, 0(r3)
		blr


;	ULong LowLevelGetCPUType(void)
;
;	Return the processor type.
;	Args:		--
;	Return:	1 -> ARM610 = MP130
;				2 -> ARM710 = eMate
;				3 -> StrongARM = MP2x00
;				4 -> PPC
;				5 -> x86

_LowLevelGetCPUType:
		li		r3, 4
		blr


;	ULong LowLevelProcRevLevel(void)
;
;	Return the processor revision level.
;	Args:		--
;	Return:	0 | 1

_LowLevelProcRevLevel:
		li		r3, 0
		blr


;	void LowLevelProcSpeed(ULong inNumOfIterations)
;
;	Perform some calculations we can time to work out the processor speed.
;	Args:		inNumOfIterations
;	Return:	--

_LowLevelProcSpeed:
;		MOV	R1, #0
;1:	MOV	R2, #-256
;		MOV	R3, #-16
;		MUL	R2, R3, R2
;		ADD	R1, R1, #1
;		TEQ	R1, R0
;		BNE	L1
		blr


;	int GetCPUVersion(void)
;
;	Return the CPU version.
;	Args:		--
;	Return	r3		CPU version

_GetCPUVersion:
		li		r0, 10
		b		SWI


;	int GetCPUMode(void);
;
;	Return the CPU mode.
;	Args:		--
;	Return	r3		CPU mode

_GetCPUMode:
		lis	r3, ha16(_gCPUmode)
		lwz	r3, lo16(_gCPUmode)(r3)
		blr


;	BOOL IsSuperMode(void);
;
;	Determine whether we are in supervisor mode.
;	Args:		--
;	Return	r3		YES if in supervisor mode

_IsSuperMode:
		lis	r3, ha16(_gCPUmode)
		lwz	r3, lo16(_gCPUmode)(r3)
		cmpwi	cr7, r3, 0x00
		beq-	cr7, 1f
		li		r3, 1
1:		blr


;	Enter an atomic action.
;	Args:		--
;	Return:	--

_EnterAtomicSWI:
		li		r0, 3
		b		SWI

;	Exit an atomic action.
;	Args:		--
;	Return:	--

_ExitAtomicSWI:
		li		r0, 4
		b		SWI

;	Enter an atomic action.
;	Args:		--
;	Return:	--

_EnterFIQAtomicSWI:
		li		r0, 30
		b		SWI

;	Exit an atomic action.
;	Args:		--
;	Return:	--

_ExitFIQAtomicSWI:
		li		r0, 31
		b		SWI


;	void ClearFIQMask(void)

_ClearFIQMask:
		blr


;-------------------------------------------------------------------------------
;	DebugStr strings

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

;-------------------------------------------------------------------------------

		.text
		.align	2

;	NewtonErr GenericSWI(int inSelector, ...);
;
;	Handle a generic SWI.
;	Args:		r3	inSelector	generic SWI sub-selector
;				r4..				args
;	Return:	r3					error code

_GenericSWI:
		stw	r3, -4(r1)
		lis	r3, ha16(_gCPUmode)
		lwz	r3, lo16(_gCPUmode)(r3)
		cmpwi	cr7, r3, 0x00
		lwz	r3, -4(r1)
		beq-	cr7, 1f
		addis	r3, 0, hi16(zot)
		ori	r3, r3, lo16(zot)
		b		_DebugStr$stub

1:		li		r0, 5
		b		SWI


;	NewtonErr GenericWithReturnSWI(int inSelector, ULong inp1, ULong inp2, ULong inp3,
;																ULong * outp1, ULong * outp2, ULong * outp3)
;
;	Handle a generic SWI that returns information.
;	Args:		r3	inSelector
;				r4	inp1
;				r5	inp2
;				r6	inp3
;				r7	outp1
;				r8	outp2
;				r9	outp3
;	Return:	r3				error code

_GenericWithReturnSWI:
		mflr	r0					; prolog
		stmw	r29, -12(r1)
		stw	r0, 8(r1)
		stwu	r1, -80(r1)

		mr		r29, r7
		mr		r30, r8
		mr		r31, r9
		li		r0, 5
		bl		SWI
; results are returned in r3 and r4..r6
		cmpwi	cr7, r29, 0
		beq-	cr7, 1f			; if outp1 != NULL
		stw	r4, 0(r29)		;   *outp1 = r4
1:		cmpwi	cr7, r30, 0
		beq-	cr7, 2f			; if outp2 != NULL
		stw	r5, 0(r30)		;   *outp2 = r5
2:		cmpwi	cr7, r31, 0
		beq-	cr7, 3f			; if outp3 != NULL
		stw	r6, 0(r31)		;   *outp3 = r6
3:
		lwz	r1, 0(r1)		; epilog
		lwz	r0, 8(r1)
		lmw	r29, -12(r1)
		mtlr	r0
		blr


RegisterDelayedFunctionGlue:
;	5 args; ARM unstacks R4
		stw	r3, -4(r1)
		lis	r3, ha16(_gCPUmode)
		lwz	r3, lo16(_gCPUmode)(r3)
		cmpwi	cr7, r3, 0x00
		lwz	r3, -4(r1)
		beq-	cr7, 1f
		addis	r3, 0, hi16(zot)
		ori	r3, r3, lo16(zot)
		b		_DebugStr$stub

1:		li		r0, 5
		b		SWI


RememberMappingUsingPAddrGlue:
;	5 args; ARM unstacks R4
		stw	r3, -4(r1)
		lis	r3, ha16(_gCPUmode)
		lwz	r3, lo16(_gCPUmode)(r3)
		cmpwi	cr7, r3, 0x00
		lwz	r3, -4(r1)
		beq-	cr7, 1f
		addis	r3, 0, hi16(zot)
		ori	r3, r3, lo16(zot)
		b		_DebugStr$stub

1:		li		r0, 5
		b		SWI


_GenerateMessageIRQ:
		li		r0, 6
		b		SWI


;-------------------------------------------------------------------------------
;	M M U
;-------------------------------------------------------------------------------
		.globl	PurgePageFromTLB
		.globl	FlushEntireTLB
		.globl	_PurgeMMUTLBEntry
		.globl	_FlushMMU
		.globl	FlushCache
		.globl	TurnOffCache
		.globl	TurnOnCache
		.globl	_SetDomainRegister

PurgePageFromTLB:
		blr

FlushEntireTLB:
		blr

_PurgeMMUTLBEntry:
		li		r0, 7
		b		SWI

_FlushMMU:
		li		r0, 8
		b		SWI

FlushCache:
_FlushIDC:
		li		r0, 9
		b		SWI

TurnOffCache:
		li		r0, 24
		b		SWI

TurnOnCache:
		li		r0, 25
		b		SWI

_SetDomainRegister:
		li		r0, 12
		b		SWI


;-------------------------------------------------------------------------------
;	P o r t s
;-------------------------------------------------------------------------------
		.globl	_GetPortSWI
		.globl	_PortSendSWI
		.globl	_PortReceiveSWI
		.globl	_PortResetFilterSWI

;	ObjectId GetPortSWI(int inWhat)
;
;	Return the ObjectId of a well-known port.
;	Args:		r3	inWhat	0 => object manager
;								1 => null
;								2 => name server
;	Return:	r3	port ObjectId

_GetPortSWI:
		li		r0, 0
		b		SWI


;	NewtonErr PortSendSWI(ObjectId inId, ULong inMsgId, ULong inReplyId, ULong inMsgType, ULong inFlags)
;
;	Send a message from a port.
;	Args:		r3	inId				port id
;				r4	inMsgId			message id
;				r5	inReplyId		reply id
;				r6	inMsgType		type of message
;				r7	inFlags			flags
;	Return:	r3						error code

_PortSendSWI:
		li		r0, 1
		b		SWI


;	NewtonErr PortReceiveSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter, ULong inFlags,
;									ULong * outSenderMsgId, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)
;
;	Receive a message at a port.
;	Args:		r3	inId					port id
;				r4	inMsgId				message id
;				r5	inMsgFilter			types of message we’re interested in
;				r6	inFlags				flags
;				r7	outSenderMsgId		sender info - sent message id
;				r8	outReplyMemId		- sent reply id
;				r9	outMsgType			- sent message type
;				r10 outSignature		- sent signature
;	Return:	r3						error code

_PortReceiveSWI:
		mflr	r0					; prolog
		stmw	r28, -16(r1)
		stw	r0, 8(r1)
		stwu	r1, -80(r1)

		mr		r28, r7
		mr		r29, r8
		mr		r30, r9
		mr		r31, r10
		li		r0, 2
		bl		SWI
; results are returned in r3 and r4-r7
		cmpwi	cr7, r28, 0
		beq-	cr7, 1f			; if outMsgId != NULL
		stw	r4, 0(r28)		;   *outMsgId = r4
1:		cmpwi	cr7, r29, 0
		beq-	cr7, 2f			; if outReplyId != NULL
		stw	r5, 0(r29)		;   *outReplyId = r5
2:		cmpwi	cr7, r30, 0
		beq-	cr7, 3f			; if outMsgType != NULL
		stw	r6, 0(r30)		;   *outMsgType = r6
3:		cmpwi	cr7, r31, 0
		beq-	cr7, 4f			; if outSignature != NULL
		stw	r7, 0(r31)		;   *outSignature = r7
4:
		lwz	r1, 0(r1)		; epilog
		lwz	r0, 8(r1)
		lmw	r28, -16(r1)
		mtlr	r0
		blr


;	NewtonErr PortResetFilterSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter)
;
;	Reset the message filter on a port.
;	Args:		r3	inId					port id
;				r4	inMsgId				message id
;				r5	inMsgFilter			types of message we’re interested in
;	Return:	r3							error code

_PortResetFilterSWI:
		li		r0, 33
		b		SWI


;-------------------------------------------------------------------------------
;	S e m a p h o r e s
;-------------------------------------------------------------------------------
		.globl	_SemaphoreOpGlue
		.globl	_Swap

;	NewtonErr SemaphoreOpGlue(ObjectId inGroupId, ObjectId inListId, SemFlags inBlocking)
;
;	Atomically swap a memory location with a new value.
;	Args:		r3	inGroupId	semaphore group
;				r4	inListId		semaphore op list
;				r5	inBlocking	should wait if blocked?
;	Return:	r3					error code

_SemaphoreOpGlue:
		li		r0, 11
		b		SWI


;	ULong Swap(ULong * ioAddr, ULong inValue)
;
;	Atomically swap a memory location with a new value.
;	NOTE	SWI MUST RESET RESERVED
;	Args:		r3	ioAddr		the memory location
;				r4	inValue		its new value
;	Return:	r3					its former value

_Swap:
		lwarx	 r0, 0,r3		; load and reserve
		stwcx. r4, 0,r3		; store new value if still reserved
		bne-	 _Swap			; loop if lost reservation
		mr		r3, r0			; return old value
		blr


;-------------------------------------------------------------------------------
;	S h a r e d   M e m o r y
;-------------------------------------------------------------------------------
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

;	NewtonErr SMemSetBufferSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inPermissions)
;
;	Set shared memory buffer.
;	Args:		r3	inId
;				r4	inBuffer
;				r5	inSize
;				r6	inPermissions
;	Return:	r3	error code

_SMemSetBufferSWI:
		li		r0, 13
		b		SWI


;	NewtonErr SMemGetSizeSWI(ObjectId inId, size_t * outSize, void ** outBuffer, ULong * outRefCon)
;
;	Get shared memory buffer size (and other info).
;	Args:		r3	inId
;				r4	outSize
;				r5	outBuffer
;				r6	outRefCon
;	Return:	r3	error code

_SMemGetSizeSWI:
		mflr	r0					; prolog
		stmw	r29, -12(r1)
		stw	r0, 8(r1)
		stwu	r1, -80(r1)

		mr		r29, r4
		mr		r30, r5
		mr		r31, r6
		li		r0, 14
		bl		SWI
; results are returned in r3 and r4..r6
		cmpwi	cr7, r29, 0
		beq-	cr7, 1f			; if outSize != NULL
		stw	r4, 0(r29)		;   *outSize = r4
1:		cmpwi	cr7, r30, 0
		beq-	cr7, 2f			; if outBuffer != NULL
		stw	r5, 0(r30)		;   *outBuffer = r5
2:		cmpwi	cr7, r31, 0
		beq-	cr7, 3f			; if outRefCon != NULL
		stw	r6, 0(r31)		;   *outRefCon = r6
3:
		lwz	r1, 0(r1)		; epilog
		lwz	r0, 8(r1)
		lmw	r29, -12(r1)
		mtlr	r0
		blr


;	NewtonErr SMemCopyToSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)
;
;	Copy buffer to shared memory.
;	Args:		r3	inId
;				r4	inBuffer
;				r5	inSize
;				r6	inOffset
;				r7	inSendersMsgId
;				r8	inSendersSignature
;	Return:	r3	error code

_SMemCopyToSharedSWI:
		li		r0, 15
		b		SWI


;	NewtonErr SMemCopyFromSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature, size_t * outSize)
;
;	Copy from shared memory to buffer.
;	Args:		r3	inId
;				r4	inBuffer
;				r5	inSize
;				r6	inOffset
;				r7	inSendersMsgId
;				r8	inSendersSignature
;				r9	outSize
;	Return:	r3	error code

_SMemCopyFromSharedSWI:
		mflr	r0					; prolog
		stmw	r30, -8(r1)
		stw	r0, 8(r1)
		stwu	r1, -80(r1)

		mr		r31, r9
		li		r0, 16
		bl		SWI
		cmpwi	cr7, r31, 0
		beq-	cr7, 1f			; if outSize != NULL
		stw	r4, 0(r31)		;   *outSize = r4
1:
		lwz	r1, 0(r1)		; epilog
		lwz	r0, 8(r1)
		lmw	r30, -8(r1)
		mtlr	r0
		blr


;	NewtonErr SMemMsgSetTimerParmsSWI(ObjectId inId, ULong inTimeout, ULong inTimeLo, ULong inTimeHi)
;
;	Set message timer.
;	Args:		r3	inId
;				r4	inTimeout
;				r5	inTimeLo
;				r6	inTimeHi
;	Return	r3	error code

_SMemMsgSetTimerParmsSWI:
		li		r0, 17
		b		SWI


;	NewtonErr SMemMsgSetMsgAvailPortSWI(ObjectId inId, ObjectId inPortId)
;
;	Set message available on port.
;	Args:		r3	inId
;				r4	inPortId
;	Return	r3	error code

_SMemMsgSetMsgAvailPortSWI:
		li		r0, 18
		b		SWI


;	NewtonErr SMemMsgGetSenderTaskIdSWI(ObjectId inId, ObjectId * outSenderTaskId)
;
;	Get message sender’s task id.
;	Args:		r3	inId
;				r4	outSenderTaskId
;	Return	r3	error code

_SMemMsgGetSenderTaskIdSWI:
		mflr	r0					; prolog
		stmw	r30, -8(r1)
		stw	r0, 8(r1)
		stwu	r1, -80(r1)

		mr		r31, r4
		li		r0, 19
		bl		SWI
		stw	r4, 0(r31)		; *outSenderTaskId = r4

		lwz	r1, 0(r1)		; epilog
		lwz	r0, 8(r1)
		lmw	r30, -8(r1)
		mtlr	r0
		blr


;	NewtonErr SMemMsgSetUserRefConSWI(ObjectId inId, ULong inRefCon)
;
;	Set message refcon.
;	Args:		r3	inId
;				r4	inRefCon
;	Return	r3	error code

_SMemMsgSetUserRefConSWI:
		li		r0, 20
		b		SWI


;	NewtonErr SMemMsgGetUserRefConSWI(ObjectId inId, ULong * outRefCon)
;
;	Get message refcon.
;	Args:		r3	inId
;				r4	outRefCon
;	Return	r3	error code

_SMemMsgGetUserRefConSWI:
		mflr	r0					; prolog
		stmw	r30, -8(r1)
		stw	r0, 8(r1)
		stwu	r1, -80(r1)

		mr		r31, r4
		li		r0, 21
		bl		SWI
		stw	r4, 0(r31)		; *outRefCon = r4

		lwz	r1, 0(r1)		; epilog
		lwz	r0, 8(r1)
		lmw	r30, -8(r1)
		mtlr	r0
		blr


;	NewtonErr SMemMsgCheckForDoneSWI(ObjectId inId, ULong inFlags, ULong * outSentById, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)
;
;	Check whether message is done.
;	Args:		r3	inId
;				r4	inFlags
;				r5	outSentById
;				r6	outReplyMemId
;				r7	outMsgType
;				r8	outSignature
;	Return	r3	error code

_SMemMsgCheckForDoneSWI:
		mflr	r0					; prolog
		stmw	r28, -16(r1)
		stw	r0, 8(r1)
		stwu	r1, -80(r1)

		li		r0, 22
		bl		SWI
; results are returned in r3 and r4-r7
		cmpwi	cr7, r28, 0
		beq-	cr7, 1f			; if outSentById != NULL
		stw	r4, 0(r28)		;   *outSentById = r4
1:		cmpwi	cr7, r29, 0
		beq-	cr7, 2f			; if outReplyMemId != NULL
		stw	r5, 0(r29)		;   *outReplyMemId = r5
2:		cmpwi	cr7, r30, 0
		beq-	cr7, 3f			; if outMsgType != NULL
		stw	r6, 0(r30)		;   *outMsgType = r6
3:		cmpwi	cr7, r31, 0
		beq-	cr7, 4f			; if outSignature != NULL
		stw	r7, 0(r31)		;   *outSignature = r7
4:
		lwz	r1, 0(r1)		; epilog
		lwz	r0, 8(r1)
		lmw	r28, -16(r1)
		mtlr	r0
		blr


;	NewtonErr SMemMsgMsgDoneSWI(ObjectId inId, long inResult, ULong inSignature)
;
;	Notify message done.
;	Args:		r3	inResult
;				r4	inSignature
;	Return	r3	error code

_SMemMsgMsgDoneSWI:
		li		r0, 23
		b		SWI


;-------------------------------------------------------------------------------
;	M o n i t o r s
;-------------------------------------------------------------------------------
		.globl	_MonitorEntryGlue
		.globl	_MonitorDispatchSWI
		.globl	_MonitorExitSWI
		.globl	_MonitorThrowSWI
		.globl	_MonitorFlushSWI

/*------------------------------------------------------------------------------
	Assembler.
------------------------------------------------------------------------------*/

;	NewtonErr MonitorDispatchSWI(ObjectId inMonitorId, int inSelector, void * inData)
;
;	Dispatch a message to a monitor.
;	Args:		r3	inMonitorId
;				r4	inSelector
;				r5	inData
;	Return:	r3	error code

_MonitorDispatchSWI:
		li		r0, 27
		b		SWI


;	NewtonErr MonitorEntryGlue(CTask * inContext, int inSelector, void * inData, ProcPtr inProc)
;
;	Initialize a task as a monitor.
;	Args:		r3	inContext			instance of task in which to run
;				r4	inSelector		
;				r5	inData
;				r6	inProc
;	Return:	r3	error code

_MonitorEntryGlue:
		stw	r3, -4(r1)
		lis	r3, ha16(_gCPUmode)
		lwz	r3, lo16(_gCPUmode)(r3)
		cmpwi	cr7, r3, 0x00
		lwz	r3, -4(r1)
		beq-	cr7, 1f
		addis	r3, 0, hi16(zotMon)
		ori	r3, r3, lo16(zotMon)
		b		_DebugStr$stub
1:
		mflr	r0					; prolog
		stmw	r29, -12(r1)
		stw	r0, 8(r1)
		stwu	r1, -80(r1)

		mtctr	r6					; call inProc
		bctrl

		lwz	r1, 0(r1)		; epilog
		lwz	r0, 8(r1)
		lmw	r29, -12(r1)
		mtlr	r0
; fall thru to exit glue…


;	NewtonErr MonitorExitSWI(long inMonitorResult, void * inContinuationPC)
;
;	Exit a monitor.
;	Args:		r3	inMonitorResult
;				r4	inContinuationPC
;	Return:	r3	error code

_MonitorExitSWI:
		li		r0, 28
		b		SWI
		addis	r3, 0, hi16(badMonExit)
		ori	r3, r3, lo16(badMonExit)
		b		_DebugStr$stub


;	void MonitorThrowSWI(ExceptionName inName, void * inData, ExceptionDesctructor inDestructor)
;
;	Throw an exception from a monitor.
;	Args:		r3	inName
;				r4	inData
;				r5	inDestructor
;	Return:	--

_MonitorThrowSWI:
		li		r0, 29
		b		SWI
		addis	r3, 0, hi16(badMonThrow)
		ori	r3, r3, lo16(badMonThrow)
		b		_DebugStr$stub


;	NewtonErr MonitorFlushSWI(ObjectId inMonitorId)
;
;	Flush messages on a monitor.
;	Args:		r3	inMonitorId
;	Return:	r3	error code

_MonitorFlushSWI:
		li		r0, 32
		b		SWI


;-------------------------------------------------------------------------------
;	S c h e d u l e r
;-------------------------------------------------------------------------------
		.globl	_DoSchedulerSWI

;	void DoSchedulerSWI(void)
;
;	Actually, the scheduler gets updated on every SWI; we just need to provide
;	a legitimate SWI.
;	Args:		--
;	Return:	--

_DoSchedulerSWI:
		li		r0, 34
		b		SWI


;-------------------------------------------------------------------------------
; S T U B S

		.symbol_stub
_DebugStr$stub:
		.indirect_symbol _DebugStr
		lis     r11,ha16(_DebugStr$lazyPtr)
		lwz     r12,lo16(_DebugStr$lazyPtr)(r11)
		mtctr   r12
		addi    r11,r11,lo16(_DebugStr$lazyPtr)
		bctr

		.lazy_symbol_pointer
_DebugStr$lazyPtr:
		.indirect_symbol _DebugStr
		.long   dyld_stub_binding_helper

#endif
