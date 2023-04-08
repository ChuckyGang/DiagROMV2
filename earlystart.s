		include "earlymacros.i"

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
		dc.b	"www.diagrom.com "
		incbin	"builddate.i"

		dc.b	"- "
			
		VERSION
_strstop:
		
		blk.b	166-(_strstop-_strstart),0		; Crapdata that needs to be here
	
		EVEN
		
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

		lea	$400,SP			; Set the stack. BUT!!! do not use it yet. we need to check chipmem first!

		move.b	#$ff,$bfe200
		move.b	#$ff,$bfe300

		move.b	#0,$bfe001			; Clear register.
		move.b	#$ff,$bfe301
		move.b	#$0,$bfe101
		move.b	#3,$bfe201	
		move.b	#0,$bfe001		; Powerled will go ON! so user can see that CPU works
		move.b	#$40,$bfed01
		move.w	#$f0f,$dff180
		move.w	#$ff00,$dff034
		move.w	#$0ff,$dff180
		move.b	#$ff,$bfd300
		or.b	#$f8,$bfd100
		nop
		and.b	#$87,$bfd100
		nop
		or.b	#$78,$bfd100

		; Lets check status of mousebuttons at start.  AAAND we have ONE register not used in
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

		; all code.  A4.. so lets store the result therea		
		move.b	#$88,$bfed01
		or.b	#$40,$bfee01		; For keyboard
						; We will print the result on the serialport later.
						
		move.l	#0,d0			; Make sure D0 is cleared.


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