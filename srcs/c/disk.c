#include "globalvars.h"
#include "generic.h"
#include "menus.h"
#include <hardware/cia.h>

void crap();

static const char DiskMenuText[] = "Memorytests\n";
static const char DiskMenuT1[] = "0 - Memorytests";
static const char DiskMenuT2[] = "1 - Memorytests";
static const char DiskMenuBack[] = "9 - Main menu";

static const char *DiskMenuItems[] = {
       DiskMenuText,
       DiskMenuT1,
       DiskMenuT2,
       DiskMenuBack,
       NULL
};

const char **DiskTestC[] = {
       DiskMenuItems,  // 0 — only slot used (MenuNumber is always 0)
};



void floppyTestC(void)
{
    // Local variables — live on the stack for the lifetime of this function.
    // floppyTestC never returns (it loops forever until mainMenu() is called),
    // so the stack frame stays alive and all pointers into it remain valid.
    // This avoids the ROM-environment problem with writable static variables
    // (no C runtime zeros BSS or copies .data to RAM in this -nostdlib build).
    uint32_t addOneCount = 0;
    char     addOneStr[12];
    MenuVar  diskMenuVars[3];

    // Initialise explicitly — avoids GCC emitting memcpy/memset calls for
    // string-literal array init, and avoids unaligned move.l from packed-struct
    // initialiser syntax hitting an odd stack address on 68000.
    addOneStr[0] = '0';
    addOneStr[1] = '\0';

    diskMenuVars[0].color = GREEN;
    diskMenuVars[0].str   = addOneStr;
    diskMenuVars[1].str   = (char *)0;
    diskMenuVars[2].str   = (char *)0;

    initScreen();
    globals->Menu         = (void *)DiskTestC;
    globals->MenuVariable = (void *)diskMenuVars;
    globals->MenuNumber   = 0;
    globals->PrintMenuFlag = 1;

    for (;;) {
        // --- per-iteration hardware work goes here ---

        printMenu();
        getInput();
        waitLong();

        uint8_t ch  = globals->GetCharData;
        int8_t  sel = -1;

        if      (ch == 0x0a || globals->LMB) sel = (int8_t)globals->MenuPos;
        else if (ch == '0') sel = 0;
        else if (ch == '1') sel = 1;
        else if (ch == '9') sel = 2;

        if (globals->RMB) { mainMenu(); return; }

        if (sel >= 0) {
            waitReleased();
            if (sel == 0) {
                // addOne: increment and copy bindec result into our local buffer
                addOneCount++;
                char *s = bindec(addOneCount);
                char *d = addOneStr;
                while ((*d++ = *s++) != '\0') {}
            } else if (sel == 1) {
                crap();
            } else {
                mainMenu(); return;
            }

            // Restore our menu context after handler returns
            globals->Menu         = (void *)DiskTestC;
            globals->MenuNumber   = 0;
            globals->MenuVariable = (void *)diskMenuVars;
            globals->PrintMenuFlag = 1;
        }
    }
}
void crap()
{
              initScreen();
              volatile struct CIA *ciaa = (struct CIA *)0xbfe001;
       volatile struct CIA *ciab = (struct CIA *)0xbfd000;
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
       ciab->ciaprb &= ~(CIAB_DSKSEL0);
       ciab->ciaprb |= CIAB_DSKSEL0;

    uint32_t id = 0;
    uint8_t mask = CIAB_DSKSEL0;
    unsigned int i;

    ciab->ciaprb |= 0xf8;  /* motor-off, deselect all */
    ciab->ciaprb &= 0x7f;  /* 1. MTRXD low */
    ciab->ciaprb &= ~mask; /* 2. SELxB low */
    ciab->ciaprb |= mask;  /* 3. SELxB high */
    ciab->ciaprb |= 0x80;  /* 4. MTRXD high */
    ciab->ciaprb &= ~mask; /* 5. SELxB low */
    ciab->ciaprb |= mask;  /* 6. SELxB high */
    for (i = 0; i < 32; i++) {
        ciab->ciaprb &= ~mask; /* 7. SELxB low */
        id = (id<<1) | ((ciaa->ciapra>>5)&1); /* 8. Read and save state of RDY */
        ciab->ciaprb |= mask;  /* 9. SELxB high */
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
