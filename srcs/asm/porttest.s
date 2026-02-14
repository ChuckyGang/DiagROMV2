       include "earlymacros.i"
       include "build/srcs/globalvars.i"
       section "porttest",code_p
       xdef   PortTestMenu
       xdef   PortTestPar
       xdef   PortTestSer
       xdef   PortTestJoystick

PortTestMenu:
	jsr	InitScreen
	move.w	#6,MenuNumber(a6)
	move.b	#1,PrintMenuFlag(a6)
	bra	MainLoop


PortTestPar:
       jsr	ClearScreen
	lea	PortParTest,a0
	move.l	#7,d1
	jsr	Print
	lea	PortParTest1,a0
	move.l	#3,d1
	jsr	Print
	lea	PortParTest2,a0
	move.l	#2,d1
	jsr	Print
.loop:
	jsr	GetInput
	cmp.b	#1,RMB(a6)
	beq	.exit
	move.b	GetCharData(a6),d0
	cmp.b	#$1b,d0	
	beq	.exit
	cmp.b	#0,BUTTON(a6)
	beq	.loop
	lea	PortParTest3,a0
	move.l	#6,d1
	jsr	Print
	move.b	#$ff,$bfe301
	jsr	WaitLong
	move.b	#0,$bfe101
	jsr	WaitLong
	move.b	#%11111111,$bfe301	; set pins output
	jsr	WaitLong
	move.b	#0,$bfe101		; Set all pins to 0
	move.b	#0,$bfd200
	clr.l	Passno(a6)
.loopa:
	clr.l	d0
	move.l	#10,d1
	jsr	SetPos
	lea	PassTxt,a0
	move.l	#4,d1
	jsr	Print
	clr.l	d0
	add.l	#1,Passno(a6)
	move.l	Passno(a6),d0
	jsr	bindec
	move.l	#6,d1
	jsr	Print			; Print out passnumber
	jsr	WaitLong
	move.b	#%00000101,$bfe301
	jsr	WaitLong
	move.b	#0,$bfe101		; Set all pins to 0
	lea	PortParTest12,a0
	move.l	#3,d1
	jsr	Print
	move.b	#0,d0
	move.b	#1,d1
	jsr	.TestPin
	jsr	WaitLong
	move.b	#%00000010,$bfe301
	jsr	WaitLong
	lea	PortParTest21,a0
	move.l	#3,d1
	jsr	Print
	move.b	#1,d0
	move.b	#0,d1
	jsr	.TestPin
	jsr	WaitLong
	move.b	#%00000100,$bfe301
	jsr	WaitLong
	move.b	#0,$bfe101		; Set all pins to 0
	lea	PortParTest34,a0
	move.l	#3,d1
	jsr	Print
	move.b	#2,d0
	move.b	#3,d1
	jsr	.TestPin
	jsr	WaitLong
	move.b	#%00001000,$bfe301
	jsr	WaitLong
	move.b	#0,$bfe101		; Set all pins to 0
	lea	PortParTest43,a0
	move.l	#3,d1
	jsr	Print
	move.b	#3,d0
	move.b	#2,d1
	jsr	.TestPin
	jsr	WaitLong
	move.b	#%00010000,$bfe301
	jsr	WaitLong
	move.b	#0,$bfe101		; Set all pins to 0
	lea	PortParTest56,a0
	move.l	#3,d1
	jsr	Print
	move.b	#4,d0
	move.b	#5,d1
	jsr	.TestPin
	jsr	WaitLong
	move.b	#%00100000,$bfe301
	jsr	WaitLong
	move.b	#0,$bfe101		; Set all pins to 0
	lea	PortParTest65,a0
	move.l	#3,d1
	jsr	Print
	move.b	#5,d0
	move.b	#4,d1
	jsr	.TestPin
	jsr	WaitLong
	move.b	#%01000000,$bfe301
	move.b	#0,$bfd200
	move.b	#%00000000,$bfe101
	jsr	WaitLong
	lea	PortParTest7p,a0
	move.l	#3,d1
	jsr	Print
	move.b	#1,d1
	move.b	#6,d0
	jsr	.TestPins
	jsr	WaitLong
	move.b	#2,$bfd200
	move.b	#0,$bfd000
	move.b	#0,$bfe301
	jsr	WaitLong
	lea	PortParTestp7,a0
	move.l	#3,d1
	jsr	Print
	move.b	#6,d1
	move.b	#1,d0
	jsr	.TestPins2
	jsr	WaitLong
	move.b	#%01000000,$bfe301
	move.b	#0,$bfd200
	move.b	#%00000000,$bfe101
	jsr	WaitLong
	lea	PortParTest7s,a0
	move.l	#3,d1
	jsr	Print
	move.b	#2,d1
	move.b	#6,d0
	jsr	.TestPins
       jsr	WaitLong
	move.b	#4,$bfd200
	move.b	#0,$bfd000
	move.b	#0,$bfe301
	jsr	WaitLong
	lea	PortParTests7,a0
	move.l	#3,d1
	jsr	Print
	move.b	#6,d1
	move.b	#2,d0
	jsr	.TestPins2
	jsr	WaitLong
	move.b	#%10000000,$bfe301
	jsr	WaitShort
	move.b	#0,$bfd000
	move.b	#%00000000,$bfe101
	jsr	WaitLong
	lea	PortParTest8b,a0
	move.l	#3,d1
	jsr	Print
	move.b	#0,d1
	move.b	#7,d0
	jsr	.TestPins
	jsr	WaitLong
	move.b	#1,$bfd200
	move.b	#0,$bfd000
	move.b	#0,$bfe301
	jsr	WaitLong
	lea	PortParTestb8,a0
	move.l	#3,d1
	jsr	Print
	move.b	#7,d1
	move.b	#0,d0
	jsr	.TestPins2
	jsr	GetInput
.nolmb:
	cmp.b	#1,BUTTON(a6)
	beq	.exit
	bne	.loopa
.exit:
	bra	PortTestMenu
.TestPin:					;	Test pin of par port
						; IN:
						;	d0 = bit to write
						;	d1 = bit to read
						; OUT:
						;	d2 = 0 = OK, 1=Fail
	clr.l	d2				; Clear errorregister
	jsr	WaitLong
	btst	d1,$bfe101			; First test that bit is 0, if not something is wrong
						; (like missing dongle! but something else can also be a problem)
	beq	.null
	bra	.fail
.null:						; bit was 0, lets set bit and test that it got high
	bset	d0,$bfe101
	jsr	WaitLong
	btst	d1,$bfe101
	beq	.was0
	lea	OOK,a0
	move.b	#2,d1
	jsr	Print
	rts
						; bit was 1, we are OK
.was0:						; ok bit was 0, something is wrong, exit with fail
	bra	.fail
	rts
.fail:
	lea	BAD,a0
	move.b	#1,d1
	jsr	Print
	rts
.TestPins:					; Same as testpin but checks CIAB instead
	clr.l	d2				; Clear errorregister
	jsr	WaitLong
	btst	d1,$bfd000			; First test that bit is 0, if not something is wrong
						; (like missing dongle! but something else can also be a problem)
	beq	.nulls
	bra	.fail
.nulls:						; bit was 0, lets set bit and test that it got high
	bset	d0,$bfe101
	jsr	WaitLong
	btst	d1,$bfd000
	beq	.was0
	lea	OOK,a0
	move.b	#2,d1
	jsr	Print
	rts
.TestPins2:					; Same as testpins but sets CIAB instead
	clr.l	d2				; Clear errorregister
	jsr	WaitLong
	btst	d1,$bfe101			; First test that bit is 0, if not something is wrong
						; (like missing dongle! but something else can also be a problem)
	beq	.nulls2
	bra	.fail
.nulls2						; bit was 0, lets set bit and test that it got high
	bset	d0,$bfd000
	jsr	WaitLong
	btst	d1,$bfe101
	beq	.was0
	lea	OOK,a0
	move.b	#2,d1
	jsr	Print
	rts


PortTestSer:
	jsr	ClearScreen
	lea	PortSerTest,a0
	move.l	#7,d1
	jsr	Print
	lea	PortSerTest1,a0
	move.l	#3,d1
	jsr	Print
	lea	PortSerTest2,a0
	move.l	#2,d1
	jsr	Print
.loop:
	jsr	GetInput
	cmp.b	#1,RMB(a6)
	beq	.exit
	move.b	GetCharData(a6),d0
	cmp.b	#$1b,d0	
	beq	.exit
	cmp.b	#0,BUTTON(a6)
	beq	.loop
	clr.l	Passno(a6)
	move.w	#0,SerTstBps(a6)
.loopa:
;	jsr	ClearScreen
	lea	PortParTest3,a0			; Lets steal that string
	move.l	#6,d1
	jsr	Print
.testloop:
	clr.l	d0
	move.l	#16,d1
	jsr	SetPos
	lea	PassTxt,a0
	move.l	#4,d1
	jsr	Print
	clr.l	d0
	add.l	#1,Passno(a6)
	move.l	Passno(a6),d0
	jsr	bindec
	move.l	#6,d1
	jsr	Print			; Print out passnumber
	lea	PortSerBps,a0
	move.l	#4,d1
       jsr	Print
	move.b	#$ff,$bfd200
	move.b	#0,$bfd000
	add.w	#1,SerTstBps(a6)
	cmp.w	#5,SerTstBps(a6)
	bne	.notmax
	move.w	#1,SerTstBps(a6)
.notmax:
	move.w	SerTstBps(a6),d0		; Get SerialSpeed value
	mulu	#4,d0				; Multiply with 4
	lea	SerText,a0			; Load table of pointers to different texts
	move.l	(a0,d0.l),a0			; load a0 with the value that a0+d0 points to (text of speed)
	move.l	#7,d1
	jsr	Print
	lea	SerSpeeds,a0
	move.l	(a0,d0),d0			; Load d0 with the value to write to the register for the correct speed.
	move.w	d0,$dff032			; Set the speed of the serialport
       lea	PortSerTest3,a0
	move.l	#3,d1
	jsr	Print
	move.l	#67,d0
	move.l	#18,d1
	jsr	SetPos
	clr.l	d6
	lea	PortSerString,a0
.serloop:
	move.b	(a0)+,d2
	cmp.b	#0,d2
	beq	.donetest
	jsr	RealLoopbacktest
	bra	.serloop
.donetest:
	move.l	d6,d0
	move.l	#2,d1
	jsr	bindec
	jsr	Print
	lea	PortSerTestB45,a0		; Test RTS->CTS
	move.l	#3,d1
	jsr	Print
	move.b	#$c0,$bfd200
	move.b	#6,d0
	move.b	#4,d1
	jsr	.TestPin
	lea	PortSerTestB46,a0		; Test RTS -> DSR
	move.l	#3,d1
	jsr	Print
	move.b	#$c0,$bfd200
	move.b	#0,$bfd000
	jsr	WaitLong
	move.b	#6,d0
	move.b	#3,d1
	jsr	.TestPin
	lea	PortSerTestB208,a0		; Test DTR -> CD
	move.l	#3,d1
	jsr	Print
	move.b	#$c0,$bfd200
	move.b	#0,$bfd000
	jsr	WaitLong
	move.b	#%10000000,$bfd200		; Set what pin is output
	move.b	#7,d0
	move.b	#5,d1
	jsr	.TestPin
	jsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne	.testloop
;	jsr	WaitReleased
.exit:
.loopend:
	jsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne.s	.loopend
	jsr	Init_Serial
	bra	PortTestMenu
.TestPin:
						; Sets a bit and checks is a bit is set at CIAB Register
						; IN
						;	D0 = Bit to set
						;	D1 = Bit to test
	clr.l	d3
	bset	d0,d3
	clr.l	d2
	move.b	d3,$bfd000
	jsr	WaitLong
	btst	d1,$bfd000
	bne	.bitset
	bra	.bitclrtst
.bitset:
	add.b	#1,d2
.bitclrtst:
	bclr	d0,d3
	move.b	d3,$bfd000
	jsr	WaitLong
	btst	d1,$bfd000
	beq	.bitclear
	bra	.showresult
.bitclear:
	add.b	#1,d2
.showresult:
	cmp.b	#2,d2
	bne	.testfail
	lea	OOK,a0
	move.b	#2,d1
	jsr	Print
	rts
.testfail:
	lea	BAD,a0
	move.b	#1,d1
	jsr	Print
	rts

PortTestJoystick:
	jsr	ClearScreen
	move.w	#$ffff,JOY0DAT(a6)
	move.w	#$ffff,JOY1DAT(a6)
	move.w	#$ffff,POT0DAT(a6)
	move.w	#$ffff,POT1DAT(a6)
	move.w	#$ffff,POTINP(a6)	
	move.b	#$ff,CIAAPRA(a6)
	clr.l	PortJoy0(a6)
	clr.l	PortJoy1(a6)
	move.w	#$fff,PortJoy0OLD(a6)
	move.w	#$fff,PortJoy1OLD(a6)
	clr.w	P0Fire(a6)
	clr.w	P1Fire(a6)
	move.w	#$fff,P0FireOLD(a6)
	move.w	#$fff,P1FireOLD(a6)
	lea	PortJoyTest,a0
	move.l	#7,d1
	jsr	Print
	lea	PortJoyTest1,a0
	move.l	#6,d1
	jsr	Print
	lea	PortJoyTestHW1,a0
	move.l	#3,d1
	jsr	Print
	lea	PortJoyTestHW2,a0
	move.l	#3,d1
	jsr	Print
	lea	PortJoyTestHW3,a0
	move.l	#3,d1
	jsr	Print
	lea	PortJoyTestHW4,a0
	move.l	#3,d1
	jsr	Print
	lea	PortJoyTestHW5,a0
	move.l	#3,d1
	jsr	Print
	lea	PortJoyTestHW6,a0
	move.l	#3,d1
	jsr	Print
	move.l	#0,d0
	move.l	#11,d1
	jsr	SetPos
	lea	PortJoyTest2,a0
	move.l	#6,d1
	jsr	Print
	lea	PortJoyTest3,a0
	move.l	#6,d1
	jsr	Print
	move.l	#0,d0
	move.l	#23,d1
	jsr	SetPos
	lea	PortJoyTestExitTxt,a0
	move.l	#7,d1
	jsr	Print
.loop:
	move.w	$dff00a,d7
	lea	JOY0DAT(a6),a0
	cmp.w	(a0),d7
	beq	.samejoy0dat
	move.w	d7,(a0)
	move.w	d7,PortJoy0(a6)
	move.l	#36,d0
	move.l	#4,d1
	jsr	SetPos
	clr.l	d0
	move.l	d7,d0
	jsr	binhexword
	move.l	#2,d1
	jsr	Print
	move.l	#47,d0
	move.l	#4,d1
	jsr	SetPos
	move.l	d7,d0
	jsr	binstring
	add.l	#16,a0
	move.l	#2,d1
	jsr	Print
.samejoy0dat:
	move.w	$dff00c,d7
	lea	JOY1DAT(a6),a0
	cmp.w	(a0),d7
	beq	.samejoy1dat
	move.w	d7,(a0)
	move.w	d7,PortJoy1(a6)
	move.l	#36,d0
	move.l	#5,d1
	jsr	SetPos
	clr.l	d0
	move.l	d7,d0
	jsr	binhexword
	move.l	#2,d1
	jsr	Print
	move.l	#47,d0
	move.l	#5,d1
	jsr	SetPos
	move.l	d7,d0
	jsr	binstring
	PUSH
	add.l	#16,a0
	move.l	#2,d1
	jsr	Print
       POP
.samejoy1dat:
	move.w	$dff012,d7
	lea	POT0DAT(a6),a0
	cmp.w	(a0),d7
	beq	.samepot0dat
	move.w	d7,(a0)
	move.l	#36,d0
	move.l	#6,d1
	jsr	SetPos
	clr.l	d0
	move.l	d7,d0
	jsr	binhexword
	move.l	#2,d1
	jsr	Print
	move.l	#47,d0
	move.l	#6,d1
	jsr	SetPos
	move.l	d7,d0
	jsr	binstring
	add.l	#16,a0
	move.l	#2,d1
	jsr	Print
.samepot0dat:
	move.w	$dff014,d7
	lea	POT1DAT(a6),a0
	cmp.w	(a0),d7
	beq	.samepot1dat
	move.w	d7,(a0)
	move.l	#36,d0
	move.l	#7,d1
	jsr	SetPos
	clr.l	d0
	move.l	d7,d0
	jsr	binhexword
	move.l	#2,d1
	jsr	Print
	move.l	#47,d0
	move.l	#7,d1
	jsr	SetPos
	move.l	d7,d0
	jsr	binstring
	add.l	#16,a0
	move.l	#2,d1
	jsr	Print
.samepot1dat:
	move.w	$dff016,d7
	lea	POTINP(a6),a0
	cmp.w	(a0),d7
	beq	.samepotinp
	move.w	d7,(a0)
	move.l	#36,d0
	move.l	#8,d1
	jsr	SetPos
	clr.l	d0
	move.l	d7,d0
	jsr	binhexword
	move.l	#2,d1
	jsr	Print
	move.l	#47,d0
	move.l	#8,d1
	jsr	SetPos
	move.l	d7,d0
	jsr	binstring
	add.l	#16,a0
	move.l	#2,d1
	jsr	Print
.samepotinp:
	clr.l	d7
	move.b	$bfe001,d7
	lea	CIAAPRA(a6),a0
	cmp.w	(a0),d7
	beq	.samefire
	move.w	d7,(a0)
	btst	#6,$bfe001
	bne	.noport0
	move.w	#1,P0Fire(a6)
	bra	.p1
.noport0:
	move.w	#0,P0Fire(a6)
.p1:
	btst	#7,$bfe001
	bne	.noport1
	move.w	#1,P1Fire(a6)
	bra	.nop
.noport1:
	move.w	#0,P1Fire(a6)
.nop:
	move.l	#36,d0
	move.l	#9,d1
	jsr	SetPos
	clr.l	d0
	move.l	d7,d0
	jsr	binhexword
	move.l	#2,d1
	jsr	Print
	move.l	#47,d0
	move.l	#9,d1
	jsr	SetPos
	move.l	d7,d0
	jsr	binstring
	add.l	#16,a0
	move.l	#2,d1
	jsr	Print
.samefire:
	clr.l	d0
	move.w	PortJoy0(a6),d0
	cmp.w	PortJoy0OLD(a6),d0
	beq	.samejoy0
	move.w	d0,PortJoy0OLD(a6)
	jsr	GetJoy
	move.l	d0,d7
	move.l	#0,d2
	jsr	PrintJoy
.samejoy0:
	clr.l	d0
	move.w	PortJoy1(a6),d0
	cmp.w	PortJoy1OLD(a6),d0
	beq	.samejoy1
	move.w	d0,PortJoy1OLD(a6)
	clr.l	d0
	move.w	PortJoy1(a6),d0
	jsr	GetJoy
	move.l	d0,d7
	move.l	#37,d2
	jsr	PrintJoy
.samejoy1:
	clr.l	d0
	move.w	P0Fire(a6),d0
	cmp.w	P0FireOLD(a6),d0
	beq	.samefire0
	move.w	d0,P0FireOLD(a6)
	move.l	#19,d0
	move.l	#17,d1
	jsr	SetPos
	lea	FIRE,a0
	cmp.w	#0,P0Fire(a6)
	bne	.nop0
	move.l	#6,d1
	bra	.p0
.nop0:
	move.l	#1,d1
.p0:
	jsr	Print
.samefire0:
	clr.l	d0
	move.w	P1Fire(a6),d0
	cmp.w	P1FireOLD(a6),d0
	beq	.samefire1
	move.w	d0,P1FireOLD(a6)
	move.l	#56,d0
	move.l	#17,d1
	jsr	SetPos
	lea	FIRE,a0
	cmp.w	#0,P1Fire(a6)
	bne	.nop1
	move.l	#6,d1
	bra	.p2
.nop1:
	move.l	#1,d1
.p2:
	jsr	Print
.samefire1:
	jsr	GetInput
	cmp.b	#$1b,GetCharData(a6)
	beq	.exit
	cmp.b	#1,RMB(a6)
	bne	.loop
	cmp.b	#1,LMB(a6)
	bne	.loop
	jsr	WaitReleased
.exit:
	bra	PortTestMenu
PrintJoy:				; Print Joystatus
					; IN =	d7 = joydata
					;	d2 = how much to add in X axis
	move.l	#20,d0
	add.l	d2,d0
	move.l	#15,d1
	jsr	SetPos
	lea.l	UP,a0
	btst	#2,d7
	beq	.noup
	move.l	#1,d1				; 4 blue   6 cyan
	bra	.up
.noup:
	move.l	#6,d1
.up:
	jsr	Print
	move.l	#19,d0
	add.l	d2,d0
	move.l	#19,d1
	jsr	SetPos
	lea.l	DOWN,a0
	btst	#0,d7
	beq	.nodown
	move.l	#1,d1
	bra	.down
.nodown:
	move.l	#6,d1
.down:	
	jsr	Print
	move.l	#13,d0
	add.l	d2,d0
	move.l	#17,d1
	jsr	SetPos
	lea	LEFT,a0
	btst	#3,d7
	beq	.noleft
	move.l	#1,d1
	bra	.left
.noleft:
	move.l	#6,d1
.left:
	jsr	Print
	move.l	#25,d0
	add.l	d2,d0
	move.l	#17,d1
	jsr	SetPos
	lea	RIGHT,a0
	btst	#1,d7
	beq	.noright
	move.l	#1,d1
	bra	.right
.noright:
	move.l	#6,d1
.right:
	jsr	Print
	rts
GetJoy:
	;			IN d0=joy data
	;			OUT: D0   bits =  0=down, 1=right, 2=up, 3=left
	PUSH
	clr.l	d7
	move.l	d0,d6
	move.l	d0,d1
	and.w	#1,d1
	and.w	#2,d0
	asr.w	#1,d0
	eor.w	d1,d0
	btst	#0,d0
	beq	.nodown
	bset	#0,d7
.nodown:
	move.l	d6,d0
	btst	#1,d0
	beq	.noright
	bset	#1,d7
.noright:
	move.l	d0,d1
	and.w	#256,d1
	asr	#8,d1
	and.w	#512,d0
	asr	#8,d0
	asr.w	#1,d0
	eor.w	d1,d0
	btst	#0,d0
	beq	.noup
	bset	#2,d7
.noup:
	move.l	d6,d0
	asr.l	#8,d0
	btst	#1,d0
	beq	.noleft
	bset	#3,d7
.noleft:
	move.l	d7,temp(a6)
	POP
	move.l	temp(a6),d0
	rts
