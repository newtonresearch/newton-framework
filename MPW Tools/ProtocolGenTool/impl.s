
; ----------------------------------------------------------------

			EXPORT	|ClassInfo__%sSFv|
			IMPORT	|Sizeof__%sSFv|
			IMPORT	|OperatorNew__%sSFv|

			IMPORT	|%s|


; TClassInfo for '%s' starts here:

local__ClassInfo__%sSFv
			DCD		0							; (reserved for now)
			DCD		|__implname__%s| - .		; SRO to implementation name
			DCD		|__protname__%s| - .		; SRO to protocol name
			DCD		|__cap__%s| - .				; SRO to capability list
			DCD		|__btbl__%s| - .			; SRO to btable
			DCD		|__monent__%s| - .			; SRO to monitor entry proc
			B		|Sizeof__%sSFv|				; branch to sizeof glue
			B		|OperatorNew__%sSFv|	; zero, or branch to Alloc method
			DCD		0							; zero, or branch to Alloc method
			B		|OperatorDelete__%sSFv|	; zero, or branch to Free method
			DCD		0							; zero, or branch to Free method
			B		|New__%sFv|		; return, or branch to New(void)
			MOV		PC,R14						; return, or branch to New(void)
			B		|Delete__%sFv|		; return, or branch to Delete
			MOV		PC,R14						; return, or branch to Delete(void)
			DCD		&%08lX						; version number
			DCD		&%08lX						; flags
			DCD		0							; (reserved)
			B		return__nil__%s				; (reserved)

ClassInfo__%sSFv
			ADR		R0,local__ClassInfo__%sSFv
			MOV		PC,R14						;

return__nil__%s
			MOV		R0,#0						;
			MOV		PC,R14						;

|__implname__%s|	DCB	"%s",0
|__protname__%s|	DCB	"%s",0
|__cap__%s|
			DCB 0

			ALIGN	4

|__btbl__%s|
			DCD		0
			B		ClassInfo__%sSFv
			B		|%s|

__monent__%d%s
; ----------------------------------------------------------------

			EXPORT	|%s|

asciz__%s
			MOV		R0,PC				; R0 -> asciz name
			MOV		PC,R14				; begone!
			DCB		"%s",0
			ALIGN	4

