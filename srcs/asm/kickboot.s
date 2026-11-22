; kickboot.s - guarded-step harness + CPU-specific steps for kickboot.c's
; "Kickstart boot check": replay what Kickstart touches between power-on and
; the moment exec installs its real exception handlers, with EVERY exception
; vector caught, so the event that gives a yellow screen under Kickstart is
; reported here instead.
;
; Recovery is the same CPU-agnostic pattern as berr.s: the handler never
; RTEs, it reloads the stack pointer saved before the step and returns from
; ksGuard, discarding the exception frame whatever its format.
;
; gcc m68k ABI: d0/d1/a0/a1 scratch, result in d0, args on stack. a6 is the
; globals pointer (-ffixed-a6) and is never touched here.
;
; 68020+/FPU/MMU instructions are dc.w-encoded: the ROM assembles -m68000.
; Section name matters (see berr.s): only named sections are placed safely.

	include	"globalvars.i"

	section "generic",code_p

KS_FIRSTVEC	equ	2		; bus error
KS_NUMVEC	equ	46		; vectors 2..47 ($8..$BC), as Kickstart fills

; KsFault record (kickboot.c):
;   0  vec       vector number, 0 = none
;   4  frame[6]  first 24 bytes at the exception stack pointer
;  28  d0-d7
;  60  a0-a6

	XDEF	_ksGuard

; int ksGuard(void (*fn)(void *arg), void *arg, KsFault *rec)
;   Runs fn(arg) at IPL 7 with vectors 2..47 pointing at the catch-all.
;   returns 0 = fn returned normally, else the vector number that fired.
_ksGuard:
	movem.l	d2-d7/a2-a5,-(sp)	; 40 bytes, args now at 44/48/52(sp)
	move.l	44(sp),a2		; fn
	move.l	48(sp),a3		; arg
	move.l	52(sp),a4		; fault record
	move.w	sr,-(sp)
	or.w	#$0700,sr
	lea	$8.w,a0			; save vectors 2..47 on the stack
	moveq	#KS_NUMVEC-1,d0
.save:	move.l	(a0)+,-(sp)
	dbf	d0,.save
	move.l	a4,KsGuardRec(a6)
	move.l	sp,KsGuardSP(a6)
	lea	$8.w,a0
	lea	ksStubs(pc),a1
	moveq	#KS_NUMVEC-1,d0
.inst:	move.l	a1,(a0)+
	addq.l	#4,a1
	dbf	d0,.inst
	move.l	a3,-(sp)
	jsr	(a2)
	addq.l	#4,sp
	moveq	#0,d0
ksGuardExit:				; d0 = result, sp = KsGuardSP
	lea	$c0.w,a0		; one past vector 47
	moveq	#KS_NUMVEC-1,d1
.rest:	move.l	(sp)+,-(a0)
	dbf	d1,.rest
	move.w	(sp)+,sr
	movem.l	(sp)+,d2-d7/a2-a5
	rts

; One 4-byte stub per vector; the bsr return address identifies the vector.
ksStubs:
	rept	KS_NUMVEC
	bsr.w	ksCatch
	endr

ksCatch:				; (sp) = stub return address, 4(sp) = frame
	or.w	#$0700,sr
	move.l	a5,-(sp)
	move.l	KsGuardRec(a6),a5
	movem.l	d0-d7/a0-a4,28(a5)
	move.l	(sp)+,80(a5)		; a5 at fault time
	move.l	a6,84(a5)
	move.l	(sp)+,d0		; stub return address
	lea	ksStubs(pc),a0
	sub.l	a0,d0
	lsr.l	#2,d0			; stub index + 1
	addq.l	#KS_FIRSTVEC-1,d0	; -> vector number
	move.l	d0,(a5)
	move.l	(sp),4(a5)		; raw frame, decoded per CPU in C
	move.l	4(sp),8(a5)
	move.l	8(sp),12(a5)
	move.l	12(sp),16(a5)
	move.l	16(sp),20(a5)
	move.l	20(sp),24(a5)
	move.l	KsGuardSP(a6),sp
	bra	ksGuardExit

; ---------------------------------------------------------------------------
; Steps. All are void fn(uint32_t *out); out[0] counts the instructions that
; completed, so a fault names the exact instruction.
; ---------------------------------------------------------------------------

	XDEF	_ksStepCacr

; Kickstart TypeOfCPU cache probe (68020/030 only - caller gates).
;   out[1] = CACR before, out[2] = readback of $0A09 (EnableI/ClearI/
;   FreezeD/ClearD). Leaves CACR = $2009 (I-cache on, D-cache off, WA) as
;   Kickstart does, which is also what the memory steps need.
_ksStepCacr:
	move.l	4(sp),a0
	clr.l	(a0)
	dc.w	$4e7a,$0002		; movec cacr,d0
	move.l	d0,4(a0)
	addq.l	#1,(a0)
	move.l	#$00000a09,d1
	dc.w	$4e7b,$1002		; movec d1,cacr
	addq.l	#1,(a0)
	dc.w	$4e7a,$1002		; movec cacr,d1
	move.l	d1,8(a0)
	addq.l	#1,(a0)
	move.l	#$00002009,d1
	dc.w	$4e7b,$1002		; movec d1,cacr
	addq.l	#1,(a0)
	rts

	XDEF	_ksSetCacr

; void ksSetCacr(uint32_t value) - unguarded, 68020/030 only
_ksSetCacr:
	move.l	4(sp),d0
	dc.w	$4e7b,$0002		; movec d0,cacr
	rts

	XDEF	_ksStepPmove

; Kickstart A3000 cold power-up: zero TC, TT0, TT1 (68030 only).
_ksStepPmove:
	move.l	4(sp),a1
	clr.l	(a1)
	lea	.zero(pc),a0
	dc.w	$f010,$4000		; pmove (a0),tc
	addq.l	#1,(a1)
	dc.w	$f010,$0800		; pmove (a0),tt0
	addq.l	#1,(a1)
	dc.w	$f010,$0c00		; pmove (a0),tt1
	addq.l	#1,(a1)
	rts
.zero:	dc.l	0

	XDEF	_ksStepFpu

; Kickstart TypeOfCPU FPU probe (pre-68040 path).
;   out[1] = FPCR readback (Kickstart: nonzero = no FPU)
;   out[2] = FSAVE frame size byte ($18 = 68881, else 68882)
_ksStepFpu:
	move.l	4(sp),a0
	clr.l	(a0)
	moveq	#0,d1
	dc.w	$f201,$9000		; fmove.l d1,fpcr
	addq.l	#1,(a0)
	dc.w	$f201,$b000		; fmove.l fpcr,d1
	move.l	d1,4(a0)
	addq.l	#1,(a0)
	tst.l	d1
	bne.s	.done
	dc.w	$f327			; fsave -(sp)
	addq.l	#1,(a0)
	moveq	#0,d0
	move.b	1(sp),d0
	move.l	d0,8(a0)
	dc.w	$f35f			; frestore (sp)+
	addq.l	#1,(a0)
.done:	rts

	XDEF	_ksStepMemND

; Non-destructive RAM check, registers only (safe even if the stack or the
; exception vectors lie inside the range): every longword is saved, written
; with a pattern and its complement, verified, restored and re-verified.
;   args[0] = start, args[1] = end (exclusive)
;   args[2] = first failing address (0 = none)
;   args[3] = value written, args[4] = value read back
_ksStepMemND:
	move.l	4(sp),a1
	movem.l	d2-d3,-(sp)
	move.l	(a1),a0
	move.l	4(a1),d2
.loop:	cmp.l	d2,a0
	bhs.s	.out
	move.l	(a0),d0
	move.l	#$f2d4b689,d1
	move.l	d1,(a0)
	nop
	move.l	(a0),d3
	cmp.l	d1,d3
	bne.s	.fail
	not.l	d1
	move.l	d1,(a0)
	nop
	move.l	(a0),d3
	cmp.l	d1,d3
	bne.s	.fail
	move.l	d0,d1
	move.l	d0,(a0)
	nop
	move.l	(a0),d3
	cmp.l	d1,d3
	bne.s	.fail
	addq.l	#4,a0
	bra.s	.loop
.fail:	move.l	d0,(a0)			; best-effort restore
	move.l	a0,8(a1)
	move.l	d1,12(a1)
	move.l	d3,16(a1)
.out:	movem.l	(sp)+,d2-d3
	rts

	XDEF	_ksStepSweep

; Word-read sweep, as exec's ROM-tag search reads its scan regions.
;   args[0] = start, args[1] = end (exclusive)
;   args[2] = number of $4AFC words seen
; A fault leaves the faulting address in a0 of the fault record.
_ksStepSweep:
	move.l	4(sp),a1
	move.l	d2,-(sp)
	move.l	(a1),a0
	move.l	4(a1),d1
	moveq	#0,d2
.loop:	cmp.l	d1,a0
	bhs.s	.out
	move.w	(a0),d0
	cmp.w	#$4afc,d0
	bne.s	.next
	addq.l	#1,d2
.next:	addq.l	#2,a0
	bra.s	.loop
.out:	move.l	d2,8(a1)
	move.l	(sp)+,d2
	rts
