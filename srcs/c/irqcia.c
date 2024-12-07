// C Version of the IRQ CIA Tests
#include "globalvars.h"
#include "generic.h"
#include <hardware/cia.h>

void IRQCIATestC(VARS)
{
       InitScreen();
       Print("3",RED);
       Print("4",WHITE);
       char* str = binhex(3);
       Print(str,1);
       Print(bindec(3),2);
       PAUSEC();
       Print("BEFORE: ",WHITE);
       Print(bindec(255),WHITE);
       Print("AFTER",WHITE);
       int var = decbin("3233");
       Print(bindec(var),WHITE);
       //while( (*(unsigned char*)0xbfe001) & (1<<6) );
       //while( !( (*(unsigned char*)0xbfe001) & (1<<6) ));
       Print("LMB",GREEN);
       testPrint();
       testPrint();

       do
       {
              custom->color[0]=custom->vhposr;
              GetInput();
       }
              while(globals->BUTTON == 0);
}
