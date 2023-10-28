		include "earlymacros.i"
		include "globalvars.i"

		section "startup",code_p

		XDEF	AnsiNull

	;	This is where it all starts
	;	I use 7 as Tab size  as it fits better for asm..
	;	Any comment of "printing" in this part of the code means out to serialport only.


_start:
		dc.b	"DIAG"
		dc.l	_begin	
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
	cmp.l	#0,d1				; check if it is 0
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
	bra	done				; yeah DONE.  lets stop...
.adrfail:
	add.l	#1,d3				; Add 1 to D3, more or less just to tell we had an error
	bra	.cont

done:

; ******************************************************************************************************************
;
; Detect chipmem
;
; ******************************************************************************************************************


	KPRINTC	_startMemDetectTxt

	lea	$0,a6			; Start to detect ram at $400, as below that is cpu controlstuff only anyways.
	lea	$0,a3
.chiploop:
	lea	.memcheckdone,a4
	jmp	.memcheck

.memcheckdone:
	; a2 == -1 if no mem was found
	cmp.l	#-1,a2
	bne		.noerr
.a:	bra	.a
	.noerr:
	move.l	#-1,d1
	move.l	#-1,d2
	move.l	#-1,d3
	move.l	#-1,d4
	move.l	#-1,d5
	move.l	#-1,d6
	move.l	#-1,d7
	lea	-1,a0
	lea	-1,a1
	lea	-1,a3
	lea	-1,a5

	lea	.adrcheckdone,a4		; Set address to jump to after check
	jmp	.adrcheck			; Do a check of addresserrors of this block
.adrcheckdone:
	add.l	#4,a2				; ok this block is done, to be sure add 4 to get to next longword of next block if needed
	move.l	a2,d2				; Copy address of end of block scanned to d2
.noadrerr:
	cmp.l	#$40000,d1
	bge	.enoughmem
	cmp.l	#$200000,d2
	bge	.nochip
	KPRINTC _notenoughtxt
	asr.l	#8,d2
	asr.l	#7,d2				; Divide next block by 64k
	add.l	#1,d2				; add one!
	asl.l	#8,d2
	asl.l	#7,d2				; Multiply number with 64k to get next block
	move.l	d2,a6
	lea	$0,a3
	clr.l	d4
	bra	.chiploop
.nochip:
	bra	.nochip
.enoughmem:
	KPRINTC	_blockok
	move.l	#-1,d0
	move.l	#-1,d1
	move.l	#-1,d2
	move.l	#-1,d3
	move.l	#-1,d4
	move.l	#-1,d5
	move.l	#-1,d6
	move.l	#-1,d7
	lea	-1,a0
	lea	-1,a1
	lea	-1,a2
	lea	-1,a4
	lea	-1,a5

						; CHIPMEM TEST DONE!   A3 = end of block, A6 = Beginning of block

	; setup stack
	move.l	#4096,stack_size(a6)		; store our stack size
	lea.l	GlobalVars_sizeof(a6),sp	; stack area starts right after global vars
	move.l	sp,stack_mem(a6)			; store the address of the stack
	adda.l	stack_size(a6),sp			; stack grows backwards

.aa:
	bsr		_test_function (a6)
	; d0 = $600dc0de 
	cmp.l	#$600dc0de,d0
	bne.b	.error

	move.l	current_vhpos(a6),d0	; read back the vhpos
	lsr.w	#8,d0					; we want the lower vertical byte
.error:
	move.b	d0,$dff181				; color the background
	bra	.aa






.memcheck:					; IN:
						;	A6 = Memadress to start scan from
						;	A4 = jumpback address.
						; OUT:
						;	A2 = Start of found mem. if -1, none found
						;	D3 = Number of 64K blocks found
						;	D0 = Last memadress of mem found. (as a bonus due to printout)
	move.l	a6,d6
	and.l	#$ffff0000,d6			; mask out so we are at the beginning of a 64k block
	move.l	d6,a6
	clr.l	d6				; Make sure d6 is clean
	clr.l	d3				; Clear blocks found
	lea	-1,a2				; Set a2 to -1 so we know if we had a working block or not (-1 == no working block yet)
.more:
	clr.l	d2				; clear d2 that contains biterrordata
	lea	_mempattern,a1		; load a1 with start of mempattern to test.
.testmore:
	cmpa.l	#$200000,a6			; Have we reached the end of chipmem?
	bge	.comparedone			; YUPP!
	move.l	(a1)+,d0		; load first value into d0
	move.l	d0,(a6)			; write that value into where a1 points to.
	nop
	nop						; just do some nops.. to take some time.
	move.l	(a6),d1			; load value where a1 points to d1

	cmpa.l	#128*1024,a6	; check to create an error
	ble	.nott				; lines to be removed
	cmpa.l	#1024*1024,a6
	bge	.nott
	;bset	#3,d1
.nott:

	;bclr	#1,d1			; create an error

	eor.l	d0,d1				; eor value what was written to what was read, d1 now will contain what bits was different
	or.l	d1,d2				; or that value into d2, so d2 will contain a list of all bad bits.
	cmp.l	#0,d0				; Was d0 = 0 then we was at end of list, so this is fully tested.
	bne	.testmore			; if not, do the loop some more

	cmp.l	#0,d2				; if d2 was 0, then we had no errors.
	beq	.noerror
	move.w	#$f00,$dff180			; Set Red screencolor
	cmpa.l	#-1,a2				; Check if we had found ram, if so this is the end of this block.
	bne	.endblock
	KPRINTC _newlinetxt			; We had an error, so print that
	KPRINTC _addrtxt
	move.l	a6,d0
	KPRINTLONG
	KPRINTC _Errortxt
	move.w	#1,d6				; Set that we had an error
	swap	d6					; Swap d6 so we can use other 16 bits for the loop
	move.w	#31,d6				; Set counter to 31 for bits to print.   To be able to print out the biterrorpattern
.loop:
	btst	d6,d2				; Check if it was error on this bit or not
	beq	.correct
	KPRINTC	_errtxt		; it was, print an red X
.goon:
	dbf	d6,.loop			; Loop until all bits are done
	swap d6						; Swap back d6 to use other 16 bits as a errordetectionvariable
	bra	.donext			; ok we have handled the printing of the biterrorpattern so jump to donext
.correct:
	KPRINTC	_dashtxt		; no biterror, print a green -
	bra	.goon				; goto loop


.noerror:
	cmp.w	#0,d6				; check d6 if lower d6 is 0, if it is we had no error
	beq	.noformererror
	KPRINTC _newlinetxt
	clr.w	d6
.noformererror
	cmpa.l	#-1,a2				; If A2 is -1, we have not found any ram yet.
	bne	.nonew
	move.l	a6,a2				; Store the current address to a2 to mark as beginning of memblock.
.nonew:
	KPRINTC	_addrtxt
	move.l	a6,d0
	KPRINTLONG
	KPRINTC	_memoktxt
	add.l	#1,d3				; Add 1 to found blocks
	move.l	d3,d0
	DBINDEC
	KPRINT
	KPRINTC	_beginrowtxt
.donext:

	cmpa.l	#$200000,a6			; Have we reached the end of chipmem?
	bge	.comparedone			; YUPP!
	add.l	#64*1024,a6			; Add 64k to next block to look for
	bra	.more				; And loop again

.comparedone:
	cmp.l	#-1,a2				; did we find any ram?
	beq	.foundnomem
	KPRINTC _hometxt			; we found mem.  make it look so last scanned mem is last address just for the look of it
	KPRINTC _addrtxt
	move.l	a6,d0
	KPRINTLONG
.foundnomem:
	KPRINTC _newlinetxt
	lea	-1,a6
	jmp	(a4)				; Jump back after test
.endblock:
	KPRINTC _blockfoundtxt
	move.l	a2,d0
	KPRINTLONG
	KPRINTC _blockfound2txt
	move.l	d3,d0
	asl.l	#8,d0
	asl.l	#8,d0
	add.l	a2,d0
	sub.l	#1,d0
	KPRINTLONG
	lea	-1,a6
	jmp	(a4)				; Jump back after test

.adrcheck:					; Check for addresserrors
; IN:
						;	A2 = Startaddress
						;	D0 = Endaddress
						; OUT:
						;	A2 = Startaddress of ok block -1 if none
						;	A3 = endaddress of ok block
						;	D1 = Bytes of ok mem
						;	A2 = End of block originally scanned.
	cmp.l	#-1,a2
	beq	.nomemfound
	KPRINTC	_adrtest
	move.l	a2,d3
	and.l	#$fffffffc,d3			; Make sure is is even to a longword
	move.l	d3,a2
	and.l	#$fffffffc,d0			; Make sure is is even to a longword
	cmp.l	#$400,a2			; Check if begining is less then $400
	bge	.nozero			; nope.. skip next
	lea	$400,a2			; as we was at $0, set it to $400 not to screw up stuff
.nozero:
	move.l	d0,a1
	sub.l	#4,a1				; a1 now contains a pointer to the last writeable longword in the block
	move.l	a1,a3				; Store start in a3 as a backup for the readtest
	move.l	d0,d2
	sub.l	a2,d2
	move.l	d2,d6				; number of bytes to fill
	asr.l	#2,d6				; make it to longwords
	asr.l	#6,d6				; calculate how many longwords to do before printing a "."
	clr.l	d5				; Clear d5 used as a counter
.adrcheckloop:
	cmp.l	d5,d6				; Have we reached to end of longwords before printing a dot?
	bne	.nodot
	move.w	4(a1),$dff180
	bchg	#1,$bfe001
	KPRINTC _dottxt			; print dot
	clr.l	d5				; clear counter
.nodot:
	move.l	a1,(a1)
	sub.l	#4,a1
	add.l	#1,d5				; Add counter
	cmp.l	a2,a1
	blt	.blockdone
	bra	.adrcheckloop
.blockdone:
	add.l	#4,a1				; Make sure a1 now points to first longword again
	KPRINTC _adrtest2
	move.l	#-1,d1
	move.l	#-1,d2
	move.l	#-1,d3
	move.l	#-1,d4
	move.l	#-1,d5
	move.l	#-1,d7
	lea	-1,a0
	lea	-1,a2
	lea	-1,a3
	lea	-1,a5
	lea	-1,a6
	clr.l	d4				; Clear d4 containing checkresultmask
	clr.l	d5				; Clear counter
	move.l	d0,a2				; copy end of block to a2
.adrcheckloop2:
	move.l	a1,d1				; d1 now contains address of a1
	move.l	(a1),d0			; read what a1 points to do d0
	;	bclr	#18,d0					;	CREATE AN ERROR!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	;move.l	#4424344,d0
	eor.l	d0,d1				; if d1 is 0 we had a perfect match if not, it contains errorbits
	cmp.l	#0,d1
	beq	.noadrerror
	or.l	d1,d4				; add in bad bits to d4
	cmp.l	#-1,a3				; Check if we had an endblock of mem stored.
	bne	.nonewend			; no new end, jump to nonewend
	move.l	a1,a3				; store current address and end
	bra	.nogood
.nonewend:
.noadrerror:
	cmp.l	#-1,a6				; Check if we had a startaddress? if not. set it
	bne	.nonewblock
	cmp.l	#-1,a3				; Check if we had an end of block already, if so we have no beginning
	bne	.nogood
	move.l	a1,a6				; Store current address as start of this block
.nogood:


.nonewblock:
	add.l	#4,a1				; Increase a1 to point to next longword
	cmp.l	d5,d6
	bne	.nodot2
	move.w	-4(a1),$dff180		; Do colorstuff
	bchg	#1,$bfe001			; do ledstuff
	cmp.l	#0,d1				; Check if we had an error
	beq	.noaerr
	KPRINTC _Etxt				; We had, print a red E
	bra	.noaerr2
.noaerr:
	KPRINTC _dottxt			; we did not, print a white .
.noaerr2:
	clr.l	d5
.nodot2:
	add.l	#1,d5
	cmp.l	a1,a2
	ble	.blockdone2
	bra	.adrcheckloop2
.blockdone2:
	cmp.l	#-1,a3				; Check if a3 is -1
	bne	.aerr				; if it wasn't then we had an error so we have an endaddress
	move.l	a1,a3				; copy end of this block to a3
.aerr:
	cmp.l	#0,d4				; d4 contains the errormask, if 0 we had no errors
	beq	.blockfine
	KPRINTC	_blockerr
	move.l	#31,d6				; Set counter to 31 for bits to print.
.loop2:
	btst	d6,d4
	beq	.correct2
	
	KPRINTC	_errtxt
.goon2:
	dbf	d6,.loop2

	KPRINTC	_disclaimer
	bra	.nomemfound
.correct2:
	KPRINTC	_dashtxt
	bra	.goon2

.blockfine:
	cmp.l	#-1,a6				; had we a working startblock
	beq	.jumpout			; if not jump to noadrerror
	cmp.l	#-1,d3
	bne	.jumpout
	move.l	a1,d3
.jumpout:
	cmp.l	#-1,a6				; had we a working startblock
	beq	.nomemfoundatall		; if not jump to noadrerror
	KPRINTC _adrmemoktxt
	move.l	a6,d0
	KPRINTLONG
	KPRINTC	_blockfound2txt
	move.l a3,d0
	KPRINTLONG

	move.l	a3,d1
	move.l	a6,d6
	sub.l	d6,d1
		
	jmp	(a4)
.nomemfoundatall:
	KPRINTC _noblockfoundtxt
	clr.l	d1			; make sure 0 bytes is reported
	jmp	(a4)
.nomemfound:
	cmp.l	#-1,a6
	bne	.nounusable
	KPRINTC	_unusabletxt
.nounusable:
	bra	.jumpout

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
	cmp.b	#0,(a0)			; end of string?
	beq	.nomore			; yes
.wait:
	cmp.w	#3000,d7			; have we timed out?
	beq	.timeout
	add.w	#1,d7				; Add 1 to timeout conter
	btst	#5,$dff018			; check if byte is transmitted
	beq.s	.wait				; if not, wait some more.
.timedout:
	move.w	#$0100,d7			; Set data for serial out
	move.b	(a0)+,d7
	move.w	d7,$dff030
	move.w	#$0001,$dff09c		; turn off the TBE bit
	clr.w	d7				; Clear timeout
	bra.s	.loop
.nomore:
	jmp	(a5)
.timeout:
	add.w	#1,d7
	swap	d7				; Swap high/low word
	add.w	#1,d7				; add 1 to counter
	cmp.w	#40,d7				; did we have too much errors?
	beq	.onetoomany			; yes.  stop serial output, we have issues with serial.
	swap	d7
	bra	.timedout
.onetoomany:					; OK we had one too many timeouts
	move.l	a7,d7
	bset	#31,d7				; Ser bit 31 to tell we had too many serial timeouts
	move.l	d7,a7
	bra	.nomore			


	;Call C crap
;	jsr	_test_function


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


_mempattern:
	dc.l	$AAAAAAAA,$55555555,$5555aaaa,$aaaa5555,$ffffffff,0

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

_diagRomTxt:
	dc.b	12,27,"[0m",27,"[40m",27,"[37m"
	dc.b	"DiagROM Amiga Diagnostic by John Hertell. "
	VERSION
	dc.b " "
	incbin	"builddate.i"
	dc.b $a,$d,0

_diagRomCheckadrtxt:
		dc.b $a,$d,$a,$d,"Checking addressdata of ROM-Space",$a,$d,0
_dottxt:
	dc.b	".",0
_hometxt:
	dc.b	$d,27,"[0m",0
_Etxt:
		dc.b	27,"[31mE",27,"[0m",0
_Errortxt:
	dc.b	" ",27,"[31mERROR ",27,"[32m (D31->D0): ",0
_blockfoundtxt:
	dc.b	$a,$d,27,"[0mMemblock found between: $",0
_blockfound2txt:
	dc.b	" and $",0
_dashtxt:
	dc.b	27,"[32m-",0
_newlinetxt:
	dc.b	$a,$d,27,"[0m",0
_errtxt:
	dc.b	27,"[31mX",0
_adrtest:
	dc.b	$a,$d,"Doing addresstesting of area.",$a,$d,"Filling space with addressdata",$a,$d,0
_adrtest2:
	dc.b	$a,$d,"Comparing addresses to stored addressdata",$a,$d,0
_adrmemoktxt:
	dc.b	$a,$d,$a,$d,27,"[0mOK memoryblock between: $",0
_blockok:
	dc.b	$a,$d,27,"[32mBlock seems OK!",$a,$d,27,"[0m",0
_blockerr:
	dc.b	$a,$d,27,"[31mERROR!! ",27,"[0m Block had addresserrors, errormask: (A31->A0)",$a,$d,0
_disclaimer:
	dc.b	$a,$d,27,"[0mErrormask is an ESTIMATE and is not an extact pointer!",0
_unusabletxt:
	dc.b	$a,$d,27,"[0mMarking block UNUSABLE!",$a,$d,$a,$d,0
_Initmousetxt:
	dc.b	$a,$d,"    Checking status of mousebuttons at power-on: ",$a,$d
	dc.b	"            ",0
_releasemousetxt:
	dc.b	$a,$d,"Release mousebuttons now or they will be tagged as STUCK and ignored!",0
_notenoughtxt:
	dc.b	$a,$d,27,"[31mNOT ENOUGH MEM IN BLOCK",27,"[0m Checking for more",$a,$d,$a,$d,0
_noblockfoundtxt:
	dc.b	$a,$d,27,"[31mNO Block found",27,"[0m",$a,$d,0
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

_startMemDetectTxt:
	dc.b	$a,$d,$a,$d,"Scan for usable Chipmem (!!NOTE THIS IS NOT A MEMORYTEST IT *IS* A SCAN)",$a,$d,$a,$d,0

_addrtxt:
	dc.b	"Addr $",0
_memoktxt:
	dc.b	27,"[32m OK",27,"[m, Number of working 64K blocks found: ",0
_beginrowtxt:
	dc.b	$d,0
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
