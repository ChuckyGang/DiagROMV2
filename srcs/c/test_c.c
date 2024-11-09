
#include "globalvars.h"
#include "generic.h"


//void Print(__reg("a0") int *a, __reg("d1") int b);
void Print(__reg("a0") int *string, __reg("d1") int color);

uint32_t test_function(VARS)
{
	// fill out current vhpos
	uint32_t* vhposr = (uint32_t*)0xdff004;
	globals->current_vhpos = *vhposr;
	// return some hexspeak (this is verified at the callsite)
//	test_function_in_asm(3, 4);
	Print("Test med mer tecken",1);
	return 0x600dc0de;
}