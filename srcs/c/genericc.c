#include "generic.h"
#include <stdbool.h>
#include <hardware/cia.h>

void PAUSEC();

void testPrint()
{
    //struct Custom *custom=(APTR)0xdff000;
    custom->color[0]=custom->vhposr;
    PAUSEC();
    Print("Hejsan",WHITE);
}


static inline bool LMB_is_down()
{
    volatile struct CIA* ciaa = (struct CIA*)0xbfe001;
    return ! (ciaa->ciapra & CIAF_GAMEPORT0); // active low
}

static inline bool LMB_is_up()
{
    return !LMB_is_down();
}

void PAUSEC()
{
    do { custom->color[0]=custom->vhposr; } while(LMB_is_up());
    do { } while(LMB_is_down());
}