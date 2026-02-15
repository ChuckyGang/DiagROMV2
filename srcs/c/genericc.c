#include "generic.h"
#include <stdbool.h>
#include <hardware/cia.h>
#include "globalvars.h"

void readSerial();

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

void clearScreen()
{

}

void setPos(uint32_t xPos, uint32_t yPos)
{
    *(uint8_t *)&globals->Xpos = (uint8_t)xPos;  // Byte access to match asm move.b
    *(uint8_t *)&globals->Ypos = (uint8_t)yPos;
    sendSerial("\x1b[");
    sendSerial(binDec(yPos+1));     // ANSI row first
    rs232_out(';');
    sendSerial(binDec(xPos+1));     // ANSI column second
    rs232_out('H');
}

void putChar(char character, uint8_t color, uint8_t xPos, uint8_t yPos)
{
    extern const uint8_t RomFont[];
    uint8_t *bpl[] = { globals->Bpl1Ptr, globals->Bpl2Ptr, globals->Bpl3Ptr };
    uint8_t xor = 0;

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
        color -= 7;                 // to get the correct color
        xor = 0xff;
    }
    int charadd = character*8;      // Find where the wanted character is in the font
    int bpladd = (yPos*640)+xPos;   // locate where in ram the character should be printed on the screen

    for (int r = 0; r < 8; r++)     // Print the character on screen
    {
        for (int p = 0; p < 3; p++) {
            if(color & (1<<p))
            {
                *(bpl[p]+(r*80)+bpladd) = xor^RomFont[r+charadd];
            }
            else
            {
                *(bpl[p]+(r*80)+bpladd) = 0;
            }
        }
    }

}

// Convert a signed 16-bit integer to a null-terminated decimal ASCII string.
// C version of oldbindec in generic.s
// Input:  value = signed 16-bit number (-32768 to 32767)
// Output: pointer to string stored in globals->bindecoutput
char *binDec(int16_t value)
{
    char *buf = (char *)globals->bindecoutput;  // Output buffer in global vars
    char *p = buf;                              // Write pointer
    int16_t v = value;

    if (v < 0) {           // Handle negative numbers
        *p++ = '-';         // Prefix with minus sign
        v = -v;             // Work with the absolute value
    }

    // Extract digits from least significant to most significant
    // (e.g. 123 produces '3','2','1' in digits[])
    char digits[6];         // Max 5 digits for 16-bit + safety
    int len = 0;
    do {
        digits[len++] = '0' + (v % 10);    // Store lowest digit as ASCII
        v /= 10;                            // Remove lowest digit
    } while (v);

    // Reverse digits into the output buffer (most significant first)
    for (int i = len - 1; i >= 0; i--)
        *p++ = digits[i];

    *p = 0;                 // Null-terminate the string
    return buf;
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

void sendSerial(char *string)
{
    while(*string)
    {
        rs232_out(*string);
        string++;
    }
}

void rs232_out(char character)
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
    sendSerial(binhex(value));
    sendSerial(" ");
    sendSerial(binstring(value));
    sendSerial(" ");
    sendSerial(bindec(value));

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