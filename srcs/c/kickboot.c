/* ============================================================================
 * kickboot.c - "Kickstart boot check": find what yellow-screens Kickstart.
 *
 * A yellow screen is exec's coldExcept handler: ANY CPU exception taken
 * between the vector fill in ColdStart and the moment StartExec installs the
 * real exception table. DiagROM boots on such a machine because it never
 * takes that exception - so this test replays, in order, what Kickstart 3.1
 * (V40 sources: exec/coldstart.asm, exec/exceptions.asm TypeOfCPU,
 * expansion/config.asm, exec/startexec.asm) touches in that window, with
 * every vector 2..47 caught by kickboot.s's ksGuard, and reports the step,
 * vector, PC and fault address of anything that fires.
 *
 * Order and bus mode follow the sources exactly, because they matter:
 *   - ColdStart writes Fat Gary to timeout mode $00 (fake DSACK after 9us).
 *     CPU/FPU detect, chip sizing, $C00000 sizing, the $F00000 ROM-tag scan,
 *     the expansion config chain ($FF000000 then $E80000) and the A3000
 *     motherboard RAM sizing ALL run in that mode: an unanswered cycle reads
 *     garbage, it does not bus-error.
 *   - Only after RAM sizing does expansion set $80 (BERR after 250ms). Then
 *     exec moves ExecBase + supervisor stack to the bottom of fast RAM and
 *     rescans $F00000-$F7FFFF for ROM tags - still on the coldExcept vectors.
 *
 * Every memory step is non-destructive (save/restore) and runs at IPL 7.
 * What this cannot replay: exec actually running out of the RAM it found, so
 * RAM faults beyond the quick checks here still need the full memory tests.
 * ========================================================================== */
#include "generic.h"
#include "globalvars.h"

extern int isGayleMachine(void);

// CPUGen is a ULONG in the globals struct that the asm writes with move.b,
// so the value sits in the most significant byte (big-endian).
#define KS_CPUGEN (*(volatile uint8_t *)&globals->CPUGen)

typedef struct {
    uint32_t vec;        // 0 = no exception
    uint32_t frame[6];   // first 24 bytes of the exception frame
    uint32_t d[8];
    uint32_t a[7];
} KsFault;

extern int  ksGuard(void (*fn)(void *), void *arg, KsFault *rec);
extern void ksStepCacr(void *out);
extern void ksStepPmove(void *out);
extern void ksStepFpu(void *out);
extern void ksStepMemND(void *args);
extern void ksStepSweep(void *args);
extern void ksSetCacr(uint32_t value);

#define KS_GARY_MODE    ((volatile uint8_t *)0x00DE0000UL)
#define KS_GARY_ENABLE  ((volatile uint8_t *)0x00DE0001UL)
#define KS_GARY_PUD     ((volatile uint8_t *)0x00DE0002UL)
#define KS_RAMSEY_VER   ((volatile uint8_t *)0x00DE0043UL)

// Where a fault would land under Kickstart.
#define KS_PREVEC  0     // before the vector fill: wild jump, not a clean yellow
#define KS_YELLOW  1     // inside the coldExcept window: yellow screen

static void ksHex(uint32_t v, uint8_t color)
{
    print(binHex(v), color);          // binHex() already prefixes '$'
}

static const char *ksVecName(uint32_t vec)
{
    switch (vec) {
    case 2:  return "Bus Error";
    case 3:  return "Address Error";
    case 4:  return "Illegal Instruction";
    case 5:  return "Divide by Zero";
    case 6:  return "CHK";
    case 7:  return "TRAPV";
    case 8:  return "Privilege Violation";
    case 9:  return "Trace";
    case 10: return "Line-A";
    case 11: return "Line-F";
    case 13: return "Coprocessor Protocol Violation";
    case 14: return "Format Error";
    case 15: return "Uninitialized Interrupt";
    case 24: return "Spurious Interrupt";
    }
    if (vec >= 25 && vec <= 31) return "Autovector Interrupt";
    if (vec >= 32 && vec <= 47) return "TRAP";
    return "Reserved vector";
}

// Decode PC and (where the frame has one) the fault address.
static void ksPrintFault(const KsFault *f, int where, int *suspects)
{
    uint32_t pc, addr = 0;
    int haveAddr = 0;

    if (KS_CPUGEN == 0 && (f->vec == 2 || f->vec == 3)) {
        // 68000 group-0 frame: FC word, access address, IR, SR, PC
        addr = (f->frame[0] << 16) | (f->frame[1] >> 16);
        pc   = (f->frame[2] << 16) | (f->frame[3] >> 16);
        haveAddr = 1;
    } else {
        pc = (f->frame[0] << 16) | (f->frame[1] >> 16);
        if (KS_CPUGEN != 0) {
            uint32_t fmt = (f->frame[1] >> 12) & 0xF;
            if (fmt == 0xA || fmt == 0xB) { addr = f->frame[4]; haveAddr = 1; }
            else if (fmt == 0x7)          { addr = f->frame[5]; haveAddr = 1; }
            else if (fmt == 0x4)          { addr = f->frame[2]; haveAddr = 1; }
            else if (fmt == 0x8) {
                addr = (f->frame[2] << 16) | (f->frame[3] >> 16);
                haveAddr = 1;
            }
        }
    }
    print("EXCEPTION ", RED);
    print(binDec((int32_t)f->vec), RED);
    print(" ", RED);
    print((char *)ksVecName(f->vec), RED);
    print("\n     PC ", RED);
    ksHex(pc, RED);
    if (haveAddr) {
        print("  fault address ", RED);
        ksHex(addr, RED);
    }
    if (where == KS_YELLOW)
        print("\n     -> Kickstart shows the YELLOW SCREEN at this point.\n", RED);
    else
        print("\n     -> Kickstart crashes here (before its yellow handler is set).\n", RED);
    (*suspects)++;
}

static int ksRun(void (*fn)(void *), void *arg, KsFault *f)
{
    f->vec = 0;
    return ksGuard(fn, arg, f);
}

// Quiet big-box test: stable known Ramsey version on a 32-bit, non-Gayle bus.
static int ksBigBox(uint8_t *ver)
{
    if (globals->ADR24BIT) return 0;
    if (isGayleMachine()) return 0;
    uint8_t v = *KS_RAMSEY_VER;
    for (int i = 0; i < 7; i++)
        if (*KS_RAMSEY_VER != v) return 0;
    if (v != 0x0D && v != 0x0F) return 0;
    *ver = v;
    return 1;
}

// ---- steps written in C (plain bus accesses) ------------------------------

// exec ColdStart chip sizing: 16K steps, shadow check against location 0.
// (Kickstart's stray write to the $200000 limit itself is not replayed.)
static void ksStepChipSize(void *arg)
{
    uint32_t *out = (uint32_t *)arg;
    volatile uint32_t * volatile zero = (volatile uint32_t *)0;
    volatile uint32_t *a3 = (volatile uint32_t *)0;
    uint32_t save0 = *zero;
    uint32_t d0 = 0;
    const uint32_t d1 = 0xF2D4B689UL;
    int restore = 0;

    *zero = 0;
    for (;;) {
        a3 = (volatile uint32_t *)((uint32_t)a3 + 0x4000);
        if ((uint32_t)a3 == 0x200000UL) break;
        d0 = *a3;
        *a3 = d1;
        restore = 1;
        if (*zero == d1) break;           // shadow of location 0
        if (*a3 == d1) { *a3 = d0; restore = 0; continue; }
        break;
    }
    if (restore) *a3 = d0;
    *zero = save0;
    out[0] = (uint32_t)a3;
}

// exec sizeExtMem ($C00000-$DC0000, 256K blocks), logic kept as written -
// including the "echo that is not the identity counts as memory" fall-through.
// Added: every word it writes is saved and put back afterwards, and INTENA is
// restored (on machines that mirror the custom chips there).
#define KS_EXT_LOWER 0x00C00000UL
#define KS_EXT_UPPER 0x00DC0000UL
#define KS_EXT_BLOCK 0x00040000UL
#define KS_W(base)   (*(volatile uint16_t *)((base) - 0x1000 + 0x9A))
#define KS_R(base)   (*(volatile uint16_t *)((base) - 0x1000 + 0x1C))

static void ksStepExtMem(void *arg)
{
    uint32_t *out = (uint32_t *)arg;      // [0]=end (0 none) [1]=chip mirror seen
    uint16_t intena = *(volatile uint16_t *)0x00DFF01CUL;
    uint16_t saved[7];
    uint32_t a4 = KS_EXT_LOWER;
    uint32_t a0 = KS_EXT_LOWER + KS_EXT_BLOCK;
    uint32_t a2;
    uint16_t d1;
    int n = 0, i;

    out[1] = 0;
    for (i = 0; i < 7; i++)           // 7 slots: the 8th would be real INTENA
        saved[i] = KS_W(KS_EXT_LOWER + (uint32_t)(i + 1) * KS_EXT_BLOCK);

    for (;;) {
        a2 = a4 + KS_EXT_BLOCK;
        n++;
        KS_W(a2) = 0x3fff;
        if (KS_R(a2) == 0) {
            KS_W(a2) = 0xbfff;
            if (KS_R(a2) == 0x3fff) { out[1] = 1; goto ischip; }
        }
        KS_W(a0) = 0;
        d1 = 0xF2D4;
        KS_W(a2) = d1;
        if (KS_W(a2) != d1) goto chkreturn;
        if (KS_W(a0) == d1 && a0 != a2) goto ismem;   // echo, not identity
        d1 = 0xB698;
        KS_W(a2) = d1;
        if (KS_W(a2) != d1) goto chkreturn;
ismem:
        a4 = a2;
        if (KS_EXT_UPPER > a4 && n < 7) continue;
        break;
    }
ischip:
    KS_W(a2) = 0x7fff;
chkreturn:
    a0 -= KS_EXT_BLOCK;
    if (a0 == a4) a4 = 0;
    out[0] = a4;

    for (i = 0; i < 7; i++)
        KS_W(KS_EXT_LOWER + (uint32_t)(i + 1) * KS_EXT_BLOCK) = saved[i];
    *(volatile uint16_t *)0x00DFF09AUL = 0x7fff;
    *(volatile uint16_t *)0x00DFF09AUL = (uint16_t)(intena | 0x8000);
}

// expansion ReadExpansionByte(offset 0): second-nybble address first.
typedef struct {
    volatile uint8_t *first, *second;
    uint32_t stage;
    uint8_t v[2];
} KsProbe;

static void ksStepProbe(void *arg)
{
    KsProbe *p = (KsProbe *)arg;
    p->stage = 0;
    p->v[0] = *p->first;
    p->stage = 1;
    p->v[1] = *p->second;
    p->stage = 2;
}

// expansion Test_A3K: invert twice, flushing the "sticky" bus in between.
static int ksTestA3K(volatile uint32_t *a0, volatile uint32_t *a2, uint32_t d3)
{
    uint32_t old4 = a0[1];
    uint32_t d1 = a0[0];
    a0[0] = ~a0[0];
    d1 = ~d1;
    if (a0 != a2) *a2 = d3;
    a0[1] = old4;
    if (a0[0] != d1) return 0;
    a0[0] = ~a0[0];
    d1 = ~d1;
    if (a0 != a2) *a2 = d3;
    a0[1] = old4;
    return a0[0] == d1;
}

#define KS_RAM_MIDDLE 0x08000000UL
#define KS_RAM_STEP   0x00100000UL

// expansion A3000 RAM sizing: down from $08000000, then up from it.
// out[0]=motherboard base, out[1]=CPU-slot end, out[2]=ROM remap seen.
static void ksStepFastSize(void *arg)
{
    uint32_t *out = (uint32_t *)arg;
    volatile uint32_t * volatile zero = (volatile uint32_t *)0;
    volatile uint32_t *mid = (volatile uint32_t *)KS_RAM_MIDDLE;
    uint32_t a0, d3;

    out[2] = 0;
    a0 = KS_RAM_MIDDLE;
    d3 = *zero;
    for (;;) {
        a0 -= KS_RAM_STEP;
        if (a0 < 0x01000000UL) break;
        if (!ksTestA3K((volatile uint32_t *)a0, zero, d3)) break;
    }
    out[0] = a0 + KS_RAM_STEP;

    if (out[0] != KS_RAM_MIDDLE) {
        // hardware ROM remap check at $07F80000 (Kickstart subtracts 512K)
        volatile uint32_t *ram = (volatile uint32_t *)0x07F80000UL;
        volatile uint32_t *rom = (volatile uint32_t *)0x00F80000UL;
        uint32_t size = KS_RAM_MIDDLE - out[0];
        uint32_t d2 = *ram;
        if (d2 == *rom) {
            *ram = size;
            if (*rom == size) out[2] = 1;
            *ram = d2;
        }
    }

    a0 = KS_RAM_MIDDLE;
    d3 = *mid;
    for (;;) {
        if (a0 >= 0x40000000UL) break;
        if (!ksTestA3K((volatile uint32_t *)a0, mid, d3)) break;
        a0 += KS_RAM_STEP;
    }
    out[1] = a0;
}

// ---- report helpers --------------------------------------------------------

static void ksMemCheck(const char *label, uint32_t start, uint32_t end, int where,
                       int *suspects)
{
    KsFault f;
    uint32_t args[5];

    args[0] = start; args[1] = end; args[2] = 0; args[3] = 0; args[4] = 0;
    print((char *)label, WHITE);
    if (ksRun(ksStepMemND, args, &f)) { ksPrintFault(&f, where, suspects); return; }
    if (args[2] == 0) { print("OK\n", GREEN); return; }
    print("BAD RAM at ", RED);
    ksHex(args[2], RED);
    print("\n     wrote ", RED);
    ksHex(args[3], RED);
    print(" read ", RED);
    ksHex(args[4], RED);
    if (args[2] < 0xC0)
        print("\n     -> exception vectors: Kickstart shows a GREEN screen.\n", RED);
    else
        print("\n     -> exec runs from here: Kickstart crashes (yellow or reset).\n", RED);
    (*suspects)++;
}

static void ksSweep(const char *label, int where, int *suspects)
{
    KsFault f;
    uint32_t args[3];

    args[0] = 0x00F00000UL; args[1] = 0x00F80000UL; args[2] = 0;
    print((char *)label, WHITE);
    if (ksRun(ksStepSweep, args, &f)) {
        print("stopped at ", RED);
        ksHex(f.a[0], RED);
        print(": ", RED);
        ksPrintFault(&f, where, suspects);
        return;
    }
    print("OK", GREEN);
    if (args[2]) {
        print(" (", YELLOW);
        print(binDec((int32_t)args[2]), YELLOW);
        print(" x $4AFC seen - possible ROM tag there)", YELLOW);
    }
    print("\n", WHITE);
}

static void ksProbe(const char *label, uint32_t first, uint32_t second,
                    int *suspects)
{
    KsFault f;
    KsProbe p;

    p.first = (volatile uint8_t *)first;
    p.second = (volatile uint8_t *)second;
    p.v[0] = p.v[1] = 0;
    print((char *)label, WHITE);
    if (ksRun(ksStepProbe, &p, &f)) { ksPrintFault(&f, KS_YELLOW, suspects); return; }
    print("no fault, reads $", GREEN);
    print(binHexByte(p.v[1]), GREEN);
    print(" $", GREEN);
    print(binHexByte(p.v[0]), GREEN);
    print("\n", WHITE);
}

// ---------------------------------------------------------------------------

void KickBootCheckC(void)
{
    KsFault f;
    uint32_t out[4];
    uint32_t origCacr = 0;
    int haveCacr = 0;
    uint8_t ramsey = 0;
    int bigBox;
    uint8_t gen = KS_CPUGEN;
    int ksSuspects = 0;               // local: mutable statics live in ROM
    int *suspects = &ksSuspects;

    clearScreen();
    print("\002Kickstart boot check\n\n", CYAN);
    print("Replays Kickstart 3.1's early boot, every CPU exception caught.\n", YELLOW);

    bigBox = ksBigBox(&ramsey);
    if (bigBox) {
        print("Ramsey $", GREEN);
        print(binHexByte(ramsey), GREEN);
        print(" - big box: A3000/A4000 steps included.\n", GREEN);
        // ColdStart: DSACK timeout mode, timeout enabled.
        *KS_GARY_MODE   = 0x00;
        *KS_GARY_ENABLE = 0x00;
        print("Gary power-up flag ($DE0002 bit 7): ", WHITE);
        print((*KS_GARY_PUD & 0x80) ? "set (cold start)\n" : "clear (warm start)\n", CYAN);
    } else {
        print("No Ramsey found: generic steps only.\n", YELLOW);
    }

    print("\n-- Before Kickstart sets its vectors --\n", CYAN);
    if (gen == 2 || gen == 3) {
        print("CPU cache register (MOVEC CACR): ", WHITE);
        if (ksRun(ksStepCacr, out, &f)) {
            ksPrintFault(&f, KS_PREVEC, suspects);
        } else {
            origCacr = out[1];
            haveCacr = 1;
            print("OK, reads back ", GREEN);
            print(binHexWord(out[2]), GREEN);
            print("\n", WHITE);
        }
    }
    if (bigBox && gen == 3) {
        print("MMU registers (PMOVE TC/TT0/TT1): ", WHITE);
        if (ksRun(ksStepPmove, out, &f)) ksPrintFault(&f, KS_PREVEC, suspects);
        else print("OK\n", GREEN);
    }

    print("\n-- Yellow-screen window, bus timeout = fake DSACK --\n", CYAN);
    ksMemCheck("Low chip RAM $8-$4000 (vectors, first ExecBase): ",
               0x8, 0x4000, KS_YELLOW, suspects);

    if (gen < 4) {
        print("FPU probe (FMOVE FPCR, FSAVE, FRESTORE): ", WHITE);
        if (ksRun(ksStepFpu, out, &f)) {
            if (f.vec == 11 || f.vec == 4) {
                print("no FPU (Line-F, Kickstart handles this)\n", GREEN);
            } else {
                print("FPU FAULT at instruction ", RED);
                print(binDec((int32_t)out[0] + 1), RED);
                print(" of 4\n     ", RED);
                ksPrintFault(&f, KS_YELLOW, suspects);
                print("     -> An FPU answers but not correctly. Check the FPU jumpers\n"
                      "        (clock/select), then seating, then the FPU chip itself.\n", YELLOW);
            }
        } else if (out[1] != 0) {
            print("no FPU (cycle ended, FPCR reads ", YELLOW);
            ksHex(out[1], YELLOW);
            print(")\n", YELLOW);
        } else {
            print(out[2] == 0x18 ? "OK, 68881\n" : "OK, 68882\n", GREEN);
        }
    } else {
        print("FPU probe: skipped (68040/060 path not replayed).\n", YELLOW);
    }

    print("Chip RAM sizing: ", WHITE);
    if (ksRun(ksStepChipSize, out, &f)) {
        ksPrintFault(&f, KS_YELLOW, suspects);
    } else {
        print("ends at ", GREEN);
        ksHex(out[0], GREEN);
        print("\n", WHITE);
    }

    print("$C00000 memory sizing: ", WHITE);
    if (ksRun(ksStepExtMem, out, &f)) {
        ksPrintFault(&f, KS_YELLOW, suspects);
    } else if (out[0] == 0) {
        print(out[1] ? "none (custom chip mirror)\n" : "none\n", GREEN);
    } else if (bigBox) {
        print("PHANTOM RAM up to ", RED);
        ksHex(out[0], RED);
        print("\n     -> no RAM exists here on a big box, yet Kickstart would add\n"
              "        and use it: crash soon after (yellow or reset).\n", RED);
        (*suspects)++;
    } else {
        print("RAM up to ", GREEN);
        ksHex(out[0], GREEN);
        print("\n", WHITE);
    }

    ksSweep("ROM tag scan $F00000-$F7FFFF: ", KS_YELLOW, suspects);

    if (bigBox)
        ksProbe("Zorro III config probe $FF000000: ", 0xFF000100UL, 0xFF000000UL, suspects);
    ksProbe("Zorro II config probe $E80000: ", 0x00E80002UL, 0x00E80000UL, suspects);

    if (bigBox) {
        uint32_t base = KS_RAM_MIDDLE;

        print("Motherboard fast RAM sizing: ", WHITE);
        if (ksRun(ksStepFastSize, out, &f)) {
            ksPrintFault(&f, KS_YELLOW, suspects);
        } else {
            base = out[0];
            if (base == KS_RAM_MIDDLE) {
                print("none found", YELLOW);
            } else {
                ksHex(base, GREEN);
                print("-$07FFFFFF (", GREEN);
                print(binDec((int32_t)((KS_RAM_MIDDLE - base) >> 20)), GREEN);
                print(" MB)", GREEN);
                if (out[2]) print(", ROM remapped at $07F80000", YELLOW);
            }
            if (out[1] != KS_RAM_MIDDLE) {
                print(", CPU slot to ", GREEN);
                ksHex(out[1], GREEN);
            }
            print("\n", WHITE);
        }

        // expansion: "Set BERR (250 mSecs)" - from here unanswered cycles fault.
        *KS_GARY_MODE = 0x80;
        print("\n-- Yellow-screen window, bus timeout = BERR after 250ms --\n", CYAN);
        if (KS_RAM_MIDDLE - base >= 0x10000UL)
            ksMemCheck("Fast RAM bottom 64K (final ExecBase + stack): ",
                       base, base + 0x10000UL, KS_YELLOW, suspects);
        ksSweep("ROM tag scan $F00000-$F7FFFF again: ", KS_YELLOW, suspects);

        *KS_GARY_MODE = 0x00;
        print("\nFat Gary timeout back to power-up mode.\n", YELLOW);
    }

    if (haveCacr)
        ksSetCacr(origCacr | 0x0808);     // restore, flushing both caches

    print("\nResult: ", WHITE);
    if (ksSuspects) {
        print(binDec((int32_t)ksSuspects), RED);
        print(" step(s) that stop Kickstart - see the red lines above.\n", RED);
    } else {
        print("nothing reproduced. Compare this screen with a working\n"
              "machine and run the full memory tests.\n", YELLOW);
    }
    print("\nPress any key/button to return to the Others menu.\n", WHITE);
    WaitButton();
    initScreen();
    globals->PrintMenuFlag = 1;
}
