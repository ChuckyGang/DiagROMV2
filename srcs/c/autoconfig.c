// DiagROM autoconfig test — C rewrite of srcs/asm/autoconfig.s (Zorro II/III
// expansion scan + configure). CURRENT STAGE: full Z2-base configure engine
// (step 4): assign/shutup writes, prompts, verification, board list.
//
// Ground rules for this file (agreed 2026-08-07, small-step rewrite):
// - Mimic REAL Kickstart expansion.library behavior (derived from the v40
//   config.asm/utils.asm sources: slot-map Z2 allocator with per-size
//   alignment, small boards to $E9xxxx by SIZE, aligned-bump Z3 from
//   $40000000, shutup/CLOGGED failure policy, 11x stability re-read) — NOT
//   the old asm's "I think it works like this" approximations.
// - Diagnostic attitude: print what the hardware actually returns (raw bytes
//   next to decodes), name every failed check with the offending value,
//   never assume a write took or that anything works, bounded waits only.
// - The old asm (AutoConfig / AutoConfigDetail / DoAutoconfig) stays intact
//   and keeps serving the memtest chain and the TF diag until this replaces
//   it. New entries live at "Other tests" 6 (Detailed) / 7 (Automatic).
// - Board-list records are byte-identical to the old asm's (TF diag walks
//   them with hardcoded offsets): manu.w, serial-HIGH-word.w, er_Type.b,
//   er_Flags.b, start.l, end.l — 14 bytes. ONE deliberate deviation: only
//   ASSIGNED boards are recorded (the old asm also recorded boards that the
//   user then shut up, which left phantom entries).
//
// Step-5/6 status: scans BOTH config spaces exactly as v40 ConfigChain does
// on big-box ROMs (verified in the real source 2026-08-08: A3000_ROMS
// builds call ConfigChain($FF000000), which sets primary=$FF000000 /
// alternate=$E80000 in a2/a4 and re-tries BOTH on every pass; the chain
// only ends when both read as non-boards; non-big-box ROMs scan $E80000
// alone). Our runtime equivalent of their build-time flag is the
// acZ3ConfigSpaceUsable() triple gate. Z3-typed boards answering at
// $E80000 (Kickstart treats ANY non-$C0 type as Zorro III — faithfully
// kept, warning printed when the type bits are actually invalid) configure
// through the same classification either way; both UAE z3_autoconfig modes
// confirmed live with a GVP Z2 + Fastlane/Z3-RAM mix, including a genuinely
// interleaved E8,E8,FF,FF,E8 chain. Step 6: every space's first touch per
// pass goes through berr.s's vector-2 guard, with Fat Gary's timeout armed
// to BERR mode first (real registers $DE0000/$DE0001 from the v40/mmu13
// sources, set to $80/$00 exactly like Kickstart) — so the two events that
// yellow-screen Kickstart on a big box (A3000 missing-daughterboard /DTACK
// float at $E80000, Buster faults at $FF000000) become printed verdicts,
// with Gary's timed-out flag distinguishing "nothing answered" from
// "BERR actively asserted". BERR paths need real-hardware calibration
// (pull the AA3000+ daughterboard); emulators don't raise these.

#include "generic.h"

// disk.c's positive A600/A1200 Gayle identification (three identical
// protocol passes of the $DE1000 ID register, safe to run on any machine).
// Reused here to gate the $FF000000 probe off machines whose bus never
// terminates an access up there.
extern int isGayleMachine(void);

// ---- ExpansionRom logical byte offsets (16-byte config ROM) ---------------
#define ER_TYPE         0
#define ER_PRODUCT      1
#define ER_FLAGS        2
#define ER_RESERVED03   3
#define ER_MANUFACTURER 4    // word, bytes 4-5
#define ER_SERIAL       6    // longword, bytes 6-9
#define ER_INITDIAGVEC  10   // word, bytes 10-11
#define ER_SIZEOF       16

// ExpansionControl logical byte offsets (registers after the config ROM)
#define EC_Z3_HIGHBASE  0x11 // word register: Z3 base address bits 31-16
#define EC_BASEADDRESS  0x12 // Z2 base: address bits 23-16, one byte
#define EC_SHUTUP       0x13 // any write disables the board until reset

// er_Type bits (configregs.i)
#define ERT_TYPEMASK    0xC0
#define ERT_ZORROII     0xC0
#define ERT_ZORROIII    0x80
#define ERTB_MEMLIST    5    // link RAM into free memory list
#define ERTB_DIAGVALID  4    // boot/diag ROM vector valid
#define ERTB_CHAINED    3    // next config space is part of the same card
#define ERT_MEMMASK     0x07

// er_Flags bits (configregs.i)
#define ERFB_MEMSPACE   7    // "wants 8MB space" (never implemented by OS)
#define ERFB_NOSHUTUP   6    // board cannot be shut up
#define ERFB_EXTENDED   5    // Z3: use extended size table; Z2: must be 0
#define ERFB_ZORRO_III  4    // Z3: must be 1;                Z2: must be 0
#define ERF_Z3_SSMASK   0x0F // Z3 sub-size; Z2: must be 0

#define E_EXPANSIONBASE ((volatile uint8_t *)0x00E80000)
#define E_Z3CONFIGBASE  ((volatile uint8_t *)0xFF000000UL)

// Ramsey version register (big-box A3000/A4000 memory controller):
// $0D = Ramsey-04, $0F = Ramsey-07.
#define RAMSEY_VERSION  ((volatile uint8_t *)0x00DE0043UL)

// Fat Gary bus-timeout control block — REAL register meanings from the v40
// sources (mmu13.asm; UAE's loose $DExxxx decode differs). One bit-7 flag
// per byte-sized register:
//   $DE0000 write: $80 = BERR after 250ms on timeout, $00 = fake DSACK
//           after 9us (reset default — dead probes "succeed" with garbage!)
//           read:  bit 7 = a cycle timed out
//   $DE0001 write: bit 7 = 0 enables the timeout machinery
// Kickstart's big-box builds set $80/$00 before configuring (config.asm
// "Set BERR (250 mSecs)") — we do the same, or a dead expansion bus would
// hang or lie instead of raising a catchable bus error.
#define GARY_TIMEOUT_MODE   ((volatile uint8_t *)0x00DE0000UL)
#define GARY_TIMEOUT_ENABLE ((volatile uint8_t *)0x00DE0001UL)

// berr.s: guarded accesses via a temporary vector-2 handler (SP-reload
// recovery, CPU-agnostic). Return 0 = cycle completed, 1 = bus error
// caught. DiagROM's first bus-error recovery of any kind.
extern int acBerrProbeRead(volatile uint8_t *addr, uint8_t *out);
extern int acBerrReadLong(volatile uint32_t *addr, uint32_t *out);
extern int acBerrWriteLong(volatile uint32_t *addr, uint32_t val);

// How many byte-identical re-reads Kickstart's ReadExpansionRom demands.
#define AC_STABILITY_PASSES 11

// Kickstart aborts after this many boards; so do we (runaway/looping board).
#define AC_MAX_BOARDS 32

// ---------------------------------------------------------------------------
// Config-space access, Kickstart ReadExpansionByte/WriteExpansionByte/
// WriteExpansionWord — byte-exact choreography and ordering.
// ---------------------------------------------------------------------------
static uint8_t acReadByte(volatile uint8_t *board, uint32_t offset, int pace)
{
    volatile uint8_t *p = board + offset * 4;
    uint8_t hi, lo;

    if (pace) { waitLong(); waitLong(); }   // old-asm pacing: slow boards wake
    else      { waitShort(); }

    // Low nibble FIRST (+2 / +$100), THEN the +0 byte — the exact access
    // order of both Kickstart's ReadExpansionByte and the old asm.
    lo = ((uint32_t)board & 0x80000000UL) ? p[0x100] : p[2];
    hi = p[0];
    return (uint8_t)((hi & 0xF0) | (lo >> 4));
}

// Whole 16-byte config ROM: byte 0 (er_Type) raw, all others inverted on the
// bus (Kickstart readrom does exactly this).
static void acReadRom(volatile uint8_t *board, uint8_t *out, int pace)
{
    out[0] = acReadByte(board, 0, pace);
    for (uint32_t i = 1; i < ER_SIZEOF; i++)
        out[i] = (uint8_t)~acReadByte(board, i, pace);
}

// Kickstart WriteExpansionByte: low nibble (shifted to the top half) to
// +2 / +$100 FIRST, then the full byte to +0 — nibble boards latch the low
// half until the +0 write arrives.
static void acWriteByte(volatile uint8_t *board, uint32_t offset, uint8_t val)
{
    volatile uint8_t *p = board + offset * 4;
    if ((uint32_t)board & 0x80000000UL) p[0x100] = (uint8_t)(val << 4);
    else                                p[2]     = (uint8_t)(val << 4);
    p[0] = val;
}

// Kickstart WriteExpansionWord: low byte as a byte to +4 FIRST, then the
// word to +0.
static void acWriteWord(volatile uint8_t *board, uint32_t offset, uint16_t val)
{
    volatile uint8_t *p = board + offset * 4;
    p[4] = (uint8_t)val;
    *(volatile uint16_t *)p = val;
}

// ---------------------------------------------------------------------------
// Kickstart Z2 slot-map allocator (v40 utils.asm, faithfully). 256 slots of
// 64K; only $20-$9F (8MB space) and $E9-$EF start out free. The map lives on
// the CALLER's stack — a mutable static would land in read-only ROM here.
// ---------------------------------------------------------------------------
#define AC_TOTALSLOTS 256

// AllocBoardMem's size-code table: {slots, slotOffset}. Offset 32 encodes
// the special alignment rules: 4MB boards go on an "odd 2MB boundary"
// ($200000, $600000, ...), 8MB boards can only ever land at $200000.
static const uint8_t acBoardMap[8][2] = {
    { 128, 32 },   // code 0: 8MB
    {   1,  0 },   // code 1: 64K
    {   2,  0 },   // code 2: 128K
    {   4,  0 },   // code 3: 256K
    {   8,  0 },   // code 4: 512K
    {  16,  0 },   // code 5: 1MB
    {  32,  0 },   // code 6: 2MB
    {  64, 32 },   // code 7: 4MB
};

// Z3 physical config sizes in 64K slots, by size code when ERFB_EXTENDED is
// set (code 7 reserved). Without ERFB_EXTENDED Kickstart forces 16MB.
static const uint16_t acZ3ExtSlots[8] = {
    256, 512, 1024, 2048, 4096, 8192, 16384, 0
};

// Z3 LOGICAL size in slots by sub-size code (config.asm Z3_LogicalSize):
// 0 = logical matches physical, 1 = board is auto-sized by the OS,
// 14/15 reserved. Includes the odd 6/10/12/14MB entries.
static const uint16_t acZ3LogicalSlots[16] = {
    0, 0, 1, 2, 4, 8, 16, 32, 64, 96, 128, 160, 192, 224, 0xFFFF, 0xFFFF
};

static void acSlotMapInit(uint8_t *map)
{
    for (int i = 0; i < AC_TOTALSLOTS; i++) map[i] = 0;          // taken
    for (int i = 0x20; i <= 0x9F; i++)      map[i] = 1;          // 8MB space
    for (int i = 0xE9; i <= 0xEF; i++)      map[i] = 1;          // $E9xxxx area
}

// First-fit over [first, last) stepping by num — Kickstart's
// AllocExpansionSlot. Returns start slot or -1; marks the slots taken.
static int acAllocSlotRange(uint8_t *map, uint32_t first, uint32_t last,
                            uint32_t num)
{
    for (uint32_t s = first; s < last; s += num) {
        int fits = 1;
        for (uint32_t i = 0; i < num; i++) {
            if (s + i >= AC_TOTALSLOTS || map[s + i] == 0) { fits = 0; break; }
        }
        if (fits) {
            for (uint32_t i = 0; i < num; i++) map[s + i] = 0;
            return (int)s;
        }
    }
    return -1;
}

// Kickstart's AllocBoardMem: boards needing <=7 slots (64K/128K/256K — RAM
// and I/O alike) FIRST try the $E8..$F0 window (slot $E8 itself is never
// free, so they land at $E9+), then fall back to the big space.
static int acAllocBoardMem(uint8_t *map, uint8_t typeByte)
{
    uint32_t code = typeByte & ERT_MEMMASK;
    uint32_t num  = acBoardMap[code][0];
    uint32_t off  = acBoardMap[code][1];

    if (num <= 7) {
        int s = acAllocSlotRange(map, 0xE8, 0xF0, num);
        if (s >= 0) return s;
    }
    return acAllocSlotRange(map, off, AC_TOTALSLOTS, num);
}

// ---------------------------------------------------------------------------
// Small print helpers
// ---------------------------------------------------------------------------
static void acPrintHexRow(const uint8_t *rom)
{
    for (int i = 0; i < ER_SIZEOF; i++) {
        print(binHexByte(rom[i]), CYAN);
        print(" ", WHITE);
    }
    print("\n", WHITE);
}

static void acPrintYesNo(int cond)
{
    print(cond ? "yes" : "no ", cond ? GREEN : WHITE);
}

static const char * const acSizeNames[8] = {
    "8MB", "64K", "128K", "256K", "512K", "1MB", "2MB", "4MB"
};
static const char * const acExtSizeNames[8] = {
    "16MB", "32MB", "64MB", "128MB", "256MB", "512MB", "1GB", "RESERVED"
};

// ---------------------------------------------------------------------------
// Stability: Kickstart re-reads the whole ROM 11 times and demands
// byte-identical results. We score it instead — an unstable readback is
// itself a diagnostic (flaky bus, floating lines).
// ---------------------------------------------------------------------------
typedef struct {
    int     matched;      // how many of the 11 re-reads were identical
    int     firstPass;    // 1-based pass number of first difference (-1 none)
    int     firstOff;     // byte offset of first difference
    uint8_t firstGot;     // what that byte read instead
} AcStability;

static void acStabilityCheck(volatile uint8_t *board, const uint8_t *rom,
                             AcStability *st)
{
    uint8_t reread[ER_SIZEOF];

    st->matched = 0;
    st->firstPass = -1;
    st->firstOff = 0;
    st->firstGot = 0;

    for (int pass = 0; pass < AC_STABILITY_PASSES; pass++) {
        acReadRom(board, reread, 0);
        int same = 1;
        for (int i = 0; i < ER_SIZEOF; i++) {
            if (reread[i] != rom[i]) {
                same = 0;
                if (st->firstPass < 0) {
                    st->firstPass = pass + 1;
                    st->firstOff  = i;
                    st->firstGot  = reread[i];
                }
                break;
            }
        }
        if (same) st->matched++;
    }
}

static void acPrintStability(const uint8_t *rom, const AcStability *st)
{
    int ok = (st->matched == AC_STABILITY_PASSES);
    print("  Stability:    ", WHITE);
    print(binDec(st->matched), ok ? GREEN : RED);
    print("/", WHITE);
    print(binDec(AC_STABILITY_PASSES), WHITE);
    if (ok) {
        print(" re-reads identical\n", GREEN);
    } else {
        print(" re-reads identical - UNSTABLE BUS?\n", RED);
        print("    first diff: pass ", YELLOW);
        print(binDec(st->firstPass), YELLOW);
        print(" byte ", YELLOW);
        print(binDec(st->firstOff), YELLOW);
        print(" read $", YELLOW);
        print(binHexByte(st->firstGot), YELLOW);
        print(" expected $", YELLOW);
        print(binHexByte(rom[st->firstOff]), YELLOW);
        print("\n", YELLOW);
    }
}

// ---------------------------------------------------------------------------
// Validation (Kickstart ReadExpansionRom rules), verdicts printed with the
// values that produced them. verbose=0 prints failures only.
// ---------------------------------------------------------------------------
static int acValidate(const uint8_t *rom, int verbose)
{
    uint16_t manu = (uint16_t)((rom[ER_MANUFACTURER] << 8) | rom[ER_MANUFACTURER + 1]);
    int res03Ok = (rom[ER_RESERVED03] == 0x00);
    int manuOk  = (manu != 0x0000) && (manu != 0xFFFF);

    if (verbose || !res03Ok) {
        print("  Reserved03:   $", WHITE);
        print(binHexByte(rom[ER_RESERVED03]), res03Ok ? GREEN : RED);
        print(res03Ok ? "  OK (must be $00)\n" : "  FAILED (must be $00)\n",
              res03Ok ? GREEN : RED);
    }
    if (verbose || !manuOk) {
        print("  Manufacturer: $", WHITE);
        print(binHexByte(rom[ER_MANUFACTURER]), manuOk ? GREEN : RED);
        print(binHexByte(rom[ER_MANUFACTURER + 1]), manuOk ? GREEN : RED);
        print(manuOk ? "  OK (not $0000/$FFFF)\n"
                     : "  FAILED (must not be $0000/$FFFF)\n",
              manuOk ? GREEN : RED);
    }
    return res03Ok && manuOk;
}

// Full field decode (Detailed mode).
static void acDisplayBoard(const uint8_t *rom)
{
    uint8_t type  = rom[ER_TYPE];
    uint8_t flags = rom[ER_FLAGS];
    uint8_t sizeCode = (uint8_t)(type & ERT_MEMMASK);
    int isZ2  = ((type & ERT_TYPEMASK) == ERT_ZORROII);
    int isZ3  = ((type & ERT_TYPEMASK) == ERT_ZORROIII);
    int ext   = (flags >> ERFB_EXTENDED) & 1;
    int z3flg = (flags >> ERFB_ZORRO_III) & 1;
    uint16_t manu = (uint16_t)((rom[ER_MANUFACTURER] << 8) | rom[ER_MANUFACTURER + 1]);

    print("  Type $", WHITE);
    print(binHexByte(type), CYAN);
    print(": ", WHITE);
    if (isZ2)      print("Zorro II ", GREEN);
    else if (isZ3) print("Zorro III", GREEN);
    else           print("INVALID type bits", RED);
    print("  size code ", WHITE);
    print(binDec(sizeCode), CYAN);
    print(" (", WHITE);
    print((char *)(ext ? acExtSizeNames[sizeCode] : acSizeNames[sizeCode]), CYAN);
    print(")\n", WHITE);

    print("  Memlist RAM: ", WHITE);  acPrintYesNo((type >> ERTB_MEMLIST) & 1);
    print("   Boot ROM: ", WHITE);    acPrintYesNo((type >> ERTB_DIAGVALID) & 1);
    print("   Chained: ", WHITE);     acPrintYesNo((type >> ERTB_CHAINED) & 1);
    print("\n", WHITE);

    print("  Product:      $", WHITE);
    print(binHexByte(rom[ER_PRODUCT]), CYAN);
    print(" (", WHITE);
    print(binDec(rom[ER_PRODUCT]), CYAN);
    print(")   Manufacturer: ", WHITE);
    print(binDec(manu), CYAN);
    print("\n", WHITE);

    print("  Flags $", WHITE);
    print(binHexByte(flags), CYAN);
    print(": shutup ", WHITE);
    acPrintYesNo(!((flags >> ERFB_NOSHUTUP) & 1));
    print("  extended-size ", WHITE);
    acPrintYesNo(ext);
    if (ext && isZ2)
        print(" (INVALID on Zorro II)", RED);
    print("  Z3-flag ", WHITE);
    print(binDec(z3flg), CYAN);
    if (z3flg != (isZ3 ? 1 : 0))
        print(" (does NOT match type bits!)", RED);
    print("\n", WHITE);

    uint32_t serial = ((uint32_t)rom[ER_SERIAL] << 24)
                    | ((uint32_t)rom[ER_SERIAL + 1] << 16)
                    | ((uint32_t)rom[ER_SERIAL + 2] << 8)
                    |  (uint32_t)rom[ER_SERIAL + 3];
    print("  Serial:       ", WHITE);
    print(binHex(serial), CYAN);           // full longword
    print("   DiagVec offset: $", WHITE);
    print(binHexByte(rom[ER_INITDIAGVEC]), CYAN);
    print(binHexByte(rom[ER_INITDIAGVEC + 1]), CYAN);
    print("\n", WHITE);
}

// ---------------------------------------------------------------------------
// Board-list record — byte-identical to the old asm's layout (TF diag walks
// it with a hardcoded 14-byte stride and these exact offsets). Only
// ASSIGNED boards are recorded.
// ---------------------------------------------------------------------------
static void acRecordBoard(const uint8_t *rom, uint32_t start, uint32_t end)
{
    if (globals->AutoConfBoards >= AC_MAX_BOARDS)
        return;
    volatile uint8_t *rec = (volatile uint8_t *)&globals->AutoConfList[0]
                          + globals->AutoConfBoards * 14;
    rec[0]  = rom[ER_MANUFACTURER];         // manufacturer word
    rec[1]  = rom[ER_MANUFACTURER + 1];
    rec[2]  = rom[ER_SERIAL];               // serial HIGH word (old-asm quirk,
    rec[3]  = rom[ER_SERIAL + 1];           // kept for record compatibility)
    rec[4]  = rom[ER_TYPE];
    rec[5]  = rom[ER_FLAGS];
    rec[6]  = (uint8_t)(start >> 24);
    rec[7]  = (uint8_t)(start >> 16);
    rec[8]  = (uint8_t)(start >> 8);
    rec[9]  = (uint8_t)start;
    rec[10] = (uint8_t)(end >> 24);
    rec[11] = (uint8_t)(end >> 16);
    rec[12] = (uint8_t)(end >> 8);
    rec[13] = (uint8_t)end;
    globals->AutoConfBoards++;
}

// Detailed-mode prompt: 1 = assign (Y/LMB), 0 = shut up (N/RMB), -1 = ESC.
static int acAsk(void)
{
    ClearBuffer();
    waitReleased();
    for (;;) {
        getInput();
        uint8_t ch = globals->GetCharData;
        if (globals->LMB) { waitReleased(); return 1; }
        if (globals->RMB) { waitReleased(); return 0; }
        if (ch == 'y' || ch == 'Y') return 1;
        if (ch == 'n' || ch == 'N') return 0;
        if (ch == 0x1b) return -1;
        waitShort();
    }
}

// ---------------------------------------------------------------------------
// Zorro III config space ($FF000000) gate. Kickstart's big-box builds probe
// this space as a matter of course, but DiagROM has no bus-error recovery
// yet, so on the wrong machine an access up there can hang the bus outright
// (same failure mode as disk.c's $DD0000 lesson from real-A1200 testing).
// Triple gate, every leg reported: (1) 32-bit address bus — on a 24-bit CPU
// $FF000000 wraps down into the $F00000 area; (2) not a Gayle machine;
// (3) the Ramsey version register answers stably with a known value —
// Ramsey and SuperBuster only exist together, so a live Ramsey is the best
// available no-BERR proxy for "the Z3 config space is actually decoded".
// Known residual risk: a 32-bit accelerator in a Zorro II box could in
// theory float-fake a stable Ramsey value; the honest fix is the planned
// bus-error handler, at which point this gate can relax.
static int acZ3ConfigSpaceUsable(void)
{
    if (globals->ADR24BIT) {
        print("Z3 config space ($FF000000) probe skipped: 24-bit CPU address bus.\n", YELLOW);
        return 0;
    }
    if (isGayleMachine()) {
        print("Z3 config space ($FF000000) probe skipped: Gayle machine (A600/A1200).\n", YELLOW);
        return 0;
    }
    uint8_t v = *RAMSEY_VERSION;
    for (int i = 0; i < 7; i++) {
        if (*RAMSEY_VERSION != v) {
            print("Z3 config space ($FF000000) probe skipped: unstable read at $DE0043,\n"
                  "no Ramsey - not a big-box machine.\n", YELLOW);
            return 0;
        }
    }
    if (v != 0x0D && v != 0x0F) {
        print("Z3 config space ($FF000000) probe skipped: $DE0043 reads $", YELLOW);
        print(binHexByte(v), YELLOW);
        print(", not a known\nRamsey version ($0D/$0F) - not a big-box machine.\n", YELLOW);
        return 0;
    }
    print("Ramsey $", GREEN);
    print(binHexByte(v), GREEN);
    print(v == 0x0F ? " (Ramsey-07)" : " (Ramsey-04)", GREEN);
    print(" - big box: probing Z3 config space $FF000000 too.\n", GREEN);
    return 1;
}

// ---------------------------------------------------------------------------
// Boot/diag ROM (DiagArea) display — v40 DoDiag's read protocol, faithfully,
// but DISPLAY ONLY: a diagnostic ROM does not execute foreign boot code
// (Kickstart would copy the area to RAM and jsr da_DiagPoint at config time).
// ---------------------------------------------------------------------------

// da_Config bits (configregs.i). Everything meaningful lives in the UPPER
// nibble, which is why v40 reads the byte raw even on nibble-wide boards.
#define DAC_BUSWIDTH    0xC0
#define DAC_NIBBLEWIDE  0x00
#define DAC_BYTEWIDE    0x40
#define DAC_WORDWIDE    0x80
#define DAC_BOOTTIME    0x30
#define DAC_NEVER       0x00
#define DAC_CONFIGTIME  0x10
#define DAC_BINDTIME    0x20

// struct DiagArea byte offsets (configvars.h)
#define DA_CONFIG    0
#define DA_FLAGS     1
#define DA_SIZE      2
#define DA_DIAGPOINT 4
#define DA_BOOTPOINT 6
#define DA_NAME      8
#define DA_SIZEOF    14

// One DiagArea byte via the bus-width stride (v40 wordToWord/byteToWord/
// nybbleToWordCopy): word-wide = contiguous, byte-wide = every 2nd byte,
// nibble-wide = high nibble at off*4, low nibble at off*4+2.
static uint8_t daRead(volatile uint8_t *area, uint32_t off, uint8_t width)
{
    if (width == DAC_WORDWIDE) return area[off];
    if (width == DAC_BYTEWIDE) return area[off * 2];
    return (uint8_t)((area[off * 4] & 0xF0) | (area[off * 4 + 2] >> 4));
}

static uint16_t daReadWord(volatile uint8_t *area, uint32_t off, uint8_t width)
{
    return (uint16_t)((daRead(area, off, width) << 8) | daRead(area, off + 1, width));
}

static void acShowDiagArea(uint32_t boardBase, const uint8_t *rom)
{
    volatile uint8_t *area;
    uint8_t cfg, width, bt;
    uint32_t vec = ((uint32_t)rom[ER_INITDIAGVEC] << 8) | rom[ER_INITDIAGVEC + 1];

    print("  Boot ROM: DiagVec ", WHITE);
    print(binHexWord((uint16_t)vec), CYAN);
    if (vec == 0) {
        print(" - DIAGVALID set but vector is 0 (Kickstart ignores it).\n", YELLOW);
        return;
    }
    area = (volatile uint8_t *)boardBase + vec;
    if (acBerrProbeRead(area, &cfg)) {
        print(" - BUS ERROR reading DiagArea at that offset!\n", RED);
        return;
    }
    width = (uint8_t)(cfg & DAC_BUSWIDTH);
    print("  da_Config $", WHITE);
    print(binHexByte(cfg), CYAN);
    print(" = ", WHITE);
    if (width == DAC_NIBBLEWIDE)    print("nibble-wide", CYAN);
    else if (width == DAC_BYTEWIDE) print("byte-wide", CYAN);
    else if (width == DAC_WORDWIDE) print("word-wide", CYAN);
    else {
        print("INVALID bus width (both bits set)\n", RED);
        return;
    }
    bt = (uint8_t)(cfg & DAC_BOOTTIME);
    if (bt == DAC_CONFIGTIME)    print(", boots at CONFIG time\n", CYAN);
    else if (bt == DAC_BINDTIME) print(", boots at bind time\n", CYAN);
    else if (bt == DAC_NEVER)    print(", boot never\n", CYAN);
    else                         print(", boot-time bits RESERVED ($30)\n", YELLOW);

    {
        uint16_t dsize = daReadWord(area, DA_SIZE, width);
        uint16_t diag  = daReadWord(area, DA_DIAGPOINT, width);
        uint16_t boot  = daReadWord(area, DA_BOOTPOINT, width);
        uint16_t name  = daReadWord(area, DA_NAME, width);

        print("   size ", WHITE);
        print(binHexWord(dsize), CYAN);
        if (dsize < DA_SIZEOF)
            print(" (smaller than the DiagArea header itself!)", RED);
        print("  DiagPoint ", WHITE);
        print(binHexWord(diag), diag ? CYAN : WHITE);
        print("  BootPoint ", WHITE);
        print(binHexWord(boot), boot ? CYAN : WHITE);
        print("\n", WHITE);
        if ((diag && diag >= dsize) || (boot && boot >= dsize))
            print("   (entry offset outside da_Size - suspicious)\n", YELLOW);

        if (name != 0 && name < dsize) {
            print("   name \"", WHITE);
            for (uint32_t i = 0; i < 32; i++) {
                uint8_t ch = daRead(area, name + i, width);
                if (ch == 0) break;
                if (ch < 32 || ch > 126) ch = '.';
                printChar((char)ch, CYAN);
            }
            print("\"\n", WHITE);
        }
    }
    print("   (displayed only - DiagROM does not run board boot code)\n", YELLOW);
}

// ---------------------------------------------------------------------------
// v40 "_Very_ quick test to ensure memory actually exists" (DoOptions_Sized),
// under the BERR guard: first longword gets $00000000 then $FFFFFFFF, each
// verified, original restored. Kickstart flags BADMEMORY silently; we print.
// On a big box this doubles as the Buster Z3 DATA-cycle test.
// ---------------------------------------------------------------------------
static void acBoardMemCheck(uint32_t start)
{
    volatile uint32_t *p = (volatile uint32_t *)start;
    uint32_t saved, got;

    print("  RAM first-longword check: ", WHITE);
    if (acBerrReadLong(p, &saved)) {
        print("BUS ERROR on first read!\n", RED);
        return;
    }
    acBerrWriteLong(p, 0x00000000UL);
    acBerrReadLong(p, &got);
    if (got != 0x00000000UL) {
        print("wrote $00000000, read ", RED);
        print(binHex(got), RED);
        print(" - BAD (Kickstart would flag BADMEMORY)\n", RED);
        acBerrWriteLong(p, saved);
        return;
    }
    acBerrWriteLong(p, 0xFFFFFFFFUL);
    acBerrReadLong(p, &got);
    if (got != 0xFFFFFFFFUL) {
        print("wrote $FFFFFFFF, read ", RED);
        print(binHex(got), RED);
        print(" - BAD (Kickstart would flag BADMEMORY)\n", RED);
        acBerrWriteLong(p, saved);
        return;
    }
    acBerrWriteLong(p, saved);
    print("OK\n", GREEN);
}

// ---------------------------------------------------------------------------
// v40 Z3 autosize sniff, faithfully (magic $F2D4B689, EZ3_SIZEGRANULARITY =
// 512K steps, dummy base read to defeat the sticky data bus, first-longword
// shadow detection for address wrap) — plus the BERR guard v40 never had.
// Returns detected size in bytes, or 0xFFFFFFFF if the board base itself
// bus-errored (caller keeps the config size).
// ---------------------------------------------------------------------------
static uint32_t acZ3AutoSize(uint32_t start, uint32_t maxBytes)
{
    volatile uint32_t *base = (volatile uint32_t *)start;
    uint32_t first, size, off = 0;
    int berr = 0;

    print("  Auto-size (Kickstart 512K sniff): ", WHITE);
    if (acBerrReadLong(base, &first)) {
        print("BUS ERROR at board base - keeping config size.\n", RED);
        return 0xFFFFFFFFUL;
    }
    acBerrWriteLong(base, 0x00000000UL);
    for (;;) {
        uint32_t save, rb, dummy, shadow;
        volatile uint32_t *p;
        off += 0x80000UL;
        if (off >= maxBytes) { size = maxBytes; break; }
        p = (volatile uint32_t *)(start + off);
        if (acBerrReadLong(p, &save)) { berr = 1; size = off; break; }
        acBerrWriteLong(p, 0xF2D4B689UL);
        acBerrReadLong(base, &dummy);          // v40: "confuse" the sticky bus
        acBerrReadLong(p, &rb);
        if (rb != 0xF2D4B689UL) { size = off; break; }     // no memory here
        acBerrReadLong(base, &shadow);
        if (shadow != 0) { acBerrWriteLong(p, save); size = off; break; }  // wrapped
        acBerrWriteLong(p, save);
    }
    acBerrWriteLong(base, first);

    if (size >= 0x100000UL) {
        print(binDec((int32_t)(size >> 20)), CYAN);
        print("MB detected", CYAN);
    } else {
        print(binDec((int32_t)(size >> 10)), CYAN);
        print("KB detected", CYAN);
    }
    if (berr)
        print(" (stopped by BUS ERROR)", YELLOW);
    print("\n", WHITE);
    return size;
}

// Kickstart ReadExpansionRom's board-or-not verdict, silently.
static int acLooksLikeBoard(const uint8_t *rom)
{
    uint16_t manu = (uint16_t)((rom[ER_MANUFACTURER] << 8) | rom[ER_MANUFACTURER + 1]);
    return rom[ER_RESERVED03] == 0x00 && manu != 0x0000 && manu != 0xFFFF;
}

// One compact verdict on WHY a config space read as no-board. The two bus
// patterns seen in the wild get named: all-$FF float = open bus, and
// all-$00 = how UAE's big boxes answer at $E80000 while a Z3 board is
// waiting in Z3 config space (seen live 2026-08-08). Anything else gets
// the raw dump plus the named failed checks.
static void acPrintNonBoard(const uint8_t *rom)
{
    int allFF = (rom[0] == 0xFF);
    for (int i = 1; i < ER_SIZEOF && allFF; i++)
        if (rom[i] != 0x00) allFF = 0;          // bytes 1+ stored inverted
    int allZero = (rom[0] == 0x00);
    for (int i = 1; i < ER_SIZEOF && allZero; i++)
        if (rom[i] != 0xFF) allZero = 0;
    if (allFF) {
        print("bus reads $FF everywhere (open bus)\n", YELLOW);
    } else if (allZero) {
        print("bus reads $00 everywhere\n", YELLOW);
    } else {
        print("non-board pattern - raw + failed checks:\n   ", YELLOW);
        acPrintHexRow(rom);
        acValidate(rom, 0);
    }
}

// A caught bus error on a config-space probe is not a malfunction of the
// test - it IS the diagnosis: this exact event is what yellow-screens
// Kickstart at boot on a big box. Report it with the machine-level meaning
// per space (hint), plus Fat Gary's timed-out flag when we armed it, which
// separates "nothing terminated the cycle" (Gary timeout -> dead/missing
// logic, e.g. no daughterboard pulling /DTACK) from "something actively
// asserted BERR" (e.g. Buster itself). Flag-clear behavior is undocumented
// in the sources we have; re-writing the mode byte re-arms it - calibrate
// on real hardware.
static void acPrintBerr(int garyArmed, const char *hint)
{
    print("BUS ERROR caught on probe", RED);
    if (garyArmed) {
        if (*GARY_TIMEOUT_MODE & 0x80)
            print(" (Gary timeout: nothing terminated the cycle)", RED);
        else
            print(" (no Gary timeout: BERR actively asserted)", RED);
        *GARY_TIMEOUT_MODE = 0x80;   // re-arm BERR mode / clear attempt
    }
    print("\n", WHITE);
    print((char *)hint, YELLOW);
}

// ---------------------------------------------------------------------------
// The engine: walk the config chain board by board, Kickstart logic +
// diagnostic reporting. Each pass probes $E80000 first, then (big boxes)
// $FF000000; the chain ends only when both read as non-boards.
// detailed=1 -> full decode + Y/N/ESC prompt per board; detailed=0
// (Automatic) -> compact output, always assign.
// ---------------------------------------------------------------------------
static void acRunAutoconfig(int detailed)
{
    uint8_t map[AC_TOTALSLOTS];       // stack, NOT static (ROM gotcha)
    uint8_t rom[ER_SIZEOF], prevRom[ER_SIZEOF];
    AcStability st;
    uint32_t z3Cursor = 0x4000;       // Z3 config slot cursor ($40000000>>16)
    int havePrev = 0;
    volatile uint8_t *prevBase = 0;
    int boardNo, assigned = 0, shutup = 0;
    int stop = 0;
    int z3Probe;

    clearScreen();
    print("\002Autoconfig - NEW - ", CYAN);
    print(detailed ? "Detailed" : "Automatic", CYAN);
    print("\n\n", WHITE);
    print("Zorro II config chain at $E80000 - Kickstart-style configuration.\n", YELLOW);
    z3Probe = acZ3ConfigSpaceUsable();
    if (z3Probe) {
        // Kickstart big-box behavior (v40 config.asm "Set BERR (250 mSecs)"):
        // timeouts must raise BERR, not fake-DSACK garbage, so a dead bus
        // becomes a caught, reportable event below.
        *GARY_TIMEOUT_MODE   = 0x80;
        *GARY_TIMEOUT_ENABLE = 0x00;
        print("Fat Gary timeout armed: BERR after 250ms (as Kickstart, $80/$00).\n", YELLOW);
    }
    if (detailed)
        print("Per board: Y/LMB = assign   N/RMB = shut up   ESC = stop scan\n", YELLOW);
    print("\n", WHITE);

    acSlotMapInit(map);

    for (boardNo = 1; boardNo <= AC_MAX_BOARDS && !stop; boardNo++) {
        // Kickstart order (v40 ConfigChain, verified in source 2026-08-08):
        // big-box ROMs (A3000_ROMS builds, WHERE_TO_LOOK=$FF000000) probe
        // the PRIMARY space $FF000000 first and fall back to the ALTERNATE
        // $E80000 (a2/a4 in ConfigChain) on every pass; the chain only ends
        // when BOTH read as non-boards. Non-big-box ROMs use $E80000 alone.
        volatile uint8_t *base = z3Probe ? E_Z3CONFIGBASE : E_EXPANSIONBASE;
        uint8_t firstRom[ER_SIZEOF];
        int firstBerr, e8Berr = 0;
        uint8_t probeByte;

        // Every space is first touched through the vector-2 guard: on a big
        // box with a dead/undecoded expansion bus this exact access is what
        // yellow-screens Kickstart - here it becomes a verdict instead.
        firstBerr = acBerrProbeRead(base, &probeByte);
        if (!firstBerr)
            acReadRom(base, rom, 1);

        if (firstBerr || !acLooksLikeBoard(rom)) {
            int found = 0;
            for (int i = 0; i < ER_SIZEOF; i++) firstRom[i] = rom[i];
            if (z3Probe) {
                base = E_EXPANSIONBASE;
                e8Berr = acBerrProbeRead(base, &probeByte);
                if (!e8Berr) {
                    acReadRom(base, rom, 1);
                    found = acLooksLikeBoard(rom);
                }
            }
            if (!found) {
                if (boardNo == 1)
                    print("No unconfigured boards answer.\n", YELLOW);
                else
                    print("\nEnd of chain.\n", GREEN);
                if (z3Probe) {
                    print("  $FF000000: ", WHITE);
                    if (firstBerr)
                        acPrintBerr(1, "     -> Z3 config space not decoded: SuperBuster fault/absent?\n");
                    else
                        acPrintNonBoard(firstRom);
                    print("  $E80000:   ", WHITE);
                    if (e8Berr)
                        acPrintBerr(1, "     -> Z2 config space dead: on an A3000 this is the missing-\n"
                                       "        daughterboard /DTACK signature; otherwise Buster/bus fault.\n");
                    else
                        acPrintNonBoard(rom);
                } else {
                    print("  $E80000:   ", WHITE);
                    if (firstBerr)
                        acPrintBerr(0, "     -> bus error probing the Z2 config space.\n");
                    else
                        acPrintNonBoard(firstRom);
                }
                break;
            }
        }

        acStabilityCheck(base, rom, &st);

        // A board answered. Same bytes from the same config space as the one
        // we just configured means either a second identical board or an
        // ignored write — say so.
        if (havePrev && base == prevBase) {
            int same = 1;
            for (int i = 0; i < ER_SIZEOF; i++)
                if (rom[i] != prevRom[i]) { same = 0; break; }
            if (same)
                print("\nWARNING: config ROM byte-identical to the previous board -\n"
                      "either a second identical board, or the previous write was IGNORED.\n", RED);
        }

        print("\nBoard ", WHITE);
        print(binDec(boardNo), CYAN);
        print(":", WHITE);
        if (base == E_Z3CONFIGBASE)
            print("  (answered in Z3 config space $FF000000)", CYAN);
        print("\n", WHITE);
        if (detailed) {
            print("  Raw: ", WHITE);
            acPrintHexRow(rom);
            acPrintStability(rom, &st);
            acValidate(rom, 1);
            acDisplayBoard(rom);
        } else {
            uint8_t type = rom[ER_TYPE];
            uint16_t manu = (uint16_t)((rom[ER_MANUFACTURER] << 8) | rom[ER_MANUFACTURER + 1]);
            int ext = (rom[ER_FLAGS] >> ERFB_EXTENDED) & 1;
            print("  ", WHITE);
            print(((type & ERT_TYPEMASK) == ERT_ZORROII) ? "Zorro II " : "Zorro III", CYAN);
            print(" ", WHITE);
            print((char *)(ext ? acExtSizeNames[type & ERT_MEMMASK]
                               : acSizeNames[type & ERT_MEMMASK]), CYAN);
            print("  manu ", WHITE);
            print(binDec(manu), CYAN);
            print("  prod $", WHITE);
            print(binHexByte(rom[ER_PRODUCT]), CYAN);
            print("\n", WHITE);
            if (st.matched != AC_STABILITY_PASSES)
                acPrintStability(rom, &st);
        }

        // ---- Kickstart classification + allocation --------------------
        uint8_t type  = rom[ER_TYPE];
        uint8_t flags = rom[ER_FLAGS];
        uint8_t sizeCode = (uint8_t)(type & ERT_MEMMASK);
        int isZ2 = ((type & ERT_TYPEMASK) == ERT_ZORROII);
        int ext  = (flags >> ERFB_EXTENDED) & 1;
        int noShutup = (flags >> ERFB_NOSHUTUP) & 1;

        int haveAddr = 0, doShutup = 0, isZ3write = 0, autoSizeWanted = 0;
        uint32_t start = 0, sizeBytes = 0, z3WindowBytes = 0;
        uint16_t slotNum = 0;

        if (!isZ2 && ((type & ERT_TYPEMASK) != ERT_ZORROIII))
            print("  NOTE: invalid type bits - Kickstart treats any non-$C0 type as Zorro III.\n", RED);
        if (isZ2 && base == E_Z3CONFIGBASE)
            print("  NOTE: Zorro II-typed board answering in Z3 config space - unusual.\n", YELLOW);

        if (isZ2) {
            int slot = acAllocBoardMem(map, type);
            if (slot >= 0) {
                slotNum   = (uint16_t)slot;
                start     = (uint32_t)slot << 16;
                sizeBytes = (uint32_t)acBoardMap[sizeCode][0] << 16;
                haveAddr  = 1;
            }
        } else {
            // Zorro III path (Kickstart ConfigBoard_Zorro_III)
            uint32_t slots = ext ? (uint32_t)acZ3ExtSlots[sizeCode] : 256;
            if (slots == 0) {
                print("  Reserved Z3 extended size code - Kickstart's table has no entry.\n", RED);
            } else {
                uint32_t aligned = (z3Cursor + slots - 1) & ~(slots - 1);
                if (aligned + slots > 0x8000UL) {
                    print("  Z3 config area exhausted (past $7FFFFFFF) - Kickstart has NO\n", RED);
                    print("  range check here and would overflow; treating as no-fit.\n", RED);
                } else {
                    z3Cursor  = aligned + slots;
                    slotNum   = (uint16_t)aligned;
                    start     = aligned << 16;
                    isZ3write = 1;
                    haveAddr  = 1;
                    // Logical size (what's actually on the board)
                    uint32_t sub = flags & ERF_Z3_SSMASK;
                    uint32_t lslots = acZ3LogicalSlots[sub];
                    if (sub == 0)            sizeBytes = slots << 16;
                    else if (sub == 1) {     // Kickstart autosizes after config
                        sizeBytes      = slots << 16;
                        z3WindowBytes  = slots << 16;
                        autoSizeWanted = 1;
                    } else if (lslots == 0xFFFF) {
                        sizeBytes = slots << 16;
                        print("  (reserved Z3 sub-size code - recording config size)\n", YELLOW);
                    } else                   sizeBytes = lslots << 16;
                }
            }
        }

        // ---- Failure policy (Kickstart ConfigBoard) -------------------
        if (!haveAddr) {
            if (noShutup) {
                print("  No space fits AND board refuses shutup -> ", RED);
                if (isZ2 && sizeCode == 1) {
                    // Kickstart's 64K exception: board stays live at the
                    // config address; chain is CLOGGED either way.
                    print("64K board left LIVE at its config address.\n", RED);
                    acRecordBoard(rom, (uint32_t)base,
                                  (uint32_t)base + 0x10000UL - 1);
                    globals->AutoConfDone = 1;
                } else {
                    print("CLOGGED.\n", RED);
                }
                print("  Kickstart stops ALL further configuration here - so do we.\n", RED);
                stop = 1;
                break;
            }
            print("  No expansion space fits this board -> shutting it up (Kickstart policy).\n", YELLOW);
            doShutup = 1;
        } else {
            print("  -> Kickstart address: ", WHITE);
            print(binHex(start), GREEN);
            print(" - ", WHITE);
            print(binHex(start + sizeBytes - 1), GREEN);
            if (isZ2)
                print(slotNum >= 0xE8 ? "  ($E9xxxx small-board area)\n"
                                      : "  (8MB space)\n", CYAN);
            else
                print("  (Zorro III)\n", CYAN);

            if (detailed) {
                print("  Assign? (Y/LMB=assign  N/RMB=shut up  ESC=stop): ", WHITE);
                int ans = acAsk();
                if (ans < 0) {
                    print("ESC\n  Scan stopped - board left unconfigured.\n", YELLOW);
                    stop = 1;
                    break;
                }
                if (ans == 0) {
                    print("no\n", YELLOW);
                    doShutup = 1;
                } else {
                    print("yes\n", GREEN);
                }
            }
        }

        // ---- The write (into whichever config space answered) ---------
        if (doShutup) {
            acWriteByte(base, EC_SHUTUP, 0);
            globals->AutoConfDone = 1;
            shutup++;
            print("  Board shut up (disabled until reset).\n", YELLOW);
        } else if (haveAddr) {
            if (isZ3write) acWriteWord(base, EC_Z3_HIGHBASE, slotNum);
            else           acWriteByte(base, EC_BASEADDRESS, (uint8_t)slotNum);
            globals->AutoConfDone = 1;
            // Board is live at `start` now — Kickstart's post-config work
            // happens here, in its order: autosize, then record, then the
            // RAM existence check and boot-ROM handling (display-only).
            if (autoSizeWanted) {
                uint32_t as = acZ3AutoSize(start, z3WindowBytes);
                if (as != 0xFFFFFFFFUL)
                    sizeBytes = as;
            }
            acRecordBoard(rom, start, start + sizeBytes - 1);
            assigned++;
            print("  Assigned.\n", GREEN);
            if ((rom[ER_TYPE] >> ERTB_MEMLIST) & 1)
                acBoardMemCheck(start);
            if ((rom[ER_TYPE] >> ERTB_DIAGVALID) & 1)
                acShowDiagArea(start, rom);
        }

        // Next loop pass re-probes both config spaces: the configured board
        // must have left them (verified via the identical-ROM warning above).
        for (int i = 0; i < ER_SIZEOF; i++) prevRom[i] = rom[i];
        havePrev = 1;
        prevBase = base;
    }

    if (boardNo > AC_MAX_BOARDS)
        print("\n32-board cap reached - a board may be ignoring its config writes.\n", RED);

    print("\nSummary: ", WHITE);
    print(binDec(assigned), assigned ? GREEN : WHITE);
    print(" assigned, ", WHITE);
    print(binDec(shutup), shutup ? YELLOW : WHITE);
    print(" shut up. Board list total now ", WHITE);
    print(binDec((int32_t)globals->AutoConfBoards), CYAN);
    print(".\n", WHITE);

    print("\nPress any key/button to return to the Others menu.\n", WHITE);
    WaitButton();
    initScreen();
    globals->PrintMenuFlag = 1;   // make mainLoop's printMenu() redraw the menu
}

void AutoConfigDetailC(void)
{
    acRunAutoconfig(1);
}

void AutoConfigAutoC(void)
{
    acRunAutoconfig(0);
}
