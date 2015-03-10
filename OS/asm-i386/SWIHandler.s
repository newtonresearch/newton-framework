/*
	File:		SWIHandler.s

	Contains:	SWI handler i386 assembler glue.
					All the functions in here have extern "C" linkage.
					See http://developer.apple.com/mac/library/documentation/DeveloperTools/Reference/Assembler/000-Introduction/introduction.html
					 or http://developer.apple.com/mac/library/documentation/DeveloperTools/Conceptual/LowLevelABI/130-IA-32_Function_Calling_Conventions/IA32.html
					for the ABI.

	Written by:	Newton Research Group, 2009.
*/

//	register offsets in CTask
		.set	taskeax,  0x10
		.set	taskebx,  0x14
		.set	taskecx,  0x18
		.set	taskedx,  0x1C
		.set	taskesi,  0x20
		.set	taskedi,  0x24
		.set	taskebp,  0x28
		.set	taskesp,  0x2C
		.set	taskeip,  0x30

		.set	taskarg1, 0x34		// kParm0
		.set	taskarg2, 0x38
		.set	taskarg3, 0x3C
		.set	taskarg4, 0x40

		.set	taskr1,	 0x44		// kReturnParm1
		.set	taskr2,	 0x48
		.set	taskr3,	 0x4C
		.set	taskr4,	 0x50

//	offsets in our stack frame, relative to ebp
		.set	frret4, 20
		.set	frret3, 16
		.set	frret2, 12
		.set	frret1,  8
		.set	frlk,    4

		.set	frebp,   0
		.set	frrets,  -4
//				unused   -8
//				unused  -12
		.set	frtask, -16
		.set	fredi,  -20
		.set	fresi,  -24
		.set	fredx,  -28
		.set	frecx,  -32
		.set	frebx,  -36
		.set	freax,  -40
		.set	frarg8, -44
		.set	frarg7, -48
		.set	frarg6, -52
		.set	frarg5, -56
		.set	frarg4, -60
		.set	frarg3, -64
		.set	frarg2, -68
		.set	frarg1, -72
		.set	frSize, 72

/* ----------------------------------------------------------------
	A SWI has a stack that looks like:
		argn
		arg1					<-- 16 byte aligned
		return address

		ebp					  0  <-- our stack frame
		unused
		unused
		unused
		task					-16
		edi
		esi
		edx
		ecx					-32
		ebx
		eax
		copied arg8
		copied arg7			-48
		copied arg6
		copied arg5
		copied arg4
		copied arg3			-64
		copied arg2
		copied arg1			<-- 16 byte aligned
		fake return address
---------------------------------------------------------------- */

		.data
		.align	2
/* ----------------------------------------------------------------
	D A T A

	gCPUmode is a boolean indicating that we’re in the SWI
	We could probably do with IRQ|FIQ enable flags too.
---------------------------------------------------------------- */
		.globl	_gCPUmode

_gCPUmode:
		.long		1


		.text
		.align	2
/* ------------------------------------------------------------- */

/* ----------------------------------------------------------------
	Handle the SWI and write return values into the stack frame.
	Args are at 20(%esp) cf. 4(%esp) for plain vanilla SWI.

	Args:		%eax			selector
				4(%esp)...	args as passed from C, on the stack
	Return:	%eax			result
---------------------------------------------------------------- */
		.globl	SWIandReturn

SWIandReturn:
		pushl		%ebp					# preserve stack frame pointer
		movl		%esp, %ebp			# set our own stack frame pointer
		subl		$frSize, %esp		# create new stack frame
		movl		$1, frrets(%ebp)

		movl		%eax, freax(%ebp)
		movl		%ebx, frebx(%ebp)
		movl		%ecx, frecx(%ebp)
		movl		%edx, fredx(%ebp)
		movl		%esi, fresi(%ebp)
		movl		%edi, fredi(%ebp)

		leal		frret1+16(%ebp), %esi
		leal		frarg1(%ebp), %edi
		movl		$8, %ecx
		rep movsd						# copy args into new stack frame

		jmp		SWIdo


/* ----------------------------------------------------------------
	Handle the SWI.

	Args:		%eax			selector
				4(%esp)...	args as passed from C, on the stack
	Return:	%eax			result
---------------------------------------------------------------- */
		.globl	SWI

SWI:
		pushl		%ebp					# preserve stack frame pointer
		movl		%esp, %ebp			# set our own stack frame pointer
		subl		$frSize, %esp		# create new stack frame
		movl		$0, frrets(%ebp)

		movl		%eax, freax(%ebp)
		movl		%ebx, frebx(%ebp)
		movl		%ecx, frecx(%ebp)
		movl		%edx, fredx(%ebp)
		movl		%esi, fresi(%ebp)
		movl		%edi, fredi(%ebp)

		leal		frret1(%ebp), %esi
		leal		frarg1(%ebp), %edi
		movl		$8, %ecx
		rep movsd						# copy args into new stack frame

SWIdo:
		movl		$1, _gCPUmode		# enter interrupt mode

		movl		freax(%ebp), %eax
		cmpl		$35, %eax			# switch on SWI selector
		jge		UndefinedSWI		# 0..34
		movl		pSWI_JT, %edx
		movl		0(%edx,%eax,4), %eax	# fetch address of dispatch
		jmp		*%eax					# go there

//	SWI handler returns here when done.
Done:
		movl		%eax, freax(%ebp)
//	SWI work is done.
//	If scheduled, switch task now.

// disable IRQ | FIQ

// if in nested interrupt, don’t swap task
		cmpl		$0, _gAtomicFIQNestCountFast
		jne		NoSwap
		cmpl		$0, _gAtomicIRQNestCountFast
		jne		NoSwap
		cmpl		$0, _gAtomicNestCount
		jne		NoSwap
		cmpl		$0, _gAtomicFIQNestCount
		jne		NoSwap

// re-enable FIQ (but not IRQ)

		cmpl		$0, _gWantDeferred
		je			1f
// enable IRQ | FIQ
		call		_DoDeferrals
// disable IRQ again
1:
		cmpl		$0, _gDoSchedule
		je			NoSwap

//	gDoSchedule == YES
		call		_Scheduler
		pushl		%eax					# push next task to run
		cmpl		$0, _gWantSchedulerToRun
		je			2f						# gWantSchedulerToRun?
		subl		$12, %esp
		call		_StartScheduler	# YES, start scheduler
		addl		$12, %esp
2:
		popl		%edx
		cmpl		%edx, _gCurrentTask	# if task hasn’t changed, don’t do task swap
		je			NoSwap

		xchgl		%edx, _gCurrentTask	# update gCurrentTask

		cmpl		$0, %edx
		je			SwapIn				# if no prev task, don’t swap it out

// edx = prev task
// gCurrentTask = new scheduled task
SwapOut:
		cmpl		$0, _gCopyDone		# gCopyDone?
		jne		1f

//	gCopyDone == NO
											# save & disable IRQ | FIQ
		movl		frebx(%ebp), %eax	# save all registers
		movl		%eax, taskebx(%edx)
		movl		frecx(%ebp), %eax
		movl		%eax, taskecx(%edx)
		movl		fredx(%ebp), %eax
		movl		%eax, taskedx(%edx)
		movl		fresi(%ebp), %eax
		movl		%eax, taskesi(%edx)
		movl		fredi(%ebp), %eax
		movl		%eax, taskedi(%edx)
		movl		frebp(%ebp), %eax
		movl		%eax, taskebp(%edx)
		leal		8(%ebp), %eax			# <-- assume stack is always aligned with frame
		movl		%eax, taskesp(%edx)
											# restore IRQ | FIQ

		movl		frlk(%ebp), %eax
		movl		%eax, taskeip(%edx)		# save task’s PC when calling here, ie its LK
		jmp		SwapIn

//	gCopyDone == YES
1:
		movl		taskeip(%edx), %eax
		movl		%eax, frlk(%ebp)	# restore saved PC in LK for return

											# save & disable IRQ | FIQ
		movl		frebx(%ebp), %eax	# save all registers
		movl		%eax, taskebx(%edx)
		movl		frecx(%ebp), %eax
		movl		%eax, taskecx(%edx)
		movl		fredx(%ebp), %eax
		movl		%eax, taskedx(%edx)
		movl		fresi(%ebp), %eax
		movl		%eax, taskesi(%edx)
		movl		fredi(%ebp), %eax
		movl		%eax, taskedi(%edx)
		movl		frebp(%ebp), %eax
		movl		%eax, taskebp(%edx)
		leal		8(%ebp), %eax			# <-- assume stack is always aligned with frame
		movl		%eax, taskesp(%edx)
											# restore IRQ | FIQ

		movl		frlk(%ebp), %eax	# sic, but redundant, surely?
		movl		%eax, taskeip(%edx)

		movl		$0, _gCopyDone		# gCopyDone = NO

// previous task state is saved
// now set up next task state

SwapIn:
		subl		$12, %esp
		movl		_gCurrentTask, %eax
		pushl		%eax
		call		_SwapInGlobals		#(gCurrentTask)
		addl		$16, %esp

		movl		$0, _gCPUmode		# exit interrupt mode

//	restore processor registers from new (current) task
		movl		_gCurrentTask, %edx
											# set saved PSR
		movl		taskeax(%edx), %eax	# restore all registers
		movl		taskebx(%edx), %ebx
		movl		taskecx(%edx), %ecx
		movl		taskesi(%edx), %esi
		movl		taskedi(%edx), %edi
		movl		taskesp(%edx), %esp	# changing the stack!
		movl		taskebp(%edx), %ebp

		pushl		taskeip(%edx)		# restore saved PC ready for return
		movl		taskedx(%edx), %edx

// set up new environment

		ret								# switch task!

//	gDoSchedule == NO
//	no task switch required
//	set up domain access anyways

NoSwap:
		cmpl		$0, _gWantSchedulerToRun
		je			1f						# gWantSchedulerToRun?
		call		_StartScheduler	# YES, start scheduler
1:
		cmpl		$0, _gCopyDone		# gCopyDone?
		jne		CopyIsDone

//	gCopyDone == NO
// set up new environment
		jmp		ExitSWI				# return to task - or to copy task

//	gCopyDone == YES
CopyIsDone:
		movl		$0, _gCopyDone		# gCopyDone = NO

		movl		_gCurrentTask, %edx
		movl		taskeip(%edx), %eax		# set up saved PC ready for return
		movl		%eax, frlk(%ebp)
		movl		taskesi(%edx), %eax		# also esi, edi
		movl		%eax, fresi(%ebp)
		movl		taskedi(%edx), %eax
		movl		%eax, fredi(%ebp)

// set up new environment

ExitSWI:
		movl		$0, _gCPUmode		# exit interrupt mode

		movl		freax(%ebp), %eax
		movl		frebx(%ebp), %ebx
		movl		frecx(%ebp), %ecx
		movl		fredx(%ebp), %edx
		movl		fresi(%ebp), %esi
		movl		fredi(%ebp), %edi

		addl		$frSize, %esp		# epilog
		popl		%ebp
		ret								# return to task!


/* ----------------------------------------------------------------
	SWI jump table.
---------------------------------------------------------------- */
		.const

pSWI_JT:					// 003AD568
		.long	SWI_JT

SWI_JT:
		.long	DoGetPort					// 0
		.long	DoPortSend
		.long	DoPortReceive
		.long	DoEnterAtomic
		.long	DoExitAtomic
		.long	DoGeneric					// 5
		.long	DoGenerateMessageIRQ
		.long	DoPurgeMMUTLBEntry
		.long	DoFlushMMU
		.long	DoFlushCache
		.long	DoGetCPUVersion			// 10
		.long	DoSemOp
		.long	DoSetDomainRegister
		.long	DoSMemSetBuffer
		.long	DoSMemGetSize
		.long	DoSMemCopyToShared		// 15
		.long	DoSMemCopyFromShared
		.long	DoSMemMsgSetTimerParms
		.long	DoSMemMsgSetMsgAvailPort
		.long	DoSMemMsgGetSenderTaskId
		.long	DoSMemMsgSetUserRefCon	// 20
		.long	DoSMemMsgGetUserRefCon
		.long	DoSMemMsgCheckForDone
		.long	DoSMemMsgMsgDone
		.long	DoTurnOffCache
		.long	DoTurnOnCache				// 25
		.long	DoLowLevelCopyDone
		.long	DoMonitorDispatch
		.long	DoMonitorExit
		.long	DoMonitorThrow
		.long	DoEnterFIQAtomic			// 30
		.long	DoExitFIQAtomic
		.long	DoMonitorFlush
		.long	DoPortResetFilter
		.long	DoScheduler					// 34


/* ----------------------------------------------------------------
	Individual SWI handlers.
---------------------------------------------------------------- */
		.text

/*  0 -------------------------------------------------------------
	ObjectId GetPortSWI(int inWhat)
---------------------------------------------------------------- */

DoGetPort:
		call		_GetPortInfo
		jmp		Done

/*  1 -------------------------------------------------------------
	NewtonErr PortSendSWI(ObjectId inId, ULong inMsgId, ULong inMemId, ULong inMsgType, ULong inFlags)
---------------------------------------------------------------- */
DoPortSend:
		movl		_gCurrentTask, %eax
		movl		%eax, frtask(%ebp)			# save gCurrentTask

		call		_PortSendKernelGlue	# might switch current task

		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		je			2f
//	gCurrentTask != NULL
		movl		taskeax(%edx), %eax		# return fRegister[kResultReg]
		jmp		Done

// gCurrentTask == NULL | save prev task state
2:		movl		%eax, freax(%ebp)
		movl		frtask(%ebp), %edx
										# save & disable IRQ | FIQ
//		stw	spsr, 0x94(r30)	# save PSR in task
										# save all registers
		movl		frebx(%ebp), %eax
		movl		%eax, taskebx(%edx)
		movl		frecx(%ebp), %eax
		movl		%eax, taskecx(%edx)
		movl		fredx(%ebp), %eax
		movl		%eax, taskedx(%edx)
		movl		fresi(%ebp), %eax
		movl		%eax, taskesi(%edx)
		movl		fredi(%ebp), %eax
		movl		%eax, taskedi(%edx)

		leal		frebp(%ebp), %eax
		movl		0(%eax), %eax
		movl		%eax, taskebp(%edx)

		leal		frebp+8(%ebp), %eax
		movl		%eax, taskesp(%edx)
										# restore IRQ | FIQ
		movl		frlk(%ebp), %eax
		movl		%eax, taskeip(%edx)		# save PC
		movl		freax(%ebp), %eax
		jmp		Done

/*  2 -------------------------------------------------------------
	NewtonErr PortReceiveSWI(ObjectId inId, ObjectId inMsgId, ULong inMsgFilter, ULong inFlags)
---------------------------------------------------------------- */
DoPortReceive:
		movl		_gCurrentTask, %eax
		movl		%eax, frtask(%ebp)		# save gCurrentTask

		call		_PortReceiveKernelGlue	# might switch current task

Received:
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		je			2f
//	gCurrentTask != NULL
		movl		taskeax(%edx), %eax		# return fRegister[kResultReg]
		jmp		Done

// gCurrentTask == NULL | save prev task state
2:		movl		%eax, freax(%ebp)
		movl		frtask(%ebp), %edx
										# save & disable IRQ | FIQ
//		stw	spsr, 0x94(r30)	# save PSR in task
										# save all registers
		movl		frebx(%ebp), %eax
		movl		%eax, taskebx(%edx)
		movl		frecx(%ebp), %eax
		movl		%eax, taskecx(%edx)
		movl		fredx(%ebp), %eax
		movl		%eax, taskedx(%edx)
		movl		fresi(%ebp), %eax
		movl		%eax, taskesi(%edx)
		movl		fredi(%ebp), %eax
		movl		%eax, taskedi(%edx)

		leal		frebp(%ebp), %eax
		movl		0(%eax), %eax
		movl		%eax, taskebp(%edx)

		leal		frebp+8(%ebp), %eax
		movl		%eax, taskesp(%edx)
										# restore IRQ | FIQ
		movl		frlk(%ebp), %eax
		movl		%eax, taskeip(%edx)		# save PC
		movl		freax(%ebp), %eax
		jmp		Done

/*  3 ---------------------------------------------------------- */
DoEnterAtomic:
// disable FIQ|IRQ
		incl		_gAtomicNestCount
		cmpl		$1, _gAtomicNestCount
		jne		1f
// save SPSR in gOldStatus
		nop
1:
// re-enable FIQ
		movl		$0, %eax
		jmp		Done

/*  4 ---------------------------------------------------------- */
DoExitAtomic:
		decl		_gAtomicNestCount
		cmpl		$0, _gAtomicNestCount
		jl			2f
		jne		1f
// reset FIQ|IRQ enables from gOldStatus
		nop
1:
		movl		$0, %eax
		jmp		Done
2:
		subl		$12, %esp
		leal		tooManyAtomicExits, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp
3:		nop
		jmp		3b

/*  5 -------------------------------------------------------------
	NewtonErr GenericSWI(int inSelector, ...)
---------------------------------------------------------------- */
DoGeneric:
		movl		_gCurrentTask, %eax
		movl		%eax, frtask(%ebp)			# save gCurrentTask

		call		_GenericSWIHandler	# call back in C

		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		je			2f
// gCurrentTask != NULL
//		movl		taskeax(%edx), %eax		# return fRegister[kResultReg]
		movl		%eax, freax(%ebp)
		movl		freax(%ebp), %eax
		jmp		Done

// gCurrentTask == NULL | save prev task state
2:		movl		%eax, freax(%ebp)
		movl		frtask(%ebp), %edx
										# save & disable IRQ | FIQ
//		stw	spsr, 0x94(r30)	# save PSR in task
										# save all registers
		movl		frebx(%ebp), %eax	# save all registers
		movl		%eax, taskebx(%edx)
		movl		frecx(%ebp), %eax
		movl		%eax, taskecx(%edx)
		movl		fredx(%ebp), %eax
		movl		%eax, taskedx(%edx)
		movl		fresi(%ebp), %eax
		movl		%eax, taskesi(%edx)
		movl		fredi(%ebp), %eax
		movl		%eax, taskedi(%edx)
		movl		%ebp, taskebp(%edx)
		movl		%esp, taskesp(%edx)
										# restore IRQ | FIQ
		movl		frlk(%ebp), %eax
		movl		%eax, taskeip(%edx)		# save PC
		movl		freax(%ebp), %eax
		jmp		Done

/*  6 ---------------------------------------------------------- */
DoGenerateMessageIRQ:
// suspect this is unreachable
		jmp		Done

/*  7 ---------------------------------------------------------- */
DoPurgeMMUTLBEntry:
// don’t know what to do here
		jmp		Done

/*  8 ---------------------------------------------------------- */
DoFlushMMU:
// don’t know what to do here
		jmp		Done

/*  9 ---------------------------------------------------------- */
DoFlushCache:
// don’t know what to do here
		jmp		Done

/* 10 ---------------------------------------------------------- */
DoGetCPUVersion:
// don’t know what to do here
		movl		$0, %eax
		jmp		Done

/* 11 ---------------------------------------------------------- */
DoSemOp:
		movl		_gCurrentTask, %eax
		movl		%eax, frtask(%ebp)

		call		_DoSemaphoreOp	#(ObjectId inGroupId, ObjectId inListId, SemFlags inBlocking, CTask * inTask)

		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		jne		2f

// gCurrentTask == NULL
		movl		frlk(%ebp), %eax
		subl		$4, %eax			# re-run this semop call
		movl		%eax, frlk(%ebp)

		movl		%eax, freax(%ebp)
		movl		frtask(%ebp), %edx
										# save & disable IRQ | FIQ
//		stw	spsr, 0x94(r30)	# save PSR in task
										# save all registers
		movl		frebx(%ebp), %eax	# save all registers
		movl		%eax, taskebx(%edx)
		movl		frecx(%ebp), %eax
		movl		%eax, taskecx(%edx)
		movl		fredx(%ebp), %eax
		movl		%eax, taskedx(%edx)
		movl		fresi(%ebp), %eax
		movl		%eax, taskesi(%edx)
		movl		fredi(%ebp), %eax
		movl		%eax, taskedi(%edx)
		movl		%ebp, taskebp(%edx)
		movl		%esp, taskesp(%edx)
										# restore IRQ | FIQ
		movl		frlk(%ebp), %eax
		movl		%eax, taskeip(%edx)		# save PC
		movl		freax(%ebp), %eax

2:		jmp		Done

/* 12 ---------------------------------------------------------- */
DoSetDomainRegister:
// don’t know what to do here
		jmp		Done

/* 13 ---------------------------------------------------------- */
DoSMemSetBuffer:
		call		_SMemSetBufferKernelGlue
		jmp		Done

/* 14 ---------------------------------------------------------- */
DoSMemGetSize:
		call		_SMemGetSizeKernelGlue	#(ObjectId inId)
/*
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		je			2f
		movl		%eax, freax(%ebp)
// gCurrentTask != NULL
// copy return values r1-r3
		movl		taskr1(%edx), %eax		# size
		movl		%eax, frret1(%ebp)
		movl		taskr2(%edx), %eax		# address
		movl		%eax, frret2(%ebp)
		movl		taskr3(%edx), %eax		# refCon
		movl		%eax, frret3(%ebp)
		movl		freax(%ebp), %eax
*/
2:		jmp		Done

/* 15 ---------------------------------------------------------- */
DoSMemCopyToShared:
		movl		_gCurrentTask, %edx
		movl		frlk(%ebp), %eax
		movl		%eax, taskeip(%edx)	# save PC

		call		_SMemCopyToKernelGlue	#(ObjectId inId, void * inBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)

		movl		%eax, freax(%ebp)
// copy return values r1-r3
		movl		_gCurrentTask, %edx
		movl		taskr1(%edx), %eax		# to
		movl		%eax, fredi(%ebp)
		movl		taskr2(%edx), %eax		# from
		movl		%eax, fresi(%ebp)
		movl		taskr3(%edx), %eax		# size
		movl		%eax, frecx(%ebp)
		movl		freax(%ebp), %eax
		cmpl		$1, %eax
		je			1f
		cmpl		$4, %eax
		je			4f
//		mr		r4, r5
		jmp		Done

1:		// return to copy engine
		leal		LowLevelCopyEngine, %edx
		movl		%edx, frlk(%ebp)			// overwrite return address on stack
		jmp		Done

4:		// return to copy engine
		leal		LowLevelCopyEngineLong, %edx
		movl		%edx, frlk(%ebp)
		jmp		Done

/* 16 ---------------------------------------------------------- */
DoSMemCopyFromShared:
		movl		_gCurrentTask, %edx
		movl		frlk(%ebp), %eax
		movl		%eax, taskeip(%edx)	# save PC

		call		_SMemCopyFromKernelGlue	#(ObjectId inId, void * outBuffer, size_t inSize, ULong inOffset, ObjectId inSendersMsgId, ULong inSendersSignature)

		movl		%eax, freax(%ebp)
// copy return values r1-r3
		movl		_gCurrentTask, %edx
		movl		taskr1(%edx), %eax		# to
		movl		%eax, fredi(%ebp)
		movl		taskr2(%edx), %eax		# from
		movl		%eax, fresi(%ebp)
		movl		taskr3(%edx), %eax		# size
		movl		%eax, frecx(%ebp)
		movl		freax(%ebp), %eax
		cmpl		$1, %eax
		je			1f
		cmpl		$4, %eax
		je			4f
//		mr		r4, r5
		jmp		Done

1:		// return to copy engine
		leal		LowLevelCopyEngine, %edx
		movl		%edx, frlk(%ebp)			// overwrite return address on stack
		jmp		Done

4:		// return to copy engine
		leal		LowLevelCopyEngineLong, %edx
		movl		%edx, frlk(%ebp)
		jmp		Done

/* 17 ---------------------------------------------------------- */
DoSMemMsgSetTimerParms:
		call		_SMemMsgSetTimerParmsKernelGlue
		jmp		Done

/* 18 ---------------------------------------------------------- */
DoSMemMsgSetMsgAvailPort:
		call		_SMemMsgSetMsgAvailPortKernelGlue
		jmp		Done

/* 19 ---------------------------------------------------------- */
DoSMemMsgGetSenderTaskId:
		call		_SMemMsgGetSenderTaskIdKernelGlue
/*
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		je			2f
		movl		%eax, freax(%ebp)
// copy return values r1-r3
		movl		taskr1(%edx), %eax
		movl		%eax, frret1(%ebp)
		movl		taskr2(%edx), %eax
		movl		%eax, frret2(%ebp)
		movl		taskr3(%edx), %eax
		movl		%eax, frret3(%ebp)
		movl		freax(%ebp), %eax
*/
2:		jmp		Done

/* 20 ---------------------------------------------------------- */
DoSMemMsgSetUserRefCon:
		call		_SMemMsgSetUserRefConKernelGlue
		jmp		Done

/* 21 ---------------------------------------------------------- */
DoSMemMsgGetUserRefCon:
		call		_SMemMsgGetUserRefConKernelGlue
/*
		movl		_gCurrentTask, %edx
		cmpl		$0, %edx
		je			2f
		movl		%eax, freax(%ebp)
// copy return values r1-r3
		movl		taskr1(%edx), %eax
		movl		%eax, frret1(%ebp)
		movl		taskr2(%edx), %eax
		movl		%eax, frret2(%ebp)
		movl		taskr3(%edx), %eax
		movl		%eax, frret3(%ebp)
		movl		freax(%ebp), %eax
*/
2:		jmp		Done

/* 22 ---------------------------------------------------------- */
DoSMemMsgCheckForDone:
		movl		_gCurrentTask, %eax
		movl		%eax, frtask(%ebp)

		call		_SMemMsgCheckForDoneKernelGlue	#(ObjectId inId, ULong inFlags)

		jmp		Received

/* 23 ---------------------------------------------------------- */
DoSMemMsgMsgDone:
		call		_SMemMsgMsgDoneKernelGlue
		jmp		Done

/* 24 ---------------------------------------------------------- */
DoTurnOffCache:
// don’t know what to do here
		jmp		Done

/* 25 ---------------------------------------------------------- */
DoTurnOnCache:
// don’t know what to do here
		jmp		Done

/* 26 ---------------------------------------------------------- */
DoLowLevelCopyDone:
		pushl		$0
		pushl		frlk(%ebp)
		pushl		_gCurrentTask
		pushl		$0
		call		_LowLevelCopyDoneFromKernelGlue	#(NewtonErr inErr, CTask * inTask, VAddr inReturn)
		addl		$16, %esp
		jmp		Done

/* 27 ---------------------------------------------------------- */
DoMonitorDispatch:
		movl		_gCurrentTask, %edx
											# save & disable IRQ | FIQ
											# save PSR in task
											# save all registers
		movl		frarg1(%ebp), %eax
		movl		%eax, taskarg1(%edx)
		movl		frarg2(%ebp), %eax
		movl		%eax, taskarg2(%edx)
		movl		frarg3(%ebp), %eax
		movl		%eax, taskarg3(%edx)
		movl		frarg4(%ebp), %eax
		movl		%eax, taskarg4(%edx)

		movl		frebx(%ebp), %eax
		movl		%eax, taskebx(%edx)
		movl		frecx(%ebp), %eax
		movl		%eax, taskecx(%edx)
		movl		fredx(%ebp), %eax
		movl		%eax, taskedx(%edx)
		movl		fresi(%ebp), %eax
		movl		%eax, taskesi(%edx)
		movl		fredi(%ebp), %eax
		movl		%eax, taskedi(%edx)
		movl		frebp(%ebp), %eax
		movl		%eax, taskebp(%edx)
		leal		8(%ebp), %eax			# <-- assume stack is always aligned with frame
		movl		%eax, taskesp(%edx)
											# restore IRQ | FIQ
		movl		frlk(%ebp), %eax
		movl		%eax, taskeip(%edx)	# save PC

		call		_MonitorDispatchKernelGlue	#(void)
		jmp		Done

/* 28 ---------------------------------------------------------- */
DoMonitorExit:
		call		_MonitorExitKernelGlue
		jmp		Done

/* 29 ---------------------------------------------------------- */
DoMonitorThrow:
		call		_MonitorThrowKernelGlue
		jmp		Done

/* 30 ---------------------------------------------------------- */
DoEnterFIQAtomic:
// disable FIQ | IRQ
		incl		_gAtomicFIQNestCount
		cmpl		$1, _gAtomicFIQNestCount
		jne		1f
// gAtomicFIQNestCount == 1
		nop							# save status bits into gOldFIQStatus
1:
		movl		$0, %eax
		jmp		Done

/* 31 ---------------------------------------------------------- */
DoExitFIQAtomic:
		decl		_gAtomicFIQNestCount
		cmpl		$0, _gAtomicFIQNestCount
		jl			2f
		jne		1f
// gAtomicFIQNestCount == 0
		nop							# restore status bits
1:
		movl		$0, %eax
		jmp		Done
2:
		subl		$12, %esp
		leal		tooManyFIQExits, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp
3:		nop
		jmp		3b

/* 32 ---------------------------------------------------------- */
DoMonitorFlush:
		call		_MonitorFlushKernelGlue
		jmp		Done

/* 33 ---------------------------------------------------------- */
DoPortResetFilter:
		call		_PortResetFilterKernelGlue
		jmp		Done

/* 34 ---------------------------------------------------------- */
DoScheduler:
		movl		$0, %eax					// return noErr
		jmp		Done

/* 35.. -------------------------------------------------------- */
UndefinedSWI:
		subl		$12, %esp
		leal		undefinedSWI, %eax
		pushl		%eax
		call		_DebugStr$stub
		addl		$16, %esp
		movl		$-1, %eax				// return non-specific error
		jmp		Done


/* ----------------------------------------------------------------
	void
	LowLevelCopyEngine(char * inTo, char * inFrom, size_t inSize)

	Copy memory, char-at-a-time.
	Args:		 %edi		inTo
				 %esi		inFrom
				 %ecx		inSize
	Return	--
---------------------------------------------------------------- */

LowLevelCopyEngine:
		rep movsb

		movl		$26, %eax
		call		SWI
		ret


/* ----------------------------------------------------------------
	void
	LowLevelCopyEngineLong(char * inTo, char * inFrom, size_t inSize)

	Copy memory, long-at-a-time.
	Args:		 %edi		inTo
				 %esi		inFrom
				 %ecx		inSize
	Return	--
---------------------------------------------------------------- */

LowLevelCopyEngineLong:
		shr		$2, %ecx
		rep movsd

		movl		$26, %eax
		call		SWI
		ret


/* ----------------------------------------------------------------
	S T U B S
---------------------------------------------------------------- */

		.section __IMPORT,__jump_table,symbol_stubs,self_modifying_code+pure_instructions,5

_DebugStr$stub:
		.indirect_symbol _DebugStr
		hlt ; hlt ; hlt ; hlt ; hlt

