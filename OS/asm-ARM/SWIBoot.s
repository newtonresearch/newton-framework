;	File:		SWIBoot.s
;
;	Contains:	SWI handler ARM assembler glue.
;
;	Written by:	Newton Research Group

		.text
;	Exports
		.globl	_SWIBoot

		.align	2


;	SWI jump table
SWI_JT:							; 003AD568
003AD568 003AD56C

003AD56C 003ADD3C
003AD570 003ADFAC
003AD574 003AE070
003AD578 00393D8C
003AD57C 00393E2C
003AD580 003ADBB4
003AD584 003ADEDC
003AD588 003ADC88
003AD58C 003ADCB0
003AD590 003ADCD4
003AD594 003ADD10
003AD598 003ADEE4
003AD59C 003ADD1C
003AD5A0 00394050
003AD5A4 00394064
003AD5A8 003940C0
003AD5AC 00394120
003AD5B0 003941A4
003AD5B4 003941B8
003AD5B8 003941CC
003AD5BC 003941F8
003AD5C0 0039420C
003AD5C4 00394238
003AD5C8 00394264
003AD5CC 003ADE7C
003AD5D0 003ADEC8
003AD5D4 00394180
003AD5D8 00394278
003AD5DC 00394370
003AD5E0 00394384
003AD5E4 00393F10
003AD5E8 00393FD4
003AD5EC 00394398
003AD5F0 003AE138
003AD5F4 003AE14C


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

;		MOV      R0, R1
;		LDR      R1, [&003ADE98]
;		STR      R0, [R1, #&000000B0]
;		LDMIA    SP!, {R0-R1}
;		???      ???

		LDR      R1, [&003ADE9C]	; if gAtomicFIQNestCountFast == 0
		LDR      R1, [R1]
		CMP      R1, #0
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
		CMP	R1, #35					; 0..35
		BGE	UndefinedSWI
		LDR	R0, [SWI_JT]
		LDR	PC, [R0, R1 LSL 2]	; jump to handler
;	Actual R0-R1 are on the stack.
;	SWI handler jumps back here when done.
SWIDone:									; 003AD750

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
		LDR	R1, [&003ADEAC]
		LDR	R1, [R1]
		CMP	R1, #0
		BNE	L003AD7C8
		LDR	R1, [&003ADEB0]
		LDR	R1, [R1]
		CMP	R1, #0
		BEQ	NoSwap2

		STMDB	SP!, {R2-R3, R10-R12, LK}
		B		L003AD7F8

003AD7C8
		STMDB	SP!, {R2-R3, R10-R12, LK}

		if (gWantDeferred)
		{
//			MSR	CPSR, #&13	; enable IRQ | FIQ
			DoDeferrals();
//			MSR	CPSR, #&93	; disable IRQ again
		}

003AD7CC E321F013 .!.. | MSR	CPSR, #&13	; enable IRQ | FIQ
003AD7D0 E1A00000 .... | NOP
003AD7D4 E1A00000 .... | NOP
003AD7D8 EB5D28EA .](. | BL	_DoDeferrals
003AD7DC E321F093 .!.. | MSR	CPSR, #&93	; disable IRQ again
003AD7E0 E1A00000 .... | NOP
003AD7E4 E1A00000 .... | NOP

		LDR	R1, [&003ADEB0]		; gDoSchedule
		LDR	R1, [R1]
		CMP	R1, #0
		BEQ	NoSwap

;	gDoSchedule == YES
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

		LDR	R2, [&003ADEB8]
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
		CMP	R1, #&1B				; undefined mode?
		ADD	R1, R0, #&18		; R1 -> fRegister[2]
	;	PSR => defined mode
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
	;	PSR => undefined mode
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
		CMP	R1, #&1B				; undefined mode?
		ADD	R1, R0, #&18		; R1 -> fRegister[2]
	;	PSR => defined mode
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
	;	PSR => undefined mode
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
		CMP	R1, #&1B
	;	PSR => defined mode
		LDMNEIA	R0, {R0-LK}^	; restore all user mode registers
		NOP
		BNE	L003AD9FC
	;	PSR => undefined mode
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

L003ADA2C:
		LDR	R1, [R1, #&7C]				; while ((task = task->fLink) != nil)
		CMP	R1, #0
		BEQ	L003ADA50
		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
		BNE	L003ADA2C

L003ADA50:
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
		BNE	&003ADB14

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

L003ADAD0:
		LDR	R1, [R1, #&7C]				; while ((task = task->fLink) != nil)
		CMP	R1, #0
		BEQ	L003ADAF4
		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
		BNE	L003ADAD0

L003ADAF4:
		STMDB	SP!, {R0-R1}
		NOP
		LDR	R1, [g0C008400]
		STR	R0, [R1, #&B0]				; g0C0084B0 = access
		LDMIA	SP!, {R0-R1}

		MCR	Internal, R0, Environment
		LDMIA	SP!, {R0-R2}				; restore working registers
		MOVS	PC, LK						; return to task!

;	gCopyDone == YES
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

L003ADB70:
		LDR	R1, [R1, #&7C]				; while ((task = task->fLink) != nil)
		CMP	R1, #0
		BEQ	L003ADB94
		LDR	R2, [R1, #&74]				; env = task->fEnvironment
		CMP	R2, #0						; if (env != nil)
		LDRNE	R2, [R2, #&10]				;	access |= env->fDomainAccess
		ORRNE	R0, R0, R2
		BNE	L003ADB70

L003ADB94:
		STMDB	SP!, {R0-R1}
		NOP
		LDR	R1, [&003ADE98]			; g0C008400
		STR	R0, [R1, #&B0]				; g0C0084B0 = access
		LDMIA	SP!, {R0-R1}

		MCR	Internal, R0, Environment
		LDMIA	SP!, {R0-R2}				; restore working registers
		MOVS	PC, LK						; return to task!

;	••••••••

;	case kGenericSWI:
L003ADBB4:
		LDR	R1, [&003ADEC0]		; gCurrentTaskSaved
		LDR	R0, [&003ADEB8]		; gCurrentTask
		LDR	R0, [R0]
		STR	R0, [R1]					; gCurrentTaskSaved = gCurrentTask

		LDMIA	SP!, {R0-R1}			; restore arg registers
		STMDB	SP!, {R4-R5}			; restore additional arg registers to stack
		STMDB	SP!, {R10-R12, LK}
		BL		_GenericSWIHandler		; &01A00008	call back in C
		LDMIA	SP!, {R10-R12, LK}

		LDR	R1, [&003ADEB8]		; gCurrentTask
		LDR	R1, [R1]
		CMP	R1, #&00000000			; if gCurrentTask != nil
		BEQ	&003ADBF8

;	gCurrentTask != nil
		ADD	R1, R1, #&14			; R1-R3 = gCurrentTask->fRegister[1-3]
		LDMIA	R1, {R1-R3}
		ADD	SP, SP, #&08
		B		SWIDone

;	gCurrentTask == nil
003ADBF8:
		LDR	R1, [&003ADEC0]		; gCurrentTaskSaved
		LDR	R0, [R1]
		MRS	R1, SPSR
		STR	R1, [R0, #&50]		; save PSR in task
		AND	R1, R1, #&1F
		CMP	R1, #&1B				; undefined mode?
		ADD	R1, R0, #&20
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
		BNE	L44
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

L44:	ADD	R1, R0, #&10
		STR	LK, [R1, #&3C]		; save PC
		ADD	SP, SP, #&08
		B		SWIDone

;	case kFlushMMUSWI:	7
003ADC88:
/*
		MRC	Internal, R1, CR0
		BIC	R1, R1, #&0000000F
		EOR	R1, R1, #&44000000
		EOR	R1, R1, #&00010000
		EORS	R1, R1, #&0000A100
		MCREQ Internal, R0, CR8
		MCRNE Internal, R0, Fault Status
*/
003ADC88 EE101F10 .... | ???      ???
003ADC8C E3C1100F .... | BIC      R1, R1, #&0000000F (15)
003ADC90 E2211311 .!.. | EOR      R1, R1, #&44000000 (1140850688)
003ADC94 E2211801 .!.. | EOR      R1, R1, #&00010000 (65536)
003ADC98 E2311CA1 .1.. | EORS     R1, R1, #&0000A100 (41216)
003ADC9C 0E080F36 ...6 | ???EQ    ???
003ADCA0 0E080F15 .... | ???EQ    ???
003ADCA4 1E060F16 .... | ???NE    ???
003ADCA8 E8BD0003 .... | LDMIA    SP!, {R0-R1}
003ADCAC EAFFFEA7 .... | B        SWIDone

; case 8
003ADCB0 EE101F10 .... | ???      ???
003ADCB4 E3C1100F .... | BIC      R1, R1, #&0000000F (15)
003ADCB8 E2211311 .!.. | EOR      R1, R1, #&44000000 (1140850688)
003ADCBC E2211801 .!.. | EOR      R1, R1, #&00010000 (65536)
003ADCC0 E2311CA1 .1.. | EORS     R1, R1, #&0000A100 (41216)
003ADCC4 0E080F17 .... | ???EQ    ???
003ADCC8 1E050F15 .... | ???NE    ???
003ADCCC E8BD0003 .... | LDMIA    SP!, {R0-R1}
003ADCD0 EAFFFE9E .... | B        SWIDone

;	case 9
003ADCD4 EE100F10 .... | ???      ???
003ADCD8 E3C0000F .... | BIC      R0, R0, #&0000000F (15)
003ADCDC E2200311 .... | EOR      R0, R0, #&44000000 (1140850688)
003ADCE0 E2200801 .... | EOR      R0, R0, #&00010000 (65536)
003ADCE4 E2300CA1 .0.. | EORS     R0, R0, #&0000A100 (41216)
003ADCE8 1A000004 .... | BNE      &003ADD00
003ADCEC E3A00301 .... | MOV      R0, #&04000000 (67108864)
003ADCF0 E2800901 .... | ADD      R0, R0, #&00004000 (16384)
003ADCF4 E4101020 .... | LDR      R1, [R0], #-&00000020
003ADCF8 E3300301 .0.. | TEQ      R0, R0, #&04000000 (67108864)
003ADCFC 1AFFFFFC .... | BNE      &003ADCF4
003ADD00 E8BD0003 .... | LDMIA    SP!, {R0-R1}
003ADD04 EE070F17 .... | ???      ???
003ADD08 E1A00000 .... | NOP      
003ADD0C EAFFFE8F .... | B        SWIDone

;	case 10
003ADD10 E8BD0003 .... | LDMIA    SP!, {R0-R1}
003ADD14 EE100F10 .... | ???      ???
003ADD18 EAFFFE8C .... | B        SWIDone

;	case 12
003ADD1C E8BD0003 .... | LDMIA    SP!, {R0-R1}
003ADD20 E92D0003 .-.. | STMDB    SP!, {R0-R1}
003ADD24 E1A00000 .... | NOP      
003ADD28 E59F1168 ...h | LDR      R1, [&003ADE98]
003ADD2C E58100B0 .... | STR      R0, [R1, #&000000B0]
003ADD30 E8BD0003 .... | LDMIA    SP!, {R0-R1}
003ADD34 EE030F13 .... | ???      ???
003ADD38 EAFFFE84 .... | B        SWIDone

;	case kGetPortSWI:	// 0	GetPortSWI(int inWhat)
L003ADD3C:
		LDMIA	SP!, {R0-R1}			; restore arg registers
		STMDB	SP!, {R10-R12, LK}
		BL		GetPortInfo				; &01AFAD14
		LDMIA	SP!, {R10-R12, LK}
		B		SWIDone

UndefinedSWI:							; 003ADD50
		DebugStr "Undefined SWI"
		LDMIA    SP!, {R0-R1}		; restore working registers
		MOV      R0, #-1				; return non-specific error
		B        SWIDone

InvalidInstr:							; 003ADD7C
		LDMIA	SP!, {R0-R1}			; restore registers
		SUB	LK, LK, #04				; re-execute instr
		MOVS	PC, LK

L003ADD7C:
		MRS      R0, SPSR
		ADD      PC, PC, R1 LSR 24

