#include "globalvars.h"
#include "generic.h"
#include "menus.h"
#include <exec/types.h>
#include <hardware/custom.h>
#define custom ((volatile struct Custom*)0xdff000)
void crap();

/* Forward declarations — definitions live further down in this file */
void     selectDrive(uint32_t floppyID);
void     floppyMotor(uint32_t floppyID);
void     floppySide(uint32_t floppyID);
void     floppyStepOut();
void     floppyStepIn();
uint32_t getFloppyID(void);
void     gotoZero(void);
static void drawFloppyTestLabels(void);
static void readTrackBuffer(uint32_t floppyID);
static void showBufferSector(void);
static int  findSectorInTrack(uint8_t wanted, uint8_t **dataOut);
static void decodeAndShowSector(uint8_t *dataPtr);
static void restoreFloppyTestScreen(int *driveMotor, int *driveSide, int *track0,
                                    int *oldbfe001, int *oldbfd100,
                                    int *ready, int *wprot, int *disk, int *oldTrack);

static const char DiskMenuText[] = "\002Floppy Test";
static const char DiskMenuT1[] = "1 - Select Disk: ";
static const char DiskMenuT2[] = "2 - Motor";
static const char DiskMenuT3[] = "3 - Change side";
static const char DiskMenuT4[] = "4 - Step out";
static const char DiskMenuT5[] = "5 - Step in";
static const char DiskMenuT6[] = "6 - Step out 10 tracks";
static const char DiskMenuT7[] = "7 - Step in 10 tracks";
static const char DiskMenuT8[] = "8 - Read track to buffer";
static const char DiskMenuTS[] = "S - Show sector from buffer";

static const char DiskMenuBack[] = "9 - Main menu";

static const char *DiskMenuItems[] = {
        DiskMenuText,
        DiskMenuT1,
        DiskMenuT2,
        DiskMenuT3,
        DiskMenuT4,
        DiskMenuT5,
        DiskMenuT6,
        DiskMenuT7,
        DiskMenuT8,
       DiskMenuTS,
       DiskMenuBack,
       NULL
};

const char **DiskTestC[] = {
       DiskMenuItems,  // 0 — only slot used (MenuNumber is always 0)
};



void floppyTestC(void)
{
    int temp=0;     //tempshit
    int driveMotor=-1;
    int driveSide=-1;
    int track0=-1;
    int oldbfe001=0;
    int oldbfd100=0;
    int ready=-1;
    int wprot=-1;
    int disk=-1;
    int oldTrack=-1;
    uint32_t floppyID = 0;
    char     addOneStr[12];
    MenuVar  diskMenuVars[9] = {0};

    addOneStr[0] = '0';
    addOneStr[1] = '\0';

    diskMenuVars[0].str   = addOneStr;

    initScreen();

    /* Allocate the track buffer once per ROM session. Chip RAM only — disk DMA
     * cannot reach fast RAM. 12980 bytes covers one DD MFM track. */
    if (!globals->trackbuff) {
        uint32_t addr = getChip(12980);
        if (addr == 0 || addr == 1) {
            setPos(0, 0);
            print("ERROR: Not enough chip RAM for track buffer", RED);
            setPos(0, 2);
            print("Press any key/mouse to return", WHITE);
            WaitButton();
            mainMenu();
            return;
        }
        globals->trackbuff = (void *)(uintptr_t)addr;
    }

    globals->Menu         = (void *)DiskTestC;
    globals->MenuVariable = (void *)diskMenuVars;
    globals->MenuNumber   = 0;
    globals->PrintMenuFlag = 1;
    int bah = 0;

    drawFloppyTestLabels();
    selectDrive(0);
    for (;;) {
        // --- per-iteration hardware work goes here ---
    if (globals->DriveOK==1)
    {
        diskMenuVars[0].color = GREEN;
    }
    else
    {
        diskMenuVars[0].color = RED;
    }

        printMenu();
        getInput();
//        waitLong();
//        setPos(0,1);
//        print(bindec(globals->MenuPos),WHITE);
 //              setPos(40,1);
 //       print(bindec(bah++),WHITE);
  //      updateFloppyData();
        uint8_t ch = globals->GetCharData;

        /* Mouse click / Enter → map menu position to its key. */
        if (globals->LMB || globals->RMB || ch == 0x0a) {
            static const uint8_t posToKey[] = {
                '1','2','3','4','5','6','7','8','S','9'
            };
            if (globals->MenuPos < (uint8_t)(sizeof(posToKey))) {
                ch = posToKey[globals->MenuPos];
            }
        }
        stepToTrack(floppyID);
        int handled = 1;
        switch (ch) {
            case '1':
                waitReleased();
                floppyID = (floppyID + 1) & 3;
                {
                    char *s = bindec(floppyID);
                    char *d = addOneStr;
                    while ((*d++ = *s++) != '\0') {}
                }
                selectDrive(floppyID);
                break;

            case '2':
                waitReleased();
                floppyMotor(floppyID);
                break;

            case '3':
                waitReleased();
                floppySide(floppyID);
                break;

            case '4':
                waitReleased();
                floppyStepOut();
                break;

            case '5':
                waitReleased();
                floppyStepIn();
                break;

            case '6':
                waitReleased();
                floppyStep10Out();
                break;

            case '7':
                waitReleased();
                floppyStep10In();
                break;

            case '8':
                waitReleased();
                readTrackBuffer(floppyID);
                restoreFloppyTestScreen(&driveMotor, &driveSide, &track0,
                                        &oldbfe001, &oldbfd100,
                                        &ready, &wprot, &disk, &oldTrack);
                break;

            case 'S':
            case 's':
                waitReleased();
                showBufferSector();
                restoreFloppyTestScreen(&driveMotor, &driveSide, &track0,
                                        &oldbfe001, &oldbfd100,
                                        &ready, &wprot, &disk, &oldTrack);
                break;

            case '9':
                waitReleased();
                mainMenu();
                return;

            default:
                handled = 0;
                break;
        }

        if (handled) {
            globals->Menu          = (void *)DiskTestC;
            globals->MenuNumber    = 0;
            globals->MenuVariable  = (void *)diskMenuVars;
            globals->PrintMenuFlag = 2;     // 2 = redraw but keep MenuPos
        }
    uint32_t id = 0;
  

 //   print("Drive ID: ",WHITE);
 //   print(binHex(id),WHITE);
 //   setPos(52,5);
 //   print(binHex(id),WHITE);

 //   setPos(13,2);
  //  print(bindec(globals->TrackNo),GREEN);

  //  setPos(24,2);
 //   print("Side",GREEN);

    if(oldTrack!=globals->TrackNo)
    {
        setPos(13,2);
        print("     ",GREEN);
        setPos(13,2);
        print(binDec(globals->TrackNo),GREEN);
        oldTrack = globals->TrackNo;
    }

    temp = CIAB->ciaprb & CIAF_DSKSIDE;
    if(driveSide != temp)
    {
   
        setPos(24,2);
        if (!(CIAB->ciaprb & CIAF_DSKSIDE)) {
            print("UPPER",GREEN);
        }
        else
        {
            print("LOWER",CYAN);
        }
        driveSide = temp;
    }


    temp = CIAB->ciaprb & CIAF_DSKMOTOR;
    if(driveMotor != temp)
    {
        setPos(39,2);
        print("   ",WHITE);
        setPos(39,2);
        if (!(CIAB->ciaprb & CIAF_DSKMOTOR)) {
            print("ON",GREEN);
        }
        else
        {
            print("OFF",CYAN);
        }
        driveMotor = temp;
    }

    /* Drive status lines (/TRK0, /RDY, /WPRO, /CHNG) are only driven
     * while the drive is selected. Pulse SEL low to sample them. */
    {
        uint8_t selBit  = 1 << (floppyID + 3);
        uint8_t selMask = CIAF_DSKSEL0 | CIAF_DSKSEL1 | CIAF_DSKSEL2 | CIAF_DSKSEL3;
        CIAB->ciaprb = (CIAB->ciaprb | selMask) & ~selBit;  // select chosen drive
        (void)*(volatile uint16_t *)0xdff1fe;               // tiny settle delay
        temp = CIAA->ciapra;                                // sample once
        CIAB->ciaprb |= selMask;                            // deselect
    }

    int wp = temp & CIAF_DSKPROT;
    if(wprot != wp)
    {
        setPos(58,2);
        print("   ",WHITE);
        setPos(58,2);
        if (!wp) {
            print("YES",GREEN);
        }
        else
        {
            print("NO",CYAN);
        }
        wprot = wp;
    }


    int dsk = temp & CIAF_DSKCHANGE;
    if(disk != dsk)
    {
        setPos(70,2);
        print("   ",WHITE);
        setPos(70,2);
        if (dsk) {                  // /CHNG high = disk present
            print("YES",GREEN);
        }
        else
        {
            print("NO",CYAN);       // /CHNG low = removed / empty
        }
        disk = dsk;
    }



    int rdy = temp & CIAF_DSKRDY;
    if(ready != rdy)
    {
        setPos(13,3);
        print("   ",WHITE);
        setPos(13,3);
        if (!rdy) {
            print("YES",GREEN);
        }
        else
        {
            print("NO",CYAN);
        }
        ready = rdy;
    }

    int tk0 = temp & CIAF_DSKTRACK0;
    if(track0 != tk0)
    {
        setPos(26,3);
        print("   ",WHITE);
        setPos(26,3);
        if (!tk0) {
            print("YES",GREEN);
        }
        else
        {
            print("NO",CYAN);
        }
        track0 = tk0;
    }

    if(oldbfe001!=temp)
    {
    setPos(41,3);
    print(binStringByte(temp),GREEN);   // $bfe001
    oldbfe001=temp;
    }
    temp = CIAB->ciaprb;
    if(oldbfd100!=temp)
    {
        setPos(61,3);
        print(binStringByte(temp),GREEN);   // $bfd100
        oldbfd100=temp;
    }



//    setPos(58,2);
//    print("WP",GREEN);

 //   setPos(70,2);
 //   print("IN",GREEN);

  //  setPos(13,3);
  //  print("RDY",GREEN);

   // setPos(26,3);
  //  print("TR0",GREEN);

    }
}


void stepToTrack(uint32_t floppyID)              // Steps to correct track if needed
{
    int wanted=globals->WantedTrackNo;
    int current=globals->TrackNo;
    if(wanted != current)
    {
        if(wanted>current)
        {
            globals->TrackNo++;
            int8_t selBit  = 1 << (floppyID + 3);
            uint8_t selMask = CIAF_DSKSEL0 | CIAF_DSKSEL1 | CIAF_DSKSEL2 | CIAF_DSKSEL3;

            CIAB->ciaprb = (CIAB->ciaprb | selMask) & ~selBit;   // select chosen drive
            CIAB->ciaprb &= ~CIAF_DSKDIREC;                      // DIR=0 → step to higher track number
            (void)*(volatile uint16_t *)0xdff1fe;                // settle
            CIAB->ciaprb &= ~CIAF_DSKSTEP;                       // step pulse low
            (void)*(volatile uint16_t *)0xdff1fe;
            CIAB->ciaprb |= CIAF_DSKSTEP;                        // step pulse high → drive steps one track
            waitLong();                                          // head settle (3ms minimum on real drives)
            CIAB->ciaprb |= selMask;                             // deselect

        }
        else
        {
            uint8_t selBit  = 1 << (floppyID + 3);
            uint8_t selMask = CIAF_DSKSEL0 | CIAF_DSKSEL1 | CIAF_DSKSEL2 | CIAF_DSKSEL3;

            CIAB->ciaprb = (CIAB->ciaprb | selMask) & ~selBit;   // select chosen drive
            CIAB->ciaprb |= CIAF_DSKDIREC;                       // DIR=1 → step toward track 0
            (void)*(volatile uint16_t *)0xdff1fe;                // settle
            CIAB->ciaprb &= ~CIAF_DSKSTEP;                       // step pulse low
            (void)*(volatile uint16_t *)0xdff1fe;
            CIAB->ciaprb |= CIAF_DSKSTEP;                        // step pulse high
            waitLong();                                          // head settle
            CIAB->ciaprb |= selMask;                             // deselect

            globals->TrackNo--;
        }
                    waitLong();     
                    waitLong();     
                    waitLong();     
                    waitLong();     
    }

}

uint32_t getFloppyID()
{
    uint32_t id=0;
      uint8_t mask = CIAF_DSKSEL0; //; << floppyID;
    unsigned int i;

    CIAB->ciaprb |= 0xf8;  /* MTR high, all drives deselected */
    CIAB->ciaprb &= 0x7f;  /* MTR low on port */
    CIAB->ciaprb &= ~mask; /* SEL low  — latches MOTOR-ON into drive */
    CIAB->ciaprb |= mask;  /* SEL high */
    CIAB->ciaprb |= 0x80;  /* MTR high on port */
    CIAB->ciaprb &= ~mask; /* SEL low  — latches MOTOR-OFF (arms ID mode) */
    CIAB->ciaprb |= mask;  /* SEL high */

    uint8_t bits[32];
    for (i = 0; i < 32; i++) {
        CIAB->ciaprb &= ~mask;
        bits[i] = (CIAA->ciapra >> 5) & 1;
        id = (id << 1) | bits[i];
        CIAB->ciaprb |= mask;
    }
//    for (i = 0; i < 32; i++) {
//        setPos(10 + i, 6);
//        print(bits[i] ? "1" : "0", YELLOW);
//    }
    return id;
}

void floppyMotor(uint32_t floppyID)
{
    uint8_t selBit  = 1 << (floppyID + 3);
    uint8_t selMask = CIAF_DSKSEL0 | CIAF_DSKSEL1 | CIAF_DSKSEL2 | CIAF_DSKSEL3;

    CIAB->ciaprb |= selMask;            // deselect all (SEL high)
    waitLong();
    CIAB->ciaprb ^= CIAF_DSKMOTOR;      // toggle motor bit while drive is deselected
    waitLong();
    CIAB->ciaprb &= ~selBit;            // SEL low → drive latches the new MTR state
    waitLong();
    CIAB->ciaprb |= selMask;            // deselect again
}

void floppySide(uint32_t floppyID)
{
    uint8_t selBit  = 1 << (floppyID + 3);
    uint8_t selMask = CIAF_DSKSEL0 | CIAF_DSKSEL1 | CIAF_DSKSEL2 | CIAF_DSKSEL3;

    CIAB->ciaprb |= selMask;            // deselect all (SEL high)
    waitLong();
    CIAB->ciaprb ^= CIAF_DSKSIDE;      // toggle motor bit while drive is deselected
    waitLong();
    CIAB->ciaprb &= ~selBit;            // SEL low → drive latches the new MTR state
    waitLong();
    CIAB->ciaprb |= selMask;            // deselect again
}

void floppyStepOut()
{
    if (globals->WantedTrackNo >= 79) return;  // 80 tracks total (0..79)
    globals->WantedTrackNo++;
}

void floppyStep10Out()
{
    if (globals->WantedTrackNo >= 79) return;  // 80 tracks total (0..79)
    globals->WantedTrackNo+=10;
    if(globals->WantedTrackNo>=79)
    {
        globals->WantedTrackNo=79;
    }
}


void floppyStepIn()
{
    if (globals->WantedTrackNo == 0) return;   // already at track 0
    globals->WantedTrackNo--;
}

void floppyStep10In()
{
    if (globals->WantedTrackNo == 0) return;   // already at track 0

    if(globals->WantedTrackNo<=10)
    {
        globals->WantedTrackNo=0;
    }
    else
        globals->WantedTrackNo-=10;
}

void gotoZero(void)
{
    globals->DriveOK = 1;

    for (int i = 0; i < 85; i++) {
        waitLong();

        if (!(CIAA->ciapra & CIAF_DSKTRACK0)) {
            /* Track 0 sensor active (signal is low = bit clear): we are at track 0 */
            globals->TrackNo = 0;
            globals->WantedTrackNo = 0;
            return;
        }
        *(volatile uint16_t *)0xdff180 = 0x00ff;    /* flash background while stepping */
        
        CIAB->ciaprb |=  CIAF_DSKDIREC;             /* direction: outward (toward track 0) */
        CIAB->ciaprb &= ~CIAF_DSKSTEP;              /* step pulse low */
        (void)*(volatile uint16_t *)0xdff1fe;        /* small delay */
        CIAB->ciaprb |=  CIAF_DSKSTEP;              /* step pulse high */
    }
    globals->DriveOK = 0;   /* exhausted all steps: no drive detected */
}

void selectDrive(uint32_t floppyID)
{
    //print("Select drive:",GREEN);
    //print(bindec(floppyID),BLUE);
    CIAB->ciaprb |=  CIAF_DSKSEL0 | CIAF_DSKSEL1 | CIAF_DSKSEL2 | CIAF_DSKSEL3;
    CIAB->ciaprb &= ~(1 << (floppyID + 3));
    gotoZero();
    setPos(52,5);
    print(binHex(getFloppyID()),GREEN);
}

void updateFloppyData()
{
    setPos(41,3);   // Print bfe001 value
    print(binStringByte(CIAA->ciapra),PURPLE);
    setPos(61,3);   // Print bfd100 value
    print(binStringByte(CIAB->ciaprb),PURPLE);
}

// ===========================================================================
// Everything below is the HDD-controller half of this file (Gayle IDE,
// A4000 IDE, A3000/A3000T SCSI, A4000T SCSI). The DeMoN 256KB cartridge
// build excludes it wholesale (~50KB) - none of these controllers can exist
// on an A500-class cartridge host - and provides stubs at the bottom of the
// file instead. The floppy half above stays in every build.
// ===========================================================================
#ifndef TARGET_DEMON

// ---------------------------------------------------------------------------
// Known controllers — single source of truth for both menus
// ---------------------------------------------------------------------------

// Wrong-machine guard for each controller. The A3000 family and the A4000T
// decode their SCSI chip in the same $DD0000 range, so probing the wrong
// controller's registers false-positives AND corrupts the live chip that
// actually lives there. Chipset (AGA vs ECS) canNOT arbitrate the SCSI
// rows — the AA3000+ (rebuilt A3000+ prototype) is a full SDMAC-SCSI A3000
// WITH real AGA — so those two gate on positive chip identification
// (ncr710Present()) instead. AGA remains a valid gate for A4000 IDE only.
#define HDD_GATE_NONE      0
#define HDD_GATE_A4000_IDE 1   // AGA machine AND not an A3000-family SDMAC machine
                               // (the AA3000+ is AGA WITH SDMAC and no IDE)
#define HDD_GATE_NCR       2   // a real 53C710 must answer at $DD0040 (A4000T SCSI)
#define HDD_GATE_NOT_NCR   3   // a real 53C710 there means A4000T — block (A3000 SCSI)

typedef struct {
    const char *name;
    int bigBoxDDBus;            // 1 = this controller's registers live in the $DD0000
                                // range, which A600/A1200 Gayle machines leave
                                // unterminated — a single access there stalls the CPU
                                // forever (no DSACK, and DiagROM has no bus-error
                                // recovery; confirmed on a real A1200 2026-07-25 by
                                // selecting "A3000/A3000T SCSI"). Every operation on
                                // these controllers is gated by isGayleMachine().
    int gate;                   // HDD_GATE_* wrong-machine guard, enforced by
                                // hddBlockedOnThisMachine() and the detect()s
    int (*detect)(void);        // 0 = absent, 1 = present
    int (*scan)(void);          // probe devices on bus; returns count found
    int (*identify)(void);      // show device info; returns count found
    int (*smart)(void);         // show SMART data; returns count found
    int (*dma)(void);           // DMA self-test — mem-to-mem where the chip can (A4000T),
                                // DMA-path register test otherwise (A3000); NULL if not applicable
    int (*dmaRepeat)(void);     // same, looped until ESC/both buttons; NULL if not applicable
    int (*xfer)(void);          // real-transfer DMA test: reads a target via DMA and
                                // compares against PIO (A3000); NULL if not applicable
    int (*xferRepeat)(void);    // same, looped until ESC/both buttons; NULL if not applicable
} HddController;

// ---------------------------------------------------------------------------
// Shared interactive unit browser
// ---------------------------------------------------------------------------
//
// One navigation shell for every controller's "Identify Devices" screen
// (SCSI, IDE, and whatever gets added later — currently A4000T SCSI's
// identifyA4000TSCSI() is still a stub, but this is where its real
// implementation should plug in too, rather than a third hand-copied loop).
// A controller supplies unit count + two per-unit callbacks (read/print one
// unit's info, and optionally its SMART data); this function owns the
// redraw/'+'/'-'/mouse/'S'/ESC handling once, identically, everywhere.
typedef void (*UnitInfoFn)(void *ctx, int unit);
typedef int  (*UnitSkipFn)(void *ctx, int unit);

// title       — printed once per redraw, e.g. "A3000/A3000T SCSI - Identify Devices"
// unitCount   — how many unit indices exist (0..unitCount-1)
// startUnit   — initial unit shown
// unitLabels  — NULL for auto "Unit N of M-1"; otherwise unitLabels[unit] printed as a header
// ctx         — opaque pointer forwarded to every callback (e.g. an IdeRegs*)
// identifyUnit— required: print unit's info
// smartUnit   — optional (NULL disables the 'S' key / hint text)
// skipUnit    — optional: return true to exclude a unit index from stepping
//               entirely (e.g. A3000 SCSI's own host ID, which can never be
//               a real target — see A3K_HOST_SCSI_ID)
// Returns 1 always — matches the "already handled its own dismissal, skip
// the generic press-any-key prompt" convention HDDTestC()'s case '4' checks.
//
// unitNext/unitPrev exist only to avoid `%` on a non-constant divisor —
// this freestanding build has no __modsi3 (same class of gap as the earlier
// __mulsi3/__divsi3 issues elsewhere in this file), and unitCount is a
// runtime parameter here, not a compile-time constant GCC could fold into a
// bitmask AND even when it happens to be a power of two.
static int unitNext(int unit, int unitCount)
{
    unit++;
    return (unit >= unitCount) ? 0 : unit;
}

static int unitPrev(int unit, int unitCount)
{
    unit--;
    return (unit < 0) ? unitCount - 1 : unit;
}

static int browseUnits(const char *title, int unitCount, int startUnit,
                        const char * const *unitLabels, void *ctx,
                        UnitInfoFn identifyUnit, UnitInfoFn smartUnit, UnitSkipFn skipUnit)
{
    int unit = startUnit;
    if (skipUnit && skipUnit(ctx, unit))
        unit = unitNext(unit, unitCount);
    int redraw = 1;

    for (;;) {
        if (redraw) {
            clearScreen();
            print((char *)title, WHITE); print("\n\n", WHITE);
            if (unitLabels) {
                print((char *)unitLabels[unit], CYAN); print(":\n", WHITE);
            } else {
                print("Unit ", WHITE); print(binDec(unit), CYAN);
                print(" of ", WHITE); print(binDec(unitCount - 1), WHITE); print("\n\n", WHITE);
            }

            identifyUnit(ctx, unit);

            print("\n+/- or mouse L/R: next/prev unit   ", WHITE);
            if (smartUnit) print("S: SMART   ", WHITE);
            print("ESC or both buttons: exit\n", WHITE);
            redraw = 0;
        }

        getInput();
        uint8_t ch  = globals->GetCharData;
        int     lmb = globals->LMB;
        int     rmb = globals->RMB;

        // A human pressing "both buttons together" almost never lands them
        // in the exact same getInput() poll - whichever button registers
        // first would otherwise immediately fire the single-button
        // next/prev branch below and block in waitReleased(), so the
        // two-button exit chord was effectively unreachable. Give the other
        // button a brief grace window to join before committing to a
        // single-button action.
        if ((lmb && !rmb) || (rmb && !lmb)) {
            for (int i = 0; i < 48 && !(lmb && rmb); i++) {
                waitShort();
                getInput();
                lmb = globals->LMB;
                rmb = globals->RMB;
            }
        }

        if ((lmb && rmb) || ch == 0x1b) {
            waitReleased();
            return 1;
        } else if (ch == '+' || (lmb && !rmb)) {
            waitReleased();
            do { unit = unitNext(unit, unitCount); } while (skipUnit && skipUnit(ctx, unit));
            redraw = 1;
        } else if (ch == '-' || (rmb && !lmb)) {
            waitReleased();
            do { unit = unitPrev(unit, unitCount); } while (skipUnit && skipUnit(ctx, unit));
            redraw = 1;
        } else if (smartUnit && (ch == 's' || ch == 'S')) {
            waitReleased();
            clearScreen();
            if (unitLabels) { print((char *)unitLabels[unit], CYAN); print(":\n\n", WHITE); }
            else            { print("Unit ", WHITE); print(binDec(unit), CYAN); print("\n\n", WHITE); }
            smartUnit(ctx, unit);
            print("\nPress any key/button to continue", WHITE);
            WaitButton();
            redraw = 1;
        }
    }
}

// ---------------------------------------------------------------------------
// Generic ATA IDE
// ---------------------------------------------------------------------------

#define GAYLE_ID_REG        ((volatile uint8_t *)0xDE1000)
// FOUND 2026-07-15: these are two SEPARATE Gayle registers, not one - IRQ
// *status* (which sources currently have a latched interrupt) lives at
// $DA9000, IRQ *enable* (which sources are configured to actually forward
// their interrupt) is a different register at $DA_A000. Confirmed against
// Amiberry's gayle.cpp (GAYLE_IRQ_1200=$9000 read/write_gayle_irq() vs
// GAYLE_INT_1200=$A000 read/write_gayle_int()). The old code read bit 0x10
// of the STATUS register and mislabeled it "enab:" - that bit in the status
// register is unrelated to IDE (PCMCIA battery/digital-audio change), so
// the reported enable state was never meaningful.
#define GAYLE_IRQ_REG       ((volatile uint8_t *)0xDA9000)   // status (per-source pending)
#define GAYLE_INT_REG       ((volatile uint8_t *)0xDAA000)   // enable (per-source forwarding on/off)
#define GAYLE_IDE_BIT       0x80   // same bit position in both registers

// Status register bits
#define IDE_BSY     0x80
#define IDE_DRDY    0x40
#define IDE_DF      0x20
#define IDE_DSC     0x10   // Seek Complete
#define IDE_DRQ     0x08
#define IDE_ERR     0x01

#define IDE_DEV_MASTER  0xA0
#define IDE_DEV_SLAVE   0xB0

#define ATA_CMD_IDENTIFY         0xEC
#define ATA_CMD_IDENTIFY_PACKET  0xA1   // ATAPI devices (CD-ROM/tape/etc) reject 0xEC with ERR
#define ATAPI_SIG_MID  0x14
#define ATAPI_SIG_HI   0xEB
#define RD32(b,o) (((uint32_t)(b)[(o)]<<24)|((uint32_t)(b)[(o)+1]<<16)|((uint32_t)(b)[(o)+2]<<8)|(uint32_t)(b)[(o)+3])
#define RDB_MAGIC   0x5244534BUL
#define PART_MAGIC  0x50415254UL
#define NO_LIST     0xFFFFFFFFUL

// Generic ATA register set — addresses and stride differ per controller
typedef struct {
    volatile uint16_t *data;
    volatile uint8_t  *features;   // write: features; read: error
    volatile uint8_t  *seccount;
    volatile uint8_t  *lba_lo;
    volatile uint8_t  *lba_mid;
    volatile uint8_t  *lba_hi;
    volatile uint8_t  *devhead;
    volatile uint8_t  *status;     // write: command; read: status
} IdeRegs;

// A1200/A600 Gayle: base $DA0000, stride 4
static const IdeRegs ideA1200Regs = {
    (volatile uint16_t *)0xDA0000,
    (volatile uint8_t  *)0xDA0004,
    (volatile uint8_t  *)0xDA0008,
    (volatile uint8_t  *)0xDA000C,
    (volatile uint8_t  *)0xDA0010,
    (volatile uint8_t  *)0xDA0014,
    (volatile uint8_t  *)0xDA0018,
    (volatile uint8_t  *)0xDA001C,
};

// A4000: base $DD2020, stride 4 - FIXED 2026-07-15. Was stride 2, which
// doesn't match this project's own already-correct A1200/A600 Gayle IDE
// (stride 4, same chip family) nor Amiberry's real register decode
// (src/gayle.cpp get_gayle_ide_reg(): `addr &= ~0x2020; addr >>= 2;` -
// dividing the register-relative offset by 4 only makes sense for
// registers genuinely spaced 4 bytes apart). Real Amiga Gayle IDE hardware
// task-file registers are 4 bytes apart on both A1200 and A4000 - this was
// a plain wrong-constant bug, not an intentional platform difference.
static const IdeRegs ideA4000Regs = {
    (volatile uint16_t *)0xDD2020,
    (volatile uint8_t  *)0xDD2024,
    (volatile uint8_t  *)0xDD2028,
    (volatile uint8_t  *)0xDD202C,
    (volatile uint8_t  *)0xDD2030,
    (volatile uint8_t  *)0xDD2034,
    (volatile uint8_t  *)0xDD2038,
    (volatile uint8_t  *)0xDD203C,
};

static uint8_t gayleReadID(void)
{
    volatile uint8_t *gid = GAYLE_ID_REG;
    uint8_t id = 0;
    *gid = 0;
    for (int i = 0; i < 8; i++)
        id = (uint8_t)((id << 1) | (*gid >> 7));
    return id;
}

static int detectGayleIDE(void)
{
    uint8_t id = gayleReadID();
    if (id == 0xFF)
        return 0;
    if (*ideA1200Regs.status == 0xFF)
        return 0;
    return 1;
}

// TRUE only when this machine positively IS an A600/A1200-class Gayle
// machine. Used as a hard gate by every big-box ($DD0000-range) controller
// path: on a real A1200, selecting "A3000/A3000T SCSI" froze the machine
// solid (user-reported on real hardware 2026-07-25). detectA3000SCSI()'s
// own loops are all iteration-bounded, so the only unbounded thing left is
// the bus cycle itself — Gayle doesn't terminate accesses in the $DD0000
// range, the CPU waits for DSACK forever, and DiagROM installs no bus-error
// recovery. The same range would hang detectA4000IDE ($DD2020) and
// detectA4000TSCSI ($DD0040) too, so all three are gated, not just the
// reported one.
//
// The Gayle ID register at $DE1000 is safe to touch on every machine
// (Gayle decodes it on A600/A1200; big boxes and A500/A2000 terminate the
// cycle via Gary/motherboard-resource decode). A real Gayle latches a fixed
// ID ($D0 = A600, $D1 = A1200) so repeated protocol passes are identical;
// on anything else the register is floating/foreign, so requiring THREE
// identical passes that also match the known $Dx family makes a false
// "this is a Gayle machine" on a real big box (which would wrongly block
// its SCSI/IDE tests) practically impossible — while a real A600/A1200
// always matches deterministically. Worst possible failure is the safe
// direction: gate open on a Gayle machine only if its own ID readback were
// unstable, which a working Gayle never is.
//
// NOT static: autoconfig.c reuses this as one leg of its $FF000000 Z3
// config space gate.
int isGayleMachine(void)
{
    uint8_t a = gayleReadID();
    uint8_t b = gayleReadID();
    uint8_t c = gayleReadID();
    return (a == b) && (b == c) && ((a & 0xF0) == 0xD0);
}

// TRUE only when this machine positively has the AGA chipset, via Lisa's
// DENISEID at $DFF07C: Lisa answers $F8, ECS Denise $FC, OCS leaves it
// floating. Requiring several identical AGA answers makes floating-bus luck
// practically impossible, and the worst failure direction is safe (a flaky
// readback blocks a test, never enables a destructive one).
//
// IMPORTANT SCOPE LIMIT (learned on real hw 2026-08-07): chipset does NOT
// determine which SCSI chip lives at $DD0000 — the AA3000+ (rebuilt A3000+
// prototype, the user's own machine) is a full SDMAC-SCSI A3000 WITH real
// AGA (genuine Alice "8374 R3"). So this gates only A4000 IDE; the two
// SCSI controllers are arbitrated by ncr710Present() (positive chip
// identification), never by chipset.
static int isAgaMachine(void)
{
    for (int i = 0; i < 8; i++) {
        if ((*(volatile uint16_t *)0xDFF07C & 0x00FF) != 0x00F8)
            return 0;
    }
    return 1;
}

// ---- Generic ATA helpers ----

static uint8_t ideWaitBSY(const IdeRegs *r)
{
    uint32_t timeout = 200000UL;
    uint8_t  st;
    do {
        st = *r->status;
        if (!(st & IDE_BSY))
            return st;
    } while (--timeout);
    return 0xFF;
}

static uint8_t ideSelectDrive(const IdeRegs *r, uint8_t devhead)
{
    *r->devhead = devhead;
    return ideWaitBSY(r);
}

static int ideDevPresent(const IdeRegs *r)
{
    *r->lba_mid = 0x55;
    *r->lba_hi  = 0xAA;
    return (*r->lba_mid == 0x55 && *r->lba_hi == 0xAA);
}

// FOUND 2026-07-15: an ATAPI device (CD-ROM/tape/etc) latches this exact
// signature into LBA_MID/LBA_HIGH after selection, per the ATA/ATAPI spec's
// standard way of telling ATAPI and plain ATA devices apart BEFORE ever
// issuing IDENTIFY - a real, documented protocol requirement, not an
// Amiberry quirk (confirmed against Amiberry's own emulation:
// src/ide.cpp's add_ide_unit() explicitly sets `ide->atapi = true` for CD
// devices). Sending the plain ATA_CMD_IDENTIFY (0xEC) to an ATAPI device
// gets rejected with ERR - this is what "CD-ROM shows an error" was.
// MUST be checked right after ideSelectDrive(), before ideDevPresent()'s
// own 0x55/0xAA write-test overwrites these same registers.
static int ideIsAtapi(const IdeRegs *r)
{
    return (*r->lba_mid == ATAPI_SIG_MID && *r->lba_hi == ATAPI_SIG_HI);
}

static uint8_t ideWaitDRQ(const IdeRegs *r)
{
    uint32_t timeout = 200000UL;
    uint8_t st;
    do {
        st = *r->status;
        if (st & IDE_BSY) continue;
        if (st & (IDE_DRQ | IDE_ERR))
            return st;
    } while (--timeout);
    return 0xFF;
}

static void ideReadWords(const IdeRegs *r, uint8_t *buf, int n)
{
    for (int i = 0; i < n; i++) {
        uint16_t w = *r->data;
        buf[i*2]   = (uint8_t)(w >> 8);
        buf[i*2+1] = (uint8_t)(w);
    }
}

static int ideReadSector(const IdeRegs *r, uint8_t lba_devhead_base, uint32_t lba, uint8_t *buf)
{
    *r->seccount = 1;
    *r->lba_lo   = (uint8_t)(lba);
    *r->lba_mid  = (uint8_t)(lba >> 8);
    *r->lba_hi   = (uint8_t)(lba >> 16);
    *r->devhead  = lba_devhead_base | (uint8_t)((lba >> 24) & 0x0F);
    ideWaitBSY(r);
    *r->status   = 0x20;
    uint8_t st = ideWaitDRQ(r);
    if (st == 0xFF || !(st & IDE_DRQ)) return 0;
    ideReadWords(r, buf, 256);
    return 1;
}

// Manual 32-bit unsigned division via binary long division (shift+compare+
// subtract only) — this freestanding build has no __divsi3 (same reasoning
// as A3000 SCSI's mul32() elsewhere in this file, which exists because
// __mulsi3 isn't linked in either), needed here for LBA->CHS translation
// (track/heads, track%heads, etc. — none of the divisors are powers of two
// in general, so a plain shift can't substitute).
static uint32_t udiv32(uint32_t n, uint32_t d, uint32_t *rem)
{
    uint32_t q = 0, r = 0;
    for (int i = 31; i >= 0; i--) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) { r -= d; q |= (1UL << i); }
    }
    if (rem) *rem = r;
    return q;
}

// CHS-mode sector read, for drives whose IDENTIFY capabilities word doesn't
// advertise LBA support (exactly what this project's own old-style
// CHS-geometry hardfile configs report — see doIdentifyIDEUnit()'s "no
// LBA" path). Translates a flat block number into real Cylinder/Head/
// Sector using the drive's own IDENTIFY-reported geometry. LBA 0 (where an
// RDB lives) and any RDB-referenced partition block are always reachable
// this way regardless of LBA-mode support — CHS addressing predates LBA
// entirely, it's not a real hardware limitation, just a different register
// convention.
static int ideReadSectorCHS(const IdeRegs *r, uint8_t devhead_base, uint32_t lba,
                             uint16_t heads, uint16_t spt, uint8_t *buf)
{
    if (heads == 0 || spt == 0) return 0;   // no valid geometry to translate with

    uint32_t rem;
    uint32_t track  = udiv32(lba, spt, &rem);     // track = lba / spt
    uint32_t sector = rem + 1;                     // CHS sector numbers are 1-based
    uint32_t cyl    = udiv32(track, heads, &rem);  // cyl = track / heads
    uint32_t head   = rem;                         // head = track % heads

    *r->seccount = 1;
    *r->lba_lo   = (uint8_t)sector;                          // CHS mode: sector number
    *r->lba_mid  = (uint8_t)cyl;                              // cylinder low byte
    *r->lba_hi   = (uint8_t)(cyl >> 8);                       // cylinder high byte
    *r->devhead  = (devhead_base & 0xF0) | (uint8_t)(head & 0x0F);  // no LBA bit
    ideWaitBSY(r);
    *r->status   = 0x20;
    uint8_t st = ideWaitDRQ(r);
    if (st == 0xFF || !(st & IDE_DRQ)) return 0;
    ideReadWords(r, buf, 256);
    return 1;
}

// Picks LBA-mode or CHS-mode addressing per-call based on `useLba` — lets
// the RDB scan (block 0..15 + the partition-block chain, both driven by
// plain flat block numbers) use one call site regardless of which mode this
// particular drive/config actually supports.
static int ideReadSectorAny(const IdeRegs *r, uint8_t devhead_base, uint32_t lba,
                             int useLba, uint16_t heads, uint16_t spt, uint8_t *buf)
{
    if (useLba) {
        uint8_t lba_dh = (uint8_t)((devhead_base & 0xF0) | 0x40);
        return ideReadSector(r, lba_dh, lba, buf);
    }
    return ideReadSectorCHS(r, devhead_base, lba, heads, spt, buf);
}

static uint8_t ideSmartCmd(const IdeRegs *r, uint8_t sub)
{
    *r->lba_mid  = 0x4F;
    *r->lba_hi   = 0xC2;
    *r->features = sub;
    *r->status   = 0xB0;
    return ideWaitBSY(r);
}

static void ideStripSpaces(char *s, int len)
{
    s[len] = '\0';
    for (int i = len - 1; i >= 0 && s[i] == ' '; i--)
        s[i] = '\0';
}

// ATA IDENTIFY string fields (model/serial/firmware) are transmitted
// byte-swapped within each 16-bit word per the ATA spec. ideReadWords()
// stores each word as (high byte, low byte) — the right order for
// reconstructing NUMERIC fields via the (buf[x]<<8)|buf[x+1] pattern used
// elsewhere — but ASCII string fields need each pair read the other way
// around, or the text comes out pairwise-swapped (confirmed live: "AU-EDI
// ElCae3n32h.fd" is "UAE-IDE Clean323.hdf" with every adjacent pair
// swapped). Only string fields need this — numeric fields are unaffected.
static void ideCopySwappedString(char *out, const uint8_t *src, int len)
{
    for (int i = 0; i < len; i += 2) {
        out[i]     = (char)src[i + 1];
        out[i + 1] = (char)src[i];
    }
}

static int ideQuickModel(const IdeRegs *r, char *out, int atapi)
{
    *r->status = atapi ? ATA_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY;
    uint8_t st = ideWaitDRQ(r);
    if (st == 0xFF || !(st & IDE_DRQ)) {
        // FOUND 2026-07-15: the ATAPI signature (ideIsAtapi()) is a one-time
        // artifact latched right after a device RESET, not something
        // re-asserted on every plain SELECT - earlier bus activity in the
        // same boot (a prior scan, another unit's identify on the same
        // shared registers) can leave it stale by the time this call
        // checks it, sending the wrong command and getting rejected. Rather
        // than require a full device-reset dance to force a fresh
        // signature, just try the other command once before giving up -
        // simpler and just as reliable in practice.
        *r->status = atapi ? ATA_CMD_IDENTIFY : ATA_CMD_IDENTIFY_PACKET;
        st = ideWaitDRQ(r);
        if (st == 0xFF || !(st & IDE_DRQ)) { out[0] = '\0'; return 0; }
    }
    // FOUND 2026-07-15: this was `static uint8_t mbuf[512]` - on this ROM's
    // hand-rolled memory layout (no standard crt0/.bss, everything carved
    // out of the fixed "workspace" sized at boot, see "Workspace needed"
    // in the boot log) a static local here collided with live stack
    // content, so by the time ideReadWords() returned, mbuf's own address
    // had been overwritten back into itself instead of holding the
    // IDENTIFY data - the model always came out empty even though the
    // hardware handshake (DRQ, the actual register reads) was working
    // perfectly (confirmed instruction-by-instruction with gdb + a
    // standalone probe that used a plain stack buffer and worked). A plain
    // stack-local buffer, exactly like doIdentifyIDEUnit()'s idbuf, doesn't
    // hit this.
    uint8_t mbuf[512];
    ideReadWords(r, mbuf, 256);
    ideCopySwappedString(out, mbuf + 54, 40);
    ideStripSpaces(out, 40);
    return 1;
}

static void printIdeStatus(uint8_t st)
{
    if (st & IDE_BSY)  print("BSY ",  RED);
    if (st & IDE_DRDY) print("DRDY ", GREEN);
    if (st & IDE_DF)   print("DF ",   RED);
    if (st & IDE_DSC)  print("DSC ",  GREEN);
    if (st & IDE_DRQ)  print("DRQ ",  RED);
    if (st & IDE_ERR)  print("ERR ",  RED);
}

// FOUND 2026-07-15: the previous version of this function printed "FOUND"/
// status/the ATAPI tag BEFORE issuing the IDENTIFY command, interleaving
// print() calls (real screen/serial I/O time) with the select->identify
// handshake - the same class of bug a temporary diagnostic print exposed
// earlier the same day (see [[project_diagrom_a4000t_scsi]] memory):
// enough delay between "device just became ready" and "issue the command"
// breaks the transfer. doIdentifyIDEUnit() never interleaves prints with
// the hardware sequence and has never shown this symptom - this rewrite
// matches that shape: select, check ATAPI, issue IDENTIFY immediately,
// THEN print everything based on the result.
static int doScanIDE(const IdeRegs *r)
{
    int found = 0;
    uint8_t st;
    // FOUND 2026-07-15: was `static char scanModel[42]` - same workspace/
    // stack collision bug as ideQuickModel()'s mbuf (see that function's
    // comment). Plain stack-local instead.
    char scanModel[42];

    print("Master: ", CYAN);
    st = ideSelectDrive(r, IDE_DEV_MASTER);
    if (st == 0xFF) {
        print("TIMEOUT\n", RED);
    } else {
        int atapi = ideIsAtapi(r);
        int ok = ideQuickModel(r, scanModel, atapi);
        if (ok) {
            print("FOUND  ", GREEN); printIdeStatus(st);
            if (atapi) print("ATAPI ", YELLOW);
            print(" \"", WHITE); print(scanModel, GREEN); print("\"\n", WHITE);
            found++;
        } else if (st & IDE_DRDY) {
            // DRDY was set (a real device answered selection) even though
            // IDENTIFY itself didn't return usable data - still worth
            // reporting as present rather than silently claiming nothing's
            // there.
            print("FOUND  ", GREEN); printIdeStatus(st);
            print(" \"\"\n", WHITE);
            found++;
        } else {
            print("NOT FOUND\n", RED);
        }
    }

    print("Slave:  ", CYAN);
    st = ideSelectDrive(r, IDE_DEV_SLAVE);
    if (st == 0xFF) {
        print("TIMEOUT\n", RED);
    } else {
        // Check ATAPI BEFORE ideDevPresent()'s own 0x55/0xAA write-test
        // would overwrite these same registers - see ideIsAtapi()'s
        // comment. ATAPI devices don't reliably answer that canary write
        // the way a plain ATA disk does, so the signature check itself IS
        // the presence test for that case.
        int atapi = ideIsAtapi(r);
        int present = atapi || ideDevPresent(r);
        if (!present) {
            print("NOT FOUND\n", RED);
        } else {
            int ok = ideQuickModel(r, scanModel, atapi);
            print("FOUND  ", GREEN); printIdeStatus(st);
            if (atapi) print("ATAPI ", YELLOW);
            print(" \"", WHITE); print(ok ? scanModel : "", GREEN); print("\"\n", WHITE);
            found++;
        }
    }

    ideSelectDrive(r, IDE_DEV_MASTER);
    return found;
}

// ---------------------------------------------------------------------------
// A1200/A600 IDE (Gayle)
// ---------------------------------------------------------------------------

static int scanGayleIDE(void)
{
    uint8_t id     = gayleReadID();
    uint8_t irqReg = *GAYLE_IRQ_REG;
    uint8_t intReg = *GAYLE_INT_REG;
    uint8_t ideErr = *ideA1200Regs.features;
    uint8_t ideSt  = *ideA1200Regs.status;

    print("\nGayle IDE Controller\n", WHITE);

    print("  Chip ID:  $", WHITE);
    print(binHexByte(id), CYAN);
    print("  ", WHITE);
    if      (id == 0xD0) print("(rev 0 - A600)\n",      CYAN);
    else if (id == 0xD1) print("(rev 1 - A1200)\n",     CYAN);
    else if ((id & 0xF0) == 0xD0) print("(Gayle)\n",   CYAN);
    else                 print("(Gayle-compatible)\n",   YELLOW);

    // DiagROM polls IDE status directly and never enables Gayle's IRQ
    // forwarding, so "PENDING" here is an expected byproduct of the drive
    // completing a command (it always raises its IRQ line, per ATA spec)
    // and nobody having acknowledged it - not a fault, hence no red/yellow.
    print("  IRQ reg:  $", WHITE);
    print(binHexByte(irqReg), CYAN);
    print("  [IDE irq:", WHITE);
    print((irqReg & GAYLE_IDE_BIT) ? "pending " : "clear   ", CYAN);
    print("enab:", WHITE);
    print((intReg & GAYLE_IDE_BIT) ? "yes" : "no", CYAN);
    print("]\n", WHITE);

    // RED means an active fault; this ERROR register is read before any
    // command in this run has been issued, so a nonzero value here is just
    // leftover state from whatever IDE command last ran (an earlier menu
    // visit, boot-time probing, etc.) - not a live error, hence no red.
    print("  IDE bus:  $", WHITE);
    print(binHexByte(ideSt), CYAN);
    print("  err: $", WHITE);
    print(binHexByte(ideErr), CYAN);
    print("\n\n", WHITE);

    print("Scanning IDE bus...\n\n", WHITE);
    return doScanIDE(&ideA1200Regs);
}

// ---------------------------------------------------------------------------
// Shared identify + SMART (works for any IdeRegs)
// ---------------------------------------------------------------------------

static void printNum3(uint32_t n)
{
    if (n < 100) print(" ", WHITE);
    if (n < 10)  print(" ", WHITE);
    print(bindec((int)n), WHITE);
}

static void printDosType(uint32_t dt)
{
    char s[5];
    s[0] = (char)(dt >> 24); if ((uint8_t)s[0] < 0x20) s[0] = '?';
    s[1] = (char)(dt >> 16); if ((uint8_t)s[1] < 0x20) s[1] = '?';
    s[2] = (char)(dt >>  8); if ((uint8_t)s[2] < 0x20) s[2] = '?';
    uint8_t v = (uint8_t)(dt);
    s[3] = (v <= 9) ? (char)('0' + v) : '?';
    s[4] = '\0';
    print(s, CYAN);
}

// One drive's full identify info (model/serial/firmware/size/CHS/RDB+
// partitions) — extracted from the old whole-bus doIdentifyIDE() so it can
// be shown one unit at a time in an interactive browser, matching
// identifyA3000SCSI()'s UX instead of a one-shot dump of both drives.
static void doIdentifyIDEUnit(const IdeRegs *r, uint8_t devhead)
{
    uint8_t idbuf[512];
    char    str[42];

    uint8_t st = ideSelectDrive(r, devhead);
    if (st == 0xFF) { print("  TIMEOUT\n", RED); return; }
    int isMaster = (devhead == IDE_DEV_MASTER);
    // Check the ATAPI signature right after select, before ideDevPresent()'s
    // own 0x55/0xAA write-test would overwrite these same registers - see
    // ideIsAtapi()'s comment for why this check has to happen here.
    int atapi = ideIsAtapi(r);
    if (isMaster  && !atapi && !(st & IDE_DRDY))  { print("  NOT PRESENT\n", YELLOW); return; }
    if (!isMaster && !atapi && !ideDevPresent(r)) { print("  NOT PRESENT\n", YELLOW); return; }

    *r->status = atapi ? ATA_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY;
    st = ideWaitDRQ(r);
    if (st == 0xFF) { print("  TIMEOUT\n", RED); return; }
    if (!(st & IDE_DRQ)) {
        // Same stale-signature fallback as ideQuickModel() - try the other
        // command once before reporting an error.
        atapi = !atapi;
        *r->status = atapi ? ATA_CMD_IDENTIFY_PACKET : ATA_CMD_IDENTIFY;
        st = ideWaitDRQ(r);
        if (st == 0xFF)      { print("  TIMEOUT\n", RED); return; }
        if (!(st & IDE_DRQ)) { print("  ERR ", RED); print(binHex(st), RED); print("\n", WHITE); return; }
    }

    ideReadWords(r, idbuf, 256);

    if (atapi) print("  Type:     ATAPI (CD-ROM/tape/etc)\n", CYAN);

    // Model: words 27-46 = bytes 54..93
    ideCopySwappedString(str, idbuf + 54, 40);
    ideStripSpaces(str, 40);
    print("  Model:    ", WHITE); print(str, GREEN); print("\n", WHITE);

    // Serial: words 10-19 = bytes 20..39
    ideCopySwappedString(str, idbuf + 20, 20);
    ideStripSpaces(str, 20);
    print("  Serial:   ", WHITE); print(str, WHITE); print("\n", WHITE);

    // Firmware: words 23-26 = bytes 46..53
    ideCopySwappedString(str, idbuf + 46, 8);
    ideStripSpaces(str, 8);
    print("  Firmware: ", WHITE); print(str, WHITE); print("\n", WHITE);

    // ATAPI devices don't use the ATA LBA28/CHS/RDB fields below the same
    // way (CD-ROM sectors are 2048 bytes, not 512, and there's no Amiga RDB
    // to walk on a data CD in this context) - Model/Serial/Firmware above is
    // the useful, correctly-decoded information for this device class.
    if (atapi) { print("\n", WHITE); return; }

    // Size: LBA28 sector count, words 60-61 = bytes 120..123
    uint32_t secs = ((uint32_t)((idbuf[122] << 8) | idbuf[123]) << 16)
                   | (uint32_t)((idbuf[120] << 8) | idbuf[121]);
    uint32_t mb = secs >> 11;
    print("  Size:     ", WHITE);
    if (mb >= 1024) {
        uint32_t gb  = mb >> 10;
        uint32_t dec = ((mb & 0x3FFU) * 10U) >> 10;
        print(bindec((int)gb), WHITE); print(".", WHITE);
        print(bindec((int)dec), WHITE); print(" GB\n", WHITE);
    } else {
        print(bindec((int)mb), WHITE); print(" MB\n", WHITE);
    }

    // CHS from IDENTIFY: word 1=cyls, word 3=heads, word 6=spt (bytes 2,6,12)
    uint16_t id_cyls  = (uint16_t)((idbuf[2]  << 8) | idbuf[3]);
    uint16_t id_heads = (uint16_t)((idbuf[6]  << 8) | idbuf[7]);
    uint16_t id_spt   = (uint16_t)((idbuf[12] << 8) | idbuf[13]);
    print("  CHS:      ", WHITE);
    print(bindec(id_cyls),  WHITE); print("c / ", WHITE);
    print(bindec(id_heads), WHITE); print("h / ", WHITE);
    print(bindec(id_spt),   WHITE); print("s\n", WHITE);

    // Check LBA support (word 49 bit 9). Absence doesn't mean the RDB is
    // unreachable — CHS addressing predates LBA entirely and can always
    // reach block 0 (or any RDB-referenced block), so fall back to
    // ideReadSectorAny()'s CHS-mode path instead of skipping outright. See
    // ideReadSectorCHS()'s comment for why this matters: this project's own
    // old-style CHS-geometry hardfile configs hit exactly this case.
    uint16_t caps = (uint16_t)((idbuf[98] << 8) | idbuf[99]);
    int useLba = (caps & 0x0200) != 0;
    if (!useLba)
        print("  (no LBA - using CHS-mode RDB scan)\n", YELLOW);

    // Scan first 16 sectors for RDB
    int rdb_found = 0;
    for (uint32_t blk = 0; blk < 16 && !rdb_found; blk++) {
        if (!ideReadSectorAny(r, devhead, blk, useLba, id_heads, id_spt, idbuf)) continue;
        if (RD32(idbuf, 0) != RDB_MAGIC) continue;
        rdb_found = 1;

        uint32_t rdb_cyls  = RD32(idbuf, 0x40);
        uint32_t rdb_spt   = RD32(idbuf, 0x44);
        uint32_t rdb_heads = RD32(idbuf, 0x48);
        uint32_t partblock = RD32(idbuf, 0x1C);

        print("  RDB blk ", WHITE); print(bindec((int)blk), WHITE); print(":\n", WHITE);
        print("    CHS:  ", WHITE);
        print(bindec((int)rdb_cyls),  WHITE); print("c / ", WHITE);
        print(bindec((int)rdb_heads), WHITE); print("h / ", WHITE);
        print(bindec((int)rdb_spt),   WHITE); print("s\n", WHITE);

        // Walk partition list
        int nparts = 0;
        while (partblock != NO_LIST && nparts < 64) {
            if (!ideReadSectorAny(r, devhead, partblock, useLba, id_heads, id_spt, idbuf)) break;
            if (RD32(idbuf, 0) != PART_MAGIC) break;

            uint32_t next    = RD32(idbuf, 0x10);
            uint32_t lowcyl  = RD32(idbuf, 0xA4);
            uint32_t highcyl = RD32(idbuf, 0xA8);
            uint32_t dostype = RD32(idbuf, 0xC0);

            // DriveName: BSTR at 0x24 (length byte then chars)
            uint8_t namelen = idbuf[0x24];
            if (namelen > 30) namelen = 30;
            for (int i = 0; i < namelen; i++) str[i] = (char)idbuf[0x25 + i];
            str[namelen]   = ':';
            str[namelen+1] = '\0';

            if (nparts == 0)
                print("    Partitions:\n", WHITE);
            print("      ", WHITE);
            print(str, GREEN);
            print("  [", WHITE);
            print(bindec((int)lowcyl),  WHITE);
            print(" - ", WHITE);
            print(bindec((int)highcyl), WHITE);
            print("]  ", WHITE);
            printDosType(dostype);
            print("\n", WHITE);

            nparts++;
            partblock = next;
        }
        if (nparts == 0)
            print("    No partitions\n", YELLOW);
    }
    if (!rdb_found)
        print("  No RDB found\n", YELLOW);
}

// Same extraction for SMART, one drive at a time — called both from the
// interactive browser's 'S' key and from doSmartIDE()'s whole-bus wrapper.
// Returns 1 if a SMART read actually succeeded (found), 0 otherwise.
static int doSmartIDEUnit(const IdeRegs *r, uint8_t devhead)
{
    static const struct { uint8_t id; const char *name; } names[] = {
        {  1, "Read Error Rate    " },
        {  3, "Spin-Up Time       " },
        {  4, "Start/Stop Count   " },
        {  5, "Reallocated Sectors" },
        {  7, "Seek Error Rate    " },
        {  9, "Power-On Hours     " },
        { 10, "Spin Retry Count   " },
        { 12, "Power Cycle Count  " },
        {190, "Airflow Temp (C)   " },
        {194, "Temperature (C)    " },
        {196, "Realloc Event Count" },
        {197, "Pending Sectors    " },
        {198, "Uncorrectable Sects" },
        {199, "UDMA CRC Errors    " },
        {  0, NULL }
    };

    uint8_t buf[512];

    uint8_t st = ideSelectDrive(r, devhead);
    if (st == 0xFF) { print("  TIMEOUT\n", RED); return 0; }
    int isMaster = (devhead == IDE_DEV_MASTER);
    if (isMaster  && !(st & IDE_DRDY))  { print("  NOT PRESENT\n", YELLOW); return 0; }
    if (!isMaster && !ideDevPresent(r)) { print("  NOT PRESENT\n", YELLOW); return 0; }

    st = ideSmartCmd(r, 0xD8);
    if (st & IDE_ERR) { print("  SMART not supported\n", YELLOW); return 0; }

    ideSmartCmd(r, 0xDA);
    uint8_t clo = *r->lba_mid;
    uint8_t chi = *r->lba_hi;
    print("  Status: ", WHITE);
    if      (clo == 0x4F && chi == 0xC2) print("OK\n",                 GREEN);
    else if (clo == 0xF4 && chi == 0x2C) print("FAILURE PREDICTED!\n", RED);
    else { print(binHex((uint32_t)(chi<<8)|clo), YELLOW); print(" (unknown)\n", YELLOW); }

    *r->lba_mid  = 0x4F;
    *r->lba_hi   = 0xC2;
    *r->features = 0xD0;
    *r->status   = 0xB0;
    st = ideWaitDRQ(r);
    if (st == 0xFF || !(st & IDE_DRQ)) { print("  SMART read failed\n", RED); return 0; }
    ideReadWords(r, buf, 256);

    print("   ID Cur Wst  Raw      Attribute\n", WHITE);

    for (int i = 0; i < 30; i++) {
        int off = 2 + i * 12;
        uint8_t id = buf[off];
        if (id == 0) continue;

        uint8_t cur  = buf[off + 3];
        uint8_t wst  = buf[off + 4];
        uint32_t raw = ((uint32_t)buf[off+8] << 24) | ((uint32_t)buf[off+7] << 16)
                     | ((uint32_t)buf[off+6] <<  8) |  (uint32_t)buf[off+5];
        int prefail  = buf[off+1] & 0x01;

        const char *name = NULL;
        for (int j = 0; names[j].id != 0; j++)
            if (names[j].id == id) { name = names[j].name; break; }

        print("  ", WHITE); printNum3(id);
        print(" ", prefail ? RED : WHITE); printNum3(cur);
        print(" ", WHITE); printNum3(wst);
        print("  ", WHITE); print(binHex(raw), WHITE);
        print(" ", WHITE);
        if (name) print((char *)name, WHITE);
        print("\n", WHITE);
    }
    return 1;
}

static int doSmartIDE(const IdeRegs *r)
{
    static const uint8_t devheads[2] = { IDE_DEV_MASTER, IDE_DEV_SLAVE };
    static const char *devnames[2]   = { "Master", "Slave " };
    int found = 0;

    print("\nSMART Data\n\n", WHITE);
    for (int d = 0; d < 2; d++) {
        print((char *)devnames[d], CYAN);
        print(":\n", WHITE);
        found += doSmartIDEUnit(r, devheads[d]);
        print("\n", WHITE);
    }
    return found;
}

// Adapters for browseUnits(): ctx is the IdeRegs* for whichever bus
// (A1200/Gayle or A4000), unit 0/1 maps to Master/Slave.
static void ideBrowseIdentify(void *ctx, int unit)
{
    static const uint8_t devheads[2] = { IDE_DEV_MASTER, IDE_DEV_SLAVE };
    doIdentifyIDEUnit((const IdeRegs *)ctx, devheads[unit]);
}

static void ideBrowseSmart(void *ctx, int unit)
{
    static const uint8_t devheads[2] = { IDE_DEV_MASTER, IDE_DEV_SLAVE };
    doSmartIDEUnit((const IdeRegs *)ctx, devheads[unit]);
}

static int doIdentifyIDE(const IdeRegs *r)
{
    static const char *labels[2] = { "Master", "Slave" };
    return browseUnits("IDE - Identify Devices", 2, 0, labels, (void *)r,
                        ideBrowseIdentify, ideBrowseSmart, NULL);
}

// doIdentifyIDE() is now the same kind of interactive per-unit browser as
// identifyA3000SCSI() (see that function's comment on the 1 = "handled its
// own dismissal" return convention), so forwarding its return value here is
// correct again — no more found-count-vs-dismissal-signal mismatch.
static int identifyGayleIDE(void) { return doIdentifyIDE(&ideA1200Regs); }
static int smartGayleIDE(void)    { return doSmartIDE(&ideA1200Regs); }

// ---------------------------------------------------------------------------
// A4000/A4000T IDE
// ---------------------------------------------------------------------------

static int detectA3000SCSI(void);   // defined in the A3000 SCSI section below

static int detectA4000IDE(void)
{
    if (isGayleMachine()) return 0;   // $DD2020 is unterminated on A600/A1200 — see isGayleMachine()
    if (!isAgaMachine()) return 0;    // A4000 IDE exists only in AGA machines
    // A3000-family machines have no motherboard IDE, but the SDMAC's
    // incomplete $DDxxxx decoding answers the status read below — confirmed
    // false green on the real AA3000+ (AGA machine, so the chipset gate
    // can't catch it; 2026-08-07). A positive SDMAC/WD33C93 answer at
    // $DD0000 therefore rules IDE out. Safe on every machine type:
    // detectA3000SCSI() is Gayle-gated, bails on a real 53C710 (A4000T,
    // where IDE genuinely coexists) via ncr710Present(), and on a real
    // A4000 just reads open bus and fails cleanly.
    if (detectA3000SCSI()) return 0;
    // No chip ID register — check if IDE bus responds
    return (*ideA4000Regs.status != 0xFF);
}

static int scanA4000IDE(void)
{
    uint8_t ideSt  = *ideA4000Regs.status;
    uint8_t ideErr = *ideA4000Regs.features;

    print("\nA4000/A4000T IDE Controller\n", WHITE);
    print("  Regs:    $DD2020  (stride 4)\n", WHITE);
    print("  IDE bus: $", WHITE);
    print(binHexByte(ideSt), ideSt == 0xFF ? RED : CYAN);
    // RED means an active fault; this ERROR register is read before any
    // command in this run has been issued, so a nonzero value here is just
    // leftover state from whatever IDE command last ran - not a live
    // error, hence no red (matches scanGayleIDE()'s same fix).
    print("  err: $", WHITE);
    print(binHexByte(ideErr), CYAN);
    if (ideSt == 0xFF)
        print("  (bus float - no IDE hardware at $DD2020)\n", RED);
    else
        print("\n", WHITE);
    print("\n", WHITE);

    print("Scanning IDE bus...\n\n", WHITE);
    return doScanIDE(&ideA4000Regs);
}

// Same forwarding note as identifyGayleIDE() above.
static int identifyA4000IDE(void) { return doIdentifyIDE(&ideA4000Regs); }
static int smartA4000IDE(void)    { return doSmartIDE(&ideA4000Regs); }

// ---------------------------------------------------------------------------
// A3000 / A3000T SCSI  (WD33C93 + SDMAC, base $DD0000)
// ---------------------------------------------------------------------------

// The A3000 does NOT wire the WD33C93 directly onto $DD0000/$DD0002 — the
// SDMAC sits in between and multiplexes it via its PORT0 indirect-addressing
// range ($DD0040-$DD004F). Register select, aux status, and register data
// are three SEPARATE ports here, unlike a bare/direct-wired WD33C93 where a
// single SASR address serves both "select register" (write) and "aux status"
// (read). Getting this wrong lets RESET appear to work (coincidental aliasing)
// while every real SCSI command silently fails.
// Commodore's own shipped scsidisk.device source (board.i, IFD IS_SCSIDISK
// "; a3000" block) uses SASRW=$48, SASR=$49, SCMD=$43. Those are real mirror
// addresses within the SDMAC's PORT0 range, but Amiberry's SDMAC emulation
// only decodes the primary $40/$41/$43 addresses (confirmed: RESET fails at
// $48/$49, succeeds at $40/$41) — it doesn't replicate real hardware's
// incomplete address-line decoding across the full mirror range. $40/$41
// are documented as valid real-hardware mirrors too, so this isn't a
// real-vs-emulator tradeoff.
#define WD_ADDR_REG   ((volatile uint32_t *)0xDD0040)   // write-only, longword: select internal reg # (SASRW mirror)
#define WD_AUX_STATUS ((volatile uint8_t  *)0xDD0041)   // read-only: auxiliary status (SASR mirror)
#define WD_REG_DATA   ((volatile uint8_t  *)0xDD0043)   // R/W: data for currently selected register (SCMD)

// SDMAC's own control register — separate from the WD33C93 entirely. PORT0
// (the WD33C93 access range above) is shared hardware that can also serve a
// direct-connect IDE/XT-AT device; PMODE selects which. Defaults to 0 (XT/AT)
// after reset, so without this the WD33C93 register I/O above still "works"
// (simple decode), but the SDMAC never actually drives real SCSI bus
// arbitration/selection for it.
// Confirmed against official board.i (IFD IS_SCSIDISK ; a3000 block): CNTR
// is at $0B, not $08 (that reverse-engineered-doc address was wrong — it
// pointed at an unrelated register, so PMODE was never actually being set).
#define SDMAC_CNTR      ((volatile uint8_t *)0xDD000B)
#define SDMAC_CNTR_PDMD   0x08 // PMODE bit (bit 3): 1 = SCSI peripheral mode, 0 = XT/AT IDE mode
// INTENA (bit 2): official board.i's ISTR bit comments say "PEND: some
// interrupt pending (only when enabled)" — without this, the SDMAC may never
// latch/expose the WD33C93's own completion status at all. This is separate
// from the CPU's own interrupt mask (DiagROM runs with CPU IRQs off by
// design) — it's a per-chip enable gate on the SDMAC itself, needed even
// when only polling (never using a real 68000 interrupt vector) because it
// gates whether the status is visible to read at all, not just whether it's
// delivered as a CPU interrupt.
#define SDMAC_CNTR_INTENA 0x04
// PRESET (bit 4): official board.i, "peripheral reset (active high)(strobe)"
// — a genuine SCSI BUS reset (asserts real RST on the bus), completely
// different from the WD33C93's own internal soft-RESET command (which only
// resets the controller chip, not any target sitting on the bus). The real
// driver never strobes this in normal operation — but it also never abandons
// a transaction mid-phase the way our minimal driver does when a poll times
// out. A target left mid-transaction from an incomplete SELECT/phase attempt
// may simply refuse to respond to a fresh SELECT_WITH_ATN until the bus is
// actually reset — a plausible explanation for "works once right after boot,
// never again on a re-scan" seen on real hardware with a SCSI2SD target.
//
// TESTED ON REAL HARDWARE 2026-07-07: strobing this hung the real A3000T
// solid (system froze printing the message right before the strobe, never
// recovered — needed a power cycle). Most likely the SDMAC stops asserting
// DTACK for some period after PRESET and our very next register write (to
// clear it) stalled the 68030's bus indefinitely; DiagROM has no bus-error
// timeout. DO NOT strobe this again without a fundamentally more careful
// approach (e.g. a bus-error handler wrapped around the access, and/or a
// real measured delay between assert/deassert rather than one waitShort()).
#define SDMAC_CNTR_PRESET 0x10

// The SDMAC has its OWN interrupt status register (ISTR) and clear-interrupts
// strobe (CINT), entirely separate from the WD33C93's own ASR that we've been
// reading through the PORT0 pass-through this whole session. Official
// driver's real interrupt handler (BoardServer) checks ISTR's PEND bit FIRST,
// before ever looking at the WD33C93's own status, and clears completions via
// CINT. We have never touched either register. If PEND latches from one
// event and is never cleared, it could plausibly block visibility of ALL
// subsequent WD33C93 completions through the shared pass-through — a strong
// candidate for "works once, then stuck" and the persistent activity LED.
// Unlike PRESET (a real bus reset), this is just a normal register strobe on
// an address range we've already written safely many times (like CNTR).
#define SDMAC_ISTR      ((volatile uint8_t *)0xDD001F)
#define SDMAC_CINT      ((volatile uint8_t *)0xDD001B)
#define SDMAC_ISTR_PINT 0x40   // peripheral (WD33C93) interrupt pending
#define SDMAC_ISTR_EOP  0x20   // end-of-process (DMA terminal count) interrupt pending
#define SDMAC_ISTR_PEND 0x10   // some interrupt pending (only visible when SDMAC INTENA is set)
#define SDMAC_ISTR_FIFOF 0x02  // DMA FIFO full
#define SDMAC_ISTR_FIFOE 0x01  // DMA FIFO empty

// DMA-path registers, used only by the register test (a3kDmaTest() below).
// Behavior re-derived from Chris Hooper's SDMAC utility
// (github.com/cdhooper/amiga_sdmac_test, sdmac.c) — the one tool known to
// exercise these on real A3000s/ReSDMACs — because the official docs don't
// admit any of the quirks that matter:
//  - WTC (Word Transfer Count): readback is revision-dependent, and that
//    difference is the ONLY software way to tell SDMAC revisions apart:
//    SDMAC-02 reads back all 24 writable bits, SDMAC-04 always returns 0 in
//    bit 2 (see a3kSdmacVersion()).
//  - ACR (DMA address): physically inside RAMSEY, not the SDMAC — the
//    address-generation side of DMA lives in the memory controller, only the
//    FIFO/count side is in the SDMAC — so pattern-testing it exercises the
//    Ramsey half of the DMA path. Bits 1-0 always read 0 (transfers are
//    longword-aligned).
//  - SSPBDAT (Synchronous Serial Peripheral Bus data): the only SDMAC-04
//    register with r/w bits testable without starting a DMA. Low 8 bits
//    tested (A3000+ docs say low 11 are r/w; ReSDMAC implements all 32 —
//    testing 8 is valid on all of them).
//  - REVISION: ReSDMAC's added version register, format 'v'<maj>'.'<min>;
//    open bus on a genuine SDMAC.
#define SDMAC_WTC           ((volatile uint32_t *)0xDD0004)
#define RAMSEY_ACR          ((volatile uint32_t *)0xDD000C)
#define SDMAC_SSPBDAT       ((volatile uint32_t *)0xDD0058)
#define SDMAC_REVISION_REG  ((volatile uint32_t *)0xDD0020)

// Real-DMA machinery (Tier 2 transfer test, a3kXferTest() below). Addresses
// and access widths straight from Linux drivers/scsi/a3000.h's register
// struct (all four are 16-bit strobes — the written VALUE is irrelevant,
// Linux writes 1; their odd byte lanes $13/$17/$1B/$3F are what Hooper's
// byte-level map lists). CINT's byte twin SDMAC_CINT above predates this —
// same register, the word form is used on the DMA path to mirror the Linux
// driver exactly.
#define SDMAC_ST_DMA        ((volatile uint16_t *)0xDD0012)   // strobe: start DMA engine
#define SDMAC_FLUSH_STROBE  ((volatile uint16_t *)0xDD0016)   // strobe: push FIFO residue to memory
#define SDMAC_CINT_STROBE   ((volatile uint16_t *)0xDD001A)   // strobe: clear latched SDMAC interrupts
#define SDMAC_SP_DMA        ((volatile uint16_t *)0xDD003E)   // strobe: stop DMA engine
// CNTR bit 1 (Linux CNTR_DDIR): DMA direction. CLEAR = SCSI->memory (read),
// SET = memory->SCSI (write) — per Linux a3000.c dma_setup(), which only
// ORs it in for !dir_in. This ROM only ever DMA-reads, so it stays 0; named
// anyway so the polarity is on record.
#define SDMAC_CNTR_DDIR   0x02

// DAWR ("DACK Width Register", write-only, 16-bit) — never written by any
// prior version of this ROM. Found by cross-referencing Linux's
// drivers/scsi/a3000.c/a3000.h and NetBSD's sys/arch/amiga/dev/ahscreg.h
// (both real, independently-written drivers for this exact chip): both
// write 3 here as one of the very first hardware accesses. NetBSD's header
// cites "according to A3000T service-manual". Real drivers do this before
// touching the WD33C93 at all, so we match that here.
#define SDMAC_DAWR      ((volatile uint16_t *)0xDD0002)
#define DAWR_A3000_VAL  3

// Ramsey (A3000 memory controller) version register — leaked AmigaOS
// a3000_hardware.i: "A3000_RamseyVersion EQU $00de0043 ;12D ramsey==$0d",
// "ANCIENT_RAMSEY EQU $7f" (pre-production chip, no real version).
#define RAMSEY_VERSION_ADDR ((volatile uint8_t *)0x00DE0043)
#define ANCIENT_RAMSEY 0x7F

// Auxiliary status bits (read from SASR without writing addr first)
#define WD_ASR_INT   0x80
#define WD_ASR_LCI   0x40
#define WD_ASR_BSY   0x20
#define WD_ASR_CIP   0x10
#define WD_ASR_DBR   0x01

// Internal WD33C93 register numbers
#define WD_OWN_ID       0x00
#define WD_CONTROL      0x01
#define WD_CONTROL_DMA  0x80   // CONTROL DM2:0 = 100 = DMA mode (Linux wd33c93.h
                               // CTRL_DMA) — the chip asserts DRQ per byte and the
                               // SDMAC moves the data; 0x00 = polled (CPU pumps
                               // WD_DATA), which is what every PIO path here writes
#define WD_TIMEOUT_REG  0x02
#define WD_CDB1         0x03   // CDB bytes 0-11 at 0x03..0x0E
#define WD_CDB8_LADDR0  0x0A   // CDB byte 8 ("Logical Address LSB") — fully r/w
                               // scratch when no command is in progress; the
                               // register a3kWdcRegCheck() pattern-tests
#define WD_TARGET_LUN   0x0F
#define WD_CMD_PHASE    0x10
#define WD_SYNC_XFER    0x11
#define WD_XFER_CNT_H   0x12
#define WD_XFER_CNT_M   0x13
#define WD_XFER_CNT_L   0x14
#define WD_DST_ID       0x15
#define WD_DST_ID_DPD   0x40   // Data Phase Direction: 1 = data phase is IN (read from target)
#define WD_SRC_ID       0x16
#define WD_SCSI_STATUS  0x17
#define WD_COMMAND      0x18
#define WD_DATA         0x19
#define WD_QUEUE_TAG    0x1A   // 33C93B ONLY — doesn't exist on 93/93A, which is
                               // exactly what makes it the B-detector (Linux wd33c93.h)

// WD33C93 commands. Real AmigaOS drivers never use the autonomous "Select
// and Transfer" command for actual I/O — they SELECT_WITH_ATN, then manually
// drive each SCSI phase with its own TRANSFER_INFO, reacting to whatever
// phase the target requests next via the status byte below.
#define WDCMD_RESET             0x00
#define WDCMD_ABORT             0x01
#define WDCMD_ASSERT_ATN        0x02   // level-I quick command, no completion INT of its
                                       // own — the consequence shows up as the target's
                                       // next phase request (MSG_OUT), same convention
                                       // as NEGATE_ACK below
#define WDCMD_NEGATE_ACK        0x03
#define WDCMD_SELECT_WITH_ATN   0x06
#define WDCMD_TRANSFER_INFO     0x20
// Single Byte Transfer mode (COMMAND register bit 7). The real driver
// (scsitask.asm SCSIPutByte/SCSIGetByte) uses this — not plain
// TRANSFER_INFO — for every single-byte phase (MSG_OUT identify, STATUS,
// MSG_IN). This distinction is what let a full transaction complete for the
// first time; see [[project_diagrom_a3000_scsi]] memory.
#define WD_CMD_SBT              0x80

// Own ID register bit 3 = EAF (Enable Advanced Features). Requesting it
// before RESET and getting WDSTS_RESET_AF back instead of plain
// WDSTS_RESET confirms the chip understands advanced features (at least a
// 33C93A) — the closest thing to a software-readable version this chip
// exposes (WD33C93B datasheet, no separate revision register documented).
#define WD_OWN_ID_EAF           0x08
// Own ID register bit 5 = RAF ("Really Advanced Features"). The microcode
// revision only parks in CDB1 after a RESET issued with THIS bit set — an
// EAF-only reset leaves CDB1 at $00. Technique straight from Hooper's
// sdmac.c (scsi_soft_reset(2)); found on real hw 2026-08-11 when his own
// WD33C93A 00-08 (microcode $09) was reported as $00 — the common A3000
// 00-03/00-04 parts genuinely have microcode $00, which masked the bug.
#define WD_OWN_ID_RAF           0x20

// WD33C93 SCSI_STATUS codes: upper nibble = group, lower nibble = code.
#define WDSTS_RESET       0x00   // reset complete (standard)
#define WDSTS_RESET_AF    0x01   // reset complete (advanced features)
#define WDSTS_SEL_COMPLETE 0x11  // select complete
#define WDSTS_TIMEOUT     0x42   // selection timeout — no device

// Phase-request codes (low nibble of SCSI_STATUS) seen in groups 1/2/3/4 —
// each means "target wants this phase now, issue TRANSFER_INFO for it".
#define WDPHASE_DATA_OUT  0x8
#define WDPHASE_DATA_IN   0x9
#define WDPHASE_CMD       0xA
#define WDPHASE_STATUS    0xB
#define WDPHASE_MSG_OUT   0xE
#define WDPHASE_MSG_IN    0xF

// OWN_ID register per the real WD33C93B datasheet: bits 7-6 = FS1/FS0
// (2-bit Frequency Select, not a 3-bit "CLK select" as an earlier comment
// here claimed), bit 5 = RAF, bit 4 = EHP, bit 3 = EAF, bits 2-0 = host SCSI
// ID. A3000 system clock ~14MHz → FS1/FS0 = 01 (12.5MHz slot) → bits 7-6 =
// 01 → 0x40. The value below predates this correction but decodes
// identically under the real layout (the other bits we don't set —
// RAF/EHP/EAF — are 0 either way), so only the comment was wrong.
// The host adapter's own SCSI ID — configurable on real A3000 hardware
// (jumper-settable), not a fixed constant, but this code has only ever used
// 7 (the conventional default). Named here so callers that need to skip
// "our own ID" (it can never respond to a SELECT — there's no target to
// find there) reference the same value the chip is actually programmed
// with, rather than a second hardcoded 7 that could drift out of sync.
#define A3K_HOST_SCSI_ID 7
#define WD_OWN_ID_VAL   (0x40 | A3K_HOST_SCSI_ID)   // FS1/FS0=12.5MHz

// SCSI commands
#define SCSI_INQUIRY    0x12
#define SCSI_INQUIRY_LEN 36
#define SCSI_READ_CAPACITY     0x25
#define SCSI_READ_CAPACITY_LEN 8

// SCSI's own SMART-equivalent: LOG SENSE(10), a standard SCSI-2+ command
// (predates ATA SMART) most real SCSI hard drives from this era support.
// Page 0x2F (Informational Exceptions) is where a drive reports predictive
// failure; ASC=$5D is the standard "FAILURE PREDICTION THRESHOLD EXCEEDED"
// family, the SCSI equivalent of ATA SMART's $F4/$2C trip signature. Page
// 0x0D (Temperature) is optional/best-effort, not every drive supports it.
#define SCSI_LOG_SENSE       0x4D
#define SCSI_LOGPAGE_IE      0x2F
#define SCSI_LOGPAGE_TEMP    0x0D
#define SCSI_LOGSENSE_LEN    32

static inline void a3k_wd_write(uint8_t reg, uint8_t val) {
    *WD_ADDR_REG = reg;
    *WD_REG_DATA = val;
}
static inline uint8_t a3k_wd_read(uint8_t reg) {
    *WD_ADDR_REG = reg;
    return *WD_REG_DATA;
}
static inline uint8_t a3k_wd_aux(void) { return *WD_AUX_STATUS; }

// ---------------------------------------------------------------------------
// SCSI PORTS interrupt (level 2 autovector, custom.intreq bit 3) — every
// Amiga HD controller, including this A3000 SDMAC/WD33C93, shares this line.
// It's edge/level-latched: once serviced once (e.g. RESET's own completion),
// it can get stuck and never re-arm for a later event unless something
// actually acknowledges it at the Paula level — re-reading the WD33C93's own
// ASR alone doesn't do that, even in a purely polled driver. This mirrors the
// general approach already used for IRQ2 in irqcia.c, but is a fresh,
// SCSI-specific handler — irqcia.c's IRQ2 body services CIA-A, not this chip.
//
// Communication with the polling code goes entirely through globals-> (never
// a plain C static) — see the A3kInquiryDebug comment above for why: this
// linker script places every static/global in read-only ROM, so a static
// written from an interrupt handler would be silently discarded.
// ---------------------------------------------------------------------------
#define SCSI_PORTS_BIT 0x0008

__interrupt void ScsiPortsIRQ(VARS)
{
    uint16_t irq = custom->intreqr;
    if (irq & SCSI_PORTS_BIT) {
        globals->ScsiIrqStatus = a3k_wd_read(WD_SCSI_STATUS);   // ack WD33C93's ASR INT, capture status
        globals->ScsiIrqPending = 1;
        globals->ScsiIrqCount++;
    }
    custom->intreq = SCSI_PORTS_BIT;
    custom->intreq = SCSI_PORTS_BIT;
}

static inline void a3k_set_sr(uint16_t sr __asm("d0"))
{
    asm volatile ("move %0,sr\n" : : "d" (sr) : "cc");
}

// Plain pointer parameter (not VARS/__asm("a6")) — this is called only from
// other C code in this file (scanA3000SCSI, itself a (void)-signature
// function reached through the shared HddController table), so it just needs
// the ambient a6 value read into a normal local and passed through normally.
static void a3k_scsi_irq_enable(volatile struct GlobalVars *globals)
{
    globals->ScsiIrqPending = 0;
    globals->ScsiIrqStatus  = 0;
    globals->ScsiIrqCount   = 0;
    *(volatile APTR *) + 0x68 = ScsiPortsIRQ;
    custom->intreq = SCSI_PORTS_BIT;             // clear any stale/latched request first
    custom->intreq = SCSI_PORTS_BIT;
    custom->intena = 0xC000 | SCSI_PORTS_BIT;    // master enable + PORTS
    custom->intena = 0xC000 | SCSI_PORTS_BIT;
    a3k_set_sr(0x2000);                          // unmask CPU interrupts so the vector can run
}

static void a3k_scsi_irq_disable(void)
{
    custom->intena = 0x7fff;    // fully quiesce again — matches IRQTestC's cleanup convention
    custom->intreq = 0x7fff;
    *(volatile APTR *) + 0x68 = RTEcode;
}

// a6 holds this ROM's globals pointer everywhere (this file is compiled with
// -ffixed-a6, so GCC never repurposes it) — read it locally wherever needed
// rather than threading it through every call site as a parameter.
static inline volatile struct GlobalVars *a3k_globals(void)
{
    volatile struct GlobalVars *g;
    asm volatile ("move.l a6,%0" : "=r" (g));
    return g;
}

// Keep the keyboard alive during long hardware waits, and latch abort
// requests. The Amiga keyboard needs its scancode handshake within ~143ms
// or it enters resync and the byte sitting in CIA-A's SDR gets clobbered —
// and getKey() only handshakes when it is actually called. The soak tests'
// abort poll runs once per rep, which is SECONDS apart once the bus is
// timing out, so on real hw (cdh's A3000 + ZuluSCSI, 2026-08-11) ESC could
// not stop a wedged DMA Transfer soak at all: every press arrived
// resync-mangled, while the level-sampled both-mouse-buttons abort still
// worked. Called from inside every bounded WD wait/pump loop — the call
// sites are arranged so it only actually runs after ~10ms+ of continuous
// WAITING, never in a healthy fast path, and never during an active SDMAC
// DMA window (a3k_wd_wait_status_dma stays hands-off by design; its ~0.8s
// bound is short enough that starvation there doesn't matter).
//
// Drains ALL queued scancodes (press+release pairs pile up between calls;
// the keyboard's own buffer is 10 deep) and LATCHES rather than returns:
// the consumer is a4kDmaAbortRequested(), whose getInput()->clearInput()
// would otherwise wipe a key consumed here before ever seeing it. Mouse
// buttons are sampled directly (CIA-A PRA bit 6 / POTGOR bit 10, both
// active-low) so a held both-buttons abort also lands mid-rep.
static void hddInputService(void)
{
    for (int i = 0; i < 12; i++) {
        getKey();
        if (!globals->keydown && !globals->keyup)
            break;                       // nothing consumed — queue empty
        if (globals->keynew && globals->key == 0x45)   // raw ESC scancode, key-down
            globals->HddEscLatch = 1;
    }
    if (!(*(volatile uint8_t *)0xBFE001 & 0x40) &&
        !(*(volatile uint16_t *)0xDFF016 & 0x0400))
        globals->HddEscLatch = 1;
}

// waitShort() paces at ~640us/iteration via the video beam position register
// (see amiga.c) — real elapsed time regardless of CPU speed, unlike a raw
// iteration-count spin. a3k_wd_init() sets the WD33C93's own internal
// selection-timeout to ~250ms (WD_TIMEOUT_REG); a fast 68060 can burn through
// a fixed instruction-count loop well before that real time has elapsed,
// giving up before the chip's own timeout interrupt ever had a chance to
// fire. ~500 iterations =~ 320ms, comfortably past that.
#define A3K_WAIT_ITERS 500

// Poll for INT, return SCSI_STATUS; 0xFF = timeout. Once a3k_scsi_irq_enable()
// is active, ScsiPortsIRQ() races us to WD_SCSI_STATUS and normally wins —
// reading it there clears the WD33C93's own ASR INT bit out from under us, so
// polling WD_ASR_INT directly would spin its full timeout on every call. Check
// the ISR-populated flag first; fall back to direct polling so this still
// works unchanged when no ISR is installed (e.g. detectA3000SCSI's path).
static uint8_t a3k_wd_wait_int(void)
{
    volatile struct GlobalVars *globals = a3k_globals();
    for (int i = 0; i < A3K_WAIT_ITERS; i++) {
        if (globals->ScsiIrqPending) {
            globals->ScsiIrqPending = 0;
            return globals->ScsiIrqStatus;
        }
        if (a3k_wd_aux() & WD_ASR_INT)
            return a3k_wd_read(WD_SCSI_STATUS);
        waitShort();
        if ((i & 15) == 15) hddInputService();   // only fires after ~10ms of waiting
    }
    return 0xFF;
}

// Wait for BSY and CIP to clear — the chip can still be internally busy for
// a moment after the completion interrupt fires. Issuing a new command while
// BSY is set gets it silently ignored (LCI) instead of executed.
static void a3k_wd_wait_ready(void)
{
    // Drain any stuck data byte(s) first — a serviced DBR can be what's
    // actually blocking BSY/CIP from ever clearing.
    for (int i = 0; i < 40; i++) {
        uint8_t asr = a3k_wd_aux();
        if (asr == 0xFF || !(asr & WD_ASR_DBR)) break;
        a3k_wd_read(WD_DATA);
    }

    uint32_t t = 1500000UL;
    while (t--) {
        if ((t & 0xFFFF) == 0) hddInputService();
        uint8_t asr = a3k_wd_aux();
        if (asr == 0xFF) continue;
        if (!(asr & (WD_ASR_BSY | WD_ASR_CIP))) return;
    }

    // Still stuck after waiting — force it with an explicit ABORT.
    a3k_wd_write(WD_COMMAND, WDCMD_ABORT);
    t = 1500000UL;
    while (t--) {
        if ((t & 0xFFFF) == 0) hddInputService();
        uint8_t asr = a3k_wd_aux();
        if (asr == 0xFF) continue;
        if (!(asr & (WD_ASR_BSY | WD_ASR_CIP))) return;
    }
}

// Genuine SCSI bus reset via the SDMAC's PRESET strobe — see SDMAC_CNTR_PRESET
// comment above. Distinct from a3k_wd_reset() (WD33C93's own internal
// soft-reset command, which does NOT drive real SCSI RST onto the bus).
//
// RULED OUT 2026-07-07, unused — kept only for reference. Strobing this on an
// already-wedged bus hung a real A3000T solid once. Retested strobing it only
// on a guaranteed-fresh-boot bus (every test that session was a fresh flash):
// the strobe itself survived safely, but the WD33C93 reset immediately
// afterward then reliably FAILED (tested with up to ~1.3s of real recovery
// delay in between, no change) — i.e. it's safe here, but makes things worse
// than not using it at all, since a fresh-boot WD33C93 reset always succeeds
// on its own without this. Something about PRESET leaves the chip
// unresponsive in a way session-level recovery can't undo. Not a viable
// recovery tool for the "works once, then stuck" symptom.
static void a3k_scsi_bus_reset(void)
{
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_PRESET;
    waitShort();
    *SDMAC_CNTR = SDMAC_CNTR_PDMD;
    waitShort();
}

static int a3k_wd_reset(void)
{
    // OWN_ID (host SCSI ID + input clock divisor) is only latched by the
    // chip AT reset time, so it must be written BEFORE issuing RESET, not
    // after — setting it afterward leaves the clock divisor unconfigured.
    a3k_wd_write(WD_OWN_ID, WD_OWN_ID_VAL);
    a3k_wd_write(WD_COMMAND, WDCMD_RESET);
    uint8_t st = a3k_wd_wait_int();
    a3k_wd_wait_ready();
    return (st == WDSTS_RESET || st == WDSTS_RESET_AF);
}

static void a3k_wd_init(void)
{
    a3k_wd_write(WD_CONTROL,   0x00);          // no advanced features, polled
    // Real driver's IDSet (post-reset init), verbatim: "JAMREG #$40,SYNC_TRANSFER
    // asynchronous + req/ack=300ns". $40 IS the real async setting — it's not
    // "0 = disabled", $00 may simply not be a valid/functional REQ/ACK timing
    // at all. We had this wrong the whole session.
    a3k_wd_write(WD_SYNC_XFER, 0x40);
    // Official board.i formula: TIMEOUT_VAL = (250ms * Speed(MHz)) / 80. For
    // the A3000's 14MHz WD33C93 clock: (250*14)/80 = 44 ($2C). Our previous
    // value of $14 (20) was an unsourced guess giving ~114ms, not 250ms —
    // real hardware testing showed ASR stuck at BSY-only, completely flat,
    // for the entire software wait window on every empty ID, consistent with
    // the chip's internal timeout never actually completing in that time.
    a3k_wd_write(WD_TIMEOUT_REG, 0x2C);        // 44 decimal = 250ms @ 14MHz
    // Real driver's IDSet, verbatim: "JAMREG #WDCF_ER,SOURCE_ID enable
    // reselection". We always wrote 0 here (in a3k_scsi_inquiry, per-select) —
    // never enabling reselection at all. WDCF_ER = bit 7 ($80).
    a3k_wd_write(WD_SRC_ID, 0x80);

    // Every reset+init pair marks the start of a fresh command batch, so a
    // still-set ISR flag here is by definition STALE — an interrupt that
    // landed after some earlier wait window expired (e.g. a select-timeout
    // arriving late while the identify browser idled at a keypress). Left
    // set, the next wait consumes it as a phantom status and every wait
    // after that is one event behind — which is how stepping through empty
    // IDs in Identify Devices wedged the bus solid on real hw (the real
    // disk's SELECT succeeded on the bus but was declared "No device" off
    // a stale status and abandoned mid-connection, BSY/LED latched on).
    // Same failure class as the DMA path's phantom "$00 at SELECT" fix.
    a3k_globals()->ScsiIrqPending = 0;
}

// Print the "WD33C93: ..." identification line: chip variant + microcode
// revision. Chip-only, zero SCSI bus activity. Issues its own EAF reset
// because the microcode revision is only DEFINED right after a completed
// RESET — it parks in the first CDB register, which is exactly when Linux's
// reset_wd33c93() reads it. Variant discrimination is Linux wd33c93_init()'s
// technique: the QUEUE TAG register ($1A) physically exists only on the
// 33C93B, so a write/readback latches only there — done with a bus-scrub
// read from a different chip (Ramsey) between write and readback per this
// file's anti-echo rule, TWO complementary patterns so a stuck value can't
// fake a B, and the original byte restored. AMD's AM33C93A second-source is
// indistinguishable from a WD "A" in software — only the package marking
// knows. Returns 1 if the chip answered the reset (line printed either
// way), and leaves the chip freshly reset + ready, so callers can continue
// straight into their own setup.
static int a3kPrintWdChip(void)
{
    a3k_wd_write(WD_OWN_ID, WD_OWN_ID_VAL | WD_OWN_ID_EAF);
    a3k_wd_write(WD_COMMAND, WDCMD_RESET);
    uint8_t rst = a3k_wd_wait_int();
    print("WD33C93: ", WHITE);
    if (rst != WDSTS_RESET && rst != WDSTS_RESET_AF) {
        print("not responding\n", RED);
        return 0;
    }

    // B-discrimination only for parts that answered the EAF reset with $01,
    // exactly as Hooper's sdmac.c gates it — a genuine 33C93B ALWAYS does.
    // Probing unconditionally is how a real-hw report (Bruce, AA3k+ZZ9000,
    // 2026-08-11) got the self-contradictory "33C93B (no Advanced
    // Features)": on a plain-$00 part the QUEUE_TAG address is undecoded,
    // and one bus-scrub read evidently isn't always enough to keep a
    // busy/loaded bus from echoing the pattern back anyway.
    int isB = 0;
    if (rst == WDSTS_RESET_AF) {
        uint8_t save = a3k_wd_read(WD_QUEUE_TAG);
        a3k_wd_write(WD_QUEUE_TAG, 0xA5);
        (void)*RAMSEY_VERSION_ADDR;         // scrub the bus so a float can't echo the pattern
        isB = (a3k_wd_read(WD_QUEUE_TAG) == 0xA5);
        if (isB) {
            a3k_wd_write(WD_QUEUE_TAG, 0x5A);
            (void)*RAMSEY_VERSION_ADDR;
            isB = (a3k_wd_read(WD_QUEUE_TAG) == 0x5A);
        }
        a3k_wd_write(WD_QUEUE_TAG, save);
    }

    // Microcode revision needs its own RESET with RAF set (see
    // WD_OWN_ID_RAF) — only meaningful on A/B parts, and the chip must not
    // be LEFT in RAF mode, so a normal EAF reset follows the read.
    uint8_t ucode = 0;
    if (rst == WDSTS_RESET_AF) {
        a3k_wd_write(WD_OWN_ID, WD_OWN_ID_VAL | WD_OWN_ID_EAF | WD_OWN_ID_RAF);
        a3k_wd_write(WD_COMMAND, WDCMD_RESET);
        (void)a3k_wd_wait_int();
        ucode = a3k_wd_read(WD_CDB1);
        a3k_wd_wait_ready();
        a3k_wd_write(WD_OWN_ID, WD_OWN_ID_VAL | WD_OWN_ID_EAF);
        a3k_wd_write(WD_COMMAND, WDCMD_RESET);
        (void)a3k_wd_wait_int();
    }

    if (isB)
        print("33C93B", CYAN);
    else if (rst == WDSTS_RESET_AF)
        print("33C93A", CYAN);
    else
        print("33C93 (or early A)", CYAN);
    print("  microcode $", WHITE);
    print(binHexByte(ucode), CYAN);
    // Package-marking decode for A-parts, from Hooper's sample table
    // ($0D would be a 33C93B, covered by the B branch above already).
    if (rst == WDSTS_RESET_AF && !isB) {
        if (ucode == 0x00)      print(" (00-03/00-04)", CYAN);
        else if (ucode == 0x08) print(" (00-06/AM33C93A)", CYAN);
        else if (ucode == 0x09) print(" (00-08)", CYAN);
    }
    print(rst == WDSTS_RESET_AF ? "  (Advanced Features)\n"
                                : "  (no Advanced Features)\n", WHITE);
    a3k_wd_wait_ready();
    return 1;
}

// Wait for INT, returning SCSI_STATUS. 0xFF = our poll timed out. Same
// real-time-budget and ISR-race notes as a3k_wd_wait_int() above.
static uint8_t a3k_wd_wait_status(void)
{
    volatile struct GlobalVars *globals = a3k_globals();
    for (int i = 0; i < A3K_WAIT_ITERS; i++) {
        if (globals->ScsiIrqPending) {
            globals->ScsiIrqPending = 0;
            return globals->ScsiIrqStatus;
        }
        uint8_t asr = a3k_wd_aux();
        if (asr != 0xFF && (asr & WD_ASR_INT))
            return a3k_wd_read(WD_SCSI_STATUS);
        waitShort();
        if ((i & 15) == 15) hddInputService();   // only fires after ~10ms of waiting
    }
    return 0xFF;
}

// Pump `count` bytes to/from WD_DATA for the phase TRANSFER_INFO was just
// issued for. Returns 1 if all `count` bytes moved, 0 if an interrupt fired
// first (phase ended early — target wants something else, or an error).
static int a3k_wd_pump(uint8_t *buf, int count, int read_dir)
{
    volatile struct GlobalVars *globals = a3k_globals();
    int idx = 0;
    uint32_t t = 1500000UL;
    while (t--) {
        // t resets to a non-multiple-of-64K after every byte, so this only
        // ever fires after ~65K consecutive empty polls — pure wait time.
        if ((t & 0xFFFF) == 0) hddInputService();
        if (globals->ScsiIrqPending) return 0;   // phase ended early — ISR already caught it
        uint8_t asr = a3k_wd_aux();
        if (asr == 0xFF) continue;
        if (asr & WD_ASR_DBR) {
            if (read_dir) buf[idx] = a3k_wd_read(WD_DATA);
            else          a3k_wd_write(WD_DATA, buf[idx]);
            idx++;
            if (idx >= count) return 1;
            t = 1500000UL;
        } else if (asr & WD_ASR_INT) {
            return 0;   // phase ended before count exhausted
        }
    }
    return 0;
}

// Issue TRANSFER_INFO for the current phase and pump `count` bytes, then wait
// for the next status byte. Returns that status (0xFF = our poll timed out).
static uint8_t a3k_wd_do_phase(uint8_t *buf, int count, int read_dir)
{
    // Real bug: WD_XFER_CNT is a 24-bit register (H/M/L), but this only ever
    // wrote the low byte, leaving H/M hardcoded to 0 — for count >= 256,
    // (uint8_t)count silently truncates (e.g. 512 -> 0x00), telling the chip
    // to transfer ZERO bytes instead of the real amount. Every caller before
    // the SCSI READ(10) block-read path stayed under 256 bytes (INQUIRY=36,
    // READ CAPACITY=8), so this never showed up until a real 512-byte sector
    // read needed it — every single read failed identically, exactly what a
    // "chip told to transfer 0 bytes" bug looks like.
    a3k_wd_write(WD_XFER_CNT_H, (uint8_t)(count >> 16));
    a3k_wd_write(WD_XFER_CNT_M, (uint8_t)(count >> 8));
    a3k_wd_write(WD_XFER_CNT_L, (uint8_t)count);
    a3k_wd_write(WD_CONTROL, 0x00);
    a3k_wd_write(WD_COMMAND, WDCMD_TRANSFER_INFO);
    if (a3k_wd_pump(buf, count, read_dir))
        return a3k_wd_wait_status();   // count exhausted, wait for completion INT
    // INT already pending from a3k_wd_pump's early exit — consume via the ISR
    // flag if it won the race, else fall back to a direct read.
    volatile struct GlobalVars *globals = a3k_globals();
    if (globals->ScsiIrqPending) {
        globals->ScsiIrqPending = 0;
        return globals->ScsiIrqStatus;
    }
    return a3k_wd_read(WD_SCSI_STATUS);
}

// Issue TRANSFER_INFO in Single Byte Transfer mode and pump exactly one
// byte, then wait for the next status. See WD_CMD_SBT comment above for why
// this — not a 1-byte count-register TRANSFER_INFO — is required for
// MSG_OUT/STATUS/MSG_IN. Returns the next status (0xFF = our poll timed out).
static uint8_t a3k_wd_do_phase_sbt(uint8_t *byteBuf, int read_dir)
{
    a3k_wd_write(WD_CONTROL, 0x00);
    a3k_wd_write(WD_COMMAND, WD_CMD_SBT | WDCMD_TRANSFER_INFO);

    uint32_t t = 500000UL;
    int gotDbr = 0;
    while (t--) {
        if ((t & 0xFFFF) == 0) hddInputService();
        uint8_t asr = a3k_wd_aux();
        if (asr != 0xFF && (asr & WD_ASR_DBR)) { gotDbr = 1; break; }
    }
    if (!gotDbr) return 0xFF;

    if (read_dir) *byteBuf = a3k_wd_read(WD_DATA);
    else          a3k_wd_write(WD_DATA, *byteBuf);

    return a3k_wd_wait_status();
}

// After the MSG_IN byte, the target leaves ACK asserted (status $20 —
// Group_2 "transfer paused with ack asserted (msg in)") until the initiator
// explicitly releases it. Real driver's ISR handles this exact status by
// issuing NEGATE_ACK (scsitask.asm G2.0000 handler). Skipping this leaves
// the WD33C93 mid-transaction, so a LATER command's SELECT gets rejected
// outright as "$40 invalid command" — this was the actual cause of "only
// ever one clean transaction per boot" across every prior session. See
// [[project_diagrom_a3000_scsi]] memory for the full history.
static void a3k_wd_negate_ack_if_paused(uint8_t st)
{
    if ((st & 0xF0) == 0x20 && (st & 0x0F) == 0x00) {
        a3k_wd_write(WD_COMMAND, WDCMD_NEGATE_ACK);
        a3k_wd_wait_status();   // wait for the resulting disconnect; result not needed here
    }
}

// Best-effort release of an abandoned transaction, called on EVERY failure
// exit from the phase walks. Before this existed, a failed walk just
// returned with the target still connected — and the next command's WD
// soft-RESET resets only the CHIP, never the target, so the target kept
// holding BSY waiting for its phase to be served: bus wedged until power
// cycle (every later SELECT st=$FF, ASR $20/$21, survives Ctrl-A-A because
// the target is its own controller). Confirmed on real hw 2026-08-11
// (cdh's A3000 + ZuluSCSI): one target latency spike beyond our poll
// timeout mid-soak became a permanent wedge, and a slow LOG SENSE behind
// the SMART key did the same temporarily. The only true bus reset (SDMAC
// PRESET) is banned — it hard-hung a real A3000T (see SDMAC_CNTR_PRESET) —
// so instead run the leftover transaction to COMPLETION the SCSI-correct
// way: ABORT any stuck WD command, assert ATN, then serve whatever phases
// the target still requests — IN bytes drained and discarded (bulk gulps
// for DATA_IN), MSG_OUT answered with the ABORT message ($06: "initiator
// aborts, go bus-free", honored since SCSI-1), other OUT phases fed zero
// filler until the target gives up on the garbage and completes. Bounded
// at every level — this must only ever improve a wedge, never become one.
#define A3K_BAILOUT_STEPS 96   // full abandoned 4KB DATA_IN = 64 gulps + phase tail
static void a3k_scsi_bailout(uint8_t lastSt)
{
    uint8_t junk[64];
    int atnSent = 0, deadWaits = 0;

    uint8_t asr = a3k_wd_aux();
    if (asr == 0xFF) return;                     // bus float — nothing to release

    // Chip still executing (a SELECT that can never win a held bus, or a
    // TRANSFER_INFO the target stopped serving): ABORT to get it back first.
    if (asr & (WD_ASR_BSY | WD_ASR_CIP)) {
        a3k_wd_write(WD_COMMAND, WDCMD_ABORT);
        lastSt = a3k_wd_wait_status();
    }

    for (int step = 0; step < A3K_BAILOUT_STEPS; step++) {
        if (lastSt == 0xFF) {
            asr = a3k_wd_aux();
            if (asr == 0xFF ||
                !(asr & (WD_ASR_INT | WD_ASR_DBR | WD_ASR_BSY | WD_ASR_CIP)))
                return;                          // chip idle, nothing pending
            if (++deadWaits >= 2) return;        // bus truly dead — stop meddling
            lastSt = a3k_wd_wait_status();
            continue;
        }
        deadWaits = 0;

        uint8_t grp = lastSt & 0xF0, ph = lastSt & 0x0F;
        if (grp == 0x20 && ph == 0x00) {         // transfer paused with ACK held
            a3k_wd_write(WD_COMMAND, WDCMD_NEGATE_ACK);
            lastSt = a3k_wd_wait_status();
            continue;
        }
        if (ph < 0x8 || (grp != 0x10 && grp != 0x20 && grp != 0x40 && grp != 0x80))
            return;   // completed / selection timeout / disconnected / unknown — done

        // Target still requesting a phase. Ask for MSG_OUT once via ATN so
        // it gives us a chance to send the ABORT message, then serve what
        // it actually asks for until it lets go.
        if (!atnSent) {
            a3k_wd_write(WD_COMMAND, WDCMD_ASSERT_ATN);   // no INT of its own
            atnSent = 1;
        }
        if (ph == WDPHASE_MSG_OUT) {
            uint8_t abortMsg = 0x06;
            lastSt = a3k_wd_do_phase_sbt(&abortMsg, 0);
        } else if (ph == WDPHASE_DATA_IN) {
            lastSt = a3k_wd_do_phase(junk, (int)sizeof(junk), 1);
        } else if (ph == WDPHASE_STATUS || ph == WDPHASE_MSG_IN) {
            junk[0] = 0;
            lastSt = a3k_wd_do_phase_sbt(&junk[0], 1);
        } else {                                 // DATA_OUT / CMD: feed filler
            junk[0] = 0;
            lastSt = a3k_wd_do_phase_sbt(&junk[0], 0);
        }
    }
}

// SELECT_WITH_ATN for one target. No DPD bit on DEST_ID — that only applies
// to the autonomous Select-and-Transfer command, never the manual
// phase-by-phase approach used here (confirmed against both the real
// driver's DoSelect(), which writes only the raw unit, and the WD33C93B
// datasheet's own DST_ID bit description). Returns the resulting status.
static uint8_t a3k_scsi_select(uint8_t unit)
{
    *SDMAC_CNTR = SDMAC_CNTR_PDMD;                     // INTENA off while selecting (real driver's fix)
    a3k_wd_write(WD_DST_ID, unit);
    a3k_wd_write(WD_COMMAND, WDCMD_SELECT_WITH_ATN);
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA; // INTENA back on

    return a3k_wd_wait_status();
}

// Runs one full SCSI command against `unit`: SELECT -> MSG_OUT(IDENTIFY) ->
// CMD -> DATA_IN -> STATUS -> MSG_IN, translated directly from the real
// scsidisk.device driver (scsitask.asm) and confirmed end-to-end on real
// A3000 hardware. Returns 1 only on a clean COMMAND COMPLETE.
static int a3k_scsi_command(uint8_t unit, const uint8_t *cdb, int cdbLen,
                             uint8_t *dataBuf, int dataLen)
{
    // Every failure exit runs a3k_scsi_bailout(st) first: abandoning the
    // walk with the target still connected is what wedged real buses (see
    // the bailout's comment). The final status/msg check does NOT bail out —
    // by then the transaction is complete and the target already released.
    uint8_t st = a3k_scsi_select(unit);
    if (!((st & 0xF0) == 0x80 && (st & 0x0F) == WDPHASE_MSG_OUT)) {
        a3k_scsi_bailout(st); return 0;
    }

    uint8_t identify = 0x80;   // IDENTIFY, LUN 0, no DiscPriv — a scan has no reselect handling
    st = a3k_wd_do_phase_sbt(&identify, 0);
    if (!((st & 0x0F) == WDPHASE_CMD && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) {
        a3k_scsi_bailout(st); return 0;
    }

    st = a3k_wd_do_phase((uint8_t *)cdb, cdbLen, 0);
    if (!((st & 0x0F) == WDPHASE_DATA_IN && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) {
        a3k_scsi_bailout(st); return 0;
    }

    st = a3k_wd_do_phase(dataBuf, dataLen, 1);
    if (!((st & 0x0F) == WDPHASE_STATUS && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) {
        a3k_scsi_bailout(st); return 0;
    }

    uint8_t statusByte = 0xFF;
    st = a3k_wd_do_phase_sbt(&statusByte, 1);
    if (!((st & 0x0F) == WDPHASE_MSG_IN && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) {
        a3k_scsi_bailout(st); return 0;
    }

    uint8_t msgByte = 0xFF;
    st = a3k_wd_do_phase_sbt(&msgByte, 1);
    a3k_wd_negate_ack_if_paused(st);

    return (statusByte == 0 && msgByte == 0);
}

// READ CAPACITY(10) response decode: bytes 0-3 = last valid LBA, bytes 4-7 =
// block size (both big-endian). Shifts only, no multiply/divide — this
// freestanding build has no __mulsi3/__divsi3 (a real link error hit this
// exact gap earlier), so only the common 512-byte-block case gets a
// friendly MB total.
static void a3k_print_capacity(const uint8_t *cap)
{
    uint32_t lastLba = ((uint32_t)cap[0] << 24) | ((uint32_t)cap[1] << 16)
                      | ((uint32_t)cap[2] << 8)  |  (uint32_t)cap[3];
    uint32_t blockSize = ((uint32_t)cap[4] << 24) | ((uint32_t)cap[5] << 16)
                        | ((uint32_t)cap[6] << 8)  |  (uint32_t)cap[7];
    if (blockSize != 512) {
        print("(non-512B blocks)", YELLOW);
        return;
    }
    uint32_t sizeMB = ((lastLba + 1) >> 1) >> 10;
    print(binDec(sizeMB), CYAN);
    print(" MB", WHITE);
}

// LOG SENSE response layout (SPC): byte 0=page code, 1=subpage, 2-3=page
// length (big-endian), then one or more parameters: 4-5=parameter code,
// 6=control byte, 7=parameter length, 8+=parameter value. Both pages we use
// here have their first (and only) parameter at code $0000, so the value
// always starts at byte 8 regardless of page — shared by A3000 and A4000T
// SCSI, same as a3k_print_capacity() above.
static void a3k_print_smart_ie(const uint8_t *buf, int len)
{
    if (len < 10) { print("  Status:   (short reply)\n", YELLOW); return; }
    uint8_t asc  = buf[8];
    uint8_t ascq = buf[9];
    print("  Status:   ", WHITE);
    if (asc == 0x00 && ascq == 0x00) {
        print("OK\n", GREEN);
    } else if (asc == 0x5D) {
        print("FAILURE PREDICTED! (ASC=$5D ASCQ=$", RED);
        print(binHexByte(ascq), RED);
        print(")\n", RED);
    } else {
        print("ASC=$", YELLOW); print(binHexByte(asc), YELLOW);
        print(" ASCQ=$", YELLOW); print(binHexByte(ascq), YELLOW);
        print(" (informational)\n", YELLOW);
    }
}

// Temperature page's first parameter value is 2 bytes: reserved, then
// current temperature in Celsius ($FF = "not available"). Best-effort only
// - plenty of real drives don't support this page at all, so the caller
// just skips this line silently on failure rather than reporting an error.
static void a3k_print_smart_temp(const uint8_t *buf, int len)
{
    if (len < 10 || buf[9] == 0xFF) return;
    print("  Temp:     ", WHITE); print(binDec(buf[9]), CYAN); print(" C\n", WHITE);
}

static void a3k_scsi_strip(char *s, int len)
{
    s[len] = '\0';
    for (int i = len - 1; i >= 0 && (s[i] == ' ' || s[i] == '\0'); i--)
        s[i] = '\0';
    // replace non-printable
    for (int i = 0; s[i]; i++)
        if ((unsigned char)s[i] < 0x20) s[i] = '?';
}

static const char * const scsiDevTypes[] = {
    "DISK", "TAPE", "PRINT", "PROC", "WORM", "CDROM", "SCAN", "OPT",
    "JUKE", "COMM", NULL, NULL, NULL, NULL, "ENCL", NULL
};

static int ncr710Present(void);   // defined in the A4000T section below

static int detectA3000SCSI(void)
{
    if (isGayleMachine()) return 0;   // $DD00xx is unterminated on A600/A1200 — the very
                                      // first SDMAC access below froze a real A1200 solid
                                      // (2026-07-25); see isGayleMachine()
    if (ncr710Present()) return 0;    // a real 53C710 answered at $DD0040: this is an A4000T,
                                      // and the WD/SDMAC accesses below would scribble the NCR
                                      // (mirror image of the A4000T-on-AA3000+ false green);
                                      // see ncr710Present()
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;   // SCSI mode + let status latch
    if (*WD_AUX_STATUS == 0xFF) return 0;   // bus float — nothing there
    // Issue RESET and check for completion interrupt
    a3k_globals()->ScsiIrqPending = 0;   // no ISR installed here — make a3k_wd_wait_int()
                                         // poll the chip, not a stale flag a previous
                                         // scan's ISR may have left set
    a3k_wd_write(WD_OWN_ID, WD_OWN_ID_VAL);   // must precede RESET — clock divisor latches at reset
    a3k_wd_write(WD_COMMAND, WDCMD_RESET);
    // Real-time-bounded wait (~320ms via waitShort(), same helper the scan
    // path already uses for this exact RESET) — the old raw 2000000-iteration
    // spin here was CPU-speed-dependent in both directions: a fast 68060
    // could burn through it before the chip's reset completed, and on a
    // wrong-machine open bus it stalled the select menu for many seconds.
    uint8_t st = a3k_wd_wait_int();
    return (st == WDSTS_RESET || st == WDSTS_RESET_AF);
}

static int scanA3000SCSI(void)
{
    volatile struct GlobalVars *globals = a3k_globals();

    *SDMAC_DAWR = DAWR_A3000_VAL;
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;   // SCSI mode + let status latch
    print("\nA3000/A3000T SCSI (WD33C93)\n", WHITE);

    uint8_t ramseyVer = *RAMSEY_VERSION_ADDR;
    print("Ramsey version: $", WHITE);
    print(binHexByte(ramseyVer), ramseyVer == ANCIENT_RAMSEY ? YELLOW : CYAN);
    print(ramseyVer == ANCIENT_RAMSEY ? "  (ancient/pre-production)\n" : "\n", WHITE);

    if (a3k_wd_aux() == 0xFF) {
        print("WD33C93: not responding (bus float)\n", RED);
        return 0;
    }

    // Full chip identification (variant + microcode revision + AF) — the
    // helper's own EAF reset replaces the plain "present/AF" reset that
    // used to live here, and leaves the chip equally reset + ready.
    if (!a3kPrintWdChip())
        return 0;

    // Enable the real PORTS (IRQ2) interrupt for the duration of the scan.
    // All Amiga HD controllers share this line, and it's edge/level-latched:
    // without something actually acknowledging it at the Paula level, it can
    // get stuck after the first event and never re-arm for later ones, no
    // matter how much we re-poll the WD33C93's own ASR directly.
    a3k_scsi_irq_enable(globals);
    print("\nScanning IDs 0-7...\n\n", WHITE);

    // Must be real stack locals, not `static` — statics land in ROM in this
    // freestanding build (past endofcode), so the chip's DMA/PIO writes into
    // them would be silently discarded.
    uint8_t inqBuf[SCSI_INQUIRY_LEN];
    uint8_t capBuf[SCSI_READ_CAPACITY_LEN];
    static const uint8_t inquiryCdb[6]    = { SCSI_INQUIRY, 0, 0, 0, SCSI_INQUIRY_LEN, 0 };
    static const uint8_t readCapCdb[10]   = { SCSI_READ_CAPACITY, 0,0,0,0,0,0,0,0, 0 };
    int found = 0;

    for (int id = 0; id <= 7; id++) {
        // Full reset before every ID, not just once at the top of the scan —
        // a prior ID's transaction can leave the chip in a non-idle state
        // that would make a later ID's SELECT fail for reasons unrelated to
        // whether anything is actually there.
        a3k_wd_reset();
        a3k_wd_init();

        uint32_t irqBefore = globals->ScsiIrqCount;
        int gotInquiry = a3k_scsi_command(id, inquiryCdb, 6, inqBuf, SCSI_INQUIRY_LEN);
        int gotCapacity = gotInquiry &&
            a3k_scsi_command(id, readCapCdb, 10, capBuf, SCSI_READ_CAPACITY_LEN);
        int ack = (globals->ScsiIrqCount != irqBefore);

        print("  ID ", WHITE);
        char idch[2] = { (char)('0' + id), 0 };
        print(idch, CYAN);
        print(ack ? "  [IRQ2 ACK]  " : "  [IRQ2 NAK]  ", ack ? GREEN : YELLOW);

        if (!gotInquiry) {
            print("-\n", RED);
            continue;
        }

        uint8_t devtype = inqBuf[0] & 0x1F;
        char vendor[9], product[17], revision[5];
        memcpy(vendor,   inqBuf + 8,  8);  a3k_scsi_strip(vendor,   8);
        memcpy(product,  inqBuf + 16, 16); a3k_scsi_strip(product,  16);
        memcpy(revision, inqBuf + 32, 4);  a3k_scsi_strip(revision,  4);
        char *dtype = (devtype < 16 && scsiDevTypes[devtype]) ?
                          (char *)scsiDevTypes[devtype] : "???";

        print(dtype, YELLOW);
        print("  \"", WHITE);
        print(vendor,   GREEN);
        print(" ",      WHITE);
        print(product,  GREEN);
        if (revision[0]) { print(" ", WHITE); print(revision, CYAN); }
        print("\"", WHITE);
        if (gotCapacity) { print("  ", WHITE); a3k_print_capacity(capBuf); }
        print("\n", WHITE);
        found++;
    }

    a3k_scsi_irq_disable();
    print("\nIRQ2 (PORTS) events serviced: ", WHITE);
    print(binDec(globals->ScsiIrqCount), globals->ScsiIrqCount ? GREEN : RED);
    print("\n", WHITE);

    return found;
}

// Manual 32-bit multiply via shift-and-add. This freestanding build has no
// __mulsi3 (see a3k_print_capacity's comment above — a real link error hit
// this exact gap before), so computing a partition's size in blocks from
// CHS geometry (cylSpan * heads * spt) can't just use the `*` operator.
// Uses only +, <<, >>, & — all native 68000 instructions, no libgcc call.
static uint32_t mul32(uint32_t a, uint32_t b)
{
    uint32_t result = 0;
    while (b) {
        if (b & 1) result += a;
        a <<= 1;
        b >>= 1;
    }
    return result;
}

// Same 512-byte-block-only shift-only MB conversion as a3k_print_capacity,
// but from a plain block count (used for partition sizes) rather than a
// raw READ CAPACITY response.
static void a3k_print_blocks_mb(uint32_t blocks)
{
    uint32_t sizeMB = (blocks >> 1) >> 10;
    print(binDec(sizeMB), CYAN);
    print(" MB", WHITE);
}

#define SCSI_READ10 0x28

// Single-block READ(10) — reuses a3k_scsi_command()'s generic phase
// machinery exactly like INQUIRY/READ CAPACITY do, just with a different
// CDB. `unit` must already have had a3k_wd_reset()/a3k_wd_init() run once
// for this batch of commands (same convention as INQUIRY+READ CAPACITY
// back-to-back in scanA3000SCSI() — no extra reset needed between calls to
// the same already-selected unit).
static int a3k_scsi_read_block(uint8_t unit, uint32_t lba, uint8_t *buf)
{
    // Unlike a3k_identify_unit()'s INQUIRY-then-READ-CAPACITY pair (two
    // commands back to back with no intervening reset, proven fine), the
    // RDB walk can issue a dozen-plus commands to the same unit in a row.
    // This chip's real-hardware behavior has been finicky enough all
    // session (see [[project_diagrom_a3000_scsi]]) that a fresh reset per
    // command here is the safe default rather than assuming state survives
    // an arbitrary run length — every read failing uniformly (not just
    // later ones) when this was missing is consistent with the chip
    // needing this, not just a "some later command in the chain" issue.
    a3k_wd_reset();
    a3k_wd_init();

    uint8_t cdb[10] = {
        SCSI_READ10, 0,
        (uint8_t)(lba >> 24), (uint8_t)(lba >> 16), (uint8_t)(lba >> 8), (uint8_t)lba,
        0, 0, 1, 0   // transfer length = 1 block
    };
    return a3k_scsi_command(unit, cdb, 10, buf, 512);
}

// Read and print everything available about one SCSI ID: INQUIRY, READ
// CAPACITY, and — for direct-access disks — RDB geometry + full partition
// list. The RDB/partition walk (RDB_MAGIC/PART_MAGIC/RD32/printDosType) is
// the same shared, file-scope logic doIdentifyIDE() already uses for IDE;
// only the sector-read primitive differs (SCSI READ(10) vs IDE PIO).
static void a3k_identify_unit(uint8_t unit)
{
    uint8_t inqBuf[SCSI_INQUIRY_LEN];
    uint8_t capBuf[SCSI_READ_CAPACITY_LEN];
    uint8_t blk[512];
    static const uint8_t inquiryCdb[6]  = { SCSI_INQUIRY, 0, 0, 0, SCSI_INQUIRY_LEN, 0 };
    static const uint8_t readCapCdb[10] = { SCSI_READ_CAPACITY, 0,0,0,0,0,0,0,0, 0 };

    a3k_wd_reset();
    a3k_wd_init();

    if (!a3k_scsi_command(unit, inquiryCdb, 6, inqBuf, SCSI_INQUIRY_LEN)) {
        print("  No device at this ID.", RED);
        // Broken-by-default visibility: a clean selection timeout leaves the
        // chip idle. BSY/CIP still set here means the probe was abandoned
        // mid-command — the wedged-bus signature, not an empty slot.
        uint8_t asr = a3k_wd_aux();
        if (asr != 0xFF && (asr & (WD_ASR_BSY | WD_ASR_CIP))) {
            print("  (WD still busy, ASR $", YELLOW);
            print(binHexByte(asr), YELLOW);
            print(" - bus may be wedged)", YELLOW);
        }
        print("\n", WHITE);
        return;
    }

    uint8_t devtype = inqBuf[0] & 0x1F;
    char vendor[9], product[17], revision[5];
    memcpy(vendor,   inqBuf + 8,  8);  a3k_scsi_strip(vendor,   8);
    memcpy(product,  inqBuf + 16, 16); a3k_scsi_strip(product,  16);
    memcpy(revision, inqBuf + 32, 4);  a3k_scsi_strip(revision,  4);
    char *dtype = (devtype < 16 && scsiDevTypes[devtype]) ?
                      (char *)scsiDevTypes[devtype] : "???";

    print("  Type:     ", WHITE); print(dtype, YELLOW); print("\n", WHITE);
    print("  Name:     ", WHITE);
    print(vendor, GREEN); print(" ", WHITE); print(product, GREEN);
    if (revision[0]) { print(" ", WHITE); print(revision, CYAN); }
    print("\n", WHITE);

    if (a3k_scsi_command(unit, readCapCdb, 10, capBuf, SCSI_READ_CAPACITY_LEN)) {
        print("  Capacity: ", WHITE);
        a3k_print_capacity(capBuf);
        print("\n", WHITE);
    }

    if (devtype != 0) {   // RDB/partitions only meaningful for direct-access disks
        print("\n", WHITE);
        return;
    }

    int rdb_found = 0;
    uint32_t rdb_heads = 0, rdb_spt = 0, partblock = NO_LIST;
    for (uint32_t b = 0; b < 16 && !rdb_found; b++) {
        // TEMP DIAGNOSTIC — remove once RDB-not-found on a known-partitioned
        // disk (Clean323.hdf) is root-caused. Distinguishes "READ(10) itself
        // failed" from "read succeeded but magic didn't match".
        int readOk = a3k_scsi_read_block(unit, b, blk);
        print("    [blk ", WHITE); print(binDec(b), CYAN);
        print(readOk ? " read OK, first4=$" : " READ FAILED", readOk ? WHITE : RED);
        if (readOk) {
            print(binHexByte(blk[0]), CYAN); print(binHexByte(blk[1]), CYAN);
            print(binHexByte(blk[2]), CYAN); print(binHexByte(blk[3]), CYAN);
        }
        print("]\n", WHITE);
        if (!readOk) continue;
        if (RD32(blk, 0) != RDB_MAGIC) continue;
        rdb_found = 1;

        uint32_t rdb_cyls = RD32(blk, 0x40);
        rdb_spt   = RD32(blk, 0x44);
        rdb_heads = RD32(blk, 0x48);
        partblock = RD32(blk, 0x1C);

        print("  Geometry: ", WHITE);
        print(binDec(rdb_cyls),  WHITE); print("c / ", WHITE);
        print(binDec(rdb_heads), WHITE); print("h / ", WHITE);
        print(binDec(rdb_spt),   WHITE); print("s\n", WHITE);
    }

    if (!rdb_found) {
        print("  (no RDB found)\n\n", YELLOW);
        return;
    }

    print("  Partitions:\n", WHITE);
    char name[32];
    int nparts = 0;
    while (partblock != NO_LIST && nparts < 32) {
        if (!a3k_scsi_read_block(unit, partblock, blk)) break;
        if (RD32(blk, 0) != PART_MAGIC) break;

        uint32_t next    = RD32(blk, 0x10);
        uint32_t lowcyl  = RD32(blk, 0xA4);
        uint32_t highcyl = RD32(blk, 0xA8);
        uint32_t dostype = RD32(blk, 0xC0);

        uint8_t namelen = blk[0x24];
        if (namelen > 30) namelen = 30;
        for (int i = 0; i < namelen; i++) name[i] = (char)blk[0x25 + i];
        name[namelen]   = ':';
        name[namelen+1] = '\0';

        uint32_t cylSpan    = highcyl - lowcyl + 1;
        uint32_t partBlocks = mul32(mul32(cylSpan, rdb_heads), rdb_spt);

        print("    ", WHITE); print(name, GREEN); print("  ", WHITE);
        a3k_print_blocks_mb(partBlocks);
        print("  [", WHITE); print(binDec(lowcyl), WHITE); print("-", WHITE);
        print(binDec(highcyl), WHITE); print("]  ", WHITE);
        printDosType(dostype);
        print("\n", WHITE);

        nparts++;
        partblock = next;
    }
    if (nparts == 0) print("    (none)\n", YELLOW);
    print("\n", WHITE);
}

// Real SCSI SMART via LOG SENSE - see the SCSI_LOG_SENSE block of #defines
// and a3k_print_smart_ie()/a3k_print_smart_temp() above for the protocol
// background. Returns 1 if the device answered LOG SENSE at all (matches
// doSmartIDEUnit()'s "found" convention), even if the IE page reports OK.
static int a3k_smart_unit(uint8_t unit)
{
    uint8_t buf[SCSI_LOGSENSE_LEN];
    static const uint8_t ieCdb[10]   = { SCSI_LOG_SENSE, 0, (1<<6)|SCSI_LOGPAGE_IE,   0,0,0,0,0, 0, SCSI_LOGSENSE_LEN };
    static const uint8_t tempCdb[10] = { SCSI_LOG_SENSE, 0, (1<<6)|SCSI_LOGPAGE_TEMP, 0,0,0,0,0, 0, SCSI_LOGSENSE_LEN };

    if (!a3k_scsi_command(unit, ieCdb, 10, buf, SCSI_LOGSENSE_LEN)) {
        print("  SMART/Log Sense not supported by this device.\n", YELLOW);
        return 0;
    }
    a3k_print_smart_ie(buf, SCSI_LOGSENSE_LEN);

    if (a3k_scsi_command(unit, tempCdb, 10, buf, SCSI_LOGSENSE_LEN))
        a3k_print_smart_temp(buf, SCSI_LOGSENSE_LEN);

    return 1;
}

// Whole-bus wrapper (matches doSmartIDE()'s shape) - currently unreachable
// via any menu (SMART was removed as a top-level HDD menu entry, it's only
// reached per-unit via the 'S' key inside Identify Devices - see
// a3kBrowseSmart() below), kept working for HddController's own "smart"
// field and in case a whole-bus entry point returns later.
static int smartA3000SCSI(void)
{
    print("\nSMART Data\n\n", WHITE);
    int found = 0;
    for (uint8_t id = 0; id < 8; id++) {
        if (id == A3K_HOST_SCSI_ID) continue;
        print("ID ", CYAN); print(binDec(id), CYAN); print(":\n", WHITE);
        found += a3k_smart_unit(id);
        print("\n", WHITE);
    }
    return found;
}

// Interactive per-unit browser: SCSI ID 0-7 one at a time, '+'/'-' or
// LEFT/RIGHT mouse to step (wrapping around), 'S' for SMART data on the
// displayed unit, ESC or both mouse buttons together to exit. Returns 1 to
// tell HDDTestC() it already handled its own dismissal (no extra "press any
// key" prompt needed — see the caller).
// Adapters for browseUnits(): no ctx needed, everything's reached via
// global-scope hardware access. skipUnit excludes the host adapter's own
// ID (A3K_HOST_SCSI_ID) — a SELECT to yourself has nothing to respond.
static void a3kBrowseIdentify(void *ctx, int unit) { (void)ctx; a3k_identify_unit((uint8_t)unit); }
static void a3kBrowseSmart(void *ctx, int unit)     { (void)ctx; a3k_smart_unit((uint8_t)unit); }
static int  a3kBrowseSkip(void *ctx, int unit)      { (void)ctx; return unit == A3K_HOST_SCSI_ID; }

static int identifyA3000SCSI(void)
{
    volatile struct GlobalVars *globals = a3k_globals();

    *SDMAC_DAWR = DAWR_A3000_VAL;
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;

    if (a3k_wd_aux() == 0xFF) {
        print("\nWD33C93: not responding (bus float)\n", RED);
        return 1;
    }

    // Same one-time chip warm-up scanA3000SCSI() does before its per-ID
    // loop — an EAF-requesting RESET (also how the chip's Advanced-Features
    // support gets learned) followed by a3k_wd_wait_ready(). Don't assume
    // this is redundant with the per-unit a3k_wd_reset()/a3k_wd_init() pair
    // used inside a3k_identify_unit() just because it looks similar: this
    // exact chip has needed every one of its proven-working init steps in
    // prior sessions, and skipping this one is what caused every unit to
    // report "No device" the first time this function was tried.
    a3k_wd_write(WD_OWN_ID, WD_OWN_ID_VAL | WD_OWN_ID_EAF);
    a3k_wd_write(WD_COMMAND, WDCMD_RESET);
    uint8_t rst = a3k_wd_wait_int();
    if (rst != WDSTS_RESET_AF && rst != WDSTS_RESET) {
        print("\nWD33C93: not responding\n", RED);
        return 1;
    }
    a3k_wd_wait_ready();
    a3k_scsi_irq_enable(globals);

    int result = browseUnits("A3000/A3000T SCSI - Identify Devices", 8, 0, NULL, NULL,
                              a3kBrowseIdentify, a3kBrowseSmart, a3kBrowseSkip);

    a3k_scsi_irq_disable();
    return result;
}

// ---------------------------------------------------------------------------
// A4000T SCSI  (NCR 53C710, base $DD0040)
// ---------------------------------------------------------------------------
//
// Completely different chip family from A3000's WD33C93 — the 53C710 is a
// "SCRIPTS processor": it executes a small autonomous program from memory
// for anything beyond raw register access (selection, phase sequencing,
// data transfer all go through SCRIPTS, not simple command-register pokes).
// Confirmed against the official AmigaOS a4091/a4000t driver source
// (modifiers.h): base address $DD0040, byte-register offsets below taken
// directly from the driver's `struct ncr710`.
//
// CRITICAL (per that same driver, verbatim): "must set EA in DCNTL _before_
// adding the int server. This is because the first access will never end
// unless the chip is set to link STERM and SLAC internally." I.e. the FIRST
// bus access to this chip after power-on must be the DCNTL write below, or
// later accesses can hang indefinitely — this isn't just wrong data, it's a
// genuine bus hang risk. Never reorder this.

#define NCR_BASE ((volatile uint8_t *)0xDD0040)

// Byte register offsets from NCR_BASE (see struct ncr710 in the official
// a4091/ncr710.h — longword registers like dsa/dsp/temp/scratch are skipped
// here since detect-only doesn't need them yet).
#define NCR_SIEN     0x00   // SCSI interrupt enable
#define NCR_SDID     0x01   // SCSI destination ID
#define NCR_SCNTL1   0x02   // SCSI control register 1
#define NCR_SCNTL0   0x03   // SCSI control register 0
#define NCR_SOCL     0x04   // SCSI output control latch (direct bus signal control)
#define NCR_SODL     0x05   // SCSI output data latch
#define NCR_SXFER    0x06   // SCSI transfer register
#define NCR_SCID     0x07   // SCSI chip ID
#define NCR_SBCL     0x08   // SCSI bus control lines
#define NCR_SSTAT2   0x0C   // SCSI status 2 (read only) — latched current phase in bits 2-0
#define NCR_SSTAT1   0x0D   // SCSI status 1 (read only)
#define NCR_SSTAT0   0x0E   // SCSI status 0 (read only) — select timeout etc.
#define NCR_DSTAT    0x0F   // DMA status (read only) — reading clears DIP
#define NCR_ISTAT    0x22   // interrupt status
#define NCR_SCRATCH  0x34   // 4-byte general-purpose scratchpad (chip offset 0x34-0x37;
                            // the XOR-3 byte-lane swap maps a whole longword onto the
                            // same 4 addresses, so all 4 bytes are plain r/w scratch)
#define NCR_DCNTL    0x38   // DMA control
// Offsets computed from the official ncr710.h struct field order (verified
// against the three offsets above, which all matched already): dien is the
// 3rd byte after dcntl (dcntl=0x38, dwt=0x39, dien=0x3A).
#define NCR_DIEN     0x3A   // DMA interrupt enable

#define NCR_SCNTL1_RST 0x08   // SCNTL1F_RST: force a SCSI bus reset
#define NCR_SCNTL1_ESR 0x20   // SCNTL1F_ESR: Enable Selection/Reselection
#define NCR_DCNTL_EA   0x20   // DCNTLF_EA: enable ALU/STERM-SLAC link (mandatory pre-access setup)
#define NCR_DCNTL_COM  0x01   // DCNTLF_COM: 53C710 (vs 53C700) compatibility mode
#define NCR_SXFER_DHP  0x80   // SXFERF_DHP: disable halt-on-parity-error (needed for async, no parity setup)
#define NCR_OWN_SCSI_ID 7     // conventional host-adapter SCSI ID (matches real driver's default st_OwnID)

#define NCR_ISTAT_DIP  0x01   // DMA (SCRIPTS/interrupt-instruction) interrupt pending
#define NCR_ISTAT_SIP  0x02   // SCSI-side interrupt pending (e.g. select timeout)
#define NCR_SSTAT0_STO 0x20   // SSTAT0F_STO: selection timeout — no device responded
#define NCR_SIEN_STO   0x20   // SIENF_STO: enable select-timeout interrupt (SCSI side, raises SIP)
#define NCR_DIEN_SIR   0x04   // DIENF_SIR: enable "SCRIPTS Interrupt instruction Received" (raises DIP —
                               // this is what our own INT script instruction generates)

// Ported 2026-07-15 from the proven debugcode.c A4000T POC (see
// [[project_diagrom_a4000t_scsi]] memory) — full chip-init sequence, real
// register byte-order fix, and a holistic SELECT->IDENTIFY->INQUIRY SCRIPTS
// program, replacing the old phase-by-phase C-driven dispatch below, which
// was never actually verified working (built on register-write byte-order
// and instruction-encoding bugs found and fixed this same day).
#define NCR_CTEST0    0x17
#define NCR_CTEST7    0x18
#define NCR_DMODE     0x3B
#define NCR_ISTATF_ABRT 0x80
#define NCR_ISTATF_RST  0x40
#define NCR_SCNTL0_EPG  0x04   // enable parity generation
#define NCR_CTEST0_ERF  0x04   // filter REQ/ACK
#define NCR_CTEST0_EAN  0x10   // enable active negation
#define NCR_CTEST0_BTD  0x40   // disable byte-to-byte timer
#define NCR_SBCL_SSCF0  0x01
#define NCR_SBCL_SSCF1  0x02
#define NCR_DMODE_FC2   0x20
#define NCR_DMODE_BL0   0x40
#define NCR_DMODE_BL1   0x80

// Longword registers (dsa/dsp/dsps/etc.) live at these offsets from NCR_BASE.
// The real driver source (ncr.c WRITE_LONG macro) writes longwords via
// offset+0x80 instead, but its own comment says this is "not required, but
// better" — an optional '040 cache-coherency workaround exploiting a real
// motherboard address mirror, not a hardware requirement. Amiberry's
// emulation isn't guaranteed to replicate that specific mirror, so we write
// to the plain offset (same address reads use).
#define NCR_DSP      0x2C   // DMA SCRIPTS pointer — writing this starts execution

// FIXED 2026-07-15 (see [[project_diagrom_a4000t_scsi]] memory for the full
// gdb-verified derivation): a naive 32-bit store here is genuinely wrong
// under Amiberry, not just an '040-cache nicety. Amiberry's A4000T register
// glue (src/ncr_scsi.cpp beswap(), used by ncr710_io_bput()/bget()) swaps
// byte offset+0<->+3 and +1<->+2 within every 4-byte-aligned register group
// on this bus, confirmed directly via gdb breakpoints on Amiberry's own
// lsi_execute_script()/lsi_reg_writeb(): a guest write to DSP's base offset
// landed on the chip's real DSP[24:31] slot - the MSB, and the one that
// actually triggers `lsi_execute_script()`. Four explicit byte writes below,
// value-to-offset order chosen to match that swap, trigger byte (guest
// offset+0) written LAST once the other three bytes are already in place.
static inline void ncr_lwrite(uint8_t off, uint32_t val)
{
    NCR_BASE[off + 3] = (uint8_t)(val & 0xFF);          // -> real DSP[0:7]  (LSB)
    NCR_BASE[off + 2] = (uint8_t)((val >> 8) & 0xFF);   // -> real DSP[8:15]
    NCR_BASE[off + 1] = (uint8_t)((val >> 16) & 0xFF);  // -> real DSP[16:23]
    NCR_BASE[off + 0] = (uint8_t)((val >> 24) & 0xFF);  // -> real DSP[24:31] (MSB) - TRIGGER, written last
}
static inline uint32_t ncr_lread(uint8_t off)
{
    uint32_t v;
    v  = (uint32_t)NCR_BASE[off + 3];
    v |= (uint32_t)NCR_BASE[off + 2] << 8;
    v |= (uint32_t)NCR_BASE[off + 1] << 16;
    v |= (uint32_t)NCR_BASE[off + 0] << 24;
    return v;
}

// ---------------------------------------------------------------------------
// A4000T shares the exact same PORTS (IRQ2, level-2 autovector) interrupt
// line as A3000's WD33C93/SDMAC — see ScsiPortsIRQ/SCSI_PORTS_BIT/a3k_globals/
// a3k_set_sr above (all Amiga HD controllers share this line, confirmed by
// user). Reuses those helpers and the SAME globals->ScsiIrqPending/
// ScsiIrqStatus/ScsiIrqCount fields directly — only one SCSI scan runs at a
// time in this ROM, so there's no collision, and it avoids duplicating
// otherwise-identical struct fields for a second chip family.
// ---------------------------------------------------------------------------
__interrupt void Ncr710PortsIRQ(VARS)
{
    uint16_t irq = custom->intreqr;
    if (irq & SCSI_PORTS_BIT) {
        uint8_t istat = NCR_BASE[NCR_ISTAT];
        if (istat != 0xFF && (istat & (NCR_ISTAT_SIP | NCR_ISTAT_DIP))) {
            globals->ScsiIrqStatus = NCR_BASE[NCR_SSTAT0];
            (void)NCR_BASE[NCR_DSTAT];   // reading this clears DIP
            globals->ScsiIrqPending = 1;
            globals->ScsiIrqCount++;
        }
    }
    custom->intreq = SCSI_PORTS_BIT;
    custom->intreq = SCSI_PORTS_BIT;
}

static void ncr_irq_enable(volatile struct GlobalVars *globals)
{
    globals->ScsiIrqPending = 0;
    globals->ScsiIrqStatus  = 0;
    globals->ScsiIrqCount   = 0;
    *(volatile APTR *) + 0x68 = Ncr710PortsIRQ;
    custom->intreq = SCSI_PORTS_BIT;
    custom->intreq = SCSI_PORTS_BIT;
    custom->intena = 0xC000 | SCSI_PORTS_BIT;
    custom->intena = 0xC000 | SCSI_PORTS_BIT;
    // FIXED 2026-07-15: STO alone leaves UDC (Unexpected Disconnect, bit2)
    // masked - a live-device transaction that pauses mid-transfer (real,
    // normal SCSI behavior) can then never raise a serviceable interrupt.
    // Ported from the proven debugcode.c POC's NCR_SIEN_ALL_BUT_FCMP_SEL.
    NCR_BASE[NCR_SIEN] = 0xAF;   // all SCSI-side interrupt sources except FCMP/SEL
    NCR_BASE[NCR_DIEN] = NCR_DIEN_SIR;
    a3k_set_sr(0x2000);
}

static void ncr_irq_disable(void)
{
    NCR_BASE[NCR_SIEN] = 0;
    NCR_BASE[NCR_DIEN] = 0;
    custom->intena = 0x7fff;
    custom->intreq = 0x7fff;
    *(volatile APTR *) + 0x68 = RTEcode;
}

// SCRIPTS instruction opcodes (verified against NCR 53C710 SCSI I/O Processor
// Programmer's Guide, Oct90, section 5 instruction encoding):
//   bits31-30 = instruction class (00=Block Move, 01=I/O, 10=Transfer Control)
//   Block Move: bits26-24 = SCSI phase (mirrors the MSG/C-D/I-O signal bits,
//     same 3-bit encoding as socl/sbcl: 0=DataOut 1=DataIn 2=Cmd 3=Status
//     6=MsgOut 7=MsgIn), bits23-0 = byte count, 2nd longword = data address.
//   I/O (Select): bits29-27=000 (Select opcode), bit0=ATN, low byte of 2nd
//     word's high byte = target SCSI ID as a BITMASK (1<<id), not a plain
//     value — this is a hardware bus-bitmask, matching how arbitration/
//     selection literally drives the SCSI data bus.
//   Transfer Control (Interrupt): bits29-27=011, control byte 0x00 = always
//     taken (no phase/data compare enabled).
#define NCR_SCRIPT_SELECT_ATN 0x41000000UL   // class=01,opcode=SELECT(000),ATN=1
// FIXED 2026-07-15: Amiberry's Transfer Control dispatch (lsi_execute_script(),
// src/qemuvga/lsi53c710.cpp) treats ANY instruction with none of bits
// 21/19/18/17 set as a silent NOP - checked BEFORE the opcode field is even
// read (`if ((insn & 0x002e0000) == 0) { NOP; break; }`). The old
// 0x98000000 had none of those bits set, so this INT was silently a no-op
// every time - DSP then walked off the end of the script buffer into
// uninitialized garbage. Bit19 set (with no specific compare-type bit)
// makes the interpreter's `cond==jmp` check trivially true, so the
// instruction is genuinely taken unconditionally instead of no-op'd. See
// [[project_diagrom_a4000t_scsi]] memory for the full gdb-verified
// derivation (found and fixed in the debugcode.c POC first, ported here).
#define NCR_SCRIPT_INT         0x98080000UL   // class=10,opcode=INT(011), unconditional (bit19 set)
#define NCR_MOVE_OPCODE_BIT    (1UL << 27)    // mandatory for initiator-mode Block Move
// General "JUMP addr, IF <phase>" form: class=10 (Transfer Control),
// opcode=000 (Jump), bit19=1 (this compare's polarity), bit17=1 (phase
// compare enabled, no carry/data compare), phase value in bits 26:24.
#define NCR_SCRIPT_JUMP_IF(phase) (0x800A0000UL | (((uint32_t)(phase)) << 24))
#define NCR_SCRIPT_MOVE(phase, count) \
    (NCR_MOVE_OPCODE_BIT | (((uint32_t)(phase)) << 24) | ((uint32_t)(count) & 0xFFFFFFUL))

// Full chip-init/reset sequence, ported 2026-07-15 from the proven
// debugcode.c POC's ncrRealInit() (bisect level 4 - every step below turned
// out to be needed, not just the minimal SCID/ESR/SXFER sequence the old
// detectA4000TSCSI() used alone). Real driver source (leaked AmigaOS a4091
// ncr.c) confirmed steps: DCNTL.EA must be the very first access after
// power-on ("the first access will never end unless the chip is set to
// link STERM and SLAC internally" - never reorder this), SCID must hold the
// host adapter's own ID as a bitmask, SCNTL1.ESR only *after* SCID is
// programmed, SXFER.DHP for async/no-parity transfers. Additional steps
// (ABRT/RST pre-pulse, pending-int-clear loop, ~250ms post-reset settle,
// CTEST0/CTEST7, SCNTL0 parity, SBCL/DMODE) came from a from-scratch
// re-derivation this same day (see [[project_diagrom_a4000t_scsi]] memory)
// and are all part of what actually got a live-device transaction working
// under Amiberry - called once per SCSI ID attempt in scanA4000TSCSI()
// below, not just once for the whole scan (same "full reset before every
// attempt" lesson A3000's WD33C93 code already applies).
// `fullSettle` gates the ~320ms post-reset wait (real driver waits 250ms
// before touching the bus after a genuine SCSI bus reset pulse) - by far the
// single most expensive part of this sequence. 2026-07-15: made conditional
// after the first working identifyA4000TSCSI() port was confirmed correct
// but noticeably slow (a typical RDB+partition walk does several of these
// resets in a row - INQUIRY, READ CAPACITY, each RDB probe block, each
// partition block). Everything ELSE in this sequence is proven necessary
// and stays unconditional; only the settle wait (needed specifically
// because SCNTL1_RST was just pulsed, not for chip-internal reconfiguration
// like CTEST0/CTEST7/SCNTL0/SBCL/DMODE) is skippable. Callers pass 0 only
// for repeated commands to a unit whose presence/timing was already proven
// by a full-settle reset moments earlier in the same identify session (see
// a4k_scsi_read_block()) - INQUIRY/READ CAPACITY and the production scanner
// always use fullSettle=1.
// `busReset` gates the SCNTL1_RST pulse itself - that bit drives the
// physical SCSI bus RST line, so EVERY attached device sees it and resets
// (aborts its current state, posts UNIT ATTENTION). Mandatory before real
// bus transactions (all SCSI callers pass 1), but the memory-to-memory DMA
// test below never touches the bus at all, and its soak/repeat mode would
// otherwise hammer every connected drive with reset pulses for hours -
// busReset=0 keeps the reset chip-internal (ISTAT ABRT + software RST +
// register reprogram) and also makes fullSettle moot (the settle exists
// only to let bus devices recover from the RST pulse that was skipped).
static int ncrChipReset(int fullSettle, int busReset)
{
    NCR_BASE[NCR_DCNTL] = NCR_DCNTL_EA | NCR_DCNTL_COM;
    if (NCR_BASE[NCR_ISTAT] == 0xFF) return 0;   // bus float — nothing there

    NCR_BASE[NCR_ISTAT] = NCR_ISTATF_ABRT;
    waitShort();
    NCR_BASE[NCR_ISTAT] = NCR_ISTATF_RST;
    NCR_BASE[NCR_ISTAT] = 0x00;
    waitShort();

    NCR_BASE[NCR_CTEST7] = NCR_BASE[NCR_CTEST7] | 0x80;   // disable burst bus mode
    NCR_BASE[NCR_CTEST0] = NCR_CTEST0_BTD | NCR_CTEST0_EAN | NCR_CTEST0_ERF;

    for (int i = 0; i < 100; i++) {
        if (!(NCR_BASE[NCR_ISTAT] & (NCR_ISTAT_SIP | NCR_ISTAT_DIP))) break;
        (void)NCR_BASE[NCR_SSTAT2];   // reading this is what actually clears pending state
        waitShort();
    }

    NCR_BASE[NCR_SCNTL0] = NCR_BASE[NCR_SCNTL0] | NCR_SCNTL0_EPG;

    if (busReset) {
        NCR_BASE[NCR_SCNTL1] = NCR_SCNTL1_RST;
        waitShort();
        NCR_BASE[NCR_SCNTL1] = 0x00;
        if (fullSettle) {
            for (int i = 0; i < 500; i++) waitShort();   // ~320ms settle - real driver waits 250ms before touching the bus
        }
    }

    NCR_BASE[NCR_SCID]   = (uint8_t)(1U << NCR_OWN_SCSI_ID);
    NCR_BASE[NCR_SCNTL1] |= NCR_SCNTL1_ESR;
    NCR_BASE[NCR_SXFER]  = NCR_SXFER_DHP;

    NCR_BASE[NCR_SBCL]  = NCR_SBCL_SSCF1 | NCR_SBCL_SSCF0;
    NCR_BASE[NCR_DMODE] = NCR_DMODE_BL1 | NCR_DMODE_BL0 | NCR_DMODE_FC2;

    // Re-assert EA as the LAST write before returning - the RST pulse above
    // clears it, and it must still be set when SELECT actually runs.
    NCR_BASE[NCR_DCNTL] = NCR_DCNTL_EA | NCR_DCNTL_COM;
    return 1;
}

// Positive 53C710 identification, safe on every big-box machine — the gate
// that decides whether $DD0040 really is an A4000T NCR chip or an
// A3000-family SDMAC answering through its incomplete address decoding
// ($DD0040 aliases $DD0000, confirmed on real hw 2026-07-05). A chipset
// (AGA/ECS) check can NOT make this call: the user's own AA3000+ (rebuilt
// A3000+ prototype) is a full A3000 — SDMAC SCSI and all — WITH real AGA,
// and there the old ISTAT!=$FF probe showed "A4000T SCSI" green and the
// scan scribbled the live SDMAC (wedged at ID 0, real hw 2026-08-07).
//
// Discriminator: SCRATCH is 4 bytes of pure r/w scratchpad on the real
// chip. Via the SDMAC alias the same addresses land on SDMAC offsets
// $34-$37 — unimplemented, no register and no access strobe there — so the
// pattern can never latch. Byte-varied patterns plus a bus-scrub read from
// a DIFFERENT chip (Ramsey version reg) between write and readback defeat
// the floating-bus echo that plagued register probing before (same
// anti-echo technique as the A3000 register test). Gayle machines must
// never reach this (unterminated range) — every caller gates on
// isGayleMachine() first.
static int ncr710Present(void)
{
    if (NCR_BASE[NCR_ISTAT] == 0xFF)   // open bus — nothing decodes here at all
        return 0;
    static const uint8_t pats[2] = { 0xA5, 0x5A };
    for (int p = 0; p < 2; p++) {
        for (int i = 0; i < 4; i++)
            NCR_BASE[NCR_SCRATCH + i] = (uint8_t)(pats[p] ^ i);
        (void)*RAMSEY_VERSION_ADDR;    // scrub the data bus so a float can't echo the pattern
        for (int i = 0; i < 4; i++)
            if (NCR_BASE[NCR_SCRATCH + i] != (uint8_t)(pats[p] ^ i))
                return 0;
    }
    return 1;
}

int detectA4000TSCSI(void)
{
    if (isGayleMachine()) return 0;   // $DD0040 is unterminated on A600/A1200 — see isGayleMachine()
    if (!ncr710Present()) return 0;   // SDMAC answering through its $DD0040 alias is NOT a 53C710 —
                                      // see ncr710Present(); ncrChipReset() on it wedges A3000 SCSI
    return ncrChipReset(1, 1);
}

// SCSI phase MCI codes — same universal 3-bit encoding used everywhere
// (mirrors the socl/sbcl MSG/C-D/I-O signal bits): 0=DataOut 1=DataIn 2=Cmd
// 3=Status 6=MsgOut 7=MsgIn. (4/5 are reserved/unused combinations.)
#define NCR_PHASE_DATA_OUT 0x0
#define NCR_PHASE_DATA_IN  0x1
#define NCR_PHASE_CMD      0x2
#define NCR_PHASE_STATUS   0x3
#define NCR_PHASE_MSG_OUT  0x6
#define NCR_PHASE_MSG_IN   0x7

// Same real-time-vs-CPU-speed lesson as A3K_WAIT_ITERS (see that comment
// above the A3000 WD33C93 code): a raw instruction-count spin runs at wildly
// different real speeds depending on host/emulation speed. waitShort() paces
// at ~640us/iteration via the video beam register. Value matches the proven
// debugcode.c POC exactly (not the old, different-architecture NCR_WAIT_ITERS).
#define NCR_SCAN_WAIT_ITERS 4000

// Ported 2026-07-15 from the proven debugcode.c POC (checkNcrFullInquiryUnit0(),
// see [[project_diagrom_a4000t_scsi]] memory for the full derivation and the
// historic first-ever successful transaction this exact shape produced).
// General-purpose holistic-transaction helper - one full SELECT+ATN ->
// JUMP IF MSG_OUT -> MOVE(IDENTIFY) -> JUMP IF CMD -> MOVE(cdb) -> JUMP IF
// DATA_IN -> MOVE(dataBuf) -> JUMP IF STATUS -> MOVE(1) -> JUMP IF MSG_IN ->
// MOVE(1) -> INT (success) SCRIPTS program per call, replacing the old
// phase-by-phase C-driven "one step, wait, read the resulting phase, decide
// next step" model (which was never actually verified working) with the
// chip's own SCRIPTS engine driving all six real phases autonomously in one
// pass - this C code only regains control once, at the very end, never
// assuming the next phase (matches Linux's 53c700.scr / NetBSD's
// siop_script.ss / the leaked AmigaOS a4091 driver). Factored out (instead
// of inlined once per caller, as the original scan-only version was) so
// scanA4000TSCSI(), a4k_scsi_read_block(), and INQUIRY/READ CAPACITY inside
// a4k_identify_unit() all share one proven implementation - mirrors the
// shape of A3000's own a3k_scsi_command(). Full chip reset before every
// call (matches the proven "reset before every attempt" pattern - see
// ncrChipReset()'s own comment); this is also the first time this chip has
// been asked to complete more than one back-to-back transaction under
// Amiberry (a4k_identify_unit() below issues several in a row), each
// isolated by its own fresh reset exactly like a3k_scsi_read_block() already
// does for A3000's WD33C93. `fullSettle` is forwarded to ncrChipReset() -
// pass 1 for INQUIRY/READ CAPACITY/the production scanner, 0 only for
// repeated commands to a unit already proven present moments earlier in the
// same identify session (see a4k_scsi_read_block()).
static int a4k_scsi_command(uint8_t unit, const uint8_t *cdb, int cdbLen,
                             uint8_t *dataBuf, int dataLen, int fullSettle)
{
    volatile struct GlobalVars *globals = a3k_globals();

    if (!ncrChipReset(fullSettle, 1)) return 0;
    ncr_irq_enable(globals);

    uint8_t identify = 0x80;   // IDENTIFY, LUN 0, no DiscPriv
    uint8_t statusByte = 0xFF, msgByte = 0xFF;

    uint32_t script[36];
    script[0]  = NCR_SCRIPT_SELECT_ATN | ((1UL << unit) << 16);
    script[1]  = (uint32_t)(uintptr_t)&script[34];   // reselect fallback
    script[2]  = NCR_SCRIPT_JUMP_IF(NCR_PHASE_MSG_OUT);
    script[3]  = (uint32_t)(uintptr_t)&script[6];
    script[4]  = NCR_SCRIPT_INT;                      // fallback: not MSG_OUT after SELECT
    script[5]  = 0;
    script[6]  = NCR_SCRIPT_MOVE(NCR_PHASE_MSG_OUT, 1);
    script[7]  = (uint32_t)(uintptr_t)&identify;
    script[8]  = NCR_SCRIPT_JUMP_IF(NCR_PHASE_CMD);
    script[9]  = (uint32_t)(uintptr_t)&script[12];
    script[10] = NCR_SCRIPT_INT;                      // fallback: not CMD after IDENTIFY
    script[11] = 0;
    script[12] = NCR_SCRIPT_MOVE(NCR_PHASE_CMD, cdbLen);
    script[13] = (uint32_t)(uintptr_t)cdb;
    script[14] = NCR_SCRIPT_JUMP_IF(NCR_PHASE_DATA_IN);
    script[15] = (uint32_t)(uintptr_t)&script[18];
    script[16] = NCR_SCRIPT_INT;                      // fallback: not DATA_IN after CDB
    script[17] = 0;
    script[18] = NCR_SCRIPT_MOVE(NCR_PHASE_DATA_IN, dataLen);
    script[19] = (uint32_t)(uintptr_t)dataBuf;
    script[20] = NCR_SCRIPT_JUMP_IF(NCR_PHASE_STATUS);
    script[21] = (uint32_t)(uintptr_t)&script[24];
    script[22] = NCR_SCRIPT_INT;                      // fallback: not STATUS after DATA_IN
    script[23] = 0;
    script[24] = NCR_SCRIPT_MOVE(NCR_PHASE_STATUS, 1);
    script[25] = (uint32_t)(uintptr_t)&statusByte;
    script[26] = NCR_SCRIPT_JUMP_IF(NCR_PHASE_MSG_IN);
    script[27] = (uint32_t)(uintptr_t)&script[30];
    script[28] = NCR_SCRIPT_INT;                      // fallback: not MSG_IN after STATUS
    script[29] = 0;
    script[30] = NCR_SCRIPT_MOVE(NCR_PHASE_MSG_IN, 1);
    script[31] = (uint32_t)(uintptr_t)&msgByte;
    script[32] = NCR_SCRIPT_INT;                      // SUCCESS checkpoint
    script[33] = 0;
    script[34] = NCR_SCRIPT_INT;                      // reselected before winning arbitration
    script[35] = 0;

    globals->ScsiIrqPending = 0;
    ncr_lwrite(NCR_DSP, (uint32_t)(uintptr_t)&script[0]);

    for (int i = 0; i < NCR_SCAN_WAIT_ITERS; i++) {
        if (globals->ScsiIrqPending) break;
        waitShort();
    }

    // Off-by-one-corrected checkpoint: s->dsp rests at &script[N+2] once
    // the INT at script[N]/script[N+1] actually fires and stops the
    // script (lsi_execute_script() advances dsp by the full 8-byte
    // instruction pair BEFORE dispatching, not after) - see
    // [[project_diagrom_a4000t_scsi]] memory for the gdb-verified proof.
    int reachedEnd = 0;
    if (globals->ScsiIrqPending) {
        globals->ScsiIrqPending = 0;
        uint32_t dsp = ncr_lread(NCR_DSP);
        reachedEnd = (dsp == (uint32_t)(uintptr_t)&script[34]);
    }
    // Caller reads globals->ScsiIrqCount immediately after this call
    // returns if it wants to accumulate an events-serviced total (see
    // scanA4000TSCSI() below) - ncr_irq_disable() doesn't touch it, only
    // the NEXT call's ncr_irq_enable() resets it.
    ncr_irq_disable();

    return reachedEnd && statusByte == 0 && msgByte == 0;
}

int scanA4000TSCSI(void)
{
    volatile struct GlobalVars *globals = a3k_globals();

    print("\nA4000T SCSI (NCR 53C710)\n", WHITE);

    if (NCR_BASE[NCR_ISTAT] == 0xFF) {
        print("  Bus float - controller not responding.\n", RED);
        return 0;
    }

    print("Scanning SCSI bus (IDs 0-6)...\n\n", WHITE);

    int found = 0;
    uint32_t totalIrqEvents = 0;
    static const uint8_t inquiryCdb[6] = { SCSI_INQUIRY, 0x00, 0x00, 0x00, SCSI_INQUIRY_LEN, 0x00 };

    for (int id = 0; id <= 6; id++) {
        print("ID ", CYAN);
        char idch[2] = { (char)('0' + id), 0 };
        print(idch, CYAN);
        print(": ", WHITE);

        uint8_t buf[SCSI_INQUIRY_LEN];
        int ok = a4k_scsi_command((uint8_t)id, inquiryCdb, 6, buf, SCSI_INQUIRY_LEN, 1);
        totalIrqEvents += globals->ScsiIrqCount;

        if (!ok) {
            print("NOT FOUND\n", RED);
            waitShort();   // let the host redraw before moving to the next ID
            continue;
        }

        uint8_t devtype = buf[0] & 0x1F;
        char vendor[9], product[17], revision[5];
        memcpy(vendor,   buf + 8,  8);  a3k_scsi_strip(vendor,   8);
        memcpy(product,  buf + 16, 16); a3k_scsi_strip(product,  16);
        memcpy(revision, buf + 32, 4);  a3k_scsi_strip(revision,  4);

        print("FOUND  ", GREEN);
        char *dtype = (devtype < 16 && scsiDevTypes[devtype]) ?
                          (char *)scsiDevTypes[devtype] : "???";
        print(dtype, YELLOW);
        print("  \"", WHITE);
        print(vendor,   GREEN);
        print(" ",      WHITE);
        print(product,  GREEN);
        if (revision[0]) { print(" ", WHITE); print(revision, CYAN); }
        print("\"\n", WHITE);
        found++;
        waitShort();   // let the host redraw before moving to the next ID
    }

    print("\nIRQ2 (PORTS) events serviced by our handler: ", WHITE);
    print(binDec(totalIrqEvents), totalIrqEvents ? GREEN : RED);
    print("\n", WHITE);

    return found;
}

#define A4K_SCSI_READ10 0x28

// Single-block READ(10) via a4k_scsi_command() - same shared holistic
// transaction machinery INQUIRY/READ CAPACITY use, just a different CDB.
// fullSettle=0: an RDB/partition walk can issue a dozen-plus of these in a
// row to a unit whose presence was already proven by a full-settle INQUIRY
// moments earlier in the same a4k_identify_unit() call - skipping the
// ~320ms post-reset settle here is what actually made that walk fast
// enough to be usable interactively (2026-07-15, see
// [[project_diagrom_a4000t_scsi]] memory for the before/after timing and
// the reasoning for why this specific wait is the skippable one).
static int a4k_scsi_read_block(uint8_t unit, uint32_t lba, uint8_t *buf)
{
    uint8_t cdb[10] = {
        A4K_SCSI_READ10, 0,
        (uint8_t)(lba >> 24), (uint8_t)(lba >> 16), (uint8_t)(lba >> 8), (uint8_t)lba,
        0, 0, 1, 0   // transfer length = 1 block
    };
    return a4k_scsi_command(unit, cdb, 10, buf, 512, 0);
}

// Read and print everything available about one SCSI ID: INQUIRY, READ
// CAPACITY, and - for direct-access disks - RDB geometry + full partition
// list. Mirrors a3k_identify_unit() exactly - the RDB/partition walk
// (RDB_MAGIC/PART_MAGIC/RD32/printDosType/mul32/a3k_print_blocks_mb) is the
// same shared, file-scope logic; only the command primitive differs (this
// chip's holistic SCRIPTS transaction vs the WD33C93's manual phase pump).
static void a4k_identify_unit(uint8_t unit)
{
    uint8_t inqBuf[SCSI_INQUIRY_LEN];
    uint8_t capBuf[SCSI_READ_CAPACITY_LEN];
    uint8_t blk[512];
    static const uint8_t inquiryCdb[6]  = { SCSI_INQUIRY, 0, 0, 0, SCSI_INQUIRY_LEN, 0 };
    static const uint8_t readCapCdb[10] = { SCSI_READ_CAPACITY, 0,0,0,0,0,0,0,0, 0 };

    if (!a4k_scsi_command(unit, inquiryCdb, 6, inqBuf, SCSI_INQUIRY_LEN, 1)) {
        print("  No device at this ID.\n", RED);
        return;
    }

    uint8_t devtype = inqBuf[0] & 0x1F;
    char vendor[9], product[17], revision[5];
    memcpy(vendor,   inqBuf + 8,  8);  a3k_scsi_strip(vendor,   8);
    memcpy(product,  inqBuf + 16, 16); a3k_scsi_strip(product,  16);
    memcpy(revision, inqBuf + 32, 4);  a3k_scsi_strip(revision,  4);
    char *dtype = (devtype < 16 && scsiDevTypes[devtype]) ?
                      (char *)scsiDevTypes[devtype] : "???";

    print("  Type:     ", WHITE); print(dtype, YELLOW); print("\n", WHITE);
    print("  Name:     ", WHITE);
    print(vendor, GREEN); print(" ", WHITE); print(product, GREEN);
    if (revision[0]) { print(" ", WHITE); print(revision, CYAN); }
    print("\n", WHITE);

    if (a4k_scsi_command(unit, readCapCdb, 10, capBuf, SCSI_READ_CAPACITY_LEN, 1)) {
        print("  Capacity: ", WHITE);
        a3k_print_capacity(capBuf);
        print("\n", WHITE);
    }

    if (devtype != 0) {   // RDB/partitions only meaningful for direct-access disks
        print("\n", WHITE);
        return;
    }

    int rdb_found = 0;
    uint32_t rdb_heads = 0, rdb_spt = 0, partblock = NO_LIST;
    for (uint32_t b = 0; b < 16 && !rdb_found; b++) {
        if (!a4k_scsi_read_block(unit, b, blk)) continue;
        if (RD32(blk, 0) != RDB_MAGIC) continue;
        rdb_found = 1;

        uint32_t rdb_cyls = RD32(blk, 0x40);
        rdb_spt   = RD32(blk, 0x44);
        rdb_heads = RD32(blk, 0x48);
        partblock = RD32(blk, 0x1C);

        print("  Geometry: ", WHITE);
        print(binDec(rdb_cyls),  WHITE); print("c / ", WHITE);
        print(binDec(rdb_heads), WHITE); print("h / ", WHITE);
        print(binDec(rdb_spt),   WHITE); print("s\n", WHITE);
    }

    if (!rdb_found) {
        print("  (no RDB found)\n\n", YELLOW);
        return;
    }

    print("  Partitions:\n", WHITE);
    char name[32];
    int nparts = 0;
    while (partblock != NO_LIST && nparts < 32) {
        if (!a4k_scsi_read_block(unit, partblock, blk)) break;
        if (RD32(blk, 0) != PART_MAGIC) break;

        uint32_t next    = RD32(blk, 0x10);
        uint32_t lowcyl  = RD32(blk, 0xA4);
        uint32_t highcyl = RD32(blk, 0xA8);
        uint32_t dostype = RD32(blk, 0xC0);

        uint8_t namelen = blk[0x24];
        if (namelen > 30) namelen = 30;
        for (int i = 0; i < namelen; i++) name[i] = (char)blk[0x25 + i];
        name[namelen]   = ':';
        name[namelen+1] = '\0';

        uint32_t cylSpan    = highcyl - lowcyl + 1;
        uint32_t partBlocks = mul32(mul32(cylSpan, rdb_heads), rdb_spt);

        print("    ", WHITE); print(name, GREEN); print("  ", WHITE);
        a3k_print_blocks_mb(partBlocks);
        print("  [", WHITE); print(binDec(lowcyl), WHITE); print("-", WHITE);
        print(binDec(highcyl), WHITE); print("]  ", WHITE);
        printDosType(dostype);
        print("\n", WHITE);

        nparts++;
        partblock = next;
    }
    if (nparts == 0) print("    (none)\n", YELLOW);
    print("\n", WHITE);
}

// Real SCSI SMART via LOG SENSE - same protocol/decode as a3k_smart_unit(),
// just issued over a4k_scsi_command()'s NCR SCRIPTS transport instead of
// the WD33C93 phase-by-phase one. fullSettle=1: a fresh standalone
// user-triggered action (browseUnits()'s 'S' key), not chained off a
// just-completed presence check in the same call, so this matches
// a4k_identify_unit()'s own INQUIRY/READ CAPACITY calls.
static int a4k_smart_unit(uint8_t unit)
{
    uint8_t buf[SCSI_LOGSENSE_LEN];
    static const uint8_t ieCdb[10]   = { SCSI_LOG_SENSE, 0, (1<<6)|SCSI_LOGPAGE_IE,   0,0,0,0,0, 0, SCSI_LOGSENSE_LEN };
    static const uint8_t tempCdb[10] = { SCSI_LOG_SENSE, 0, (1<<6)|SCSI_LOGPAGE_TEMP, 0,0,0,0,0, 0, SCSI_LOGSENSE_LEN };

    if (!a4k_scsi_command(unit, ieCdb, 10, buf, SCSI_LOGSENSE_LEN, 1)) {
        print("  SMART/Log Sense not supported by this device.\n", YELLOW);
        return 0;
    }
    a3k_print_smart_ie(buf, SCSI_LOGSENSE_LEN);

    if (a4k_scsi_command(unit, tempCdb, 10, buf, SCSI_LOGSENSE_LEN, 0))
        a3k_print_smart_temp(buf, SCSI_LOGSENSE_LEN);

    return 1;
}

// Adapters for browseUnits(): no ctx needed, everything's reached via
// global-scope hardware access. skipUnit excludes the host adapter's own
// ID (NCR_OWN_SCSI_ID) - a SELECT to yourself has nothing to respond.
static void a4kBrowseIdentify(void *ctx, int unit) { (void)ctx; a4k_identify_unit((uint8_t)unit); }
static void a4kBrowseSmart(void *ctx, int unit)     { (void)ctx; a4k_smart_unit((uint8_t)unit); }
static int a4kBrowseSkip(void *ctx, int unit) { (void)ctx; return unit == NCR_OWN_SCSI_ID; }

static int identifyA4000TSCSI(void)
{
    if (!ncrChipReset(1, 1)) {
        print("\nA4000T SCSI: not responding (bus float)\n", RED);
        return 1;
    }
    return browseUnits("A4000T SCSI - Identify Devices", 8, 0, NULL, NULL,
                        a4kBrowseIdentify, a4kBrowseSmart, a4kBrowseSkip);
}

// Whole-bus wrapper (matches doSmartIDE()'s shape) - currently unreachable
// via any menu (see smartA3000SCSI()'s comment above, same situation),
// kept working for HddController's own "smart" field.
static int smartA4000TSCSI(void)
{
    print("\nSMART Data\n\n", WHITE);
    int found = 0;
    for (uint8_t id = 0; id < 8; id++) {
        if (id == NCR_OWN_SCSI_ID) continue;
        print("ID ", CYAN); print(binDec(id), CYAN); print(":\n", WHITE);
        found += a4k_smart_unit(id);
        print("\n", WHITE);
    }
    return found;
}

// ---------------------------------------------------------------------------
// A4000T SCSI DMA Test — memory-to-memory only, no SCSI bus activity at all.
// ---------------------------------------------------------------------------
//
// Ported 2026-07-15 from terriblefire/ncrtest (ncr_dmatest.c BuildDMAScript():
// a 2-instruction SCRIPTS program — a Memory Move followed by an INT — used
// to validate the 53C710's own DMA engine independent of any SCSI device).
// That project runs as a normal exec.library CLI executable (AllocMem/
// AllocAbs/AddIntServer/Wait/Signal); this ROM has none of that, so the move
// is driven instead by the same proven ncrChipReset()/ncr_irq_enable()/
// Ncr710PortsIRQ()/globals->ScsiIrqPending polling machinery
// a4k_scsi_command() already uses above for real SCSI transactions, and
// buffers come from this ROM's own getMemory() bump allocator instead of
// AllocMem.
//
// "No SCSI bus activity" is literal: the chip is initialized ONCE per test
// entry with ncrChipReset(0, 0) — busReset=0, so SCNTL1_RST is never pulsed
// and attached devices never see anything — and each move just writes DSP on
// the already-initialized engine. Only a FAILED move re-runs that chip-only
// init to recover the engine. That's what makes the repeat mode safe to
// soak-run for hours with real disks on the bus.
//
// This is a stress test for TWO suspects at once, and the direction order is
// the diagnosis: Local (Chip) -> Local (Chip) runs first as the
// motherboard-only baseline — the 53C710, its DMA engine and Chip RAM all
// live on the A4000T board, no CPU card involvement — so if THAT fails, the
// motherboard/SCSI side is bad no matter what card is fitted. The CPU board
// directions then add the accelerator's bus interface: baseline passing
// while only CPU-board directions fail points squarely at the card's
// DMA/bus-arbitration path.
//
// Memory Move is a genuinely different SCRIPTS instruction class from the
// Block Move used everywhere else in this file (NCR_SCRIPT_MOVE): Block Move
// carries a SCSI phase in bits26-24 and only moves between the chip and the
// SCSI bus; Memory Move (class=11, opcode byte 0xC0 per the NCR 53C710
// Programmer's Guide and ncrtest's memmove_inst) moves directly between two
// arbitrary memory addresses with no SCSI phase involved at all - exactly
// what's needed to test the DMA engine/bus wiring in isolation.
#define NCR_DMA_MOVE_OPCODE 0xC0000000UL   // class=11 (Memory Move), bits23-0=length in bytes

#define NCR_DMATEST_BUF_SIZE 4096UL
#define NCR_DMATEST_PATTERN_ZEROS       0
#define NCR_DMATEST_PATTERN_ONES        1
#define NCR_DMATEST_PATTERN_WALKING     2
#define NCR_DMATEST_PATTERN_ALTERNATING 3
#define NCR_DMATEST_PATTERN_RANDOM      4
#define NCR_DMATEST_NUM_PATTERNS        5

// Which stage of a pattern run died — the distinction matters on real
// hardware: TIMEOUT = engine never interrupted at all (hung/bus-faulted;
// with DIEN only enabling SIR, a genuine bus fault raises no interrupt and
// lands here), BADSTOP = interrupt fired but the script halted somewhere
// other than the success checkpoint, MISMATCH = the move "succeeded" but the
// data compared bad (the interesting one for marginal RAM/bus wiring).
#define NCR_DMAFAIL_TIMEOUT  1
#define NCR_DMAFAIL_BADSTOP  2
#define NCR_DMAFAIL_MISMATCH 3

typedef struct {
    int      reason;     // NCR_DMAFAIL_*
    uint32_t dsp;        // where the script actually was (TIMEOUT/BADSTOP)
    uint32_t mismatch;   // first differing byte offset (MISMATCH)
    uint8_t  istat;      // chip status captured at failure (TIMEOUT/BADSTOP)
    uint8_t  dstat;
} DmaFailInfo;

// One src->dst pairing over the RAM regions this system actually has; the
// per-system matrix is built by a4kDmaBuildDirections() below.
typedef struct {
    const char *label;
    uint8_t    *src;
    uint8_t    *dst;
} DmaDirection;
#define NCR_DMATEST_MAX_DIRS 4

// const-of-const so the whole array lands in .rodata — a merely
// pointer-mutable static would go to .data, which this linker script maps
// into ROM (see the A4000 IDE "static scratch buffer" gotcha).
static const char *const a4kDmaPatternNames[NCR_DMATEST_NUM_PATTERNS] = {
    "ZEROS", "ONES", "WALKING", "ALTERNATING", "RANDOM"
};

static void a4kDmaFillPattern(uint8_t *buf, uint32_t size, int pattern, uint32_t *seed)
{
    for (uint32_t i = 0; i < size; i++) {
        switch (pattern) {
            case NCR_DMATEST_PATTERN_ZEROS:       buf[i] = 0x00; break;
            case NCR_DMATEST_PATTERN_ONES:        buf[i] = 0xFF; break;
            case NCR_DMATEST_PATTERN_WALKING:     buf[i] = (uint8_t)(1U << (i & 7)); break;
            case NCR_DMATEST_PATTERN_ALTERNATING: buf[i] = (i & 1) ? 0xAA : 0x55; break;
            default:   // PATTERN_RANDOM — same LCG constants as ncrtest's GetRandom(),
                       // via mul32() since this freestanding build has no __mulsi3
                       // (see mul32()'s own comment above, same gap hit before)
                *seed = mul32(*seed, 1103515245UL) + 12345UL;
                buf[i] = (uint8_t)(*seed >> 16);
                break;
        }
    }
}

// Chip-only engine bring-up: ncrChipReset() with busReset=0 never touches
// the physical SCSI bus (see its comment), and fullSettle is moot without a
// bus pulse. Called once per test entry — NOT per move like the SCSI-command
// paths, which need a bus-visible reset before every real transaction; a
// halted-at-INT engine restarts cleanly by just writing DSP again — and
// again as recovery whenever a move fails and leaves the engine in an
// unknown state.
static int a4kDmaEngineInit(void)
{
    if (!ncrChipReset(0, 0)) return 0;
    ncr_irq_enable(a3k_globals());
    return 1;
}

// Run one Memory Move SCRIPTS program (src -> dst, size bytes) and confirm
// the chip actually reached the trailing INT. Mirrors a4k_scsi_command()'s
// own off-by-one-corrected checkpoint (see its comment above): dsp rests at
// &script[N+2] once the INT at script[N]/script[N+1] fires and halts the
// script, since lsi_execute_script() advances dsp by the full instruction
// pair before dispatching it (Memory Move itself is 3 words: opcode+length,
// src, dst — 12 bytes — then the 8-byte INT lands dsp on &script[5]).
// On failure fills *fail with reason + chip state for the caller to report,
// then re-runs the chip-only init so the NEXT pattern starts from a known
// engine instead of inheriting this failure.
static int a4k_dma_move(uint8_t *src, uint8_t *dst, uint32_t size, DmaFailInfo *fail)
{
    volatile struct GlobalVars *globals = a3k_globals();
    uint32_t script[6];

    script[0] = NCR_DMA_MOVE_OPCODE | (size & 0xFFFFFFUL);
    script[1] = (uint32_t)(uintptr_t)src;
    script[2] = (uint32_t)(uintptr_t)dst;
    script[3] = NCR_SCRIPT_INT;
    script[4] = 0;

    globals->ScsiIrqPending = 0;
    ncr_lwrite(NCR_DSP, (uint32_t)(uintptr_t)&script[0]);

    int fired = 0;
    for (int i = 0; i < NCR_SCAN_WAIT_ITERS; i++) {
        if (globals->ScsiIrqPending) { fired = 1; break; }
        waitShort();
    }

    if (fired) {
        globals->ScsiIrqPending = 0;
        uint32_t dsp = ncr_lread(NCR_DSP);
        if (dsp == (uint32_t)(uintptr_t)&script[5])
            return 1;
        fail->reason = NCR_DMAFAIL_BADSTOP;
        fail->dsp    = dsp;
    } else {
        fail->reason = NCR_DMAFAIL_TIMEOUT;
        fail->dsp    = ncr_lread(NCR_DSP);   // where it hung — diagnostic gold
    }
    fail->istat = NCR_BASE[NCR_ISTAT];
    fail->dstat = NCR_BASE[NCR_DSTAT];   // reading clears DIP; engine is re-init'd below anyway

    a4kDmaEngineInit();
    return 0;
}

// Fill src with the pattern and dst with its byte-wise COMPLEMENT — never
// the same value, so an engine that moves nothing at all can't vacuously
// "pass" any pattern (with a plain zero prefill the ZEROS pattern proved
// nothing beyond the INT firing) — then run one Memory Move and verify
// every byte. Region-agnostic: src/dst can be any RAM this ROM can address.
static int a4kDmaRunPattern(uint8_t *src, uint8_t *dst, int pattern,
                            uint32_t *seed, DmaFailInfo *fail)
{
    a4kDmaFillPattern(src, NCR_DMATEST_BUF_SIZE, pattern, seed);
    for (uint32_t i = 0; i < NCR_DMATEST_BUF_SIZE; i++)
        dst[i] = (uint8_t)~src[i];

    if (!a4k_dma_move(src, dst, NCR_DMATEST_BUF_SIZE, fail))
        return 0;

    for (uint32_t i = 0; i < NCR_DMATEST_BUF_SIZE; i++) {
        if (src[i] != dst[i]) {
            fail->reason   = NCR_DMAFAIL_MISMATCH;
            fail->mismatch = i;
            return 0;
        }
    }
    return 1;
}

// One-line failure detail; the caller prints any "PATTERN: " prefix first.
// Raw dsp/ISTAT/DSTAT go on screen deliberately — "treat the hardware as
// broken and show what it actually did" is this ROM's whole job, and a
// generic "did not complete" can't distinguish a hung engine from a bus
// fault from a wrong-checkpoint halt.
static void a4kDmaPrintFail(const DmaFailInfo *fail)
{
    if (fail->reason == NCR_DMAFAIL_MISMATCH) {
        print("FAILED (mismatch at offset ", RED);
        print(binHex(fail->mismatch), RED);
        print(")\n", RED);
        return;
    }
    if (fail->reason == NCR_DMAFAIL_TIMEOUT)
        print("FAILED (no DMA IRQ, dsp=", RED);
    else
        print("FAILED (bad stop, dsp=", RED);
    print(binHex(fail->dsp), RED);
    print(" ISTAT=", RED); print(binHexByte(fail->istat), RED);
    print(" DSTAT=", RED); print(binHexByte(fail->dstat), RED);
    print(")\n", RED);
}

// Blank one display row. The repeat/soak tests draw their frame once and
// overwrite numbers in place; only the variable-width "Last fail" lines can
// shrink between redraws, so only those need explicit erasing. Must cover
// the FULL 80 columns — wrapped fail lines reach the last one (a 78-space
// blank left a 2-char "xp" remnant on real hw). The last column is done
// with putChar() (no cursor advance): print()ing col 79 would auto-wrap,
// and on the bottom screen row that wrap would scroll the whole frame.
static void soakBlankRow(uint32_t row)
{
    setPos(0, row);
    print("                                                                               ", WHITE);
    putChar(' ', WHITE, 79, (uint8_t)row);
}

// Same ESC-or-both-buttons abort convention as browseUnits() above. Polled
// between EVERY pattern, not once per pass: getInput()'s clearInput() means
// nothing latches between polls, so a once-per-pass poll silently drops any
// press that didn't span the exact poll instant — and on failing hardware a
// pass can spend many seconds in timeout waits.
static int a4kDmaAbortRequested(void)
{
    // Drain the whole keyboard queue first (multiple press+release pairs
    // pile up between polls once reps get slow) and pick up anything
    // hddInputService() latched from inside a wait loop — getInput()'s
    // clearInput() would wipe a key consumed there before we saw it.
    hddInputService();
    getInput();
    if ((globals->LMB && globals->RMB) || globals->GetCharData == 0x1b ||
        globals->HddEscLatch) {
        globals->HddEscLatch = 0;
        waitReleased();
        return 1;
    }
    return 0;
}

// Build the per-system direction matrix into dirs[NCR_DMATEST_MAX_DIRS].
// On a TF-accelerated A4000T the Fast RAM this ROM's own memory test found
// is the accelerator ("CPU board") RAM, while Chip RAM is always physically
// on the A4000T motherboard ("local") — so the four directions split the
// two suspects: Local->Local exercises the motherboard alone (baseline,
// listed first), the other three add the CPU board's bus interface.
//
// Chip pair comes from ONE getChip(2*size) call split in half — getChip()
// never bumps its reservation pointer, so repeat calls return overlapping
// blocks (see its comment in genericc.c); one call per arena, made here
// before any loop, same rule the floppy code follows. Fast pair comes from
// getMemory(), which prefers Fast RAM but silently falls back to Chip when
// Fast is absent/full — the range check below catches that fallback so a
// Chip buffer is never mislabeled "CPU board" (getMemory() alone could also
// never produce a genuine cross-region pair, which is why getChip() is used
// for the Chip side at all).
static int a4kDmaBuildDirections(DmaDirection *dirs, int *haveChip, int *haveFast)
{
    uint8_t *chipA = NULL, *chipB = NULL, *fastA = NULL, *fastB = NULL;
    int n = 0;

    uint32_t caddr = getChip(2 * NCR_DMATEST_BUF_SIZE);
    if (caddr != 0 && caddr != 1) {   // 0=no chip RAM, 1=not enough room
        chipA = (uint8_t *)(uintptr_t)caddr;
        chipB = chipA + NCR_DMATEST_BUF_SIZE;
    }

    if (globals->FastStart != 0) {
        uint8_t *fa = (uint8_t *)getMemory(NCR_DMATEST_BUF_SIZE);
        uint8_t *fb = (uint8_t *)getMemory(NCR_DMATEST_BUF_SIZE);
        uint32_t fs = (uint32_t)(uintptr_t)globals->FastStart;
        uint32_t fe = (uint32_t)(uintptr_t)globals->FastEnd;
        if (fa && fb &&
            (uint32_t)(uintptr_t)fa >= fs && (uint32_t)(uintptr_t)fa < fe &&
            (uint32_t)(uintptr_t)fb >= fs && (uint32_t)(uintptr_t)fb < fe) {
            fastA = fa;
            fastB = fb;
        }
    }

    if (chipA) {
        dirs[n].label = "Local (Chip) -> Local (Chip)";
        dirs[n].src = chipA; dirs[n].dst = chipB; n++;
    }
    if (fastA) {
        dirs[n].label = "CPU board    -> CPU board   ";
        dirs[n].src = fastA; dirs[n].dst = fastB; n++;
    }
    if (chipA && fastA) {
        dirs[n].label = "CPU board    -> Local (Chip)";
        dirs[n].src = fastA; dirs[n].dst = chipA; n++;
        dirs[n].label = "Local (Chip) -> CPU board   ";
        dirs[n].src = chipA; dirs[n].dst = fastA; n++;
    }

    *haveChip = (chipA != NULL);
    *haveFast = (fastA != NULL);
    return n;
}

static int a4kDmaTest(void)
{
    uint32_t seed = 0x12345678UL;
    DmaDirection dirs[NCR_DMATEST_MAX_DIRS];
    int haveChip, haveFast;

    print("\nA4000T SCSI - DMA Test (memory-to-memory, no SCSI bus activity)\n\n", WHITE);

    if (NCR_BASE[NCR_ISTAT] == 0xFF) {
        print("  Bus float - controller not responding.\n", RED);
        return 0;
    }

    int nDirs = a4kDmaBuildDirections(dirs, &haveChip, &haveFast);
    if (nDirs == 0) {
        print("  Could not allocate any test buffers.\n", RED);
        return 0;
    }
    if (!haveChip)
        print("  Local (Chip) directions skipped - no Chip RAM buffer available.\n", YELLOW);
    if (!haveFast)
        print("  CPU board directions skipped - no Fast RAM detected/free.\n", YELLOW);

    if (!a4kDmaEngineInit()) {
        print("  Controller reset failed.\n", RED);
        return 0;
    }

    uint32_t passed = 0, total = 0;
    int aborted = 0;

    for (int d = 0; d < nDirs && !aborted; d++) {
        print("  ", WHITE); print((char *)dirs[d].label, CYAN);
        print("  (", WHITE);
        print(binHex((uint32_t)(uintptr_t)dirs[d].src), CYAN);
        print(" -> ", WHITE);
        print(binHex((uint32_t)(uintptr_t)dirs[d].dst), CYAN);
        print("):\n", WHITE);

        for (int p = 0; p < NCR_DMATEST_NUM_PATTERNS; p++) {
            DmaFailInfo fail;
            print("    ", WHITE); print((char *)a4kDmaPatternNames[p], YELLOW); print(": ", WHITE);
            total++;
            if (a4kDmaRunPattern(dirs[d].src, dirs[d].dst, p, &seed, &fail)) {
                print("PASSED\n", GREEN);
                passed++;
            } else {
                a4kDmaPrintFail(&fail);
            }
            if (a4kDmaAbortRequested()) { aborted = 1; break; }
        }
    }

    ncr_irq_disable();

    print("\nResult: ", WHITE);
    print(binDec((int32_t)passed), passed == total ? GREEN : RED);
    print(" / ", WHITE);
    print(binDec((int32_t)total), WHITE);
    print(" checks passed", WHITE);
    if (aborted) print("  (stopped early)", YELLOW);
    print("\n", WHITE);

    return (int)passed;
}

// Soak version of a4kDmaTest() — runs the same direction matrix over and
// over until ESC or both mouse buttons together (polled between every
// pattern via a4kDmaAbortRequested(), see its comment). Output is compact —
// one line per direction per pass instead of one per pattern — so a whole
// pass fits the screen without scrolling — the frame is drawn ONCE and each
// pass overwrites only the numbers in place (no blink), and what matters
// for an overnight soak stays visible: a cumulative fail
// counter per direction (an intermittent fault at 3am must still be visible
// at 8am) plus a "last fail" line preserving the full diagnostic detail
// (pass number, direction, pattern, reason/dsp/ISTAT/DSTAT or mismatch
// offset) of the most recent failure.
// Buffers are allocated once, before the loop (not per-iteration):
// getMemory()'s bump arena is never reset inside the loop (clearScreen()
// only wipes the display), so per-iteration allocation would run it out
// after a handful of passes. Reusing the same pointers across iterations
// is safe as long as nothing calls getMemory()/getChip() again inside the
// loop — which this doesn't.
static int a4kDmaTestRepeat(void)
{
    uint32_t seed = 0x87654321UL;
    DmaDirection dirs[NCR_DMATEST_MAX_DIRS];
    uint32_t dirFails[NCR_DMATEST_MAX_DIRS] = { 0, 0, 0, 0 };
    int haveChip, haveFast;

    if (NCR_BASE[NCR_ISTAT] == 0xFF) {
        print("\nA4000T SCSI - DMA Test: Bus float - controller not responding.\n", RED);
        return 0;
    }

    int nDirs = a4kDmaBuildDirections(dirs, &haveChip, &haveFast);
    if (nDirs == 0) {
        print("\nA4000T SCSI - DMA Test: Could not allocate any test buffers.\n", RED);
        return 0;
    }

    if (!a4kDmaEngineInit()) {
        print("\nA4000T SCSI - DMA Test: Controller reset failed.\n", RED);
        return 0;
    }

    globals->HddEscLatch = 0;   // stale-latch guard, same as the A3000 repeat modes

    uint32_t iterations = 0, totalPassed = 0, totalChecks = 0;
    DmaFailInfo lastFail;
    uint32_t lastFailPass = 0;
    int lastFailDir = 0, lastFailPattern = 0, haveFail = 0;
    int aborted = 0;

    // The frame is drawn ONCE; each pass only overwrites the numbers in
    // place with setPos() — a per-pass clearScreen() made the whole display
    // blink. Every counter is monotonic so lines never get shorter; the one
    // exception is the "Last fail" line (its detail text varies), which is
    // blanked before each redraw and only redrawn when a NEW fail arrives.
    clearScreen();
    print("\002A4000T SCSI - DMA Test (Repeat)\n\n", WHITE);
    print("ESC or both buttons: stop\n\n", WHITE);
    if (!haveChip)
        print("(Local (Chip) directions skipped - no Chip RAM buffer available)\n\n", YELLOW);
    if (!haveFast)
        print("(CPU board directions skipped - no Fast RAM detected/free)\n\n", YELLOW);
    uint32_t passRow   = 4 + (haveChip ? 0 : 2) + (haveFast ? 0 : 2);
    uint32_t totalsRow = passRow + (uint32_t)nDirs + 2;
    uint32_t failRow   = totalsRow + 1;

    while (!aborted) {
        setPos(0, passRow);
        print("Pass ", WHITE); print(binDec((int32_t)(iterations + 1)), CYAN); print(":", WHITE);

        int newFail = 0;
        for (int d = 0; d < nDirs && !aborted; d++) {
            uint32_t passedDir = 0;

            for (int p = 0; p < NCR_DMATEST_NUM_PATTERNS; p++) {
                DmaFailInfo fail;
                if (a4kDmaRunPattern(dirs[d].src, dirs[d].dst, p, &seed, &fail)) {
                    passedDir++;
                } else {
                    dirFails[d]++;
                    lastFail        = fail;
                    lastFailPass    = iterations + 1;
                    lastFailDir     = d;
                    lastFailPattern = p;
                    haveFail        = 1;
                    newFail         = 1;
                }
                totalChecks++;
                if (a4kDmaAbortRequested()) { aborted = 1; break; }
            }
            totalPassed += passedDir;

            setPos(0, passRow + 1 + (uint32_t)d);
            print("  ", WHITE); print((char *)dirs[d].label, CYAN); print(": ", WHITE);
            print(binDec((int32_t)passedDir),
                  passedDir == NCR_DMATEST_NUM_PATTERNS ? GREEN : RED);
            print("/", WHITE); print(binDec(NCR_DMATEST_NUM_PATTERNS), WHITE);
            print("   fails so far: ", WHITE);
            print(binDec((int32_t)dirFails[d]), dirFails[d] ? RED : GREEN);
            print("  ", WHITE);   // pad: passedDir can lose a digit vs last pass
        }
        iterations++;

        setPos(0, totalsRow);
        print("Running totals over ", WHITE); print(binDec((int32_t)iterations), CYAN);
        print(" pass(es): ", WHITE);
        print(binDec((int32_t)totalPassed), totalPassed == totalChecks ? GREEN : YELLOW);
        print(" / ", WHITE);
        print(binDec((int32_t)totalChecks), WHITE);
        print(" checks passed", WHITE);

        if (haveFail && newFail) {
            soakBlankRow(failRow);
            soakBlankRow(failRow + 1);   // fail line can wrap onto a second row
            setPos(0, failRow);
            print("Last fail: pass ", WHITE);
            print(binDec((int32_t)lastFailPass), CYAN);
            print("  ", WHITE); print((char *)dirs[lastFailDir].label, CYAN);
            print("  ", WHITE); print((char *)a4kDmaPatternNames[lastFailPattern], YELLOW);
            print(": ", WHITE);
            a4kDmaPrintFail(&lastFail);
        }
    }

    ncr_irq_disable();
    return (int)totalPassed;
}

// ---------------------------------------------------------------------------
// A3000 SDMAC/Ramsey DMA register test — the A3000 slot for the same "4 -
// DMA Test" menu entry the A4000T fills with its memory-to-memory test.
// ---------------------------------------------------------------------------
//
// A memory-to-memory port of the A4000T test is IMPOSSIBLE here, not just
// unimplemented: the 53C710 has a SCRIPTS Memory Move instruction that copies
// RAM->RAM with no bus phase, while the SDMAC is a single-purpose pipe
// between memory and the WD33C93's SCSI port — there is no chip mode in
// which its DMA engine touches two memory buffers. So Tier 1 (this) pattern-
// tests every software-reachable register of the DMA path instead — still
// with ZERO SCSI bus activity (ST_DMA is never strobed, no WD command is
// issued): ACR (the Ramsey half: DMA address generation), WTC or SSPBDAT
// (the SDMAC half, revision-dependent — see the register map comment above),
// and a WD33C93 scratch register through the PORT0 pass-through (the path
// DMA data itself flows through). Register semantics re-derived from Chris
// Hooper's SDMAC utility (see that same comment). Tier 2 — a real-transfer
// DMA-vs-PIO read compare, which DOES need a live target on the bus — is
// deliberately not here; see the project memory.
//
// Deviation from Hooper's technique: his tool writes each register via its
// +$100 decode mirror and reads back via the primary address, proving the
// value latched in a real register rather than echoing off a floating bus.
// Amiberry precedent says mirrors are risky there (it decodes only primary
// WD33C93 addresses — see WD_ADDR_REG's comment), so this writes the primary
// address and scrubs the bus with an unrelated read (RAMSEY_VERSION_ADDR, a
// different chip) between write and readback instead — same anti-echo
// property, no reliance on mirror decode.

static const uint32_t a3kRegTestValues[] = {
    0x00000000, 0xffffffff, 0xa5a5a5a5, 0x5a5a5a5a, 0xc3c3c3c3, 0x3c3c3c3c,
    0xd2d2d2d2, 0x2d2d2d2d, 0x4b4b4b4b, 0xb4b4b4b4, 0xe1e1e1e1, 0x1e1e1e1e,
    0x87878787, 0x78787878, 0xffff0000, 0x0000ffff, 0xff00ff00, 0x00ff00ff,
    0xf0f0f0f0, 0x0f0f0f0f,
};
#define A3K_NUM_REG_VALUES (int)(sizeof(a3kRegTestValues)/sizeof(a3kRegTestValues[0]))

typedef struct {
    uint32_t    wrote;   // first failing pattern, as written (post-mask)
    uint32_t    read;    // what came back
    const char *note;    // NULL, or extra context ("WD CONTROL changed")
} A3kRegFail;

// SDMAC revision probe, ported from Hooper's get_sdmac_version(): the ONLY
// known way to tell revisions apart in software is WTC readback behavior —
// SDMAC-02 reads back all 24 writable bits, SDMAC-04 always returns bit 2 as
// 0. Any OTHER readback shape is a genuine fault, which doubles this probe
// as the first test: full 32-bit follow of a non-trivial pattern means the
// supposedly read-only high byte followed the write (open bus / no decode),
// and bit 2 reading 1 on a low-24 mismatch fits neither revision.
// Returns 2 or 4, or 0 with *why pointing at a printable reason.
static int a3kSdmacVersion(const char **why)
{
    static const uint32_t probe[6] = {
        0x00000000, 0xffffffff, 0xa5a5a5a5, 0x5a5a5a5a, 0xc2c2c3c3, 0x3c3c3c3c
    };
    int version = 2;

    uint8_t istr = *SDMAC_ISTR;
    if ((istr & SDMAC_ISTR_FIFOE) && (istr & SDMAC_ISTR_FIFOF)) {
        *why = "ISTR claims FIFO both empty AND full";
        return 0;
    }

    for (int pass = 0; pass < 6; pass++) {
        uint32_t wvalue = probe[pass];
        uint32_t ovalue = *SDMAC_WTC;
        *SDMAC_WTC = wvalue;
        (void)*RAMSEY_VERSION_ADDR;          // scrub the bus (see header comment)
        uint32_t rvalue = *SDMAC_WTC;
        *SDMAC_WTC = ovalue;

        if (rvalue == wvalue) {
            if (wvalue != 0x00000000UL && wvalue != 0xffffffffUL) {
                *why = "WTC read-only bits followed a write";
                return 0;
            }
        } else if (((rvalue ^ wvalue) & 0x00ffffffUL) == 0) {
            /* low 24 bits followed: SDMAC-02 behavior */
        } else if ((rvalue & 0x04) == 0) {
            if (wvalue & 0x04)
                version = 4;                 // bit 2 written 1, read 0: SDMAC-04
        } else {
            *why = "WTC readback bit corruption";
            return 0;
        }
    }
    return version;
}

// Pattern write/readback of one 32-bit register. wmask limits what gets
// written (never-writable bits forced 0), rmask limits what's compared on
// read (bits that legitimately don't read back — e.g. WTC's high byte —
// ignored; ACR passes all-ones here so stuck bits 1-0 WOULD be caught).
// Returns the number of failing patterns (0 = pass), first failure in *fail.
// Original register value is restored — nothing here survives the test.
static int a3kRegPatternCheck(volatile uint32_t *reg, uint32_t wmask,
                              uint32_t rmask, A3kRegFail *fail)
{
    int errs = 0;
    uint32_t ovalue = *reg;
    for (int i = 0; i < A3K_NUM_REG_VALUES; i++) {
        uint32_t wvalue = a3kRegTestValues[i] & wmask;
        *reg = wvalue;
        (void)*RAMSEY_VERSION_ADDR;
        uint32_t rvalue = *reg & rmask;
        if (rvalue != (wvalue & rmask)) {
            if (errs++ == 0) {
                fail->wrote = wvalue;
                fail->read  = rvalue;
                fail->note  = NULL;
            }
        }
    }
    *reg = ovalue;
    return errs;
}

static int a3kCheckAcr(A3kRegFail *fail)
{
    return a3kRegPatternCheck(RAMSEY_ACR, 0xFFFFFFFCUL, 0xFFFFFFFFUL, fail);
}
static int a3kCheckWtc(A3kRegFail *fail)
{
    return a3kRegPatternCheck(SDMAC_WTC, 0x00FFFFFFUL, 0x00FFFFFFUL, fail);
}
static int a3kCheckSspb(A3kRegFail *fail)
{
    return a3kRegPatternCheck(SDMAC_SSPBDAT, 0x000000FFUL, 0x000000FFUL, fail);
}

// WD33C93 register file through the SDMAC PORT0 pass-through — the same
// select-then-data indirection every byte of DMA data ultimately crosses.
// Pattern-tests the CDB "Logical Address LSB" scratch byte while guarding
// that CONTROL didn't change underneath: a wrong value in CONTROL after
// writing register $0A is the signature of a broken register-select path
// (writes landing in the wrong internal register), which a plain
// write/readback of one register can't distinguish from a healthy chip.
static int a3kWdcRegCheck(A3kRegFail *fail)
{
    int errs = 0;
    uint8_t covalue = a3k_wd_read(WD_CONTROL);
    uint8_t ovalue  = a3k_wd_read(WD_CDB8_LADDR0);
    for (int i = 0; i < A3K_NUM_REG_VALUES; i++) {
        uint8_t wvalue = (uint8_t)a3kRegTestValues[i];
        a3k_wd_write(WD_CDB8_LADDR0, wvalue);
        (void)*RAMSEY_VERSION_ADDR;
        uint8_t crvalue = a3k_wd_read(WD_CONTROL);
        uint8_t rvalue  = a3k_wd_read(WD_CDB8_LADDR0);
        if (rvalue != wvalue) {
            if (errs++ == 0) {
                fail->wrote = wvalue;
                fail->read  = rvalue;
                fail->note  = NULL;
            }
        } else if (crvalue != covalue) {
            if (errs++ == 0) {
                fail->wrote = covalue;
                fail->read  = crvalue;
                fail->note  = "WD CONTROL changed";
            }
        }
    }
    a3k_wd_write(WD_CDB8_LADDR0, ovalue);
    return errs;
}

typedef struct {
    const char *label;
    int (*run)(A3kRegFail *fail);
} A3kRegCheck;
#define A3K_MAX_REG_CHECKS 3

// Build the check list this system can actually run: ACR always (it's in
// Ramsey — meaningful even when the SDMAC half looks dead), the SDMAC check
// matching the detected revision (skipped when the revision probe failed —
// its failure already IS the diagnosis), and the WD33C93 check unless the
// chip is busy/absent. checks[] must be a caller's stack array, never a
// static — function pointers are written into it, and statics land in
// read-only ROM here (see the A4000 IDE gotcha).
static int a3kBuildRegChecks(A3kRegCheck *checks, int sdmacVer, int wdcOk)
{
    int n = 0;
    checks[n].label = "Ramsey ACR (DMA address)   ";
    checks[n].run   = a3kCheckAcr;
    n++;
    if (sdmacVer == 2) {
        checks[n].label = "SDMAC WTC (transfer count) ";
        checks[n].run   = a3kCheckWtc;
        n++;
    } else if (sdmacVer == 4) {
        checks[n].label = "SDMAC SSPBDAT (serial bus) ";
        checks[n].run   = a3kCheckSspb;
        n++;
    }
    if (wdcOk) {
        checks[n].label = "WD33C93 regs (pass-through)";
        checks[n].run   = a3kWdcRegCheck;
        n++;
    }
    return n;
}

// One-line failure detail, caller prints the "label: " prefix. Raw
// wrote/read values on screen deliberately — same "show what the hardware
// actually did" rule as a4kDmaPrintFail(): the failing bit PATTERN (one
// lane? one byte? everything?) is the diagnosis on a real board.
static void a3kRegPrintFail(const A3kRegFail *fail, int errs)
{
    print("FAILED (wrote $", RED);
    print(binHex(fail->wrote), RED);
    print(" read $", RED);
    print(binHex(fail->read), RED);
    if (fail->note) {
        print(", ", RED);
        print((char *)fail->note, RED);
    }
    print(", ", RED);
    print(binDec(errs), RED);
    print("/", RED);
    print(binDec(A3K_NUM_REG_VALUES), RED);
    print(" patterns)\n", RED);
}

// Shared entry setup for both test modes: SCSI mode + bus-float guard, then
// Ramsey/SDMAC identification. Returns 0 on bus float (caller bails), 1
// otherwise, with *verOut = SDMAC revision (0 = probe failed, reason
// printed). Identification prints as a side effect — it doubles as the
// test's "what am I looking at" report (Ramsey names per Hooper's tool:
// $0D=Ramsey-04, $0F=Ramsey-07, $7F=pre-production).
static int a3kRegTestSetup(int *verOut)
{
    *SDMAC_DAWR = DAWR_A3000_VAL;
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;

    if (*WD_AUX_STATUS == 0xFF) {
        print("  Bus float - controller not responding.\n", RED);
        return 0;
    }

    uint8_t ramseyVer = *RAMSEY_VERSION_ADDR;
    print("Ramsey: ", WHITE);
    if (ramseyVer == 0x0D)
        print("Ramsey-04 ($0D)\n", CYAN);
    else if (ramseyVer == 0x0F)
        print("Ramsey-07 ($0F)\n", CYAN);
    else if (ramseyVer == ANCIENT_RAMSEY)
        print("ancient/pre-production ($7F)\n", YELLOW);
    else {
        print("unknown version $", YELLOW);
        print(binHexByte(ramseyVer), YELLOW);
        print("\n", WHITE);
    }

    a3kPrintWdChip();   // variant + microcode line; non-fatal here — the
                        // register test's own wdcOk logic handles a dead WD

    const char *why = "";
    int ver = a3kSdmacVersion(&why);
    print("SDMAC:  ", WHITE);
    if (ver == 2) {
        print("SDMAC-02\n", CYAN);
    } else if (ver == 4) {
        print("SDMAC-04", CYAN);
        uint32_t rev = *SDMAC_REVISION_REG;   // ReSDMAC-only; open bus on real SDMAC
        if ((rev >> 24) == 'v' && ((rev >> 8) & 0xFF) == '.') {
            char revStr[5];
            revStr[0] = (char)(rev >> 24);
            revStr[1] = (char)(rev >> 16);
            revStr[2] = (char)(rev >> 8);
            revStr[3] = (char)rev;
            revStr[4] = 0;
            print("  (ReSDMAC ", CYAN);
            print(revStr, CYAN);
            print(")", CYAN);
        }
        print("\n", WHITE);
    } else {
        print("revision probe FAILED - ", RED);
        print((char *)why, RED);
        print("\n", WHITE);
    }

    *verOut = ver;
    return 1;
}

static int a3kDmaTest(void)
{
    A3kRegCheck checks[A3K_MAX_REG_CHECKS];
    A3kRegFail fail;
    int ver;

    print("\nA3000/A3000T SCSI - DMA Register Test (no SCSI bus activity)\n", WHITE);
    print("EXPERIMENTAL - please report results\n\n", YELLOW);
    print("The SDMAC cannot move memory-to-memory like the A4000T's 53C710,\n", WHITE);
    print("so this pattern-tests the DMA path's registers instead.\n\n", WHITE);

    if (!a3kRegTestSetup(&ver))
        return 0;

    uint8_t aux = a3k_wd_aux();
    int wdcOk = !(aux & (WD_ASR_BSY | WD_ASR_CIP));
    if (!wdcOk)
        print("(WD33C93 busy - register check skipped)\n", YELLOW);
    print("\n", WHITE);

    int nChecks = a3kBuildRegChecks(checks, ver, wdcOk);
    uint32_t passed = 0, total = 0;

    for (int c = 0; c < nChecks; c++) {
        print("  ", WHITE);
        print((char *)checks[c].label, CYAN);
        print(": ", WHITE);
        total++;
        int errs = checks[c].run(&fail);
        if (errs == 0) {
            print("PASSED\n", GREEN);
            passed++;
        } else {
            a3kRegPrintFail(&fail, errs);
        }
    }

    print("\nResult: ", WHITE);
    print(binDec((int32_t)passed), passed == total ? GREEN : RED);
    print(" / ", WHITE);
    print(binDec((int32_t)total), WHITE);
    print(" checks passed", WHITE);
    if (ver == 0)
        print("  (SDMAC check not run)", YELLOW);
    print("\n", WHITE);

    return (int)passed;
}

// Soak version — same shape as a4kDmaTestRepeat() (cumulative per-check fail
// tallies, "last fail" line with full detail, in-place display updates,
// abort via a4kDmaAbortRequested()). One structural difference: a
// single register check finishes in microseconds, not the seconds an A4000T
// DMA move takes, so one displayed pass runs each check A3K_REGSOAK_REPS
// times — otherwise the screen would redraw hundreds of times a second and a
// "pass" count would be meaningless as a soak unit. Abort is still polled
// inside the rep loop (every 32 reps) so ESC stays immediate.
#define A3K_REGSOAK_REPS 1000

static int a3kDmaTestRepeat(void)
{
    A3kRegCheck checks[A3K_MAX_REG_CHECKS];
    A3kRegFail fail;
    uint32_t checkFails[A3K_MAX_REG_CHECKS] = { 0, 0, 0 };
    int ver;

    globals->HddEscLatch = 0;   // an ESC latched during an earlier scan/identify
                                // wait must not insta-abort this fresh soak

    print("\002A3000/A3000T SCSI - DMA Register Test (Repeat)\n", WHITE);
    print("\002EXPERIMENTAL - please report results\n\n", YELLOW);
    if (!a3kRegTestSetup(&ver))
        return 0;

    uint8_t aux = a3k_wd_aux();
    int wdcOk = !(aux & (WD_ASR_BSY | WD_ASR_CIP));
    int nChecks = a3kBuildRegChecks(checks, ver, wdcOk);

    uint32_t iterations = 0, totalPassed = 0, totalChecks = 0;
    A3kRegFail lastFail;
    uint32_t lastFailPass = 0;
    int lastFailCheck = 0, lastFailErrs = 0, haveFail = 0;
    int aborted = 0;

    // Draw-once frame with in-place number updates, same shape as
    // a4kDmaTestRepeat() — see the layout comment there. NO clearScreen():
    // the frame starts BELOW the Ramsey/SDMAC identification the setup just
    // printed, so that stays visible for the whole soak (user request). Row
    // layout is therefore read off the live cursor, not hardcoded.
    print("\nESC or both buttons: stop\n\n", WHITE);
    if (ver == 0)
        print("(SDMAC revision probe failed - SDMAC check skipped)\n\n", YELLOW);
    if (!wdcOk)
        print("(WD33C93 busy - register check skipped)\n\n", YELLOW);
    uint32_t passRow   = *(volatile uint8_t *)&globals->Ypos;   // row lives in the MSB byte (see setPos)
    uint32_t totalsRow = passRow + (uint32_t)nChecks + 2;
    uint32_t failRow   = totalsRow + 1;

    while (!aborted) {
        setPos(0, passRow);
        print("Pass ", WHITE);
        print(binDec((int32_t)(iterations + 1)), CYAN);
        print(" (", WHITE);
        print(binDec(A3K_REGSOAK_REPS), CYAN);
        print(" reps per check):", WHITE);

        int newFail = 0;
        for (int c = 0; c < nChecks && !aborted; c++) {
            uint32_t passedReps = 0;

            for (int rep = 0; rep < A3K_REGSOAK_REPS; rep++) {
                int errs = checks[c].run(&fail);
                if (errs == 0) {
                    passedReps++;
                } else {
                    checkFails[c]++;
                    lastFail      = fail;
                    lastFailPass  = iterations + 1;
                    lastFailCheck = c;
                    lastFailErrs  = errs;
                    haveFail      = 1;
                    newFail       = 1;
                }
                if ((rep & 31) == 31 && a4kDmaAbortRequested()) {
                    aborted = 1;
                    break;
                }
            }
            totalPassed += passedReps;
            totalChecks += A3K_REGSOAK_REPS;

            setPos(0, passRow + 1 + (uint32_t)c);
            print("  ", WHITE);
            print((char *)checks[c].label, CYAN);
            print(": ", WHITE);
            print(binDec((int32_t)passedReps),
                  passedReps == A3K_REGSOAK_REPS ? GREEN : RED);
            print("/", WHITE);
            print(binDec(A3K_REGSOAK_REPS), WHITE);
            print("   fails so far: ", WHITE);
            print(binDec((int32_t)checkFails[c]), checkFails[c] ? RED : GREEN);
            print("    ", WHITE);   // pad: passedReps can lose up to 3 digits vs last pass
        }
        iterations++;

        setPos(0, totalsRow);
        print("Running totals over ", WHITE);
        print(binDec((int32_t)iterations), CYAN);
        print(" pass(es): ", WHITE);
        print(binDec((int32_t)totalPassed), totalPassed == totalChecks ? GREEN : YELLOW);
        print(" / ", WHITE);
        print(binDec((int32_t)totalChecks), WHITE);
        print(" reps passed", WHITE);

        if (haveFail && newFail) {
            soakBlankRow(failRow);
            soakBlankRow(failRow + 1);   // fail line can wrap onto a second row
            setPos(0, failRow);
            print("Last fail: pass ", WHITE);
            print(binDec((int32_t)lastFailPass), CYAN);
            print("  ", WHITE);
            print((char *)checks[lastFailCheck].label, CYAN);
            print(": ", WHITE);
            a3kRegPrintFail(&lastFail, lastFailErrs);
        }
    }

    return (int)totalPassed;
}

// ---------------------------------------------------------------------------
// A3000 SCSI DMA Transfer Test (Tier 2) — real SDMAC DMA vs the proven PIO
// path, menu entries 6/7.
// ---------------------------------------------------------------------------
//
// The Tier 1 register test above proves the DMA path's registers latch; it
// cannot prove data actually flows through the SDMAC FIFO into memory via
// Ramsey arbitration — the path behind the classic A3000 "DMA corruption"
// fault family. This test does: it reads the SAME 8 blocks (LBA 0-7, the
// always-present RDB area — strictly READ-ONLY, nothing is ever written to
// the disk) once through the proven CPU-pumped PIO path and once through
// real SDMAC DMA, and compares. Unlike everything else in this menu it
// REQUIRES a working SCSI target and generates real bus traffic — which is
// why it's a separate menu entry instead of part of the bus-quiet "DMA
// Test" above.
//
// The DMA programming sequence is taken verbatim from the Linux A3000
// driver pair (drivers/scsi/a3000.c dma_setup()/dma_stop() for the SDMAC
// half, drivers/scsi/wd33c93.c transfer_bytes() for the WD half and their
// ordering) — a real, independently-written driver for this exact
// hardware — NOT reverse-engineered from docs. Everything around the data
// phase (select, phases, reset discipline) reuses this file's machinery
// already confirmed on real A3000 hardware.
//
// A double-PIO reference read guards the verdict: both PIO reads must agree
// before DMA is judged against them, so an unstable medium/PIO path reports
// as exactly that instead of being blamed on DMA.

#define A3K_XFER_BLOCKS 8
#define A3K_XFER_BYTES  (A3K_XFER_BLOCKS * 512)
#define A3K_XFER_LBA    0

// Failure forensics for the DMA phase walk — a bare "command did not
// complete" can't distinguish a walk that died before DATA_IN from a DMA
// engine that never moved a byte from one that stalled mid-FIFO (each points
// at completely different hardware). Lives on the stack, never a static (this
// linker script puts statics in read-only ROM — see A3kInquiryDebug's note).
#define A3K_DMAPH_NONE    0
#define A3K_DMAPH_SELECT  1
#define A3K_DMAPH_MSGOUT  2
#define A3K_DMAPH_CMD     3
#define A3K_DMAPH_DATA    4
#define A3K_DMAPH_STATUS  5
#define A3K_DMAPH_MSGIN   6

static const char * const a3kDmaPhaseNames[] = {
    "?", "SELECT", "MSG_OUT", "CMD", "DATA_IN", "STATUS", "MSG_IN"
};

typedef struct {
    uint8_t  failPhase;   // A3K_DMAPH_* step where the walk stopped (NONE = clean)
    uint8_t  st;          // WD status byte at that point (0xFF = poll timeout)
    uint8_t  istr;        // SDMAC ISTR snapshot at the moment of a DATA_IN stall
    uint8_t  stalled;     // DATA_IN DMA timed out (the drain below may still recover)
    uint32_t acrDelta;    // bytes the engine's address counter (ACR) advanced
    uint32_t drained;     // bytes recovered by the PIO fallback drain
    // Engine-end forensics for the deterministic-corruption hunt (captured on
    // every DMA data phase, at moments the real drivers also touch hw):
    uint32_t acrArmDelta; // ACR readback minus programmed value BEFORE ST_DMA —
                          // nonzero = the CPU('s card) mangled the register write
    uint32_t acrEndDelta; // ACR minus buffer start right after the completion
                          // INT: 4096 = engine did every longword beat; ~2048 =
                          // it stepped in words; anything else = skipped beats
    uint8_t  istrEnd;     // ISTR right after completion, BEFORE teardown clears
                          // it — bit 3 = FIFO under-run, bit 2 = over-run
                          // (latched, read-clear; never visible after teardown)
} A3kDmaDebug;

// VERTB tick used purely to wake the STOP'd CPU in a3k_wd_wait_status_dma
// below — ack Paula and return; the wait loop counts wakeups as its timeout.
// Same double-ack convention as ScsiPortsIRQ.
__interrupt void A3kVertBAckIRQ(VARS)
{
    custom->intreq = 0x0020;
    custom->intreq = 0x0020;
}

// DMA twin of a3k_wd_wait_status(). While the SDMAC engine is live the CPU
// must stay off the bus COMPLETELY — learned on real hardware in rounds:
//
// Round 1: polling the WD33C93's ASR during DMA collides with the engine's
// DACK cycles on the shared PORT0 pass-through — the transfer never
// completed at all ("command did not complete", wedged bus).
//
// Round 2: polling the SDMAC's own ISTR instead let the transfer complete
// but corrupted — CPU slave accesses to the SDMAC register file during
// FIFO bus-mastership.
//
// Round 3: polling only a RAM flag STILL corrupted, deterministically, on
// a 68060 CPU card (Ramsey-07, SDMAC-04): with DiagROM's caches-off
// default, EVERY instruction fetch of the polling loop crosses the
// motherboard bus and competes with SDMAC mastership through the CPU
// card's arbiter — engine-end forensics showed ACR beats silently lost
// (end+3402 / end+2344 of 4096, no FIFO errors latched). Under the OS the
// 060 runs cached and the bus is quiet during DMA, which is why the same
// machine boots from SCSI fine.
//
// So: STOP. Park the CPU entirely (no fetches, no bus cycles) until an
// interrupt: the SCSI INT2 ISR wakes us on completion, and a temporary
// VERTB tick (installed by the caller for exactly this window) wakes us
// 50/60x a second purely as a timeout clock. The single post-timeout ISTR
// look is the safety net for a completion whose INT2 never arrived; by
// then disturbing a (dead) transfer no longer matters.
#define A3K_DMA_STOP_TICKS 40   // vblank wakeups =~ 0.7-0.8s budget
static uint8_t a3k_wd_wait_status_dma(void)
{
    volatile struct GlobalVars *globals = a3k_globals();
    for (int i = 0; i < A3K_DMA_STOP_TICKS; i++) {
        if (globals->ScsiIrqPending) {
            globals->ScsiIrqPending = 0;
            return globals->ScsiIrqStatus;
        }
        asm volatile ("stop #0x2000" ::: "cc", "memory");
    }
    if (*SDMAC_ISTR & SDMAC_ISTR_PINT)
        return a3k_wd_read(WD_SCSI_STATUS);
    return 0xFF;
}

// Post-stall bus release. Abandoning the target mid-DATA_IN is what left
// real hardware with the SCSI LED latched on and every device unreachable
// until reset: the target holds BSY waiting to hand over its remaining
// bytes, a WD soft-RESET doesn't release it, and the only true bus reset
// (SDMAC PRESET) hard-hung a real A3000T (see SDMAC_CNTR_PRESET). Instead,
// with the SDMAC engine stopped and the WD switched back to polled mode by
// the caller, its DRQ becomes DBR — so pump the leftover bytes by CPU into
// nowhere purely so the transaction can finish and the target disconnect
// cleanly. Returns the post-drain WD status (ideally the STATUS-phase
// request, letting the caller's normal walk complete), 0xFF = bus truly dead.
static uint8_t a3k_dma_drain(A3kDmaDebug *dbg)
{
    volatile struct GlobalVars *globals = a3k_globals();
    uint32_t t = 1500000UL;
    while (t--) {
        if ((t & 0xFFFF) == 0) hddInputService();   // t resets per drained byte — wait time only
        if (globals->ScsiIrqPending) {
            globals->ScsiIrqPending = 0;
            return globals->ScsiIrqStatus;
        }
        uint8_t asr = a3k_wd_aux();
        if (asr == 0xFF) continue;
        if (asr & WD_ASR_DBR) {
            (void)a3k_wd_read(WD_DATA);
            dbg->drained++;
            t = 1500000UL;
        } else if (asr & WD_ASR_INT) {
            return a3k_wd_read(WD_SCSI_STATUS);
        }
    }
    return 0xFF;
}

// DMA twin of a3k_wd_do_phase(read_dir=1): same TRANSFER_INFO for the
// DATA_IN phase, but bytes move SCSI -> SDMAC FIFO -> memory via DACK
// cycles instead of the CPU pumping WD_DATA. Order per wd33c93.c
// transfer_bytes(): arm the SDMAC FIRST (it idles until the WD asserts
// DRQ), then put the WD in DMA mode, load the 24-bit count, TRANSFER_INFO,
// and wait for the phase-end INT. WTC is never written — the WD's own
// transfer counter governs length (Linux never touches WTC either).
// Teardown always runs, success or timeout, per dma_stop(): INTENA off
// (direction bit stays clear — read), FLUSH then wait FIFO-empty so the
// last partial longword lands in memory (bounded wait — Linux spins
// forever, a diagnostic ROM on broken hardware must not), CINT, SP_DMA,
// CNTR restored, WD CONTROL back to polled for the STATUS/MSG_IN phases.
// buf MUST be longword-aligned (Linux A3000_XFER_MASK) and in
// SDMAC-reachable RAM — callers guarantee both.
static uint8_t a3k_wd_do_phase_dma_in(uint8_t *buf, int count, A3kDmaDebug *dbg)
{
    // Wake source for the STOP'd wait below (see a3k_wd_wait_status_dma):
    // VERTB tick, installed only for this DMA window.
    *(volatile APTR *) + 0x6C = A3kVertBAckIRQ;
    custom->intreq = 0x0020;
    custom->intreq = 0x0020;
    custom->intena = 0x8020;                               // SET | VERTB

    *SDMAC_CNTR   = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;   // DDIR clear = SCSI->memory
    *RAMSEY_ACR   = (uint32_t)(uintptr_t)buf;
    // Engine still idle — verify the address actually latched before arming.
    dbg->acrArmDelta = *RAMSEY_ACR - (uint32_t)(uintptr_t)buf;
    *SDMAC_ST_DMA = 1;

    a3k_wd_write(WD_CONTROL, WD_CONTROL_DMA);
    a3k_wd_write(WD_XFER_CNT_H, (uint8_t)(count >> 16));
    a3k_wd_write(WD_XFER_CNT_M, (uint8_t)(count >> 8));
    a3k_wd_write(WD_XFER_CNT_L, (uint8_t)count);
    a3k_wd_write(WD_COMMAND, WDCMD_TRANSFER_INFO);

    uint8_t st = a3k_wd_wait_status_dma();

    // DMA window over — retire the VERTB wake source before anything else.
    custom->intena = 0x0020;                               // CLR VERTB
    custom->intreq = 0x0020;
    custom->intreq = 0x0020;
    *(volatile APTR *) + 0x6C = RTEcode;

    if (st != 0xFF) {
        // Completion INT delivered — this is exactly when the real drivers
        // read the hardware, so these two probes are safe. ISTR first: its
        // latched error bits (FIFO over/under-run) are read-clear and the
        // teardown below would destroy them.
        dbg->istrEnd     = *SDMAC_ISTR;
        dbg->acrEndDelta = *RAMSEY_ACR - (uint32_t)(uintptr_t)buf;
    }

    if (st == 0xFF) {
        // Snapshot BEFORE teardown mutates them: ISTR (FIFOE/FIFOF tell FIFO
        // state), and how far Ramsey's address counter got — 0 means the
        // engine never wrote memory at all, partial means it stalled mid-run.
        dbg->stalled  = 1;
        dbg->istr     = *SDMAC_ISTR;
        dbg->acrDelta = *RAMSEY_ACR - (uint32_t)(uintptr_t)buf;
    }

    *SDMAC_CNTR = SDMAC_CNTR_PDMD;
    *SDMAC_FLUSH_STROBE = 1;
    for (int i = 0; i < A3K_WAIT_ITERS; i++) {
        if (*SDMAC_ISTR & SDMAC_ISTR_FIFOE)
            break;
        waitShort();
    }
    *SDMAC_CINT_STROBE = 1;
    *SDMAC_SP_DMA = 1;
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;
    a3k_wd_write(WD_CONTROL, 0x00);

    // Engine stopped, WD back in polled mode — try to finish the transfer by
    // CPU so the target lets go of the bus instead of wedging it (see
    // a3k_dma_drain). A recovered status still counts as a FAILED DMA row —
    // a3kXferRun checks dbg->stalled — the drain is purely bus hygiene.
    if (st == 0xFF)
        st = a3k_dma_drain(dbg);

    return st;
}

// DMA twin of a3k_scsi_command(): identical phase walk (SELECT ->
// MSG_OUT(IDENTIFY) -> CMD -> DATA_IN -> STATUS -> MSG_IN) with ONLY the
// DATA_IN phase swapped for the SDMAC version above. Deliberately a
// separate function rather than a flag on the original — that path is
// confirmed on real hardware and stays byte-identical.
static int a3k_scsi_command_dma(uint8_t unit, const uint8_t *cdb, int cdbLen,
                                uint8_t *dataBuf, int dataLen, A3kDmaDebug *dbg)
{
    // Same bailout-before-abandoning rule as a3k_scsi_command() — including
    // after a DATA_IN whose internal drain couldn't finish the job (the WD
    // is back in polled mode by then, so the bailout's machinery applies).
    // The final status/msg check stays bailout-free: transaction complete.
    uint8_t st = a3k_scsi_select(unit);
    if (!((st & 0xF0) == 0x80 && (st & 0x0F) == WDPHASE_MSG_OUT)) {
        dbg->failPhase = A3K_DMAPH_SELECT; dbg->st = st;
        a3k_scsi_bailout(st); return 0;
    }

    uint8_t identify = 0x80;
    st = a3k_wd_do_phase_sbt(&identify, 0);
    if (!((st & 0x0F) == WDPHASE_CMD && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) {
        dbg->failPhase = A3K_DMAPH_MSGOUT; dbg->st = st;
        a3k_scsi_bailout(st); return 0;
    }

    st = a3k_wd_do_phase((uint8_t *)cdb, cdbLen, 0);
    if (!((st & 0x0F) == WDPHASE_DATA_IN && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) {
        dbg->failPhase = A3K_DMAPH_CMD; dbg->st = st;
        a3k_scsi_bailout(st); return 0;
    }

    st = a3k_wd_do_phase_dma_in(dataBuf, dataLen, dbg);
    if (!((st & 0x0F) == WDPHASE_STATUS && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) {
        dbg->failPhase = A3K_DMAPH_DATA; dbg->st = st;
        a3k_scsi_bailout(st); return 0;
    }

    uint8_t statusByte = 0xFF;
    st = a3k_wd_do_phase_sbt(&statusByte, 1);
    if (!((st & 0x0F) == WDPHASE_MSG_IN && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) {
        dbg->failPhase = A3K_DMAPH_STATUS; dbg->st = st;
        a3k_scsi_bailout(st); return 0;
    }

    uint8_t msgByte = 0xFF;
    st = a3k_wd_do_phase_sbt(&msgByte, 1);
    a3k_wd_negate_ack_if_paused(st);

    if (statusByte != 0 || msgByte != 0) {
        dbg->failPhase = A3K_DMAPH_MSGIN; dbg->st = st; return 0;
    }
    return 1;
}

// N-block READ(10) with a PIO or DMA data phase. Fresh reset+init per
// command — same finicky-chip convention as a3k_scsi_read_block() (see its
// comment; the soak mode issues hundreds of these back to back, exactly the
// "arbitrary run length" that comment warns about).
static int a3k_scsi_read_blocks(uint8_t unit, uint32_t lba, uint8_t nblocks,
                                uint8_t *buf, int dataLen, int useDma,
                                A3kDmaDebug *dbg)
{
    a3k_wd_reset();
    a3k_wd_init();
    // Stale-ISR-flag clear (the "at SELECT st=$00" phantom found on real hw)
    // now lives at the end of a3k_wd_init() itself, shared by every path.

    uint8_t cdb[10] = {
        SCSI_READ10, 0,
        (uint8_t)(lba >> 24), (uint8_t)(lba >> 16), (uint8_t)(lba >> 8), (uint8_t)lba,
        0, 0, nblocks, 0
    };
    return useDma ? a3k_scsi_command_dma(unit, cdb, 10, buf, dataLen, dbg)
                  : a3k_scsi_command(unit, cdb, 10, buf, dataLen);
}

// First SCSI ID that answers READ CAPACITY — any readable disk-like target
// will do. Same full-reset-per-ID discipline as scanA3000SCSI(). Returns
// the ID, or -1 if the bus has nothing usable on it.
static int a3kXferFindTarget(void)
{
    uint8_t capBuf[SCSI_READ_CAPACITY_LEN];
    static const uint8_t readCapCdb[10] = { SCSI_READ_CAPACITY, 0,0,0,0,0,0,0,0, 0 };

    for (int id = 0; id <= 7; id++) {
        if (id == A3K_HOST_SCSI_ID) continue;
        a3k_wd_reset();
        a3k_wd_init();
        if (a3k_scsi_command(id, readCapCdb, 10, capBuf, SCSI_READ_CAPACITY_LEN))
            return id;
    }
    return -1;
}

#define A3K_XFERFAIL_CMD      1   // phase walk never completed (DMA hang lands here)
#define A3K_XFERFAIL_NODATA   2   // command "worked" but the buffer is untouched
#define A3K_XFERFAIL_MISMATCH 3   // data arrived, some of it wrong — the interesting one
#define A3K_XFERFAIL_DMASTALL 4   // DMA timed out but the PIO drain completed the
                                  // command — bus released, DMA still judged broken

typedef struct {
    int         reason;     // A3K_XFERFAIL_*
    uint32_t    offset;     // first differing byte (MISMATCH)
    uint8_t     expect;     // reference byte there
    uint8_t     got;        // what the transfer delivered
    uint32_t    badBytes;   // total differing bytes (MISMATCH)
    A3kDmaDebug dbg;        // DMA-walk forensics (all-zero on PIO rows)
    // MISMATCH shape analysis — 2026-07-29 real hw delivered ~75% wrong
    // bytes, i.e. ~1 in 4 right: that ratio is the fingerprint of a byte-lane
    // fault in the FIFO's byte->longword packing, not random noise, but only
    // per-lane counts can confirm it. poison = bytes still holding the
    // complement prefill (DMA never wrote there, distinct from wrote-wrong).
    uint32_t    laneBad[4]; // differing bytes by (offset & 3)
    uint32_t    poison;     // bytes still equal to the prefill poison
    uint32_t    dumpOff;    // start offset of the 16-byte dump below
    uint8_t     exp16[16];  // reference bytes at dumpOff
    uint8_t     got16[16];  // delivered bytes at dumpOff
} A3kXferFail;

typedef struct {
    const char *label;
    uint8_t    *buf;       // destination; longword-aligned for the DMA rows
    int         useDma;    // 0 = the PIO stability row
    const char *failHint;  // extra context line printed under a failure, or NULL
} A3kXferDir;
#define A3K_XFER_MAX_DIRS 3

// One read into dir->buf, compared against the PIO reference. The buffer is
// prefilled with the byte-wise complement first (a4kDmaRunPattern()'s
// anti-vacuous-pass trick) — which also makes "moved nothing" its own
// detectable verdict: a byte can never equal its own complement, so a
// buffer still complement everywhere after a "successful" command means the
// SDMAC never wrote memory at all (on this machine that's what DMA into
// CPU-card RAM the SDMAC can't reach looks like — distinct from corruption).
static int a3kXferRun(int target, const A3kXferDir *dir, const uint8_t *ref,
                      A3kXferFail *fail)
{
    A3kDmaDebug dbg = { 0, 0, 0, 0, 0, 0 };

    for (uint32_t i = 0; i < A3K_XFER_BYTES; i++)
        dir->buf[i] = (uint8_t)~ref[i];

    int cmdOk = a3k_scsi_read_blocks((uint8_t)target, A3K_XFER_LBA,
                                     A3K_XFER_BLOCKS, dir->buf,
                                     A3K_XFER_BYTES, dir->useDma, &dbg);
    fail->dbg = dbg;
    if (!cmdOk) {
        fail->reason = A3K_XFERFAIL_CMD;
        return 0;
    }
    // Command completed, but only because the PIO drain bailed out a stalled
    // DMA — the buffer contents are meaningless (drained bytes were
    // discarded) and DMA did NOT do its job. Never let this look like a pass.
    if (dbg.stalled) {
        fail->reason = A3K_XFERFAIL_DMASTALL;
        return 0;
    }

    uint32_t bad = 0, untouched = 0, first = 0;
    int haveFirst = 0;
    fail->laneBad[0] = fail->laneBad[1] = fail->laneBad[2] = fail->laneBad[3] = 0;
    for (uint32_t i = 0; i < A3K_XFER_BYTES; i++) {
        if (dir->buf[i] == (uint8_t)~ref[i])
            untouched++;
        if (dir->buf[i] != ref[i]) {
            if (!haveFirst) { haveFirst = 1; first = i; }
            bad++;
            fail->laneBad[i & 3]++;
        }
    }
    if (bad == 0)
        return 1;

    fail->reason   = (untouched == A3K_XFER_BYTES) ? A3K_XFERFAIL_NODATA
                                                   : A3K_XFERFAIL_MISMATCH;
    fail->offset   = first;
    fail->expect   = ref[first];
    fail->got      = dir->buf[first];
    fail->badBytes = bad;
    fail->poison   = untouched;
    fail->dumpOff  = first & ~3UL;
    if (fail->dumpOff > A3K_XFER_BYTES - 16)
        fail->dumpOff = A3K_XFER_BYTES - 16;
    for (uint32_t i = 0; i < 16; i++) {
        fail->exp16[i] = ref[fail->dumpOff + i];
        fail->got16[i] = dir->buf[fail->dumpOff + i];
    }
    return 0;
}

#define A3K_XFER_PIO_TRIES 3

// Chip-state suffix for a failed-attempt line: WD auxiliary status + SDMAC
// ISTR, both safe to read here (no DMA is ever active on these paths).
static void a3kXferPrintBusState(void)
{
    print(" (ASR $", YELLOW);
    print(binHexByte(a3k_wd_aux()), YELLOW);
    print(" ISTR $", YELLOW);
    print(binHexByte(*SDMAC_ISTR), YELLOW);
    print(")\n", YELLOW);
}

// Forensics line under a CMD/DMASTALL verdict. "moved x/4096" is Ramsey's
// address counter: 0 = the engine never wrote memory at all; "drained" is
// how many leftover bytes the PIO bail-out had to pump to free the bus.
static void a3kXferPrintDbg(const A3kDmaDebug *dbg)
{
    if (dbg->failPhase == A3K_DMAPH_NONE && !dbg->stalled)
        return;   // PIO row, or nothing captured — keep the old terse output
    print("    at ", YELLOW);
    print((char *)a3kDmaPhaseNames[dbg->stalled ? A3K_DMAPH_DATA
                                                : dbg->failPhase], YELLOW);
    print(" st=$", YELLOW);
    print(binHexByte(dbg->st), YELLOW);
    if (dbg->stalled) {
        print(" ISTR=$", YELLOW);
        print(binHexByte(dbg->istr), YELLOW);
        print(" moved ", YELLOW);
        print(binDec((int32_t)dbg->acrDelta), YELLOW);
        print("/", YELLOW);
        print(binDec(A3K_XFER_BYTES), YELLOW);
        print(" drained ", YELLOW);
        print(binDec((int32_t)dbg->drained), YELLOW);
    }
    print("\n", YELLOW);
}

static void a3kXferPrintFail(const A3kXferFail *fail)
{
    if (fail->reason == A3K_XFERFAIL_CMD) {
        print("FAILED (command did not complete)\n", RED);
        a3kXferPrintDbg(&fail->dbg);
        return;
    }
    if (fail->reason == A3K_XFERFAIL_DMASTALL) {
        print("FAILED (DMA stalled - PIO drain released the bus)\n", RED);
        a3kXferPrintDbg(&fail->dbg);
        return;
    }
    if (fail->reason == A3K_XFERFAIL_NODATA) {
        print("FAILED (buffer untouched - no data reached memory)\n", RED);
        return;
    }
    print("FAILED (first diff at ", RED);   // binHex() supplies the '$' itself
    print(binHex(fail->offset), RED);
    print(": expected $", RED);
    print(binHexByte(fail->expect), RED);
    print(" got $", RED);
    print(binHexByte(fail->got), RED);
    print(", ", RED);
    print(binDec((int32_t)fail->badBytes), RED);
    print(" bytes differ)\n", RED);

    // Shape analysis (see the A3kXferFail comment): equal-ish lane counts =
    // corruption spread evenly; one hot/cold lane = FIFO byte-lane fault;
    // nonzero poison = regions DMA never wrote at all.
    print("    lanes ", YELLOW);
    for (int l = 0; l < 4; l++) {
        print(binDec((int32_t)fail->laneBad[l]), YELLOW);
        print(l < 3 ? "/" : "", YELLOW);
    }
    print("  poison ", YELLOW);
    print(binDec((int32_t)fail->poison), YELLOW);
    print("  dump at ", YELLOW);
    print(binHex(fail->dumpOff), YELLOW);
    print("\n    engine: arm+", YELLOW);
    print(binDec((int32_t)fail->dbg.acrArmDelta), YELLOW);
    print("  end+", YELLOW);
    print(binDec((int32_t)fail->dbg.acrEndDelta), YELLOW);
    print("  ISTR $", YELLOW);
    print(binHexByte(fail->dbg.istrEnd), YELLOW);
    if (fail->dbg.istrEnd & 0x08)
        print(" UNDERRUN", RED);
    if (fail->dbg.istrEnd & 0x04)
        print(" OVERRUN", RED);
    print("\n    exp", YELLOW);
    for (int i = 0; i < 16; i++) {
        print(" ", YELLOW);
        print(binHexByte(fail->exp16[i]), YELLOW);
    }
    print("\n    got", YELLOW);
    for (int i = 0; i < 16; i++) {
        print(" ", YELLOW);
        print(binHexByte(fail->got16[i]), YELLOW);
    }
    print("\n", YELLOW);
}

// One-line verdict for the soak's "Last:" summary row. The full forensics
// belong to the FIRST fail's block, drawn once and never overwritten —
// learned from cdh's overnight BFG9060 run (2026-08-13): the wedge onset at
// ~pass 71 was overwritten ~87000 times before anyone saw the screen, and
// only the near-equal per-row tallies let the onset be reconstructed at
// all. Every branch stays short enough that the whole "Last:" row fits in
// 80 columns, and no newline is ever printed: on NTSC this row can be the
// bottom one, where a wrap or newline would scroll the frame.
static void a3kXferPrintTerse(const A3kXferFail *fail)
{
    switch (fail->reason) {
    case A3K_XFERFAIL_CMD:
        print("no completion", RED);
        // PIO rows never fill dbg (failPhase stays NONE) — suffix is
        // DMA-row-only.
        if (fail->dbg.failPhase != A3K_DMAPH_NONE) {
            print(" at ", RED);
            print((char *)a3kDmaPhaseNames[fail->dbg.failPhase], RED);
            print(" st=$", RED);
            print(binHexByte(fail->dbg.st), RED);
        }
        return;
    case A3K_XFERFAIL_DMASTALL:
        print("DMA stall (drain freed bus)", RED);
        return;
    case A3K_XFERFAIL_NODATA:
        print("no data reached memory", RED);
        return;
    default:
        print("mismatch (", RED);
        print(binDec((int32_t)fail->badBytes), RED);
        print(" bytes)", RED);
        return;
    }
}

// One-time setup shared by both modes: SCSI mode + bus-float guard, find a
// target, allocate/align every buffer, take the double-PIO reference.
// Returns the target ID (>= 0) with dirs/nDirs/ref filled in, or -1 after
// printing why. All allocations happen HERE, before any soak loop —
// the getMemory()/getChip() bump arena has no free, so per-pass allocation
// would exhaust it (see a4kDmaTestRepeat()'s comment).
// Caller owns a3k_scsi_irq_enable()/_disable() bracketing.
static int a3kXferSetup(A3kXferDir *dirs, int *nDirs, uint8_t **refOut)
{
    *SDMAC_DAWR = DAWR_A3000_VAL;
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;

    if (*WD_AUX_STATUS == 0xFF) {
        print("  Bus float - controller not responding.\n", RED);
        return -1;
    }

    // Chip RAM is the baseline DMA destination (always SDMAC-reachable);
    // reference + stability buffers are CPU-only so plain getMemory() RAM
    // is fine wherever it lands. +4 on DMA destinations for manual
    // longword alignment (ACR requirement, Linux A3000_XFER_MASK).
    uint8_t *ref  = (uint8_t *)getMemory(A3K_XFER_BYTES);
    uint8_t *pio2 = (uint8_t *)getMemory(A3K_XFER_BYTES);
    uint8_t *chip = NULL, *fast = NULL;

    uint32_t caddr = getChip(A3K_XFER_BYTES + 4);
    if (caddr != 0 && caddr != 1)
        chip = (uint8_t *)(uintptr_t)((caddr + 3) & ~3UL);

    if (globals->FastStart != 0) {
        uint8_t *fb = (uint8_t *)getMemory(A3K_XFER_BYTES + 4);
        uint32_t fs = (uint32_t)(uintptr_t)globals->FastStart;
        uint32_t fe = (uint32_t)(uintptr_t)globals->FastEnd;
        if (fb && (uint32_t)(uintptr_t)fb >= fs && (uint32_t)(uintptr_t)fb < fe)
            fast = (uint8_t *)(uintptr_t)(((uint32_t)(uintptr_t)fb + 3) & ~3UL);
    }

    if (!ref || !pio2 || !chip) {
        print("  Could not allocate test buffers.\n", RED);
        return -1;
    }

    // Destination addresses on screen: on accelerated machines "Fast" may be
    // CPU-card local RAM rather than motherboard fast — where that buffer
    // actually lives changes how a DMA verdict must be read.
    print("Buffers: chip ", WHITE);
    print(binHex((uint32_t)(uintptr_t)chip), CYAN);
    if (fast) {
        print("  fast ", WHITE);
        print(binHex((uint32_t)(uintptr_t)fast), CYAN);
    }
    print("\n", WHITE);

    print("Searching for a SCSI target...\n", WHITE);
    int target = a3kXferFindTarget();
    if (target < 0) {
        print("  No responding target found - this test needs a readable disk.\n", RED);
        return -1;
    }
    print("Target: SCSI ID ", WHITE);
    print(binDec(target), CYAN);
    print("\n", WHITE);

    // Double-PIO reference: prefill both reads with each other's poison is
    // pointless — what matters is they were taken independently and agree.
    //
    // Both baseline reads get A3K_XFER_PIO_TRIES attempts: on real hardware
    // (2026-07-29) the target scan's READ CAPACITY succeeded and the
    // immediately-following READ(10) failed once — the documented finicky
    // "works once" behavior of this chip/target combo, not a hard fault. A
    // single flake must not abort the whole test, and when it IS persistent
    // the per-attempt ASR/ISTR snapshot goes on screen instead of nothing.
    int refOk = 0;
    for (int attempt = 1; attempt <= A3K_XFER_PIO_TRIES && !refOk; attempt++) {
        refOk = a3k_scsi_read_blocks((uint8_t)target, A3K_XFER_LBA,
                                     A3K_XFER_BLOCKS, ref, A3K_XFER_BYTES,
                                     0, NULL);
        if (!refOk) {
            print("  PIO reference read failed - attempt ", YELLOW);
            print(binDec(attempt), YELLOW);
            a3kXferPrintBusState();
        }
    }
    if (!refOk) {
        print("  PIO reference read failed - cannot establish a baseline.\n", RED);
        return -1;
    }

    A3kXferDir pioDir = { "PIO", pio2, 0, NULL };
    A3kXferFail fail;
    int stabOk = 0;
    for (int attempt = 1; attempt <= A3K_XFER_PIO_TRIES && !stabOk; attempt++) {
        stabOk = a3kXferRun(target, &pioDir, ref, &fail);
        if (!stabOk) {
            print("  PIO stability read failed - attempt ", YELLOW);
            print(binDec(attempt), YELLOW);
            a3kXferPrintBusState();
        }
    }
    if (!stabOk) {
        print("  Two PIO reads disagree - unstable medium/PIO path, cannot judge DMA: ", RED);
        a3kXferPrintFail(&fail);
        return -1;
    }
    print("PIO baseline: two independent reads match.\n\n", GREEN);

    int n = 0;
    dirs[n].label = "PIO re-read (stability)"; dirs[n].buf = pio2;
    dirs[n].useDma = 0; dirs[n].failHint = NULL; n++;
    dirs[n].label = "DMA -> Chip RAM        "; dirs[n].buf = chip;
    dirs[n].useDma = 1; dirs[n].failHint = NULL; n++;
    if (fast) {
        dirs[n].label = "DMA -> Fast RAM        "; dirs[n].buf = fast;
        dirs[n].useDma = 1;
        dirs[n].failHint =
            "  (a Fast-only fail can also mean CPU-card RAM the SDMAC cannot reach)";
        n++;
    }
    *nDirs  = n;
    *refOut = ref;
    return target;
}

static int a3kXferTest(void)
{
    A3kXferDir dirs[A3K_XFER_MAX_DIRS];
    A3kXferFail fail;
    uint8_t *ref;
    int nDirs = 0;

    print("\nA3000/A3000T SCSI - DMA Transfer Test (READ-ONLY, blocks 0-7)\n", WHITE);
    print("EXPERIMENTAL - please report results\n\n", YELLOW);
    print("Reads the same 8 blocks via proven PIO and via real SDMAC DMA and\n", WHITE);
    print("compares - exercising the FIFO/Ramsey path the register test can't.\n\n", WHITE);

    a3k_scsi_irq_enable(a3k_globals());
    int target = a3kXferSetup(dirs, &nDirs, &ref);
    if (target < 0) {
        a3k_scsi_irq_disable();
        return 0;
    }

    uint32_t passed = 0, total = 0;
    for (int d = 0; d < nDirs; d++) {
        print("  ", WHITE);
        print((char *)dirs[d].label, CYAN);
        print(": ", WHITE);
        total++;
        if (a3kXferRun(target, &dirs[d], ref, &fail)) {
            print("PASSED\n", GREEN);
            passed++;
        } else {
            a3kXferPrintFail(&fail);
            if (dirs[d].failHint) {
                print((char *)dirs[d].failHint, YELLOW);
                print("\n", YELLOW);
            }
        }
    }

    print("\nResult: ", WHITE);
    print(binDec((int32_t)passed), passed == total ? GREEN : RED);
    print(" / ", WHITE);
    print(binDec((int32_t)total), WHITE);
    print(" checks passed\n", WHITE);

    a3k_scsi_irq_disable();
    return (int)passed;
}

// Soak version — same persistent-tallies/last-fail/abort shape as the other
// repeat modes. Each displayed pass runs every direction A3K_XFERSOAK_REPS
// times; every rep is a full reset+select+READ(10) (tens of ms each), so a
// pass is a meaningful soak unit without the screen redrawing constantly.
// Abort is polled between reps — each rep is internally bounded by the
// phase-machinery timeouts, so ESC response stays sub-second even on
// hardware that's hanging every transfer.
#define A3K_XFERSOAK_REPS 5

// Consecutive completed passes in which EVERY rep of EVERY row failed with
// a command timeout before the soak declares the bus wedged and halts
// itself. Once a target wedges (holding the bus after an abandoned
// transaction the drain couldn't finish — cdh's overnight BFG9060 run),
// nothing ever passes again and further passes are pure noise that also
// scrolls the onset forensics away; 3 passes = 15*nDirs straight timeouts,
// beyond any transient flake, reached within seconds of the event.
#define A3K_XFERSOAK_WEDGE_PASSES 3

static int a3kXferTestRepeat(void)
{
    A3kXferDir dirs[A3K_XFER_MAX_DIRS];
    A3kXferFail fail;
    uint32_t dirFails[A3K_XFER_MAX_DIRS] = { 0, 0, 0 };
    uint8_t *ref;
    int nDirs = 0;

    print("\002A3000/A3000T SCSI - DMA Transfer Test (Repeat)\n", WHITE);
    print("\002EXPERIMENTAL - please report results\n\n", YELLOW);
    a3k_scsi_irq_enable(a3k_globals());
    globals->HddEscLatch = 0;   // stale-latch guard; an ESC during the setup
                                // below still latches and aborts at pass 1
    int target = a3kXferSetup(dirs, &nDirs, &ref);
    if (target < 0) {
        a3k_scsi_irq_disable();
        return 0;
    }

    uint32_t iterations = 0, totalPassed = 0, totalChecks = 0;
    A3kXferFail firstFail, lastFail;
    uint32_t firstFailPass = 0, lastFailPass = 0;
    int firstFailDir = 0, lastFailDir = 0, haveFail = 0, firstDrawn = 0;
    int aborted = 0, wedgeStreak = 0, wedged = 0;

    // Draw-once frame with in-place number updates, same shape as
    // a4kDmaTestRepeat() — see the layout comment there. NO clearScreen():
    // the frame starts BELOW the setup's buffer-address/target output, so
    // that stays visible for the whole soak (user request); row layout is
    // read off the live cursor. The tall block is the FIRST fail's full
    // forensics — drawn once when it happens and never blanked or redrawn,
    // so the onset evidence survives an overnight soak: worst case
    // (MISMATCH) is the header+FAILED line (long enough to wrap onto a
    // second row) + lanes/poison/dump line + engine line + exp and got
    // hex-dump lines + direction hint = 7 rows. Below it, later fails get
    // a ONE-row "Last:" summary via a3kXferPrintTerse() (blanked before
    // each redraw; guaranteed not to wrap). On NTSC (25 rows) that summary
    // row IS roughly the bottom row — which is why it never prints a
    // newline; PAL (32 rows) has slack regardless.
    print("\nESC or both buttons: stop\n\n", WHITE);
    uint32_t passRow   = *(volatile uint8_t *)&globals->Ypos;   // row lives in the MSB byte (see setPos)
    uint32_t totalsRow = passRow + (uint32_t)nDirs + 2;
    uint32_t failRow   = totalsRow + 1;
    uint32_t lastRow   = failRow + 7;

    while (!aborted && !wedged) {
        setPos(0, passRow);
        print("Pass ", WHITE);   // target ID already on screen from the setup output above
        print(binDec((int32_t)(iterations + 1)), CYAN);
        print(" (", WHITE);
        print(binDec(A3K_XFERSOAK_REPS), CYAN);
        print(" reps per row):", WHITE);

        int newFail = 0;
        int allCmdTimeouts = 1;   // whole pass = nothing but CMD timeouts?
        for (int d = 0; d < nDirs && !aborted; d++) {
            uint32_t passedReps = 0;

            for (int rep = 0; rep < A3K_XFERSOAK_REPS; rep++) {
                if (a3kXferRun(target, &dirs[d], ref, &fail)) {
                    passedReps++;
                    allCmdTimeouts = 0;
                } else {
                    if (fail.reason != A3K_XFERFAIL_CMD)
                        allCmdTimeouts = 0;
                    dirFails[d]++;
                    lastFail     = fail;
                    lastFailPass = iterations + 1;
                    lastFailDir  = d;
                    if (!haveFail) {
                        firstFail     = fail;
                        firstFailPass = iterations + 1;
                        firstFailDir  = d;
                    }
                    haveFail     = 1;
                    newFail      = 1;
                }
                totalChecks++;
                if (a4kDmaAbortRequested()) {
                    aborted = 1;
                    break;
                }
            }
            totalPassed += passedReps;

            setPos(0, passRow + 1 + (uint32_t)d);
            print("  ", WHITE);
            print((char *)dirs[d].label, CYAN);
            print(": ", WHITE);
            print(binDec((int32_t)passedReps),
                  passedReps == A3K_XFERSOAK_REPS ? GREEN : RED);
            print("/", WHITE);
            print(binDec(A3K_XFERSOAK_REPS), WHITE);
            print("   fails so far: ", WHITE);
            print(binDec((int32_t)dirFails[d]), dirFails[d] ? RED : GREEN);
        }
        iterations++;

        // Wedge watch — only COMPLETED passes count (an ESC mid-pass must
        // not add a short pass to the streak).
        if (!aborted) {
            wedgeStreak = allCmdTimeouts ? wedgeStreak + 1 : 0;
            if (wedgeStreak >= A3K_XFERSOAK_WEDGE_PASSES)
                wedged = 1;
        }

        setPos(0, totalsRow);
        print("Running totals over ", WHITE);
        print(binDec((int32_t)iterations), CYAN);
        print(" pass(es): ", WHITE);
        print(binDec((int32_t)totalPassed), totalPassed == totalChecks ? GREEN : YELLOW);
        print(" / ", WHITE);
        print(binDec((int32_t)totalChecks), WHITE);
        print(" reads passed", WHITE);

        if (newFail) {
            if (!firstDrawn) {
                // Immutable onset block — the rows below it were never
                // written, so nothing needs blanking, now or ever.
                setPos(0, failRow);
                print("First fail: pass ", WHITE);
                print(binDec((int32_t)firstFailPass), CYAN);
                print("  ", WHITE);
                print((char *)dirs[firstFailDir].label, CYAN);
                print(": ", WHITE);
                a3kXferPrintFail(&firstFail);
                if (dirs[firstFailDir].failHint) {
                    print((char *)dirs[firstFailDir].failHint, YELLOW);
                }
                firstDrawn = 1;
            }
            soakBlankRow(lastRow);
            setPos(0, lastRow);
            print("Last: pass ", WHITE);
            print(binDec((int32_t)lastFailPass), CYAN);
            print("  ", WHITE);
            print((char *)dirs[lastFailDir].label, CYAN);
            print(": ", WHITE);
            a3kXferPrintTerse(&lastFail);
        }
    }

    a3k_scsi_irq_disable();

    if (wedged) {
        // The frame is done updating, so sequential prints (and the scroll
        // they cause on NTSC, where lastRow is the bottom row) are safe
        // now. Unlike the ESC path — which consumed its own dismissal
        // keypress — a self-halt needs an explicit WaitButton() or the
        // caller's initScreen() would wipe this verdict instantly.
        setPos(0, lastRow);
        print("\n\n", WHITE);
        print("Bus wedged: every command in ", RED);
        print(binDec(A3K_XFERSOAK_WEDGE_PASSES), RED);
        print(" consecutive passes timed out.\n", RED);
        print("Soak halted at pass ", WHITE);
        print(binDec((int32_t)iterations), CYAN);
        print(" (first fail: pass ", WHITE);
        print(binDec((int32_t)firstFailPass), CYAN);
        print(").\n", WHITE);
        print("If Scan Devices now finds no IDs, the target is holding the bus:\n", YELLOW);
        print("a warm reboot may not release it - power the machine off and on.\n", YELLOW);
        print("\nPress any key/button to continue", WHITE);
        WaitButton();
    }

    return (int)totalPassed;
}

// ---- stub functions (hardware not yet implemented) -------------------------

// A2091 SCSI and GVP SCSI removed from this list 2026-07-15 - deferred,
// not implemented yet, no stub entries cluttering the menu in the meantime
// (matches this project's usual approach of not shipping visible menu
// options for genuinely unimplemented hardware). Re-add when real
// detect/scan/identify/smart implementations exist for them.
static const HddController hddControllers[] = {
    { "A1200/A600 IDE",    0, HDD_GATE_NONE,    detectGayleIDE,   scanGayleIDE,   identifyGayleIDE,   smartGayleIDE,   NULL,       NULL,             NULL,        NULL               },
    { "A4000/A4000T IDE",  1, HDD_GATE_A4000_IDE, detectA4000IDE,   scanA4000IDE,   identifyA4000IDE,   smartA4000IDE,   NULL,       NULL,             NULL,        NULL               },
    { "A3000/A3000T SCSI", 1, HDD_GATE_NOT_NCR, detectA3000SCSI,  scanA3000SCSI,  identifyA3000SCSI,  smartA3000SCSI,  a3kDmaTest, a3kDmaTestRepeat, a3kXferTest, a3kXferTestRepeat  },
    { "A4000T SCSI",       1, HDD_GATE_NCR,     detectA4000TSCSI, scanA4000TSCSI, identifyA4000TSCSI, smartA4000TSCSI, a4kDmaTest, a4kDmaTestRepeat, NULL,        NULL               },
};
#define NUM_HDD_CONTROLLERS (int)(sizeof(hddControllers)/sizeof(hddControllers[0]))

// A600/A1200 hard-stall guard for every dispatch path, not just detect():
// selection marks an absent controller RED but still activates it, so the
// user could still press Scan/Identify/DMA with a big-box controller active
// on a Gayle machine — which would touch the unterminated $DD0000 range and
// freeze the machine exactly like the original selection hang this guards
// against. One chokepoint here covers every menu operation. Prints its own
// explanation when blocking so the caller only needs the boolean.
static int hddBlockedOnThisMachine(int idx)
{
    if (hddControllers[idx].bigBoxDDBus && isGayleMachine()) {
        print("\nNot possible on this machine: A600/A1200 Gayle detected.\n", RED);
        print("This controller's registers ($DD0000 range) are not decoded on\n", YELLOW);
        print("Gayle machines - accessing them would freeze this Amiga.\n", YELLOW);
        return 1;
    }
    if (hddControllers[idx].gate == HDD_GATE_A4000_IDE) {
        if (!isAgaMachine()) {
            print("\nNot possible on this machine: no AGA chipset (Lisa) found.\n", RED);
            print("This controller exists only in AGA machines (A4000/A4000T).\n", YELLOW);
            return 1;
        }
        if (detectA3000SCSI()) {
            print("\nNot possible on this machine: SDMAC/WD33C93 SCSI found at $DD0000.\n", RED);
            print("A3000-family machines (incl. AA3000+) have no motherboard IDE - the\n", YELLOW);
            print("$DD2020 range answers via the SDMAC and IDE ops would scribble it.\n", YELLOW);
            return 1;
        }
    }
    if (hddControllers[idx].gate == HDD_GATE_NCR && !ncr710Present()) {
        print("\nNot possible on this machine: no NCR 53C710 answers at $DD0040.\n", RED);
        print("On A3000-family machines (incl. AA3000+) that range is the SDMAC -\n", YELLOW);
        print("running A4000T SCSI ops on it wedges the machine's real SCSI.\n", YELLOW);
        return 1;
    }
    if (hddControllers[idx].gate == HDD_GATE_NOT_NCR && ncr710Present()) {
        print("\nNot possible on this machine: an NCR 53C710 answers at $DD0040.\n", RED);
        print("This is an A4000T - its SCSI range holds the NCR chip, and A3000\n", YELLOW);
        print("SCSI ops on it would wedge the machine's real SCSI.\n", YELLOW);
        return 1;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// Menu 0 — main HDD test
// ---------------------------------------------------------------------------

static const char HDDMenuText[]  = "\002HDD Controller Test";
static const char HDDMenu1[]     = "1 - Active Controller:";
static const char HDDMenu2[]     = "2 - Scan Devices";
static const char HDDMenu3[]     = "3 - Identify Devices";
static const char HDDMenu4[]     = "4 - DMA Test";
static const char HDDMenu5[]     = "5 - DMA Test (Repeat)";
static const char HDDMenu6[]     = "6 - DMA Transfer Test";
static const char HDDMenu7[]     = "7 - DMA Transfer Test (Repeat)";
static const char HDDMenuBack[]  = "9 - Main Menu";

// SMART data ('5') was removed as a top-level menu entry — it's now reached
// via 'S' from inside Identify Devices, per-unit, instead of a separate
// whole-controller pass. "Autodetect" was removed 2026-07-15 (never worked
// reliably - user confirmed) — Active Controller is set via the "Select
// Controller" submenu (MenuNumber 1) instead.
// '4'/'5' (DMA Test / DMA Test Repeat) exist for A4000T SCSI (mem-to-mem
// engine test) and A3000 SCSI (SDMAC/Ramsey register test — the closest
// possible equivalent, see a3kDmaTest()); NULL for the IDE controllers.
// '6'/'7' (DMA Transfer Test) is A3000-only for now: a real DMA-vs-PIO read
// compare needing a live SCSI target (see a3kXferTest()). All entries are
// shown for all controllers anyway, matching '2'/'3''s own "Not implemented
// for this controller." fallback rather than hiding/renumbering the menu per
// active controller.
static const char *HDDMenuItems[] = {
    HDDMenuText,
    HDDMenu1, HDDMenu2, HDDMenu3, HDDMenu4, HDDMenu5, HDDMenu6, HDDMenu7, HDDMenuBack,
    NULL
};

// ---------------------------------------------------------------------------
// Menu 1 — controller select list
// ---------------------------------------------------------------------------

static const char HDDSelText[]  = "\002Select Controller";
static const char HDDSel1[]     = "1 - A1200/A600 IDE";
static const char HDDSel2[]     = "2 - A4000/A4000T IDE";
static const char HDDSel3[]     = "3 - A3000/A3000T SCSI";
static const char HDDSel4[]     = "4 - A4000T SCSI";
static const char HDDSelBack[]  = "9 - Back";

static const char *HDDSelItems[] = {
    HDDSelText,
    HDDSel1, HDDSel2, HDDSel3, HDDSel4, HDDSelBack,
    NULL
};

static const char **HDDTestMenu[] = {
    HDDMenuItems,   // MenuNumber 0
    HDDSelItems,    // MenuNumber 1
};

// ---------------------------------------------------------------------------

void HDDTestC()
{
    int activeController = 0;
    MenuVar hddMenuVars[8] = {{0}};   // one slot per menu-0 item (printMenu indexes vars[i] per item)

    hddMenuVars[0].str   = (char *)hddControllers[0].name;
    hddMenuVars[0].color = WHITE;

    initScreen();

    globals->Menu          = (void *)HDDTestMenu;
    globals->MenuVariable  = (void *)hddMenuVars;
    globals->MenuNumber    = 0;
    globals->PrintMenuFlag = 1;

    for (;;) {
        printMenu();
        getInput();

        uint8_t ch = globals->GetCharData;
        uint8_t mn = globals->MenuNumber;

        if (mn == 0) {
            // ---- main HDD menu ----
            if (globals->LMB || globals->RMB || ch == 0x0a) {
                static const uint8_t posToKey[] = { '1','2','3','4','5','6','7','9' };
                if (globals->MenuPos < (uint8_t)sizeof(posToKey))
                    ch = posToKey[globals->MenuPos];
            }
            int handled = 1;
            switch (ch) {
                case '1':
                    waitReleased();
                    initScreen();
                    globals->MenuVariable  = NULL;
                    globals->MenuNumber    = 1;
                    globals->PrintMenuFlag = 1;
                    break;

                case '2':
                    waitReleased();
                    clearScreen();
                    if (hddBlockedOnThisMachine(activeController))
                        ;   // guard printed its own message
                    else if (hddControllers[activeController].scan)
                        hddControllers[activeController].scan();
                    else
                        print("\nNot implemented for this controller.\n", RED);
                    print("\nPress any key/button to continue", WHITE);
                    WaitButton();
                    initScreen();
                    globals->PrintMenuFlag = 1;
                    break;

                case '3': {
                    waitReleased();
                    clearScreen();
                    // A return of 1 means the controller's own identify() is
                    // fully interactive and already handled its own
                    // dismissal (e.g. identifyA3000SCSI()'s unit browser) —
                    // don't also show the generic one-shot prompt below.
                    int selfDismissed = 0;
                    if (hddBlockedOnThisMachine(activeController))
                        ;   // guard printed its own message
                    else if (hddControllers[activeController].identify)
                        selfDismissed = hddControllers[activeController].identify();
                    else
                        print("\nNot implemented for this controller.\n", RED);
                    if (!selfDismissed) {
                        print("\nPress any key/button to continue", WHITE);
                        WaitButton();
                    }
                    initScreen();
                    globals->PrintMenuFlag = 1;
                    break;
                }

                case '4':
                    waitReleased();
                    clearScreen();
                    if (hddBlockedOnThisMachine(activeController))
                        ;   // guard printed its own message
                    else if (hddControllers[activeController].dma)
                        hddControllers[activeController].dma();
                    else
                        print("\nNot implemented for this controller.\n", RED);
                    print("\nPress any key/button to continue", WHITE);
                    WaitButton();
                    initScreen();
                    globals->PrintMenuFlag = 1;
                    break;

                case '5':
                    waitReleased();
                    clearScreen();
                    // dmaRepeat() runs its own loop and already consumes the
                    // ESC/both-buttons that stopped it (see a4kDmaTestRepeat()),
                    // same self-dismissing convention as case '3''s identify() —
                    // no extra "press any key" prompt needed on the way out.
                    if (hddBlockedOnThisMachine(activeController)) {
                        print("\nPress any key/button to continue", WHITE);
                        WaitButton();
                    } else if (hddControllers[activeController].dmaRepeat) {
                        hddControllers[activeController].dmaRepeat();
                    } else {
                        print("\nNot implemented for this controller.\n", RED);
                        print("\nPress any key/button to continue", WHITE);
                        WaitButton();
                    }
                    initScreen();
                    globals->PrintMenuFlag = 1;
                    break;

                case '6':
                    waitReleased();
                    clearScreen();
                    if (hddBlockedOnThisMachine(activeController))
                        ;   // guard printed its own message
                    else if (hddControllers[activeController].xfer)
                        hddControllers[activeController].xfer();
                    else
                        print("\nNot implemented for this controller.\n", RED);
                    print("\nPress any key/button to continue", WHITE);
                    WaitButton();
                    initScreen();
                    globals->PrintMenuFlag = 1;
                    break;

                case '7':
                    waitReleased();
                    clearScreen();
                    // xferRepeat() self-dismisses like dmaRepeat()/identify()
                    // — its loop already consumed the ESC/both-buttons.
                    if (hddBlockedOnThisMachine(activeController)) {
                        print("\nPress any key/button to continue", WHITE);
                        WaitButton();
                    } else if (hddControllers[activeController].xferRepeat) {
                        hddControllers[activeController].xferRepeat();
                    } else {
                        print("\nNot implemented for this controller.\n", RED);
                        print("\nPress any key/button to continue", WHITE);
                        WaitButton();
                    }
                    initScreen();
                    globals->PrintMenuFlag = 1;
                    break;

                case '9':
                    waitReleased();
                    GOTO_MAINMENU();

                default:
                    handled = 0;
                    break;
            }
            (void)handled;

        } else {
            // ---- controller select menu (MenuNumber 1) ----
            if (globals->LMB || globals->RMB || ch == 0x0a) {
                static const uint8_t posToKey[] = { '1','2','3','4','9' };
                if (globals->MenuPos < (uint8_t)sizeof(posToKey))
                    ch = posToKey[globals->MenuPos];
            }
            switch (ch) {
                case '1': case '2': case '3': case '4': {
                    waitReleased();
                    int idx = ch - '1';
                    if (idx < NUM_HDD_CONTROLLERS) {
                        activeController = idx;
                        int found = hddControllers[idx].detect &&
                                    hddControllers[idx].detect();
                        hddMenuVars[0].str   = (char *)hddControllers[idx].name;
                        hddMenuVars[0].color = found ? GREEN : RED;
                    }
                    initScreen();
                    globals->MenuVariable  = (void *)hddMenuVars;
                    globals->MenuNumber    = 0;
                    globals->PrintMenuFlag = 1;
                    break;
                }
                case '9':
                    waitReleased();
                    initScreen();
                    globals->MenuVariable  = (void *)hddMenuVars;
                    globals->MenuNumber    = 0;
                    globals->PrintMenuFlag = 1;
                    break;
                default:
                    break;
            }
        }
    }
}

#else  /* TARGET_DEMON */

// Stubs for the excluded HDD half (see the banner where the #ifndef opens).
// The menu entry stays selectable but explains itself instead of vanishing.
void HDDTestC(void)
{
    initScreen();
    print("\002HDD Controller Test\n\n", CYAN);
    print("Not included in the DeMoN cartridge build: no supported HDD\n", YELLOW);
    print("controller (Gayle/A3000/A4000 class) can exist on this machine.\n", YELLOW);
    print("\nPress any key/button to return.\n", WHITE);
    WaitButton();
    initScreen();
    globals->PrintMenuFlag = 1;
}

// autoconfig.c's Z3-space gate leg: an A500-class cartridge host never has
// Gayle (its 68000 fails autoconfig's 24-bit gate first anyway).
int isGayleMachine(void)
{
    return 0;
}

#endif /* TARGET_DEMON */

void crap()
{
    initScreen();
    print("\002Floppytest (New Experimental)\n\n",CYAN);
       print("This test will run much better with working CIA Timer.\nIs CIA Timers tested ok: ",CYAN);
       if(globals->ODDCIATIMEROK==1)
       {
              print("ODD CIA OK ",GREEN);
       }
       else
       {
              print("ODD CIA NOT TESTED/NOT OK ",RED);
       }

       print(" | ",WHITE);

       if(globals->EVENCIATIMEROK==1)
       {
              print("EVEN CIA OK ",GREEN);
       }
       else
       {
              print("EVEN CIA NOT TESTED/NOT OK ",RED);
       }
    CIAB->ciaprb &= ~CIAF_DSKSEL0;
    CIAB->ciaprb |=  CIAF_DSKSEL0;

    uint32_t id = 0;
    uint8_t mask = CIAF_DSKSEL0;
    unsigned int i;

    CIAB->ciaprb |= 0xf8;  /* motor-off, deselect all */
    CIAB->ciaprb &= 0x7f;  /* 1. MTRXD low */
    CIAB->ciaprb &= ~mask; /* 2. SELxB low */
    CIAB->ciaprb |= mask;  /* 3. SELxB high */
    CIAB->ciaprb |= 0x80;  /* 4. MTRXD high */
    CIAB->ciaprb &= ~mask; /* 5. SELxB low */
    CIAB->ciaprb |= mask;  /* 6. SELxB high */
    for (i = 0; i < 32; i++) {
        CIAB->ciaprb &= ~mask; /* 7. SELxB low */
        id = (id<<1) | ((CIAA->ciapra>>5)&1); /* 8. Read and save state of RDY */
        CIAB->ciaprb |= mask;  /* 9. SELxB high */
    }

    print("Drive ID: ",WHITE);
    print(binHex(id),WHITE);

    putChar('a',RED,10,10);
    putChar('a',R_RED,11,10);

    putChar('a',CYAN,20,10);
    putChar('a',R_CYAN,21,10);
    PAUSEC();
//    scrollScreen();
    print("hejsan",WHITE);

       print("\n\nDONE. Press any key/button to exit",WHITE);
       do
       {

              GetInput();
       }
             while(globals->BUTTON == 0);
}


static void drawFloppyTestLabels(void)
{
    setPos(6,2);   print("Track:",YELLOW);
    setPos(18,2);  print("Side:",YELLOW);
    setPos(32,2);  print("Motor:",YELLOW);
    setPos(45,2);  print("WProtection:",YELLOW);
    setPos(64,2);  print("Disk:",YELLOW);
    setPos(6,3);   print("Ready:",YELLOW);
    setPos(18,3);  print("Track0:",YELLOW);
    setPos(32,3);  print("$bfe001:",YELLOW);
    setPos(52,3);  print("$bdf100:",YELLOW);
    setPos(42,5);  print("Drive ID: ",WHITE);
}

/* Clear the screen and force the floppy-test display to fully redraw on the
 * next loop iteration (used after option 8 / 'S' which paint over it). */
static void restoreFloppyTestScreen(int *driveMotor, int *driveSide, int *track0,
                                    int *oldbfe001, int *oldbfd100,
                                    int *ready, int *wprot, int *disk, int *oldTrack)
{
    initScreen();
    drawFloppyTestLabels();
    setPos(52, 5);
    print(binHex(getFloppyID()), GREEN);
    *driveMotor = -1; *driveSide = -1; *track0 = -1;
    *oldbfe001  = 0;  *oldbfd100 = 0;
    *ready      = -1; *wprot     = -1; *disk = -1;
    *oldTrack   = -1;
    globals->PrintMenuFlag = 1;
}

/* 'S' option: show a sector from the existing track buffer (no DMA read).
 * Prompts for the sector to display so you can browse what was last read. */
static void showBufferSector(void)
{
    setPos(0, 27);
    print("Sector to show (0-10): ", WHITE);
    int32_t s = inputDecNum(globals->sector);
    if (s < 0) return;
    if (s > 10) s = 10;
    globals->sector = (uint8_t)s;

    initScreen();
    setPos(0, 0);
    print("Showing buffer contents for sector ", WHITE);
    print(binDec(globals->sector), YELLOW);

    uint8_t *data = 0;
    int found = findSectorInTrack(globals->sector, &data);
    if (found < 0) {
        setPos(0, 2);
        print("Sector not present in buffer (no $4489 match - read a track first?)", RED);
        setPos(0, 27);
        print("Press any key/mouse to continue", WHITE);
        WaitButton();
        return;
    }

    setPos(0, 2);
    print("Sector ", WHITE);
    print(binDec(found), GREEN);
    print(" found in buffer.", WHITE);

    setPos(0, 5);
    print("Decoded sector data (first 320 bytes):", YELLOW);

    decodeAndShowSector(data);

    setPos(0, 28);
    print("Press any key/mouse to continue", WHITE);
    WaitButton();
}

/* Scan trackbuff for an Amiga $4489 sync, decode the info long that follows,
 * return the byte-offset of the data block (sync+56 bytes) when sector matches.
 * Returns -1 on miss. */
static int findSectorInTrack(uint8_t wanted, uint8_t **dataOut)
{
    uint16_t *w      = (uint16_t *)globals->trackbuff;
    uint8_t  *bufEnd = (uint8_t  *)globals->trackbuff + 12980;

    while ((uint8_t *)(w + 1) <= bufEnd) {
        if (*w++ != 0x4489) continue;
        if ((uint8_t *)w + 8 > bufEnd) return -1;
        uint32_t *lp   = (uint32_t *)w;
        uint32_t info  = ((lp[0] & 0x55555555u) << 1) | (lp[1] & 0x55555555u);
        uint8_t  sec   = (info >> 8) & 0xffu;       /* info byte 2 = sector */
        if (sec == wanted) {
            if ((uint8_t *)w + 56 + 1024 > bufEnd) return -1;
            *dataOut = (uint8_t *)w + 56;            /* skip info+label+chksums */
            return sec;
        }
    }
    return -1;
}

/* MFM-decode and show 320 bytes (20 rows × 16 bytes) of the sector data,
 * hex on the left, ASCII on the right (non-printable → '.'). */
static void decodeAndShowSector(uint8_t *dataPtr)
{
    for (int row = 0; row < 20; row++) {
        setPos(0, 7 + row);
        print(binHex((uint32_t)(row * 16)), CYAN);
        printChar(':', WHITE);
        printChar(' ', WHITE);
        uint32_t *odd = (uint32_t *)(dataPtr + row * 16);
        uint32_t *evn = (uint32_t *)(dataPtr + row * 16 + 0x200);
        uint8_t bytes[16];
        for (int i = 0; i < 4; i++) {
            uint32_t v = ((odd[i] & 0x55555555u) << 1) | (evn[i] & 0x55555555u);
            print(binHex(v), GREEN);
            printChar(' ', WHITE);
            bytes[i*4    ] = (uint8_t)(v >> 24);
            bytes[i*4 + 1] = (uint8_t)(v >> 16);
            bytes[i*4 + 2] = (uint8_t)(v >>  8);
            bytes[i*4 + 3] = (uint8_t)v;
        }
        printChar('|', WHITE);
        for (int i = 0; i < 16; i++) {
            uint8_t b = bytes[i];
            printChar((b >= 0x20 && b < 0x7f) ? b : '.', YELLOW);
        }
        printChar('|', WHITE);
    }
}

/* --- Drive control helpers (1:1 with asm equivalents) ------------------ */
static void diskSelect(uint8_t selBit, uint8_t selMask)        /* .SelectDrive */
{
    CIAB->ciaprb |= selMask;            /* unselect all */
    waitLong();
    CIAB->ciaprb &= ~selBit;            /* select target */
    waitLong();
}

static void diskUnselect(uint8_t selMask)                       /* .UnSelectDrive */
{
    CIAB->ciaprb |= selMask;
    waitLong();
}

static void diskMotorOn(uint8_t selBit, uint8_t selMask)        /* .MotorOn */
{
    diskSelect(selBit, selMask);
    waitLong();
    CIAB->ciaprb &= ~CIAF_DSKMOTOR;     /* motor bit low = on */
    waitLong();
    CIAB->ciaprb &= ~selBit;            /* redundant bclr SEL (asm does it too) */
}

static void diskMotorOff(uint8_t selBit, uint8_t selMask)       /* .MotorOff */
{
    diskSelect(selBit, selMask);
    CIAB->ciaprb |= selMask;            /* deselect all again */
    waitLong();
    CIAB->ciaprb |= CIAF_DSKMOTOR;      /* motor bit high = off */
    waitLong();
    CIAB->ciaprb &= ~selBit;            /* re-select to latch motor-off */
    diskUnselect(selMask);
}

/* Matches asm .WaitReady: spins while /DSKRDY is asserted (bit clear),
 * exits on the next index pulse or timeout. The asm relies on this loop as a
 * coarse settle delay between motor-on and DMA. */
static void diskWaitReady(void)
{
    uint32_t to = 0xffff;
    while (to--) {
        (void)CIAA->ciapra;             /* dummy read, asm does this */
        if (CIAA->ciapra & CIAF_DSKRDY) break;
    }
}

/* Step the head one track out, then back in. Mirrors stepToTrack() — the
 * single-write select, several settle waitLongs after the step, then full
 * deselect. On FS-UAE (and real drives) this is what actually clears /CHNG
 * and gets the disk emulation streaming data. Head returns to original
 * cylinder, globals->TrackNo unchanged. */
static void diskClearChange(uint8_t selBit, uint8_t selMask)
{
    /* Step OUT one track (DIR=0). */
    CIAB->ciaprb = (CIAB->ciaprb | selMask) & ~selBit;
    CIAB->ciaprb &= ~CIAF_DSKDIREC;
    (void)*(volatile uint16_t *)0xdff1fe;
    CIAB->ciaprb &= ~CIAF_DSKSTEP;
    (void)*(volatile uint16_t *)0xdff1fe;
    CIAB->ciaprb |=  CIAF_DSKSTEP;
    waitLong();
    CIAB->ciaprb |= selMask;
    waitLong(); waitLong(); waitLong(); waitLong();

    /* Step IN one track (DIR=1) — back to original cylinder. */
    CIAB->ciaprb = (CIAB->ciaprb | selMask) & ~selBit;
    CIAB->ciaprb |=  CIAF_DSKDIREC;
    (void)*(volatile uint16_t *)0xdff1fe;
    CIAB->ciaprb &= ~CIAF_DSKSTEP;
    (void)*(volatile uint16_t *)0xdff1fe;
    CIAB->ciaprb |=  CIAF_DSKSTEP;
    waitLong();
    CIAB->ciaprb |= selMask;
    waitLong(); waitLong(); waitLong(); waitLong();
}

/* One full select → motor on → DMA read → motor off cycle. The buffer
 * (globals->trackbuff) ends up with whatever MFM data the drive streamed
 * for the cylinder the head is currently parked on. Returns the intreqr
 * value seen at end of DMA wait (bit 1 = DSKBLK fired). */
static uint16_t diskDmaReadTrack(uint32_t floppyID)
{
    uint8_t selBit  = 1 << (floppyID + 3);
    uint8_t selMask = CIAF_DSKSEL0 | CIAF_DSKSEL1 | CIAF_DSKSEL2 | CIAF_DSKSEL3;

    diskSelect(selBit, selMask);
    waitLong();
    diskMotorOn(selBit, selMask);
    diskWaitReady();
    diskSelect(selBit, selMask);

    custom->dsklen  = 0x4000;
    *(volatile uint32_t *)0xdff020 = (uint32_t)(uintptr_t)globals->trackbuff;
    custom->dsksync = 0x4489;
    custom->adkcon  = 0x7f00;
    custom->adkcon  = 0x9500;
    custom->intreq  = 0x0002;
    custom->dmacon  = 0x8210;
    uint16_t dsklen = 0x8000 | 0x1900;
    custom->dsklen  = dsklen;
    custom->dsklen  = dsklen;

    uint32_t spin = 0xffffff;
    while (spin--) {
        (void)CIAA->ciapra;
        if (custom->intreqr & 0x0002) break;
    }
    uint16_t intreqrAfter = custom->intreqr;

    custom->dsklen = 0x4000;
    waitLong();
    diskMotorOff(selBit, selMask);
    waitLong();
    diskUnselect(selMask);
    return intreqrAfter;
}

static void readTrackBuffer(uint32_t floppyID)
{
    /* Prompt for sector number — default = last chosen value. */
    setPos(0, 27);
    print("Sector to read (0-10): ", WHITE);
    int32_t s = inputDecNum(globals->sector);
    if (s < 0) return;            /* ESC aborts */
    if (s > 10) s = 10;
    globals->sector = (uint8_t)s;

    initScreen();
    setPos(0, 0);
    print("Reading track ", WHITE);
    print(binDec(globals->TrackNo), YELLOW);
    print(", looking for sector ", WHITE);
    print(binDec(globals->sector), YELLOW);
    print("...", WHITE);

    /* Wipe buffer so a silently-failed DMA can't masquerade as success
     * by leaving stale sector data from a previous read. */
    {
        volatile uint8_t *p = (volatile uint8_t *)globals->trackbuff;
        for (int i = 0; i < 12980; i++) p[i] = 0;
    }

    uint16_t intreqr = diskDmaReadTrack(floppyID);

    /* DSKBLK (bit 1) of intreqr fires when DMA completes. If it never set,
     * the buffer is whatever DMA managed to transfer before the spin loop
     * timed out — usually nothing useful. */
    setPos(0, 1);
    print("DMA intreqr=", WHITE);
    print(binHex(intreqr), (intreqr & 0x0002) ? GREEN : RED);
    if (!(intreqr & 0x0002)) {
        print("  DSKBLK never fired (DMA timeout)", RED);
    }

    uint8_t *data = 0;
    int found = findSectorInTrack(globals->sector, &data);
    if (found < 0) {
        setPos(0, 5);
        print("Error finding sector (possibly read error)", RED);
        setPos(0, 27);
        print("Press any key/mouse to continue", WHITE);
        WaitButton();
        return;
    }

    setPos(0, 5);
    print("Sector ", WHITE);
    print(binDec(found), GREEN);
    print(" of track ", WHITE);
    print(binDec(globals->TrackNo), GREEN);
    print(" found in buffer. Decoded data follows:", YELLOW);

    decodeAndShowSector(data);

    setPos(0, 28);
    print("Press any key/mouse to continue", WHITE);
    WaitButton();
}