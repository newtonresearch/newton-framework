;	File:		SWIBoot.s
;
;	Contains:	SWI handler ARM assembler glue.
;
;	Written by:	Newton Research Group

		.text
		.globl	_SWIBoot
		.align	2

;	Handle the SWI.
;	Args:		R0..			args as passed from C, but all in registers
;	Return:	R0..			result(s)

SWIBoot:									; 003AD698
		STMDB	SP!, {R0-R1}			; save working registers
		LDR	R0, [LK, #-4]			; fetch SWI instruction causing this call
		AND	R1, R0, #&0F000000	; it should be 0xEFxxxxxx
		TEQ	R1, #&0F000000
		BNE	InvalidInstr
		AND	R1, R0, #&F0000000
		TEQ	R1, #&E0000000
		BNE	L003ADD7C

		MRS	R1, SPSR					; MUST be in user mode
		AND	R1, R1, #&1F
		CMP	R1, #&10
		CMPNE	R1, #&00
		BEQ	1f
		DebugStr "SWI from non-user mode (rebooting)."
		MSR	CPSR, #&D3
		NOP
		NOP
		B		&00000000
1:
		MOV	R1,     #&00000055
		ADD	R1, R1, #&00005500
		ADD	R1, R1, #&00550000
		ADD	R1, R1, #&55000000
		STMDB	SP!, {R0-R1}			; stack SWI instr, R/W DCR access

		MOV	R0, R1
		LDR	R1, [&003ADE98]	; 0C008400
		STR	R0, [R1, #&B0]		;g0C0084B0 = access
		LDMIA	SP!, {R0-R1}
		MCR	Internal, R1, Environment

		LDR	R1, [&003ADE9C]	; if gAtomicFIQNestCountFast == 0
		LDR	R1, [R1]
		CMP	R1, #0
		LDREQ	R1, [&003ADEA0]		; && gAtomicIRQNestCountFast == 0
		LDREQ	R1, [R1]
		CMPEQ	R1, #0
		LDREQ	R1, [&003ADEA4]		; && gAtomicNestCount == 0
		LDREQ	R1, [R1]
		CMPEQ	R1, #0
		LDREQ	R1, [&003ADEA8]		; && gAtomicFIQNestCount == 0
		LDREQ	R1, [R1]
		CMPEQ	R1, #0
		BGT	2f
		MSR	CPSR, #&13				;   not nested so re-enable IRQ | FIQ
		NOP
		NOP
2:											; 003AD734
		MOV	R1, LK					; fetch SWI instruction again
		LDR	R1, [R1, #-4]
		BIC	R1, R1, #&FF000000	; switch on SWI selector
		CMP	R1, #35					; 0..34
		BGE	UndefinedSWI
		LDR	R0, [pSWI_JT]
		LDR	PC, [R0, R1 LSL 2]	; jump to handler
;	Actual R0-R1 are on the stack.
;	SWI handler jumps back here when done.
Done:										; 003AD750
;	SWI work is done.
;	If scheduled, switch task now.

		STMDB	SP!, {R0-R1}			; stack R0-R1 for saving in task registers later
		MSR	CPSR, #&D3				; disable IRQ | FIQ
		NOP
		NOP

		LDR	R1, [&003ADE9C]	; gAtomicFIQNestCountFast
		LDR	R1, [R1]
		CMP	R1, #0
		LDREQ	R1, [&003ADEA0]	; gAtomicIRQNestCountFast
		LDREQ	R1, [R1]
		CMPEQ	R1, #0
		LDREQ	R1, [&003ADEA4]	; gAtomicNestCount
		LDREQ	R1, [R1]
		CMPEQ	R1, #0
		LDREQ	R1, [&003ADEA8]	; gAtomicFIQNestCount
		LDREQ	R1, [R1]
		CMPEQ	R1, #0
		BGT	NoSwap2				; if in nested interrupt, don’t swap!
		MSR	CPSR, #&93			; not nested so re-enable FIQ (but not IRQ)
		NOP
		NOP
		LDR	R1, [&003ADEAC]	; gWantDeferred
		LDR	R1, [R1]
		CMP	R1, #0
		BNE	DoDeferred
		LDR	R1, [&003ADEB0]		; gDoSchedule
		LDR	R1, [R1]
		CMP	R1, #0
		BEQ	NoSwap2

		STMDB	SP!, {R2-R3, R10-R12, LK}
		B		DoScheduled
; this section can be organized more logically, surely?
DoDeferred:
		STMDB	SP!, {R2-R3, R10-R12, LK}

		MSR	CPSR, #&13	; enable IRQ | FIQ
		NOP
		NOP
		BL		_DoDeferrals
		MSR	CPSR, #&93	; disable IRQ again
		NOP
		NOP

		LDR	R1, [&003ADEB0]		; gDoSchedule
		LDR	R1, [R1]
		CMP	R1, #0
		BEQ	NoSwap

;	gDoSchedule == YES
DoScheduled:
		BL		_Scheduler				; R0 = next task to run
		LDR	R1, [&003ADEB4]		; gWantSchedulerToRun?
		LDR	R1, [R1]
		CMP	R1, #0
		BEQ	1f
		STMDB	SP!, {R0}				; YES, preserve task in R0
		BL		_StartScheduler		; and start scheduler
		LDMIA	SP!, {R0}
1:		LDR	R1, [&003ADEB8]		; gCurrentTask
		LDR	R1, [R1]
		CMP	R0, R1					; if task hasn’t changed, don’t do task swap
		BEQ	NoSwap

		LDR	R2, [&003ADEB8]		; gCurrentTask
		STR	R0, [R2]					; update gCurrentTask
		CMP	R1, #&00000000
		ADDEQ	SP, SP, #&00000020 (32)
		BEQ	SwapIn					; if no previous task, don’t swap it out

SwapOut:
		LDMIA	SP!, {R2-R3, R10-R12, LK}
		MOV	R0, R1					; R0 = previous gCurrentTask;
		LDR	R1, [&003ADEBC]		; gCopyDone
		LDR	R1, [R1]
		CMP	R1, #0
		BNE	L003AD8E0
;	gCopyDone == NO
		MRS	R1, SPSR
		STR	R1, [R0, #&50]		; save PSR in task
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; EQ => undefined mode
		ADD	R1, R0, #&18		; R1 = &fRegister[2]

		STMDB	SP!, {R0}			; save task pointer
		MRS	R0, CPSR				; copy current CPSR
		MSR	CPSR, #&D3			; disable IRQ | FIQ
		NOP
		NOP
		STMNEIA	R1!, {R2-R7}	; save registers in task
		STMNEIA	R1, {R8-LK}^	; save user mode registers
		NOP
		MSR	CPSR, R0				; restore CPSR
		NOP
		NOP
		LDMIA	SP!, {R0}			; restore task pointer
		BNE	L003AD8CC
										; else PSR => undefined mode
		STMIA	R1!, {R2-R12}		; save R2-R12
		NOP
		MRS	R4, CPSR
		MRS	R5, SPSR
		MSR	CPSR, R5				; use SPSR
		NOP
		NOP
		STMIA	R1, {SP-LK}			; save R13-R14_undef
		NOP
		MSR	CPSR, R4				; restore CPSR
		NOP
		NOP
L003AD8CC:
		ADD	R1, R0, #&10
		STR	LK, [R1, #&3C]		; save PC
		LDMIA	SP!, {R2-R3}
		STMIA	R1, {R2-R3}			; save stacked R0-R1
		B		SwapIn

;	gCopyDone == YES
L003AD8E0:
		MOV	R1, R0
		ADD	R0, R0, #&4C
		LDR	LK, [R0]				; restore saved PC in LK for return
		ADD	R0, R1, #&10		; R0 -> fRegister[0]
		LDMIA	R0, {R2-R3}
		STMIA	SP, {R2-R3}			; stack saved R0-R1
		MRS	R1, SPSR
		STR	R1, [R0, #&50]		; save SPSR
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; EQ => undefined mode
		ADD	R1, R0, #&18		; R1 = &fRegister[2]

		STMDB	SP!, {R0}			; save task pointer
		MRS	R0, CPSR				; copy current CPSR
		MSR	CPSR, #&D3			; disable IRQ | FIQ
		NOP
		NOP
		STMNEIA	R1!, {R2-R7}	; save registers in task
		STMNEIA	R1, {R8-LK}^	; save user mode registers
		NOP
		MSR	CPSR, R0				; restore CPSR
		NOP
		NOP
		LDMIA	SP!, {R0}			; restore task pointer
		BNE	L003AD974
										; else PSR => undefined mode
		STMIA	R1!, {R2-R12}		; save R2-R12
		NOP
		MRS	R4, CPSR
		MRS	R5, SPSR
		MSR	CPSR, R5				; use SPSR
		NOP
		NOP
		STMIA	R1, {SP-LK}			; save R13-R14_undef
		NOP
		MSR	CPSR, R4				; restore CPSR
		NOP
		NOP
L003AD974:
		ADD	R1, R0, #&10
		STR	LK, [R1, #&3C]		; save PC
		LDMIA	SP!, {R2-R3}
		STMIA	R1, {R2-R3}			; save stacked R0-R1
		LDR	R0, [&003ADEBC]	; gCopyDone
		MOV	R1, #0
		STR	R1, [R0]

; previous task state is saved
; now set up next task state

SwapIn:								; 003AD990
		LDR	R0, [&003ADEB8]	; gCurrentTask
		LDR	R0, [R0]
		MOV	R4, R0
		BL		_SwapInGlobals		; SwapInGlobals(gCurrentTask)
		MOV	R0, R4
;	restore processor registers from new (current) task
		ADD	R1, R0, #&10
		LDR	LK, [R0, #&3C]		; restore saved PC in LK for return
		LDR	R1, [R0, #&40]
		MSR	SPSR, R1				; set saved PSR
		NOP
		NOP
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; EQ => undefined mode

		LDMNEIA	R0, {R0-LK}^	; restore all user mode registers
		NOP
		BNE	L003AD9FC
										; else PSR => undefined mode
		LDR	R2, [R0, #&40]
		MRS	R1, CPSR						; save CPSR
		MSR	CPSR, R2						; set saved PSR
		NOP
		NOP
		LDR	SP, [R0, #&34]				; set saved SP_undef
		LDR	LK, [R0, #&38]				; set saved LK_undef
		MSR	CPSR, R1						; restore CPSR
		NOP
		NOP
		LDMIA	R0, {R0-R12}				; set saved registers
L003AD9FC:
		STMDB	SP!, {R0-R2}				; preserve working registers
		LDR	R1, [&003ADEB8]			; gCurrentTask
		LDR	R1, [R1]
		MOV	R0, #0						; access = 0

		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2

		LDR	R2, [R1, #&78]				; env = task->fSMemEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
1:
		LDR	R1, [R1, #&7C]				; while ((task = task->fLink) != nil)
		CMP	R1, #0
		BEQ	2f
		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
		BNE	1b
2:
		STMDB	SP!, {R0-R1}
		NOP
		LDR	R1, [&003ADE98]			; g0C008400
		STR	R0, [R1, #&B0]				; g0C0084B0 = access
		LDMIA	SP!, {R0-R1}

		MCR	Internal, R0, Environment
		LDMIA	SP!, {R0-R2}				; restore working registers
		MOVS	PC, LK						; return to task!

;	gDoSchedule == NO
;	no task switch required
;	set up domain access anyways

NoSwap:										; 003ADA70
		LDMIA	SP!, {R2-R3, R10-R12, LK}
NoSwap2:										; 003ADA74
		STMDB	SP!, {R2-R3, R12, LK}
		LDR	R1, [&003ADEB4]			; gWantSchedulerToRun?
		LDR	R1, [R1]
		CMP	R1, #0
		BLNE	_StartScheduler			; YES, start scheduler
		LDMIA	SP!, {R2-R3, R12, LK}

		LDR	R0, [&003ADEBC]		; gCopyDone
		LDR	R0, [R0]
		CMP	R0, #0
		BNE	CopyIsDone

;	gCopyDone == NO
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R0-R2}				; preserve working registers
		LDR	R1, [&003ADEB8]			; gCurrentTask
		MOV	R0, #0						; access = 0

		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2

		LDR	R2, [R1, #&78]				; env = task->fSMemEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
1:
		LDR	R1, [R1, #&7C]				; while ((task = task->fLink) != nil)
		CMP	R1, #0
		BEQ	2f
		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
		BNE	1b
2:
		STMDB	SP!, {R0-R1}
		NOP
		LDR	R1, [g0C008400]
		STR	R0, [R1, #&B0]				; g0C0084B0 = access
		LDMIA	SP!, {R0-R1}

		MCR	Internal, R0, Environment
		LDMIA	SP!, {R0-R2}				; restore working registers
		MOVS	PC, LK						; return to task!

;	gCopyDone == YES
CopyIsDone:
		LDR	R0, [&003ADEBC]		; gCopyDone
		MOV	R1, #0
		STR	R1, [R0]					;  = NO

		LDR	R0, [&003ADEB8]		; gCurrentTask
		LDR	R0, [R0]
		MOV	R1, R0
		ADD	R0, R0, #&4C
		LDR	LK, [R0]						; set up LK to return to task
		ADD	R0, R1, #&10
		LDMIA	R0, {R0-R1}
		ADD	SP, SP, #8

		STMDB	SP!, {R0-R2}
		LDR	R1, [&003ADEB8]			; gCurrentTask
		LDR	R1, [R1]
		MOV	R0, #0						; access = 0

		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2

		LDR	R2, [R1, #&78]				; env = task->fSMemEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
1:
		LDR	R1, [R1, #&7C]				; while ((task = task->fLink) != nil)
		CMP	R1, #0
		BEQ	2f
		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
		BNE	1b
2:
		STMDB	SP!, {R0-R1}
		NOP
		LDR	R1, [&003ADE98]			; g0C008400
		STR	R0, [R1, #&B0]				; g0C0084B0 = access
		LDMIA	SP!, {R0-R1}

		MCR	Internal, R0, Environment
		LDMIA	SP!, {R0-R2}				; restore working registers
		MOVS	PC, LK						; return to task!


;-------------------------------------------------------------------------------
;	SWI jump table.
;-------------------------------------------------------------------------------

pSWI_JT:							; 003AD568
	.long	SWI_JT

SWI_JT:
	.long	DoGetPort				; 0
	.long	DoPortSend
	.long	DoPortReceive
	.long	DoEnterAtomic
	.long	DoExitAtomic
	.long	DoGeneric				; 5
	.long	DoGenerateMessageIRQ
	.long	DoPurgeMMUTLBEntry
	.long	DoFlushMMU
	.long	DoFlushCache
	.long	DoGetCPUVersion		; 10
	.long	DoSemOp
	.long	DoSetDomainRegister
	.long	DoSMemSetBuffer
	.long	DoSMemGetSize
	.long	DoSMemCopyToShared	; 15
	.long	DoSMemCopyFromShared
	.long	DoSMemMsgSetTimerParms
	.long	DoSMemMsgSetMsgAvailPort
	.long	DoSMemMsgGetSenderTaskId
	.long	DoSMemMsgSetUserRefCon	; 20
	.long	DoSMemMsgGetUserRefCon
	.long	DoSMemMsgCheckForDone
	.long	DoSMemMsgMsgDone
	.long	DoTurnOffCache
	.long	DoTurnOnCache			; 25
	.long	DoLowLevelCopyDone
	.long	DoMonitorDispatch
	.long	DoMonitorExit
	.long	DoMonitorThrow
	.long	DoEnterFIQAtomic		; 30
	.long	DoExitFIQAtomic
	.long	DoMonitorFlush
	.long	DoPortResetFilter
	.long	DoScheduler				; 34

;-------------------------------------------------------------------------------
;	Individual SWI handlers.
;-------------------------------------------------------------------------------

;--0----------------------------------------------------------------------------
;	ObjectId GetPortSWI(int inWhat)
DoGetPort:
		LDMIA	SP!, {R0-R1}			; restore arg registers
		STMDB	SP!, {R10-R12, LK}
		BL		GetPortInfo
		LDMIA	SP!, {R10-R12, LK}
		B		Done

;--1----------------------------------------------------------------------------
; PortSendSWI(ObjectId inId, ULong inMsgId, ULong inReplyId, ULong inMsgType, ULong inFlags)
DoPortSend:
		LDR	R1, [&003ADEB8]		; gCurrentTask
		LDR	R1, [R1]
		STMDB	SP!, {R1}				; save gCurrentTask
		LDR	R0, [SP, #&04]			; restore arg registers
		LDR	R1, [SP, #&08]
		STMDB	SP!, {R4, R10-R12, LR}
		BL		PortSendKernelGlue	; might switch current task
		LDMIA	SP!, {R4, R10-R12, LR}
		LDMIA	SP!, {R0}				; R0 = saved (prev) gCurrentTask
		ADD	SP, SP, #&08			; discard stacked arg registers

		LDR	R1, [&003ADEB8]		; R1 = gCurrentTask
		LDR	R1, [R1]
		CMP	R1, #&00000000
		BNE	2f

; gCurrentTask == nil | save prev task state
		MRS	R1, SPSR
		STR	R1, [R0, #&50]		; save PSR in task
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; EQ => undefined mode
		ADD	R1, R0, #&20		; R1 = &fRegister[4]

		STMDB	SP!, {R0}			; save task pointer
		MRS	R0, CPSR				; copy current CPSR
		MSR	CPSR, #&D3			; disable IRQ | FIQ
		NOP
		NOP
		STMNEIA	R1!, {R4-R7}	; save registers in task
		STMNEIA	R1, {R8-LK}^	; save user mode registers
		NOP
		MSR	CPSR, R0				; restore CPSR
		NOP
		NOP
		LDMIA	SP!, {R0}			; restore task pointer
		BNE	1f
										; else PSR => undefined mode
		STMIA	R1!, {R4-R12}		; save R4-R12
		NOP
		MRS	R4, CPSR
		MRS	R5, SPSR
		MSR	CPSR, R5				; use SPSR
		NOP
		NOP
		STMIA	R1, {SP-LK}			; save R13-R14_undef
		NOP
		MSR	CPSR, R4				; restore CPSR
		NOP
		NOP

1:		ADD	R1, R0, #&10
		STR	LK, [R1, #&3C]		; save PC
		B		Done

;	gCurrentTask != nil
2:		LDR	R0, [R0, #&10]		; return fRegister[0]
		B		Done

;--2----------------------------------------------------------------------------
; PortReceiveSWI(ObjectId inId, ObjectId inMsgId, ULong inMsgFilter, ULong inFlags)
DoPortReceive:
		LDR	R1, [&003ADEB8]		; gCurrentTask
		LDR	R1, [R1]
		STMDB	SP!, {R1}				; save gCurrentTask
		LDR	R0, [SP, #&04]			; restore arg registers
		LDR	R1, [SP, #&08]
		STMDB	SP!, {R4, R10-R12, LR}
		BL		PortReceiveKernelGlue	; might switch current task?
		LDMIA	SP!, {R4, R10-R12, LR}
		LDMIA	SP!, {R0}				; R0 = saved (prev) gCurrentTask
		ADD	SP, SP, #&08			; discard stacked arg registers
Received:
		LDR	R1, [&003ADEB8]		; R1 = gCurrentTask
		LDR	R1, [R1]
		CMP	R1, #&00000000
		BNE	2f

; gCurrentTask == nil | save prev task state
		MRS	R1, SPSR
		STR	R1, [R0, #&50]		; save PSR in task
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; EQ => undefined mode
		ADD	R1, R0, #&20		; R1 = &fRegister[4]

		STMDB	SP!, {R0}			; save task pointer
		MRS	R0, CPSR				; copy current CPSR
		MSR	CPSR, #&D3			; disable IRQ | FIQ
		NOP
		NOP
		STMNEIA	R1!, {R4-R7}	; save registers in task
		STMNEIA	R1, {R8-LK}^	; save user mode registers
		NOP
		MSR	CPSR, R0				; restore CPSR
		NOP
		NOP
		LDMIA	SP!, {R0}			; restore task pointer
		BNE	1f
										; else PSR => undefined mode
		STMIA	R1!, {R4-R12}		; save R4-R12
		NOP
		MRS	R4, CPSR
		MRS	R5, SPSR
		MSR	CPSR, R5				; use SPSR
		NOP
		NOP
		STMIA	R1, {SP-LK}			; save R13-R14_undef
		NOP
		MSR	CPSR, R4				; restore CPSR
		NOP
		NOP

1:		ADD	R1, R0, #&10
		STR	LK, [R1, #&3C]		; save PC
		B		Done

;	gCurrentTask != nil
2:		LDR	R0, [R0, #&10]		; return fRegister[0]
		B		Done

;--3----------------------------------------------------------------------------
;
DoEnterAtomic:
		LDMIA	SP!, {R0-R1}
		MSR	CPSR, #&93
		NOP
		NOP
		LDR	R2, [&00394460]	; gAtomicNestCount
		LDR	R0, [R2]
		ADDS	R0, R0, #1
		STR	R0, [R2]
		LDR	R2, [&00394464]	; gOldStatus
		CMP	R0, #1
		MRS	R0, SPSR
		STREQ	R0, [R2]
		MRS	R0, SPSR
		ORR	R0, R0, #&80 (128)
		MSR	CPSR, R0
		NOP
		NOP
		MOV	R0, #0
		B		Done

;--4----------------------------------------------------------------------------
;
DoExitAtomic:
		LDMIA	SP!, {R0-R1}
		LDR	R2, [&00394460]	; gAtomicNestCount
		LDR	R0, [R2]
		SUBS	R0, R0, #1
		STR	R0, [R2]
		BLT	L00393E74
		BNE	L00393E6C
		LDR	R2, [&00394464]	; gOldStatus
		LDR	R0, [R2]
		MRS	R1, SPSR
		AND	R0, R0, #&C0
		BIC	R1, R1, #&C0
		ORR	R1, R1, R0
		MSR	CPSR, R1
		NOP
		NOP
L00393E6C:
		MOV	R0, #0
		B		Done
L00393E74:
		DebugStr "Exit Atomic called too many times!!!"
		MOV	PC, LK

;--5----------------------------------------------------------------------------
;	GenericSWI(int inSelector, ...)
DoGeneric:
		LDR	R1, [&003ADEC0]		; gCurrentTaskSaved
		LDR	R0, [&003ADEB8]		; gCurrentTask
		LDR	R0, [R0]
		STR	R0, [R1]					; gCurrentTaskSaved = gCurrentTask

		LDMIA	SP!, {R0-R1}			; restore arg registers
		STMDB	SP!, {R4-R5}			; restore additional arg registers to stack
		STMDB	SP!, {R10-R12, LK}
		BL		_GenericSWIHandler	; &01A00008	call back in C
		LDMIA	SP!, {R10-R12, LK}
		ADD	SP, SP, #&08			; discard stacked arg registers

		LDR	R1, [&003ADEB8]		; R1 = gCurrentTask
		LDR	R1, [R1]
		CMP	R1, #&00000000
		BNE	2f

;	gCurrentTask == nil | save prev task state
		LDR	R1, [&003ADEC0]		; gCurrentTaskSaved
		LDR	R0, [R1]

		MRS	R1, SPSR
		STR	R1, [R0, #&50]		; save PSR in task
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; EQ => undefined mode
		ADD	R1, R0, #&20		; R1 = &fRegister[4]

		STMDB	SP!, {R0}			; save task pointer
		MRS	R0, CPSR				; copy current CPSR
		MSR	CPSR, #&D3			; disable IRQ | FIQ
		NOP
		NOP
		STMNEIA	R1!, {R4-R7}	; save registers in task
		STMNEIA	R1, {R8-LK}^	; save user mode registers
		NOP
		MSR	CPSR, R0				; restore CPSR
		NOP
		NOP
		LDMIA	SP!, {R0}			; restore task pointer
		BNE	1f
										; else PSR => undefined mode
		STMIA	R1!, {R4-R12}		; save R4-R12
		NOP
		MRS	R4, CPSR
		MRS	R5, SPSR
		MSR	CPSR, R5				; use SPSR
		NOP
		NOP
		STMIA	R1, {SP-LK}			; save R13-R14_undef
		NOP
		MSR	CPSR, R4				; restore CPSR
		NOP
		NOP

1:		ADD	R1, R0, #&10
		STR	LK, [R1, #&3C]		; save PC
		B		Done

;	gCurrentTask != nil
2:		ADD	R1, R1, #&14			; R1-R3 = gCurrentTask->fRegister[1-3]
		LDMIA	R1, {R1-R3}
		B		Done

;--6----------------------------------------------------------------------------
DoGenerateMessageIRQ:
		LDMIA	SP!, {R0-R1}
		B		Done

;--7----------------------------------------------------------------------------
DoPurgeMMUTLBEntry:
		MRC	Internal, R1, CR0
		BIC	R1, R1, #&0000000F
		EOR	R1, R1, #&44000000
		EOR	R1, R1, #&00010000
		EORS	R1, R1, #&0000A100
003ADC9C 0E080F36 ...6 | MCREQ    Internal, R0, CR8
003ADCA0 0E080F15 .... | MCREQ    Internal, R0, CR8
003ADCA4 1E060F16 .... | MCRNE    Internal, R0, Fault Address
		LDMIA	SP!, {R0-R1}
		B		Done

;--8----------------------------------------------------------------------------
DoFlushMMU:
		MRC	Internal, R1, CR0
		BIC	R1, R1, #&0000000F
		EOR	R1, R1, #&44000000
		EOR	R1, R1, #&00010000
		EORS	R1, R1, #&0000A100
		MCREQ Internal, R0, CR8					; StrongARM uses CR8
		MCRNE Internal, R0, Fault Status
		LDMIA	SP!, {R0-R1}
		B		Done

;--9----------------------------------------------------------------------------
DoFlushCache:
		MRC	Internal, R0, CR0
		BIC	R0, R0, #&0000000F
		EOR	R0, R0, #&44000000
		EOR	R0, R0, #&00010000
		EORS	R0, R0, #&0000A100
		BNE	2f
		MOV	R0,     #&04000000
		ADD	R0, R0, #&00004000
1:
		LDR	R1, [R0], #-&20
		TEQ	R0, #&04000000
		BNE	1b

2:		LDMIA	SP!, {R0-R1}
		MCR	Internal, R0, CR7
		NOP
		B		Done

;-10----------------------------------------------------------------------------
DoGetCPUVersion:
		LDMIA	SP!, {R0-R1}
		MCR	Internal, R0, CR0
		B		Done

;-11----------------------------------------------------------------------------
DoSemOp:
		LDMIA	SP, {R0-R1}
		LDR	R3, [&003ADEB8]		; gCurrentTask
		LDR	R3, [R3]
		STMDB	SP!, {R2-R3}
		STMDB	SP!, {R10-R12, LR}
		BL		DoSemaphoreOp
		LDMIA	SP!, {R10-R12, LR}
		LDMIA	SP!, {R2-R3}

		LDR	R1, [&003ADEB8]		; gCurrentTask
		LDR	R1, [R1]
		CMP	R1, #&00000000
		ADDNE	SP, SP, #&08
		BNE	Done

		SUB	LK, LK, #4
		MOV	R0, R3
		MRS	R1, SPSR
		STR	R1, [R0, #&50]		; save PSR in task
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; undefined mode?
		ADD	R1, R0, #&18
		STMDB	SP!, {R0}			; save task pointer
		MRS	R0, CPSR				; copy current CPSR
		MSR	CPSR, #&D3			; disable IRQ | FIQ
		NOP
		NOP
		STMNEIA	R1!, {R2-R7}	; save registers in task
		STMNEIA	R1, {R8-LK}^	; save user mode registers
		NOP
		MSR	CPSR, R0				; restore CPSR
		NOP
		NOP
		LDMIA	SP!, {R0}			; restore task pointer
		BNE	1f
										; else PSR => undefined mode
		STMIA	R1!, {R2-R12}		; save R4-R12
		NOP
		MRS	R4, CPSR
		MRS	R5, SPSR
		MSR	CPSR, R5				; use SPSR
		NOP
		NOP
		STMIA	R1, {SP-LK}			; save R13-R14_undef
		NOP
		MSR	CPSR, R4				; restore CPSR
		NOP
		NOP

1:		ADD	R1, R0, #&10
		STR	LK, [R1, #&3C]		; save PC
		LDMIA	SP!, {R2-R3}
		STMIA	R1, {R2-R3}			; set up R0,R1
		B		Done

;-12----------------------------------------------------------------------------
DoSetDomainRegister:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R0-R1}
		NOP
		LDR	R1, [&003ADE98]	; 0C008400
		STR	R0, [R1, #&B0]		;g0C0084B0 = access
		LDMIA	SP!, {R0-R1}
		MCR	Internal, R0, Environment
		B		Done

;-13----------------------------------------------------------------------------
DoSMemSetBuffer:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		SMemSetBufferKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		B		Done

;-14----------------------------------------------------------------------------
DoSMemGetSize:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		SMemGetSizeKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		LDR	R1, [&00393954]		; gCurrentTask
		LDR	R1, [R1]
		CMP	R1, #&00000000
		BEQ	Done

		ADD	R1, R1, #&14
		LDMIA	R1, {R1-R3}
		B		Done

;-15----------------------------------------------------------------------------
DoSMemCopyToShared:
		LDR	R0, [&00393954]		; gCurrentTask
		LDR	R0, [R0]
		ADD	R0, R0, #&4C
		STR	LR, [R0]
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R4-R5, R10-R12, LR}
		BL		SMemCopyToKernelGlue
		LDMIA	SP!, {R4-R5, R10-R12, LR}
		MOV	R3, R0

		LDR	R0, [&00393954]		; gCurrentTask
		LDR	R0, [R0]
		ADD	R0, R0, #&10
		LDMIA	R0, {R0-R2}
		CMP	R3, #1
		BEQ	1f
		CMP	R3, #4
		BEQ	4f
		MOV	R1, R2
		MOV	R0, R3
		B		Done
1:
		MOV	LR, LowLevelCopyEngine
		B		Done
4:
		MOV	LR, LowLevelCopyEngineLong
		B		Done

;-16----------------------------------------------------------------------------
DoSMemCopyFromShared:
		LDR	R0, [&00393954]		; gCurrentTask
		LDR	R0, [R0]
		ADD	R0, R0, #&4C
		STR	LR, [R0]
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R4-R5, R10-R12, LR}
		BL		SMemCopyFromKernelGlue
		LDMIA	SP!, {R4-R5, R10-R12, LR}
		MOV	R3, R0

		LDR	R0, [&00393954]		; gCurrentTask
		LDR	R0, [R0]
		ADD	R0, R0, #&10
		LDMIA	R0, {R0-R2}
		CMP	R3, #1
		BEQ	1f
		CMP	R3, #4
		BEQ	4f
		MOV	R1, R2
		MOV	R0, R3
		B		Done
1:
		MOV	LR, LowLevelCopyEngine
		B		Done
4:
		MOV	LR, LowLevelCopyEngineLong
		B		Done

;-17----------------------------------------------------------------------------
DoSMemMsgSetTimerParms:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		SMemMsgSetTimerParmsKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		B		Done

;-18----------------------------------------------------------------------------
DoSMemMsgSetMsgAvailPort:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		SMemMsgSetMsgAvailPortKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		B		Done

;-19----------------------------------------------------------------------------
DoSMemMsgGetSenderTaskId:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		SMemMsgGetSenderTaskIdKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		LDR	R1, [&00393954]		; gCurrentTask
		LDR	R1, [R1]
		CMP	R1, #&00000000
		BEQ	Done

		ADD	R1, R1, #&14
		LDMIA	R1, {R1-R3}
		B		Done

;-20----------------------------------------------------------------------------
DoSMemMsgSetUserRefCon:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		SMemMsgSetUserRefConKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		B		Done

;-21----------------------------------------------------------------------------
DoSMemMsgGetUserRefCon:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		SMemMsgGetUserRefConKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		LDR	R1, [&00393954]		; gCurrentTask
		LDR	R1, [R1]
		CMP	R1, #&00000000
		BEQ	Done

		ADD	R1, R1, #&14			; return fRegister[1..3]
		LDMIA	R1, {R1-R3}
		B		Done

;-22----------------------------------------------------------------------------
DoSMemMsgCheckForDone:
		LDR	R1, [&00393954]		; gCurrentTask
		LDR	R1, [R1]
		STMDB	SP!, {R1}
		LDR	R0, [SP, #&04]
		LDR	R1, [SP, #&08]
		STMDB	SP!, {R10-R12, LR}
		BL		SMemMsgCheckForDoneKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		LDMIA	SP!, {R0}
		ADD	SP, SP, #&08
		B		Received

;-23----------------------------------------------------------------------------
DoSMemMsgMsgDone:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		SMemMsgMsgDoneKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		B		Done

;-24----------------------------------------------------------------------------
DoTurnOffCache:
		LDMIA	SP!, {R0-R1}
		LDR	R0, [&003ADEC4]		; gInitialCPUMode = &000011B5 [RO]
		LDR	R0, [R0]
		SUB	R0, R0, #&00000004
		SUB	R0, R0, #&00001000
		MCR	Internal, R0, Configuration
		B		Done

;-25----------------------------------------------------------------------------
DoTurnOnCache:
		LDMIA	SP!, {R0-R1}
		LDR	R0, [&003ADEC4]		; gInitialCPUMode = &000011B5 [RO]
		LDR	R0, [R0]
		MCR	Internal, R0, Configuration
		B		Done

;-26----------------------------------------------------------------------------
DoLowLevelCopyDone:
		LDMIA    SP!, {R0-R1}
		MOV      R0, #0
		LDR      R1, [&00393954]		; gCurrentTask
		LDR      R1, [R1]
		MOV      R2, LR
		STMDB    SP!, {R10-R12, LR}
		BL       LowLevelCopyDoneFromKernelGlue
		LDMIA    SP!, {R10-R12, LR}
		B        Done

;-27----------------------------------------------------------------------------
DoMonitorDispatch:
		LDR      R0, [&00393954]		; gCurrentTask
		LDR      R0, [R0]

		MRS	R1, SPSR
		STR	R1, [R0, #&50]		; save PSR in task
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; EQ => undefined mode
		ADD	R1, R0, #&18		; R1 = &fRegister[2]

		STMDB	SP!, {R0}			; save task pointer
		MRS	R0, CPSR				; copy current CPSR
		MSR	CPSR, #&D3			; disable IRQ | FIQ
		NOP
		NOP
		STMNEIA	R1!, {R2-R7}	; save registers in task
		STMNEIA	R1, {R8-LK}^	; save user mode registers
		NOP
		MSR	CPSR, R0				; restore CPSR
		NOP
		NOP
		LDMIA	SP!, {R0}			; restore task pointer
		BNE	1f
										; else PSR => undefined mode
		STMIA	R1!, {R2-R12}		; save R2-R12
		NOP
		MRS	R4, CPSR
		MRS	R5, SPSR
		MSR	CPSR, R5				; use SPSR
		NOP
		NOP
		STMIA	R1, {SP-LK}			; save R13-R14_undef
		NOP
		MSR	CPSR, R4				; restore CPSR
		NOP
		NOP
1:
		ADD	R1, R0, #&10
		STR	LK, [R1, #&3C]		; save PC
		LDMIA	SP!, {R2-R3}
		STMIA	R1, {R2-R3}			; save stacked R0-R1

		STMDB    SP!, {R10-R12, LK}
		BL       MonitorDispatchKernelGlue
		LDMIA    SP!, {R10-R12, LK}
		B        Done

;-28----------------------------------------------------------------------------
DoMonitorExit:
		LDMIA    SP!, {R0-R1}
		STMDB    SP!, {R10-R12, LR}
		BL       MonitorExitKernelGlue
		LDMIA    SP!, {R10-R12, LR}
		B        Done

;-29----------------------------------------------------------------------------
DoMonitorThrow:
		LDMIA    SP!, {R0-R1}
		STMDB    SP!, {R10-R12, LR}
		BL       MonitorThrowKernelGlue
		LDMIA    SP!, {R10-R12, LR}
		B        Done

;-30----------------------------------------------------------------------------
DoEnterFIQAtomic:
		LDMIA    SP!, {R0-R1}
		MSR      CPSR, #&D3
		NOP
		NOP
		LDR      R2, [&00394468]	; gAtomicFIQNestCount
		LDR      R0, [R2]
		ADDS     R0, R0, #1
		STR      R0, [R2]
		CMP      R0, #1
		MOVNE    R0, #0
		BNE      Done

		LDR      R2, [&0039446C]	; gOldFIQStatus
		MRS      R0, CPSR
		STR      R0, [R2]
		MRS      R0, CPSR
		ORR      R0, R0, #&C0
		MSR      CPSR, R0
		NOP
		NOP
		MOV      R0, #0
		B        Done

;-31----------------------------------------------------------------------------
DoExitFIQAtomic:
		LDMIA    SP!, {R0-R1}
		LDR      R2, [&00394468]	; gAtomicFIQNestCount
		LDR      R0, [R2]
		SUBS     R0, R0, #1
		STR      R0, [R2]
		BLT      2f
		BNE      1f
		LDR      R2, [&0039446C]	; gOldFIQStatus
		LDR      R0, [R2]
		MRS      R1, SPSR
		AND      R0, R0, #&C0
		BIC      R1, R1, #&C0
		ORR      R1, R1, R0
		MSR      CPSR, R1
		NOP
		NOP
1:
		MOV      R0, #0
		B        Done
2:
		DebugStr	"Exit FIQ Atomic called too many times!!!"
		MOV      PC, LK


;-32----------------------------------------------------------------------------
DoMonitorFlush:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		MonitorFlushKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		B		Done

;-33----------------------------------------------------------------------------
DoPortResetFilter:
		LDMIA	SP!, {R0-R1}
		STMDB	SP!, {R10-R12, LR}
		BL		PortResetFilterKernelGlue
		LDMIA	SP!, {R10-R12, LR}
		B		Done

;-34----------------------------------------------------------------------------
DoScheduler:
		LDMIA	SP!, {R0-R1}
		MOV	R0, #0
		B		Done

;-35..--------------------------------------------------------------------------
UndefinedSWI:
		DebugStr "Undefined SWI"
		LDMIA	SP!, {R0-R1}		; restore working registers
		MOV	R0, #-1				; return non-specific error
		B		Done

InvalidInstr:
		LDMIA	SP!, {R0-R1}			; restore registers
		SUB	LK, LK, #04				; re-execute instr
		MOVS	PC, LK

L003ADD7C:
		MRS	R0, SPSR
		ADD	PC, PC+8, R1 LSR 24
		NOP
;		...

