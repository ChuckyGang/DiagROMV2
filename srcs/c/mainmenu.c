#include "generic.h"
#include "menus.h"

// ---------------------------------------------------------------------------
// Static helpers (internal only)
// ---------------------------------------------------------------------------

static void printStatus(void)
{
    uint32_t y = globals->NTSC ? 26 : 31;
    setPos(0, y);
    print((char *)StatusLine, YELLOW);
}

static void updateStatus(void)
{
    uint32_t y = globals->NTSC ? 26 : 31;

    // Serial speed at x=8
    setPos(8, y);
    print((char *)SerText[globals->SerialSpeed], WHITE);

    // CPU type at x=25
    setPos(25, y);
    print((char *)globals->CPUPointer, WHITE);

    // Chip RAM in kB at x=40
    setPos(40, y);
    print(bindec(toKB(globals->TotalChip)), WHITE);
    print("kB", WHITE);

    // Fast RAM at x=57 (stored already in appropriate unit)
    setPos(57, y);
    print(bindec(globals->TotalFast), WHITE);

    // Globals base address at x=70
    setPos(70, y);
    print(binHex((uint32_t)globals), WHITE);
}

static void menuUp(void)
{
    if (globals->MenuPos > 0)
        globals->MenuPos--;
}

static void menuDown(void)
{
    if (globals->MenuPos < globals->MenuEntrys)
        globals->MenuPos++;
}

// Print a single menu item in the given color.
// item is 0-based (0 = first real item, not the label).
static void printMenuItem(uint8_t item, uint8_t color)
{
    const char **items = ((const char ***)globals->Menu)[globals->MenuNumber];
    setPos(20, 5 + item * 2);
    print((char *)items[item + 1], color);
}

static void handleMenu(void)
{
    if (globals->MenuChoose == 0) {
        if (globals->MBUTTON == 0) {
            // Keyboard shortcut check
            uint8_t *keys = MenuKeys[globals->MenuNumber];
            uint8_t ch = globals->GetCharData;
            for (uint8_t i = 0; keys[i] != 0; i++) {
                if (keys[i] == ch) {
                    globals->MarkItem = i;
                    goto released;
                }
            }
            return;
        }
        // Mouse button pressed — wait for release
        do {
            getInput();
            waitShort();
        } while (globals->MBUTTON != 0);
    }

released:
    globals->MenuChoose = 0;
    MenuCode[globals->MenuNumber][globals->MarkItem]();
}

// ---------------------------------------------------------------------------
// printMenu — externally visible (called from disktest.s, audiotest.s)
// ---------------------------------------------------------------------------
void printMenu(void)
{
    uint16_t menuNum = globals->MenuNumber;

    // Reset state when menu changes
    if (menuNum != globals->OldMenuNumber) {
        globals->MarkItem    = 0;
        globals->OldMarkItem = 0;
        globals->OldMenuNumber = menuNum;
    }

    if (globals->PrintMenuFlag != 0) {
        if (globals->PrintMenuFlag != 2)
            globals->MenuPos = 0;

        const char **items = ((const char ***)globals->Menu)[menuNum];

        // Count actual items (items[0]=label, items[1..N]=items, items[N+1]=NULL)
        int count = 0;
        for (int i = 1; items[i] != NULL; i++) count++;
        // asm stored count-1 so .Down stops at the right boundary
        globals->MenuEntrys = (uint8_t)(count - 1);

        const MenuVar *vars = (const MenuVar *)globals->MenuVariable;

        // Print label (only when updating all)
        setPos(0, 0);
        if (globals->UpdateMenuNumber == 0)
            print((char *)items[0], WHITE);

        // Print items
        uint8_t ypos = 5;
        for (int i = 0; i < count; i++) {
            uint8_t itemNum = (uint8_t)(i + 1);   // 1-indexed, matches asm d4
            if (globals->UpdateMenuNumber == 0 || globals->UpdateMenuNumber == itemNum) {
                setPos(20, ypos);
                print((char *)items[i + 1], CYAN);
                if (vars) {
                    print(" ", CYAN);
                    if (vars[i].str)
                        print(vars[i].str, vars[i].color);
                }
            }
            ypos += 2;
        }

        globals->UpdateMenuFlag   = 1;
        globals->UpdateMenuNumber = 0;
    }

    // Navigation — always runs regardless of PrintMenuFlag
    uint8_t ch = globals->GetCharData;
    if (ch == 30) {                     // up arrow
        menuUp();
    } else if (ch == 31) {              // down arrow
        menuDown();
    } else if (ch == 0x0a) {           // Enter
        globals->MenuChoose = 1;
        ClearBuffer();
    }

    // Mouse scroll accumulator
    if (globals->CurAddY != 0) {
        globals->MenuMouseSub = 0;
        globals->MenuMouseAdd += globals->CurAddY;
        if (globals->MenuMouseAdd >= 40) {
            globals->MenuMouseAdd = 0;
            menuDown();
        }
    }
    if (globals->CurSubY != 0) {
        globals->MenuMouseAdd = 0;
        globals->MenuMouseSub += globals->CurSubY;
        if (globals->MenuMouseSub >= 40) {
            globals->MenuMouseSub = 0;
            menuUp();
        }
    }

    // Update highlight if item changed or menu just printed
    globals->MenuPos  = globals->MenuPos;   // no-op; keeps value
    globals->MarkItem = globals->MenuPos;

    if (globals->PrintMenuFlag != 0 ||
        (uint8_t)globals->OldMarkItem != globals->MarkItem) {
        // Redraw formerly highlighted item in normal colour
        printMenuItem((uint8_t)globals->OldMarkItem, CYAN);
        globals->OldMarkItem = globals->MarkItem;
        // Draw newly highlighted item in reverse colour
        printMenuItem(globals->MarkItem, R_CYAN);
    }

    globals->PrintMenuFlag = 0;
}

void mainLoop(void)
{
    for (;;) {
        printMenu();
        getInput();
        handleMenu();
    }
}

// ---------------------------------------------------------------------------
// Externally visible functions (asm wrappers in mainmenu.s call these)
// ---------------------------------------------------------------------------

void filterON(void)
{
    *(volatile uint8_t *)0xbfe001 &= ~(1 << 1);    // bclr #1,$bfe001
}

void filterOFF(void)
{
    *(volatile uint8_t *)0xbfe001 |= (1 << 1);     // bset #1,$bfe001
}

void swapMode(void)
{
    globals->NTSC    = 0;
    *(volatile uint16_t *)0xdff180 = 0xfff;        // white flash
    globals->SCRNMODE ^= 0x20;
    if (globals->SCRNMODE == 0x20)
        globals->NTSC = 1;
    *(volatile uint16_t *)0xdff1dc = globals->SCRNMODE;  // BEAMCON0
    mainMenu();
}

void exitDiag(void)
{
    // Flash screen indefinitely (intentional infinite loop)
    for (;;) {
        *(volatile uint16_t *)0xdff180 = 0;
        while (*(volatile uint8_t *)0xdff006 != 0xf0);
        *(volatile uint16_t *)0xdff180 = 0xfff;
    }
}

void initScreen(void)
{
    clearScreen();
    printStatus();
    updateStatus();
    setPos(0, 0);
}

void mainMenu(void)
{
    ClearBuffer();
    filterON();
    clearScreen();
    printStatus();
    updateStatus();
    setPos(0, 0);
    globals->Menu         = (void *)Menus;
    globals->MenuVariable = (void *)0;
    globals->MenuNumber   = 0;
    globals->PrintMenuFlag = 1;
    ClearBuffer();
    mainLoop();
}
