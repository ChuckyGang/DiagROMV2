// C Version of the IRQ CIA Tests
#include "globalvars.h"
#include "generic.h"
#include <hardware/cia.h>

// YES I know this is a messy file..  I am just screwing around with ideas and tests now..
// will be more tidy when I know how I want shit

static void setSR(__reg("d0") uint16_t v) = "\tmove\td0,sr\n";
void IRQCode();
void IRQCode2();
void IRQ1();
void IRQ2();
void IRQ3();
void IRQ4();
void IRQ5();
void IRQ6();
void IRQ7();
int readCIAA();
int readCIAAa();

       #define IR1 0x7
       #define IR2 0x8
       #define IR3 0x70
       #define IR4 0x780
       #define IR5 0x1800
       #define IR6 0x2000

void IRQCIATestC(VARS)
{
       InitScreen();
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       struct CIA *ciab = (struct CIA *)0xbfd000;

       ciab->ciatbhi=0;
       ciab->ciatblo=0;
       ciab->ciatahi=0;
       ciab->ciatalo=0;

              Print("\002CIA Test\n",WHITE);
              Print("This test requires IRQ3 to work. Is it tested: ",GREEN);
       if(globals->IRQ3OK == 1)
              Print("YES\n",GREEN);
              else
              {
                     Print("NO ",RED);
                     Print("Test might be unreliable\n",GREEN);
              }
       Print("\002This is during DEV and output will make NO sense whatsoever to people!\n",CYAN);
       Print("\002BUT DO PLEASE SEND ME SCREENSHOTS OF YOUR MACHINE WITH MACHINESPEC\n",CYAN);
       Print("\002WHAT MACHINE. WHAT OSC. PAL/NSTC ETC\n",CYAN);
       globals->Frames=0;
       custom->color[0]=0xf0;

       *(volatile APTR *) + 0x68 = IRQ2;
       *(volatile APTR *) + 0x6c = IRQCode;
       *(volatile APTR *) + 0x78 = IRQ6;

       //globals->Frames=50;
       Print("\nIRQ2 HITS: ",WHITE);
       Print(bindec(globals->IRQ2),GREEN);

       custom->intena = 0xc000+IR2+IR3+IR6;
       custom->intena = 0xc000+IR2+IR3+IR6;
       setSR(0x2000);                           // ; Start IRQ

       polledcia();

       //polledcia();

       do
       {

              GetInput();
       }
             while(globals->BUTTON == 0);

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
}


void polledcia(VARS)
{
       int counter=0;
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       struct CIA *ciab = (struct CIA *)0xbfd000;
       Print("\nTOD is:  ",CYAN);
       globals->IRQ2=0;
       globals->IRQ6=0;
       ciaa->ciatodhi=0;
       ciaa->ciatodmid=0;
       ciaa->ciatodlow=0;          // Clear CIA, meaning also start the timer
       Print(binhex(readTODA()),WHITE);
       
       Print("\n\n Testing EVEN CIA\n",WHITE);
       Print("\n\nCIAA Timer A:",GREEN);
       ciaa->ciacra=CIACRAF_START|CIACRAF_SPMODE|CIACRAF_LOAD;
       ciaa->ciaicr = 0x7e;
       ciaa->ciaicr = 0x81;
       BYTE ciadone;
       int timeout=0;
       int failed=0;
       globals->Frames=0;
       do
       {
              ciaa->ciatalo=0xb5; //ae
              ciaa->ciatahi=0x1b; //00                   //     time to wait
              timeout = globals->Frames;
              do
              {
                     if(globals->Frames>timeout+30) 
                     {
                            timeout=0;
                            Print("NO ICR Triggered, FAILED",RED);
                            failed=1;
                            break;
                     }
                     ciadone = (ciaa->ciaicr&1)==0;
              } while (ciadone);
                     custom->color[0]=0x335;
                     counter++;
                     (*(volatile unsigned char *)0xbfe001) ^=(1<<1);
       } while (globals->Frames<200);
       Print(bindec(counter),GREEN);
       

//       for(int a=0;a<200;a++)                                  // Do aloop 200 times with a wait via CIA timer A
//       {
//              ciaa->ciatalo=0xb5; //ae
//              ciaa->ciatahi=0x1b; //00                   //     time to wait
//              do
//              {
//                     timeout++;
//                     custom->color[1]=timeout;
//                     if(timeout>2850000) 
//                     {
//                            timeout=0;
//                            Print("NO ICR Triggered, FAILED",RED);
//                            failed=1;
//                            break;
//                     }
//                     ciadone = (ciaa->ciaicr&1)==0;
//              } while (ciadone);
//                     custom->color[0]=0x335;
//                     counter++;
//                     timeout=0;
//                     if(failed==1)
//                     {
//                            break;
//                     }
//       }
//       //while (!(Frames>40) && (globals->IRQLevDone==0));

//      ciaa->ciacra=!CIACRAF_START|!CIACRAF_SPMODE|!CIACRAF_LOAD;

      Print("\nCIAA Timer B:",GREEN);

       failed=0;
       timeout=0;
       Print(bindec(timeout),GREEN);
       ciaa->ciacrb=CIACRBF_START|CIACRBF_LOAD;
       for(int a=0;a<200;a++)                                  // Do aloop 200 times with a wait via CIA timer B
       {
              ciaa->ciatblo=0xb5; //ae
              ciaa->ciatbhi=0x1b; //00                   //     time to wait
              do
              {
                     timeout++;
                     if(timeout>1850000) 
                     {
                            custom->color[0]=0xff3;
                            timeout=0;
                            Print("NO!! ICR Triggered, FAILED",RED);
                            failed=1;
                            break;

                     }
                     ciadone = (ciaa->ciaicr&2)==0;
              } while (ciadone);
                     custom->color[0]=0x440;
                     counter++;
                     timeout=0;
                     if(failed==1)
                     {
                            break;
                     }
       }
       ciaa->ciacrb=!CIACRBF_START|!CIACRBF_LOAD;

    //               } while (!(timeout>30000) && (globals->ICR==0));

       //PAUSEC();

       Print("\n\nNumber of frames: ",WHITE);
       Print(bindec(globals->Frames),WHITE);
       Log("Counter: ",counter);
       Print("\nTOD is:  ",BLUE);
       Print(binhex(readTODA()),WHITE);
       Print("\nIRQ2 (if not 0 your cia CAN trigger IRQs! YAY!): ",WHITE);
       Print(bindec(globals->IRQ2),GREEN);

       // ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

       Print("\n\n Testing ODD CIA\n",WHITE);
       Print("\nIRQ6: ",WHITE);
       Print(bindec(globals->IRQ6),GREEN);

       Print("\nTOD is:  ",BLUE);
      Print(binhex(readTOD()),WHITE);
      Print("\nNumber of frames: ",WHITE);
      Print(bindec(globals->Frames),WHITE);

      Print("\nCIAB Timer A:",GREEN);
       ciab->ciatodhi=0;
       ciab->ciatodmid=0;
       ciab->ciatodlow=0;
     //  ciab->ciaicr = 0x7e;
     //  ciab->ciaicr = 0x81;
      failed=0;
      timeout=0;
       globals->Frames=0;
       for(int a=0;a<200;a++)                                  // Do aloop 200 times with a wait via CIA timer A
       {
             ciab->ciacra=CIACRAF_START|CIACRAF_RUNMODE;//|!CIACABF_LOAD;
            ciab->ciatalo=0xb5; //ae
             ciab->ciatahi=0x1b; //00                   //     time to wait
              do {
                     timeout++;
                     if(timeout>5850000) 
                     {
                            custom->color[0]=0xff3;
                            timeout=0;
                            Print("NO!! ICR Triggered, FAILED",RED);
                            //failed=1;
                            break;

                     }
                     ciadone = (ciab->ciaicr&1)==0;
                 //   Print(binhex((ciab->ciatahi<<8)|ciab->ciatalo),WHITE);
                 //   Print(" ",WHITE);
                 //   Print(binhex(ciadone),CYAN);
                                       // Print("\n",WHITE);
              } while (ciadone);
                     custom->color[0]=0x335;
                     counter++;
                     timeout=0;
                     if(failed==1)
                     {
                            break;
                     }
       }

      Print("\nCIAB Timer B:",GREEN);
       failed=0;
       for(int a=0;a<200;a++)                                  // Do aloop 200 times with a wait via CIA timer A
       {
            ciab->ciatblo=0xb5; //ae
             ciab->ciatbhi=0x1b; //00                   //     time to wait
             ciab->ciacra=CIACRBF_START|CIACRBF_RUNMODE|!CIACRBF_LOAD;
             do {
                     timeout++;
                     if(timeout>1850000) 
                     {
                            custom->color[0]=0xff3;
                            timeout=0;
                            Print("NO!! ICR Triggered, FAILED",RED);
                            failed=1;
                            break;

                     }
                     ciadone = (ciab->ciaicr&2)==0;
                    Print(binhex((ciab->ciatbhi<<8)|ciab->ciatblo),WHITE);
                    Print(" ",WHITE);
                    Print(binhex(ciadone),CYAN);
                    Print("\n",WHITE);
              } while (ciadone);
                     custom->color[0]=0x335;
                     counter++;
                     timeout=0;
                     if(failed==1)
                     {
                            break;
                     }
       }


      
      Print("\nTOD is:  ",BLUE);
      Print(binhex(readTOD()),WHITE);
      Print("\nNumber of frames: ",WHITE);
      Print(bindec(globals->Frames),WHITE);
      Print("\nIRQ6: ",WHITE);
      Print(bindec(globals->IRQ6),GREEN);
}

void crapcode(VARS)
{
       PAUSEC();
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       struct CIA *ciab = (struct CIA *)0xbfd000;


       return 0;

       //       char cia=ciaa->ciacra;
       //       cia=cia&&0xc0;
       //       cia=cia||8;
       //       ciaa->ciacra=cia;

       //readCIAA();

//       custom->intena = 0x20;
//       custom->intena = 0x20;

       ciab->ciacrb=clearBit(ciaa->ciacrb,6);
       ciaa->ciatodhi=0;
       ciaa->ciatodmid=0;
       ciaa->ciatodlow=0;
 
       //return 0;
       int tod,tod1;
/*
       //int tod=(ciaa->ciatodlow)|(ciaa->ciatodmid<<8)|(ciaa->ciatodhi<<16);


       tod=readTOD();
       do
       {
              // code
       } while (tod!=readTOD());
       

       Log("tod: ",tod);

       for(int a=0;a<500000;a++);

       tod=readTOD();

       Log("tod: ",tod);
*/
       tod=0;
       int hi,lo;

       hi=ciaa->ciatahi;
       lo=ciaa->ciatalo;
       tod=(hi<<8)|lo;
       Log("tod: ",tod);

       hi=ciaa->ciatahi;
       lo=ciaa->ciatalo;
       tod=(hi<<8)|lo;
       Log("tod: ",tod);

       hi=ciaa->ciatahi;
       lo=ciaa->ciatalo;
       tod=(hi<<8)|lo;
       Log("tod: ",tod);

       hi=ciaa->ciatalo;
       lo=ciaa->ciatahi;
       tod=(hi<<8)|lo;
       Log("tod: ",tod);


      // ciaa->ciatodhi=0;
      // ciaa->ciatodmid=0;
      // ciaa->ciatodlow=0;


       ciaa->ciacra = !CIACRBF_START | !CIACRBF_RUNMODE | CIACRBF_LOAD;
      // ciaa->ciacrb = !CIACRBF_START | CIACRBF_LOAD;

              ciaa->ciatalo=20;
              ciaa->ciatahi=20;

              ciaa->ciatblo=0;
              ciaa->ciatbhi=0;
       ciab->ciacra = !CIACRBF_START | CIACRBF_LOAD;



       //PAUSEC();
      // return 0;

       //*(volatile APTR *) + 0x68 = IRQCode2;
       //setSR(0x2000);                           // ; Start IRQ
       //custom->intena = 0xc008;
       //custom->intena = 0xc008;


       //ciaa->ciacra = CIACRBF_START | CIACRBF_LOAD;
       //ciaa->ciacrb = CIACRBF_START | CIACRBF_LOAD;
      // ciaa->ciaicr = 0x7f;
      //  ciab->ciaicr = 0x7f;
      // for(int a=0;a<10;a++)
      // {
      // hi=ciaa->ciatahi;
      // lo=ciaa->ciatalo;
      // tod=(hi<<8)|lo;
     //  Print(binhex(readCIAAa()),WHITE);
     //  Print(" ",WHITE);
     //  Print(binhex(readCIAAb()),WHITE);
     //  Print("\n",WHITE);
       

       //custom->intena = 0x7fff;
       //custom->intena = 0x7fff;
       //setSR(0);
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

void crap(VARS)
{
       /*       
 
// To wait 1/100 second would require waiting 10,000 microseconds.
// The timer register would be set to (10,000 / 1.3968255).


      // custom->intreq = 0x7fff;
      // custom->intena = 0x7fff;             //    ; Disable all IRQs



    ciaa->ciatblo = 0;
    ciaa->ciatbhi = 0;
    ciaa->ciacrb = CIACRBF_START | CIACRBF_LOAD;

       //globals->Frames=0;

       Print("HEJSAN",GREEN);

       /*while(globals->Frames<200)
       {
        //      Log(globals->Frames);
        //counter++;
        //Log(globals->Frames);
      }


       if(globals->NTSC==1)
       {
              Print("NTSC",WHITE);
       }
       Log("dff004: ",custom->vposr);
       Log("tblo: ",ciaa->ciatblo);
       Log("tbhi: ",ciaa->ciatbhi);

        Print("SVEJSAN",BLUE);


       custom->intreq = 0x7fff;
       custom->intena = 0x7fff;             //    ; Disable all IRQs
       Print("Number of frames: ",WHITE);
       Print(bindec(globals->Frames),WHITE);
       *(volatile APTR *) + 0x6c = RTEcode;
       Log("vhposdir: ",(custom->vhposr&0xff00)>>8);
       do
              {
              }
       while(((custom->vhposr&0xff00)>>8)!=100)
    ciaa->ciatblo = 0;
    ciaa->ciatbhi = 0;
    ciaa->ciacrb = CIACRBF_START | CIACRBF_LOAD;
       Log("tblo: ",ciaa->ciatblo);
       Log("tbhi: ",ciaa->ciatbhi);

       do
       {
              custom->color[0]=0x00;
       }
       while(((custom->vhposr&0xff00)>>8)!=110)
       {
              custom->color[0]=0xfff;
       }
              Log("tblo: ",ciaa->ciatblo);
       Log("tbhi: ",ciaa->ciatbhi);


*/
}
__interrupt void IRQCode(VARS)
{
       int irq = custom->intreqr;
       custom->intreq = irq&0x70;
       custom->intreq = irq&0x70;
       if(irq&0x20)                              // Check if it is a VBlank IRQ
        {
          // PAUSEC();
           globals->Frames++;
       }
       else
       {
       globals->IRQLevDone=3;
       globals->IRQ3+=1;
       }
       int irq2 = custom->intreqr;
}


__interrupt void IRQCode2(VARS)
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       struct CIA *ciab = (struct CIA *)0xbfd000;
       Log("IRQ2",2);
       custom->color[0]=0xfff;
       //custom->intreq = 0x8;
       //custom->intreq = 0x8;
      // custom->intreq = 0x7fff;
      // custom->intreq = 0x7fff;
       custom->intreq = 0x78;
       //custom->intena = 0x78;
       ciaa->ciaicr = 0x7f;
       ciab->ciaicr = 0x7f;
}

void IRQTestC(VARS)
{
       static void setSR(__reg("d0") uint16_t v) = "\tmove\td0,sr\n";
       struct CIA *ciaa = (struct CIA *)0xbfe001;
    int counter=0;
       InitScreen();
              Print("\002IRQ Test EXPERIMENTAL Written in C\n",WHITE);
              Print("\nTo start IRQ test press any key, ESC or Right Mousebutton to cancel",GREEN);

        do
       {

              GetInput();
       }
             while(globals->BUTTON == 0);
              if(globals->GetCharData==0x1b)
                     return;
              if(globals->RMB==1)
                     return;
       Print("\nSetting IRQ TEST\n",WHITE);

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

       triggerIRQ(1,0x4);
       triggerIRQ(2,0x8);
       triggerIRQ(3,0x40);
       triggerIRQ(4,0x780);
       triggerIRQ(5,0x1000);
       triggerIRQ(6,0x2000);

       Print("\n\nPress any button to exit",CYAN);

       ClearBuffer();

       do
       {

              GetInput();
       }
             while(globals->BUTTON == 0);

       custom->intreq=0x7fff;
       custom->intena=0x7fff;

       *(volatile APTR *) + 0x64 = RTEcode;
       *(volatile APTR *) + 0x68 = RTEcode;
       *(volatile APTR *) + 0x6c = RTEcode;
       *(volatile APTR *) + 0x70 = RTEcode;
       *(volatile APTR *) + 0x74 = RTEcode;
       *(volatile APTR *) + 0x78 = RTEcode;
       *(volatile APTR *) + 0x7c = RTEcode;
    //   ClearBuffer();
    //   PAUSEC();
}

void triggerIRQ(VARS, int num, int mask)
{
       //int mask = 0x4;
       globals->IRQ1=0;
       globals->IRQ2=0;
       globals->IRQ3=0;
       globals->IRQ4=0;
       globals->IRQ5=0;
       globals->IRQ6=0;
       globals->IRQLevDone=0;       
       custom->intreq=0x8000+mask;
       custom->intreq=0x8000+mask;
       Print("\nTrigger IRQ: ",WHITE);
       Print(bindec(num),GREEN);
       int Frames=0;
      do
       {
              do
              {
              } while (custom->vhposr>>8!=0x40);
              do
              {
              } while (custom->vhposr>>8!=0x41);
              Frames++;
         } while (!(Frames>40) && (globals->IRQLevDone==0));
                  Print(" Triggered IRQ: ",WHITE);
         Print(bindec(globals->IRQLevDone),GREEN);
         if(num==3)
         {
              globals->IRQ3OK=1;
         }

              if(globals->IRQLevDone!=num)
         {
              Print("   -   ERROR",RED);
         }
         Print("\n",WHITE);
}

__interrupt void IRQ1(VARS)
{
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
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       int irq = custom->intreqr;
       globals->ICR = ciaa->ciaicr;       // Store icr to be handled later! and also so we do something so read is done as read clears IRQ
       globals->IRQ2+=1;
      // custom->color[0]=0xff3;
       custom->intreq = irq&0x8;
       custom->intreq = irq&0x8;
       irq = custom->intreqr;
       globals->IRQLevDone=2;
}

__interrupt void IRQ3(VARS)
{
       int irq = custom->intreqr;
       custom->intreq = irq&0x70;
       custom->intreq = irq&0x70;
       if(irq&0x20)                              // Check if it is a VBlank IRQ
        {
            //  Print("VBlank",RED);
       }
       else
       {
       globals->IRQLevDone=3;
       globals->IRQ3+=1;
       }
       int irq2 = custom->intreqr;
}

__interrupt void IRQ4(VARS)
{
       int irq = custom->intreqr;
       custom->intreq = irq&0x780;
       custom->intreq = irq&0x780;
       globals->IRQ4+=1;
       globals->IRQLevDone=4;
}

__interrupt void IRQ5(VARS)
{
       int irq = custom->intreqr;
       globals->IRQ5+=1;
       custom->intreq = irq&0x1800;
       custom->intreq = irq&0x1800;
       custom->adkcon = 0x7fff;
       globals->IRQLevDone=5;
}

__interrupt void IRQ6(VARS)
{
       int irq = custom->intreqr;
       struct CIA *ciab = (struct CIA *)0xbfd000;
       int cia = custom->intreqr;
       globals->ICR = ciab->ciaicr;
       globals->IRQ6+=1;
       custom->intreq = irq&0x2000;
       custom->intreq = irq&0x2000;
       globals->IRQLevDone=6;
}

__interrupt void IRQ7(VARS)
{
        custom->color[0]=0xff;
       globals->IRQ7=1;
}