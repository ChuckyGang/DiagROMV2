; Amiga-specific assembly macros

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
