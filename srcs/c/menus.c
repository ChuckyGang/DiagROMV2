#include "generic.h"
#include "menus.h"

// Forward declarations: asm handlers not yet converted to C.
// __asm("name") tells GCC to use the bare symbol name (no _ prefix).
extern void SystemInfoTest(void)      __asm("SystemInfoTest");
extern void AudioMenu(void)           __asm("AudioMenu");
extern void MemtestMenu(void)         __asm("MemtestMenu");
extern void IRQCIAtestMenu(void)      __asm("IRQCIAtestMenu");
extern void GFXtestMenu(void)         __asm("GFXtestMenu");
extern void PortTestMenu(void)        __asm("PortTestMenu");
extern void DiskTest(void)            __asm("DiskTest");
extern void KeyBoardTest(void)        __asm("KeyBoardTest");
extern void OtherTest(void)           __asm("OtherTest");
extern void Setup(void)               __asm("Setup");
extern void About(void)               __asm("About");

extern void AudioSimple(void)         __asm("AudioSimple");
extern void AudioMod(void)            __asm("AudioMod");

extern void CheckDetectedChip(void)   __asm("CheckDetectedChip");
extern void CheckExtendedChip(void)   __asm("CheckExtendedChip");
extern void CheckDetectedMBMem(void)  __asm("CheckDetectedMBMem");
extern void CheckExtended16MBMem(void) __asm("CheckExtended16MBMem");
extern void ForceExtended16MBMem(void) __asm("ForceExtended16MBMem");
extern void Detectallmemory(void)     __asm("Detectallmemory");
extern void CheckMemManual(void)      __asm("CheckMemManual");
extern void CheckMemEdit(void)        __asm("CheckMemEdit");
extern void AutoConfig(void)          __asm("AutoConfig");

extern void IRQCIAIRQTest(void)       __asm("IRQCIAIRQTest");
extern void IRQCIACIATest(void)       __asm("IRQCIACIATest");
extern void IRQCIATest(void)          __asm("IRQCIATest");
extern void IRQTestC(void)            __asm("IRQTestC");

extern void GFXTestScreen(void)       __asm("GFXTestScreen");
extern void GFXTestScroll(void)       __asm("GFXTestScroll");
extern void GFXTestRaster(void)       __asm("GFXTestRaster");
extern void GFXTestRGB(void)          __asm("GFXTestRGB");
extern void gfxC(void)                __asm("gfxC");
extern void gfxChigh(void)            __asm("gfxChigh");
extern void gfxCAga(void)             __asm("gfxCAga");
extern void gfxCAgaHigh(void)         __asm("gfxCAgaHigh");

extern void PortTestPar(void)         __asm("PortTestPar");
extern void PortTestSer(void)         __asm("PortTestSer");
extern void PortTestJoystick(void)    __asm("PortTestJoystick");

extern void RTCTest(void)             __asm("RTCTest");
extern void AutoConfigDetail(void)    __asm("AutoConfigDetail");
extern void ShowMemAddress(void)      __asm("ShowMemAddress");
extern void RTCTestC(void)            __asm("RTCTestC");
extern void TF1260(void)              __asm("TF1260");

extern void DiskdriveTest(void)       __asm("DiskdriveTest");
extern void GayleTest(void)           __asm("GayleTest");
extern void GayleExp(void)            __asm("GayleExp");
extern void floppyTestC(void)         __asm("floppyTestC");

// C functions defined in mainmenu.c
void mainMenu(void);
void swapMode(void);

// ---------------------------------------------------------------------------
// Serial speed strings (fixed-width, 6 chars + null)
// ---------------------------------------------------------------------------
static const char bpsNone[]   = "N/A   ";
static const char bps2400[]   = "2400  ";
static const char bps9600[]   = "9600  ";
static const char bps19200[]  = "19200 ";
static const char bps38400[]  = "38400 ";
static const char bps115200[] = "115200";
static const char bpsLoop[]   = "LOOP  ";

const char *SerText[] = {
    bpsNone, bps2400, bps9600, bps19200, bps38400, bps115200, bpsLoop, bpsNone
};

// ---------------------------------------------------------------------------
// Main Menu  (MenuNumber 0)
// ---------------------------------------------------------------------------
// MainMenuText stays in data.s — it uses VERSION macro + incbin "builddate.i"
static const char mainMenu1[]  = "0 - Systeminfo";
static const char mainMenu2[]  = "1 - Audiotests";
static const char mainMenu3[]  = "2 - Memorytests";
static const char mainMenu4[]  = "3 - IRQ/CIA Tests";
static const char mainMenu5[]  = "4 - Graphicstests";
static const char mainMenu6[]  = "5 - Porttests";
static const char mainMenu7[]  = "6 - Drivetests";
static const char mainMenu8[]  = "7 - Keyboardtests";
static const char mainMenu9[]  = "8 - Other tests";
static const char mainMenu10[] = "S - Setup";
static const char mainMenu11[] = "A - About";

static const char *mainMenuItems[] = {
    MainMenuText,
    mainMenu1, mainMenu2, mainMenu3, mainMenu4, mainMenu5, mainMenu6,
    mainMenu7, mainMenu8, mainMenu9, mainMenu10, mainMenu11,
    NULL
};
static MenuHandler mainMenuCode[] = {
    SystemInfoTest, AudioMenu, MemtestMenu, IRQCIAtestMenu, GFXtestMenu,
    PortTestMenu, DiskTest, KeyBoardTest, OtherTest, Setup, About, swapMode
};
static uint8_t mainMenuKey[] = { '0','1','2','3','4','5','6','7','8','s','a',' ',0 };

// ---------------------------------------------------------------------------
// Audio Menu  (MenuNumber 2)
// ---------------------------------------------------------------------------
static const char audioMenuText[] = "\002Audiotests\n\n";
static const char audioMenu1[]    = "1 - Simple waveformtest";
static const char audioMenu2[]    = "2 - Play test-module";
static const char audioMenu3[]    = "9 - MainMenu";

static const char *audioMenuItems[] = {
    audioMenuText, audioMenu1, audioMenu2, audioMenu3, NULL
};
static MenuHandler audioMenuCode[] = { AudioSimple, AudioMod, mainMenu };
static uint8_t audioMenuKey[]      = { '1','2','9',0 };

// ---------------------------------------------------------------------------
// Memory test Menu  (MenuNumber 3)
// ---------------------------------------------------------------------------
static const char memtestText[]    = "\x02Memorytests\n\n";
static const char memtestMenu1[]   = "1 - Test detected chipmem";
static const char memtestMenu2[]   = "2 - Extended chipmemtest (MIGHT crash on older amigas)";
static const char memtestMenu3[]   = "3 - Test detected fastmem";
static const char memtestMenu4[]   = "4 - Fast scan of 16MB fastmem-areas";
static const char memtestMenu5[]   = "5 - Slow scan of 16MB fastmem-areas";
static const char memtestMenu6[]   = "6 - Complete Memorydetection";
static const char memtestMenu7[]   = "7 - Manual memorytest (NEW)";
static const char memtestMenu8[]   = "8 - Manual memoryedit";
static const char memtestMenu9[]   = "9 - Autoconfig - Automatic";
static const char memtestMenu10[]  = "0 - Mainmenu";

static const char *memtestMenuItems[] = {
    memtestText,
    memtestMenu1, memtestMenu2, memtestMenu3, memtestMenu4, memtestMenu5,
    memtestMenu6, memtestMenu7, memtestMenu8, memtestMenu9, memtestMenu10,
    NULL
};
static MenuHandler memtestMenuCode[] = {
    CheckDetectedChip, CheckExtendedChip, CheckDetectedMBMem,
    CheckExtended16MBMem, ForceExtended16MBMem, Detectallmemory,
    CheckMemManual, CheckMemEdit, AutoConfig, mainMenu
};
static uint8_t memtestMenuKey[] = { '1','2','3','4','5','6','7','8','9','0',0 };

// ---------------------------------------------------------------------------
// IRQ/CIA Test Menu  (MenuNumber 4)
// ---------------------------------------------------------------------------
static const char irqCIAText[]     = "\x02IRQ & CIA Tests\n\n";
static const char irqCIAMenu1[]    = "1 - Test IRQs (OLD! SOON REMOVED)";
static const char irqCIAMenu2[]    = "2 - Test CIAs (OLD! SOON REMOVED)";
static const char irqCIAMenu3[]    = "3 - New Test CIAs";
static const char irqCIAMenu4[]    = "4 - New Test IRQ";
static const char irqCIAMenu5[]    = "9 - Mainmenu";

static const char *irqCIAMenuItems[] = {
    irqCIAText, irqCIAMenu1, irqCIAMenu2, irqCIAMenu3, irqCIAMenu4, irqCIAMenu5, NULL
};
static MenuHandler irqCIAMenuCode[] = {
    IRQCIAIRQTest, IRQCIACIATest, IRQCIATest, IRQTestC, mainMenu
};
static uint8_t irqCIAMenuKey[] = { '1','2','3','4','9',0 };

// ---------------------------------------------------------------------------
// Graphics Test Menu  (MenuNumber 5)
// ---------------------------------------------------------------------------
static const char gfxText[]    = "\x02Graphicstests\n\n";
static const char gfxMenu1[]   = "1 - Testpicture in lowres 32Col";
static const char gfxMenu2[]   = "2 - Test Scroll";
static const char gfxMenu3[]   = "3 - Test raster (button to exit)";
static const char gfxMenu4[]   = "4 - RGB-test";
static const char gfxMenu5[]   = "5 - Testscreen 320x256 (New C)";
static const char gfxMenu6[]   = "6 - Testscreen 640x512 (New C)";
static const char gfxMenu7[]   = "7 - Testscreen 320x256 AGA (New C)";
static const char gfxMenu8[]   = "8 - Testscreen 640x512 AGA (New C)";
static const char gfxMenu9[]   = "9 - Exit to mainmenu";

static const char *gfxMenuItems[] = {
    gfxText,
    gfxMenu1, gfxMenu2, gfxMenu3, gfxMenu4, gfxMenu5,
    gfxMenu6, gfxMenu7, gfxMenu8, gfxMenu9,
    NULL
};
static MenuHandler gfxMenuCode[] = {
    GFXTestScreen, GFXTestScroll, GFXTestRaster, GFXTestRGB,
    gfxC, gfxChigh, gfxCAga, gfxCAgaHigh, mainMenu
};
static uint8_t gfxMenuKey[] = { '1','2','3','4','5','6','7','8','9',0 };

// ---------------------------------------------------------------------------
// Port Test Menu  (MenuNumber 6)
// ---------------------------------------------------------------------------
static const char portText[]   = "\x02Porttests\n\n";
static const char portMenu1[]  = "1 - Parallel Port";
static const char portMenu2[]  = "2 - Serial Port";
static const char portMenu3[]  = "3 - Joystick/Mouse Ports";
static const char portMenu4[]  = "9 - Mainmenu";

static const char *portMenuItems[] = {
    portText, portMenu1, portMenu2, portMenu3, portMenu4, NULL
};
static MenuHandler portMenuCode[] = {
    PortTestPar, PortTestSer, PortTestJoystick, mainMenu
};
static uint8_t portMenuKey[] = { '1','2','3','9',0 };

// ---------------------------------------------------------------------------
// Other Test Menu  (MenuNumber 7)
// ---------------------------------------------------------------------------
static const char otherText[]  = "\x02Other tests\n\n";
static const char otherMenu1[] = "1 - RTC Test (TO BE REMOVED NOT WORKING)";
static const char otherMenu2[] = "2 - Autoconfig - Detailed";
static const char otherMenu3[] = "3 - ShowMemAddress Content";
static const char otherMenu4[] = "4 - New RTC Test";
static const char otherMenu5[] = "8 - TF360/TF1260 Diag";
static const char otherMenu6[] = "9 - Mainmenu";

static const char *otherMenuItems[] = {
    otherText,
    otherMenu1, otherMenu2, otherMenu3, otherMenu4, otherMenu5, otherMenu6,
    NULL
};
static MenuHandler otherMenuCode[] = {
    RTCTest, AutoConfigDetail, ShowMemAddress, RTCTestC, TF1260, mainMenu
};
static uint8_t otherMenuKey[] = { '1','2','3','4','8','9',0 };

// ---------------------------------------------------------------------------
// Disk Test Menu  (MenuNumber 8)
// ---------------------------------------------------------------------------
static const char diskText[]   = "\002Disktests\n\n";
static const char diskMenu1[]  = "1 - Diskdrivetest (old experimental)";
static const char diskMenu2[]  = "2 - Gayletest (A600/1200 etc IDE)";
static const char diskMenu3[]  = "3 - Gary-IDE test (A4000)";
static const char diskMenu4[]  = "4 - Floppytest (New Experimental)";
static const char diskMenu5[]  = "9 - Mainmenu";

static const char *diskMenuItems[] = {
    //diskText, diskMenu1, diskMenu2, diskMenu3, diskMenu4, diskMenu5, NULL
    diskText, diskMenu1, diskMenu2, diskMenu3, diskMenu5, NULL
};
static MenuHandler diskMenuCode[] = {
    //DiskdriveTest, GayleTest, GayleExp, floppyTestC, mainMenu
    DiskdriveTest, GayleTest, GayleExp, mainMenu
};
//static uint8_t diskMenuKey[] = { '1','2','3','4','9',0 };
static uint8_t diskMenuKey[] = { '1','2','3','9',0 };

// ---------------------------------------------------------------------------
// Top-level tables indexed by MenuNumber
// Slots 1, 9, 10 are unused (NULL)
// ---------------------------------------------------------------------------
const char **Menus[] = {
    mainMenuItems,  // 0
    NULL,           // 1 (unused)
    audioMenuItems, // 2
    memtestMenuItems,
    irqCIAMenuItems,
    gfxMenuItems,
    portMenuItems,
    otherMenuItems,
    diskMenuItems,
    NULL,           // 9 (unused)
    NULL            // 10 (unused)
};

MenuHandler *MenuCode[] = {
    mainMenuCode,
    NULL,
    audioMenuCode,
    memtestMenuCode,
    irqCIAMenuCode,
    gfxMenuCode,
    portMenuCode,
    otherMenuCode,
    diskMenuCode,
    NULL,
    NULL
};

uint8_t *MenuKeys[] = {
    mainMenuKey,
    NULL,
    audioMenuKey,
    memtestMenuKey,
    irqCIAMenuKey,
    gfxMenuKey,
    portMenuKey,
    otherMenuKey,
    diskMenuKey,
    NULL,
    NULL
};
