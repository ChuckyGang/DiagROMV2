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
       Print("\002RTC TEST    **** EXPERIMENTAL ****\n\n",CYAN);
       volatile uint8_t *rtc = (volatile uint8_t *)0xdc0000;
       uint8_t rtcArray[40];
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

       // Initialize RTC - matches working ASM code (stride 4 addressing)
       // Ricoh: 0x35=Mode(0x0D), 0x39=Test(0x0E), 0x37 between them
       // OKI uses stride 2, but these writes seem to work for both
       *(rtc + 0x39) = 0x08;  // Test register / reset (Reg 14, stride 4)
       *(rtc + 0x35) = 0x08;  // Mode: Timer Enable=1, Bank 0 (Reg 13, stride 4)

       InitScreen();
       uint8_t ricochMonth = ((*(rtc+41)&0xf)*10)+(*(rtc+37)&0xf); // Read from where Ricoh have its month stored (Reg10*10+Reg9, stride 4)

       SetPos(0,4);
       Print("RTC Chipset: ",WHITE);

              if(ricochMonth>12)
       {
              ricoh=FALSE;
       }
       else
       {
              ricoh=TRUE;
       }
       printRTCChip(ricoh);
       SetPos(0,5);
       Print("Year: ",WHITE);
       SetPos(0,6);
       Print("Month: ",WHITE);
       SetPos(0,7);
       Print("Date: ",WHITE);
       SetPos(10,7);
       Print("Day of week: ",WHITE);
       SetPos(0,8);
       Print("Time:",WHITE);
       SetPos(0,15);
       Print("\002S - Swap Chipset\n",WHITE);
       Print("\002D - Dump RTC content\n",WHITE);
       Print("\002T - Measure RTC second accuracy (uses CIAB HSYNC counter)\n",WHITE);
       Print("\002ESC (or Mousebutton) - EXIT\n",WHITE);
       Print("\n\n\n\n ALL EXPERIMENTAL",RED);
       ricochMonth = ((*(rtc+41)&0xf)*10)+(*(rtc+37)&0xf);
       do
       {
              for(int i=0;i<40;i++)                     // Reads from the RTC Chip to the Array
              {
                     rtcArray[i]=(*(rtc+(i*2)+1))&0xf;
              }
              if(ricoh)
              {
                     second = (rtcArray[2]*10)+rtcArray[0];
                     minute = (rtcArray[6]*10)+rtcArray[4];
                     hour = (rtcArray[10]*10)+rtcArray[8];
                     day = rtcArray[12];
                     date = (rtcArray[16]*10)+rtcArray[14];
                     month = (rtcArray[20]*10)+rtcArray[18];
                     year = (rtcArray[24]*10)+rtcArray[22];
              }
              else
              {
                     second = (rtcArray[2]*10)+rtcArray[1];
                     minute = (rtcArray[6]*10)+rtcArray[5];
                     hour = (rtcArray[10]*10)+rtcArray[9];
                     ampm = FALSE;
                     if(hour>24)
                     {
                            ampm=TRUE;
                            am=hour&0x30;
                            hour=hour&0x1f;
                            hour=hour-10;
                     }
                     day = rtcArray[24];
                     date = (rtcArray[14]*10)+rtcArray[12];
                     month = (rtcArray[18]*10)+rtcArray[16];
                     year = ((rtcArray[22]%10)*10)+rtcArray[20];
              }

             if(second!=oldSecond)
             {
                     oldSecond=second;
                     SetPos(6,8);
                     Print("Time: ",GREEN);
                     SetPos(6,8);
                     if(hour<10)
                     Print("0",YELLOW);
                     Print(bindec(hour),YELLOW);
                     Print(":",GREEN);
                     if(minute<10)
                            Print("0",YELLOW);
                     Print(bindec(minute),YELLOW);
                     Print(":",GREEN);
                     if(second<10)
                            Print("0",YELLOW);
                     Print(bindec(second),YELLOW);
                     if(ampm)
                     {
                            if(am)
                            Print(" PM",CYAN);
                            else
                            Print("   ",CYAN);
                     }
                     else
                     {
                            Print("   ",CYAN);
                     }

                     SetPos(6,5);
                     Print(bindec(year),CYAN);

                     SetPos(7,6);
                     if(month<10)
                            Print("0",CYAN);
                     Print(bindec(month),CYAN);
//
                     SetPos(6,7);
                     if(date<10)
                           Print("0",CYAN);
                     Print(bindec(date),CYAN);
//
                     SetPos(23,7);
                     Print(bindec(day),CYAN);

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
                            SetPos(0,10);
                            for(int i=0;i<20;i++)                     // Prints the Array on screen
                            {
                                   Print(bindec(rtcArray[i]),YELLOW);
                                   Print(":",GREEN);
                            }
                            exit=FALSE;
                            SetPos(0,11);
                            Print("0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9\n",GREEN);
                            for(int i=20;i<40;i++)                     // Prints the Array on screen
                            {
                                   Print(bindec(rtcArray[i]),YELLOW);
                                   Print(":",GREEN);
                            }
                     }
                     if(globals->GetCharData=='t')
                     {
                            SetPos(0,10);
                            Print("Measuring RTC (wait ~11 sec)...",WHITE);
                            int hsync = measureRTCSecond();
                            SetPos(0,11);
                            Print("HSYNC/10sec: ",WHITE);
                            Print(bindec(hsync),CYAN);
                            Print(" (PAL=156250, NTSC=157340)",GREEN);
                            SetPos(0,12);
                            if(hsync > 150000 && hsync < 165000)
                                   Print("RTC timing OK",GREEN);
                            else if(hsync < 150000)
                                   Print("RTC runs FAST",RED);
                            else
                                   Print("RTC runs SLOW",RED);
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
//       ClearBuffer();
//       do
//       {
//
//              GetInput();
//       }
//             while(globals->BUTTON == 0);
//
//             ClearBuffer();
}

int measureRTCSecond(void)
{
       volatile struct CIA *ciab = (struct CIA *)0xbfd000;
       volatile uint8_t *rtc = (volatile uint8_t *)0xdc0000;
       uint8_t sec;

       // Read seconds units digit - register 0 at offset 0x01
       sec = (*(rtc + 0x01)) & 0xf;

       // Wait for second to change (sync to edge)
       while (((*(rtc + 0x01)) & 0xf) == sec) {}

       // Reset CIAB TOD counter (counts HSYNC ~15625/sec PAL)
       ciab->ciacrb &= ~0x80;  // Write TOD mode, not alarm
       ciab->ciatodhi = 0;
       ciab->ciatodmid = 0;
       ciab->ciatodlow = 0;    // Writing low byte starts counter

       // Wait for 10 second changes
       for (int i = 0; i < 10; i++) {
              sec = (*(rtc + 0x01)) & 0xf;
              while (((*(rtc + 0x01)) & 0xf) == sec) {}
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
              SetPos(0,4);
       Print("RTC Chipset: ",WHITE);

       if(!ricoh)
       {
              Print("OKI  ",CYAN);
       }
       else
       {
              Print("RICOH",CYAN);
       }
}
