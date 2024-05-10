       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "terriblefire",code_p
       xref   TF1260

TF1260:						; Some TF360/TF1260 Diag tests
	cmp.b	#0,AutoConfDone(a6)		; Check if we had autoconfig done.
	bne	.done
	jsr	ClearScreen
	lea	TF1260Txt,a0
	move.l	#6,d1
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	TF1260AutoConfNotTxt,a0
	move.l	#1,d1
	jsr	Print
	move.b	#0,AutoConfMode(a6)		; Set that we do not want a more detailed autoconfig mode
	jsr	DoAutoconfig
.done:						; Yes.. autoconfig is done..
	jsr	ClearScreen
	clr.l	TF1260IOStart(a6)
	clr.l	TF1260IOEnd(a6)
	clr.l	TF1260MemStart(a6)
	clr.l	TF1260MemEnd(a6)
	lea	TF1260Txt,a0
	move.l	#6,d1
	jsr	Print
	jsr	PrintCPU
	lea	FlagTxt,a0
	jsr	Print
	lea	PCRFlagsTxt,a0
	move.l	#2,d1
	jsr	Print
	move.l	PCRReg(a6),d0
	jsr	binstring
	move.l	#3,d1
	jsr	Print
	lea	AutoConfList(a6),a1
	move.l	AutoConfBoards(a6),d7
	sub.l	#1,d7
.loop:
	cmp.w	#5080,(a1)			; Check if we have correct manufacurer no.
	bne	.notright
						; ok we found one entry that seems correct
	clr.l	d0
	move.b	5(a1),d0			; Read flags to d0
	btst	#5,d0
	beq	.io
	move.l	6(a1),TF1260MemStart(a6)
	move.l	10(a1),TF1260MemEnd(a6)
	bra	.notright
.io:
	move.l	6(a1),TF1260IOStart(a6)
	move.l	10(a1),TF1260IOEnd(a6)
.notright:
	add.l	#14,a1				; go to nextblock in list
	dbf	d7,.loop			; loop through all cards.
	lea	TF1260ControllerTxt,a0
	move.l	#3,d1
	jsr	Print
	cmp.l	#0,TF1260IOStart(a6)
	bne	.TFCont
	lea	NOT,a0
	move.l	#1,d1
	jsr	Print
	lea	DDETECTED,a0
	jsr	Print
	bra	.CheckMem
.TFCont:
	lea	DETECTEDTxt,a0
	move.l	#2,d1
	jsr	Print
	lea	SpaceTxt,a0
	jsr	Print
	move.l	TF1260IOStart(a6),d0
	jsr	binhex
	move.l	#3,d1
	jsr	Print
	lea	MinusTxt,a0
	jsr	Print
	move.l	TF1260IOEnd(a6),d0
	jsr	binhex
	move.l	#3,d1
	jsr	Print
.CheckMem:
	lea	TF1260MemTxt,a0
	move.l	#3,d1
	jsr	Print
	cmp.l	#0,TF1260MemStart(a6)
	bne	.TFram
	lea	NOT,a0
	move.l	#1,d1
	jsr	Print
	lea	DDETECTED,a0
	jsr	Print
	bra	.Ramdone
.TFram:
	lea	DETECTEDTxt,a0
	move.l	#2,d1
	jsr	Print
	lea	SpaceTxt,a0
	jsr	Print
	move.l	TF1260MemStart(a6),d0
	jsr	binhex
	move.l	#3,d1
	jsr	Print
	lea	MinusTxt,a0
	jsr	Print
	move.l	TF1260MemEnd(a6),d0
	jsr	binhex
	move.l	#3,d1
	jsr	Print
.Ramdone:
	cmp.l	#0,TF1260IOStart(a6)
	bne	.ContFound

	lea	TF1260NotTxt,a0
	move.l	#1,d0
	jsr	Print
	bra	.loopa
.ContFound:
.loopa:
	jsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne	.loopa
;	jsr	WaitButton
	bra	OtherTest
