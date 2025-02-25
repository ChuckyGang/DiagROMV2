// C Version of the IRQ CIA Tests
#include "globalvars.h"
#include "generic.h"
#include <hardware/cia.h>


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
void IRQCIATestC(VARS)
{
       int counter=0;

       struct CIA *ciaa = (struct CIA *)0xbfe001;
       struct CIA *ciab = (struct CIA *)0xbfd000;
       InitScreen();
              Print("\0023CIA Test\n",WHITE);
              Print("This test requires IRQ3 to work. Is it tested: ",GREEN);
       if(globals->IRQ3OK == 1)
              Print("YES\n",GREEN);
              else
              {
              Print("NO ",RED);
              Print("Test might be unreliable\n",GREEN);
              }

       globals->Frames=0;
       
       custom->bplcon2=
       custom->color[0]=0xf0;
       custom->intena = 0x7fff;
       Log("irq: ",IRQCode);
       *(volatile APTR *) + 0x6c = IRQCode;
       setSR(0x2000);                           // ; Start IRQ
       custom->intena = 0xc020;
       custom->intena = 0xc020;

       for(int a=0;a<200;a++)
       {
              char cia=ciaa->ciacra;
              cia=cia&&0xc0;
              cia=cia||8;
              ciaa->ciacra=cia;
              ciaa->ciatalo=0xb5; //ae
              ciaa->ciatahi=0x1b; //00

              do
              {
                     custom->color[0]=0;
              } while (ciaa->ciaicr==0);
                     custom->color[0]=0xfff;
                     counter++;
       }

       Print("Number of frames: ",WHITE);
       Print(bindec(globals->Frames),WHITE);
       Log("Counter: ",counter);
       //       char cia=ciaa->ciacra;
       //       cia=cia&&0xc0;
       //       cia=cia||8;
       //       ciaa->ciacra=cia;

       //readCIAA();

       custom->intena = 0x20;
       custom->intena = 0x20;

       ciaa->ciacrb=clearBit(ciaa->ciacrb,6);
       ciaa->ciatodhi=0;
       ciaa->ciatodmid=0;
       ciaa->ciatodlow=0;
       //int tod=(ciaa->ciatodlow)|(ciaa->ciatodmid<<8)|(ciaa->ciatodhi<<16);

       int tod,tod1;
       tod=readTOD();
       do
       {
              /* code */
       } while (tod!=readTOD());
       

       Log("tod: ",tod);

       for(int a=0;a<500000;a++);

       tod=readTOD();

       Log("tod: ",tod);

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
              ciaa->ciacrb=clearBit(ciaa->ciacrb,6);
       ciaa->ciatodhi=0;
       ciaa->ciatodmid=0;
       ciaa->ciatodlow=0;

       ciaa->ciacra = !CIACRBF_START | !CIACRBF_RUNMODE | CIACRBF_LOAD;
      // ciaa->ciacrb = !CIACRBF_START | CIACRBF_LOAD;

              ciaa->ciatalo=20;
              ciaa->ciatahi=20;

              ciaa->ciatblo=0;
              ciaa->ciatbhi=0;
       ciaa->ciacra = !CIACRBF_START | CIACRBF_LOAD;

       hej:
       *(volatile APTR *) + 0x68 = IRQCode2;
       setSR(0x2000);                           // ; Start IRQ
       custom->intena = 0xc008;
       custom->intena = 0xc008;


       ciaa->ciacra = CIACRBF_START | CIACRBF_LOAD;
       //ciaa->ciacrb = CIACRBF_START | CIACRBF_LOAD;
       ciaa->ciaicr = 0x7f;
        ciab->ciaicr = 0x7f;
       for(int a=0;a<10;a++)
       {
       hi=ciaa->ciatahi;
       lo=ciaa->ciatalo;
       tod=(hi<<8)|lo;
       Print(binhex(readCIAAa()),WHITE);
       Print(" ",WHITE);
       Print(binhex(readCIAAb()),WHITE);
       Print("\n",WHITE);
       }

       //custom->intena = 0x7fff;
       //custom->intena = 0x7fff;
       //setSR(0);
 
       do

       {

              GetInput();
       }
             while(globals->BUTTON == 0);

      /* custom->intreq = 0x7fff;
       custom->intena = 0x7fff;             //    ; Disable all IRQs
       *(volatile APTR *) + 0x6c = RTEcode;
       *(volatile APTR *) + 0x68 = RTEcode;*/
}

int readTOD()
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       int todhi,todmid,todlow;
       todhi=ciaa->ciatodhi;
       todmid=ciaa->ciatodmid;
       todlow=ciaa->ciatodlow;
       return (todhi<<16)|(todmid<<8)|todlow;

}

int readCIAAa()
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       int hi=ciaa->ciatahi;
       int lo=ciaa->ciatalo;
       return(hi<<8)|lo;
}

int readCIAAb()
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       int hi=ciaa->ciatbhi;
       int lo=ciaa->ciatblo;
       return(hi<<8)|lo;
}


int readECLK()
{
/*
       read_eclk
    movem.l    d1-d5,-(sp)
    ; read eclk 709/716 kHz (32bit counter; wraps in ~100 min)
eclk_retry
    move.b    $bfdd00,d5    ; icr (clear)
    move.b    $bfd700,d0    ; tbhi
    move.b    $bfd600,d1    ; tblo
    move.b    $bfd500,d2    ; tahi
    move.b    $bfd400,d3    ; talo
    move.b    $bfd500,d4    ; tahi again
    move.b    $bfdd00,d5    ; icr (check)
    btst    #0,d5        ; ta underflowed?
    bne.b    eclk_retry
    cmp.b    d4,d2        ; talo byte wrapped?
    beq.b    eclk_ok
    move.b    d4,d2        ; update tahi
    move.b    #$ff,d3        ; reset talo
eclk_ok:
    lsl.w    #8,d0
    lsl.w    #8,d2
    move.b    d1,d0
    move.b    d3,d2
    swap    d0
    move.w    d2,d0
    not.l    d0
    movem.l    (sp)+,d1-d5
    rts
    */


}

int readCIAA()
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       int a,b;
       ciaa->ciatblo = 0;
       ciaa->ciatbhi = 0;
       ciaa->ciacrb = CIACRBF_START | !CIACRBF_LOAD;

       for(a=0;a<1000;a++);

       a=ciaa->ciatblo;
       b=ciaa->ciatbhi;



       int cia=(b<<8)|a;

       Log("a: ",a);
       Log("b: ",b);
       Log("cia: ",cia);
       Print(binhex(cia),WHITE);

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
              int hej=128
       //Log(counter);
       Log(hej);
       hej = hej<<3;
       Log(hej);
       hej=setBit(hej,1)
       Log(hej);
       hej=clearBit(hej,1)
       Log(hej);
       hej=toggleBit(hej,32)
       Log(hej);
       hej=toggleBit(hej,32)
       Log(hej);
}
__interrupt void IRQCode(VARS)
{
       globals->Frames++;
       custom->color[0]=0xff;
      // custom->intena = 0x70;
       custom->intreq = 0x70;

       //custom->intreq = 0xffff;
       //custom->intreq = 0x7fff;

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
              Print("\002IRQ Test modell ny!!!!\n",WHITE);
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

       ciaa->ciaicr=0xff;
       ciaa->ciaicr=0x7f;
       ciaa->ciaicr=0xf;
       //ciaa->ciaicr=0x81
       Print("\nSetting IRQ TEST\n",WHITE);

       *(volatile APTR *) + 0x64 = IRQ1;
       *(volatile APTR *) + 0x68 = IRQ2;
       *(volatile APTR *) + 0x6c = IRQ3;
       *(volatile APTR *) + 0x70 = IRQ4;
       *(volatile APTR *) + 0x74 = IRQ5;
       *(volatile APTR *) + 0x78 = IRQ6;
       *(volatile APTR *) + 0x7c = IRQ7;
       Print("\nIRQ Set\n",BLUE);

       // dummy read to clear out ICR before setting the ICR mask

       
       //custom->intreq = 0x7fff;
       //custom->intena = 0xc05e;

        //  setSR(0x2000);
        //         custom->intena = 0xefff;
        //  PAUSEC();
      //custom->intreq=0xc001;

      // do
      // {
      //        custom->color[0]=custom->vhposr>>8;
      // } while (TRUE);



       //custom->intreq=0x8004;

       //Print("\nDONE\n",GREEN);

       //Log("IRQ1: ",globals->IRQ1);
       //Log("IRQ2: ",globals->IRQ2);
       //Log("IRQ3: ",globals->IRQ3);
       //Log("IRQ4: ",globals->IRQ4);
       //Log("IRQ5: ",globals->IRQ5);
       //Log("IRQ6: ",globals->IRQ6);
       //Log("IRQ7: ",globals->IRQ7);

       //custom->intena = 0x8008;
       //Log("IRQ1: ",globals->IRQ1);
       //Log("IRQ2: ",globals->IRQ2);
       //Log("IRQ3: ",globals->IRQ3);
       //Log("IRQ4: ",globals->IRQ4);
       //Log("IRQ5: ",globals->IRQ5);
       //Log("IRQ6: ",globals->IRQ6);
       //Log("IRQ7: ",globals->IRQ7);



       //do
       //{
       //       custom->color[0]=custom->vhposr>>8;
       //} while (TRUE);

       // do
       //{

       //       GetInput();
      // }
       //      while(globals->BUTTON == 0);

       Print("Trigg!",GREEN);

       //DisableCache();
         setSR(0x2000);
                // custom->intena = 0xc07f;
                custom->adkcon=0x7fff;
                custom->intena = 0xe7ff;
       Log("IRQ1: ",globals->IRQ1);
       Log("IRQ2: ",globals->IRQ2);
       Log("IRQ3: ",globals->IRQ3);
       Log("IRQ4: ",globals->IRQ4);
       Log("IRQ5: ",globals->IRQ5);
       Log("IRQ6: ",globals->IRQ6);
       Log("IRQ7: ",globals->IRQ7); 

       PAUSEC();
       custom->intreq=0x8040;
       custom->intreq=0x8040;

       uint32_t a = ciaa->ciaicr;
       ciaa->ciaicr=(a >> 1) | 0xffff;             // Enable IRQ 1, 2 and 3 only so far

        do
       {

              GetInput();
              Print("  .  ",WHITE);
       }
             while(globals->BUTTON == 0);
       custom->intreq=0x7fff;
       custom->intena=0x7fff;
}


__interrupt void IRQ1(VARS)
{
       int irq = custom->intreqr;
       custom->intreq = irq&0x7;
       custom->intreq = irq&0x7;
       //Print("IRQ1",WHITE);
       globals->IRQ1+=1;
       custom->color[0]=0x555;
}

__interrupt void IRQ2(VARS)
{
       struct CIA *ciaa = (struct CIA *)0xbfe001;
       int irq = custom->intreqr;
       ciaa->ciaicr=0x7f;
       ciaa->ciaicr=0x81;
       int icr = ciaa->ciaicr;
       //Print("IRQ2",WHITE);
       //Print(bindec(ciaa->ciaicr));
       globals->IRQ2+=1;
       custom->color[0]=0x450;
       //custom->intreq = irq&0x8;
       //custom->intreq = irq&0x8;
       irq = custom->intreqr;
       //Log("IRQ2",irq);
}

__interrupt void IRQ3(VARS)
{
       int irq = custom->intreqr;
       //Print("IRQ3",WHITE);
       custom->intreq = irq&0x70;
       custom->intreq = irq&0x70;
       if(irq&0x20)                              // Check if it is a VBlank IRQ
        {
              //Log("VBlank",irq);
              Print("VBlank",RED);
         //     custom->color[0]=0x044;
        }
       else
       //custom->color[0]=0xfff;
       globals->IRQ3+=1;

       int irq2 = custom->intreqr;
       // Log("IRQ3",irq2);
}

__interrupt void IRQ4(VARS)
{
       Print("IRQ4",WHITE);
       int irq = custom->intreqr;
       custom->intreq = irq&0x780;
       custom->intreq = irq&0x780;
       //custom->color[0]=0xff4;
       globals->IRQ4+=1;
       //custom->intreq = cia;
}

__interrupt void IRQ5(VARS)
{
       int irq = custom->intreqr;
       Print("IRQ5",WHITE);
       Print(binhex(irq));
       Print(binhex(irq&1800));
       Print(binhex(custom->adkconr));
       Print("\n",WHITE);
       //custom->color[0]=0x222;
       globals->IRQ5+=1;
       custom->intreq = irq&1800;
       custom->intreq = irq&1800;
//       custom->adkcon = 0x7fff;
//       custom->adkcon=0x7fff;
}

__interrupt void IRQ6(VARS)
{
       Print("IRQ6",WHITE);
       int cia = custom->intreqr;
       //custom->color[0]=0xf3f;
       globals->IRQ6+=1;
       //custom->intreq = cia;
}

__interrupt void IRQ7(VARS)
{
       Log("IRQ7:",7);
       custom->color[0]=0xff;
      // custom->intena = 0x70;
       globals->IRQ7=1;
 //      custom->intreq = 0x70;
}