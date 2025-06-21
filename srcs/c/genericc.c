#include "generic.h"
#include <stdbool.h>
#include <hardware/cia.h>

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

void Log(char *string,int value)
{
    #if DEBUG>=1
    SendSerial("\n\x0d");
    SendSerial(string);
    SendSerial("Value: ");
    SendSerial(binhex(value));
    SendSerial(" ");
    SendSerial(binstring(value));
    SendSerial(" ");
    SendSerial(bindec(value));

    SendSerial("\n\x0d");
    #endif
}

int setBit(int value, int bit)
{
    return(value | (1 << (bit-1)));
}

int clearBit(int value, int bit)
{
    return((value & (~(1 << (bit-1)))));
}

int toggleBit(int value, int bit)
{
    return(value ^ (1 << (bit -1)));
}

void initIRQ3(int code)
{
    Log("IRQ: ",code);
    //*(volatile APTR *) + 0x6c = code;
}