;	File:		SWIGlue.s
;
;	Contains:	SWI function call ARM assembler glue.
;
;	Written by:	Newton Research Group

		.text

;-------------------------------------------------------------------------------
;	M i s c e l l a n e o u s
;-------------------------------------------------------------------------------
		.globl	_SetAndClearBitsAtomic
		.globl	_LowLevelGetCPUType
		.globl	_LowLevelProcRevLevel
		.globl	_LowLevelProcSpeed
		.globl	_GetCPUMode
		.globl	_GetCPUVersion
		.globl	_IsSuperMode
		.globl	_EnterAtomicSWI
		.globl	_ExitAtomicSWI
		.globl	_EnterFIQAtomicSWI
		.globl	_ExitFIQAtomicSWI
		.globl	_GenericSWI
		.globl	_GenericWithReturnSWI

		.align	2

;	void SetAndClearBitsAtomic(ULong * ioTarget, ULong inSetBits, ULong inClearBits)
;
;	Perform atomic bit twiddling.
;	Args:		R0	ioTarget			bits to be twiddled
;				R1	inSetBits		bits to be set in target
;				R2	inClearBits		bits to be cleared in target
;	Return:	--

_SetAndClearBitsAtomic:
		MRS	R3, CPSR
		AND	R3, R3, #&C0
		CMP	R3, #&C0
		BEQ	1f
		STMDB	SP!, {R4-R6, LK}
		MOV	R4, R0
		MOV	R5, R1
		MOV	R6, R2
		BL		_EnterFIQAtomic
		LDR	R3, [R4]
		ORR	R3, R3, R5
		BIC	R3, R3, R6
		STR	R3, [R4]
		BL		_ExitFIQAtomic
		LDMIA	SP!, {R4-R6, PC}
1:
		LDR	R3, [R0]
		ORR	R3, R3, R1
		BIC	R3, R3, R2
		STR	R3, [R0]
		MOV	PC, LK


;	ULong LowLevelGetCPUType(void)
;
;	Return the processor type.
;	Args:		--
;	Return:	1 -> ARM610 = MP130
;				2 -> ARM710 = eMate
;				3 -> StrongARM = MP2x00

_LowLevelGetCPUType:
		MRC	Internal, R1, CR0
		BIC	R1, R1, #&0F
		EOR	R1, R1, #&44000000
		EOR	R1, R1, #&00010000
		EORS	R1, R1, #&0000A100
		MOVEQ	R0, #&03					; if CPU = D-1-A10 (StrongARM 1010 in MP2000) it’s 3

		MRC	Internal, R1, CR0
		BIC	R1, R1, #&0F
		EOR	R1, R1, #&41000000
		EOR	R1, R1, #&00040000
		EORS	R1, R1, #&00007100
		MOVEQ	R0, #&02					; if CPU = A-4-710 (ARM710 in eMate) it’s 2

		MOV	PC, LK					; default should be ARM610 in MP130; it’s 1


;	ULong LowLevelProcRevLevel(void)
;
;	Return the processor revision level.
;	Args:		--
;	Return:	0 | 1

_LowLevelProcRevLevel:
		MRC	Internal, R0, CR0
		AND	R1, R0, #&0F
		BIC	R0, R0, #&0F
		EOR	R0, R0, #&41000000
		EOR	R0, R0, #&00040000
		EORS	R0, R0, #&00007100
		CMPNE R1, #2
		MOVGE R0, #1					; if CPU = A-4-710 or rev >= 2 it’s 1
		MOVLT	R0, #0
		MOV	PC, LK


;	void LowLevelProcSpeed(ULong inNumOfIterations)
;
;	Perform some calculations we can time to work out the processor speed.
;	Args:		inNumOfIterations
;	Return:	--

_LowLevelProcSpeed:
		MOV	R1, #0
L1:	MOV	R2, #-256
		MOV	R3, #-16
		MUL	R2, R3, R2
		ADD	R1, R1, #1
		TEQ	R1, R0
		BNE	L1
		MOV	PC, LK


;	int GetCPUVersion(void)
;
;	Return the CPU version.
;	Args:		--
;	Return	R0		CPU version

_GetCPUVersion:
		SWI	10
		MOV	PC, LK


;	int GetCPUMode(void)
;
;	Return the CPU mode.
;	Args:		--
;	Return	R0		CPU mode

_GetCPUMode:
		MRS	R0, CPSR
		AND	R0, R0, #&1F
		MOV	PC, LK


;	BOOL IsSuperMode(void)
;
;	Determine whether we are in supervisor mode.
;	Args:		--
;	Return	R0		YES if in supervisor mode

_IsSuperMode:
		MRS	R0, CPSR
		AND	R0, R0, #&1F
		CMP	R0, #&10
		CMPNE	R0, #&00
		MOVNE	R0, 1
		MOVEQ	R0, 0
		MOV	PC, LK


;	Enter an atomic action.
;	Args:		--
;	Return:	--

_EnterAtomicSWI:
		SWI	3
		MOV	PC, LK

;	Exit an atomic action.
;	Args:		--
;	Return:	--

_ExitAtomicSWI:
		SWI	4
		MOV	PC, LK

;	Enter an atomic action.
;	Args:		--
;	Return:	--

_EnterFIQAtomicSWI:
		SWI	30
		MOV	PC, LK

;	Exit an atomic action.
;	Args:		--
;	Return:	--

_ExitFIQAtomicSWI:
		SWI	31
		MOV	PC, LK


;	void ClearFIQMask(void)

_ClearFIQMask:
		MRS	R0, SPSR
		BIC	R0, R0, #&40
		MSR	SPSR, R0
		NOP
		NOP
		MOV	PC, LK


;	NewtonErr GenericSWI(int inSelector, ...);
;
;	Handle a generic SWI.
;	Args:		R0	inSelector	generic SWI sub-selector
;				R1..				args
;	Return:	R0					error code

_GenericSWI:
		MRS	R12, CPSR
		AND	R12, R12, #&1F			; must be in user mode
		CMP	R12, #&10
		CMPNE	R12, #&00
		BEQ	1f
		DebugStr	"Zot! GenericSWI called from non-user mode."
1:		SWI	5
		MOV	PC, LK


;	NewtonErr GenericWithReturnSWI(int inSelector, ULong inp1, ULong inp2, ULong inp3,
;																ULong * outp1, ULong * outp2, ULong * outp3)
;
;	Handle a generic SWI that returns information.
;	Args:		R0	inSelector
;				R1	inp1
;				R2	inp2
;				R3	inp3
;				SP+00	outp1
;				SP+04	outp2
;				SP+08	outp3
;	Return:	R0				error code

_GenericWithReturnSWI:
		SWI	5						; shouldn’t we do a user-mode check as above?
		STMDB	SP!, {R0}			; save return code
		LDR	R0, [SP, #&04]
		CMP	R0, #&00000000		; if outp1 != NULL
		STRNE	R1, [R0]				;   *outp1 = r1
		LDR	R0, [SP, #&08]
		CMP	R0, #&00000000		; if outp2 != NULL
		STRNE	R2, [R0]				;   *outp2 = r2
		LDR	R0, [SP, #&0C]
		CMP	R0, #&00000000		; if outp3 != NULL
		STRNE	R3, [R0]				;   *outp3 = r3
		LDMIA	SP!, {R0}			; restore return code
		MOV	PC, LK


RegisterDelayedFunctionGlue:
		STMDB	SP!, {R4}
		LDR	R4, [SP, #&04]
		MRS	R12, CPSR
		AND	R12, R12, #&1F
		CMP	R12, #&10
		CMPNE	R12, #&00
		BEQ	1f
		DebugStr	"Zot!  GenericSWI called from non-user mode."
1:		SWI	5
		LDMIA	SP!, {R4}
		MOV	PC, LK


RememberMappingUsingPAddrGlue:
		STMDB	SP!, {R4}
		LDR	R4, [SP, #&04]
		MRS	R12, CPSR
		AND	R12, R12, #&1F
		CMP	R12, #&10
		CMPNE	R12, #&00
		BEQ	1f
		DebugStr	"Zot!  GenericSWI called from non-user mode."
1:		SWI	5
		LDMIA	SP!, {R4}
		MOV	PC, LK


_GenerateMessageIRQ:
		SWI	6
		MOV	PC, LK


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
003AD524 EE101F10 .... | MRC	Internal, R1, CR0
003AD528 E3C1100F .... | BIC	R1, R1, #&0000000F
003AD52C E2211311 .!.. | EOR	R1, R1, #&44000000
003AD530 E2211801 .!.. | EOR	R1, R1, #&00010000
003AD534 E2311CA1 .1.. | EORS	R1, R1, #&0000A100
003AD538 0E080F36 ...6 | ???EQ    ???
003AD53C 0E080F15 .... | ???EQ    ???
003AD540 1E060F16 .... | ???NE    ???
003AD544 E1A0F00E .... | MOV	PC, LK

FlushEntireTLB:
003AD548 EE101F10 .... | MRC	Internal, R1, CR0
003AD54C E3C1100F .... | BIC	R1, R1, #&0000000F
003AD550 E2211311 .!.. | EOR	R1, R1, #&44000000
003AD554 E2211801 .!.. | EOR	R1, R1, #&00010000
003AD558 E2311CA1 .1.. | EORS	R1, R1, #&0000A100
003AD55C 0E080F17 .... | ???EQ    ???
003AD560 1E050F15 .... | ???NE    ???
003AD564 E1A0F00E .... | MOV	PC, LK

_PurgeMMUTLBEntry:
		SWI	7
		MOV	PC, LK

_FlushMMU:
		SWI	8
		MOV	PC, LK

FlushCache:
_FlushIDC:
		SWI	9
		MOV	PC, LK

TurnOffCache:
		SWI	24
		MOV	PC, LK

TurnOnCache:
		SWI	25
		MOV	PC, LK

_SetDomainRegister:
		SWI	12
		MOV	PC, LK


;-------------------------------------------------------------------------------
;	P o r t s
;-------------------------------------------------------------------------------
		.globl	_GetPortSWI
		.globl	_PortSendSWI
		.globl	_PortReceiveSWI
		.globl	_PortResetFilterSWI
		.align	2

;	ObjectId GetPortSWI(int inWhat)
;
;	Return the ObjectId of a well-known port.
;	Args:		R0	inWhat	0 => object manager
;								1 => null
;								2 => name server
;	Return:	R0	port ObjectId

_GetPortSWI:
		SWI	0
		MOV	PC, LK


;	NewtonErr PortSendSWI(ObjectId inId, ULong inMsgId, ULong inReplyId, ULong inMsgType, ULong inFlags)
;
;	Send a message from a port.
;	Args:		R0	inId				port id
;				R1	inMsgId			message id
;				R2	inReplyId		reply id
;				R3	inMsgType		type of message
;				SP+00	inFlags		flags
;	Return:	R0						error code

_PortSendSWI:
		STMDB	SP!, {R4}
		LDR	R4, [SP, #&04]
		SWI	1
		LDMIA	SP!, {R4}
		MOV	PC, LK


;	NewtonErr PortReceiveSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter, ULong inFlags,
;									ULong * outSenderMsgId, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)
;
;	Receive a message at a port.
;	Args:		R0	inId					port id
;				R1	inMsgId				message id
;				R2	inMsgFilter			types of message we’re interested in
;				R3	inFlags				flags
;				SP+00	outSenderMsgId		sender info - sent message id
;				SP+04	outReplyMemId		- sent reply id
;				SP+08	outMsgType			- sent message type
;				SP+0C outSignature		- sent signature
;	Return:	R0						error code

_PortReceiveSWI:
		STMDB	SP!, {R4-R5}
		SWI	2
		LDR	R5, [SP, #&08]
		CMP	R5, #&00000000		; if outSenderMsgId != NULL
		STRNE	R1, [R5]				;   *outSenderMsgId = R1
		LDR	R5, [SP, #&0C]
		CMP	R5, #&00000000		; if outReplyMemId != NULL
		STRNE	R2, [R5]				;   *outReplyMemId = R2
		LDR	R5, [SP, #&10]
		CMP	R5, #&00000000		; if outMsgType != NULL
		STRNE	R3, [R5]				;   *outMsgType = R3
		LDR	R5, [SP, #&14]
		CMP	R5, #&00000000		; if outSignature != NULL
		STRNE	R4, [R5]				;   *outSignature = R4
		LDMIA	SP!, {R4-R5}
		MOV	PC, LK


;	NewtonErr PortResetFilterSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter)
;
;	Reset the message filter on a port.
;	Args:		R0	inId					port id
;				R1	inMsgId				message id
;				R2	inMsgFilter			types of message we’re interested in
;	Return:	R0							error code

_PortResetFilterSWI:
		SWI	33
		MOV	PC, LK


;-------------------------------------------------------------------------------
;	S e m a p h o r e s
;-------------------------------------------------------------------------------
		.globl	_SemaphoreOpGlue
		.globl	_Swap

		.align	2

;	NewtonErr SemaphoreOpGlue(ObjectId inGroupId, ObjectId inListId, SemFlags inBlocking)
;
;	Atomically swap a memory location with a new value.
;	Args:		R0	inGroupId	semaphore group
;				R1	inListId		semaphore op list
;				R2	inBlocking	should wait if blocked?
;	Return:	R0					error code

_SemaphoreOpGlue:
		SWI	11
		MOV	PC, LK


;	ULong Swap(ULong * ioAddr, ULong inValue)
;
;	Atomically swap a memory location with a new value.
;	Args:		R0	ioAddr		the memory location
;				R1	inValue		its new value
;	Return:	R0					its former value

_Swap:
		SWP	R0, R1, [R0]
		MOV	PC, LK


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
		.globl	LowLevelCopyEngine
		.globl	LowLevelCopyEngineLong

		.align	2

;	NewtonErr SMemSetBufferSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inPermissions)
;
;	Set shared memory buffer.
;	Args:		R0	inId
;				R1	inBuffer
;				R2	inSize
;				R3	inPermissions
;	Return:	R0	error code

_SMemSetBufferSWI:
		SWI	13
		MOV	PC, LK


;	NewtonErr SMemGetSizeSWI(ObjectId inId, size_t * outSize, void ** outBuffer, ULong * outRefCon)
;
;	Get shared memory buffer size (and other info).
;	Args:		R0	inId
;				R1	outSize
;				R2	outBuffer
;				R3	outRefCon
;	Return:	R0	error code

_SMemGetSizeSWI:
		STMDB	SP!, {R1-R3}
		SWI	14
		LDMIA	SP!, {R12}
		CMP	R12, #&00000000		; if outSize != NULL
		STRNE	R1, [R12]				;   *outSize = R1
		LDMIA	SP!, {R12}
		CMP	R12, #&00000000		; if outBuffer != NULL
		STRNE	R2, [R12]				;   *outBuffer = R2
		LDMIA	SP!, {R12}
		CMP	R12, #&00000000		; if outRefCon != NULL
		STRNE	R3, [R12]				;   *outRefCon = R3
		MOV	PC, LK


;	NewtonErr SMemCopyToSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)
;
; Copy buffer to shared memory.
;	Args:		R0	inId
;				R1	inBuffer
;				R2	inSize
;				R3	inOffset
;				SP+00	inSendersMsgId
;				SP+04	inSendersSignature
;	Return:	R0	error code

_SMemCopyToSharedSWI:
		STMDB	SP!, {R4-R5}
		LDR	R4, [SP, #&00000008]
		LDR	R5, [SP, #&0000000C]
		SWI	15
		LDMIA	SP!, {R4-R5}
		MOV	PC, LK


;	NewtonErr SMemCopyFromSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature, size_t * outSize)
;
; Copy from shared memory to buffer.
;	Args:		R0	inId
;				R1	inBuffer
;				R2	inSize
;				R3	inOffset
;				SP+00	inSendersMsgId
;				SP+04	inSendersSignature
;				SP+08	outSize
;	Return:	R0	error code

_SMemCopyFromSharedSWI:
		STMDB	SP!, {R4-R5}
		LDR	R4, [SP, #&08]		; pass args in registers
		LDR	R5, [SP, #&0C]
		SWI	16
		LDMIA	SP!, {R4-R5}
		LDR	R2, [SP, #&08]
		CMP	R2, #&00000000		; if outSize != NULL
		STRNE	R1, [R2]				;   *outSize = R1
		MOV	PC, LK


;	NewtonErr SMemMsgSetTimerParmsSWI(ObjectId inId, ULong inTimeout, ULong inTimeLo, ULong inTimeHi)
;
;	Set message timer.
;	Args:		R0	inId
;				R1	inTimeout
;				R2	inTimeLo
;				R3	inTimeHi
;	Return	R0	error code

_SMemMsgSetTimerParmsSWI:
		SWI	17
		MOV	PC, LK


;	NewtonErr SMemMsgSetMsgAvailPortSWI(ObjectId inId, ObjectId inPortId)
;
;	Set message available on port.
;	Args:		R0	inId
;				R1	inPortId
;	Return	R0	error code

_SMemMsgSetMsgAvailPortSWI:
		SWI	18
		MOV	PC, LK


;	NewtonErr SMemMsgGetSenderTaskIdSWI(ObjectId inId, ObjectId * outSenderTaskId)
;
;	Get message sender’s task id.
;	Args:		R0	inId
;				R1	outSenderTaskId
;	Return	R0	error code

_SMemMsgGetSenderTaskIdSWI:
		STMDB	SP!, {R1}
		SWI	19
		LDMIA	SP!, {R2}
		STR	R1, [R2]
		MOV	PC, LK


;	NewtonErr SMemMsgSetUserRefConSWI(ObjectId inId, ULong inRefCon)
;
;	Set message refcon.
;	Args:		R0	inId
;				R1	inRefCon
;	Return	R0	error code

_SMemMsgSetUserRefConSWI:
		SWI	20
		MOV	PC, LK


;	NewtonErr SMemMsgGetUserRefConSWI(ObjectId inId, ULong * outRefCon)
;
;	Get message refcon.
;	Args:		R0	inId
;				R1	outRefCon
;	Return	R0	error code

_SMemMsgGetUserRefConSWI:
		STMDB	SP!, {R1}
		SWI	21
		LDMIA	SP!, {R2}
		STR	R1, [R2]
		MOV	PC, LK


;	NewtonErr SMemMsgCheckForDoneSWI(ObjectId inId, ULong inFlags, ULong * outSentById, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)
;
;	Check whether message is done.
;	Args:		R0	inId
;				R1	inFlags
;				R2	outSentById
;				R3	outReplyMemId
;				SP+00	outMsgType
;				SP+04	outSignature
;	Return	R0	error code

_SMemMsgCheckForDoneSWI:
		STMDB	SP!, {R2-R5}
		SWI	22
		LDR	R5, [SP]
		CMP	R5, #&00000000		; if outSentById != NULL
		STRNE	R1, [R5]				;   *outSentById = R1
		LDR	R5, [SP, #&04]
		CMP	R5, #&00000000		; if outReplyMemId != NULL
		STRNE	R2, [R5]				;   *outReplyMemId = R2
		LDR	R5, [SP, #&10]
		CMP	R5, #&00000000		; if outMsgType != NULL
		STRNE	R3, [R5]				;   *outMsgType = R3
		LDR	R5, [SP, #&14]
		CMP	R5, #&00000000		; if outSignature != NULL
		STRNE	R4, [R5]				;   *outSignature = R4
		LDR	R4, [SP, #&08]
		LDR	R5, [SP, #&0C]
		ADD	SP, SP, #&10
		MOV	PC, LK


;	NewtonErr SMemMsgMsgDoneSWI(ObjectId inId, long inResult, ULong inSignature)
;
;	Notify message done.
;	Args:		R0	inResult
;				R1	inSignature
;	Return	R0	error code

_SMemMsgMsgDoneSWI:
		SWI	23
		MOV	PC, LK


;	void LowLevelCopyEngine(char * inTo, char * inFrom, size_t inSize)
;
;	Copy memory, char-at-a-time.
;	Args:		R0	inTo
;				R1	inFrom
;				R2	inSize
;	Return	--

LowLevelCopyEngine:
		CMP	R2, #0
		SWIEQ	26
		LDRB	R3, [R1], #1
		STRB	R3, [R0], #1
		SUBS	R2, R2, #1
		B		LowLevelCopyEngine


;	void LowLevelCopyEngineLong(char * inTo, char * inFrom, size_t inSize)
;
;	Copy memory, long-at-a-time.
;	Args:		R0	inTo
;				R1	inFrom
;				R2	inSize
;	Return	--

LowLevelCopyEngineLong:
		CMP	R2, #0
		SWIEQ	26
		LDR	R3, [R1], #4
		STR	R3, [R0], #4
		SUBS	R2, R2, #4
		B		LowLevelCopyEngineLong


;-------------------------------------------------------------------------------
;	M o n i t o r s
;-------------------------------------------------------------------------------

;	NewtonErr MonitorDispatchSWI(ObjectId inMonitorId, int inSelector, void * inData)
;
;	Dispatch a message to a monitor.
;	Args:		R0	inMonitorId
;				R1	inSelector
;				R2	inData
;	Return:	R0	error code

_MonitorDispatchSWI:
		SWI	27
		MOV	PC, LK


;	NewtonErr MonitorExitSWI(long inMonitorResult, void * inContinuationPC)
;
;	Exit a monitor.
;	Args:		R0	inMonitorResult
;				R1	inContinuationPC
;	Return:	R0	error code

_MonitorExitSWI:
		SWI	28
		DebugStr	"MonitorExitSWI failed!  This should never happen..."


;	void MonitorThrowSWI(char * inName, void * inData, void * inDestructor)
;
;	Throw an exception from a monitor.
;	Args:		R0	inName
;				R1	inData
;				R2	inDestructor
;	Return:	--

_MonitorThrowSWI:
		SWI	29
		DebugStr	"MonitorThrowSWI failed; check your head."


;	NewtonErr MonitorFlushSWI(ObjectId inMonitorId)
;
;	Flush messages on a monitor.
;	Args:		R0	inMonitorId
;	Return:	R0	error code

_MonitorFlushSWI:
		SWI	32
		MOV	PC, LK


;-------------------------------------------------------------------------------
;	S c h e d u l e r
;-------------------------------------------------------------------------------

;	void DoSchedulerSWI(void)
;
;	Actually, the scheduler gets updated on every SWI; we just need to provide
;	a legitimate SWI.
;	Args:		--
;	Return:	--

_DoSchedulerSWI:
		SWI	34
		MOV	PC, LK

