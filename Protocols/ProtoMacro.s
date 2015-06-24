/*
	File:		ProtoMacro.s

	Contains:	P-class protocol macro glue.
					These are CProtocol sub-class functions.

	Written by:	Newton Research Group, 2009.
*/

#if __LP64__
/* ----------------------------------------------------------------
	Firstly, for protocol dispatch.
	In LP64 ABI
		args are in				%rdi %rsi %rdx %rcx %r8 %r9
		return value is in	%rax
---------------------------------------------------------------- */

.macro New
		pushq		%rbp						# prolog
		movq		%rsp, %rbp
		movq		%rdi, %rsi				# implementation name is 2nd arg to AllocInstanceByName
		leaq		$0(%rip), %rdi			# interface name is 1st arg
		call		__Z19AllocInstanceByNamePKcS0_
		leave
		cmp		$(0), %rax				# success?
		jne		1f
		ret
1:		movq		%rax, %rdi				# replace impl name w/ instance; we fall thru to Dispatch
.endmacro


.macro Delete
		pushq		%rbp						# prolog
		movq		%rsp, %rbp
		movl		$($0), %eax
		movq		8(%rdi), %rdi			# rdi -> "real" this
		pushq		%rdi						# save instance
		subq		$(8), %rsp				# keep stack 16-byte aligned
		movq		16(%rdi), %r10			# r10 -> btable
		movl		0(%r10,%rax,4), %eax	# fetch address of dispatch branch
		addq		%r10, %rax
		call		*%rax						# call it
		addq		$(8), %rsp				# realign stack
		popq		%rdi
		leave
		jmp		__Z12FreeInstanceP9CProtocol	# tail call to free mem
.endmacro


.macro Dispatch
		movl		$($0), %eax
		movq		8(%rdi), %r10			# r10 -> "real" this
		movq		16(%r10), %r10			# r10 -> btable
		movl		0(%r10,%rax,4), %eax	# point to dispatch fn
		addq		%r10, %rax
		jmp		*%rax						# go there
.endmacro

/* ----------------------------------------------------------------
	Secondly, for monitors.
---------------------------------------------------------------- */

.macro MonitorGlue
MonCall:										# rdi = this
		movq		8(%rdi), %r10			# r10 -> "real" this
		movq		16(%r10), %r10			# r10 -> btable
		movl		8(%r10,%rax,4), %eax	# point to dispatch fn
		addq		%r10, %rax
		jmp		*%rax						# go there
												# should go via kMonitorDispatchSWI
												# and catch return errors:
0:		leaq		1f(%rip), %rdi
		movl		$(0), %esi
		movl		$(0), %edx
		call		_Throw
		.align	2
1:		.asciz	"evt.ex.moncall"
		.align	2
.endmacro


.macro MonDelete
		movl		$($0), %eax
		call		MonCall
		jmp		__ZN9CProtocol14destroyMonitorEv
.endmacro


.macro MonDispatch
		movl		$($0), %eax
		jmp		MonCall
.endmacro


#else
/* ----------------------------------------------------------------
	Firstly, for protocol dispatch.
---------------------------------------------------------------- */

.macro BRAZ
		.long		$0 - 0b
.endmacro

.macro BRA
		.long		$0 - 4b
.endmacro

.macro NOBRA
		.long		0
.endmacro


.macro RetNil
		movl		$(0), %eax
		ret
.endmacro


.macro ItIsHere
		leal		9f, %eax
		ret
		.align	2
9:		
.endmacro


.macro New
		pushl		%ebp
		movl		%esp, %ebp
		subl		$(8), %esp				# keep stack 16-byte aligned
		movl		8(%ebp), %eax			# get implementation name off the stack
		pushl		%eax
		call		$0							# interface name
		pushl		%eax
		call		__Z19AllocInstanceByNamePKcS0_
		leave
		cmp		$(0), %eax				# success?
		jne		1f
		ret
1:		movl		%eax, 4(%esp)			# replace impl name w/ instance; we fall thru to Dispatch
.endmacro


.macro Delete
		pushl		%ebp
		movl		%esp, %ebp
		subl		$(8), %esp				# keep stack 16-byte aligned
		movl		8(%ebp), %eax			# get this off the stack
		movl		4(%eax), %eax			# eax -> "real" this
		pushl		%eax
		movl		8(%eax), %eax			# eax -> btable
		movl		$($0), %edx
		addl		0(%eax,%edx,4), %eax	# fetch address of dispatch branch
		call		*%eax						# call it
		leave
		jmp		__Z12FreeInstanceP9CProtocol	# tail call to free mem
.endmacro


.macro Dispatch
		movl		4(%esp), %eax			# get this off the stack
		movl		4(%eax), %eax			# eax -> "real" this
		movl		8(%eax), %eax			# eax -> btable
		movl		$($0), %edx
		addl		0(%eax,%edx,4), %eax	# fetch address of dispatch branch
		jmp		*%eax						# go there
.endmacro

/* ----------------------------------------------------------------
	Secondly, for monitors.
---------------------------------------------------------------- */

.macro MonitorGlue
MonCall:
		movl		4(%esp), %eax			# get this off the stack
		movl		4(%eax), %eax			# eax -> "real" this
		movl		8(%eax), %eax			# eax -> btable
		addl		8(%eax,%edx,4), %eax	# fetch address of dispatch branch
		jmp		*%eax						# go there
												# should go via kMonitorDispatchSWI
												# and catch return errors:
		call		0f
0:		pop		%ebx
		leal		1f-0b(%ebx), %eax
		pushl		$(0)
		pushl		$(0)
		pushl		%eax
		call		_Throw
		.align	2
1:		.asciz	"evt.ex.moncall"
		.align	2
.endmacro


.macro MonDelete
		movl		$($0), %edx
		call		MonCall
		jmp		__ZN9CProtocol14destroyMonitorEv
.endmacro


.macro MonDispatch
		movl		$($0), %edx
		jmp		MonCall
.endmacro

#endif
