       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "irqcia",code_p
       xdef   IRQCIAtestMenu
       xdef   IRQCIAIRQTest
       xdef   IRQCIACIATest
       xdef   IRQCIATest
	xdef	RTEcode
	xdef	CIALevTst

CIATIME	EQU	174
;	equ	174			(10000ms / 1.3968255 for PAL)

IRQCIAtestMenu:
	bsr	InitScreen
	move.w	#4,MenuNumber(a6)
	move.b	#1,PrintMenuFlag(a6)
	bra	MainLoop

IRQCIAIRQTest:
	bsr	InitScreen
	lea	IRQCIAIRQTestText,a0
	move.w	#2,d1
	bsr	Print
.loop:
	bsr	GetInput
	clr.w	IRQLev7(a6)
	cmp.b	#$1b,GetCharData(a6)
	beq	.exit
	cmp.b	#1,RMB(a6)
	beq	.exit
	cmp.b	#1,BUTTON(a6)
	bne	.loop
	lea	IRQCIAIRQTestText2,a0
	move.w	#7,d1
	bsr	Print
	bsr	WaitReleased
	move.w	#$2000,sr			; Set SR to allow IRQs
	lea	IRQLev1Txt,a0
	move.l	#6,d1
	bsr	Print
	move.l	#IRQLevTest,d0
	move.l	d0,$64			; Set up IRQ Level 1
	clr.w	IRQLevDone(a6)		; Clear variable, we let the IRQ set it. if it gets set. we have working IRQ
	move.w	#$9000,$dff09c
	move.w	#$c004,$dff09a			; Enable IRQ
	move.w	#$c004,$dff09a			; Enable IRQ
	move.w	#$8004,$dff09c			; Trigger IRQ
	bsr	TestIRQ
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	cmp.b	#2,d0
	bne	.done1
	bsr	WaitReleased
.done1:
	lea	NewLineTxt,a0
	bsr	Print
	lea	IRQLev2Txt,a0
	move.l	#6,d1
	bsr	Print
	clr.w	IRQLevDone(a6)		; Clear variable, we let the IRQ set it. if it gets set. we have working IRQ
	move.l	#IRQLevTest,$68			; Set up IRQ Level 2
	move.w	#$c008,$dff09a			; Enable IRQ
	move.w	#$c008,$dff09a			; Enable IRQ
	move.w	#$8008,$dff09c			; Trigger IRQ
	bsr	TestIRQ
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	cmp.b	#2,d0
	beq	.done2
	bsr	WaitReleased
.done2:
	lea	NewLineTxt,a0
	bsr	Print
	lea	IRQLev3Txt,a0
	move.l	#6,d1
	bsr	Print
	clr.w	IRQLevDone(a6)		; Clear variable, we let the IRQ set it. if it gets set. we have working IRQ
	move.l	#IRQLevTest,$6c			; Set up IRQ Level 3
	move.w	#$c020,$dff09a			; Enable IRQ
	move.w	#$c020,$dff09a			; Enable IRQ
	move.w	#$8020,$dff09c			; Trigger IRQ
	bsr	TestIRQ
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	cmp.b	#2,d0
	beq	.done3
	bsr	WaitReleased
.done3:
	lea	NewLineTxt,a0
	bsr	Print
	lea	IRQLev4Txt,a0
	move.l	#6,d1
	bsr	Print
	clr.w	IRQLevDone(a6)		; Clear variable, we let the IRQ set it. if it gets set. we have working IRQ
	move.l	#IRQLevTest,$70			; Set up IRQ Level 4
	move.w	#$c080,$dff09a			; Enable IRQ
	move.w	#$c080,$dff09a			; Enable IRQ
	move.w	#$8080,$dff09c			; Trigger IRQ
	bsr	TestIRQ
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	cmp.b	#2,d0
	beq	.done4
	bsr	WaitReleased
.done4:
	lea	NewLineTxt,a0
	bsr	Print
	lea	IRQLev5Txt,a0
	move.l	#6,d1
	bsr	Print
	clr.w	IRQLevDone(a6)		; Clear variable, we let the IRQ set it. if it gets set. we have working IRQ
	move.l	#IRQLevTest,$74			; Set up IRQ Level 5
	move.w	#$c800,$dff09a			; Enable IRQ
	move.w	#$c800,$dff09a			; Enable IRQ
	move.w	#$8800,$dff09c			; Trigger IRQ
	bsr	TestIRQ
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	cmp.b	#2,d0
	beq	.done5
	bsr	WaitReleased
.done5:
	lea	NewLineTxt,a0
	bsr	Print
	lea	IRQLev6Txt,a0
	move.l	#6,d1
	bsr	Print
	clr.w	IRQLevDone(a6)		; Clear variable, we let the IRQ set it. if it gets set. we have working IRQ
	move.l	#IRQLevTest,$78			; Set up IRQ Level 6
	move.w	#$e000,$dff09a			; Enable IRQ
	move.w	#$e000,$dff09a			; Enable IRQ
	move.w	#$a000,$dff09c			; Trigger IRQ
	bsr	TestIRQ
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	cmp.b	#2,d0
	beq	.done6
	bsr	WaitReleased
.done6:
	lea	NewLineTxt,a0
	bsr	Print
	lea	IRQLev7Txt,a0
	move.l	#6,d1
	bsr	Print
	move.w	#1,IRQLev7(a6)
	clr.w	IRQLevDone(a6)		; Clear variable, we let the IRQ set it. if it gets set. we have working IRQ
	move.l	#IRQLevTest,$78			; Set up IRQ Level 7
	bsr	TestIRQ
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	cmp.b	#2,d0
	beq	.done7
	bsr	WaitReleased
.done7:
       jsr	ClearBuffer
	lea	IRQTestDone,a0
	move.l	#2,d1
	bsr	Print
	bsr	WaitButton
.exit:
	bsr	IRQCIAtestMenu

IRQCIACIATest:
	cmp.b	#1,RASTER(a6)			; Check if we have a working raster, if not we are unable to
	bne	.noraster			; count frames (for timing) so not possible to perform tests
	bsr	InitScreen
	lea	CIATestTxt,a0
	move.w	#2,d1
	bsr	Print
	lea	CIATestTxt2,a0
	move.w	#2,d1
	bsr	Print
.loop:
	bsr	GetInput
	cmp.b	#$1b,GetCharData(a6)
	beq	IRQCIAtestMenu
	cmp.b	#1,RMB(a6)
	beq	IRQCIAtestMenu
	cmp.b	#1,BUTTON(a6)
	bne.s	.loop
	lea	CIATestTxt3,a0
	move.w	#4,d1
	bsr	Print
       move.w	#$7fff,$dff09a			; Kill all chip interrupts
	lea	CIAATestAATxt,a0
	lea	$bfe001,a5			; load a5 with a base
	lea	$bfe001,a4			; load a5 with a base
	lea	$bfee01,a3			; load a5 with a base
	move.l	#0,d2
	move.l	#7,d5	
	bsr	.TestCIA
	lea	CIAATestBATxt,a0
	lea	$bfe201,a5			; load a5 with a base
	lea	$bfe001,a4			; load a5 with a base
	lea	$bfef01,a3			; load a5 with a base
	move.l	#1,d2
	move.l	#8,d5	
	bsr	.TestCIA
	bsr	TestATOD
	lea	CIAATestABTxt,a0
	lea	$bfd000,a5			; load a5 with a base
	lea	$bfd000,a4			; load a5 with a base
	lea	$bfde00,a3			; load a5 with a base
	move.l	#0,d2
	move.l	#10,d5	
	bsr	.TestCIA
	lea	CIAATestBBTxt,a0
	lea	$bfd200,a5			; load a5 with a base
	lea	$bfd000,a4			; load a5 with a base
	lea	$bfdf00,a3			; load a5 with a base
	move.l	#1,d2
	move.l	#11,d5	
	bsr	.TestCIA
	bsr	TestBTOD
	jsr	ClearBuffer
	lea	ButtonExit,a0
	move.l	#1,d0
	bsr	Print
.keyloop:
	bsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne.s	 .keyloop
	bra	IRQCIAtestMenu
.noraster:					; We had no working raster, print errormessage
	bsr	InitScreen			; and prompt for keypress to go back to mainmenu.
	lea	CIANoRasterTxt,a0
	move.w	#1,d1
	bsr	Print
	lea	CIANoRasterTxt2,a0
	move.w	#2,d1
	bsr	Print
.nrloop:
	bsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne.s	.nrloop
	bra	MainMenu

.TestCIA:
	clr.l	d0
	move.l	d5,d1
	bsr	SetPos
	move.l	#3,d1
	bsr	Print
	clr.w	Frames(a6)			; Clear number frames
	clr.w	TickFrame(a6)
       clr.l	Ticks(a6)
	move.l	#CIALevTst,$6c			; Set up IRQ Level 3
	move.w	#$c020,$dff09a			; Enable IRQ
	move.w	#$c020,$dff09a			; Enable IRQ
	move.w	#$2000,sr			; Set SR to allow IRQs
	move.b	(a3),d0				; Set control register A on CIAA
	move.b	d0,CIACtrl(a6)
	andi.b	#$c0,d0				; Do not touch bits we are not
	ori.b	#8,d0				; Using...
	move.b	d0,(a3)
	move.w	#7812,d6
	clr.l	d7
.loopa:
	move.l	#$f0,$dff180
	move.b	$400(a5),CIACtrl+1(a6)
	move.b	$500(a5),CIACtrl+2(a6)
	move.b	#(CIATIME&$FF),$400(a5)
	move.b	#(CIATIME>>8),$500(a5)			; Set registers to wait for 10000ms
.wait:
	move.w	#$0,$dff180
	cmp.w	#120,Frames(a6)
	bge	.vblankoverrun
	btst	d2,$d00(a4)
	beq	.wait
	add.l	#1,Ticks(a6)
	move.w	#$f,$dff180
.no:
	dbf	d6,.loopa				; Repeat this so we are doing it for a while
	bset	#0,(a3)
	clr.l	d6				; Clear D6, meaning we have executed this without Vblank overrun
	bra	.exit
.vblankoverrun:	
	move.l	#1,d6				; Set it as 1, to mark we had a overrun

.exit:
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	move.l	#RTEcode,$6c			; Restore IRC Vector to empty code
	move.b	CIACtrl(a6),(a3)
	move.b	CIACtrl+1(a6),$400(a5)
	move.b	CIACtrl+2(a6),$500(a5)
	move.w	Frames(a6),TickFrame(a6)
	move.l	#35,d0
	move.l	d5,d1
	bsr	SetPos
	move.l	Ticks(a6),d0
	asl.l	#8,d0
	bsr	bindec
	move.w	#2,d1
	bsr	Print
	lea	ms,a0
	bsr	Print
	move.w	TickFrame(a6),d0
;	cmp.w	#105,d0
;	bge	.underrun
	cmp.w	#95,d0
	ble	.underrun
	bra	.nounderrun
.underrun:
	lea	VblankUnderrunTXT,a0
	move.l	#1,d1
	bsr	Print
	move.l	#2,d6
	bra	.nooverrun
.nounderrun:
	cmp.b	#1,d6
	bne	.nooverrun
	lea	VblankOverrunTXT,a0
	move.l	#1,d1
       bsr	Print
.nooverrun:
	cmp.b	#0,d6				; Check d6, if it isnt 0, we had a failure
	beq	.nooverrun2
	move.l	#70,d0
	move.l	d5,d1
	bsr	SetPos
	lea	FAILED,a0
	move.w	#1,d1
	bsr	Print
	rts
.nooverrun2:
	move.l	#70,d0
	move.l	d5,d1
	bsr	SetPos
	lea	OK,a0
	move.l	#2,d1
	bsr	Print
	rts
	
;	clr.l	d0
;	move.l	#1,d1
;	bsr	SetPos
;	clr.l	d0
;	move.w	TickFrame(a6),d0
;	bsr	bindec
;	move.l	#3,d1
;	bsr	Print
;	rts

CIALevTst:
	move.w	#$020,$dff09c			; Enable IRQ
	move.w	#$020,$dff09c			; Enable IRQ
	add.w	#1,Frames(a6)				; Add 1 to Frames so we can keep count of frames shown.
							; (or VBlanks)
	TOGGLEPWRLED
.no:
	rte

TestATOD:
	lea	CIATestATOD,a0
	clr.l	d0
	move.l	#9,d1
	bsr	SetPos
	move.l	#3,d1
	bsr	Print
	clr.w	Frames(a6)			; Clear number frames
	clr.l	Ticks(a6)
	move.l	#CIALevTst,$6c			; Set up IRQ Level 3
	move.w	#$c020,$dff09a			; Enable IRQ
	move.w	#$c020,$dff09a			; Enable IRQ;
;	move.w	$dff01c,$d7
	bclr	#7,$bfef01
	move.b	#0,$bfea01
	move.b	#0,$bfe901
	move.b	#0,$bfe801
.loopa:

	moveq	#0,d6
	move.b	$bfea01,d6
	lsl.l	#8,d6
	move.b	$bfe901,d6
	lsl.l	#7,d6
	move.b	$bfe801,d6
	cmp.l	Ticks(a6),d6
	beq.s	.no
	move.w	#$f,$dff180
.no:
	move.l	d6,Ticks(a6)
	clr.l	d0
	move.l	#10,d1
	bsr	SetPos
	clr.l	d0
	cmp.w	#100,Frames(a6)		; Check if we have tested for 200 VBlanks
	blt	.loopa
       move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	move.l	#RTEcode,$6c			; Restore IRC Vector to empty code
	move.l	#35,d0
	move.l	#9,d1
	bsr	SetPos
	move.l	Ticks(a6),d0
	bsr	bindec
	move.w	#2,d1
	bsr	Print
	lea	ticks,a0
	bsr	Print
	cmp.l	#95,d6
	ble	.tooslow
	cmp.l	#105,d6
	bge	.toofast
	move.l	#70,d0
	move.l	#9,d1
	bsr	SetPos
	lea	OK,a0
	move.w	#2,d1
	bsr	Print
	rts
.tooslow:
	move.l	#1,d1
	lea	CIATickSlowTxt,a0
	bsr	Print
	move.l	#70,d0
	move.l	#9,d1
	bsr	SetPos
	lea	FAILED,a0
	move.l	#1,d1
	bsr	Print
	rts
.toofast:
	move.l	#1,d1
	lea	CIATickFastTxt,a0
	bsr	Print
	move.l	#70,d0
	move.l	#9,d1
	bsr	SetPos
	lea	FAILED,a0
	move.l	#1,d1
	bsr	Print
	rts

TestBTOD:
	lea	CIATestBTOD,a0
	clr.l	d0
	move.l	#12,d1
	bsr	SetPos
	move.l	#3,d1
	bsr	Print
	clr.w	Frames(a6)			; Clear number frames
	clr.l	Ticks(a6)
	move.l	#CIALevTst,$6c			; Set up IRQ Level 3
	move.w	#$c020,$dff09a			; Enable IRQ
	move.w	#$c020,$dff09a			; Enable IRQ;
;	move.w	$dff01c,$d7
	bclr	#7,$bfdf00
	move.b	#0,$bfda00
	move.b	#0,$bfd900
	move.b	#0,$bfd800
.loopa:
	moveq	#0,d6
	move.b	$bfda00,d6
	lsl.l	#8,d6
	move.b	$bfd900,d6
	lsl.l	#8,d6
	move.b	$bfd800,d6
	cmp.l	Ticks(a6),d6
	beq.s	.no
	move.w	#$f,$dff180
.no:
	move.l	d6,Ticks(a6)
	clr.l	d0
	move.l	#12,d1
	bsr	SetPos
	clr.l	d0
	cmp.w	#100,Frames(a6)		; Check if we have tested for 200 VBlanks
	ble	.loopa
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	move.l	#RTEcode,$6c			; Restore IRC Vector to empty code
	move.l	#35,d0
	move.l	#12,d1
	bsr	SetPos
	move.l	Ticks(a6),d0
	bsr	bindec
	move.w	#2,d1
	bsr	Print
	lea	ticks,a0
	bsr	Print
	cmp.l	#30000,d6
	ble	.tooslow
	cmp.l	#32000,d6
	bge	.toofast
	move.l	#70,d0
	move.l	#12,d1
	bsr	SetPos
	lea	OK,a0
	move.w	#2,d1
	bsr	Print
	rts
.tooslow:
	move.l	#1,d1
	lea	CIATickSlowTxt,a0
	bsr	Print
	move.l	#70,d0
	move.l	#12,d1
	bsr	SetPos
	lea	FAILED,a0
	move.l	#1,d1
	bsr	Print
	rts
.toofast:
	move.l	#1,d1
	lea	CIATickFastTxt,a0
	bsr	Print
	move.l	#70,d0
	move.l	#12,d1
	bsr	SetPos
	lea	FAILED,a0
	move.l	#1,d1
	bsr	Print
	rts

IRQCIATest:
	bsr	_IRQCIATestC
       bra    MainMenu


TestIRQ:				; Test if IRQ was triggered
					; OUT:
					;	d0 = 0	== Everything sucessful
					;	d0 = 1	== We have failure
					;	d0 = 2	== User pressed cancel
	clr.l	d0
	move.w	#100,d7
.loop:
	bsr	GetInput			; Check for input from user
	cmp.b	#1,BUTTON(a6)			; If button is pressed, exit
	beq	.exitloop
	bsr	WaitLong
	cmp.w	#1,IRQLevDone(a6)		; Check if IRQLevDone is set, done in IRQ routine
	beq	.yes
	dbf	d7,.loop
	lea	FAILED,a0
	move.l	#1,d1
	cmp.w	#1,IRQLev7(a6)
	bne	.no7
	move.w	#2,d1
	lea	NONE,a0
.no7
	bsr	Print
	move.b	#1,d0				; we exited loop, test failed
	rts
.yes:
	lea	OK,a0
	move.l	#2,d1
	bsr	Print
	rts
.exitloop:
	lea	CANCELED,a0
	move.l	#3,d1
	bsr	Print
	rts

IRQLevTest:					; Small IRQ Rouine, all it does is to set IRQLevDone to 1
	move.w	#$fff,$dff180
	move.w	#1,IRQLevDone(a6)
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	rte

RTEcode:					; Just to have something to point IRQ to.. doing nothing
	move.w	#$444,$dff180
	rte
