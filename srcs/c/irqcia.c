// C Version of the IRQ CIA Tests
//void Print(__reg("a0") int *string, __reg("d1") int color);
#include "globalvars.h"
#include "generic.h"

void IRQCIATestC(VARS)
{
       InitScreen();
       Print("Test",1);
       Print("Bahh test",2);
       GetInput();
        //Print(hexbin(3),1);
       PAUSE();
       while(globals->BUTTON==0)
       {
              GetInput();
       }
}
