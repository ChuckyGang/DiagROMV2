#include "globalvars.h"
#include "generic.h"
#include <hardware/cia.h>
#include "c/rtc.i"
int measureRTCSecond(void);

static void setSR(uint16_t sr __asm("d0"))
{
       asm volatile (
              "move	%0,sr\n"
              :
              : "d" (sr)
              : "cc"
              );
}


void RTCTestC(VARS)
{
       print("\002RTC TEST    **** EXPERIMENTAL ****\n\n",CYAN);
       volatile uint8_t *rtc = (volatile uint8_t *)0xdc0000;
       uint8_t rtcArray[16];
       uint8_t ricoh=FALSE;
       uint8_t ampm=FALSE;
       uint8_t am=FALSE;
       uint8_t second;
       uint8_t oldSecond=0xff;
       uint8_t minute;
       uint8_t hour;
       uint8_t day;
       uint8_t year;
       uint8_t month;
       uint8_t date;
       int exit=FALSE;

       // Initialize RTC (Ricoh RP5C01 / OKI MSM6242B)
       // All writes use +3 byte offset within 4-byte register blocks (matching ASM convention)
       *(rtc + 0x37) = 0x04;  // Mode register (Reg 13): stop timer (TE=0) before init
       *(rtc + 0x3b) = 0x00;  // Test register (Reg 14): MUST be 0x00 for normal operation
                               // Writing non-zero here puts chip in fast-count test mode!
       *(rtc + 0x37) = 0x08;  // Mode register (Reg 13): start timer (TE=1, Bank 0)

       InitScreen();
       // Detect chip: read Reg10 (Ricoh month-tens) and Reg9 (Ricoh month-units)
       // using +3 offset within each 4-byte block, matching ASM read convention
       uint8_t ricochMonth = ((*(rtc+43)&0xf)*10)+(*(rtc+39)&0xf); // Reg10*10+Reg9, offset +3

       setPos(0,4);
       print("RTC Chipset: ",WHITE);

              if(ricochMonth>12)
       {
              ricoh=FALSE;
       }
       else
       {
              ricoh=TRUE;
       }
       printRTCChip(ricoh);
       setPos(0,5);
       print("Year: ",WHITE);
       setPos(0,6);
       print("Month: ",WHITE);
       setPos(0,7);
       print("Date: ",WHITE);
       setPos(10,7);
       print("Day of week: ",WHITE);
       setPos(0,8);
       print("Time:",WHITE);
       setPos(0,15);
       print("\002S - Swap Chipset\n",WHITE);
       print("\002D - Dump RTC content\n",WHITE);
       print("\002T - Measure RTC second accuracy (uses CIAB HSYNC counter)\n",WHITE);
       print("\002ESC (or Mousebutton) - EXIT\n",WHITE);
       print("\n\n\n\n ALL EXPERIMENTAL",RED);
       ricochMonth = ((*(rtc+43)&0xf)*10)+(*(rtc+39)&0xf);
       do
       {
              // Read all 16 RTC registers using stride 4, offset +3 (matches ASM inner loop)
              // ASM: lea $dc0003,a1 / add.l #4,a1 → reads from $DC0003,$DC0007,$DC000B,...
              // i.e. rtcArray[N] = *(rtc + N*4 + 3) = register N directly
              for(int i=0;i<16;i++)
              {
                     *(rtc + 0x37) = 0x08;  // write mode reg before each read (matches ASM)
                     rtcArray[i]=(*(rtc+(i*4)+3))&0xf;
              }
              if(ricoh)
              {
                     // Ricoh RP5C01: Reg0=sec1, Reg1=sec10, Reg2=min1, Reg3=min10,
                     //               Reg4=hr1,  Reg5=hr10,  Reg6=dow,
                     //               Reg7=day1, Reg8=day10, Reg9=mon1, Reg10=mon10,
                     //               Reg11=yr1, Reg12=yr10
                     second = (rtcArray[1]*10)+rtcArray[0];
                     minute = (rtcArray[3]*10)+rtcArray[2];
                     hour = (rtcArray[5]*10)+rtcArray[4];
                     day = rtcArray[6];
                     date = (rtcArray[8]*10)+rtcArray[7];
                     month = (rtcArray[10]*10)+rtcArray[9];
                     year = (rtcArray[12]*10)+rtcArray[11];
              }
              else
              {
                     // OKI MSM6242B: Reg0=sec1, Reg1=sec10, Reg2=min1, Reg3=min10,
                     //               Reg4=hr1,  Reg5=hr10,
                     //               Reg6=day1, Reg7=day10, Reg8=mon1, Reg9=mon10,
                     //               Reg10=yr1, Reg11=yr10, Reg12=dow
                     second = (rtcArray[1]*10)+rtcArray[0];
                     minute = (rtcArray[3]*10)+rtcArray[2];
                     hour = (rtcArray[5]*10)+rtcArray[4];
                     ampm = FALSE;
                     if(hour>24)
                     {
                            ampm=TRUE;
                            am=hour&0x30;
                            hour=hour&0x1f;
                            hour=hour-10;
                     }
                     day = rtcArray[12];
                     date = (rtcArray[7]*10)+rtcArray[6];
                     month = (rtcArray[9]*10)+rtcArray[8];
                     year = (rtcArray[11]*10)+rtcArray[10];
              }

             if(second!=oldSecond)
             {
                     oldSecond=second;
                     setPos(6,8);
                     print("Time: ",GREEN);
                     setPos(6,8);
                     if(hour<10)
                     print("0",YELLOW);
                     print(binDec(hour),YELLOW);
                     print(":",GREEN);
                     if(minute<10)
                            print("0",YELLOW);
                     print(binDec(minute),YELLOW);
                     print(":",GREEN);
                     if(second<10)
                            print("0",YELLOW);
                     print(binDec(second),YELLOW);
                     if(ampm)
                     {
                            if(am)
                            print(" PM",CYAN);
                            else
                            print("   ",CYAN);
                     }
                     else
                     {
                            print("   ",CYAN);
                     }

                     setPos(6,5);
                     int yearFull = (year >= 78) ? (year + 1900) : (year + 2000);
                     print(binDec(yearFull),CYAN);

                     setPos(7,6);
                     if(month<10)
                            print("0",CYAN);
                     print(binDec(month),CYAN);
//
                     setPos(6,7);
                     if(date<10)
                           print("0",CYAN);
                     print(binDec(date),CYAN);
//
                     setPos(23,7);
                     print(binDec(day),CYAN);

              }
              GetInput();
              if(globals->BUTTON)
              {
                     //exit=TRUE;
                     if(globals->GetCharData=='s')
                     {
                            ricoh = !ricoh;
                            exit=FALSE;
                            printRTCChip(ricoh);

                     }

                     if(globals->GetCharData=='d')
                     {
                            setPos(0,10);
                            for(int i=0;i<16;i++)                     // Prints the Array on screen
                            {
                                   print(binDec(rtcArray[i]),YELLOW);
                                   print(":",GREEN);
                            }
                            exit=FALSE;
                            setPos(0,11);
                            print("0 1 2 3 4 5 6 7 8 9 A B C D E F\n",GREEN);
                     }
                     if(globals->GetCharData=='t')
                     {
                            setPos(0,10);
                            print("Measuring RTC (wait ~11 sec)...",WHITE);
                            int hsync = measureRTCSecond();
                            setPos(0,11);
                            print("HSYNC/10sec: ",WHITE);
                            print(binDec(hsync),CYAN);
                            print(" (PAL=156250, NTSC=157340)",GREEN);
                            setPos(0,12);
                            if(hsync > 150000 && hsync < 165000)
                                   print("RTC timing OK",GREEN);
                            else if(hsync < 150000)
                                   print("RTC runs FAST",RED);
                            else
                                   print("RTC runs SLOW",RED);
                            exit=FALSE;
                     }
                     if(globals->GetCharData==0x1b)
                     {
                            exit=1;
                     }
                     if(globals->LMB || globals->RMB)
                     {
                            exit=1;
                     }
              }

       }
             while(!exit);
}

int measureRTCSecond(void)
{
       volatile struct CIA *ciab = (struct CIA *)0xbfd000;
       volatile uint8_t *rtc = (volatile uint8_t *)0xdc0000;
       uint8_t sec;

       // Read seconds units digit - Reg0 at offset +3 (matches ASM convention)
       sec = (*(rtc + 0x03)) & 0xf;

       // Wait for second to change (sync to edge)
       while (((*(rtc + 0x03)) & 0xf) == sec) {}

       // Reset CIAB TOD counter (counts HSYNC ~15625/sec PAL)
       ciab->ciacrb &= ~0x80;  // Write TOD mode, not alarm
       ciab->ciatodhi = 0;
       ciab->ciatodmid = 0;
       ciab->ciatodlow = 0;    // Writing low byte starts counter

       // Wait for 10 second changes
       for (int i = 0; i < 10; i++) {
              sec = (*(rtc + 0x03)) & 0xf;
              while (((*(rtc + 0x03)) & 0xf) == sec) {}
       }

       // Read elapsed HSYNC count (read hi first to latch)
       int hi = ciab->ciatodhi;
       int mid = ciab->ciatodmid;
       int lo = ciab->ciatodlow;

       // Return total for 10 seconds (avoid 32-bit divide)
       return (hi << 16) | (mid << 8) | lo;
}

void printRTCChip(uint8_t ricoh)
{
       setPos(0,4);
       print("RTC Chipset: ",WHITE);

       if(!ricoh)
       {
              print("OKI  ",CYAN);
       }
       else
       {
              print("RICOH",CYAN);
       }
}
