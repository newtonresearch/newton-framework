			MACRO
$label		MonitorEntry $maxMeths, $mtable
			TST		R1,R1					; range-check selector
			BMI		%%F1						; 
			CMP		R1,#$maxMeths			; 
			BGE		%%F1				; 
			STMFD	R13!,{R2,R14}			; save result-pointer, LK
			LDR		R0,[R0,#4]				; R0 -> true object
			ADD		R12,PC,#$mtable-.-8		; R12 -> entry btable
			ADD		R12,R12,R1,LSL #3		; R12 -> method's <argcopy/entry> pair
			MOV		R14,PC					; arrange to return at LDMEA below
;		--- don't put anything here
			MOV		PC,R12					; call argcopy, then call method
;		--- don't put anything here
			ADD		R13,R13,#8*4			; assumes 8 stack args
			LDMIA	R13!,{R2,R14}			; restore result-pointer, LK
			STR		R0,[R2]					; store result
			MOV		R0,#0					; return success
			MOV		PC,R14					; return from monitor

1			MOV		R0,#-1					; error
			MOV		PC,R14					; 
			MEND

			MonitorEntry %d,__mbt__%s

__mbt__%s
			B		|argcopy_default|
			B		|%s|

argcopy_default
			MOV		R3,#8					; (assume 8 words of stack)

CopyNArgs	SUB		R13,R13,R3,LSL #2		; make room for stack args
			ADD		R2,R2,#16				; R2 -> stacked args
			B		%%F2					;
1			LDR		R1,[R2,R3,LSL #2]		; copy an argument
			STR		R1,[R13,R3,LSL #2]		;   in a rather
2			SUBS	R3,R3,#1				;     slow loop
			BPL		%%B1					;
			SUB		R2,R2,#12				; R2 -> register args
			LDMIA	R2,{R1,R2,R3}			; load register args
0			ADD		PC,R12,#4				; jump to branch to method
 
;+
;  Cheezy Monitor Glue
;
;	Arg pushing is not optimized.  Non-32-bit arguments or results are not
;	supported.  [All in the name of Alpha].
;
;-

kReserved	EQU		0						; reserved, zero in true object
kThisPtr	EQU		4						; -> true "this"
kBTablePtr	EQU		8						; -> BTable to use
kObjID		EQU		12						; kernel object id, or zero


kMonitorDispatchSWI	EQU	27

			IMPORT	Throw					; somewhere in ROM
			IMPORT	|DestroyMonitor__9TProtocolFv|

;	----------------------------------------------------------------
;
;	Monitor glue dispatch
;
;	On entry:	R0		-> object
;				R12		 = selector
;				SP		-> stacked arguments
;
MonitorDispatch
			LDR		R0,[R0,#kObjID]			; R0 = monitor ID
			MOV		R1,R12					; R1 = method selector
			SUB		R13,R13,#4				; make room for result
			MOV		R2,R13					; R2 -> result, stacked arguments
			SWI		kMonitorDispatchSWI		; call monitor (SWI 27)
			TST		R0,R0					; did monitor call succeed?
			BNE		MonitorCallException	; no, generate an exception
			LDR		R0,[R13]				; R0 = result from method call
			ADD		R13,R13,#16				; clean up stuff we pushed
0			MOV		PC,R14					;  and boogie back to client

MonitorCallException
			MOV		R1,#0					; Throw("evt.ex.moncall", 0, 0)
			MOV		R2,#0					; 
			MOV		R0,PC					; 
;		--- don't put anything here			; 
			B		Throw					; 
;		--- don't put anything here			; 
			DCB		"evt.ex.moncall",0		; 
			ALIGN	4						; 

;+
;  Macro for monitor dispatch (calls congealed glue above)
;
;-
			MACRO
$label		CallMonitor $selector
			STMFD	R13!,{R1-R3}			; push three reg args
			MOV		R12,#$selector			; R12 = selector
			B		MonitorDispatch			; go dispatch
			MEND

;+
;  Macro to murder a monitor
;
;-
			MACRO
$label		DeleteMonitor	$selector
			STMFD	R13!,{R14}				; push LK
			STMFD	R13!,{R1-R3}			; push three reg args
			MOV		R12,#$selector			; R12 = selector
			BL		MonitorDispatch			; go dispatch
			LDMFD	R13!,{R14}				; restore LK
			B		DestroyMonitor__9TProtocolFv	; destroy monitor coming back
			MEND

; ----------------------------------------------------------------

asciz__%s
			MOV		R0,PC				; R0 -> asciz name
			MOV		PC,R14				; begone!
			DCB		"%s",0
			ALIGN	4

			EXPORT	|%s|
%s
			New			asciz__%s
			MOV			PC,R14
			DeleteMonitor %d
			CallMonitor %d
