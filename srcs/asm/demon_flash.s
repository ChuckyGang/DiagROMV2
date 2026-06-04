; ============================================================================
; demon_flash.s — DeMoN II firmware-flash support (DEMON-only)
;
; Whole file is gated behind TARGET_DEMON (the Makefiles wildcard srcs/asm/*.s
; into BOTH builds; like demon_boot.s, the kickstart object stays empty).
;
; STEP 1 (this file, so far): read-only flash chip detection.
;   The two flash chips that make up the 256KB cartridge ROM at $A80000
;   (U3 = high byte / even lane, U5 = low byte / odd lane) are put into the
;   JEDEC autoselect (product-identification) mode and their (mfr<<8|device)
;   signatures are read back and reported over USB-C.  NO erase / NO program
;   -> zero data-write risk.
;
;   Why a relocated stub: while a chip is in autoselect mode it stops
;   returning code, so the detect sequence cannot run from the flash it is
;   probing ($A80000).  We copy a small position-independent stub into DeMoN
;   SRAM and run it there.  Running from SRAM (NOT chip RAM) also keeps the
;   FT245/USB-C alive (see CLAUDE.md 6.1), so we can print the result.  JP1 is
;   indifferent for SRAM execution (confirmed on hardware).
;
;   The actual FLASH (erase/program) must instead run from CHIP RAM and kills
;   USB-C until reset -> that will be the LAST action of a later step.
;
; ----------------------------------------------------------------------------
; CREDITS: the low-level flash algorithm (flashcode — JEDEC unlock / sector- or
; bulk-erase / program with toggle-bit polling, written word-wide across U3+U5)
; and the Y-modem receiver are ported from the Action Replay 5 firmware
; (AR5.asm) by REbEL / QUARTEX, which was itself written for the DeMoN II
; cartridge.  Reused here with thanks.  The DiagROM front-end and integration
; are this project's; the proven flash/transfer cores are theirs.
; ----------------------------------------------------------------------------
; ============================================================================

        ifd TARGET_DEMON

        include "earlymacros.i"
        include "globalvars.i"

        section "demonflash",code_p

        xdef    demonFlashDetect      ; C-callable: -> D0 = (U3<<16)|U5 chip ids
        xdef    demonFlashReceive     ; C-callable: -> D0 = 0 received+valid / nonzero
        xdef    demonFlashWrite       ; C-callable: pre-flight + flash; rts(D0!=0) on fail
        xref    usb_putc
        xref    usb_putc_str
        xref    usb_drain_rx
        xref    ClearScreen           ; clear video bitplane on entry
        xref    Print                 ; a0=string, d1=color (bitplane)
        xref    SetPos                ; d0=xpos, d1=ypos (text cursor)

ROMB    equ     $a80000              ; DEMON_ROM_BASE (flash base)
UNLK1   equ     $a80000+($5555*2)    ; $a8aaaa : JEDEC unlock cycle 1 addr (even lane)
UNLK2   equ     $a80000+($2aaa*2)    ; $a85554 : JEDEC unlock cycle 2 addr (even lane)

ID_ATMEL equ    $1fd5                ; Atmel AT29C010A
ID_AMD   equ    $0120               ; AMD Am29F010
ID_SST   equ    $bfb5                ; SST39SF010A

DEMON_USB_DATA equ $bbfff1           ; FT245 data (byte)
DEMON_USB_STAT equ $bbfff2           ; FT245 status (word): b1=rxf(1=empty) b0=txe(1=full)

; Y-modem image buffer in DeMoN SRAM (256KB). Readable from chip-RAM flashcode
; with JP1 2-3 (step 3), and clear of chip-RAM video / globals.
DEMON_FLASH_IMG equ $b00000          ; $b00000-$b3ffff (256KB)
DEMON_YM_TMP    equ $b80000          ; scratch for header / null packets (>= 1KB)
IMG_SIZE        equ $40000           ; 256KB
DEMON_RAM_BASE  equ $ac0000          ; end of flash / start of DeMoN SRAM (verify end)

; --- flash command/unlock addresses (word access -> both chips in parallel) ---
FL_UNLK1 equ    $a80000+($5555*2)    ; $a8aaaa
FL_UNLK2 equ    $a80000+($2aaa*2)    ; $a85554

; --- custom chip registers ($dff000) ---
CUST_INTENAR equ $dff01c
CUST_INTENA  equ $dff09a
CUST_DMACONR equ $dff002
CUST_DMACON  equ $dff096
CUST_COLOR00 equ $dff180

; --- progress bar geometry (bitplane stride 80 bytes/pixel-row, like AR5) ---
BPL_STRIDE   equ 80
FL_PROGROW   equ 97*BPL_STRIDE-8     ; "FLASHING" bar pixel-row byte offset
FL_VERIFYROW equ 129*BPL_STRIDE-8    ; "VERIFYING" bar pixel-row byte offset

; --- flashcode scratch buffer in chip RAM (within the probe-validated page) ---
;   Offset into Chipmemstuff (globals->BPL): start of ECSCopperList[68], which is
;   used only by the ECS graphics test -> free during flash, and it sits before
;   the 64KB page boundary. Mirrors srcs/globalvars.h Chipmemstuff layout; if that
;   struct changes, update this. ~5KB contiguous free here (ECS+ECS2+ptplay+audio).
FLBUF_OFF    equ $f0bc              ; 61628

X_SOH   equ     $01
X_STX   equ     $02
X_EOT   equ     $04
X_ACK   equ     $06
X_NAK   equ     $15
X_CAN   equ     $18
YM_CHARTIMO equ $80000               ; ~1-2s per-char receive timeout (busy loop)

;=============================================================================
; demonFlashDetect — C-callable read-only flash chip detect.
;   Runs the detect stub from SRAM (USB stays alive). Returns the two JEDEC
;   signatures packed in D0: U3 (even lane) in bits 31..16, U5 (odd) in 15..0.
;   Preserves all callee-saved registers so it is safe to call from C.
;=============================================================================
demonFlashDetect:
        movem.l d2-d7/a2-a6,-(sp)
        bsr     run_detect_stub       ; -> D6 = U3 id, D7 = U5 id
        moveq   #0,d0
        move.w  d6,d0
        swap    d0
        move.w  d7,d0                 ; D0 = (U3<<16)|U5
        movem.l (sp)+,d2-d7/a2-a6
        rts

;=============================================================================
; print_hex16 — print D0.W as 4 uppercase hex digits over USB-C.
;   usb_putc preserves all data/addr regs, so D2/D3 survive across it.
;=============================================================================
print_hex16:
        move.w  d0,d3
        moveq   #3,d2
.lp:
        rol.w   #4,d3
        move.w  d3,d0
        and.w   #$000f,d0
        cmp.b   #10,d0
        blt.s   .digit
        add.b   #'A'-10,d0
        bra.s   .out
.digit:
        add.b   #'0',d0
.out:
        bsr     usb_putc
        dbf     d2,.lp
        rts

;=============================================================================
; print_chip_name — given D0.W = signature, print a friendly name + CRLF.
;=============================================================================
print_chip_name:
        cmp.w   #ID_ATMEL,d0
        beq.s   .atmel
        cmp.w   #ID_AMD,d0
        beq.s   .amd
        cmp.w   #ID_SST,d0
        beq.s   .sst
        bsr     usb_putc_str
        dc.b    ' (UNKNOWN - not in DeMoN2 spec!)',13,10,0
        even
        rts
.atmel:
        bsr     usb_putc_str
        dc.b    ' Atmel AT29C010A',13,10,0
        even
        rts
.amd:
        bsr     usb_putc_str
        dc.b    ' AMD Am29F010',13,10,0
        even
        rts
.sst:
        bsr     usb_putc_str
        dc.b    ' SST39SF010A',13,10,0
        even
        rts

;=============================================================================
; usb_getc_to — receive one byte with timeout.
;   In:  D1.L = timeout loop count.   Out: D0.L = byte (0..255) or -1 timeout.
;   Clobbers D0,D1 only.
;=============================================================================
usb_getc_to:
.wait:
        move.w  DEMON_USB_STAT,d0
        btst    #1,d0                 ; rxf: 1 = RX FIFO empty
        beq.s   .got
        subq.l  #1,d1
        bne.s   .wait
        moveq   #-1,d0
        rts
.got:
        moveq   #0,d0
        move.b  DEMON_USB_DATA,d0
        rts

;=============================================================================
; crc16_calc — CRC16-CCITT/XMODEM (poly $1021, init 0) over D2 bytes at A0.
;   Out: D0.W = crc.  Clobbers D0,D1,D2,A0.
;=============================================================================
crc16_calc:
        moveq   #0,d0
.cb:
        move.b  (a0)+,d1
        lsl.w   #8,d1
        eor.w   d1,d0
        moveq   #7,d1                 ; reuse d1 as bit counter
.cbit:
        add.w   d0,d0
        bcc.s   .cnp
        eor.w   #$1021,d0
.cnp:
        dbf     d1,.cbit
        subq.l  #1,d2
        bne.s   .cb
        rts

;=============================================================================
; ym_getpacket — receive one Y-modem packet payload into (A1).
;   (Y-modem receiver ported from AR5.asm by REbEL / QUARTEX — see file header.)
;   In:  A1 = destination buffer.
;   Out: D0 = 0 ok / 1 EOT / 2 CAN / 3 timeout-or-error.
;        When ok: D2.L = payload length (128 or 1024), D3.B = block number,
;                 A1 advanced past payload.
;   Preserves D4-D7/A2-A6 (saves D4-D5); clobbers D0-D3/A0.
;=============================================================================
ym_getpacket:
        movem.l d4-d5,-(sp)
        move.l  #YM_CHARTIMO,d1
        bsr     usb_getc_to
        cmp.l   #-1,d0
        beq     .err
        cmp.b   #X_SOH,d0
        beq.s   .soh
        cmp.b   #X_STX,d0
        beq.s   .stx
        cmp.b   #X_EOT,d0
        beq     .eot
        cmp.b   #X_CAN,d0
        beq     .can
        bra     .err                  ; unexpected byte
.soh:
        move.l  #128,d2
        bra.s   .hdr
.stx:
        move.l  #1024,d2
.hdr:
        move.l  #YM_CHARTIMO,d1
        bsr     usb_getc_to
        cmp.l   #-1,d0
        beq     .err
        move.b  d0,d3                 ; block number
        move.l  #YM_CHARTIMO,d1
        bsr     usb_getc_to
        cmp.l   #-1,d0
        beq     .err
        move.b  d3,d4
        not.b   d4
        cmp.b   d0,d4                 ; complement check: d0 == ~blk ?
        bne     .err
        move.l  a1,a0                 ; payload start (for CRC)
        move.l  d2,d5                 ; byte counter
.rdpl:
        move.l  #YM_CHARTIMO,d1
        bsr     usb_getc_to
        cmp.l   #-1,d0
        beq     .err
        move.b  d0,(a1)+
        subq.l  #1,d5
        bne.s   .rdpl
        ; receive 2 CRC bytes -> d4
        move.l  #YM_CHARTIMO,d1
        bsr     usb_getc_to
        cmp.l   #-1,d0
        beq     .err
        move.w  d0,d4
        lsl.w   #8,d4
        move.l  #YM_CHARTIMO,d1
        bsr     usb_getc_to
        cmp.l   #-1,d0
        beq     .err
        and.w   #$ff,d0
        or.w    d0,d4                 ; d4 = received CRC
        move.l  d2,-(sp)             ; save payload len
        bsr     crc16_calc           ; a0=start, d2=len -> d0=crc
        move.l  (sp)+,d2
        and.l   #$ffff,d4
        cmp.w   d4,d0
        bne.s   .err
        moveq   #0,d0
        bra.s   .ret
.eot:
        moveq   #1,d0
        bra.s   .ret
.can:
        moveq   #2,d0
        bra.s   .ret
.err:
        moveq   #3,d0
.ret:
        movem.l (sp)+,d4-d5
        rts

;=============================================================================
; validate_image — check the received image at A0 (preserved).
;   ACTI signature + ROM version + checksum (same as ar5flasher.e).
;   Out: D0 = 0 valid / nonzero invalid.  Prints result over USB.
;=============================================================================
validate_image:
        move.l  4(a0),d0
        cmp.l   #'ACTI',d0
        beq.s   .sigok
        bsr     usb_putc_str
        dc.b    'INVALID: ACTI signature not found',13,10,0
        even
        moveq   #1,d0
        rts
.sigok:
        move.l  $7c(a0),d0
        and.l   #$fffc0000,d0
        cmp.l   #ROMB,d0
        beq.s   .verok
        bsr     usb_putc_str
        dc.b    'INVALID: wrong ROM version (need DeMoN v2 @ $A80000)',13,10,0
        even
        moveq   #2,d0
        rts
.verok:
        lea     $7c(a0),a1
        move.l  a0,a2
        add.l   #IMG_SIZE-4,a2        ; a2 = &stored_checksum (displacement too big for d(An))
        moveq   #0,d1
.csum:
        add.l   (a1)+,d1
        cmp.l   a1,a2
        bne.s   .csum
        move.l  (a2),d2              ; stored checksum
        cmp.l   d1,d2
        beq.s   .csok
        bsr     usb_putc_str
        dc.b    'VALID but checksum mismatch (would be corrected on flash)',13,10,0
        even
        moveq   #0,d0
        rts
.csok:
        bsr     usb_putc_str
        dc.b    'VALID: ACTI + DeMoN v2 + checksum OK -> ready to flash',13,10,0
        even
        moveq   #0,d0
        rts

;=============================================================================
; FlashReceive — receive a firmware image via standard Y-modem (CRC, 1K blocks)
;   into the DeMoN SRAM buffer, then validate.  Runs from ROM (USB alive).
;   NO flash.  Ymodem-G (streaming) was dropped: unreliable on hardware (FIFO
;   overrun, no recovery) — the per-block ACK path reaches ~14 KB/s with 1K
;   blocks, plenty for a 256KB image (~18s).
;=============================================================================
demonFlashReceive:
        movem.l d2-d7/a2-a4,-(sp)
        bsr     usb_drain_rx
        bsr     usb_putc_str
        dc.b    13,10,'== Receive firmware via Y-modem (no flash) ==',13,10
        dc.b    'Start the Y-modem send from your terminal now...',13,10,0
        even
        lea     DEMON_FLASH_IMG,a4    ; image write pointer
        moveq   #0,d6                 ; bytes stored
        ; ---- header (block 0): poll 'C'/'G' until it arrives ----
        moveq   #15,d7                ; poll attempts
.hpoll:
        moveq   #'C',d0
        bsr     usb_putc
        lea     DEMON_YM_TMP,a1
        bsr     ym_getpacket
        tst.l   d0
        beq.s   .hgot
        cmp.l   #2,d0
        beq     .cancelled
        dbf     d7,.hpoll
        bra     .timeout
.hgot:
        tst.b   d3                    ; header must be block 0
        bne     .timeout
        moveq   #X_ACK,d0
        bsr     usb_putc
        moveq   #1,d4                 ; next expected data block = 1
        moveq   #'C',d0               ; request first data block
        bsr     usb_putc
        ; ---- data blocks ----
.dloop:
        move.l  a4,a1                 ; receive directly into the image
        bsr     ym_getpacket
        tst.l   d0
        beq.s   .dgot
        cmp.l   #1,d0
        beq     .eot
        cmp.l   #2,d0
        beq     .cancelled
        moveq   #X_NAK,d0             ; timeout/CRC error -> NAK, retry
        bsr     usb_putc
        bra.s   .dloop
.dgot:
        move.b  d4,d0
        cmp.b   d3,d0
        beq.s   .dseq
        subq.b  #1,d0                 ; previous block? (duplicate)
        cmp.b   d3,d0
        bne.s   .dnak
        moveq   #X_ACK,d0             ; dup -> re-ACK, ignore
        bsr     usb_putc
        bra.s   .dloop
.dnak:
        moveq   #X_NAK,d0
        bsr     usb_putc
        bra.s   .dloop
.dseq:
        add.l   d2,a4                 ; commit block
        add.l   d2,d6
        addq.b  #1,d4
        cmp.l   #IMG_SIZE,d6
        bhi     .toobig
        moveq   #X_ACK,d0
        bsr     usb_putc
        moveq   #'.',d0              ; progress
        bsr     usb_putc
        bra     .dloop
.eot:
        moveq   #X_NAK,d0             ; Y-modem: NAK first EOT
        bsr     usb_putc
        lea     DEMON_YM_TMP,a1
        bsr     ym_getpacket          ; expect 2nd EOT
        moveq   #X_ACK,d0             ; ACK it
        bsr     usb_putc
        moveq   #'C',d0              ; request batch terminator
        bsr     usb_putc
        lea     DEMON_YM_TMP,a1
        bsr     ym_getpacket          ; null header block
        moveq   #X_ACK,d0
        bsr     usb_putc
        ; ---- report + validate ----
        bsr     usb_putc_str
        dc.b    13,10,'Received $',0
        even
        move.l  d6,d0
        swap    d0
        bsr     print_hex16
        move.l  d6,d0
        bsr     print_hex16
        bsr     usb_putc_str
        dc.b    ' bytes.',13,10,0
        even
        lea     DEMON_FLASH_IMG,a0
        bsr     validate_image        ; D0 = 0 valid / nonzero invalid
        bra     .finish
.timeout:
        bsr     usb_putc_str
        dc.b    13,10,'Y-modem receive TIMEOUT / aborted.',13,10,0
        even
        moveq   #1,d0
        bra     .finish
.cancelled:
        bsr     usb_putc_str
        dc.b    13,10,'Transfer cancelled by sender.',13,10,0
        even
        moveq   #1,d0
        bra     .finish
.toobig:
        bsr     usb_putc_str
        dc.b    13,10,'ERROR: image larger than 256KB - aborted.',13,10,0
        even
        moveq   #1,d0
.finish:
        movem.l (sp)+,d2-d7/a2-a4
        rts                           ; D0 = status to the C caller

;=============================================================================
; demonFlashWrite — C-callable: flash the received image to the cartridge ROM.
;   The C flash menu has already shown the warning screen and got the user's
;   confirmation; this routine does NOT prompt.  Pre-flight runs from DeMoN ROM
;   with USB alive: validate image, detect chips, fix checksum, render the
;   on-screen UI, then copy the flashcode into chip RAM and jump there.  From
;   that jump on, the cartridge runs from chip RAM with JP1 set to 2-3: flash
;   ($a80000) + image ($b00000) stay visible but USB is dead -> all feedback is
;   on screen (progress bars, then a solid colour).  Requires Mode A.
;   Returns (D0!=0) only if a pre-flight check fails; never returns on success.
;=============================================================================
demonFlashWrite:
        movem.l d2-d7/a2-a5,-(sp)
        bsr     usb_drain_rx
        ; ---- Mode A required (need chip RAM to run the flashcode + a display) ----
        tst.b   NoDraw(a6)
        beq.s   .modeok
        bsr     usb_putc_str
        dc.b    13,10,'FLASH aborted: needs Mode A (working chip RAM + display).',13,10,0
        even
        bra     .abort
.modeok:
        bsr     usb_putc_str
        dc.b    13,10,'== FLASH firmware to cartridge ==',13,10
        dc.b    'Validating image at $b00000 ...',13,10,0
        even
        ; ---- validate the received image ----
        lea     DEMON_FLASH_IMG,a0
        bsr     validate_image
        tst.l   d0
        beq.s   .imgok
        bsr     usb_putc_str
        dc.b    'No valid image in SRAM. Receive one first with (r).',13,10,0
        even
        bra     .abort
.imgok:
        ; ---- detect flash chips (read-only, runs from SRAM, USB alive) ----
        bsr     run_detect_stub       ; -> D6=U3 id, D7=U5 id
        move.w  d6,d0
        bsr     id_valid
        bne.s   .badid
        move.w  d7,d0
        bsr     id_valid
        beq.s   .idok
.badid:
        bsr     usb_putc_str
        dc.b    'Unsupported flash chip - cannot flash.',13,10,0
        even
        bra     .abort
.idok:
        bsr     usb_putc_str
        dc.b    'Flash chip id=$',0
        even
        move.w  d7,d0
        bsr     print_hex16
        move.w  d7,d0
        bsr     print_chip_name       ; prints name + CRLF
        ; ---- fix checksum: sum of longs $7c..$3fff8, store at $3fffc ----
        lea     DEMON_FLASH_IMG+$7c,a0
        lea     DEMON_FLASH_IMG+IMG_SIZE-4,a1
        moveq   #0,d0
.csloop:
        add.l   (a0)+,d0
        cmp.l   a0,a1
        bne.s   .csloop
        move.l  d0,(a1)               ; store corrected checksum
        ; ---- on-screen UI (rendered now, while ROM is still readable) ----
        ; The C menu already showed the warning screen and confirmed; from here
        ; on we commit (USB dies at the jsr (a4) below).
        jsr     ClearScreen
        moveq   #34,d0
        moveq   #10,d1
        jsr     SetPos
        lea     .flashingtxt(pc),a0
        moveq   #3,d1
        jsr     Print
        moveq   #33,d0
        moveq   #14,d1
        jsr     SetPos
        lea     .verifyingtxt(pc),a0
        moveq   #3,d1
        jsr     Print
        ; ---- copy flashcode into chip RAM (within the probe-validated page) ----
        move.l  BPL(a6),a1
        add.l   #FLBUF_OFF,a1
        move.l  a1,a4                 ; A4 = flashcode entry point in chip RAM
        lea     flashcode(pc),a0
        lea     flashcode_end(pc),a2
.cpfc:
        move.w  (a0)+,(a1)+
        cmp.l   a0,a2
        bne.s   .cpfc
        ; ---- registers for the chip-RAM flashcode, then jump (USB dies here) ----
        move.w  d7,d4                ; D4 = chip id (path selector)
        move.l  Bpl1Ptr(a6),a5       ; A5 = displayed bitplane base (progress bars)
        jsr     (a4)                 ; -> chip RAM; never returns (loops on colour)
.abort:
        bsr     usb_putc_str
        dc.b    13,10,'Flash pre-flight failed (no valid image / bad chip).',13,10,0
        even
        moveq   #1,d0                ; nonzero -> tell the C caller it failed
        movem.l (sp)+,d2-d7/a2-a5
        rts
.flashingtxt:
        dc.b    'FLASHING..',0
.verifyingtxt:
        dc.b    'VERIFYING...',0
        even

;=============================================================================
; run_detect_stub — copy the read-only detect stub into SRAM and run it there.
;   Out: D6.W = U3 (even) id, D7.W = U5 (odd) id.  USB stays alive (SRAM exec).
;=============================================================================
run_detect_stub:
        movem.l d0-d2/a0-a2,-(sp)
        lea     detect_stub(pc),a0
        lea     DEMON_YM_TMP,a1
        lea     detect_stub_end(pc),a2
.cpy:
        move.w  (a0)+,(a1)+
        cmp.l   a0,a2
        bne.s   .cpy
        jsr     DEMON_YM_TMP
        move.w  d0,d6
        move.w  d1,d7
        movem.l (sp)+,d0-d2/a0-a2
        rts

;=============================================================================
; id_valid — D0.W = id. Returns Z=1 (D0=0) if id in {Atmel,SST,AMD}, else Z=0.
;=============================================================================
id_valid:
        cmp.w   #ID_ATMEL,d0
        beq.s   .yes
        cmp.w   #ID_SST,d0
        beq.s   .yes
        cmp.w   #ID_AMD,d0
        beq.s   .yes
        moveq   #1,d0                ; invalid -> Z=0
        rts
.yes:
        moveq   #0,d0                ; valid -> Z=1
        rts

;=============================================================================
; flashcode — copied into chip RAM and executed there (faithful port of the
;   AR5.asm flashcode by REbEL / QUARTEX: WORD-wide writes program U3+U5 in
;   parallel — see file header for credits).
;   In:  D4.W = chip id ($1FD5 Atmel / $BFB5 SST / $0120 AMD)
;        A5   = displayed bitplane base (Bpl1Ptr) for the two progress bars
;   Position-independent: absolute device addresses ($a8xxxx/$b00000/$dffxxx) +
;   PC-relative branches only.  Interrupts are disabled (an IRQ would vector
;   through the now-unreadable ROM).  DMA is left on so the on-screen UI stays
;   live; it is killed only at the very end for a solid result colour.
;   Never returns: ends in goodflash (green) / badflash (red) colour loop.
;=============================================================================
flashcode:
        move.w  #$7fff,CUST_INTENA      ; interrupts off

        ; ---- SST/AMD: bulk-erase both chips first ----
        cmp.w   #ID_AMD,d4
        beq.s   .erase
        cmp.w   #ID_SST,d4
        bne.s   .noerase
.erase:
        move.w  #$aaaa,FL_UNLK1
        move.w  #$5555,FL_UNLK2
        move.w  #$8080,FL_UNLK1
        move.w  #$aaaa,FL_UNLK1
        move.w  #$5555,FL_UNLK2
        move.w  #$1010,FL_UNLK1
        lea     $a80008,a0
        bsr     fc_wait
.noerase:
        lea     $a80000,a0            ; A0 = flash write pointer
        lea     DEMON_FLASH_IMG,a2   ; A2 = image source
        move.w  #1024-1,d2           ; 1024 sectors (128 words each)
.sector:
        cmp.w   #ID_ATMEL,d4
        bne.s   .nosdp
        ; Atmel: software-data-protect unlock once per sector
        move.w  #$aaaa,FL_UNLK1
        move.w  #$5555,FL_UNLK2
        move.w  #$a0a0,FL_UNLK1
.nosdp:
        move.w  #128-1,d1
        ; skip the first 2 bytes ($a80000/01) at the very start
        cmp.l   #$a80000,a0
        bne.s   .word
        addq.l  #2,a0
        addq.l  #2,a2
        subq.w  #1,d1
.word:
        cmp.w   #ID_AMD,d4
        beq.s   .sstw
        cmp.w   #ID_SST,d4
        bne.s   .nosstw
.sstw:
        ; SST/AMD: program-unlock before every word
        move.w  #$aaaa,FL_UNLK1
        move.w  #$5555,FL_UNLK2
        move.w  #$a0a0,FL_UNLK1
.nosstw:
        move.w  (a2)+,(a0)+
        cmp.w   #ID_AMD,d4
        beq.s   .sstwait
        cmp.w   #ID_SST,d4
        bne.s   .nosstwait
.sstwait:
        move.l  d1,d5                ; preserve inner counter across the wait
        bsr     fc_wait
        move.l  d5,d1
.nosstwait:
        dbf     d1,.word
        bsr     fc_wait              ; Atmel: wait once at end of sector
        lea     FL_PROGROW(a5),a3
        bsr     fc_progress
        dbf     d2,.sector

        ; ---- verify ($a80004..$ac0000), word-wise ----
        moveq   #-1,d7
        move.l  #$20000-1,d4
        lea     $a80004,a0
        lea     DEMON_FLASH_IMG+4,a1
.vloop:
        move.l  d4,d2
        and.l   #$3f,d2
        bne.s   .vcmp
        move.l  d4,d2
        lsr.l   #7,d2
        lea     FL_VERIFYROW(a5),a3
        bsr     fc_progress
.vcmp:
        cmp.w   (a0)+,(a1)+
        beq.s   .vok
        moveq   #0,d7               ; mismatch
.vok:
        subq.l  #1,d4
        cmp.l   #DEMON_RAM_BASE,a0
        bne.s   .vloop
        tst.l   d7
        beq.s   .fail
        ; ---- success: blank to solid green ----
        move.w  #$7fff,CUST_INTENA
        move.w  #$7fff,CUST_DMACON
.good:
        move.w  #$00f0,CUST_COLOR00
        bra.s   .good
.fail:
        move.w  #$7fff,CUST_INTENA
        move.w  #$7fff,CUST_DMACON
.bad:
        move.w  #$0f00,CUST_COLOR00
        bra.s   .bad

; toggle-bit poll: both lanes of the last written word must read stable twice
fc_wait:
        move.b  -2(a0),d0
        move.b  -2(a0),d1
        cmp.b   d1,d0
        bne.s   fc_wait
        move.b  -1(a0),d0
        move.b  -1(a0),d1
        cmp.b   d1,d0
        bne.s   fc_wait
        rts

; progress bar (verbatim AR5 DoProgress): A3 = row base, D2 = count
fc_progress:
        move.w  d2,d0
        lsr.w   #1,d0
        moveq   #0,d1
        move.w  d0,d1
        lsr.w   #3,d1
        sub.l   d1,a3
        and.w   #7,d0
        bset    d0,(a3)
        bset    d0,80(a3)
        bset    d0,160(a3)
        bset    d0,240(a3)
        bset    d0,320(a3)
        bset    d0,400(a3)
        bset    d0,480(a3)
        bset    d0,560(a3)
        rts
flashcode_end:

;=============================================================================
; detect_stub — copied into DeMoN SRAM and executed there.
;   Position-independent: only absolute addresses ($A8xxxx flash, $DFFxxx
;   custom) and PC-relative short branches (bsr.s/bne.s).
;   Output: D0.W = U3 (even-lane) signature, D1.W = U5 (odd-lane) signature.
;   Clobbers D0-D2/D4-D5.  Interrupts + DMA are disabled across the autoselect
;   windows (an IRQ would vector through ROM while a chip is unreadable).
;=============================================================================
detect_stub:
        ; save + disable interrupts and DMA (mirrors the AR5 flasher)
        move.w  $dff01c,d4            ; INTENAR
        swap    d4
        move.w  $dff002,d4            ; DMACONR (low word of D4)
        or.l    #$80008000,d4         ; force SET bit for the later restore
        move.w  #$7fff,$dff09a        ; INTENA: disable all interrupts
        move.w  #$7fff,$dff096        ; DMACON: disable all DMA
        ; ---- chip on the even byte lane (U3, high) ----
        move.b  #$aa,UNLK1
        move.b  #$55,UNLK2
        move.b  #$90,UNLK1            ; enter autoselect
        bsr.w   .delay
        moveq   #0,d0
        move.b  ROMB+$400,d0          ; manufacturer id (even lane)
        lsl.w   #8,d0
        move.b  ROMB+$402,d0          ; device id (even lane)
        move.b  #$aa,UNLK1
        move.b  #$55,UNLK2
        move.b  #$f0,UNLK1            ; exit autoselect (back to read mode)
        bsr.w   .delay
        ; ---- chip on the odd byte lane (U5, low) ----
        move.b  #$aa,UNLK1+1
        move.b  #$55,UNLK2+1
        move.b  #$90,UNLK1+1
        bsr.w   .delay
        moveq   #0,d1
        move.b  ROMB+$400+1,d1
        lsl.w   #8,d1
        move.b  ROMB+$402+1,d1
        move.b  #$aa,UNLK1+1
        move.b  #$55,UNLK2+1
        move.b  #$f0,UNLK1+1
        bsr.w   .delay
        ; restore DMA + interrupts (D4 has bit15 forced set in both words)
        move.l  d4,d5
        move.w  d5,$dff096           ; restore DMACON
        swap    d5
        move.w  d5,$dff09a           ; restore INTENA
        rts
.delay:
        move.l  #$8000,d2
.dl:
        subq.l  #1,d2
        bne.s   .dl
        rts
detect_stub_end:

        endc                          ; ifd TARGET_DEMON
