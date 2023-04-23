
VER:	MACRO
	dc.b "2"			; Versionnumber
	ENDM
REV:	MACRO
	dc.b "0 BETA"			; Revisionmumber
	ENDM

VERSION:	MACRO
	dc.b	"V"			; Generates versionstring.
	VER
	dc.b	"."
	REV
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

DBINHEX: MACRO
		; A0 points to string of content of D0 (byte)
		lea .return\@,a5
		bra DumpBinHex
.return\@:
		ENDM

DBINDEC: MACRO
		; A0 points to string of content of D0 (byte)
		lea .return\@,a5
		bra DumpBinDec
.return\@:
		ENDM

KPRINT:	MACRO
		; Dump to serialport what A0 points to
		lea	.return\@,a5
		bra	DumpSerial
.return\@:
		ENDM

KPRINTC: MACRO 
		; print a contstant from a pointer
		lea	\1,a0		
		lea	.return\@,a5
		bra	DumpSerial		
.return\@:
		ENDM 
