#include "generic.h"
#include <stdbool.h>
#include <hardware/cia.h>
#include "globalvars.h"

void readSerial();
void print(char *string __asm("a0"), uint8_t color __asm("d1"));
char *binHex(uint32_t value __asm("d0"));

// Byte access macros for uint32_t fields that asm accesses with move.b (big-endian MSB)
#define BYTE(field) (*(volatile uint8_t *)&(field))

void getHWReg(VARS)
{
    globals->BLTDDAT = custom->bltddat;
    globals->DMACONR = custom->dmaconr;
    globals->VPOSR = custom->vposr;
    globals->VHPOSR = custom->vhposr;
    globals->DSKDATR = custom->dskdatr;
    globals->JOY0DAT = custom->joy0dat;
    globals->JOY1DAT = custom->joy1dat;
    globals->CLXDAT = custom->clxdat;
    globals->ADKCONR = custom->adkconr;
    globals->POT0DAT = custom->pot0dat;
    globals->POT1DAT = custom->pot1dat;
    globals->POTINP = custom->potinp;
    globals->SERDATR = custom->serdatr;
    globals->DSKBYTR = custom->dskbytr;
    globals->INTENAR = custom->intenar;
    globals->INTREQR = custom->intreqr;
    globals->DENISEID = custom->deniseid;
    globals->HHPOSR = custom->hhposr;
 }

void clearScreen()              // Clears the screen
{
    if(!globals->NoDraw)
    {
        uint32_t *p0 = (uint32_t *)globals->Bpl1Ptr;
        uint32_t *p1 = (uint32_t *)globals->Bpl2Ptr;
        uint32_t *p2 = (uint32_t *)globals->Bpl3Ptr;
        for(int i=0; i<(20*256); i++)
        {
            *p0++ = 0;
            *p1++ = 0;
            *p2++ = 0;
        }
    }
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

void putChar(char character __asm("d0"), uint8_t color __asm("d1"), uint8_t xPos __asm("d2"), uint8_t yPos __asm("d3"))
{
    extern const uint8_t RomFont[];
    uint8_t xormask = 0;

    if(character==0x1)      // If char is 0x1 then do not print, send space to serialport
    {
        rs232_out(' ');
        return;
    }
    if(globals->NoDraw)     // if we are in "NoDraw" mode.  do not print on screen,
    {
        TOGGLEPWR;
        custom->color[0]=character; // to make user understand that there are SOME acticvity send charvalue as backrroundcolor
        rs232_out(character);       // Send char to serialport.
        return;
    }
    rs232_out(character);           // Send character to serialport
    character -= 32;                // Subtract 32 to the char we got.  as font starts with a space
    if(color>7)                     // If color is more then 8, make it reversed
    {
        color -= 8;                 // to get the correct color
        xormask = 0xff;
    }

    // character<<3 instead of character*8 (shift vs 70-cycle MULU on 68000)
    const uint8_t *font = &RomFont[(uint8_t)character << 3];

    // yPos*640 = yPos*512 + yPos*128, all via shifts (avoids MULU)
    uint16_t bpladd = ((uint16_t)yPos << 9) + ((uint16_t)yPos << 7) + xPos;

    // Load bitplane pointers once, pre-add bpladd offset
    uint8_t *p0 = (uint8_t *)globals->Bpl1Ptr + bpladd;
    uint8_t *p1 = (uint8_t *)globals->Bpl2Ptr + bpladd;
    uint8_t *p2 = (uint8_t *)globals->Bpl3Ptr + bpladd;

    // Pre-compute which planes are active (avoids branch per plane per row)
    uint8_t c0 = (color & 1) ? 0xff : 0x00;
    uint8_t c1 = (color & 2) ? 0xff : 0x00;
    uint8_t c2 = (color & 4) ? 0xff : 0x00;

    for (int16_t r = 0; r < 8; r++)     // Print the character on screen
    {
        uint8_t glyph = xormask ^ *font++;
        // Mask glyph with plane enable: either full glyph or 0
        *p0 = glyph & c0;
        *p1 = glyph & c1;
        *p2 = glyph & c2;
        p0 += 80;                       // Next row in bitplane (stride = 80 bytes)
        p1 += 80;
        p2 += 80;
    }
}

void romChecksum()
{
    extern uint32_t checksums[];
    extern uint32_t endchecksums;

    print("\n\nDoing ROM Checksumtest: (64K blocks, Green OK, Red Failed)\n", 3);

    uint32_t *csPtr = checksums;
    uint32_t csStart = (uint32_t)checksums;
    uint32_t csEnd = (uint32_t)&endchecksums;

    for(int block = 0; block < 8; block++)
    {
        uint32_t *rom = (uint32_t *)(0xF80000 + block * 0x10000);
        uint32_t sum = 0;

        // Sum entire 64K block - tight loop, no branch per word
        for(int i = 0; i < 0x4000; i += 8)
        {
            sum += rom[0]; sum += rom[1]; sum += rom[2]; sum += rom[3];
            sum += rom[4]; sum += rom[5]; sum += rom[6]; sum += rom[7];
            rom += 8;
        }

        // Subtract out any checksum longwords that fell inside this block
        uint32_t blockStart = 0xF80000 + block * 0x10000;
        uint32_t blockEnd = blockStart + 0x10000;
        if(csStart < blockEnd && csEnd > blockStart)
        {
            uint32_t *cs = (uint32_t *)csStart;
            while((uint32_t)cs < csEnd)
            {
                if((uint32_t)cs >= blockStart && (uint32_t)cs < blockEnd)
                    sum -= *cs;
                cs++;
            }
        }

        uint8_t color;
        if(sum == *csPtr++)
            color = 2;                           // Green = OK
        else
            color = 1;                           // Red = Failed

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

void printChar(char character __asm("d0"), uint8_t color __asm("d1"))            // Prints a char on screen, handles X, Y postiion, scrolling etc.
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
            invCol=color-8;
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
        if(BYTE(globals->Ypos)>26)
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

 void initSerial()
 {
    if(globals->NoSerial==1)        // If No Serial is 1 exit
    {
        return;
    }

    custom->intena = 0x4000;                                     // Clear master interrupt enable
    uint16_t serialSpeed = globals->SerialSpeed;
    static const int baudRates[] = {0,1492,373,187,94,30,0,0};
    custom->serper = baudRates[serialSpeed];                     // Set serial port baud rate

    *(volatile uint8_t *)0xbfd000 = 0x4f;                        // Set DTR high (CIAB PRA)
    custom->intena = 0x0801;                                     // Clear TBE + EXTER interrupt enable bits
    custom->intreq = 0x0801;                                     // Clear TBE + EXTER interrupt request flags
}

void sendSerial(char *string __asm("a0"))
{
    while(*string)
    {
        rs232_out(*string);
        string++;
    }
}

void rs232_out(char character __asm("d0"))
{
    if(globals->SerialSpeed == 0 || globals->SerialSpeed == 5 || globals->NoSerial == 1)
        return;

    readSerial();                                    // Poll serial input while we're here

    // Wait for TSRE (bit 13) BEFORE writing - must confirm shift register is idle
    // (matching original asm logic: wait-before-write, not write-then-wait)
    uint32_t timeout = 0x90000;                      // Timeout counter - no timers available
    while(timeout > 0)
    {
        (void)(*(volatile uint8_t *)0xbfe001);       // Byte read from CIA - slow bus, used purely as delay
        timeout--;

        if(custom->serdatr & (1 << 13))              // Check TBE bit (bit 13) - transmit buffer empty?
            break;
    }

    custom->serdat = 0x0100 | (uint8_t)character;    // Send byte (bit 8 = stop bit, lower 8 = data)
    custom->intreq = 0x0001;                         // Clear TBE interrupt flag
}

void readSerial()
{
    if(globals->SerialSpeed == 0 || globals->SerialSpeed == 5)
        return;

    uint16_t serdatr = custom->serdatr;             // Read SERDATR ($dff018)
    uint8_t data = (uint8_t)serdatr;                 // Lower 8 bits = received byte

    if(data != globals->OldSerial)                   // Change from last scan?
    {
        // New char detected
    }
    else if(!(serdatr & (1 << 14)))                  // Check RBF bit (bit 14) - buffer full?
    {
        return;                                      // No new data, exit
    }

    globals->SerData = 1;                            // Flag that we have serial data
    globals->OldSerial = data;                       // Store current byte
    custom->intreq = 0x0800;                         // Clear RBF bit in INTREQ ($dff09c)
    custom->intreq = 0x0800;                         // Write twice (hardware quirk)
    globals->BUTTON = 1;                             // Signal a "button" press

    uint8_t bufpos = globals->SerBufLen;             // Current buffer position
    globals->SerBufLen = bufpos + 1;                 // Increment buffer length
    globals->SerBuf[bufpos] = data;                  // Store byte in buffer
}

static inline bool LMB_is_down()
{
    volatile struct CIA* ciaa = (struct CIA*)0xbfe001;
    return ! (ciaa->ciapra & CIAF_GAMEPORT0); // active low
}

static inline bool LMB_is_up()
{
    return !LMB_is_down();
}

void PAUSEC()
{
    do { custom->color[0]=custom->vhposr; } while(LMB_is_up());
    do { } while(LMB_is_down());
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
static void WaitPressed(void)
{
    uint32_t d7 = 0;
    for (;;) {
        d7++;
        if (d7 == 0xffff) return;
        GetInput();
        if (globals->BUTTON == 1) return;
    }
}

// WaitReleased: waits until all buttons are released (or timeout, marks stuck inputs)
static void WaitReleased(void)
{
    uint32_t d7 = 0;
    for (;;) {
        *(volatile uint8_t *)0xdff180 = *(volatile uint8_t *)0xdff006;
        d7++;
        if (d7 == 0xffff) {
            globals->STUCKP1LMB = globals->P1LMB;
            globals->STUCKP2LMB = globals->P2LMB;
            globals->STUCKP1LMB = globals->P1LMB;   // mirrors original asm
            globals->STUCKP2RMB = globals->P2RMB;
            globals->STUCKP1MMB = globals->P1MMB;
            globals->STUCKP2MMB = globals->P2MMB;
            return;
        }
        GetInput();
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
        GetInput();
    globals->SerBufLen = 0;
    ClearInput();
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