       include "earlymacros.i"
       include "build/srcs/globalvars.i"

       xref	_RomFont
       xref	_putChar
	xref	_printChar
       xref	_setPos
	xref	_clearScreen

       section "generic",code_p
	xdef	GetHWReg
	xdef	Init_Serial
	xdef	SendSerial
	xdef	WaitShort
;	xdef	_bindec
	xdef	bindec
	xdef	oldbindec
	xdef	_binhex
	xdef	binhex
	xdef	CopyMem
	xdef	Print
	xdef	PrintChar
	xdef	PutChar
	xdef	_scrollScreen:
	xdef	ScrollScreen
	xdef	ClearScreen
;	xdef	_SetPos
	xdef	SetPos
	xdef	GetPos
	xdef	RomChecksum
	xdef	DetectCPU
	xdef	PrintCPU
	xdef 	EnglishKey
	xdef	EnglishKeyShifted
	xdef	_ClearBuffer
	xdef	ClearBuffer
	xdef	_GetInput
	xdef	GetInput
	xdef	DefaultVars
	xdef	UnimplInst
	xdef	_binstring
	xdef	binstring
	xdef	BusError
	xdef	WaitButton
	xdef	binhexword
	xdef	Trap
	xdef	WaitLong
	xdef	WaitReleased
	xdef	WaitPressed
	xdef	_GetChip
	xdef	GetChip
	xdef	_binhexbyte
	xdef	binhexbyte
	xdef	RunCode
	xdef	GetMemory
	xdef	ToKB
	xdef	Random
	xdef	DeleteLine
	xdef	DetectMemory
	xdef	InputHexNum
	xdef	StrLen
	xdef	GetMouse
	xdef	_GetSerial
	xdef	_hexbin
	xdef	hexbin
	xdef	InputDecNum
	xdef	hexbytetobin
	xdef	_decbin
	xdef	decbin
	xdef	GetChar
	xdef	GetHex
	xdef	MakePrintable
	xdef	binstringbyte
	xdef	EnableCache
	xdef	_DisableCache
	xdef	DisableCache
	xdef	SameRow
	xdef	DevPrint
	xdef	_PAUSE

WaitShort:					; Wait a short time, aprox 10 rasterlines. (or exact IF we have detected working raster)
	PUSH
	cmp.b	#1,RASTER(a6)			; Check if we have a confirmed working raster
	beq	.raster
	move.l	#$1000,d0			; if now.  lets try to wait some anyway.
	bsr	ReadSerial			; as we have no IRQs..  read serialport just in case
.loop:
	move.b	$bfe001,d1			; Dummyread from slow memory
	move.b	$dff006,d1
	dbf	d0,.loop
	POP
	rts
.raster:
	bsr	ReadSerial			; as we have no IRQs..  read serialport just in case
	move.b	$dff006,d0			; Get what rasterline we are at now
	add.b	#10,d0				; Add 10
.rasterloop:
	cmp.b	$dff006,d0
	bne.s	 .rasterloop
	POP
	rts


;------------------------------------------------------------------------------------------

DetectCPU:					; Detects CPU, FPU etc.
; Code more or less from romanworkshop.blutu.pl/menu/amiasm.htm
; IB!  a5 Contains address to instruction after branch to here. so it can exit there
; if not correct cpu

	move.l	#"TEST",$700			; Put "TEST" into $700
	clr.l	PCRReg(a6)			; Clear PCRReg value
	clr.b	CPU060Rev(a6)			; Clear 060 CPU Rev value
	clr.b	MMU(a6)			; Clear the MMU Flag
	clr.b	ADR24BIT(a6)			; Clear the 24Bit addressmode flag
	cmp.l	#"TEST",$700			; Check if $700 is "TEST" if not.  we assume having memoroissues at lower chipmem.
						; so CPU detection will just fail and crash.  put 680x0 as string of cpu.
	bne	.nochip
	clr.l	$700				; Clear $700

	move.l	#"24AD",$4000700		; Write "24AD" to highmem $700
	cmp.l	#"24AD",$700			; IF memory is readable at $700 instead. we are using a cpu with 24 bit address.
	bne	.no24bit
	move.b	#1,ADR24BIT(a6)
.no24bit:
	moveq	#$0,d1				; Set CPU detected.  begin with "0" as 68000
	move.l	#.notabove68k,$10		; Set illegal instruction to this
	movec	VBR,d3				; Supported by 010+	dc.l	$4e7a3801		;movec VBR,d3	- move VBR to d3
	moveq	#$10,d2
	move.l	d2,a1
	add.l	d3,a1
	move.l	(a1),d2			; take a backup of current value
	lea	.notabove68k,a0
	move.l	a0,(a1)
	moveq	#$1,d1				; Set 68010
	moveq	#$10,d2
	move.l	d2,a1
	add.l	d3,a1
	move.l	(a1),d2
	lea	.cpu3,a0
	move.l	a0,(a1)
	move.l	d3,a2
	moveq	#$2c,d3
	add.l	d3,a2
	move.l	(a2),d3			; Line 111 will happen when illegal instruction happens.
	lea	.above010,a0
	move.l	a0,(a2)
	move.l	a7,a3
	movec	CACR,d1			;dc.l	$4e7a1002		;movec CACR,d1	; 020-060?
	moveq	#$2,d1				; Set 68020
	movec	ITT0,d1			; Supported in 040-060
	moveq	#$4,d1				; Set 68040
	movec	pcr,d1				;dc.l	$4e7a1808		; movec pcr,dq	; Supported by 060
	move.l	d1,PCRReg(a6)			; Store the value for future use
	move.l	d1,d7
	moveq	#$5,d1				; Set 68060
						; OK We have 060, this cpu have some nice features, like the PCR register that shows its config.
						; and we just read it.. so lets.. use it
	movec	PCR,d4
	bclr	#1,d4
	movec	d4,PCR				; Make sure FPU is enabled
	and.l	#$0000ff00,d7
	asr.l	#8,d7
	move.b	d7,CPU060Rev(a6)		; Store the 060 Revisionnumber
	movec	PCR,d4
	swap	d4
	cmp.l	#$0440,d4
	bne	.novamp
						; Ohnooez..  someone is running this on a fake cpu..  a "080"
	moveq	#$6,d1				; Set 68080  YUKK  or well   68FAIL as it is no real stuff...   
.novamp:
.above010:
	move.l	d2,(a1)
	move.l	d3,(a2)
	move.l	a3,a7
.notabove68k:
	move.l	#BusError,$8
	move.l	#IllegalError,$10
	move.l	#UnimplInst,$2c
	move.b	d1,CPUGen(a6)			; Store generation of CPU
	cmp.b	#3,d1
	blt	.lower020			; check if we have 020 or lower then skip next instruction
	clr.b	ADR24BIT(a6)			; Clear the 24Bit addressmode flag
						; as some blizzards seem to screw up my 24 bit adr. detection
.lower020:
	move.l	#0,d1
	move.l	#.chkfpu,$10
	move.l	#$2c,d2
	move.l	d2,a1
	move.l	(a1),d2
	lea	.nofpu,a0
	move.l	a0,(a1)
	move.l	a7,a2
	cmp.b	#0,CPUGen(a6)			; Check if we had 68000
	beq	.nofpu				; YUP!.  we had
	move.l	d2,(a1)
	dc.l	$4e7a3801			; movec VBR,d3	(crash on 68k)
	add.l	d3,a1
	move.l	(a1),d2
	move.l	a0,(a1)
	dc.l	$f201583a			; ftst.b,d1
	dc.w	$f327				; FSAVE
.chkfpu:
	move.l	a2,d3
	sub.l	a7,d3
	moveq	#1,d1				; Set 68881
	cmp.b	#$1c,d3
	beq	.nofpu
	moveq	#2,d1				; Set 68882
	cmp.b	#$3c,d3
	beq	.nofpu
	moveq	#3,d1				; Set 68040
	cmp.b	#4,d3
	beq	.nofpu
	moveq	#4,d1				; Set 68060
	move.l	d2,(a1)
.nofpu:
	move.l	d1,FPU(a6)
	lea	FPUString,a0
	move.b	d1,FPU(a6)
	mulu	#6,d1
	add.l	d1,a0
	move.l	a0,FPUPointer(a6)
	move.l	#BusError,$8			; This time to a routine that can present more data.
	move.l	#IllegalError,$10
	move.l	#UnimplInst,$2c
.mmutest:
	move.b	#4,MMU(a6)			; Lets set a fake value of "MMU Detected"
						; Lets skipthat MMU detection,  it is buggy (now even removed!)
	move.l	#BusError,$8			; This time to a routine that can present more data.
	move.l	#IllegalError,$10
	move.l	#UnimplInst,$2c
	move.l	#Trap,$80			; Restored all exceptions etc touched here
	clr.l	d1
	move.b	CPUGen(a6),d1			; Get CPU Gen from memory, lets find out the real string
	cmp.b	#1,d1				; Check if we had 010
	ble	.cpudone			; if equal or lover than. skip the rest
	cmp.b	#2,d1				; Check if we have a 020
	bne	.no020	
	cmp.b	#0,ADR24BIT(a6)		; check if we have 24bit adr mode
	beq	.full020
	move.b	#2,d1				; Set 68EC20
	bra	.cpudone
.full020:
	move.b	#3,d1				; Set 68020
	bra	.cpudone
.no020:
	cmp.b	#3,d1				; Check if we have a 030
	bne	.no030
	cmp.b	#0,MMU(a6)			; Check if we have a MMU
	bne	.full030
	move.b	#4,d1				; Set 68EC30
	bra	.cpudone	
.full030:
	move.b	#5,d1				; Set 68030
	bra	.cpudone
.no030:
	cmp.b	#4,d1				; Check if we have a 040
	bne	.no040
	cmp.b	#0,MMU(a6)			; Check if we have a MMU
	bne	.mmu040
	move.b	#6,d1				; no mmu, so no FPu so set 68EC40
	bra	.cpudone
.mmu040:
	cmp.b	#0,FPU(a6)			; Check if we have a FPU
	bne	.full040
	move.b	#7,d1				; Set 68LC40
	bra	.cpudone
.full040:
	move.b	#8,d1				; Set 68040
	bra	.cpudone
.no040:
	cmp.b	#5,d1				; Check if we have a 060
	bne	.no060
	cmp.b	#0,MMU(a6)
	bne	.mmu060yes
	move.b	#9,d1				; no mmu no fpu so set 68EC60
	bra	.cpudone
.mmu060yes:
	cmp.b	#0,FPU(a6)
	bne	.full060
	move.b	#10,d1				; Set 68LC60
	cmp.b	#3,CPU060Rev(a6)		; Check if we had rev 3.
	bne.s	.noEC
	move.b	#9,d1				; set 68EC60
.noEC
	bra	.cpudone
.full060:
	move.b	#11,d1				; set 68060
	bra	.cpudone
.no060:					;DQFUQ?  ok something went nuts we did not have ANY CPU?
	cmp.b	#6,d1
	bne	.novampcrap
	move.b	#12,d1
	bra	.cpudone
.novampcrap:
	move.b	#13,d1				;So set 68???
.cpudone:
	;	move.l	#0,d1
	move.b	d1,CPU(a6)			; Store CPU model
	lea	CPUString,a0
	mulu	#7,d1				; Multiply with 7 to point at correct part of string
	add.l	d1,a0
	move.l	a0,CPUPointer(a6)
	jmp	(a5)
.cpu3:
	cmp.b	#2,d1
	bne.w	.notabove68k
	dc.w	$f02f,$6200,$fffe		;Pmove I-PSR 
	moveq	#$3,d1				; Set 68030
	bra	.notabove68k
.nochip:
	move.b	#0,FPU(a6)
	move.b	#0,MMU(a6)
	move.b	#0,CPUGen(a6)
	clr.l	d1
	move.b	#13,d1				; set 68060
	bra	.cpudone
;------------------------------------------------------------------------------------------

SSPError:
	move.l	a0,DebugA0(a6)		; Store a0 to DebugA0 so we have it saved. as next line will overwrite it
	lea	SSPErrorTxt,a0
	bra	ErrorScreen

BusError:
	move.l	a0,DebugA0(a6)
	lea	BusErrorTxt,a0
	bra	ErrorScreen

AddressError:
	move.l	a0,DebugA0(a6)
	lea	AddressErrorTxt,a0
	bra	ErrorScreen

IllegalError:
	move.l	a0,DebugA0(a6)
	lea	IllegalErrorTxt,a0
	bra	ErrorScreen

DivByZero:
	move.l	a0,DebugA0(a6)
	lea	DivByZeroTxt,a0
	bra	ErrorScreen

ChkInst:
	move.l	a0,DebugA0(a6)
	lea	ChkInstTxt,a0
	bra	ErrorScreen

TrapV:
	move.l	a0,DebugA0(a6)
	lea	TrapVTxt,a0
	bra	ErrorScreen

PrivViol:
	move.l	a0,DebugA0(a6)
	lea	PrivViolTxt,a0
	bra	ErrorScreen

Trace:
	move.l	a0,DebugA0(a6)
	lea	TraceTxt,a0
	bra	ErrorScreen

UnimplInst:
	move.l	a0,DebugA0(a6)
	lea	UnImplInstrTxt,a0
	bra	ErrorScreen
	
Trap:
	move.l	a0,DebugA0(a6)
	lea	TrapTxt,a0
	bra	ErrorScreen

oldbindec:					; Converts a binary number to decimal textstring
	; this is my old bin->dec code. it is still here as I need a bin-dec
	; convertion done for ANSI stuff in my print routine. and that can
	; overwrite other data when printing. so to separate the different things
	; why not have this left.  this only handles word and no longwords...
	;
	; INDATA:
	;	D0 = binary number (word)
	; OUTDATA:
	;	A0 = Pointer to "bindecoutput" contining the string

	PUSH
	lea	bindecoutput(a6),a0
	move.b	#$20,d1
	tst.w	d0
	bpl	.notneg
	move.b	#$2d,d1
	neg.w	d0
	clr.l	d3
.notneg:
	move.b	d1,(a0)
	add.l	#5,a0
	move.w	#4,d1
.loop:
	ext.l	d0
	divs	#10,d0
	swap	d0
	move.b	d0,-(a0)
	add.b	#$30,(a0)
	swap	d0
	dbra	d1,.loop
	clr.l	d0
.scroll:
	move.w	#6,d2
	lea	bindecoutput(a6),a0
	lea	bindecoutput+1(a6),a1
	move.b	(a0),d1
	cmp.b	#"0",d1
	bne.s	.stop
	add.b	#1,d0
	cmp.b	#5,d0
	beq.s	.stop
.scroll1:
	move.b	(a1)+,(a0)+
	dbf	d2,.scroll1
	bra.s	.scroll
.stop:
	POP
	lea	bindecoutput(a6),a0
	rts



	; *********************************************
	;
	; $VER:	Binary2Decimal.s 0.2b (22.12.15)
	;
	; Author: 	Highpuff
	; Orginal code: Ludis Langens
	;
	; In:	D0.L = Hex / Binary
	;
	; Out:	A0.L = Ptr to null-terminated String
	;	D0.L = String Length (Zero if null on input)
	;
	; *********************************************


b2dNegative	equ	0			; 0 = Only Positive numbers
						; 1 = Both Positive / Negative numbers

	; *********************************************

_bindec:
	bsr	bindec
	move.l	a0,d0
	rts
bindec:		movem.l	d1-d5/a1,-(sp)

		moveq	#0,d1			; Clear D1/2/3/4/5
		moveq	#0,d2
		moveq	#0,d3
		moveq	#0,d4
		moveq	#0,d5

		lea.l	b2dString+12(a6),a0
		movem.l	d1-d3,-(a0)	; Clear String buffer

		neg.l	d0			; D0.L ! D0.L = 0?
		bne	.notZero		; If NOT True, Move on...
		move.b	#$30,(a0)		; Put a ASCII Zero in buffer
		moveq	#1,d0			; Set Length to 1
		bra	.b2dExit		; Exit	
		
.notZero:	neg.l	d0			; Restore D0.L

	IF b2dNegative			; Is b2dNegative True?

		move.l	d0,d1			; D1.L = D0.L
		swap	d1			; Swap Upper Word with Lower Word
		rol.w	#1,d1			; MSB  = First byte
		btst	#0,d1			; Negative?
		beq	.notNegative		; If not, jump to .notNegative
		move.b	#$2d,(a0)+		; Add a '-' to the String
		neg.l	d0			; Make D0.L positive
.notNegative:	moveq	#0,d1			; Clear D1 after use

	endc

.lftAlign:	addx.l	d0,d0			; D0.L = D0.L << 1
		bcc.s	.lftAlign		; Until CC is set (all trailing zeros are gone)

.b2dLoop:	abcd.b	d1,d1			; xy00000000
		abcd.b	d2,d2			; 00xy000000
		abcd.b	d3,d3			; 0000xy0000
		abcd.b	d4,d4			; 000000xy00
		abcd.b	d5,d5			; 00000000xy
		add.l	d0,d0			; D0.L = D0.L << 1
		bne.s	.b2dLoop		; Loop until D0.L = 0
	
		; Line up the 5x Bytes

		lea.l	b2dTemp(a6),a1	; A1.L = b2dTemp Ptr
		move.b	d5,(a1)		; b2dTemp = d5.xx.xx.xx.xx
		move.b	d4,1(a1)		; b2dTemp = d5.d4.xx.xx.xx
		move.b	d3,2(a1)		; b2dTemp = d5.d4.d3.xx.xx
		move.b	d2,3(a1)		; b2dTemp = d5.d4.d3.d2.xx
		move.b	d1,4(a1)		; b2dTemp = d5.d4.d3.d2.d1


		; Convert Nibble to Byte
		
		moveq	#5-1,d5		; 5 bytes (10 Bibbles) to check
.dec2ASCII:	move.b	(a1)+,d1		; D1.W = 00xy
		ror.w	#4,d1			; D1.W = y00x
		move.b	d1,(a0)+		; Save ASCII
		sub.b	d1,d1			; D1.B = 00
		rol.w	#4,d1			; D1.W = 000y
		move.b	d1,(a0)+		; Save ASCII
		dbf	d5,.dec2ASCII		; Loop until done...

		sub.l	#10,a0			; Point to first byte (keep "-" if it exists)
		move.l	a0,a1

		; Find where the numbers start and trim it...

		moveq	#10-1,d5		; 10 Bytes total to check
.trimZeros:	move.b	(a0),d0		; Move byte to D0.B
		bne.s	.trimSkip		; Not Zero? Exit loop
		add.l	#1,a0			; Next Character Byte
		dbf	d5,.trimZeros		; Loop
.trimSkip:	move.b	(a0)+,d0		; Move Number to D0.B
		add.b	#$30,d0		; Add ASCII Offset to D0.B
		move.b	d0,(a1)+		; Move to buffer
		dbf	d5,.trimSkip		; Loop

		; Get string length

		move.l	a1,d0			; D0.L = EOF b2dString
		lea.l	b2dString(a6),a0	; A0.L = SOF b2dString
		sub.l	a0,d0			; D0.L = b2dString.Length
		move.b	#0,(a0,d0)
.b2dExit:	movem.l	(sp)+,d1-d5/a1
		rts

_binhex:
	bsr	binhex
	move.l	a0,d0
	rts

binhex:					; Converts a binary number to hex
	; INDATA:
	;	D0 = binary number
	; OUTDATA:
	;	A0 = Pointer to "binhexoutput" containing the string
	PUSH
	move.l	d0,-(sp)			; value (arg 1)
	jsr	_binHex
	addq.l	#4,sp				; clean up stack
	POP
	lea	binhexoutput(a6),a0		; Set a0 after POP so it isn't overwritten
	rts

ErrorScreen:
	move.w	(a7),DebSR(a6)		; Store what was in stack as first word is a copy of SR at crash
	move.l	2(a7),DebPC(a6)		; and next longword is PC
	move.l	d0,DebD0(a6)			; first store everything in registers
	move.l	d1,DebD1(a6)			; and for visability etc.. I do several move instead of movem. dunno why :)
	move.l	d2,DebD2(a6)
	move.l	d3,DebD3(a6)
	move.l	d4,DebD4(a6)
	move.l	d5,DebD5(a6)
	move.l	d6,DebD6(a6)
	move.l	d7,DebD7(a6)
	move.l	a0,DebA0(a6)
	move.l	a1,DebA1(a6)
	move.l	a2,DebA2(a6)
	move.l	a3,DebA3(a6)
	move.l	a4,DebA4(a6)
	move.l	a5,DebA5(a6)
	move.l	a6,DebA6(a6)
	move.l	a7,DebA7(a6)			; OK now everything is stored.
	bsr	ClearScreen
	move.l	d1,DebugD1(a6)
	move.l	#1,d1
	bsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	CrashTxt,a0
	bsr	Print
	move.l	DebugA0(a6),a0
	move.l	DebugD1(a6),d1
	bsr	DebugScreen

	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print

	lea	AnyKeyMouseTxt,a0
	move.l	#5,d1
	bsr	Print

	bsr	ClearBuffer

	bsr	WaitButton
	bra	MainMenu

DebugScreen:					; This dumps out registers..
	PUSH
	clr.l	d0
	move.l	#3,d1
	jsr	SetPos
	lea	DebugTxt,a0
	move.l	#3,d1
	jsr	Print
	clr.l	d0
	move.l	#3,d1
	jsr	SetPos
	move.l	DebD0(a6),d0
	jsr	binhex
	move.l	#2,d1
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebD1(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebD2(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebD3(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebD4(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebD5(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebD6(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebD7(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebA0(a6),d0
	jsr	binhex
	move.l	#3,d1
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebA1(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebA2(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebA3(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebA4(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebA5(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebA6(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	move.l	DebA7(a6),d0
	jsr	binhex
	jsr	Print
	lea	SPACE,a0
	jsr	Print
	clr.l	d0
	lea	NewLineTxt,a0
	jsr	Print
	lea	DebugSR,a0
	jsr	Print
	clr.l	d0
	move.w	DebSR(a6),d0
	jsr	binhexword
	move.l	#2,d1
	jsr	Print
	lea	DebugADR,a0
	move.l	#3,d1
	jsr	Print
	clr.l	d0
	move.l	DebPC(a6),d0
	move.l	d0,a4
	jsr	binhex
	move.l	#2,d1
	jsr	Print
	lea	DebugContent,a0
	move.l	#3,d1
	jsr	Print
	move.l	#19,d6
	clr.l	d5
.contentloop2:
	clr.l	d0
	move.b	(a4)+,d0
	jsr	binhexbyte
	jsr	Print
	add.b	#1,d5
	cmp.b	#4,d5				; 4th byte?
	bne	.not42
	move.b	#" ",d0
	jsr	PrintChar

	clr.l	d5
.not42:
	dbf	d6,.contentloop2
	add.l	#4,a5
	lea	NewLineTxt,a0
	jsr	Print

	lea	StackTxt,a0
	move.l	#1,d1
	jsr	Print

	move.l	#14,d7
	move.l	DebA7(a6),a3
	add.l	#16,a3
.stackloop:
	move.l	(a3)+,d0
	bsr	binhex
	move.l	#6,d1
	jsr	Print
	lea	SPACE,a0
	jsr	Print
.nostacknewline:
	dbf	d7,.stackloop

	clr.l	d7
	lea	$64,a5				; Level1 pointer
.irqloop:
	add.b	#1,d7
	cmp.b	#8,d7
	beq	.endloop
	lea	NewLineTxt,a0
	jsr	Print
	lea	DebugIRQ,a0
	move.l	#3,d1
	jsr	Print
	move.l	d7,d0
	jsr	bindec
	move.l	#3,d1
	jsr	Print
	lea	DebugIRQPoint,a0
	jsr	Print
	move.l	(a5),d0			; Get where IRQ points to
	move.l	d0,a4				; Store a copy of it in A4, to be able to print content
	jsr	binhex
	jsr	Print
	lea	DebugContent,a0
	jsr	Print
	move.l	#15,d6
	clr.l	d5
.contentloop:
	clr.l	d0
	move.b	(a4)+,d0
	jsr	binhexbyte
	jsr	Print
	add.b	#1,d5
	cmp.b	#4,d5				; 4th byte?
	bne	.not4
	move.b	#" ",d0
	jsr	PrintChar
	clr.l	d5
.not4:
	dbf	d6,.contentloop
	add.l	#4,a5
	bra	.irqloop
.endloop:
	lea	NewLineTxt,a0
	jsr	Print
	lea	NewLineTxt,a0
	jsr	Print
	lea	DebugROM,a0
	jsr	Print
	cmp.w	#$1114,$0
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
	cmp.w	#$1114,$f80000
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
	lea	NewLineTxt,a0
	jsr	Print
	bsr	PrintCPU
	lea	DebugPWR,a0
	move.l	#3,d1
	jsr	Print
	move.l	PowerONStatus(a6),d0
	jsr	binstring
	jsr	Print
	lea	BuildTxt,a0
	move.l	#2,d1
	jsr	Print
	lea	BuiltdateTxt,a0
	move.l	#2,d1
	jsr	Print
	POP
	rts

_ClearBuffer:
ClearBuffer:
	move.l	#20,d7
.loop
	bsr	GetInput
	dbf	d7,.loop
	clr.b	SerBufLen(a6)
	bsr	ClearInput
	move.b	#0,SerBufLen(a6)		; Check if we have a serialbuffer, if not, just exit
	rts

WaitButton:					; Waits until a button is pressed AND released
	bsr	WaitPressed
	bsr	WaitReleased
	rts

WaitPressed:					; Waits until some "button" is pressed
	clr.l	d7				; Clear d7 that is used for a timeout counter
.loop:
	add.l	#1,d7				; Add 1 to the timout counter
	cmp.l	#$ffff,d7			; did we count for a lot of times? well then there is a timeout
	beq	.timeout
	bsr	GetInput			; get inputdata
	cmp.b	#1,BUTTON(a6)			; check if any button was pressed.
	bne	.loop				; nope. lets loop
	rts
.timeout:
	rts
	move.b	P1LMB(a6),STUCKP1LMB(a6)	; ok we had a timeout. so we GUESS a port is stuck.
	move.b	P2LMB(a6),STUCKP2LMB(a6)	; if we just simply copy the status of all keys
	move.b	P1LMB(a6),STUCKP1LMB(a6)	; to the STUCK version. we will disable all stuck ports
	move.b	P2RMB(a6),STUCKP2RMB(a6)
	move.b	P1MMB(a6),STUCKP1MMB(a6)
	move.b	P2MMB(a6),STUCKP2MMB(a6)
	rts

WaitReleased:					; Waits until some "button" is unreleased
	clr.l	d7				; Clear d7 that is used for a timeout counter
.loop:
	move.b	$dff006,$dff180
	add.l	#1,d7				; Add 1 to the timout counter
	cmp.l	#$ffff,d7			; did we count for a lot of times? well then there is a timeout
	beq	.timeout
	bsr	GetInput			; get inputdata
	cmp.b	#0,BUTTON(a6)			; check if any button was pressed.
	bne	.loop				; nope. lets loop
	rts
.timeout:
	move.b	P1LMB(a6),STUCKP1LMB(a6)	; ok we had a timeout. so we GUESS a port is stuck.
	move.b	P2LMB(a6),STUCKP2LMB(a6)	; if we just simply copy the status of all keys
	move.b	P1LMB(a6),STUCKP1LMB(a6)	; to the STUCK version. we will disable all stuck ports
	move.b	P2RMB(a6),STUCKP2RMB(a6)
	move.b	P1MMB(a6),STUCKP1MMB(a6)
	move.b	P2MMB(a6),STUCKP2MMB(a6)
	rts

_binhexbyte:
	bsr	binhexbyte
	move.l	a0,d0
	rts
binhexbyte:
		; Same as binhex but only for one byte.
	PUSH
	lea	hextab,a1			; location of hexstring source
	lea	binhexoutput(a6),a0
	clr.l	(a0)
	clr.l	4(a0)
	clr.w	8(a0)				; Clear the area first.
	add.l	#9,a0
	move.l	#1,d1
.loop:
	move.l	d0,d2
	and.l	#15,d2
	move.b	(a1,d2),-(a0)
	lsr.l	#4,d0
	dbra	d1,.loop
	POP
	lea	binhexoutput+7(a6),a0
	rts

binhexword:
		; Same as binhex but only for one word.
	PUSH
	lea	hextab,a1			; location of hexstring source
	lea	binhexoutput(a6),a0
	clr.l	(a0)
	clr.l	4(a0)
	clr.w	8(a0)				; Clear the area first.
	add.l	#9,a0
	move.l	#3,d1
.loop:
	move.l	d0,d2
	and.l	#15,d2
	move.b	(a1,d2),-(a0)
	lsr.l	#4,d0
	dbra	d1,.loop
	POP
	lea	binhexoutput+4(a6),a0
	move.b	#"$",(a0)
	rts
		; Same as binhex but only for one byte.

_binstring:
	bsr	binstring
	move.l	a0,d0
	rts

binstring:
	; Converts a binary number (longword) to binary string
	; INDATA:
	;	D0 = binary number
	; OUTDATA:
	;	A0 = Poiner to outputstring
	PUSH
	move.l	#31,d7
	lea	binstringoutput(a6),a0
.loop:
	btst	d7,d0
	beq	.notset
	move.b	#"1",(a0)+
	bra	.done
.notset:
	move.b	#"0",(a0)+
.done:
	dbf	d7,.loop
	move.b	#0,(a0)

	POP
	lea	binstringoutput(a6),a0
	rts
_GetInput:
GetInput:
						; Check inputsignals and return actions.
						; in: none
						; out: d0 - messageflags.
						;	bits:
						;		0 = Mouse moved
						;		1 = Mouse button
						;		2 = Keyboard action happened
						;		3 = Serial action happened
	PUSH
	clr.l	d0				; Clear d0..
	bsr	ClearInput
	clr.l	d1
	clr.l	d0
	bsr	GetMouseData
	move.l	d0,d1
	cmp.b	#0,DISPAULA(a6)
	bne	.paulabad
	bsr	GetCharSerial
	cmp.b	#0,d0
	beq	.noserial
.paulabad:
	move.b	d0,GetCharData(a6)
	move.b	#1,BUTTON(a6)
	move.l	d1,d0
	bset	#3,d1
.noserial:
	move.l	d1,d0
.getkey:
	move.l	d0,d1
	cmp.b	#1,OVLErr(a6)			; If we had OVL error, CIA is most likly broke. ignore keyboard
	beq	.noovl
	bsr	GetCharKey
.noovl:
	cmp	#0,d0
	beq	.nokey
	move.b	#1,BUTTON(a6)
	move.b	d0,GetCharData(a6)
	move.l	d1,d0
	bset	#2,d0
	bra	.exit
.nokey:
	move.l	d1,d0	
.exit:
	move.l	d0,InputRegister(a6)
	POP
	move.l	InputRegister(a6),d0
	rts

_GetSerial:
GetSerial:					; Reads serialport and returns first char in buffer.
	cmp.w	#0,SerialSpeed(a6)		; if serialport is disabled.  skip all serial stuff
	beq	.exit
	cmp.w	#5,SerialSpeed(a6)
	beq	.exit
	move.b	#0,SerData(a6)
	bsr	ReadSerial
	cmp.b	#0,SerBufLen(a6)		; Check if we have a serialbuffer, if not, just exit
	beq	.exit
						; OK, we do have a serialbuffer, so return first char in buffer
	clr.l	d6
	move.b	SerBufLen(a6),d6
	lea	SerBuf(a6),a5
	move.b	(a5),Serial(a6)		; Read char in the buffer and put it to "Serial" that is the output variable
	PUSH
	move.l	#$fe,d6
.loop:
	move.b	1(a5),(a5)+
	dbf	d6,.loop
	sub.b	#1,SerBufLen(a6)
	move.b	#0,(a5)			; Clear the last byte in the buffer
	POP
	bset	#3,d0				; Mark that we had a serialevent
	rts
.exit:
	rts

ClearInput:
	move.w	#0,CurAddX(a6)
	move.w	#0,CurSubX(a6)
	move.w	#0,CurAddY(a6)
	move.w	#0,CurSubY(a6)
	move.b	#0,MOUSE(a6)
	move.b	#0,BUTTON(a6)			; Clear the generic "Button" variable
	move.b	#0,LMB(a6)			; I use move as clr actually does a read first
	move.b	#0,P1LMB(a6)
	move.b	#0,P2LMB(a6)
	move.b	#0,RMB(a6)
	move.b	#0,P1RMB(a6)
	move.b	#0,P2RMB(a6)
	move.b	#0,key(a6)
	move.b	#0,Serial(a6)
	move.b	#0,GetCharData(a6)
	move.b	#0,MBUTTON(a6)
	rts


GetMouseData:
		; Get data from mouse.. ANY port.
	move.w	$dff016,d1			; Read POTINP to d1
	and.w	#$fe,d1			; mask out bit 0
	cmp.b	#0,d1				; ok  if d1 is 0 we should have a working paula as bit 1-7 always shold be 0
						; but a bad paula can give random numbers. so DiagROM messes up presses etc.
						; so DISABLE all paulachecks.
	beq	.paulaok
	move.b	#1,DISPAULA(a6)		; ok it was not 0.  so lets disable paulastuff
.paulaok:
	cmp.b	#0,DISPAULA(a6)
	bne	.paulabad
	;First handle Y position
	bsr	.CheckButton
	move.l	d0,d4				; Store d0 in d4 temporary
	move.b	$dff00a,d2
	move.b	OldMouse1Y(a6),d1
	cmp.b	d1,d2				; Check to old Y pos at mouse 1. if differs, mouse is moved.
	bne	.Mouse1YMove
.Check2Y:
	move.b	$dff00c,d2
	move.b	OldMouse2Y(a6),d1
	cmp.b	d1,d2
	bne	.Mouse2YMove
.CheckX:
	move.b	$dff00b,d2
	move.b	OldMouse1X(a6),d1
	cmp.b	d1,d2
	bne.w	.Mouse1XMove
.Check2X:
	move.b	$dff00d,d2
	move.b	OldMouse2X(a6),d1
	cmp.b	d1,d2
	bne.w	.Mouse2XMove
		; ok now we have a raw "mouse" data from either port.
		; but maybe time to convert it to a real X, Y cursor instead.
		; that goes between 0 and 640 on X, and 0 and 512 on Y.
	clr.l	d0
	clr.l	d1
	move.b	MouseX(a6),d0			; Store X pos in d0
	move.b	OldMouseX(a6),d1		; Store old X pos value in d1
	cmp.b	d1,d0				; Compare them to check if we have mousemovement
	bne	.XMove				; We have mousemovements in the X Axis
.CheckY:
	clr.l	d0
	clr.l	d1
	move.b	MouseY(a6),d0			; Store Y pos in d0
	move.b	OldMouseY(a6),d1		; Store old Y pos value in d1
	cmp.b	d1,d0				; Compare them to check if we have mousemovement
	bne	.YMove				; We have mousemovements in the Y Axis
.DoneM:
	move.l	d4,d0				; Restore d0 to keep flags
.paulabad:
	rts
.XMove:
	bset	#0,d4				; Set flag that we have mousemovements, d4 is temporary
	move.b	#1,MOUSE(a6)
	move.b	d0,OldMouseX(a6)		; Store current to old.
						; OK , we have a movement, but what direction?
	bsr	.GetMouseDir
	cmp.b	#1,d1				; Check what direction
	beq	.backX
	add.w	d0,CurX(a6)
	move.w	d0,CurAddX(a6)
	cmp.w	#640,CurX(a6)
	bge	.highX
	bra	.DoneX
.highX:
	move	#640,CurX(a6)
	bra	.DoneX
.backX:
	sub.w	d0,CurX(a6)
	move.w	d0,CurSubX(a6)
	cmp.w	#0,CurX(a6)
	blt	.MaxX
	bra	.DoneX
.MaxX:
	move.w	#0,CurX(a6)
	bra	.DoneX
.DoneX:
	bra	.CheckY
.YMove:
	bset	#0,d4				; Set flag that we have mousemovements, d4 is temporary
	move.b	#1,MOUSE(a6)
	move.b	d0,OldMouseY(a6)		; Store current to old.
						; OK , we have a movement, but what direction?
	bsr	.GetMouseDir
	cmp.b	#1,d1				; Check what direction
	beq	.backY
	add.w	d0,CurY(a6)
	move.w	d0,CurAddY(a6)
	cmp.w	#512,CurY(a6)
	bge	.highY
	bra	.DoneY
.highY:
	move	#512,CurY(a6)
	bra	.DoneY
.backY:
	move.w	d0,CurSubY(a6)
	sub.w	d0,CurY(a6)
	cmp.w	#0,CurY(a6)
	blt	.MaxY
	bra	.DoneY
.MaxY:
	move.w	#0,CurY(a6)
	bra	.DoneY
.DoneY:
	bra	.DoneM
.GetMouseDir:
		; INDATA:
		;	D0 = Old pos
		;	D1 = New pos
		; OUTDATA:
		;	D0 = Number of steps
		;	D1 = if 0 = "backwards"
	move.l	d0,d2
	move.l	d1,d3				; Store values
	cmp.b	d0,d1				; Check what direction mousemovement is
	blt	.Lower				; ok we have a lower value
	sub.b	d0,d1				; Calculate how big the movement was.
	move.l	d1,d0				; put it in d0
	move.b	#1,d1				; Mark as "forward" movement
	cmp.w	#128,d0				; Check if we had a BIG movement.
	bge	.highadd			; yes.  so it must be the OPPOSITE direction instead
	rts
.highadd:
	move.b	#255,d1
	sub.b	d1,d0
	clr.b	d1
	rts	
.Lower:
	sub.b	d1,d0
	clr.l	d1				; Mark as "backward"
	cmp.w	#128,d0
	bge	.highsub
	rts	
.highsub:
	move.b	#255,d1
	sub.b	d1,d0
	move.b	#1,d1
	rts
.Mouse2XMove:
	move.b	d2,OldMouse2X(a6)
	sub.b	d2,d1
	sub.b	d1,MouseX(a6)
	bset	#0,d0
	bra	.CheckButton
.Mouse1XMove:
	move.b	d2,OldMouse1X(a6)
	sub.b	d2,d1
	sub.b	d1,MouseX(a6)
	bset	#0,d0
	bra	.Check2X
.Mouse1YMove:
	move.b	d2,OldMouse1Y(a6)
	sub.b	d2,d1				; Get delta from old value
	sub.b	d1,MouseY(a6)
	bset	#0,d0
	bra	.Check2Y
.Mouse2YMove:
	move.b	d2,OldMouse2Y(a6)
	sub.b	d2,d1
	sub.b	d1,MouseY(a6)
	bset	#0,d0
	bra	.CheckX	
.CheckButton:					; X and Y are now checked, lets check the buttons.
	cmp.b	#0,STUCKP1LMB(a6)		; Check if button was marked as stuck, if so. skip it
	bne	.nolmb1
	btst	#6,$bfe001			; Check LMB
	beq	.P1LMB
.nolmb1:
	cmp.b	#0,STUCKP2LMB(a6)
	bne	.CheckRight
	btst	#7,$bfe001
	beq	.P2LMB
.CheckRight:
	cmp.b	#0,STUCKP1RMB(a6)
	bne	.normb1
	btst	#10,$dff016			; Check RMB port 1
	beq	.P1RMB
.normb1:
	cmp.b	#0,STUCKP2RMB(a6)
	bne	.CheckMiddle
	btst	#14,$dff016			; Check RMB port 2
	beq	.P2RMB
.CheckMiddle
	cmp.b	#0,STUCKP1MMB(a6)
	bne	.nommb1
	btst	#8,$dff016			; Check MMB
	beq	.MMB
.nommb1:
	cmp.b	#0,STUCKP2MMB(a6)
	bne	.Done
	btst	#12,$dff016
	beq	.MMB
.Done:
	rts	

.P1LMB:
	move.b	#1,P1LMB(a6)
	bra	.LMB
.P2LMB:
	move.b	#1,P2LMB(a6)
.LMB:
	move.b	#1,BUTTON(a6)
	move.b	#1,MBUTTON(a6)
	move.b	#1,LMB(a6)			; Mark LMB as pressed
	bset	#1,d0
	bra	.CheckRight
.P1RMB:
	move.b	#1,P1RMB(a6)
	bra	.RMB
.P2RMB:
	move.b	#1,P2RMB(a6)
.RMB:
	move.b	#1,MBUTTON(a6)
	move.b	#1,BUTTON(a6)
	move.b	#1,RMB(a6)
	bset	#1,d0
	rts
.MMB:
	move.b	#1,MBUTTON(a6)
	move.b	#1,BUTTON(a6)
	move.b	#1,MMB(a6)
	bset	#1,d0
	rts

GetChar:					; Reads keyboard and serialport and returns the value in D0
	bsr	GetCharKey
	cmp.b	#0,d0
	bne	.noserial
	bsr	GetCharSerial
.noserial:
	move.b	d0,GetCharData(a6)
	rts

GetCharSerial:
	PUSH
	clr.b	Serial(a6)
	bsr	GetSerial			; Read Serialport
	cmp.b	#1,SerAnsiFlag(a6)		; Are we in ANSI mode?
	beq	.ansimode
	POP
	clr.l	d0
	move.b	Serial(a6),d0			; Return what was in serial, if nothing it will be 0 (nothing happend)
	cmp.b	#$1b,d0			; is it ESC? if so. we might be in ANSI mode.
	beq	.ansion
	cmp.b	#$d,d0				; is it a linefeed?
	bne	.nolf
	move.b	#$a,d0				; convert it to CR
.nolf:
	rts
.ansion:
	move.b	#1,SerAnsiFlag(a6)		; Set flag that we are in ANSImode
	move.w	#0,SerAnsiChecks(a6)		; Clear ansicheck variable
	move.b	#0,d0				; return that nothing was recieved
	move.b	d0,Serial(a6)
	rts
.ansimode:
	move.b	Serial(a6),d0			; Load the serialdata to d0
	clr.l	d1				; clear d1 to make sure we do not get crapdata
	cmp.b	#0,d0				; did we get a 0 from serialport?
	beq	.sernull			; if so.. handle it
	cmp.b	#$1b,d0
	beq	.sernull
	cmp.b	#32,d0				; Strip away all nonascii chars
	blt	.noascii
	cmp.b	#1,SerAnsi35Flag(a6)
	beq	.ansimode35on
	cmp.b	#1,SerAnsi36Flag(a6)
	beq	.ansimode36on
	cmp.b	#$38,d0
	beq	.pgup
	cmp.b	#$36,d0
	beq	.pgdown
	cmp.b	#$41,d0			;UP
	beq	.up
	cmp.b	#$42,d0			;DOWN
	beq	.down
	cmp.b	#$43,d0			;RIGHT
	beq	.right
	cmp.b	#$44,d0			;LEFT
	beq	.left
	cmp.b	#$36,d0			;possible pgdwn
	beq	.ansimode36
	cmp.b	#$35,d0
	beq	.ansimode35			;possible pgup
.noascii:
	clr.b	d0
	bra	.ansiexit
.ansichar
	move.b	#0,SerAnsiFlag(a6)
	bra	.ansiexit
.ansimode35:
	move.b	#1,SerAnsi35Flag(a6)
	bra	.ansiexit
.ansimode36:
	move.b	#1,SerAnsi36Flag(a6)
	bra	.ansiexit
.ansimode35on:
	clr.b	SerAnsi35Flag(a6)
	cmp.b	#$7e,d0			; we have pgup
	beq	.pgup
	clr.b	SerAnsiFlag(a6)
	rts
.ansimode36on:
	clr.b	SerAnsi36Flag(a6)
	cmp.b	#$7e,d0			; we have pgdown
	beq	.pgdown
	clr.b	SerAnsiFlag(a6)
	rts
.pgup:
	clr.b	SerAnsiFlag(a6)
	move.b	#1,skipnextkey(a6)
	move.l	#1,d0
	bra	.ansiexit
.pgdown:
	clr.b	SerAnsiFlag(a6)
	move.b	#1,skipnextkey(a6)
	move.l	#2,d0
	bra	.ansiexit
.up:
	clr.b	SerAnsiFlag(a6)
	move.l	#30,d0
	bra	.ansiexit
.down:
	clr.b	SerAnsiFlag(a6)
	move.l	#31,d0
	bra	.ansiexit
.right:
	clr.b	SerAnsiFlag(a6)
	move.l	#28,d0
	bra	.ansiexit
.left:
	clr.b	SerAnsiFlag(a6)
	move.l	#29,d0
	bra	.ansiexit
.exitchar:
	POP
	clr.l	d0
	rts
.nochar:
	move.b	#$1b,d0
	move.b	#0,SerAnsiFlag(a6)
.ansiexit:
	move.b 	d0,Serial(a6)
.ansidone:
	POP
	clr.l	d0
	move.b	Serial(a6),d0			; Return what was in serial, if nothing it will be 0 (nothing happend)
	rts
.sernull:
	clr.b	d0				; OK we had a binary 0 as result
	add.w	#1,SerAnsiChecks(a6)		; add number of times we run through this
	cmp.w	#$f,SerAnsiChecks(a6)		; is it max?
	bne	.ansiexit			; if not. just exit with 0
	TOGGLEPWRLED
	move.w	#0,SerAnsiChecks(a6)		; ok we had too many checks. guess nothing happened. so exit with an ESC.
	bra	.nochar		

GetCharKey:
		; Keyboard have priority
	PUSH
	move.b	#0,keyresult(a6)
	bsr	GetKey				; Read keyboard
	cmp.b	#1,keynew(a6)			; Did we have a new keypress on the keyboard?
	bne	.no				; no, do serialstuff instead
	lea	keymap(a6),a0
	move.l	(a0),a0			; Set wanted keymap.
	bsr	ConvertKey			; Convert keyscan to actual ASCII
	cmp.b	#0,skipnextkey(a6)		; Check if skipnextkey is set
	bne	.skipnextkey
.no:
	POP
	clr.l	d0
	move.b	keyresult(a6),d0
	rts
.skipnextkey:					; ok we are instructed to simply skip this keypress.
	move.b	#0,BUTTON(a6)
	move.b	#0,keyresult(a6)
	move.b	#0,skipnextkey(a6)
	beq	.no

GetKey:
	PUSH					; Read keyboard
	clr.b	keynew(a6)			; Clear keynew variable, will be set if we have a new keypress
	clr.b	keyup(a6)
	clr.b	keydown(a6)
	btst	#3,$bfed01			; Check if keyboard data ready (SP bit in ICR)
	beq	.nodata				; No new data - exit early
	move.b	$bfec01,d0			; Read keyboard
	move.b	d0,scancode(a6)		; Store the original scancode
	ror.b	#1,d0
	not.b	d0
	move.b	d0,key(a6)			; after rotates etc, store the keycode
	btst	#7,d0				; Test if key is up or down
	beq	.down
	move.b	#1,keyup(a6)			; Set that a key was released
	clr.b	keystatus(a6)
	bra	.handshake
.down:
	move.b	#1,keydown(a6)		; Set that a key was pressed
	move.b	#1,keystatus(a6)
	move.b	#1,keynew(a6)
.handshake:
	bset	#6,$bfee01			; Set handshakebit
	bsr	WaitShort			; Wait for keyboard to see handshake
	bsr	WaitShort
	bclr	#6,$bfee01			; Clear the handshakebit
						; OK.  we have read the buffer and also cleared it. Lets handle it.
	bclr	#7,d0				; We clear the up/down bit, so we know what key we handled
	cmp.b	#$60,d0
	beq	.shift
	cmp.b	#$61,d0
	beq	.shift
	cmp.b	#$62,d0
	beq	.capsshift			; Now we have handled shift
	cmp.b	#$64,d0
	beq	.alt
	cmp.b	#$65,d0
	beq	.alt				; Now we have handled alt
	cmp.b	#$63,d0
	beq	.ctrl
	cmp.b	#$67,d0
	beq	.ctrl				; Now we have handled ctrl
.keydone:
	POP
	move.b	key(a6),d0
	rts
.alt:
	move.b	keystatus(a6),keyalt(a6)
	move.b	#0,key(a6)
	bra	.keydone
.ctrl:
	move.b	keystatus(a6),keyctrl(a6)
	move.b	#0,key(a6)
	bra	.keydone
.shift:					; we have a happening on the SHIFT key
	cmp.b	#0,keycaps(a6)		; Check if caps is pressed
	bne	.caps
	move.b	#0,key(a6)
	move.b	keystatus(a6),keyshift(a6)
.caps:
	bra	.keydone
.capsshift:
	move.b	keystatus(a6),keycaps(a6)
	move.b	keystatus(a6),keyshift(a6)
	move.b	#0,key(a6)
	bra	.keydone
.nodata:
	POP
	move.b	key(a6),d0			; Return previous key (no new data)
	rts

GetHex:					; Takes an ASCII and returns only valid chars for hex. (and backspace/enter)

						; Input:
						;	D0 = Char

						; Output:
						; 	D0 = Char
	cmp.b	#"0",d0
	blt	.nonumber
	cmp.b	#"9",d0
	bgt	.nonumber
	rts
.nonumber:
	bclr	#5,d0				; Make it uppercase
	cmp.b	#"A",d0
	blt	.nochar
	cmp.b	#"F",d0
	bgt	.nochar
	rts
.nochar:
	cmp.b	#8,d0
	bne	.nobackspace
	rts
.nobackspace:
	cmp.b	#$d,d0
	bne	.checkenter
						; we had linefeed? convert to an enter :)
	move.b	#$a,d0
.checkenter:
	cmp.b	#$a,d0
	bne	.noenter
	rts
.noenter:
	cmp.b	#27,d0
	bne	.noesc
	rts
.noesc:
	move.b	#0,d0
	rts
	
GetDec:					; Takes an ASCII and returns only valid chars for dec. (and backspace/enter)
	; Input:
	;	D0 = Char

	; Output:
	; 	D0 = Char
	cmp.b	#"0",d0
	blt	.nonumber
	cmp.b	#"9",d0
	bgt	.nonumber
	rts
.nonumber:
	cmp.b	#8,d0
	bne	.nobackspace
	rts
.nobackspace:
	cmp.b	#$d,d0
	bne	.checkenter
	; we had linefeed? convert to an enter :)
	move.b	#$a,d0
.checkenter:
	cmp.b	#$a,d0
	bne	.noenter
	rts
.noenter:
	cmp.b	#27,d0
	bne	.noesc
	rts
.noesc:
	move.b	#0,d0
	rts


ConvertKey:					; Converts keystroke to char.
	; INDATA:
	; a0=pointer to keymap
	move.b	(a0,d0),d1
	move.b	d1,keypressed(a6)
	move.b	d1,keyresult(a6)
	move.b	(EnglishKeyShifted-EnglishKey)(a0,d0),d1
	move.b	d1,keypressedshifted(a6)
	cmp.b	#0,keyshift(a6)
	beq	.notshift
	move.b	d1,keyresult(a6)
.notshift:
	rts	

PrintCPU:
		; Prints CPU Information
	lea	CPUTxt,a0
	move.l	#2,d1
	bsr	Print
	move.l	CPUPointer(a6),a0
	move.l	#2,d1
	bsr	Print
	clr.l	d7
	cmp.b	#5,CPUGen(a6)			; Check if we had 060 gen of CPU, if so, print revisionnumber
	bne	.no060
	move.b	CPU060Rev(a6),d7
	lea	REVTxt,a0
	bsr	Print
	move.l	d7,d0
	bsr	bindec
	bsr	Print
.no060:
	lea	FPUTxt,a0
	move.l	#2,d1
	bsr	Print
	move.l	FPUPointer(a6),a0
	move.l	#2,d1
	bsr	Print
	lea	MMUTxt,a0
	move.l	#2,d1
	bsr	Print
	clr.l	d0
	move.b	MMU(a6),d0
	cmp.b	#0,d0
	beq	.nommu
	lea	NOTCHECKED,a0
	bra	.mmuprint
.nommu:
	lea	NO,a0
.mmuprint:
	bsr	Print
	rts

WaitLong:					; Wait a short time, aprox 10 rasterlines. (or exact IF we have detected working raster)
	PUSH
	cmp.b	#1,RASTER(a6)			; Check if we have a confirmed working raster
	beq	.raster
	move.w	#3,d1
	bsr	ReadSerial			; as we have no IRQs..  read serialport just in case
.loop2
	move.l	#$fff,d0			; if now.  lets try to wait some anyway.
.loop:
	move.b	$bfe001,d2			; Dummyread from slow memory
	move.b	$dff006,d2
	dbf	d0,.loop
	dbf	d1,.loop2
	POP
	rts
.raster:
	cmp.b	#$90,$dff006
	bne.s	.raster			; Wait for rasterline $90
	bsr	ReadSerial			; as we have no IRQs..  read serialport just in case
.rasterloop:
	cmp.b	#$8f,$dff006
	bne.s	 .rasterloop			; Wait for rasterline $8f, meaning we have waited for one frame
	POP
	rts

GetMemory:					; Get memory from workmem.  Fastmem prio.
						; IN:
						;	D0 = size wanted
						; OUT:
						;	A0 = startaddress of memory, if 0=no memory
	PUSH
	move.l	d0,d6				; We move size to d6...
	clr.l	d7
	move.l	FastStart(a6),d0		; D0 now contains start of fastmem
	move.l	FastEnd(a6),d1		; D1 now contains end of fastmem
	move.l	d1,d2
	sub.l	d0,d2				; D2 now contains fastmemsize
	move.l	ChipStart(a6),d3		; D3 now contains start of chipmem
	move.l	ChipEnd(a6),d4		; D4 now contains end of chipmem
	move.l	d4,d5
	sub.l	d3,d5				; D6 now contains chipmemsize
	cmp.l	#0,d0
	beq	.nofast
	cmp.l	d6,d2				; Check if we had enough
	blt	.nofast			; we did not have enough fast..
						; ok we had enough ram..
	bra	.hadmem			; so go to "hadmem" to handle last part
.nofast:
	cmp.l	#0,d3
	beq	.nochip			; we had nochip! out of mem!
	cmp.l	d6,d5
	blt	.nochip			; we had not enough chip!  out of mem!
						; ok we had mem. to be lazy we copy over chipmem registers to where fastmem regs was
	move.l	d3,d0
	move.l	d4,d1				; Start and end is all we need
	bra	.hadmem
.nochip:					; ok sorry!  out of memory!we are screwed return 0 as memory
	clr.l	MemAdr(a6)
	bra	.memdone
.hadmem:					; We had memory, just figure out if we should give from start or end of ram
	move.b	WorkOrder(a6),d7		; if d7 is 0=work from back, if not work from start.
	cmp.b	#0,d7				; Check status
	beq	.FromBack
	move.l	d0,MemAdr(a6)
	bra	.memdone
.FromBack:
	sub.l	d6,d1
	sub.l	#1,d1				; Subtract with 1 more or it will be an odd address
	move.l	d1,MemAdr(a6)			; Store it and return it
.memdone:
	POP
	move.l	MemAdr(a6),a0			; Return answer
	rts

_GetChip:
	bsr	GetChip
	cmp	#1,d0
	bne	.noone
	clr.l	d0				; Get rid of that "not enough chipmem" idea for C code  but not in asm to keep compability.
.noone:
	rts
GetChip:					; Gets extra chipmem below the reserved workarea.
						; IN = D0=Size requested
						; OUT = D0=Startaddress of chipmem.  1=not enough, 0=no chipmem
	PUSH
	clr.l	GetChipAddr(a6)		; Clear the address returned.
	cmp.l	#0,TotalChip(a6)		; if there are no chipmem, exit
	beq	.exit	
	move.l	ChipUnreserved(a6),d1	; Get total amount of nonused chipmem
	cmp.l	d0,d1				; Compare it with amount of mem wanted
	blt	.low				; we did not have enough. exit
	move.l	ChipUnreservedAddr(a6),d1	; ok load d1 with value of last usable noreserved chipmemarea
	sub.l	d0,d1				; Subtract with amount of memory wanted
	move.l	d1,GetChipAddr(a6)		; Store it to returnvalue
	move.l	d1,a0				; Now lets clear the ram
	asr.l	#2,d0
.loop:
	clr.l	(a0)+
	dbf	d0,.loop	
	bra	.exit
.low:
	move.l	#1,GetChipAddr(a6)		; put 1 into returnvalue, telling we did not have enough mem.
.exit: 
	POP
	move.l	GetChipAddr(a6),d0		; Return the value
	rts


RunCode:					; Copy a routine to RAM, run it from there and return.
						; IN =	A0 = link to routine
						; 	D0 = length of routine (max 64K)
	PUSH
	add.l	#4,d0				; add 4 bytes to be sure
	move.l	a0,a1				; copy link to routine to a0
	move.l	a0,a5
	move.l	d0,d7
	bsr	GetMemory			; get memory
	cmp.l	#0,a0				; if A0 is 0, we was out of memory, exit
	beq	RunCodeInRom
	move.l	a0,d1				; store memaddress to d0
	add.l	#4,d1				; add 4 to be sure
	asr.l	#2,d1				; as this migight put start 2 bytes before
	asl.l	#2,d1				; Make sure start is on a even 32 bit location!
	move.l	d1,a0
						; A0 now contains pointer where to copy routine
	lea	$0,a0				; kuk.  disable rum!!
	move.l	a0,RunCodeStart(a6)		; Store first address of where code is
	move.l	a0,a2				; make a backup of address
	move.l	a1,a3
	move.l	d0,d3
.loop:
	move.b	(a1)+,(a0)+
	dbf	d0,.loop			; copy routine to RAM
						; lets verify so the data is readable (working mem)
	move.l	a2,a0
	move.l	a3,a1
	sub.l	#4,d3
.loopa:
	move.b	(a0)+,d6
	move.b	(a1)+,d5
	cmp.b	d5,d6				; Compare memory
	bne	RunCodeInRom			; we failed
	dbf	d3,.loopa
						; memtest succeeded.  so lets run in ram.
	move.l	a0,RunCodeEnd(a6)		; Store where end of code is
.run:
	jsr	(a2)				; jump to routine
	POP
	rts
RunCodeInRom:
	move.l	a5,RunCodeStart(a6)
	move.l	a5,a4
	add.l	d7,a4
	move.l	a4,RunCodeEnd(a6)
	jsr	(a5)	
	POP
	rts

ToKB:						; Convert D0 to KB (divide by 1024)
	asr.l	#8,d0
	asr.l	#2,d0
	rts


Random:					;  out: d0 will contain a "random" number
	add.l	d3,d0
	add.l	d4,d0
	add.b	$dff006,d0
	add.l	d1,d0
	swap	d0
	add.b	$dff007,d0
	add.l	d2,d0
	add.l	d5,d0
	add.l	d6,d0
	add.l	d7,d0
	rts
	

DeleteLine:					; Delete line D0 on screen, scrolls everything under it up one line
	PUSH
	move.b	Xpos(a6),d6			; Make a backup of XPos.
	move.b	Ypos(a6),d7			; Make a backup of YPos
	clr.l	d1
	move.b	d0,d1				; set Y pos. (we used d0 to be lazy and logic)
	move.l	d1,d5
	move.l	#0,d0				; Set 0 to X pos. we already have Y pos in d0
	bsr	SetPos
	lea	DELLINE,a0			; Send ANSI command to delete line and scroll up
	move.l	#3,d1
	bsr	Print
	cmp.b	#0,NoDraw(a6)			; Check if we should draw
	bne	.exit
						; Lets Scroll up physical screen
	move.l	Bpl1Ptr(a6),a0		; load A0 with address of BPL1
	move.l	Bpl2Ptr(a6),a1		; load A1 with address of BPL2
	move.l	Bpl3Ptr(a6),a2		; load A2 with address of BPL3
	move.l	#31,d4				; Last line is 31
	sub.l	d5,d4				; subtract number of lines to where we was
	mulu	#640,d4			; calculate where in memory this is
	mulu	#640,d5			; calulate where to start scroll
	add.l	d5,a0
	add.l	d5,a1
	add.l	d5,a2
	move.l	#-1,(a0)
	move.l	#-1,(a1)
	move.l	#-1,(a2)
	divu	#4,d4
.loop:
	move.l	640(a0),(a0)+
	move.l	640(a1),(a1)+
	move.l	640(a2),(a2)+	
	dbf	d4,.loop
	move.w	#159,d4
.loop2:
	clr.l	(a0)+
	clr.l	(a1)+
	clr.l	(a2)+				; Clear last row
	dbf	d4,.loop2
.exit:
	clr.l	d0				; Restore old X and Y cordinates
	clr.l	d1
	move.b	d6,d0
	move.b	d7,d1
	bsr	SetPos
	POP
	rts

DetectMemory:
						; D1 Total block of known working ram in 16K blocks (clear before first use)
						; A0 first usable addr
						; a1 First addr to scan
						; a2 Addr to end
						; a3 Addr to jump after done (as this does not use any stack
						; only OK registers to use as write: (d1), d2,d3,d4,d5,d6,d7, a0,a1,a2,a5


						; D0 is a special "in" never to be modified but taken as a "random" generator for shadowcontrol

						; OUT:	d1 = blocks of found mem
						;	a0 = first usable address
						;	a1 = last usable address
	move.l	a1,d7
	and.l	#$fffffffc,d7			; just strip so we always work in longword area (just to be sure)
	move.l	d7,a1
	move.l	a3,d7				; Store jumpaddress in D7
	lea	$0,a0				; clear a0
.Detect:
	lea	MEMCheckPattern,a3
	move.l	(a1),d3			; Take a backup of content in memory to D3
.loop:
	cmp.l	a1,a2				; check if we tested all memory
	blo	.wearedone			; we have, we are done!
	move.l	(a3)+,d2			; Store value to test for in D2	
	move.l	d2,(a1)			; Store testvalue to a1
	move.l	#"CRAP",4(a1)			; Just to put crap at databus. so if a stuck buffer reads what is last written will get crap
	nop
	nop
	nop
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
	move.l	(a1),d4			; read value from a1 to d4
						; Reading several times.  as sometimes reading once will give the correct answer on bad areas.
	cmp.l	d4,d2				; Compare values
	bne	.failed			; ok failed, no working ram here.
	cmp.l	#0,d2				; was value 0? ok end of list
	bne	.loop				; if not, lets do this test again
						; we had 0, we have working RAM
	move.l	a1,a5				; OK lets see if this is actual CORRECT ram and now just a shadow.
	move.l	a5,(a1)			; So we store the address we found in that location.
	move.l	#32,d6				; ok we do test 31 bits
	move.l	a5,d5
.loopa:
	cmp.l	#0,d6
	beq	.done				; we went all to bit 0.. we are done I guess
	sub.l	#1,d6
	cmp.l	#0,d6
	beq	.done				; we went all to bit 0.. we are done I guess	---------
	btst	d6,d5				; scan until it isnt a 0
	beq.s	.loopa
.bitloop:
	bclr	d6,d5				; ok. we are at that address, lets clear first bit of that address
	move.l	d5,a3
	cmp.l	(a3),a5			; ok check if that address contains the address we detected, if so. we have a "shadow"
	beq	.shadow
	cmp.l	#0,a3				; it was 0, so we "assume" we got memory
	beq	.mem
						; ok we didnt have a shadow here
						; a5 will contain address if there was detected ram
	sub.l	#1,d6
	cmp.l	#4,d6
	beq	.mem				; ok we was at 4 bits away..  we can be PRETTY sure we do not have a shadow here.  we found mem
	bra	.bitloop
.mem:
	move.l	d3,(a1)			; restore backup of data
	cmp.l	(a1),d0			; check if value at a1 is the same as d0. this means we have a shadow on top and we have already tested
	beq	.shadowdone			; this memory.  basically: we are done
	cmp.l	#0,a0				; check if a0 was 0, if so, this is the first working address
	bne	.wehadmem
	move.l	a5,a0				; so a5 contained the address we found, copy it to a0
	move.l	d7,16(a1)			; ok store d7 into what a1 points to.. to say that this is a block of mem)
.wehadmem:
	add.l	#4,d1				; OK we found mem, lets add 4 do d1(as old routine was 64K blocks  now 256.  being lazy)
	bra	.next
.wearedone:
	bra	.done
.shadow:
	TOGGLEPWRLED				; Flash with powerled doing this.. 
.failed:
	move.l	d3,(a1)			; restore backup of data
	cmp.l	#0,a0				; ok was a0 0? if so, we havent found memory that works yet, lets loop until all area is tested
	bne	.done
.next:
	move.l	d0,(a1)			; put a note at the first found address. to mark this as already tagged
	move.l	a0,4(a1)			; put a note of first block found
	move.l	a1,8(a1)			; where this block was
	move.l	d1,12(a1)			; total amount of 64k blocks found
						; Strangly enough. this seems to also write onscreen at diagrom?
	add.l	#256*1024,a1			; Add 256k for next block to test
	bra	.Detect
.shadowdone:
	TOGGLEPWRLED				; Flash with powerled doing this.. 
.done:
	move.l	d7,a3				; Restore jumpaddress
	sub.l	#1,a1
	jmp	(a3)

InputHexNum:					; Inputs a 32 bit hexnumber
						; INDATA
						;	A0 = Defualtaddress
	PUSH
	move.b	Xpos(a6),CheckMemManualX(a6)
	move.b	Ypos(a6),CheckMemManualY(a6); Store X and Y positions
	move.l	a0,d0				; Store the defaultaddress in d0
	jsr	binhex				; Convert it to hex
	add.l	#1,a0				; Skip first $ sign in string
	move.l	#8,d0
	lea	CheckMemStartAdrTxt(a6),a1	; Clear workspace
.clearloop:
	clr.b	(a1,d0)
	dbf	d0,.clearloop
	move.l	#7,d0
	clr.l	d7				; Clear d7, if this is 0 later we had not had any 0 yet
.hexloop:
	move.b	(a0)+,d1			; Store char in d1
	cmp.b	#"0",d1				; is it a 0?
	bne	.nozero
	cmp.b	#0,d7				; Check if d7 is 0. if so, we will skip this
	beq	.zero
.nozero:
	move.b	d1,(a1)+			; Copy to where a1 points to
	move.b	#1,d7				; We had a nonzero.  set d7 to 1 so we handle 0 in the future
.zero:
	dbf	d0,.hexloop			; Copy string to defaultadress to be shown
	lea	CheckMemStartAdrTxt(a6),a5	; Store pointer to string at a5
	move.l	a5,a0
	move.l	#7,d1
	jsr	Print				; Print it
	jsr	StrLen				; Get Stringlength
	move.l	d0,d6
	clr.l	d7				; Clear d7, this is the current position of the string
	sub.b	#1,d7				; Change d7 so we will force a update of cursor first time
.loop:
	jsr	GetMouse
	cmp.b	#1,RMB(a6)
	beq	.exit
	cmp.b	#1,LMB(a6)
	beq	.exit
	bsr	WaitShort
	jsr	GetChar			; Get a char from keyboard/serial
	bsr	WaitLong
	cmp.b	#"x",d0			; did user press X?
	beq	.xpressed
	cmp.b	#$7f,d0			; did we have backspace from serial?
	beq	.backspace
.gethex:
	jsr	GetHex				; Strip it to hexnumbers
	cmp.b	#0,d0				; if returned value is 0, we had no keypress
	beq	.no
	cmp.b	#$1b,d0			; Was ESC pressed?
	beq	.exit				; if so, Exit
	cmp.b	#$a,d0				; did user press enter?
	beq	.enter				; if so, we are done
	cmp.b	#$8,d0				; Did we have a backspace?
	bne	.nobackspace			; no
						; oh. we had. lets erase one char
.backspace:
	move.b	#$0,(a5,d6)			; Store a null at that position
	cmp.b	#0,d6				; check if we are at the back?
	beq	.backmax			; yes, do not remove
	move.b	#" ",d0
	sub.b	#1,d6				; Subtract one
	move.b	d0,(a5,d6)			; Put char in memory
	bra	.back
.nobackspace:
	cmp.b	#8,d6				; Check if we have max number of chars
	beq	.nomore
	move.b	d0,(a5,d6)			; Put char in memory
	add.b	#1,d6
.back:
	move.l	#7,d1
	jsr	PrintChar			; Print the char
.backmax:
.nomore:
.no:	cmp.b	d6,d7				; Check if d6 and d7 is same, if not, update cursor
	beq	.same
	move.b	d6,d7
	bsr	.putcursor			; Put cursor
.same:
	bra	.loop
.exit:
	POP
	move.l	#-1,d0				; Show we had an exit
	rts
.xpressed:					; X is pressed, lets clear the whole area.
	clr.l	d6
	move.l	#7,d0
.xloop:
	move.b	#" ",(a5,d0)
	dbf	d0,.xloop
	clr.l	d7
	bsr	.putcursor
	lea	space8,a0
	move.l	#7,d1
	jsr	Print
	clr.l	d7
	bsr	.putcursor
	clr.l	d6
	clr.l	d0
	bra.w	.gethex
.enter:
	cmp.b	#0,d6				; was cursor at 0? then we had nothing
	beq	.exit
	bsr	.putcursor
	move.l	#" ",d0
	jsr	PrintChar			; Print a space to remove the old cursor

	clr.l	d6				; Clear d6, we need to check how many numbers we have
.countloop:
	move.b	(a5,d6),d0			; load char in string
	cmp.b	#0,d0				; is it a null?
	beq	.null
	cmp.b	#" ",d0			; same with space
	beq	.null
	add.b	#1,d6				; nope, so lets add 1 to the counter
	cmp.b	#8,d6				; Check if we actually DID have 8 chars, then no rotate of data is needed
	beq	.norotate
	bra	.countloop			; do it all over again
.null:						; ok we had a null, before doing 8 chars.
						; We had less then 8 chars, meaning we need to trimp it to 8 chars.
	move.l	d6,d7
	sub.b	#1,d7
	move.l	#7,d0
.copyloop2:
	move.b	(a5,d7),(a5,d0)
	sub.b	#1,d0
	dbf	d7,.copyloop2
						; ok now we have moved the data to the end of the string, lets fill up with 0
	move.l	#8,d0
	sub.b	d6,d0				; d0 now contains of how many 0 to put in
	sub.b	#1,d0
.fill:
	move.b	#"0",(a5,d0)
	dbf	d0,.fill
.norotate:
	move.b	CheckMemManualX(a6),d0
	move.b	CheckMemManualY(a6),d1
	sub.l	#1,d0				; Set cursor to the first adress, minus pone
	jsr	SetPos
	lea	CheckMemStartAdrTxt(a6),a0
	jsr	hexbin
	POP
	move.l	HexBinBin(a6),d0		; return the value
	rts
.putcursor:
	PUSH
	move.b	CheckMemManualX(a6),d0
	add.b	d7,d0				; Add postion to X pos to get correct position
	move.b	CheckMemManualY(a6),d1
	jsr	SetPos
	clr.l	d0
	move.b	(a5,d7),d0			; Load current char from string
	move.l	#11,d1
	jsr	PrintChar			; Print it reversed
	move.b	CheckMemManualX(a6),d0
	add.b	d7,d0				; Add postion to X pos to get correct position
	move.b	CheckMemManualY(a6),d1
	jsr	SetPos
	POP
	rts	

StrLen:
						; Returns length of string
						; IN:
						;	A0 = Pointer to nullterminated string
						; OUT:
						;	D0 = Length of string
	PUSH
	clr.l	d0				; Clear d0
.loop:
	move.b	(a0)+,d7			; Load d7 with char
	cmp.b	#0,d7
	beq	.exit				; Exit if we found a null
	cmp.b	#2,d7				; if centercommand, skip char
	beq	.skip
	add.l	#1,d0				; add 1 to stringlength
.skip:
	bra	.loop
.exit:
	move.l	d0,temp(a6)			; Store length in temp. as we will restore all registers
	POP
	move.l	temp(a6),d0			; So back to D0 again
	rts

GetMouse:
	PUSH
	clr.l	d0				; Clear d0..
	move.b	#0,BUTTON(a6)			; Clear the generic "Button" variable
	move.b	#0,LMB(a6)			; I use move as clr actually does a read first
	move.b	#0,P1LMB(a6)
	move.b	#0,P2LMB(a6)
	move.b	#0,RMB(a6)
	move.b	#0,P1RMB(a6)
	move.b	#0,P2RMB(a6)
	move.b	#0,MBUTTON(a6)		; Clear the generic "Mbutton" variable
	clr.l	d1
	bsr	GetMouseData
	move.l	d0,InputRegister(a6)
	POP
	move.l	InputRegister(a6),d0
	rts

_hexbin:
hexbin:						; Converts a hex string to binary
	; INDATA: A0 = String (8 hex chars)
	; OUTDATA: D0 = binary number
	PUSH
	move.l	a0,-(sp)			; string (arg 1)
	jsr	_hexBin
	addq.l	#4,sp
	move.l	d0,HexBinBin(a6)
	POP
	move.l	HexBinBin(a6),d0
	rts

InputDecNum:					; Inputs a 32 bit hexnumber
						; INDATA
						;	A0 = Defualtaddress
	PUSH
	move.b	Xpos(a6),CheckMemManualX(a6)
	move.b	Ypos(a6),CheckMemManualY(a6); Store X and Y positions
	move.l	a0,d0				; Store the defaultaddress in d0
	jsr	bindec				; Convert it to hex
	move.l	#8,d0
	lea	CheckMemStartAdrTxt(a6),a1	; Clear workspace
.clearloop:
	clr.b	(a1,d0)
	dbf	d0,.clearloop
	move.l	#7,d0
.decloop:
	move.b	(a0)+,d1			; Store char in d1
	cmp.b	#0,d1				; Check if d7 is 0. if so, we will skip this
	beq	.zero
	move.b	d1,(a1)+			; Copy to where a1 points to
	dbf	d0,.decloop
.zero:
	lea	CheckMemStartAdrTxt(a6),a5	; Store pointer to string at a5
	move.l	a5,a0
	move.l	#7,d1
	jsr	Print				; Print it
	jsr	StrLen				; Get Stringlength
	move.l	d0,d6
	clr.l	d7				; Clear d7, this is the current position of the string
	sub.b	#1,d7				; Change d7 so we will force a update of cursor first time
.loop:
	jsr	GetMouse
	cmp.b	#1,RMB(a6)
	beq	.exit
	cmp.b	#1,LMB(a6)
	beq	.exit
	bsr	WaitShort
	jsr	GetChar			; Get a char from keyboard/serial
	bsr	WaitLong
	cmp.b	#"x",d0			; did user press X?
	beq	.xpressed
	cmp.b	#$7f,d0			; did we have backspace from serial?
	beq	.backspace
.getdec:
	jsr	GetDec				; Strip it to hexnumbers
	cmp.b	#0,d0				; if returned value is 0, we had no keypress
	beq	.no
	cmp.b	#$1b,d0			; Was ESC pressed?
	beq	.exit				; if so, Exit
	cmp.b	#$a,d0				; did user press enter?
	beq	.enter				; if so, we are done
	cmp.b	#$8,d0				; Did we have a backspace?
	bne	.nobackspace			; no
						; oh. we had. lets erase one char
.backspace:
	move.b	#$0,(a5,d6)			; Store a null at that position
	cmp.b	#0,d6				; check if we are at the back?
	beq	.backmax			; yes, do not remove
	move.b	#" ",d0
	sub.b	#1,d6				; Subtract one
	move.b	d0,(a5,d6)			; Put char in memory
	bra	.back
.nobackspace:
	cmp.b	#8,d6				; Check if we have max number of chars
	beq	.nomore
	move.b	d0,(a5,d6)			; Put char in memory
	add.b	#1,d6
.back:
	move.l	#7,d1
	jsr	PrintChar			; Print the char
.backmax:
.nomore:
.no:	cmp.b	d6,d7				; Check if d6 and d7 is same, if not, update cursor
	beq	.same
	move.b	d6,d7
	bsr	.putcursor			; Put cursor
.same:
	bra	.loop
.exit:
	POP
	move.l	#-1,d0				; Show we had an exit
	rts
.xpressed:					; X is pressed, lets clear the whole area.
	clr.l	d6
	move.l	#7,d0
.xloop:
	move.b	#" ",(a5,d0)
	dbf	d0,.xloop
	clr.l	d7
	bsr	.putcursor
	lea	space8,a0
	move.l	#7,d1
	jsr	Print
	clr.l	d7
	bsr	.putcursor
	clr.l	d6
	clr.l	d0
	bra.w	.getdec

.enter:
	cmp.b	#0,d6				; was cursor at 0? then we had nothing
	beq	.exit
	bsr	.putcursor
	move.l	#" ",d0
	jsr	PrintChar			; Print a space to remove the old cursor
	clr.l	d6				; Clear d6, we need to check how many numbers we have
	move.l	a5,a0
	jsr	decbin
	POP
	move.l	DecBinBin(a6),d0		; return the value
	rts
.putcursor:
	PUSH
	move.b	CheckMemManualX(a6),d0
	add.b	d7,d0				; Add postion to X pos to get correct position
	move.b	CheckMemManualY(a6),d1
	jsr	SetPos
	clr.l	d0
	move.b	(a5,d7),d0			; Load current char from string
	move.l	#11,d1
	jsr	PrintChar			; Print it reversed
	move.b	CheckMemManualX(a6),d0
	add.b	d7,d0				; Add postion to X pos to get correct position
	move.b	CheckMemManualY(a6),d1
	jsr	SetPos
	POP
	rts	

hexbytetobin:
	clr.l	d2				; Clear D2 that holds the ASCII code
	move.b	(a0)+,d2			; Read one byte of the string
	bsr	.tobin				; Convert to binary
	move.l	d2,d1				; Store the value in D1
	move.b	(a0)+,d2			; Read next char to complete this byte
	bsr	.tobin				; Convert to binary
	asl.l	#4,d1				; Rotate the first char 4 bits
	add.l	d1,d2				; add d1 to d2, d2 will now contain this byte in binary
	rts
.tobin:
	cmp.b	#"A",d2			; Check if it is "A"
	blt	.nochar			; Lower then A, this is not a char
	sub.l	#7,d2				; ok we have a char, subtract 7
.nochar:
	sub.l	#$30,d2			; Subtract $30, converting it to binary.
	rts
_decbin:
decbin:					; Convert a decimal string to binary number
	; IN: A0 = String
	; OUT: D0 = Number in binary
	PUSH
	move.l	a0,-(sp)			; string (arg 1)
	jsr	_decBin
	addq.l	#4,sp
	move.l	d0,DecBinBin(a6)
	POP
	move.l	DecBinBin(a6),d0
	rts

MakePrintable:
						; Makes the char in D0 printable. remove controlchars etc.
	cmp.b	#" ",d0
	ble	.lessthenspace		; is less then space.. make it space.
	rts
.lessthenspace:
	move.b	#" ",d0
	rts

binstringbyte:
						; Converts a binary number (byte) to binary string
						; INDATA:
						;	D0 = binary number
						; OUTDATA:
						;	A0 = Poiner to outputstring
	PUSH
	move.l	#7,d7
	lea	binstringoutput(a6),a0
.loop:
	btst	d7,d0
	beq	.notset
	move.b	#"1",(a0)+
	bra	.done
.notset:
	move.b	#"0",(a0)+
.done:
	dbf	d7,.loop
	move.b	#0,(a0)
	POP
	lea	binstringoutput(a6),a0
	rts

EnableCache:
	PUSH
	move.l	#$0808,d1
	movec	d1,CACR
	move.l	#$0101,d1
	movec	d1,CACR
	POP
	rts

_DisableCache:
DisableCache:
	PUSH
	move.l	#$0808,d1
	movec	d1,CACR
	move.l	#0,d1
	movec	d1,CACR
	POP
	rts

SameRow:
						; Changes so we print on the same row. just clears the X column
	PUSH
	clr.b	Xpos(a6)
	move.b	#$d,d0
	bsr	rs232_out
	POP
	rts

DevPrint:
	clr.l	d0
	move.l	#25,d1
	jsr	SetPos
	lea	UnderDevTxt,a0
	move.l	#1,d1
	jsr	Print
	clr.l	d0
	clr.l	d1
	jsr	SetPos
	rts

DefaultVars:					; Set defualtvalues
	move.l	$400,CheckMemEditScreenAdr(a6)
	move.b	#0,skipnextkey(a6)
	rts

_PAUSE:
	PAUSE
	rts


_scrollScreen:
ScrollScreen:
	cmp.b	#0,NoDraw(a6)			; Check if we should draw
	bne	.exit

	PUSH
	move.l	Bpl1Ptr(a6),a0		; load A0 with address of BPL1
	move.l	Bpl2Ptr(a6),a1		; load A1 with address of BPL2
	move.l	Bpl3Ptr(a6),a2		; load A2 with address of BPL3
	move.l	BPLSIZE(a6),d0		; How much data is one screen
	sub.l	#640,d0			; Subtract 8 pixels
	divu	#4,d0				; Divide by 4 to get longwords.
.loop:
	move.l	640(a0),(a0)+
	move.l	640(a1),(a1)+
	move.l	640(a2),(a2)+	
	dbf	d0,.loop

	move.w	#159,d0
.loop2:
	clr.l	(a0)+
	clr.l	(a1)+
	clr.l	(a2)+				; Clear last row
	dbf	d0,.loop2
	POP
.exit:
	rts

GetPos:
	clr.l	d0
	clr.l	d1
	move.b	Xpos(a6),d0
	move.b	Ypos(a6),d1
	rts

	
CopyMem:
	; Copy one block memory to another
	; INDATA:
	;	A0 = Source
	;	D0 = Bytes to copy. (YES. being lazy, we do this bytestyle)
	;	A1 = Destination
	clr.l	d7
.loop:
	move.b	(a0)+,(a1)+
	add.l	#1,d7
	cmp.l	d7,d0
	bgt	.loop				; YES a DBF would do just fine. but i want to support more then 64k
	rts


hextab:
	dc.b	"0123456789ABCDEF"		; For bin->hex convertion

	EVEN
EnglishKey::
	dc.b	" 1234567890-=| 0"
	dc.b	"qwertyuiop[] "; 1c
	dc.b	"123asdfghjkl;`" ; 2a
	dc.b	"  456 zxcvbnm,./ " ;3b
	dc.b	".789 "
	dc.b	8 ; backspace
	dc.b	9 ; Tab
	dc.b	$d ; Return
	dc.b    $a ; Enter (44)
	dc.b	27 ; esc
	dc.b	127 ; del
	dc.b	"   " ; Undefined
	dc.b	"-" ; - on numpad
	dc.b	" " ; Undefined
	dc.b	30 ; Up
	dc.b	31 ;down
	dc.b	28 ; forward
	dc.b	29 ; backward
	dc.b	"1" ;f1
	dc.b	"2" ;f2
	dc.b	"3" ;f3
	dc.b	"4" ;f4
	dc.b	"5" ;f5
	dc.b	"6" ;f6
	dc.b	"7" ;f7
	dc.b	"8" ;f8
	dc.b	"9" ;f9
	dc.b	"0" ;f10
	dc.b	"()/*+"
	dc.b	0 ; Help
EnglishKeyShifted::
	; Shifted
	dc.b	"~!@#$%^& ()_+| 0QWERTYUIOP{} 123ASDFGHJKL:",34,"  456 ZXCVBNM<>? .789          - "
	dc.b	1 ; Up
	dc.b	2 ;down
	dc.b	0 ; forward
	dc.b	0 ; backward
	dc.b	0 ;f1
	dc.b	0 ;f2
	dc.b	0 ;f3
	dc.b	0 ;f4
	dc.b	0 ;f5
	dc.b	0 ;f6
	dc.b	0 ;f7
	dc.b	0 ;f8
	dc.b	0 ;f9
	dc.b	0 ;f10
	dc.b	"()/*+"
	dc.b	0 ; Help


Init_Serial:
	PUSH
	jsr	_initSerial
	POP
	rts

SendSerial:
		; Indata a0=string to send to serialport
		; nullterminated
	PUSH
	move.l	a0,-(sp)			; Push string pointer for C calling convention
	jsr	_sendSerial
	addq.l	#4,sp				; Clean up stack
	POP
	rts

rs232_out:
	PUSH					; Save all registers (C may clobber d0/d1/a0/a1)
	move.l	d0,-(sp)			; Push character (d0) onto stack for C calling convention
	jsr	_rs232_out
	addq.l	#4,sp				; Clean up stack
	POP					; Restore all registers
	rts

	; This contains the generic code for all general-purpose stuff
GetHWReg:					; Dumps all readable HW registers to memory
	jsr	_getHWReg
	rts

PutChar:
	PUSH
	move.l	d3,-(sp)		; yPos (arg 4)
	move.l	d2,-(sp)		; xPos (arg 3)
	move.l	d1,-(sp)		; color (arg 2)
	move.l	d0,-(sp)		; character (arg 1)
	jsr	_putChar
	lea	16(sp),sp		; clean up 4 args from stack
	POP
	rts

PrintChar:				; Puts a char on screen and add X, Y variables depending on char etc.
	PUSH
	move.l	d1,-(sp)		; color (arg 2)
	move.l	d0,-(sp)		; character (arg 1)
	jsr	_printChar
	lea	8(sp),sp		; clean up 4 args from stack
	POP
	RTS

Print:						; Prints a string
	; INDATA:
	; A0 = string to print, nullterminated
	; D1 = Color
	PUSH
	move.l	d1,-(sp)			; color (arg 2)
	move.l	a0,-(sp)			; string (arg 1)
	jsr	_print
	lea	8(sp),sp			; clean up 2 args from stack
	POP
	rts


SetPos:					; Set cursor at wanted position on screen
	; Indata:
	; d0 = xpos
	; d1 = ypos
	PUSH
	move.l	d1,-(sp)		; yPos (arg 2)
	move.l	d0,-(sp)		; xPos (arg 1)
	jsr	_setPos
	lea	8(sp),sp		; clean up 2 args from stack
	POP
	rts

ClearScreen:
	PUSH
	jsr	_clearScreen
	POP
	rts

ReadSerial:					; Read serialport, and if anything there store it in the buffer
	PUSH
	jsr	_readSerial
	POP
	rts

RomChecksum:
	PUSH
	jsr	_romChecksum
	POP
	rts
