// C Version of the IRQ CIA Tests
#include "globalvars.h"
#include "generic.h"
#include <hardware/custom.h>
#define custom ((volatile struct Custom*)0xdff000)
#include <hardware/cia.h>
#include "c/irqcia.i"
// YES I know this is a messy file..  I am just screwing around with ideas and tests now..
// will be more tidy when I know how I want shit
// ALSO! I need to learn to comment WAY better!!! so learning BE GENTLE! :-D


static void setSR(uint16_t sr __asm("d0"))
{
       asm volatile (
              "move	%0,sr\n"
              :
              : "d" (sr)
              : "cc"
              );
}

int readTOD()
{
//       struct CIA *ciaa = (struct CIA *)0xbfe001;
       struct CIA *ciab = (struct CIA *)0xbfd000;
       return (ciab->ciatodhi<<16)|(ciab->ciatodmid<<8)|ciab->ciatodlow;

}

int readTODA()
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       //struct CIA *ciab = (struct CIA *)0xbfd000;
       return (ciaa->ciatodhi<<16)|(ciaa->ciatodmid<<8)|ciaa->ciatodlow;

}

       #define IR1 0x7
       #define IR2 0x8
       #define IR3 0x70
       #define IR4 0x780
       #define IR5 0x1800
       #define IR6 0x2000


void detectTOD(VARS)                      // to test ToD bug!.  apparenlty cias can cocund 0x20 21 22.. 2D 2E 2F 20 30 31 32 etc..  that ALARM triggers on
{
              volatile struct CIA *ciaa = (struct CIA *)0xbfe001;
       volatile struct CIA *ciab = (struct CIA *)0xbfd000;
              globals->IRQ6=0;
       print("\n\n Testing ODD CIA ALARM 2 Seconds: ",WHITE);
       custom->color[0]=0x0;
       globals->Frames=0;
                     ciaa->ciaicr = 0x80;
              ciaa->ciacra = 0x00;
              ciaa->ciatodhi = 0;
              ciaa->ciatodmid =0;
              ciaa->ciatodlow=0x21;
              ciaa->ciacrb|= 0x80;
              ciaa->ciaicr = 0x84;
              unsigned int current_tod = (ciaa->ciatodhi << 16) | (ciaa->ciatodmid << 8) | ciaa->ciatodlow;
              unsigned int alarm_value = 0x20; // Use 100 for PAL
              ciaa->ciatodlow = (alarm_value & 0xFF);        // Low byte
              ciaa->ciatodmid = ((alarm_value >> 8) & 0xFF); // Middle byte
              ciaa->ciatodhi = ((alarm_value >> 16) & 0xFF); // High byte (latches value)
              ciaa->ciacrb &= ~0x80;
              //PAUSEC();
       custom->intena = 0xc000+IR2+IR3;
       custom->intena = 0xc000+IR2+IR3;
       globals->Frames=0;
       globals->IRQ2=0;
       globals->ODDALARMOK=0;
              do
              {
                     if(globals->IRQ2)
                     {
                            print("OK",GREEN);
                            globals->ODDALARMOK=1;
                     break;
                     }
                     custom->color[0]=0x006;
                     custom->color[0]=0x000;
              }
              while(globals->Frames<200);

}


void timedcia(VARS)
{
       volatile struct CIA *ciaa = (struct CIA *)0xbfe001;
       volatile struct CIA *ciab = (struct CIA *)0xbfd000;
       globals->IRQ6=0;
       print("\n\n Testing ODD CIA ALARM 2 Seconds: ",WHITE);
       custom->color[0]=0x0;
       globals->Frames=0;
              ciaa->ciacra = 0x00;
              ciaa->ciatodhi = 0;
              ciaa->ciatodmid =0;
              ciaa->ciatodlow=0;
              ciaa->ciacrb|= 0x80;
              ciaa->ciaicr = 0x84;
              unsigned int current_tod = (ciaa->ciatodhi << 16) | (ciaa->ciatodmid << 8) | ciaa->ciatodlow;
              unsigned int alarm_value = current_tod + 100; // Use 100 for PAL
              ciaa->ciatodlow = (alarm_value & 0xFF);        // Low byte
              ciaa->ciatodmid = ((alarm_value >> 8) & 0xFF); // Middle byte
              ciaa->ciatodhi = ((alarm_value >> 16) & 0xFF); // High byte (latches value)
              ciaa->ciacrb &= ~0x80;
       custom->intena = 0xc000+IR2+IR3;
       custom->intena = 0xc000+IR2+IR3;
       globals->Frames=0;
       globals->IRQ2=0;
       globals->ODDALARMOK=0;
              do
              {
                     if(globals->IRQ2)
                     {
                            print("OK",GREEN);
                            globals->ODDALARMOK=1;
                     break;
                     }
                     custom->color[0]=0x006;
                     custom->color[0]=0x000;
              }
              while(globals->Frames<200);
              if(globals->IRQ2==0)
              {
                     print(" FAILED",RED);
              }

              print("\n Testing EVEN CIA ALARM 2 Seconds: ",WHITE);
              ciab->ciacra = 0x00;
              ciab->ciatodhi = 0;
              ciab->ciatodmid =0;
              ciab->ciatodlow=0;
              ciab->ciacrb|= 0x80;
              ciab->ciaicr = 0x84;
              unsigned int current_tod2 = (ciab->ciatodhi << 16) | (ciab->ciatodmid << 8) | ciab->ciatodlow;
              unsigned int alarm_value2 = current_tod + 31250;
              ciab->ciatodlow = (alarm_value2 & 0xFF);        // Low byte
              ciab->ciatodmid = ((alarm_value2 >> 8) & 0xFF); // Middle byte
              ciab->ciatodhi = ((alarm_value2 >> 16) & 0xFF); // High byte (latches value)
              ciab->ciacrb &= ~0x80;
       custom->intena = 0xc000+IR6+IR3;
       custom->intena = 0xc000+IR6+IR3;
       globals->Frames=0;
       globals->IRQ6=0;
              do
              {
                     if(globals->IRQ6)
                     {
                            print("OK",GREEN);
                     break;
                     }
                     custom->color[0]=0x060;
                     custom->color[0]=0x000;
              }
              while(globals->Frames<200);
       if(globals->IRQ6==0)
              {
                     print(" FAILED",RED);
              }
}

void polledcia(VARS)
{
       int counter=0;
       int timeout=0;
       int failed=0;
       int ticks=0;
       int tod=0;
       int ciadone=0;
       volatile struct CIA *ciaa = (struct CIA *)0xbfe001;
       volatile struct CIA *ciab = (struct CIA *)0xbfd000;
       globals->IRQ2=0;
       globals->IRQ6=0;
       print("\n Testing ODD CIA Steps of 1ms each\n",WHITE);
       print("\n\nCIAA Timer A:",GREEN);

       globals->Frames=0;
       print("             ",WHITE);
       do
       {
              custom->color[0]=0x0;
              ciaa->ciatalo=198;
              ciaa->ciatahi=2;     // Set 1ms timing
              ciaa->ciacra=CIACRAF_RUNMODE|CIACRAF_LOAD;
              ciaa->ciacra=CIACRAF_START|CIACRAF_RUNMODE;
              timeout = globals->Frames;
              counter++;
              do
              {
                     if(globals->Frames>timeout+10) 
                     {
                            timeout=0;
                            print("NO ICR Triggered, FAILED",RED);
                            failed=1;
                            break;
                     }
                     if(globals->Frames>200)
                     {
                           print("TIMEOUT",RED);
                           globals->ODDCIATIMEROK=0;
                            failed=1;
                            //break;
                     }
                     ciadone = ((volatile uint8_t)ciaa->ciaicr&1)==0;
       } while (!failed&&ciadone);
       togglePwrLED();
       custom->color[0]=0x550;
                            timeout = globals->Frames;
              } while (globals->Frames<=122);

              print("Counter: ",WHITE);
              print(binDec(counter),CYAN);
              globals->ODDCIATIMEROK=0;
              if(checkCiaOK(counter)==1)
              {
                      globals->ODDCIATIMEROK=1;      
              }

       print("CIAA Timer B:",GREEN);
       counter = 0;
       globals->Frames=0;
       print("             ",WHITE);
       do
       {
              custom->color[0]=0x0;
              ciaa->ciatblo=198;
              ciaa->ciatbhi=2;     // Set 1ms timing
              ciaa->ciacrb=CIACRAF_RUNMODE|CIACRAF_LOAD;
              ciaa->ciacrb=CIACRAF_START|CIACRAF_RUNMODE;
              timeout = globals->Frames;
              counter++;
              do
              {
                     if(globals->Frames>timeout+10) 
                     {
                            timeout=0;
                            print("NO ICR Triggered, FAILED",RED);
                            failed=1;
                            globals->ODDCIATIMEROK=0;
                            break;
                     }
                     if(globals->Frames>200)
                     {
                           print("TIMEOUT",RED);
                           globals->ODDCIATIMEROK=0;
                            failed=1;
                     }
                     ciadone = ((volatile uint8_t)ciaa->ciaicr&2)==0;
       } while (!failed&&ciadone);
              togglePwrLED();
              custom->color[0]=0x660;
                            timeout = globals->Frames;
              } while (globals->Frames<=122);

              print("Counter: ",WHITE);
              print(binDec(counter),CYAN);

              if(checkCiaOK(counter)==1)
              {
                      globals->ODDCIATIMEROK=1;      
              }
              print("CIAA TOD (VSync)",GREEN);
              ciaa->ciacrb=!CIACRBB_ALARM;
              ciaa->ciatodhi=0;
              ciaa->ciatodmid=0;
              ciaa->ciatodlow=0;
              globals->Frames=0;
              do
              {
                     custom->color[0]=0x006;
                     custom->color[0]=0x000;
              }
              while(globals->Frames<101);
              tod = (ciaa->ciatodhi<<16)|(ciaa->ciatodmid<<8)|ciaa->ciatodlow;
              print("          ",WHITE);
              print("Counter: ",WHITE);
              print(binDec(tod),CYAN);

              if(tod<98)
              {
                     print(" - TOO SLOW",RED);
                     globals->ODDTODOK=0;
              }
              else
              if(tod>122)
              {
                     print("  - TOO FAST",RED);
                     globals->ODDTODOK=0;
              }
              else
              {
                     print("  - OK", GREEN);
                     globals->ODDTODOK=1;
              }
              print("\n",WHITE);

       print("\n Testing EVEN CIA Steps of 1ms each\n",WHITE);
       print("\nCIAB Timer A:",GREEN);
       counter = 0;
       globals->Frames=0;
       print("             ",WHITE);
       do
       {
              custom->color[0]=0x0;
              ciab->ciatalo=198;
              ciab->ciatahi=2;     // Set 1ms timing
              ciab->ciacra=CIACRAF_RUNMODE|CIACRAF_LOAD;
              ciab->ciacra=CIACRAF_START|CIACRAF_RUNMODE;
              timeout = globals->Frames;
              counter++;
              do
              {
                     if(globals->Frames>timeout+10) 
                     {
                            timeout=0;
                            print("NO ICR Triggered, FAILED",RED);
                            failed=1;
                            break;
                     }
                     if(globals->Frames>200)
                     {
                           print("TIMEOUT",RED);
                            failed=1;
                            //break;
                     }
                     ciadone = ((volatile uint8_t)ciab->ciaicr&1)==0;
       } while (!failed&&ciadone);
              togglePwrLED();
              custom->color[0]=0x660;
                            timeout = globals->Frames;
              } while (globals->Frames<=122);

              print("Counter: ",WHITE);
              print(binDec(counter),CYAN);
              globals->EVENCIATIMEROK=0;
              if(checkCiaOK(counter)==1)
              {
                      globals->EVENCIATIMEROK=1;      
              }
       print("CIAB Timer B:",GREEN);

       counter = 0;
       globals->Frames=0;
       print("             ",WHITE);
       do
       {
              custom->color[0]=0x0;
              ciab->ciatblo=198;
              ciab->ciatbhi=2;     // Set 1ms timing
              ciab->ciacrb=CIACRAF_RUNMODE|CIACRAF_LOAD;
              ciab->ciacrb=CIACRAF_START|CIACRAF_RUNMODE;
              timeout = globals->Frames;
              counter++;
              do
              {
                     if(globals->Frames>timeout+10) 
                     {
                            timeout=0;
                            print("NO ICR Triggered, FAILED",RED);
                            failed=1;
                            break;
                     }
                     if(globals->Frames>200)
                     {
                           print("TIMEOUT",RED);
                            failed=1;
                            //break;
                     }
                     ciadone = ((volatile uint8_t)ciab->ciaicr&2)==0;
       } while (!failed&&ciadone);
              togglePwrLED();
              custom->color[0]=0x660;
                            timeout = globals->Frames;
              } while (globals->Frames<=122);

              print("Counter: ",WHITE);
              print(binDec(counter),CYAN);
              globals->EVENCIATIMEROK=0;
              if(checkCiaOK(counter)==1)
              {
                      globals->EVENCIATIMEROK=1;      
              }

              print("CIAB TOD (Hsync)",GREEN);
              ciab->ciacrb=!CIACRBB_ALARM;
              ciab->ciatodhi=0;
              ciab->ciatodmid=0;
              ciab->ciatodlow=0;
              globals->Frames=0;
              do
              {
                     custom->color[0]=0x006;
                     custom->color[0]=0x000;
              }
              while(globals->Frames<101);
              tod = (ciab->ciatodhi<<16)|(ciab->ciatodmid<<8)|ciab->ciatodlow;
              print("          ",WHITE);
              print("Counter: ",WHITE);
              print(binDec(tod),CYAN);
              int ntsc = 0;
              if(tod>25900 && tod <26800)   // ~101 NTSC frames x 262.5 lines
              {
                     print("  - OK", GREEN);
                     print(" 60Hz",WHITE);
                     ntsc = 1;
              }
              if(ntsc!=1)
              {
                     if(tod<30800)
                     {
                            print(" - TOO SLOW",RED);
                     }
                     else
                     if(tod>31700)
                     {
                            print("  - TOO FAST",RED);
                     }
                     else
                     {
                            print("  - OK", GREEN);
                     }
              }
              print("\n",WHITE);

              print("\nCheck if CIAA can trigger IRQ2: ",WHITE);

       ciaa->ciacra = 0x00;          // Stop timer
       ciaa->ciaicr = 0x7f;          // Clear all CIAA ICR enables (bit7=0 = clear mode)
       (void)ciaa->ciaicr;           // Read ICR to clear any pending flags
       custom->intreq = IR2;         // Clear any stale INTREQ bit 3 (leftover from polled tests)
       globals->Frames=0;
       globals->IRQ2=0;              // Reset flag before enabling interrupts
       ciaa->ciatalo = 0x10;
       ciaa->ciatahi = 0x00;
       ciaa->ciaicr = 0x81;          // Enable Timer A interrupt
       custom->intena = 0xc000+IR2+IR3;
       custom->intena = 0xc000+IR2+IR3;
       ciaa->ciacra = 0x11;          // Start timer last (minimize race window)
              do
              {
                     custom->color[0]=0x006;
                     custom->color[0]=0x000;
              }
              while(!globals->IRQ2&&globals->Frames<50);

                     ciaa->ciacra = 0x00;
       if(globals->IRQ2!=0)
       {
              print("OK",GREEN);
              globals->ODDIRQOK=1;
       }
       else
       {
              print("FAILED",RED);
              globals->ODDIRQOK=0;
       }

              print("\nCheck if CIAB can trigger IRQ6: ",WHITE);

       custom->intena = 0xc000+IR3+IR6;
       custom->intena = 0xc000+IR3+IR6;

       ciab->ciacra = 0x00;
       ciab->ciatalo = 0x10;
       ciab->ciatahi = 0x00;
       ciab->ciaicr = 0x81;
       ciab->ciacra = 0x11;
       globals->IRQ6=0;
       globals->Frames=0;
              do
              {
                     custom->color[0]=0x006;
                     custom->color[0]=0x000;
              }
              while(!globals->IRQ6&&globals->Frames<50);
       ciab->ciacra = 0x00;
       if(globals->IRQ6!=0)
       {
              print("OK",GREEN);
       }
       else
       {
              print("FAILED",RED);
       }

       custom->intena = 0xc000;
       custom->intena = 0xc000;

}

int checkCiaOK(int counter)
{
              int result=0;

              if(counter<2050)     // was 2200: real-68000 loop overhead
              {                    // (DMA contention, E-clock sync) lands
                     result=2;     // healthy machines near ~2100-2180
              }
              else
              if(counter>2600)
              {
                     result=3;
              }
              else
              {
                     print("  - OK", GREEN);
                     result=1;
              }

              if(result!=1)
                     {
                     if(counter<1700)
                     {
                            result=2;
                     }
                     else
                     if(counter>2049)
                     {
                            result=3;
                     }
                     else
                     {
                            print("  - OK", GREEN);
                            print("  60Hz",WHITE);
                            result=1;
                     }
       }
              if(result!=1)
              {
                     if(result==2)
                            print(" - TOO SLOW",RED);
                     if(result==3)
                            print(" - TOO FAST",RED);
              }
       print("\n",WHITE);
       return result;
}

void ciaok(VARS)
{
       if(globals->ODDCIATIMEROK)
       {
              print("ODD Timer OK\n",GREEN);
       }
              else
              print("FAIL",RED);
       
              if(globals->ODDTODOK)
       {
              print("ODD TOD OK\n",GREEN);
       }
              else
              print("FAIL",RED);
       
                     if(globals->ODDIRQOK)
       {
              print("ODD IRQ OK\n",GREEN);
       }
              else
              print("FAIL",RED);

                           if(globals->ODDALARMOK)
      {
             print("ODD ALARM OK\n",GREEN);
      }
              else
              print("FAIL",RED);
       return;
}

__interrupt void IRQCode(VARS)
{
       custom->color[0]=0x0f0;
       int irq = custom->intreqr;
       custom->intreq = irq&0x70;
       custom->intreq = irq&0x70;
       if(irq&0x20)                              // Check if it is a VBlank IRQ
       {
              globals->Frames++;
       }
       globals->IRQLevDone=3;
       globals->IRQ3+=1;
}


__interrupt void IRQCode2(VARS)
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       struct CIA *ciab = (struct CIA *)0xbfd000;
       custom->color[0]=0xfff;
       custom->intreq = 0x78;
       ciaa->ciaicr = 0x7f;
       ciab->ciaicr = 0x7f;
}



void triggerIRQ(VARS, int num, int mask)
{
       globals->IRQ1=0;
       globals->IRQ2=0;
       globals->IRQ3=0;
       globals->IRQ4=0;
       globals->IRQ5=0;
       globals->IRQ6=0;
       custom->intreq=0x8000+mask;
       custom->intreq=0x8000+mask;
       print("\nTrigger IRQ: ",WHITE);
       print(binDec(num),GREEN);
       int Frames=0;
       volatile uint16_t* irqlevdone = &globals->IRQLevDone;
      do
       {
              do
              {
              } while (custom->vhposr>>8!=0x40);
              do
              {
              } while (custom->vhposr>>8!=0x41);
              Frames++;
         } while (!(Frames>40) && (*irqlevdone==0));
                  print(" Triggered IRQ: ",WHITE);
         print(binDec(globals->IRQLevDone),GREEN);
         if(num==3)
         {
              globals->IRQ3OK=1;
         }

              if(globals->IRQLevDone!=num)
         {
              print("   -   ERROR",RED);
         }
         print("\n",WHITE);
}

__interrupt void IRQ1(VARS)
{
       custom->color[0]=0xf00;
       int irq = custom->intreqr;
       if(irq&0x4)
       {
              globals->IRQ1+=1;
              globals->IRQLevDone=1;
       }
       custom->intreq = irq&0x7;
       custom->intreq = irq&0x7;
}

__interrupt void IRQ2(VARS)
{
       custom->color[0]=0xff0;
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       int irq = custom->intreqr;
       globals->ICR = ciaa->ciaicr;       // Store icr to be handled later! and also so we do something so read is done as read clears IRQ
       globals->IRQ2+=1;
       custom->intreq = irq&0x8;
       custom->intreq = irq&0x8;
       irq = custom->intreqr;
       globals->IRQLevDone=2;
       ciaa->ciacra = 0x0;
}

__interrupt void IRQ3(VARS)
{
       custom->color[0]=0x0f0;
       int irq = custom->intreqr;
       custom->intreq = irq&0x70;
       custom->intreq = irq&0x70;
       globals->IRQ3+=1;
       if(irq&0x20)                              // Check if it is a VBlank IRQ
       {
            //  print("VBlank",RED);
       }
       else
       {
              globals->IRQLevDone=3;
       }
}

__interrupt void IRQ4(VARS)
{
       custom->color[0]=0x0ff;
       int irq = custom->intreqr;
       custom->intreq = irq&0x780;
       custom->intreq = irq&0x780;
       globals->IRQ4+=1;
       globals->IRQLevDone=4;
}

__interrupt void IRQ5(VARS)
{
       custom->color[0]=0x00f;
       int irq = custom->intreqr;
       globals->IRQ5+=1;
       custom->intreq = irq&0x1800;
       custom->intreq = irq&0x1800;
       custom->adkcon = 0x7fff;
       globals->IRQLevDone=5;
}

__interrupt void IRQ6(VARS)
{
       custom->color[0]=0xf0f;
       int irq = custom->intreqr;
       struct CIA *ciab = (struct CIA *)0xbfd000;
       int cia = custom->intreqr;
       globals->ICR = ciab->ciaicr;
       globals->IRQ6+=1;
       custom->intreq = irq&0x2000;
       custom->intreq = irq&0x2000;
       globals->IRQLevDone=6;
       ciab->ciacra = 0x0;
}

__interrupt void IRQ7(VARS)
{
       custom->color[0]=0xfff;
       globals->IRQ7=1;
}

void IRQTestC(VARS)
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
    int counter=0;
       initScreen();
              print("\002IRQ Test EXPERIMENTAL Written in C\n",WHITE);
              print("\nTo start IRQ test press any key, ESC or Right Mousebutton to cancel",GREEN);

        do
       {

              GetInput();
       }
             while(globals->BUTTON == 0);
              if(globals->GetCharData==0x1b)
                     return;
              if(globals->RMB==1)
                     return;
       print("\nSetting IRQ TEST\n",WHITE);

       *(volatile APTR *) + 0x64 = IRQ1;
       *(volatile APTR *) + 0x68 = IRQ2;
       *(volatile APTR *) + 0x6c = IRQ3;
       *(volatile APTR *) + 0x70 = IRQ4;
       *(volatile APTR *) + 0x74 = IRQ5;
       *(volatile APTR *) + 0x78 = IRQ6;
       *(volatile APTR *) + 0x7c = IRQ7;
       setSR(0x2000);
       custom->adkcon=0x7fff;
       custom->intena = 0xc000+IR1+IR2+IR3+IR4+IR5+IR6;
       globals->IRQ3OK=0;

       triggerIRQ(globals,1,0x4);
       triggerIRQ(globals,2,0x8);
       triggerIRQ(globals,3,0x40);
       triggerIRQ(globals,4,0x780);
       triggerIRQ(globals,5,0x1000);
       triggerIRQ(globals,6,0x2000);

       print("\n\nPress any button to exit",CYAN);

       custom->intreq=0x7fff;
       custom->intena=0x7fff;

       *(volatile APTR *) + 0x64 = RTEcode;
       *(volatile APTR *) + 0x68 = RTEcode;
       *(volatile APTR *) + 0x6c = RTEcode;
       *(volatile APTR *) + 0x70 = RTEcode;
       *(volatile APTR *) + 0x74 = RTEcode;
       *(volatile APTR *) + 0x78 = RTEcode;
       *(volatile APTR *) + 0x7c = RTEcode;

       ClearBuffer();

       do
       {

              GetInput();
       }
             while(globals->BUTTON == 0);

   //   PAUSEC();
}

// ---------------------------------------------------------------------------
// CIA TOD-bug check — fingerprints early 8520s vs the late 391078-01 respin.
//
// The bug (verified against WinUAE cia.cpp checkalarm(), which models what
// was measured on real chips): during a ripple carry the TOD counter shows a
// TRANSIENT wrong value for a couple of cycles — it counts ...2D 2E 2F ->
// 20 -> 30..., the just-wrapped digit visible before the carry lands. Time-
// keeping is unaffected, but the ALARM comparator sees the transient, so an
// alarm set to that phantom value false-triggers. All DIP-era 8520s
// (A1000..A500+/A600 boards) have it; the PLCC 391078-01 (A1200/A4000/CD32)
// fixed it. UAE only models the transient on the TODMED carry (alarm low 12
// bits zero); real chips reportedly glitch on the TODLOW carry too — so leg
// 3 is expected to stay silent in emulators either way and is a real-
// hardware data point.
//
// Method per chip, pure ICR polling (all CIA int sources masked, no
// vectors involved):
//   leg 1  sanity: a real alarm a few ticks ahead MUST fire — proves the
//          alarm/ICR path works, otherwise silence below would be
//          meaningless and we say so instead of "bug fixed".
//   leg 2  TOD=$002FFF alarm=$002000: never a stable counter value from
//          here (next stop is $003000), but exactly the phantom shown
//          during the MED carry -> fires only on a buggy chip.
//   leg 3  TOD=$00002D alarm=$000020: same idea on the LOW carry.
// CIA-A TOD ticks at the 50/60Hz TICK, CIA-B at HSYNC, hence the different
// frame budgets. Every wait is frame-bounded AND iteration-capped.
// ---------------------------------------------------------------------------
static void todSet(volatile struct CIA *cia, uint32_t v)
{
    cia->ciacrb &= (uint8_t)~CIACRBF_ALARM;
    cia->ciatodhi  = (uint8_t)(v >> 16);   // HI write stops the counter
    cia->ciatodmid = (uint8_t)(v >> 8);
    cia->ciatodlow = (uint8_t)v;           // LOW write restarts it
}

static void todSetAlarm(volatile struct CIA *cia, uint32_t v)
{
    cia->ciacrb |= CIACRBF_ALARM;
    cia->ciatodhi  = (uint8_t)(v >> 16);
    cia->ciatodmid = (uint8_t)(v >> 8);
    cia->ciatodlow = (uint8_t)v;
    cia->ciacrb &= (uint8_t)~CIACRBF_ALARM;
}

// Read the live TOD counter. HI read engages the latch, LOW read releases
// it, so reading HI->MID->LOW is the correct full-value protocol.
static uint32_t todRead(volatile struct CIA *cia)
{
    uint32_t hi  = cia->ciatodhi;
    uint32_t mid = cia->ciatodmid;
    uint32_t lo  = cia->ciatodlow;
    return (hi << 16) | (mid << 8) | lo;
}

// 1 = ALRM flag set within `frames` VBL frames, 0 = stayed silent.
static int todAlarmWait(volatile struct CIA *cia, int frames)
{
    uint32_t spin = 2000000UL;             // hard cap in case VBL/Frames dies
    globals->Frames = 0;
    do {
        if (cia->ciaicr & CIAICRF_ALRM)    // read also clears
            return 1;
        // Touch TODLOW every pass: side-effect-free on real chips (only a
        // HI read engages the latch), but REQUIRED under UAE, whose lazy
        // TOD machinery only materializes pending increments (and their
        // alarm compares) on register access — found 2026-08-08 when the
        // CIA-A leg starved on an emulated A3000 while CIA-B passed.
        (void)cia->ciatodlow;
    } while ((int)globals->Frames < frames && --spin);
    return 0;
}

static void todBugTestOne(volatile struct CIA *cia, const char *name, int isCiaA)
{
    int budget = isCiaA ? 60 : 3;          // frames; covers >=3 TOD ticks each

    print((char *)name, CYAN);
    cia->ciaicr = 0x7F;                    // mask ALL CIA int sources: poll only
    (void)cia->ciaicr;                     // clear stale flags

    todSet(cia, 0x000010);
    todSetAlarm(cia, 0x000013);            // real alarm 3 ticks ahead
    if (!todAlarmWait(cia, budget)) {
        // Print the evidence, not just a verdict: where TOD actually is
        // tells apart a dead tick source, an ignored TOD write, and a dead
        // alarm compare.
        uint32_t now = todRead(cia);
        print(" alarm never fired. TOD $000010 -> ", RED);
        print(binHex(now), RED);
        print(" during wait:\n   ", RED);
        if (now == 0x000010)
            print("TOD never ticked - tick source dead?", RED);
        else if (now < 0x000013)
            print("TOD ticking but too slow for the wait budget?!", RED);
        else
            print("TOD passed the alarm value silently - alarm/ICR faulty.", RED);
        print(" No TOD-bug verdict.\n", RED);
        return;
    }
    print(" alarm circuit OK.", GREEN);

    (void)cia->ciaicr;
    todSet(cia, 0x002FFF);
    todSetAlarm(cia, 0x002000);            // phantom value of the MED carry
    int med = todAlarmWait(cia, budget);

    (void)cia->ciaicr;
    todSet(cia, 0x00002D);
    todSetAlarm(cia, 0x000020);            // phantom value of the LOW carry
    int low = todAlarmWait(cia, budget);

    print("  carry glitch: MED ", WHITE);
    print(med ? "YES" : "no ", med ? YELLOW : GREEN);
    print("  LOW ", WHITE);
    print(low ? "YES" : "no ", low ? YELLOW : GREEN);
    print("\n   -> ", WHITE);
    if (med || low)
        print("TOD BUG PRESENT - early 8520 (pre-391078-01)\n", YELLOW);
    else
        print("TOD bug not detected - 391078-01 class (or fixed)\n", GREEN);

    cia->ciaicr = 0x7F;                    // leave nothing armed
    (void)cia->ciaicr;
}

static void todBugTest(void)
{
    print("\nCIA TOD-bug check (early 8520 vs 391078-01):\n", WHITE);
    todBugTestOne((volatile struct CIA *)0xbfe001, " ODD  CIA (A):", 1);
    todBugTestOne((volatile struct CIA *)0xbfd000, " EVEN CIA (B):", 0);
}

void IRQCIATestC(VARS)
{
       initScreen();
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       struct CIA *ciab = (struct CIA *)0xbfd000;

       ciab->ciatbhi=0;
       ciab->ciatblo=0;
       ciab->ciatahi=0;
       ciab->ciatalo=0;

              print("\002CIA Test\n",WHITE);
              print("This test requires IRQ3 to work. Is it tested: ",GREEN);
       if(globals->IRQ3OK == 1)
              print("YES\n",GREEN);
              else
              {
                     print("NO ",RED);
                     print("Test might be unreliable\n",GREEN);
              }
       print("\002This is during DEV and tests MIGHT be unreliable!\n",CYAN);
       globals->Frames=0;
       custom->color[0]=0x60;

       *(volatile APTR *) + 0x68 = IRQ2;
       *(volatile APTR *) + 0x6c = IRQCode;
       *(volatile APTR *) + 0x78 = IRQ6;

       custom->intena = 0xc000+IR3;
       custom->intena = 0xc000+IR3;
       setSR(0x2000);                           // ; Start IRQ
       polledcia(globals);
       timedcia(globals);

       todBugTest();                      // last CIA test: 8520 revision fingerprint
       //ciaok(globals);

       print("\n\nDONE. Press any key/button to exit",WHITE);

             custom->intreq=0x7fff;
             custom->intena=0x7fff;
      
             *(volatile APTR *) + 0x64 = RTEcode;
             *(volatile APTR *) + 0x68 = RTEcode;
             *(volatile APTR *) + 0x6c = RTEcode;
             *(volatile APTR *) + 0x70 = RTEcode;
             *(volatile APTR *) + 0x74 = RTEcode;
             *(volatile APTR *) + 0x78 = RTEcode;
             *(volatile APTR *) + 0x7c = RTEcode;

       ClearBuffer();
       do
       {

              GetInput();
       }
             while(globals->BUTTON == 0);

             ClearBuffer();
}
