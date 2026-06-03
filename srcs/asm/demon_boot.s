; ============================================================================
; demon_boot.s — DeMoN II cartridge boot prologue for Diagrom
;
; This file REPLACES the first portion of earlystart.s (everything from
; _start through the chip-RAM detection and workmem setup).  It produces
; the cartridge header at $A80000 with the NMI entry vector at $A8007C,
; sets up workmem in DeMoN RAM, optionally probes chip RAM for a usable
; framebuffer page, and finally chains into _begin in earlystart.s.
;
; To use:
;   - assemble this file in place of the first 360 lines of earlystart.s
;   - in earlystart.s, RENAME the existing "_begin:" label to "_diag_init:"
;   - this file defines a new "_begin:" that prepares the environment
;     and falls through to _diag_init
;   - keep all the other routines (DumpSerial, checkiffastmem, _mempattern,
;     POSTBusError, etc.) unchanged in earlystart.s
;
; Build with: vasmm68k_mot -DTARGET_DEMON=1 ...
; ============================================================================

; The kickstart Makefile picks up every srcs/asm/*.s file via wildcard, so
; this file IS assembled in both builds.  Without this guard the kickstart
; link would fail with duplicate _start / _begin / rom_base symbols, and
; the assembler would fail outright trying to incbin build_demon/srcs/
; builddate.i (which only exists during the demon build).
        ifd TARGET_DEMON

        include "earlymacros.i"
        include "globalvars.i"

;-----------------------------------------------------------------------------
; Cartridge memory map constants (from DeMoN.v decode)
;-----------------------------------------------------------------------------
DEMON_ROM_BASE  equ $a80000
DEMON_RAM_BASE  equ $ac0000
DEMON_RAM_END   equ $bbfff0           ; just before USB regs
DEMON_USB_DATA  equ $bbfff1           ; byte access, LDS
DEMON_USB_STAT  equ $bbfff2           ; word access
DEMON_AR_REG    equ $a80000           ; hardware register (cause/config)
DEMON_PTPLAYER  equ $bb0000           ; MOD player code buffer (4.5KB, RAM DeMoN)

NMI_VECTOR_OFS  equ $7c               ; CPU autovector 31 = NMI level 7

;-----------------------------------------------------------------------------
; Symbols exported
;-----------------------------------------------------------------------------
        xdef    _demon_entry
        xdef    _begin
        xdef    rom_base
        xdef    RAMUsage
        xdef    INITBAUD
        xdef    DEMON_PTPLAYER
        xref    _diag_init            ; renamed _begin in earlystart.s
        xref    DumpSerial
        xref    POSTBusError
        xref    POSTAddressError
        xref    POSTIllegalError
        xref    POSTDivByZero
        xref    POSTChkInst
        xref    POSTTrapV
        xref    POSTPrivViol
        xref    POSTTrace
        xref    POSTUnimplInst

rom_base:       equ DEMON_ROM_BASE
INITBAUD:       equ 373               ; 9600 (legacy, unused on DeMoN)
STACKSIZE:      equ 16384
RAMUsage:       equ GlobalVars_sizeof+STACKSIZE+Chipmemstuff_sizeof+4096

        xdef    STACKSIZE

;=============================================================================
; SECTION: demon_header — placed FIRST in ROM by link_demon.txt
;          Lives at $A80000 ... $A80100 approximately.
;=============================================================================
        section "startup",code_p

;-----------------------------------------------------------------------------
; $A80000: arReg overlap.  This 2-byte location is the DeMoN hardware
; register according to DeMoN.v line 92, so a ROM read here is NOT a
; ROM read at all — it returns cause0/cause1 from the CPLD.  Put a
; dummy here.
;-----------------------------------------------------------------------------
_demon_entry:
        dc.w    $0000                 ; $A80000 — arReg overlap
        dc.w    $0000                 ; $A80002 — pad

;-----------------------------------------------------------------------------
; $A80004-$A8007B: identification + version string (visible on the bus
; if anyone dumps the ROM, and harmless if the CPU jumps here by mistake
; because the bytes assemble to a $0000 ORI.B which is a slow no-op).
;-----------------------------------------------------------------------------
_id_start:
        dc.b    "ACTI"                ; magic at $A80004 (Action Replay compat)
        dc.b    "N2DG"                ; ('DeMoN2 DiaGrom')
        dc.b    "$VER: DiagROM/DeMoN2 by N.Avanzi (orig. J.Hertell) "
        VERSION
        dc.b    " "
        incbin  "build_demon/srcs/builddate.i"
_id_end:
        ; pad up to NMI vector slot
        ds.b    NMI_VECTOR_OFS-($04+(_id_end-_id_start))

;-----------------------------------------------------------------------------
; $A8007C: NMI level-7 autovector.  This is where the CPU jumps when
; the CPLD asserts IPL2..0=000 and OVR/DTACK during the IACK cycle.
; The longword is fetched while OVR is asserted, so the CPU reads it
; from ROM (not from chip RAM).
;-----------------------------------------------------------------------------
        dc.l    _begin                ; jump to our setup code

;-----------------------------------------------------------------------------
; $A80080+: more padding to clear away from the autovector area before
; the rest of the .text section begins.
;-----------------------------------------------------------------------------
        ds.b    $80-($7C+4)

;=============================================================================
; SECTION: startup — main entry code, runs from ROM, called from NMI vector
;=============================================================================
        section "startup",code_p

;-----------------------------------------------------------------------------
; _begin: NMI entry point.  At this point:
;   - CPU is in supervisor mode
;   - SR = $2700 (level 7 masked)
;   - SSP points somewhere in chip RAM (left by Kickstart)
;   - OVR has been released (we're now executing from ROM normally)
;   - runningflag in CPLD is asserted, so no further triggers
;-----------------------------------------------------------------------------
_begin:
        ;----- 1. Move stack to DeMoN RAM IMMEDIATELY -----
        ;       (forget whatever Kickstart had set; chip RAM may be bad)
        lea     DEMON_RAM_END,sp

        ;----- 2. Force IPL mask to 7 in case anything tries to fiddle -----
        move.w  #$2700,sr

        ;----- 3. Stop ALL DMA and disable ALL interrupts before touching
        ;          chip RAM (Kickstart had blitter/audio/etc running) -----
        move.w  #$7fff,$dff096        ; DMACON: clear all DMA + master
        move.w  #$7fff,$dff09a        ; INTENA: disable all
        move.w  #$7fff,$dff09c        ; INTREQ: clear all
        move.w  #$7fff,$dff09c        ; (twice, AGA bug compat)

        ;----- 4. CIA timers + ICR off (might be running and firing IRQs) -----
        move.b  #$7f,$bfee01          ; CIA-A ICR mask: disable all
        move.b  #$7f,$bfdd00          ; CIA-B ICR mask: disable all
        move.b  #$00,$bfe701          ; CIA-A CRA: stop timer A
        move.b  #$00,$bfe801          ; CIA-A CRB: stop timer B
        move.b  #$00,$bfd700          ; CIA-B CRA
        move.b  #$00,$bfd800          ; CIA-B CRB

        ;----- 5. Init CIA-A registers (same as original earlystart) -----
        ;       Power LED on, audio filter off
        move.b  #$ff,$bfe201          ; DDRA all output
        move.b  #$00,$bfe001          ; PRA = 0 → bit 1 low → POWER LED BRIGHT
        move.b  #$ff,$bfe301          ; DDRB all output

        ;----- 6. Visible boot color: yellow on background -----
        move.w  #$0ff0,$dff180
        move.w  #$0f80,$dff180        ; DEBUG: ORANGE after yellow

        ;----- 7. Init DeMoN USB serial (drain any stale RX data) -----
        clr.b   $BBFFE0                ; init USB sticky-disconnect flag = 0
        bsr     usb_drain_rx
        move.w  #$000f,$dff180        ; DEBUG: BLUE after usb_drain_rx

        ;----- 8. Send boot banner via USB serial (always works regardless
        ;        of chip RAM state) -----
        lea     _demon_banner(pc),a0
        bsr     usb_puts
        move.w  #$00ff,$dff180        ; DEBUG: CYAN after usb_puts

        ;----- 9. Place exception vectors in chip RAM low ($00-$3F).
        ;        WARNING: this writes to chip RAM!  If chip RAM is dead,
        ;        these writes go nowhere but don't cause a fault.
        ;        Diagrom's POST handlers will catch faulty exceptions
        ;        and visually flag them. -----
        move.l  #POSTBusError,$8
        move.l  #POSTAddressError,$c
        move.l  #POSTIllegalError,$10
        move.l  #POSTDivByZero,$14
        move.l  #POSTChkInst,$18
        move.l  #POSTTrapV,$1c
        move.l  #POSTPrivViol,$20
        move.l  #POSTTrace,$24
        move.l  #POSTUnimplInst,$28
        move.l  #POSTUnimplInst,$2c
        move.w  #$0850,$dff180        ; DEBUG: BROWN - POST vectors written

        ;----- 10. Setup A6 = workmem base IN DEMON RAM.
        ;        This is the big departure from kickstart-mode Diagrom:
        ;        instead of allocating GlobalVars at the top of chip RAM,
        ;        we put it in our cartridge RAM where it is GUARANTEED
        ;        to be readable/writable. -----
        move.l  #DEMON_RAM_BASE,a6    ; A6 = base of GlobalVars
        move.w  #$0058,$dff180        ; DEBUG: DEEP-BLUE - A6 set

        ;----- 11. Setup stack ABOVE GlobalVars in DeMoN RAM -----
        lea.l   GlobalVars_sizeof(a6),sp
        move.w  #$05ff,$dff180        ; DEBUG: PALE-CYAN - stack set up
        adda.l  #32,sp                ; small safety buffer
        move.l  sp,d0
        and.l   #$fffffffe,d0         ; even-align
        move.l  d0,sp
        ;       stack now grows downward from ~AC1100 toward AC1004
        ;       — plenty of headroom

        move.l  sp,stack_mem(a6)
        move.l  #STACKSIZE,stack_size(a6)
        move.l  sp,d0
        add.l   #STACKSIZE,d0
        move.l  d0,sp                 ; stack high water mark
        move.l  a6,startblock(a6)
        lea.l   DEMON_RAM_END,a0
        move.l  a0,endblock(a6)

        ;----- 12. Clear GlobalVars region (we are in possibly-uninitialised
        ;          DeMoN RAM, which is SRAM — undefined power-on contents). -----
        move.w  #$0ff8,$dff180        ; DEBUG: LIGHT-YELLOW - about to clrvars
        move.l  a6,a0
        move.l  #GlobalVars_sizeof/4-1,d0
.clrvars:
        clr.l   (a0)+
        dbf     d0,.clrvars

        ;----- 13. Magenta: about to probe chip RAM -----
        move.w  #$0f0f,$dff180

        bsr     usb_putc_str
        dc.b    13,10,'Probing chip RAM for framebuffer page...',13,10,0
        even

        ;----- 14. Try to find one usable 64KB chip RAM page for
        ;          Chipmemstuff (bitplanes + copperlist).  We try a few
        ;          candidate addresses; first one that passes a quick
        ;          non-shadow + write/read test wins. -----
        move.w  #$0f08,$dff180        ; DEBUG: PINK - about to call probe_chip_pages
        bsr     probe_chip_pages
        move.w  #$080f,$dff180        ; DEBUG: PURPLE - probe returned, checking D0
        ;       D0 = chosen page address, or 0 if nothing usable

        tst.l   d0
        beq.s   .no_video

        ;----- 15a. Chip RAM usable: setup Chipmemstuff in chip RAM -----
        move.w  #$0afc,$dff180        ; DEBUG: TURQUOISE - probe OK branch
        move.l  d0,ChipmemBlock(a6)
        move.l  d0,BPL(a6)
        move.l  d0,a0
        ;       compute Bpl1/2/3 pointers using existing logic
        ;       (mirrors earlystart.s lines 745-750)
        move.l  #Bpl1str,d1
        move.l  #Bpl2str,d2
        sub.l   d1,d2
        sub.l   #4,d2
        move.l  d2,BPLSIZE(a6)
        ;       Note: NoDraw stays 0 (cleared by clrvars), video is enabled
        move.w  #$00f0,$dff180        ; green: chip RAM OK
        bsr     usb_putc_str
        dc.b    13,10,'Chip RAM framebuffer page found',13,10,0
        even
        bra.s   .video_decision_done

.no_video:
        ;----- 15b. No usable chip RAM: force NoDraw mode -----
        ;          (also set bit 21 of startupflags = NotEnoughChip,
        ;          and bit 12 = NoDraw — these are parsed by initCode in C)
        move.l  #$00301000,startupflags(a6)   ; bits 21, 20, 12 set
        clr.l   ChipStart(a6)
        clr.l   ChipEnd(a6)
        clr.l   ChipmemBlock(a6)                ; chip RAM dead, must remain 0
        ;----- Set BPL only to safe DeMoN RAM scratch area -----
        ;      initcode writes BPL1/BPL2/BPL3 markers to globals->BPL. With BPL=0,
        ;      those markers land at $0 (CPU vectors!). Redirect them to safe
        ;      DeMoN SRAM at $BB8000. ChipmemBlock stays 0 so memtest does not
        ;      mistake DeMoN RAM for chip RAM.
        move.l  #$00BB8000,BPL(a6)
        move.l  #$00000280,BPLSIZE(a6)          ; small bpl (640 bytes), unused in NoDraw
        move.w  #$0f00,$dff180        ; red: no usable chip RAM
        bsr     usb_putc_str
        dc.b    13,10,'!! No usable chip RAM — serial-only mode',13,10,0
        even
        bsr     chip_bit_scan         ; determine which 16 data bits are faulty
        bsr     show_chip_bits        ; show 16-band bit map (exits on serial key)

.video_decision_done:

        ;----- 16. Black background, hand control to original Diagrom main -----
        move.w  #$0000,$dff180

        bsr     usb_putc_str
        dc.b    13,10,'--- Entering Diagrom main ---',13,10,0
        even

        bra     _diag_init            ; renamed entry in earlystart.s

;=============================================================================
; usb_drain_rx — empty any stale data from the FT245 RX FIFO
;-----------------------------------------------------------------------------
usb_drain_rx:
        movem.l d0-d1,-(sp)
        move.l  #$1000,d1             ; safety timeout: drain max 4096 bytes
.lp:    move.w  DEMON_USB_STAT,d0
        btst    #1,d0                 ; ft_rxf bit: 1=FIFO empty
        bne.s   .end                  ; bit set = empty → exit
        move.b  DEMON_USB_DATA,d0     ; consume one byte
        subq.l  #1,d1
        bne.s   .lp                   ; loop unless timeout reached
.end:   movem.l (sp)+,d0-d1
        rts

;=============================================================================
; usb_putc — send one byte (in D0.B) over USB serial, blocking on TX full
; Preserves all regs except CCR
;-----------------------------------------------------------------------------
usb_putc:
        movem.l d1-d2/a0,-(sp)
        ;----- Sticky disconnect flag -----
        ; USB_FLAG = $BBFFE0 (in DeMoN RAM, NOT touched by Diagrom workmem).
        ; 0 = USB OK, !=0 = USB disconnected or FIFO full, drop fast.
        tst.b   $BBFFE0
        beq.s   .normal_wait          ; flag=0: normal path
        ;----- Quick re-check: maybe USB came back online -----
        move.w  DEMON_USB_STAT,d2
        btst    #0,d2                 ; ft_txe bit: 1=FIFO full
        bne.s   .quickdrop            ; still full → drop, keep flag set
        ;----- USB recovered! reset flag and write byte -----
        clr.b   $BBFFE0
        move.b  d0,DEMON_USB_DATA
        bra.s   .done
.quickdrop:
        ;----- Just drop the byte, no wait -----
        bra.s   .done
.normal_wait:
        move.l  #$1000,d1             ; safety timeout (~140us, much shorter)
.wait:  subq.l  #1,d1
        beq.s   .set_flag             ; timeout: set sticky flag
        move.w  DEMON_USB_STAT,d2
        btst    #0,d2                 ; ft_txe bit: 1=FIFO full
        bne.s   .wait
        move.b  d0,DEMON_USB_DATA
        bra.s   .done
.set_flag:
        move.b  #1,$BBFFE0            ; mark USB as disconnected/full
.done:
        movem.l (sp)+,d1-d2/a0
        rts

;=============================================================================
; usb_puts — send NUL-terminated string at A0, A0 advances past terminator
;-----------------------------------------------------------------------------
usb_puts:
        move.b  (a0)+,d0
        beq.s   .end
        bsr     usb_putc
        bra.s   usb_puts
.end:   rts

;=============================================================================
; usb_putc_str — send inline string after this BSR, like a printf-of-the-poor
;   BSR usb_putc_str
;   DC.B "text",0
;   EVEN
;   ...continues here...
;-----------------------------------------------------------------------------
usb_putc_str:
        move.l  (sp)+,a0              ; pop return PC = ptr to inline string
.lp:    move.b  (a0)+,d0
        beq.s   .end
        bsr     usb_putc
        bra.s   .lp
.end:   ; align A0 up to even, then push as new return PC
        move.l  a0,d0
        addq.l  #1,d0
        and.l   #$fffffffe,d0
        move.l  d0,-(sp)
        rts

;=============================================================================
; probe_chip_pages — try candidate addresses, return first usable 64KB page
;   Input:  none
;   Output: D0 = page address (suitable for ChipmemBlock), or 0 if none
;   Clobbers: D0-D3, A0-A1
;-----------------------------------------------------------------------------
        ; Candidates in priority order.  $10000 is the safest (well above
        ; the exception vector area).  $20000 is the fallback if low chip
        ; is bad.  $40000 catches the case where the first DRAM is dead
        ; but the second is alive on A500+.
probe_chip_pages:
        movem.l d4-d7/a2-a3,-(sp)
        lea     .candidates(pc),a3
.next_cand:
        move.l  (a3)+,d4              ; load candidate addr
        beq     .all_failed           ; sentinel 0 = end of list
        move.l  d4,a0
        bsr     probe_one_page
        tst.l   d0
        bne     .got_it
        bra.s   .next_cand
.all_failed:
        moveq   #0,d0
.got_it:
        movem.l (sp)+,d4-d7/a2-a3
        rts
.candidates:
        dc.l    $00010000             ; 64K
        dc.l    $00020000             ; 128K
        dc.l    $00040000             ; 256K
        dc.l    $00060000             ; 384K
        dc.l    $00080000             ; A500+ Trapdoor low
        dc.l    $00100000             ; A500+ Trapdoor mid
        dc.l    0                     ; sentinel

;-----------------------------------------------------------------------------
; probe_one_page — quick test of a 64KB page
;   Input:  A0 = page base addr (must be in possible chip RAM)
;   Output: D0 = A0 if page is usable, 0 otherwise
;   Strategy:
;     1. Active aliasing check: write V1 to A0 and a different value to $0,
;        then verify A0 still reads V1 (catches a page that mirrors low
;        memory / unmapped chip RAM, with no false positive on stale $0)
;     2. Walking-bit + complement test on 1024 bytes (NOT full 64K — we
;        just need confidence that the page works; the full test is for
;        the diagnostic menu later)
;-----------------------------------------------------------------------------
probe_one_page:
        movem.l d1-d3/a1,-(sp)

        ;--- Active aliasing check ---
        ;    Write V1 to the candidate and a DIFFERENT value to $0, then verify
        ;    the candidate still reads V1.  This detects a candidate that
        ;    mirrors low memory ($0) WITHOUT the false positives of the old
        ;    passive "$0 == SHDW" test, which tripped whenever $0 already held
        ;    SHDW (stale, left by a prior probe / retained across warm reset).
        ;    $0 is saved and restored. ---
        move.l  $0,d1                ; save $0
        move.l  #$53484457,(a0)      ; V1 = "SHDW" -> candidate
        move.l  #$deadc0de,$0        ; distinct value -> $0
        nop
        nop
        move.l  (a0),d3             ; read candidate back
        move.l  d1,$0               ; restore $0 (cmp below re-sets flags)
        cmp.l   #$53484457,d3       ; candidate still holds V1?
        bne.s   .bad                ; no -> mirrors $0 -> reject

        ;--- Walking-bit test on 1KB at A0 ---
        move.l  a0,a1
        moveq   #0,d1                 ; pattern accumulator
        move.l  #256-1,d2             ; 256 longwords = 1KB
.w1:    move.l  d1,(a1)+
        addq.l  #1,d1
        dbf     d2,.w1

        move.l  a0,a1
        moveq   #0,d1
        move.l  #256-1,d2
.r1:    cmp.l   (a1)+,d1
        bne.s   .bad
        addq.l  #1,d1
        dbf     d2,.r1

        ;--- Complement test ---
        move.l  a0,a1
        moveq   #0,d1
        move.l  #256-1,d2
.w2:    move.l  d1,d3
        not.l   d3
        move.l  d3,(a1)+
        addq.l  #1,d1
        dbf     d2,.w2

        move.l  a0,a1
        moveq   #0,d1
        move.l  #256-1,d2
.r2:    move.l  d1,d3
        not.l   d3
        cmp.l   (a1)+,d3
        bne.s   .bad
        addq.l  #1,d1
        dbf     d2,.r2

        move.l  a0,d0                 ; success: return page addr
        bra.s   .out
.bad:   moveq   #0,d0
.out:   movem.l (sp)+,d1-d3/a1
        rts

;=============================================================================
; chip_bit_scan — determine which of the 16 chip-RAM data bits (D0..D15)
;   are faulty, by writing/reading known patterns at one chip address.
;   In Modalita' B no chip page is usable, but we still want to know WHICH
;   bits fail (e.g. a removed RAM chip = one or more dead bits everywhere).
;   Output: D4.W = fail mask (1 = bit faulty), D5.W = tested mask (1 = bit
;           exercised). Clobbers D0-D2/A0.
;=============================================================================
chip_bit_scan:
        movem.l d1-d2/a0,-(sp)
        moveq   #0,d4                 ; fail mask
        move.w  #$ffff,d5             ; tested mask (we exercise all 16 bits)
        lea     $00010000,a0          ; chip test address (64K page)
        ; pattern 1: all ones
        move.w  #$ffff,(a0)
        nop
        nop
        move.w  (a0),d0
        eor.w   #$ffff,d0             ; bits that differ from $ffff
        or.w    d0,d4
        ; pattern 2: all zeros
        move.w  #$0000,(a0)
        nop
        nop
        move.w  (a0),d0
        ; bits that differ from $0000 are simply the set bits
        or.w    d0,d4
        ; pattern 3: $AAAA
        move.w  #$aaaa,(a0)
        nop
        nop
        move.w  (a0),d0
        eor.w   #$aaaa,d0
        or.w    d0,d4
        ; pattern 4: $5555
        move.w  #$5555,(a0)
        nop
        nop
        move.w  (a0),d0
        eor.w   #$5555,d0
        or.w    d0,d4
        movem.l (sp)+,d1-d2/a0
        rts
;=============================================================================
; show_chip_bits — draw 16 horizontal bands (one per data bit D0..D15)
;   using raster-timed COLOR00 writes (works with NO chip RAM, like the
;   raster test). D0=top band ... D15=bottom band.
;     green = bit ok, red = bit faulty, blue = bit not tested.
;   Exits when a key arrives on the FT245 serial OR after a safety timeout,
;   so the serial menu still becomes available afterwards.
;   Input: D4.W = fail mask, D5.W = tested mask. Clobbers D0-D3/A0.
;=============================================================================
SHOWBITS_TOP    equ $30               ; first visible rasterline used
SHOWBITS_BANDH  equ 12                ; rasterlines per band (16*12 = 192)
SHOWBITS_TIMEOUT equ 60*30            ; ~30 s at 60 fields/s safety timeout
SHOWBITS_LSTART equ $10               ; first rasterline painted (black band above)
show_chip_bits:
        movem.l d1-d7/a0,-(sp)
        bsr     usb_drain_rx          ; clear stale RX so we don't exit at once
        bsr     usb_putc_str
        dc.b    13,10,'Chip RAM data-bit map (D0=top..D15=bottom): '
        dc.b    'green=ok red=bad blue=untested',13,10
        dc.b    'Press any key on this terminal to continue to menu.',13,10,0
        even
        move.l  #SHOWBITS_TIMEOUT,d6  ; field countdown
.field:
        ; $dff006 exposes only the low 8 bits of the vertical beam position,
        ; which wraps past line 255 (PAL has ~312 lines).  We therefore also
        ; gate every line match on V8 == 0 (bit 0 of VPOSR $dff004), so a
        ; target line is only accepted in the first half of the frame.
        ; Sync: wait for second half (V8=1), then for the new frame (V8=0).
.s1:    move.w  $dff004,d0
        btst    #0,d0                ; wait until V8 == 1 (lower screen)
        beq.s   .s1
.s2:    move.w  $dff004,d0
        btst    #0,d0                ; then wait until V8 == 0 (new frame)
        bne.s   .s2
        ; --- phase 1: black from LSTART to TOP-1 ---
        move.w  #SHOWBITS_LSTART,d7
.pre:
.pw:    move.w  $dff004,d0
        btst    #0,d0
        bne.s   .pw                  ; ignore matches when V8=1
        cmp.b   $dff006,d7
        bne.s   .pw
        move.w  #$000,$dff180
        addq.w  #1,d7
        cmp.w   #SHOWBITS_TOP,d7
        blt.s   .pre
        ; --- phase 2: 16 bands ---
        moveq   #0,d3
.nextband:
        move.w  d5,d0
        btst    d3,d0                ; tested?
        beq.s   .bblue
        move.w  d4,d0
        btst    d3,d0                ; faulty?
        bne.s   .bred
        move.w  #$0f0,d2             ; green
        bra.s   .bcol
.bred:  move.w  #$f00,d2             ; red
        bra.s   .bcol
.bblue: move.w  #$00f,d2             ; blue
.bcol:
        moveq   #0,d1
.bline:
        move.w  d3,d0
        mulu    #SHOWBITS_BANDH,d0
        add.w   d1,d0
        add.w   #SHOWBITS_TOP,d0     ; d0 = absolute target line
.bw:    move.w  $dff004,d7
        btst    #0,d7
        bne.s   .bw                  ; ignore matches when V8=1
        cmp.b   $dff006,d0
        bne.s   .bw
        tst.w   d1
        bne.s   .bcolour
        move.w  #$000,$dff180        ; separator
        bra.s   .badv
.bcolour:
        move.w  d2,$dff180
.badv:
        addq.w  #1,d1
        cmp.w   #SHOWBITS_BANDH,d1
        blt.s   .bline
        addq.w  #1,d3
        cmp.w   #16,d3
        blt.s   .nextband
        ; frame done
        move.w  #$000,$dff180
        move.w  DEMON_USB_STAT,d0
        btst    #1,d0
        beq     .keypressed
        subq.l  #1,d6
        bne     .field
        bra     .keypressed
.keypressed:
        move.b  DEMON_USB_DATA,d0     ; consume the key (ignore value)
        move.w  #$000,$dff180         ; restore black background
        movem.l (sp)+,d1-d7/a0
        rts
;=============================================================================
; Boot banner
;=============================================================================
_demon_banner:
        dc.b    13,10
        dc.b    '==========================================',13,10
        dc.b    'DiagROM on DeMoN II cartridge',13,10
        dc.b    'ROM @ $A80000   RAM @ $AC0000',13,10
        dc.b    'USB serial @ $BBFFF0 (FT245)',13,10
        dc.b    '==========================================',13,10,0
        even

        endc                          ; ifd TARGET_DEMON
        end
