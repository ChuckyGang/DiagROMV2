#include "globalvars.h"
#include "generic.h"
#include "menus.h"
#define custom ((volatile struct Custom*)0xdff000)
void crap();

/* Forward declarations — definitions live further down in this file */
void     selectDrive(uint32_t floppyID);
void     floppyMotor(uint32_t floppyID);
void     floppySide(uint32_t floppyID);
void     floppyStepOut(uint32_t floppyID);
void     floppyStepIn(uint32_t floppyID);
uint32_t getFloppyID(void);
void     gotoZero(void);

static const char DiskMenuText[] = "\002Floppy Test";
static const char DiskMenuT1[] = "1 - Select Disk: ";
static const char DiskMenuT2[] = "2 - Motor";
static const char DiskMenuT3[] = "3 - Change side";
static const char DiskMenuT4[] = "4 - Step out";
static const char DiskMenuT5[] = "5 - Step in";
static const char DiskMenuT6[] = "6 - Step out 10 tracks";
static const char DiskMenuT7[] = "7 - Stel in 10 tracks";
static const char DiskMenuT8[] = "8 - Read track to buffer";

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
    globals->Menu         = (void *)DiskTestC;
    globals->MenuVariable = (void *)diskMenuVars;
    globals->MenuNumber   = 0;
    globals->PrintMenuFlag = 1;
    int bah = 0;

    setPos(6,2);
    print("Track:",YELLOW);
    setPos(18,2);
    print("Side:",YELLOW);
    setPos(32,2);
    print("Motor:",YELLOW);
    setPos(45,2);
    print("WProtection:",YELLOW);
    setPos(64,2);
    print("Disk:",YELLOW);

    setPos(6,3);
    print("Ready:",YELLOW);
    setPos(18,3);
    print("Track0:",YELLOW);
    setPos(32,3);
    print("$bfe001:",YELLOW);
    setPos(52,3);
    print("$bdf100:",YELLOW);
    setPos(42,5);
    print("Drive ID: ",WHITE);
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

        /* Mouse click / Enter → treat as the current menu item's digit key */
        if (globals->LMB || globals->RMB || ch == 0x0a) {
            ch = '1' + globals->MenuPos;
        }

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
                floppyStepOut(floppyID);
                break;

            case '5':
                waitReleased();
                floppyStepIn(floppyID);
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

void floppyStepOut(uint32_t floppyID)
{
    if (globals->TrackNo >= 79) return;  // 80 tracks total (0..79)

    uint8_t selBit  = 1 << (floppyID + 3);
    uint8_t selMask = CIAF_DSKSEL0 | CIAF_DSKSEL1 | CIAF_DSKSEL2 | CIAF_DSKSEL3;

    CIAB->ciaprb = (CIAB->ciaprb | selMask) & ~selBit;   // select chosen drive
    CIAB->ciaprb &= ~CIAF_DSKDIREC;                      // DIR=0 → step to higher track number
    (void)*(volatile uint16_t *)0xdff1fe;                // settle
    CIAB->ciaprb &= ~CIAF_DSKSTEP;                       // step pulse low
    (void)*(volatile uint16_t *)0xdff1fe;
    CIAB->ciaprb |= CIAF_DSKSTEP;                        // step pulse high → drive steps one track
    waitLong();                                          // head settle (3ms minimum on real drives)
    CIAB->ciaprb |= selMask;                             // deselect

    globals->TrackNo++;
}

void floppyStepIn(uint32_t floppyID)
{
    if (globals->TrackNo == 0) return;   // already at track 0

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