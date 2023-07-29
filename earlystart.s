		include "earlymacros.i"

		section "startup",code_p

		XDEF	AnsiNull

	;	This is where it all starts
	;	I use 7 as Tab size  as it fits better for asm..
	;	Any comment of "printing" in this part of the code means out to serialport only.


_start:
		dc.w $1114		; this just have to be here for ROM. code starts at $2

		jmp	_begin
	
		dc.l	POSTBusError				; Hardcoded pointers
		dc.l	POSTAddressError			; if something is wrong rom starts at $0
		dc.l	POSTIllegalError			; so this will actually be pointers to
		dc.l	POSTDivByZero				; traps.
		dc.l	POSTChkInst
		dc.l	POSTTrapV
		dc.l	POSTPrivViol
		dc.l	POSTTrace
		dc.l	POSTUnimplInst

_strstart:
		dc.b	"IHOL : :6U6U,A,B1U1U5767U,U,8181 1 0    "	; This string will make a readable text on each 32 bit
		dc.b	"HILO: : U6U6A,B,U1U17576,U,U18181 0     "	; rom what socket to use. (SOME programmingsoftware does byteshift so both orders)
	
		dc.b	"$VER: DiagROM Amiga Diagnostic by John Hertell. "
		VERSION
		dc.b " "
		incbin	"builddate.i"
_strstop:
		
		blk.b	166-(_strstop-_strstart),0		; Crapdata that needs to be here
	
		EVEN

		;cnop 0,16


; ******************************************************************************************************************
;
; Init of Diagrom, lets get started
;
; ******************************************************************************************************************

	
_begin:	

		clr.l	d0
		clr.l	d1
		clr.l	d2
		clr.l	d3
		clr.l	d4
		clr.l	d5
		clr.l	d6
		clr.l	d7
		lea	$0,a0
		lea	$0,a1
		lea	$0,a2
		lea	$0,a3
		lea	$0,a4
		lea	$0,a5
		lea	$0,a6

		lea	$0,SP			; Set the stack. BUT!!! do not use it yet. we need to check chipmem first! so meanwhile we use it as a dirty register

		move.b	#$ff,$bfe200
		move.b	#$ff,$bfe300

		move.b	#0,$bfe001		; Clear register.
		move.b	#$ff,$bfe301
		move.b	#$0,$bfe101
		move.b	#3,$bfe201	
		move.b	#0,$bfe001		; Powerled will go ON! so user can see that CPU works.  you know.  ACTIVITY!  
		move.b	#$40,$bfed01
		move.w	#$f0f,$dff180		; Set color to magenta (you should not be able to see this)
		move.w	#$ff00,$dff034
		move.w	#$0ff,$dff180		; Set color to cyan
		move.b	#$ff,$bfd300
		or.b	#$f8,$bfd100
		nop
		and.b	#$87,$bfd100
		nop
		or.b	#$78,$bfd100

		move.l	#POSTBusError,$8
		move.l	#POSTAddressError,$c
		move.l	#POSTIllegalError,$10
		move.l	#POSTDivByZero,$14
		move.l	#POSTChkInst,$18
		move.l	#POSTTrapV,$1c
		move.l	#POSTPrivViol,$20
		move.l	#POSTTrace,$24
		move.l	#POSTUnimplInst,$28
		move.l	#POSTUnimplInst,$2c

		move.b	#$88,$bfed01
		or.b	#$40,$bfee01		; For keyboard
						; We will print the result on the serialport later.
						
;	Do an addresscheck on the ROM data.
	
	KPRINTC _diagRomTxt

	; some writes to check some logicanalyzer shit at start.  just ignore :)  so addresslines and datalines steps up one bit at a time at startup
	move.l	#%00000000000000000000000000000001,%00000000000000000000000000000000
	move.b	#1,$1	; bytewrite as it is at an odd address
	move.l	#%00000000000000000000000000000010,%00000000000000000000000000000010
	move.l	#%00000000000000000000000000000100,%00000000000000000000000000000100
	move.l	#%00000000000000000000000000001000,%00000000000000000000000000001000
	move.l	#%00000000000000000000000000010000,%00000000000000000000000000010000
	move.l	#%00000000000000000000000000100000,%00000000000000000000000000100000
	move.l	#%00000000000000000000000001000000,%00000000000000000000000001000000
	move.l	#%00000000000000000000000010000000,%00000000000000000000000010000000
	move.l	#%00000000000000000000000100000000,%00000000000000000000000100000000
	move.l	#%00000000000000000000001000000000,%00000000000000000000001000000000
	move.l	#%00000000000000000000010000000000,%00000000000000000000010000000000
	move.l	#%00000000000000000000100000000000,%00000000000000000000100000000000
	move.l	#%00000000000000000001000000000000,%00000000000000000001000000000000
	move.l	#%00000000000000000010000000000000,%00000000000000000010000000000000
	move.l	#%00000000000000000100000000000000,%00000000000000000100000000000000
	move.l	#%00000000000000001000000000000000,%00000000000000001000000000000000
	move.l	#%00000000000000010000000000000000,%00000000000000010000000000000000
	move.l	#%00000000000000100000000000000000,%00000000000000100000000000000000
	move.l	#%00000000000001000000000000000000,%00000000000001000000000000000000
	move.l	#%00000000000010000000000000000000,%00000000000010000000000000000000
	move.l	#%00000000000100000000000000000000,%00000000000100000000000000000000
	move.l	#%00000000001000000000000000000000,%00000000001000000000000000000000
	move.l	#%00000000010000000000000000000000,%00000000010000000000000000000000
	move.l	#%00000000100000000000000000000000,%00000000100000000000000000000000
	move.l	#%00000001000000000000000000000000,%00000001000000000000000000000000
	move.l	#%00000010000000000000000000000000,%00000010000000000000000000000000
	move.l	#%00000100000000000000000000000000,%00000100000000000000000000000000
	move.l	#%00001000000000000000000000000000,%00001000000000000000000000000000
	move.l	#%00010000000000000000000000000000,%00010000000000000000000000000000
	move.l	#%00100000000000000000000000000000,%00100000000000000000000000000000
	move.l	#%01000000000000000000000000000000,%01000000000000000000000000000000
	move.l	#%10000000000000000000000000000000,%10000000000000000000000000000000

; ******************************************************************************************************************
;
; Check what mousebuttons was pressed during powerup, to be used later to make alternative startups and behaviour
;
; ******************************************************************************************************************



	clr.l	d0					; Clear d0 that is a temporary statusregister
			

					; We will print the result on the serialport later.
					
	btst	#6,$bfe001		; Check LMB port 1
	bne	.NOP1LMB		; NOT pressed.. Skip to next
	bset	#29,d0
.NOP1LMB:
	btst	#7,$bfe001		; Check LMB port 2
	bne	.NOP2LMB
	bset	#26,d0
.NOP2LMB:
	btst	#10,$dff016		; Check RMB port 1
	bne	.NOP1RMB
	bset	#28,d0
.NOP1RMB:
	btst	#14,$dff016		; Check RMB port 2
	bne	.NOP2RMB
	bset	#25,d0
.NOP2RMB:
	btst	#8,$dff016		; Check MMB Port 1
	bne	.NOP1MMB
	bset	#27,d0			; MMB Port 1
.NOP1MMB:
	btst	#12,$dff016		; Check MMB Port 2
	bne.s	.NOP2MMB
	bset	#24,d0			; MMB Port 2
.NOP2MMB:

	move.l	a7,d7
	or.l	d0,d7			; merge in the result into the A7 statusregister
	move.l	d7,a7			


	KPRINTC _Initmousetxt

	;	Print out status of pressed buttons at poweron
	btst	#29,d0
	beq	.pnop1lmb
	KPRINTC _InitP1LMBtxt
.pnop1lmb:
	btst	#26,d0
	beq	.pnop2lmb
	KPRINTC _InitP2LMBtxt
.pnop2lmb:
	btst	#28,d0
	beq	.pnop1rmb
	KPRINTC _InitP1RMBtxt
.pnop1rmb:
	btst	#25,d0
	beq	.pnop2rmb
	KPRINTC _InitP2RMBtxt
.pnop2rmb:
	btst	#27,d0
	beq	.pnop1mmb
	KPRINTC _InitP1MMBtxt
.pnop1mmb:
	btst	#24,d0
	beq	.pnop2mmb
	KPRINTC _InitP2MMBtxt
.pnop2mmb:
	cmp.l	#0,d0
	beq	.nopressed		; if no mouse was pressed skip next print
	KPRINTC	_releasemousetxt
.nopressed:

; ******************************************************************************************************************
;
; Check ROM Addressdata to see if romspace can be addressed correctly
;
; ******************************************************************************************************************






	KPRINTC _diagRomCheckadrtxt
_adrcheck:
	lea	_endofcode,a2			; Load address of last used address in rom of the code. located in checksums.s that is in the end of the rom-code
	lea	_autovec,a1			; Load address of autovec from autovec.s that is placed dead-last on the rom. so we know last address
	move.l	a2,d0
	move.l	a1,d1
	sub.l	d0,d1				; get the size of unused space in the rom, space that is padded with addressdata.
						; (done by the chedcksum code)
	asr.l	#6,d1				; d1 contains number of bytes between "point"
	move.l	d1,d2				; make a backup of it
	clr.l	d3				; d3 should be 0 or we had errors
.adrloop:
	move.l	(a2),d0			; As checksum file pads with addressdata.  load d0 with content of a0
	cmp.l	a2,d0				; they should match
	bne	.adrfail			; if not go to adrfail

.cont:
	sub.l	#1,d1				; Subtract 1 to d1
	cmp.l	#0,d1				; chdck if it is 0
	bne	.noprint			; if not, skip printing

	move.l	d2,d1				; restore the number of bytes between "point" to be printed
	cmp.l	#0,d3				; check if d3 was 0 if not.  we had an error and go to .err
	bne.s	.err
	KPRINTC _dottxt			; print a "."
	bchg	#1,$bfe001
	move.b	$dff006,$dff181
	clr.l	d3				; clear d3
	bra	.noerr				; do next
.err:
	KPRINTC _Etxt				; print an "E" to tell we had an error
	move.l	a7,d3
	bset	#30,d3
	move.l	d3,a7				; ok as we cannot use the Stack, I use the stackpointer (a7) as a statusregister... setting bit 31
						; high means that we had an error.  Will still continue but it is noted.
	clr.l	d3				; clear d3 to handle errors next time

.noerr:
.noprint:
	add.l	#4,a2				; Add so we check next longword
	cmp.l	a2,a1				; are we done?
	bhi	.adrloop			; no.  lets do it one more time
	bra	.done				; yeah DONE.  lets stop...
.adrfail:
	add.l	#1,d3				; Add 1 to D3, more or less just to tell we had an error
	bra	.cont
.done:

	KPRINTC	_startMemDetectTxt

	lea	$ffffffff,a1
	lea	$0,a2

; ******************************************************************************************************************
;
; 	Lets do some memorydetection. So lets scan memory from $400 and up to max 2MB. (we do not care about emulators doing more then 2MB chip)
;	We scan 64k blocks, even if I think 256 is the least configuration available on any Amiga.
;
; ******************************************************************************************************************



	lea	$0,a6				; set A6 to $0 this is the first block we check. (even if TECHNICALLY everything below $400 will be ignored later)
	clr.l	d4				; Clear the OK bit register
.detectloop:

	lea	memTestPattern,a4		; Get the testpattern
	clr.l	d0				; Clear register for tempdata
	clr.l	d1				; Clear register for readdata
	clr.l	d2				; Clear register for "failurebits"
.memcheckloop:
	move.w	#$0f0,$dff180			; Set Screencolor to GREEN
	move.l	(a4)+,d0			; get next value to test
	move.l	d0,(a6)			; write it into ram.
	nop					; Just wait some cycles.  
	nop
	move.l	#0,$3e0			; Write uninteresting data to an address we do NOT test all just to be sure we
						; do not use data from a buffer or whatever.
	nop					; just add some nop to wait some cycles
	nop
	move.l	(a6),d1			; read the real value
	cmp.l	#$100000,a6
	blt	.erro
	cmp.l	#$120000,a6
	bgt	.erro
	bclr	#3,d1		; create an error
	bset	#4,d1		; create another error
.erro:
	eor.l	d0,d1				; IF the read was the same as the write, d1 should be 0
	or.l	d1,d2				; we OR in the data to d2, when all is done d2 will contain a list of all failed bits.
	cmp.l	#0,d0				; was d0 zero, then we have tested all bitpatterns.
	bne.s	.memcheckloop
	cmp.l	#0,d2				; was it ok?
	beq	.wehadnoerr
	cmp.l	#$0,a2				; Check if A2 is 0
	bne	.wehadnoerr			; no it wasn't so we do not print a newline
	KPRINTC _newlinetxt			; Write a newline
.wehadnoerr:
	KPRINTC	_CheckAdrTxt		; Print out string about addr to be checked
	move.l	a6,d0
	KPRINTLONG				; Print out memaddress

	cmp.l	#0,d2				; if d2 is 0 we had no errors and this memaddress should be fine!
	beq	.okram
						; d2 was bad.  we HAD errors.. lets print it out.
	move.w	#$f00,$dff180			; Set sceencolor to RED.
	bra	.badram			; Go to badram
.okram:
	cmp.l	#$ffffffff,a1				; Check if A1 is ffffffff?
	bne	.nonewram			; no it wasn't so we already have a memstart
	move.l	a6,a1				; ok we didn't have any new working ram. so make a note of where this block started.
	move.l	#"MEM!",(a1)			; Write the string MEM! to first longword as a small test.
	move.l	a7,d7
	bset	#23,d7				; Ser bit 29 to tell we have found mem in block 1
	move.l	d7,a7
.nonewram:
	cmp.l	#0,a2
	beq	.firstblock
	cmp.l	#"MEM2",(a3)			; did we have this stored already?
	beq	.firstblock			; yes so skip
	move.l	a6,a3				; so we already HAD a block  lets save data about this extrablock we found.
	move.l	a7,d7
	bset	#22,d7				; Ser bit 28 to tell we have found mem in block 2
	move.l	d7,a7
	move.l	#"MEM2",(a3)			; store a string to be able to test.

.firstblock:
	move.w	#$000,$dff180			; Set Screencolor to BLACK
	add.l	#1,d4				; Add one to OK counter
	lea	_OKtxt,a0			; Print we had ok memory
	KPRINT	
	move.l	d4,d0
	DBINDEC				; Print number of OK blocks
	KPRINT
	KPRINTC _gobacktxt
.nextram:

	add.l	#64*1024,a6			; Add 64k for next block
	cmp.l	#$200000,a6			; did we exceed 2MB of chipmem?
	blt	.detectloop			; No, test more

.badram:
	cmp.l	#0,a3				; Check if A3 is 0, if not we had a 2:nd block of ram
	bne	.wehad2nd
	cmp.l	#$ffffffff,a1				; Check if A1 is 0, we had no good ram.
	beq	.nogoodram
	cmp.l	#0,a2				; had we already a end of a good block?
	bne	.nogoodram			; Wrong label, but we had so skip ths..
	move.l	a6,a2
	move.l	a6,4(a1)			; Write the end of the block to 1nd longword at found block
	bra	.nogoodram
.wehad2nd:
	move.l	a6,4(a3)
.nogoodram:
	cmp.l	#$1ff000,a6			; check again..  was we out of loop then we end
	bgt	.wearedone
	KPRINTC	_FAILEDtxt		; Write we had failed ram
.ok:
	move.l	#31,d1				; Make a loop to go through all bits
.bitloop:
	btst	d1,d2				; check if bit at D1 was ok or not
	beq	.okbit				; it was, go to okbit
	KPRINTC	_bitbadtxt		; it wasn't, write we had a bad bit
.bitdone:
	dbf	d1,.bitloop			; loop through all bits
	KPRINTC	_newlinetxt
	bra	.nextram			; Do next block
.okbit:
	KPRINTC	_bitoktxt		; Write good bit
	bra	.bitdone			; loop

.wearedone:

	cmp.l	#$ffffffff,a1			; check if A1 had a real address
	beq	.nochipfound
	KPRINTC _newlinetxt
	KPRINTC _blockfound
	move.l	a1,d0
	KPRINTLONG				; Print out memaddress
	KPRINTC _andtxt
	move.l	a2,d0
	sub.l	#1,d0
	KPRINTLONG				; Print out memaddress

	cmp.l	#"MEM2",a3			; check if A3 had a real address
	beq	.no2nd
	KPRINTC _2ndblockfound
	move.l	a3,d0
	KPRINTLONG
	KPRINTC _andtxt
	move.l	4(a3),d0
	sub.l	#1,d0
	KPRINTLONG
.no2nd:
	.nochipfound:
	; no chipmem found. fix this!
; ******************************************************************************************************************
;
; Memoryblocks found, lets see if it can be correctly addressed
;
; ******************************************************************************************************************
	lea	.after,a5
	bra	CheckAdrErr
.after:
	swap	d3
.after2:
	cmp.w	#0,d3
	bne.s	.after2
.craploop:
	move.b	$dff006,$dff181
	bra	.craploop
	;KPRINTC	String2

;move.l	#255,d0
;	DBINDEC
;	KPRINT
;	move.l	#255,d0
;	DBINHEX
;	KPRINT


	lea	_checksums,a0
		
		loop:
			;move.b	$dff006,$dff181
			bra	loop
	

		KPRINTC	AnsiNull


;		End of earlystartup.  ok we have register in use lets document them here:
;		A6 = pointer to first accessable memory to use as workspace, this pointer is static.  A6 is NOT to be touched as register
; 		from this point. (with the ONLY exception of moduleplaying)
;		A7 is a statusregister.. from this point.. this is explanation of all set bits:
;
;		Bit
;		31	=	We had too many serial timeouts disable serial out
;		30	=	We had an addresserror in romscan.
;		23	=	We found chipmem at block 1
;		22	=	We found chipmem at block 2

POSTBusError:				; Hardcoded pointers
POSTAddressError:			; if something is wrong rom starts at $0
POSTIllegalError:			; so this will actually be pointers to
POSTDivByZero:				; traps.
POSTChkInst:
POSTTrapV:
POSTPrivViol:
POSTTrace:
POSTUnimplInst:
		rts


		DumpSerial:
			move.l	a7,d7
			btst	#31,d7				; Check if timeoutbit is set.. if so skip this
			bne	.nomore
			move.w	#$4000,$dff09a
			move.w	#32,$dff032			; Set the speed of the serialport (115200BPS)
			move.b	#$4f,$bfd000			; Set DTR high
			move.w	#$0801,$dff09a
			move.w	#$0801,$dff09c
		
			clr.l	d7				; Clear d7
		.loop:
			move.b	(a0)+,d7
			cmp.b	#0,d7				; end of string?
			beq	.nomore			; yes
			clr.l	d6
			move.l	#7000,d6			; Load d6 with a timeoutvariable. only test this number of times.
								; if paula cannot tell if serial is output we will not end up in a wait-forever-loop.
								; and as we cannot use timers. we have to do this dirty style of coding...

			swap	d6				; Swap d6 so we use other 16 bits for other data (to not use too many registers)
		.timeoutloop:	
			move.b	$bfe001,d6			; just read crapdata, we do not care but reading from CIA is slow... for timeout stuff only
			swap	d6				; Swap it back now again
			sub.w	#1,d6				; count down timeout value
			cmp.w	#0,d6				; if 0, timeout.
			beq	.endloop
			swap	d6
			move.w	$dff018,d6
			btst	#13,d6				; Check TBE bit
			beq.s	.timeoutloop
			bra	.notimeout
		.endloop:
			bchg	#1,$bfe001
			swap	d6
			move.w	d6,$dff180
			swap	d7				; Swap d7, so we use the top 16 bits for a timeout-counter
			add.w	#1,d7				; Add 1 to d7 that we had a (yet anoter?) timeout
			cmp.w	#22,d7
			beq	.onetoomany
			swap	d7
		.notimeout:
			move.w	#$0100,d6
			move.b	d7,d6
			move.w	d6,$dff030			; send it to serial
			move.w	#$0001,$dff09c		; turn off the TBE bit
			bra.s	.loop
		.nomore:
			jmp	(a5)
		.onetoomany:
			; OK we had one too many timeouts
			move.l	a7,d7
			bset	#31,d7				; Ser bit 31 to tell we had too many serial timeouts
			move.l	d7,a7
			bra	.nomore			

CheckAdrErr:	
	move.l	a5,d2
	move.l	a2,a4			; copy end of first block to a4.
	move.l	a2,d0
	move.l	a1,d1
	sub.l	d1,d0			; D0 now contains how many bytes of memory found..

	KPRINTC	_block1adrtxt
	KPRINTC	_adrfilltxt
	;IN:
	;	A4 = End of block
	;	D0 = Number of bytes of block
	;	D1 = Start of block
	;	sub.l	d0,a6
	move.l	a4,a6
	sub.l	d0,a6			; a6 now (temporary) contains a pointer to the beginning of the block
	move.l	d0,d3			; make a copy of size to d3
	asr.l	#2,d3			; get number of longwords to be written
	asr.l	#5,d3
	move.l	d3,d4
	move.l	a6,d1
	; usable registers:
	;	d1,d2 (d0,d6,d7 until serial out)
	; d3 is a countdown and d4 is a stored value of longwords between "."
	.adrloop:
		sub.l	#4,a4

		move.l	a4,d7
		bclr	#2,d7
		move.l	d7,a0		;create error..  use a0 as temp instead of a4

		move.l	a0,(a4)		; Write the address to the address,  so $400 contains $400 etc.
		cmp.l	$400,a4
		ble	.endloop		; If we are lower then $400 end this
		cmp.l	a6,a4
		beq	.endloop		; if we are at end of block, end this
		sub.l	#1,d3			; count down 1 do d3
		cmp.l	#0,d3			; check if it is 0.  (time to print a .)
		bne.s	.adrloop		; no. so
		move.l	d4,d3			; Reset the counter
		KPRINTC _dottxt		; print a "."
		bchg	#1,$bfe001		; Flash the LEd
		btst	#1,$bfe001
		beq	.yellow		; Set color on screen depending on status of LED
		move.w	#$00f,$dff180
		bra	.adrloop
	.yellow:
		move.w	#$ff0,$dff180
		bra	.adrloop
						; We have filled the block with addressdata
	.endloop:
		cmp.l	$400,a6		; check if the beginning is lower then 400
		blt	.low400		; if it was lower then 400 set it to 400 so we ignore first 1kb of data used for registers etc
	.endloop2:
		; ok we now have filled the block of ram with its memaddress, lets test if it stay the same
		KPRINTC _adrtesttxt

		move.l	a6,a4
		move.l	d1,a6			; restore Startaddress to a6
		add.l	d0,a6
		sub.l	#4,a4			; Subtract first address with 4.  just to be lazy at next step
		move.l	d4,d3			; Restore the "." counter
	.adrloop2:
		add.l	#4,a4
		cmp.l	a6,a4
		bge	.end

		cmp.l	$400,a4
		ble	.adrloop2

		move.l	a4,d7
		bclr	#2,d7
		move.l	d7,a0		;create error..  use a0 as temp instead of a4
		
		move.l	a0,d0
		cmp.l	(a4),d0		;check if content in address is correct
		bne	.error
	.errdone:
		sub.w	#1,d3
		cmp.w	#0,d3
		bne	.adrloop2
		move.w	d4,d3
		KPRINTC	_dottxt
		bchg	#1,$bfe001
		btst	#1,$bfe001
		beq	.yellow2
		move.w	#$00f,$dff180
		bra	.adrloop2
	.yellow2:
		move.w	#$ff0,$dff180
		bra	.adrloop2
	.end:
		move.l	d2,a5
		jmp	(a5)
	.error:
		swap	d3
		add.w	#1,d3
		cmp.w	#10,d3
		beq	.toomany
		swap	d3
		move.w	#$f00,$dff180
		KPRINTC	_adrerr
		move.l	a4,d0
		KPRINTLONG
		KPRINTC	_adrerr2
		move.l	a4,d7
		bclr	#2,d7
		move.l	d7,a0			;error
		move.l	(a0),d0
							KPRINTLONG
							KPRINTC	_adrerr3
			move.l	a4,d7
			bclr	#2,d7
			move.l	d7,a0			; error
			move.l	(a0),d0
			move.l	a4,d1
			eor.l	d1,d0
			move.w	#31,d1
		.bitloop:
			btst	d1,d0
			beq	.okbit				; it was, go to okbit
			KPRINTC	_bitbadtxt		; it wasn't, write we had a bad bit
		.bitdone:
				dbf	d1,.bitloop			; loop through all bits
			KPRINTC	_newlinetxt
			bra	.done			; Do next block
		.okbit:
		KPRINTC	_bitoktxt		; Write good bit
		bra	.bitdone			; loop
		.done:	
			bra	.errdone
	.low400:
		lea	$400,a6
		bra	.endloop2

	.toomany:
		KPRINTC	_adrerr4
		bra	.end
		
		; ************************************************ Lets check it is stored correct
		
			KPRINTC	_adrtesttxt
		
					
				
		
				
		


			;			KPRINTC _dottxt			; print a "."
;			KPRINTC	_adrerr
;			KPRINTLONG				; Print out memaddress
;			KPRINTC	_adrerr2
;			move.l	d3,d0;
;			KPRINTLONG				; Print out content
;			KPRINTC	_adrerr3
			move.l	d5,a5
			jmp	(a5)				; exit this


DumpBinHex:
	lea	Hexnumbers,a0
	clr.l	d7
	move.b	d0,d7
	asl.l	#2,d7
	add.l	d7,a0
	jmp	(a5)
			
DumpBinDec:
	lea	Decnumbers,a0
	asl.l	#2,d0
	add.l	d0,a0
	jmp	(a5)
				
		;Call C crap
				jsr	_test_function

memTestPattern:
		dc.l	$ffffffff,$f0f0f0f0,$0f0f0f0f,$f0f00f0f,$0f0ff0f0,$ffff0000,$0000ffff,$aaaaaaaa,$55555555,$aaaa5555,$5555aaaa,0



_diagRomTxt:
	dc.b	12,27,"[0m",27,"[40m",27,"[37m"
	dc.b	"DiagROM Amiga Diagnostic by John Hertell. "
	VERSION
	dc.b " "
	incbin	"builddate.i"
	dc.b $a,$d,0

_Initmousetxt:
		dc.b	$a,$d,"    Checking status of mousebuttons at power-on: ",$a,$d
		dc.b	"            ",0
_releasemousetxt:
		dc.b	$a,$d,"Release mousebuttons now or they will be tagged as STUCK and ignored!",0
_block1adrtxt:
	dc.b	$a,$d,$a,$d,"Checking block 1 for addresserrors",0
_adrfilltxt:
	dc.b	$a,$d,"Filling area with addressdata",$a,$d,0
_adrtesttxt:
	dc.b	$a,$d,"Comparing if addressdata is correct",$a,$d,0
_adrerr:
	dc.b	$a,$d,27,"[31mAdrerr:",27,"[0m $",0
_adrerr2:
	dc.b	" read: $",0
_adrerr3:
	dc.b	" Diff is: ",0
_adrerr4:
	dc.b	$a,$d,"Too many addresserrors, this block is unusable!",0
_InitP1LMBtxt:
	dc.b	"P1LMB ",0
_InitP2LMBtxt:
	dc.b	"P2LMB ",0
_InitP1RMBtxt:
	dc.b	"P1RMB ",0
_InitP2RMBtxt:
	dc.b	"P2RMB ",0
_InitP1MMBtxt:
	dc.b	"P1MMB ",0
_InitP2MMBtxt:
	dc.b	"P2MMB ",0

_diagRomCheckadrtxt:
	dc.b $a,$d,$a,$d,"Checking addressdata of ROM-Space",$a,$d,0
_startMemDetectTxt:
	dc.b	$a,$d,$a,$d,"Scan for usable Chipmem (!!NOTE THIS IS NOT A MEMORYTEST IT *IS* A SCAN)",$a,$d,0


_dottxt:
	dc.b	".",0

_Etxt:
	dc.b	"E",0
_bitoktxt:
	dc.b	27,"[32m-",0
_bitbadtxt:
	dc.b	27,"[31mX",0
_CheckAdrTxt:
	dc.b	"Check address: $",0
_gobacktxt:
	dc.b	$d,0
_OKtxt:
	dc.b " - ",27,"[32mOK",27,"[0m  Number of 64K blocks found: ",0

_FAILEDtxt:
	dc.b	" - ",27,"[31mUNUSABLE:",27,"[0m D31-D0 ",0
_newlinetxt:
	dc.b	27,"[0m",$a,$d,0
_nulltxt:
	dc.b	27,"[0m",$d,0
_blockfound:
	dc.b	$a,$d,27,"[0mChipmem found beween: ",0
_2ndblockfound:
	dc.b	$a,$d,27,"[0mALSO a 2nd Chipmem block found between: ",0
_andtxt:
	dc.B	" and ",0
Decnumbers:
	dc.b "0",0,0,0
	dc.b "1",0,0,0
	dc.b "2",0,0,0
	dc.b "3",0,0,0
	dc.b "4",0,0,0
	dc.b "5",0,0,0
	dc.b "6",0,0,0
	dc.b "7",0,0,0
	dc.b "8",0,0,0
	dc.b "9",0,0,0
	dc.b "10",0,0
	dc.b "11",0,0
	dc.b "12",0,0
	dc.b "13",0,0
	dc.b "14",0,0
	dc.b "15",0,0
	dc.b "16",0,0
	dc.b "17",0,0
	dc.b "18",0,0
	dc.b "19",0,0
	dc.b "20",0,0
	dc.b "21",0,0
	dc.b "22",0,0
	dc.b "23",0,0
	dc.b "24",0,0
	dc.b "25",0,0
	dc.b "26",0,0
	dc.b "27",0,0
	dc.b "28",0,0
	dc.b "29",0,0
	dc.b "30",0,0
	dc.b "31",0,0
	dc.b "32",0,0
	dc.b "33",0,0
	dc.b "34",0,0
	dc.b "35",0,0
	dc.b "36",0,0
	dc.b "37",0,0
	dc.b "38",0,0
	dc.b "39",0,0
	dc.b "40",0,0
	dc.b "41",0,0
	dc.b "42",0,0
	dc.b "43",0,0
	dc.b "44",0,0
	dc.b "45",0,0
	dc.b "46",0,0
	dc.b "47",0,0
	dc.b "48",0,0
	dc.b "49",0,0
	dc.b "50",0,0
	dc.b "51",0,0
	dc.b "52",0,0
	dc.b "53",0,0
	dc.b "54",0,0
	dc.b "55",0,0
	dc.b "56",0,0
	dc.b "57",0,0
	dc.b "58",0,0
	dc.b "59",0,0
	dc.b "60",0,0
	dc.b "61",0,0
	dc.b "62",0,0
	dc.b "63",0,0
	dc.b "64",0,0
	dc.b "65",0,0
	dc.b "66",0,0
	dc.b "67",0,0
	dc.b "68",0,0
	dc.b "69",0,0
	dc.b "70",0,0
	dc.b "71",0,0
	dc.b "72",0,0
	dc.b "73",0,0
	dc.b "74",0,0
	dc.b "75",0,0
	dc.b "76",0,0
	dc.b "77",0,0
	dc.b "78",0,0
	dc.b "79",0,0
	dc.b "80",0,0
	dc.b "81",0,0
	dc.b "82",0,0
	dc.b "83",0,0
	dc.b "84",0,0
	dc.b "85",0,0
	dc.b "86",0,0
	dc.b "87",0,0
	dc.b "88",0,0
	dc.b "89",0,0
	dc.b "90",0,0
	dc.b "91",0,0
	dc.b "92",0,0
	dc.b "93",0,0
	dc.b "94",0,0
	dc.b "95",0,0
	dc.b "96",0,0
	dc.b "97",0,0
	dc.b "98",0,0
	dc.b "99",0,0
	dc.b "100",0
	dc.b "101",0
	dc.b "102",0
	dc.b "103",0
	dc.b "104",0
	dc.b "105",0
	dc.b "106",0
	dc.b "107",0
	dc.b "108",0
	dc.b "109",0
	dc.b "110",0
	dc.b "111",0
	dc.b "112",0
	dc.b "113",0
	dc.b "114",0
	dc.b "115",0
	dc.b "116",0
	dc.b "117",0
	dc.b "118",0
	dc.b "119",0
	dc.b "120",0
	dc.b "121",0
	dc.b "122",0
	dc.b "123",0
	dc.b "124",0
	dc.b "125",0
	dc.b "126",0
	dc.b "127",0
	dc.b "128",0
	dc.b "129",0
	dc.b "130",0
	dc.b "131",0
	dc.b "132",0
	dc.b "133",0
	dc.b "134",0
	dc.b "135",0
	dc.b "136",0
	dc.b "137",0
	dc.b "138",0
	dc.b "139",0
	dc.b "140",0
	dc.b "141",0
	dc.b "142",0
	dc.b "143",0
	dc.b "144",0
	dc.b "145",0
	dc.b "146",0
	dc.b "147",0
	dc.b "148",0
	dc.b "149",0
	dc.b "150",0
	dc.b "151",0
	dc.b "152",0
	dc.b "153",0
	dc.b "154",0
	dc.b "155",0
	dc.b "156",0
	dc.b "157",0
	dc.b "158",0
	dc.b "159",0
	dc.b "160",0
	dc.b "161",0
	dc.b "162",0
	dc.b "163",0
	dc.b "164",0
	dc.b "165",0
	dc.b "166",0
	dc.b "167",0
	dc.b "168",0
	dc.b "169",0
	dc.b "170",0
	dc.b "171",0
	dc.b "172",0
	dc.b "173",0
	dc.b "174",0
	dc.b "175",0
	dc.b "176",0
	dc.b "177",0
	dc.b "178",0
	dc.b "179",0
	dc.b "180",0
	dc.b "181",0
	dc.b "182",0
	dc.b "183",0
	dc.b "184",0
	dc.b "185",0
	dc.b "186",0
	dc.b "187",0
	dc.b "188",0
	dc.b "189",0
	dc.b "190",0
	dc.b "191",0
	dc.b "192",0
	dc.b "193",0
	dc.b "194",0
	dc.b "195",0
	dc.b "196",0
	dc.b "197",0
	dc.b "198",0
	dc.b "199",0
	dc.b "200",0
	dc.b "201",0
	dc.b "202",0
	dc.b "203",0
	dc.b "204",0
	dc.b "205",0
	dc.b "206",0
	dc.b "207",0
	dc.b "208",0
	dc.b "209",0
	dc.b "210",0
	dc.b "211",0
	dc.b "212",0
	dc.b "213",0
	dc.b "214",0
	dc.b "215",0
	dc.b "216",0
	dc.b "217",0
	dc.b "218",0
	dc.b "219",0
	dc.b "220",0
	dc.b "221",0
	dc.b "222",0
	dc.b "223",0
	dc.b "224",0
	dc.b "225",0
	dc.b "226",0
	dc.b "227",0
	dc.b "228",0
	dc.b "229",0
	dc.b "230",0
	dc.b "231",0
	dc.b "232",0
	dc.b "233",0
	dc.b "234",0
	dc.b "235",0
	dc.b "236",0
	dc.b "237",0
	dc.b "238",0
	dc.b "239",0
	dc.b "240",0
	dc.b "241",0
	dc.b "242",0
	dc.b "243",0
	dc.b "244",0
	dc.b "245",0
	dc.b "246",0
	dc.b "247",0
	dc.b "248",0
	dc.b "249",0
	dc.b "250",0
	dc.b "251",0
	dc.b "252",0
	dc.b "253",0
	dc.b "254",0
	dc.b "255",0

Hexnumbers:
			dc.b "00",0,0
			dc.b "01",0,0
			dc.b "02",0,0
			dc.b "03",0,0
			dc.b "04",0,0
			dc.b "05",0,0
			dc.b "06",0,0
			dc.b "07",0,0
			dc.b "08",0,0
			dc.b "09",0,0
			dc.b "0A",0,0
			dc.b "0B",0,0
			dc.b "0C",0,0
			dc.b "0D",0,0
			dc.b "0E",0,0
			dc.b "0F",0,0
			dc.b "10",0,0
			dc.b "11",0,0
			dc.b "12",0,0
			dc.b "13",0,0
			dc.b "14",0,0
			dc.b "15",0,0
			dc.b "16",0,0
			dc.b "17",0,0
			dc.b "18",0,0
			dc.b "19",0,0
			dc.b "1A",0,0
			dc.b "1B",0,0
			dc.b "1C",0,0
			dc.b "1D",0,0
			dc.b "1E",0,0
			dc.b "1F",0,0
			dc.b "20",0,0
			dc.b "21",0,0
			dc.b "22",0,0
			dc.b "23",0,0
			dc.b "24",0,0
			dc.b "25",0,0
			dc.b "26",0,0
			dc.b "27",0,0
			dc.b "28",0,0
			dc.b "29",0,0
			dc.b "2A",0,0
			dc.b "2B",0,0
			dc.b "2C",0,0
			dc.b "2D",0,0
			dc.b "2E",0,0
			dc.b "2F",0,0
			dc.b "30",0,0
			dc.b "31",0,0
			dc.b "32",0,0
			dc.b "33",0,0
			dc.b "34",0,0
			dc.b "35",0,0
			dc.b "36",0,0
			dc.b "37",0,0
			dc.b "38",0,0
			dc.b "39",0,0
			dc.b "3A",0,0
			dc.b "3B",0,0
			dc.b "3C",0,0
			dc.b "3D",0,0
			dc.b "3E",0,0
			dc.b "3F",0,0
			dc.b "40",0,0
			dc.b "41",0,0
			dc.b "42",0,0
			dc.b "43",0,0
			dc.b "44",0,0
			dc.b "45",0,0
			dc.b "46",0,0
			dc.b "47",0,0
			dc.b "48",0,0
			dc.b "49",0,0
			dc.b "4A",0,0
			dc.b "4B",0,0
			dc.b "4C",0,0
			dc.b "4D",0,0
			dc.b "4E",0,0
			dc.b "4F",0,0
			dc.b "50",0,0
			dc.b "51",0,0
			dc.b "52",0,0
			dc.b "53",0,0
			dc.b "54",0,0
			dc.b "55",0,0
			dc.b "56",0,0
			dc.b "57",0,0
			dc.b "58",0,0
			dc.b "59",0,0
			dc.b "5A",0,0
			dc.b "5B",0,0
			dc.b "5C",0,0
			dc.b "5D",0,0
			dc.b "5E",0,0
			dc.b "5F",0,0
			dc.b "60",0,0
			dc.b "61",0,0
			dc.b "62",0,0
			dc.b "63",0,0
			dc.b "64",0,0
			dc.b "65",0,0
			dc.b "66",0,0
			dc.b "67",0,0
			dc.b "68",0,0
			dc.b "69",0,0
			dc.b "6A",0,0
			dc.b "6B",0,0
			dc.b "6C",0,0
			dc.b "6D",0,0
			dc.b "6E",0,0
			dc.b "6F",0,0
			dc.b "70",0,0
			dc.b "71",0,0
			dc.b "72",0,0
			dc.b "73",0,0
			dc.b "74",0,0
			dc.b "75",0,0
			dc.b "76",0,0
			dc.b "77",0,0
			dc.b "78",0,0
			dc.b "79",0,0
			dc.b "7A",0,0
			dc.b "7B",0,0
			dc.b "7C",0,0
			dc.b "7D",0,0
			dc.b "7E",0,0
			dc.b "7F",0,0
			dc.b "80",0,0
			dc.b "81",0,0
			dc.b "82",0,0
			dc.b "83",0,0
			dc.b "84",0,0
			dc.b "85",0,0
			dc.b "86",0,0
			dc.b "87",0,0
			dc.b "88",0,0
			dc.b "89",0,0
			dc.b "8A",0,0
			dc.b "8B",0,0
			dc.b "8C",0,0
			dc.b "8D",0,0
			dc.b "8E",0,0
			dc.b "8F",0,0
			dc.b "90",0,0
			dc.b "91",0,0
			dc.b "92",0,0
			dc.b "93",0,0
			dc.b "94",0,0
			dc.b "95",0,0
			dc.b "96",0,0
			dc.b "97",0,0
			dc.b "98",0,0
			dc.b "99",0,0
			dc.b "9A",0,0
			dc.b "9B",0,0
			dc.b "9C",0,0
			dc.b "9D",0,0
			dc.b "9E",0,0
			dc.b "9F",0,0
			dc.b "A0",0,0
			dc.b "A1",0,0
			dc.b "A2",0,0
			dc.b "A3",0,0
			dc.b "A4",0,0
			dc.b "A5",0,0
			dc.b "A6",0,0
			dc.b "A7",0,0
			dc.b "A8",0,0
			dc.b "A9",0,0
			dc.b "AA",0,0
			dc.b "AB",0,0
			dc.b "AC",0,0
			dc.b "AD",0,0
			dc.b "AE",0,0
			dc.b "AF",0,0
			dc.b "B0",0,0
			dc.b "B1",0,0
			dc.b "B2",0,0
			dc.b "B3",0,0
			dc.b "B4",0,0
			dc.b "B5",0,0
			dc.b "B6",0,0
			dc.b "B7",0,0
			dc.b "B8",0,0
			dc.b "B9",0,0
			dc.b "BA",0,0
			dc.b "BB",0,0
			dc.b "BC",0,0
			dc.b "BD",0,0
			dc.b "BE",0,0
			dc.b "BF",0,0
			dc.b "C0",0,0
			dc.b "C1",0,0
			dc.b "C2",0,0
			dc.b "C3",0,0
			dc.b "C4",0,0
			dc.b "C5",0,0
			dc.b "C6",0,0
			dc.b "C7",0,0
			dc.b "C8",0,0
			dc.b "C9",0,0
			dc.b "CA",0,0
			dc.b "CB",0,0
			dc.b "CC",0,0
			dc.b "CD",0,0
			dc.b "CE",0,0
			dc.b "CF",0,0
			dc.b "D0",0,0
			dc.b "D1",0,0
			dc.b "D2",0,0
			dc.b "D3",0,0
			dc.b "D4",0,0
			dc.b "D5",0,0
			dc.b "D6",0,0
			dc.b "D7",0,0
			dc.b "D8",0,0
			dc.b "D9",0,0
			dc.b "DA",0,0
			dc.b "DB",0,0
			dc.b "DC",0,0
			dc.b "DD",0,0
			dc.b "DE",0,0
			dc.b "DF",0,0
			dc.b "E0",0,0
			dc.b "E1",0,0
			dc.b "E2",0,0
			dc.b "E3",0,0
			dc.b "E4",0,0
			dc.b "E5",0,0
			dc.b "E6",0,0
			dc.b "E7",0,0
			dc.b "E8",0,0
			dc.b "E9",0,0
			dc.b "EA",0,0
			dc.b "EB",0,0
			dc.b "EC",0,0
			dc.b "ED",0,0
			dc.b "EE",0,0
			dc.b "EF",0,0
			dc.b "F0",0,0
			dc.b "F1",0,0
			dc.b "F2",0,0
			dc.b "F3",0,0
			dc.b "F4",0,0
			dc.b "F5",0,0
			dc.b "F6",0,0
			dc.b "F7",0,0
			dc.b "F8",0,0
			dc.b "F9",0,0
			dc.b "FA",0,0
			dc.b "FB",0,0
			dc.b "FC",0,0
			dc.b "FD",0,0
			dc.b "FE",0,0
			dc.b "FF",0,0
