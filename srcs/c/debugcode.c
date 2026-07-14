#include "globalvars.h"
#include "generic.h"
#include <exec/types.h>
#include <hardware/custom.h>
#define custom ((volatile struct Custom *)0xdff000)

// Clean, dedicated home for ad-hoc hardware experiments. Only linked in and
// called when built with `make DEBUGCODE=1` (see Makefile) — that build
// skips the boot-time "hold a key" gate, forces serial on unconditionally,
// and calls DebugCode() before the menu system is ever engaged. Falls
// through to the normal menu afterward as a fallback, but this function is
// the actual point of a DEBUGCODE build: almost a clean ROM of its own to
// "get loose" in, no menu navigation needed for each test cycle.
//
// Rewrite freely between test sessions — no promises about tidiness or
// backward compatibility with whatever was here last time.
//
// Step 1 (2026-07-14): detect the A3000's onboard WD33C93 SCSI chip and
// Ramsey memory controller are present, and report whatever version
// information is actually available for each. See
// [[project_diagrom_a3000_scsi]] memory for the wider investigation this
// small-steps rethink is part of.

// --- Ramsey (A3000 memory controller) ------------------------------------
// Source: leaked AmigaOS a3000_hardware.i (v40_src/include/hardware/):
//   A3000_RamseyVersion EQU $00de0043  ;12D ramsey==$0d
//   ANCIENT_RAMSEY      EQU $7f
// i.e. the version register is a plain byte at $DE0043; a pre-production
// ("ancient") Ramsey reads back $7F there instead of a real revision code.
// $0D is the only revision value directly confirmed in that source (real
// AmigaOS boot code compares against it); other byte values are reported
// as raw hex rather than guessed at.
#define RAMSEY_VERSION_ADDR ((volatile uint8_t *)0x00DE0043)
#define ANCIENT_RAMSEY 0x7F

static void checkRamsey(void)
{
    uint8_t ver = *RAMSEY_VERSION_ADDR;
    print("Ramsey version register ($DE0043): $", WHITE);
    print(binHexByte(ver), CYAN);
    if (ver == ANCIENT_RAMSEY) {
        print("  (ANCIENT_RAMSEY - pre-production chip, no real version)\n", YELLOW);
    } else if (ver == 0x0D) {
        print("  (confirmed value from leaked AmigaOS source, rev 4)\n", GREEN);
    } else {
        print("  (present, revision not in our sourced reference)\n", GREEN);
    }
}

// --- WD33C93 SCSI chip (A3000 onboard, via SDMAC pass-through) -----------
// Same register addresses disk.c's A3000 SCSI driver uses (WD_ADDR_REG/
// WD_AUX_STATUS/WD_REG_DATA there) - not shared code, this file is a
// deliberately separate sandbox, but the addresses themselves are a
// confirmed-correct hardware fact, not something to re-derive from scratch.
#define WD_ADDR_REG   ((volatile uint32_t *)0xDD0040)   // write-only, longword: select internal reg #
#define WD_AUX_STATUS ((volatile uint8_t  *)0xDD0041)   // read-only: auxiliary status
#define WD_REG_DATA   ((volatile uint8_t  *)0xDD0043)   // R/W: data for currently selected register

// SDMAC's own control register - must select SCSI peripheral mode (PMODE)
// before the WD33C93 pass-through registers respond meaningfully at all.
#define SDMAC_CNTR       ((volatile uint8_t *)0xDD000B)
#define SDMAC_CNTR_PDMD  0x08
#define SDMAC_CNTR_INTENA 0x04

// DAWR ("DACK Width Register", write-only, 16-bit) - a register we have
// NEVER written in any version of this ROM's SCSI code. Found by cross-
// referencing Linux's drivers/scsi/a3000.c/a3000.h and NetBSD's
// sys/arch/amiga/dev/ahscreg.h (both real, independently-written, decades-
// proven drivers for this exact chip): both write the value 3 here as one
// of the very first hardware accesses, before touching the WD33C93 at all.
// NetBSD's header cites this outright: "according to A3000T service-
// manual". Neither the leaked AmigaOS scsitask.asm nor any prior DiagROM
// session ever referenced this register - it likely lives in a lower-level
// Kickstart/board-init routine in the real OS that runs before
// scsidisk.device ever gets control, which we never had visibility into
// before finding these two independent sources.
#define SDMAC_DAWR       ((volatile uint16_t *)0xDD0002)
#define DAWR_A3000_VAL   3

#define WD_OWN_ID     0x00
#define WD_SCSI_STATUS 0x17
#define WD_COMMAND    0x18

#define WDCMD_RESET   0x00

// Own ID register bits relevant here (WD33C93B datasheet, section 3.1.3):
//   bit 3 = EAF (Enable Advanced Features)
// Requesting EAF before RESET and getting WDSTS_RESET_AF back instead of
// plain WDSTS_RESET tells us the chip actually understands/supports
// advanced features - i.e. it's at least a 33C93A, not an original 33C93.
// This does NOT reliably distinguish 33C93A from 33C93B on its own (no
// documented software-readable "which sub-revision" register exists in the
// datasheet we have) - reported honestly as "supports advanced features",
// not as a specific chip revision.
#define WD_OWN_ID_EAF 0x08

#define WDSTS_RESET     0x00
#define WDSTS_RESET_AF  0x01

static inline void wd_write(uint8_t reg, uint8_t val)
{
    *WD_ADDR_REG = reg;
    *WD_REG_DATA = val;
}

static inline uint8_t wd_read(uint8_t reg)
{
    *WD_ADDR_REG = reg;
    return *WD_REG_DATA;
}

static void checkWD33C93(void)
{
    *SDMAC_DAWR = DAWR_A3000_VAL;   // NEW (2026-07-14) - see SDMAC_DAWR comment above
    print("Wrote DAWR ($DD0002) = ", WHITE);
    print(binDec(DAWR_A3000_VAL), CYAN);
    print("\n", WHITE);

    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;   // SCSI mode + let status latch

    uint8_t asr = *WD_AUX_STATUS;
    print("WD33C93 aux status ($DD0041): $", WHITE);
    print(binHexByte(asr), asr == 0xFF ? RED : CYAN);
    print("\n", WHITE);

    if (asr == 0xFF) {
        print("  Bus float - chip not responding, nothing else to check.\n", RED);
        return;
    }
    print("  Chip present.\n", GREEN);

    // Request advanced features, then reset, and see what the chip actually
    // acknowledges.
    wd_write(WD_OWN_ID, WD_OWN_ID_EAF);
    wd_write(WD_COMMAND, WDCMD_RESET);

    uint32_t t = 2000000UL;
    uint8_t st = 0xFF;
    while (--t) {
        uint8_t a = *WD_AUX_STATUS;
        if (a != 0xFF && (a & 0x80)) {   // WD_ASR_INT
            st = wd_read(WD_SCSI_STATUS);
            break;
        }
    }

    print("  Reset status: $", WHITE);
    print(binHexByte(st), CYAN);
    if (st == WDSTS_RESET_AF) {
        print("  (RESET_AF - chip supports Advanced Features, so at least a 33C93A)\n", GREEN);
    } else if (st == WDSTS_RESET) {
        print("  (plain RESET - Advanced Features not acknowledged; may be an\n"
              "   original 33C93, or EAF request wasn't honored for some other reason)\n", YELLOW);
    } else {
        print("  (unexpected/no response within budget)\n", RED);
    }
}

// --- IRQ2 (PORTS) handler ------------------------------------------------
// Step 2 (2026-07-14): every Amiga HD controller, including this A3000
// SDMAC/WD33C93, shares the level-2 autovector PORTS interrupt
// (custom.intreq bit 3). This is the exact same validated pattern already
// used in disk.c's ScsiPortsIRQ/a3k_scsi_irq_enable/a3k_scsi_irq_disable
// (confirmed firing on real hardware in earlier sessions) - reimplemented
// fresh here rather than reused, since this file is a deliberately separate
// sandbox, not sharing code with the "production" driver in disk.c.
//
// Talks back to the rest of the ROM only through globals-> — never a plain
// C static. This linker script (srcs/link.txt) puts every static/global
// into the ROM's single read-only output section (no writable RAM section
// exists at all in this freestanding build), so a static written from an
// interrupt handler would be silently discarded; globals-> lives in real
// allocated RAM instead. Reuses the SAME ScsiIrqPending/ScsiIrqStatus/
// ScsiIrqCount fields disk.c's driver already defines in globalvars.h -
// they're generic "last SCSI PORTS completion" fields, not owned by any one
// driver.
#define SCSI_PORTS_BIT 0x0008

__interrupt void DebugPortsIRQ(VARS)
{
    uint16_t irq = custom->intreqr;
    if (irq & SCSI_PORTS_BIT) {
        globals->ScsiIrqStatus = wd_read(WD_SCSI_STATUS);   // ack WD33C93's ASR INT, capture status
        globals->ScsiIrqPending = 1;
        globals->ScsiIrqCount++;
    }
    custom->intreq = SCSI_PORTS_BIT;
    custom->intreq = SCSI_PORTS_BIT;
}

static inline void debug_set_sr(uint16_t sr __asm("d0"))
{
    asm volatile ("move %0,sr\n" : : "d" (sr) : "cc");
}

static void debugIrqEnable(volatile struct GlobalVars *globals)
{
    globals->ScsiIrqPending = 0;
    globals->ScsiIrqStatus  = 0;
    globals->ScsiIrqCount   = 0;
    *(volatile APTR *) + 0x68 = DebugPortsIRQ;
    custom->intreq = SCSI_PORTS_BIT;             // clear any stale/latched request first
    custom->intreq = SCSI_PORTS_BIT;
    custom->intena = 0xC000 | SCSI_PORTS_BIT;    // master enable + PORTS
    custom->intena = 0xC000 | SCSI_PORTS_BIT;
    debug_set_sr(0x2000);                        // unmask CPU interrupts so the vector can run
}

static void debugIrqDisable(void)
{
    custom->intena = 0x7fff;    // fully quiesce again
    custom->intreq = 0x7fff;
    *(volatile APTR *) + 0x68 = RTEcode;
}

static inline volatile struct GlobalVars *debugGlobals(void)
{
    volatile struct GlobalVars *g;
    asm volatile ("move.l a6,%0" : "=r" (g));
    return g;
}

// Host SCSI ID (bits 2-0) + clock divisor (bits 7-6, FS1/FS0 - see
// checkWD33C93() comment): $40 = FS1=0/FS0=1 = divisor 3, correct for the
// A3000's ~14MHz WD33C93 input clock (matches the leaked AmigaOS board.i
// comment: "a3000 has 14Mhz clock, uses 3"). | 7 = host ID 7.
#define WD_OWN_ID_VAL 0x47

// Does a RESET actually generate a real IRQ2 (PORTS) event we can catch?
// The datasheet is explicit that RESET always completes with a status
// interrupt, so this is a real, guaranteed-to-fire event - a clean way to
// confirm the whole IRQ2 pipeline (vector patch, custom.intena, our
// handler, globals-> handoff) genuinely works end to end BEFORE trying
// anything SELECT-related, rather than assuming it from disk.c's earlier
// (separate codebase) success.
static void checkIRQ2(void)
{
    volatile struct GlobalVars *globals = debugGlobals();

    print("Enabling IRQ2 (PORTS) handler, then issuing a fresh RESET...\n", WHITE);
    debugIrqEnable(globals);

    wd_write(WD_OWN_ID, WD_OWN_ID_VAL);   // must precede RESET - clock divisor latches at reset time
    wd_write(WD_COMMAND, WDCMD_RESET);

    // Real-time-paced wait (waitShort() paces via the video beam position
    // register, ~640us/iteration, independent of CPU speed) - not a raw
    // instruction-count spin. ~500 iterations =~ 320ms, comfortably past
    // the chip's own reset time.
    int i;
    for (i = 0; i < 500; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }

    print("Waited ", WHITE);
    print(binDec(i), CYAN);
    print(" iterations (~640us each) for the IRQ to fire.\n", WHITE);

    debugIrqDisable();

    print("IRQ2 (PORTS) events serviced: ", WHITE);
    print(binDec(globals->ScsiIrqCount), globals->ScsiIrqCount ? GREEN : YELLOW);
    print(globals->ScsiIrqCount ? "  (RESET's completion DID trigger a real IRQ2)\n"
                                 : "  (RESET's completion did NOT trigger IRQ2 - pipeline problem)\n",
          globals->ScsiIrqCount ? GREEN : RED);
    print("Last SCSI_STATUS seen: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), CYAN);
    print("\n", WHITE);
}

// --- Real driver init (IDSet) + DoSelect(), translated directly ---------
// Step 3 (2026-07-14): translating the real, confirmed-working AmigaOS
// scsidisk.device driver (scsitask.asm) directly instead of reconstructing
// from memory - "AmigaOS (and AROS) is confirmed working, so follow that."
//
// IDSet (the real driver's post-reset init, ~line 1364): runs once, right
// after a plain (non-advanced-features) RESET completes.
//   JAMREG #0,CONTROL              - DMA mode disabled
//   JAMREG TIMEOUT_VAL,TIMEOUT     - selection timeout period (44 = 0x2C,
//                                    confirmed in other/a4091/board.i)
//   JAMREG #$40,SYNC_TRANSFER      - asynchronous, req/ack=300ns
//   JAMREG #WDCF_ER,SOURCE_ID      - enable reselection (WDCF_ER = bit7 =
//                                    0x80, confirmed in board.i BITDEF)
#define WD_CONTROL     0x01
#define WD_TIMEOUT_REG 0x02
#define WD_SYNC_XFER   0x11
#define WD_SRC_ID      0x16
#define WD_DST_ID      0x15
#define WD_TARGET_LUN  0x0F

#define WD_TIMEOUT_VAL 44        // 0x2C - confirmed in board.i
#define WD_SRC_ID_ER   0x80      // WDCF_ER, confirmed in board.i BITDEF

#define WDCMD_SELECT_WITH_ATN 0x06

// Does the real driver's init sequence, from a clean (non-AF) reset. Must
// be called with the IRQ2 handler already enabled - waits on
// globals->ScsiIrqPending for the reset completion, exactly like real
// AmigaOS does (IDSet only runs once SCSIService has dispatched the reset's
// own completion interrupt).
static int wdRealInit(volatile struct GlobalVars *globals)
{
    globals->ScsiIrqPending = 0;
    wd_write(WD_OWN_ID, WD_OWN_ID_VAL);   // no EAF - plain reset, matches real IDSet path
    wd_write(WD_COMMAND, WDCMD_RESET);

    int i;
    for (i = 0; i < 500; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    if (!globals->ScsiIrqPending) {
        print("  RESET did not complete - aborting init.\n", RED);
        return 0;
    }
    globals->ScsiIrqPending = 0;

    wd_write(WD_CONTROL, 0x00);
    wd_write(WD_TIMEOUT_REG, WD_TIMEOUT_VAL);
    wd_write(WD_SYNC_XFER, 0x40);
    wd_write(WD_SRC_ID, WD_SRC_ID_ER);
    print("  Real driver's IDSet sequence applied (CONTROL/TIMEOUT/SYNC_XFER/SOURCE_ID).\n", GREEN);
    return 1;
}

// DoSelect() (~line 819): the ENTIRE real selection sequence for a plain
// Select-with-ATN, verbatim:
//   move.b CNTR(a4),d0 / bclr DMAB_INTENA,d0 / move.b d0,CNTR(a4)  - INTENA off
//   PUTREG hu_Unit(a0),DEST_ID       - JUST the raw target unit, no DPD -
//                                      DPD only applies to the autonomous
//                                      Select-and-Transfer command, which
//                                      this manual-phase approach never
//                                      uses (confirmed against the
//                                      datasheet's own DST_ID bit
//                                      description).
//   PUTREG #wd_SELECT_WITH_ATN,COMMAND
//   bset DMAB_INTENA,d0 / move.b d0,CNTR(a4)  - INTENA back on
// Returns the status byte, or 0xFF if no interrupt arrived at all.
static uint8_t checkSelect(volatile struct GlobalVars *globals, uint8_t unit)
{
    print("Selecting unit ", WHITE);
    print(binDec(unit), CYAN);
    print("...\n", WHITE);

    globals->ScsiIrqPending = 0;

    *SDMAC_CNTR &= ~SDMAC_CNTR_INTENA;             // INTENA off (real driver's "bclr DMAB_INTENA")
    wd_write(WD_DST_ID, unit);                     // just the raw unit - no DPD, see comment above
    wd_write(WD_COMMAND, WDCMD_SELECT_WITH_ATN);
    *SDMAC_CNTR |= SDMAC_CNTR_INTENA;               // INTENA back on

    int i;
    for (i = 0; i < 4000; i++) {                    // generous budget - real hardware needed more than
        if (globals->ScsiIrqPending) break;         // the chip's own ~250ms timeout in earlier sessions
        waitShort();
    }

    print("  Waited ", WHITE);
    print(binDec(i), CYAN);
    print(" iterations for a result.\n", WHITE);

    if (!globals->ScsiIrqPending) {
        print("  NO interrupt at all - stuck, same as the historical symptom.\n", RED);
        return 0xFF;
    }
    globals->ScsiIrqPending = 0;

    print("  Status: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), CYAN);
    print("\n", WHITE);
    return globals->ScsiIrqStatus;
}

// --- G1.1110 (message-out phase) / SCSIPutByte(), translated directly ---
// Step 4 (2026-07-14): for a SINGLE-BYTE message (our IDENTIFY case),
// G1.1110 falls through to its "NotSync" path, which is just SCSIPutByte()
// (~line 955) - NOT the count-register SCSIPutBytes() multi-byte routine
// used for the CDB/DATA_IN phases. Verbatim:
//   PUTREG #0,CONTROL                    - DMA off
//   PUTREG #WDCF_SBT!wd_TRANSFER_INFO,COMMAND
//   [poll DBR - Data Buffer Ready]
//   PUTREG data,DATA_REG
// Crucially, SCSIPutByte() does NOT wait inline for a completion interrupt
// afterward - it returns immediately once the byte is written. The next
// interrupt (whatever it reports) is serviced entirely separately, by the
// main SCSIService dispatch loop - there is no "wait right here" step in
// the real architecture. We mirror that structurally: send the byte, then
// separately wait for whatever IRQ2 event comes next, rather than treating
// it as one atomic operation.
#define WD_CMD_SBT 0x80
#define WD_DATA    0x19
#define WDCMD_TRANSFER_INFO 0x20
#define WDCMD_NEGATE_ACK    0x03

static uint8_t checkMsgOutIdentify(volatile struct GlobalVars *globals, uint8_t identifyByte)
{
    print("Sending IDENTIFY ($", WHITE);
    print(binHexByte(identifyByte), CYAN);
    print(") via SBT (matches real SCSIPutByte, not the count-register path)...\n", WHITE);

    globals->ScsiIrqPending = 0;

    wd_write(WD_CONTROL, 0x00);
    wd_write(WD_COMMAND, WD_CMD_SBT | WDCMD_TRANSFER_INFO);

    int i;
    int gotDbr = 0;
    for (i = 0; i < 500000; i++) {
        uint8_t asr = *WD_AUX_STATUS;
        if (asr != 0xFF && (asr & 0x01)) { gotDbr = 1; break; }   // WD_ASR_DBR
    }
    if (!gotDbr) {
        print("  DBR never asserted - target didn't accept the message byte.\n", RED);
        return;
    }
    wd_write(WD_DATA, identifyByte);
    print("  Byte written (DBR seen after ", WHITE);
    print(binDec(i), CYAN);
    print(" polls).\n", WHITE);

    int j;
    for (j = 0; j < 4000; j++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("  Waited ", WHITE);
    print(binDec(j), CYAN);
    print(" iterations for the NEXT IRQ2 event.\n", WHITE);

    if (!globals->ScsiIrqPending) {
        print("  NO further interrupt - stuck here, same as the historical symptom.\n", RED);
        return 0xFF;
    }
    globals->ScsiIrqPending = 0;
    print("  Next status: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), CYAN);
    print("\n", WHITE);
    return globals->ScsiIrqStatus;
}

// --- G1.1010 (command phase) / SCSIPutBytes(), translated directly ------
// Step 5 (2026-07-14): unlike the single-byte MSG_OUT phase, the command
// phase sends a whole CDB (6 bytes for INQUIRY) - the real driver's G1.1010
// just calls SCSIPutBytes() (the count-register multi-byte routine), not
// SBT. Verbatim shape:
//   [transfer count registers <- CDB length]
//   PUTREG #0,CONTROL
//   PUTREG #wd_TRANSFER_INFO,COMMAND
//   [poll DBR, write each byte as it asserts; if an interrupt arrives
//    before the count is exhausted, exit immediately - the real driver
//    checks a stashed interrupt-event counter each iteration for this]
// Same structural pattern as MSG_OUT: returns without waiting inline for a
// completion interrupt once all bytes are sent - that's handled separately.
#define WD_XFER_CNT_H 0x12
#define WD_XFER_CNT_M 0x13
#define WD_XFER_CNT_L 0x14

static uint8_t checkCmdPhase(volatile struct GlobalVars *globals, const uint8_t *cdb, int len)
{
    print("Sending CDB (", WHITE);
    print(binDec(len), CYAN);
    print(" bytes) via count-register TRANSFER_INFO...\n", WHITE);

    globals->ScsiIrqPending = 0;

    wd_write(WD_XFER_CNT_H, 0);
    wd_write(WD_XFER_CNT_M, 0);
    wd_write(WD_XFER_CNT_L, (uint8_t)len);
    wd_write(WD_CONTROL, 0x00);
    wd_write(WD_COMMAND, WDCMD_TRANSFER_INFO);

    int idx = 0;
    uint32_t t;
    for (t = 0; t < 2000000UL; t++) {
        if (globals->ScsiIrqPending) break;   // early exit - unexpected phase change mid-CDB
        uint8_t asr = *WD_AUX_STATUS;
        if (asr == 0xFF) continue;
        if (asr & 0x01) {   // DBR
            wd_write(WD_DATA, cdb[idx]);
            idx++;
            if (idx >= len) break;
        }
    }

    print("  Sent ", WHITE);
    print(binDec(idx), CYAN);
    print(" of ", WHITE);
    print(binDec(len), CYAN);
    print(" bytes.\n", WHITE);

    if (idx < len && !globals->ScsiIrqPending) {
        print("  Stalled mid-CDB.\n", RED);
        return 0xFF;
    }

    int j;
    for (j = 0; j < 4000; j++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("  Waited ", WHITE);
    print(binDec(j), CYAN);
    print(" iterations for the NEXT IRQ2 event.\n", WHITE);

    if (!globals->ScsiIrqPending) {
        print("  NO further interrupt - stuck here.\n", RED);
        return 0xFF;
    }
    globals->ScsiIrqPending = 0;
    print("  Next status: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), CYAN);
    print("\n", WHITE);
    return globals->ScsiIrqStatus;
}

// --- G1.1001 (data-in phase) / SCSIGetBytes(), translated directly ------
// Step 6 (2026-07-14): the read-side mirror of checkCmdPhase() - same
// count-register TRANSFER_INFO shape, just reading DATA instead of writing
// it. This is what actually retrieves the INQUIRY response bytes.
static uint8_t checkDataInPhase(volatile struct GlobalVars *globals, uint8_t *buf, int len)
{
    print("Reading DATA_IN (", WHITE);
    print(binDec(len), CYAN);
    print(" bytes) via count-register TRANSFER_INFO...\n", WHITE);

    globals->ScsiIrqPending = 0;

    wd_write(WD_XFER_CNT_H, 0);
    wd_write(WD_XFER_CNT_M, 0);
    wd_write(WD_XFER_CNT_L, (uint8_t)len);
    wd_write(WD_CONTROL, 0x00);
    wd_write(WD_COMMAND, WDCMD_TRANSFER_INFO);

    int idx = 0;
    uint32_t t;
    for (t = 0; t < 2000000UL; t++) {
        if (globals->ScsiIrqPending) break;   // early exit - unexpected phase change mid-transfer
        uint8_t asr = *WD_AUX_STATUS;
        if (asr == 0xFF) continue;
        if (asr & 0x01) {   // DBR
            buf[idx] = wd_read(WD_DATA);
            idx++;
            if (idx >= len) break;
        }
    }

    print("  Got ", WHITE);
    print(binDec(idx), CYAN);
    print(" of ", WHITE);
    print(binDec(len), CYAN);
    print(" bytes.\n", WHITE);

    if (idx > 0) {
        int show = idx < 16 ? idx : 16;
        print("  Bytes: ", WHITE);
        for (int k = 0; k < show; k++) {
            print(binHexByte(buf[k]), CYAN);
            print(" ", WHITE);
        }
        print("\n", WHITE);
    }

    // Standard INQUIRY layout: byte 0 = peripheral device type, bytes 8-15
    // = vendor ID (8 ASCII chars), 16-31 = product ID (16 chars), 32-35 =
    // product revision (4 chars). Only meaningful if this was a real
    // INQUIRY response (len==36) and we got at least the vendor/product
    // fields.
    if (len == 36 && idx >= 36) {
        char field[17];
        int k;

        print("  Device type: $", WHITE);
        print(binHexByte(buf[0]), CYAN);
        print((buf[0] & 0x1F) == 0 ? " (Direct Access - disk)\n" : "\n", WHITE);

        for (k = 0; k < 8; k++) field[k] = (char)buf[8 + k];
        field[8] = 0;
        print("  Vendor:   \"", WHITE);
        print(field, GREEN);
        print("\"\n", WHITE);

        for (k = 0; k < 16; k++) field[k] = (char)buf[16 + k];
        field[16] = 0;
        print("  Product:  \"", WHITE);
        print(field, GREEN);
        print("\"\n", WHITE);

        for (k = 0; k < 4; k++) field[k] = (char)buf[32 + k];
        field[4] = 0;
        print("  Revision: \"", WHITE);
        print(field, GREEN);
        print("\"\n", WHITE);
    }

    if (idx < len && !globals->ScsiIrqPending) {
        print("  Stalled mid-DATA_IN.\n", RED);
        return 0xFF;
    }

    int j;
    for (j = 0; j < 4000; j++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("  Waited ", WHITE);
    print(binDec(j), CYAN);
    print(" iterations for the NEXT IRQ2 event.\n", WHITE);

    if (!globals->ScsiIrqPending) {
        print("  NO further interrupt - stuck here.\n", RED);
        return 0xFF;
    }
    globals->ScsiIrqPending = 0;
    print("  Next status: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), CYAN);
    print("\n", WHITE);
    return globals->ScsiIrqStatus;
}

// --- G1.1011 (status phase) and G1.1111 (message-in phase) --------------
// Step 7 (2026-07-14): both are single bytes, same SBT shape as the
// MSG_OUT/IDENTIFY send (checkMsgOutIdentify) - just reading instead of
// writing. Together with DATA_IN, this completes the whole INQUIRY
// transaction end to end: SELECT -> MSG_OUT -> CMD -> DATA_IN -> STATUS ->
// MSG_IN (command complete).
static uint8_t checkSingleByteRead(volatile struct GlobalVars *globals, const char *label, uint8_t *outByte)
{
    print("Reading ", WHITE);
    print((char *)label, WHITE);
    print(" via SBT...\n", WHITE);

    globals->ScsiIrqPending = 0;

    wd_write(WD_CONTROL, 0x00);
    wd_write(WD_COMMAND, WD_CMD_SBT | WDCMD_TRANSFER_INFO);

    int i;
    int gotDbr = 0;
    for (i = 0; i < 500000; i++) {
        uint8_t asr = *WD_AUX_STATUS;
        if (asr != 0xFF && (asr & 0x01)) { gotDbr = 1; break; }   // WD_ASR_DBR
    }
    if (!gotDbr) {
        print("  DBR never asserted.\n", RED);
        return 0xFF;
    }
    *outByte = wd_read(WD_DATA);
    print("  Got $", WHITE);
    print(binHexByte(*outByte), CYAN);
    print(" (DBR seen after ", WHITE);
    print(binDec(i), CYAN);
    print(" polls).\n", WHITE);

    int j;
    for (j = 0; j < 4000; j++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("  Waited ", WHITE);
    print(binDec(j), CYAN);
    print(" iterations for the NEXT IRQ2 event.\n", WHITE);

    if (!globals->ScsiIrqPending) {
        print("  NO further interrupt.\n", YELLOW);
        return 0xFF;
    }
    globals->ScsiIrqPending = 0;
    print("  Next status: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), CYAN);
    print("\n", WHITE);
    return globals->ScsiIrqStatus;
}

// G2.0000 ("Transfer paused with ack asrtd (msg in)"), translated directly
// from scsitask.asm's real ISR-level handler for this exact status group -
// verbatim:
//   PUTREG #wd_NEGATE_ACK,COMMAND    - issue a negate ack command
// The real driver's SCSIGetByte() reads the MSG_IN data byte, but doesn't by
// itself release ACK - after COMMAND COMPLETE the target still expects the
// initiator to negate ACK before it will drop to BUS FREE. Skipping this
// step leaves the SBIC mid-transaction, which is exactly why the historical
// symptom above (2026-07-14 real-hw run) showed a SECOND command's SELECT
// coming back as G3.0000 "an invalid command was issued" ($40) - the chip
// wasn't idle yet when we tried to issue a fresh SELECT_WITH_ATN.
static void negateAckAndWaitDisconnect(volatile struct GlobalVars *globals)
{
    print("  G2.0000 (paused, ACK asserted) - issuing NEGATE_ACK...\n", WHITE);
    globals->ScsiIrqPending = 0;
    wd_write(WD_COMMAND, WDCMD_NEGATE_ACK);

    int i;
    for (i = 0; i < 4000; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    if (!globals->ScsiIrqPending) {
        print("  NO disconnect interrupt after NEGATE_ACK.\n", RED);
        return;
    }
    globals->ScsiIrqPending = 0;
    print("  Post-negate-ack status: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), CYAN);
    print("\n", WHITE);
}

// Generic full-command runner: SELECT -> MSG_OUT(IDENTIFY) -> CMD ->
// DATA_IN -> STATUS -> MSG_IN, reusing every phase function above. Returns
// 1 only on a full, clean COMMAND COMPLETE; 0 if any phase didn't match
// what was expected (printed by each phase function already).
static int runScsiCommand(volatile struct GlobalVars *globals, uint8_t unit,
                           const uint8_t *cdb, int cdbLen, uint8_t *dataBuf, int dataLen)
{
    uint8_t st = checkSelect(globals, unit);
    if (!((st & 0xF0) == 0x80 && (st & 0x0F) == 0xE)) return 0;

    print("\n--- G1.1110: MSG_OUT phase requested ---\n", WHITE);
    st = checkMsgOutIdentify(globals, 0x80);   // IDENTIFY, LUN 0, no DiscPriv
    if (!((st & 0x0F) == 0xA && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) return 0;

    print("\n--- G1.1010: CMD phase requested ---\n", WHITE);
    st = checkCmdPhase(globals, cdb, cdbLen);
    if (!((st & 0x0F) == 0x9 && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) return 0;

    print("\n--- G1.1001: DATA_IN phase requested ---\n", WHITE);
    st = checkDataInPhase(globals, dataBuf, dataLen);
    if (!((st & 0x0F) == 0xB && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) return 0;

    print("\n--- G1.1011: STATUS phase requested ---\n", WHITE);
    uint8_t statusByte;
    st = checkSingleByteRead(globals, "STATUS byte", &statusByte);
    print("SCSI status byte: $", WHITE);
    print(binHexByte(statusByte), statusByte == 0 ? GREEN : YELLOW);
    print("\n", WHITE);
    if (!((st & 0x0F) == 0xF && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) return 0;

    print("\n--- G1.1111: MSG_IN phase requested ---\n", WHITE);
    uint8_t msgByte;
    st = checkSingleByteRead(globals, "MSG_IN byte", &msgByte);
    print("Message-in byte: $", WHITE);
    print(binHexByte(msgByte), msgByte == 0 ? GREEN : YELLOW);
    print(msgByte == 0 ? "  (COMMAND COMPLETE)\n" : "\n", msgByte == 0 ? GREEN : WHITE);

    if ((st & 0xF0) == 0x20 && (st & 0x0F) == 0x0) {
        negateAckAndWaitDisconnect(globals);
    }

    return (msgByte == 0);
}

// READ CAPACITY (10) response: bytes 0-3 = last valid LBA (big-endian),
// bytes 4-7 = block size in bytes (big-endian). Deliberately avoids
// multiply/divide (this freestanding build has no __mulsi3/__divsi3 -
// division already caused a real link error earlier this session) - uses
// shifts only, so it's only shown for the extremely common 512-byte-block
// case (blocks>>1 = KB, since 512B/1024B = 1/2).
static void printReadCapacity(const uint8_t *buf)
{
    uint32_t lastLba = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16)
                      | ((uint32_t)buf[2] << 8) | buf[3];
    uint32_t blockSize = ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16)
                        | ((uint32_t)buf[6] << 8) | buf[7];
    uint32_t totalBlocks = lastLba + 1;

    print("  Last LBA: ", WHITE);
    print(binDec(lastLba), CYAN);
    print("  Block size: ", WHITE);
    print(binDec(blockSize), CYAN);
    print(" bytes\n", WHITE);

    if (blockSize == 512) {
        uint32_t sizeKB = totalBlocks >> 1;
        uint32_t sizeMB = sizeKB >> 10;
        print("  Capacity: ", WHITE);
        print(binDec(sizeMB), GREEN);
        print(" MB (", WHITE);
        print(binDec(sizeKB), CYAN);
        print(" KB)\n", WHITE);
    } else {
        print("  (non-512 block size - not computing a friendly total)\n", YELLOW);
    }
}

// ============================================================================
// A4000T (NCR 53C710) - same "clean playground, small steps, translate the
// real driver exactly" methodology as the A3000/WD33C93 section above. No
// real A4000T hardware access this round - Amiberry only (scsi0_a4000t with
// a live SCSI2SD-equivalent hardfile). See [[project_diagrom_a4000t_scsi]]
// memory for the wider investigation (last known symptom: SELECT completes
// then the target immediately bails with SSTAT0F_MA|SSTAT0F_UDC before the
// phase loop ever runs - three script-construction hypotheses already ruled
// out there, so this round starts from chip INIT instead).
//
// Register offsets confirmed by byte-counting the official leaked
// a4091/ncr710.h `struct ncr710` layout, cross-checked against disk.c's
// already-independently-verified offsets (SIEN=0, SDID=1, SCNTL1=2,
// ISTAT=0x22, DSP=0x2C, DCNTL=0x38, DIEN=0x3A all match).
#define NCR_BASE ((volatile uint8_t *)0xDD0040)

#define NCR_SIEN     0x00
#define NCR_SCNTL1   0x02
#define NCR_SCNTL0   0x03
#define NCR_SXFER    0x06
#define NCR_SCID     0x07
#define NCR_SBCL     0x08
#define NCR_SSTAT2   0x0C
#define NCR_SSTAT0   0x0E
#define NCR_DSTAT    0x0F
#define NCR_CTEST0   0x17
#define NCR_CTEST7   0x18
#define NCR_ISTAT    0x22
#define NCR_DCNTL    0x38
#define NCR_DIEN     0x3A
#define NCR_DMODE    0x3B
#define NCR_DSP      0x2C

#define NCR_ISTATF_ABRT 0x80
#define NCR_ISTATF_RST  0x40
#define NCR_ISTAT_SIP   0x02
#define NCR_ISTAT_DIP   0x01

#define NCR_SCNTL0_EPG  0x04   // enable parity generation
#define NCR_SCNTL1_RST  0x08
#define NCR_SCNTL1_ESR  0x20   // enable selection/reselection
#define NCR_SXFER_DHP   0x80   // disable halt-on-parity-error (async)
#define NCR_CTEST0_ERF  0x04   // filter REQ/ACK
#define NCR_CTEST0_EAN  0x10   // enable active negation
#define NCR_CTEST0_BTD  0x40   // disable byte-to-byte timer
#define NCR_SBCL_SSCF0  0x01
#define NCR_SBCL_SSCF1  0x02
#define NCR_DMODE_FC2   0x20
#define NCR_DMODE_BL0   0x40
#define NCR_DMODE_BL1   0x80
#define NCR_DCNTL_EA    0x20
#define NCR_DCNTL_COM   0x01
// Clock-factor bits (DCNTLF_CF0/CF1) are picked by the real driver from its
// NCR_CLOCK_FREQ constant (50, shared by both A4091 and A4000T builds in
// ncr.c - not board-specific). Working through the driver's own #if ladder
// for exactly 50MHz resolves to DCNTL_VAL=0 (no CF bits) - so the existing
// EA|COM-only DCNTL write already matches this specific case; not a gap.

#define NCR_SIEN_STO 0x20   // enable select-timeout interrupt
// Real driver (ncr.c): "b->sien = (UBYTE) ~(SIENF_FCMP | SIENF_SEL);" -
// everything except Function-Complete(bit6) and Selected-as-target(bit4).
// STO alone (what this file used before) leaves UDC (Unexpected Disconnect,
// bit2) masked - Amiberry's own emulation logged "inf. loop with UDC
// masked" followed by a SIGSEGV crash once SELECT genuinely started
// reaching the real target (after the DSP byte-order fix), so this is a
// real requirement, not just closer-to-the-real-driver tidiness.
#define NCR_SIEN_ALL_BUT_FCMP_SEL 0xAF
#define NCR_DIEN_SIR 0x04   // enable "SCRIPTS INT instruction received"

#define NCR_OWN_SCSI_ID 7

// SCSI phase codes (bits 26-24 of Block Move / Transfer Control
// instructions, and the low 3 bits of SSTAT2) - same encoding disk.c's
// NCR_PHASE_* constants already use.
#define NCR_PHASE_MSG_OUT 0x6

static inline void ncr_write(uint8_t reg, uint8_t val) { NCR_BASE[reg] = val; }
static inline uint8_t ncr_read(uint8_t reg) { return NCR_BASE[reg]; }
// FOUND 2026-07-14 by reading Amiberry's own emulation source directly
// (src/qemuvga/lsi53c710.cpp, the actual model behind scsi0_a4000t): DSP is
// emulated as four SEPARATELY byte-addressed registers (offsets +0..+3 =
// DSP[0:7]..DSP[24:31]), and script execution is kicked off ONLY by the
// write that lands on the HIGHEST offset (+3 = DSP[24:31]). Amiberry's
// generic longword-write dispatcher decomposes a plain 32-bit CPU store
// into byte-pokes in address order +3,+2,+1,+0 (i.e. the VALUE's LSB goes
// to the LOWEST address) - so a naive `*(uint32_t*)ptr = val` write makes
// the trigger byte (+3) land FIRST, with only one byte of the real address
// in place and the rest still holding a leftover/garbage value. Execution
// then starts from that garbage address before the rest of the pointer is
// ever written - explaining the wildly inconsistent results seen all
// session (including the historical $84, likely also just whatever
// garbage happened to be at a near-random address, not a real response).
// This matches the leaked ncr710.h's own cryptic note: "internal scripts
// use little-endian addresses" - DSP genuinely needs byte-order-reversed
// writes, not a natural big-endian 32-bit store. Fix: four explicit byte
// writes, low-address-gets-LSB, with the real trigger byte (CPU address
// +3) written LAST so the full, correct address is already in place.
static inline void ncr_lwrite(uint8_t off, uint32_t val)
{
    volatile uint8_t *p = (uint8_t *)NCR_BASE + off;
    p[0] = (uint8_t)(val & 0xFF);
    p[1] = (uint8_t)((val >> 8) & 0xFF);
    p[2] = (uint8_t)((val >> 16) & 0xFF);
    p[3] = (uint8_t)((val >> 24) & 0xFF);   // trigger byte - must be written last
}
static inline uint32_t ncr_lread(uint8_t off)
{
    return *(volatile uint32_t *)((uint8_t *)NCR_BASE + off);
}

// Step 1 (setup + chip check): translated directly from ncr.c's init
// sequence, the part BEFORE any SELECT is attempted - considerably more
// thorough than disk.c's current detectA4000TSCSI() (which only does
// SCID/ESR/SXFER plus the mandatory first DCNTL write). New steps added
// here that disk.c's production code doesn't have yet: the ISTAT-based
// ABRT+RST pulse pair BEFORE the SCNTL1 bus reset, CTEST0 (byte-to-byte
// timer off / active negation / REQ-ACK filter), CTEST7 (disable burst bus
// mode), a real wait-for-pending-ints-to-clear loop (reading SSTAT2 each
// iteration is what actually clears them - same "reading IS the ack"
// pattern as A3000's SASR-before-SCSI_STATUS rule), SCNTL0 (parity
// generation), a genuine ~250ms settle wait AFTER the reset pulse (real
// driver: "now must wait 250ms before trying to use the bus"), SBCL (sync
// clock factor, written even in async mode), and DMODE (DMA burst length +
// bus function codes) - this last one is the standout candidate for
// anything bus-level, since function codes affect how the chip's own
// bus-master cycles present themselves.
// BISECTION 2026-07-14: even the bare SELECT+INT test (no JUMP, no MOVE -
// the exact historical shape that used to reliably get $84 MA|UDC against
// this live device) now gets a full timeout with NO interrupt at all under
// this fuller init. Since nothing else changed, one of the NEW writes below
// is the regression. `level` lets DebugCode() bisect without rewriting this
// function each time:
//   0 = old minimal init only (matches disk.c's current detectA4000TSCSI:
//       DCNTL, plain SCNTL1 RST toggle with no settle, SCID/ESR/SXFER)
//   1 = level 0 + CTEST0/CTEST7
//   2 = level 1 + SCNTL0 (parity generation)
//   3 = level 2 + SBCL/DMODE
//   4 = level 3 + the ABRT/RST pre-pulse, pending-int-clear loop, and the
//       ~250ms post-reset settle wait (the full sequence as originally
//       written)
// Bisection dial - change this single value between test rebuilds, see the
// level-meaning comment above ncrRealInit().
#define NCR_INIT_BISECT_LEVEL 4

static int ncrRealInit(int level)
{
    // CRITICAL (real driver, verbatim): "must set EA in DCNTL before adding
    // the int server. This is because the first access will never end
    // unless the chip is set to link STERM and SLAC internally." The very
    // FIRST bus access after power-on must be this exact write - never
    // reorder it after a presence/float check.
    ncr_write(NCR_DCNTL, NCR_DCNTL_EA | NCR_DCNTL_COM);

    if (ncr_read(NCR_ISTAT) == 0xFF) {
        print("  Bus float - chip not responding.\n", RED);
        return 0;
    }
    print("  Chip present. (bisect level ", GREEN);
    print(binDec(level), CYAN);
    print(")\n", GREEN);

    int i;
    if (level >= 4) {
        ncr_write(NCR_ISTAT, NCR_ISTATF_ABRT);
        waitShort();
        ncr_write(NCR_ISTAT, NCR_ISTATF_RST);
        ncr_write(NCR_ISTAT, 0x00);
        waitShort();
    }

    if (level >= 1) {
        ncr_write(NCR_CTEST7, ncr_read(NCR_CTEST7) | 0x80);   // disable burst bus mode
        ncr_write(NCR_CTEST0, NCR_CTEST0_BTD | NCR_CTEST0_EAN | NCR_CTEST0_ERF);
    }

    if (level >= 4) {
        for (i = 0; i < 100; i++) {
            if (!(ncr_read(NCR_ISTAT) & (NCR_ISTAT_SIP | NCR_ISTAT_DIP))) break;
            (void)ncr_read(NCR_SSTAT2);   // reading this is what actually clears the pending state
            waitShort();
        }
        print("  Cleared pending ints after ", WHITE);
        print(binDec(i), CYAN);
        print(" iterations.\n", WHITE);
    }

    if (level >= 2) {
        ncr_write(NCR_SCNTL0, ncr_read(NCR_SCNTL0) | NCR_SCNTL0_EPG);
    }

    ncr_write(NCR_SCNTL1, NCR_SCNTL1_RST);
    waitShort();
    ncr_write(NCR_SCNTL1, 0x00);
    if (level >= 4) {
        for (i = 0; i < 500; i++) waitShort();   // ~320ms settle - real driver waits 250ms before touching the bus
    }

    ncr_write(NCR_SCID, (uint8_t)(1U << NCR_OWN_SCSI_ID));
    ncr_write(NCR_SCNTL1, ncr_read(NCR_SCNTL1) | NCR_SCNTL1_ESR);
    ncr_write(NCR_SXFER, NCR_SXFER_DHP);

    if (level >= 3) {
        ncr_write(NCR_SBCL, NCR_SBCL_SSCF1 | NCR_SBCL_SSCF0);
        ncr_write(NCR_DMODE, NCR_DMODE_BL1 | NCR_DMODE_BL0 | NCR_DMODE_FC2);
    }

    // Register readback (this session) showed DCNTL reading back $00 here,
    // not the $21 (EA|COM) written at the very top of this function - the
    // SCNTL1 RST pulse (and/or the ISTAT ABRT+RST pair) clears it. Per the
    // datasheet's own warning, EA must be set before the chip's autonomous
    // SCRIPTS engine is ever kicked off (SELECT/JUMP/etc.) or "the first
    // access will never end" - exactly the total-silence symptom seen
    // testing SELECT against unit 0. Re-assert it here, as the LAST write
    // before returning, so it's still set when SELECT actually runs.
    ncr_write(NCR_DCNTL, NCR_DCNTL_EA | NCR_DCNTL_COM);

    print("  Init applied for this level.\n", GREEN);
    return 1;
}

// Step 2 (setup IRQ): same PORTS/IRQ2 shared-line pattern as A3000's
// DebugPortsIRQ, but reading NCR_SSTAT0/DSTAT instead of the WD33C93's
// SCSI_STATUS. A separate handler/enable/disable trio, not shared with the
// WD33C93 one above, but the same globals->ScsiIrqPending/Status/Count
// fields (only one SCSI test runs at a time in this ROM).
// BUG FOUND (user pushed back on "sure IRQ works??" - rightly so): this
// used to read SSTAT0 and set ScsiIrqPending on ANY IRQ2 assertion, with no
// check that the NCR chip itself actually has a pending SCSI-side
// interrupt. A genuine select timeout takes a hardware-fixed 250ms (per
// the datasheet: "does not respond within the 250 ms timeout period" - not
// configurable, not instant) - so the "Waited 0 iterations" STO result
// seen all session was almost certainly a stray/leftover IRQ2 event (this
// line is shared with other Amiga hardware) being misread as a real,
// freshly-generated timeout. Fixed to gate on ISTAT's SIP/DIP bits first,
// mirroring disk.c's already-correct Ncr710PortsIRQ exactly.
__interrupt void DebugPortsIRQ_NCR(VARS)
{
    uint16_t irq = custom->intreqr;
    if (irq & SCSI_PORTS_BIT) {
        uint8_t istat = ncr_read(NCR_ISTAT);
        if (istat != 0xFF && (istat & (NCR_ISTAT_SIP | NCR_ISTAT_DIP))) {
            globals->ScsiIrqStatus = ncr_read(NCR_SSTAT0);
            (void)ncr_read(NCR_DSTAT);   // reading this clears DIP
            globals->ScsiIrqPending = 1;
            globals->ScsiIrqCount++;
        }
    }
    custom->intreq = SCSI_PORTS_BIT;
    custom->intreq = SCSI_PORTS_BIT;
}

static void ncrDebugIrqEnable(volatile struct GlobalVars *globals)
{
    globals->ScsiIrqPending = 0;
    globals->ScsiIrqStatus  = 0;
    globals->ScsiIrqCount   = 0;
    *(volatile APTR *) + 0x68 = DebugPortsIRQ_NCR;
    custom->intreq = SCSI_PORTS_BIT;
    custom->intreq = SCSI_PORTS_BIT;
    custom->intena = 0xC000 | SCSI_PORTS_BIT;
    custom->intena = 0xC000 | SCSI_PORTS_BIT;
    ncr_write(NCR_SIEN, NCR_SIEN_ALL_BUT_FCMP_SEL);
    ncr_write(NCR_DIEN, NCR_DIEN_SIR);
    debug_set_sr(0x2000);
}

static void ncrDebugIrqDisable(void)
{
    ncr_write(NCR_SIEN, 0);
    ncr_write(NCR_DIEN, 0);
    custom->intena = 0x7fff;
    custom->intreq = 0x7fff;
    *(volatile APTR *) + 0x68 = RTEcode;
}

// Step 3 (sanity check): the 53C710 has no cheap "reset always completes
// with a status interrupt" event the way the WD33C93 does, so the closest
// safe equivalent is issuing a real SELECT to a deliberately-empty ID (6 -
// nothing lives there in the current Amiberry profile) and confirming the
// chip's own select-timeout (SIEN_STO) genuinely reaches our ISR. This is
// pure plumbing verification - unit 0 (the live device) isn't touched yet.
#define NCR_SCRIPT_SELECT_ATN 0x41000000UL   // class=01, opcode=SELECT(000), ATN=1
#define NCR_SCRIPT_INT        0x98000000UL   // class=10, opcode=INT(011)

static void checkNcrIRQ2(volatile struct GlobalVars *globals)
{
    print("Issuing SELECT to ID 6 (expected empty) to confirm IRQ2 fires...\n", WHITE);

    uint32_t script[4];
    script[0] = NCR_SCRIPT_SELECT_ATN | ((1UL << 6) << 16);
    script[1] = 0;
    script[2] = NCR_SCRIPT_INT;
    script[3] = 0;

    globals->ScsiIrqPending = 0;
    ncr_lwrite(NCR_DSP, (uint32_t)(uintptr_t)&script[0]);

    int i;
    for (i = 0; i < 4000; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("Waited ", WHITE);
    print(binDec(i), CYAN);
    print(" iterations for a result.\n", WHITE);

    if (!globals->ScsiIrqPending) {
        print("NO interrupt at all - IRQ2 plumbing problem.\n", RED);
        return;
    }
    globals->ScsiIrqPending = 0;
    print("SSTAT0: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), globals->ScsiIrqStatus & NCR_SIEN_STO ? GREEN : YELLOW);
    print(globals->ScsiIrqStatus & NCR_SIEN_STO ? "  (STO - select timeout, as expected)\n" : "\n",
          WHITE);
}

// Step 4 (the real experiment): SELECT unit 0 - the live device - using an
// ATOMIC, single on-chip script: SELECT+ATN immediately followed by an
// on-chip phase check (JUMP ..., IF MSG_OUT) BEFORE ever attempting the
// identify MOVE, with an explicit fallback INT for any other phase. This
// is the exact shape of the real driver's script, confirmed two ways this
// round:
//   1) Linux's 53c700.scr (fetched via WebFetch): "SELECT ATN id, Reselect
//      / JUMP Finish, WHEN STATUS / JUMP SendIdentifyMsg, IF MSG_OUT / INT
//      NOT_MSG_OUT_AFTER_SELECTION" - i.e. select, THEN branch on phase,
//      THEN (only if MSG_OUT) send identify. Never an unconditional MOVE.
//   2) The real NCR 53C710 Data Manual's own Block Move description:
//      "If the SCSI phase bits do not match [the instruction's phase
//      field]... phase mismatch interrupt and the command is not
//      executed" - i.e. a bare MOVE chained straight after SELECT is a
//      genuine, real risk if the target isn't in exactly that phase yet,
//      which is exactly why real scripts branch first instead of assuming.
// Every previous attempt against this live device (both the split
// select+move across two CPU-driven scripts, AND the "most correct so
// far" atomic SELECT+MOVE-identify with no phase check at all - see
// [[project_diagrom_a4000t_scsi]] memory) skipped this branch-and-fallback
// structure entirely. This is the first attempt that matches the real
// driver's on-chip handling exactly.
//
// Bit encodings below were derived directly from the NCR 53C710 Data
// Manual's instruction-format figures (Transfer Control: Figure 5-4,
// Block Move: Figure 5-1) and cross-checked against the real, compiled
// a4091/a4000t driver bytes in nrc_script.c (e.g. 0x0F000002 = MOVE
// opcode-bit-set + phase=MSG_IN(111) + count=2, confirming the "opcode
// bit27 must be 1 for a Block Move in INITIATOR mode" rule - disk.c's
// current ncr_buildMove() does NOT set this bit, a previously-undiscovered
// bug that's simply never been reached yet, since the live-device SELECT
// has always bailed before the phase-dispatch loop ever ran).
#define NCR_SCRIPT_JUMP_IF_MSGOUT 0x860A0000UL   // JUMP addr, IF MSG_OUT
#define NCR_SCRIPT_JUMP_ALWAYS    0x80000000UL   // JUMP addr (compare disabled - always taken)
#define NCR_MOVE_OPCODE_BIT       (1UL << 27)    // mandatory for initiator-mode Block Move

// Isolation step 0 (added after finding the real crash signature - a host
// SIGSEGV accessing address 0xc, i.e. a null-pointer-plus-offset deref - in
// Amiberry's own src/qemuvga/lsi53c710.cpp): the SELECT opcode handler there
// (`case 0: /* Select */`) transitions straight to PHASE_MO (message-out)
// the instant it sees our script's ATN bit set, with no MOVE ever having run
// yet. Every crashing test so far (this file's checkNcrSelectBareUnit0/
// checkNcrSelectUnit0, disk.c's own scanA4000TSCSI) sets ATN. This variant
// clears it (plain 0x40 opcode byte, no ATN) so SELECT lands in PHASE_CMD
// instead - isolates whether the MSG_OUT phase-entry path itself is what's
// crashing Amiberry, independent of anything else about our script shape.
#define NCR_SCRIPT_SELECT_NOATN 0x40000000UL   // class=01,opcode=SELECT(000),ATN=0

static void checkNcrSelectNoAtnUnit0(volatile struct GlobalVars *globals)
{
    print("Issuing bare SELECT+INT, NO ATN (-> PHASE_CMD not MSG_OUT) to unit 0...\n", WHITE);

    uint32_t script[4];
    script[0] = NCR_SCRIPT_SELECT_NOATN | ((1UL << 0) << 16);
    script[1] = (uint32_t)(uintptr_t)&script[2];   // reselect fallback -> the same INT below
    script[2] = NCR_SCRIPT_INT;
    script[3] = 0;

    globals->ScsiIrqPending = 0;
    ncr_lwrite(NCR_DSP, (uint32_t)(uintptr_t)&script[0]);

    int i;
    for (i = 0; i < 4000; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("Waited ", WHITE);
    print(binDec(i), CYAN);
    print(" iterations. ", WHITE);
    if (!globals->ScsiIrqPending) {
        print("NO interrupt at all.\n", RED);
        return;
    }
    globals->ScsiIrqPending = 0;
    print("SSTAT0: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), WHITE);
    print("\n", WHITE);
}

// Isolation step A: bare SELECT+INT against unit 0 (the exact shape the
// historical split-script scanner uses, which got $84 MA|UDC) - but run
// under THIS session's fuller chip init (CTEST0/CTEST7/SCNTL0/SBCL/DMODE),
// which has never been tested against the live device before. Establishes
// whether the improved init changes the baseline symptom at all, before
// adding any JUMP/MOVE complexity on top of it.
static void checkNcrSelectBareUnit0(volatile struct GlobalVars *globals)
{
    print("Issuing bare SELECT+INT (historical shape) to unit 0...\n", WHITE);

    uint32_t script[4];
    script[0] = NCR_SCRIPT_SELECT_ATN | ((1UL << 0) << 16);
    script[1] = (uint32_t)(uintptr_t)&script[2];   // reselect fallback -> the same INT below
    script[2] = NCR_SCRIPT_INT;
    script[3] = 0;

    globals->ScsiIrqPending = 0;
    ncr_lwrite(NCR_DSP, (uint32_t)(uintptr_t)&script[0]);

    int i;
    for (i = 0; i < 4000; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("Waited ", WHITE);
    print(binDec(i), CYAN);
    print(" iterations. ", WHITE);
    if (!globals->ScsiIrqPending) {
        print("NO interrupt at all.\n", RED);
        return;
    }
    globals->ScsiIrqPending = 0;
    print("SSTAT0: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), WHITE);
    print("\n", WHITE);
}

// Isolation step B: SELECT + an unconditional ("always taken", compare
// disabled entirely) JUMP, straight to an INT - no phase compare, no MOVE.
// Tests whether merely adding ANY Transfer Control instruction after a
// live-target SELECT already breaks things, independent of phase-matching
// or Block Move encoding.
static void checkNcrSelectJumpOnly(volatile struct GlobalVars *globals)
{
    print("Issuing SELECT + unconditional JUMP (no MOVE) to unit 0...\n", WHITE);

    uint32_t script[8];
    script[0] = NCR_SCRIPT_SELECT_ATN | ((1UL << 0) << 16);
    script[1] = (uint32_t)(uintptr_t)&script[6];   // reselect fallback - real handler
    script[2] = NCR_SCRIPT_JUMP_ALWAYS;
    script[3] = (uint32_t)(uintptr_t)&script[4];   // jump target: the INT right after
    script[4] = NCR_SCRIPT_INT;                     // success checkpoint (jump taken)
    script[5] = 0;
    script[6] = NCR_SCRIPT_INT;                     // reselect-fallback checkpoint
    script[7] = 0;

    globals->ScsiIrqPending = 0;
    ncr_lwrite(NCR_DSP, (uint32_t)(uintptr_t)&script[0]);

    int i;
    for (i = 0; i < 4000; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("Waited ", WHITE);
    print(binDec(i), CYAN);
    print(" iterations. ", WHITE);
    if (!globals->ScsiIrqPending) {
        print("NO interrupt at all.\n", RED);
        return;
    }
    globals->ScsiIrqPending = 0;
    uint32_t dsp = ncr_lread(NCR_DSP);
    print("SSTAT0: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), WHITE);
    print(dsp == (uint32_t)(uintptr_t)&script[5] ? "  (JUMP taken OK)\n" :
          dsp == (uint32_t)(uintptr_t)&script[7] ? "  (reselect fallback hit)\n" :
          "  (unexpected DSP)\n", WHITE);
}

// Cross-checked 2026-07-14 against three independent real drivers - Linux's
// 53c700.scr ("SELECT ATN Device_ID, Reselect" -> a real Reselect: handler),
// NetBSD's amiga siop_script.ss ("SELECT ATN FROM ds_Device, REL(reselect)"
// -> a real reselect: handler that does WAIT RESELECT + validates the
// incoming message), and the leaked AmigaOS a4091 driver (same shape) - ALL
// THREE point the SELECT instruction's second/fallback operand (used only
// if reselected before winning arbitration) at a REAL handler, never a bare
// zero. Our first attempt at this atomic script left it as literal 0 and
// got a full timeout with NO interrupt at all - consistent with the chip
// jumping to fetch its next instruction from address $00000000 and hanging
// there if that rare edge case is ever hit. Fixed: now points at a real,
// distinguishable INT instruction instead (script[10]) - we don't need full
// reselection handling for this single-shot diagnostic, just a safe,
// informative landing spot instead of null.
static void checkNcrSelectUnit0(volatile struct GlobalVars *globals)
{
    print("Issuing atomic SELECT+phase-check+MOVE(identify) to unit 0...\n", WHITE);

    uint8_t identify = 0x80;   // IDENTIFY, LUN 0, no DiscPriv
    uint32_t script[12];
    script[0]  = NCR_SCRIPT_SELECT_ATN | ((1UL << 0) << 16);
    script[1]  = (uint32_t)(uintptr_t)&script[10];   // reselect fallback - a real handler now, not 0
    script[2]  = NCR_SCRIPT_JUMP_IF_MSGOUT;
    script[3]  = (uint32_t)(uintptr_t)&script[6];    // jump target: the MOVE below
    script[4]  = NCR_SCRIPT_INT;                      // fallback: phase wasn't MSG_OUT
    script[5]  = 0;
    script[6]  = NCR_MOVE_OPCODE_BIT | ((uint32_t)NCR_PHASE_MSG_OUT << 24) | 1UL;
    script[7]  = (uint32_t)(uintptr_t)&identify;
    script[8]  = NCR_SCRIPT_INT;                      // success: identify byte sent
    script[9]  = 0;
    script[10] = NCR_SCRIPT_INT;                      // reselected before winning arbitration
    script[11] = 0;

    globals->ScsiIrqPending = 0;
    ncr_lwrite(NCR_DSP, (uint32_t)(uintptr_t)&script[0]);

    int i;
    for (i = 0; i < 4000; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }
    print("Waited ", WHITE);
    print(binDec(i), CYAN);
    print(" iterations for a result.\n", WHITE);

    if (!globals->ScsiIrqPending) {
        print("NO interrupt at all.\n", RED);
        return;
    }
    globals->ScsiIrqPending = 0;
    uint32_t dsp = ncr_lread(NCR_DSP);
    print("SSTAT0: $", WHITE);
    print(binHexByte(globals->ScsiIrqStatus), WHITE);
    print("  DSP: $", WHITE);
    print(binHexWord((uint16_t)(dsp >> 16)), CYAN);
    print(binHexWord((uint16_t)dsp), CYAN);
    print("\n", WHITE);

    if (dsp == (uint32_t)(uintptr_t)&script[5]) {
        print("-> Fallback branch taken: phase was NOT MSG_OUT after SELECT.\n", YELLOW);
    } else if (dsp == (uint32_t)(uintptr_t)&script[9]) {
        print("-> MSG_OUT branch taken: identify byte was sent successfully!\n", GREEN);
    } else if (dsp == (uint32_t)(uintptr_t)&script[11]) {
        print("-> Reselected before winning arbitration (unexpected for this test).\n", YELLOW);
    } else {
        print("-> DSP doesn't match any expected checkpoint (unexpected path).\n", RED);
    }
}

// BUG FOUND 2026-07-14: the A3000 section below (wd_write() et al) does
// `*WD_ADDR_REG = reg` - a 32-BIT write to $DD0040. On real A3000 hardware
// that's the SDMAC's legitimate "select register N" mechanism, but $DD0040
// is ALSO NCR_BASE for the A4000T section further down - so every WD
// register-select write there was silently smashing FOUR of the NCR
// chip's own registers at once (SIEN/SDID/SCNTL1/SCNTL0, bytes 0-3 of that
// same longword) with garbage WD register indices, run UNCONDITIONALLY
// before the A4000T section ever got a chance to init the chip. This is
// what caused a run of confusing "total silence, even at bisect level 0"
// results tonight - nothing wrong with the SELECT/JUMP/MOVE encoding or the
// chip-init sequence, just two sections of this shared debug playground
// stepping on each other's identical physical address range. Gate them so
// only one runs per build - flip this to 0 to go back to A3000-only work.
#define DEBUGCODE_TARGET_A4000T 1

void DebugCode(void)
{
    print("\n=== DebugCode ===\n\n", WHITE);

#if !DEBUGCODE_TARGET_A4000T
    print("--- Ramsey ---\n", WHITE);
    checkRamsey();

    print("\n--- WD33C93 ---\n", WHITE);
    checkWD33C93();

    print("\n--- IRQ2 ---\n", WHITE);
    checkIRQ2();

    print("\n--- Real-driver init ---\n", WHITE);
    volatile struct GlobalVars *globals = debugGlobals();
    debugIrqEnable(globals);
    if (wdRealInit(globals)) {
        print("\n=== INQUIRY (unit 0) ===\n", WHITE);
        uint8_t inquiryData[36];
        static const uint8_t inquiryCdb[6] = { 0x12, 0x00, 0x00, 0x00, 36, 0x00 };
        if (runScsiCommand(globals, 0, inquiryCdb, 6, inquiryData, 36)) {
            print("\n*** INQUIRY TRANSACTION COMPLETE ***\n", GREEN);
        }

        print("\n=== READ CAPACITY (unit 0) ===\n", WHITE);
        uint8_t capData[8];
        static const uint8_t readCapCdb[10] = { 0x25, 0,0,0,0,0,0,0,0, 0 };
        if (runScsiCommand(globals, 0, readCapCdb, 10, capData, 8)) {
            print("\n*** READ CAPACITY TRANSACTION COMPLETE ***\n", GREEN);
            printReadCapacity(capData);
        }
    }
    debugIrqDisable();
    print("\nTotal IRQ2 events this run: ", WHITE);
    print(binDec(globals->ScsiIrqCount), CYAN);
    print("\n", WHITE);
#else
    volatile struct GlobalVars *globals = debugGlobals();
#endif

    print("\n\n=== A4000T (NCR 53C710) ===\n\n", WHITE);
    print("--- Chip init ---\n", WHITE);
    if (ncrRealInit(NCR_INIT_BISECT_LEVEL)) {
        print("\n--- IRQ2 setup ---\n", WHITE);
        ncrDebugIrqEnable(globals);
        checkNcrIRQ2(globals);

        print("\n--- Register readback (confirm, don't assume) ---\n", WHITE);
        print("SCID=$", WHITE);   print(binHexByte(ncr_read(NCR_SCID)), CYAN);
        print(" SCNTL1=$", WHITE); print(binHexByte(ncr_read(NCR_SCNTL1)), CYAN);
        print(" DCNTL=$", WHITE); print(binHexByte(ncr_read(NCR_DCNTL)), CYAN);
        print(" SXFER=$", WHITE); print(binHexByte(ncr_read(NCR_SXFER)), CYAN);
        print(" ISTAT=$", WHITE); print(binHexByte(ncr_read(NCR_ISTAT)), CYAN);
        print(" SIEN=$", WHITE); print(binHexByte(ncr_read(NCR_SIEN)), CYAN);
        print("\n", WHITE);

        // Reordered 2026-07-14: run simplest-first so a crash pinpoints which
        // ingredient (bare SELECT vs +JUMP vs +JUMP+MOVE) actually triggers
        // it, instead of always crashing on the most complex test before the
        // simpler ones ever get a turn. NO-ATN goes first of all: real crash
        // evidence (host SIGSEGV in lsi53c710.cpp) points at the ATN-forced
        // PHASE_MO transition inside Amiberry's own SELECT handler, so this
        // isolates that specific path before anything else runs.
        print("\n--- SELECT unit 0 (bare, NO ATN -> PHASE_CMD) ---\n", WHITE);
        checkNcrSelectNoAtnUnit0(globals);

        print("\n--- SELECT unit 0 (bare, historical shape) ---\n", WHITE);
        checkNcrSelectBareUnit0(globals);

        print("\n--- SELECT unit 0 (+ unconditional JUMP, no MOVE) ---\n", WHITE);
        checkNcrSelectJumpOnly(globals);

        print("\n--- SELECT unit 0 (atomic, phase-checked) ---\n", WHITE);
        checkNcrSelectUnit0(globals);

        ncrDebugIrqDisable();
        print("\nTotal NCR IRQ2 events this run: ", WHITE);
        print(binDec(globals->ScsiIrqCount), CYAN);
        print("\n", WHITE);
    }

    // Direct A/B check: does disk.c's actual PRODUCTION scanner still get
    // the historical $84 (MA|UDC) against this exact live device, in this
    // exact same boot, right after every one of my own tests above got
    // total silence? Settles whether the issue is in my debug harness
    // specifically or something environmental. (detectA4000TSCSI/
    // scanA4000TSCSI temporarily de-static'd in disk.c for this call.)
    print("\n\n=== disk.c production scanA4000TSCSI() (direct A/B) ===\n\n", WHITE);
    if (detectA4000TSCSI()) {
        scanA4000TSCSI();
    } else {
        print("detectA4000TSCSI() returned false.\n", RED);
    }

    print("\n=== DebugCode done ===\n", WHITE);
    print("Press any key/mouse to continue to the main menu...\n", WHITE);
    WaitButton();
}
