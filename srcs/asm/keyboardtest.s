       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "keyboardtest",code_p
       xref   KeyBoardTest

KeyBoardTest:
	jsr	InitScreen
	lea	KeyBoardTestText,a0
	move.l	#7,d1
	jsr	Print
	lea	KeyBoardTestCodeTxt,a0
	move.l	#6,d1
	jsr	Print
	lea	KeyBoardTestCodeTxt2,a0
	move.l	#6,d1
	jsr	Print
	move.b	#0,KeyBOld(a6)

.loop:
	jsr	GetInput
	jsr	WaitShort			; just wait a short time
	move.b	scancode(a6),d0
	cmp.b	#0,d0
	beq	.null				; If scancode was 0 we had noting

	move.b	KeyBOld(a6),d1
	cmp.b	d0,d1				; Check if it is the same as last scan
	beq	.samecode
	move.b	d0,KeyBOld(a6)


	move.l	#43,d0
	move.l	#2,d1
	jsr	SetPos
	lea	Space3,a0
	jsr	Print
	move.l	#43,d0
	move.l	#2,d1
	jsr	SetPos
	clr.l	d0
	move.b	scancode(a6),d0
	cmp.b	#116,d0				; is it 116? (esc released)
	beq	.exit
	jsr	bindec
	move.l	#3,d1
	jsr	Print

	move.l	#17,d0
	move.l	#3,d1
	jsr	SetPos

	move.b	scancode(a6),d0
	jsr	binstringbyte
	move.l	#3,d1
	jsr	Print

	move.l	#32,d0
	move.l	#3,d1
	jsr	SetPos

	move.b	scancode(a6),d0
	move.l	#3,d1
	jsr	binhexbyte
	jsr	Print
	
	move.l	#62,d0
	move.l	#2,d1
	jsr	SetPos

	clr.l	d0
	move.b	key(a6),d0
	jsr	bindec
	move.l	#3,d1
	jsr	Print

	move.l	#58,d0
	move.l	#3,d1
	jsr	SetPos

	move.b	key(a6),d0
	jsr	binstringbyte
	move.l	#3,d1
	jsr	Print

	move.l	#73,d0
	move.l	#3,d1
	jsr	SetPos

	move.b	key(a6),d0
	jsr	binhexbyte
	move.l	#3,d1
	jsr	Print


	move.l	#73,d0
	move.l	#2,d1
	jsr	SetPos
	move.l	#2,d1

	lea	keyresult(a6),a0
	move.b	(a0),d0
	cmp.b	#0,d0				; Check if it was no char, then do not print
	beq	.samecode
	move.l	#3,d1
	jsr	MakePrintable
	jsr	PrintChar
.null:
.samecode:
	cmp.b	#$1b,Serial(a6)
	beq	.exit
	cmp.b	#1,MBUTTON(a6)
	bne.w	.loop
.exit:

	jmp	MainMenu

