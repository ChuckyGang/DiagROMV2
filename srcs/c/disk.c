#include "globalvars.h"
#include "generic.h"
#include <hardware/cia.h>
void floppyTestC(VARS)
{
       volatile struct CIA *ciaa = (struct CIA *)0xbfe001;
       volatile struct CIA *ciab = (struct CIA *)0xbfd000;
       InitScreen();
       print("\002Floppytest (New Experimental)\n\n",CYAN);
       print("This test will run much better with working CIA Timer.\nIs CIA Timers tested ok: ",CYAN);
       if(globals->ODDCIATIMEROK==1)
       {
              print("ODD CIA OK ",GREEN);
       }
       else
       {
              print("ODD CIA NOT TESTED/NOT OK ",RED);
       }

       print(" | ",WHITE);

       if(globals->EVENCIATIMEROK==1)
       {
              print("EVEN CIA OK ",GREEN);
       }
       else
       {
              print("EVEN CIA NOT TESTED/NOT OK ",RED);
       }
       ciab->ciaprb &= ~(CIAB_DSKSEL0);
       ciab->ciaprb |= CIAB_DSKSEL0;

    uint32_t id = 0;
    uint8_t mask = CIAB_DSKSEL0;
    unsigned int i;

    ciab->ciaprb |= 0xf8;  /* motor-off, deselect all */
    ciab->ciaprb &= 0x7f;  /* 1. MTRXD low */
    ciab->ciaprb &= ~mask; /* 2. SELxB low */
    ciab->ciaprb |= mask;  /* 3. SELxB high */
    ciab->ciaprb |= 0x80;  /* 4. MTRXD high */
    ciab->ciaprb &= ~mask; /* 5. SELxB low */
    ciab->ciaprb |= mask;  /* 6. SELxB high */
    for (i = 0; i < 32; i++) {
        ciab->ciaprb &= ~mask; /* 7. SELxB low */
        id = (id<<1) | ((ciaa->ciapra>>5)&1); /* 8. Read and save state of RDY */
        ciab->ciaprb |= mask;  /* 9. SELxB high */
    }

    print("Drive ID: ",WHITE);
    print(binHex(id),WHITE);

    putChar('a',RED,10,10);
    putChar('a',R_RED,11,10);

    putChar('a',CYAN,20,10);
    putChar('a',R_CYAN,21,10);
    PAUSEC();
//    scrollScreen();
    print("hejsan",WHITE);

       print("\n\nDONE. Press any key/button to exit",WHITE);
       do
       {

              GetInput();
       }
             while(globals->BUTTON == 0);
}