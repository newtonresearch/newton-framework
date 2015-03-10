#if defined(__i386__)
#include "../asm-i386/SWIHandler.s"

#else
;	File:		SWIHandler.s
;
;	Contains:	SWI handler PPC assembler glue.
;					See http://developer.apple.com/documentation/DeveloperTools/Conceptual/LowLevelABI/index.html
;					 or http://the.wall.riscom.net/books/proc/ppc/cwg/a_abi.html
;					for the ABI.
;
;	Written by:	Newton Research Group

;	register offsets in CTask
		.set	taskr0,  0x10
		.set	taskr1,  0x14
		.set	taskr2,  0x18
		.set	taskr3,  0x1C
		.set	taskr4,  0x20
		.set	taskr5,  0x24
		.set	taskr6,  0x28
		.set	taskr7,  0x2C
		.set	taskr11, 0x3C
		.set	taskr12, 0x40
		.set	taskr13, 0x44
		.set	taskr14, 0x48
		.set	taskr28, 0x80
		.set	taskr29, 0x84
		.set	taskr30, 0x88
		.set	taskr31, 0x8C
		.set	tasklk,  0x90
		.set	taskpc,  0x94

;	offsets in our stack frame
		.set	frlk,   60
		.set	frr28,  64
		.set	frr29,  68
		.set	frr30,  72
		.set	frr31,  76
		.set	frSize, 80

;-------------------------------------------------------------------------------
		.data
		.align	2
		.globl	_gCPUmode

_gCPUmode:
		.long		1


;-------------------------------------------------------------------------------
		.text
		.align	2
		.globl	_InitRegisters

;	Initialize non-volatile registers for task.
;	Args:		r3				address of fRegister[11]
;	Return:	r3				unchanged

_InitRegisters:
		stmw	r11, 0(r3)
		blr

;-------------------------------------------------------------------------------
		.text
		.align	2
		.globl	SWI

;	Handle the SWI.
;	Args:		r0				selector
;				r3..			args as passed from C, but all in registers
;	Return:	r3..			result(s)

SWI:
; prolog
		stmw	r28, -16(r1)		; save working registers
		mflr	r30
		stw	r30, 8(r1)			; save LK in super frame
		stwu	r1, -frSize(r1)	; create activation record

		lis	r31, ha16(_gCPUmode)	; enter interrupt mode
		li		r30, 1
		stw	r30, lo16(_gCPUmode)(r31)

; g0C0084B0 = access

		cmpwi	cr7, r0, 35				; switch on SWI selector
		bge-	cr7, UndefinedSWI		; 0..34
		lis	r30, ha16(pSWI_JT)
		lwz	r30, lo16(pSWI_JT)(r30)
		slwi	r31, r0, 2
		lwzx	r30, r31, r30
		mtctr	r30
		bctr								; jump to handler

;	SWI handler jumps back here when done.
Done:
;	SWI work is done.
;	If scheduled, switch task now.

		mflr	r31
		stw	r31, frlk(r1)			; save LK

; disable IRQ | FIQ

; if in nested interrupt, don’t swap task
		lis	r30, ha16(_gAtomicFIQNestCountFast)
		lwz	r31, lo16(_gAtomicFIQNestCountFast)(r30)
		cmpwi	cr7, r31, 0
		bne-	cr7, NoSwap
		lis	r30, ha16(_gAtomicIRQNestCountFast)
		lwz	r31, lo16(_gAtomicIRQNestCountFast)(r30)
		cmpwi	cr7, r31, 0
		bne-	cr7, NoSwap
		lis	r30, ha16(_gAtomicNestCount)
		lwz	r31, lo16(_gAtomicNestCount)(r30)
		cmpwi	cr7, r31, 0
		bne-	cr7, NoSwap
		lis	r30, ha16(_gAtomicFIQNestCount)
		lwz	r31, lo16(_gAtomicFIQNestCount)(r30)
		cmpwi	cr7, r31, 0
		bne-	cr7, NoSwap

; re-enable FIQ (but not IRQ)

		lis	r30, ha16(_gWantDeferred)
		lwz	r31, lo16(_gWantDeferred)(r30)
		cmpwi	cr7, r31, 0
		beq+	cr7, 1f
; enable IRQ | FIQ
		bl		_DoDeferrals
; disable IRQ again
1:
		lis	r31, ha16(_gDoSchedule)
		lwz	r31, lo16(_gDoSchedule)(r31)
		cmpwi	cr7, r31, 0
		beq+	cr7, NoSwap

;	gDoSchedule == YES
		bl		_Scheduler
		mr		r31, r3					; r31 = next task to run
		lis	r30, ha16(_gWantSchedulerToRun)
		lwz	r30, lo16(_gWantSchedulerToRun)(r30)
		cmpwi	cr7, r30, 0
		beq+	cr7, 2f					; gWantSchedulerToRun?
		bl		_StartScheduler		; YES, start scheduler
2:
		addis	r3, 0, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r3)	; r30 = current task
		cmpw	cr7, r31, r30			; if task hasn’t changed, don’t do task swap
		beq+	cr7, NoSwap

		stw	r31, lo16(_gCurrentTask)(r3)	; update gCurrentTask

		cmpwi	cr7, r30, 0
		beq-	cr7, SwapIn			; if no prev task, don’t swap it out

; r30 = prev task
; r31 = new scheduled task
SwapOut:
		lwz	r31, frlk(r1)			; restore LK
		mtlr	r31

		lis	r31, ha16(_gCopyDone)
		lwz	r31, lo16(_gCopyDone)(r31)	; gCopyDone?
		cmpwi	cr7, r31, 0
		bne+	cr7, 1f

;	gCopyDone == NO
										; save & disable IRQ | FIQ
		stmw	r0, taskr0(r30)	; save all registers
										; restore IRQ | FIQ
		lwz	r31, frr30(r1)		; overwrite correct r30-r31
		stw	r31, taskr30(r30)
		lwz	r31, frr31(r1)
		stw	r31, taskr31(r30)

		mflr	r31
		stw	r31, tasklk(r30)
		stw	r31, taskpc(r30)		; save task’s PC when calling here, ie its LK
		b		SwapIn

;	gCopyDone == YES
1:
		lwz	r31, taskpc(r30)
		mtlr	r31					; restore saved PC in LK for return

										; save & disable IRQ | FIQ
		stmw	r0, taskr0(r30)	; save all registers
										; restore IRQ | FIQ
		lwz	r31, frr30(r1)		; overwrite correct r30-r31
		stw	r31, taskr30(r30)
		lwz	r31, frr31(r1)
		stw	r31, taskr31(r30)

		lwz	r31, 88(r1)				; fetch saved LK
		stw	r31, taskpc(r30)		; save as PC

		lis	r30, ha16(_gCopyDone)
		li		r31, 0
		stw	r31, lo16(_gCopyDone)(r30)	; gCopyDone = NO

; previous task state is saved
; now set up next task state

SwapIn:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)	; r30 = current task
		mr		r3, r30
		bl		_SwapInGlobals		; SwapInGlobals(gCurrentTask)

		lis	r31, ha16(_gCPUmode)	; exit interrupt mode
		li		r3, 0
		stw	r3, lo16(_gCPUmode)(r31)

;	restore processor registers from new (current) task
		lwz	r31, taskpc(r30)
		mtctr	r31					; restore saved PC ready for return
										; set saved PSR
		lwz	r31, tasklk(r30)
		mtlr	r31
		mr		r3, r30
		lmw	r4, taskr4(r3)		; restore all user mode registers
		lwz	r1, taskr1(r3)		; set up stack activation record
		lwz	r3, taskr3(r3)

; set up new environment

		bctr							; switch task!

;	gDoSchedule == NO
;	no task switch required
;	set up domain access anyways

NoSwap:
		lis	r30, ha16(_gWantSchedulerToRun)
		lwz	r31, lo16(_gWantSchedulerToRun)(r30)
		cmpwi	cr7, r31, 0
		beq+	cr7, 1f					; gWantSchedulerToRun?
		bl		_StartScheduler		; YES, start scheduler
1:
		lis	r30, ha16(_gCopyDone)
		lwz	r31, lo16(_gCopyDone)(r30)
		cmpwi	cr7, r31, 0
		bne+	cr7, CopyIsDone

;	gCopyDone == NO
; set up new environment
		lwz	r31, frlk(r1)			; restore LK
		mtlr	r31
		b		ExitSWI					; return to task - or to copy task

;	gCopyDone == YES
CopyIsDone:
		lis	r30, ha16(_gCopyDone)
		li		r31, 0					; gCopyDone = NO
		stw	r31, lo16(_gCopyDone)(r30)

		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		lwz	r31, taskpc(r30)
		mtlr	r31						; set up LK to return to task

; set up new environment

ExitSWI:
		lis	r31, ha16(_gCPUmode)	; exit interrupt mode
		li		r30, 0
		stw	r30, lo16(_gCPUmode)(r31)

		lwz	r1, 0(r1)		; epilog
		lmw	r28, -16(r1)
		blr						; return to task!


;-------------------------------------------------------------------------------
;	SWI jump table.
;-------------------------------------------------------------------------------
;		.const
		.data

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
		.text

;--0----------------------------------------------------------------------------
;	ObjectId GetPortSWI(int inWhat)
DoGetPort:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_GetPortInfo
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;--1----------------------------------------------------------------------------
; PortSendSWI(ObjectId inId, ULong inMsgId, ULong inReplyId, ULong inMsgType, ULong inFlags)
DoPortSend:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)		; save gCurrentTask

		mflr	r31
		stw	r31, frlk(r1)
		bl		_PortSendKernelGlue	; might switch current task
		lwz	r31, frlk(r1)
		mtlr	r31

; r30|R0 = saved (prev) gCurrentTask
; r31|R1 = gCurrentTask
		lis	r31, ha16(_gCurrentTask)
		lwz	r31, lo16(_gCurrentTask)(r31)
		cmpwi	cr7, r31, 0
		bne-	cr7, 1f

; gCurrentTask == nil | save prev task state
										; save & disable IRQ | FIQ
;		stw	spsr, 0x94(r30)	; save PSR in task
		stmw	r0, taskr0(r30)	; save all registers
										; restore IRQ | FIQ
		lwz	r31, frr30(r1)		; overwrite correct r30-r31
		stw	r31, taskr30(r30)
		lwz	r31, frr31(r1)
		stw	r31, taskr31(r30)

		mflr	r31
		stw	r31, taskpc(r30)		; save PC
		b		Done

;	gCurrentTask != nil
1:		lwz	r3, taskr3(r30)		; return fRegister[kResultReg]
		b		Done

;--2----------------------------------------------------------------------------
; PortReceiveSWI(ObjectId inId, ObjectId inMsgId, ULong inMsgFilter, ULong inFlags)
DoPortReceive:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)		; save gCurrentTask

		mflr	r31
		stw	r31, frlk(r1)
		bl		_PortReceiveKernelGlue	; might switch current task
		lwz	r31, frlk(r1)
		mtlr	r31

Received:
; r30|R0 = saved (prev) gCurrentTask
; r31|R1 = gCurrentTask
		lis	r31, ha16(_gCurrentTask)
		lwz	r31, lo16(_gCurrentTask)(r31)
		cmpwi	cr7, r31, 0
		bne-	cr7, 1f

; gCurrentTask == nil | save prev task state
										; save & disable IRQ | FIQ
;		stw	spsr, 0x94(r30)	; save PSR in task
		stmw	r0, taskr0(r30)	; save all registers
										; restore IRQ | FIQ
		lwz	r31, frr30(r1)		; overwrite correct r30-r31
		stw	r31, taskr30(r30)
		lwz	r31, frr31(r1)
		stw	r31, taskr31(r30)

		mflr	r31
		stw	r31, taskpc(r30)		; save PC
		b		Done

;	gCurrentTask != nil
1:		lwz	r3, taskr3(r30)		; return fRegister[kResultReg]
		b		Done

;--3----------------------------------------------------------------------------
;
DoEnterAtomic:
; disable FIQ|IRQ
		lis	r30, ha16(_gAtomicNestCount)
		lwz	r31, lo16(_gAtomicNestCount)(r30)
		addi	r31, r31, 1
		stw	r31, lo16(_gAtomicNestCount)(r30)
		cmpwi	cr7, r31, 1
		bne-	cr7, 1f
; save SPSR in gOldStatus
		nop
1:
; re-enable FIQ
		li		r3, 0
		b		Done

;--4----------------------------------------------------------------------------
;
DoExitAtomic:
		lis	r30, ha16(_gAtomicNestCount)
		lwz	r31, lo16(_gAtomicNestCount)(r30)
		subi	r31, r31, 1
		stw	r31, lo16(_gAtomicNestCount)(r30)
		cmpwi	cr7, r31, 0
		blt-	cr7, 2f
		bne-	cr7, 1f
; reset FIQ|IRQ enables from gOldStatus
		nop
1:
		li		r3, 0
		b		Done
2:
		addis	r3, 0, hi16(tooManyAtomicExits)
		ori	r3, r3, lo16(tooManyAtomicExits)
		b		_DebugStr$stub

;--5----------------------------------------------------------------------------
;	GenericSWI(int inSelector, ...)
DoGeneric:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		lis	r31, ha16(_gCurrentTaskSaved)
		stw	r30, lo16(_gCurrentTaskSaved)(r31)	; gCurrentTaskSaved = gCurrentTask

		mflr	r31
		stw	r31, frlk(r1)
		bl		_GenericSWIHandler	; call back in C
		lwz	r31, frlk(r1)
		mtlr	r31

		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		cmpwi	cr7, r30, 0
		bne+	cr7, 2f

; gCurrentTask == nil | save prev task state
		lis	r30, ha16(_gCurrentTaskSaved)
		lwz	r30, lo16(_gCurrentTaskSaved)(r30)
										; save & disable IRQ | FIQ
;		stw	spsr, 0x94(r30)	; save PSR in task
		stw	r0, taskr0(r30)
		stw	r1, taskr1(r30)
		stw	r2, taskr2(r30)
		stmw	r7, taskr7(r30)	; save some registers
										; restore IRQ | FIQ
		lwz	r31, frr30(r1)		; overwrite correct r30-r31
		stw	r31, taskr30(r30)
		lwz	r31, frr31(r1)
		stw	r31, taskr31(r30)
		mflr	r31
		stw	r31, taskpc(r30)		; save PC
		b		Done

; gCurrentTask != nil
2:		lwz	r4, taskr4(r30)			; return registers r4-r6
		lwz	r5, taskr5(r30)
		lwz	r6, taskr6(r30)
		b		Done

;--6----------------------------------------------------------------------------
DoGenerateMessageIRQ:
; suspect this is unreachable
		b		Done

;--7----------------------------------------------------------------------------
DoPurgeMMUTLBEntry:
; don’t know what to do here
		b		Done

;--8----------------------------------------------------------------------------
DoFlushMMU:
; don’t know what to do here
		b		Done

;--9----------------------------------------------------------------------------
DoFlushCache:
; don’t know what to do here
		b		Done

;-10----------------------------------------------------------------------------
DoGetCPUVersion:
; don’t know what to do here
		li		r3, 0
		b		Done

;-11----------------------------------------------------------------------------
DoSemOp:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)	; original saves in R3

		mflr	r31
		stw	r31, frlk(r1)
		bl		_DoSemaphoreOp
		lwz	r31, frlk(r1)
		mtlr	r31

		lis	r31, ha16(_gCurrentTask)
		lwz	r31, lo16(_gCurrentTask)(r31)
		cmpwi	cr7, r31, 0
		bne-	cr7, Done
; gCurrentTask == nil
		mflr	r31
		subi	r31, r31, 4			; re-run this semop call
		mtlr	r31
										; save & disable IRQ | FIQ
;		stw	spsr, 0x94(r30)	; save PSR in task
		stmw	r0, taskr0(r30)	; save all registers
										; restore IRQ | FIQ
		lwz	r31, frr30(r1)		; overwrite correct r30-r31
		stw	r31, taskr30(r30)
		lwz	r31, frr31(r1)
		stw	r31, taskr31(r30)
		mflr	r31
		stw	r31, taskpc(r30)		; save PC

		mr		r3, r30
		b		Done

;-12----------------------------------------------------------------------------
DoSetDomainRegister:
; don’t know what to do here
		b		Done

;-13----------------------------------------------------------------------------
DoSMemSetBuffer:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemSetBufferKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-14----------------------------------------------------------------------------
DoSMemGetSize:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemGetSizeKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31

		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		cmpwi	cr7, r30, 0
		beq-	cr7, Done
; gCurrentTask != nil
		lwz	r4, taskr4(r30)			; return registers r4-r6
		lwz	r5, taskr5(r30)
		lwz	r6, taskr6(r30)
		b		Done

;-15----------------------------------------------------------------------------
DoSMemCopyToShared:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		mflr	r31
		stw	r31, taskpc(r30)			; save LK, we’re going to change it

		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemCopyToKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		mr		r31, r3

		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		lwz	r3, taskr3(r30)			; return registers r3-r5
		lwz	r4, taskr4(r30)
		lwz	r5, taskr5(r30)
		cmpwi	cr7, r31, 1
		beq-	cr7, 1f
		cmpwi	cr7, r31, 4
		beq-	cr7, 4f
		mr		r4, r5
		mr		r3, r31
		b		Done
1:
		lis	r30, hi16(LowLevelCopyEngine)
		ori	r30, r30, lo16(LowLevelCopyEngine)
		mtlr	r30
		b		Done
4:
		lis	r30, hi16(LowLevelCopyEngineLong)
		ori	r30, r30, lo16(LowLevelCopyEngineLong)
		mtlr	r30
		b		Done

;-16----------------------------------------------------------------------------
DoSMemCopyFromShared:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		mflr	r31
		stw	r31, taskpc(r30)

		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemCopyFromKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		mr		r31, r3

		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		lwz	r3, taskr3(r30)			; return registers r3-r5
		lwz	r4, taskr4(r30)
		lwz	r5, taskr5(r30)
		cmpwi	cr7, r31, 1
		beq-	cr7, 1f
		cmpwi	cr7, r31, 4
		beq-	cr7, 4f
		mr		r4, r5
		mr		r3, r31
		b		Done
1:
		lis	r30, hi16(LowLevelCopyEngine)
		ori	r30, r30, lo16(LowLevelCopyEngine)
		mtlr	r30
		b		Done
4:
		lis	r30, hi16(LowLevelCopyEngineLong)
		ori	r30, r30, lo16(LowLevelCopyEngineLong)
		mtlr	r30
		b		Done

;-17----------------------------------------------------------------------------
DoSMemMsgSetTimerParms:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemMsgSetTimerParmsKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-18----------------------------------------------------------------------------
DoSMemMsgSetMsgAvailPort:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemMsgSetMsgAvailPortKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-19----------------------------------------------------------------------------
DoSMemMsgGetSenderTaskId:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemMsgGetSenderTaskIdKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31

		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		cmpwi	cr7, r30, 0
		beq-	cr7, Done
; gCurrentTask != nil
		lwz	r4, taskr4(r30)			; return registers r4-r6
		lwz	r5, taskr5(r30)
		lwz	r6, taskr6(r30)
		b		Done

;-20----------------------------------------------------------------------------
DoSMemMsgSetUserRefCon:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemMsgSetUserRefConKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-21----------------------------------------------------------------------------
DoSMemMsgGetUserRefCon:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemMsgGetUserRefConKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31

		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		cmpwi	cr7, r30, 0
		beq-	cr7, Done
; gCurrentTask != nil
		lwz	r4, taskr4(r30)			; return registers r4-r6
		lwz	r5, taskr5(r30)
		lwz	r6, taskr6(r30)
		b		Done

;-22----------------------------------------------------------------------------
DoSMemMsgCheckForDone:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemMsgCheckForDoneKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		mr		r3, r30
		b		Received

;-23----------------------------------------------------------------------------
DoSMemMsgMsgDone:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_SMemMsgMsgDoneKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-24----------------------------------------------------------------------------
DoTurnOffCache:
; don’t know what to do here
		b		Done

;-25----------------------------------------------------------------------------
DoTurnOnCache:
; don’t know what to do here
		b		Done

;-26----------------------------------------------------------------------------
DoLowLevelCopyDone:
		li		r3, 0
		lis	r30, ha16(_gCurrentTask)
		lwz	r4, lo16(_gCurrentTask)(r30)
		mflr	r5
		mflr	r31
		stw	r31, frlk(r1)
		bl		_LowLevelCopyDoneFromKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-27----------------------------------------------------------------------------
DoMonitorDispatch:
		lis	r30, ha16(_gCurrentTask)
		lwz	r30, lo16(_gCurrentTask)(r30)
										; save & disable IRQ | FIQ
;		stw	spsr, 0x94(r30)	; save PSR in task
		stmw	r0, taskr0(r30)	; save all registers
										; restore IRQ | FIQ
		lwz	r31, frr30(r1)		; overwrite correct r30-r31
		stw	r31, taskr30(r30)
		lwz	r31, frr31(r1)
		stw	r31, taskr31(r30)

		mflr	r31
		stw	r31, taskpc(r30)		; save PC

		mflr	r31
		stw	r31, frlk(r1)
		bl		_MonitorDispatchKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-28----------------------------------------------------------------------------
DoMonitorExit:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_MonitorExitKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-29----------------------------------------------------------------------------
DoMonitorThrow:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_MonitorThrowKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-30----------------------------------------------------------------------------
DoEnterFIQAtomic:
; disable FIQ | IRQ
		lis	r30, ha16(_gAtomicFIQNestCount)
		lwz	r31, lo16(_gAtomicFIQNestCount)(r30)
		addi	r31, r31, 1
		stw	r31, lo16(_gAtomicFIQNestCount)(r30)
		cmpwi	cr7, r31, 1
		bne-	cr7, 1f
; gAtomicFIQNestCount == 1
		nop							; save status bits into gOldFIQStatus
1:
		li		r3, 0
		b		Done

;-31----------------------------------------------------------------------------
DoExitFIQAtomic:
		lis	r30, ha16(_gAtomicFIQNestCount)
		lwz	r31, lo16(_gAtomicFIQNestCount)(r30)
		subi	r31, r31, 1
		stw	r31, lo16(_gAtomicFIQNestCount)(r30)
		cmpwi	cr7, r31, 0
		blt-	cr7, 2f
		bne-	cr7, 1f
; gAtomicFIQNestCount == 0
		nop							; restore status bits
1:
		li		r3, 0
		b		Done
2:
		addis	r3, 0, hi16(tooManyFIQExits)
		ori	r3, r3, lo16(tooManyFIQExits)
		b		_DebugStr$stub

;-32----------------------------------------------------------------------------
DoMonitorFlush:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_MonitorFlushKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-33----------------------------------------------------------------------------
DoPortResetFilter:
		mflr	r31
		stw	r31, frlk(r1)
		bl		_PortResetFilterKernelGlue
		lwz	r31, frlk(r1)
		mtlr	r31
		b		Done

;-34----------------------------------------------------------------------------
DoScheduler:
		li		r3, 0					; return noErr
		b		Done

;-35..--------------------------------------------------------------------------
UndefinedSWI:
		addis	r3, 0, hi16(undefinedSWI)
		ori	r3, r3, lo16(undefinedSWI)
		bl		_DebugStr$stub
		li		r3, -1				; return non-specific error
		b		Done


;-------------------------------------------------------------------------------
;	void LowLevelCopyEngine(char * inTo, char * inFrom, size_t inSize)
;
;	Copy memory, char-at-a-time.
;	Args:		r3	inTo
;				r4	inFrom
;				r5	inSize
;	Return	--

LowLevelCopyEngine:
		cmpwi	cr7, r5, 0
		beq-	cr7, 1f
2:		lbz	r0, 0(r4)
		addi	r4, r4, 1
		stb	r0, 0(r3)
		addi	r3, r3, 1
		subic.	r5, r5, 1
		bne+	2b
1:		li		r0, 26
		b		SWI


;-------------------------------------------------------------------------------
;	void LowLevelCopyEngineLong(char * inTo, char * inFrom, size_t inSize)
;
;	Copy memory, long-at-a-time.
;	Args:		r3	inTo
;				r4	inFrom
;				r5	inSize
;	Return	--

LowLevelCopyEngineLong:
		cmpwi	cr7, r5, 0
		beq-	cr7, 1f
2:		lwz	r0, 0(r4)
		addi	r4, r4, 4
		stw	r0, 0(r3)
		addi	r3, r3, 4
		subic.	r5, r5, 4
		bne+	2b
1:		li		r0, 26
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
