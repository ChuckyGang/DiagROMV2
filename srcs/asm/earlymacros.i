
VER:	MACRO
	dc.b "2"			; Versionnumber
	ENDM
REV:	MACRO
	dc.b "0 ALFA"			; Revisionmumber
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

PAUSE:		MACRO
.waitdown\@	move.b	$dff006,$dff181
		btst	#6,$bfe001
		bne.s	.waitdown\@
.waitup\@	btst	#6,$bfe001
		beq.s	.waitup\@
		ENDM

PAUSE2:	MACRO
.waitdown\@	move.b	$dff007,$dff181
		btst	#6,$bfe001
		bne.s	.waitdown\@
.waitup\@	btst	#6,$bfe001
		beq.s	.waitup\@
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

KPRINT9600: MACRO 
		; print a contstant from a pointer
		lea	\1,a0		
		lea	.return\@,a5
		bra	DumpSerial9600
.return\@:
		ENDM 

KPRINTC: MACRO 
		; print a contstant from a pointer
		lea	\1,a0		
		lea	.return\@,a5
		bra	DumpSerial		
.return\@:
		ENDM 

KPRINTLONG:	MACRO
		; print out the longword of D0
		move.l	d0,d5
		asr.l	#8,d0
		asr.l	#8,d0
		asr.l	#8,d0
		lea	.return1\@,a5
		bra	DumpBinHex
.return1\@:
		KPRINT
		move.l	d5,d0
		asr.l	#8,d0
		asr.l	#8,d0
		lea	.return2\@,a5
		bra	DumpBinHex
.return2\@:
		KPRINT
		move.l	d5,d0
		asr.l	#8,d0
		lea	.return3\@,a5
		bra	DumpBinHex
.return3\@:
		KPRINT
		move.l	d5,d0
		lea	.return4\@,a5
		bra	DumpBinHex
.return4\@:
		KPRINT
		ENDM


