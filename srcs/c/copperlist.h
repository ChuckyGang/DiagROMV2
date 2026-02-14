#ifndef COPPERLIST_H
#define COPPERLIST_H

#include <exec/types.h>

struct CopIns
{
       union
       {
              ULONG dat;
              struct CopWait
              {
                     UWORD vwait;
                     UWORD hwait;
              } wait;
              struct CopMove
              {
                     UWORD addr;
                     UWORD data;
              } move;
       };
};

struct CopPair
{
       struct CopIns hi;
       struct CopIns lo;
};

struct RomEcsCopper
{
       struct CopPair       sprpt[8];
       struct CopIns        bplcon[3];
       struct CopIns        ddfstrt;
       struct CopIns        ddfstop;
       struct CopIns        diwstrt;
       struct CopIns        diwstop;
       struct CopIns        bplmod[2];
       struct CopIns        color[32];
       struct CopPair       bplpt[5];
       struct CopIns        copend;
};

struct RomAgaColorBank
{
	struct CopIns        bank_hi;
	struct CopIns        color_hi[32];
	struct CopIns        bank_lo;
	struct CopIns        color_lo[32];
};

struct RomAgaCopper
{
	struct CopPair       sprpt[8];
	struct CopIns        bplcon[4];
	struct CopIns        bplcon4;
	struct CopIns        fmode;
	struct CopIns        ddfstrt;
	struct CopIns        ddfstop;
	struct CopIns        diwstrt;
	struct CopIns        diwstop;
	struct CopIns        bplmod[2];
	struct RomAgaColorBank colorbank[8];
	struct CopIns        bplcon3_reset;
	struct CopPair       bplpt[8];
	struct CopIns        copend;
};

extern struct RomEcsCopper romEcsLowResCopper;
extern struct RomEcsCopper romEcsHiResCopper;
extern struct RomAgaCopper romAgaLowResCopper;
extern struct RomAgaCopper romAgaHiResCopper;

#endif
