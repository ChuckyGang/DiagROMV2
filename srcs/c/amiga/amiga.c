// Amiga platform implementation
// All Amiga custom chipset / CIA hardware access lives here.

#include "c/generic.h"
#include "c/platform.h"
#include <stdbool.h>
#include <hardware/custom.h>
#include <hardware/cia.h>

#define custom ((volatile struct Custom*)0xdff000)

// Byte access macros for uint32_t fields that asm accesses with move.b (big-endian MSB)
#define BYTE(field) (*(volatile uint8_t *)&(field))

// ---------------------------------------------------------------------------
// HAL helpers (called from shared code)
// ---------------------------------------------------------------------------

void togglePwrLED(void)
{
    (*(volatile unsigned char *)0xbfe001) ^= (1 << 1);
}

void clearDisplay(void)
{
    uint32_t* p0 = (uint32_t *)globals->Bpl1Ptr;
    uint32_t* p1 = (uint32_t *)globals->Bpl2Ptr;
    uint32_t* p2 = (uint32_t *)globals->Bpl3Ptr;
    for (int i = 0; i < (20 * 256); i++)
    {
        *p0++ = 0;
        *p1++ = 0;
        *p2++ = 0;
    }
}

void swapVideoMode(void)
{
    *(volatile uint16_t *)0xdff180 = 0xfff;        // white flash
    globals->SCRNMODE ^= 0x20;
    globals->NTSC = (globals->SCRNMODE & 0x20) ? 0 : 1;  // 0x20=PAL, 0x00=NTSC
    *(volatile uint16_t *)0xdff1dc = globals->SCRNMODE;  // BEAMCON0
}

void rasterFeedback(void)
{
    *(volatile uint8_t *)0xdff180 = *(volatile uint8_t *)0xdff006;
}

// ---------------------------------------------------------------------------
// Platform functions (same name as shared declarations in generic.h)
// ---------------------------------------------------------------------------

void getHWReg(void)
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

void putChar(char character __asm("d0"), uint8_t color __asm("d1"), uint8_t xPos __asm("d2"), uint8_t yPos __asm("d3"))
{
    extern const uint8_t RomFont[];
    uint8_t xormask = 0;

    if (character == 0x1)      // If char is 0x1 then do not print, send space to serialport
    {
        rs232_out(' ');
        return;
    }
    if (globals->NoDraw)     // if we are in "NoDraw" mode.  do not print on screen,
    {
        togglePwrLED();
        custom->color[0] = character; // to make user understand that there are SOME acticvity send charvalue as backrroundcolor
        rs232_out(character);       // Send char to serialport.
        return;
    }
    rs232_out(character);           // Send character to serialport
    character -= 32;                // Subtract 32 to the char we got.  as font starts with a space
    if (color > 7)                  // If color is more then 8, make it reversed
    {
        color -= 8;                 // to get the correct color
        xormask = 0xff;
    }

    // character<<3 instead of character*8 (shift vs 70-cycle MULU on 68000)
    const uint8_t* font = &RomFont[(uint8_t)character << 3];

    // yPos*640 = yPos*512 + yPos*128, all via shifts (avoids MULU)
    uint16_t bpladd = ((uint16_t)yPos << 9) + ((uint16_t)yPos << 7) + xPos;

    // Load bitplane pointers once, pre-add bpladd offset
    uint8_t* p0 = (uint8_t *)globals->Bpl1Ptr + bpladd;
    uint8_t* p1 = (uint8_t *)globals->Bpl2Ptr + bpladd;
    uint8_t* p2 = (uint8_t *)globals->Bpl3Ptr + bpladd;

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

void scrollScreen(void)
{
    if (globals->NoDraw)
        return;

    uint32_t* bpl1 = (uint32_t *)globals->Bpl1Ptr;
    uint32_t* bpl2 = (uint32_t *)globals->Bpl2Ptr;
    uint32_t* bpl3 = (uint32_t *)globals->Bpl3Ptr;
    uint32_t n = (globals->BPLSIZE - 640) / 4;  // 4960 longwords; 160 lw per line

    // Unrolled 8x: 8 longword copies per iteration, branch overhead paid once per 32 bytes
    for (uint32_t i = 0; i < n; i += 8, bpl1 += 8, bpl2 += 8, bpl3 += 8)
    {
        bpl1[0]=bpl1[160]; bpl1[1]=bpl1[161]; bpl1[2]=bpl1[162]; bpl1[3]=bpl1[163];
        bpl1[4]=bpl1[164]; bpl1[5]=bpl1[165]; bpl1[6]=bpl1[166]; bpl1[7]=bpl1[167];
        bpl2[0]=bpl2[160]; bpl2[1]=bpl2[161]; bpl2[2]=bpl2[162]; bpl2[3]=bpl2[163];
        bpl2[4]=bpl2[164]; bpl2[5]=bpl2[165]; bpl2[6]=bpl2[166]; bpl2[7]=bpl2[167];
        bpl3[0]=bpl3[160]; bpl3[1]=bpl3[161]; bpl3[2]=bpl3[162]; bpl3[3]=bpl3[163];
        bpl3[4]=bpl3[164]; bpl3[5]=bpl3[165]; bpl3[6]=bpl3[166]; bpl3[7]=bpl3[167];
    }

    // Clear last line (160 longwords = 640 bytes), also unrolled 8x
    for (int i = 0; i < 160; i += 8, bpl1 += 8, bpl2 += 8, bpl3 += 8)
    {
        bpl1[0]=0; bpl1[1]=0; bpl1[2]=0; bpl1[3]=0;
        bpl1[4]=0; bpl1[5]=0; bpl1[6]=0; bpl1[7]=0;
        bpl2[0]=0; bpl2[1]=0; bpl2[2]=0; bpl2[3]=0;
        bpl2[4]=0; bpl2[5]=0; bpl2[6]=0; bpl2[7]=0;
        bpl3[0]=0; bpl3[1]=0; bpl3[2]=0; bpl3[3]=0;
        bpl3[4]=0; bpl3[5]=0; bpl3[6]=0; bpl3[7]=0;
    }
}

void initSerial(void)
{
    if (globals->NoSerial == 1)        // If No Serial is 1 exit
        return;

    custom->intena = 0x4000;                                     // Clear master interrupt enable
    uint16_t serialSpeed = globals->SerialSpeed;
    static const int baudRates[] = {0, 1492, 373, 187, 94, 30, 0, 0};
    custom->serper = baudRates[serialSpeed];                     // Set serial port baud rate

    *(volatile uint8_t *)0xbfd000 = 0x4f;                        // Set DTR high (CIAB PRA)
    custom->intena = 0x0801;                                     // Clear TBE + EXTER interrupt enable bits
    custom->intreq = 0x0801;                                     // Clear TBE + EXTER interrupt request flags
}

void rs232_out(char character __asm("d0"))
{
    if (globals->SerialSpeed == 0 || globals->SerialSpeed == 5 || globals->NoSerial == 1)
        return;

    readSerial();                                    // Poll serial input while we're here

    // Wait for TSRE (bit 13) BEFORE writing - must confirm shift register is idle
    // (matching original asm logic: wait-before-write, not write-then-wait)
    uint32_t timeout = 0x90000;                      // Timeout counter - no timers available
    while (timeout > 0)
    {
        (void)(*(volatile uint8_t *)0xbfe001);       // Byte read from CIA - slow bus, used purely as delay
        timeout--;

        if (custom->serdatr & (1 << 13))              // Check TBE bit (bit 13) - transmit buffer empty?
            break;
    }

    custom->serdat = 0x0100 | (uint8_t)character;    // Send byte (bit 8 = stop bit, lower 8 = data)
    custom->intreq = 0x0001;                         // Clear TBE interrupt flag
}

void readSerial(void)
{
    if (globals->SerialSpeed == 0 || globals->SerialSpeed == 5)
        return;

    uint16_t serdatr = custom->serdatr;             // Read SERDATR ($dff018)
    uint8_t data = (uint8_t)serdatr;                 // Lower 8 bits = received byte

    if (data != globals->OldSerial)                   // Change from last scan?
    {
        // New char detected
    }
    else if (!(serdatr & (1 << 14)))                  // Check RBF bit (bit 14) - buffer full?
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

static inline bool LMB_is_down(void)
{
    volatile struct CIA* ciaa = (struct CIA*)0xbfe001;
    return !(ciaa->ciapra & CIAF_GAMEPORT0); // active low
}

static inline bool LMB_is_up(void)
{
    return !LMB_is_down();
}

void PAUSEC(void)
{
    do { custom->color[0] = custom->vhposr; } while (LMB_is_up());
    do { } while (LMB_is_down());
}

void waitShort(void)
{
    volatile uint8_t* vbeam = (volatile uint8_t *)0xdff006;
    if (globals->RASTER == 1)
    {
        readSerial();
        uint8_t target = *vbeam + 10;
        while (*vbeam != target)
            ;
    }
    else
    {
        readSerial();
        for (uint32_t i = 0; i < 0x1001; i++)
        {
            (void)(*(volatile uint8_t *)0xbfe001);
            (void)*vbeam;
        }
    }
}

void waitLong(void)
{
    if (globals->RASTER == 1)
    {
        while (*(volatile uint8_t *)0xdff006 != 0x90) ;
        readSerial();
        while (*(volatile uint8_t *)0xdff006 != 0x8f) ;
    }
    else
    {
        readSerial();
        for (int j = 0; j < 4; j++)
        {
            for (volatile int i = 0; i < 0x1000; i++)
            {
                (void)*(volatile uint8_t *)0xbfe001;
                (void)*(volatile uint8_t *)0xdff006;
            }
        }
    }
}

uint8_t getKey(void)
{
    globals->keynew = 0;
    globals->keyup = 0;
    globals->keydown = 0;

    if (!(*(volatile uint8_t *)0xbfed01 & (1 << 3)))
        return globals->key;    // No new keyboard data

    uint8_t raw = *(volatile uint8_t *)0xbfec01;   // CIA-A SDR
    globals->scancode = raw;
    raw = ~((raw >> 1) | (raw << 7));               // ror.b #1, not.b
    globals->key = raw;

    if (raw & 0x80)
    {
        globals->keyup = 1;
        globals->keystatus = 0;
    }
    else
    {
        globals->keydown = 1;
        globals->keystatus = 1;
        globals->keynew = 1;
    }

    *(volatile uint8_t *)0xbfee01 |= (1 << 6);     // bset #6 - handshake start
    waitShort();
    waitShort();
    *(volatile uint8_t *)0xbfee01 &= ~(uint8_t)(1 << 6);  // bclr #6 - handshake end

    raw &= 0x7f;    // clear up/down bit

    if (raw == 0x60 || raw == 0x61)                  // Shift keys
    {
        if (!globals->keycaps)
        {
            globals->key = 0;
            globals->keyshift = globals->keystatus;
        }
    }
    else if (raw == 0x62)                            // Caps lock
    {
        globals->keycaps = globals->keystatus;
        globals->keyshift = globals->keystatus;
        globals->key = 0;
    }
    else if (raw == 0x64 || raw == 0x65)             // Alt
    {
        globals->keyalt = globals->keystatus;
        globals->key = 0;
    }
    else if (raw == 0x63 || raw == 0x67)             // Ctrl
    {
        globals->keyctrl = globals->keystatus;
        globals->key = 0;
    }

    return globals->key;
}

static void calcMouseDir(uint8_t currPos, uint8_t oldPos, uint8_t* steps, uint8_t* backward)
{
    // Replicates .GetMouseDir: cmp.b d0(curr),d1(old); blt .Lower = (int8_t)old < (int8_t)curr
    if ((int8_t)oldPos < (int8_t)currPos)            // .Lower: forward (add to cursor)
    {
        uint8_t delta = currPos - oldPos;
        *backward = 0;
        if (delta >= 128) { delta = (uint8_t)(delta - 255); *backward = 1; }
        *steps = delta;
    }
    else                                             // backward (subtract from cursor)
    {
        uint8_t delta = oldPos - currPos;
        *backward = 1;
        if (delta >= 128) { delta = (uint8_t)(delta - 255); *backward = 0; }
        *steps = delta;
    }
}

static uint32_t checkButton(uint32_t flags)
{
    // P1LMB: CIA-A PRA bit 6, active low
    if (!globals->STUCKP1LMB && !(*(volatile uint8_t *)0xbfe001 & (1 << 6)))
    {
        globals->P1LMB = 1; globals->BUTTON = 1; globals->MBUTTON = 1; globals->LMB = 1;
        flags |= (1 << 1);
        goto checkright;
    }
    // P2LMB: CIA-A PRA bit 7, active low
    if (!globals->STUCKP2LMB && !(*(volatile uint8_t *)0xbfe001 & (1 << 7)))
    {
        globals->P2LMB = 1; globals->BUTTON = 1; globals->MBUTTON = 1; globals->LMB = 1;
        flags |= (1 << 1);
    }
checkright:
    // P1RMB: POTINP bit 10, active low
    if (!globals->STUCKP1RMB && !(custom->potinp & (1 << 10)))
    {
        globals->P1RMB = 1; globals->MBUTTON = 1; globals->BUTTON = 1; globals->RMB = 1;
        flags |= (1 << 1); return flags;
    }
    // P2RMB: POTINP bit 14, active low
    if (!globals->STUCKP2RMB && !(custom->potinp & (1 << 14)))
    {
        globals->P2RMB = 1; globals->MBUTTON = 1; globals->BUTTON = 1; globals->RMB = 1;
        flags |= (1 << 1); return flags;
    }
    // P1MMB: POTINP bit 8, active low
    if (!globals->STUCKP1MMB && !(custom->potinp & (1 << 8)))
    {
        globals->MBUTTON = 1; globals->BUTTON = 1; globals->MMB = 1;
        flags |= (1 << 1); return flags;
    }
    // P2MMB: POTINP bit 12, active low
    if (!globals->STUCKP2MMB && !(custom->potinp & (1 << 12)))
    {
        globals->MBUTTON = 1; globals->BUTTON = 1; globals->MMB = 1;
        flags |= (1 << 1); return flags;
    }
    return flags;
}

uint32_t getMouseData(void)
{
    if (custom->potinp & 0xfe)
        globals->DISPAULA = 1;
    if (globals->DISPAULA)
    {
        globals->InputRegister = 0;
        return 0;
    }

    uint32_t d4 = checkButton(0);

    // Raw hardware counter deltas -> update logical MouseX/MouseY
    { uint8_t n = *(volatile uint8_t *)0xdff00a, o = globals->OldMouse1Y;
      if (n != o) { globals->OldMouse1Y = n; globals->MouseY -= (uint8_t)(o - n); } }
    { uint8_t n = *(volatile uint8_t *)0xdff00c, o = globals->OldMouse2Y;
      if (n != o) { globals->OldMouse2Y = n; globals->MouseY -= (uint8_t)(o - n); } }
    { uint8_t n = *(volatile uint8_t *)0xdff00b, o = globals->OldMouse1X;
      if (n != o) { globals->OldMouse1X = n; globals->MouseX -= (uint8_t)(o - n); } }
    // Mouse2X change: early exit (replicates .Mouse2XMove bra .CheckButton)
    { uint8_t n = *(volatile uint8_t *)0xdff00d, o = globals->OldMouse2X;
      if (n != o)
      {
          globals->OldMouse2X = n; globals->MouseX -= (uint8_t)(o - n);
          uint32_t f = checkButton(d4 | (1 << 0));
          globals->InputRegister = f; return f;
      } }

    // Cursor position update (X axis)
    { uint8_t cur = globals->MouseX, old = globals->OldMouseX;
      if (cur != old)
      {
          d4 |= (1 << 0); globals->MOUSE = 1; globals->OldMouseX = cur;
          uint8_t steps, bwd; calcMouseDir(cur, old, &steps, &bwd);
          if (bwd)
          {
              globals->CurX -= steps; globals->CurSubX = steps;
              if ((int16_t)globals->CurX < 0) globals->CurX = 0;
          }
          else
          {
              globals->CurX += steps; globals->CurAddX = steps;
              if (globals->CurX >= 640) globals->CurX = 640;
          }
      } }

    // Cursor position update (Y axis)
    { uint8_t cur = globals->MouseY, old = globals->OldMouseY;
      if (cur != old)
      {
          d4 |= (1 << 0); globals->MOUSE = 1; globals->OldMouseY = cur;
          uint8_t steps, bwd; calcMouseDir(cur, old, &steps, &bwd);
          if (bwd)
          {
              globals->CurY -= steps; globals->CurSubY = steps;
              if ((int16_t)globals->CurY < 0) globals->CurY = 0;
          }
          else
          {
              globals->CurY += steps; globals->CurAddY = steps;
              if (globals->CurY >= 512) globals->CurY = 512;
          }
      } }

    globals->InputRegister = d4;
    return d4;
}

int32_t random(int32_t d0 __asm("d0"), int32_t d1 __asm("d1"), int32_t d2 __asm("d2"),
               int32_t d3 __asm("d3"), int32_t d4 __asm("d4"), int32_t d5 __asm("d5"),
               int32_t d6 __asm("d6"), int32_t d7 __asm("d7"))
{
    uint32_t u = (uint32_t)d0;
    u += (uint32_t)d3;
    u += (uint32_t)d4;
    u = (u & 0xFFFFFF00u) | ((u + *(volatile uint8_t *)0xdff006) & 0xFFu);  // add.b $dff006,d0
    u += (uint32_t)d1;
    u = (u << 16) | (u >> 16);                                               // swap d0
    u = (u & 0xFFFFFF00u) | ((u + *(volatile uint8_t *)0xdff007) & 0xFFu);  // add.b $dff007,d0
    u += (uint32_t)d2;
    u += (uint32_t)d5;
    u += (uint32_t)d6;
    u += (uint32_t)d7;
    return (int32_t)u;
}

void filterON(void)
{
    *(volatile uint8_t *)0xbfe001 &= ~(1 << 1);    // bclr #1,$bfe001
}

void filterOFF(void)
{
    *(volatile uint8_t *)0xbfe001 |= (1 << 1);     // bset #1,$bfe001
}

void exitDiag(void)
{
    // Flash screen indefinitely (intentional infinite loop)
    for (;;)
    {
        *(volatile uint16_t *)0xdff180 = 0;
        while (*(volatile uint8_t *)0xdff006 != 0xf0);
        *(volatile uint16_t *)0xdff180 = 0xfff;
    }
}
