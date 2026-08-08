; DiagROM's first bus-error guard (vector 2) — C-callable probe-read helpers
; for autoconfig.c's expansion-space probes: catch the exact condition that
; yellow-screens Kickstart on a big box (bad/absent Buster Z3 decode, A3000
; daughterboard missing so /DTACK floats) and REPORT it instead of crashing.
;
; CPU-agnostic recovery pattern (design agreed 2026-08-07): the handler never
; RTEs — it reloads the stack pointer saved before the probe and jumps to the
; recovery point, discarding the exception frame wholesale, so the wildly
; different 68000/020/030/040/060 bus-fault frame formats never matter.
; DiagROM never writes VBR (CPU detect only reads it), so the vector lives at
; absolute $8 on every CPU.
;
; gcc m68k ABI: d0/d1/a0/a1 scratch, result in d0; args on stack.

; SECTION NAME MATTERS: link.txt only places named sections; orphans get
; swept by the catch-all to AFTER .checksums, where tools/checksum.c then
; overwrites them with the ROM address-line test pattern (self-pointing
; longwords) — calling the "code" there crashes with Illegal Instruction
; (found the hard way 2026-08-08; same trap as the -g/.stab ROM-overflow
; incident). "generic" is placed early with the other C-callable helpers.
	section "generic",code_p

	XDEF	_acBerrProbeRead

; int acBerrProbeRead(volatile uint8_t *addr, uint8_t *out)
;   returns 0 = read completed, *out = the byte
;           1 = bus error caught (vector restored, *out untouched)
_acBerrProbeRead:
	move.l	4(sp),a0		; probe address
	move.l	8(sp),a1		; out pointer
	movem.l	d2/a2/a3,-(sp)		; callee-saved scratch
	move.l	$8.w,d1			; save current bus-error vector
	lea	.berr(pc),a2
	move.l	a2,$8.w		; install guard
	move.l	sp,a3			; recovery SP (registers stay live
					; across the exception — only the
					; frame lands on the stack)
	nop				; settle pipeline before the probe
	move.b	(a0),d2		; THE probed read
	nop				; let the cycle fully complete/fault
	move.b	d2,(a1)
	moveq	#0,d0			; 0 = clean read
	bra.s	.out
.berr:					; entered in exception context; we
	move.l	a3,sp			; are always supervisor, so discard
	moveq	#1,d0			; the frame and carry on — no RTE
.out:
	move.l	d1,$8.w		; restore previous vector
	movem.l	(sp)+,d2/a2/a3
	rts

	XDEF	_acBerrReadLong

; int acBerrReadLong(volatile uint32_t *addr, uint32_t *out)
;   0 = read completed (*out = value), 1 = bus error caught
_acBerrReadLong:
	move.l	4(sp),a0
	move.l	8(sp),a1
	movem.l	d2/a2/a3,-(sp)
	move.l	$8.w,d1
	lea	.berrl(pc),a2
	move.l	a2,$8.w
	move.l	sp,a3
	nop
	move.l	(a0),d2
	nop
	move.l	d2,(a1)
	moveq	#0,d0
	bra.s	.outl
.berrl:
	move.l	a3,sp
	moveq	#1,d0
.outl:
	move.l	d1,$8.w
	movem.l	(sp)+,d2/a2/a3
	rts

	XDEF	_acBerrWriteLong

; int acBerrWriteLong(volatile uint32_t *addr, uint32_t val)
;   0 = write cycle completed, 1 = bus error caught
_acBerrWriteLong:
	move.l	4(sp),a0
	movem.l	a2/a3,-(sp)		; arg 'val' now sits at 16(sp)
	move.l	$8.w,d1
	lea	.berrw(pc),a2
	move.l	a2,$8.w
	move.l	sp,a3
	nop
	move.l	16(sp),(a0)
	nop
	moveq	#0,d0
	bra.s	.outw
.berrw:
	move.l	a3,sp
	moveq	#1,d0
.outw:
	move.l	d1,$8.w
	movem.l	(sp)+,a2/a3
	rts
