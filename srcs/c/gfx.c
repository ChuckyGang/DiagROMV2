// C Version of the GFX tests
#include "globalvars.h"
#include "generic.h"
#include <stddef.h>
#include <stdbool.h>
#include <exec/types.h>
#include <hardware/custom.h>
#include <hardware/dmabits.h>
#include <hardware/blit.h>
#include "c/gfx.i"
#include "c/copperlist.h"

void SetBplPtr(struct RomEcsCopper* list, int bitplane, void* addr)
{
       ULONG hi_lo = (ULONG)addr;
       ULONG lo_hi = (hi_lo >> 16) | (hi_lo << 16);
       list->bplpt[bitplane].hi.move.data = (UWORD)lo_hi;
       list->bplpt[bitplane].lo.move.data = (UWORD)hi_lo;
}

void SetSprPtr(struct RomEcsCopper* list, int sprite, void* addr)
{
       ULONG hi_lo = (ULONG)addr;
       ULONG lo_hi = (hi_lo >> 16) | (hi_lo << 16);
       list->sprpt[sprite].hi.move.data = (UWORD)lo_hi;
       list->sprpt[sprite].lo.move.data = (UWORD)hi_lo;
}

void SetSprPtrAga(struct RomAgaCopper* list, int sprite, void* addr)
{
       ULONG hi_lo = (ULONG)addr;
       ULONG lo_hi = (hi_lo >> 16) | (hi_lo << 16);
       list->sprpt[sprite].hi.move.data = (UWORD)lo_hi;
       list->sprpt[sprite].lo.move.data = (UWORD)hi_lo;
}

void SetBplPtrAga(struct RomAgaCopper* list, int bitplane, void* addr)
{
       ULONG hi_lo = (ULONG)addr;
       ULONG lo_hi = (hi_lo >> 16) | (hi_lo << 16);
       list->bplpt[bitplane].hi.move.data = (UWORD)lo_hi;
       list->bplpt[bitplane].lo.move.data = (UWORD)hi_lo;
}

void SetColLowRes(struct RomEcsCopper* list)
{
    // Temporary AGA palette (32-bit per color)
    uint32_t agaPalette[32];

    // Temporary ECS palette (16-bit per color)
    int16_t ecsPalette[32];

    // Fill the AGA palette
    makePalette(32, agaPalette);

    // Convert AGA palette to ECS palette
    makeECS(32, agaPalette, ecsPalette);

    // Number of colors in ECS palette
    int numColors = sizeof(ecsPalette) / sizeof(ecsPalette[0]);
    // Copy ECS palette into copperlist and optionally print for debugging
    for (int i = 0; i < numColors; i++)
    {
        // Assign the ECS color to the copperlist
        list->color[i].move.data = ecsPalette[i];
    }
}


void SetColHiRes(struct RomEcsCopper* list)
{
    // Temporary AGA palette (32-bit per color)
    uint32_t agaPalette[16];

    // Temporary ECS palette (16-bit per color)
    int16_t ecsPalette[16];

    // Fill the AGA palette
    makePalette(16, agaPalette);

    // Convert AGA palette to ECS palette
    makeECS(16, agaPalette, ecsPalette);

    // Number of colors in ECS palette
    int numColors = sizeof(ecsPalette) / sizeof(ecsPalette[0]);

    // Copy ECS palette into copperlist and optionally print for debugging
    for (int i = 0; i < numColors; i++)
    {
        // Assign the ECS color to the copperlist
        list->color[i].move.data = ecsPalette[i];
    }
}

void SetColAga(struct RomAgaCopper* list)
{
    // AGA 256-color palette (32-bit per color)
    uint32_t agaPalette[256];

    // Fill the 256-color palette
    makePalette(256, agaPalette);

    // Write colors to all 8 banks (32 colors each)
    for (int bank = 0; bank < 8; bank++)
    {
        for (int i = 0; i < 32; i++)
        {
            uint32_t color = agaPalette[bank * 32 + i];

            // Upper nibbles (bits 7-4 of each component)
            uint16_t r_hi = (color >> 20) & 0xF;
            uint16_t g_hi = (color >> 12) & 0xF;
            uint16_t b_hi = (color >> 4) & 0xF;
            list->colorbank[bank].color_hi[i].move.data = (r_hi << 8) | (g_hi << 4) | b_hi;

            // Lower nibbles (bits 3-0 of each component)
            uint16_t r_lo = (color >> 16) & 0xF;
            uint16_t g_lo = (color >> 8) & 0xF;
            uint16_t b_lo = color & 0xF;
            list->colorbank[bank].color_lo[i].move.data = (r_lo << 8) | (g_lo << 4) | b_lo;
        }
    }
}

void ResetAgaRegs(void)
{
    custom->bplcon3 = 0;
    custom->bplcon4 = 0;
    custom->fmode=0;
    custom->bplcon3=0x200;
    for (int i = 0; i < 32; i++)
    custom->color[i]=0;
    custom->bplcon3 = 0;
}

void clearLowRes(uint8_t *bitPlane)
{
    waitBlit();
    custom->bltcon0 = 0x100;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xffff;
    custom->bltalwm = 0xffff;
    custom->dmacon = 0x8040;
    custom->bltdpt = (void *)bitPlane;
    custom->bltdmod = 0;
    custom->bltsize = 0x4014;
}

void clearHiRes(uint8_t *bitPlane)
{

//    for (int i = 0; i < (80*512)/4; i++) {
//        *(uint32_t *)bitPlane = 0;
//        bitPlane += 4;
//    }
        waitBlit();
    custom->bltcon0 = 0x100;
    custom->bltcon1 = 0;
    custom->bltafwm = 0xffff;
    custom->bltalwm = 0xffff;
    custom->dmacon = 0x8040;
    custom->bltdpt = (void *)bitPlane;
    custom->bltdmod = 0;
    custom->bltsize = 0x8028;
}

void plotPixel(int x, int y, int color,
               int scaleX, int scaleY, int scaleCol,
               uint8_t **bplPointers)
{

   /* Scale coordinates */
    x >>= scaleX;
    y >>= scaleY;

    /* Bytes per line */
    int bytesPerLine = 80 >> scaleX;
    /* Scale color */
    color >>= scaleCol;

    /* X position */
    int byteOffset = x / 8;
    int bitOffset  = 7 - (x % 8);

    /* Y position (NO MULTIPLY) */
    int address = byteOffset;
    for (int i = 0; i < y; i++) {
        address += bytesPerLine;
    }

    uint8_t mask = (uint8_t)(1 << bitOffset);

    for (int p = 0; p < scaleColToBpl(scaleCol); p++) {
        uint8_t *ptr = (uint8_t *)((uintptr_t)bplPointers[p] + address);
        if (color & (1 << p))
            *ptr |= mask;
        else
            *ptr &= (uint8_t)~mask;
    }
}

static inline int32_t muls_16x16(int16_t a, int16_t b)
{
    int32_t result = a;
    asm volatile(
        "muls.w %1,%0"
        : "+d"(result)
        : "dmi"(b)
    );
    return result;
}

static inline int32_t divs_32by16(int32_t dividend, int16_t divisor)
{
    asm volatile(
        "divs.w %1,%0"
        : "+d"(dividend)
        : "dmi"(divisor)
    );
    return (int16_t)dividend;
}

void drawCircle(int xc, int yc, int r, int color, int scaleX, int scaleY, int scaleCol, uint8_t **bplPointers)
{
    short x = 0;
    short y = r;
    short d = 3 - (2 * r);
    short y_num = 110;
    short y_den = 100;

    while (x <= y) {

        // Y-axis scaling (integer only)
        short ay = divs_32by16(muls_16x16(y, y_num), y_den);
        short by = divs_32by16(muls_16x16(x, y_num), y_den);

        // Octants
        plotPixel(xc + x, yc + ay, color, scaleX, scaleY, scaleCol, bplPointers);
        plotPixel(xc + x, yc - ay, color, scaleX, scaleY, scaleCol, bplPointers);
        plotPixel(xc - x, yc - ay, color, scaleX, scaleY, scaleCol, bplPointers);
        plotPixel(xc - x, yc + ay, color, scaleX, scaleY, scaleCol, bplPointers);

        plotPixel(xc + y, yc + by, color, scaleX, scaleY, scaleCol, bplPointers);
        plotPixel(xc + y, yc - by, color, scaleX, scaleY, scaleCol, bplPointers);
        plotPixel(xc - y, yc - by, color, scaleX, scaleY, scaleCol, bplPointers);
        plotPixel(xc - y, yc + by, color, scaleX, scaleY, scaleCol, bplPointers);

        // Bresenham step
        if (d < 0)
            d += (4 * x) + 6;
        else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void drawFilledCircle(int xc, int yc, int r, int color, int scaleX, int scaleY, int scaleCol, uint8_t **bplPointers)
{
    int x = 0;
    int y = r;
    int d = 3 - 2 * r;

    while (x <= y) {
        // Top horizontal line
        if (x > 0) 
            drawLine(xc - x, yc - y, xc + x, yc - y, color, scaleX, scaleY, scaleCol, bplPointers);

        // Bottom horizontal line
        if (x > 0 && y != 0)
            drawLine(xc - x, yc + y, xc + x, yc + y, color, scaleX, scaleY, scaleCol, bplPointers);

        // Left/right “diagonal” spans
        if (x != y) {
            drawLine(xc - y, yc - x, xc + y, yc - x, color, scaleX, scaleY, scaleCol, bplPointers);
            drawLine(xc - y, yc + x, xc + y, yc + x, color, scaleX, scaleY, scaleCol, bplPointers);
        }

        if (d < 0) {
            d += 4 * x + 6;
        } else {
            d += 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}


void *memcpy(void *dest, const void *src, size_t n)
{
    unsigned char *d = dest;
    const unsigned char *s = src;
    while (n--)
        *d++ = *s++;
    return dest;
}

int waitBlit()
{
//    while ((volatile) custom->dmaconr & DMAF_BLTDONE) {}
    volatile uint16_t timeout = 65535;

    while ((custom->dmaconr & DMAF_BLTDONE) && timeout--)
    {
        /* busy wait */
    }

    /* return 0 = success, 1 = timeout */
    return (timeout == 0);
}


const uint8_t Octant_Table[8] = {
    0 * 4 + 1,  // Octant 0
    4 * 4 + 1,  // Octant 1
    2 * 4 + 1,  // Octant 2
    5 * 4 + 1,  // Octant 3
    1 * 4 + 1,  // Octant 4
    6 * 4 + 1,  // Octant 5
    3 * 4 + 1,  // Octant 6
    7 * 4 + 1   // Octant 7
};

int scaleColToBpl(int scaleCol)         // Convert scaleCol to number of bitplanes
{
    if(scaleCol==5)
        return 4;
    if(scaleCol==4)
        return 5;
    if(scaleCol==1)
        return 8;
    return 1;
}

void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, int color,int scaleX, int scaleY, int scaleCol, uint8_t **bplPointers)
{
    color = color >> scaleCol;
    int32_t offset;   /* work register (address) */
    int32_t octant;   /* octant/sign flags */
    int16_t deltaX, deltaY; /* deltaX, deltaY */
    int16_t tmp;
    uint8_t bytesPerLine=0;
    if (scaleX==0)
        bytesPerLine=80;
    if (scaleX==1)
        bytesPerLine=40;
    int16_t maskWord = 0xffff;
    uint8_t *addr;
    x1 = x1 >> scaleX;
    x2 = x2 >> scaleX;
    y1 = y1 >> scaleY;
    y2 = y2 >> scaleY;
    /* ------------------------------------ */
    /* Address setup: Y1 * bytesPerLine + (X1 >> 3) */
    /* ------------------------------------ */
    offset = muls_16x16((int16_t)y1, (int16_t)bytesPerLine);

    octant = x1 & 0xFFF0;
    octant >>= 3;                 /* (x1 & 0xFFF0) / 8 */
    offset += octant;
 
    /* ------------------------------------ */
    /* Compute deltas and octant bits       */
    /* ------------------------------------ */
    octant = 0;
    deltaY = (int16_t)(y2 - y1);
    octant <<= 1;
    if (deltaY < 0) {
        deltaY = -deltaY;
        octant |= 1;
    }

    deltaX = (int16_t)(x2 - x1);
    octant <<= 1;
    if (deltaX < 0) {
        deltaX = -deltaX;
        octant |= 1;
    }

    /* Compare deltaY vs deltaX */
    tmp = (int16_t)(deltaY - deltaX);
    if (tmp < 0) {
        int16_t t = deltaX;
        deltaX = deltaY;
        deltaY = t;
    }
    octant = (octant << 1) | ((tmp < 0) ? 1 : 0);
    /* Lookup octant */
    octant = Octant_Table[octant & 7];
    /* ------------------------------------ */
    /* Setup blitter registers              */
    /* ------------------------------------ */
    for (int p = 0; p < scaleColToBpl(scaleCol) ; p++)
    {

        if (color & (1 << p))
                       maskWord = 0xffff;
            else
                       maskWord = 0x0;

            addr = (uint8_t *)offset;
            addr += (uintptr_t)bplPointers[p];
            waitBlit();
            custom->bltbmod = (int16_t)(deltaX + deltaX);

            if ((int16_t)(deltaX - deltaY) > 0)
                octant |= 0x40;                    /* set sign flag */

            custom->bltapt = (void *)(uintptr_t)(deltaX - deltaY);
            custom->bltamod = (int16_t)(deltaX - deltaY);
            custom->bltadat = 0x8000;
            custom->bltbdat = maskWord;        /* A2 word → BLTBDAT */
            custom->bltafwm = 0xFFFF;
            custom->bltcon0 = ((uint16_t)x1 & 0x000F) << 12 | 0x0BCA;
            custom->bltcon1 = octant;
            /* Line start address */
            custom->bltcpt = (void *)addr;
            custom->bltdpt = (void *)addr;
            /* Modulo setup */
            custom->bltcmod = bytesPerLine;
            custom->bltdmod = bytesPerLine;
            /* Length setup: deltaY << 6 + 2 */
            custom->bltsize = (uint16_t)(((int32_t)deltaY << 6) + 2);
        }
}

void makePalette(int maxColors,uint32_t *palette)         // Make a palette with maxcolors, store them in Palette.  this is using AGA Palette
{
    int steps = ((uint32_t)256 / (uint16_t)maxColors);     // Calculate how much to skip to make a palette of chosen colors
    int color = 0x0;                                        // Make sure first is black
    int number=0;
    palette[number++]=color;                                // Store it
    color = 0xffffff;                                       // Set to white
    for(int grey=0;grey<(64u/(uint16_t)steps)-1;grey++)     // Lets do greyscale
    {
        palette[number++]=color;
        color -= 0x040404*steps;
    }
    color = 0xff0000;
    for(int red=0;red<(64u/(uint16_t)steps);red++)          // Red Scale
    {
        palette[number++]=color;
            color -= 0x040000*steps;
    }
    color = 0x00ff00;
    for(int green=0;green<(64u/(uint16_t)steps);green++)    // Green
    {
        palette[number++]=color;
            color -= 0x000400*steps;
    }
    color = 0x0000ff;
    for(int blue=0;blue<(64u/(uint16_t)steps);blue++)       // Blue
    {
        palette[number++]=color;
            color -= 0x00000004*steps;
    }
}

void makeECS(int maxColors, uint32_t palette[], int16_t ecsPalette[])
{
    for (int i = 0; i < maxColors; i++)
    {
        uint32_t color = palette[i];
        uint16_t ECS = 0;

        // Extract upper 4 bits of each AGA color component
        uint16_t r = (color >> 20) & 0xF;  // bits 23-20
        uint16_t g = (color >> 12) & 0xF;  // bits 15-12
        uint16_t b = (color >> 4)  & 0xF;  // bits 7-4

        // Pack into 12-bit ECS color: 0xRGB
        ECS = (r << 8) | (g << 4) | b;

        ecsPalette[i] = ECS;
    }
}

void gfxC(VARS)
{
    InitScreen();
        printGfxtst(globals);
    int chipMemSize=0;
    chipMemSize=(sizeof(romEcsLowResCopper))+4+(40*512*5);        // Calculate amount of chipmem needed
    char* chipmem=GetChip(chipMemSize);                       // Get chipmem (this can only allocate ONE block)
    uint8_t  *bplPointers[5];
    if(chipmem)
       {
              *(uint32_t *)chipmem=0; // Clear first longword.  to be used as a fake sprite
                struct RomEcsCopper *copperlist = (struct RomEcsCopper *)(chipmem + 4);
              memcpy(copperlist, &romEcsLowResCopper, sizeof(romEcsLowResCopper));
              for(int i=0;i<8;i++)
              {
                     SetSprPtr(copperlist,i,chipmem);          // Set all spritepointers to the 0 longword
              }
              custom->cop1lc = (uint32_t)(uintptr_t)copperlist;
              uint8_t *bitPlane = (uint8_t *)copperlist + sizeof(romEcsLowResCopper);

              for(int i=0;i<5;i++)
              {
                    bplPointers[i]=bitPlane;
                    bitPlane=bitPlane+40*256;
                    SetBplPtr(copperlist,i,bplPointers[i]);
                    print("\nBitplane no: ",WHITE);
                    print(binDec(i),WHITE);
                    print(" at: ",WHITE);
                    print(binHex((int)bplPointers[i]),WHITE);
                    clearLowRes(bplPointers[i]);
              }
            SetColLowRes(copperlist);

        makeTestPicture(1,1,4,bplPointers);

       }
              else
       {
              print("FAIL! NOT ENOUGH CHIPMEM!",RED);
       }

       print("\n\nDONE. Press any key/button to exit",WHITE);
       do
       {

              GetInput();
       }    while(globals->BUTTON == 0);

       SetMenuCopper();
}



void gfxCAga(VARS)
{
    InitScreen();
    printGfxtst(globals);
    int chipMemSize=0;
    chipMemSize=(sizeof(romAgaLowResCopper))+4+(40*512*8);        // Calculate amount of chipmem needed
    char* chipmem=GetChip(chipMemSize);                       // Get chipmem (this can only allocate ONE block)
    uint8_t  *bplPointers[8];
    if(chipmem)
       {
              *(uint32_t *)chipmem=0; // Clear first longword.  to be used as a fake sprite
                struct RomAgaCopper *copperlist = (struct RomAgaCopper *)(chipmem + 4);
              memcpy(copperlist, &romAgaLowResCopper, sizeof(romAgaLowResCopper));
              // FMODE=3 (64-bit fetch) requires negative modulo to compensate for extra prefetch
              for(int i=0;i<8;i++)
              {
                     SetSprPtrAga(copperlist,i,chipmem);
              }
              custom->cop1lc = (uint32_t)(uintptr_t)copperlist;
              uint8_t *bitPlane = (uint8_t *)copperlist + sizeof(romAgaLowResCopper);

              for(int i=0;i<8;i++)
              {
                    bplPointers[i]=bitPlane;
                    bitPlane=bitPlane+40*256;
                    print("\nBitplane no: ",WHITE);
                    print(binDec(i),WHITE);
                    print(" at: ",WHITE);
                    print(binHex((int)bplPointers[i]),WHITE);
                    SetBplPtrAga(copperlist,i,bplPointers[i]);
                    clearLowRes(bplPointers[i]);
              }
              copperlist->bplmod[0].move.data = -8;  // Compensate for FMODE=3 extra 8-byte prefetch
              copperlist->bplmod[1].move.data = -8;
            SetColAga(copperlist);

        makeTestPicture(1,1,1,bplPointers);

       }
              else
       {
              print("FAIL! NOT ENOUGH CHIPMEM!",RED);
       }

       print("\n\nDONE. Press any key/button to exit",WHITE);
       do
       {

              GetInput();
       }    while(globals->BUTTON == 0);
       SetMenuCopper();
       ResetAgaRegs();
}



void gfxCAgaHigh(VARS)
{
    InitScreen();
    printGfxtst(globals);
    int chipMemSize=0;
    chipMemSize=(sizeof(romAgaHiResCopper))+4+(80*512*8);        // Calculate amount of chipmem needed
    char* chipmem=GetChip(chipMemSize);                       // Get chipmem (this can only allocate ONE block)
    uint8_t  *bplPointers[8];
    struct RomAgaCopper *copperlist = (struct RomAgaCopper *)(chipmem + 4);
    if(chipmem)
       {
              *(uint32_t *)chipmem=0; // Clear first longword.  to be used as a fake sprite

              memcpy(copperlist, &romAgaHiResCopper, sizeof(romAgaHiResCopper));
              // FMODE=3 (64-bit fetch) requires negative modulo to compensate for extra prefetch
              for(int i=0;i<8;i++)
              {
                     SetSprPtrAga(copperlist,i,chipmem);
              }
              custom->cop1lc = (uint32_t)(uintptr_t)copperlist;
              uint8_t *bitPlane = (uint8_t *)copperlist + sizeof(romAgaHiResCopper);

              for(int i=0;i<8;i++)
              {
                     bplPointers[i]=bitPlane;
                     bitPlane=bitPlane+80*512;
                     print("\nBitplane no: ",WHITE);
                     print(binDec(i),WHITE);
                     print(" at: ",WHITE);
                     print(binHex((int)bplPointers[i]),WHITE);
                     SetBplPtrAga(copperlist,i,bplPointers[i]);
                     clearHiRes(bplPointers[i]);
              }
              copperlist->bplmod[0].move.data = 72;  // 80 - 16 for FMODE=3 hi-res interlaced
              copperlist->bplmod[1].move.data = 72;
            SetColAga(copperlist);

        makeTestPicture(0,0,1,bplPointers);

       }
              else
       {
              print("FAIL! NOT ENOUGH CHIPMEM!",RED);
       }

       print("\n\nDONE. Press any key/button to exit",WHITE);
       do
       {

                 uint16_t vposr = custom->vposr;

      if (vposr & 0x8000) {
          // Odd field (LOF=1) - start at line 0
          for (int i = 0; i < 8; i++) {
              SetBplPtrAga(copperlist, i, bplPointers[i] + 80);
          }
      } else {
          // Even field (LOF=0) - start at line 1 (+80 bytes)
          for (int i = 0; i < 8; i++) {
              SetBplPtrAga(copperlist, i, bplPointers[i]);
          }
      }

              GetInput();
       }    while(globals->BUTTON == 0);
       SetMenuCopper();
       ResetAgaRegs();
}



void printGfxtst(VARS)
{
    print("\002 GFX Testpicture",WHITE);
    print("\n\nThis will generate a testpicture using 640x512 and will scale down\n",WHITE);
    print("to wanted resolution, blitter and cpu is used during this and will not be\n",WHITE);
    print("fast on slower machines, When done, press any key / button to go back to normal\n",WHITE);;
    print("\n\nAs code (for now) runs in ROM, Accelerators will not really help!\n\n",CYAN);
    print("There will be NO check if screenmode will work on your machine it will just try",YELLOW);
    print("\n\n\n",WHITE);
    print("\n\nDONE. Press any key/button to exit",WHITE);
    globals->BUTTON = 0;
    do
       {
        GetInput();
       }    while(globals->BUTTON == 0);
           globals->BUTTON = 0;
}

void gfxChigh(VARS)
{
    InitScreen();
    printGfxtst(globals);
    int chipMemSize=0;
    chipMemSize=(sizeof(romEcsHiResCopper))+4+(80*512*4);        // Calculate amount of chipmem needed
    char* chipmem=GetChip(chipMemSize);                       // Get chipmem (this can only allocate ONE block)
    uint8_t  *bplPointers[4];
    struct RomEcsCopper *copperlist = (struct RomEcsCopper *)(chipmem + 4);
    if(chipmem)
       {
              *(uint32_t *)chipmem=0; // Clear first longword.  to be used as a fake sprite
                
              memcpy(copperlist, &romEcsHiResCopper, sizeof(romEcsHiResCopper));
              for(int i=0;i<8;i++)
              {
                     SetSprPtr(copperlist,i,chipmem);          // Set all spritepointers to the 0 longword
              }
              custom->cop1lc = (uint32_t)(uintptr_t)copperlist;
              uint8_t *bitPlane = (uint8_t *)copperlist + sizeof(romEcsHiResCopper);

              for(int i=0;i<4;i++)
              {
                    bplPointers[i]=bitPlane;
                    bitPlane=bitPlane+80*512;
                    print("\nBitplane no: ",WHITE);
                    print(binDec(i),WHITE);
                    print(" at: ",WHITE);
                    print(binHex((int)bplPointers[i]),WHITE);
                    SetBplPtr(copperlist,i,bplPointers[i]);
                    clearHiRes(bplPointers[i]);
              }

            copperlist->bplmod[0].move.data=80;
            copperlist->bplmod[1].move.data=80;
            SetColHiRes(copperlist);

       makeTestPicture(0,0,5,bplPointers);

    }
              else
       {
              print("FAIL! NOT ENOUGH CHIPMEM!",RED);
       }

        globals->BUTTON = 0;

       print("\n\nDONE. Press any key/button to exit",WHITE);

       do
       {

         uint16_t vposr = custom->vposr;

      if (vposr & 0x8000) {
          // Odd field (LOF=1) - start at line 0
          for (int i = 0; i < 4; i++) {
              SetBplPtr(copperlist, i, bplPointers[i] + 80);
          }
      } else {
          // Even field (LOF=0) - start at line 1 (+80 bytes)
          for (int i = 0; i < 4; i++) {
              SetBplPtr(copperlist, i, bplPointers[i]);
          }
      }
              GetInput();
       }    while(globals->BUTTON == 0);


       SetMenuCopper();
}

void makeTestPicture(int scaleX, int scaleY, int scaleCol,uint8_t **bplPointers)
{

    for(int i=0;i<16;i++)
    {
        drawLine((i*40),0,(i*40),(512),32,scaleX,scaleY,scaleCol, bplPointers);
    }

    for(short i=0;i<512;i++)
    {
        drawLine(i+64,82,i+64,123,(i/5)+24,scaleX,scaleY,scaleCol,bplPointers);

    }

        for(short i=0;i<512;i++)
    {
        drawLine(i+64,162,i+64,202,(i/4)+128,scaleX,scaleY,scaleCol,bplPointers);

    }

        for(short i=0;i<512;i++)
    {
        drawLine(i+64,242,i+64,283,(i/4)+256,scaleX,scaleY,scaleCol,bplPointers);

    }
    for(short i=0;i<512;i++)
    {
        drawLine(i+64,322,i+64,362,(i/4)+384,scaleX,scaleY,scaleCol,bplPointers);

    }

        for(int i=0;i<13;i++)
    {
        drawLine(0,(i*40),(640),(i*40),32,scaleX, scaleY, scaleCol, bplPointers);
    }


    drawFilledCircle(40,40,36,32,scaleX,scaleY,scaleCol,bplPointers);
    drawFilledCircle(600,40,36,128,scaleX,scaleY,scaleCol,bplPointers);
    drawFilledCircle(40,440,36,256,scaleX,scaleY,scaleCol,bplPointers);
    drawFilledCircle(600,440,36,384,scaleX,scaleY,scaleCol,bplPointers);
    drawCircle((640/2),(512/2),200,32,scaleX,scaleY,scaleCol,bplPointers);
}