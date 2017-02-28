/*
	File:		SWIHandler.s

	Contains:	SWI handler LP64 assembler glue.
					All the functions in here have extern "C" linkage.
					See
					 http://www.x86-64.org/documentation/abi.pdf
					for the ABI.
					 https://developer.apple.com/library/mac/documentation/darwin/conceptual/64bitporting/64bitporting.pdf
					 http://www.classes.cs.uchicago.edu/archive/2009/spring/22620-1/docs/handout-03.pdf
					 http://www.cs.cmu.edu/~fp/courses/15213-s07/misc/asm64-handout.pdf
					for 64-bit assembler info.

					In LP64 ABI
						args are in				%rdi %rsi %rdx %rcx %r8 %r9
						return value is in	%rax

	Written by:	Newton Research Group, 2014.
*/

//	register offsets in CTask
		.set	taskrax,  0x18
		.set	taskrbx,  0x20
		.set	taskrcx,  0x28
		.set	taskrdx,  0x30
		.set	taskrsi,  0x38
		.set	taskrdi,  0x40
		.set	taskrbp,  0x48
		.set	taskrsp,  0x50
		.set	taskrip,  0x58
		.set	taskr8,   0x60
		.set	taskr9,   0x68
		.set	taskr10,  0x70
		.set	taskr11,  0x78
		.set	taskr12,  0x80
		.set	taskr13,  0x88
		.set	taskr14,  0x90
		.set	taskr15,  0x98
		.set	taskPSR,  0xE8		// sizeof(CObject) + 26*sizeof(long) == 0x18 + 0xD0 == 0xE8

		.set	taskarg1, 0x40		// == kParm0; rdi
		.set	taskarg2, 0x38
		.set	taskarg3, 0x30
		.set	taskarg4, 0x28

		.set	taskr1,	 0xC0		// == kReturnParm1; sizeof(CObject) + 21*sizeof(long) == 0x18 + 0xA8 == 0xC0
		.set	taskr2,	 0xC8
		.set	taskr3,	 0xD0
		.set	taskr4,	 0xD8

//	offsets in our stack frame, relative to rbp
		.set	frlk,    8

		.set	frrbp,   0
		.set	frtask, -8
		.set	frr15,  -16
		.set	frr14,  -24
		.set	frr13,  -32
		.set	frr12,  -40
		.set	frr11,  -48
		.set	frr10,  -56
		.set	frr9,   -64
		.set	frr8,   -72
		.set	frrdi,  -80
		.set	frrsi,  -88
		.set	frrdx,  -96
		.set	frrcx,  -104
		.set	frrbx,  -112
		.set	frrax,  -120
		.set	frSize, 128

/* ----------------------------------------------------------------
	A SWI has a stack that looks like:
		argn
		arg7					<-- 16 byte aligned
		return address

		rbp					<-- our stack frame, 16 byte aligned
		task
		r15
		r14
		r13
		r12
		r11
		r10
		r9
		r8
		rdi
		rsi
		rdx
		rcx
		rbx
		rax
		--						<-- 16 byte aligned
---------------------------------------------------------------- */

		.data
		.align	2
/* ----------------------------------------------------------------
	D A T A

	gCPUmode is a boolean indicating that we’re in the SWI.
	(This was originally in the ARM CPSR.)
	We could probably do with IRQ|FIQ enable flags too.
	We only use user and supervisor modes here.
---------------------------------------------------------------- */
		.globl	_gCPSR

_gSPSR:
		.long		1

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
	Handle the SWI.

	Args:		%rax				selector
				%rdi %rsi...	args as passed from C
	Return:	%rax				result
---------------------------------------------------------------- */
		.globl	SWI

SWI:
		pushq		%rbp					# preserve stack frame pointer
		movq		%rsp, %rbp			# set our own stack frame pointer
		subq		$frSize, %rsp		# create new stack frame

		movq		%rax, frrax(%rbp)
		movq		%rbx, frrbx(%rbp)
		movq		%rcx, frrcx(%rbp)
		movq		%rdx, frrdx(%rbp)
		movq		%rsi, frrsi(%rbp)
		movq		%rdi, frrdi(%rbp)
		movq		%r8,  frr8(%rbp)
		movq		%r9,  frr9(%rbp)
		movq		%r10, frr10(%rbp)
		movq		%r11, frr11(%rbp)
		movq		%r12, frr12(%rbp)
		movq		%r13, frr13(%rbp)
		movq		%r14, frr14(%rbp)
		movq		%r15, frr15(%rbp)

		movl		$(svc_32+IRQdisable+FIQdisable), %eax
		xchgl		%eax, _gCPSR(%rip)	# set supervisor mode, disable interrupts
		movl		%eax, _gSPSR(%rip)	# save PSR at SWI entry
		andl		$mode_mask, %eax
		cmpl		$usr_32, %eax		# MUST be in user mode
		je			1f
		leaq		nonUserModeSWI(%rip), %rdi
		call		_DebugStr$stub
		// reboot
1:
		movq		frrax(%rbp), %rax
		cmpl		$35, %eax			# switch on SWI selector
		jl			2f						# 0..34
		movl		$35, %eax			# UndefinedSWI selector
2:
		leaq		SWI_JT(%rip), %r11
		movl		0(%r11,%rax,4), %eax	# fetch offset of dispatch
		movslq	%eax,%rax				# sign extend
		addq		%r11, %rax				# make address
		jmp		*%rax						# go there

//	SWI handler jumps here when done.
Done:
		movq		%rax, frrax(%rbp)	# preserve return value
//	SWI work is done.
//	If scheduled, switch task now.

// disable IRQ | FIQ
		movl		$(svc_32+IRQdisable+FIQdisable), _gCPSR(%rip)

// if in nested interrupt, don’t swap task
		cmpl		$0, _gAtomicFIQNestCountFast(%rip)
		jne		NoSwap
		cmpl		$0, _gAtomicIRQNestCountFast(%rip)
		jne		NoSwap
		cmpl		$0, _gAtomicNestCount(%rip)
		jne		NoSwap
		cmpl		$0, _gAtomicFIQNestCount(%rip)
		jne		NoSwap

// re-enable FIQ (but not IRQ) --> fire pending FIQ
		movl		$(svc_32+IRQdisable), _gCPSR(%rip)

		cmpl		$0, _gWantDeferred(%rip)
		je			1f
// enable IRQ | FIQ --> fire pending IRQ,FIQ
		movl		$(svc_32), _gCPSR(%rip)
		call		_DoDeferrals
// disable IRQ again
		movl		$(svc_32+IRQdisable), _gCPSR(%rip)
1:
		cmpl		$0, _gDoSchedule(%rip)
		je			NoSwap

//	gDoSchedule == YES
		call		_Scheduler
		pushq		%rax					# push next task to run -- can never be NULL
		cmpb		$0, _gWantSchedulerToRun(%rip)
		je			2f						# gWantSchedulerToRun?
		subq		$8, %rsp
		call		_StartScheduler	# YES, start scheduler
		addq		$8, %rsp
2:
		popq		%r11
		cmpq		%r11, _gCurrentTask(%rip)	# if task hasn’t changed, don’t do task swap
		je			NoSwap

		xchgq		%r11, _gCurrentTask(%rip)	# update gCurrentTask

		cmpq		$0, %r11
		je			SwapIn				# if no prev task, don’t swap it out

// r11 = prev task
// gCurrentTask = new scheduled task
SwapOut:
		cmpb		$0, _gCopyDone(%rip)		# gCopyDone?
		jne		1f

//	gCopyDone == NO
		movl		_gSPSR(%rip), %eax		# save PSR in task
		movq		%rax, taskPSR(%r11)

		movl		_gCPSR(%rip), %ecx
		movl		$(svc_32+IRQdisable+FIQdisable), %eax
		xchgl		%eax, _gCPSR(%rip)		# disable interrupts while we’re saving registers

		movq		frrbx(%rbp), %rax			# save all registers
		movq		%rax, taskrbx(%r11)
		movq		frrcx(%rbp), %rax
		movq		%rax, taskrcx(%r11)
		movq		frrdx(%rbp), %rax
		movq		%rax, taskrdx(%r11)
		movq		frrsi(%rbp), %rax
		movq		%rax, taskrsi(%r11)
		movq		frrdi(%rbp), %rax
		movq		%rax, taskrdi(%r11)
		movq		frr8(%rbp), %rax
		movq		%rax, taskr8(%r11)
		movq		frr9(%rbp), %rax
		movq		%rax, taskr9(%r11)
		movq		frr10(%rbp), %rax
		movq		%rax, taskr10(%r11)
		movq		frr11(%rbp), %rax
		movq		%rax, taskr11(%r11)
		movq		frr12(%rbp), %rax
		movq		%rax, taskr12(%r11)
		movq		frr13(%rbp), %rax
		movq		%rax, taskr13(%r11)
		movq		frr14(%rbp), %rax
		movq		%rax, taskr14(%r11)
		movq		frr15(%rbp), %rax
		movq		%rax, taskr15(%r11)

		movq		frrbp(%rbp), %rax
		movq		%rax, taskrbp(%r11)
		leaq		16(%rbp), %rax				# <-- assume stack is always aligned with frame
		movq		%rax, taskrsp(%r11)

		movl		%ecx, _gCPSR(%rip)		# re-enable interrupts --> fire pending interrupt

		movq		frlk(%rbp), %rax
		movq		%rax, taskrip(%r11)		# save task’s PC when calling here, ie its LK
		jmp		SwapIn

//	gCopyDone == YES
1:
		movl		_gSPSR(%rip), %eax		# save PSR in task
		movq		%rax, taskPSR(%r11)

		movl		_gCPSR(%rip), %ecx
		movl		$(svc_32+IRQdisable+FIQdisable), %eax
		xchgl		%eax, _gCPSR(%rip)		# disable interrupts while we’re saving registers

		movq		taskrip(%r11), %rax
		movq		%rax, frlk(%rbp)			# restore saved PC in LK for return

		movq		frrbx(%rbp), %rax			# save all registers
		movq		%rax, taskrbx(%r11)
		movq		frrcx(%rbp), %rax
		movq		%rax, taskrcx(%r11)
		movq		frrdx(%rbp), %rax
		movq		%rax, taskrdx(%r11)
		movq		frrsi(%rbp), %rax
		movq		%rax, taskrsi(%r11)
		movq		frrdi(%rbp), %rax
		movq		%rax, taskrdi(%r11)
		movq		frr8(%rbp), %rax
		movq		%rax, taskr8(%r11)
		movq		frr9(%rbp), %rax
		movq		%rax, taskr9(%r11)
		movq		frr10(%rbp), %rax
		movq		%rax, taskr10(%r11)
		movq		frr11(%rbp), %rax
		movq		%rax, taskr11(%r11)
		movq		frr12(%rbp), %rax
		movq		%rax, taskr12(%r11)
		movq		frr13(%rbp), %rax
		movq		%rax, taskr13(%r11)
		movq		frr14(%rbp), %rax
		movq		%rax, taskr14(%r11)
		movq		frr15(%rbp), %rax
		movq		%rax, taskr15(%r11)

		movq		frrbp(%rbp), %rax
		movq		%rax, taskrbp(%r11)
		leaq		16(%rbp), %rax			# <-- assume stack is always aligned with frame
		movq		%rax, taskrsp(%r11)

		movq		frlk(%rbp), %rax	# sic, but redundant, surely?
		movq		%rax, taskrip(%r11)

		movl		%ecx, _gCPSR(%rip)		# re-enable interrupts --> fire pending interrupt

		movb		$0, _gCopyDone(%rip)		# gCopyDone = NO

// previous task state is saved
// now set up next task state

SwapIn:
		movq		_gCurrentTask(%rip), %rdi
		call		_SwapInGlobals #(gCurrentTask)

//	restore processor registers from new (current) task
		movq		_gCurrentTask(%rip), %r11
		movq		taskPSR(%r11), %rax
		movl		%eax, _gCPSR(%rip)	# restore interrupt mode --> fire pending interrupt
		call		_ServicePendingInterrupts

		movq		_gCurrentTask(%rip), %r11
		movq		taskrax(%r11), %rax	# restore all registers
		movq		taskrbx(%r11), %rbx
		movq		taskrcx(%r11), %rcx
		movq		taskrdx(%r11), %rdx
		movq		taskrsi(%r11), %rsi
		movq		taskrdi(%r11), %rdi
		movq		taskr8(%r11),  %r8
		movq		taskr9(%r11),  %r9
		movq		taskr10(%r11), %r10
		movq		taskr12(%r11), %r12
		movq		taskr13(%r11), %r13
		movq		taskr14(%r11), %r14
		movq		taskr15(%r11), %r15

		movq		taskrbp(%r11), %rbp
		movq		taskrsp(%r11), %rsp	# changing the stack!

		pushq		taskrip(%r11)			# restore saved PC ready for return
		movq		taskr11(%r11), %r11	# can finally restore r11

// set up new environment

		ret								# switch task!

//	gDoSchedule == NO
//	no task switch required
//	set up domain access anyways

NoSwap:
		cmpb		$0, _gWantSchedulerToRun(%rip)
		je			1f								# gWantSchedulerToRun?
		call		_StartScheduler			# YES, start scheduler
1:
		cmpb		$0, _gCopyDone(%rip)		# gCopyDone?
		jne		CopyIsDone

//	gCopyDone == NO
// set up new environment
		jmp		ExitSWI						# return to task - or to copy task

//	gCopyDone == YES
CopyIsDone:
		movb		$0, _gCopyDone(%rip)		# gCopyDone = NO

		movq		_gCurrentTask(%rip), %r11
		movq		taskrip(%r11), %rax		# set up saved PC ready for return
		movq		%rax, frlk(%rbp)
		movq		taskrsi(%r11), %rax		# also rsi, rdi
		movq		%rax, frrsi(%rbp)
		movq		taskrdi(%r11), %rax
		movq		%rax, frrdi(%rbp)

// set up new environment

ExitSWI:
		movl		_gSPSR(%rip), %eax
		movl		%eax, _gCPSR(%rip)		# restore interrupt mode --> fire pending interrupt
		call		_ServicePendingInterrupts

		movq		frrax(%rbp), %rax			# restore registers
		movq		frrbx(%rbp), %rbx
		movq		frrcx(%rbp), %rcx
		movq		frrdx(%rbp), %rdx
		movq		frrsi(%rbp), %rsi
		movq		frrdi(%rbp), %rdi
		movq		frr8(%rbp),  %r8
		movq		frr9(%rbp),  %r9
		movq		frr10(%rbp), %r10
		movq		frr11(%rbp), %r11
		movq		frr12(%rbp), %r12
		movq		frr13(%rbp), %r13
		movq		frr14(%rbp), %r14
		movq		frr15(%rbp), %r15

		addq		$frSize, %rsp		# epilog
		popq		%rbp
		ret								# return to task!

/* ----------------------------------------------------------------
	Copy Engine.
---------------------------------------------------------------- */

/* ----------------------------------------------------------------
	void
	LowLevelCopyEngine(char * inTo, char * inFrom, size_t inSize)

	Copy memory, char-at-a-time.
	Args:		 %rdi		inTo
				 %rsi		inFrom
				 %rcx		inSize
	Return	--
---------------------------------------------------------------- */

LowLevelCopyEngine:
		rep movsb

		movl		$26, %eax	# DoLowLevelCopyDone
		call		SWI
		ret


/* ----------------------------------------------------------------
	void
	LowLevelCopyEngineLong(char * inTo, char * inFrom, size_t inSize)

	Copy memory, long-at-a-time. (That’s 32-bits-at-a-time.)
	Args:		 %rdi		inTo
				 %rsi		inFrom
				 %rcx		inSize
	Return	--
---------------------------------------------------------------- */

LowLevelCopyEngineLong:
		shr		$2, %rcx
		rep movsd

		movl		$26, %eax	# DoLowLevelCopyDone
		call		SWI
		ret


/* ----------------------------------------------------------------
	Individual SWI handlers.
---------------------------------------------------------------- */

/*  0 -------------------------------------------------------------
	ObjectId
	GetPortSWI(int inWhat)
---------------------------------------------------------------- */

DoGetPort:
		call		_GetPortInfo
		jmp		Done


/*  1 -------------------------------------------------------------
	NewtonErr
	PortSendSWI(ObjectId inId, ULong inMsgId, ULong inMemId, ULong inMsgType, ULong inFlags)
---------------------------------------------------------------- */
DoPortSend:
		movq		_gCurrentTask(%rip), %rax
		movq		%rax, frtask(%rbp)			# save gCurrentTask

		call		_PortSendKernelGlue		# might switch current task

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			2f
//	gCurrentTask != NULL
		movq		taskrax(%r11), %rax		# return fRegister[kResultReg]
		jmp		Done

// gCurrentTask == NULL => task switched
// so save prev task state
2:		movq		%rax, frrax(%rbp)		# save return code
		movq		frtask(%rbp), %r11

		movl		_gSPSR(%rip), %eax		# save PSR in task
		movq		%rax, taskPSR(%r11)

		movl		_gCPSR(%rip), %ecx
		movl		$(svc_32+IRQdisable+FIQdisable), %eax
		xchgl		%eax, _gCPSR(%rip)		# disable interrupts while we’re saving registers

		movq		frrbx(%rbp), %rax			# save all registers
		movq		%rax, taskrbx(%r11)
		movq		frrcx(%rbp), %rax
		movq		%rax, taskrcx(%r11)
		movq		frrdx(%rbp), %rax
		movq		%rax, taskrdx(%r11)
		movq		frrsi(%rbp), %rax
		movq		%rax, taskrsi(%r11)
		movq		frrdi(%rbp), %rax
		movq		%rax, taskrdi(%r11)
		movq		frr8(%rbp), %rax
		movq		%rax, taskr8(%r11)
		movq		frr9(%rbp), %rax
		movq		%rax, taskr9(%r11)
		movq		frr10(%rbp), %rax
		movq		%rax, taskr10(%r11)
		movq		frr11(%rbp), %rax
		movq		%rax, taskr11(%r11)
		movq		frr12(%rbp), %rax
		movq		%rax, taskr12(%r11)
		movq		frr13(%rbp), %rax
		movq		%rax, taskr13(%r11)
		movq		frr14(%rbp), %rax
		movq		%rax, taskr14(%r11)
		movq		frr15(%rbp), %rax
		movq		%rax, taskr15(%r11)

		movq		frrbp(%rbp), %rax
		movq		%rax, taskrbp(%r11)
		leaq		frrbp+16(%rbp), %rax		# <-- assume stack is always aligned with frame
		movq		%rax, taskrsp(%r11)

		movl		%ecx, _gCPSR(%rip)		# re-enable interrupts --> fire pending interrupt

		movq		frlk(%rbp), %rax
		movq		%rax, taskrip(%r11)	# save PC

		movq		frrax(%rbp), %rax		# restore return code
		jmp		Done


/*  2 -------------------------------------------------------------
	NewtonErr
	PortReceiveSWI(ObjectId inId, ObjectId inMsgId, ULong inMsgFilter, ULong inFlags)
---------------------------------------------------------------- */
DoPortReceive:
		movq		_gCurrentTask(%rip), %rax
		movq		%rax, frtask(%rbp)		# save gCurrentTask

		call		_PortReceiveKernelGlue	# might switch current task

Received:
		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			2f
//	gCurrentTask != NULL
		movq		taskrax(%r11), %rax		# return fRegister[kResultReg]
		jmp		Done

// gCurrentTask == NULL | save prev task state
2:		movq		%rax, frrax(%rbp)
		movq		frtask(%rbp), %r11

		movl		_gSPSR(%rip), %eax		# save PSR in task
		movq		%rax, taskPSR(%r11)

		movl		_gCPSR(%rip), %ecx
		movl		$(svc_32+IRQdisable+FIQdisable), %eax
		xchgl		%eax, _gCPSR(%rip)		# disable interrupts while we’re saving registers

		movq		frrbx(%rbp), %rax			# save all registers
		movq		%rax, taskrbx(%r11)
		movq		frrcx(%rbp), %rax
		movq		%rax, taskrcx(%r11)
		movq		frrdx(%rbp), %rax
		movq		%rax, taskrdx(%r11)
		movq		frrsi(%rbp), %rax
		movq		%rax, taskrsi(%r11)
		movq		frrdi(%rbp), %rax
		movq		%rax, taskrdi(%r11)
		movq		frr8(%rbp), %rax
		movq		%rax, taskr8(%r11)
		movq		frr9(%rbp), %rax
		movq		%rax, taskr9(%r11)
		movq		frr10(%rbp), %rax
		movq		%rax, taskr10(%r11)
		movq		frr11(%rbp), %rax
		movq		%rax, taskr11(%r11)
		movq		frr12(%rbp), %rax
		movq		%rax, taskr12(%r11)
		movq		frr13(%rbp), %rax
		movq		%rax, taskr13(%r11)
		movq		frr14(%rbp), %rax
		movq		%rax, taskr14(%r11)
		movq		frr15(%rbp), %rax
		movq		%rax, taskr15(%r11)

		movq		frrbp(%rbp), %rax
		movq		%rax, taskrbp(%r11)
		leaq		frrbp+16(%rbp), %rax		# <-- assume stack is always aligned with frame
		movq		%rax, taskrsp(%r11)

		movl		%ecx, _gCPSR(%rip)		# re-enable interrupts --> fire pending interrupt

		movq		frlk(%rbp), %rax
		movq		%rax, taskrip(%r11)		# save PC

		movq		frrax(%rbp), %rax			# restore return code
		jmp		Done


/*  3 -------------------------------------------------------------
	don’t think this is used
---------------------------------------------------------------- */
DoEnterAtomic:
// disable FIQ|IRQ
		incl		_gAtomicNestCount(%rip)
		cmpl		$1, _gAtomicNestCount(%rip)
		jne		1f
// save SPSR in gOldStatus
		nop
1:
// re-enable FIQ
		movl		$0, %eax
		jmp		Done


/*  4 -------------------------------------------------------------
	don’t think this is used
---------------------------------------------------------------- */
DoExitAtomic:
		decl		_gAtomicNestCount(%rip)
		cmpl		$0, _gAtomicNestCount(%rip)
		jl			2f
		jne		1f
// reset FIQ|IRQ enables from gOldStatus
		nop
1:
		movl		$0, %eax
		jmp		Done
2:
		leaq		tooManyAtomicExits(%rip), %rdi
		call		_DebugStr$stub
3:		nop
		jmp		3b


/*  5 -------------------------------------------------------------
	NewtonErr
	GenericSWI(int inSelector, ...)
---------------------------------------------------------------- */
DoGeneric:
		movq		_gCurrentTask(%rip), %rax
		movq		%rax, frtask(%rbp)

		call		_GenericSWIHandler	# call back in C

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		je			2f
// gCurrentTask != NULL
//		movl		taskrax(%r11), %rax		# return fRegister[kResultReg]
		movq		%rax, frrax(%rbp)
		movq		frrax(%rbp), %rax
		jmp		Done

// gCurrentTask == NULL | save prev task state
2:		movq		%rax, frrax(%rbp)
		movq		frtask(%rbp), %r11

		movl		_gSPSR(%rip), %eax		# save PSR in task
		movq		%rax, taskPSR(%r11)

		movl		_gCPSR(%rip), %ecx
		movl		$(svc_32+IRQdisable+FIQdisable), %eax
		xchgl		%eax, _gCPSR(%rip)		# disable interrupts while we’re saving registers

		movq		frrbx(%rbp), %rax			# save all registers
		movq		%rax, taskrbx(%r11)
		movq		frrcx(%rbp), %rax
		movq		%rax, taskrcx(%r11)
		movq		frrdx(%rbp), %rax
		movq		%rax, taskrdx(%r11)
		movq		frrsi(%rbp), %rax
		movq		%rax, taskrsi(%r11)
		movq		frrdi(%rbp), %rax
		movq		%rax, taskrdi(%r11)

		movq		frr8(%rbp), %rax
		movq		%rax, taskr8(%r11)
		movq		frr9(%rbp), %rax
		movq		%rax, taskr9(%r11)
		movq		frr10(%rbp), %rax
		movq		%rax, taskr10(%r11)
		movq		frr11(%rbp), %rax
		movq		%rax, taskr11(%r11)
		movq		frr12(%rbp), %rax
		movq		%rax, taskr12(%r11)
		movq		frr13(%rbp), %rax
		movq		%rax, taskr13(%r11)
		movq		frr14(%rbp), %rax
		movq		%rax, taskr14(%r11)
		movq		frr15(%rbp), %rax
		movq		%rax, taskr15(%r11)

		movq		%rbp, taskrbp(%r11)
		movq		%rsp, taskrsp(%r11)

		movl		%ecx, _gCPSR(%rip)		# re-enable interrupts --> fire pending interrupt

		movq		frlk(%rbp), %rax
		movq		%rax, taskrip(%r11)		# save PC

		movq		frrax(%rbp), %rax
		jmp		Done


/*  6 -------------------------------------------------------------
	don’t think this is reachable
---------------------------------------------------------------- */
DoGenerateMessageIRQ:
		jmp		Done


/*  7 -------------------------------------------------------------
	don’t know what to do here
---------------------------------------------------------------- */
DoPurgeMMUTLBEntry:
		jmp		Done


/*  8 -------------------------------------------------------------
	don’t know what to do here
---------------------------------------------------------------- */
DoFlushMMU:
		jmp		Done


/*  9 -------------------------------------------------------------
	don’t know what to do here
---------------------------------------------------------------- */
DoFlushCache:
		jmp		Done


/* 10 -------------------------------------------------------------
	don’t know what to do here
---------------------------------------------------------------- */
DoGetCPUVersion:
		movl		$0, %eax
		jmp		Done


/* 11 -------------------------------------------------------------
	NewtonErr
	SemaphoreOpGlue(ObjectId inGroupId, ObjectId inListId, SemFlags inBlocking)
---------------------------------------------------------------- */
DoSemOp:
		movq		_gCurrentTask(%rip), %rcx
		movq		%rcx, frtask(%rbp)

		call		_DoSemaphoreOp	#(ObjectId inGroupId, ObjectId inListId, SemFlags inBlocking, CTask * inTask)

		movq		_gCurrentTask(%rip), %r11
		cmpq		$0, %r11
		jne		2f

// gCurrentTask == NULL
		movq		frlk(%rbp), %rax
		subq		$4, %rax			# re-run this semop call
		movq		%rax, frlk(%rbp)

		movq		%rax, frrax(%rbp)
		movq		frtask(%rbp), %r11

		movl		_gSPSR(%rip), %ecx		# save PSR in task
		movq		%rcx, taskPSR(%r11)

		movl		$(svc_32+IRQdisable+FIQdisable), %eax
		xchgl		%eax, _gCPSR(%rip)		# disable interrupts while we’re saving registers

		movq		frrbx(%rbp), %rax			# save all registers
		movq		%rax, taskrbx(%r11)
		movq		frrcx(%rbp), %rax
		movq		%rax, taskrcx(%r11)
		movq		frrdx(%rbp), %rax
		movq		%rax, taskrdx(%r11)
		movq		frrsi(%rbp), %rax
		movq		%rax, taskrsi(%r11)
		movq		frrdi(%rbp), %rax
		movq		%rax, taskrdi(%r11)

		movq		frr8(%rbp), %rax
		movq		%rax, taskr8(%r11)
		movq		frr9(%rbp), %rax
		movq		%rax, taskr9(%r11)
		movq		frr10(%rbp), %rax
		movq		%rax, taskr10(%r11)
		movq		frr11(%rbp), %rax
		movq		%rax, taskr11(%r11)
		movq		frr12(%rbp), %rax
		movq		%rax, taskr12(%r11)
		movq		frr13(%rbp), %rax
		movq		%rax, taskr13(%r11)
		movq		frr14(%rbp), %rax
		movq		%rax, taskr14(%r11)
		movq		frr15(%rbp), %rax
		movq		%rax, taskr15(%r11)

		movq		%rbp, taskrbp(%r11)
		movq		%rsp, taskrsp(%r11)

		movl		%ecx, _gCPSR(%rip)		# re-enable interrupts --> fire pending interrupt

		movq		frlk(%rbp), %rax
		movq		%rax, taskrip(%r11)		# save PC
		movq		frrax(%rbp), %rax

2:		jmp		Done


/* 12 -------------------------------------------------------------
	don’t know what to do here
---------------------------------------------------------------- */
DoSetDomainRegister:
		jmp		Done


/* 13 -------------------------------------------------------------
	NewtonErr
	SMemSetBufferSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inPermissions)
---------------------------------------------------------------- */
DoSMemSetBuffer:
		call		_SMemSetBufferKernelGlue
		jmp		Done


/* 14 -------------------------------------------------------------
	NewtonErr
	SMemGetSizeSWI(ObjectId inId, size_t * outSize, void ** outBuffer, OpaqueRef * outRefCon)
---------------------------------------------------------------- */
DoSMemGetSize:
		call		_SMemGetSizeKernelGlue	#(ObjectId inId)
// results are in gCurrentTask->fRegister[kReturnParm1..3]
2:		jmp		Done


/* 15 -------------------------------------------------------------
	NewtonErr
	SMemCopyToSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)
---------------------------------------------------------------- */
DoSMemCopyToShared:
		movq		_gCurrentTask(%rip), %r11
		movq		frlk(%rbp), %rax
		movq		%rax, taskrip(%r11)		# save PC

		call		_SMemCopyToKernelGlue #(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)

		movq		%rax, frrax(%rbp)			# save return value
// copy return values r1-r3
		movq		_gCurrentTask(%rip), %r11
		movq		taskr1(%r11), %rax		# to
		movq		%rax, frrdi(%rbp)
		movq		taskr2(%r11), %rax		# from
		movq		%rax, frrsi(%rbp)
		movq		taskr3(%r11), %rax		# size
		movq		%rax, frrcx(%rbp)
		movq		frrax(%rbp), %rax
		cmpl		$1, %eax
		je			1f
		cmpl		$4, %eax
		je			4f
//		mr		r4, r5
		jmp		Done

1:		// return to copy engine
		leaq		LowLevelCopyEngine(%rip), %r11
		movq		%r11, frlk(%rbp)			# overwrite return address on stack
		jmp		Done

4:		// return to copy engine
		leaq		LowLevelCopyEngineLong(%rip), %r11
		movq		%r11, frlk(%rbp)
		jmp		Done


/* 16 -------------------------------------------------------------
	NewtonErr
	SMemCopyFromSharedSWI(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature, size_t * outSize)
---------------------------------------------------------------- */
DoSMemCopyFromShared:
		movq		_gCurrentTask(%rip), %r11
		movq		frlk(%rbp), %rax
		movq		%rax, taskrip(%r11)		# save PC

		call		_SMemCopyFromKernelGlue #(ObjectId inId, void * outBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)

		movq		%rax, frrax(%rbp)			# save return value
// copy return values r1-r3
		movq		_gCurrentTask(%rip), %r11
		movq		taskr1(%r11), %rax		# to
		movq		%rax, frrdi(%rbp)
		movq		taskr2(%r11), %rax		# from
		movq		%rax, frrsi(%rbp)
		movq		taskr3(%r11), %rax		# size
		movq		%rax, frrcx(%rbp)
		movq		frrax(%rbp), %rax
		cmpl		$1, %eax
		je			1f
		cmpl		$4, %eax
		je			4f
//		mr		r4, r5
		jmp		Done

1:		// return to copy engine
		leaq		LowLevelCopyEngine(%rip), %r11
		movq		%r11, frlk(%rbp)			// overwrite return address on stack
		jmp		Done

4:		// return to copy engine
		leaq		LowLevelCopyEngineLong(%rip), %r11
		movq		%r11, frlk(%rbp)
		jmp		Done


/* 17 -------------------------------------------------------------
	NewtonErr
	SMemMsgSetTimerParmsSWI(ObjectId inId, ULong inTimeout, int64_t inDelay)
---------------------------------------------------------------- */
DoSMemMsgSetTimerParms:
		call		_SMemMsgSetTimerParmsKernelGlue
		jmp		Done


/* 18 -------------------------------------------------------------
	NewtonErr
	SMemMsgSetMsgAvailPortSWI(ObjectId inId, ObjectId inPortId)
---------------------------------------------------------------- */
DoSMemMsgSetMsgAvailPort:
		call		_SMemMsgSetMsgAvailPortKernelGlue
		jmp		Done


/* 19 -------------------------------------------------------------
	NewtonErr
	SMemMsgGetSenderTaskIdSWI(ObjectId inId, ObjectId * outSenderTaskId)
---------------------------------------------------------------- */
DoSMemMsgGetSenderTaskId:
		call		_SMemMsgGetSenderTaskIdKernelGlue
// result is in gCurrentTask->fRegister[kReturnParm1]
2:		jmp		Done


/* 20 -------------------------------------------------------------
	NewtonErr
	SMemMsgSetUserRefConSWI(ObjectId inId, ULong inRefCon)
---------------------------------------------------------------- */
DoSMemMsgSetUserRefCon:
		call		_SMemMsgSetUserRefConKernelGlue
		jmp		Done


/* 21 -------------------------------------------------------------
	NewtonErr
	SMemMsgGetUserRefConSWI(ObjectId inId, OpaqueRef * outRefCon)
---------------------------------------------------------------- */
DoSMemMsgGetUserRefCon:
		call		_SMemMsgGetUserRefConKernelGlue
// result is in gCurrentTask->fRegister[kReturnParm1]
2:		jmp		Done


/* 22 -------------------------------------------------------------
	NewtonErr
	SMemMsgCheckForDoneSWI(ObjectId inId, ULong inFlags, ULong * outSentById, ULong * outReplyMemId, ULong * outMsgType, ULong * outSignature)
---------------------------------------------------------------- */
DoSMemMsgCheckForDone:
		movq		_gCurrentTask(%rip), %rax
		movq		%rax, frtask(%rbp)

		call		_SMemMsgCheckForDoneKernelGlue #(ObjectId inId, ULong inFlags)
// results are in gCurrentTask->fRegister[kReturnParm1..4]

		jmp		Received


/* 23 -------------------------------------------------------------
	NewtonErr
	SMemMsgMsgDoneSWI(ObjectId inId, NewtonErr inResult, ULong inSignature)
---------------------------------------------------------------- */
DoSMemMsgMsgDone:
		call		_SMemMsgMsgDoneKernelGlue
		jmp		Done


/* 24 -------------------------------------------------------------
	don’t know what to do here
---------------------------------------------------------------- */
DoTurnOffCache:
		jmp		Done


/* 25 -------------------------------------------------------------
	don’t know what to do here
---------------------------------------------------------------- */
DoTurnOnCache:
		jmp		Done


/* 26 -------------------------------------------------------------
	Tail call from LowLevelCopyEngine
---------------------------------------------------------------- */
DoLowLevelCopyDone:
		movl		$0, %edi
		movq		_gCurrentTask(%rip), %rsi
		movq		frlk(%rbp), %rdx
		call		_LowLevelCopyDoneFromKernelGlue #(NewtonErr inErr, CTask * inTask, VAddr inReturn) = (noErr, gCurrentTask, LK)
		jmp		Done


/* 27 -------------------------------------------------------------
	NewtonErr
	MonitorDispatchSWI(ObjectId inMonitorId, int inSelector, OpaqueRef inData)
---------------------------------------------------------------- */
DoMonitorDispatch:
		movq		_gCurrentTask(%rip), %r11

		movl		_gSPSR(%rip), %eax		# save PSR in task
		movq		%rax, taskPSR(%r11)

		movl		_gCPSR(%rip), %ecx
		movl		$(svc_32+IRQdisable+FIQdisable), %eax
		xchgl		%eax, _gCPSR(%rip)		# disable interrupts while we’re saving registers

		movq		frrdi(%rbp), %rax			# save registers rbx, rcx, rdx, rsi, rdi, rbp, rsp, rip
		movq		%rax, taskarg1(%r11)
		movq		frrsi(%rbp), %rax
		movq		%rax, taskarg2(%r11)
		movq		frrdx(%rbp), %rax
		movq		%rax, taskarg3(%r11)
		movq		frrcx(%rbp), %rax
		movq		%rax, taskarg4(%r11)

		movq		frrbx(%rbp), %rax
		movq		%rax, taskrbx(%r11)
		movq		frrcx(%rbp), %rax
		movq		%rax, taskrcx(%r11)
		movq		frrdx(%rbp), %rax
		movq		%rax, taskrdx(%r11)
		movq		frrsi(%rbp), %rax
		movq		%rax, taskrsi(%r11)
		movq		frrdi(%rbp), %rax
		movq		%rax, taskrdi(%r11)

		movq		frr8(%rbp), %rax
		movq		%rax, taskr8(%r11)
		movq		frr9(%rbp), %rax
		movq		%rax, taskr9(%r11)
		movq		frr10(%rbp), %rax
		movq		%rax, taskr10(%r11)
		movq		frr11(%rbp), %rax
		movq		%rax, taskr11(%r11)
		movq		frr12(%rbp), %rax
		movq		%rax, taskr12(%r11)
		movq		frr13(%rbp), %rax
		movq		%rax, taskr13(%r11)
		movq		frr14(%rbp), %rax
		movq		%rax, taskr14(%r11)
		movq		frr15(%rbp), %rax
		movq		%rax, taskr15(%r11)

		movq		frrbp(%rbp), %rax
		movq		%rax, taskrbp(%r11)
		leaq		frrbp+16(%rbp), %rax		# <-- assume stack is always aligned with frame
		movq		%rax, taskrsp(%r11)

		movl		%ecx, _gCPSR(%rip)		# re-enable interrupts --> fire pending interrupt

		movq		frlk(%rbp), %rax
		movq		%rax, taskrip(%r11)	# save PC

		call		_MonitorDispatchKernelGlue #(void)
		jmp		Done


/* 28 -------------------------------------------------------------
	NewtonErr
	MonitorExitSWI(int inMonitorResult, void * inContinuationPC)
---------------------------------------------------------------- */
DoMonitorExit:
		call		_MonitorExitKernelGlue
		jmp		Done


/* 29 -------------------------------------------------------------
	void
	MonitorThrowSWI(ExceptionName inName, void * inData, ExceptionDestructor inDestructor)
---------------------------------------------------------------- */
DoMonitorThrow:
		call		_MonitorThrowKernelGlue
		jmp		Done


/* 30 -------------------------------------------------------------
	don’t think this is used
---------------------------------------------------------------- */
DoEnterFIQAtomic:
// disable FIQ | IRQ
		incl		_gAtomicFIQNestCount(%rip)
		cmpl		$1, _gAtomicFIQNestCount(%rip)
		jne		1f
// gAtomicFIQNestCount == 1
		nop							# save status bits into gOldFIQStatus
1:
		movl		$0, %eax
		jmp		Done


/* 31 -------------------------------------------------------------
	don’t think this is used
---------------------------------------------------------------- */
DoExitFIQAtomic:
		decl		_gAtomicFIQNestCount(%rip)
		cmpl		$0, _gAtomicFIQNestCount(%rip)
		jl			2f
		jne		1f
// gAtomicFIQNestCount == 0
		nop							# restore status bits
1:
		movl		$0, %eax
		jmp		Done
2:
		leaq		tooManyFIQExits(%rip), %rdi
		call		_DebugStr$stub
3:		nop
		jmp		3b


/* 32 -------------------------------------------------------------
	NewtonErr
	MonitorFlushSWI(ObjectId inMonitorId)
---------------------------------------------------------------- */
DoMonitorFlush:
		call		_MonitorFlushKernelGlue
		jmp		Done


/* 33 -------------------------------------------------------------
	NewtonErr
	PortResetFilterSWI(ObjectId inId, ULong inMsgId, ULong inMsgFilter)
---------------------------------------------------------------- */
DoPortResetFilter:
		call		_PortResetFilterKernelGlue
		jmp		Done


/* 34 -------------------------------------------------------------
	NewtonErr
	DoSchedulerSWI(void)
---------------------------------------------------------------- */
DoScheduler:
		movl		$0, %eax					# return noErr
		jmp		Done


/* 35.. -----------------------------------------------------------
	Anything else is undefined.
---------------------------------------------------------------- */
UndefinedSWI:
		leaq		undefinedSWI(%rip), %rdi
		call		_DebugStr$stub
		movl		$-1, %eax				# return non-specific error
		jmp		Done


/* ----------------------------------------------------------------
	SWI jump table.
	Gotta keep it all PC-relative.
---------------------------------------------------------------- */
		.const

SWI_JT:
		.long	DoGetPort - SWI_JT					// 0
		.long	DoPortSend - SWI_JT
		.long	DoPortReceive - SWI_JT
		.long	DoEnterAtomic - SWI_JT
		.long	DoExitAtomic - SWI_JT
		.long	DoGeneric - SWI_JT					// 5
		.long	DoGenerateMessageIRQ - SWI_JT
		.long	DoPurgeMMUTLBEntry - SWI_JT
		.long	DoFlushMMU - SWI_JT
		.long	DoFlushCache - SWI_JT
		.long	DoGetCPUVersion - SWI_JT			// 10
		.long	DoSemOp - SWI_JT
		.long	DoSetDomainRegister - SWI_JT
		.long	DoSMemSetBuffer - SWI_JT
		.long	DoSMemGetSize - SWI_JT
		.long	DoSMemCopyToShared - SWI_JT		// 15
		.long	DoSMemCopyFromShared - SWI_JT
		.long	DoSMemMsgSetTimerParms - SWI_JT
		.long	DoSMemMsgSetMsgAvailPort - SWI_JT
		.long	DoSMemMsgGetSenderTaskId - SWI_JT
		.long	DoSMemMsgSetUserRefCon - SWI_JT	// 20
		.long	DoSMemMsgGetUserRefCon - SWI_JT
		.long	DoSMemMsgCheckForDone - SWI_JT
		.long	DoSMemMsgMsgDone - SWI_JT
		.long	DoTurnOffCache - SWI_JT
		.long	DoTurnOnCache - SWI_JT				// 25
		.long	DoLowLevelCopyDone - SWI_JT
		.long	DoMonitorDispatch - SWI_JT
		.long	DoMonitorExit - SWI_JT
		.long	DoMonitorThrow - SWI_JT
		.long	DoEnterFIQAtomic - SWI_JT			// 30
		.long	DoExitFIQAtomic - SWI_JT
		.long	DoMonitorFlush - SWI_JT
		.long	DoPortResetFilter - SWI_JT
		.long	DoScheduler - SWI_JT					// 34

		.long UndefinedSWI - SWI_JT				// 35


/* ----------------------------------------------------------------
	S T U B S
---------------------------------------------------------------- */

		.section __IMPORT,__jump_table,symbol_stubs,self_modifying_code+pure_instructions,5

_DebugStr$stub:
		.indirect_symbol _DebugStr
		hlt ; hlt ; hlt ; hlt ; hlt

