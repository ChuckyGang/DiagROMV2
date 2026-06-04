/* ============================================================================
 * flashmenu.c — DeMoN II firmware flasher front-end (video menu)
 *
 * Replaces the old serial-only Setup sub-menu with an on-screen (Amiga monitor)
 * menu, driven from othertest.s `Setup:` (jsr _setupMenu / jmp _mainMenu).
 *
 * On entry it auto-detects the two flash chips and shows the cartridge's
 * current firmware version, then offers:
 *   1 - Upload firmware (Y-modem)  : <version of the uploaded image, or "none">
 *   2 - Flash firmware             : enabled only once a valid image is loaded
 *
 * Choosing (2) shows a full-screen warning + confirmation; "no" returns to the
 * flash menu, "yes" commits to the flash (which never returns on success).
 *
 * Both this screen and the Setup screen are CUSTOM screens (not DiagROM
 * array-menus): an array menu fires its highlighted item on Enter, so the very
 * Enter used to pick the parent item could immediately launch a child.  To stay
 * consistent with the rest of DiagROM they still support MOUSE navigation +
 * click and the same reverse-colour highlight, via the shared selectStep()
 * helper below (a trimmed copy of mainmenu.c's printMenu/handleMenu logic).
 *
 * The heavy lifting (chip detect, Y-modem receive into SRAM $b00000, and the
 * chip-RAM flashcode) lives in srcs/asm/demon_flash.s and is exposed here as
 * three C-callable primitives.  Firmware flashing is a Mode-A feature: there is
 * no usable display in Mode B (NoDraw), so we bail out there.
 *
 * MANTRA (.bss is not writable on the DeMoN link, see CLAUDE.md 6.2): this file
 * keeps NO writable globals/statics.  "Is a firmware loaded?" is re-derived each
 * time by validating the image still sitting in DeMoN SRAM at $b00000.
 *
 * The whole front-end exists in BOTH builds (the Makefiles wildcard
 * srcs/c/*.c): on the kickstart build flashMenu() is the "DeMoN only" notice
 * (Chucky's pattern, as used for the Kickstart ROM check), and none of the
 * DeMoN-only asm symbols are referenced there.
 * ========================================================================== */
#include "generic.h"

/* ----------------------------------------------------------------------------
 * selectStep — one input step over a vertical list of `n` selectable rows.
 *
 * Replicates mainmenu.c's mouse handling so the custom Setup/Flash screens feel
 * like the array menus: vertical mouse movement (accumulated to a threshold of
 * 40 in MenuMouseAdd/Sub, exactly like printMenu) moves the highlight, a mouse
 * click or Enter activates the highlighted row, and number/letter keys are
 * passed back to the caller as shortcuts.
 *
 * Updates *sel (clamped to [0, n-1]) and returns:
 *   SEL_MOVED  the highlight changed  -> caller redraws the items
 *   SEL_FIRE   current row activated  -> caller runs item *sel
 *   SEL_NONE   nothing happened
 *   >0         an ASCII key the caller maps to a shortcut
 * Present in both builds (uses only generic input primitives).
 * -------------------------------------------------------------------------- */
#define SEL_NONE   0
#define SEL_FIRE   (-1)
#define SEL_MOVED  (-2)

static int selectStep(int *sel, int n)
{
    getInput();

    if (globals->CurAddY) {                       /* mouse moved down */
        globals->MenuMouseSub = 0;
        globals->MenuMouseAdd += globals->CurAddY;
        if (globals->MenuMouseAdd >= 40) {
            globals->MenuMouseAdd = 0;
            if (*sel < n - 1) { (*sel)++; return SEL_MOVED; }
        }
        return SEL_NONE;
    }
    if (globals->CurSubY) {                        /* mouse moved up */
        globals->MenuMouseAdd = 0;
        globals->MenuMouseSub += globals->CurSubY;
        if (globals->MenuMouseSub >= 40) {
            globals->MenuMouseSub = 0;
            if (*sel > 0) { (*sel)--; return SEL_MOVED; }
        }
        return SEL_NONE;
    }

    if (globals->MBUTTON) {                        /* click -> activate sel */
        do { getInput(); waitShort(); } while (globals->MBUTTON);
        return SEL_FIRE;
    }

    if (globals->BUTTON) {                         /* keyboard / serial */
        uint8_t k = globals->GetCharData;
        globals->BUTTON = 0;
        if (k == 30) {                             /* up arrow (like printMenu) */
            if (*sel > 0) { (*sel)--; return SEL_MOVED; }
            return SEL_NONE;
        }
        if (k == 31) {                             /* down arrow */
            if (*sel < n - 1) { (*sel)++; return SEL_MOVED; }
            return SEL_NONE;
        }
        if (k == 0x0a || k == 0x0d)                /* Enter -> highlighted   */
            return SEL_FIRE;
        return (int)k;                             /* shortcut -> caller     */
    }
    return SEL_NONE;
}

/* draw one Setup item; label flips to reverse colour when highlighted. */
static void drawSetupItem(int idx, int hi)
{
    uint8_t col = hi ? R_CYAN : CYAN;
    if (idx == 0) {
        setPos(20, 5);
#ifdef TARGET_DEMON
        print("1 - Flash firmware", col);
#else
        print("1 - Flash firmware (only on DeMoN)", col);
#endif
    } else {
        setPos(20, 7);
        print("9 - Mainmenu", col);
    }
}

#ifdef TARGET_DEMON

#define CART_BASE   0x00a80000u      /* DeMoN ROM base (current firmware)      */
#define IMG_BASE    0x00b00000u      /* SRAM image buffer (uploaded firmware)  */
#define IMG_SIZE    0x00040000u      /* 256 KB image                           */
#define VER_OFS     0x0c             /* "$VER:" tag offset in the image header */
#define ACTI_SIG    0x41435449u      /* 'ACTI' at +4                           */

/* C-callable primitives from srcs/asm/demon_flash.s (DEMON-only). */
extern uint32_t demonFlashDetect(void)  __asm("demonFlashDetect");  /* (U3<<16)|U5 */
extern int      demonFlashReceive(void) __asm("demonFlashReceive"); /* 0 = ok      */
extern int      demonFlashWrite(void)   __asm("demonFlashWrite");   /* fail -> !=0 */

static const char *chipName(uint16_t id)
{
    switch (id) {
        case 0x1fd5: return "Atmel AT29C010A";
        case 0xbfb5: return "SST39SF010A";
        case 0x0120: return "AMD Am29F010";
        default:     return "Unknown / unsupported";
    }
}

/* A valid DeMoN v2 image present at `base`?  Same checks as demon_flash.s
 * validate_image: 'ACTI' magic at +4, ROM version vector at +$7c, AND the full
 * sum-of-longs checksum (longs $7c..$3fff8 vs the stored value at $3fffc).
 *
 * The checksum is recomputed here on EVERY menu redraw (not just after an
 * upload) so a firmware left in SRAM across a soft reset is only reported as
 * "loaded" while it is still bit-for-bit intact.  ~64K longs over DeMoN SRAM,
 * a fraction of a second; run once per screen redraw, never per mouse move.
 *
 * Compiled at -O2 (the project default is -O0): the checksum loop is otherwise
 * spilled to the stack every iteration (~1s over 64K longs).  -O2 keeps the
 * pointer/accumulator in registers (add.l (An)+,Dn) -> ~0.25s.  Same trick as
 * genericc.c. */
__attribute__((optimize("O2")))
static int hasImage(uint32_t base)
{
    const volatile uint32_t *p = (const volatile uint32_t *)base;
    if (p[1] != ACTI_SIG) return 0;                       /* 'ACTI' at +4   */
    if ((p[0x7c / 4] & 0xfffc0000u) != CART_BASE) return 0;/* version @ +$7c */

    const volatile uint32_t *q   = (const volatile uint32_t *)(base + 0x7c);
    const volatile uint32_t *end = (const volatile uint32_t *)(base + IMG_SIZE - 4);
    uint32_t sum = 0;
    while (q < end)
        sum += *q++;
    if (sum != *end)                                      /* stored @ $3fffc */
        return 0;
    return 1;
}

/* Concise human-readable version from the "$VER:" tag at offset $0C, or NULL if
 * there is no "$VER:" tag (e.g. a non-DiagROM image).  Our own images start the
 * $VER with the "DiagROM/DeMoN2 " program name; we skip past it so the menu
 * shows just the version+date (e.g. "V2.1 BETA 2026-06-03").  Images that do not
 * carry that name fall back to the full $VER text. */
static const char *verString(uint32_t base)
{
    const char *v = (const char *)(base + VER_OFS);
    if (!(v[0] == '$' && v[1] == 'V' && v[2] == 'E' && v[3] == 'R' && v[4] == ':'))
        return 0;
    v += 6;                                               /* skip "$VER: "  */

    /* Return just past the "DiagROM/DeMoN2 " program name, if present. */
    {
        const char *a = v;
        const char *b = "DiagROM/DeMoN2 ";
        while (*b && *a == *b) { a++; b++; }
        if (!*b)
            return a;
    }
    return v;
}

/* Search [base, base+IMG_SIZE) for the NUL-terminated `needle`; return a
 * pointer to the first match or NULL.  -O2 like hasImage (the project default
 * -O0 would spill this 256KB scan to the stack). */
__attribute__((optimize("O2")))
static const char *memFind(uint32_t base, const char *needle)
{
    const volatile char *hay = (const volatile char *)base;
    int len = 0;
    while (needle[len]) len++;
    if (!len) return 0;
    for (uint32_t i = 0; i + (uint32_t)len <= IMG_SIZE; i++) {
        int j = 0;
        while (j < len && hay[i + j] == needle[j]) j++;
        if (j == len) return (const char *)(base + i);
    }
    return 0;
}

/* Print up to `max` chars from p, stopping at NUL/CR/LF or the first space /
 * '(' (so "V5.2.1 (24-Dec-2025)" prints just the "V5.2.1" token, no date). */
static void printBounded(const char *p, uint8_t color, int max)
{
    for (int i = 0; i < max; i++) {
        char c = p[i];
        if (c == 0 || c == 13 || c == 10 || c == ' ' || c == '(') break;
        printChar(c, color);
    }
}

/* Print the firmware version for the image at `base` in `color`.  Returns 1 if
 * the firmware was recognised (and a version printed), 0 otherwise (the caller
 * then prints its own fallback).  Recognises:
 *   - DiagROM/DeMoN2 : "$VER:" tag at $0C (handled by verString)
 *   - original Action Replay 5 : header at +4 starts "ACTION"; the real version
 *     ("V5.2.1 (24-Dec-2025)") is pulled from the about-text marker. */
static int printFwVersion(uint32_t base, uint8_t color)
{
    const char *v = verString(base);
    if (v) { print((char *)v, color); return 1; }

    const volatile char *h = (const volatile char *)base;
    if (h[4]=='A' && h[5]=='C' && h[6]=='T' && h[7]=='I' && h[8]=='O' && h[9]=='N') {
        const char *m = memFind(base, "ACTION REPLAY AMIGA ");
        if (m) {                            /* m+20 -> "V5.2.1 (24-Dec-2025)" */
            print("Action Replay ", color);
            printBounded(m + 20, color, 40);
        } else {
            print("Action Replay 5", color);
        }
        return 1;
    }
    return 0;
}

/* Static part of the flash screen: centred title + current-cartridge info. */
static void drawFlashStatic(uint16_t u3, uint16_t u5)
{
    clearScreen();
    setPos(0, 0);
    print("\002DeMoN II firmware flasher\n", CYAN);    /* centred, like others */

    setPos(20, 3);
    print("Rom type         : ", WHITE);
    if (u3 == u5) {
        print((char *)chipName(u5), GREEN);
    } else {
        print((char *)chipName(u3), GREEN);
        print(" / ", WHITE);
        print((char *)chipName(u5), GREEN);
    }

    setPos(20, 4);
    print("Firmware version : ", WHITE);
    if (!printFwVersion(CART_BASE, GREEN))
        print("unknown (no $VER tag)", YELLOW);
}

/* One selectable row (0=upload, 1=flash, 2=back).  The label flips to reverse
 * colour when highlighted; the dynamic version suffix keeps its own colour.
 *
 * `drawVer` controls whether item 0's version suffix is (re)drawn: it is set
 * only on a FULL redraw, and cleared on a highlight move.  This matters because
 * resolving an AR5 image's version scans the whole 256KB image (printFwVersion
 * -> memFind); doing that on every mouse/arrow step made the menu lag with an
 * AR5 image loaded.  On a move we only recolour the label (cols 20..), leaving
 * the already-drawn version text (cols 52..) untouched — exactly how the stock
 * printMenu recolours an item without reprinting its MenuVariable. */
static void drawFlashItem(int idx, int hi, int haveImg, int drawVer)
{
    uint8_t col = hi ? R_CYAN : CYAN;
    if (idx == 0) {
        setPos(20, 6);
        print("1 - Upload firmware (Y-modem) : ", col);
        if (drawVer) {
            setPos(52, 6);                      /* just past the fixed label */
            if (haveImg) {
                if (!printFwVersion(IMG_BASE, GREEN))
                    print("loaded (unknown version)", GREEN);
            } else {
                print("none", YELLOW);
            }
        }
    } else if (idx == 1) {
        setPos(20, 8);
        if (haveImg) {
            print("2 - Flash firmware to cartridge", col);
        } else {
            print("2 - Flash firmware to cartridge ", col);
            print("(upload a firmware first)", YELLOW);
        }
    } else {
        setPos(20, 10);
        print("Q - back to menu", col);
    }
}

static int confirmFlash(void)
{
    clearScreen();
    setPos(0, 2);
    print("\002*** WARNING - FLASHING THE CARTRIDGE ROM ***\n\n", RED);
    print("\002Flashing overwrites the cartridge firmware. If power is lost\n", WHITE);
    print("\002during the process the cartridge may be left corrupted\n", WHITE);
    print("\002(recover by setting JP2 to 2-3, then flashing again).\n\n", WHITE);
    print("\002Before continuing:\n", WHITE);
    print("\002set JP1 to 2-3 NOW\n", YELLOW);
    print("\002USB-C goes SILENT during flashing - watch the SCREEN:\n", YELLOW);
    print("\002progress bars, then GREEN = OK / RED = FAIL, then reset.\n\n", YELLOW);
    print("\002Proceed with flashing?   Y = yes    N = no\n", CYAN);

    for (;;) {
        getInput();
        if (!globals->BUTTON) continue;
        uint8_t k = globals->GetCharData;
        globals->BUTTON = 0;
        if (k == 'Y' || k == 'y') return 1;
        if (k == 'N' || k == 'n' || k == 27) return 0;
    }
}

void flashMenu(void)
{
    /* Mode B (no chip RAM / no display): firmware flashing is not supported. */
    if (globals->NoDraw)
        return;

    uint32_t ids = demonFlashDetect();
    uint16_t u3 = (uint16_t)(ids >> 16);
    uint16_t u5 = (uint16_t)(ids & 0xffff);

    for (;;) {                                  /* outer: full redraw */
        int haveImg = hasImage(IMG_BASE);
        int sel = 0, i;

        drawFlashStatic(u3, u5);
        for (i = 0; i < 3; i++)
            drawFlashItem(i, i == sel, haveImg, 1);   /* full: draw version too */

        globals->MenuMouseAdd = 0;
        globals->MenuMouseSub = 0;

        for (;;) {                              /* inner: input loop */
            int r = selectStep(&sel, 3);
            if (r == SEL_MOVED) {
                for (i = 0; i < 3; i++)
                    drawFlashItem(i, i == sel, haveImg, 0);  /* move: label only */
                continue;
            }

            int act = 0;                        /* 1=upload 2=flash 3=back */
            if      (r == SEL_FIRE)             act = sel + 1;
            else if (r == '1')                  act = 1;
            else if (r == '2')                  act = 2;
            else if (r == 'q' || r == 'Q' || r == 27) act = 3;

            if (act == 1) {
                setPos(20, 14);
                print("Receiving via USB-C - start the Y-modem send now...", CYAN);
                demonFlashReceive();            /* transfers over serial; ~18s */
                break;                          /* outer redraw with new state */
            } else if (act == 2) {
                if (!haveImg)
                    continue;                   /* disabled until an upload     */
                if (confirmFlash()) {
                    demonFlashWrite();          /* never returns on success     */
                    /* returned -> pre-flight failed (no valid image / bad chip) */
                    setPos(0, 16);
                    print("Flash did not start (pre-flight failed).\n", RED);
                    print("Press any key to continue.\n", CYAN);
                    WaitButton();
                }
                break;                          /* "no" or failure -> redraw    */
            } else if (act == 3) {
                return;
            }
        }
    }
}

#else  /* !TARGET_DEMON : kickstart build — present but disabled */

void flashMenu(void)
{
    clearScreen();
    setPos(0, 0);
    print("\nFirmware flashing\n\n", CYAN);
    print("This option is available only for the DeMoN version of DiagROM.\n\n", YELLOW);
    print("Press any key to return to the menu.\n", CYAN);
    WaitButton();
}

#endif /* TARGET_DEMON */

/* ----------------------------------------------------------------------------
 * setupMenu — the "S - Setup" submenu (present in both builds).
 *
 * Custom screen (not a DiagROM array-menu) so the Enter used to pick "S - Setup"
 * can never auto-launch the highlighted child.  Still supports mouse navigation
 * + click and the reverse-colour highlight via selectStep(), to match the rest
 * of DiagROM; '1' and '9' (and q/ESC) also work as keyboard shortcuts.
 * -------------------------------------------------------------------------- */
void setupMenu(void)
{
    if (globals->NoDraw)                 /* Mode B: no display */
        return;

    for (;;) {                           /* outer: redraw whole screen */
        int sel = 0, i;
        clearScreen();
        setPos(0, 0);
        print("\002Setup\n\n", CYAN);    /* centred title, like other submenus */
        for (i = 0; i < 2; i++)
            drawSetupItem(i, i == sel);

        globals->MenuMouseAdd = 0;
        globals->MenuMouseSub = 0;

        for (;;) {                       /* inner: input loop */
            int r = selectStep(&sel, 2);
            if (r == SEL_MOVED) {
                for (i = 0; i < 2; i++)
                    drawSetupItem(i, i == sel);
                continue;
            }

            int act = 0;                 /* 1=flash 2=mainmenu */
            if      (r == SEL_FIRE)       act = sel + 1;
            else if (r == '1')            act = 1;
            else if (r == '9' || r == 'q' || r == 'Q' || r == 27) act = 2;

            if (act == 1) {
                flashMenu();             /* DeMoN: flasher / kickstart: notice */
                break;                   /* redraw the Setup screen on return  */
            }
            if (act == 2)
                return;
        }
    }
}
