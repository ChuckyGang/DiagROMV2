
VER:	MACRO
	dc.b "1"			; Versionnumber
	ENDM
REV:	MACRO
	dc.b "2.1"			; Revisionmumber
	ENDM

VERSION:	MACRO
	dc.b	"V"			; Generates versionstring.
	VER
	dc.b	"."
	REV
	ENDM

EDITION:	MACRO
;	dc.b	" - Revision Edition"
	ENDM

PUSH:	MACRO
	movem.l a0-a6/d0-d7,-(a7)	;Store all registers in the stack
	ENDM

POP:	MACRO
	movem.l (a7)+,a0-a6/d0-d7	;Restore the registers from the stack
	ENDM

TOGGLEPWRLED: MACRO
	bchg	#1,$bfe001
	ENDM
	
PAROUT: MACRO
	move.b	\1,$bfe101
	ENDM
	
VBLT:		MACRO
.vblt\@		btst	#14,$dff002
		bne.s	.vblt\@
		ENDM


KPRINTSTR:	MACRO
		; print a string passed as a paramter
		lea	.txt\@,a0		
		lea	.return\@,a5
		bra	DumpSerial		
.txt\@: 
		dc.b \1, 
		EVEN
.return\@:
		ENDM 

KPRINTC: MACRO 
		; print a contstant from a pointer
		lea	\1,a0		
		lea	.return\@,a5
		bra	DumpSerial		
.return\@:
		ENDM 
