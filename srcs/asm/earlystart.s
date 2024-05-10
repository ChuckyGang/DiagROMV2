		include "earlymacros.i"
		include "build/srcs/globalvars.i"

		section "startup",code_p

;		XDEF	AnsiNull
		xdef	DumpSerial

rom_base:	equ $f80000	
RAMUsage: EQU GlobalVars_sizeof+STACKSIZE+Chipmemstuff_sizeof+4096		; Total amount of ram needed for DiagROM to work (plus some bufferdata for stack etc)
INITBAUD: EQU 183			; Init baudrate  115200

		xdef RAMUsage
		xdef INITBAUD
		xdef rom_base
STACKSIZE:	EQU	8192						; Set the size of the stack

	;	This is where it all starts
	;	I use 7 as Tab size  as it fits better for asm..
	;	Any comment of "printing" in this part of the code means out to serialport only.

	;	This is the init part..  NO ram can be used.  meaning NO varables.  NO stack.
	;	meaning NO subroutines just jump with NO RETURN!
	;	this code IS messy and very hard to write and maintain.
	;	Random setting variables to -1 is done while coding to see what registers is set.
	;	however as I also am forced to use -1 to say "not found" there might be parts where I
	;	set registers to that mode when not neded.  keeping them to not break stuff. low prio to clean that stuff up!

	;	Make reservation of English is not my first language, add to that dyslexia and a BAD BAD BAD habit of typos.. :)

	;	This is a horrible maze of weird registerusage and important data is moved around registers more then a politician moves
	;	money to avoid being cought of being corrupt. any change in this code can (and WILL) cause harm that might hurt human life as we know it
	;	think one, two even EIGHT times before changing anything, the world might blow up or you will find yourself falling out of a spaceship with a whale!

	;	A BIG ThankYou to Erik Hemming for helping me with thie Visual Code setup allowing me to combine asm/c etc.
	
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
		incbin	"build\srcs\builddate.i"
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
	move.b	#$ff,$bfe200
	move.b	#$ff,$bfe300
	move.b	#0,$bfe001		; Clear register.
	move.b	#$ff,$bfe301
	move.b	#$0,$bfe101
	move.b	#3,$bfe201	
	move.b	#0,$bfe001		; Powerled will go ON! so user can see that CPU works.  you know.  ACTIVITY!  **THE** most important tool to see if an amiga is alive
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
	move.l	#POSTBusError,$8	; Set trapregisters for the CPU. (HOPEFULLY you have working chipmem! going without tests here)
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

	cmp.l	#"PPC!",d5		; Check if D5 is "PPC!" if so. we had been reset and should not reset again
	beq	.wehadreset
	cmp.l	#" PPC",$f00092	; If the strinbg " PPC" is located here, we have a CSPPC
	bne	.nocsppc

	lea	$4,a0			; OK we had a CSPPC in the machine, we need to do some magic
	move.l	#5,d6
.csloop:
	move.l	#$ffff,d7
	bchg	#1,$bfe001		; Some LED flashity..
	move.w	d6,$dff180
.csloop2:
	move.b	$bfe001,d0		; Nonsense read from something slow
	dbf	d7,.csloop2
	dbf	d6,.csloop

	move.l	#"PPC!",d5		; Set D5 to tell DiagROM we had a reset
	move.w	#$2700,sr
.even:					; this apparenlty needs to be a on a even longword....
	reset
	move.l	(a0),a0
	jmp	(a0)
.wehadreset:
	move.b	#0,$bfe001		; Powerled will go ON! so user can see that CPU works

.nocsppc:
	clr.l	d0			; Lets clear all registers just to have a "nice" startup
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
	lea	$0,SP			; Set the stack. BUT!!! do not use it yet. we need to check memory first! so meanwhile we use it as a dirty status register

	KPRINTC _diagRomTxt		; Print some text

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
	bset	#28,d0
.NOP2LMB:
	btst	#10,$dff016		; Check RMB port 1
	bne	.NOP1RMB
	bset	#27,d0
.NOP1RMB:
	btst	#14,$dff016		; Check RMB port 2
	bne	.NOP2RMB
	bset	#26,d0
.NOP2RMB:
	btst	#8,$dff016		; Check MMB Port 1
	bne	.NOP1MMB
	bset	#25,d0			; MMB Port 1
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
	btst	#28,d0
	beq	.pnop2lmb
	KPRINTC _InitP2LMBtxt
.pnop2lmb:
	btst	#27,d0
	beq	.pnop1rmb
	KPRINTC _InitP1RMBtxt
.pnop1rmb:
	btst	#26,d0
	beq	.pnop2rmb
	KPRINTC _InitP2RMBtxt
.pnop2rmb:
	btst	#25,d0
	beq	.pnop1mmb
	KPRINTC _InitP1MMBtxt
.pnop1mmb:
	btst	#24,d0
	beq	.pnop2mmb
	KPRINTC _InitP2MMBtxt
.pnop2mmb:
	cmp.l	#0,d0
	beq	.nopressed		; if no mouse was pressed skip next print

.nopressed:

	move.w	#$200,$dff100			; This is needed or we will not see any colour changes on screen
	move.w	#0,$dff110			; This aswell!

; ******************************************************************************************************************
;
; Check ROM Addressdata to see if romspace can be addressed correctly
;
; ******************************************************************************************************************
	KPRINTC _diagRomCheckadrtxt
_romadrcheck:
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
	move.w	#$f00,$dff180
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
	move.l	a7,d1
	add.b	$dff007,d1			; we just create some kind of "random data"
	add.b	$dff006,d1			; by altering last byte of a7. adding data that MIGHT be random to eachother
	add.b	$dff00a,d1			; hopefully we will get a random answer, this is used as a seed for parts that needs new
	add.b	$dff00b,d1			; values every run. (like to find ghostmemory)
	move.l	d1,a7


	KPRINTC	_releasemousetxt
	KPRINTC	_checkovltxt
	cmp.l	#"DIAG",$0			; Check if $0 contains "DIAG" if so, OVL is NOT working.
	bne	.ovlok
	move.l	a7,d0
	bset	#9,d0				; Set we had OVL Error
	move.l	d0,a7
	KPRINTC	_FAILtxt
	bra	.ovlnotok
.ovlok:
	KPRINTC	_OKtxt
.ovlnotok:
	KPRINTC	_newlinetxt
; ******************************************************************************************************************
;
; Detect chipmem
;
; ******************************************************************************************************************


	KPRINTC	_startMemDetectTxt


	lea	$0,a6				; Start to detect ram at $0, as below that is cpu controlstuff only anyways.
						; ACTUALLY detectroutine will change this to $400 as first k on a 68k is for CPU registers and stuff
	lea	$0,a3				; Clear a3
.chiploop:
	lea	.memcheckdone,a4		; As we do not have a stack. mark where to jump to after memcheck is done
	jmp	.memcheck			; Do the memorycheck, lets find some chipmem

.memcheckdone:
						; a2 == -1 if no mem was found
	cmp.l	#-1,a2				; IF A2 is -1 then we found no chipmem
	bne		.noerr			; if not.  go to "noerr" as we did have some chipmem

	KPRINTC _nochiptxt			; Print that no chipmem was found, and then lets try to find some fastmem (or othermem) instead. sadly we cannot use
						; autoconfig (as this requires ram) so we will only find nonautoconfig ram. 
	lea	.fastdetectdone2,a6
	bra	checkiffastmem		; Check if there are any fastmem.
.fastdetectdone2:
	move.l	a7,d0
	bset	#21,d0				; Set bit for not enough chipmem found
	move.l	d0,a7
	cmp.l	#0,a2				; IF A2 is 0 we did not find any fastmem
	bne	.fastfound
	bra	_chipfailed
.fastfound:
	move.l	a7,d0
	bset	#12,d0				; As we have no chipmem, BUT fastmem set the "No Draw" bit
	move.l	d0,a7
	bra	.nochipbutfast
.noerr:					; ok we found some chipmem, lets see if we have any addresserror issues
	lea	.adrcheckdone,a4		; Set address to jump to after check
	jmp	Adrcheck			; Do a check of addresserrors of this block
.adrcheckdone:
	cmp.l	#"ADDR",d3			; did we have an addresserror?
	bne	.noflag			; no do not set the flag
	move.l	a7,d2
	bset	#22,d2				; Set bit 22 to tell we had an addresserror at chipmemcheck
	move.l	d2,a7
.noflag:
	cmp.l	#$400,a2			; Check if A2 is $400 if so we had low chipem. meaning we have registers for the cpu
	bne	.no400
	move.l	a7,d4
	bset	#10,d4				; Set bit 10 to tell we had mem at $400
	move.l	d4,a7
.no400:
.noadrerr:
	cmp.l	#RAMUsage,d1			; Check if we had enough of ram
	bge	.enoughmem
	add.l	#4,a2				; ok this block is done, to be sure add 4 to get to next longword of next block if needed
	move.l	a2,d2				; Copy address of end of block scanned to d2
	cmp.l	#$200000,d2			; Did we pass all 2MB chip?
	bge	.nochip			; if so we did not have enough of chipram

	KPRINTC _notenoughtxt
	asr.l	#8,d2
	asr.l	#7,d2				; Divide next block by 64k
	add.l	#1,d2				; add one!
	asl.l	#8,d2
	asl.l	#7,d2				; Multiply number with 64k to get next block
	move.l	d2,a6
	lea	$0,a3
	clr.l	d4
	bra	.chiploop			; Well HOPEFULLY this will check all mem in blocks.
.nochip:
	move.l	d4,d1				; do a copy of d4 d1 that contains the addr error bitpattern
	KPRINTC _noram
	move.l	a7,d0
	bset	#21,d0				; Set bit for not enough chipmem found
	move.l	d0,a7
	cmp.l	#"ADDR",d3			; Check if we had an addresserror
	bne	.noaddrerr			; nope jump to noaddrerr
	move.l	#-1,d2
	move.l	#-1,d3
	move.l	#-1,d4
	move.l	#-1,d5
	lea	-1,a1
	lea	-1,a2
	lea	-1,a4
	lea	-1,a5
	lea	.fastdetectdone,a6
	bra	checkiffastmem		; Check if there are any fastmem.
.fastdetectdone:
	clr.l	d4				; we had an addresserror so show that screen
	clr.l	d5
.adrfailloop:
	cmp.w	#20,d5
	bne	.adrnochange
	clr.w	d5
	eor	#$ff0,d4
	bchg	#1,$bfe001
.adrnochange:
	add.w	#1,d5
	move.w	d4,$dff180
	move.w	#$f00,d6
	lea	.errloop,a6
	bra	ShowErr
.errloop:
	bra	.adrfailloop

.noaddrerr:
	bra	.noaddrerr			; kuk!  we had no addresserror still somehing went bobo.. loop forever
.nochipbutfast:
.enoughmem:
	KPRINTC	_blockok

	clr.l	d0					; Clear d0 that is a temporary statusregister

; We will print the result on the serialport later.

; OK lets check mousepresses again to tell what mode of operation to use.
	btst	#6,$bfe001		; Check LMB port 1
	bne	.NOP1LMB		; NOT pressed.. Skip to next
	bset	#19,d0
.NOP1LMB:
	btst	#7,$bfe001		; Check LMB port 2
	bne	.NOP2LMB
	bset	#18,d0
.NOP2LMB:
	btst	#10,$dff016		; Check RMB port 1
	bne	.NOP1RMB
	bset	#17,d0
.NOP1RMB:
	btst	#14,$dff016		; Check RMB port 2
	bne	.NOP2RMB
	bset	#16,d0
.NOP2RMB:
	btst	#8,$dff016		; Check MMB Port 1
	bne	.NOP1MMB
	bset	#15,d0			; MMB Port 1
.NOP1MMB:
	btst	#12,$dff016		; Check MMB Port 2
	bne.s	.NOP2MMB
	bset	#14,d0			; MMB Port 2
.NOP2MMB:
	move.l	a7,d6
	or.l	d0,d6			; merge in the result into the A7 statusregister
	move.l	d6,a7			
	KPRINTC _PostInitmousetxt
	btst	#19,d0
	beq	.pnop1lmb
	btst	#29,d6
	beq	.p1lmbnr
	KPRINTC _redtxt
	bra	.p1
.p1lmbnr:
	KPRINTC _cleartxt
.p1:
	KPRINTC _InitP1LMBtxt
.pnop1lmb:
	btst	#18,d0
	beq	.pnop2lmb
	btst	#28,d6
	beq	.p2lmbnr
	KPRINTC _redtxt
	bra	.p2
.p2lmbnr:
	KPRINTC _cleartxt
.p2:
	KPRINTC _InitP2LMBtxt
.pnop2lmb:
	btst	#17,d0
	beq	.pnop1rmb
	btst	#27,d6
	beq	.p1rmbnr
	KPRINTC _redtxt
	bra	.p1r
.p1rmbnr:
	KPRINTC _cleartxt
.p1r:
	KPRINTC _InitP1RMBtxt
.pnop1rmb:
	btst	#16,d0
	beq	.pnop2rmb
	btst	#26,d6
	beq	.p2rmbnr
	KPRINTC _redtxt
	bra	.p2r
.p2rmbnr:
	KPRINTC _cleartxt
.p2r:
	KPRINTC _InitP2RMBtxt
.pnop2rmb:
	btst	#15,d0
	beq	.pnop1mmb
	btst	#25,d6
	beq	.p1mmbnr
	KPRINTC _redtxt
	bra	.p1m
.p1mmbnr:
	KPRINTC _cleartxt
.p1m:
	KPRINTC _InitP1MMBtxt
.pnop1mmb:
	btst	#14,d0
	beq	.pnop2mmb
	btst	#24,d6
	beq	.p2mmbnr
	KPRINTC _redtxt
	bra	.p2m
.p2mmbnr:
	KPRINTC _cleartxt
.p2m:
	KPRINTC _InitP2MMBtxt
.pnop2mmb:
	cmp.l	#0,d0
	beq	.nopressed		; if no mouse was pressed skip next print
.nopressed:
	KPRINTC _newlinetxt	

;--------------------------------------------------------------------------------------------------------------------------------------
;
; Most of the initcode is done
;
;--------------------------------------------------------------------------------------------------------------------------------------
	move.l	a7,d0							; lets do a check what mode to start with by taking status of mousebuttons at poweron
	move.l	d0,d1
	move.l	d0,d2
	and.l	#%00111111000000000000000000000000,d1		; mask out bits from poweron
	and.l	#%00000000000011111100000000000000,d2		; mask out bits from after meminit
	asr.l	#8,d1
	asr.l	#8,d1
	asr.l	#8,d1
	asr.l	#8,d2
	asr.l	#6,d2							; rotate bits to "start" of register
	eor.l	d1,d2							; d2 now will contain what bits was NOT stuck
	move.l	d1,d3
	eor.l	d2,d3							; d3 contains stuck bits.  so 0=no stuck
	cmp.b	#0,d3
	bne	.nostuck
	move.l	a7,d7
	bset	#11,d7							; Set bit 11 to show we had stuck keys
	move.l	d7,a7
.nostuck:
	btst	#5,d2
	bne	.LMB
	btst	#4,d2
	bne	.LMB
.LMBDONE:
	move.l	a6,d3					; Make a backup of what was in A6 to D3 to save it as it contains data needed later
	move.l	a3,d0
	sub.l	#RAMUsage,d0
	and.l	#$fffffffc,d0			; Make sure is is even to a longword
	move.l	d0,a6				; we MIGHT have a change of where baseaddress shold be so set it again

	move.l	a7,d0							; lets do a check what mode to start with by taking status of mousebuttons at poweron
	move.l	d0,d1
	move.l	d0,d2
	and.l	#%00111111000000000000000000000000,d1		; mask out bits from poweron
	and.l	#%00000000000011111100000000000000,d2		; mask out bits from after meminit
	asr.l	#8,d1
	asr.l	#8,d1
	asr.l	#8,d1
	asr.l	#8,d2
	asr.l	#6,d2							; rotate bits to "start" of register
	eor.l	d1,d2							; d2 now will contain what bits was NOT stuck
							; rotate bits to "start" of register
	btst	#3,d2
	bne	.RMB
	btst	#2,d2
	bne	.RMB
.RMBDONE:
	bra	.nomouse
.LMB:
	KPRINTC	_lmbtxt
	move.l	#-1,d0
	move.l	#-1,d1
	move.l	#-1,d2

	move.l	#-1,d4
	move.l	#-1,d5
	move.l	#-1,d6
	move.l	#-1,d7
	lea	-1,a0
	lea	-1,a1
	;lea	-1,a2
	lea	-1,a4
	lea	-1,a5
	;lea	-1,a3				; Set A3 to -1 to see if we found any mem

	move.l	a2,d6				; Make a backup of start of chip found
	lea	.lmboh,a6
	bra	checkiffastmem
.lmboh:
	move.l	a7,d0
	bset	#13,d0				; Set we found fastmem
	bset	#12,d0				; Set "nodraw" mode, no screenoutput
	cmp.l	#-1,a3				; Check if A3 is -1 if so we did not find any fastmem
	bne	.nofast
	bclr	#13,d0				; Clear "Found fastmem" mode as we apparently did not find that
	move.l	d1,a6
	move.l	d6,a1
.nofast:
	move.l	d0,a7				; copy back data to A7 as statusregister
.fast:
	bra	.LMBDONE

.RMB:
	KPRINTC	_rmbtxt
	move.l	a7,d0
	bset	#8,d0				; Set we had reversed workorder (using ram from START instead of from end)
	move.l	d0,a7
	move.l	a2,d0
	and.l	#$fffffffc,d0			; Make sure is is even to a longword
	move.l	d0,a6
	bra	.RMBDONE

.nomouse:					; A6 should now contain first usable block of RAM.
	move.w	#$fff,$dff180			; Set to bright white screen
	move.l	d3,d4				; Make a copy of d3 (start of chipmem) to d4
	KPRINTC _clearworktxt
	move.l	#RAMUsage,d0
	move.l	d0,d7
	asr.l	#2,d0
	move.l	a6,a0
.clearloop:
	clr.l	(a0)+
	dbf	d0,.clearloop
	KPRINTC _workspace
	move.l	#RAMUsage,d0
	KPRINTLONG
	move.l	a7,d1				; Make a copy of A7 that we used as a statusregister to d1
	KPRINTC _baseadr
	move.l	a6,d0
	KPRINTLONG
	move.l	a2,ChipStart(a6)
	move.l	a3,ChipEnd(a6)		; Store where the chipend block is
	btst	#13,d1
	beq	.nofastatboot
						; As we DID scan for fastmem aswell.  some data needs to be "rewritten"
	move.l	d4,ChipStart(a6)		; and start of it
	move.l	d6,ChipEnd(a6)

.nofastatboot:
	move.l	d1,startupflags(a6)		; Store startupflags as we used the A7 register for this. (we had an backup in D1)
	move.l	a6,startblock(a6)		; Store where workblock starts
	move.l	a1,endblock(a6)		; Store where the workblock ends

	move.l	#STACKSIZE,stack_size(a6)		; store our stack size
	lea.l	GlobalVars_sizeof(a6),sp	; stack area starts right after global vars
	adda.l	#32,sp				; add a SMALL buffer
	KPRINTC _stacktxt
	move.l	sp,d0
	and.l	#$fffffffe,d0
	move.l	d0,sp				; Make sure this is at an even address
	KPRINTLONG
	move.l	sp,stack_mem(a6)			; store the address of the stack
	move.l	stack_size(a6),d1
	add.l	d1,sp					; add size to stack as it grows backwards.
	KPRINTC _stacksettxt
	move.l	sp,d0
	KPRINTLONG
	move.l	a6,d0
	add.l	#EndVar+4,d0
	move.l	d0,EndVar(a6)				; Store the end of the variableblock
	move.l	SP,d0					; As we just set the stack. bitplanes are after it!
	add.l	#8,d0					; add a small buffer
	move.l	d0,ChipmemBlock(a6)			; Store pointer to the chipmemblock
	KPRINTC _chipblocktxt
	KPRINTLONG
	move.l	d0,BPL(a6)				; Store that pointer to BPL
	move.l	#Bpl1str,d1
	move.l	#Bpl2str,d2
	sub.l	d1,d2
	sub.l	#4,d2
	move.l	d2,BPLSIZE(a6)			; Store the size of a bitplane
	KPRINTC _starttxt

	bra	Initcode

	move.l	current_vhpos(a6),d1	; read back the vhpos
	;lsr.w	#8,d0					; we want the lower vertical byte
	;bsr		_test_function	; d0 = $600dc0de 	


	.error:
	move.b	d0,$dff181				; color the background
	bra	.error

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
	cmp.l	#$0,a6
	beq	.noadrzero
	move.l	#"SHDW",(a6)			; Write "SHDW" and see if it is readable at adr 0 then we have a shadow and we are out of chipmem
	cmp.l	#"SHDW",$0
	beq	.comparedone
.noadrzero:
	cmpa.l	#$200000,a6			; Have we reached the end of chipmem?
	bge	.comparedone			; YUPP!
	move.l	(a1)+,d0			; load first value into d0
	move.l	d0,(a6)			; write that value into where a1 points to.
	nop
	nop					; just do some nops.. to take some time.
	move.l	(a6),d1			; load value where a1 points to d1

;	cmpa.l	#18*1024,a6			; check to create an error
;	ble	.nott				; lines to be removed
;	cmpa.l	#1024*1024,a6
;	bge	.nott


.nott:

	;bclr	#4,d1				; create an error
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
	move.l	a7,d0
	bset	#23,d0				; Set bit that we had biterrors in chipmem
	move.l	d0,a7
	move.l	a6,d0
	KPRINTLONG
	KPRINTC _Errortxt
	move.w	#1,d6				; Set that we had an error
	swap	d6				; Swap d6 so we can use other 16 bits for the loop
	move.w	#31,d6				; Set counter to 31 for bits to print.   To be able to print out the biterrorpattern
.loop:
	btst	d6,d2				; Check if it was error on this bit or not
	beq	.correct
	KPRINTC	_errtxt		; it was, print an red X
.goon:
	dbf	d6,.loop			; Loop until all bits are done
	swap d6				; Swap back d6 to use other 16 bits as a errordetectionvariable
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
	move.l	a6,a2				; Store the current address to a2 to mark as beginning of memblock

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

Adrcheck:					; Check for addresserrors
; IN:
						;	A2 = Startaddress
						;	D0 = Endaddress
						; OUT:
						;	A2 = Startaddress of ok block -1 if none
						;	A3 = endaddress of ok block
						;	D1 = Bytes of ok mem
					;	rts
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
	clr.l	d7				; Clear d7 used as a counter
.adrcheckloop:
	cmp.l	d7,d6				; Have we reached to end of longwords before printing a dot?
	bne	.nodot
	move.w	4(a1),$dff180
	bchg	#1,$bfe001
	KPRINTC _dottxt			; print dot
	clr.l	d7				; clear counter
.nodot:
	move.l	a1,(a1)
	sub.l	#4,a1
	add.l	#1,d7				; Add counter
	cmp.l	a2,a1
	blt	.blockdone
	bra	.adrcheckloop
.blockdone:
	add.l	#4,a1				; add 4 to address
	KPRINTC _adrtest2
						; We have now filled the space with its address. lets check if it is stored like that.
	lea	-1,a2				; Set A2 to -1 to tell we haven't found any ram (yet)
	lea	-1,a3				; set A3 to -1 to tell we haven't had an end of a block (yet)
	clr.l	d7				; Clear d7 used as a counter
	clr.l	d4				; Clear d4 used as a mask of addresserrors
	lea	0,a0				; Set a0 to 0 to indicate no errors
.adrcheckloop2:
	cmp.l	d7,d6
	bne	.nodot2
	bchg	#1,$bfe001
	move.w	4(a1),$dff180

	cmp.l	#1,a0
	bne	.nobiterr
	move.w	#$f00,$dff180			; Set screen to RED
	KPRINTC _Etxt				; We had, print a red E
	bra	.errdone
.nobiterr:
	KPRINTC _dottxt
.errdone:
	clr.l	d7
	lea	0,a0				; Set a0 to 0 to indicate no errors
.nodot2:
	move.l	(a1),d1
	;bclr	#20,d1				; Create an error
	move.l	a1,d3
	eor.l	d3,d1				; d1 should be 0 if no error
	tst.l	d1				; Is d1 0?  if so we had no error
	beq	.noerr
	or.l	d1,d4				; Add currenterror to d4
	lea	1,a0				; Set a0 to 1 to indicate we had an error in this memblock
	cmp.l	#-1,a2				; did we have a memblock?
	beq	.nomemblock
	cmp.l	#-1,a3				; did we have an end already?
	bne	.nomemblock
	move.l	a1,a3				; Store this as the end.
	sub.l	#1,a3				; Subtract with 1 as we want to show LAST working byte
.nomemblock:
	bra	.nonewram			; jump to nonewram
.noerr:
	cmp.l	#-1,a2				; did we find any working RAM?
	bne	.nonewram
	move.l	a1,a2				; Store memaddress to a2 indicating first block of ram found
.nonewram:
	add.l	#4,a1
	add.l	#1,d7
	cmp.l	d0,a1
	bge	.blockdone2
	bra	.adrcheckloop2
.blockdone2:
	cmp.l	#-1,a2				; did we find any ram?
	beq	.nomemfound
	cmp.l	#-1,a3				; if so, did we find a end already?
	bne	.memendfound
	move.l	a1,a3				; if not set a1 to end.
	sub.l	#1,a3				; and subtract with 1 to show last working byte
.memendfound:
	cmp.l	#0,d4				; did we have any addresserrors
	beq	.printblock
	KPRINTC	_blockerr
	move.l	#"ADDR",d3			; Put ADDR into D3 marking we had addresserrors
	move.l	#31,d6				; Set counter to 31 for bits to print.
.loop2:
	btst	d6,d4
	beq	.correct2
	KPRINTC	_errtxt
.goon2:
	dbf	d6,.loop2
	KPRINTC	_disclaimer
	bra	.printblock
.correct2:
	KPRINTC	_dashtxt
	bra	.goon2

.printblock:
	KPRINTC _adrmemoktxt
	move.l	d5,d6
	move.l	a2,d0
	KPRINTLONG
	KPRINTC	_blockfound2txt
	move.l a3,d0
	KPRINTLONG
	move.l	a3,d1
	sub.l	a2,d1				; make d1 number of bytes found
	bra	.done
.nomemfound:
	KPRINTC _noblockfoundtxt
	clr.l	d1				; make sure 0 bytes is reported
.done:
	move.w	#$532,$dff180			; Just set a color that is not red
	jmp	(a4)

DumpSerial:
	move.l	a7,d7
	btst	#31,d7				; Check if timeoutbit is set.. if so skip this
	bne	.nomore
	move.w	#$4000,$dff09a
	move.w	#INITBAUD,$dff032			; Set the speed of the serialport (115200BPS)
	move.b	#$4f,$bfd000			; Set DTR high
	move.w	#$0801,$dff09a
	move.w	#$0801,$dff09c
	clr.l	d7				; Clear d7
.loop:
	cmp.b	#0,(a0)			; end of string?
	beq	.nomore			; yes
.wait:
	cmp.w	#30000,d7			; have we timed out?
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

checkiffastmem:

	move.l	a7,d4
	bset	#20,d4				; Set bit 20 to tell we scanned for fastmem
	add.b	$dff007,d4
	add.b	$dff006,d4
	add.b	$dff00a,d4
	add.b	$dff00b,d4			; we just create some kind of "random data"
	move.l	d4,a7				; a7 is now in some kind of "random state"

	move.l	a6,d4				; Store jumpbackpointer
						; Lets do a quick check if we have a CPU with 24 bit addressing
	move.l	#"NONE",$400			; Check if we have a 24bit adressing cpu.
	move.l	#"24BT",$40000400
	cmp.l	#"24BT",$400
	beq	.24bit
	lea	$8000000,a1
	lea	$1FFFFFFF,a2
	lea	.cpuslotdone,a6
	bra	detectmem
.cpuslotdone:
	cmp.l	#-1,d2
	bne	.foundmem
	lea	$4000000,a1
	cmp.l	#" PPC",$f00090		; IF $f0090 contains " BPPC" you have a BPPC and it will switch to 68000 if checking ram too low.
	bra	.bppc
	lea	$1800000,a1			; as we did not have a BPPC we can scan lower
.bppc:
	lea	$7FFFFFF,a2
	lea	.mboardmem,a6
	bra	detectmem
.mboardmem:
	cmp.l	#-1,d2
	bne	.foundmem

.24bit:					; we are in the scope of 24 bit addressing
	lea	$200000,a1
	lea	$9fffff,a2
	lea	.z2done,a6
	bra	detectmem
.z2done:

	lea	$c00000,a1
	lea	$c80000,a2
	lea	.rangerdone,a6
	bra	detectmem
.rangerdone:
	move.l	d4,a6				; Restore jumpbackpointer
	jmp	(a6)

.foundmem:
	move.l	a3,d5
	move.l	d6,a6
	move.l	d2,A2
	move.l	d3,d0
	move.l	d4,a4
	bra	Adrcheck
	
detectmem:
	KPRINTC _detmem
	move.l	a1,d0
	KPRINTLONG
	KPRINTC _blockfound2txt
	move.l	a2,d0
	KPRINTLONG
	move.l	#-1,d2				; Set D2 to -1 will contain memaddress of first detected block, if still -1 no ram was found
.checkmore:
	lea _mempattern,a4
	clr.l	d0
	clr.l	d5				; clear d5 that will contain biterrors after memcheck
.memloop:
	move.l	#"DONE",d3
	move.l	a7,d7
	eor.l	d7,d3				; eor "DONE" with a7 so it is a "random number" so we do not detect old data
	cmp.l	(a1),d3			; Check if address contains a trigger for shadowram already detected
	beq .noram				; if so.  we trigger this as "noram"

	move.l	(a4)+,d3
	move.l	d3,(a1)
	move.l	#"CRAP",4(a1)			; Write crapdata into memaddress next to that we check. just to make sure buffers etc put something else on the bus.
						; (HELLO 3000 and AA3000+ I am talking to your weird behaviour)
	nop
	nop
	nop					; Just do some NOPs. "to be sure"
	move.l	(a1),d0
	eor.l	d3,d0
	or.l	d0,d5				; Add biterrors to d5

	cmp.l	#0,d3				; check if d3 was 0, if so this longword is tested if not.. loop until it is.
	bne.s	.memloop

	move.l	#"DONE",d0
	move.l	a7,d7
	eor.l	d7,d0				; make so the string is "kinda" random
	move.l	d0,(a1)			; Put in a "triggermark" to find shadowram

	cmp.l	#0,d5				; d5 should be 0 if memory was fine if not.  we had no memory here
	bne	.noram

	cmp.l	#-1,d2				; ok we HAD ram, is this the fist block
	bne	.nonewram
	move.l	a1,d2				; ok this is new ram so lets put startblock of that ram to d2
	bra	.nonewram
.noram:
	cmp.l	#-1,d2				; ok there was an error in the ram.  just check IF we already found ram, if so. we are at the end of the block
	beq	.noramyet
	move.l	a1,d3				; ok as we already had found ram, this is where ram end so lets remember this block and exit
	bra	.ramfound			; and jump to ramfound.  we are done here.
.noramyet:
.nonewram:
	add.l	#64*1024,a1			; Add 64K for next block to check
	move.w	a1,$dff180
	cmp.l	a1,a2
	bge	.checkmore
.ramfound:
	cmp.l	#-1,d2
	beq	.noramfound
	KPRINTC _memdetected
	move.l	d2,d0
	move.l	a1,d3
	KPRINTLONG
	KPRINTC _blockfound2txt
	move.l	d3,d0
	KPRINTLONG
.done:
	jmp	(a6)				; Jump back
.noramfound:
	KPRINTC	_noramtxt
	bra	.done


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


_chipfailed:
	clr.l	d4
	clr.l	d5
.failloop:
	cmp.w	#30,d5
	bne	.nochange
	clr.w	d5
	eor.w	#$f00,d4
	bchg	#1,$bfe001
.nochange:
	add.w	#1,d5
	move.w	d4,$dff180
	lea	_mempattern,a0
	lea	$400,a1
	clr.l	d1
.memloop:
	move.l	(a0)+,d2
	move.l	d2,(a1)
	nop
	nop
	move.l	#$000,$3f0			; Write some crap to chipmem, so we have some OTHER data on the bus
	nop
	nop
	move.l	(a1),d3
	;bclr	#1,d3				; Create an error
	eor.l	d2,d3
	or.l	d3,d1

	cmp.l	#0,d2
	bne.s	.memloop
	lea	.done,a6
	move.w	#$00f,d6
	bra	ShowErr
.done:
	bra	.failloop
	
ShowErr:
	move.l	#31,d7				; Lets loop 32 bits
	move.l	#$20,d0			; Start at line $20
.bitloop:
	move.w	#0,$dff180
	
.waitstart:
	cmp.b	$dff006,d0			; Wait for rasterline in d0
	bne.s	.waitstart
	move.w	#$f55,$dff180
	add.b	#2,d0				; Width of line
.waitnext:
	cmp.b	$dff006,d0			; Wait for this line
	bne.s	.waitnext
	btst	d7,d1
	bne	.no
	move.w	d6,$dff180
	bra	.done
.no:
	move.w	#$050,$dff180
.done:
	add.b	#4,d0
.waitnext2:
	cmp.b	$dff006,d0
	bne.s	.waitnext2
	dbf	d7,.bitloop
.waitend:

	move.w	d4,$dff180
	cmp.b	#$22,$dff006
	bne.s	.waitend
	jmp	(a6)


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

