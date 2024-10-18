
#include "globalvars.h"

__section(".text") void Print(__reg("a0") int *a, __reg("d1") int b);



__section(".text") uint32_t test_function(__reg("a6") struct GlobalVars* globals)
{
	// fill out current vhpos
	uint32_t* vhposr = (uint32_t*)0xdff004;
	globals->current_vhpos = *vhposr;
	// return some hexspeak (this is verified at the callsite)
//	test_function_in_asm(3, 4);
	Print("Test med mer tecken",1);
	return 0x600dc0de;
}