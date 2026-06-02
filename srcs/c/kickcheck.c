/* ============================================================================
 * kickcheck.c — Kickstart ROM identification test
 *
 * Reads the host Amiga's Kickstart ROM, computes its CRC32, and compares
 * it against a table of known Kickstart CRC32 values to identify which
 * Kickstart is installed and confirm its integrity.
 *
 * Kickstart ROM mapping on a stock Amiga:
 *   256 KB ROM (Kick 1.x): $FC0000-$FFFFFF, mirrored at $F80000
 *   512 KB ROM (Kick 2.x/3.x): $F80000-$FFFFFF
 *   1 MB ROM   (ext+kick):    $F00000-$FFFFFF (rare on A500)
 *
 * The CRC32 is the standard (zlib/PNG) variant: reflected, polynomial
 * 0xEDB88320, init 0xFFFFFFFF, final XOR 0xFFFFFFFF — matching the values
 * used by WinUAE / Amiga Forever ROM databases.
 *
 * MANTRA: this test executes from DeMoN ROM and builds its CRC table on
 * the stack (DeMoN RAM) at runtime, so nothing relies on chip RAM or a
 * writable .bss. Reading the Kickstart is read-only, so there is no
 * runningflag/FT245 interaction.
 *
 * This test only makes sense on the DeMoN II cartridge, which runs ALONGSIDE
 * the host Kickstart.  On a standard DiagROM build, DiagROM itself occupies
 * the Kickstart socket ($F80000-$FFFFFF), so there is no separate Kickstart
 * to read.  The menu entry is still shown there (flagged "DeMoN only") but
 * selecting it just explains that — see the #else stub below.
 * ========================================================================== */
#include "generic.h"
#include "globalvars.h"

#ifdef TARGET_DEMON

#define KICK_BASE_512  0x00f80000u
#define KICK_BASE_256  0x00fc0000u
#define KICK_BASE_1M   0x00f00000u

#define KICK_SIZE_256  0x00040000u
#define KICK_SIZE_512  0x00080000u
#define KICK_SIZE_1M   0x00100000u

static uint32_t crc32Range(uint32_t base, uint32_t size, const uint32_t *table)
{
    const volatile uint8_t *p = (const volatile uint8_t *)base;
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < size; i++)
        crc = table[(crc ^ p[i]) & 0xFFu] ^ (crc >> 8);
    return crc ^ 0xFFFFFFFFu;
}

static uint32_t detectKickSize(uint32_t *base)
{
    const volatile uint32_t *lo = (const volatile uint32_t *)KICK_BASE_512;
    const volatile uint32_t *hi = (const volatile uint32_t *)KICK_BASE_256;

    int mirrored = 1;
    for (int i = 0; i < 8; i++) {
        uint32_t off = (uint32_t)i * 0x8000u;
        if (lo[off >> 2] != hi[off >> 2]) { mirrored = 0; break; }
    }

    if (mirrored) {
        *base = KICK_BASE_256;
        return KICK_SIZE_256;
    }
    *base = KICK_BASE_512;
    return KICK_SIZE_512;
}

/* Known Kickstart CRC32 values for Amiga 500 / 2000 (standard .rom images).
 * Values confirmed by reading real chips with this test are authoritative. */
struct KickEntry { uint32_t crc; const char *name; };
static const struct KickEntry kickList[] = {
    { 0xA6CE1636u, "Kickstart 1.2 (33.180)" },
    { 0xC4F0F55Fu, "Kickstart 1.3 (34.5)" },
    { 0xC3BDB240u, "Kickstart 2.04 (37.175)" },
    { 0x64466C2Au, "Kickstart 2.05 (37.300)" },
    { 0x43B0DF7Bu, "Kickstart 2.05 (37.350)" },
    { 0xFC24AE0Du, "Kickstart 3.1 (40.063)" },
    { 0x568F8786u, "Hyperion 3.1.4 (46.143)" },
    { 0xE1F50B0Bu, "Hyperion 3.2 (47.115)" },
};

static const char *kickName(uint32_t crc)
{
    for (unsigned i = 0; i < sizeof(kickList)/sizeof(kickList[0]); i++)
        if (kickList[i].crc == crc)
            return kickList[i].name;
    return 0;
}

void kickCheck(VARS)
{
    clearScreen();
    print("\002Kickstart ROM check\n\n", CYAN);

    uint32_t base;
    uint32_t size = detectKickSize(&base);

    print("ROM base   : ", WHITE);
    print(binHex(base), GREEN);
    print("\n", WHITE);

    print("ROM size   : ", WHITE);
    print(binHex(size), GREEN);
    print(" (", WHITE);
    print(binHex(size >> 10), GREEN);
    print(" KB)\n", WHITE);

    /* Build CRC32 table on the stack (DeMoN RAM) for fast table-driven CRC.
       Local array = stack = DeMoN RAM, so it is writable and honours the
       mantra without any fixed address or .bss dependency. */
    uint32_t table[256];
    for (uint32_t n = 0; n < 256; n++) {
        uint32_t cc = n;
        for (int k = 0; k < 8; k++)
            cc = (cc & 1u) ? (0xEDB88320u ^ (cc >> 1)) : (cc >> 1);
        table[n] = cc;
    }
    print("Computing CRC32, please wait...\n", WHITE);
    uint32_t crc = crc32Range(base, size, table);

    print("CRC32      : ", WHITE);
    print(binHex(crc), YELLOW);
    print("\n\n", WHITE);

    {
        const char *name = kickName(crc);
        print("Kickstart  : ", WHITE);
        if (name)
            print((char *)name, GREEN);
        else
            print("Unknown / not in database", YELLOW);
        print("\n\n", WHITE);
    }

    print("Press any key to return to menu.\n", CYAN);
    do {
        GetInput();
    } while (!globals->BUTTON);
}

#else /* !TARGET_DEMON */

/* Standard (Kickstart-socket) build: DiagROM is the Kickstart, so there is
 * nothing separate to check.  Show a short notice and return to the menu. */
void kickCheck(VARS)
{
    clearScreen();
    print("\002Kickstart ROM check\n\n", CYAN);
    print("This option is available only for the DeMoN version of DiagROM.\n\n", YELLOW);
    print("Press any key to continue and back to Others menu.\n", CYAN);
    do {
        GetInput();
    } while (!globals->BUTTON);
}

#endif /* TARGET_DEMON */
