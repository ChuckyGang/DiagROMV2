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
	xdef	_bindec
	xdef	bindec
	xdef	oldbindec
	xdef	_binhex
	xdef	binhex
	xdef	CopyMem
	xdef	Print
	xdef	PrintChar
	xdef	PutChar
	xdef	ScrollScreen
	xdef	ClearScreen
;	xdef	_SetPos
	xdef	SetPos
	xdef	GetPos
	xdef	RomChecksum
	xdef	DetectCPU
	xdef 	EnglishKey
	xdef	EnglishKeyShifted
	xdef	_ClearInput
	xdef	_GetInput
	xdef	GetInput
	xdef	DefaultVars
	xdef	UnimplInst
	xdef	binstring
	xdef	BusError
	xdef	binhexword
	xdef	Trap
	xdef	WaitLong
	xdef	WaitReleased
	xdef	WaitPressed
	xdef	_GetChip
	xdef	GetChip
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
	xref	_mainMenu

WaitShort:					; Wait a short time, aprox 10 rasterlines. (or exact IF we have detected working raster)
	PUSH
	jsr	_waitShort
	POP
	rts


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


ErrorScreen:
	move.w	(a7),DebSR(a6)		; Store SR from exception stack frame
	move.l	2(a7),DebPC(a6)		; Store PC from exception stack frame
	move.l	d0,DebD0(a6)
	move.l	d1,DebD1(a6)
	move.l	d2,DebD2(a6)
	move.l	d3,DebD3(a6)
	move.l	d4,DebD4(a6)
	move.l	d5,DebD5(a6)
	move.l	d6,DebD6(a6)
	move.l	d7,DebD7(a6)
	move.l	a0,DebA0(a6)		; a0 = error title (set by exception handler)
	move.l	a1,DebA1(a6)
	move.l	a2,DebA2(a6)
	move.l	a3,DebA3(a6)
	move.l	a4,DebA4(a6)
	move.l	a5,DebA5(a6)
	move.l	a6,DebA6(a6)
	move.l	a7,DebA7(a6)
	jsr	_errorScreenC			; a0 still has error title (register param)
	jmp	_mainMenu

; --- DebugScreen removed: converted to C debugScreen() ---
; --- ClearBuffer removed: converted to C ClearBuffer() ---
; --- WaitButton removed: converted to C WaitButton() ---

WaitPressed:					; Waits until some "button" is pressed
	jsr	_waitPressed
	rts

WaitReleased:					; Waits until some "button" is unreleased
	jsr	_waitReleased
	rts

_GetInput:
GetInput:
	PUSH
	jsr	_getInput
	POP
	move.l	InputRegister(a6),d0
	rts

_GetSerial:
GetSerial:
	PUSH
	jsr	_getSerial
	POP
	rts

ClearInput:
	PUSH
	jsr	_clearInput
	POP

	rts


_GetMouseData::
GetMouseData:
	PUSH
	jsr	_getMouseData
	POP
	move.l	InputRegister(a6),d0
	rts

GetChar:					; Reads keyboard and serialport and returns the value in D0
	PUSH
	jsr	_getChar
	POP
	move.b	GetCharData(a6),d0
	rts

_GetCharSerial::
GetCharSerial:
	PUSH
	jsr	_getCharSerial
	POP
	clr.l	d0
	move.b	Serial(a6),d0
	rts

_GetCharKey::
GetCharKey:
	PUSH
	jsr	_getCharKey
	POP
	clr.l	d0
	move.b	keyresult(a6),d0
	rts

_GetKey::
GetKey:
	PUSH
	jsr	_getKey
	POP
	move.b	key(a6),d0
	rts

GetHex:					; Takes an ASCII and returns only valid chars for hex. (and backspace/enter)
	jsr	_getHex
	rts

GetDec:					; Takes an ASCII and returns only valid chars for dec. (and backspace/enter)
	jsr	_getDec
	rts


; --- ConvertKey removed: converted to C convertKey() ---

; --- PrintCPU removed: converted to C PrintCPU() ---

WaitLong:					; Wait a short time, aprox 10 rasterlines. (or exact IF we have detected working raster)
	PUSH
	jsr	_waitLong
	POP
	rts

GetMemory:					; Get memory from workmem.  Fastmem prio.
	PUSH
	jsr	_getMemory
	POP
	move.l	MemAdr(a6),a0
	rts

_GetChip:
	jsr	_getChip
	cmp.l	#1,d0
	bne	.noone
	clr.l	d0
.noone:
	rts
GetChip:					; Gets extra chipmem below the reserved workarea.
	PUSH
	jsr	_getChip
	POP
	move.l	GetChipAddr(a6),d0
	rts


RunCode:					; Copy a routine to RAM, run it from there and return.
	PUSH
	jsr	_runCode
	POP
	rts
ToKB:						; Convert D0 to KB (divide by 1024)
	jsr	_toKB
	rts

Random:					;  out: d0 will contain a "random" number
	jsr	_random
	rts
	

DeleteLine:					; Delete line D0 on screen, scrolls everything under it up one line
	PUSH
	jsr	_deleteLine
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
	PUSH
	jsr	_inputHexNum
	move.l	d0,temp(a6)
	POP
	move.l	temp(a6),d0
	rts
StrLen:
	PUSH
	jsr	_strLen
	move.l	d0,temp(a6)
	POP
	move.l	temp(a6),d0
	rts

GetMouse:
	PUSH
	jsr	_getMouse
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
	PUSH
	jsr	_inputDecNum
	move.l	d0,temp(a6)
	POP
	move.l	temp(a6),d0
	rts

hexbytetobin:
	jsr	_hexByteToBin
	move.l	d0,d2
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
	jsr	_makePrintable
	rts

binstringbyte:
						; Converts a binary number (byte) to binary string
						; INDATA:
						;	D0 = binary number
						; OUTDATA:
						;	A0 = Poiner to outputstring
	PUSH
	jsr	_binStringByte
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
	PUSH
	jsr	_sameRow
	POP
	rts

DevPrint:
	jsr	_devPrint
	rts

DefaultVars:					; Set defualtvalues
	jsr	_defaultVars
	rts

_PAUSE:
	PAUSE
	rts


ScrollScreen:
	jsr	_scrollScreen
	rts

GetPos:
	clr.l	d0
	clr.l	d1
	move.b	Xpos(a6),d0
	move.b	Ypos(a6),d1
	rts

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

	
CopyMem:
	; Copy one block memory to another
	; INDATA:
	;	A0 = Source
	;	D0 = Bytes to copy.
	;	A1 = Destination
	jsr	_copyMem
	rts

binhexbyte:
	; D0 = byte value, returns A0 = pointer to 2-char hex string in binhexoutput+7
	PUSH
	jsr	_binHexByte
	POP
	lea	binhexoutput+7(a6),a0
	rts

binhexword:
	; D0 = word value, returns A0 = pointer to "$XXXX" string in binhexoutput+4
	PUSH
	jsr	_binHexWord
	POP
	lea	binhexoutput+4(a6),a0
	rts

binstring:
	; D0 = 32-bit value, returns A0 = pointer to 32-char binary string
	PUSH
	jsr	_binString
	POP
	lea	binstringoutput(a6),a0
	rts


hextab:
	dc.b	"0123456789ABCDEF"		; For bin->hex convertion

	EVEN
_EnglishKey::
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
_EnglishKeyShifted::
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
	; Indata a0=string to send to serialport, nullterminated
	PUSH
	jsr	_sendSerial
	POP
	rts

rs232_out:
	; Indata d0=character
	PUSH
	jsr	_rs232_out
	POP
	rts

	; This contains the generic code for all general-purpose stuff
GetHWReg:					; Dumps all readable HW registers to memory
	jsr	_getHWReg
	rts

PutChar:
	; Indata: d0=char, d1=color, d2=xPos, d3=yPos
	PUSH
	jsr	_putChar
	POP
	rts

PrintChar:				; Puts a char on screen and add X, Y variables depending on char etc.
	; Indata: d0=char, d1=color
	PUSH
	jsr	_printChar
	POP
	rts

Print:					; Prints a string
	; Indata: a0=string (nullterminated), d1=color
	PUSH
	jsr	_print
	POP
	rts

SetPos:					; Set cursor at wanted position on screen
	; Indata: d0=xpos, d1=ypos
	PUSH
	jsr	_setPos
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

binhex:					; Converts a binary number to hex
	; INDATA:  D0 = binary number
	; OUTDATA: A0 = Pointer to "binhexoutput" containing the string
	PUSH
	jsr	_binHex
	POP
	lea	binhexoutput(a6),a0		; Set a0 after POP so it isn't overwritten
	rts
