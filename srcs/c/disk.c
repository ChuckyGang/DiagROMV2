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

// ---------------------------------------------------------------------------
// Known controllers — single source of truth for both menus
// ---------------------------------------------------------------------------

typedef struct {
    const char *name;
    int (*detect)(void);        // 0 = absent, 1 = present
    int (*scan)(void);          // probe devices on bus; returns count found
    int (*identify)(void);      // show device info; returns count found
    int (*smart)(void);         // show SMART data; returns count found
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
            initScreen();
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
            initScreen();
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
#define GAYLE_INT_REG       ((volatile uint8_t *)0xDA9000)
#define GAYLE_INT_IDE       0x80
#define GAYLE_INT_IDEENAB   0x10

// Status register bits
#define IDE_BSY     0x80
#define IDE_DRDY    0x40
#define IDE_DF      0x20
#define IDE_DSC     0x10   // Seek Complete
#define IDE_DRQ     0x08
#define IDE_ERR     0x01

#define IDE_DEV_MASTER  0xA0
#define IDE_DEV_SLAVE   0xB0

#define ATA_CMD_IDENTIFY  0xEC
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

// A4000: base $DD2020, stride 2
static const IdeRegs ideA4000Regs = {
    (volatile uint16_t *)0xDD2020,
    (volatile uint8_t  *)0xDD2022,
    (volatile uint8_t  *)0xDD2024,
    (volatile uint8_t  *)0xDD2026,
    (volatile uint8_t  *)0xDD2028,
    (volatile uint8_t  *)0xDD202A,
    (volatile uint8_t  *)0xDD202C,
    (volatile uint8_t  *)0xDD202E,
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

static int ideQuickModel(const IdeRegs *r, char *out)
{
    *r->status = ATA_CMD_IDENTIFY;
    uint8_t st = ideWaitDRQ(r);
    if (st == 0xFF || !(st & IDE_DRQ)) { out[0] = '\0'; return 0; }
    static uint8_t mbuf[512];
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

// Generic scan: master (DRDY or IDENTIFY fallback) + slave (canary), prints model in quotes
static int doScanIDE(const IdeRegs *r)
{
    int found = 0;
    uint8_t st;
    static char scanModel[42];

    print("Master: ", CYAN);
    st = ideSelectDrive(r, IDE_DEV_MASTER);
    if (st == 0xFF) {
        print("TIMEOUT\n", RED);
    } else if (st & IDE_DRDY) {
        print("FOUND  ", GREEN); printIdeStatus(st);
        print(" \"", WHITE); ideQuickModel(r, scanModel);
        print(scanModel, GREEN); print("\"\n", WHITE);
        found++;
    } else {
        /* DRDY not set but controller responded — try IDENTIFY (some emulators skip DRDY) */
        if (ideQuickModel(r, scanModel)) {
            print("FOUND  ", GREEN); printIdeStatus(st);
            print(" \"", WHITE); print(scanModel, GREEN); print("\"\n", WHITE);
            found++;
        } else {
            print("NOT FOUND\n", RED);
        }
    }

    print("Slave:  ", CYAN);
    st = ideSelectDrive(r, IDE_DEV_SLAVE);
    if (st == 0xFF) {
        print("TIMEOUT\n", RED);
    } else if (!ideDevPresent(r)) {
        print("NOT FOUND\n", RED);
    } else {
        print("FOUND  ", GREEN); printIdeStatus(st);
        print(" \"", WHITE); ideQuickModel(r, scanModel);
        print(scanModel, GREEN); print("\"\n", WHITE);
        found++;
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

    print("  INT reg:  $", WHITE);
    print(binHexByte(intReg), (intReg & GAYLE_INT_IDE) ? RED : CYAN);
    print("  [IDE irq:", WHITE);
    print((intReg & GAYLE_INT_IDE)     ? "PENDING " : "clear   ", (intReg & GAYLE_INT_IDE) ? RED : GREEN);
    print("enab:", WHITE);
    print((intReg & GAYLE_INT_IDEENAB) ? "yes" : "no", WHITE);
    print("]\n", WHITE);

    print("  IDE bus:  $", WHITE);
    print(binHexByte(ideSt), CYAN);
    print("  err: $", WHITE);
    print(binHexByte(ideErr), ideErr ? RED : CYAN);
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
    if (isMaster  && !(st & IDE_DRDY))  { print("  NOT PRESENT\n", YELLOW); return; }
    if (!isMaster && !ideDevPresent(r)) { print("  NOT PRESENT\n", YELLOW); return; }

    *r->status = ATA_CMD_IDENTIFY;
    st = ideWaitDRQ(r);
    if (st == 0xFF)      { print("  TIMEOUT\n", RED); return; }
    if (!(st & IDE_DRQ)) { print("  ERR ", RED); print(binHex(st), RED); print("\n", WHITE); return; }

    ideReadWords(r, idbuf, 256);

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
// A4000 IDE
// ---------------------------------------------------------------------------

static int detectA4000IDE(void)
{
    // No chip ID register — check if IDE bus responds
    return (*ideA4000Regs.status != 0xFF);
}

static int scanA4000IDE(void)
{
    uint8_t ideSt  = *ideA4000Regs.status;
    uint8_t ideErr = *ideA4000Regs.features;

    print("\nA4000 IDE Controller\n", WHITE);
    print("  Regs:    $DD2020  (stride 2)\n", WHITE);
    print("  IDE bus: $", WHITE);
    print(binHexByte(ideSt), ideSt == 0xFF ? RED : CYAN);
    print("  err: $", WHITE);
    print(binHexByte(ideErr), ideErr ? RED : CYAN);
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
#define WD_TIMEOUT_REG  0x02
#define WD_CDB1         0x03   // CDB bytes 0-11 at 0x03..0x0E
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

// WD33C93 commands. Real AmigaOS drivers never use the autonomous "Select
// and Transfer" command for actual I/O — they SELECT_WITH_ATN, then manually
// drive each SCSI phase with its own TRANSFER_INFO, reacting to whatever
// phase the target requests next via the status byte below.
#define WDCMD_RESET             0x00
#define WDCMD_ABORT             0x01
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
        uint8_t asr = a3k_wd_aux();
        if (asr == 0xFF) continue;
        if (!(asr & (WD_ASR_BSY | WD_ASR_CIP))) return;
    }

    // Still stuck after waiting — force it with an explicit ABORT.
    a3k_wd_write(WD_COMMAND, WDCMD_ABORT);
    t = 1500000UL;
    while (t--) {
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
    uint8_t st = a3k_scsi_select(unit);
    if (!((st & 0xF0) == 0x80 && (st & 0x0F) == WDPHASE_MSG_OUT)) return 0;

    uint8_t identify = 0x80;   // IDENTIFY, LUN 0, no DiscPriv — a scan has no reselect handling
    st = a3k_wd_do_phase_sbt(&identify, 0);
    if (!((st & 0x0F) == WDPHASE_CMD && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) return 0;

    st = a3k_wd_do_phase((uint8_t *)cdb, cdbLen, 0);
    if (!((st & 0x0F) == WDPHASE_DATA_IN && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) return 0;

    st = a3k_wd_do_phase(dataBuf, dataLen, 1);
    if (!((st & 0x0F) == WDPHASE_STATUS && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) return 0;

    uint8_t statusByte = 0xFF;
    st = a3k_wd_do_phase_sbt(&statusByte, 1);
    if (!((st & 0x0F) == WDPHASE_MSG_IN && ((st & 0xF0) == 0x10 || (st & 0xF0) == 0x80))) return 0;

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

static int detectA3000SCSI(void)
{
    *SDMAC_CNTR = SDMAC_CNTR_PDMD | SDMAC_CNTR_INTENA;   // SCSI mode + let status latch
    if (*WD_AUX_STATUS == 0xFF) return 0;   // bus float — nothing there
    // Issue RESET and check for completion interrupt
    a3k_wd_write(WD_OWN_ID, WD_OWN_ID_VAL);   // must precede RESET — clock divisor latches at reset
    a3k_wd_write(WD_COMMAND, WDCMD_RESET);
    uint32_t t = 2000000UL;
    while (--t)
        if (a3k_wd_aux() & WD_ASR_INT) {
            uint8_t st = a3k_wd_read(WD_SCSI_STATUS);
            return (st == WDSTS_RESET || st == WDSTS_RESET_AF);
        }
    return 0;
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

    // Request Advanced Features on this reset only to learn whether the
    // chip acknowledges them (at least a 33C93A) — the closest thing to a
    // software-readable chip version this part exposes.
    a3k_wd_write(WD_OWN_ID, WD_OWN_ID_VAL | WD_OWN_ID_EAF);
    a3k_wd_write(WD_COMMAND, WDCMD_RESET);
    uint8_t rst = a3k_wd_wait_int();
    print("WD33C93: ", WHITE);
    if (rst == WDSTS_RESET_AF)
        print("present (Advanced Features supported)\n", GREEN);
    else if (rst == WDSTS_RESET)
        print("present (plain reset only)\n", GREEN);
    else {
        print("not responding\n", RED);
        return 0;
    }
    a3k_wd_wait_ready();

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

static int smartA3000SCSI(void)
{
    print("SMART data: not yet implemented for this controller.\n", YELLOW);
    return 0;
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
static void a3kBrowseSmart(void *ctx, int unit)     { (void)ctx; (void)unit; smartA3000SCSI(); }
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

// Longword registers (dsa/dsp/dsps/etc.) live at these offsets from NCR_BASE.
// The real driver source (ncr.c WRITE_LONG macro) writes longwords via
// offset+0x80 instead, but its own comment says this is "not required, but
// better" — an optional '040 cache-coherency workaround exploiting a real
// motherboard address mirror, not a hardware requirement. Amiberry's
// emulation isn't guaranteed to replicate that specific mirror, so we write
// to the plain offset (same address reads use).
#define NCR_DSP      0x2C   // DMA SCRIPTS pointer — writing this starts execution

static inline void ncr_lwrite(uint8_t off, uint32_t val) {
    *(volatile uint32_t *)((uint8_t *)NCR_BASE + off) = val;
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
    NCR_BASE[NCR_SIEN] = NCR_SIEN_STO;
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
#define NCR_SCRIPT_INT        0x98000000UL   // class=10,opcode=INT(011)

// Every SCRIPTS "step" we run is exactly [[action]] followed by [[INT]] — our
// C code always regains control after one step, reads the chip's actual
// current phase, and decides what to run next (mirrors the phase-by-phase
// dispatch loop used for A3000's WD33C93, just driving SCRIPTS instead of
// simple command-register writes).
//
// `script` must point to a 4-longword buffer that's a real stack local in
// the caller — NEVER `static`/global: statics land in ROM in this
// freestanding build (past endofcode), and the 53C710 bus-masters this
// buffer directly, so a ROM address would have the chip executing whatever
// garbage the checksum tool's self-test pattern put there.

static void ncr_buildSelect(uint32_t *script, uint8_t target)
{
    // Real bug found by diffing against the leaked AmigaOS a4091 driver's
    // ncr710.h `struct io_inst` (op/id/io1/io2 byte layout, verbatim):
    //   UBYTE op;   // 01XXX00A  X=opcode, A=select_with_atn
    //   UBYTE id;   // 87654321  SCSI ID (bitmask, one bit per ID line)
    //   UBYTE io1;  // 00000CT0
    //   UBYTE io2;  // 0A00N000
    //   ULONG res;  // reserved — must be 0, NOT an address
    // The target-ID bitmask belongs in the SECOND byte (bits23-16), not the
    // low byte (bits7-0) — the low byte is io2 (ack/atn bus-line controls).
    // The previous code OR'd the bitmask into bits7-0 instead, leaving the
    // real ID byte 0x00 (selecting no SCSI ID line at all) while stomping
    // io2's ack/atn control bits with the bitmask value instead. It also
    // wrote a garbage self-referencing pointer into the reserved second
    // longword instead of 0. Both are now fixed to match the real struct.
    script[0] = NCR_SCRIPT_SELECT_ATN | ((1UL << target) << 16);
    script[1] = 0;   // reserved, per struct io_inst — must be 0
    script[2] = NCR_SCRIPT_INT;
    script[3] = 0;
}

static void ncr_buildMove(uint32_t *script, uint8_t phaseMci, uint8_t *buf, int count)
{
    script[0] = ((uint32_t)phaseMci << 24) | ((uint32_t)count & 0xFFFFFFUL);
    script[1] = (uint32_t)(uintptr_t)buf;
    script[2] = NCR_SCRIPT_INT;
    script[3] = 0;
}

// Same real-time-vs-CPU-speed lesson as A3K_WAIT_ITERS (see that comment,
// above the A3000 WD33C93 code): a raw instruction-count spin runs at wildly
// different real speeds depending on host/emulation speed, and a straight
// busy-loop also burns 100% CPU under emulation for no reason. waitShort()
// paces at ~640us/iteration via the video beam register, independent of CPU
// speed. ~500 * 640us =~ 320ms.
#define NCR_WAIT_ITERS 500

// Start the given script and wait for it to interrupt (either our own INT
// instruction, or a hardware error like select timeout — both raise one of
// ISTAT's pending bits). Returns the SSTAT0 byte read while clearing the
// interrupt (so the caller can check e.g. NCR_SSTAT0_STO), or 0xFF if our
// own poll timed out without the chip ever interrupting.
//
// CACHE CONTRACT: the caller must call DisableCache() before building the
// script buffer (ncr_buildSelect()/ncr_buildMove()) and keep it disabled
// through this call — those writes must bypass the cache entirely rather
// than relying on DisableCache()'s own cache-clear step to retroactively
// flush them (unclear semantics for a write-back cache; safer to never let
// them get cached in the first place). This function re-enables the cache
// itself immediately after the DSP write, before its wait loop — do NOT
// also call EnableCache() again after this returns. This split exists
// because waitShort()'s raster-exact-match poll can spin forever if the
// cache is off while it runs (prior investigation), which real-time pacing
// this loop via waitShort() now requires avoiding.
//
// IRQ RACE NOTE (same as A3000's wait functions): once ncr_irq_enable() is
// active, Ncr710PortsIRQ races this loop to NCR_SSTAT0/DSTAT and normally
// wins. Check globals->ScsiIrqPending first; fall back to direct ISTAT
// polling so this still works unchanged if the IRQ was never enabled.
static uint8_t ncr_runAndWait(uint32_t *script)
{
    ncr_lwrite(NCR_DSP, (uint32_t)(uintptr_t)&script[0]);
    EnableCache();
    volatile struct GlobalVars *globals = a3k_globals();
    for (int i = 0; i < NCR_WAIT_ITERS; i++) {
        if (globals->ScsiIrqPending) {
            globals->ScsiIrqPending = 0;
            return globals->ScsiIrqStatus;
        }
        uint8_t istat = NCR_BASE[NCR_ISTAT];
        if (istat != 0xFF && (istat & (NCR_ISTAT_SIP | NCR_ISTAT_DIP))) {
            // Reading these clears the pending bit that caused the interrupt.
            uint8_t sstat0 = NCR_BASE[NCR_SSTAT0];
            (void)NCR_BASE[NCR_DSTAT];
            return sstat0;
        }
        waitShort();
    }
    return 0xFF;
}

int detectA4000TSCSI(void)
{
    // TEMP DIAGNOSTIC checkpoints — remove once A4000T scan hang is found
    print("CHK4: enter detectA4000TSCSI\n", YELLOW);

    // MUST be the very first access to this chip — see comment above.
    NCR_BASE[NCR_DCNTL] = NCR_DCNTL_EA | NCR_DCNTL_COM;
    print("CHK5: past DCNTL write\n", YELLOW);

    if (NCR_BASE[NCR_ISTAT] == 0xFF) {
        print("CHK6: bus float, returning 0\n", YELLOW);
        return 0;   // bus float — nothing there
    }
    print("CHK7: past ISTAT float check (chip present)\n", YELLOW);

    // Soft-reset the SCSI bus and confirm the chip is still responding
    // (readable, non-floating) afterward.
    NCR_BASE[NCR_SCNTL1] = NCR_SCNTL1_RST;
    NCR_BASE[NCR_SCNTL1] = 0x00;
    print("CHK8: past soft reset\n", YELLOW);

    // Real chip-init sequence found by reading the leaked AmigaOS a4091
    // driver (ncr.c) end to end, not just the register map — three steps
    // it does that detect-only code had skipped, all confirmed required
    // before Selection can work at all:
    //   1. SCID must hold the *host adapter's own* SCSI ID as a bitmask
    //      (`b->scid = 1 << g->st_OwnID;` in ncr.c) — left at its power-on
    //      default (0) otherwise, which collides with target ID 0, the very
    //      first ID scanA4000TSCSI() selects.
    //   2. SCNTL1's ESR bit ("Enable Selection/Reselection") must be set,
    //      and only *after* SCID is programmed (driver comment: "Enable
    //      Selection/Reselection (after setting ID)") — without it the chip
    //      cannot perform Selection at all, so every SELECT script issued
    //      so far has been running with selection logic disabled at the
    //      chip level.
    //   3. SXFER's DHP bit ("disable halt on parity error") for async,
    //      no-parity-setup transfers — driver: `b->sxfer = SXFERF_DHP;`.
    NCR_BASE[NCR_SCID]   = (uint8_t)(1U << NCR_OWN_SCSI_ID);
    NCR_BASE[NCR_SCNTL1] |= NCR_SCNTL1_ESR;
    NCR_BASE[NCR_SXFER]  = NCR_SXFER_DHP;
    print("CHK9: past chip-init sequence (SCID/ESR/SXFER)\n", YELLOW);

    int detResult = (NCR_BASE[NCR_ISTAT] != 0xFF);
    print("CHK10: detectA4000TSCSI about to return\n", YELLOW);
    return detResult;
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

int scanA4000TSCSI(void)
{
    volatile struct GlobalVars *globals = a3k_globals();

    print("CHK11: enter scanA4000TSCSI\n", YELLOW);
    print("\nA4000T SCSI (NCR 53C710)\n", WHITE);

    if (NCR_BASE[NCR_ISTAT] == 0xFF) {
        print("  Bus float - controller not responding.\n", RED);
        return 0;
    }
    print("CHK12: past bus-float check in scan\n", YELLOW);

    // buf/identify/statusByte/msgByte are all mutated by the chip (DMA) or by
    // us and read back afterward — must be real stack locals, never `static`
    // (statics land in ROM in this freestanding build). `cdb` is genuinely
    // read-only and fine as static const (the chip only ever reads it).
    //
    // `script` is different: it's not just DMA'd into, it's the address the
    // chip's own autonomous SCRIPTS *instruction fetcher* bus-masters from —
    // a categorically different kind of access than a data Block Move. The
    // real AmigaOS driver (ncr.c/ncr710.h) never places its SCRIPTS/DSA
    // buffers on the CPU's call stack — always AllocMem'd, persistent
    // storage. Testing whether Amiberry's SCRIPTS-fetch path has a bug
    // specific to stack addresses (as opposed to allocator-returned RAM):
    // allocate via getMemory() instead of a stack array.
    uint8_t buf[SCSI_INQUIRY_LEN];
    static const uint8_t cdb[6] = { SCSI_INQUIRY, 0x00, 0x00, 0x00, SCSI_INQUIRY_LEN, 0x00 };
    uint8_t identify, statusByte, msgByte;
    uint32_t *script = (uint32_t *)getMemory(4 * sizeof(uint32_t));
    if (!script) {
        print("  Not enough memory for SCRIPTS buffer.\n", RED);
        return 0;
    }
    print("CHK13: got SCRIPTS buffer, entering ID loop\n", YELLOW);
    ncr_irq_enable(globals);
    print("Scanning SCSI bus (IDs 0-6)...\n\n", WHITE);

    // The 53C710 is an independent bus-master: it fetches its SCRIPTS
    // instructions from `script` and DMAs into/out of `buf` etc. directly
    // against physical RAM, bypassing the CPU data cache. On a 68040
    // (copyback cache by default) the CPU's writes to `script` may still be
    // sitting in cache, unflushed, when NCR_DSP is kicked off — the chip
    // would then fetch stale/garbage RAM contents as its "instructions",
    // which as an autonomous DMA engine could scribble anywhere. Each
    // build-then-run pair below is individually bracketed with
    // DisableCache()/EnableCache() — narrowly, so print()/waitShort() in
    // between ID attempts always run with cache on (waitShort()'s exact-match
    // raster-beam poll can miss its target and spin forever if cache is off
    // while it runs — confirmed by testing the wider bracket).
    int found = 0;

    for (int id = 0; id <= 6; id++) {
        print("CHK14: id loop top, id=", CYAN);
        char idch2[2] = { (char)('0' + id), 0 };
        print(idch2, CYAN);
        print("\n", CYAN);

        print("ID ", CYAN);
        char idch[2] = { (char)('0' + id), 0 };
        print(idch, CYAN);
        print(": ", WHITE);

        for (int i = 0; i < SCSI_INQUIRY_LEN; i++) buf[i] = 0;
        identify = 0x80;   // IDENTIFY, LUN 0, no reselect/disconnect
        statusByte = 0xFF;
        msgByte = 0xFF;

        DisableCache();
        ncr_buildSelect(script, (uint8_t)id);
        print("CHK15: built select script, calling runAndWait\n", CYAN);
        uint8_t sstat0 = ncr_runAndWait(script);   // re-enables cache internally — see its comment
        print("CHK16: runAndWait returned\n", CYAN);
        if (sstat0 == 0xFF) { print("POLL TIMEOUT\n", RED); waitShort(); continue; }
        if (sstat0 & NCR_SSTAT0_STO) { print("NOT FOUND\n", RED); waitShort(); continue; }

        // Selected. Walk phases until data+status have been captured, the
        // chip disconnects/errors, or we've taken an unreasonable number of
        // steps — the target dictates each next phase, we just follow it
        // (same idea as the A3000 WD33C93 manual phase loop).
        int gotData = 0;
        for (int step = 0; step < 8; step++) {
            uint8_t mci = NCR_BASE[NCR_SSTAT2] & 0x07;
            uint8_t *pbuf; int plen;
            switch (mci) {
                case NCR_PHASE_MSG_OUT: pbuf = &identify;   plen = 1; break;
                case NCR_PHASE_CMD:     pbuf = (uint8_t *)cdb; plen = 6; break;
                case NCR_PHASE_DATA_IN: pbuf = buf;         plen = SCSI_INQUIRY_LEN; break;
                case NCR_PHASE_STATUS:  pbuf = &statusByte; plen = 1; break;
                case NCR_PHASE_MSG_IN:  pbuf = &msgByte;    plen = 1; break;
                default: pbuf = NULL; plen = 0; break;
            }
            if (!pbuf) break;   // unexpected phase (or bus floating) — stop here

            DisableCache();
            ncr_buildMove(script, mci, pbuf, plen);
            uint8_t moveResult = ncr_runAndWait(script);   // re-enables cache internally
            if (moveResult == 0xFF) break;
            if (mci == NCR_PHASE_DATA_IN) gotData = 1;
            if (mci == NCR_PHASE_MSG_IN)  break;   // command-complete message — done
        }

        if (!gotData && statusByte == 0xFF) {
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

    ncr_irq_disable();
    print("\nIRQ2 (PORTS) events serviced by our handler: ", WHITE);
    print(binDec(globals->ScsiIrqCount), globals->ScsiIrqCount ? GREEN : RED);
    print("\n", WHITE);

    return found;
}

static int identifyA4000TSCSI(void) { return 0; }
static int smartA4000TSCSI(void)    { return 0; }

// ---- stub functions (hardware not yet implemented) -------------------------

static int detectNotImplemented(void)  { return 0; }
static int scanNotImplemented(void)    { return 0; }
static int identifyNotImplemented(void){ return 0; }
static int smartNotImplemented(void)   { return 0; }

// ---------------------------------------------------------------------------

static const HddController hddControllers[] = {
    { "A1200/A600 IDE",    detectGayleIDE,   scanGayleIDE,   identifyGayleIDE,   smartGayleIDE   },
    { "A4000 IDE",         detectA4000IDE,   scanA4000IDE,   identifyA4000IDE,   smartA4000IDE   },
    { "A3000/A3000T SCSI", detectA3000SCSI,  scanA3000SCSI,  identifyA3000SCSI,  smartA3000SCSI  },
    { "A2091 SCSI",        detectNotImplemented, scanNotImplemented, identifyNotImplemented, smartNotImplemented },
    { "GVP SCSI",          detectNotImplemented, scanNotImplemented, identifyNotImplemented, smartNotImplemented },
    { "A4000T SCSI",       detectA4000TSCSI, scanA4000TSCSI, identifyA4000TSCSI, smartA4000TSCSI },
};
#define NUM_HDD_CONTROLLERS (int)(sizeof(hddControllers)/sizeof(hddControllers[0]))

// ---------------------------------------------------------------------------
// Menu 0 — main HDD test
// ---------------------------------------------------------------------------

static const char HDDMenuText[]  = "\002HDD Controller Test";
static const char HDDMenu1[]     = "1 - Active Controller:";
static const char HDDMenu2[]     = "2 - Autodetect";
static const char HDDMenu3[]     = "3 - Scan Devices";
static const char HDDMenu4[]     = "4 - Identify Devices";
static const char HDDMenuBack[]  = "9 - Main Menu";

// SMART data ('5') was removed as a top-level menu entry — it's now reached
// via 'S' from inside Identify Devices, per-unit, instead of a separate
// whole-controller pass.
static const char *HDDMenuItems[] = {
    HDDMenuText,
    HDDMenu1, HDDMenu2, HDDMenu3, HDDMenu4, HDDMenuBack,
    NULL
};

// ---------------------------------------------------------------------------
// Menu 1 — controller select list
// ---------------------------------------------------------------------------

static const char HDDSelText[]  = "\002Select Controller";
static const char HDDSel1[]     = "1 - A1200/A600 IDE";
static const char HDDSel2[]     = "2 - A4000 IDE";
static const char HDDSel3[]     = "3 - A3000/A3000T SCSI";
static const char HDDSel4[]     = "4 - A2091 SCSI";
static const char HDDSel5[]     = "5 - GVP SCSI";
static const char HDDSel6[]     = "6 - A4000T SCSI";
static const char HDDSelBack[]  = "9 - Back";

static const char *HDDSelItems[] = {
    HDDSelText,
    HDDSel1, HDDSel2, HDDSel3, HDDSel4, HDDSel5, HDDSel6, HDDSelBack,
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
    MenuVar hddMenuVars[6] = {{0}};

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
                static const uint8_t posToKey[] = { '1','2','3','4','9' };
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

                case '2': {
                    waitReleased();
                    // Try each controller starting after current, wrap around once
                    int start = activeController;
                    int found = 0;
                    do {
                        if (++activeController >= NUM_HDD_CONTROLLERS)
                            activeController = 0;
                        if (hddControllers[activeController].detect &&
                            hddControllers[activeController].detect()) {
                            found = 1;
                            break;
                        }
                    } while (activeController != start);
                    setPos(43, 5);
                    print("                ", WHITE);
                    hddMenuVars[0].str   = (char *)hddControllers[activeController].name;
                    hddMenuVars[0].color = found ? GREEN : RED;
                    globals->PrintMenuFlag = 2;
                    break;
                }

                case '3':
                    waitReleased();
                    initScreen();
                    if (hddControllers[activeController].scan)
                        hddControllers[activeController].scan();
                    else
                        print("\nNot implemented for this controller.\n", RED);
                    print("\nPress any key/button to continue", WHITE);
                    WaitButton();
                    initScreen();
                    globals->PrintMenuFlag = 1;
                    break;

                case '4': {
                    waitReleased();
                    initScreen();
                    // A return of 1 means the controller's own identify() is
                    // fully interactive and already handled its own
                    // dismissal (e.g. identifyA3000SCSI()'s unit browser) —
                    // don't also show the generic one-shot prompt below.
                    int selfDismissed = 0;
                    if (hddControllers[activeController].identify)
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
                static const uint8_t posToKey[] = { '1','2','3','4','5','6','9' };
                if (globals->MenuPos < (uint8_t)sizeof(posToKey))
                    ch = posToKey[globals->MenuPos];
            }
            switch (ch) {
                case '1': case '2': case '3': case '4': case '5': case '6': {
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