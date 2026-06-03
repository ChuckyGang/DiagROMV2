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
        print("Sector not present in buffer (no $4489 match — read a track first?)", RED);
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