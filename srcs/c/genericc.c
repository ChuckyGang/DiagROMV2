#include "generic.h"
#include <string.h>
#include "globalvars.h"

void readSerial();
void print(char *string __asm("a0"), uint8_t color __asm("d1"));
char *binHex(uint32_t value __asm("d0"));

// Forward declarations for input subsystem
void waitShort(void);
void getSerial(void);
uint8_t getCharSerial(void);
uint8_t getKey(void);
uint8_t getCharKey(void);
uint32_t getMouseData(void);
uint32_t getInput(void);

// Byte access macros for uint32_t fields that asm accesses with move.b (big-endian MSB)
#define BYTE(field) (*(volatile uint8_t *)&(field))

void clearScreen()              // Clears the screen
{
    if(!globals->NoDraw)
        clearDisplay();
    sendSerial("\x1b[0m\x1b[40m\x1b[37m");
    sendSerial("\x1b[2J");
    rs232_out('\x0c');
    sendSerial("\x1b[0m\x1b[40m\x1b[37m");
    setPos(0,0);
}

void setPos(uint32_t xPos __asm("d0"), uint32_t yPos __asm("d1"))
{
    BYTE(globals->Xpos) = xPos;
    BYTE(globals->Ypos) = yPos;
    sendSerial("\x1b[");
    sendSerial(binDec(yPos+1));     // ANSI row first
    rs232_out(';');
    sendSerial(binDec(xPos+1));     // ANSI column second
    rs232_out('H');
}

#ifndef ROM_BASE
#define ROM_BASE 0xF80000
#endif

void romChecksum()
{
    extern uint32_t checksums[];
    extern uint32_t endchecksums;

    print("\n\nDoing ROM Checksumtest: (64K blocks, Green OK, Red Failed)\n", 3);

    uint32_t csStart = (uint32_t)checksums;
    uint32_t csEnd   = (uint32_t)&endchecksums;

    for(int block = 0; block < 8; block++)
    {
        uint32_t blockBase = (uint32_t)ROM_BASE + (uint32_t)block * (uint32_t)0x10000;
        volatile uint32_t *rom = (volatile uint32_t *)blockBase;
        uint32_t sum = 0;
        int i;

        // Sum all 4-aligned longwords in this block, skipping the checksum
        // value array.  Matches checksum.c exactly: skip any 4-aligned address
        // that falls in [csStart, csEnd).
        for(i = 0; i < 0x4000; i++)
        {
            uint32_t addr = blockBase + (uint32_t)i * 4;
            if (addr >= csStart && addr < csEnd)
                continue;
            sum += rom[i];
        }

        uint8_t color = (sum == checksums[block]) ? 2 : 1;

        print(binHex(sum), color);
        print(" ", color);
    }
}

void print(char *string __asm("a0"), uint8_t color __asm("d1"))                  // Prints a string on screen
{
    if(*string == 2)                                     // If first byte is 2, center the string
    {
        string++;                                        // Skip the center marker
        char *s = string;
        int len = 0;
        while(*s)                                        // Count printable chars
        {
            if(*s > 31)
                len++;
            s++;
        }
        if(len < 80)                                     // Only center if it fits on one row
        {
            int spaces = (80 - len) / 2;
            for(int i = 0; i < spaces; i++)
                printChar(' ', color);
        }
    }
    int count = 0;
    while(*string && count < 3000)                       // Print string, max 3000 chars safety limit
    {
        printChar(*string++, color);
        count++;
    }
}

void printChar(char character __asm("d0"), uint8_t color __asm("d1"))            // Prints a char on screen, handles X, Y position, scrolling etc.
{
    uint8_t invCol=0;
    if(character==0xd)
    {
        return;
    }
    if(color!=globals->Color)               // if color is not same as last time, we had a colorchange. lets handle it on serialport
    {
        globals->Color=color;               // Save new color as color used
        if(color>7)                         // If color is more then 7, it should be inverted
        {
            globals->Inverted = 1;
            invCol=color-7;
            sendSerial("\x1b[30m");          // Black foreground
            sendSerial("\x1b[");
            rs232_out('4');
            rs232_out('0' + invCol);         // Color is 0-7, single digit
            rs232_out('m');
        }
        else
        {
            if(globals->Inverted)            // Was last char inverted? Clear it
            {
                sendSerial("\x1b[0m\x1b[40m\x1b[37m");
                globals->Inverted = 0;
            }
            sendSerial("\x1b[");
            rs232_out('3');
            rs232_out('0' + color);          // Color is 0-7, single digit
            rs232_out('m');
        }
    }
    if(character==0xa)      // if it is hex a, do a new line
    {
        printCharNewLine();
        return;
    }

    putChar(character, color, BYTE(globals->Xpos), BYTE(globals->Ypos));
    BYTE(globals->Xpos)++;
    if(BYTE(globals->Xpos)>79)
    {
        printCharNewLine();
    }

}

void printCharNewLine()
{
    BYTE(globals->Xpos)=0;
    BYTE(globals->Ypos)++;
    rs232_out('\x0a');
    rs232_out('\x0d');
    if(globals->NTSC)
    {
        if(BYTE(globals->Ypos)>24)
        {
            scrollScreen();
            BYTE(globals->Xpos)=0;
            BYTE(globals->Ypos)--;
        }
    }
    else
        if(BYTE(globals->Ypos)>31)
        {
            scrollScreen();
            BYTE(globals->Xpos)=0;
            BYTE(globals->Ypos)--;
        }
    
}

// Convert a 32-bit value to hex string with leading "$"
// Output: pointer to string stored in globals->binhexoutput
char *binHex(uint32_t value __asm("d0"))
{
    static const char hextab[] = "0123456789ABCDEF";
    char *buf = (char *)globals->binhexoutput;
    buf[0] = '$';
    for(int i = 7; i >= 0; i--)
    {
        buf[i + 1] = hextab[value & 0xF];
        value >>= 4;
    }
    buf[9] = 0;
    return buf;
}

char *binHexByte(uint32_t value __asm("d0"))
{
    static const char hextab[] = "0123456789ABCDEF";
    char *buf = (char *)globals->binhexoutput;
    buf[7] = hextab[(value >> 4) & 0xF];
    buf[8] = hextab[value & 0xF];
    buf[9] = '\0';
    return &buf[7];
}

char *binHexWord(uint32_t value __asm("d0"))
{
    static const char hextab[] = "0123456789ABCDEF";
    char *buf = (char *)globals->binhexoutput;
    buf[4] = '$';
    buf[5] = hextab[(value >> 12) & 0xF];
    buf[6] = hextab[(value >> 8) & 0xF];
    buf[7] = hextab[(value >> 4) & 0xF];
    buf[8] = hextab[value & 0xF];
    buf[9] = '\0';
    return &buf[4];
}

char *binString(uint32_t value __asm("d0"))
{
    char *buf = (char *)globals->binstringoutput;
    for(int i = 31; i >= 0; i--)
        *buf++ = (value >> i) & 1 ? '1' : '0';
    *buf = '\0';
    return (char *)globals->binstringoutput;
}

// Input:  value = signed 32-bit number
// Output: pointer to string stored in globals->bindecoutput
char *binDec(int32_t value)
{
    char *buf = (char *)globals->bindecoutput;
    char *p = buf;
    int32_t v = value;

    if (v < 0) {
        *p++ = '-';
        v = -v;
    }

    char digits[11];        // Max 10 digits for 32-bit + safety
    int len = 0;
    uint32_t u = (uint32_t)v;
    do {
        if(u <= 0xFFFF) {                       // 68000 divu handles 16-bit
            uint16_t s = (uint16_t)u;
            digits[len++] = '0' + (s % 10);
            u = s / 10;
        } else {                                // Split 32-bit into two 16-bit divu ops
            uint16_t hi = u >> 16;
            uint16_t lo = (uint16_t)u;
            uint16_t qhi = hi / 10;
            uint16_t rhi = hi % 10;
            uint32_t tmp = ((uint32_t)rhi << 16) | lo;  // Fits in divu (hi < 10)
            uint16_t qlo, rem;
            __asm__ volatile (
                "divu #10,%0"
                : "+d"(tmp)
            );
            qlo = (uint16_t)tmp;
            rem = (uint16_t)(tmp >> 16);
            digits[len++] = '0' + rem;
            u = ((uint32_t)qhi << 16) | qlo;
        }
    } while (u);

    for (int i = len - 1; i >= 0; i--)
        *p++ = digits[i];

    *p = 0;
    return buf;
}

// Convert a hex string (up to 8 chars) to a 32-bit value
// Input:  string pointing to hex digits (no $ prefix expected)
// Output: 32-bit binary value
uint32_t hexBin(char *string)
{
    uint32_t result = 0;
    for(int i = 0; i < 8; i++)
    {
        uint8_t c = *string++;
        if(c >= 'A')
            c -= 7;
        c -= '0';
        result = (result << 4) | (c & 0xF);
    }
    return result;
}

// Convert a decimal string to a binary number (16-bit max)
// Input:  null-terminated decimal string
// Output: binary value
uint32_t decBin(char *string)
{
    uint32_t result = 0;
    while(*string)
    {
        result = result * 10 + (*string - '0');
        string++;
    }
    return result;
}

void sendSerial(char *string __asm("a0"))
{
    while(*string)
    {
        rs232_out(*string);
        string++;
    }
}

void Log(char *string,int value)
{
    #if DEBUG>=1
    sendSerial("\n\x0d");
    sendSerial(string);
    sendSerial("Value: ");
    sendSerial(binHex(value));
    sendSerial(" ");
    sendSerial(binString(value));
    sendSerial(" ");
    sendSerial(binDec(value));

    sendSerial("\n\x0d");
    #endif
}

int setBit(int value, int bit)
{
    return(value | (1 << (bit-1)));
}

int clearBit(int value, int bit)
{
    return((value & (~(1 << (bit-1)))));
}

int toggleBit(int value, int bit)
{
    return(value ^ (1 << (bit -1)));
}

void initIRQ3(int code)
{
    Log("IRQ: ",code);
    //*(volatile APTR *) + 0x6c = code;
}

// WaitPressed: waits until any button is pressed (or $ffff-iteration timeout)
// waitShort() normalizes ~640µs/iteration across CPU speeds (68000 through 060)
static void WaitPressed(void)
{
    uint32_t d7 = 0;
    for (;;) {
        d7++;
        if (d7 == 0xffff) return;
        waitShort();
        getInput();
        if (globals->BUTTON == 1) return;
    }
}

// WaitReleased: waits until all buttons are released (or timeout, marks stuck inputs)
// waitShort() normalizes ~640µs/iteration across CPU speeds (68000 through 060)
static void WaitReleased(void)
{
    uint32_t d7 = 0;
    for (;;) {
        rasterFeedback();
        d7++;
        waitShort();
        if (d7 == 0xffff) {
            globals->STUCKP1LMB = globals->P1LMB;
            globals->STUCKP2LMB = globals->P2LMB;
            globals->STUCKP1LMB = globals->P1LMB;   // mirrors original asm
            globals->STUCKP2RMB = globals->P2RMB;
            globals->STUCKP1MMB = globals->P1MMB;
            globals->STUCKP2MMB = globals->P2MMB;
            return;
        }
        getInput();
        if (globals->BUTTON == 0) return;
    }
}

void WaitButton(void)
{
    WaitPressed();
    WaitReleased();
}

void ClearBuffer(void)
{
    for (int i = 0; i <= 20; i++)   // dbf with d7=20 → 21 iterations
        getInput();
    globals->SerBufLen = 0;
    clearInput();
    globals->SerBufLen = 0;
}

void PrintCPU(void)
{
    print("\nCPU: ", GREEN);
    print((char *)globals->CPUPointer, GREEN);
    if (globals->CPUGen == 5) {     // 060 gen: also print revision number
        print(" Rev: ", GREEN);
        print(bindec(globals->CPU060Rev), GREEN);
    }
    print(" FPU: ", GREEN);
    print((char *)globals->FPUPointer, GREEN);
    print(" MMU: ", GREEN);
    if (globals->MMU != 0)
        print("NOT CHECKED", GREEN);
    else
        print("NO ", GREEN);
}

extern char BuiltdateTxt[];     // incbin in data.s, cannot be a C literal

void debugScreen(void)
{
    // Register dump header
    setPos(0, 3);
    print("Debugdata (Dump of CPU Registers D0-D7/A0-A7):", YELLOW);
    setPos(0, 3);

    // D registers
    print(binHex(globals->DebD0), GREEN);  print(" ", GREEN);
    print(binHex(globals->DebD1), GREEN);  print(" ", GREEN);
    print(binHex(globals->DebD2), GREEN);  print(" ", GREEN);
    print(binHex(globals->DebD3), GREEN);  print(" ", GREEN);
    print(binHex(globals->DebD4), GREEN);  print(" ", GREEN);
    print(binHex(globals->DebD5), GREEN);  print(" ", GREEN);
    print(binHex(globals->DebD6), GREEN);  print(" ", GREEN);
    print(binHex(globals->DebD7), GREEN);  print(" ", GREEN);

    // A registers
    print(binHex(globals->DebA0), YELLOW); print(" ", YELLOW);
    print(binHex(globals->DebA1), YELLOW); print(" ", YELLOW);
    print(binHex(globals->DebA2), YELLOW); print(" ", YELLOW);
    print(binHex(globals->DebA3), YELLOW); print(" ", YELLOW);
    print(binHex(globals->DebA4), YELLOW); print(" ", YELLOW);
    print(binHex(globals->DebA5), YELLOW); print(" ", YELLOW);
    print(binHex(globals->DebA6), YELLOW); print(" ", YELLOW);
    print(binHex(globals->DebA7), YELLOW); print(" ", YELLOW);
    print("\n\r", YELLOW);

    // SR and PC
    print("SR: ", YELLOW);
    print(binHexWord(globals->DebSR >> 16), GREEN);
    print(" ADR: ", YELLOW);
    uint8_t *pc = (uint8_t *)globals->DebPC;
    print(binHex(globals->DebPC), GREEN);
    print(" Content: ", YELLOW);

    // 20 bytes of content at PC address
    for(int i = 0; i < 20; i++)
    {
        print(binHexByte(*pc++), YELLOW);
        if((i + 1) % 4 == 0)
            printChar(' ', YELLOW);
    }
    print("\n\r", YELLOW);

    // Stack dump: 15 longwords starting 16 bytes above crash SP
    print("\n  Stack:  ", RED);
    uint32_t *sp = (uint32_t *)(globals->DebA7 + 16);
    for(int i = 0; i < 15; i++)
    {
        print(binHex(*sp++), CYAN);
        print(" ", CYAN);
    }

    // IRQ exception vectors (levels 1-7, vector table starts at 0x64)
    volatile uint32_t *ivec = (volatile uint32_t *)0x64;
    for(int irq = 1; irq <= 7; irq++)
    {
        print("\n\r", CYAN);
        print("IRQ Level ", YELLOW);
        print(binDec(irq), YELLOW);
        print(" Points to: ", YELLOW);
        uint32_t vecAddr = *ivec;
        uint8_t *vec = (uint8_t *)vecAddr;
        print(binHex(vecAddr), YELLOW);
        print(" Content: ", YELLOW);
        for(int i = 0; i < 16; i++)
        {
            print(binHexByte(*vec++), YELLOW);
            if((i + 1) % 4 == 0)
                printChar(' ', YELLOW);
        }
        ivec++;
    }

    print("\n\r", YELLOW);
    print("\n\r", YELLOW);

    // ROM presence checks
    print("Is $1114 readable at addr $0 (ROM still at $0): ", YELLOW);
    if(*(volatile uint16_t *)0x0 == 0x1114)
    {
        print("YES", RED);
        print("\n\r", RED);
    }
    else
    {
        print("NO ", GREEN);
        print("\n\r", GREEN);
    }

    print("Is $1114 readable at addr $f80000 (Real ROM addr): ", YELLOW);
    if(*(volatile uint16_t *)0xf80000 == 0x1114)
    {
        print("YES", GREEN);
        print("\n\r", GREEN);
    }
    else
    {
        print("NO ", RED);
        print("\n\r", RED);
    }

    PrintCPU();

    print("\nPoweronflags: ", YELLOW);
    print(binString(globals->PowerONStatus), YELLOW);
    print("  Builddate: ", GREEN);
    print(BuiltdateTxt, GREEN);
}

void errorScreenC(char *errorTitle __asm("a0"))
{
    clearScreen();
    print(errorTitle, RED);
    print("\n\r", RED);
    print("\x02" "DiagROM CRASHED - Software/Hardware failure - Unexpected event", RED);
    debugScreen();
    print("\n\r", RED);
    print("\n\r", RED);
    print("\x02" "Press any key/mouse to continue", PURPLE);
    ClearBuffer();
    WaitButton();
}

void clearInput()
{
    globals->CurAddX=0;
    globals->CurSubX=0;
    globals->CurAddY=0;
    globals->CurSubY=0;
    globals->MOUSE=0;
    globals->BUTTON=0;
    globals->LMB=0;
    globals->P1LMB=0;
    globals->P2LMB=0;
    globals->RMB=0;
    globals->P1RMB=0;
    globals->P2RMB=0;
    globals->MMB=0;
    globals->P1MMB=0;
    globals->P2MMB=0;
    globals->key=0;
    globals->Serial=0;
    globals->GetCharData=0;
    globals->MBUTTON=0;
}

uint8_t getChar(void)
{
    uint8_t result = getCharKey();
    if (result == 0)
        result = getCharSerial();
    globals->GetCharData = result;
    return result;
}

/* C copies of EnglishKey / EnglishKeyShifted (currently defined in generic.s).
   To activate when generic.s is removed:
     - drop _EnglishKey:: / _EnglishKeyShifted:: alias labels from generic.s
     - rename these arrays to EnglishKey[] / EnglishKeyShifted[]
     - drop 'static const' so the linker exports them */
static const uint8_t EnglishKey_C[] = {
    /* 0x00-0x0F: number row */
    ' ','1','2','3','4','5','6','7','8','9','0','-','=','|',' ','0',
    /* 0x10-0x1C: qwerty row */
    'q','w','e','r','t','y','u','i','o','p','[',']',' ',
    /* 0x1D-0x2A: asdf row */
    '1','2','3','a','s','d','f','g','h','j','k','l',';','`',
    /* 0x2B-0x3B: zxcv row */
    ' ',' ','4','5','6',' ','z','x','c','v','b','n','m',',','.','/',' ',
    /* 0x3C-0x40: numpad ./789 and space bar */
    '.','7','8','9',' ',
    8,           /* 0x41 backspace */
    9,           /* 0x42 Tab */
    0x0d,        /* 0x43 Return */
    0x0a,        /* 0x44 Enter */
    27,          /* 0x45 esc */
    127,         /* 0x46 del */
    ' ',' ',' ', /* 0x47-0x49 Undefined */
    '-',         /* 0x4A numpad - */
    ' ',         /* 0x4B Undefined */
    30,          /* 0x4C Up */
    31,          /* 0x4D down */
    28,          /* 0x4E forward */
    29,          /* 0x4F backward */
    '1','2','3','4','5','6','7','8','9','0', /* 0x50-0x59 f1-f10 */
    '(',')','/','*','+',                      /* 0x5A-0x5E numpad */
    0            /* 0x5F Help */
};

static const uint8_t EnglishKeyShifted_C[] = {
    /* 0x00-0x0F: number row shifted */
    '~','!','@','#','$','%','^','&',' ','(',')','_','+','|',' ','0',
    /* 0x10-0x1C: qwerty row shifted */
    'Q','W','E','R','T','Y','U','I','O','P','{','}',' ',
    /* 0x1D-0x2A: asdf row shifted (0x2A = '"', dc.b 34 in asm) */
    '1','2','3','A','S','D','F','G','H','J','K','L',':','"',
    /* 0x2B-0x3B: zxcv row shifted */
    ' ',' ','4','5','6',' ','Z','X','C','V','B','N','M','<','>','?',' ',
    /* 0x3C-0x4B: numpad ./789, space bar, bs/tab/ret/enter/esc/del/undef×3, numpad-, undef */
    '.','7','8','9',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','-',' ',
    1,           /* 0x4C Up shifted */
    2,           /* 0x4D down shifted */
    0,           /* 0x4E forward shifted */
    0,           /* 0x4F backward shifted */
    0,0,0,0,0,0,0,0,0,0, /* 0x50-0x59 f1-f10 shifted */
    '(',')','/','*','+',  /* 0x5A-0x5E numpad */
    0            /* 0x5F Help shifted */
};

void convertKey(uint8_t keycode __asm("d0"), uint8_t *keymap __asm("a0"), uint8_t *keymapShifted __asm("a1"))
{
    uint8_t unshifted = keymap[keycode];
    globals->keypressed[0] = unshifted;
    globals->keyresult = unshifted;
    uint8_t shifted = keymapShifted[keycode];
    globals->keypressedshifted[0] = shifted;
    if (globals->keyshift)
        globals->keyresult = shifted;
}

// ─────── Input subsystem ───────

void getSerial(void)
{
    if (globals->SerialSpeed == 0 || globals->SerialSpeed == 5)
        return;
    globals->SerData = 0;
    readSerial();
    if (globals->SerBufLen == 0)
        return;
    globals->Serial = globals->SerBuf[0];
    for (int i = 0; i < 255; i++)
        globals->SerBuf[i] = globals->SerBuf[i + 1];
    globals->SerBufLen--;
    globals->SerBuf[255] = 0;
}

uint8_t getCharSerial(void)
{
    globals->Serial = 0;
    getSerial();

    if (!globals->SerAnsiFlag) {
        uint8_t c = globals->Serial;
        if (c == 0x1b) {
            globals->SerAnsiFlag = 1;
            globals->SerAnsiChecks = 0;
            globals->Serial = 0;
            return 0;
        }
        if (c == 0x0d) return 0x0a;
        return c;
    }

    // ANSI mode
    uint8_t c = globals->Serial;
    if (c == 0 || c == 0x1b) {
        c = 0;
        globals->SerAnsiChecks++;
        waitShort();  // Normalize timeout across CPU speeds (~640µs per check)
        if (globals->SerAnsiChecks >= 0x0f) {
            togglePwrLED();
            globals->SerAnsiChecks = 0;
            c = 0x1b;
            globals->SerAnsiFlag = 0;
        }
        globals->Serial = c;
        return c;
    }

    if (c < 32) { globals->Serial = 0; return 0; }

    if (globals->SerAnsi35Flag) {
        globals->SerAnsi35Flag = 0;
        if (c == 0x7e) { globals->SerAnsiFlag = 0; globals->skipnextkey = 1; c = 1; }
        else           { globals->SerAnsiFlag = 0; return 0; }  // asm rts-without-POP bug: return 0
    } else if (globals->SerAnsi36Flag) {
        globals->SerAnsi36Flag = 0;
        if (c == 0x7e) { globals->SerAnsiFlag = 0; globals->skipnextkey = 1; c = 2; }
        else           { globals->SerAnsiFlag = 0; return 0; }
    } else if (c == 0x38)               { globals->SerAnsiFlag = 0; globals->skipnextkey = 1; c = 1; }
    else if (c == 0x36)                 { globals->SerAnsiFlag = 0; globals->skipnextkey = 1; c = 2; }
    else if (c == 0x41)                 { globals->SerAnsiFlag = 0; c = 30; }  // UP
    else if (c == 0x42)                 { globals->SerAnsiFlag = 0; c = 31; }  // DOWN
    else if (c == 0x43)                 { globals->SerAnsiFlag = 0; c = 28; }  // RIGHT
    else if (c == 0x44)                 { globals->SerAnsiFlag = 0; c = 29; }  // LEFT
    else if (c == 0x35)                 { globals->SerAnsi35Flag = 1; c = 0; }
    else                                { c = 0; }

    globals->Serial = c;
    return c;
}

uint8_t getCharKey(void)
{
    globals->keyresult = 0;
    getKey();
    if (globals->keynew) {
        convertKey(globals->key, (uint8_t *)globals->keymap, (uint8_t *)globals->keymapShifted);
        if (globals->skipnextkey) {
            globals->BUTTON = 0;
            globals->keyresult = 0;
            globals->skipnextkey = 0;
        }
    }
    return globals->keyresult;
}

uint32_t getInput(void)
{
    clearInput();
    uint32_t d0 = getMouseData();
    uint32_t d1 = d0;

    if (globals->DISPAULA) {
        // Paula bad: replicate asm .paulabad behavior (stores mouse flags as GetCharData)
        globals->GetCharData = (uint8_t)d0;
        globals->BUTTON = 1;
        d1 |= (1 << 3);
    } else {
        uint8_t serial = getCharSerial();
        if (serial != 0) {
            globals->GetCharData = serial;
            globals->BUTTON = 1;
            d0 = d1;
            d1 |= (1 << 3);
        }
    }
    d0 = d1;

    d1 = d0;
    if (globals->OVLErr == 1) {
        // CIA broken: skip keyboard; check if any flags set (replicates .noovl merge path)
        if (d0 != 0) {
            globals->BUTTON = 1;
            globals->GetCharData = (uint8_t)d0;
            d0 = d1 | (1 << 2);
        } else {
            d0 = d1;
        }
    } else {
        uint8_t key = getCharKey();
        if (key != 0) {
            globals->BUTTON = 1;
            globals->GetCharData = key;
            d0 = d1 | (1 << 2);
        } else {
            d0 = d1;
        }
    }

    globals->InputRegister = d0;
    return d0;
}

__attribute__((optimize("O2")))
extern char UnderDevTxt[];

static uint8_t hexNibble(uint8_t c)
{
    if (c >= 'A') c -= 7;
    return c - 0x30;
}

uint8_t getHex(uint8_t c __asm("d0"))
{
    if (c >= '0' && c <= '9') return c;
    c &= ~0x20u;                    // bclr #5 — make uppercase
    if (c >= 'A' && c <= 'F') return c;
    if (c == 8) return c;           // backspace
    if (c == '\r') c = '\n';        // CR → LF (matching asm)
    if (c == '\n') return c;
    if (c == 27) return c;          // ESC
    return 0;
}

uint8_t getDec(uint8_t c __asm("d0"))
{
    if (c >= '0' && c <= '9') return c;
    if (c == 8) return c;           // backspace
    if (c == '\r') c = '\n';
    if (c == '\n') return c;
    if (c == 27) return c;          // ESC
    return 0;
}

uint32_t strLen(char *str __asm("a0"))
{
    uint32_t len = 0;
    uint8_t c;
    while ((c = (uint8_t)*str++) != 0) {
        if (c != 2)                 // skip centre-command byte
            len++;
    }
    return len;
}

void sameRow(void)
{
    BYTE(globals->Xpos) = 0;
    rs232_out('\r');
}

void defaultVars(void)
{
    globals->CheckMemEditScreenAdr = (void *)*(volatile uint32_t *)0x400;
    globals->skipnextkey = 0;
}

void devPrint(void)
{
    setPos(0, 25);
    print(UnderDevTxt, RED);
    setPos(0, 0);
}

uint8_t hexByteToBin(char *str __asm("a0"))
{
    uint8_t hi = hexNibble((uint8_t)str[0]);
    uint8_t lo = hexNibble((uint8_t)str[1]);
    return (uint8_t)((hi << 4) | lo);
}

uint32_t getMouse(void)
{
    globals->BUTTON  = 0;
    globals->LMB     = 0;
    globals->P1LMB   = 0;
    globals->P2LMB   = 0;
    globals->RMB     = 0;
    globals->P1RMB   = 0;
    globals->P2RMB   = 0;
    globals->MBUTTON = 0;
    uint32_t result = getMouseData();
    globals->InputRegister = result;
    return result;
}

void waitPressed(void)
{
    for (uint32_t i = 1; i < 0xffff; i++) {
        waitShort();
        getInput();
        if (globals->BUTTON == 1)
            return;
    }
    // timeout — original had dead code here; just return
}

void waitReleased(void)
{
    for (uint32_t i = 1; i < 0xffff; i++) {
        rasterFeedback();
        waitShort();
        getInput();
        if (globals->BUTTON == 0)
            return;
    }
    // timeout — mark stuck inputs
    globals->STUCKP1LMB = globals->P1LMB;
    globals->STUCKP2LMB = globals->P2LMB;
    globals->STUCKP2RMB = globals->P2RMB;
    globals->STUCKP1MMB = globals->P1MMB;
    globals->STUCKP2MMB = globals->P2MMB;
}

uint8_t makePrintable(uint8_t c __asm("d0"))
{
    return (c <= ' ') ? ' ' : c;
}

char *binStringByte(uint8_t val __asm("d0"))
{
    char *out = (char *)globals->binstringoutput;
    for (int bit = 7; bit >= 0; bit--)
        *out++ = (val & (1 << bit)) ? '1' : '0';
    *out = 0;
    return (char *)globals->binstringoutput;
}


void copyMem(void *src __asm("a0"), uint32_t count __asm("d0"), void *dst __asm("a1"))
{
    memcpy(dst, src, count);
}

int32_t toKB(int32_t val __asm("d0"))
{
    return val >> 10;
}



uint32_t getChip(uint32_t size __asm("d0"))
{
    globals->GetChipAddr = 0;
    if (globals->TotalChip == 0)
        return 0;
    if (globals->ChipUnreserved < size) {
        globals->GetChipAddr = (void *)1;
        return 1;                              // "not enough" — asm callers check for this
    }
    uint32_t addr = (uint32_t)globals->ChipUnreservedAddr - size;
    globals->GetChipAddr = (void *)(uintptr_t)addr;
    uint32_t *p = (uint32_t *)(uintptr_t)addr;
    uint32_t n = (size >> 2) + 1;             // dbf runs size/4 + 1 times
    for (uint32_t i = 0; i < n; i++) p[i] = 0;
    return addr;
}

void *getMemory(uint32_t size __asm("d0"))
{
    uint32_t fastStart = (uint32_t)globals->FastStart;
    uint32_t fastEnd   = (uint32_t)globals->FastEnd;
    uint32_t chipStart = (uint32_t)globals->ChipStart;
    uint32_t chipEnd   = (uint32_t)globals->ChipEnd;
    uint32_t start, end;

    if (fastStart != 0 && (fastEnd - fastStart) >= size) {
        start = fastStart;
        end   = fastEnd;
    } else if (chipStart == 0 || (chipEnd - chipStart) < size) {
        globals->MemAdr = 0;
        return 0;
    } else {
        start = chipStart;
        end   = chipEnd;
    }

    uint32_t addr = (globals->WorkOrder == 0) ? (end - size - 1) : start;
    globals->MemAdr = (void *)(uintptr_t)addr;
    return globals->MemAdr;
}

void deleteLine(uint8_t line __asm("d0"))
{
    static const char dellineStr[] = {27, '[', '1', 'M', 0};
    uint8_t savedX = BYTE(globals->Xpos);
    uint8_t savedY = BYTE(globals->Ypos);

    setPos(0, line);
    print((char *)dellineStr, YELLOW);

    if (!globals->NoDraw) {
        uint32_t offset = (uint32_t)line * 640;
        uint32_t *p1 = (uint32_t *)((uint8_t *)globals->Bpl1Ptr + offset);
        uint32_t *p2 = (uint32_t *)((uint8_t *)globals->Bpl2Ptr + offset);
        uint32_t *p3 = (uint32_t *)((uint8_t *)globals->Bpl3Ptr + offset);

        p1[0] = p2[0] = p3[0] = 0xFFFFFFFF;  // mark first longword (matches original asm)

        uint32_t lw = (uint32_t)(31 - line) * 160;
        for (uint32_t i = 0; i <= lw; i++) {  // dbf: lw+1 iterations
            p1[i] = p1[i + 160];
            p2[i] = p2[i + 160];
            p3[i] = p3[i + 160];
        }
        uint32_t *e1 = p1 + lw + 1;
        uint32_t *e2 = p2 + lw + 1;
        uint32_t *e3 = p3 + lw + 1;
        for (int i = 0; i < 160; i++) {       // dbf #159: 160 iterations
            e1[i] = e2[i] = e3[i] = 0;
        }
    }

    setPos(savedX, savedY);
}

void runCode(void *routine __asm("a0"), uint32_t length __asm("d0"))
{
    // RAM execution disabled in original asm ("kuk. disable rum!!")
    // Always execute directly from the given location (RunCodeInRom path)
    globals->RunCodeStart = routine;
    globals->RunCodeEnd   = (void *)((uint8_t *)routine + length + 4);
    ((void (*)(void))routine)();
}

static void inputPutCursor(char *buf, uint8_t pos)
{
    uint8_t bx = globals->CheckMemManualX;
    uint8_t by = globals->CheckMemManualY;
    setPos(bx + pos, by);
    printChar((uint8_t)buf[pos], R_BLUE);
    setPos(bx + pos, by);
}

int32_t inputHexNum(void *defaultAddr __asm("a0"))
{
    globals->CheckMemManualX = BYTE(globals->Xpos);
    globals->CheckMemManualY = BYTE(globals->Ypos);

    // Format default address as hex, skip '$' prefix
    char *hexStr = binHex((uint32_t)(uintptr_t)defaultAddr) + 1;

    // Clear 9-byte workspace (indices 0..8)
    char *ws = (char *)globals->CheckMemStartAdrTxt;
    for (int i = 0; i <= 8; i++) ws[i] = 0;

    // Copy 8 hex chars, stripping leading zeros
    char *dst = ws;
    int hadNonZero = 0;
    for (int i = 0; i < 8; i++) {
        char c = *hexStr++;
        if (c == '0' && !hadNonZero) continue;
        *dst++ = c;
        hadNonZero = 1;
    }

    print(ws, WHITE);
    int d6 = (int)strLen(ws);  // cursor position (= string length)
    int d7 = -1;               // force cursor update on first iteration

    for (;;) {
        getMouse();
        if (globals->RMB == 1 || globals->LMB == 1) return -1;
        waitShort();
        uint8_t ch = getChar();
        waitLong();

        if (ch == 'x') {
            for (int i = 0; i <= 7; i++) ws[i] = ' ';
            d6 = d7 = 0;
            inputPutCursor(ws, 0);
            print("        ", WHITE);
            inputPutCursor(ws, 0);
            continue;  // d6==d7, no cursor update needed
        }

        uint8_t raw = ch;
        if (ch != 0x7f) ch = getHex(ch);  // serial BS bypasses filter

        if (ch != 0) {
            if (ch == 0x1b) {
                return -1;
            } else if (ch == '\n') {
                if (d6 == 0) return -1;
                inputPutCursor(ws, (uint8_t)d7);
                printChar(' ', WHITE);
                setPos(globals->CheckMemManualX - 1, globals->CheckMemManualY);
                // Left-pad with zeros so hexBin always sees 8 digits
                if (d6 < 8) {
                    int pad = 8 - d6;
                    for (int i = 7; i >= pad; i--) ws[i] = ws[i - pad];
                    for (int i = 0; i < pad; i++) ws[i] = '0';
                }
                return (int32_t)hexBin(ws);
            } else if (ch == 0x08 || raw == 0x7f) {
                ws[d6] = 0;
                if (d6 > 0) {
                    ws[--d6] = ' ';
                    printChar(' ', WHITE);
                }
            } else {
                if (d6 < 8) {
                    ws[d6++] = ch;
                    printChar(ch, WHITE);
                }
            }
        }

        if (d6 != d7) {
            d7 = d6;
            inputPutCursor(ws, (uint8_t)d7);
        }
    }
}

int32_t inputDecNum(uint32_t defaultVal __asm("a0"))
{
    globals->CheckMemManualX = BYTE(globals->Xpos);
    globals->CheckMemManualY = BYTE(globals->Ypos);

    // Format default value as decimal string
    char *decStr = bindec((int)defaultVal);

    // Clear 9-byte workspace
    char *ws = (char *)globals->CheckMemStartAdrTxt;
    for (int i = 0; i <= 8; i++) ws[i] = 0;

    // Copy up to 8 chars until null
    char *dst = ws;
    for (int i = 0; i < 8; i++) {
        char c = *decStr++;
        if (c == 0) break;
        *dst++ = c;
    }

    print(ws, WHITE);
    int d6 = (int)strLen(ws);
    int d7 = -1;

    for (;;) {
        getMouse();
        if (globals->RMB == 1 || globals->LMB == 1) return -1;
        waitShort();
        uint8_t ch = getChar();
        waitLong();

        if (ch == 'x') {
            for (int i = 0; i <= 7; i++) ws[i] = ' ';
            d6 = d7 = 0;
            inputPutCursor(ws, 0);
            print("        ", WHITE);
            inputPutCursor(ws, 0);
            continue;
        }

        uint8_t raw = ch;
        if (ch != 0x7f) ch = getDec(ch);

        if (ch != 0) {
            if (ch == 0x1b) {
                return -1;
            } else if (ch == '\n') {
                if (d6 == 0) return -1;
                inputPutCursor(ws, (uint8_t)d7);
                printChar(' ', WHITE);
                return (int32_t)decBin(ws);
            } else if (ch == 0x08 || raw == 0x7f) {
                ws[d6] = 0;
                if (d6 > 0) {
                    ws[--d6] = ' ';
                    printChar(' ', WHITE);
                }
            } else {
                if (d6 < 8) {
                    ws[d6++] = ch;
                    printChar(ch, WHITE);
                }
            }
        }

        if (d6 != d7) {
            d7 = d6;
            inputPutCursor(ws, (uint8_t)d7);
        }
    }
}
