       include "earlymacros.i"
       include "build/srcs/globalvars.i"        
       section "othertest",code_p
       xdef   OtherTest
       xdef   Setup
       xdef   About
       xdef   RTCTest
       xdef   ShowMemAddress
       xdef   SystemInfoTest


OtherTest:
	jsr	InitScreen
	move.w	#7,MenuNumber(a6)
	move.b	#1,PrintMenuFlag(a6)
	jmp	MainLoop



RTCTest:
	jsr	ClearScreen
	jsr	DevPrint
	move.l	#0,d0
	move.l	#17,d1
	jsr	SetPos
	lea	RTCadjust1,a0
	move.l	#7,d1
	jsr	Print
	lea	RTCadjust10,a0
	move.l	#7,d1
	jsr	Print
	move.l	#0,d0
	move.l	#20,d1
	jsr	SetPos
	move.b	#$8,$dc0039	; do some reset
	lea	RTCIrq,a0
	move.l	#2,d1
	jsr	Print
	lea	RTCIrq2,a0
	move.l	#2,d1
	jsr	Print
	move.b	#$8,$dc0035	; more reset
	clr.l	RTCold(a6)
	clr.w	RTC1secframe(a6)
	clr.w	RTC10secframe(a6)
	clr.w	RTCsec(a6)
	clr.w	RTCirq(a6)
.loopa:
	move.b	#8,$dc0037
	jsr	WaitShort
	lea	$dc0003,a1
	clr.l	d0
	move.b	(a1),d0
	asl.l	#8,d0
	add.b	1(a1),d0
	asl.l	#8,d0
	add.b	2(a1),d0
	asl.l	#8,d0
	add.b	3(a1),d0			; Now we have read a longword. 68k friendly from odd address
;	move.b	$dff006,$dff181
	cmp.l	RTCold(a6),d0			; Check if first byte have changed.
	beq.w	.nochange
	move.l	d0,RTCold(a6)
	add.w	#1,RTCsec(a6)
	cmp.w	#10,RTCsec(a6)
	bne	.sec10				; do this every 10 second
	clr.w	RTCsec(a6)
	move.w	RTC10secframe(a6),d0
	cmp.w	#0,.skip10			; if we had a 0, we just started
	move.w	Frames(a6),d1
	move.w	d1,RTC10secframe(a6)
	sub.w	d0,d1
	clr.l	d7
	move.w	d1,d7
	move.l	#65,d0
	move.l	#18,d1
	jsr	SetPos
	clr.l	d1
	lea	FiveSpacesTxt,a0
	jsr	Print
	move.l	#65,d0
	move.l	#18,d1
	jsr	SetPos
	move.l	d7,d0
	jsr	bindec
	move.l	#3,d1
	jsr	Print
.sec10:
.skip10:
	move.w	RTC1secframe(a6),d0
	cmp.w	#0,.skip			; if we had a 0, we just started
	move.w	Frames(a6),d1
	move.w	d1,RTC1secframe(a6)
	sub.w	d0,d1
	clr.l	d7
	move.w	d1,d7
	move.l	#65,d0
	move.l	#17,d1
	jsr	SetPos
	clr.l	d1
	lea	FiveSpacesTxt,a0
	jsr	Print
	move.l	#65,d0
	move.l	#17,d1
	jsr	SetPos
	move.l	d7,d0
	jsr	bindec
	move.l	#3,d1
	jsr	Print
.skip:
	clr.l	d0
	clr.l	d1
	jsr	SetPos
	lea	RTCByteTxt,a0
	lea	RTCString(a6),a2
	move.l	#3,d1
	jsr	Print
	move.l	#13,d7
.loop:
	move.b	#8,$dc0037
	jsr	WaitShort
	clr.l	d0
	move.b	(a1),d0
	move.b	d0,d1
	and.b	#$f,d1				;Strip away top 4 bits
	move.b	d1,(a2)+
	jsr	binhexbyte
	move.l	#2,d1
	jsr	Print
	add.l	#4,a1
	dbf	d7,.loop
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	RTCBitTxt,a0
	move.l	#3,d1
	jsr	Print
	move.b	#8,$dc0037
	jsr	WaitShort
	lea	RTCString(a6),a1
	move.l	#13,d7
.loop1:
	clr.l	d0
	move.b	(a1)+,d0
	jsr	binstringbyte
	move.l	#2,d1
	jsr	Print
	lea	SpaceTxt,a0
	jsr	Print
	cmp.b	#7,d7
	bne	.nope
	lea	NewLineTxt,a0
	jsr	Print
.nope:
	dbf	d7,.loop1
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	RTCString(a6),a0		; load a0 to string from RTC
	jsr	ricoh
	jsr	oki
	clr.l	d0
.nochange:
	jsr	GetInput
	cmp.b	#1,LMB(a6)
	beq.s	.irq
	cmp.b	#" ",keyresult(a6)
	beq.s	.irq
	cmp.b	#1,BUTTON(a6)
	bne	.loopa
.end:
	cmp.w	#0,RTCirq(a6)			; did we run the IRQ code?
	beq	.noirq
	move.w	#$7fff,$dff09c			; Disable all INTREQ
	move.w	#$7fff,$dff09a			; Disable all INTREQ
	move.l	#RTEcode,$6c			; Restore IRC Vector to empty code
.noirq:
	jsr	WaitButton
	bra	OtherTest
.irq:
	cmp.b	#1,RMB(a6)
	beq.s	.end
	cmp.w	#0,RTCirq(a6)			; check if no IRQ is running
	bne	.running			; if it is go to running
	move.w	#1,RTCirq(a6)			; set it to 1
	clr.w	Frames(a6)			; Clear number frames
	clr.w	TickFrame(a6)
	clr.l	Ticks(a6)
	move.l	#CIALevTst,$6c			; Set up IRQ Level 3
	move.w	#$c020,$dff09a			; Enable IRQ
	move.w	#$c020,$dff09a			; Enable IRQ
	move.w	#$2000,sr			; Set SR to allow IRQs
	move.w	#$fff,$dff180
.running:
	bra	.loopa
ricoh:						; RICOH chipset detected.
	lea	RTCRicoh,a0
	move.l	#6,d1
	jsr	Print
	move.l	#2,d1
	lea	RTCString(a6),a0		; load a0 to string from RTC
	lea	RTCDay,a1
	clr.l	d0
	move.b	6(a0),d0
	mulu	#10,d0
	move.l	a1,a0
	add.l	d0,a0
	jsr	Print
	move.b	#" ",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	clr.l	d0
	move.b	12(a0),d0
	mulu	#10,d0
	add.b	11(a0),d0			; We have now the year, 2 digits
	cmp.b	#78,d0				; Check for 78
	bge	.r19				; more or equal to 78. we are in 19xx
	add.l	#2000,d0
	bra	.rno19
.r19:
	add.l	#1900,d0
.rno19
	jsr	bindec
	jsr	Print
						; Now year is printed
	move.b	#"-",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	clr.l	d0
	move.b	10(a0),d0
	mulu	#10,d0
	add.b	9(a0),d0			; We have now the month
	sub.l	#1,d0
	cmp.b	#12,d0
	blt	.rnoover
	move.l	#12,d0
.rnoover:
	mulu	#4,d0				; Multiply with 4 to get where string start
	lea	RTCMonth,a5
	move.l	a5,a0
	add.l	d0,a0
	jsr	Print
	move.b	#"-",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	clr.l	d0
	move.b	8(a0),d0
	add.b	#$30,d0
	jsr	PrintChar
	move.b	7(a0),d0
	add.b	#$30,d0
	jsr	PrintChar
	move.b	#" ",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	add.l	#6,a0
	move.l	#5,d7
	clr.l	d6				; Clear d6 as it is a counter when to print a :
.rloop:
	cmp.b	#2,d6				; time to print : ?
	bne	.rnocolon
	move.b	#":",d0
	jsr	PrintChar
	clr.l	d6
.rnocolon:
	add.l	#1,d6
	move.b	-(a0),d0
	add.b	#$30,d0
	jsr	PrintChar
	dbf	d7,.rloop
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	rts
oki:
	lea	RTCOKI,a0
	move.l	#6,d1
	jsr	Print
	move.l	#2,d1
	lea	RTCString(a6),a0		; load a0 to string from RTC
	lea	RTCDay,a1
	clr.l	d0
	move.b	6(a0),d0
	mulu	#10,d0
	move.l	a1,a0
	add.l	d0,a0
	jsr	Print
	move.b	#" ",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	clr.l	d0
	move.b	11(a0),d0
	mulu	#10,d0
	add.b	10(a0),d0			; We have now the year, 2 digits
	add.l	#1900,d0
	jsr	bindec
	move.l	#2,d1
	jsr	Print
	move.b	#"-",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	clr.l	d0
	move.b	9(a0),d0
	mulu	#10,d0
	add.b	8(a0),d0			; We have now the month
	sub.l	#1,d0
	cmp.b	#11,d0
	blt	.okinoover
	move.l	#12,d0
.okinoover:
	mulu	#4,d0				; Multiply with 4 to get where string start
       lea	RTCMonth,a5
	move.l	a5,a0
	add.l	d0,a0
	jsr	Print
	move.b	#"-",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	clr.l	d0
	move.b	7(a0),d0
	add.b	#$30,d0
	jsr	PrintChar
	move.b	6(a0),d0
	add.b	#$30,d0
	jsr	PrintChar
	move.b	#" ",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	clr.l	d0
	clr.l	d7				; if d7 is not 0, we are in PM
	move.b	5(a0),d0
	btst	#2,d0
	beq	.ono
	move.b	#1,d7				; Set that we are in PM
	bclr	#2,d0
.ono:
	mulu	#10,d0
	add.b	4(a0),d0
	cmp.b	#0,d7
	beq.w	.ono1
	sub.b	#2,d0
	cmp.b	#254,d0
	beq.b	.oki8
	cmp.b	#255,d0
	beq.b	.oki9
	bra	.ono2
.oki8:
	move.b	#8,d0
	bra.s	.ono2
.oki9:
	move.b	#9,d0
	bra.w	.ono2
.ono2:
	add.b	#12,d0
.ono1:
	cmp.b	#9,d0
	bgt	.okilow
	PUSH
	move.b	#"0",d0
	jsr	PrintChar
	POP
.okilow:
	jsr	bindec
	jsr	Print
	move.b	#":",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	move.b	3(a0),d0
	add.b	#"0",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	move.b	2(a0),d0
	add.b	#"0",d0
	jsr	PrintChar
	move.b	#":",d0
	jsr	PrintChar

	lea	RTCString(a6),a0		; load a0 to string from RTC
	move.b	1(a0),d0
	add.b	#"0",d0
	jsr	PrintChar
	lea	RTCString(a6),a0		; load a0 to string from RTC
	move.b	(a0),d0
	add.b	#"0",d0
	jsr	PrintChar
	rts

ShowMemAddress:
	jsr	InitScreen
	lea	ShowMemAdrTxt,a0
	move.l	#6,d1
	jsr	Print
	lea	ShowMemAdrTxt2,a0
	move.l	#6,d1
	jsr	Print
	lea	ShowMemAdrTxt3,a0
	move.l	#6,d1
	jsr	Print
	lea	$0,a0
	jsr	InputHexNum
	cmp.l	#-1,d0
	beq	.exit
	move.l	d0,ShowMemAdr(a6)
	jsr	binhex
	move.l	#2,d1
	jsr	Print
	lea	ShowMemTypeTxt,a0
	move.l	#6,d1
	jsr	Print
.Inploop:
	jsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne	.Inploop
	clr.l	d6				; clear d6 setting what type of read to do
	move.b	keypressed(a6),d0
	bclr	#5,d0				; make result uppercase
	cmp.b	#"B",d0
	beq	.byte
	cmp.b	#"W",d0
	beq	.word
	cmp.b	#"L",d0
	beq	.longword
	beq	.exit
.byte:
	lea	ByteTxt,a0
	jsr	Print
	move.l	#1,d6				; Set byte read
	bra	.next
.word:
	lea	WordTxt,a0
	jsr	Print
	move.l	#2,d6				; Set word read
	bra	.next
.longword:
	lea	LongWordTxt,a0
	jsr	Print
	move.l	#3,d6				; Set longword read
.next:
	lea	NewLineTxt,a0
	jsr	Print
	move.l	#0,d0
	move.l	#10,d1
	jsr	SetPos
	lea	ShowMemTxt,a0
	move.l	#5,d1
	jsr	Print
.loopa:
	move.l	#31,d0
	move.l	#10,d1
	jsr	SetPos
	move.l	ShowMemAdr(a6),a0		; Get memadress and put in a0
	clr.l	d0				; Clear d0 to be sure
	cmp.l	#1,d6				; are we in bytemode
	beq	.bytemode
	cmp.l	#2,d6
	beq	.wordmode
	move.l	(a0),d0
	bra	.modedone
.bytemode:
	move.b	(a0),d0
	bra	.modedone
.wordmode:
	move.w	(a0),d0
.modedone:
	cmp.l	d0,d7				; is it same as old value?
	beq	.same
	move.l	d0,d7
	cmp.l	#1,d6
	beq	.printbyte
	cmp.l	#2,d6
	beq	.printword
	jsr	binhex
	bra	.printdone
.printbyte:
	jsr	binhexbyte
	bra	.printdone
.printword:
	jsr	binhexword	
.printdone:
	move.w	#3,d1
	jsr	Print
.same:
	jsr	GetInput
	cmp.b	#1,BUTTON(a6)
	bne	.loopa
.exit:
	bra	OtherTest	

TF1260:
       jmp    MainMenu

Setup:
       jmp    MainMenu

About:
	jsr	ClearScreen
	lea	AboutTxt,a0
	move.l	#1,d1
	jsr	Print
	lea	AboutTxt2,a0
	move.l	#7,d1
	jsr	Print
	jsr	ClearBuffer
	jsr	WaitPressed
	jsr	WaitReleased
	jmp	MainMenu

SystemInfoTest:
       jsr	InitScreen
       lea	SystemInfoTxt,a0
       move.w	#2,d1
       jsr	Print
       lea	SystemInfoHWTxt,a0
       move.w	#2,d1
       jsr	Print

       jsr	GetHWReg
       jsr	PrintHWReg

       lea	NewLineTxt,a0
       jsr	Print


       lea	WorkTxt,a0
       move.l	#6,d1
       jsr	Print

       move.l	BaseStart(a6),d0			; Get startaddress of chipmem
       jsr	binhex
       move.l	#2,d1
       jsr	Print

       lea	MinusTxt,a0
       jsr	Print


       move.l	BaseEnd(a6),d0			; Get startaddress of chipmem
       jsr	binhex
       move.l	#2,d1
       jsr	Print


       lea	WorkSizeTxt,a0
       move.l	#6,d1
       jsr	Print


       move.l	BaseEnd(a6),d0
       sub.l	BaseStart(a6),d0
       divu	#1024,d0
       swap	d0
       clr.w	d0
       swap	d0

       jsr	bindec
       move.l	#2,d1
       jsr	Print


       lea	KB,a0
       move.l	#2,d1
       jsr	Print

       lea	RomSizeTxt,a0
       move.l	#6,d1
       jsr	Print


       move.l	EndRom,d0
       sub.l	#rom_base,d0
       divu	#1024,d0
       swap	d0
       clr.w	d0
       swap	d0

       jsr	bindec
       move.l	#2,d1
       jsr	Print

       lea	KB,a0
       jsr	Print

       lea	WorkOrderTxt,a0
       move.l	#6,d1
       jsr	Print

       cmp.b	#0,WorkOrder(a6)
       beq	.normalorder


       lea	StartTxt2,a0
       jsr	Print
       bra	.orderdone
       
.normalorder:
       lea	EndTxt2,a0
       jsr	Print

.orderdone:

       lea	ChipTxt,a0
       move.l	#6,d1
       jsr	Print

       move.l	ChipStart(a6),d0
       jsr	binhex
       move.l	#2,d1
       jsr	Print
       lea	MinusTxt,a0
       jsr	Print
       move.l	ChipEnd(a6),d0
       jsr	binhex
       jsr	Print


       lea	FastTxt,a0
       move.l	#6,d1
       jsr	Print


       move.l	FastStart(a6),d0
       jsr	binhex
       move.l	#2,d1
       jsr	Print
       lea	MinusTxt,a0
       jsr	Print
       move.l	FastEnd(a6),d0
       jsr	binhex
       jsr	Print


       jsr	RomChecksum

       lea	.CpuDone,a5
       jsr	DetectCPU
.CpuDone:
       jsr	PrintCPU

       clr.l	d0
       move.b	CPUGen(a6),d0
       cmp.b	#5,d0
       bne	.no060

       lea	FlagTxt,a0
       jsr	Print
       lea	PCRFlagsTxt,a0
       move.l	#2,d1
       jsr	Print
       move.l	PCRReg(a6),d0
       jsr	binstring
       move.l	#3,d1
       jsr	Print



;	move.l	#2,d1
;	lea	EXPERIMENTAL,a0
;	jsr	Print
;	move.l	#DetMMU,$80
;	trap	#0
;	move.l	d6,d0
;	move.l	#3,d1
;	jsr	bindec
;	jsr	Print

.no060


       move.l	#BusError,$8		; This time to a routine that can present more data.
       move.l	#UnimplInst,$2c
       move.l	#Trap,$80

       lea	NewLineTxt,a0
       jsr	Print
       lea	DebugROM,a0
       move.l	#3,d1
       jsr	Print

       cmp.w	#"DG",$2
       bne	.no1114at0
       lea	YES,a0
       move.l	#1,d1
       jsr	Print
       bra	.yes1114at0

.no1114at0:
       lea	NO,a0
       move.l	#2,d1
       jsr	Print
.yes1114at0

       lea	NewLineTxt,a0
       jsr	Print
       lea	DebugROM2,a0
       move.l	#3,d1
       jsr	Print
       cmp.w	#"DG",$f80002
       bne	.no1114atf8
       lea	YES,a0
       move.l	#2,d1
       jsr	Print
       bra	.yes1114atf8

.no1114atf8:
       lea	NO,a0
       move.l	#1,d1
       jsr	Print
.yes1114atf8:
       lea	DebugROM3,a0
       move.l	#3,d1
       jsr	Print

       cmp.w	#$1111,$f00000
       bne	.no1111atf0
       lea	YES,a0
       move.l	#2,d1
       jsr	Print
       bra	.donerom

.no1111atf0:
       lea	NO,a0
       move.l	#2,d1
       jsr	Print
.donerom:	

       lea	StuckButtons,a0
       move.l	#3,d1
       jsr	Print

       clr.l	d7				; Clear d7.  set it to 1 if a button was stuck.

       cmp.b	#0,STUCKP1LMB(a6)
       beq	.nop1lmb
       lea	InitP1LMBtxt,a0
       move.l	#1,d7
       move.l	#1,d1
       jsr	Print
.nop1lmb:
       cmp.b	#0,STUCKP2LMB(a6)
       beq	.nop2lmb
       lea	InitP2LMBtxt,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.nop2lmb:
       cmp.b	#0,STUCKP1RMB(a6)
       beq	.nop1rmb
       lea	InitP1RMBtxt,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.nop1rmb:
       cmp.b	#0,STUCKP2RMB(a6)
       beq	.nop2rmb
       lea	InitP2RMBtxt,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.nop2rmb:
       cmp.b	#0,DISPAULA(a6)
       beq	.nobadpaula
       lea	BadPaulaTXT,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.nobadpaula:
       cmp.b	#0,OVLErr(a6)
       beq	.noovlerr
       lea	OvlErrTxt,a0
       move.l	#1,d1
       move.l	#1,d7
       jsr	Print
.noovlerr:

       cmp.l	#0,d7
       bne	.stuck
       lea	NONE,a0
       move.l	#2,d1
       jsr	Print
.stuck:
       lea	NewLineTxt,a0
       jsr	Print

       jsr	WaitButton
       jmp	MainMenu

PrintHWReg:
       lea	BLTDDATTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	BLTDDAT(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	DMACONRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	DMACONR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	VPOSRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	VPOSR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print
       lea	VHPOSRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	VHPOSR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	DSKDATRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	DSKDATR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	JOY0DATTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	JOY0DAT(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print
       lea	POT0DATTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	POT0DAT(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	POT1DATTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	POT1DAT(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	POTINPTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	POTINP(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print
       lea	SERDATRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	SERDATR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	DSKBYTRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	DSKBYTR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	INTENARTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	INTENAR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print
       lea	INTREQRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	INTREQR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	DENISEIDTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	DENISEID(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	Space3,a0
       jsr	Print
       lea	HHPOSRTxt,a0
       move.w	#7,d1
       jsr	Print
       move.w	HHPOSR(a6),d0
       jsr	binhexword
       move.w	#3,d1
       jsr	Print
       lea	NewLineTxt,a0
       jsr	Print

       rts
