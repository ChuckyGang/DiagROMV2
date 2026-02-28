#include "generic.h"
#include <stdint.h>
#include <stddef.h>

// ROM data symbols from data.s (need _Xxx:: aliases there)
extern uint8_t RomMenuCopper[];
extern uint8_t EndRomMenuCopper[];
extern uint8_t MenuBplPnt[];
extern uint8_t RomEcsCopper[];
extern uint8_t EndRomEcsCopper[];
extern uint8_t RomEcsCopper2[];
extern uint8_t EndRomEcsCopper2[];
extern uint8_t ROMAudioWaves[];
extern uint8_t EndROMAudioWaves[];
extern char    InitTxt[];      // uses VERSION + incbin builddate.i

// ProTracker labels from new initcode.s (need _Xxx:: aliases there)
extern uint8_t MT_Init[];
extern uint8_t MT_End[];
extern uint8_t MT_Music[];
extern uint8_t mt_MasterVol[];
extern uint8_t mt_END[];

// Shared strings from data.s (need _Xxx:: aliases there)
extern char YES[];
extern char NO[];
extern char DETECTED[];
extern char SFAILED[];
extern char NewLineTxt[];
extern char KB[];
extern char MinusTxt[];

// Keymap from generic.s (already has _EnglishKey:: alias)
extern uint8_t EnglishKey[];

// Menus array from menus.c
extern const char **Menus[];

// Struct returned (via pointer in a4) by callDetectMemory
typedef struct {
    uint32_t blocks;
    void    *start;
    void    *end;
} DetectResult;

// asm wrappers in new initcode.s
extern void callDetectMemory(uint32_t randVal  __asm("d0"),
                             void *scanStart   __asm("a1"),
                             void *scanEnd     __asm("a2"),
                             DetectResult *out __asm("a4"));
extern void callDetectCPU(void);

// C function accessed via asm wrapper RealLoopbacktest in new initcode.s
int realLoopbackTest(char testChar __asm("d0"));

#define INITBAUD  373
#define STACKSIZE 16384
#define RAMUSAGE  ((uint32_t)(sizeof(GlobalVars) + STACKSIZE + sizeof(Chipmemstuff) + 4096))

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

static void printDetected(uint32_t s, uint32_t e)
{
    print("  - Fastmem found between: $", CYAN);
    print(binHex(s) + 1, CYAN);
    print(" - $", CYAN);
    print(binHex(e) + 1, CYAN);
    print(NewLineTxt, CYAN);
}

static void fixBitplane(uint16_t *cop, void **bplList)
{
    uint32_t ptr;
    while ((ptr = (uint32_t)*bplList++) != 0) {
        cop[3] = (uint16_t)(ptr & 0xffffu);  // PTL at copper entry +6
        cop[1] = (uint16_t)(ptr >> 16);      // PTH at copper entry +2
        cop += 4;                             // advance 8 bytes
    }
}

static void initStuff(void)
{
    sendSerial("   - Do final Bitplanedata in Menu Copperlist\r\n");
    void **bplList = &globals->Bpl1Ptr;
    uint32_t offset = (uint32_t)MenuBplPnt - (uint32_t)RomMenuCopper;
    uint16_t *copperPos = (uint16_t *)((uint8_t *)globals->MenuCopper + offset);
    fixBitplane(copperPos, bplList);
    globals->SCRNMODE |= 0x20u;   // set bit 5 = PAL mode
}

static void detFastMem(uint32_t randVal)
{
    DetectResult r;
    globals->FastMem = 0;

    print("\nChecking if a 24 Bit address cpu is used: ", WHITE);

    int bppc      = (*(volatile uint32_t *)0xf00090 == 0x20505043u); // " PPC"
    int has32     = 0;
    int skipNobppc = 0;  // 1 = memory found in 32-bit range, skip nobppc detect

    if (!bppc) {
        // Probe for 32-bit addressing (3 attempts at >16MB addresses)
        *(volatile uint32_t *)0x700 = 0x4e4f4e45u;    // "NONE"
        *(volatile uint32_t *)0x40000700 = 0x32344254u; // "24BT"
        if (*(volatile uint32_t *)0x700 != 0x32344254u) has32 = 1;

        *(volatile uint32_t *)0x700 = 0x4e4f4e45u;
        *(volatile uint32_t *)0x2000700 = 0x32344254u;
        if (*(volatile uint32_t *)0x700 != 0x32344254u) has32 = 1;

        *(volatile uint32_t *)0x700 = 0x4e4f4e45u;
        *(volatile uint32_t *)0x4000700 = 0x32344254u;
        if (*(volatile uint32_t *)0x700 != 0x32344254u) has32 = 1;

        if (has32) {
            print(NO, RED);
            print(NewLineTxt, RED);

            // A3000/A4000 motherboard memory ($1M - $128M)
            print(" - Checking for A3000/A4000 Motherboardmemory\n", WHITE);
            randVal ^= 0x01110000u;
            callDetectMemory(randVal, (void *)0x1000000, (void *)0x7ffffff, &r);
            globals->FastMem += r.blocks;   // add even if zero
            if (r.start != 0) {
                globals->FastStart = r.start;
                globals->FastEnd   = r.end;
                printDetected((uint32_t)r.start, (uint32_t)r.end);
            }

            // A3000/A4000 CPU board memory ($128M - $384M)
            print(" - Checking for CPU-Board Memory (most A3k/A4k)\n", WHITE);
            randVal ^= 0x01110000u;
            callDetectMemory(randVal, (void *)0x8000000, (void *)0x17ffffff, &r);
            if (r.start != 0) {
                globals->FastMem  += r.blocks;
                globals->FastStart = r.start;
                globals->FastEnd   = r.end;
                printDetected((uint32_t)r.start, (uint32_t)r.end);
            }

            if (globals->FastStart != 0) {
                // Memory found in 32-bit range — skip nobppc, go to 24-bit check
                skipNobppc = 1;
            } else {
                // No 32-bit memory yet — try A1200 CPU board (via nobppc)
                print(" - Checking for CPU-Board Memory (most A1200)\n"
                      "    (WILL crash with A3640/A3660 and Maprom on)\n", WHITE);
                randVal ^= 0x01010000u;
            }
        } else {
            // 24-bit CPU
            print(YES, GREEN);
            print(NewLineTxt, GREEN);
        }
    } else {
        // BlizzardPPC: scan a smaller range to avoid crashing the 68k
        print("   - BPPC Found, detecting in a smaller memoryarea\n", CYAN);
    }

    // Detect large RAM ($40M+) — skipped if 32-bit memory was already found
    if (!skipNobppc) {
        randVal ^= 0x11010000u;
        callDetectMemory(randVal, (void *)0x40000000, (void *)0xee000000, &r);
        if (r.start != 0) {
            globals->FastStart = r.start;
            globals->FastEnd   = r.end;
            globals->FastMem  += r.blocks;
            printDetected((uint32_t)r.start, (uint32_t)r.end);
        }
    }

    // 24-bit non-autoconfig area ($2M - $10M)
    print(" - Checking for Memory in 24 Bit area (NON AUTOCONFIG)\n", WHITE);
    randVal ^= 0x10010000u;
    callDetectMemory(randVal, (void *)0x200000, (void *)0x9fffff, &r);
    if (r.start != 0) {
        if (globals->FastStart == 0) {
            globals->FastStart = r.start;
            globals->FastEnd   = r.end;
        }
        globals->FastMem += r.blocks;
        printDetected((uint32_t)r.start, (uint32_t)r.end);
    }

    // Ranger / Fakefast area ($C00000 - $C80000)
    print(" - Checking for Memory in Ranger or Fakefast area\n", WHITE);
    randVal ^= 0x10010000u;
    randVal ^= 0x10110000u;
    callDetectMemory(randVal, (void *)0xc00000, (void *)0xc80000, &r);
    if (r.start != 0) {
        if (globals->FastStart == 0) {
            globals->FastStart = r.start;
            globals->FastEnd   = r.end;
        }
        globals->FastMem += r.blocks;
        printDetected((uint32_t)r.start, (uint32_t)r.end);
    }
}

// ---------------------------------------------------------------------------
// Exported functions
// ---------------------------------------------------------------------------

int realLoopbackTest(char testChar __asm("d0"))
{
    // Serial port setup
    *(volatile uint16_t *)0xdff09a = 0x4000;   // INTENA: disable all
    *(volatile uint16_t *)0xdff032 = INITBAUD;  // SERPER: 9600 baud
    *(volatile uint8_t  *)0xbfd000 = 0x4f;      // CIAA PRA: DTR high
    *(volatile uint16_t *)0xdff09a = 0x0801;    // INTENA: enable serial
    *(volatile uint16_t *)0xdff09c = 0x0801;    // INTREQ: clear serial

    // Wait for TBE (transmit buffer empty, bit 13 of SERDATR)
    uint32_t timeout = 10000;
    while (1) {
        (void)*(volatile uint8_t *)0xbfe001;    // slow CIA-A read for timing
        timeout--;
        if (timeout == 0) break;
        if (*(volatile uint16_t *)0xdff018 & (1u << 13)) break;
    }

    // Send the test character (bit 8 = stop bit)
    *(volatile uint16_t *)0xdff030 = 0x0100u | (uint8_t)testChar; // SERDAT
    *(volatile uint16_t *)0xdff09c = 0x0001u;  // INTREQ: clear TBE

    // Wait for RBFULL (receive buffer full, bit 14 of SERDATR) or timeout
    timeout = 10000;
    uint16_t received = 0;
    while (1) {
        (void)*(volatile uint8_t *)0xbfe001;
        if (timeout == 0) { received = 0; break; }
        timeout--;
        received = *(volatile uint16_t *)0xdff018;
        if (received & (1u << 14)) break;
    }

    // Return 1 if received char matches sent char, else 0
    return ((uint8_t)received == (uint8_t)testChar) ? 1 : 0;
}

void clearSerial(void)
{
    for (int i = 0; i < 2; i++) {
        *(volatile uint16_t *)0xdff09a = 0x4000;
        *(volatile uint16_t *)0xdff032 = INITBAUD;
        *(volatile uint8_t  *)0xbfd000 = 0x4f;
        *(volatile uint16_t *)0xdff09a = 0x0801;
        *(volatile uint16_t *)0xdff09c = 0x0801;

        uint32_t timeout = 10000;
        while (1) {
            (void)*(volatile uint8_t *)0xbfe001;
            if (timeout == 0) break;
            timeout--;
            if (*(volatile uint16_t *)0xdff018 & (1u << 14)) break;
        }
    }
}

void clearScreenNoSerial(void)
{
    if (!globals->NoDraw) {
        uint32_t *p0 = (uint32_t *)globals->Bpl1Ptr;
        uint32_t *p1 = (uint32_t *)globals->Bpl2Ptr;
        uint32_t *p2 = (uint32_t *)globals->Bpl3Ptr;
        uint32_t count = globals->BPLSIZE >> 2;
        for (uint32_t i = 0; i <= count; i++) {
            *p0++ = 0;
            *p1++ = 0;
            *p2++ = 0;
        }
    }
    globals->Xpos = 0;
    globals->Ypos = 0;
}

void setMenuCopper(void)
{
    sendSerial("    Set Start of copper (COP1LCH $dff080): ");
    *(volatile uint32_t *)0xdff080 = (uint32_t)globals->MenuCopper;
    sendSerial("Done\r\n");

    sendSerial("    Starting Copper (COPJMP1 $dff088): ");
    (void)*(volatile uint16_t *)0xdff088;   // read COPJMP1 to trigger copper restart
    sendSerial("Done\r\n");

    sendSerial("    Set all DMA enablebits (DMACON $dff096) to Enabled: ");
    *(volatile uint16_t *)0xdff096 = 0x87e0u;
    sendSerial("Done\r\n");

    sendSerial("    Set Beam Conter control register to 32 (PAL) (BEAMCON0 $dff1dc): ");
    *(volatile uint16_t *)0xdff1dc = 32;
    sendSerial("Done\r\n");
}

void copyToChip(void)
{
    Chipmemstuff *chip = (Chipmemstuff *)globals->ChipmemBlock;

    sendSerial(" - Setting up Chipmemdata\r\n");

    // Menu copper list
    sendSerial("   - Copy Menu Copperlist from ROM to memory at: ");
    globals->MenuCopper = chip->MenuCopperList;
    copyMem(RomMenuCopper,
            (uint32_t)(EndRomMenuCopper - RomMenuCopper),
            chip->MenuCopperList);
    sendSerial(binHex((uint32_t)globals->MenuCopper));
    sendSerial(NewLineTxt);

    // ECS copper list
    sendSerial("   - Copy ECS TestCopperlist from ROM to memory at: ");
    globals->ECSCopper = chip->ECSCopperList;
    copyMem(RomEcsCopper,
            (uint32_t)(EndRomEcsCopper - RomEcsCopper),
            chip->ECSCopperList);
    sendSerial(binHex((uint32_t)globals->ECSCopper));
    sendSerial(NewLineTxt);

    // ECS copper list 2
    sendSerial("   - Copy ECS testCopperlist2 from ROM to memory at: ");
    globals->ECSCopper2 = chip->ECSCopper2List;
    copyMem(RomEcsCopper2,
            (uint32_t)(EndRomEcsCopper2 - RomEcsCopper2),
            chip->ECSCopper2List);
    sendSerial(binHex((uint32_t)globals->ECSCopper2));
    sendSerial(NewLineTxt);

    // Fill sprite pointer entries in MenuCopper with DummySprite address
    sendSerial("   - Fixing Bitplane Pointers etc in Menu Copperlist\r\n");
    {
        uint16_t *cop = (uint16_t *)globals->MenuCopper;
        uint32_t dsptr = (uint32_t)globals->DummySprite;
        for (int i = 0; i < 8; i++) {
            cop[1] = (uint16_t)(dsptr >> 16);
            cop[3] = (uint16_t)(dsptr & 0xffffu);
            cop += 4;
        }
    }

    // Audio wave data
    sendSerial("   - Copy Audio Data from ROM to memory at: ");
    globals->AudioWaves = chip->AudioWaveData;
    copyMem(ROMAudioWaves,
            (uint32_t)(EndROMAudioWaves - ROMAudioWaves),
            chip->AudioWaveData);
    sendSerial(binHex((uint32_t)globals->AudioWaves));
    sendSerial(NewLineTxt);

    // ProTracker replay routine
    sendSerial("   - Copy Protracker replayroutine from ROM to memory at: ");
    void *ptBase = chip->ptplayroutine;
    globals->ptplay = ptBase;
    copyMem(MT_Init,
            (uint32_t)(mt_END - MT_Init),
            chip->ptplayroutine);
    sendSerial(binHex((uint32_t)globals->ptplay));
    sendSerial(NewLineTxt);

    // Store offsets to ProTracker entry points within the copied code
    globals->AudioModInit  = ptBase;
    globals->AudioModEnd   = (uint8_t *)ptBase + (MT_End      - MT_Init);
    globals->AudioModMusic = (uint8_t *)ptBase + (MT_Music    - MT_Init);
    globals->AudioModMVol  = (uint8_t *)ptBase + (mt_MasterVol - MT_Init);
}

void initCode(void)
{
    // -----------------------------------------------------------------------
    // Parse startup flags set by earlystart.s
    // -----------------------------------------------------------------------
    sendSerial("\n\n---- Setting up startupflags depending detections during startup\r\n");

    uint32_t sf = globals->startupflags;

    // Button stuck detection: compare button state at power-on (bits 29:24)
    // with state after meminit (bits 19:14).  Bits that stayed the same = stuck.
    uint32_t powerOnBtns   = (sf & 0x3f000000u) >> 24;
    uint32_t afterInitBtns = (sf & 0x000fc000u) >> 14;
    uint32_t notStuck      = afterInitBtns ^ powerOnBtns;
    uint32_t stuckBits     = powerOnBtns ^ notStuck;   // = afterInitBtns

    if (stuckBits & (1u << 5)) {
        globals->STUCKP1LMB = 1;
        sendSerial("P1LMB Stuck at boot and being disabled\r\n");
    }
    if (stuckBits & (1u << 4)) {
        globals->STUCKP2LMB = 1;
        sendSerial("P2LMB Stuck at boot and being disabled\r\n");
    }
    if (stuckBits & (1u << 3)) {
        globals->STUCKP1RMB = 1;
        sendSerial("P1RMB Stuck at boot and being disabled\r\n");
    }
    if (stuckBits & (1u << 2)) {
        globals->STUCKP2RMB = 1;
        sendSerial("P2RMB Stuck at boot and being disabled\r\n");
    }
    if (stuckBits & (1u << 1)) {
        globals->STUCKP1MMB = 1;
        sendSerial("P1MMB Stuck at boot and being disabled\r\n");
    }
    if (stuckBits & (1u << 0)) {
        globals->STUCKP2MMB = 1;
        sendSerial("P2MMB Stuck at boot and being disabled\r\n");
    }

    if (sf & (1u << 31)) {
        globals->NoSerial = 1;
        sendSerial("Serial out is disabled\r\n");
    }
    if (sf & (1u << 30)) {
        globals->RomAdrErr = 1;
        sendSerial("ROM Adressing errors during boot\r\n");
    }
    if (sf & (1u << 23)) {
        globals->ChipBitErr = 1;
        sendSerial("Biterrors in chipmem during boot\r\n");
    }
    if (sf & (1u << 22)) {
        globals->ChipAdrErr = 1;
        sendSerial("Addressserrors in chipmem during boot\r\n");
    }
    if (sf & (1u << 21)) {
        globals->NotEnoughChip = 1;
        globals->ChipStart = 0;
        globals->ChipEnd   = 0;
        sendSerial("Not enough Chipmem during boot\r\n");
    }
    if (sf & (1u << 20)) {
        globals->ScanFastMem = 1;
        sendSerial("Fastmemscanning done during boot\r\n");
    }
    if (sf & (1u << 13)) {
        globals->FastFound = 1;
        sendSerial("Fastmem found during early init\r\n");
    }
    if (sf & (1u << 12)) {
        globals->NoDraw = 1;
        sendSerial("No Printing on screen (NoDraw) being done\r\n");
    }
    if (sf & (1u << 11)) {
        globals->StuckMouse = 1;
        // (StuckMouseTxt deliberately omitted — matches asm comment)
    }
    if (sf & (1u << 10)) {
        globals->MemAt400 = 1;
        sendSerial("\nWe had memory at $400 making IRQ, CPU Detection etc more reliable\r\n");
    }
    if (sf & (1u << 9)) {
        globals->OVLErr = 1;
        sendSerial("OVL Error, meaning ROM is mirrored to $0 making chipmem at romsize not available there\r\n");
    }
    if (sf & (1u << 8)) {
        globals->WorkOrder = 1;
        sendSerial("Reverse workorder enabled (using beginning of block instead of ending)\r\n");
    }

    // -----------------------------------------------------------------------
    // Calculate chip memory size and workspace pointers
    // -----------------------------------------------------------------------
    uint32_t chipStart = (uint32_t)globals->ChipStart;
    if (chipStart == 0x400) chipStart = 0;
    globals->TotalChip = (uint32_t)globals->ChipEnd + 1 - chipStart;

    if (!globals->NotEnoughChip) {
        if (!globals->WorkOrder) {
            globals->GetChipAddr        = globals->ChipStart;
            uint32_t top                = (uint32_t)globals - 16;
            globals->ChipUnreservedAddr = (void *)top;
            globals->ChipUnreserved     = (top - (uint32_t)globals->ChipStart) & ~1u;
        } else {
            uint32_t top                = (uint32_t)globals->ChipEnd & ~1u;
            globals->ChipUnreservedAddr = (void *)top;
            globals->ChipUnreserved     = top - (uint32_t)globals->ChipStart;
        }
    }

    sendSerial("\r\n---- Startupflags done\n\n");

    // -----------------------------------------------------------------------
    // Hardware register snapshot, base addresses
    // -----------------------------------------------------------------------
    getHWReg(globals);

    globals->BaseStart = globals;
    globals->BaseEnd   = (uint8_t *)globals + RAMUSAGE;

    // -----------------------------------------------------------------------
    // Serial loopback detection
    // -----------------------------------------------------------------------
    sendSerial("\r\nTesting if serial loopbackadapter is installed: ");
    clearSerial();
    clearSerial();

    int loopCount = 0;
    loopCount += realLoopbackTest('<');
    loopCount += realLoopbackTest('>');

    if (loopCount > 0) {
        globals->SerialSpeed = 3;
        globals->LoopB       = 1;
        sendSerial(" DETECTED\r\n");
    } else {
        sendSerial(" NOT DETECTED\r\n");
    }

    // -----------------------------------------------------------------------
    // Set serial speed
    // -----------------------------------------------------------------------
    *(volatile uint16_t *)0xdff180 = 0xaaa;

    if (!globals->NoSerial && !globals->LoopB) {
        globals->SerialSpeed = 2;           // 38400 baud
    } else if (globals->LoopB) {
        globals->SerialSpeed = 2;
        globals->NoSerial    = 1;           // loopback = disable serial output
    } else {
        globals->SerialSpeed = 0;
    }

    // -----------------------------------------------------------------------
    // Detect Agnus/Alice chip
    // -----------------------------------------------------------------------
    sendSerial("Detecting agnuschip: ");

    static const uint8_t agnusID[] =
        { 0, 0x10, 0x20, 0x30, 0x22, 0x31, 0x22, 0x32, 0x23, 0x33, 0xff };
    static const char agnusTxt[][15] = {
        "8367/8371 PAL ",
        "8361/8370 NTSC",
        "8372 R4 PAL   ",
        "8372 R4 NTSC  ",
        "8372 R5 PAL   ",
        "8372 R5 NTSC  ",
        "8374 R2 PAL   ",
        "8374 R2 NTSC  ",
        "8374 R3 PAL   ",
        "8374 R3 NTSC  ",
    };

    uint8_t agnusRaw = (uint8_t)(*(volatile uint16_t *)0xdff004 >> 8);
    globals->AGNUS = agnusRaw;
    int agnusIdx = 0;
    while (agnusID[agnusIdx] != 0xff && agnusID[agnusIdx] != agnusRaw)
        agnusIdx++;
    if (agnusID[agnusIdx] == 0xff) agnusIdx--;   // unknown: use last entry
    globals->NTSC = agnusIdx & 1;
    sendSerial((char *)agnusTxt[agnusIdx]);
    sendSerial(NewLineTxt);

    // -----------------------------------------------------------------------
    // Detect working raster
    // -----------------------------------------------------------------------
    sendSerial("Detecting if we have a working raster: ");
    uint8_t raster0 = *(volatile uint8_t *)0xdff006;
    waitShort(); waitShort(); waitShort();
    uint8_t raster1 = *(volatile uint8_t *)0xdff006;
    if (raster0 != raster1) {
        globals->RASTER = 1;
        sendSerial(DETECTED);
    } else {
        globals->RASTER = 0;
        sendSerial(SFAILED);
    }
    sendSerial(NewLineTxt);

    // -----------------------------------------------------------------------
    // Default keymap, chip/fast memory sizes via serial
    // -----------------------------------------------------------------------
    globals->keymap = EnglishKey;
    *(volatile uint16_t *)0xdff180 = 0x999;

    sendSerial("Detected Chipmem: ");
    sendSerial(bindec((int32_t)(globals->TotalChip >> 10)));
    sendSerial(KB);
    sendSerial(NewLineTxt);

    sendSerial("Detected Motherboard Fastmem (not reliable result): ");
    sendSerial(bindec((int32_t)globals->BootMBFastmem));
    sendSerial(bindec((int32_t)globals->BootMBFastmem));  // intentional double (matches asm)
    sendSerial(KB);
    sendSerial(NewLineTxt);

    *(volatile uint16_t *)0xdff180 = 0x888;
    sendSerial(" - Doing Initstuff\r\n");

    // -----------------------------------------------------------------------
    // Set up bitplane structure in the BPL block
    // -----------------------------------------------------------------------
    {
        uint8_t *a5      = (uint8_t *)globals->BPL;
        uint32_t bplsize = globals->BPLSIZE;

        *(uint32_t *)a5 = 0x42504c31u;   // "BPL1" marker
        a5 += 4;
        globals->Bpl1Ptr = a5;
        a5 += bplsize;

        *(uint32_t *)a5 = 0x42504c32u;   // "BPL2" marker
        a5 += 4;
        globals->Bpl2Ptr = a5;
        a5 += bplsize;

        *(uint32_t *)a5 = 0x42504c33u;   // "BPL3" marker
        a5 += 4;
        globals->Bpl3Ptr = a5;
        a5 += bplsize;

        *(uint32_t *)a5 = 0x454e4421u;   // "END!" marker
        a5 += 4;
        *(uint32_t *)a5 = 0;             // dummysprite (cleared)
        globals->DummySprite = a5;
        a5 += 4;
    }

    copyToChip();

    *(volatile uint16_t *)0xdff180 = 0x777;
    sendSerial("   - Bitplane 1 at: $");
    sendSerial(binHex((uint32_t)globals->Bpl1Ptr) + 1);
    sendSerial(NewLineTxt);
    sendSerial("   - Bitplane 2 at: $");
    sendSerial(binHex((uint32_t)globals->Bpl2Ptr) + 1);
    sendSerial(NewLineTxt);
    sendSerial("   - Bitplane 3 at: $");
    sendSerial(binHex((uint32_t)globals->Bpl3Ptr) + 1);
    sendSerial(NewLineTxt);

    globals->BplNull = 0;
    initStuff();

    sendSerial(" - Initstuff done!\r\n\r\n");

    // -----------------------------------------------------------------------
    // Graphics + copper setup (only when NoDraw is off)
    // -----------------------------------------------------------------------
    if (!globals->NoDraw) {
        *(volatile uint16_t *)0xdff180 = 0x555;
        setMenuCopper();
        sendSerial("    Set POTGO to all OUTPUT ($FF00) (POTGO $dff034): ");
        *(volatile uint16_t *)0xdff034 = 0xff00;
        sendSerial("Done\r\n");

        *(volatile uint16_t *)0xdff180 = 0x333;
        clearScreenNoSerial();
        print(InitTxt, WHITE);
    } else {
        sendSerial("We are in a nonchip/nodraw mode. Serialoutput is all we got.\r\n"
                   "colourflash on screen is actually chars that should be printed on screen.\r\n"
                   "Just to tell user something happens\r\n\r\n");
    }

    // -----------------------------------------------------------------------
    // ROM checksum + CPU detection
    // -----------------------------------------------------------------------
    romChecksum();
    callDetectCPU();
    PrintCPU();

    // -----------------------------------------------------------------------
    // Fast memory detection
    // -----------------------------------------------------------------------
    print("\n\nChecking for fastmem\n"
          "Pressing left mousebutton will cancel detection (if hanged)\n\n", YELLOW);
    uint32_t randVal = *(volatile uint8_t *)0xdff006;
    detFastMem(randVal);

    // -----------------------------------------------------------------------
    // Print work area ranges on screen
    // -----------------------------------------------------------------------
    print("\nExtra workareas Chipmem: ", WHITE);
    print(binHex((uint32_t)globals->ChipStart), WHITE);
    print(MinusTxt, WHITE);
    print(binHex((uint32_t)globals->ChipEnd), WHITE);
    print("  Fastmem: ", WHITE);
    print(binHex((uint32_t)globals->FastStart), WHITE);
    print(MinusTxt, WHITE);
    print(binHex((uint32_t)globals->FastEnd), WHITE);

    globals->TotalFast = globals->FastMem << 6;

    // -----------------------------------------------------------------------
    // Disclaimer + serial enable prompt
    // -----------------------------------------------------------------------
    print("\n\nIF This ROM is sold, if above 10eur+hardware cost 25% MUST be donated to\n"
          "an LEGITIMATE charity of some kind, like curing cancer for example... \n"
          "If you paid more than 10Eur + Hardware + Shipping, please ask what charity you\n"
          "have supported!!!   This software is free to use. With source for VIEWING ONLY!\n"
          "Please report ANY issues. Testresults cannot be guaranteed at this moment\n\n"
          "Go to www.diagrom.com or http://github.com/ChuckyGang/DiagROM2 for information\n\n",
          GREEN);

    print("\r\nPlease read the readme.txt file in the download archive for instructions"
          "\r\nDiagROM is mainly for people with technical knowledge of the Amiga"
          "\r\nand might not be fully 'stright forward' for all - Delivered AS IS"
          "\r\n\r\nTo use serial communication please hold down ANY key now\r\n"
          "OR click the RIGHT mousebutton.\r\n", WHITE);

    // -----------------------------------------------------------------------
    // Serial timeout wait — give user a chance to press a key
    // -----------------------------------------------------------------------
    if (!globals->NoDraw && !globals->NoSerial && !globals->LoopB) {
        ClearBuffer();
        uint32_t dotCount = 0;
        for (uint32_t d7 = 1200; d7 != (uint32_t)-1; d7--) {
            *(volatile uint8_t *)0xdff181 = (uint8_t)d7;  // flash color
            uint32_t input = getInput();
            if (globals->RMB) goto serial_on;
            if (input & 4)    goto serial_on;   // key pressed
            if (input & 8)    goto serial_on;   // serial data
            if (++dotCount >= 16) {
                print(".", WHITE);
                dotCount = 0;
            }
            waitShort();
            waitShort();
        }
        sendSerial("\x1b[0m\r\nNo key pressed, disabling any serialcommunications.\r\n");
        globals->SerialSpeed = 0;
    } else if (globals->NoDraw) {
        TOGGLEPWR;
        globals->SerialSpeed = 2;
        sendSerial("We are in a nonchip/nodraw mode. Serialoutput is all we got.\r\n");
    }

serial_on:
    ClearBuffer();
    defaultVars();
    globals->Menu = (void *)Menus;
    mainMenu();
}
